/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#if defined(_WIN32) || defined(__APPLE__)
#include <unordered_set>
#else
#include <tr1/unordered_set>
namespace std {
  using tr1::unordered_set;
};
#endif

#include "base/string_utilities.h"
#include "base/util_functions.h"
#include "base/log.h"

#include "grtpp_util.h"

#include "mysql_parser_module.h"
#include "MySQLLexer.h"
#include "mysql-parser.h"
#include "mysql-syntax-check.h"
#include "mysql-scanner.h"
#include "mysql-recognition-types.h"

#include "objimpl/wrapper/parser_ContextReference_impl.h"
#include "grtdb/db_object_helpers.h"

using namespace grt;
using namespace parser;

DEFAULT_LOG_DOMAIN("parser")

GRT_MODULE_ENTRY_POINT(MySQLParserServicesImpl);

//--------------------------------------------------------------------------------------------------

parser_ContextReferenceRef MySQLParserServicesImpl::createParserContext(GrtCharacterSetsRef charsets,
                                                                        GrtVersionRef version,
                                                                        const std::string &sql_mode,
                                                                        int case_sensitive) {
  parser::MySQLParserContext::Ref context =
    MySQLParserServices::createParserContext(charsets, version, case_sensitive != 0);
  context->use_sql_mode(sql_mode);
  return parser_context_to_grt(context);
}

//--------------------------------------------------------------------------------------------------

/**
* Signals any ongoing process to stop. This must be called from a different thread than from where
* the processing was started to make it work.
* XXX: this is not reliable, change to CAS (compare-and-swap).
*/
size_t MySQLParserServicesImpl::stopProcessing() {
  _stop = true;
  return 0;
}

//--------------------------------------------------------------------------------------------------

/**
*	Helper to bring the index type string into a commonly used form.
*/
std::string formatIndexType(std::string indexType) {
  indexType = indexType.substr(0, indexType.find(' ')); // Only first word is meaningful.
  indexType = base::toupper(indexType);
  if (indexType == "KEY")
    indexType = "INDEX";
  return indexType;
}

//--------------------------------------------------------------------------------------------------

/**
* If the current token is a definer clause collect the details and return it as string.
*/
static std::string getDefiner(MySQLRecognizerTreeWalker &walker) {
  std::string definer;
  if (walker.is(DEFINER_SYMBOL)) {
    walker.next(2); // Skip DEFINER + equal sign.
    if (walker.is(CURRENT_USER_SYMBOL)) {
      definer = walker.tokenText(true);
      walker.next();
      walker.skipIf(OPEN_PAR_SYMBOL, 2);
    } else {
      // A user@host entry.
      definer = walker.tokenText(true);
      walker.next();
      switch (walker.tokenType()) {
        case AT_TEXT_SUFFIX:
          definer += walker.tokenText();
          walker.next();
          break;
        case AT_SIGN_SYMBOL:
          walker.next(); // Skip @.
          definer += '@' + walker.tokenText(true);
          walker.next();
      }
    }
  }

  return definer;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the identifier text for a dot_identifier sequence (see this rule in the grammar for details).
 * The caller is responsible for checking the validity of that token on enter.
 * Advances the walker to the first position after the identifier.
 */
static std::string getDotIdentifier(MySQLRecognizerTreeWalker &walker) {
  std::string result;
  if (walker.is(DOT_SYMBOL)) {
    walker.next();
    result = walker.tokenText();
  } else
    result = walker.tokenText().substr(1);
  walker.next();

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
* Read an object identifier "(id?.)?id" from the current walker position which must be
* one of the *_REF_TOKEN or *_NAME_TOKENS types.
* This functions tries get as much info as possible even if the syntax is not fully correct.
*
* On return the walker points to the first token after the id.
*/
static Identifier getIdentifier(MySQLRecognizerTreeWalker &walker) {
  Identifier result;
  switch (walker.tokenType()) {
    // Single ids only.
    case SERVER_REF_TOKEN:
    case SERVER_NAME_TOKEN:
    case SCHEMA_REF_TOKEN:
    case SCHEMA_NAME_TOKEN:
    case LOGFILE_GROUP_NAME_TOKEN:
    case LOGFILE_GROUP_REF_TOKEN:
    case TABLESPACE_NAME_TOKEN:
    case TABLESPACE_REF_TOKEN:
    case UDF_NAME_TOKEN:
    case INDEX_NAME_TOKEN:
      walker.next();
      if (walker.isIdentifier()) {
        result.second = walker.tokenText();
        walker.next();
      }
      break;
    case ENGINE_REF_TOKEN:
      walker.next();
      result.second = walker.tokenText(); // text or identifier
      walker.next();
      break;
    case IDENTIFIER: // For indices, PKs.
    case BACK_TICK_QUOTED_ID:
      result.second = walker.tokenText(); // text or identifier
      walker.next();
      break;

    // Qualified identifiers.
    case TABLE_NAME_TOKEN:     // schema.table
    case TABLE_REF_TOKEN:      // ditto
    case VIEW_NAME_TOKEN:      // schema.view
    case VIEW_REF_TOKEN:       // ditto
    case TRIGGER_NAME_TOKEN:   // schema.trigger
    case TRIGGER_REF_TOKEN:    // table.trigger
    case PROCEDURE_NAME_TOKEN: // schema.procedure
    case PROCEDURE_REF_TOKEN:  // ditto
    case FUNCTION_NAME_TOKEN:  // schema.function
    case FUNCTION_REF_TOKEN:   // ditto
    case EVENT_NAME_TOKEN:     // schema.event
    case EVENT_REF_TOKEN:      // ditto
      walker.next();
      if (walker.is(DOT_SYMBOL) || walker.is(DOT_IDENTIFIER)) {
        // Starting with a dot (ODBC style).
        result.second = getDotIdentifier(walker);
      } else if (walker.isIdentifier()) {
        result.second = walker.tokenText();
        walker.next();
        if (walker.is(DOT_SYMBOL) || walker.is(DOT_IDENTIFIER)) {
          result.first = result.second;
          result.second = getDotIdentifier(walker);
        }
      }
      break;
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
* Same as getIdentifer but for column references (id, .id, id.id, id.id.id) or column names (id).
*
* On return the walker points to the first token after the id.
*/
static ColumnIdentifier getColumnIdentifier(MySQLRecognizerTreeWalker &walker) {
  ColumnIdentifier result;
  if (walker.is(DOT_SYMBOL) || walker.is(DOT_IDENTIFIER))
    result.column = getDotIdentifier(walker);
  else {
    // id
    result.column = walker.tokenText();
    walker.next();
    if (walker.is(DOT_SYMBOL) || walker.is(DOT_IDENTIFIER)) {
      // id.id
      result.table = result.column;
      result.column = getDotIdentifier(walker);

      if (walker.is(DOT_SYMBOL) || walker.is(DOT_IDENTIFIER)) {
        // id.id.id
        result.schema = result.table;
        result.table = result.column;
        result.column = getDotIdentifier(walker);
      }
    }
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
* Collects a list of comma separated values (table/columns lists, enums etc.) enclosed by parentheses.
* The comma separated list is returned and the walker points to the first token after the closing
* parenthesis when done.
*/
static std::string getValueList(MySQLRecognizerTreeWalker &walker, bool keepQuotes = false) {
  std::string result;
  if (walker.is(OPEN_PAR_SYMBOL)) {
    while (true) {
      walker.next();
      if (!result.empty())
        result += ", ";
      result += walker.tokenText(keepQuotes);
      walker.next();
      if (walker.tokenType() != COMMA_SYMBOL)
        break;
    }
    walker.next();
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
* Collects a list of names that are enclosed by parentheses into individual entries.
* On return the walker points to the first token after the closing
* parenthesis when done.
*/
static std::vector<std::string> getNamesList(MySQLRecognizerTreeWalker &walker) {
  std::vector<std::string> result;
  if (walker.is(OPEN_PAR_SYMBOL)) {
    while (true) {
      walker.next();
      result.push_back(walker.tokenText());
      walker.next();
      if (walker.tokenType() != COMMA_SYMBOL)
        break;
    }
    walker.next();
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
*	Returns the text for an expression (optionally within parentheses).
*/
static std::string getExpression(MySQLRecognizerTreeWalker &walker) {
  bool skipParens = walker.is(OPEN_PAR_SYMBOL);

  if (skipParens)
    walker.next();
  std::string text = walker.textForTree();
  walker.skipSubtree();

  if (skipParens)
    walker.next(); // Skip CLOSE_PAR.

  return text;
}

//--------------------------------------------------------------------------------------------------

/**
*	Extracts the charset name from a "charset charset_name" rule sequence).
*/
static std::string getCharsetName(MySQLRecognizerTreeWalker &walker) {
  if (!walker.is(CHAR_SYMBOL) && !walker.is(CHARSET_SYMBOL))
    return "";

  walker.next();
  walker.skipIf(SET_SYMBOL); // From CHAR SET.
  walker.skipIf(EQUAL_OPERATOR);

  if (walker.is(BINARY_SYMBOL)) {
    walker.next();
    return "BINARY";
  }

  std::string name = walker.tokenText(); // string or identifier
  walker.next();
  return name;
}

//--------------------------------------------------------------------------------------------------

/**
*	Compares the given typename with what is in the type list, including the synonyms and returns
*	the type whose name or synonym matches.
*/
static db_SimpleDatatypeRef findType(grt::ListRef<db_SimpleDatatype> types, const std::string &name) {
  for (size_t c = types.count(), i = 0; i < c; ++i) {
    if (base::same_string(types[i]->name(), name, false))
      return types[i];

    // Type has not the default name, but maybe one of the synonyms.
    for (grt::StringListRef::const_iterator synonym = types[i]->synonyms().begin();
         synonym != types[i]->synonyms().end(); ++synonym) {
      if (base::same_string(*synonym, name, false))
        return types[i];
    }
  }

  return db_SimpleDatatypeRef();
}

//--------------------------------------------------------------------------------------------------

/**
* The next 2 functions take a charset or collation and retrieve the associated charset/collation pair.
*/
static std::pair<std::string, std::string> detailsForCharset(const std::string &charset, const std::string &collation,
                                                             const std::string &defaultCharset) {
  std::pair<std::string, std::string> result;
  if (!charset.empty()) {
    result.first = base::tolower(charset);
    if (result.first == "default")
      result.first = base::tolower(defaultCharset);

    if (!collation.empty()) {
      result.second = base::tolower(collation);

      // Clear collation if it's default collation or it belongs to another character set.
      if ((result.second == defaultCollationForCharset(result.first)) ||
          (result.first != charsetForCollation(result.second)))
        result.second = "";
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

static std::pair<std::string, std::string> detailsForCollation(const std::string &collation,
                                                               const std::string &defaultCollation) {
  std::pair<std::string, std::string> result;
  if (!collation.empty()) {
    result.second = base::tolower(collation);
    if (result.second == "default")
      result.second = base::tolower(defaultCollation);

    // Clear collation if it's default collation.
    result.first = charsetForCollation(result.second);
    if (defaultCollationForCharset(result.first) == result.second)
      result.second = "";
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

// In order to be able to resolve column references in indices and foreign keys we must store
// column names along with additional info and do the resolution after everything has been parsed.
// This is necessary because index/FK definitions and column definitions can appear in random order
// and also referenced tables can appear after an FK definition.
// Similarly, when resolving table names (qualified or not) we can do that also only once all parsing
// is done.
struct DbObjectReferences {
  typedef enum { Index, Referencing, Referenced, TableRef } ReferenceType;

  ReferenceType type;

  // Only one of these is valid (or none), depending on type.
  db_ForeignKeyRef foreignKey;
  db_IndexRef index;

  // These 2 are only used for target tables.
  Identifier targetIdentifier;

  // The list of column names for resolution. For indices the column names are stored in the
  // index column entries that make up an index.
  std::vector<std::string> columnNames;

  db_mysql_TableRef table; // The referencing table.
  DbObjectReferences(db_ForeignKeyRef fk, ReferenceType type_) {
    foreignKey = fk;
    type = type_;
  }

  DbObjectReferences(db_IndexRef index_) {
    index = index_;
    type = Index;
  }

  // For pure table references.
  DbObjectReferences(Identifier identifier) {
    targetIdentifier = identifier;
    type = TableRef;
  }
};

typedef std::vector<DbObjectReferences> DbObjectsRefsCache;

//--------------------------------------------------------------------------------------------------

/**
*	Returns the schema with the given name. If it doesn't exist it will be created.
*/
static db_mysql_SchemaRef ensureSchemaExists(db_CatalogRef catalog, const std::string &name, bool caseSensitive) {
  db_SchemaRef result = find_named_object_in_list(catalog->schemata(), name, caseSensitive);
  if (!result.is_valid()) {
    result = db_mysql_SchemaRef(grt::Initialized);
    result->createDate(base::fmttime(0, DATETIME_FMT));
    result->lastChangeDate(result->createDate());
    result->owner(catalog);
    result->name(name);
    result->oldName(name);
    std::pair<std::string, std::string> info = detailsForCharset(
      catalog->defaultCharacterSetName(), catalog->defaultCollationName(), catalog->defaultCharacterSetName());
    result->defaultCharacterSetName(info.first);
    result->defaultCollationName(info.second);
    catalog->schemata().insert(result);
  }
  return db_mysql_SchemaRef::cast_from(result);
}

//--------------------------------------------------------------------------------------------------

static void fillTableCreateOptions(MySQLRecognizerTreeWalker &walker, db_CatalogRef catalog, db_mysql_SchemaRef schema,
                                   db_mysql_TableRef table, bool caseSensitive) {
  std::string schemaName = schema.is_valid() ? schema->name() : "";
  std::string defaultCharset = schema.is_valid() ? schema->defaultCharacterSetName() : "";
  std::string defaultCollation = schema.is_valid() ? schema->defaultCollationName() : "";
  if (defaultCollation.empty() && !defaultCharset.empty())
    defaultCollation = defaultCollationForCharset(defaultCharset);

  while (true) // create_table_options
  {
    switch (walker.tokenType()) {
      case ENGINE_SYMBOL:
      case TYPE_SYMBOL: {
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        Identifier identifier = getIdentifier(walker);
        table->tableEngine(identifier.second);
        break;
      }

      case MAX_ROWS_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->maxRows(walker.tokenText());
        walker.next();
        break;

      case MIN_ROWS_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->minRows(walker.tokenText());
        walker.next();
        break;

      case AVG_ROW_LENGTH_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->avgRowLength(walker.tokenText());
        walker.next();
        break;

      case PASSWORD_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->password(walker.tokenText());
        walker.skipSubtree(); // Skip string subtree. Can be PARAM_MARKER too, but skipSubtree() still works for that.
        break;

      case COMMENT_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->comment(walker.tokenText());
        walker.skipSubtree(); // Skip over string sub tree.
        break;

      case AUTO_INCREMENT_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->nextAutoInc(walker.tokenText());
        walker.next();
        break;

      case PACK_KEYS_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->packKeys(walker.tokenText());
        walker.next();
        break;

      case STATS_AUTO_RECALC_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->statsAutoRecalc(walker.tokenText());
        walker.next();
        break;

      case STATS_PERSISTENT_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->statsPersistent(walker.tokenText());
        walker.next();
        break;

      case STATS_SAMPLE_PAGES_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->statsSamplePages(base::atoi<size_t>(walker.tokenText()));
        walker.next();
        break;

      case CHECKSUM_SYMBOL:
      case TABLE_CHECKSUM_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->checksum(base::atoi<size_t>(walker.tokenText()));
        walker.next();
        break;

      case DELAY_KEY_WRITE_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->delayKeyWrite(base::atoi<size_t>(walker.tokenText()));
        walker.next();
        break;

      case ROW_FORMAT_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->rowFormat(walker.tokenText());
        walker.next();
        break;

      case UNION_SYMBOL: // Only for merge engine.
      {
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        walker.next(); // Skip OPEN_PAR.
        std::string value;
        while (true) {
          if (!value.empty())
            value += ", ";
          Identifier identifier = getIdentifier(walker);

          if (identifier.first.empty())
            // In order to avoid diff problems explicitly qualify unqualified tables
            // with the current schema name.
            value += schemaName + '.' + identifier.second;
          else {
            ensureSchemaExists(catalog, identifier.first, caseSensitive);
            value += identifier.first + "." + identifier.second;
          }

          if (walker.tokenType() != COMMA_SYMBOL)
            break;
          walker.next();
        }
        walker.next();
        table->mergeUnion(value);
        break;
      }

      case DEFAULT_SYMBOL:
      case COLLATE_SYMBOL:
      case CHAR_SYMBOL:
      case CHARSET_SYMBOL:
        walker.skipIf(DEFAULT_SYMBOL);
        if (walker.is(COLLATE_SYMBOL)) {
          walker.next();
          walker.skipIf(EQUAL_OPERATOR);

          std::pair<std::string, std::string> info = detailsForCollation(walker.tokenText(), defaultCollation);
          table->defaultCharacterSetName(info.first);
          table->defaultCollationName(info.second);
        } else {
          walker.next();
          walker.skipIf(SET_SYMBOL); // From CHAR SET.
          walker.skipIf(EQUAL_OPERATOR);

          std::pair<std::string, std::string> info =
            detailsForCharset(walker.tokenText(), defaultCollation, defaultCharset);
          table->defaultCharacterSetName(info.first);
          table->defaultCollationName(info.second); // Collation name or DEFAULT.
        }
        walker.next();
        break;

      case INSERT_METHOD_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->mergeInsert(walker.tokenText());
        walker.next();
        break;

      case DATA_SYMBOL:
        walker.next(2); // Skip DATA DIRECTORY.
        walker.skipIf(EQUAL_OPERATOR);
        table->tableDataDir(walker.tokenText());
        walker.skipSubtree(); // Skip string subtree.
        break;

      case INDEX_SYMBOL:
        walker.next(2); // Skip INDEX DIRECTORY.
        walker.skipIf(EQUAL_OPERATOR);
        table->tableIndexDir(walker.tokenText());
        walker.skipSubtree(); // Skip string subtree.
        break;

      case TABLESPACE_SYMBOL:
        walker.next();
        table->tableSpace(walker.tokenText());
        walker.next();
        break;

      case STORAGE_SYMBOL:
        //(DISK_SYMBOL | MEMORY_SYMBOL) ignored for now, as not documented.
        walker.next(2);
        break;

      case CONNECTION_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->connectionString(walker.tokenText());
        walker.skipSubtree(); // Skip string sub tree.
        break;

      case KEY_BLOCK_SIZE_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        table->keyBlockSize(walker.tokenText());
        walker.next();
        break;

      case COMMA_SYMBOL:
        walker.next();
        return;

      default:
        return;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a normalized list of comma separated values within parentheses.
 */
static std::string getPartitionValueList(MySQLRecognizerTreeWalker &walker) {
  std::string value;
  walker.next();
  while (true) {
    if (!value.empty())
      value += ", ";
    if (walker.is(MAXVALUE_SYMBOL))
      value += "MAXVALUE";
    else
      value += getExpression(walker);
    if (!walker.is(COMMA_SYMBOL))
      break;
    walker.next();
  }
  walker.next();
  return value;
}

//--------------------------------------------------------------------------------------------------

/**
*	Parses main and sub partitions.
*/
static void getPartitionDefinition(MySQLRecognizerTreeWalker &walker, db_mysql_PartitionDefinitionRef definition) {
  walker.next(); // Skip PARTITION or SUBPARTITION.
  definition->name(walker.tokenText());
  walker.next();
  if (walker.is(VALUES_SYMBOL)) // Appears only for main partitions.
  {
    walker.next();
    if (walker.is(LESS_SYMBOL)) {
      walker.next(2); // Skip LESS THAN.
      if (walker.is(MAXVALUE_SYMBOL))
        definition->value("MAXVALUE");
      else
        definition->value(getPartitionValueList(walker));
    } else {
      // Otherwise IN.
      walker.next();
      definition->value(getPartitionValueList(walker));
    }
  }

  bool done = false;
  while (!done) { // Zero or more.
    switch (walker.tokenType()) {
      case TABLESPACE_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        definition->tableSpace(walker.tokenText());
        walker.next();
        break;
      case STORAGE_SYMBOL:
      case ENGINE_SYMBOL:
        walker.next(walker.is(STORAGE_SYMBOL) ? 2 : 1);
        walker.skipIf(EQUAL_OPERATOR);
        walker.next(); // Skip ENGINE_REF_TOKEN.
        definition->engine(walker.tokenText());
        walker.next();
        break;
      case NODEGROUP_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        definition->nodeGroupId(base::atoi<size_t>(walker.tokenText()));
        walker.next();
        break;
      case MAX_ROWS_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        definition->maxRows(walker.tokenText());
        walker.next();
        break;
      case MIN_ROWS_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        definition->minRows(walker.tokenText());
        walker.next();
        break;
      case DATA_SYMBOL:
        walker.next(2);
        walker.skipIf(EQUAL_OPERATOR);
        definition->dataDirectory(walker.tokenText());
        walker.next();
        break;
      case INDEX_SYMBOL:
        walker.next(2);
        walker.skipIf(EQUAL_OPERATOR);
        definition->indexDirectory(walker.tokenText());
        walker.next();
        break;
      case COMMENT_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        definition->comment(walker.tokenText());
        walker.next();
        break;
      default:
        done = true;
        break;
    }
  }

  // Finally the sub partition definitions (optional).
  // Only appearing for main partitions (i.e. there is no unlimited nesting).
  definition->subpartitionDefinitions().remove_all();
  if (walker.is(OPEN_PAR_SYMBOL)) {
    walker.next();
    while (true) {
      db_mysql_PartitionDefinitionRef subdefinition(grt::Initialized);
      getPartitionDefinition(walker, subdefinition);
      definition->subpartitionDefinitions().insert(subdefinition);
      if (!walker.is(COMMA_SYMBOL))
        break;
      walker.next();
    }

    walker.next();
  }
}

//--------------------------------------------------------------------------------------------------

static void fillTablePartitioning(MySQLRecognizerTreeWalker &walker, db_mysql_TableRef &table) {
  if (!walker.is(PARTITION_SYMBOL))
    return;

  walker.next(2); // Skip PARTITION BY.
  bool linear = walker.skipIf(LINEAR_SYMBOL);
  unsigned type = walker.tokenType();
  table->partitionType((linear ? "LINEAR " : "") + base::toupper(walker.tokenText())); // HASH, KEY, RANGE, LIST.
  walker.next();
  switch (type) {
    case HASH_SYMBOL:
      table->partitionExpression(getExpression(walker));
      break;
    case KEY_SYMBOL:
      if (walker.is(ALGORITHM_SYMBOL)) {
        walker.next(2); // Skip ALGORITHM EQUAL.
        table->partitionKeyAlgorithm(base::atoi<size_t>(walker.tokenText()));
        walker.next();
      }
      table->partitionExpression(getValueList(walker));
      break;
    case RANGE_SYMBOL:
    case LIST_SYMBOL:
      if (walker.is(OPEN_PAR_SYMBOL))
        table->partitionExpression(getExpression(walker));
      else {
        walker.next(); // Skip COLUMNS.
        table->partitionExpression(getValueList(walker));
      }
      break;
  }

  if (walker.is(PARTITIONS_SYMBOL)) {
    walker.next();
    table->partitionCount(base::atoi<size_t>(walker.tokenText()));
    walker.next();
  }

  if (walker.is(SUBPARTITION_SYMBOL)) {
    walker.next(2); // Skip SUBPARTITION BY.
    linear = walker.skipIf(LINEAR_SYMBOL);
    table->subpartitionType((linear ? "LINEAR " : "") + base::toupper(walker.tokenText()));
    if (walker.is(HASH_SYMBOL)) {
      walker.next();
      table->subpartitionExpression(getExpression(walker));
    } else {
      // Otherwise KEY type.
      if (walker.is(ALGORITHM_SYMBOL)) {
        walker.next(2); // Skip ALGORTIHM EQUAL.
        table->subpartitionKeyAlgorithm(base::atoi<size_t>(walker.tokenText()));
        walker.next();
      }
      table->subpartitionExpression(getValueList(walker));
    }

    if (walker.is(SUBPARTITIONS_SYMBOL)) {
      walker.next();
      table->subpartitionCount(base::atoi<size_t>(walker.tokenText()));
      walker.next();
    }
  }

  // Finally the partition definitions.
  table->partitionDefinitions().remove_all();
  if (walker.is(OPEN_PAR_SYMBOL)) {
    walker.next();
    while (true) {
      db_mysql_PartitionDefinitionRef definition(grt::Initialized);
      definition->owner(table);
      getPartitionDefinition(walker, definition);
      table->partitionDefinitions().insert(definition);
      if (!walker.is(COMMA_SYMBOL))
        break;
      walker.next();
    }

    walker.next();
  }

  // If no partition count was given use the number of definitions we found.
  if (table->partitionCount() == 0)
    table->partitionCount(table->partitionDefinitions().count());

  // Similar for sub partitions. Code taken from old parser, but it looks strange.
  // Must all partitions have the same number of sub partitions?
  if (table->partitionDefinitions().count() > 0)
    table->subpartitionCount(table->partitionDefinitions()[0]->subpartitionDefinitions().count());
}

//--------------------------------------------------------------------------------------------------

static void fillIndexColumns(MySQLRecognizerTreeWalker &walker, db_TableRef &table, db_mysql_IndexRef index) {
  index->columns().remove_all();

  walker.next(); // Opening parenthesis.
  while (true) {
    db_mysql_IndexColumnRef column(grt::Initialized);
    column->owner(index);
    index->columns().insert(column);

    std::string referenceName = walker.tokenText();
    walker.next();
    if (walker.is(OPEN_PAR_SYMBOL)) // Field length.
    {
      walker.next();
      column->columnLength(base::atoi<size_t>(walker.tokenText()));
      walker.next(2);
    }

    if (walker.is(ASC_SYMBOL) || walker.is(DESC_SYMBOL)) {
      column->descend(walker.is(DESC_SYMBOL));
      walker.next();
    }

    if (table.is_valid()) {
      db_ColumnRef referencedColumn = find_named_object_in_list(table->columns(), referenceName, false);
      if (referencedColumn.is_valid())
        column->referencedColumn(referencedColumn);
    }

    if (walker.tokenType() != COMMA_SYMBOL)
      break;
  }
  walker.next(); // Closing parenthesis.
}

//--------------------------------------------------------------------------------------------------

static void fillIndexOptions(MySQLRecognizerTreeWalker &walker, db_mysql_IndexRef index) {
  while (true) {
    // Unlimited occurrences.
    switch (walker.tokenType()) {
      case USING_SYMBOL:
      case TYPE_SYMBOL:
        walker.next();
        index->indexKind(walker.tokenText());
        walker.next();
        break;

      case KEY_BLOCK_SIZE_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        index->keyBlockSize(base::atoi<size_t>(walker.tokenText()));
        walker.next();
        break;

      case COMMENT_SYMBOL:
        walker.next();
        index->comment(walker.tokenText());
        walker.next();
        break;

      case WITH_SYMBOL: // WITH PARSER
        walker.next(2);
        index->withParser(walker.tokenText());
        walker.next();
        break;

      default:
        return;
    }
  }
}

//--------------------------------------------------------------------------------------------------

static void fillDataTypeAndAttributes(MySQLRecognizerTreeWalker &walker, db_CatalogRef catalog, db_mysql_TableRef table,
                                      db_mysql_ColumnRef column) {
  db_SimpleDatatypeRef simpleType;
  ssize_t precision = -1;
  ssize_t scale = -1;
  ssize_t length = -1;
  std::string explicitParams;

  bool explicitDefaultValue = false;
  bool explicitNullValue = false;

  column->defaultValue("");
  column->autoIncrement(0);

  StringListRef flags = column->flags();
  flags.remove_all();

  // A type name can consist of up to 3 parts (e.g. "national char varying").
  std::string type_name = walker.tokenText();

  switch (walker.tokenType()) {
    case DOUBLE_SYMBOL:
      walker.next();
      if (walker.is(PRECISION_SYMBOL))
        walker.next(); // Simply ignore syntactic sugar.
      break;

    case NATIONAL_SYMBOL:
      walker.next();
      type_name += " " + walker.tokenText();
      walker.next();
      if (walker.is(VARYING_SYMBOL)) {
        type_name += " " + walker.tokenText();
        walker.next();
      }
      break;

    case NCHAR_SYMBOL:
      walker.next();
      if (walker.is(VARCHAR_SYMBOL) || walker.is(VARYING_SYMBOL)) {
        type_name += " " + walker.tokenText();
        walker.next();
      }
      break;

    case CHAR_SYMBOL:
      walker.next();
      if (walker.is(VARYING_SYMBOL)) {
        type_name += " " + walker.tokenText();
        walker.next();
      }
      break;

    case LONG_SYMBOL:
      walker.next();
      switch (walker.tokenType()) {
        case CHAR_SYMBOL:                               // LONG CHAR VARYING
          if (walker.lookAhead(true) == VARYING_SYMBOL) // Otherwise we may get e.g. LONG CHAR SET...
          {
            type_name += " " + walker.tokenText();
            walker.next();
            type_name += " " + walker.tokenText();
            walker.next();
          }
          break;

        case VARBINARY_SYMBOL:
        case VARCHAR_SYMBOL:
          type_name += " " + walker.tokenText();
          walker.next();
      }
      break;

    default:
      walker.next();
  }

  simpleType = findType(catalog->simpleDatatypes(), type_name);
  if (simpleType.is_valid()) // Should always be valid at this point.
  {
    if (walker.is(OPEN_PAR_SYMBOL)) {
      // We use the simple type properties for char length to learn if we have a length here or a precision.
      // We could indicate that in the grammar instead, however the handling in WB is a bit different
      // than what the server grammar would suggest (e.g. the length is also used for integer types, in the grammar).
      if (simpleType->characterMaximumLength() != bec::EMPTY_TYPE_MAXIMUM_LENGTH ||
          simpleType->characterOctetLength() != bec::EMPTY_TYPE_OCTET_LENGTH) {
        walker.next(); // Skip OPEN_PAR.
        if (walker.is(INT_NUMBER)) {
          length = base::atoi<size_t>(walker.tokenText());
          walker.next();
        }
        walker.next(); // Skip CLOSE_PAR.
      } else {
        if (simpleType->name() == "SET" || simpleType->name() == "ENUM")
          explicitParams = "(" + getValueList(walker, true) + ")";
        else {
          // Finally all cases with either precision, scale or both.
          walker.next(); // Skip OPEN_PAR.
          if (simpleType->numericPrecision() != bec::EMPTY_TYPE_PRECISION)
            precision = base::atoi<size_t>(walker.tokenText());
          else
            length = base::atoi<size_t>(walker.tokenText());
          walker.next();
          if (walker.is(COMMA_SYMBOL)) {
            walker.next();
            scale = base::atoi<size_t>(walker.tokenText());
            walker.next();
          }
          walker.next(); // Skip CLOSE_PAR.
        }
      }
    }

    // Collect additional flags + charset.
    bool done = false;
    while (!done) {
      switch (walker.tokenType()) {
        // case BYTE_SYMBOL: seems not be used anywhere.
        case SIGNED_SYMBOL:
        case UNSIGNED_SYMBOL:
        case ZEROFILL_SYMBOL: {
          std::string value = base::toupper(walker.tokenText());
          walker.next();
          if (flags.get_index(value) == BaseListRef::npos)
            flags.insert(value);
          break;
        }

        case ASCII_SYMBOL:
        case UNICODE_SYMBOL: {
          std::string value = base::toupper(walker.tokenText());
          walker.next();
          if (flags.get_index(value) == BaseListRef::npos)
            flags.insert(value);

          // Could be followed by BINARY.
          if (walker.skipIf(BINARY_SYMBOL)) {
            if (flags.get_index("BINARY") == BaseListRef::npos)
              flags.insert("BINARY");
          }
          break;
        }

        case BINARY_SYMBOL: {
          walker.next();
          if (flags.get_index("BINARY") == BaseListRef::npos)
            flags.insert("BINARY");

          switch (walker.tokenType()) {
            case ASCII_SYMBOL:
            case UNICODE_SYMBOL: {
              std::string value = base::toupper(walker.tokenText());
              walker.next();
              if (flags.get_index(value) == BaseListRef::npos)
                flags.insert(value);

              break;
            }
            case CHAR_SYMBOL:
            case CHARSET_SYMBOL:
              std::pair<std::string, std::string> info =
                detailsForCharset(getCharsetName(walker), column->collationName(), table->defaultCharacterSetName());
              column->characterSetName(info.first);
              column->collationName(info.second);
              break;
          }
          break;
        }

        case CHAR_SYMBOL:
        case CHARSET_SYMBOL: // The grammar allows for weird syntax like: "char char set binary binary".
        {
          std::pair<std::string, std::string> info =
            detailsForCharset(getCharsetName(walker), column->collationName(), table->defaultCharacterSetName());
          column->characterSetName(info.first);
          column->collationName(info.second);

          if (walker.is(BINARY_SYMBOL)) {
            if (flags.get_index("BINARY") == BaseListRef::npos)
              flags.insert("BINARY");
          }
          break;
        }

        default:
          done = true;
          break;
      }
    }

    // Column attributes. There should really only be one attribute of each type if at all,
    // but the grammar allows for unlimited repetitions, so we have to handle that properly.
    done = false;
    while (!done) {
      switch (walker.tokenType()) {
        case NOT_SYMBOL:
        case NULL_SYMBOL:
        case NULL2_SYMBOL: {
          column->isNotNull(walker.skipIf(NOT_SYMBOL));
          walker.next(); // Skip NULL/NULL2.

          explicitNullValue = true;

          break;
        }
        case DEFAULT_SYMBOL: {
          // Default values.
          // Note: for DEFAULT NOW (and synonyms) there can be an additional ON UPDATE NOW (and synonyms).
          //       We store both parts together in the defaultValue(). Keep in mind however that
          //       attributes can be in any order and appear multiple times.
          //       In order to avoid trouble we convert all NOW synonyms to CURRENT_TIMESTAMP.
          std::string existingDefault = column->defaultValue();

          // Remove any previous default value. This will also remove ON UPDATE if it was there plus
          // any another default value. It doesn't handle time precision either.
          // We can either have that or concatenate all default values (which is really wrong).
          // TODO: revise the decision to put both into the default value.
          if (existingDefault != "ON UPDATE CURRENT_TIMESTAMP")
            existingDefault = "";
          walker.next();
          if (walker.is(NOW_SYMBOL)) {
            // As written above, convert all synonyms. This can cause trouble with additional
            // precision, which we may have later to handle.
            std::string newDefault = "CURRENT_TIMESTAMP";
            walker.next();
            if (walker.is(OPEN_PAR_SYMBOL)) // Additional precision.
            {
              newDefault += "(";
              walker.next();
              if (walker.is(INT_NUMBER)) // Optional precision.
              {
                newDefault += walker.tokenText();
                walker.next();
              }
              newDefault += ")";
              walker.next();
            }
            if (!existingDefault.empty())
              newDefault += " " + existingDefault;
            column->defaultValue(newDefault);
          } else {
            // signed_literal
            std::string newDefault;
            if (walker.is(MINUS_OPERATOR) || walker.is(PLUS_OPERATOR)) {
              if (walker.is(MINUS_OPERATOR))
                newDefault = "-";
              walker.next();
              newDefault += walker.tokenText(); // The actual value.
            } else {
              // Any literal (string, number, bool , null, temporal, bit, hex).
              // We need to keep quotes around strings in order to distinguish between no default
              // and an empty string default.
              newDefault = walker.tokenText(true);

              // Temporal values have second part (the actual value).
              if (walker.is(DATE_SYMBOL) || walker.is(TIME_SYMBOL) || walker.is(TIMESTAMP_SYMBOL)) {
                walker.next();
                newDefault += " " + walker.tokenText(true);
              }
            }

            walker.skipSubtree();
            column->defaultValue(newDefault);

            if (base::same_string(newDefault, "NULL", false))
              column->defaultValueIsNull(true);
          }

          explicitDefaultValue = true;
          break;
        }

        case ON_SYMBOL: {
          // As mentioned above we combine DEFAULT NOW and ON UPDATE NOW into a common default value.
          std::string newDefault = column->defaultValue();
          if (base::hasPrefix(newDefault, "CURRENT_TIMESTAMP"))
            newDefault += " ON UPDATE CURRENT_TIMESTAMP";
          else
            newDefault = "ON UPDATE CURRENT_TIMESTAMP";
          walker.next(3);
          if (walker.is(OPEN_PAR_SYMBOL)) {
            newDefault += "(";
            walker.next();
            if (walker.is(IDENTIFIER)) // Optional precision.
            {
              newDefault += walker.tokenText();
              walker.next();
            }
            newDefault += ")";
            walker.next();
          }
          column->defaultValue(newDefault);
          explicitDefaultValue = true;

          break;
        }

        case AUTO_INCREMENT_SYMBOL:
          walker.next();
          column->autoIncrement(1);
          break;

        case SERIAL_SYMBOL: // SERIAL DEFAULT VALUE is an alias for NOT NULL AUTO_INCREMENT UNIQUE.
        case UNIQUE_SYMBOL: {
          if (walker.is(UNIQUE_SYMBOL)) {
            walker.next();
            walker.skipIf(KEY_SYMBOL);
          } else {
            walker.next(3);
            column->isNotNull(1);
            column->autoIncrement(1);
          }

          // Add new unique index for that column.
          db_mysql_IndexRef index(grt::Initialized);
          index->owner(table);
          index->unique(1);
          index->indexType("UNIQUE");

          db_mysql_IndexColumnRef index_column(grt::Initialized);
          index_column->owner(index);
          index_column->referencedColumn(column);

          index->columns().insert(index_column);
          table->indices().insert(index);

          break;
        }

        case PRIMARY_SYMBOL:
          walker.next();
        // fall through
        case KEY_SYMBOL: {
          walker.next();
          db_mysql_IndexRef index(grt::Initialized);
          index->owner(table);

          index->isPrimary(1);
          table->primaryKey(index);
          index->indexType("PRIMARY");
          index->name("PRIMARY");
          index->oldName("PRIMARY");

          db_mysql_IndexColumnRef indexColumn(grt::Initialized);
          indexColumn->owner(index);
          indexColumn->referencedColumn(column);

          index->columns().insert(indexColumn);
          table->indices().insert(index);

          break;
        }

        case COMMENT_SYMBOL:
          walker.next();
          column->comment(walker.tokenText());
          walker.skipSubtree();
          break;

        case COLLATE_SYMBOL: {
          walker.next();

          std::pair<std::string, std::string> info =
            detailsForCollation(walker.tokenText(), table->defaultCollationName());
          column->characterSetName(info.first);
          column->collationName(info.second);
          walker.next();
          break;
        }

        case COLUMN_FORMAT_SYMBOL: // Ignored by the server, so we ignore it here too.
          walker.next(2);
          break;

        case STORAGE_SYMBOL: // No info available, might later become important.
          walker.next(2);
          break;

        default:
          done = true;
          break;
      }
    }
  }

  // Generated columns. Handle them after column attributes, as the can be a collation before the actual definition.
  if (walker.is(GENERATED_SYMBOL) || walker.is(AS_SYMBOL)) {
    column->generated(1);

    // GENRATED ALWAYS is optional.
    if (walker.tokenType() == GENERATED_SYMBOL)
      walker.next(2);

    walker.next(2); // Skip AS (.
    column->expression(walker.textForTree());
    walker.skipSubtree();
    walker.next(); // Skip ).

    if (walker.is(VIRTUAL_SYMBOL) || walker.is(STORED_SYMBOL)) // Storage type of the gcol.
    {
      column->generatedStorage(walker.tokenText());
      walker.next();
    }
  }

  column->userType(db_UserDatatypeRef()); // We always have normal data types here.
  column->simpleType(simpleType);
  // structuredType ignored here

  column->precision(precision);
  column->scale(scale);
  column->length(length);
  column->datatypeExplicitParams(explicitParams);

  if (simpleType.is_valid() && base::same_string(simpleType->name(), "TIMESTAMP", false)) {
    if (!explicitNullValue)
      column->isNotNull(1);
  }

  if (!column->isNotNull() && !explicitDefaultValue)
    bec::ColumnHelper::set_default_value(column, "NULL");
}

//--------------------------------------------------------------------------------------------------

static void fillColumnReference(MySQLRecognizerTreeWalker &walker, const std::string &schemaName,
                                DbObjectReferences &references) {
  walker.next(); // Skip REFERENCES_SYMBOL.

  Identifier identifier = getIdentifier(walker);
  references.targetIdentifier = identifier;
  if (identifier.first.empty())
    references.targetIdentifier.first = schemaName;

  if (walker.is(OPEN_PAR_SYMBOL))
    references.columnNames = getNamesList(walker);

  if (walker.is(MATCH_SYMBOL)) // MATCH is ignored by MySQL.
    walker.next(2);

  // Finally ON DELETE/ON UPDATE can be in any order but only once each.
  while (walker.is(ON_SYMBOL)) {
    walker.next();
    bool isDelete = walker.is(DELETE_SYMBOL);
    walker.next();

    std::string ruleText;
    switch (walker.tokenType()) {
      case RESTRICT_SYMBOL:
      case CASCADE_SYMBOL:
        ruleText = walker.tokenText();
        walker.next();
        break;
      case SET_SYMBOL:
      case NO_SYMBOL:
        ruleText = walker.tokenText();
        walker.next();
        ruleText += " " + walker.tokenText();
        walker.next();
        break;
    }

    if (isDelete)
      references.foreignKey->deleteRule(ruleText);
    else
      references.foreignKey->updateRule(ruleText);
  }
}

//--------------------------------------------------------------------------------------------------

/**
*	Similar to the fillIndexDetails function, but used for CREATE TABLE key definitions which
*	are slightly different.
*/
static void fillRefIndexDetails(MySQLRecognizerTreeWalker &walker, std::string &constraintName, db_mysql_IndexRef index,
                                db_mysql_TableRef table, DbObjectsRefsCache &refCache) {
  // Not every key type supports every option, but all varying parts are optional and always
  // in the same order.
  if (walker.is(COLUMN_REF_TOKEN)) {
    walker.next();
    ColumnIdentifier identifier = getColumnIdentifier(walker);
    if (constraintName.empty())
      constraintName = identifier.column;
  }

  // index_type in the grammar.
  if (walker.is(USING_SYMBOL) || walker.is(TYPE_SYMBOL)) {
    walker.next();
    index->indexKind(base::toupper(walker.tokenText())); // BTREE, RTREE, HASH.
    walker.next();
  }

  // index_columns in the grammar (mandatory).
  walker.next(); // Skip OPEN_PAR.
  DbObjectReferences references(index);
  references.table = table;

  while (true) {
    db_mysql_IndexColumnRef indexColumn(grt::Initialized);
    indexColumn->owner(index);
    indexColumn->name(getIdentifier(walker).second);
    references.index->columns().insert(indexColumn);
    if (walker.is(OPEN_PAR_SYMBOL)) {
      // Field length.
      walker.next();
      indexColumn->columnLength(base::atoi<size_t>(walker.tokenText()));
      walker.next(2); // Skip INTEGER and CLOSE_PAR.
    }
    if (walker.is(ASC_SYMBOL) || walker.is(DESC_SYMBOL)) {
      indexColumn->descend(walker.is(DESC_SYMBOL));
      walker.next();
    }

    if (!walker.is(COMMA_SYMBOL))
      break;
    walker.next();
  }

  refCache.push_back(references);
  walker.next(); // Skip CLOSE_PAR.

  // Zero or more index_option.
  bool done = false;
  while (!done) {
    switch (walker.tokenType()) {
      case USING_SYMBOL:
      case TYPE_SYMBOL:
        walker.next();
        index->indexKind(base::toupper(walker.tokenText()));
        walker.next();
        break;

      case KEY_BLOCK_SIZE_SYMBOL:
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);
        index->keyBlockSize(base::atoi<size_t>(walker.tokenText()));
        break;

      case COMMENT_SYMBOL:
        walker.next();
        index->comment(walker.tokenText());
        walker.skipSubtree();
        break;

      default:
        done = true;
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
*	Collects details for a new key/index definition (parses the key_def subrule in the grammar).
*/
static void processTableKeyItem(MySQLRecognizerTreeWalker &walker, db_CatalogRef catalog, const std::string &schemaName,
                                db_mysql_TableRef table, bool autoGenerateFkNames, DbObjectsRefsCache &refCache) {
  db_mysql_IndexRef index(grt::Initialized);
  index->owner(table);

  std::string constraintName;
  if (walker.is(CONSTRAINT_SYMBOL)) {
    walker.next();
    if (walker.is(COLUMN_REF_TOKEN)) {
      walker.next();
      ColumnIdentifier identifier = getColumnIdentifier(walker);
      constraintName = identifier.column;
    }
  }

  bool isForeignKey = false; // Need the new index only for non-FK constraints.
  switch (walker.tokenType()) {
    case PRIMARY_SYMBOL:
      index->isPrimary(1);
      table->primaryKey(index);
      constraintName = "PRIMARY";
      index->indexType("PRIMARY");
      walker.next(2); // Skip PRIMARY KEY.
      break;

    case FOREIGN_SYMBOL: {
      isForeignKey = true;
      walker.next(2); // Skip FOREIGN KEY.

      if (walker.is(COLUMN_REF_TOKEN)) {
        walker.next();
        ColumnIdentifier identifier = getColumnIdentifier(walker);
        constraintName = identifier.column;
      }

      db_mysql_ForeignKeyRef fk(grt::Initialized);
      fk->owner(table);
      fk->name(constraintName);
      fk->oldName(constraintName);

      if (fk->name().empty() && autoGenerateFkNames) {
        std::string name = bec::TableHelper::generate_foreign_key_name();
        fk->name(name);
        fk->oldName(name);
      }

      // index_columns in the grammar (mandatory).
      {
        DbObjectReferences references(fk, DbObjectReferences::Referencing);

        // Columns used in the FK might not have been parsed yet, so add the column refs
        // to our cache as well and resolve them when we are fully done.
        references.targetIdentifier.first = schemaName;
        references.targetIdentifier.second = table->name();
        references.table = table;

        references.columnNames = getNamesList(walker);
        refCache.push_back(references);
      }

      DbObjectReferences references(fk, DbObjectReferences::Referenced);
      references.table = table;
      fillColumnReference(walker, schemaName, references);
      table->foreignKeys().insert(fk);
      refCache.push_back(references);

      break;
    }

    case UNIQUE_SYMBOL:
    case INDEX_SYMBOL:
    case KEY_SYMBOL:
      if (walker.is(UNIQUE_SYMBOL)) {
        index->unique(1);
        index->indexType("UNIQUE");
        walker.next();
      } else
        index->indexType(formatIndexType(walker.tokenText()));

      walker.next();
      break;

    case FULLTEXT_SYMBOL:
    case SPATIAL_SYMBOL:
      index->indexType(formatIndexType(walker.tokenText()));
      walker.next();
      if (walker.is(INDEX_SYMBOL) || walker.is(KEY_SYMBOL))
        walker.next();
      break;
  }

  if (!isForeignKey) {
    fillRefIndexDetails(walker, constraintName, index, table, refCache);
    index->name(constraintName);
    index->oldName(constraintName);
    table->indices().insert(index);
  }
}

//--------------------------------------------------------------------------------------------------

/**
*	Collects details for a column/key/index definition.
*	Column references cannot be resolved here so we use the refCache and do that after all
*	definitions have been read and we can access the columns.
*/
static void fillTableCreateItem(MySQLRecognizerTreeWalker &walker, db_CatalogRef catalog, const std::string &schemaName,
                                db_mysql_TableRef table, bool autoGenerateFkNames, DbObjectsRefsCache &refCache) {
  walker.next();
  switch (walker.tokenType()) {
    case COLUMN_NAME_TOKEN: // A column definition.
    {
      walker.next();
      db_mysql_ColumnRef column(grt::Initialized);
      column->owner(table);

      // Column/key identifiers can be qualified, but they must always point to the table at hand
      // so it's rather useless and we ignore schema and table ids here.
      ColumnIdentifier identifier = getColumnIdentifier(walker);
      column->name(identifier.column);
      column->oldName(column->name());
      walker.next(); // Skip DATA_TYPE_TOKEN.
      fillDataTypeAndAttributes(walker, catalog, table, column);
      table->columns().insert(column);

      if (walker.is(REFERENCES_SYMBOL)) {
        // This is a so called "inline references specification", which is not supported by
        // MySQL. We parse it nonetheless as it may require to create stub tables and
        // the old parser created foreign key entries for these.
        db_mysql_ForeignKeyRef fk(grt::Initialized);
        fk->owner(table);
        fk->columns().insert(column);
        fk->many(true);
        fk->referencedMandatory(column->isNotNull());
        table->foreignKeys().insert(fk);

        DbObjectReferences references(fk, DbObjectReferences::Referenced);
        references.table = table;
        fillColumnReference(walker, schemaName, references);
        refCache.push_back(references);
      } else if (walker.is(CHECK_SYMBOL)) {
        // CHECK (expression). Ignored by the server.
        walker.next(3);       // Skip CHECK OPEN_PAR_SYMBOL EXPRESSION_TOKEN.
        walker.skipSubtree(); // Skip over expression subtree.
        walker.next();        // Skip CLOSE_PAR_SYMBOL.
      }

      break;
    }

    case CONSTRAINT_SYMBOL:
    case PRIMARY_SYMBOL:
    case FOREIGN_SYMBOL:
    case UNIQUE_SYMBOL:
    case INDEX_SYMBOL:
    case KEY_SYMBOL:
    case FULLTEXT_SYMBOL:
    case SPATIAL_SYMBOL:
      processTableKeyItem(walker, catalog, schemaName, table, autoGenerateFkNames, refCache);
      break;

    case CHECK_SYMBOL: {
      // CHECK (expression). Ignored by the server.
      walker.next(3);       // Skip CHECK OPEN_PAR_SYMBOL EXPRESSION_TOKEN.
      walker.skipSubtree(); // Skip over expression subtree.
      walker.next();        // Skip CLOSE_PAR_SYMBOL.
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

static std::pair<std::string, bool> fillTableDetails(MySQLRecognizerTreeWalker &walker, db_mysql_CatalogRef catalog,
                                                     db_mysql_SchemaRef schema, db_mysql_TableRef &table,
                                                     bool caseSensistive, bool autoGenerateFkNames,
                                                     DbObjectsRefsCache &refCache) {
  std::pair<std::string, bool> result("", false);

  walker.next();
  table->isTemporary(walker.skipIf(TEMPORARY_SYMBOL));
  walker.next();
  if (walker.is(IF_SYMBOL)) {
    walker.next();
    result.second = walker.is(NOT_SYMBOL);
    walker.next();
    walker.skipIf(EXISTS_SYMBOL);
  }

  Identifier identifier = getIdentifier(walker);
  result.first = identifier.first;
  if (!identifier.first.empty())
    schema = ensureSchemaExists(catalog, identifier.first, caseSensistive);

  table->name(identifier.second);
  table->oldName(identifier.second);

  // Special case: copy existing table.
  if ((walker.is(OPEN_PAR_SYMBOL) && walker.lookAhead(true) == LIKE_SYMBOL) || walker.is(LIKE_SYMBOL)) {
    walker.next(walker.is(OPEN_PAR_SYMBOL) ? 2 : 1);
    Identifier reference = getIdentifier(walker);
    db_SchemaRef schema = catalog->defaultSchema();
    if (!reference.first.empty())
      schema = find_named_object_in_list(catalog->schemata(), reference.first);
    if (schema.is_valid()) {
      db_TableRef otherTable = find_named_object_in_list(schema->tables(), reference.second);
      if (otherTable.is_valid()) {
        bool isTemporary = table->isTemporary() != 0; // Value set already on the existing table and would get lost.
        table = grt::copy_object(db_mysql_TableRef::cast_from(otherTable));
        table->isTemporary(isTemporary);
      }
    }
  } else {
    // Note: we ignore here any part that uses a select/union clause.
    //       All of that is neither relevant for our table nor is it returned that way
    //       from the server.
    table->primaryKey(db_mysql_IndexRef());
    table->indices().remove_all();
    table->columns().remove_all();
    table->foreignKeys().remove_all();

    if (walker.is(OPEN_PAR_SYMBOL)) {
      walker.next();
      if (walker.is(PARTITION_SYMBOL)) {
        fillTablePartitioning(walker, table);
        walker.next(); // Skip CLOSE_PAR.

        // Ignore union clause. Since we are at the end of the table create statement
        // we don't need to skip anything else.
      } else {
        // Table create items.
        std::string schemaName = schema.is_valid() ? schema->name() : "";
        while (true) {
          fillTableCreateItem(walker, catalog, schemaName, table, autoGenerateFkNames, refCache);
          if (!walker.is(COMMA_SYMBOL))
            break;
          walker.next();
        }

        walker.next(); // Skip CLOSE_PAR.

        fillTableCreateOptions(walker, catalog, schema, table, caseSensistive);
        fillTablePartitioning(walker, table);

        // table_creation_source ignored.
      }
    } else {
      fillTableCreateOptions(walker, catalog, schema, table, caseSensistive);
      fillTablePartitioning(walker, table);

      // table_creation_source ignored.
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
*	Resolves all column/table references we collected before to existing objects.
*	If any of the references does not point to a valid object, we create a stub object for it.
*/
void resolveReferences(db_mysql_CatalogRef catalog, DbObjectsRefsCache refCache, bool caseSensitive) {
  grt::ListRef<db_mysql_Schema> schemata = catalog->schemata();

  for (DbObjectsRefsCache::iterator refIt = refCache.begin(); refIt != refCache.end(); ++refIt) {
    DbObjectReferences references = (*refIt);
    // Referenced table. Only used for foreign keys.
    db_mysql_TableRef referencedTable;
    if (references.type != DbObjectReferences::Index) {
      db_mysql_SchemaRef schema = find_named_object_in_list(schemata, references.targetIdentifier.first, caseSensitive);
      if (!schema.is_valid()) // Implicitly create the schema if we reference one not yet created.
        schema = ensureSchemaExists(catalog, references.targetIdentifier.first, caseSensitive);

      referencedTable = find_named_object_in_list(schema->tables(), references.targetIdentifier.second, caseSensitive);
      if (!referencedTable.is_valid()) {
        // If we don't find a table with the given name we create a stub object to be used instead.
        referencedTable = db_mysql_TableRef(grt::Initialized);
        referencedTable->owner(schema);
        referencedTable->isStub(1);
        referencedTable->name(references.targetIdentifier.second);
        referencedTable->oldName(references.targetIdentifier.second);
        schema->tables().insert(referencedTable);
      }

      if (references.foreignKey.is_valid() && (references.type == DbObjectReferences::Referenced))
        references.foreignKey->referencedTable(referencedTable);

      if (references.table.is_valid() && !references.table->tableEngine().empty() &&
          referencedTable->tableEngine().empty())
        referencedTable->tableEngine(references.table->tableEngine());
    }

    // Resolve columns.
    switch (references.type) {
      case DbObjectReferences::Index: {
        // Filling column references for an index.
        for (grt::ListRef<db_IndexColumn>::const_iterator indexIt = references.index->columns().begin();
             indexIt != references.index->columns().end(); ++indexIt) {
          db_mysql_ColumnRef column = find_named_object_in_list(references.table->columns(), (*indexIt)->name(), false);

          // Reset name field to avoid unnecessary trouble with test code.
          (*indexIt)->name("");
          if (column.is_valid())
            (*indexIt)->referencedColumn(column);
        }
        break;
      }

      case DbObjectReferences::Referencing: {
        // Filling column references for the referencing table.
        for (std::vector<std::string>::iterator nameIt = references.columnNames.begin();
             nameIt != references.columnNames.end(); ++nameIt) {
          db_mysql_ColumnRef column = find_named_object_in_list(references.table->columns(), *nameIt, false);
          if (column.is_valid())
            references.foreignKey->columns().insert(column);
        }
        break;
      }

      case DbObjectReferences::Referenced: {
        // Column references for the referenced table.
        int columnIndex = 0;

        for (std::vector<std::string>::iterator columnNameIt = references.columnNames.begin();
             columnNameIt != references.columnNames.end(); ++columnNameIt) {
          db_mysql_ColumnRef column = find_named_object_in_list(referencedTable->columns(), *columnNameIt,
                                                                false); // MySQL columns are always case-insensitive.

          if (!column.is_valid()) {
            if (referencedTable->isStub()) {
              column = db_mysql_ColumnRef(grt::Initialized);
              column->owner(referencedTable);
              column->name(*columnNameIt);
              column->oldName(*columnNameIt);

              // For the stub column we use all the data type settings from the foreign key column.
              db_mysql_ColumnRef templateColumn =
                db_mysql_ColumnRef::cast_from(references.foreignKey->columns().get(columnIndex));
              column->simpleType(templateColumn->simpleType());
              column->userType(templateColumn->userType());
              column->structuredType(templateColumn->structuredType());
              column->precision(templateColumn->precision());
              column->scale(templateColumn->scale());
              column->length(templateColumn->length());
              column->datatypeExplicitParams(templateColumn->datatypeExplicitParams());
              column->formattedType(templateColumn->formattedType());

              StringListRef templateFlags = templateColumn->flags();
              StringListRef flags = column->flags();

              for (grt::StringListRef::const_iterator flagIt = templateColumn->flags().begin();
                   flagIt != templateColumn->flags().end(); ++flagIt)
                flags.insert(*flagIt);

              column->characterSetName(templateColumn->characterSetName());
              column->collationName(templateColumn->collationName());

              referencedTable->columns().insert(column);
              references.foreignKey->referencedColumns().insert(column);
            } else {
              // Column not found in a non-stub table. We only add stub columns to stub tables.
              references.table->foreignKeys().gremove_value(references.foreignKey);
              break; // No need to check other columns. That FK is done.
            }
          } else
            references.foreignKey->referencedColumns().insert(column);

          ++columnIndex;
        }

        // Once done with adding all referenced columns add an index for the foreign key if it doesn't exist yet.
        //
        // Don't add an index if there are no FK columns, however.
        // TODO: Review this. There is no reason why we shouldn't create an index in this case.
        // Not sure what this decision is based on, but since the index is purely composed of
        // columns of the referencing table (which are known) it doesn't matter if there are FK columns or not.
        // But that's how the old parser did it, so we replicate this for now.
        //
        // Similarly, if a stub column is found the first time (i.e. created) the old parser did not
        // add an index for it either, which seems to be totally wrong. So we deviate here from that
        // behavior and create an index too in such cases.
        if (references.columnNames.empty())
          continue;

        ListRef<db_Column> fkColumns = references.foreignKey->columns();
        db_IndexRef foundIndex;
        for (grt::ListRef<db_mysql_Index>::const_iterator indexIt = references.table->indices().begin();
             indexIt != references.table->indices().end(); ++indexIt) {
          ListRef<db_IndexColumn> indexColumns = (*indexIt)->columns();

          bool indexMatches = true;

          // Go over all FK columns (not the index columns as they might differ).
          // Check that all FK columns are at the beginning of the index, in the same order.
          for (size_t i = 0; i < fkColumns->count(); ++i) {
            if (i >= indexColumns->count() || fkColumns->get(i) != indexColumns.get(i)->referencedColumn()) {
              indexMatches = false;
              break;
            }
          }

          if (indexMatches) {
            foundIndex = *indexIt;
            break;
          }
        }

        if (foundIndex.is_valid()) {
          if ((*foundIndex->indexType()).empty())
            foundIndex->indexType("INDEX");
          references.foreignKey->index(foundIndex);
        } else {
          // No valid index found, so create a new one.
          db_mysql_IndexRef index(grt::Initialized);
          index->owner(references.table);
          index->name(references.foreignKey->name());
          index->oldName(index->name());
          index->indexType("INDEX");
          references.foreignKey->index(index);

          for (ListRef<db_Column>::const_iterator columnIt = fkColumns.begin(); columnIt != fkColumns.end();
               ++columnIt) {
            db_mysql_IndexColumnRef indexColumn(grt::Initialized);
            indexColumn->owner(index);
            indexColumn->referencedColumn(*columnIt);
            index->columns().insert(indexColumn);
          }

          references.table->indices().insert(index);
        }
        break;
      }
      default:
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
*	Parses all values defined by the sql into the given table.
*	In opposition to other parse functions we pass the target object in by reference because it is possible that
*	the sql contains a LIKE clause (e.g. "create table a like b") which requires to duplicate the
*	referenced table and hence replace the inner value of the passed in table reference.
*/
size_t MySQLParserServicesImpl::parseTable(parser::MySQLParserContext::Ref context, db_mysql_TableRef table,
                                           const std::string &sql) {
  logDebug2("Parse table\n");

  table->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateTable);
  size_t error_count = context->recognizer()->error_info().size();
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  if (error_count == 0) {
    db_mysql_CatalogRef catalog;
    db_mysql_SchemaRef schema;
    if (table->owner().is_valid()) {
      schema = db_mysql_SchemaRef::cast_from(table->owner());
      catalog = db_mysql_CatalogRef::cast_from(schema->owner());
    }

    DbObjectsRefsCache refCache;
    fillTableDetails(walker, catalog, schema, table, context->case_sensitive(), true, refCache);
    resolveReferences(catalog, refCache, context->case_sensitive());
  } else {
    // Finished with errors. See if we can get at least the index name out.
    if (walker.advanceToType(TABLE_NAME_TOKEN, true)) {
      Identifier identifier = getIdentifier(walker);
      table->name(identifier.second + "_SYNTAX_ERROR");
    }
  }

  return error_count;
}

//--------------------------------------------------------------------------------------------------

std::pair<std::string, std::string> fillTriggerDetails(MySQLRecognizerTreeWalker &walker, db_mysql_TriggerRef trigger) {
  // There's no need for checks if any of the walker calls fail.
  // If we arrive here the syntax must be correct.
  trigger->enabled(1);

  walker.next(); // Skip CREATE.
  trigger->definer(getDefiner(walker));
  walker.next();
  Identifier identifier = getIdentifier(walker);
  trigger->name(identifier.second); // We store triggers relative to the tables they act on,
  // so we ignore here any qualifying schema.
  trigger->oldName(trigger->name());

  trigger->timing(walker.tokenText());
  walker.next();
  trigger->event(walker.tokenText());
  walker.next();

  // The referenced table is not stored in the trigger object as that is defined by it's position
  // in the grt tree. But we return schema + table to aid further processing.
  walker.next(); // Skip ON_SYMBOL.
  identifier = getIdentifier(walker);

  walker.skipTokenSequence(FOR_SYMBOL, EACH_SYMBOL, ROW_SYMBOL, 0);
  ANTLR3_UINT32 type = walker.tokenType();
  if (type == FOLLOWS_SYMBOL || type == PRECEDES_SYMBOL) {
    trigger->ordering(walker.tokenText());
    walker.next();
    trigger->otherTrigger(walker.tokenText());
    walker.next();
  }

  return identifier;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseTriggerSql(parser_ContextReferenceRef context_ref, db_mysql_TriggerRef trigger,
                                                const std::string &sql) {
  parser::MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseTrigger(context, trigger, sql);
}

//--------------------------------------------------------------------------------------------------

/**
* Parses the given sql as trigger create script and fills all found details in the given trigger ref.
* If there's an error nothing is changed.
* Returns the number of errors.
*/
size_t MySQLParserServicesImpl::parseTrigger(parser::MySQLParserContext::Ref context, db_mysql_TriggerRef trigger,
                                             const std::string &sql) {
  logDebug2("Parse trigger\n");

  trigger->sqlDefinition(base::trim(sql));
  trigger->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateTrigger);
  size_t error_count = context->recognizer()->error_info().size();
  int result_flag = 0;
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  if (error_count == 0)
    fillTriggerDetails(walker, trigger);
  else {
    result_flag = 1;

    // Finished with errors. See if we can get at least the trigger name out.
    if (walker.advanceToType(TRIGGER_NAME_TOKEN, true)) {
      Identifier identifier = getIdentifier(walker);
      trigger->name(identifier.second);
      trigger->oldName(trigger->name());
    }

    // Another attempt: find the ordering as we may need to manipulate this.
    if (walker.advanceToType(ROW_SYMBOL, true)) {
      walker.next();
      if (walker.is(FOLLOWS_SYMBOL) || walker.is(PRECEDES_SYMBOL)) {
        trigger->ordering(walker.tokenText());
        walker.next();
        if (walker.isIdentifier()) {
          trigger->otherTrigger(walker.tokenText());
          walker.next();
        }
      }
    }
  }

  trigger->modelOnly(result_flag);
  if (trigger->owner().is_valid()) {
    // TODO: this is modeled after the old parser code but doesn't make much sense this way.
    //       There's only one flag for all triggers. So, at least there should be a scan over all triggers
    //       when determining this flag.
    db_TableRef table = db_TableRef::cast_from(trigger->owner());
    if (result_flag == 1)
      table->customData().set("triggerInvalid", grt::IntegerRef(1));
    else
      table->customData().remove("triggerInvalid");
  }
  return error_count;
}

//--------------------------------------------------------------------------------------------------

/**
*	Returns schema name and ignore flag.
*/
std::pair<std::string, bool> fillViewDetails(MySQLRecognizerTreeWalker &walker, db_mysql_ViewRef view) {
  walker.next(); // Skip CREATE.

  std::pair<std::string, bool> result("", walker.is(OR_SYMBOL)); // OR REPLACE
  walker.skipIf(OR_SYMBOL, 2);

  if (walker.is(ALGORITHM_SYMBOL)) {
    walker.next(2); // ALGORITHM and EQUAL.
    switch (walker.tokenType()) {
      case MERGE_SYMBOL:
        view->algorithm(1);
        break;
      case TEMPTABLE_SYMBOL:
        view->algorithm(2);
        break;
      default:
        view->algorithm(0);
        break;
    }
    walker.next();
  } else
    view->algorithm(0);

  view->definer(getDefiner(walker));

  walker.skipIf(SQL_SYMBOL, 3); // SQL SECURITY (DEFINER | INVOKER)

  walker.next(1); // Skip VIEW.
  Identifier identifier = getIdentifier(walker);
  result.first = identifier.first;

  view->name(identifier.second);
  view->oldName(view->name());

  // Skip over the column list, if given. We don't use it atm.
  if (walker.is(OPEN_PAR_SYMBOL))
    getNamesList(walker);
  walker.next();        // Skip AS.
  walker.skipSubtree(); // Skip SELECT subtree.

  view->withCheckCondition(walker.is(WITH_SYMBOL));

  view->modelOnly(0);

  return result;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseViewSql(parser_ContextReferenceRef context_ref, db_mysql_ViewRef view,
                                             const std::string &sql) {
  parser::MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseView(context, view, sql);
}

//--------------------------------------------------------------------------------------------------

/**
* Parses the given sql as a create view script and fills all found details in the given view ref.
* If there's an error nothing changes. If the sql contains a schema reference other than that the
* the view is in the view's name will be changed (adds _WRONG_SCHEMA) to indicate that.
*/
size_t MySQLParserServicesImpl::parseView(parser::MySQLParserContext::Ref context, db_mysql_ViewRef view,
                                          const std::string &sql) {
  logDebug2("Parse view\n");

  view->sqlDefinition(base::trim(sql));
  view->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateView);
  size_t error_count = context->recognizer()->error_info().size();
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  if (error_count == 0) {
    db_mysql_SchemaRef schema;
    if (view->owner().is_valid())
      schema = db_mysql_SchemaRef::cast_from(view->owner());
    std::pair<std::string, bool> info = fillViewDetails(walker, view);
    if (!info.first.empty() && schema.is_valid()) {
      if (!base::same_string(schema->name(), info.first, context->case_sensitive())) {
        view->name(*view->name() + "_WRONG_SCHEMA");
        view->oldName(view->name());
      }
    }

  } else {
    // Finished with errors. See if we can get at least the view name out.
    if (walker.advanceToType(VIEW_NAME_TOKEN, true)) {
      Identifier identifier = getIdentifier(walker);
      view->name(identifier.second);
      view->oldName(view->name());
    }
    view->modelOnly(1);
  }

  return error_count;
}

//--------------------------------------------------------------------------------------------------

std::string fillRoutineDetails(MySQLRecognizerTreeWalker &walker, db_mysql_RoutineRef routine) {
  walker.next(); // Skip CREATE.
  routine->definer(getDefiner(walker));

  // A UDF is also a function and will be handled as such here.
  walker.skipIf(AGGREGATE_SYMBOL);
  if (walker.is(FUNCTION_SYMBOL))
    routine->routineType("function");
  else
    routine->routineType("procedure");

  walker.next(1); // Skip FUNCTION/PROCEDURE.
  Identifier identifier = getIdentifier(walker);

  routine->name(identifier.second);
  routine->oldName(routine->name());

  if (walker.is(RETURNS_SYMBOL)) {
    // UDF.
    routine->routineType("udf");
    walker.next();
    routine->returnDatatype(walker.tokenText());

    // SONAME is currently ignored.
  } else {
    // Parameters.
    ListRef<db_mysql_RoutineParam> params = routine->params();
    params.remove_all();
    walker.next(); // Skip OPEN_PAR.

    while (!walker.is(CLOSE_PAR_SYMBOL)) {
      db_mysql_RoutineParamRef param(grt::Initialized);
      param->owner(routine);

      switch (walker.tokenType()) {
        case IN_SYMBOL:
        case OUT_SYMBOL:
        case INOUT_SYMBOL:
          param->paramType(walker.tokenText());
          walker.next();
          break;
      }

      param->name(walker.tokenText());
      walker.next();

      // DATA_TYPE_TOKEN.
      param->datatype(walker.textForTree());
      params.insert(param);

      walker.skipSubtree();
      if (!walker.is(COMMA_SYMBOL))
        break;
      walker.next();
    }
    walker.next(); // Skip CLOSE_PAR.

    if (walker.is(RETURNS_SYMBOL)) {
      walker.next();

      // DATA_TYPE_TOKEN.
      routine->returnDatatype(walker.textForTree());
      walker.nextSibling();
    }

    if (walker.is(ROUTINE_CREATE_OPTIONS)) {
      // For now we only store comments and security settings.
      walker.next();
      bool done = false;
      do {
        switch (walker.tokenType()) {
          case SQL_SYMBOL:
            walker.next(2); // Skip SQL + SECURITY (both are siblings)
            routine->security(walker.tokenText());
            walker.next(); // Skip DEFINER/INVOKER
            break;

          case COMMENT_SYMBOL:
            walker.next();
            routine->comment(walker.tokenText());
            walker.skipSubtree();
            break;

          // Some options we just skip.
          case NOT_SYMBOL:
          case DETERMINISTIC_SYMBOL:
            walker.next(walker.is(NOT_SYMBOL) ? 2 : 1);
            break;

          case CONTAINS_SYMBOL:
          case LANGUAGE_SYMBOL:
          case NO_SYMBOL:
            walker.next(2);
            break;

          case READS_SYMBOL:
          case MODIFIES_SYMBOL:
            walker.next(3);
            break;

          default:
            done = true;
        }
      } while (!done);
    }
  }

  routine->modelOnly(0);

  return identifier.first;
}

//--------------------------------------------------------------------------------------------------

/**
* Tries to find the name and schema of a routine using a simple scan, as this is called only in error
* case and we have no AST to walk through.
* Returns a tuple with name and the found routine type. Both can be empty.
*/
std::pair<std::string, std::string> getRoutineNameAndType(parser::MySQLParserContext::Ref context,
                                                          const std::string &sql) {
  std::pair<std::string, std::string> result = std::make_pair("unknown", "unknown");
  std::shared_ptr<MySQLScanner> scanner = context->createScanner(sql);

  if (scanner->token_type() != CREATE_SYMBOL)
    return result;

  // Scan definer clause. Handling here is similar to getDefiner() only that we deal here with
  // potentially wrong syntax, have a scanner instead of the walker and don't need the definer value.
  scanner->next();
  if (scanner->is(DEFINER_SYMBOL)) {
    scanner->next();
    scanner->skipIf(EQUAL_OPERATOR);

    if (scanner->is(CURRENT_USER_SYMBOL)) {
      scanner->next();
      if (scanner->skipIf(OPEN_PAR_SYMBOL))
        scanner->skipIf(CLOSE_PAR_SYMBOL);
    } else {
      // A user@host entry.
      if (scanner->is_identifier() || scanner->is(SINGLE_QUOTED_TEXT))
        scanner->next();
      switch (scanner->token_type()) {
        case AT_TEXT_SUFFIX:
          scanner->next();
          break;
        case AT_SIGN_SYMBOL:
          scanner->next();
          if (scanner->is_identifier() || scanner->is(SINGLE_QUOTED_TEXT))
            scanner->next();
          break;
      }
    }
  }

  scanner->skipIf(AGGREGATE_SYMBOL);
  switch (scanner->token_type()) {
    case PROCEDURE_SYMBOL:
      result.second = "procedure";
      scanner->next();
      break;

    case FUNCTION_SYMBOL: // Both normal function and UDF.
      result.second = "function";
      scanner->next();
      break;
  }

  if (scanner->is_identifier()) {
    result.first = base::unquote(scanner->token_text());
    scanner->next();
    if (scanner->skipIf(DOT_SYMBOL)) {
      // Qualified identifier.
      if (scanner->is_identifier())
        result.first = base::unquote(scanner->token_text());
    }
  }

  if (scanner->is(RETURNS_SYMBOL))
    result.second = "udf";

  return result;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseRoutineSql(parser_ContextReferenceRef context_ref, db_mysql_RoutineRef routine,
                                                const std::string &sql) {
  parser::MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseRoutine(context, routine, sql);
}

//--------------------------------------------------------------------------------------------------

/**
* Parses the given sql as a create function/procedure script and fills all found details in the given routine ref.
* If there's an error nothing changes. If the sql contains a schema reference other than that the
* the routine is in the routine's name will be changed (adds _WRONG_SCHEMA) to indicate that.
*/
size_t MySQLParserServicesImpl::parseRoutine(parser::MySQLParserContext::Ref context, db_mysql_RoutineRef routine,
                                             const std::string &sql) {
  logDebug2("Parse routine\n");

  routine->sqlDefinition(base::trim(sql));
  routine->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateRoutine);
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  size_t error_count = context->recognizer()->error_info().size();
  if (error_count == 0) {
    std::string schemaName = fillRoutineDetails(walker, routine);
    if (!schemaName.empty() && routine->owner().is_valid()) {
      db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(routine->owner());
      if (!base::same_string(schema->name(), schemaName, false)) // Routine names are never case sensitive.
      {
        routine->name(*routine->name() + "_WRONG_SCHEMA");
        routine->oldName(routine->name());
      }
    }
  } else {
    // Finished with errors. See if we can get at least the routine name out.
    std::pair<std::string, std::string> values = getRoutineNameAndType(context, sql);
    routine->name(values.first + "_SYNTAX_ERROR");
    routine->routineType(values.second);

    routine->modelOnly(1);
  }

  return error_count;
}

//--------------------------------------------------------------------------------------------------

bool consider_as_same_type(std::string type1, std::string type2) {
  if (type1 == type2)
    return true;

  if (type1 == "function" && type2 == "udf")
    return true;

  if (type2 == "function" && type1 == "udf")
    return true;

  return false;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseRoutinesSql(parser_ContextReferenceRef context_ref, db_mysql_RoutineGroupRef group,
                                                 const std::string &sql) {
  parser::MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseRoutines(context, group, sql);
}

//--------------------------------------------------------------------------------------------------

/**
* Parses the given sql as a list of create function/procedure statements.
* In case of an error handling depends on the error position. We try to get most of the routines out
* of the script.
*
* This process has two parts attached:
*   - Update the sql text + properties for any routine that is in the script in the owning schema.
*   - Update the list of routines in the given routine group to what is in the script.
*/
size_t MySQLParserServicesImpl::parseRoutines(parser::MySQLParserContext::Ref context, db_mysql_RoutineGroupRef group,
                                              const std::string &sql) {
  logDebug2("Parse routine group\n");

  size_t error_count = 0;

  std::vector<std::pair<size_t, size_t> > ranges;
  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges, "\n");

  grt::ListRef<db_Routine> routines = group->routines();
  routines.remove_all();

  db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(group->owner());
  grt::ListRef<db_Routine> schema_routines = schema->routines();

  int sequence_number = 0;
  int syntax_error_counter = 1;

  for (std::vector<std::pair<size_t, size_t> >::iterator iterator = ranges.begin(); iterator != ranges.end();
       ++iterator) {
    std::string routineSQL = sql.substr(iterator->first, iterator->second);
    context->recognizer()->parse(sql.c_str() + iterator->first, iterator->second, true,
                                 MySQLParseUnit::PuCreateRoutine);
    size_t local_error_count = context->recognizer()->error_info().size();
    error_count += local_error_count;

    // Before filling a routine we need to know if there's already one with that name in the schema.
    // Hence we first extract the name and act based on that.
    MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
    std::pair<std::string, std::string> values = getRoutineNameAndType(context, routineSQL);

    // If there's no usable info from parsing preserve at least the code and generate a
    // name for the routine using a counter.
    if (values.first == "unknown" || values.second == "unknown") {
      // Create a new routine instance.
      db_mysql_RoutineRef routine = db_mysql_RoutineRef(grt::Initialized);
      routine->createDate(base::fmttime(0, DATETIME_FMT));
      routine->lastChangeDate(routine->createDate());
      routine->owner(schema);
      schema_routines.insert(routine);

      routine->name(*group->name() + "_SYNTAX_ERROR_" + std::to_string(syntax_error_counter++));
      routine->routineType("unknown");
      routine->modelOnly(1);
      routine->sqlDefinition(base::trim(routineSQL));

      routines.insert(routine);
    } else {
      db_mysql_RoutineRef routine;
      for (size_t i = 0; i < schema_routines.count(); ++i) {
        // Stored functions and UDFs share the same namespace.
        // Stored routine names are not case sensitive.
        db_RoutineRef candidate = schema_routines[i];
        std::string name = candidate->name();

        // Remove automatically added appendixes before comparing names.
        if (base::hasSuffix(name, "_WRONG_SCHEMA"))
          name.resize(name.size() - 13);
        if (base::hasSuffix(name, "_SYNTAX_ERROR"))
          name.resize(name.size() - 13);

        if (base::same_string(values.first, name, false) &&
            consider_as_same_type(values.second, candidate->routineType())) {
          routine = db_mysql_RoutineRef::cast_from(candidate);
          break;
        }
      }

      walker.reset();
      if (!routine.is_valid()) {
        // Create a new routine instance.
        routine = db_mysql_RoutineRef(grt::Initialized);
        routine->createDate(base::fmttime(0, DATETIME_FMT));
        routine->owner(schema);
        schema_routines.insert(routine);
      }

      if (local_error_count == 0)
        fillRoutineDetails(walker, routine);
      else {
        routine->name(values.first + "_SYNTAX_ERROR");
        routine->routineType(values.second);

        routine->modelOnly(1);
      }

      routine->sqlDefinition(base::trim(routineSQL));
      routine->lastChangeDate(base::fmttime(0, DATETIME_FMT));

      // Finally add the routine to the group if it isn't already there.
      bool found = false;
      for (size_t i = 0; i < routines.count(); ++i) {
        if (base::same_string(routine->name(), routines[i]->name(), false)) {
          found = true;
          break;
        }
      }
      if (!found)
        routines.insert(routine);
    }
  }

  return error_count;
}

//--------------------------------------------------------------------------------------------------

static void fillSchemaOptions(MySQLRecognizerTreeWalker &walker, db_mysql_CatalogRef catalog,
                              db_mysql_SchemaRef schema) {
  std::string defaultCharset = catalog.is_valid() ? catalog->defaultCharacterSetName() : "";
  std::string defaultCollation = catalog.is_valid() ? catalog->defaultCollationName() : "";

  // Charset or collation info.
  bool done = false;
  while (!done) {
    walker.skipIf(DEFAULT_SYMBOL);
    switch (walker.tokenType()) {
      case CHAR_SYMBOL: // CHARACTER is mapped to CHAR.
      case CHARSET_SYMBOL: {
        std::pair<std::string, std::string> info =
          detailsForCharset(getCharsetName(walker), defaultCollation, defaultCharset);
        schema->defaultCharacterSetName(info.first);
        schema->defaultCollationName(info.second);
        walker.next();
        break;
      }

      case COLLATE_SYMBOL: {
        walker.next();
        walker.skipIf(EQUAL_OPERATOR);

        std::pair<std::string, std::string> info = detailsForCollation(walker.tokenText(), defaultCollation);
        schema->defaultCharacterSetName(info.first);
        schema->defaultCollationName(info.second);
        walker.next();
        break;
      }

      default:
        done = true;
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

static bool fillSchemaDetails(MySQLRecognizerTreeWalker &walker, db_mysql_CatalogRef catalog,
                              db_mysql_SchemaRef schema) {
  bool ignoreIfExists = false;

  walker.next(2); // Skip CREATE SCHEMA.
  if (walker.is(IF_SYMBOL)) {
    ignoreIfExists = true;
    walker.next(3); // Skip IF NOT EXISTS.
  }
  Identifier identifier = getIdentifier(walker);
  schema->name(identifier.second);
  schema->oldName(schema->name());

  fillSchemaOptions(walker, catalog, schema);

  return ignoreIfExists;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseSchema(parser::MySQLParserContext::Ref context, db_mysql_SchemaRef schema,
                                            const std::string &sql) {
  logDebug2("Parse schema\n");

  schema->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuGeneric);
  size_t error_count = context->recognizer()->error_info().size();
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  if (error_count == 0)
    fillSchemaDetails(walker, db_mysql_CatalogRef::cast_from(schema->owner()), schema);
  else {
    // Finished with errors. See if we can get at least the schema name out.
    if (walker.advanceToType(SCHEMA_NAME_TOKEN, true)) {
      Identifier identifier = getIdentifier(walker);
      schema->name(identifier.second + "_SYNTAX_ERROR");
    }
  }

  return error_count;
}

//--------------------------------------------------------------------------------------------------

Identifier fillIndexDetails(MySQLRecognizerTreeWalker &walker, db_mysql_CatalogRef catalog, db_mysql_SchemaRef schema,
                            db_mysql_IndexRef index, bool caseSensitiv) {
  walker.next(); // Skip CREATE.
  if (walker.is(ONLINE_SYMBOL) || walker.is(OFFLINE_SYMBOL))
    walker.next();

  index->unique(0);
  switch (walker.tokenType()) {
    case UNIQUE_SYMBOL:
    case INDEX_SYMBOL:
      if (walker.is(UNIQUE_SYMBOL)) {
        index->unique(1);
        index->indexType("UNIQUE");
        walker.next();
      } else
        index->indexType(formatIndexType(walker.tokenText()));

      walker.next();
      break;

    case FULLTEXT_SYMBOL:
    case SPATIAL_SYMBOL:
      index->indexType(formatIndexType(walker.tokenText()));
      walker.next(2); // Skip FULLTEXT/SPATIAL INDEX.
      break;
  }

  Identifier identifier = getIdentifier(walker);
  index->name(identifier.second);
  index->oldName(index->name());

  if (walker.is(USING_SYMBOL) || walker.is(TYPE_SYMBOL)) {
    walker.next();
    index->indexKind(walker.tokenText());
    walker.next();
  }

  walker.next(); // Skip ON.
  identifier = getIdentifier(walker);

  // Index columns.
  // Note: the referenced column for an index column can only be set if we can find the table
  //       for it (via the catalog reference).
  db_TableRef table;
  if (catalog.is_valid()) {
    if (!identifier.first.empty())
      schema = find_named_object_in_list(catalog->schemata(), identifier.first, caseSensitiv);
    if (schema.is_valid())
      table = find_named_object_in_list(schema->tables(), identifier.second, caseSensitiv);

    // As last resort, check the owner, if we haven't found a table yet.
    if (!table.is_valid() && index->owner().is_valid())
      table = db_TableRef::cast_from(index->owner());
  }

  fillIndexColumns(walker, table, index);
  fillIndexOptions(walker, index);

  bool done = false;
  while (!done) {
    // Actually only one occurrence of each. But in any order.
    switch (walker.tokenType()) {
      case ALGORITHM_SYMBOL: {
        walker.next();
        if (walker.is(EQUAL_OPERATOR))
          walker.next();

        // The algorithm can be any text, but allowed are only a small number of values.
        std::string algorithm = base::toupper(walker.tokenText());
        if (algorithm == "DEFAULT" || algorithm == "INPLACE" || algorithm == "COPY")
          index->algorithm(algorithm);

        break;
      }

      case LOCK_SYMBOL: {
        walker.next();
        if (walker.is(EQUAL_OPERATOR))
          walker.next();

        // The lock type can be any text, but allowed are only a small number of values.
        std::string lock = base::toupper(walker.tokenText());
        if (lock == "DEFAULT" || lock == "NONE" || lock == "SHARED" || lock == "EXCLUSIVE")
          index->lockOption(lock);

        break;
      }

      default:
        done = true;
        break;
    }
  }

  return identifier;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseIndex(parser::MySQLParserContext::Ref context, db_mysql_IndexRef index,
                                           const std::string &sql) {
  logDebug2("Parse index\n");

  index->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateIndex);
  size_t error_count = context->recognizer()->error_info().size();
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  if (error_count == 0) {
    db_mysql_CatalogRef catalog;
    db_mysql_SchemaRef schema;

    if (index->owner().is_valid()) {
      schema = db_mysql_SchemaRef::cast_from(index->owner()->owner());
      catalog = db_mysql_CatalogRef::cast_from(schema->owner());
    }
    fillIndexDetails(walker, catalog, schema, index, context->case_sensitive());
  } else {
    // Finished with errors. See if we can get at least the index name out.
    if (walker.advanceToType(INDEX_NAME_TOKEN, true)) {
      Identifier identifier = getIdentifier(walker);
      index->name(identifier.second + "_SYNTAX_ERROR");
    }
  }

  return error_count;
}

//--------------------------------------------------------------------------------------------------

/**
 *	Returns schema name and ignore flag.
 */
std::pair<std::string, bool> fillEventDetails(MySQLRecognizerTreeWalker &walker, db_mysql_EventRef event) {
  std::pair<std::string, bool> result("", false);

  walker.next(); // Skip CREATE.
  event->definer(getDefiner(walker));
  walker.next(); // Skip EVENT.

  if (walker.is(IF_SYMBOL)) {
    result.second = true;
    walker.next(3); // Skip IF NOT EXISTS.
  }

  Identifier identifier = getIdentifier(walker);
  result.first = identifier.first;
  event->name(identifier.second);
  event->oldName(event->name());
  walker.next(2); // Skip ON SCHEDULE.

  event->useInterval(walker.tokenType() != AT_SYMBOL);
  if (event->useInterval()) {
    walker.next();                   // Skip EVERY.
    event->at(walker.textForTree()); // The full expression subtree.
    walker.skipSubtree();
    event->intervalUnit(walker.tokenText());
    walker.next();

    if (walker.is(STARTS_SYMBOL)) {
      walker.next();
      event->intervalStart(walker.textForTree());
      walker.skipSubtree();
    }

    if (walker.is(ENDS_SYMBOL)) {
      walker.next();
      event->intervalEnd(walker.textForTree());
      walker.skipSubtree();
    }
  } else {
    walker.next();
    event->at(walker.textForTree()); // The full expression subtree.
    walker.skipSubtree();
  }

  if (walker.is(ON_SYMBOL)) // ON COMPLETION NOT? PRESERVE
  {
    walker.next(2);
    event->preserved(walker.tokenType() != NOT_SYMBOL);
    walker.next(event->preserved() ? 1 : 2);
  }

  bool enabled = true;
  if (walker.is(ENABLE_SYMBOL) || walker.is(DISABLE_SYMBOL)) {
    // Enabled/Disabled is optional.
    enabled = walker.is(ENABLE_SYMBOL);
    walker.next();
    if (walker.is(ON_SYMBOL))
      walker.next(2); // Skip ON SLAVE.
  }
  event->enabled(enabled);

  if (walker.is(COMMENT_SYMBOL)) {
    walker.next();
    event->comment(walker.tokenText());
    walker.skipSubtree();
  }
  walker.next(); // Skip DO.

  return result;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseEvent(parser::MySQLParserContext::Ref context, db_mysql_EventRef event,
                                           const std::string &sql) {
  logDebug2("Parse event\n");

  event->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateEvent);
  size_t error_count = context->recognizer()->error_info().size();
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  if (error_count == 0)
    fillEventDetails(walker, event);
  else {
    // Finished with errors. See if we can get at least the event name out.
    if (walker.advanceToType(EVENT_NAME_TOKEN, true)) {
      Identifier identifier = getIdentifier(walker);
      event->name(identifier.second + "_SYNTAX_ERROR");
    }
  }

  return error_count;
}

//--------------------------------------------------------------------------------------------------

void fillLogfileGroupDetails(MySQLRecognizerTreeWalker &walker, db_mysql_LogFileGroupRef group) {
  walker.next(3); // Skip CREATE LOGFILE GROUP.
  Identifier identifier = getIdentifier(walker);
  group->name(identifier.second);
  group->oldName(group->name());

  walker.next(2); // Skip ADD (UNDOFILE | REDOFILE).
  group->undoFile(walker.tokenText());
  walker.skipSubtree();

  if (walker.is(LOGFILE_GROUP_OPTIONS_TOKEN)) {
    walker.next();
    bool done = false;
    while (!done) {
      // Unlimited occurrences.
      unsigned int token = walker.tokenType();
      switch (token) {
        case INITIAL_SIZE_SYMBOL:
        case UNDO_BUFFER_SIZE_SYMBOL:
        case REDO_BUFFER_SIZE_SYMBOL: {
          walker.next();
          walker.skipIf(EQUAL_OPERATOR);
          std::string value = walker.tokenText();
          walker.next();

          // Value can have a suffix. And it can be a hex number (atoi is supposed to handle that).
          size_t factor = 1;
          switch (::tolower(value[value.size() - 1])) {
            // All cases fall through.
            case 'g':
              factor *= 1024;
            case 'm':
              factor *= 1024;
            case 'k':
              factor *= 1024;
              value[value.size() - 1] = 0;
          }
          if (token == INITIAL_SIZE_SYMBOL)
            group->initialSize(factor * base::atoi<size_t>(value));
          else
            group->undoBufferSize(factor * base::atoi<size_t>(value));

          break;
        }

        case NODEGROUP_SYMBOL:
          walker.next();
          walker.skipIf(EQUAL_OPERATOR);

          // An integer or hex number (no suffix).
          group->nodeGroupId(base::atoi<size_t>(walker.tokenText()));
          walker.next();

          break;

        case WAIT_SYMBOL:
        case NO_WAIT_SYMBOL:
          group->wait(token == WAIT_SYMBOL);
          walker.next();
          break;

        case COMMENT_SYMBOL:
          walker.next();
          walker.skipIf(EQUAL_OPERATOR);
          group->comment(walker.tokenText());
          walker.skipSubtree();
          break;

        case STORAGE_SYMBOL:
        case ENGINE_SYMBOL:
          walker.next(token == STORAGE_SYMBOL ? 2 : 1);
          walker.skipIf(EQUAL_OPERATOR);
          walker.next(); // Skip ENGINE_REF_TOKEN.
          group->engine(walker.tokenText());
          walker.next();
          break;

        default:
          done = true;
          break;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseLogfileGroup(parser::MySQLParserContext::Ref context,
                                                  db_mysql_LogFileGroupRef group, const std::string &sql) {
  logDebug2("Parse logfile group\n");

  group->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateLogfileGroup);
  size_t error_count = context->recognizer()->error_info().size();
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  if (error_count == 0)
    fillLogfileGroupDetails(walker, group);
  else {
    if (walker.advanceToType(LOGFILE_GROUP_NAME_TOKEN, true)) {
      walker.next();
      std::string name = walker.tokenText();
      group->name(name + "_SYNTAX_ERROR");
    }
  }

  return error_count;
}

//--------------------------------------------------------------------------------------------------

void fillServerDetails(MySQLRecognizerTreeWalker &walker, db_mysql_ServerLinkRef server) {
  walker.next(2); // Skip CREATE SERVER.
  Identifier identifier = getIdentifier(walker);
  server->name(identifier.second);
  server->oldName(server->name());

  walker.next(3); // Skip FOREIGN DATA WRAPPER.
  server->wrapperName(walker.tokenText());
  walker.next(3); // Skip <name> OPTIONS OPEN_PAR_SYMBOL.

  while (true) {
    switch (walker.tokenType()) {
      case HOST_SYMBOL:
        walker.next();
        server->host(walker.tokenText());
        walker.next();
        break;
      case DATABASE_SYMBOL:
        walker.next();
        server->schema(walker.tokenText());
        walker.next();
        break;
      case USER_SYMBOL:
        walker.next();
        server->user(walker.tokenText());
        walker.next();
        break;
      case PASSWORD_SYMBOL:
        walker.next();
        server->password(walker.tokenText());
        walker.next();
        break;
      case SOCKET_SYMBOL:
        walker.next();
        server->socket(walker.tokenText());
        walker.next();
        break;
      case OWNER_SYMBOL:
        walker.next();
        server->ownerUser(walker.tokenText());
        walker.next();
        break;
      case PORT_SYMBOL:
        walker.next();
        server->port(walker.tokenText()); // The grt definition should be int not string...
        walker.next();
        break;
    }

    if (walker.is(CLOSE_PAR_SYMBOL))
      break;
    walker.next(); // Skip comma.
  }
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseServer(parser::MySQLParserContext::Ref context, db_mysql_ServerLinkRef server,
                                            const std::string &sql) {
  logDebug2("Parse server\n");

  server->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateServer);
  size_t error_count = context->recognizer()->error_info().size();
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  if (error_count == 0)
    fillServerDetails(walker, server);
  else {
    if (walker.advanceToType(LOGFILE_GROUP_NAME_TOKEN, true)) {
      Identifier identifier = getIdentifier(walker);
      server->name(identifier.second + "_SYNTAX_ERROR");
    }
  }

  return error_count;
}

//--------------------------------------------------------------------------------------------------

void fillTablespaceDetails(MySQLRecognizerTreeWalker &walker, db_CatalogRef catalog,
                           db_mysql_TablespaceRef tablespace) {
  walker.next(2); // Skip CREATE TABLESPACE.
  Identifier identifier = getIdentifier(walker);
  tablespace->name(identifier.second);
  tablespace->oldName(tablespace->name());

  walker.next(2); // Skip ADD DATAFILE.
  tablespace->dataFile(walker.tokenText());
  walker.skipSubtree();

  if (walker.is(USE_SYMBOL)) {
    walker.next(3); // Skip USE LOGFILE GROUP.
    Identifier identifier = getIdentifier(walker);
    if (catalog.is_valid()) {
      db_LogFileGroupRef logfileGroup = find_named_object_in_list(catalog->logFileGroups(), identifier.second);
      if (logfileGroup.is_valid())
        tablespace->logFileGroup(logfileGroup);
    }
  }

  if (walker.is(TABLESPACE_OPTIONS_TOKEN)) {
    walker.next();
    bool done = false;
    while (!done) {
      // Unlimited occurrences.
      unsigned int token = walker.tokenType();
      switch (token) {
        case INITIAL_SIZE_SYMBOL:
        case AUTOEXTEND_SIZE_SYMBOL:
        case MAX_SIZE_SYMBOL:
        case EXTENT_SIZE_SYMBOL: {
          walker.next();
          walker.skipIf(EQUAL_OPERATOR);
          std::string value = walker.tokenText();
          walker.next();

          // Value can have a suffix. And it can be a hex number (atoi is supposed to handle that).
          size_t factor = 1;
          switch (::tolower(value[value.size() - 1])) {
            // All cases fall through.
            case 'g':
              factor *= 1024;
            case 'm':
              factor *= 1024;
            case 'k':
              factor *= 1024;
              value[value.size() - 1] = 0;
          }

          size_t number = factor * base::atoi<size_t>(value);
          switch (token) {
            case INITIAL_SIZE_SYMBOL:
              tablespace->initialSize(number);
              break;
            case AUTOEXTEND_SIZE_SYMBOL:
              tablespace->autoExtendSize(number);
              break;
            case MAX_SIZE_SYMBOL:
              tablespace->maxSize(number);
              break;
            case EXTENT_SIZE_SYMBOL:
              tablespace->extentSize(number);
              break;
          }

          break;
        }

        case NODEGROUP_SYMBOL:
          walker.next();
          walker.skipIf(EQUAL_OPERATOR);

          // An integer or hex number (no suffix).
          tablespace->nodeGroupId(base::atoi<size_t>(walker.tokenText()));
          walker.next();

          break;

        case WAIT_SYMBOL:
        case NO_WAIT_SYMBOL:
          tablespace->wait(token == WAIT_SYMBOL);
          walker.next();
          break;

        case COMMENT_SYMBOL:
          walker.next();
          walker.skipIf(EQUAL_OPERATOR);
          tablespace->comment(walker.tokenText());
          walker.skipSubtree();
          break;

        case STORAGE_SYMBOL:
        case ENGINE_SYMBOL: {
          walker.next(token == STORAGE_SYMBOL ? 2 : 1);
          walker.skipIf(EQUAL_OPERATOR);
          Identifier identifier = getIdentifier(walker);
          tablespace->engine(identifier.second);
          break;
        }

        default:
          done = true;
          break;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseTablespace(parser::MySQLParserContext::Ref context,
                                                db_mysql_TablespaceRef tablespace, const std::string &sql) {
  logDebug2("Parse tablespace\n");

  tablespace->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateServer);
  size_t error_count = context->recognizer()->error_info().size();
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  if (error_count == 0) {
    db_CatalogRef catalog;
    if (tablespace->owner().is_valid() && tablespace->owner()->owner().is_valid())
      catalog = db_CatalogRef::cast_from(tablespace->owner()->owner()->owner());
    fillTablespaceDetails(walker, catalog, tablespace);
  } else {
    if (walker.advanceToType(TABLESPACE_NAME_TOKEN, true)) {
      Identifier identifier = getIdentifier(walker);
      tablespace->name(identifier.second + "_SYNTAX_ERROR");
    }
  }

  return error_count;
}

size_t MySQLParserServicesImpl::parseSQLIntoCatalogSql(parser_ContextReferenceRef context_ref,
                                                       db_mysql_CatalogRef catalog, const std::string &sql,
                                                       grt::DictRef options) {
  parser::MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseSQLIntoCatalog(context, catalog, sql, options);
}

/**
*	Expects the sql to be a single or multi-statement text in utf-8 encoding which is parsed and
*	the details are used to build a grt tree. Existing objects are replaced unless the SQL has
*	an "if not exist" clause (or no "or replace" clause for views).
*	Statements handled are: create, drop and table rename, everything else is ignored.
*
*  Note for case sensitivity: only schema, table and trigger names *can* be case sensitive.
*  This is determined by the case_sensitive() function of the given context. All other objects
*  are searched for case-insensitively.
*
*	@result Returns the number of errors found during parsing.
*/
size_t MySQLParserServicesImpl::parseSQLIntoCatalog(parser::MySQLParserContext::Ref context,
                                                    db_mysql_CatalogRef catalog, const std::string &sql,
                                                    grt::DictRef options) {
  std::set<MySQLQueryType> relevantQueryTypes;
  relevantQueryTypes.insert(QtAlterDatabase);
  relevantQueryTypes.insert(QtAlterLogFileGroup);
  relevantQueryTypes.insert(QtAlterFunction);
  relevantQueryTypes.insert(QtAlterProcedure);
  relevantQueryTypes.insert(QtAlterServer);
  relevantQueryTypes.insert(QtAlterTable);
  relevantQueryTypes.insert(QtAlterTableSpace);
  relevantQueryTypes.insert(QtAlterEvent);
  relevantQueryTypes.insert(QtAlterView);

  relevantQueryTypes.insert(QtCreateTable);
  relevantQueryTypes.insert(QtCreateIndex);
  relevantQueryTypes.insert(QtCreateDatabase);
  relevantQueryTypes.insert(QtCreateEvent);
  relevantQueryTypes.insert(QtCreateView);
  relevantQueryTypes.insert(QtCreateRoutine);
  relevantQueryTypes.insert(QtCreateProcedure);
  relevantQueryTypes.insert(QtCreateFunction);
  relevantQueryTypes.insert(QtCreateUdf);
  relevantQueryTypes.insert(QtCreateTrigger);
  relevantQueryTypes.insert(QtCreateLogFileGroup);
  relevantQueryTypes.insert(QtCreateServer);
  relevantQueryTypes.insert(QtCreateTableSpace);

  relevantQueryTypes.insert(QtDropDatabase);
  relevantQueryTypes.insert(QtDropEvent);
  relevantQueryTypes.insert(QtDropFunction);
  relevantQueryTypes.insert(QtDropProcedure);
  relevantQueryTypes.insert(QtDropIndex);
  relevantQueryTypes.insert(QtDropLogfileGroup);
  relevantQueryTypes.insert(QtDropServer);
  relevantQueryTypes.insert(QtDropTable);
  relevantQueryTypes.insert(QtDropTablespace);
  relevantQueryTypes.insert(QtDropTrigger);
  relevantQueryTypes.insert(QtDropView);

  relevantQueryTypes.insert(QtRenameTable);

  relevantQueryTypes.insert(QtUse);

  logDebug2("Parse sql into catalog\n");

  bool caseSensitive = context->case_sensitive();

  std::string startSchema = options.get_string("schema");
  db_mysql_SchemaRef currentSchema;
  if (!startSchema.empty())
    currentSchema = ensureSchemaExists(catalog, startSchema, caseSensitive);

  bool defaultSchemaCreated = false;
  bool autoGenerateFkNames = options.get_int("gen_fk_names_when_empty") != 0;
  // bool reuseExistingObjects = options.get_int("reuse_existing_objects") != 0;

  if (!currentSchema.is_valid()) {
    currentSchema = db_mysql_SchemaRef::cast_from(catalog->defaultSchema());
    if (!currentSchema.is_valid()) {
      db_SchemaRef df = find_named_object_in_list(catalog->schemata(), "default_schema", caseSensitive);
      if (!df.is_valid())
        defaultSchemaCreated = true;
      currentSchema = ensureSchemaExists(catalog, "default_schema", caseSensitive);
    }
  }

  size_t errorCount = 0;
  MySQLRecognizer *recognizer = context->recognizer();
  std::shared_ptr<MySQLQueryIdentifier> queryIdentifier = context->createQueryIdentifier();

  std::vector<std::pair<size_t, size_t> > ranges;
  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges, "\n");

  grt::ListRef<GrtObject> createdObjects = grt::ListRef<GrtObject>::cast_from(options.get("created_objects"));
  if (!createdObjects.is_valid()) {
    createdObjects = grt::ListRef<GrtObject>(grt::Initialized);
    options.set("created_objects", createdObjects);
  }

  // Collect textual FK references into a local cache. At the end this is used
  // to find actual ref tables + columns, when all tables have been parsed.
  DbObjectsRefsCache refCache;
  for (std::vector<std::pair<size_t, size_t> >::iterator iterator = ranges.begin(); iterator != ranges.end();
       ++iterator) {
    // std::string ddl(sql.c_str() + iterator->first, iterator->second);
    MySQLQueryType queryType = queryIdentifier->getQueryType(sql.c_str() + iterator->first, iterator->second, true);
    size_t errors = queryIdentifier->error_info().size(); // Can only be lexer errors.
    if (errors > 0) {
      errorCount += errors;
      continue;
    }

    if (relevantQueryTypes.count(queryType) == 0)
      continue; // Something we are not interested in. Don't bother parsing it.

    recognizer->parse(sql.c_str() + iterator->first, iterator->second, true, MySQLParseUnit::PuGeneric);
    errors = recognizer->error_info().size();
    if (errors > 0) {
      errorCount += errors;
      continue;
    }

    MySQLRecognizerTreeWalker walker = recognizer->tree_walker();

    switch (queryType) {
      case QtCreateTable: {
        db_mysql_TableRef table(grt::Initialized);
        table->createDate(base::fmttime(0, DATETIME_FMT));
        table->lastChangeDate(table->createDate());

        std::pair<std::string, bool> result =
          fillTableDetails(walker, catalog, currentSchema, table, caseSensitive, autoGenerateFkNames, refCache);
        db_mysql_SchemaRef schema = currentSchema;
        if (!result.first.empty() && !base::same_string(schema->name(), result.first, caseSensitive))
          schema = ensureSchemaExists(catalog, result.first, caseSensitive);
        table->owner(schema);

        // Ignore tables that use a name that is already used for a view (no drop/new-add takes place then).
        db_mysql_ViewRef existingView = find_named_object_in_list(schema->views(), table->name());
        if (!existingView.is_valid()) {
          db_TableRef existingTable = find_named_object_in_list(schema->tables(), table->name());
          if (existingTable.is_valid()) {
            // Ignore if the table exists already?
            if (!result.second) {
              schema->tables()->remove(existingTable);
              schema->tables().insert(table);
              createdObjects.insert(table);
            }
          } else {
            schema->tables().insert(table);
            createdObjects.insert(table);
          }
        }

        break;
      }

      case QtCreateIndex: {
        db_mysql_IndexRef index(grt::Initialized);
        index->createDate(base::fmttime(0, DATETIME_FMT));
        index->lastChangeDate(index->createDate());

        Identifier tableReference = fillIndexDetails(walker, catalog, currentSchema, index, caseSensitive);
        db_SchemaRef schema = currentSchema;
        if (!tableReference.first.empty() && !base::same_string(schema->name(), tableReference.first, caseSensitive))
          schema = ensureSchemaExists(catalog, tableReference.first, caseSensitive);
        db_TableRef table = find_named_object_in_list(schema->tables(), tableReference.second, caseSensitive);
        if (table.is_valid()) {
          index->owner(table);

          db_IndexRef existing = find_named_object_in_list(table->indices(), index->name());
          if (existing.is_valid())
            table->indices()->remove(existing);
          table->indices().insert(index);
          createdObjects.insert(index);
        }

        break;
      }

      case QtCreateDatabase: {
        db_mysql_SchemaRef schema(grt::Initialized);
        schema->createDate(base::fmttime(0, DATETIME_FMT));
        schema->lastChangeDate(schema->createDate());

        std::pair<std::string, std::string> info = detailsForCharset(
          catalog->defaultCharacterSetName(), catalog->defaultCollationName(), catalog->defaultCharacterSetName());
        schema->defaultCharacterSetName(info.first);
        schema->defaultCollationName(info.second);

        bool ignoreIfExists = fillSchemaDetails(walker, catalog, schema);
        schema->owner(catalog);

        db_SchemaRef existing = find_named_object_in_list(catalog->schemata(), schema->name(), caseSensitive);
        if (existing.is_valid()) {
          if (!ignoreIfExists) {
            catalog->schemata()->remove(existing);
            catalog->schemata().insert(schema);
            createdObjects.insert(schema);
          }
        } else {
          catalog->schemata().insert(schema);
          createdObjects.insert(schema);
        }

        break;
      }

      case QtUse: {
        walker.next(); // Skip USE.
        Identifier identifier = getIdentifier(walker);
        currentSchema = ensureSchemaExists(catalog, identifier.second, caseSensitive);
        break;
      }

      case QtCreateEvent: {
        db_mysql_EventRef event(grt::Initialized);
        event->sqlDefinition(base::trim(recognizer->text()));
        event->createDate(base::fmttime(0, DATETIME_FMT));
        event->lastChangeDate(event->createDate());

        std::pair<std::string, bool> result = fillEventDetails(walker, event);
        db_SchemaRef schema = currentSchema;
        if (!result.first.empty() && !base::same_string(schema->name(), result.first, false))
          schema = ensureSchemaExists(catalog, result.first, false);
        event->owner(schema);

        db_EventRef existing = find_named_object_in_list(schema->events(), event->name());
        if (existing.is_valid()) {
          if (!result.second) // Ignore if exists?
          {
            schema->events()->remove(existing);
            schema->events().insert(event);
            createdObjects.insert(event);
          }
        } else {
          schema->events().insert(event);
          createdObjects.insert(event);
        }

        break;
      }

      case QtCreateView: {
        db_mysql_ViewRef view(grt::Initialized);
        view->sqlDefinition(base::trim(recognizer->text()));
        view->createDate(base::fmttime(0, DATETIME_FMT));
        view->lastChangeDate(view->createDate());

        std::pair<std::string, bool> result = fillViewDetails(walker, view);
        db_mysql_SchemaRef schema = currentSchema;
        if (!result.first.empty() && !base::same_string(schema->name(), result.first, caseSensitive))
          schema = ensureSchemaExists(catalog, result.first, caseSensitive);
        view->owner(schema);

        // Ignore views that use a name that is already used for a table (no drop/new-add takes place then).
        db_mysql_TableRef existingTable = find_named_object_in_list(schema->tables(), view->name());
        if (!existingTable.is_valid()) {
          db_mysql_ViewRef existingView = find_named_object_in_list(schema->views(), view->name());
          if (existingView.is_valid()) {
            if (!result.second) // Ignore if exists?
            {
              schema->views()->remove(existingView);
              schema->views().insert(view);
              createdObjects.insert(view);
            }
          } else {
            schema->views().insert(view);
            createdObjects.insert(view);
          }
        }

        break;
      }

      case QtCreateProcedure:
      case QtCreateFunction:
      case QtCreateUdf: {
        db_mysql_RoutineRef routine(grt::Initialized);
        routine->sqlDefinition(base::trim(recognizer->text()));
        routine->createDate(base::fmttime(0, DATETIME_FMT));
        routine->lastChangeDate(routine->createDate());

        std::string schemaName = fillRoutineDetails(walker, routine);
        db_SchemaRef schema = currentSchema;
        if (!schemaName.empty() && !base::same_string(schema->name(), schemaName, false))
          schema = ensureSchemaExists(catalog, schemaName, caseSensitive);
        routine->owner(schema);

        db_RoutineRef existing = find_named_object_in_list(schema->routines(), routine->name());
        if (existing.is_valid())
          schema->routines()->remove(existing);
        schema->routines().insert(routine);
        createdObjects.insert(routine);

        break;
      }

      case QtCreateTrigger: {
        db_mysql_TriggerRef trigger(grt::Initialized);
        trigger->sqlDefinition(base::trim(recognizer->text()));
        trigger->createDate(base::fmttime(0, DATETIME_FMT));
        trigger->lastChangeDate(trigger->createDate());

        std::pair<std::string, std::string> tableName = fillTriggerDetails(walker, trigger);

        // Trigger table referencing is a bit different than for other objects because we need
        // the table now to add the trigger to it. We cannot defer that to the resolveReferences() call.
        // This has the implication that we can only work with tables we have found so far.
        db_SchemaRef schema = currentSchema;
        if (!tableName.first.empty())
          schema = ensureSchemaExists(catalog, tableName.first, caseSensitive);
        db_TableRef table = find_named_object_in_list(schema->tables(), tableName.second, caseSensitive);
        if (!table.is_valid()) {
          // If we don't find a table with the given name we create a stub object to be used instead.
          table = db_mysql_TableRef(grt::Initialized);
          table->owner(schema);
          table->isStub(1);
          table->name(tableName.second);
          table->oldName(tableName.second);
          schema->tables().insert(table);
          createdObjects.insert(table);
        }

        trigger->owner(table);

        db_TriggerRef existing = find_named_object_in_list(table->triggers(), trigger->name());
        if (existing.is_valid())
          table->triggers()->remove(existing);
        table->triggers().insert(trigger);
        createdObjects.insert(trigger);

        break;
      }

      case QtCreateLogFileGroup: {
        db_mysql_LogFileGroupRef group(grt::Initialized);
        group->createDate(base::fmttime(0, DATETIME_FMT));
        group->lastChangeDate(group->createDate());

        fillLogfileGroupDetails(walker, group);
        group->owner(catalog);

        db_LogFileGroupRef existing = find_named_object_in_list(catalog->logFileGroups(), group->name());
        if (existing.is_valid())
          catalog->logFileGroups()->remove(existing);
        catalog->logFileGroups().insert(group);
        createdObjects.insert(group);

        break;
      }

      case QtCreateServer: {
        db_mysql_ServerLinkRef server(grt::Initialized);
        server->createDate(base::fmttime(0, DATETIME_FMT));
        server->lastChangeDate(server->createDate());

        fillServerDetails(walker, server);
        server->owner(catalog);

        db_ServerLinkRef existing = find_named_object_in_list(catalog->serverLinks(), server->name());
        if (existing.is_valid())
          catalog->serverLinks()->remove(existing);
        catalog->serverLinks().insert(server);
        createdObjects.insert(server);

        break;
      }

      case QtCreateTableSpace: {
        db_mysql_TablespaceRef tablespace(grt::Initialized);
        tablespace->createDate(base::fmttime(0, DATETIME_FMT));
        tablespace->lastChangeDate(tablespace->createDate());

        fillTablespaceDetails(walker, catalog, tablespace);
        tablespace->owner(catalog);

        db_TablespaceRef existing = find_named_object_in_list(catalog->tablespaces(), tablespace->name());
        if (existing.is_valid())
          catalog->tablespaces()->remove(existing);
        catalog->tablespaces().insert(tablespace);
        createdObjects.insert(tablespace);

        break;
      }

      case QtDropDatabase: {
        walker.next(2);              // DROP DATABASE.
        walker.skipIf(IF_SYMBOL, 2); // IF EXISTS.
        Identifier identifier = getIdentifier(walker);
        db_SchemaRef schema = find_named_object_in_list(catalog->schemata(), identifier.second);
        if (schema.is_valid()) {
          catalog->schemata()->remove(schema);
          if (catalog->defaultSchema() == schema)
            catalog->defaultSchema(db_mysql_SchemaRef());
          if (currentSchema == schema)
            currentSchema = db_mysql_SchemaRef::cast_from(catalog->defaultSchema());
          if (!currentSchema.is_valid())
            currentSchema = ensureSchemaExists(catalog, "default_schema", caseSensitive);
        }
        break;
      }

      case QtDropEvent: {
        walker.next(2);
        walker.skipIf(IF_SYMBOL, 2);
        Identifier identifier = getIdentifier(walker);
        db_SchemaRef schema = currentSchema;
        if (!identifier.first.empty())
          schema = ensureSchemaExists(catalog, identifier.first, caseSensitive);
        db_EventRef event = find_named_object_in_list(schema->events(), identifier.second);
        schema->events()->remove(event);
        break;
      }

      case QtDropProcedure:
      case QtDropFunction: // Including UDFs.
      {
        walker.next(2);
        walker.skipIf(IF_SYMBOL, 2);
        Identifier identifier = getIdentifier(walker);
        db_SchemaRef schema = currentSchema;
        if (!identifier.first.empty())
          schema = ensureSchemaExists(catalog, identifier.first, caseSensitive);
        db_RoutineRef routine = find_named_object_in_list(schema->routines(), identifier.second);
        schema->routines()->remove(routine);
        break;
      }

      case QtDropIndex: {
        walker.next();
        if (walker.is(ONLINE_SYMBOL) || walker.is(OFFLINE_SYMBOL))
          walker.next();
        walker.next();
        std::string name = getIdentifier(walker).second;
        walker.next(); // Skip ON.

        Identifier reference = getIdentifier(walker);
        db_SchemaRef schema = currentSchema;
        if (!reference.first.empty())
          schema = ensureSchemaExists(catalog, reference.first, caseSensitive);
        db_TableRef table = find_named_object_in_list(schema->tables(), reference.second);
        if (table.is_valid()) {
          db_IndexRef index = find_named_object_in_list(table->indices(), name);
          if (index.is_valid())
            table->indices()->remove(index);
        }
        break;
      }

      case QtDropLogfileGroup: {
        walker.next(3); // Skip DROP LOGFILE GROUP.
        Identifier identifier = getIdentifier(walker);
        db_LogFileGroupRef group = find_named_object_in_list(catalog->logFileGroups(), identifier.second);
        if (group.is_valid())
          catalog->logFileGroups()->remove(group);

        break;
      }

      case QtDropServer: {
        walker.next(2);              // Skip DROP SERVER.
        walker.skipIf(IF_SYMBOL, 2); // Skip IF EXISTS.
        Identifier identifier = getIdentifier(walker);
        db_ServerLinkRef server = find_named_object_in_list(catalog->serverLinks(), identifier.second);
        if (server.is_valid())
          catalog->serverLinks()->remove(server);

        break;
      }

      case QtDropTable:
      case QtDropView: {
        bool isView = queryType == QtDropView;
        walker.next();
        walker.skipIf(TEMPORARY_SYMBOL);
        walker.next();               // Skip TABLE | TABLES | VIEW.
        walker.skipIf(IF_SYMBOL, 2); // Skip IF EXISTS.

        // We can have a list of tables to drop here.
        while (true) {
          Identifier identifier = getIdentifier(walker);
          db_SchemaRef schema = currentSchema;
          if (!identifier.first.empty())
            schema = ensureSchemaExists(catalog, identifier.first, caseSensitive);
          if (isView) {
            db_ViewRef view = find_named_object_in_list(schema->views(), identifier.second);
            if (view.is_valid())
              schema->views()->remove(view);
          } else {
            db_TableRef table = find_named_object_in_list(schema->tables(), identifier.second);
            if (table.is_valid())
              schema->tables()->remove(table);
          }
          if (walker.tokenType() != COMMA_SYMBOL)
            break;
          walker.next();
        }

        break;
      }

      case QtDropTablespace: {
        walker.next(2);
        Identifier identifier = getIdentifier(walker);
        db_TablespaceRef tablespace = find_named_object_in_list(catalog->tablespaces(), identifier.second);
        if (tablespace.is_valid())
          catalog->tablespaces()->remove(tablespace);

        break;
      }

      case QtDropTrigger: {
        walker.next(2);
        walker.skipIf(IF_SYMBOL, 2);
        Identifier identifier = getIdentifier(walker);

        // Even though triggers are schema level objects they work on specific tables
        // and that's why we store them under the affected tables, not in the schema object.
        // This however makes it more difficult to find the trigger to delete, as we have to
        // iterate over all tables.
        db_SchemaRef schema = currentSchema;
        if (!identifier.first.empty())
          schema = ensureSchemaExists(catalog, identifier.first, caseSensitive);
        for (grt::ListRef<db_Table>::const_iterator table = schema->tables().begin(); table != schema->tables().end();
             ++table) {
          db_TriggerRef trigger = find_named_object_in_list((*table)->triggers(), identifier.second);
          if (trigger.is_valid()) {
            (*table)->triggers()->remove(trigger);
            break; // A trigger can only be assigned to a single table, so we can stop here.
          }
        }
        break;
      }

      case QtRenameTable: {
        // Renaming a table is special as you can use it also to rename a view and to move
        // a table from one schema to the other (not for views, though).
        // Due to the way we store triggers we have an easy life wrt. related triggers.
        walker.next(2);

        while (true) {
          // Unlimited value pairs.
          Identifier source = getIdentifier(walker);
          db_SchemaRef sourceSchema = currentSchema;
          if (!source.first.empty())
            sourceSchema = ensureSchemaExists(catalog, source.first, caseSensitive);

          walker.next(); // Skip TO.
          Identifier target = getIdentifier(walker);
          db_SchemaRef targetSchema = currentSchema;
          if (!target.first.empty())
            targetSchema = ensureSchemaExists(catalog, target.first, caseSensitive);

          db_ViewRef view = find_named_object_in_list(sourceSchema->views(), source.second);
          if (view.is_valid()) {
            // Cannot move between schemas.
            if (sourceSchema == targetSchema)
              view->name(target.second);
          } else {
            // Renaming a table.
            db_TableRef table = find_named_object_in_list(sourceSchema->tables(), source.second);
            if (table.is_valid()) {
              if (sourceSchema != targetSchema) {
                sourceSchema->tables()->remove(table);
                targetSchema->tables().insert(table);
                createdObjects.insert(table);
              }
              table->name(target.second);
            }
          }

          if (walker.tokenType() != COMMA_SYMBOL)
            break;
          walker.next();
        }

        break;
      }

      // Alter commands. At the moment we only support a limited number of cases as we mostly
      // need SQL-to-GRT conversion for create scripts.
      case QtAlterDatabase: {
        db_mysql_SchemaRef schema = currentSchema;
        walker.next(); // Skip DATABASE.
        if (walker.isIdentifier()) {
          Identifier identifier = getIdentifier(walker);
          schema = ensureSchemaExists(catalog, identifier.second, caseSensitive);
        }
        schema->lastChangeDate(base::fmttime(0, DATETIME_FMT));
        fillSchemaOptions(walker, catalog, schema);

        break;
      }

      case QtAlterLogFileGroup:
        break;

      case QtAlterFunction:
        break;

      case QtAlterProcedure:
        break;

      case QtAlterServer:
        break;

      case QtAlterTable: // Alter table only for adding/removing indices and for renames.
      {                  // This is more than the old parser did.
        walker.next();
        if (walker.is(ONLINE_SYMBOL) || walker.is(OFFLINE_SYMBOL))
          walker.next();
        walker.skipIf(IGNORE_SYMBOL);
        walker.next(); // Skip TABLE.
        Identifier identifier = getIdentifier(walker);
        db_mysql_SchemaRef schema = currentSchema;
        if (!identifier.first.empty())
          schema = ensureSchemaExists(catalog, identifier.first, caseSensitive);

        db_mysql_TableRef table = find_named_object_in_list(schema->tables(), identifier.second, caseSensitive);
        if (table.is_valid()) {
          // There must be at least one alter item.
          while (true) {
            switch (walker.lookAhead(true)) {
              case ADD_SYMBOL:
                walker.next();
                switch (walker.lookAhead(true)) {
                  case CONSTRAINT_SYMBOL:
                  case PRIMARY_SYMBOL:
                  case FOREIGN_SYMBOL:
                  case UNIQUE_SYMBOL:
                  case INDEX_SYMBOL:
                  case KEY_SYMBOL:
                  case FULLTEXT_SYMBOL:
                  case SPATIAL_SYMBOL:
                    walker.next();
                    processTableKeyItem(walker, catalog, schema->name(), table, autoGenerateFkNames, refCache);
                    break;

                  case RENAME_SYMBOL: {
                    walker.next(2);
                    if (walker.is(TO_SYMBOL) || walker.is(AS_SYMBOL))
                      walker.next();

                    identifier = getIdentifier(walker);
                    db_SchemaRef targetSchema = currentSchema;
                    if (!identifier.first.empty())
                      targetSchema = ensureSchemaExists(catalog, identifier.first, caseSensitive);

                    db_ViewRef view = find_named_object_in_list(schema->views(), identifier.second);
                    if (view.is_valid()) {
                      // Cannot move between schemas.
                      if (schema == targetSchema)
                        view->name(identifier.second);
                    } else {
                      // Renaming a table.
                      db_TableRef table = find_named_object_in_list(schema->tables(), identifier.second);
                      if (table.is_valid()) {
                        if (schema != targetSchema) {
                          schema->tables()->remove(table);
                          targetSchema->tables().insert(table);
                        }
                        table->name(identifier.second);
                      }
                    }

                    break;
                  }

                  default:
                    walker.up();
                    walker.skipSubtree();
                }
                break;

              default:
                walker.skipSubtree();
            }

            if (!walker.is(COMMA_SYMBOL))
              break;
            walker.next();
          }
        }
        break;
      }

      case QtAlterTableSpace:
        break;
      case QtAlterEvent:
        break;
      case QtAlterView:
        break;
      default:
        continue; // Ignore anything else.
    }
  }

  resolveReferences(catalog, refCache, context->case_sensitive());

  // Remove the default_schema we may have created at the start, if it is empty.
  if (defaultSchemaCreated) {
    currentSchema = ensureSchemaExists(catalog, "default_schema", caseSensitive);
    if (currentSchema->tables().count() == 0 && currentSchema->views().count() == 0 &&
        currentSchema->routines().count() == 0 && currentSchema->synonyms().count() == 0 &&
        currentSchema->sequences().count() == 0 && currentSchema->events().count() == 0)
      catalog->schemata().remove_value(currentSchema);
  }

  return errorCount;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::doSyntaxCheck(parser_ContextReferenceRef context_ref, const std::string &sql,
                                              const std::string &type) {
  parser::MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  MySQLParseUnit query_type = MySQLParseUnit::PuGeneric;
  if (type == "view")
    query_type = MySQLParseUnit::PuCreateView;
  else if (type == "routine")
    query_type = MySQLParseUnit::PuCreateRoutine;
  else if (type == "trigger")
    query_type = MySQLParseUnit::PuCreateTrigger;
  else if (type == "event")
    query_type = MySQLParseUnit::PuCreateEvent;

  return checkSqlSyntax(context, sql.c_str(), sql.size(), query_type);
}

//--------------------------------------------------------------------------------------------------

/**
* Parses the given text as a specific query type (see parser for supported types).
* Returns the error count.
*/
size_t MySQLParserServicesImpl::checkSqlSyntax(parser::MySQLParserContext::Ref context, const char *sql, size_t length,
                                               MySQLParseUnit type) {
  context->syntax_checker()->parse(sql, length, true, type);
  return context->syntax_checker()->error_info().size();
}

//--------------------------------------------------------------------------------------------------

/**
* Helper to collect text positions to references of the given schema.
* We only come here if there was no syntax error.
*/
void collectSchemaNameOffsets(parser::MySQLParserContext::Ref context, std::list<size_t> &offsets,
                              const std::string schema_name) {
  // Don't try to optimize the creation of the walker. There must be a new instance for each parse run
  // as it stores references to results in the parser.
  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  bool case_sensitive = context->case_sensitive();
  while (walker.next()) {
    switch (walker.tokenType()) {
      case SCHEMA_NAME_TOKEN:
      case SCHEMA_REF_TOKEN:
        if (base::same_string(walker.tokenText(), schema_name, case_sensitive)) {
          size_t pos = walker.tokenOffset();
          if (walker.is(BACK_TICK_QUOTED_ID) || walker.is(SINGLE_QUOTED_TEXT))
            ++pos;
          offsets.push_back(pos);
        }
        break;

      case TABLE_NAME_TOKEN:
      case TABLE_REF_TOKEN: {
        walker.next();
        if (walker.isIdentifier() &&
            (walker.lookAhead(false) == DOT_SYMBOL || walker.lookAhead(false) == DOT_IDENTIFIER)) {
          // A table ref not with leading dot but a qualified identifier.
          if (base::same_string(walker.tokenText(), schema_name, case_sensitive)) {
            size_t pos = walker.tokenOffset();
            if (walker.is(BACK_TICK_QUOTED_ID) || walker.is(SINGLE_QUOTED_TEXT))
              ++pos;
            offsets.push_back(pos);
          }
        }
        break;
      }

      case COLUMN_REF_TOKEN: // Field and key names (id, .id, id.id, id.id.id).
        walker.next();
        // No leading dot (which means no schema/table) and at least one dot after the first id.
        if (walker.isIdentifier() &&
            (walker.lookAhead(false) == DOT_SYMBOL || walker.lookAhead(false) == DOT_IDENTIFIER)) {
          std::string name = walker.tokenText();
          size_t pos = walker.tokenOffset();
          if (walker.is(BACK_TICK_QUOTED_ID) || walker.is(SINGLE_QUOTED_TEXT))
            ++pos;

          walker.next();
          walker.skipIf(DOT_SYMBOL);
          walker.next();

          if (walker.lookAhead(false) == DOT_SYMBOL || walker.lookAhead(false) == DOT_IDENTIFIER) // Fully qualified.
          {
            if (base::same_string(name, schema_name, case_sensitive))
              offsets.push_back(pos);
          }
        }
        break;

      // All those can have schema.id or only id.
      case VIEW_REF_TOKEN:
      case VIEW_NAME_TOKEN:
      case TRIGGER_REF_TOKEN:
      case TRIGGER_NAME_TOKEN:
      case PROCEDURE_REF_TOKEN:
      case PROCEDURE_NAME_TOKEN:
      case FUNCTION_REF_TOKEN:
      case FUNCTION_NAME_TOKEN:
        walker.next();
        if (walker.lookAhead(false) == DOT_SYMBOL || walker.lookAhead(false) == DOT_IDENTIFIER) {
          if (base::same_string(walker.tokenText(), schema_name, case_sensitive)) {
            size_t pos = walker.tokenOffset();
            if (walker.is(BACK_TICK_QUOTED_ID) || walker.is(SINGLE_QUOTED_TEXT))
              ++pos;
            offsets.push_back(pos);
          }
        }
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
* Replace all occurrences of the old by the new name according to the offsets list.
*/
void replace_schema_names(std::string &sql, const std::list<size_t> &offsets, size_t length,
                          const std::string new_name) {
  bool remove_schema = new_name.empty();
  for (std::list<size_t>::const_reverse_iterator iterator = offsets.rbegin(); iterator != offsets.rend(); ++iterator) {
    std::string::size_type start = *iterator;
    std::string::size_type replace_length = length;
    if (remove_schema) {
      // Make sure we also remove quotes and the dot.
      if (start > 0 && (sql[start - 1] == '`' || sql[start - 1] == '"')) {
        --start;
        ++replace_length;
      }
      ++replace_length;
    }
    sql.replace(start, replace_length, new_name);
  }
}

//--------------------------------------------------------------------------------------------------

void rename_in_list(grt::ListRef<db_DatabaseDdlObject> list, parser::MySQLParserContext::Ref context,
                    MySQLParseUnit unit, const std::string old_name, const std::string new_name) {
  for (size_t i = 0; i < list.count(); ++i) {
    std::string sql = list[i]->sqlDefinition();
    context->recognizer()->parse(sql.c_str(), sql.size(), true, unit);
    size_t error_count = context->recognizer()->error_info().size();
    if (error_count == 0) {
      std::list<size_t> offsets;
      collectSchemaNameOffsets(context, offsets, old_name);
      if (!offsets.empty()) {
        replace_schema_names(sql, offsets, old_name.size(), new_name);
        list[i]->sqlDefinition(sql);
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::doSchemaRefRename(parser_ContextReferenceRef context_ref, db_mysql_CatalogRef catalog,
                                                  const std::string old_name, const std::string new_name) {
  parser::MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return renameSchemaReferences(context, catalog, old_name, new_name);
}

//--------------------------------------------------------------------------------------------------

/**
* Goes through all schemas in the catalog and changes all db objects to refer to the new name if they
* currently refer to the old name. We also iterate non-related schemas in order to have some
* consolidation/sanitizing in effect where wrong schema references were used.
*/
size_t MySQLParserServicesImpl::renameSchemaReferences(parser::MySQLParserContext::Ref context,
                                                       db_mysql_CatalogRef catalog, const std::string old_name,
                                                       const std::string new_name) {
  logDebug("Rename schema references\n");

  ListRef<db_mysql_Schema> schemas = catalog->schemata();
  for (size_t i = 0; i < schemas.count(); ++i) {
    db_mysql_SchemaRef schema = schemas[i];
    rename_in_list(schema->views(), context, MySQLParseUnit::PuCreateView, old_name, new_name);
    rename_in_list(schema->routines(), context, MySQLParseUnit::PuCreateRoutine, old_name, new_name);

    grt::ListRef<db_mysql_Table> tables = schemas[i]->tables();
    for (grt::ListRef<db_mysql_Table>::const_iterator iterator = tables.begin(); iterator != tables.end(); ++iterator)
      rename_in_list((*iterator)->triggers(), context, MySQLParseUnit::PuCreateTrigger, old_name, new_name);
  }

  return 0;
}

//--------------------------------------------------------------------------------------------------

static const unsigned char *skip_leading_whitespace(const unsigned char *head, const unsigned char *tail) {
  while (head < tail && *head <= ' ')
    head++;
  return head;
}

//--------------------------------------------------------------------------------------------------

bool is_line_break(const unsigned char *head, const unsigned char *line_break) {
  if (*line_break == '\0')
    return false;

  while (*head != '\0' && *line_break != '\0' && *head == *line_break) {
    head++;
    line_break++;
  }
  return *line_break == '\0';
}

//--------------------------------------------------------------------------------------------------

grt::BaseListRef MySQLParserServicesImpl::getSqlStatementRanges(const std::string &sql) {
  grt::BaseListRef list(true);
  std::vector<std::pair<size_t, size_t> > ranges;

  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges);

  for (std::vector<std::pair<size_t, size_t> >::const_iterator i = ranges.begin(); i != ranges.end(); ++i) {
    grt::BaseListRef item(true);
    item.ginsert(grt::IntegerRef(i->first));
    item.ginsert(grt::IntegerRef(i->second));
    list.ginsert(item);
  }
  return list;
}

//--------------------------------------------------------------------------------------------------

/**
* A statement splitter to take a list of sql statements and split them into individual statements,
* return their position and length in the original string (instead the copied strings).
*/
size_t MySQLParserServicesImpl::determineStatementRanges(const char *sql, size_t length,
                                                         const std::string &initial_delimiter,
                                                         std::vector<std::pair<size_t, size_t> > &ranges,
                                                         const std::string &line_break) {
  _stop = false;
  std::string delimiter = initial_delimiter.empty() ? ";" : initial_delimiter;
  const unsigned char *delimiter_head = (unsigned char *)delimiter.c_str();

  const unsigned char keyword[] = "delimiter";

  const unsigned char *head = (unsigned char *)sql;
  const unsigned char *tail = head;
  const unsigned char *end = head + length;
  const unsigned char *new_line = (unsigned char *)line_break.c_str();
  bool have_content = false; // Set when anything else but comments were found for the current statement.

  while (!_stop && tail < end) {
    switch (*tail) {
      case '/': // Possible multi line comment or hidden (conditional) command.
        if (*(tail + 1) == '*') {
          tail += 2;
          bool is_hidden_command = (*tail == '!');
          while (true) {
            while (tail < end && *tail != '*')
              tail++;
            if (tail == end) // Unfinished comment.
              break;
            else {
              if (*++tail == '/') {
                tail++; // Skip the slash too.
                break;
              }
            }
          }

          if (!is_hidden_command && !have_content)
            head = tail; // Skip over the comment.
        } else
          tail++;

        break;

      case '-': // Possible single line comment.
      {
        const unsigned char *end_char = tail + 2;
        if (*(tail + 1) == '-' && (*end_char == ' ' || *end_char == '\t' || is_line_break(end_char, new_line))) {
          // Skip everything until the end of the line.
          tail += 2;
          while (tail < end && !is_line_break(tail, new_line))
            tail++;
          if (!have_content)
            head = tail;
        } else
          tail++;

        break;
      }

      case '#': // MySQL single line comment.
        while (tail < end && !is_line_break(tail, new_line))
          tail++;
        if (!have_content)
          head = tail;
        break;

      case '"':
      case '\'':
      case '`': // Quoted string/id. Skip this in a local loop.
      {
        have_content = true;
        char quote = *tail++;
        while (tail < end && *tail != quote) {
          // Skip any escaped character too.
          if (*tail == '\\')
            tail++;
          tail++;
        }
        if (*tail == quote)
          tail++; // Skip trailing quote char to if one was there.

        break;
      }

      case 'd':
      case 'D': {
        have_content = true;

        // Possible start of the keyword DELIMITER. Must be at the start of the text or a character,
        // which is not part of a regular MySQL identifier (0-9, A-Z, a-z, _, $, \u0080-\uffff).
        unsigned char previous = tail > (unsigned char *)sql ? *(tail - 1) : 0;
        bool is_identifier_char = previous >= 0x80 || (previous >= '0' && previous <= '9') ||
                                  ((previous | 0x20) >= 'a' && (previous | 0x20) <= 'z') || previous == '$' ||
                                  previous == '_';
        if (tail == (unsigned char *)sql || !is_identifier_char) {
          const unsigned char *run = tail + 1;
          const unsigned char *kw = keyword + 1;
          int count = 9;
          while (count-- > 1 && (*run++ | 0x20) == *kw++)
            ;
          if (count == 0 && *run == ' ') {
            // Delimiter keyword found. Get the new delimiter (everything until the end of the line).
            tail = run++;
            while (run < end && !is_line_break(run, new_line))
              run++;
            delimiter = base::trim(std::string((char *)tail, run - tail));
            delimiter_head = (unsigned char *)delimiter.c_str();

            // Skip over the delimiter statement and any following line breaks.
            while (is_line_break(run, new_line))
              run++;
            tail = run;
            head = tail;
          } else
            tail++;
        } else
          tail++;

        break;
      }

      default:
        if (*tail > ' ')
          have_content = true;
        tail++;
        break;
    }

    if (*tail == *delimiter_head) {
      // Found possible start of the delimiter. Check if it really is.
      size_t count = delimiter.size();
      if (count == 1) {
        // Most common case. Trim the statement and check if it is not empty before adding the range.
        head = skip_leading_whitespace(head, tail);
        if (head < tail)
          ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));
        head = ++tail;
        have_content = false;
      } else {
        const unsigned char *run = tail + 1;
        const unsigned char *del = delimiter_head + 1;
        while (count-- > 1 && (*run++ == *del++))
          ;

        if (count == 0) {
          // Multi char delimiter is complete. Tail still points to the start of the delimiter.
          // Run points to the first character after the delimiter.
          head = skip_leading_whitespace(head, tail);
          if (head < tail)
            ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));
          tail = run;
          head = run;
          have_content = false;
        }
      }
    }
  }

  // Add remaining text to the range list.
  head = skip_leading_whitespace(head, tail);
  if (head < tail)
    ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));

  return 0;
}

//--------------------------------------------------------------------------------------------------

/*
 * Provides a dictionary with the definition of a user.
 * The walker must be at the starting point of a user definition.
 * returned dictionary:
 *                      user : the username or CURRENT_USER
 *           (optional) host : the host name if available
 *           (optional) id_method : the authentication method if available
 *           (optional) id_password: the authentication string if available
 */
grt::DictRef parseUserDefinition(MySQLRecognizerTreeWalker &walker) {
  grt::DictRef result(true);

  result.gset("user", walker.tokenText());

  if (walker.is(CURRENT_USER_SYMBOL) && walker.lookAhead(false) == OPEN_PAR_SYMBOL)
    walker.next(3);
  else
    walker.next();

  if (walker.is(COMMA_SYMBOL) || walker.is(EOF))
    return result; // Most simple case. No further options.

  if (walker.is(AT_SIGN_SYMBOL) || walker.is(AT_TEXT_SUFFIX)) {
    std::string host;
    if (walker.skipIf(AT_SIGN_SYMBOL))
      host = walker.tokenText();
    else
      host = walker.tokenText().substr(1); // Skip leading @.
    result.gset("host", host);
    walker.next();
  }

  if (walker.is(IDENTIFIED_SYMBOL)) {
    walker.next();
    if (walker.is(BY_SYMBOL)) {
      walker.next();
      walker.skipIf(PASSWORD_SYMBOL);

      result.gset("id_method", "PASSWORD");
      result.gset("id_string", walker.tokenText());
      walker.next();
    } else // Must be WITH then.
    {
      walker.next();
      result.gset("id_method", walker.tokenText());
      walker.next();

      if (walker.is(AS_SYMBOL) || walker.is(BY_SYMBOL)) {
        walker.next();
        result.gset("id_string", walker.tokenText());
        walker.next();
      }
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

grt::DictRef collectGrantDetails(MySQLRecognizer *recognizer) {
  grt::DictRef data = grt::DictRef(true);

  MySQLRecognizerTreeWalker walker = recognizer->tree_walker();

  walker.next(); // Skip GRANT.

  // Collect all privileges.
  grt::StringListRef list = grt::StringListRef(grt::Initialized);
  while (true) {
    std::string privileges = walker.tokenText();
    walker.next();
    if (walker.is(OPEN_PAR_SYMBOL)) {
      privileges += " (";
      walker.next();
      std::string list;
      while (true) {
        if (!list.empty())
          list += ", ";
        list += walker.tokenText();
        walker.next();
        if (!walker.is(COMMA_SYMBOL))
          break;
        walker.next();
      }

      privileges += list + ")";
      walker.next();
    } else {
      // Just a list of identifiers or certain keywords not allowed as identifiers.
      while (walker.isIdentifier() || walker.is(OPTION_SYMBOL) || walker.is(DATABASES_SYMBOL)) {
        privileges += " " + walker.tokenText();
        walker.next();
      }
    }

    list.insert(privileges);
    if (!walker.is(COMMA_SYMBOL))
      break;
    walker.next();
  }

  data.set("privileges", list);

  // The subtree for the target details includes the ON keyword at the beginning, which we have to remove.
  data.gset("target", base::trim(walker.textForTree().substr(2)));
  walker.skipSubtree();
  walker.next(); // Skip TO.

  // Now the user definitions.
  grt::DictRef users = grt::DictRef(true);
  data.set("users", users);

  while (true) {
    grt::DictRef user = parseUserDefinition(walker);
    users.set(user.get_string("user"), user);
    if (!walker.is(COMMA_SYMBOL))
      break;
    walker.next();
  }

  if (walker.is(REQUIRE_SYMBOL)) {
    grt::DictRef requirements(true);
    walker.next();
    switch (walker.tokenType()) {
      case SSL_SYMBOL:
      case X509_SYMBOL:
      case NONE_SYMBOL:
        requirements.gset(walker.tokenText(), "");
        walker.next();
        break;

      default:
        while (walker.is(CIPHER_SYMBOL) || walker.is(ISSUER_SYMBOL) || walker.is(SUBJECT_SYMBOL)) {
          std::string option = walker.tokenText();
          walker.next();
          requirements.gset(option, walker.tokenText());
          walker.next();
          walker.skipIf(AND_SYMBOL);
        }
        break;
    }
    data.set("requirements", requirements);
  }

  if (walker.is(WITH_SYMBOL)) {
    grt::DictRef options(true);
    walker.next();
    bool done = false;
    while (!done) {
      switch (walker.tokenType()) {
        case GRANT_SYMBOL:
          options.gset("grant", "");
          walker.next(2);
          break;

        case MAX_QUERIES_PER_HOUR_SYMBOL:
        case MAX_UPDATES_PER_HOUR_SYMBOL:
        case MAX_CONNECTIONS_PER_HOUR_SYMBOL:
        case MAX_USER_CONNECTIONS_SYMBOL: {
          std::string option = walker.tokenText();
          walker.next();
          options.gset(option, walker.tokenText());
          walker.next();
          break;
        }

        default:
          done = true;
          break;
      }
    }

    data.set("options", options);
  }

  return data;
}

//--------------------------------------------------------------------------------------------------

grt::DictRef MySQLParserServicesImpl::parseStatementDetails(parser_ContextReferenceRef context_ref,
                                                            const std::string &sql) {
  parser::MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseStatement(context, sql);
}

//--------------------------------------------------------------------------------------------------

grt::DictRef MySQLParserServicesImpl::parseStatement(parser::MySQLParserContext::Ref context, const std::string &sql) {
  // This part can potentially grow very large because of the sheer amount of possible query types.
  // So it should be moved into an own file if it grows beyond a few 100 lines.
  MySQLRecognizer *recognizer = context->recognizer();
  recognizer->parse(sql.c_str(), sql.size(), true, MySQLParseUnit::PuGeneric);
  if (recognizer->has_errors()) {
    // Return the error message in case of syntax errors.
    grt::DictRef result(true);
    result.gset("error", recognizer->error_info()[0].message);
    return result;
  }

  std::shared_ptr<MySQLQueryIdentifier> queryIdentifier = context->createQueryIdentifier();
  MySQLQueryType queryType = queryIdentifier->getQueryType(sql.c_str(), sql.size(), true);

  switch (queryType) {
    case QtGrant:
    case QtGrantProxy:
      return collectGrantDetails(recognizer);

    default: {
      grt::DictRef result(true);
      result.gset("error", "Unsupported query type (" + std::to_string(queryType) + ")");
      return result;
    }
  }
}

//--------------------------------------------------------------------------------------------------

std::string MySQLParserServicesImpl::replaceTokenSequence(parser_ContextReferenceRef context_ref,
                                                          const std::string &sql, size_t start_token, size_t count,
                                                          grt::StringListRef replacements) {
  parser::MySQLParserContext::Ref context = parser_context_from_grt(context_ref);

  std::vector<std::string> list;
  list.reserve(replacements->count());
  std::copy(replacements.begin(), replacements.end(), std::back_inserter(list));
  return replaceTokenSequenceWithText(context, sql, start_token, count, list);
}

//--------------------------------------------------------------------------------------------------

std::string MySQLParserServicesImpl::replaceTokenSequenceWithText(parser::MySQLParserContext::Ref context,
                                                                  const std::string &sql, size_t start_token,
                                                                  size_t count,
                                                                  const std::vector<std::string> replacements) {
  std::string result;
  context->recognizer()->parse(sql.c_str(), sql.size(), true, MySQLParseUnit::PuGeneric);
  size_t error_count = context->recognizer()->error_info().size();
  if (error_count > 0)
    return "";

  MySQLRecognizerTreeWalker walker = context->recognizer()->tree_walker();
  if (!walker.advanceToType((unsigned)start_token, true))
    return sql;

  // Get the index of each token in the input stream and use that in the input lexer to find
  // tokens to replace. Don't touch any other (including whitespaces).
  // The given start_token can only be a visible token.

  // First find the range of the text before the start token and copy that unchanged.
  ANTLR3_MARKER current_index = walker.tokenIndex();
  if (current_index > 0) {
    ParserToken token = context->recognizer()->token_at_index(current_index - 1);
    result = sql.substr(0, token.stop - sql.c_str() + 1);
  }

  // Next replace all tokens we have replacements for. Remember tokens are separated by hidden tokens
  // which must be added to the result unchanged.
  size_t c = std::min(count, replacements.size());
  size_t i = 0;
  for (; i < c; ++i) {
    ++current_index; // Ignore the token.
    result += replacements[i];

    // Append the following separator token.
    ParserToken token = context->recognizer()->token_at_index(current_index++);
    if (token.type == ANTLR3_TOKEN_INVALID)
      return result; // Premature end. Too many values given to replace.
    result += token.text;

    --count;
  }

  if (i < replacements.size()) {
    // Something left to add before continuing with the rest of the SQL.
    // In order to separate replacements properly they must include necessary whitespaces.
    // We don't add any here.
    while (i < replacements.size())
      result += replacements[i++];
  }

  if (count > 0) {
    // Some tokens left without replacement. Skip them and add the rest of the query unchanged.
    // Can be a large number to indicate the removal of anything left.
    current_index += count;
  }

  // Finally add the remaining text.
  ParserToken token = context->recognizer()->token_at_index(current_index);
  if (token.type != ANTLR3_TOKEN_INVALID)
    result += token.start; // Implicitly zero-terminated.

  return result;
}

//--------------------------------------------------------------------------------------------------
