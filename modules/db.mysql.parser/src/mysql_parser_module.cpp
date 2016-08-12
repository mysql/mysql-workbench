/*
 * Copyright (c) 2013, 2016, Oracle and/or its affiliates. All rights reserved.
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
#include "mysql-recognition-types.h"

#include "MySQLLexer.h"
#include "MySQLParser.h"
#include "MySQLParserBaseListener.h"

#include "objimpl/wrapper/parser_ContextReference_impl.h"
#include "grtdb/db_object_helpers.h"

using namespace grt;
using namespace parsers;

using namespace antlr4;
using namespace antlr4::atn;

DEFAULT_LOG_DOMAIN("parser")

GRT_MODULE_ENTRY_POINT(MySQLParserServicesImpl);

//--------------------------------------------------------------------------------------------------

long shortVersion(const GrtVersionRef &version)
{
  ssize_t short_version;
  if (version.is_valid())
  {
    short_version = version->majorNumber() * 10000;
    if (version->minorNumber() > -1)
      short_version += version->minorNumber() * 100;
    else
      short_version += 500;
    if (version->releaseNumber() > -1)
      short_version += version->releaseNumber();
  }
  else
    short_version = 50500; // Assume minimal supported version.

  return (long)short_version;
}

//------------------ MySQLParserContextImpl ------------------------------------------------------

struct MySQLParserContextImpl : public MySQLParserContext {
  ANTLRInputStream input;
  MySQLLexer lexer;
  CommonTokenStream tokens;
  MySQLParser parser;

  GrtVersionRef version;
  std::string mode;

  bool caseSensitive;
  std::vector<ParserErrorInfo> errors;

  MySQLParserContextImpl(GrtCharacterSetsRef charsets, GrtVersionRef version_, bool caseSensitive)
    : lexer(&input), tokens(&lexer), parser(&tokens), caseSensitive(caseSensitive)
  {
    std::set<std::string> filteredCharsets;
    for (size_t i = 0; i < charsets->count(); i++)
      filteredCharsets.insert("_" + base::tolower(*charsets[i]->name()));
    lexer.charsets = filteredCharsets;
    updateServerVersion(version_);
  }

  virtual bool isCaseSensitive() override
  {
    return caseSensitive;
  }

  virtual void updateServerVersion(GrtVersionRef newVersion) override
  {
    if (version != newVersion)
    {
      version = newVersion;
      lexer.serverVersion = shortVersion(version);
      parser.serverVersion = lexer.serverVersion;

      if (lexer.serverVersion < 50503)
      {
        lexer.charsets.erase("_utf8mb4");
        lexer.charsets.erase("_utf16");
        lexer.charsets.erase("_utf32");
      }
      else
      {
        // Duplicates are automatically ignored.
        lexer.charsets.insert("_utf8mb4");
        lexer.charsets.insert("_utf16");
        lexer.charsets.insert("_utf32");
      }
    }
  }

  virtual void updateSqlMode(const std::string &mode) override
  {
    this->mode = mode;
    lexer.sqlModeFromString(mode);
    parser.sqlMode = lexer.sqlMode;
  }

  virtual GrtVersionRef serverVersion() const override
  {
    return version;
  }

  virtual std::string sqlMode() const override
  {
    return mode;
  }

  void addError(const std::string &message, ssize_t tokenType, size_t start, size_t line, size_t offsetInLine,
                size_t length)
  {
    ParserErrorInfo info = { message, tokenType, start, line, offsetInLine, length};
    errors.push_back(info);
  }

  /**
   * Returns a collection of errors from the last parser run. The start position is offset by the given
   * value (used to adjust error position in a larger context).
   */
  virtual std::vector<ParserErrorInfo> errorsWithOffset(size_t offset) const override
  {
    std::vector<ParserErrorInfo> result = errors;

    for (auto &entry : result)
      entry.charOffset += offset;

    return result;
  }

  bool parse(const std::string &text, MySQLParseUnit unit, MySQLParserBaseListener *listener)
  {
    parser.removeParseListeners();
    parser.addParseListener(listener);
    input.load(text);
    startParsing(false, unit);
    return errors.empty();
  }

  bool errorCheck(const std::string &text, MySQLParseUnit unit)
  {
    parser.removeParseListeners();
    input.load(text);
    startParsing(true, unit);
    return errors.empty();
  }

  MySQLQueryType determineQueryType(const std::string &text)
  {
    input.load(text);
    lexer.setInputStream(&input);
    return lexer.determineQueryType();
  }

private:
  void parseUnit(MySQLParseUnit unit)
  {
    switch (unit)
    {
      case MySQLParseUnit::PuCreateTable:
        parser.create_table();
        break;
      case MySQLParseUnit::PuCreateTrigger:
        parser.create_trigger();
        break;
      case MySQLParseUnit::PuCreateView:
        parser.create_view();
        break;
      case MySQLParseUnit::PuCreateRoutine:
        parser.create_routine();
        break;
      case MySQLParseUnit::PuCreateEvent:
        parser.create_event();
        break;
      case MySQLParseUnit::PuCreateIndex:
        parser.create_index();
        break;
      case MySQLParseUnit::PuGrant:
        parser.parse_grant();
        break;
      case MySQLParseUnit::PuDataType:
        parser.data_type_definition();
        break;
      case MySQLParseUnit::PuCreateLogfileGroup:
        parser.create_logfile_group();
        break;
      case MySQLParseUnit::PuCreateServer:
        parser.create_server();
        break;
      case MySQLParseUnit::PuCreateTablespace:
        parser.create_tablespace();
        break;
      default:
        parser.query();
        break;
    }
  }

  void startParsing(bool fast, MySQLParseUnit unit)
  {
    errors.clear();
    lexer.setInputStream(&input); // Not just reset(), which only rewinds the current position.
    lexer.inVersionComment = false; // In the case something was wrong in the previous parse call.
    tokens.setTokenSource(&lexer);
    parser.setBuildParseTree(!fast);

    // First parse with the bail error strategy to get quick feedback for correct queries.
    parser.setErrorHandler(std::make_shared<BailErrorStrategy>());
    parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::SLL);

    try {
      tokens.fill();
    } catch (IllegalStateException &) {
      addError("Error: illegal state found, probably unfinished string.", 0, 0, 0, 0, 1);
    }

    try {
      parseUnit(unit);
    } catch (ParseCancellationException &pce) {
      if (!fast)
      {
        // If parsing was cancelled we either really have a syntax error or we need to do a second step,
        // now with the default strategy and LL parsing.
        tokens.reset();
        parser.reset();
        parser.setErrorHandler(std::make_shared<DefaultErrorStrategy>());
        parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::LL);
        parseUnit(unit);
      }
    }
  }
};

//------------------ MySQLParserServicesImpl -------------------------------------------------------

MySQLParserContext::Ref MySQLParserServicesImpl::createParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version,
  const std::string &sqlMode, bool caseSensitive)
{
  MySQLParserContext::Ref context = std::make_shared<MySQLParserContextImpl>(charsets, version, caseSensitive != 0);
  context->updateSqlMode(sqlMode);
  return context;
}

//--------------------------------------------------------------------------------------------------

parser_ContextReferenceRef MySQLParserServicesImpl::createNewParserContext(GrtCharacterSetsRef charsets,
  GrtVersionRef version, const std::string &sqlMode, int caseSensitive)
{
  MySQLParserContext::Ref context = std::make_shared<MySQLParserContextImpl>(charsets, version, caseSensitive != 0);
  context->updateSqlMode(sqlMode);
  return parser_context_to_grt(context);
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::tokenFromString(MySQLParserContext::Ref context, const std::string &token)
{
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  if (impl == nullptr)
    return ParserToken::INVALID_TYPE;

  return impl->lexer.getTokenType(token);
}

//--------------------------------------------------------------------------------------------------

/**
 *	Helper to bring the index type string into a commonly used form.
 */
std::string formatIndexType(std::string indexType)
{
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
static std::string getDefiner(Scanner &scanner)
{
  std::string definer;
  if (scanner.is(MySQLLexer::DEFINER_SYMBOL))
  {
    scanner.next(2); // Skip DEFINER + equal sign.
    if (scanner.is(MySQLLexer::CURRENT_USER_SYMBOL))
    {
      definer = scanner.tokenText(true);
      scanner.next();
      scanner.skipIf(MySQLLexer::OPEN_PAR_SYMBOL, 2);
    }
    else
    {
      // A user@host entry.
      definer = scanner.tokenText(true);
      scanner.next();
      switch (scanner.tokenType())
      {
      case MySQLLexer::AT_TEXT_SUFFIX:
        definer += scanner.tokenText();
        scanner.next();
        break;
      case MySQLLexer::AT_SIGN_SYMBOL:
        scanner.next(); // Skip @.
        definer += '@' + scanner.tokenText(true);
        scanner.next();
      }
    }
  }

  return definer;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the identifier text for a dot_identifier sequence (see this rule in the grammar for details).
 * The caller is responsible for checking the validity of that token on enter.
 * Advances the scanner to the first position after the identifier.
 */
static std::string getDotIdentifier(Scanner &scanner)
{
  std::string result;
  if (scanner.is(MySQLLexer::DOT_SYMBOL))
  {
    scanner.next();
    result = scanner.tokenText();
  }
  else
    result = scanner.tokenText().substr(1);
  scanner.next();

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Read an object identifier "(id?.)?id" from the current scanner position.
 * This functions tries get as much info as possible even if the syntax is not fully correct.
 *
 * On return the scanner points to the first token after the id.
 */
static Identifier getIdentifier(Scanner &scanner)
{
  Identifier result;
  if (scanner.is(MySQLLexer::DOT_SYMBOL) || scanner.is(MySQLLexer::DOT_IDENTIFIER))
  {
    // Starting with a dot (ODBC style).
    result.second = getDotIdentifier(scanner);
  }
  else
  {
    result.second = scanner.tokenText();
    scanner.next();
    if (scanner.is(MySQLLexer::DOT_SYMBOL) || scanner.is(MySQLLexer::DOT_IDENTIFIER))
    {
      result.first = result.second;
      result.second = getDotIdentifier(scanner);
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Same as getIdentifer but for column references (id, .id, id.id, id.id.id) or column names (id).
 *
 * On return the scanner points to the first token after the id.
 */
static ColumnIdentifier getColumnIdentifier(Scanner &scanner)
{
  ColumnIdentifier result;
  if (scanner.is(MySQLLexer::DOT_SYMBOL) || scanner.is(MySQLLexer::DOT_IDENTIFIER))
    result.column = getDotIdentifier(scanner);
  else
  {
    // id
    result.column = scanner.tokenText();
    scanner.next();
    if (scanner.is(MySQLLexer::DOT_SYMBOL) || scanner.is(MySQLLexer::DOT_IDENTIFIER))
    {
      // id.id
      result.table = result.column;
      result.column = getDotIdentifier(scanner);

      if (scanner.is(MySQLLexer::DOT_SYMBOL) || scanner.is(MySQLLexer::DOT_IDENTIFIER))
      {
        // id.id.id
        result.schema = result.table;
        result.table = result.column;
        result.column = getDotIdentifier(scanner);
      }
    }
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Collects a list of comma separated values (table/columns lists, enums etc.) enclosed by parentheses.
 * The comma separated list is returned and the scanner points to the first token after the closing
 * parenthesis when done.
 */
static std::string getValueList(Scanner &scanner, bool keepQuotes = false)
{
  std::string result;
  if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
  {
    while (true)
    {
      scanner.next();
      if (!result.empty())
        result += ", ";
      result += scanner.tokenText(keepQuotes);
      scanner.next();
      if (scanner.tokenType() != MySQLLexer::COMMA_SYMBOL)
        break;
    }
    scanner.next();
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Collects a list of names that are enclosed by parentheses into individual entries.
 * On return the scanner points to the first token after the closing
 * parenthesis when done.
 */
static std::vector<std::string> getNamesList(Scanner &scanner)
{
  std::vector<std::string> result;
  if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
  {
    while (true)
    {
      scanner.next();
      result.push_back(scanner.tokenText());
      scanner.next();
      if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
        break;
    }
    scanner.next();
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
*	Returns the text for an expression (optionally within parentheses).
*/
static std::string getExpression(Scanner &scanner)
{
  bool skipParens = scanner.is(MySQLLexer::OPEN_PAR_SYMBOL);

  if (skipParens)
    scanner.next();
  std::string text; // XXX: = scanner.textForTree();
  //scanner.skipSubtree();

  if (skipParens)
    scanner.next(); // Skip CLOSE_PAR.

  return text;
}

//--------------------------------------------------------------------------------------------------

/**
*	Extracts the charset name from a "charset charset_name" rule sequence).
*/
static std::string getCharsetName(Scanner &scanner)
{
  if (!scanner.is(MySQLLexer::CHAR_SYMBOL) && !scanner.is(MySQLLexer::CHARSET_SYMBOL))
    return "";

  scanner.next();
  scanner.skipIf(MySQLLexer::SET_SYMBOL); // From CHAR SET.
  scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);

  if (scanner.is(MySQLLexer::BINARY_SYMBOL))
  {
    scanner.next();
    return "BINARY";
  }

  std::string name = scanner.tokenText(); // string or identifier
  scanner.next();
  return name;
}

//--------------------------------------------------------------------------------------------------

/**
 * The next 2 functions take a charset or collation and retrieve the associated charset/collation pair.
 */
static std::pair<std::string, std::string> detailsForCharset(const std::string &charset,
  const std::string &collation, const std::string &defaultCharset)
{
  std::pair<std::string, std::string> result;
  if (!charset.empty())
  {
    result.first = base::tolower(charset);
    if (result.first == "default")
      result.first = base::tolower(defaultCharset);

    if (!collation.empty())
    {
      result.second = base::tolower(collation);

      // Clear collation if it's default collation or it belongs to another character set.
      if ((result.second == defaultCollationForCharset(result.first))
        || (result.first != charsetForCollation(result.second)))
        result.second = "";
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

static std::pair<std::string, std::string> detailsForCollation(const std::string &collation,
  const std::string &defaultCollation)
{
  std::pair<std::string, std::string> result;
  if (!collation.empty())
  {
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
// Similarly, when resolving table names (qualified or not), we can do that also only after all parsing
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
  DbObjectReferences(db_ForeignKeyRef fk, ReferenceType type_)
  {
    foreignKey = fk;
    type = type_;
  }

  DbObjectReferences(db_IndexRef index_)
  {
    index = index_;
    type = Index;
  }

  // For pure table references.
  DbObjectReferences(Identifier identifier)
  {
    targetIdentifier = identifier;
    type = TableRef;
  }
};

typedef std::vector<DbObjectReferences> DbObjectsRefsCache;

//--------------------------------------------------------------------------------------------------

/**
*	Returns the schema with the given name. If it doesn't exist it will be created.
*/
static db_mysql_SchemaRef ensureSchemaExists(db_CatalogRef catalog, const std::string &name, bool caseSensitive)
{
  db_SchemaRef result = find_named_object_in_list(catalog->schemata(), name, caseSensitive);
  if (!result.is_valid())
  {
    result = db_mysql_SchemaRef(grt::Initialized);
    result->createDate(base::fmttime(0, DATETIME_FMT));
    result->lastChangeDate(result->createDate());
    result->owner(catalog);
    result->name(name);
    result->oldName(name);
    std::pair<std::string, std::string> info = detailsForCharset(catalog->defaultCharacterSetName(),
      catalog->defaultCollationName(), catalog->defaultCharacterSetName());
    result->defaultCharacterSetName(info.first);
    result->defaultCollationName(info.second);
    catalog->schemata().insert(result);
  }
  return db_mysql_SchemaRef::cast_from(result);
}

//--------------------------------------------------------------------------------------------------

static void fillTableCreateOptions(Scanner &scanner, db_CatalogRef catalog,
  db_mysql_SchemaRef schema, db_mysql_TableRef table, bool caseSensitive)
{
  std::string schemaName = schema.is_valid() ? schema->name() : "";
  std::string defaultCharset = schema.is_valid() ? schema->defaultCharacterSetName() : "";
  std::string defaultCollation = schema.is_valid() ? schema->defaultCollationName() : "";
  if (defaultCollation.empty() && !defaultCharset.empty())
    defaultCollation = defaultCollationForCharset(defaultCharset);

  while (true) // create_table_options
  {
    switch (scanner.tokenType())
    {
    case MySQLLexer::ENGINE_SYMBOL:
    case MySQLLexer::TYPE_SYMBOL:
    {
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      Identifier identifier = getIdentifier(scanner);
      table->tableEngine(identifier.second);
      break;
    }

    case MySQLLexer::MAX_ROWS_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->maxRows(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::MIN_ROWS_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->minRows(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::AVG_ROW_LENGTH_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->avgRowLength(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::PASSWORD_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->password(scanner.tokenText());
        // XXX: scanner.skipSubtree(); // Skip string subtree. Can be PARAM_MARKER too, but skipSubtree() still works for that.
      break;

    case MySQLLexer::COMMENT_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->comment(scanner.tokenText());
      // XXX: scanner.skipSubtree(); // Skip over string sub tree.
      break;

    case MySQLLexer::AUTO_INCREMENT_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->nextAutoInc(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::PACK_KEYS_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->packKeys(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::STATS_AUTO_RECALC_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->statsAutoRecalc(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::STATS_PERSISTENT_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->statsPersistent(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::STATS_SAMPLE_PAGES_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->statsSamplePages(base::atoi<size_t>(scanner.tokenText()));
      scanner.next();
      break;

    case MySQLLexer::CHECKSUM_SYMBOL:
    case MySQLLexer::TABLE_CHECKSUM_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->checksum(base::atoi<size_t>(scanner.tokenText()));
      scanner.next();
      break;

    case MySQLLexer::DELAY_KEY_WRITE_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->delayKeyWrite(base::atoi<size_t>(scanner.tokenText()));
      scanner.next();
      break;

    case MySQLLexer::ROW_FORMAT_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->rowFormat(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::UNION_SYMBOL: // Only for merge engine.
    {
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      scanner.next(); // Skip OPEN_PAR.
      std::string value;
      while (true)
      {
        if (!value.empty())
          value += ", ";
        Identifier identifier = getIdentifier(scanner);

        if (identifier.first.empty())
          // In order to avoid diff problems explicitly qualify unqualified tables
          // with the current schema name.
          value += schemaName + '.' + identifier.second;
        else
        {
          ensureSchemaExists(catalog, identifier.first, caseSensitive);
          value += identifier.first + "." + identifier.second;
        }

        if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
          break;
        scanner.next();
      }
      scanner.next();
      table->mergeUnion(value);
      break;
    }

    case MySQLLexer::DEFAULT_SYMBOL:
    case MySQLLexer::COLLATE_SYMBOL:
    case MySQLLexer::CHAR_SYMBOL:
    case MySQLLexer::CHARSET_SYMBOL:
      scanner.skipIf(MySQLLexer::DEFAULT_SYMBOL);
      if (scanner.is(MySQLLexer::COLLATE_SYMBOL))
      {
        scanner.next();
        scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);

        std::pair<std::string, std::string> info = detailsForCollation(scanner.tokenText(), defaultCollation);
        table->defaultCharacterSetName(info.first);
        table->defaultCollationName(info.second);
      }
      else
      {
        scanner.next();
        scanner.skipIf(MySQLLexer::SET_SYMBOL); // From CHAR SET.
        scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);

        std::pair<std::string, std::string> info = detailsForCharset(scanner.tokenText(),
          defaultCollation, defaultCharset);
        table->defaultCharacterSetName(info.first);
        table->defaultCollationName(info.second); // Collation name or DEFAULT.
      }
      scanner.next();
      break;

    case MySQLLexer::INSERT_METHOD_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->mergeInsert(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::DATA_SYMBOL:
      scanner.next(2); // Skip DATA DIRECTORY.
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->tableDataDir(scanner.tokenText());
      // XXX: scanner.skipSubtree(); // Skip string subtree.
      break;

    case MySQLLexer::INDEX_SYMBOL:
      scanner.next(2); // Skip INDEX DIRECTORY.
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->tableIndexDir(scanner.tokenText());
      // XXX: scanner.skipSubtree(); // Skip string subtree.
      break;

    case MySQLLexer::TABLESPACE_SYMBOL:
      scanner.next();
      table->tableSpace(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::STORAGE_SYMBOL:
      //(DISK_SYMBOL | MEMORY_SYMBOL) ignored for now, as not documented.
      scanner.next(2);
      break;

    case MySQLLexer::CONNECTION_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->connectionString(scanner.tokenText());
      // XXX: scanner.skipSubtree(); // Skip string sub tree.
      break;

    case MySQLLexer::KEY_BLOCK_SIZE_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      table->keyBlockSize(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::COMMA_SYMBOL:
      scanner.next();
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
static std::string getPartitionValueList(Scanner &scanner)
{
  std::string value;
  scanner.next();
  while (true)
  {
    if (!value.empty())
      value += ", ";
    if (scanner.is(MySQLLexer::MAXVALUE_SYMBOL))
      value += "MAXVALUE";
    else
      value += getExpression(scanner);
    if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
      break;
    scanner.next();
  }
  scanner.next();
  return value;
}

//--------------------------------------------------------------------------------------------------

/**
*	Parses main and sub partitions.
*/
static void getPartitionDefinition(Scanner &scanner, db_mysql_PartitionDefinitionRef definition)
{
  scanner.next(); // Skip PARTITION or SUBPARTITION.
  definition->name(scanner.tokenText());
  scanner.next();
  if (scanner.is(MySQLLexer::VALUES_SYMBOL)) // Appears only for main partitions.
  {
    scanner.next();
    if (scanner.is(MySQLLexer::LESS_SYMBOL))
    {
      scanner.next(2); // Skip LESS THAN.
      if (scanner.is(MySQLLexer::MAXVALUE_SYMBOL))
        definition->value("MAXVALUE");
      else
        definition->value(getPartitionValueList(scanner));
    }
    else
    {
      // Otherwise IN.
      scanner.next();
      definition->value(getPartitionValueList(scanner));
    }
  }

  bool done = false;
  while (!done)
  { // Zero or more.
    switch (scanner.tokenType())
    {
    case MySQLLexer::TABLESPACE_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      definition->tableSpace(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::STORAGE_SYMBOL:
    case MySQLLexer::ENGINE_SYMBOL:
      scanner.next(scanner.is(MySQLLexer::STORAGE_SYMBOL) ? 2 : 1);
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      scanner.next(); // Skip ENGINE_REF_TOKEN.
      definition->engine(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::NODEGROUP_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      definition->nodeGroupId(base::atoi<size_t>(scanner.tokenText()));
      scanner.next();
      break;
    case MySQLLexer::MAX_ROWS_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      definition->maxRows(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::MIN_ROWS_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      definition->minRows(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::DATA_SYMBOL:
      scanner.next(2);
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      definition->dataDirectory(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::INDEX_SYMBOL:
      scanner.next(2);
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      definition->indexDirectory(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::COMMENT_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      definition->comment(scanner.tokenText());
      scanner.next();
      break;
    default:
      done = true;
      break;
    }
  }

  // Finally the sub partition definitions (optional).
  // Only appearing for main partitions (i.e. there is no unlimited nesting).
  definition->subpartitionDefinitions().remove_all();
  if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
  {
    scanner.next();
    while (true)
    {
      db_mysql_PartitionDefinitionRef subdefinition(grt::Initialized);
      getPartitionDefinition(scanner, subdefinition);
      definition->subpartitionDefinitions().insert(subdefinition);
      if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
        break;
      scanner.next();
    }

    scanner.next();
  }
}

//--------------------------------------------------------------------------------------------------

static void fillTablePartitioning(Scanner &scanner, db_mysql_TableRef &table)
{
  if (!scanner.is(MySQLLexer::PARTITION_SYMBOL))
    return;

  scanner.next(2); // Skip PARTITION BY.
  bool linear = scanner.skipIf(MySQLLexer::LINEAR_SYMBOL);
  ssize_t type = scanner.tokenType();
  table->partitionType((linear ? "LINEAR " : "") + base::toupper(scanner.tokenText())); // MySQLLexer::HASH, MySQLLexer::KEY, MySQLLexer::RANGE, LIST.
  scanner.next();
  switch (type)
  {
  case MySQLLexer::HASH_SYMBOL:
    table->partitionExpression(getExpression(scanner));
    break;
  case MySQLLexer::KEY_SYMBOL:
    if (scanner.is(MySQLLexer::ALGORITHM_SYMBOL))
    {
      scanner.next(2); // Skip ALGORITHM EQUAL.
      table->partitionKeyAlgorithm(base::atoi<size_t>(scanner.tokenText()));
      scanner.next();
    }
    table->partitionExpression(getValueList(scanner));
    break;
  case MySQLLexer::RANGE_SYMBOL:
  case MySQLLexer::LIST_SYMBOL:
    if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
      table->partitionExpression(getExpression(scanner));
    else
    {
      scanner.next(); // Skip COLUMNS.
      table->partitionExpression(getValueList(scanner));
    }
    break;
  }

  if (scanner.is(MySQLLexer::PARTITIONS_SYMBOL))
  {
    scanner.next();
    table->partitionCount(base::atoi<size_t>(scanner.tokenText()));
    scanner.next();
  }

  if (scanner.is(MySQLLexer::SUBPARTITION_SYMBOL))
  {
    scanner.next(2); // Skip SUBPARTITION BY.
    linear = scanner.skipIf(MySQLLexer::LINEAR_SYMBOL);
    table->subpartitionType((linear ? "LINEAR " : "") + base::toupper(scanner.tokenText()));
    if (scanner.is(MySQLLexer::HASH_SYMBOL))
    {
      scanner.next();
      table->subpartitionExpression(getExpression(scanner));
    }
    else
    {
      // Otherwise KEY type.
      if (scanner.is(MySQLLexer::ALGORITHM_SYMBOL))
      {
        scanner.next(2); // Skip ALGORTIHM EQUAL.
        table->subpartitionKeyAlgorithm(base::atoi<size_t>(scanner.tokenText()));
        scanner.next();
      }
      table->subpartitionExpression(getValueList(scanner));
    }

    if (scanner.is(MySQLLexer::SUBPARTITIONS_SYMBOL))
    {
      scanner.next();
      table->subpartitionCount(base::atoi<size_t>(scanner.tokenText()));
      scanner.next();
    }
  }

  // Finally the partition definitions.
  table->partitionDefinitions().remove_all();
  if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
  {
    scanner.next();
    while (true)
    {
      db_mysql_PartitionDefinitionRef definition(grt::Initialized);
      definition->owner(table);
      getPartitionDefinition(scanner, definition);
      table->partitionDefinitions().insert(definition);
      if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
        break;
      scanner.next();
    }

    scanner.next();
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

static void fillIndexColumns(Scanner &scanner, db_TableRef &table, db_mysql_IndexRef index)
{
  index->columns().remove_all();

  scanner.next(); // Opening parenthesis.
  while (true)
  {
    db_mysql_IndexColumnRef column(grt::Initialized);
    column->owner(index);
    index->columns().insert(column);

    std::string referenceName = scanner.tokenText();
    scanner.next();
    if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL)) // Field length.
    {
      scanner.next();
      column->columnLength(base::atoi<size_t>(scanner.tokenText()));
      scanner.next(2);
    }

    if (scanner.is(MySQLLexer::ASC_SYMBOL) || scanner.is(MySQLLexer::DESC_SYMBOL))
    {
      column->descend(scanner.is(MySQLLexer::DESC_SYMBOL));
      scanner.next();
    }

    if (table.is_valid())
    {
      db_ColumnRef referencedColumn = find_named_object_in_list(table->columns(), referenceName, false);
      if (referencedColumn.is_valid())
        column->referencedColumn(referencedColumn);
    }

    if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
      break;
  }
  scanner.next(); // Closing parenthesis.
}

//--------------------------------------------------------------------------------------------------

static void fillIndexOptions(Scanner &scanner, db_mysql_IndexRef index)
{
  while (true)
  {
    // Unlimited occurrences.
    switch (scanner.tokenType())
    {
    case MySQLLexer::USING_SYMBOL:
    case MySQLLexer::TYPE_SYMBOL:
      scanner.next();
      index->indexKind(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::KEY_BLOCK_SIZE_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      index->keyBlockSize(base::atoi<size_t>(scanner.tokenText()));
      scanner.next();
      break;

    case MySQLLexer::COMMENT_SYMBOL:
      scanner.next();
      index->comment(scanner.tokenText());
      scanner.next();
      break;

    case MySQLLexer::WITH_SYMBOL: // WITH PARSER
      scanner.next(2);
      index->withParser(scanner.tokenText());
      scanner.next();
      break;

    default:
      return;
    }
  }
}

//--------------------------------------------------------------------------------------------------

static void fillDataTypeAndAttributes(Scanner &scanner, db_CatalogRef catalog, db_mysql_TableRef table,
  db_mysql_ColumnRef column)
{
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
  std::string type_name = scanner.tokenText();

  switch (scanner.tokenType())
  {
  case MySQLLexer::DOUBLE_SYMBOL:
    scanner.next();
    if (scanner.is(MySQLLexer::PRECISION_SYMBOL))
      scanner.next(); // Simply ignore syntactic sugar.
    break;

  case MySQLLexer::NATIONAL_SYMBOL:
    scanner.next();
    type_name += " " + scanner.tokenText();
    scanner.next();
    if (scanner.is(MySQLLexer::VARYING_SYMBOL))
    {
      type_name += " " + scanner.tokenText();
      scanner.next();
    }
    break;

  case MySQLLexer::NCHAR_SYMBOL:
    scanner.next();
    if (scanner.is(MySQLLexer::VARCHAR_SYMBOL) || scanner.is(MySQLLexer::VARYING_SYMBOL))
    {
      type_name += " " + scanner.tokenText();
      scanner.next();
    }
    break;

  case MySQLLexer::CHAR_SYMBOL:
    scanner.next();
    if (scanner.is(MySQLLexer::VARYING_SYMBOL))
    {
      type_name += " " + scanner.tokenText();
      scanner.next();
    }
    break;

  case MySQLLexer::LONG_SYMBOL:
    scanner.next();
    switch (scanner.tokenType())
    {
    case MySQLLexer::CHAR_SYMBOL: // LONG CHAR VARYING
      if (scanner.lookAhead() == MySQLLexer::VARYING_SYMBOL) // Otherwise we may get e.g. LONG CHAR SET...
      {
        type_name += " " + scanner.tokenText();
        scanner.next();
        type_name += " " + scanner.tokenText();
        scanner.next();
      }
      break;

    case MySQLLexer::VARBINARY_SYMBOL:
    case MySQLLexer::VARCHAR_SYMBOL:
      type_name += " " + scanner.tokenText();
      scanner.next();
    }
    break;

  default:
    scanner.next();
  }

  simpleType = MySQLParserServices::findDataType(catalog->simpleDatatypes(), catalog->version(), type_name);
  if (simpleType.is_valid()) // Should always be valid at this point.
  {
    if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
    {
      // We use the simple type properties for char length to learn if we have a length here or a precision.
      // We could indicate that in the grammar instead, however the handling in WB is a bit different
      // than what the server grammar would suggest (e.g. the length is also used for integer types, in the grammar).
      if (simpleType->characterMaximumLength() != bec::EMPTY_TYPE_MAXIMUM_LENGTH
        || simpleType->characterOctetLength() != bec::EMPTY_TYPE_OCTET_LENGTH)
      {
        scanner.next(); // Skip OPEN_PAR.
        if (scanner.is(MySQLLexer::INT_NUMBER))
        {
          length = base::atoi<size_t>(scanner.tokenText());
          scanner.next();
        }
        scanner.next(); // Skip CLOSE_PAR.
      }
      else
      {
        if (simpleType->name() == "SET" || simpleType->name() == "ENUM")
          explicitParams = "(" + getValueList(scanner, true) + ")";
        else
        {
          // Finally all cases with either precision, scale or both.
          scanner.next(); // Skip OPEN_PAR.
          if (simpleType->numericPrecision() != bec::EMPTY_TYPE_PRECISION)
            precision = base::atoi<size_t>(scanner.tokenText());
          else
            length = base::atoi<size_t>(scanner.tokenText());
          scanner.next();
          if (scanner.is(MySQLLexer::COMMA_SYMBOL))
          {
            scanner.next();
            scale = base::atoi<size_t>(scanner.tokenText());
            scanner.next();
          }
          scanner.next(); // Skip CLOSE_PAR.
        }
      }
    }

    // Collect additional flags + charset.
    bool done = false;
    while (!done)
    {
      switch (scanner.tokenType())
      {
        // case MySQLLexer::BYTE_SYMBOL: seems not be used anywhere.
      case MySQLLexer::SIGNED_SYMBOL:
      case MySQLLexer::UNSIGNED_SYMBOL:
      case MySQLLexer::ZEROFILL_SYMBOL:
      {
        std::string value = base::toupper(scanner.tokenText());
        scanner.next();
        if (flags.get_index(value) == BaseListRef::npos)
          flags.insert(value);
        break;
      }

      case MySQLLexer::ASCII_SYMBOL:
      case MySQLLexer::UNICODE_SYMBOL:
      {
        std::string value = base::toupper(scanner.tokenText());
        scanner.next();
        if (flags.get_index(value) == BaseListRef::npos)
          flags.insert(value);

        // Could be followed by BINARY.
        if (scanner.skipIf(MySQLLexer::BINARY_SYMBOL))
        {
          if (flags.get_index("BINARY") == BaseListRef::npos)
            flags.insert("BINARY");
        }
        break;
      }

      case MySQLLexer::BINARY_SYMBOL:
      {
        scanner.next();
        if (flags.get_index("BINARY") == BaseListRef::npos)
          flags.insert("BINARY");

        switch (scanner.tokenType())
        {
        case MySQLLexer::ASCII_SYMBOL:
        case MySQLLexer::UNICODE_SYMBOL:
        {
          std::string value = base::toupper(scanner.tokenText());
          scanner.next();
          if (flags.get_index(value) == BaseListRef::npos)
            flags.insert(value);

          break;
        }
        case MySQLLexer::CHAR_SYMBOL:
        case MySQLLexer::CHARSET_SYMBOL:
          std::pair<std::string, std::string> info = detailsForCharset(getCharsetName(scanner),
            column->collationName(), table->defaultCharacterSetName());
          column->characterSetName(info.first);
          column->collationName(info.second);
          break;
        }
        break;
      }

      case MySQLLexer::CHAR_SYMBOL:
      case MySQLLexer::CHARSET_SYMBOL: // The grammar allows for weird syntax like: "char char set binary binary".
      {
        std::pair<std::string, std::string> info = detailsForCharset(getCharsetName(scanner),
          column->collationName(), table->defaultCharacterSetName());
        column->characterSetName(info.first);
        column->collationName(info.second);

        if (scanner.is(MySQLLexer::BINARY_SYMBOL))
        {
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
    while (!done)
    {
      switch (scanner.tokenType())
      {
      case MySQLLexer::NOT_SYMBOL:
      case MySQLLexer::NULL_SYMBOL:
      case MySQLLexer::NULL2_SYMBOL:
      {
        column->isNotNull(scanner.skipIf(MySQLLexer::NOT_SYMBOL));
        scanner.next(); // Skip NULL/NULL2.

        explicitNullValue = true;

        break;
      }
      case MySQLLexer::DEFAULT_SYMBOL:
      {
        // Default values.
        // Note: for DEFAULT NOW (and synonyms) there can be an additional ON UPDATE NOW (and synonyms).
        //       We store both parts together in the defaultValue(). Keep in mind however that
        //       attributes can be in any order and appear multiple times.
        //       In order to avoid trouble we convert all NOW synonyms to CURRENT_TIMESTAMP.
        std::string existingDefault = column->defaultValue();

        // Remove any previous default value. This will also remove ON UPDATE if it was there plus
        // any another default value. It doesn't handle time precision either.
        // We can either have that or concatenate all default values (which is really wrong).
        // MySQLLexer::TODO: revise the decision to put both into the default value.
        if (existingDefault != "ON UPDATE CURRENT_TIMESTAMP")
          existingDefault = "";
        scanner.next();
        if (scanner.is(MySQLLexer::NOW_SYMBOL))
        {
          // As written above, convert all synonyms. This can cause trouble with additional
          // precision, which we may have later to handle.
          std::string newDefault = "CURRENT_TIMESTAMP";
          scanner.next();
          if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL)) // Additional precision.
          {
            newDefault += "(";
            scanner.next();
            if (scanner.is(MySQLLexer::INT_NUMBER)) // Optional precision.
            {
              newDefault += scanner.tokenText();
              scanner.next();
            }
            newDefault += ")";
            scanner.next();
          }
          if (!existingDefault.empty())
            newDefault += " " + existingDefault;
          column->defaultValue(newDefault);
        }
        else
        {
          // signed_literal
          std::string newDefault;
          if (scanner.is(MySQLLexer::MINUS_OPERATOR) || scanner.is(MySQLLexer::PLUS_OPERATOR))
          {
            if (scanner.is(MySQLLexer::MINUS_OPERATOR))
              newDefault = "-";
            scanner.next();
            newDefault += scanner.tokenText(); // The actual value.
          }
          else
          {
            // Any literal (string, number, bool , null, temporal, bit, hex).
            // We need to keep quotes around strings in order to distinguish between no default
            // and an empty string default.
            newDefault = scanner.tokenText(true);

            // Temporal values have second part (the actual value).
            if (scanner.is(MySQLLexer::DATE_SYMBOL) || scanner.is(MySQLLexer::TIME_SYMBOL) || scanner.is(MySQLLexer::TIMESTAMP_SYMBOL))
            {
              scanner.next();
              newDefault += " " + scanner.tokenText(true);
            }
          }

          // XXX: scanner.skipSubtree();
          column->defaultValue(newDefault);

          if (base::same_string(newDefault, "NULL", false))
            column->defaultValueIsNull(true);
        }

        explicitDefaultValue = true;
        break;
      }

      case MySQLLexer::ON_SYMBOL:
      {
        // As mentioned above we combine DEFAULT NOW and ON UPDATE NOW into a common default value.
        std::string newDefault = column->defaultValue();
        if (base::hasPrefix(newDefault, "CURRENT_TIMESTAMP"))
          newDefault += " ON UPDATE CURRENT_TIMESTAMP";
        else
          newDefault = "ON UPDATE CURRENT_TIMESTAMP";
        scanner.next(3);
        if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
        {
          newDefault += "(";
          scanner.next();
          if (scanner.is(MySQLLexer::IDENTIFIER)) // Optional precision.
          {
            newDefault += scanner.tokenText();
            scanner.next();
          }
          newDefault += ")";
          scanner.next();
        }
        column->defaultValue(newDefault);
        explicitDefaultValue = true;

        break;
      }

      case MySQLLexer::AUTO_INCREMENT_SYMBOL:
        scanner.next();
        column->autoIncrement(1);
        break;

      case MySQLLexer::SERIAL_SYMBOL: // SERIAL DEFAULT VALUE is an alias for NOT NULL AUTO_INCREMENT UNIQUE.
      case MySQLLexer::UNIQUE_SYMBOL:
      {
        if (scanner.is(MySQLLexer::UNIQUE_SYMBOL))
        {
          scanner.next();
          scanner.skipIf(MySQLLexer::KEY_SYMBOL);
        }
        else
        {
          scanner.next(3);
          column->isNotNull(1);
          column->autoIncrement(1);
        }

        // Add new unique index for that column.
        db_mysql_IndexRef index;
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

      case MySQLLexer::PRIMARY_SYMBOL:
        scanner.next();
        // fall through
      case MySQLLexer::KEY_SYMBOL:
      {
        scanner.next();
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

      case MySQLLexer::COMMENT_SYMBOL:
        scanner.next();
        column->comment(scanner.tokenText());
          // XXX: scanner.skipSubtree();
        break;

      case MySQLLexer::COLLATE_SYMBOL:
      {
        scanner.next();

        std::pair<std::string, std::string> info = detailsForCollation(scanner.tokenText(), table->defaultCollationName());
        column->characterSetName(info.first);
        column->collationName(info.second);
        scanner.next();
        break;
      }

      case MySQLLexer::COLUMN_FORMAT_SYMBOL: // Ignored by the server, so we ignore it here too.
        scanner.next(2);
        break;

      case MySQLLexer::STORAGE_SYMBOL: // No info available, might later become important.
        scanner.next(2);
        break;

      default:
        done = true;
        break;
      }
    }
  }

  // Generated columns. Handle them after column attributes, as the can be a collation before the actual definition.
  if (scanner.is(MySQLLexer::GENERATED_SYMBOL) || scanner.is(MySQLLexer::AS_SYMBOL))
  {
    column->generated(1);

    // GENRATED ALWAYS is optional.
    if (scanner.tokenType() == MySQLLexer::GENERATED_SYMBOL)
      scanner.next(2);

    scanner.next(2); // Skip AS (.
    // XXX: column->expression(scanner.textForTree());
    // XXX: scanner.skipSubtree();
    scanner.next(); // Skip ).

    if (scanner.is(MySQLLexer::VIRTUAL_SYMBOL) || scanner.is(MySQLLexer::STORED_SYMBOL)) // Storage type of the gcol.
    {
      column->generatedStorage(scanner.tokenText());
      scanner.next();
    }
  }

  column->userType(db_UserDatatypeRef()); // We always have normal data types here.
  column->simpleType(simpleType);
  // structuredType ignored here

  column->precision(precision);
  column->scale(scale);
  column->length(length);
  column->datatypeExplicitParams(explicitParams);

  if (simpleType.is_valid() && base::same_string(simpleType->name(), "TIMESTAMP", false))
  {
    if (!explicitNullValue)
      column->isNotNull(1);
  }

  if (!column->isNotNull() && !explicitDefaultValue)
    bec::ColumnHelper::set_default_value(column, "NULL");
}

//--------------------------------------------------------------------------------------------------

static void fillColumnReference(Scanner &scanner, const std::string &schemaName,
  DbObjectReferences &references)
{
  scanner.next(); // Skip REFERENCES_SYMBOL.

  Identifier identifier = getIdentifier(scanner);
  references.targetIdentifier = identifier;
  if (identifier.first.empty())
    references.targetIdentifier.first = schemaName;

  if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
    references.columnNames = getNamesList(scanner);

  if (scanner.is(MySQLLexer::MATCH_SYMBOL)) // MATCH is ignored by MySQL.
    scanner.next(2);

  // Finally ON DELETE/ON UPDATE can be in any order but only once each.
  while (scanner.is(MySQLLexer::ON_SYMBOL))
  {
    scanner.next();
    bool isDelete = scanner.is(MySQLLexer::DELETE_SYMBOL);
    scanner.next();

    std::string ruleText;
    switch (scanner.tokenType())
    {
    case MySQLLexer::RESTRICT_SYMBOL:
    case MySQLLexer::CASCADE_SYMBOL:
      ruleText = scanner.tokenText();
      scanner.next();
      break;
    case MySQLLexer::SET_SYMBOL:
    case MySQLLexer::NO_SYMBOL:
      ruleText = scanner.tokenText();
      scanner.next();
      ruleText += " " + scanner.tokenText();
      scanner.next();
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
static void fillRefIndexDetails(Scanner &scanner, std::string &constraintName,
  db_mysql_IndexRef index, db_mysql_TableRef table, DbObjectsRefsCache &refCache)
{
  // Not every key type supports every option, but all varying parts are optional and always
  // in the same order.
  /* XXX
  if (scanner.is(MySQLLexer::COLUMN_REF_TOKEN))
  {
    scanner.next();
    ColumnIdentifier identifier = getColumnIdentifier(scanner);
    if (constraintName.empty())
      constraintName = identifier.column;
  }
   */

  // index_type in the grammar.
  if (scanner.is(MySQLLexer::USING_SYMBOL) || scanner.is(MySQLLexer::TYPE_SYMBOL))
  {
    scanner.next();
    index->indexKind(base::toupper(scanner.tokenText())); // BTREE, RTREE, HASH.
    scanner.next();
  }

  // index_columns in the grammar (mandatory).
  scanner.next(); // Skip OPEN_PAR.
  DbObjectReferences references(index);
  references.table = table;

  while (true)
  {
    db_mysql_IndexColumnRef indexColumn(grt::Initialized);
    indexColumn->owner(index);
    indexColumn->name(getIdentifier(scanner).second);
    references.index->columns().insert(indexColumn);
    if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
    {
      // Field length.
      scanner.next();
      indexColumn->columnLength(base::atoi<size_t>(scanner.tokenText()));
      scanner.next(2); // Skip INTEGER and CLOSE_PAR.
    }
    if (scanner.is(MySQLLexer::ASC_SYMBOL) || scanner.is(MySQLLexer::DESC_SYMBOL))
    {
      indexColumn->descend(scanner.is(MySQLLexer::DESC_SYMBOL));
      scanner.next();
    }

    if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
      break;
    scanner.next();
  }

  refCache.push_back(references);
  scanner.next(); // Skip CLOSE_PAR.

  // Zero or more index_option.
  bool done = false;
  while (!done)
  {
    switch (scanner.tokenType())
    {
    case MySQLLexer::USING_SYMBOL:
    case MySQLLexer::TYPE_SYMBOL:
      scanner.next();
      index->indexKind(base::toupper(scanner.tokenText()));
      scanner.next();
      break;

    case MySQLLexer::KEY_BLOCK_SIZE_SYMBOL:
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
      index->keyBlockSize(base::atoi<size_t>(scanner.tokenText()));
      break;

    case MySQLLexer::COMMENT_SYMBOL:
      scanner.next();
      index->comment(scanner.tokenText());
        // XXX: scanner.skipSubtree();
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
static void processTableKeyItem(Scanner &scanner, db_CatalogRef catalog,
  const std::string &schemaName, db_mysql_TableRef table, bool autoGenerateFkNames, DbObjectsRefsCache &refCache)
{
  db_mysql_IndexRef index(grt::Initialized);
  index->owner(table);

  std::string constraintName;
  if (scanner.is(MySQLLexer::CONSTRAINT_SYMBOL))
  {
    scanner.next();
    /* XXX:
    if (scanner.is(MySQLLexer::COLUMN_REF_TOKEN))
    {
      scanner.next();
      ColumnIdentifier identifier = getColumnIdentifier(scanner);
      constraintName = identifier.column;
    }
     */
  }

  bool isForeignKey = false; // Need the new index only for non-FK constraints.
  switch (scanner.tokenType())
  {
  case MySQLLexer::PRIMARY_SYMBOL:
    index->isPrimary(1);
    table->primaryKey(index);
    constraintName = "PRIMARY";
    index->indexType("PRIMARY");
    scanner.next(2); // Skip PRIMARY KEY.
    break;

  case MySQLLexer::FOREIGN_SYMBOL:
  {
    isForeignKey = true;
    scanner.next(2); // Skip FOREIGN KEY.

    /* XXX:
    if (scanner.is(MySQLLexer::COLUMN_REF_TOKEN))
    {
      scanner.next();
      ColumnIdentifier identifier = getColumnIdentifier(scanner);
      constraintName = identifier.column;
    }
     */

    db_mysql_ForeignKeyRef fk(grt::Initialized);
    fk->owner(table);
    fk->name(constraintName);
    fk->oldName(constraintName);

    if (fk->name().empty() && autoGenerateFkNames)
    {
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

      references.columnNames = getNamesList(scanner);
      refCache.push_back(references);
    }

    DbObjectReferences references(fk, DbObjectReferences::Referenced);
    references.table = table;
    fillColumnReference(scanner, schemaName, references);
    table->foreignKeys().insert(fk);
    refCache.push_back(references);

    break;
  }

  case MySQLLexer::UNIQUE_SYMBOL:
  case MySQLLexer::INDEX_SYMBOL:
  case MySQLLexer::KEY_SYMBOL:
    if (scanner.is(MySQLLexer::UNIQUE_SYMBOL))
    {
      index->unique(1);
      index->indexType("UNIQUE");
      scanner.next();
    }
    else
      index->indexType(formatIndexType(scanner.tokenText()));

    scanner.next();
    break;

  case MySQLLexer::FULLTEXT_SYMBOL:
  case MySQLLexer::SPATIAL_SYMBOL:
    index->indexType(formatIndexType(scanner.tokenText()));
    scanner.next();
    if (scanner.is(MySQLLexer::INDEX_SYMBOL) || scanner.is(MySQLLexer::KEY_SYMBOL))
      scanner.next();
    break;
  }

  if (!isForeignKey)
  {
    fillRefIndexDetails(scanner, constraintName, index, table, refCache);
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
static void fillTableCreateItem(Scanner &scanner, db_CatalogRef catalog,
  const std::string &schemaName, db_mysql_TableRef table, bool autoGenerateFkNames, DbObjectsRefsCache &refCache)
{/* XXX:
  scanner.next();
  switch (scanner.tokenType())
  {
  case MySQLLexer::COLUMN_NAME_TOKEN: // A column definition.
  {
    scanner.next();
    db_mysql_ColumnRef column(grt::Initialized);
    column->owner(table);

    // Column/key identifiers can be qualified, but they must always point to the table at hand
    // so it's rather useless and we ignore schema and table ids here.
    ColumnIdentifier identifier = getColumnIdentifier(scanner);
    column->name(identifier.column);
    column->oldName(column->name());
    scanner.next(); // Skip DATA_TYPE_TOKEN.
    fillDataTypeAndAttributes(scanner, catalog, table, column);
    table->columns().insert(column);

    if (scanner.is(MySQLLexer::REFERENCES_SYMBOL))
    {
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
      fillColumnReference(scanner, schemaName, references);
      refCache.push_back(references);
    }
    else
      if (scanner.is(MySQLLexer::CHECK_SYMBOL))
      {
        // CHECK (expression). Ignored by the server.
        scanner.next(3); // Skip CHECK OPEN_PAR_SYMBOL EXPRESSION_TOKEN.
        scanner.skipSubtree(); // Skip over expression subtree.
        scanner.next(); // Skip CLOSE_PAR_SYMBOL.
      }

    break;
  }

  case MySQLLexer::CONSTRAINT_SYMBOL:
  case MySQLLexer::PRIMARY_SYMBOL:
  case MySQLLexer::FOREIGN_SYMBOL:
  case MySQLLexer::UNIQUE_SYMBOL:
  case MySQLLexer::INDEX_SYMBOL:
  case MySQLLexer::KEY_SYMBOL:
  case MySQLLexer::FULLTEXT_SYMBOL:
  case MySQLLexer::SPATIAL_SYMBOL:
    processTableKeyItem(scanner, catalog, schemaName, table, autoGenerateFkNames, refCache);
    break;

  case MySQLLexer::CHECK_SYMBOL:
  {
    // CHECK (expression). Ignored by the server.
    scanner.next(3); // Skip CHECK OPEN_PAR_SYMBOL EXPRESSION_TOKEN.
    scanner.skipSubtree(); // Skip over expression subtree.
    scanner.next(); // Skip CLOSE_PAR_SYMBOL.
    break;
  }
  }
  */
}

//--------------------------------------------------------------------------------------------------

static std::pair<std::string, bool> fillTableDetails(Scanner &scanner,
  db_mysql_CatalogRef catalog, db_mysql_SchemaRef schema, db_mysql_TableRef &table,
  bool caseSensistive, bool autoGenerateFkNames, DbObjectsRefsCache &refCache)
{
  std::pair<std::string, bool> result("", false);

  scanner.next();
  table->isTemporary(scanner.skipIf(MySQLLexer::TEMPORARY_SYMBOL));
  scanner.next();
  if (scanner.is(MySQLLexer::IF_SYMBOL))
  {
    scanner.next();
    result.second = scanner.is(MySQLLexer::NOT_SYMBOL);
    scanner.next();
    scanner.skipIf(MySQLLexer::EXISTS_SYMBOL);
  }

  Identifier identifier = getIdentifier(scanner);
  result.first = identifier.first;
  if (!identifier.first.empty())
    schema = ensureSchemaExists(catalog, identifier.first, caseSensistive);

  table->name(identifier.second);
  table->oldName(identifier.second);

  // Special case: copy existing table.
  if ((scanner.is(MySQLLexer::OPEN_PAR_SYMBOL) && scanner.lookAhead() == MySQLLexer::LIKE_SYMBOL)
    || scanner.is(MySQLLexer::LIKE_SYMBOL))
  {
    scanner.next(scanner.is(MySQLLexer::OPEN_PAR_SYMBOL) ? 2 : 1);
    Identifier reference = getIdentifier(scanner);
    db_SchemaRef schema = catalog->defaultSchema();
    if (!reference.first.empty())
      schema = find_named_object_in_list(catalog->schemata(), reference.first);
    if (schema.is_valid())
    {
      db_TableRef otherTable = find_named_object_in_list(schema->tables(), reference.second);
      if (otherTable.is_valid())
      {
        bool isTemporary = table->isTemporary() != 0; // Value set already on the existing table and would get lost.
        table = grt::copy_object(db_mysql_TableRef::cast_from(otherTable));
        table->isTemporary(isTemporary);
      }
    }
  }
  else
  {
    // Note: we ignore here any part that uses a select/union clause.
    //       All of that is neither relevant for our table nor is it returned that way
    //       from the server.
    table->primaryKey(db_mysql_IndexRef());
    table->indices().remove_all();
    table->columns().remove_all();
    table->foreignKeys().remove_all();

    if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
    {
      scanner.next();
      if (scanner.is(MySQLLexer::PARTITION_SYMBOL))
      {
        fillTablePartitioning(scanner, table);
        scanner.next(); // Skip CLOSE_PAR.

        // Ignore union clause. Since we are at the end of the table create statement
        // we don't need to skip anything else.
      }
      else
      {
        // Table create items.
        std::string schemaName = schema.is_valid() ? schema->name() : "";
        while (true)
        {
          fillTableCreateItem(scanner, catalog, schemaName, table, autoGenerateFkNames, refCache);
          if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
            break;
          scanner.next();
        }

        scanner.next(); // Skip CLOSE_PAR.

        fillTableCreateOptions(scanner, catalog, schema, table, caseSensistive);
        fillTablePartitioning(scanner, table);

        // table_creation_source ignored.
      }
    }
    else
    {
      fillTableCreateOptions(scanner, catalog, schema, table, caseSensistive);
      fillTablePartitioning(scanner, table);

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
void resolveReferences(db_mysql_CatalogRef catalog, DbObjectsRefsCache &refCache, bool caseSensitive)
{
  grt::ListRef<db_mysql_Schema> schemata = catalog->schemata();

  for (DbObjectsRefsCache::iterator refIt = refCache.begin(); refIt != refCache.end(); ++refIt)
  {
    DbObjectReferences references = (*refIt);
    // Referenced table. Only used for foreign keys.
    db_mysql_TableRef referencedTable;
    if (references.type != DbObjectReferences::Index)
    {
      db_mysql_SchemaRef schema = find_named_object_in_list(schemata, references.targetIdentifier.first, caseSensitive);
      if (!schema.is_valid()) // Implicitly create the schema if we reference one not yet created.
        schema = ensureSchemaExists(catalog, references.targetIdentifier.first, caseSensitive);

      referencedTable = find_named_object_in_list(schema->tables(), references.targetIdentifier.second, caseSensitive);
      if (!referencedTable.is_valid())
      {
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

      if (references.table.is_valid() && !references.table->tableEngine().empty()
        && referencedTable->tableEngine().empty())
        referencedTable->tableEngine(references.table->tableEngine());
    }

    // Resolve columns.
    switch (references.type)
    {
    case DbObjectReferences::Index:
    {
      // Filling column references for an index.
      for (grt::ListRef<db_IndexColumn>::const_iterator indexIt = references.index->columns().begin(); indexIt != references.index->columns().end(); ++indexIt)
      {
        db_mysql_ColumnRef column = find_named_object_in_list(references.table->columns(), (*indexIt)->name(), false);

        // Reset name field to avoid unnecessary trouble with test code.
        (*indexIt)->name("");
        if (column.is_valid())
          (*indexIt)->referencedColumn(column);
      }
      break;
    }

    case DbObjectReferences::Referencing:
    {
      // Filling column references for the referencing table.
      for (std::vector<std::string>::iterator nameIt = references.columnNames.begin(); nameIt != references.columnNames.end(); ++nameIt)
      {
        db_mysql_ColumnRef column = find_named_object_in_list(references.table->columns(), *nameIt, false);
        if (column.is_valid())
          references.foreignKey->columns().insert(column);
      }
      break;
    }

    case DbObjectReferences::Referenced:
    {
      // Column references for the referenced table.
      int columnIndex = 0;

      for (std::vector<std::string>::iterator columnNameIt = references.columnNames.begin(); columnNameIt != references.columnNames.end(); ++columnNameIt)
      {
        db_mysql_ColumnRef column = find_named_object_in_list(referencedTable->columns(), *columnNameIt, false); // MySQL columns are always case-insensitive.

        if (!column.is_valid())
        {
          if (referencedTable->isStub())
          {
            column = db_mysql_ColumnRef(grt::Initialized);
            column->owner(referencedTable);
            column->name(*columnNameIt);
            column->oldName(*columnNameIt);

            // For the stub column we use all the data type settings from the foreign key column.
            db_mysql_ColumnRef templateColumn = db_mysql_ColumnRef::cast_from(references.foreignKey->columns().get(columnIndex));
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

            for (grt::StringListRef::const_iterator flagIt = templateColumn->flags().begin(); flagIt != templateColumn->flags().end(); ++flagIt)
              flags.insert(*flagIt);

            column->characterSetName(templateColumn->characterSetName());
            column->collationName(templateColumn->collationName());

            referencedTable->columns().insert(column);
            references.foreignKey->referencedColumns().insert(column);
          }
          else
          {
            // Column not found in a non-stub table. We only add stub columns to stub tables.
            references.table->foreignKeys().gremove_value(references.foreignKey);
            break; // No need to check other columns. That FK is done.
          }
        }
        else
          references.foreignKey->referencedColumns().insert(column);

        ++columnIndex;
      }

      // Once done with adding all referenced columns add an index for the foreign key if it doesn't exist yet.
      // 
      // Don't add an index if there are no FK columns, however.
      // MySQLLexer::TODO: Review this. There is no reason why we shouldn't create an index in this case.
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
      for (grt::ListRef<db_mysql_Index>::const_iterator indexIt = references.table->indices().begin(); indexIt != references.table->indices().end(); ++indexIt)
      {
        ListRef<db_IndexColumn> indexColumns = (*indexIt)->columns();

        bool indexMatches = true;

        // Go over all FK columns (not the index columns as they might differ).
        // Check that all FK columns are at the beginning of the index, in the same order.
        for (size_t i = 0; i < fkColumns->count(); ++i)
        {
          if (i >= indexColumns->count() || fkColumns->get(i) != indexColumns.get(i)->referencedColumn())
          {
            indexMatches = false;
            break;
          }
        }

        if (indexMatches)
        {
          foundIndex = *indexIt;
          break;
        }
      }

      if (foundIndex.is_valid())
      {
        if ((*foundIndex->indexType()).empty())
          foundIndex->indexType("INDEX");
        references.foreignKey->index(foundIndex);
      }
      else
      {
        // No valid index found, so create a new one.
        db_mysql_IndexRef index(grt::Initialized);
        index->owner(references.table);
        index->name(references.foreignKey->name());
        index->oldName(index->name());
        index->indexType("INDEX");
        references.foreignKey->index(index);

        for (ListRef<db_Column>::const_iterator columnIt = fkColumns.begin(); columnIt != fkColumns.end(); ++columnIt)
        {
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

class TableParseListener : public MySQLParserBaseListener
{
public:
  db_mysql_TableRef table;
  db_mysql_CatalogRef catalog;
  db_mysql_SchemaRef schema;
  DbObjectsRefsCache &refCache;

  TableParseListener(db_mysql_TableRef table, DbObjectsRefsCache &refCache) : table(table), refCache(refCache)
  {
    schema = db_mysql_SchemaRef::cast_from(table->owner());
    catalog = db_mysql_CatalogRef::cast_from(schema->owner());
  }

};

/**
 * Parses all values defined by the sql into the given table.
 * In opposition to other parse functions we pass the target object in by reference because it is possible that
 * the sql contains a LIKE clause (e.g. "create table a like b") which requires to duplicate the
 * referenced table and hence replace the inner value of the passed in table reference.
 */
size_t MySQLParserServicesImpl::parseTable(MySQLParserContext::Ref context,
  db_mysql_TableRef table, const std::string &sql)
{
  if (!table.is_valid())
  {
    logError("Invalid table storage used for parsing table SQL.\n");
    return 1;
  }

  logDebug2("Parse table\n");

  table->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  if (impl == nullptr)
    return 1;

  DbObjectsRefsCache refCache;
  TableParseListener listener(table, refCache);
  impl->parse(sql, MySQLParseUnit::PuCreateTable, &listener);
  if (impl->errors.empty())
  {
    //fillTableDetails(scanner, catalog, schema, table, context->caseSensitive, true, refCache);
    resolveReferences(listener.catalog, refCache, impl->caseSensitive);
  }
  else
  {
    // Finished with errors. See if we can get at least the table name out.
    /* XXX:
    Scanner scanner = impl->createScanner();
    if (scanner.advanceToType(MySQLLexer::TABLE_NAME_TOKEN, true))
    {
      Identifier identifier = getIdentifier(scanner);
      table->name(identifier.second + "_SYNTAX_ERROR");
    }
     */
  }

  return impl->errors.size();
}

//--------------------------------------------------------------------------------------------------

std::pair<std::string, std::string> fillTriggerDetails(Scanner &scanner, db_mysql_TriggerRef trigger)
{
  // There's no need for checks if any of the scanner calls fail.
  // If we arrive here the syntax must be correct.
  trigger->enabled(1);

  scanner.next(); // Skip CREATE.
  trigger->definer(getDefiner(scanner));
  scanner.next();
  Identifier identifier = getIdentifier(scanner);
  trigger->name(identifier.second); // We store triggers relative to the tables they act on,
  // so we ignore here any qualifying schema.
  trigger->oldName(trigger->name());

  trigger->timing(scanner.tokenText());
  scanner.next();
  trigger->event(scanner.tokenText());
  scanner.next();

  // The referenced table is not stored in the trigger object as that is defined by it's position
  // in the grt tree. But we return schema + table to aid further processing.
  scanner.next(); // Skip ON_SYMBOL.
  identifier = getIdentifier(scanner);

  scanner.skipTokenSequence(MySQLLexer::FOR_SYMBOL, MySQLLexer::EACH_SYMBOL, MySQLLexer::ROW_SYMBOL, 0);
  size_t type = scanner.tokenType();
  if (type == MySQLLexer::FOLLOWS_SYMBOL || type == MySQLLexer::PRECEDES_SYMBOL)
  {
    trigger->ordering(scanner.tokenText());
    scanner.next();
    trigger->otherTrigger(scanner.tokenText());
    scanner.next();
  }

  return identifier;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseTriggerSql(parser_ContextReferenceRef context_ref,
  db_mysql_TriggerRef trigger, const std::string &sql)
{
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseTrigger(context, trigger, sql);
}

//--------------------------------------------------------------------------------------------------

/**
* Parses the given sql as trigger create script and fills all found details in the given trigger ref.
* If there's an error nothing is changed.
* Returns the number of errors.
*/
size_t MySQLParserServicesImpl::parseTrigger(MySQLParserContext::Ref context, db_mysql_TriggerRef trigger,
  const std::string &sql)
{
  logDebug2("Parse trigger\n");

  trigger->sqlDefinition(base::trim(sql));
  trigger->lastChangeDate(base::fmttime(0, DATETIME_FMT));
/* XXX:
  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateTrigger);
  size_t error_count = context->recognizer()->error_info().size();
  int result_flag = 0;
  Scanner scanner = context->recognizer()->tree_scanner();
  if (error_count == 0)
    fillTriggerDetails(scanner, trigger);
  else
  {
    result_flag = 1;

    // Finished with errors. See if we can get at least the trigger name out.
    if (scanner.advanceToType(MySQLLexer::TRIGGER_NAME_TOKEN, true))
    {
      Identifier identifier = getIdentifier(scanner);
      trigger->name(identifier.second);
      trigger->oldName(trigger->name());
    }

    // Another attempt: find the ordering as we may need to manipulate this.
    if (scanner.advanceToType(MySQLLexer::ROW_SYMBOL, true))
    {
      scanner.next();
      if (scanner.is(MySQLLexer::FOLLOWS_SYMBOL) || scanner.is(MySQLLexer::PRECEDES_SYMBOL))
      {
        trigger->ordering(scanner.tokenText());
        scanner.next();
        if (scanner.isIdentifier())
        {
          trigger->otherTrigger(scanner.tokenText());
          scanner.next();
        }
      }
    }
  }

  trigger->modelOnly(result_flag);
  if (trigger->owner().is_valid())
  {
    // MySQLLexer::TODO: this is modeled after the old parser code but doesn't make much sense this way.
    //       There's only one flag for all triggers. So, at least there should be a scan over all triggers
    //       when determining this flag.
    db_TableRef table = db_TableRef::cast_from(trigger->owner());
    if (result_flag == 1)
      table->customData().set("triggerInvalid", grt::IntegerRef(1));
    else
      table->customData().remove("triggerInvalid");
  }
  return error_count;
 */
  return 1;
}

//--------------------------------------------------------------------------------------------------

/**
*	Returns schema name and ignore flag.
*/
std::pair<std::string, bool> fillViewDetails(Scanner &scanner, db_mysql_ViewRef view)
{
  scanner.next(); // Skip CREATE.

  std::pair<std::string, bool> result("", scanner.is(MySQLLexer::OR_SYMBOL)); // OR REPLACE
  scanner.skipIf(MySQLLexer::OR_SYMBOL, 2);

  if (scanner.is(MySQLLexer::ALGORITHM_SYMBOL))
  {
    scanner.next(2); // ALGORITHM and EQUAL.
    switch (scanner.tokenType())
    {
    case MySQLLexer::MERGE_SYMBOL:
      view->algorithm(1);
      break;
    case MySQLLexer::TEMPTABLE_SYMBOL:
      view->algorithm(2);
      break;
    default:
      view->algorithm(0);
      break;
    }
    scanner.next();
  }
  else
    view->algorithm(0);

  view->definer(getDefiner(scanner));

  scanner.skipIf(MySQLLexer::SQL_SYMBOL, 3); // SQL SECURITY (DEFINER | INVOKER)

  scanner.next(1); // Skip VIEW.
  Identifier identifier = getIdentifier(scanner);
  result.first = identifier.first;

  view->name(identifier.second);
  view->oldName(view->name());

  // Skip over the column list, if given. We don't use it atm.
  if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
    getNamesList(scanner);
  scanner.next(); // Skip AS.
  // XXX: scanner.skipSubtree(); // Skip SELECT subtree.

  view->withCheckCondition(scanner.is(MySQLLexer::WITH_SYMBOL));

  view->modelOnly(0);

  return result;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseViewSql(parser_ContextReferenceRef context_ref,
  db_mysql_ViewRef view, const std::string &sql)
{
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseView(context, view, sql);
}

//--------------------------------------------------------------------------------------------------

/**
* Parses the given sql as a create view script and fills all found details in the given view ref.
* If there's an error nothing changes. If the sql contains a schema reference other than that the
* the view is in the view's name will be changed (adds _WRONG_SCHEMA) to indicate that.
*/
size_t MySQLParserServicesImpl::parseView(MySQLParserContext::Ref context,
  db_mysql_ViewRef view, const std::string &sql)
{
  logDebug2("Parse view\n");
/* XXX:
  view->sqlDefinition(base::trim(sql));
  view->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateView);
  size_t error_count = context->recognizer()->error_info().size();
  Scanner scanner = context->recognizer()->tree_scanner();
  if (error_count == 0)
  {
    db_mysql_SchemaRef schema;
    if (view->owner().is_valid())
      schema = db_mysql_SchemaRef::cast_from(view->owner());
    std::pair<std::string, bool> info = fillViewDetails(scanner, view);
    if (!info.first.empty() && schema.is_valid())
    {
      if (!base::same_string(schema->name(), info.first, context->case_sensitive()))
      {
        view->name(*view->name() + "_WRONG_SCHEMA");
        view->oldName(view->name());
      }
    }

  }
  else
  {
    // Finished with errors. See if we can get at least the view name out.
    if (scanner.advanceToType(MySQLLexer::VIEW_NAME_TOKEN, true))
    {
      Identifier identifier = getIdentifier(scanner);
      view->name(identifier.second);
      view->oldName(view->name());
    }
    view->modelOnly(1);
  }

  return error_count;
 */
  return 1;
}

//--------------------------------------------------------------------------------------------------

std::string fillRoutineDetails(Scanner &scanner, db_mysql_RoutineRef routine)
{
  scanner.next(); // Skip CREATE.
  routine->definer(getDefiner(scanner));

  // A UDF is also a function and will be handled as such here.
  scanner.skipIf(MySQLLexer::AGGREGATE_SYMBOL);
  if (scanner.is(MySQLLexer::FUNCTION_SYMBOL))
    routine->routineType("function");
  else
    routine->routineType("procedure");

  scanner.next(1); // Skip FUNCTION/PROCEDURE.
  Identifier identifier = getIdentifier(scanner);

  routine->name(identifier.second);
  routine->oldName(routine->name());

  if (scanner.is(MySQLLexer::RETURNS_SYMBOL))
  {
    // UDF.
    routine->routineType("udf");
    scanner.next();
    routine->returnDatatype(scanner.tokenText());

    // SONAME is currently ignored.
  }
  else
  {
    // Parameters.
    ListRef<db_mysql_RoutineParam> params = routine->params();
    params.remove_all();
    scanner.next(); // Skip OPEN_PAR.

    while (!scanner.is(MySQLLexer::CLOSE_PAR_SYMBOL))
    {
      db_mysql_RoutineParamRef param(grt::Initialized);
      param->owner(routine);

      switch (scanner.tokenType())
      {
      case MySQLLexer::IN_SYMBOL:
      case MySQLLexer::OUT_SYMBOL:
      case MySQLLexer::INOUT_SYMBOL:
        param->paramType(scanner.tokenText());
        scanner.next();
        break;
      }

      param->name(scanner.tokenText());
      scanner.next();

      // DATA_TYPE_TOKEN.
      // XXX: param->datatype(scanner.textForTree());
      params.insert(param);

      // XXX: scanner.skipSubtree();
      if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
        break;
      scanner.next();
    }
    scanner.next(); // Skip CLOSE_PAR.

    if (scanner.is(MySQLLexer::RETURNS_SYMBOL))
    {
      scanner.next();

      // DATA_TYPE_TOKEN.
      // XXX: routine->returnDatatype(scanner.textForTree());
      scanner.next();
    }

    // XXX: if (scanner.is(MySQLLexer::ROUTINE_CREATE_OPTIONS))
    {
      // For now we only store comments and security settings.
      scanner.next();
      bool done = false;
      do
      {
        switch (scanner.tokenType())
        {
        case MySQLLexer::SQL_SYMBOL:
          scanner.next(2); // Skip SQL + SECURITY (both are siblings)
          routine->security(scanner.tokenText());
          scanner.next(); // Skip DEFINER/INVOKER
          break;

        case MySQLLexer::COMMENT_SYMBOL:
          scanner.next();
          routine->comment(scanner.tokenText());
          // XXX: scanner.skipSubtree();
          break;

          // Some options we just skip.
        case MySQLLexer::NOT_SYMBOL:
        case MySQLLexer::DETERMINISTIC_SYMBOL:
          scanner.next(scanner.is(MySQLLexer::NOT_SYMBOL) ? 2 : 1);
          break;

        case MySQLLexer::CONTAINS_SYMBOL:
        case MySQLLexer::LANGUAGE_SYMBOL:
        case MySQLLexer::NO_SYMBOL:
          scanner.next(2);
          break;

        case MySQLLexer::READS_SYMBOL:
        case MySQLLexer::MODIFIES_SYMBOL:
          scanner.next(3);
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
std::pair<std::string, std::string> getRoutineNameAndType(MySQLParserContext::Ref context,
  const std::string &sql)
{
  std::pair<std::string, std::string> result = std::make_pair("unknown", "unknown");
  /* XXX:
  std::shared_ptr<MySQLScanner> scanner = context->createScanner(sql);

  if (scanner->token_type() != CREATE_SYMBOL)
    return result;

  // Scan definer clause. Handling here is similar to getDefiner() only that we deal here with
  // potentially wrong syntax, have a scanner instead of the scanner and don't need the definer value.
  scanner->next();
  if (scanner->is(MySQLLexer::DEFINER_SYMBOL))
  {
    scanner->next();
    scanner->skipIf(MySQLLexer::EQUAL_OPERATOR);

    if (scanner->is(MySQLLexer::CURRENT_USER_SYMBOL))
    {
      scanner->next();
      if (scanner->skipIf(MySQLLexer::OPEN_PAR_SYMBOL))
        scanner->skipIf(MySQLLexer::CLOSE_PAR_SYMBOL);
    }
    else
    {
      // A user@host entry.
      if (scanner->is_identifier() || scanner->is(MySQLLexer::SINGLE_QUOTED_TEXT))
        scanner->next();
      switch (scanner->token_type())
      {
      case MySQLLexer::AT_TEXT_SUFFIX:
        scanner->next();
        break;
      case MySQLLexer::AT_SIGN_SYMBOL:
        scanner->next();
        if (scanner->is_identifier() || scanner->is(MySQLLexer::SINGLE_QUOTED_TEXT))
          scanner->next();
        break;
      }
    }
  }

  scanner->skipIf(MySQLLexer::AGGREGATE_SYMBOL);
  switch (scanner->token_type())
  {
  case MySQLLexer::PROCEDURE_SYMBOL:
    result.second = "procedure";
    scanner->next();
    break;

  case MySQLLexer::FUNCTION_SYMBOL: // Both normal function and UDF.
    result.second = "function";
    scanner->next();
    break;
  }

  if (scanner->is_identifier())
  {
    result.first = base::unquote(scanner->token_text());
    scanner->next();
    if (scanner->skipIf(MySQLLexer::DOT_SYMBOL))
    {
      // Qualified identifier.
      if (scanner->is_identifier())
        result.first = base::unquote(scanner->token_text());
    }
  }

  if (scanner->is(MySQLLexer::RETURNS_SYMBOL))
    result.second = "udf";
*/
  return result;

}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseRoutineSql(parser_ContextReferenceRef context_ref,
  db_mysql_RoutineRef routine, const std::string &sql)
{
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseRoutine(context, routine, sql);
}

//--------------------------------------------------------------------------------------------------

/**
* Parses the given sql as a create function/procedure script and fills all found details in the given routine ref.
* If there's an error nothing changes. If the sql contains a schema reference other than that the
* the routine is in the routine's name will be changed (adds _WRONG_SCHEMA) to indicate that.
*/
size_t MySQLParserServicesImpl::parseRoutine(MySQLParserContext::Ref context,
  db_mysql_RoutineRef routine, const std::string &sql)
{
  logDebug2("Parse routine\n");
/* XXX:
  routine->sqlDefinition(base::trim(sql));
  routine->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateRoutine);
  Scanner scanner = context->recognizer()->tree_scanner();
  size_t error_count = context->recognizer()->error_info().size();
  if (error_count == 0)
  {
    std::string schemaName = fillRoutineDetails(scanner, routine);
    if (!schemaName.empty() && routine->owner().is_valid())
    {
      db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(routine->owner());
      if (!base::same_string(schema->name(), schemaName, false)) // Routine names are never case sensitive.
      {
        routine->name(*routine->name() + "_WRONG_SCHEMA");
        routine->oldName(routine->name());
      }
    }
  }
  else
  {
    // Finished with errors. See if we can get at least the routine name out.
    std::pair<std::string, std::string> values = getRoutineNameAndType(context, sql);
    routine->name(values.first + "_SYNTAX_ERROR");
    routine->routineType(values.second);

    routine->modelOnly(1);
  }

  return error_count;
 */
  return 1;
}

//--------------------------------------------------------------------------------------------------

bool consider_as_same_type(std::string type1, std::string type2)
{
  if (type1 == type2)
    return true;

  if (type1 == "function" && type2 == "udf")
    return true;

  if (type2 == "function" && type1 == "udf")
    return true;

  return false;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseRoutinesSql(parser_ContextReferenceRef context_ref,
  db_mysql_RoutineGroupRef group, const std::string &sql)
{
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
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
size_t MySQLParserServicesImpl::parseRoutines(MySQLParserContext::Ref context,
  db_mysql_RoutineGroupRef group, const std::string &sql)
{
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  if (impl == nullptr)
  {
    logError("Invalid parser context passed in SQL.\n");
    return 1;
  }

  logDebug2("Parse routine group\n");

  size_t error_count = 0;

  std::vector<std::pair<size_t, size_t> > ranges;
  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges, "\n");

  grt::ListRef<db_Routine> routines = group->routines();
  routines.remove_all();

  db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(group->owner());
  grt::ListRef<db_Routine> schema_routines = schema->routines();

  // XXX: int sequence_number = 0;
  int syntax_error_counter = 1;

  for (std::vector<std::pair<size_t, size_t> >::iterator iterator = ranges.begin(); iterator != ranges.end(); ++iterator)
  {
    std::string routineSQL = sql.substr(iterator->first, iterator->second);
    // XXX: context->recognizer()->parse(sql.c_str() + iterator->first, iterator->second, true, MySQLParseUnit::PuCreateRoutine);
    size_t local_error_count = 0; // XXX: context->recognizer()->error_info().size();
    error_count += local_error_count;

    // Before filling a routine we need to know if there's already one with that name in the schema.
    // Hence we first extract the name and act based on that.
    Scanner scanner(&impl->tokens); // XXX = context->recognizer()->tree_scanner();
    std::pair<std::string, std::string> values = getRoutineNameAndType(context, routineSQL);

    // If there's no usable info from parsing preserve at least the code and generate a
    // name for the routine using a counter.
    if (values.first == "unknown" || values.second == "unknown")
    {
      // Create a new routine instance.
      db_mysql_RoutineRef routine = db_mysql_RoutineRef(grt::Initialized);
      routine->createDate(base::fmttime(0, DATETIME_FMT));
      routine->lastChangeDate(routine->createDate());
      routine->owner(schema);
      schema_routines.insert(routine);

      routine->name(*group->name() + "_SYNTAX_ERROR_" + base::to_string(syntax_error_counter++));
      routine->routineType("unknown");
      routine->modelOnly(1);
      routine->sqlDefinition(base::trim(routineSQL));

      routines.insert(routine);
    }
    else
    {
      db_mysql_RoutineRef routine;
      for (size_t i = 0; i < schema_routines.count(); ++i)
      {
        // Stored functions and UDFs share the same namespace.
        // Stored routine names are not case sensitive.
        db_RoutineRef candidate = schema_routines[i];
        std::string name = candidate->name();

        // Remove automatically added appendixes before comparing names.
        if (base::hasSuffix(name, "_WRONG_SCHEMA"))
          name.resize(name.size() - 13);
        if (base::hasSuffix(name, "_SYNTAX_ERROR"))
          name.resize(name.size() - 13);

        if (base::same_string(values.first, name, false) && consider_as_same_type(values.second, candidate->routineType()))
        {
          routine = db_mysql_RoutineRef::cast_from(candidate);
          break;
        }
      }

      scanner.reset();
      if (!routine.is_valid())
      {
        // Create a new routine instance.
        routine = db_mysql_RoutineRef(grt::Initialized);
        routine->createDate(base::fmttime(0, DATETIME_FMT));
        routine->owner(schema);
        schema_routines.insert(routine);
      }

      if (local_error_count == 0)
        fillRoutineDetails(scanner, routine);
      else
      {
        routine->name(values.first + "_SYNTAX_ERROR");
        routine->routineType(values.second);

        routine->modelOnly(1);
      }

      routine->sqlDefinition(base::trim(routineSQL));
      routine->lastChangeDate(base::fmttime(0, DATETIME_FMT));

      // Finally add the routine to the group if it isn't already there.
      bool found = false;
      for (size_t i = 0; i < routines.count(); ++i)
      {
        if (base::same_string(routine->name(), routines[i]->name(), false))
        {
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

static void fillSchemaOptions(Scanner &scanner, db_mysql_CatalogRef catalog,
  db_mysql_SchemaRef schema)
{
  std::string defaultCharset = catalog.is_valid() ? catalog->defaultCharacterSetName() : "";
  std::string defaultCollation = catalog.is_valid() ? catalog->defaultCollationName() : "";

  // Charset or collation info.
  bool done = false;
  while (!done)
  {
    scanner.skipIf(MySQLLexer::DEFAULT_SYMBOL);
    switch (scanner.tokenType())
    {
    case MySQLLexer::CHAR_SYMBOL: // CHARACTER is mapped to CHAR.
    case MySQLLexer::CHARSET_SYMBOL:
    {
      std::pair<std::string, std::string> info = detailsForCharset(getCharsetName(scanner),
        defaultCollation, defaultCharset);
      schema->defaultCharacterSetName(info.first);
      schema->defaultCollationName(info.second);
      scanner.next();
      break;
    }

    case MySQLLexer::COLLATE_SYMBOL:
    {
      scanner.next();
      scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);

      std::pair<std::string, std::string> info = detailsForCollation(scanner.tokenText(), defaultCollation);
      schema->defaultCharacterSetName(info.first);
      schema->defaultCollationName(info.second);
      scanner.next();
      break;
    }

    default:
      done = true;
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

static bool fillSchemaDetails(Scanner &scanner, db_mysql_CatalogRef catalog,
  db_mysql_SchemaRef schema)
{
  bool ignoreIfExists = false;

  scanner.next(2); // Skip CREATE SCHEMA.
  if (scanner.is(MySQLLexer::IF_SYMBOL))
  {
    ignoreIfExists = true;
    scanner.next(3); // Skip IF NOT EXISTS.
  }
  Identifier identifier = getIdentifier(scanner);
  schema->name(identifier.second);
  schema->oldName(schema->name());

  fillSchemaOptions(scanner, catalog, schema);

  return ignoreIfExists;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseSchema(MySQLParserContext::Ref context, db_mysql_SchemaRef schema,
  const std::string &sql)
{
  logDebug2("Parse schema\n");
/* XXX:
  schema->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuGeneric);
  size_t error_count = context->recognizer()->error_info().size();
  Scanner scanner = context->recognizer()->tree_scanner();
  if (error_count == 0)
    fillSchemaDetails(scanner, db_mysql_CatalogRef::cast_from(schema->owner()), schema);
  else
  {
    // Finished with errors. See if we can get at least the schema name out.
    if (scanner.advanceToType(MySQLLexer::SCHEMA_NAME_TOKEN, true))
    {
      Identifier identifier = getIdentifier(scanner);
      schema->name(identifier.second + "_SYNTAX_ERROR");
    }
  }

  return error_count;
 */
  return 1;
}

//--------------------------------------------------------------------------------------------------

Identifier fillIndexDetails(Scanner &scanner, db_mysql_CatalogRef catalog,
  db_mysql_SchemaRef schema, db_mysql_IndexRef index, bool caseSensitiv)
{
  scanner.next(); // Skip CREATE.
  if (scanner.is(MySQLLexer::ONLINE_SYMBOL) || scanner.is(MySQLLexer::OFFLINE_SYMBOL))
    scanner.next();

  index->unique(0);
  switch (scanner.tokenType())
  {
  case MySQLLexer::UNIQUE_SYMBOL:
  case MySQLLexer::INDEX_SYMBOL:
    if (scanner.is(MySQLLexer::UNIQUE_SYMBOL))
    {
      index->unique(1);
      index->indexType("UNIQUE");
      scanner.next();
    }
    else
      index->indexType(formatIndexType(scanner.tokenText()));

    scanner.next();
    break;

  case MySQLLexer::FULLTEXT_SYMBOL:
  case MySQLLexer::SPATIAL_SYMBOL:
    index->indexType(formatIndexType(scanner.tokenText()));
    scanner.next(2); // Skip FULLTEXT/SPATIAL INDEX.
    break;
  }

  Identifier identifier = getIdentifier(scanner);
  index->name(identifier.second);
  index->oldName(index->name());

  if (scanner.is(MySQLLexer::USING_SYMBOL) || scanner.is(MySQLLexer::TYPE_SYMBOL))
  {
    scanner.next();
    index->indexKind(scanner.tokenText());
    scanner.next();
  }

  scanner.next(); // Skip ON.
  identifier = getIdentifier(scanner);

  // Index columns.
  // Note: the referenced column for an index column can only be set if we can find the table
  //       for it (via the catalog reference).
  db_TableRef table;
  if (catalog.is_valid())
  {
    if (!identifier.first.empty())
      schema = find_named_object_in_list(catalog->schemata(), identifier.first, caseSensitiv);
    if (schema.is_valid())
      table = find_named_object_in_list(schema->tables(), identifier.second, caseSensitiv);

    // As last resort, check the owner, if we haven't found a table yet.
    if (!table.is_valid() && index->owner().is_valid())
      table = db_TableRef::cast_from(index->owner());
  }

  fillIndexColumns(scanner, table, index);
  fillIndexOptions(scanner, index);

  bool done = false;
  while (!done)
  {
    // Actually only one occurrence of each. But in any order.
    switch (scanner.tokenType())
    {
    case MySQLLexer::ALGORITHM_SYMBOL:
    {
      scanner.next();
      if (scanner.is(MySQLLexer::EQUAL_OPERATOR))
        scanner.next();

      // The algorithm can be any text, but allowed are only a small number of values.
      std::string algorithm = base::toupper(scanner.tokenText());
      if (algorithm == "DEFAULT" || algorithm == "INPLACE" || algorithm == "COPY")
        index->algorithm(algorithm);

      break;
    }

    case MySQLLexer::LOCK_SYMBOL:
    {
      scanner.next();
      if (scanner.is(MySQLLexer::EQUAL_OPERATOR))
        scanner.next();

      // The lock type can be any text, but allowed are only a small number of values.
      std::string lock = base::toupper(scanner.tokenText());
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

size_t MySQLParserServicesImpl::parseIndex(MySQLParserContext::Ref context, db_mysql_IndexRef index,
  const std::string &sql)
{
  logDebug2("Parse index\n");

  /* XXX:
  index->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateIndex);
  size_t error_count = context->recognizer()->error_info().size();
  Scanner scanner = context->recognizer()->tree_scanner();
  if (error_count == 0)
  {
    db_mysql_CatalogRef catalog;
    db_mysql_SchemaRef schema;

    if (index->owner().is_valid())
    {
      schema = db_mysql_SchemaRef::cast_from(index->owner()->owner());
      catalog = db_mysql_CatalogRef::cast_from(schema->owner());
    }
    fillIndexDetails(scanner, catalog, schema, index, context->case_sensitive());
  }
  else
  {
    // Finished with errors. See if we can get at least the index name out.
    if (scanner.advanceToType(MySQLLexer::INDEX_NAME_TOKEN, true))
    {
      Identifier identifier = getIdentifier(scanner);
      index->name(identifier.second + "_SYNTAX_ERROR");
    }
  }

  return error_count;
   */
  return 1;
}

//--------------------------------------------------------------------------------------------------

/**
 *	Returns schema name and ignore flag.
 */
std::pair<std::string, bool> fillEventDetails(Scanner &scanner, db_mysql_EventRef event)
{
  std::pair<std::string, bool> result("", false);

  scanner.next(); // Skip CREATE.
  event->definer(getDefiner(scanner));
  scanner.next(); // Skip EVENT.

  if (scanner.is(MySQLLexer::IF_SYMBOL))
  {
    result.second = true;
    scanner.next(3); // Skip IF NOT EXISTS.
  }

  Identifier identifier = getIdentifier(scanner);
  result.first = identifier.first;
  event->name(identifier.second);
  event->oldName(event->name());
  scanner.next(2); // Skip ON SCHEDULE.

  event->useInterval(scanner.tokenType() != MySQLLexer::AT_SYMBOL);
  if (event->useInterval())
  {
    scanner.next(); // Skip EVERY.
    // XXX: event->at(scanner.textForTree()); // The full expression subtree.
    // XXX: scanner.skipSubtree();
    event->intervalUnit(scanner.tokenText());
    scanner.next();

    if (scanner.is(MySQLLexer::STARTS_SYMBOL))
    {
      scanner.next();
      // XXX: event->intervalStart(scanner.textForTree());
      // XXX: scanner.skipSubtree();
    }

    if (scanner.is(MySQLLexer::ENDS_SYMBOL))
    {
      scanner.next();
      // XXX: event->intervalEnd(scanner.textForTree());
      // XXX: scanner.skipSubtree();
    }
  }
  else
  {
    scanner.next();
    // XXX: event->at(scanner.textForTree()); // The full expression subtree.
    // XXX: scanner.skipSubtree();
  }

  if (scanner.is(MySQLLexer::ON_SYMBOL)) // ON COMPLETION NOT? PRESERVE
  {
    scanner.next(2);
    event->preserved(!scanner.is(MySQLLexer::NOT_SYMBOL));
    scanner.next(event->preserved() ? 1 : 2);
  }

  bool enabled = true;
  if (scanner.is(MySQLLexer::ENABLE_SYMBOL) || scanner.is(MySQLLexer::DISABLE_SYMBOL))
  {
    // Enabled/Disabled is optional.
    enabled = scanner.is(MySQLLexer::ENABLE_SYMBOL);
    scanner.next();
    if (scanner.is(MySQLLexer::ON_SYMBOL))
      scanner.next(2); // Skip ON SLAVE.
  }
  event->enabled(enabled);

  if (scanner.is(MySQLLexer::COMMENT_SYMBOL))
  {
    scanner.next();
    event->comment(scanner.tokenText());
    // XXX: scanner.skipSubtree();
  }
  scanner.next(); // Skip DO.

  return result;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseEvent(MySQLParserContext::Ref context, db_mysql_EventRef event,
  const std::string &sql)
{
  logDebug2("Parse event\n");
/* XXX:
  event->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateEvent);
  size_t error_count = context->recognizer()->error_info().size();
  Scanner scanner = context->recognizer()->tree_scanner();
  if (error_count == 0)
    fillEventDetails(scanner, event);
  else
  {
    // Finished with errors. See if we can get at least the event name out.
    if (scanner.advanceToType(MySQLLexer::EVENT_NAME_TOKEN, true))
    {
      Identifier identifier = getIdentifier(scanner);
      event->name(identifier.second + "_SYNTAX_ERROR");
    }
  }

  return error_count;
 */
  return 1;
}

//--------------------------------------------------------------------------------------------------

void fillLogfileGroupDetails(Scanner &scanner, db_mysql_LogFileGroupRef group)
{
  scanner.next(3); // Skip CREATE LOGFILE GROUP.
  Identifier identifier = getIdentifier(scanner);
  group->name(identifier.second);
  group->oldName(group->name());

  scanner.next(2); // Skip ADD (UNDOFILE | REDOFILE).
  group->undoFile(scanner.tokenText());
  // XXX: scanner.skipSubtree();

  // XXX: if (scanner.is(MySQLLexer::LOGFILE_GROUP_OPTIONS_TOKEN))
  {
    scanner.next();
    bool done = false;
    while (!done)
    {
      // Unlimited occurrences.
      ssize_t token = scanner.tokenType();
      switch (token)
      {
      case MySQLLexer::INITIAL_SIZE_SYMBOL:
      case MySQLLexer::UNDO_BUFFER_SIZE_SYMBOL:
      case MySQLLexer::REDO_BUFFER_SIZE_SYMBOL:
      {
        scanner.next();
        scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
        std::string value = scanner.tokenText();
        scanner.next();

        // Value can have a suffix. And it can be a hex number (atoi is supposed to handle that).
        size_t factor = 1;
        switch (::tolower(value[value.size() - 1]))
        {
          // All cases fall through.
        case 'g':
          factor *= 1024;
        case 'm':
          factor *= 1024;
        case 'k':
          factor *= 1024;
          value[value.size() - 1] = 0;
        }
        if (token == MySQLLexer::INITIAL_SIZE_SYMBOL)
          group->initialSize(factor * base::atoi<size_t>(value));
        else
          group->undoBufferSize(factor * base::atoi<size_t>(value));

        break;
      }

      case MySQLLexer::NODEGROUP_SYMBOL:
        scanner.next();
        scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);

        // An integer or hex number (no suffix).
        group->nodeGroupId(base::atoi<size_t>(scanner.tokenText()));
        scanner.next();

        break;

      case MySQLLexer::WAIT_SYMBOL:
      case MySQLLexer::NO_WAIT_SYMBOL:
        group->wait(token == MySQLLexer::WAIT_SYMBOL);
        scanner.next();
        break;

      case MySQLLexer::COMMENT_SYMBOL:
        scanner.next();
        scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
        group->comment(scanner.tokenText());
          // XXX: scanner.skipSubtree();
        break;

      case MySQLLexer::STORAGE_SYMBOL:
      case MySQLLexer::ENGINE_SYMBOL:
        scanner.next(token == MySQLLexer::STORAGE_SYMBOL ? 2 : 1);
        scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
        scanner.next(); // Skip ENGINE_REF_TOKEN.
        group->engine(scanner.tokenText());
        scanner.next();
        break;

      default:
        done = true;
        break;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::parseLogfileGroup(MySQLParserContext::Ref context,
  db_mysql_LogFileGroupRef group, const std::string &sql)
{
  logDebug2("Parse logfile group\n");

  /* XXX:
  group->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateLogfileGroup);
  size_t error_count = context->recognizer()->error_info().size();
  Scanner scanner = context->recognizer()->tree_scanner();
  if (error_count == 0)
    fillLogfileGroupDetails(scanner, group);
  else
  {
    if (scanner.advanceToType(MySQLLexer::LOGFILE_GROUP_NAME_TOKEN, true))
    {
      scanner.next();
      std::string name = scanner.tokenText();
      group->name(name + "_SYNTAX_ERROR");
    }
  }

  return error_count;
   */
  return 1;
}

//--------------------------------------------------------------------------------------------------

void fillServerDetails(Scanner &scanner, db_mysql_ServerLinkRef server)
{
  scanner.next(2); // Skip CREATE SERVER.
  Identifier identifier = getIdentifier(scanner);
  server->name(identifier.second);
  server->oldName(server->name());

  scanner.next(3); // Skip FOREIGN DATA WRAPPER.
  server->wrapperName(scanner.tokenText());
  scanner.next(3); // Skip <name> OPTIONS OPEN_PAR_SYMBOL.

  while (true)
  {
    switch (scanner.tokenType())
    {
    case MySQLLexer::HOST_SYMBOL:
      scanner.next();
      server->host(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::DATABASE_SYMBOL:
      scanner.next();
      server->schema(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::USER_SYMBOL:
      scanner.next();
      server->user(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::PASSWORD_SYMBOL:
      scanner.next();
      server->password(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::SOCKET_SYMBOL:
      scanner.next();
      server->socket(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::OWNER_SYMBOL:
      scanner.next();
      server->ownerUser(scanner.tokenText());
      scanner.next();
      break;
    case MySQLLexer::PORT_SYMBOL:
      scanner.next();
      server->port(scanner.tokenText()); // The grt definition should be int not string...
      scanner.next();
      break;
    }

    if (scanner.is(MySQLLexer::CLOSE_PAR_SYMBOL))
      break;
    scanner.next(); // Skip comma.
  }
}

//--------------------------------------------------------------------------------------------------

class ServerParseListener : public MySQLParserBaseListener
{
public:
  db_mysql_ServerLinkRef server;

  ServerParseListener(db_mysql_ServerLinkRef server_) : server(server_) {}

  virtual void exitCreate_server_tail(MySQLParser::Create_server_tailContext *ctx) override
  {
    std::string name = ctx->server_name()->getText();
    server->name(name);
  }
  
};

size_t MySQLParserServicesImpl::parseServer(MySQLParserContext::Ref context,
  db_mysql_ServerLinkRef server, const std::string &sql)
{
  logDebug2("Parse server\n");

  /* XXX:
  server->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateServer);
  size_t error_count = context->recognizer()->error_info().size();
  Scanner scanner = context->recognizer()->tree_scanner();
  if (error_count == 0)
    fillServerDetails(scanner, server);
  else
  {
    if (scanner.advanceToType(MySQLLexer::LOGFILE_GROUP_NAME_TOKEN, true))
    {
      Identifier identifier = getIdentifier(scanner);
      server->name(identifier.second + "_SYNTAX_ERROR");
    }
  }

  return error_count;
   */
  return 1;
}

//--------------------------------------------------------------------------------------------------

void fillTablespaceDetails(Scanner &scanner, db_CatalogRef catalog,
  db_mysql_TablespaceRef tablespace)
{
  scanner.next(2); // Skip CREATE TABLESPACE.
  Identifier identifier = getIdentifier(scanner);
  tablespace->name(identifier.second);
  tablespace->oldName(tablespace->name());

  scanner.next(2); // Skip ADD DATAFILE.
  tablespace->dataFile(scanner.tokenText());
  // XXX: scanner.skipSubtree();

  if (scanner.is(MySQLLexer::USE_SYMBOL))
  {
    scanner.next(3); // Skip USE LOGFILE GROUP.
    Identifier identifier = getIdentifier(scanner);
    if (catalog.is_valid())
    {
      db_LogFileGroupRef logfileGroup = find_named_object_in_list(catalog->logFileGroups(), identifier.second);
      if (logfileGroup.is_valid())
        tablespace->logFileGroup(logfileGroup);
    }
  }

  // XXX: if (scanner.is(MySQLLexer::TABLESPACE_OPTIONS_TOKEN))
  {
    scanner.next();
    bool done = false;
    while (!done)
    {
      // Unlimited occurrences.
      ssize_t token = scanner.tokenType();
      switch (token)
      {
      case MySQLLexer::INITIAL_SIZE_SYMBOL:
      case MySQLLexer::AUTOEXTEND_SIZE_SYMBOL:
      case MySQLLexer::MAX_SIZE_SYMBOL:
      case MySQLLexer::EXTENT_SIZE_SYMBOL:
      {
        scanner.next();
        scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
        std::string value = scanner.tokenText();
        scanner.next();

        // Value can have a suffix. And it can be a hex number (atoi is supposed to handle that).
        size_t factor = 1;
        switch (::tolower(value[value.size() - 1]))
        {
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
        switch (token)
        {
        case MySQLLexer::INITIAL_SIZE_SYMBOL:
          tablespace->initialSize(number);
          break;
        case MySQLLexer::AUTOEXTEND_SIZE_SYMBOL:
          tablespace->autoExtendSize(number);
          break;
        case MySQLLexer::MAX_SIZE_SYMBOL:
          tablespace->maxSize(number);
          break;
        case MySQLLexer::EXTENT_SIZE_SYMBOL:
          tablespace->extentSize(number);
          break;
        }

        break;
      }

      case MySQLLexer::NODEGROUP_SYMBOL:
        scanner.next();
        scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);

        // An integer or hex number (no suffix).
        tablespace->nodeGroupId(base::atoi<size_t>(scanner.tokenText()));
        scanner.next();

        break;

      case MySQLLexer::WAIT_SYMBOL:
      case MySQLLexer::NO_WAIT_SYMBOL:
        tablespace->wait(token == MySQLLexer::WAIT_SYMBOL);
        scanner.next();
        break;

      case MySQLLexer::COMMENT_SYMBOL:
        scanner.next();
        scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
        tablespace->comment(scanner.tokenText());
          // XXX: scanner.skipSubtree();
        break;

      case MySQLLexer::STORAGE_SYMBOL:
      case MySQLLexer::ENGINE_SYMBOL:
      {
        scanner.next(token == MySQLLexer::STORAGE_SYMBOL ? 2 : 1);
        scanner.skipIf(MySQLLexer::EQUAL_OPERATOR);
        Identifier identifier = getIdentifier(scanner);
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

size_t MySQLParserServicesImpl::parseTablespace(MySQLParserContext::Ref context,
  db_mysql_TablespaceRef tablespace, const std::string &sql)
{
  logDebug2("Parse tablespace\n");

  /**
  tablespace->lastChangeDate(base::fmttime(0, DATETIME_FMT));

  context->recognizer()->parse(sql.c_str(), sql.length(), true, MySQLParseUnit::PuCreateServer);
  size_t error_count = context->recognizer()->error_info().size();
  Scanner scanner = context->recognizer()->tree_scanner();
  if (error_count == 0)
  {
    db_CatalogRef catalog;
    if (tablespace->owner().is_valid() && tablespace->owner()->owner().is_valid())
      catalog = db_CatalogRef::cast_from(tablespace->owner()->owner()->owner());
    fillTablespaceDetails(scanner, catalog, tablespace);
  }
  else
  {
    if (scanner.advanceToType(MySQLLexer::TABLESPACE_NAME_TOKEN, true))
    {
      Identifier identifier = getIdentifier(scanner);
      tablespace->name(identifier.second + "_SYNTAX_ERROR");
    }
  }

  return error_count;
   */
  return 1;
}

size_t MySQLParserServicesImpl::parseSQLIntoCatalogSql(parser_ContextReferenceRef context_ref, db_mysql_CatalogRef catalog,
          const std::string &sql, grt::DictRef options)
{
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseSQLIntoCatalog( context, catalog, sql, options);
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
size_t MySQLParserServicesImpl::parseSQLIntoCatalog(MySQLParserContext::Ref context,
  db_mysql_CatalogRef catalog, const std::string &sql, grt::DictRef options)
{
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  if (impl == nullptr)
  {
    logError("Invalid parser context passed in SQL.\n");
    return 1;
  }

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

  bool caseSensitive = impl->caseSensitive;

  std::string startSchema = options.get_string("schema");
  db_mysql_SchemaRef currentSchema;
  if (!startSchema.empty())
    currentSchema = ensureSchemaExists(catalog, startSchema, caseSensitive);

  bool defaultSchemaCreated = false;
  // XXX: bool autoGenerateFkNames = options.get_int("gen_fk_names_when_empty") != 0;
  //bool reuseExistingObjects = options.get_int("reuse_existing_objects") != 0;

  if (!currentSchema.is_valid())
  {
    currentSchema = db_mysql_SchemaRef::cast_from(catalog->defaultSchema());
    if (!currentSchema.is_valid())
    {
      db_SchemaRef df = find_named_object_in_list(catalog->schemata(), "default_schema", caseSensitive);
      if (!df.is_valid())
        defaultSchemaCreated = true;
      currentSchema = ensureSchemaExists(catalog, "default_schema", caseSensitive);
    }
  }

  size_t errorCount = 0;
  // XXX: std::shared_ptr<MySQLQueryIdentifier> queryIdentifier = context->createQueryIdentifier();

  std::vector<std::pair<size_t, size_t>> ranges;
  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges, "\n");

  grt::ListRef<GrtObject> createdObjects = grt::ListRef<GrtObject>::cast_from(options.get("created_objects"));
  if (!createdObjects.is_valid())
  {
    createdObjects = grt::ListRef<GrtObject>(grt::Initialized);
    options.set("created_objects", createdObjects);
  }
/* XXX:
  // Collect textual FK references into a local cache. At the end this is used
  // to find actual ref tables + columns, when all tables have been parsed.
  DbObjectsRefsCache refCache;
  for (std::vector<std::pair<size_t, size_t> >::iterator iterator = ranges.begin(); iterator != ranges.end(); ++iterator)
  {
    //std::string ddl(sql.c_str() + iterator->first, iterator->second);
    MySQLQueryType queryType = queryIdentifier->getQueryType(sql.c_str() + iterator->first, iterator->second, true);
    size_t errors = queryIdentifier->error_info().size(); // Can only be lexer errors.
    if (errors > 0)
    {
      errorCount += errors;
      continue;
    }

    if (relevantQueryTypes.count(queryType) == 0)
      continue; // Something we are not interested in. Don't bother parsing it.

    // XXX: impl->parse(sql.c_str() + iterator->first, iterator->second, MySQLParseUnit::PuGeneric);
    // XXX: errors = recognizer->error_info().size();
    if (errors > 0)
    {
      errorCount += errors;
      continue;
    }

    Scanner scanner = recognizer->tree_scanner();

    switch (queryType)
    {
    case QtCreateTable:
    {
      db_mysql_TableRef table(grt::Initialized);
      table->createDate(base::fmttime(0, DATETIME_FMT));
      table->lastChangeDate(table->createDate());

      std::pair<std::string, bool> result = fillTableDetails(scanner, catalog, currentSchema,
        table, caseSensitive, autoGenerateFkNames, refCache);
      db_mysql_SchemaRef schema = currentSchema;
      if (!result.first.empty() && !base::same_string(schema->name(), result.first, caseSensitive))
        schema = ensureSchemaExists(catalog, result.first, caseSensitive);
      table->owner(schema);

      // Ignore tables that use a name that is already used for a view (no drop/new-add takes place then).
      db_mysql_ViewRef existingView = find_named_object_in_list(schema->views(), table->name());
      if (!existingView.is_valid())
      {
        db_TableRef existingTable = find_named_object_in_list(schema->tables(), table->name());
        if (existingTable.is_valid())
        {
          // Ignore if the table exists already?
          if (!result.second)
          {
            schema->tables()->remove(existingTable);
            schema->tables().insert(table);
            createdObjects.insert(table);
          }
        }
        else
        {
          schema->tables().insert(table);
          createdObjects.insert(table);
        }
      }

      break;
    }

    case QtCreateIndex:
    {
      db_mysql_IndexRef index(grt::Initialized);
      index->createDate(base::fmttime(0, DATETIME_FMT));
      index->lastChangeDate(index->createDate());

      Identifier tableReference = fillIndexDetails(scanner, catalog, currentSchema, index, caseSensitive);
      db_SchemaRef schema = currentSchema;
      if (!tableReference.first.empty() && !base::same_string(schema->name(), tableReference.first, caseSensitive))
        schema = ensureSchemaExists(catalog, tableReference.first, caseSensitive);
      db_TableRef table = find_named_object_in_list(schema->tables(), tableReference.second, caseSensitive);
      if (table.is_valid())
      {
        index->owner(table);

        db_IndexRef existing = find_named_object_in_list(table->indices(), index->name());
        if (existing.is_valid())
          table->indices()->remove(existing);
        table->indices().insert(index);
        createdObjects.insert(index);
      }

      break;
    }

    case QtCreateDatabase:
    {
      db_mysql_SchemaRef schema(grt::Initialized);
      schema->createDate(base::fmttime(0, DATETIME_FMT));
      schema->lastChangeDate(schema->createDate());

      std::pair<std::string, std::string> info = detailsForCharset(catalog->defaultCharacterSetName(),
        catalog->defaultCollationName(), catalog->defaultCharacterSetName());
      schema->defaultCharacterSetName(info.first);
      schema->defaultCollationName(info.second);

      bool ignoreIfExists = fillSchemaDetails(scanner, catalog, schema);
      schema->owner(catalog);

      db_SchemaRef existing = find_named_object_in_list(catalog->schemata(), schema->name(), caseSensitive);
      if (existing.is_valid())
      {
        if (!ignoreIfExists)
        {
          catalog->schemata()->remove(existing);
          catalog->schemata().insert(schema);
          createdObjects.insert(schema);
        }
      }
      else
      {
        catalog->schemata().insert(schema);
        createdObjects.insert(schema);
      }

      break;
    }

    case QtUse:
    {
      scanner.next(); // Skip USE.
      Identifier identifier = getIdentifier(scanner);
      currentSchema = ensureSchemaExists(catalog, identifier.second, caseSensitive);
      break;
    }

    case QtCreateEvent:
    {
      db_mysql_EventRef event(grt::Initialized);
      event->sqlDefinition(base::trim(recognizer->text()));
      event->createDate(base::fmttime(0, DATETIME_FMT));
      event->lastChangeDate(event->createDate());

      std::pair<std::string, bool> result = fillEventDetails(scanner, event);
      db_SchemaRef schema = currentSchema;
      if (!result.first.empty() && !base::same_string(schema->name(), result.first, false))
        schema = ensureSchemaExists(catalog, result.first, false);
      event->owner(schema);

      db_EventRef existing = find_named_object_in_list(schema->events(), event->name());
      if (existing.is_valid())
      {
        if (!result.second) // Ignore if exists?
        {
          schema->events()->remove(existing);
          schema->events().insert(event);
          createdObjects.insert(event);
        }
      }
      else
      {
        schema->events().insert(event);
        createdObjects.insert(event);
      }

      break;
    }

    case QtCreateView:
    {
      db_mysql_ViewRef view(grt::Initialized);
      view->sqlDefinition(base::trim(recognizer->text()));
      view->createDate(base::fmttime(0, DATETIME_FMT));
      view->lastChangeDate(view->createDate());

      std::pair<std::string, bool> result = fillViewDetails(scanner, view);
      db_mysql_SchemaRef schema = currentSchema;
      if (!result.first.empty() && !base::same_string(schema->name(), result.first, caseSensitive))
        schema = ensureSchemaExists(catalog, result.first, caseSensitive);
      view->owner(schema);

      // Ignore views that use a name that is already used for a table (no drop/new-add takes place then).
      db_mysql_TableRef existingTable = find_named_object_in_list(schema->tables(), view->name());
      if (!existingTable.is_valid())
      {
        db_mysql_ViewRef existingView = find_named_object_in_list(schema->views(), view->name());
        if (existingView.is_valid())
        {
          if (!result.second) // Ignore if exists?
          {
            schema->views()->remove(existingView);
            schema->views().insert(view);
            createdObjects.insert(view);
          }
        }
        else
        {
          schema->views().insert(view);
          createdObjects.insert(view);
        }
      }

      break;
    }

    case QtCreateProcedure:
    case QtCreateFunction:
    case QtCreateUdf:
    {
      db_mysql_RoutineRef routine(grt::Initialized);
      routine->sqlDefinition(base::trim(recognizer->text()));
      routine->createDate(base::fmttime(0, DATETIME_FMT));
      routine->lastChangeDate(routine->createDate());

      std::string schemaName = fillRoutineDetails(scanner, routine);
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

    case QtCreateTrigger:
    {
      db_mysql_TriggerRef trigger(grt::Initialized);
      trigger->sqlDefinition(base::trim(recognizer->text()));
      trigger->createDate(base::fmttime(0, DATETIME_FMT));
      trigger->lastChangeDate(trigger->createDate());

      std::pair<std::string, std::string> tableName = fillTriggerDetails(scanner, trigger);

      // Trigger table referencing is a bit different than for other objects because we need
      // the table now to add the trigger to it. We cannot defer that to the resolveReferences() call.
      // This has the implication that we can only work with tables we have found so far.
      db_SchemaRef schema = currentSchema;
      if (!tableName.first.empty())
        schema = ensureSchemaExists(catalog, tableName.first, caseSensitive);
      db_TableRef table = find_named_object_in_list(schema->tables(), tableName.second, caseSensitive);
      if (!table.is_valid())
      {
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

    case QtCreateLogFileGroup:
    {
      db_mysql_LogFileGroupRef group(grt::Initialized);
      group->createDate(base::fmttime(0, DATETIME_FMT));
      group->lastChangeDate(group->createDate());

      fillLogfileGroupDetails(scanner, group);
      group->owner(catalog);

      db_LogFileGroupRef existing = find_named_object_in_list(catalog->logFileGroups(), group->name());
      if (existing.is_valid())
        catalog->logFileGroups()->remove(existing);
      catalog->logFileGroups().insert(group);
      createdObjects.insert(group);

      break;
    }

    case QtCreateServer:
    {
      db_mysql_ServerLinkRef server(grt::Initialized);
      server->createDate(base::fmttime(0, DATETIME_FMT));
      server->lastChangeDate(server->createDate());

      fillServerDetails(scanner, server);
      server->owner(catalog);

      db_ServerLinkRef existing = find_named_object_in_list(catalog->serverLinks(), server->name());
      if (existing.is_valid())
        catalog->serverLinks()->remove(existing);
      catalog->serverLinks().insert(server);
      createdObjects.insert(server);

      break;
    }

    case QtCreateTableSpace:
    {
      db_mysql_TablespaceRef tablespace(grt::Initialized);
      tablespace->createDate(base::fmttime(0, DATETIME_FMT));
      tablespace->lastChangeDate(tablespace->createDate());

      fillTablespaceDetails(scanner, catalog, tablespace);
      tablespace->owner(catalog);

      db_TablespaceRef existing = find_named_object_in_list(catalog->tablespaces(), tablespace->name());
      if (existing.is_valid())
        catalog->tablespaces()->remove(existing);
      catalog->tablespaces().insert(tablespace);
      createdObjects.insert(tablespace);

      break;
    }

    case QtDropDatabase:
    {
      scanner.next(2); // DROP DATABASE.
      scanner.skipIf(MySQLLexer::IF_SYMBOL, 2); // IF EXISTS.
      Identifier identifier = getIdentifier(scanner);
      db_SchemaRef schema = find_named_object_in_list(catalog->schemata(), identifier.second);
      if (schema.is_valid())
      {
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

    case QtDropEvent:
    {
      scanner.next(2);
      scanner.skipIf(MySQLLexer::IF_SYMBOL, 2);
      Identifier identifier = getIdentifier(scanner);
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
      scanner.next(2);
      scanner.skipIf(MySQLLexer::IF_SYMBOL, 2);
      Identifier identifier = getIdentifier(scanner);
      db_SchemaRef schema = currentSchema;
      if (!identifier.first.empty())
        schema = ensureSchemaExists(catalog, identifier.first, caseSensitive);
      db_RoutineRef routine = find_named_object_in_list(schema->routines(), identifier.second);
      schema->routines()->remove(routine);
      break;
    }

    case QtDropIndex:
    {
      scanner.next();
      if (scanner.is(MySQLLexer::ONLINE_SYMBOL) || scanner.is(MySQLLexer::OFFLINE_SYMBOL))
        scanner.next();
      scanner.next();
      std::string name = getIdentifier(scanner).second;
      scanner.next(); // Skip ON.

      Identifier reference = getIdentifier(scanner);
      db_SchemaRef schema = currentSchema;
      if (!reference.first.empty())
        schema = ensureSchemaExists(catalog, reference.first, caseSensitive);
      db_TableRef table = find_named_object_in_list(schema->tables(), reference.second);
      if (table.is_valid())
      {
        db_IndexRef index = find_named_object_in_list(table->indices(), name);
        if (index.is_valid())
          table->indices()->remove(index);
      }
      break;
    }

    case QtDropLogfileGroup:
    {
      scanner.next(3); // Skip DROP LOGFILE GROUP.
      Identifier identifier = getIdentifier(scanner);
      db_LogFileGroupRef group = find_named_object_in_list(catalog->logFileGroups(), identifier.second);
      if (group.is_valid())
        catalog->logFileGroups()->remove(group);

      break;
    }

    case QtDropServer:
    {
      scanner.next(2); // Skip DROP SERVER.
      scanner.skipIf(MySQLLexer::IF_SYMBOL, 2); // Skip IF EXISTS.
      Identifier identifier = getIdentifier(scanner);
      db_ServerLinkRef server = find_named_object_in_list(catalog->serverLinks(), identifier.second);
      if (server.is_valid())
        catalog->serverLinks()->remove(server);

      break;
    }

    case QtDropTable:
    case QtDropView:
    {
      bool isView = queryType == QtDropView;
      scanner.next();
      scanner.skipIf(MySQLLexer::TEMPORARY_SYMBOL);
      scanner.next(); // Skip TABLE | TABLES | VIEW.
      scanner.skipIf(MySQLLexer::IF_SYMBOL, 2); // Skip IF EXISTS.

      // We can have a list of tables to drop here.
      while (true)
      {
        Identifier identifier = getIdentifier(scanner);
        db_SchemaRef schema = currentSchema;
        if (!identifier.first.empty())
          schema = ensureSchemaExists(catalog, identifier.first, caseSensitive);
        if (isView)
        {
          db_ViewRef view = find_named_object_in_list(schema->views(), identifier.second);
          if (view.is_valid())
            schema->views()->remove(view);
        }
        else
        {
          db_TableRef table = find_named_object_in_list(schema->tables(), identifier.second);
          if (table.is_valid())
            schema->tables()->remove(table);
        }
        if (scanner.tokenType() != COMMA_SYMBOL)
          break;
        scanner.next();
      }

      break;
    }

    case QtDropTablespace:
    {
      scanner.next(2);
      Identifier identifier = getIdentifier(scanner);
      db_TablespaceRef tablespace = find_named_object_in_list(catalog->tablespaces(), identifier.second);
      if (tablespace.is_valid())
        catalog->tablespaces()->remove(tablespace);

      break;
    }

    case QtDropTrigger:
    {
      scanner.next(2);
      scanner.skipIf(MySQLLexer::IF_SYMBOL, 2);
      Identifier identifier = getIdentifier(scanner);

      // Even though triggers are schema level objects they work on specific tables
      // and that's why we store them under the affected tables, not in the schema object.
      // This however makes it more difficult to find the trigger to delete, as we have to
      // iterate over all tables.
      db_SchemaRef schema = currentSchema;
      if (!identifier.first.empty())
        schema = ensureSchemaExists(catalog, identifier.first, caseSensitive);
      for (grt::ListRef<db_Table>::const_iterator table = schema->tables().begin(); table != schema->tables().end(); ++table)
      {
        db_TriggerRef trigger = find_named_object_in_list((*table)->triggers(), identifier.second);
        if (trigger.is_valid())
        {
          (*table)->triggers()->remove(trigger);
          break; // A trigger can only be assigned to a single table, so we can stop here.
        }
      }
      break;
    }

    case QtRenameTable:
    {
      // Renaming a table is special as you can use it also to rename a view and to move
      // a table from one schema to the other (not for views, though).
      // Due to the way we store triggers we have an easy life wrt. related triggers.
      scanner.next(2);

      while (true)
      {
        // Unlimited value pairs.
        Identifier source = getIdentifier(scanner);
        db_SchemaRef sourceSchema = currentSchema;
        if (!source.first.empty())
          sourceSchema = ensureSchemaExists(catalog, source.first, caseSensitive);

        scanner.next(); // Skip TO.
        Identifier target = getIdentifier(scanner);
        db_SchemaRef targetSchema = currentSchema;
        if (!target.first.empty())
          targetSchema = ensureSchemaExists(catalog, target.first, caseSensitive);

        db_ViewRef view = find_named_object_in_list(sourceSchema->views(), source.second);
        if (view.is_valid())
        {
          // Cannot move between schemas.
          if (sourceSchema == targetSchema)
            view->name(target.second);
        }
        else
        {
          // Renaming a table.
          db_TableRef table = find_named_object_in_list(sourceSchema->tables(), source.second);
          if (table.is_valid())
          {
            if (sourceSchema != targetSchema)
            {
              sourceSchema->tables()->remove(table);
              targetSchema->tables().insert(table);
              createdObjects.insert(table);
            }
            table->name(target.second);
          }
        }

        if (scanner.tokenType() != COMMA_SYMBOL)
          break;
        scanner.next();
      }

      break;
    }

    // Alter commands. At the moment we only support a limited number of cases as we mostly
    // need SQL-to-GRT conversion for create scripts.
    case QtAlterDatabase:
    {
      db_mysql_SchemaRef schema = currentSchema;
      scanner.next(); // Skip DATABASE.
      if (scanner.isIdentifier())
      {
        Identifier identifier = getIdentifier(scanner);
        schema = ensureSchemaExists(catalog, identifier.second, caseSensitive);
      }
      schema->lastChangeDate(base::fmttime(0, DATETIME_FMT));
      fillSchemaOptions(scanner, catalog, schema);

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
    {                // This is more than the old parser did. 
      scanner.next();
      if (scanner.is(MySQLLexer::ONLINE_SYMBOL) || scanner.is(MySQLLexer::OFFLINE_SYMBOL))
        scanner.next();
      scanner.skipIf(MySQLLexer::IGNORE_SYMBOL);
      scanner.next(); // Skip TABLE.
      Identifier identifier = getIdentifier(scanner);
      db_mysql_SchemaRef schema = currentSchema;
      if (!identifier.first.empty())
        schema = ensureSchemaExists(catalog, identifier.first, caseSensitive);

      db_mysql_TableRef table = find_named_object_in_list(schema->tables(), identifier.second, caseSensitive);
      if (table.is_valid())
      {
        // There must be at least one alter item.
        while (true)
        {
          switch (scanner.lookAhead(true))
          {
          case MySQLLexer::ADD_SYMBOL:
            scanner.next();
            switch (scanner.lookAhead(true))
            {
            case MySQLLexer::CONSTRAINT_SYMBOL:
            case MySQLLexer::PRIMARY_SYMBOL:
            case MySQLLexer::FOREIGN_SYMBOL:
            case MySQLLexer::UNIQUE_SYMBOL:
            case MySQLLexer::INDEX_SYMBOL:
            case MySQLLexer::KEY_SYMBOL:
            case MySQLLexer::FULLTEXT_SYMBOL:
            case MySQLLexer::SPATIAL_SYMBOL:
              scanner.next();
              processTableKeyItem(scanner, catalog, schema->name(), table, autoGenerateFkNames, refCache);
              break;

            case MySQLLexer::RENAME_SYMBOL:
            {
              scanner.next(2);
              if (scanner.is(MySQLLexer::TO_SYMBOL) || scanner.is(MySQLLexer::AS_SYMBOL))
                scanner.next();

              identifier = getIdentifier(scanner);
              db_SchemaRef targetSchema = currentSchema;
              if (!identifier.first.empty())
                targetSchema = ensureSchemaExists(catalog, identifier.first, caseSensitive);

              db_ViewRef view = find_named_object_in_list(schema->views(), identifier.second);
              if (view.is_valid())
              {
                // Cannot move between schemas.
                if (schema == targetSchema)
                  view->name(identifier.second);
              }
              else
              {
                // Renaming a table.
                db_TableRef table = find_named_object_in_list(schema->tables(), identifier.second);
                if (table.is_valid())
                {
                  if (schema != targetSchema)
                  {
                    schema->tables()->remove(table);
                    targetSchema->tables().insert(table);
                  }
                  table->name(identifier.second);
                }
              }

              break;
            }

            default:
              scanner.up();
              scanner.skipSubtree();
            }
            break;

          default:
            scanner.skipSubtree();
          }

          if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
            break;
          scanner.next();
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
  if (defaultSchemaCreated)
  {
    currentSchema = ensureSchemaExists(catalog, "default_schema", caseSensitive);
    if (currentSchema->tables().count() == 0 && currentSchema->views().count() == 0
      && currentSchema->routines().count() == 0 && currentSchema->synonyms().count() == 0
      && currentSchema->sequences().count() == 0 && currentSchema->events().count() == 0)
      catalog->schemata().remove_value(currentSchema);
  }
*/
  return errorCount;
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::doSyntaxCheck(parser_ContextReferenceRef context_ref,
  const std::string &sql, const std::string &type)
{
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  MySQLParseUnit query_type = MySQLParseUnit::PuGeneric;
  if (type == "view")
    query_type = MySQLParseUnit::PuCreateView;
  else
    if (type == "routine")
      query_type = MySQLParseUnit::PuCreateRoutine;
    else
      if (type == "trigger")
        query_type = MySQLParseUnit::PuCreateTrigger;
      else
        if (type == "event")
          query_type = MySQLParseUnit::PuCreateEvent;

  return checkSqlSyntax(context, sql.c_str(), sql.size(), query_type);
}

//--------------------------------------------------------------------------------------------------

/**
 * Parses the given text as a specific query type (see parser for supported types).
 * Returns the error count.
 */
size_t MySQLParserServicesImpl::checkSqlSyntax(MySQLParserContext::Ref context, const char *sql,
  size_t length, MySQLParseUnit type)
{
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  if (impl == nullptr)
    return 0;

  impl->errorCheck(sql, type);
  return impl->errors.size();
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper to collect text positions to references of the given schema.
 * We only come here if there was no syntax error.
 */
void collectSchemaNameOffsets(MySQLParserContext::Ref context, std::list<size_t> &offsets, const std::string schema_name)
{
  /* XXX
  // Don't try to optimize the creation of the scanner. There must be a new instance for each parse run
  // as it stores references to results in the parser.
  Scanner scanner = context->recognizer()->tree_scanner();
  bool case_sensitive = context->case_sensitive();
  while (scanner.next()) {
    switch (scanner.tokenType())
    {
    case MySQLLexer::SCHEMA_NAME_TOKEN:
    case MySQLLexer::SCHEMA_REF_TOKEN:
      if (base::same_string(scanner.tokenText(), schema_name, case_sensitive))
      {
        size_t pos = scanner.tokenOffset();
        if (scanner.is(MySQLLexer::BACK_TICK_QUOTED_ID) || scanner.is(MySQLLexer::SINGLE_QUOTED_TEXT))
          ++pos;
        offsets.push_back(pos);
      }
      break;

    case MySQLLexer::TABLE_NAME_TOKEN:
    case MySQLLexer::TABLE_REF_TOKEN:
    {
      scanner.next();
      if (scanner.isIdentifier() && (scanner.lookAhead(false) == DOT_SYMBOL || scanner.lookAhead(false) == DOT_IDENTIFIER))
      {
        // A table ref not with leading dot but a qualified identifier.
        if (base::same_string(scanner.tokenText(), schema_name, case_sensitive))
        {
          size_t pos = scanner.tokenOffset();
          if (scanner.is(MySQLLexer::BACK_TICK_QUOTED_ID) || scanner.is(MySQLLexer::SINGLE_QUOTED_TEXT))
            ++pos;
          offsets.push_back(pos);
        }
      }
      break;
    }

    case MySQLLexer::COLUMN_REF_TOKEN: // Field and key names (id, .id, id.id, id.id.id).
      scanner.next();
      // No leading dot (which means no schema/table) and at least one dot after the first id.
      if (scanner.isIdentifier() && (scanner.lookAhead(false) == DOT_SYMBOL || scanner.lookAhead(false) == DOT_IDENTIFIER))
      {
        std::string name = scanner.tokenText();
        size_t pos = scanner.tokenOffset();
        if (scanner.is(MySQLLexer::BACK_TICK_QUOTED_ID) || scanner.is(MySQLLexer::SINGLE_QUOTED_TEXT))
          ++pos;

        scanner.next();
        scanner.skipIf(MySQLLexer::DOT_SYMBOL);
        scanner.next();

        if (scanner.lookAhead(false) == DOT_SYMBOL || scanner.lookAhead(false) == DOT_IDENTIFIER) // Fully qualified.
        {
          if (base::same_string(name, schema_name, case_sensitive))
            offsets.push_back(pos);
        }
      }
      break;

    // All those can have schema.id or only id.
    case MySQLLexer::VIEW_REF_TOKEN:
    case MySQLLexer::VIEW_NAME_TOKEN:
    case MySQLLexer::TRIGGER_REF_TOKEN:
    case MySQLLexer::TRIGGER_NAME_TOKEN:
    case MySQLLexer::PROCEDURE_REF_TOKEN:
    case MySQLLexer::PROCEDURE_NAME_TOKEN:
    case MySQLLexer::FUNCTION_REF_TOKEN:
    case MySQLLexer::FUNCTION_NAME_TOKEN:
      scanner.next();
      if (scanner.lookAhead(false) == DOT_SYMBOL || scanner.lookAhead(false) == DOT_IDENTIFIER)
      {
        if (base::same_string(scanner.tokenText(), schema_name, case_sensitive))
        {
          size_t pos = scanner.tokenOffset();
          if (scanner.is(MySQLLexer::BACK_TICK_QUOTED_ID) || scanner.is(MySQLLexer::SINGLE_QUOTED_TEXT))
            ++pos;
          offsets.push_back(pos);
        }
      }
      break;
    }
  }*/
}

//--------------------------------------------------------------------------------------------------

/**
* Replace all occurrences of the old by the new name according to the offsets list.
*/
void replace_schema_names(std::string &sql, const std::list<size_t> &offsets, size_t length,
  const std::string new_name)
{
  bool remove_schema = new_name.empty();
  for (std::list<size_t>::const_reverse_iterator iterator = offsets.rbegin(); iterator != offsets.rend(); ++iterator)
  {
    std::string::size_type start = *iterator;
    std::string::size_type replace_length = length;
    if (remove_schema)
    {
      // Make sure we also remove quotes and the dot.
      if (start > 0 && (sql[start - 1] == '`' || sql[start - 1] == '"'))
      {
        --start;
        ++replace_length;
      }
      ++replace_length;
    }
    sql.replace(start, replace_length, new_name);
  }
}

//--------------------------------------------------------------------------------------------------

void rename_in_list(grt::ListRef<db_DatabaseDdlObject> list, MySQLParserContext::Ref context,
  MySQLParseUnit unit, const std::string old_name, const std::string new_name)
{/* XXX
  for (size_t i = 0; i < list.count(); ++i)
  {
    std::string sql = list[i]->sqlDefinition();
    context->recognizer()->parse(sql.c_str(), sql.size(), true, unit);
    size_t error_count = context->recognizer()->error_info().size();
    if (error_count == 0)
    {
      std::list<size_t> offsets;
      collectSchemaNameOffsets(context, offsets, old_name);
      if (!offsets.empty())
      {
        replace_schema_names(sql, offsets, old_name.size(), new_name);
        list[i]->sqlDefinition(sql);
      }
    }
  }*/
}

//--------------------------------------------------------------------------------------------------

size_t MySQLParserServicesImpl::doSchemaRefRename(parser_ContextReferenceRef context_ref,
  db_mysql_CatalogRef catalog, const std::string old_name, const std::string new_name)
{
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return renameSchemaReferences(context, catalog, old_name, new_name);
}

//--------------------------------------------------------------------------------------------------

/**
* Goes through all schemas in the catalog and changes all db objects to refer to the new name if they
* currently refer to the old name. We also iterate non-related schemas in order to have some
* consolidation/sanitizing in effect where wrong schema references were used.
*/
size_t MySQLParserServicesImpl::renameSchemaReferences(MySQLParserContext::Ref context,
  db_mysql_CatalogRef catalog, const std::string old_name, const std::string new_name)
{
  logDebug("Rename schema references\n");

  ListRef<db_mysql_Schema> schemas = catalog->schemata();
  for (size_t i = 0; i < schemas.count(); ++i)
  {
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

static const unsigned char* skip_leading_whitespace(const unsigned char *head, const unsigned char *tail)
{
  while (head < tail && *head <= ' ')
    head++;
  return head;
}

//--------------------------------------------------------------------------------------------------

bool is_line_break(const unsigned char *head, const unsigned char *line_break)
{
  if (*line_break == '\0')
    return false;

  while (*head != '\0' && *line_break != '\0' && *head == *line_break)
  {
    head++;
    line_break++;
  }
  return *line_break == '\0';
}

//--------------------------------------------------------------------------------------------------

grt::BaseListRef MySQLParserServicesImpl::getSqlStatementRanges(const std::string &sql)
{
  grt::BaseListRef list(true);
  std::vector<std::pair<size_t, size_t> > ranges;

  determineStatementRanges(sql.c_str(), sql.size(), ";", ranges);

  for (std::vector<std::pair<size_t, size_t> >::const_iterator i = ranges.begin(); i != ranges.end(); ++i)
  {
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
  const std::string &line_break)
{
  std::string delimiter = initial_delimiter.empty() ? ";" : initial_delimiter;
  const unsigned char *delimiter_head = (unsigned char*)delimiter.c_str();

  const unsigned char keyword[] = "delimiter";

  const unsigned char *head = (unsigned char *)sql;
  const unsigned char *tail = head;
  const unsigned char *end = head + length;
  const unsigned char *new_line = (unsigned char*)line_break.c_str();
  bool have_content = false; // Set when anything else but comments were found for the current statement.

  while (tail < end)
  {
    switch (*tail)
    {
    case '/': // Possible multi line comment or hidden (conditional) command.
      if (*(tail + 1) == '*')
      {
        tail += 2;
        bool is_hidden_command = (*tail == '!');
        while (true)
        {
          while (tail < end && *tail != '*')
            tail++;
          if (tail == end) // Unfinished comment.
            break;
          else
          {
            if (*++tail == '/')
            {
              tail++; // Skip the slash too.
              break;
            }
          }
        }

        if (!is_hidden_command && !have_content)
          head = tail; // Skip over the comment.
      }
      else
        tail++;

      break;

    case '-': // Possible single line comment.
    {
      const unsigned char *end_char = tail + 2;
      if (*(tail + 1) == '-' && (*end_char == ' ' || *end_char == '\t' || is_line_break(end_char, new_line)))
      {
        // Skip everything until the end of the line.
        tail += 2;
        while (tail < end && !is_line_break(tail, new_line))
          tail++;
        if (!have_content)
          head = tail;
      }
      else
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
      while (tail < end && *tail != quote)
      {
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
    case 'D':
    {
      have_content = true;

      // Possible start of the keyword DELIMITER. Must be at the start of the text or a character,
      // which is not part of a regular MySQL identifier (0-9, A-Z, a-z, _, $, \u0080-\uffff).
      unsigned char previous = tail > (unsigned char *)sql ? *(tail - 1) : 0;
      bool is_identifier_char = previous >= 0x80
        || (previous >= '0' && previous <= '9')
        || ((previous | 0x20) >= 'a' && (previous | 0x20) <= 'z')
        || previous == '$'
        || previous == '_';
      if (tail == (unsigned char *)sql || !is_identifier_char)
      {
        const unsigned char *run = tail + 1;
        const unsigned char *kw = keyword + 1;
        int count = 9;
        while (count-- > 1 && (*run++ | 0x20) == *kw++)
          ;
        if (count == 0 && *run == ' ')
        {
          // Delimiter keyword found. Get the new delimiter (everything until the end of the line).
          tail = run++;
          while (run < end && !is_line_break(run, new_line))
            run++;
          delimiter = base::trim(std::string((char *)tail, run - tail));
          delimiter_head = (unsigned char*)delimiter.c_str();

          // Skip over the delimiter statement and any following line breaks.
          while (is_line_break(run, new_line))
            run++;
          tail = run;
          head = tail;
        }
        else
          tail++;
      }
      else
        tail++;

      break;
    }

    default:
      if (*tail > ' ')
        have_content = true;
      tail++;
      break;
    }

    if (*tail == *delimiter_head)
    {
      // Found possible start of the delimiter. Check if it really is.
      size_t count = delimiter.size();
      if (count == 1)
      {
        // Most common case. Trim the statement and check if it is not empty before adding the range.
        head = skip_leading_whitespace(head, tail);
        if (head < tail)
          ranges.push_back(std::make_pair<size_t, size_t>(head - (unsigned char *)sql, tail - head));
        head = ++tail;
        have_content = false;
      }
      else
      {
        const unsigned char *run = tail + 1;
        const unsigned char *del = delimiter_head + 1;
        while (count-- > 1 && (*run++ == *del++))
          ;

        if (count == 0)
        {
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
 * The scanner must be at the starting point of a user definition.
 * returned dictionary:
 *              user : the username or CURRENT_USER
 *   (optional) host : the host name if available
 *   (optional) id_method : the authentication method if available
 *   (optional) id_password: the authentication string if available
 */
grt::DictRef parseUserDefinition(Scanner &scanner)
{
  grt::DictRef result(true);

  result.gset("user", scanner.tokenText());

  if (scanner.is(MySQLLexer::CURRENT_USER_SYMBOL) && scanner.lookAhead() == MySQLLexer::OPEN_PAR_SYMBOL)
    scanner.next(3);
  else
    scanner.next();

  if (scanner.is(MySQLLexer::COMMA_SYMBOL) || scanner.is(MySQLLexer::EOF))
    return result; // Most simple case. No further options.

  if (scanner.is(MySQLLexer::AT_SIGN_SYMBOL) || scanner.is(MySQLLexer::AT_TEXT_SUFFIX))
  {
    std::string host;
    if (scanner.skipIf(MySQLLexer::AT_SIGN_SYMBOL))
      host = scanner.tokenText();
    else
      host = scanner.tokenText().substr(1); // Skip leading @.
    result.gset("host", host);
    scanner.next();
  }

  if (scanner.is(MySQLLexer::IDENTIFIED_SYMBOL))
  {
    scanner.next();
    if (scanner.is(MySQLLexer::BY_SYMBOL))
    {
      scanner.next();
      scanner.skipIf(MySQLLexer::PASSWORD_SYMBOL);

      result.gset("id_method", "PASSWORD");
      result.gset("id_string", scanner.tokenText());
      scanner.next();
    }
    else // Must be WITH then.
    {
      scanner.next();
      result.gset("id_method", scanner.tokenText());
      scanner.next();

      if (scanner.is(MySQLLexer::AS_SYMBOL) || scanner.is(MySQLLexer::BY_SYMBOL))
      {
        scanner.next();
        result.gset("id_string", scanner.tokenText());
        scanner.next();
      }
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------
/* XXX:
grt::DictRef collectGrantDetails(MySQLRecognizer *recognizer)
{
  grt::DictRef data = grt::DictRef(true);

  Scanner scanner = recognizer->tree_scanner();

  scanner.next(); // Skip GRANT.

  // Collect all privileges.
  grt::StringListRef list = grt::StringListRef(grt::Initialized);
  while (true)
  {
    std::string privileges = scanner.tokenText();
    scanner.next();
    if (scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
    {
      privileges += " (";
      scanner.next();
      std::string list;
      while (true)
      {
        if (!list.empty())
          list += ", ";
        list += scanner.tokenText();
        scanner.next();
        if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
          break;
        scanner.next();
      }

      privileges += list + ")";
      scanner.next();
    }
    else
    {
      // Just a list of identifiers or certain keywords not allowed as identifiers.
      while (scanner.isIdentifier() || scanner.is(MySQLLexer::OPTION_SYMBOL) || scanner.is(MySQLLexer::DATABASES_SYMBOL))
      {
        privileges += " " + scanner.tokenText();
        scanner.next();
      }
    }

    list.insert(privileges);
    if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
      break;
    scanner.next();
  }

  data.set("privileges", list);

  // The subtree for the target details includes the ON keyword at the beginning, which we have to remove.
  // XXX: data.gset("target", base::trim(scanner.textForTree().substr(2)));
  // XXX: scanner.skipSubtree();
  scanner.next(); // Skip TO.

  // Now the user definitions.
  grt::DictRef users = grt::DictRef(true);
  data.set("users", users);

  while (true)
  {
    grt::DictRef user = parseUserDefinition(scanner);
    users.set(user.get_string("user"), user);
    if (!scanner.is(MySQLLexer::COMMA_SYMBOL))
      break;
    scanner.next();
  }

  if (scanner.is(MySQLLexer::REQUIRE_SYMBOL))
  {
    grt::DictRef requirements(true);
    scanner.next();
    switch (scanner.tokenType())
    {
    case MySQLLexer::SSL_SYMBOL:
    case MySQLLexer::X509_SYMBOL:
    case MySQLLexer::NONE_SYMBOL:
      requirements.gset(scanner.tokenText(), "");
      scanner.next();
      break;

    default:
      while (scanner.is(MySQLLexer::CIPHER_SYMBOL) || scanner.is(MySQLLexer::ISSUER_SYMBOL) || scanner.is(MySQLLexer::SUBJECT_SYMBOL))
      {
        std::string option = scanner.tokenText();
        scanner.next();
        requirements.gset(option, scanner.tokenText());
        scanner.next();
        scanner.skipIf(MySQLLexer::AND_SYMBOL);
      }
      break;
    }
    data.set("requirements", requirements);
  }

  if (scanner.is(MySQLLexer::WITH_SYMBOL))
  {
    grt::DictRef options(true);
    scanner.next();
    bool done = false;
    while (!done)
    {
      switch (scanner.tokenType())
      {
      case MySQLLexer::GRANT_SYMBOL:
        options.gset("grant", "");
        scanner.next(2);
        break;

      case MySQLLexer::MAX_QUERIES_PER_HOUR_SYMBOL:
      case MySQLLexer::MAX_UPDATES_PER_HOUR_SYMBOL:
      case MySQLLexer::MAX_CONNECTIONS_PER_HOUR_SYMBOL:
      case MySQLLexer::MAX_USER_CONNECTIONS_SYMBOL:
      {
        std::string option = scanner.tokenText();
        scanner.next();
        options.gset(option, scanner.tokenText());
        scanner.next();
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
*/
//--------------------------------------------------------------------------------------------------

grt::DictRef MySQLParserServicesImpl::parseStatementDetails(parser_ContextReferenceRef context_ref, const std::string &sql)
{
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);
  return parseStatement(context, sql);
}

//--------------------------------------------------------------------------------------------------

grt::DictRef MySQLParserServicesImpl::parseStatement(MySQLParserContext::Ref context, const std::string &sql)
{
  // This part can potentially grow very large because of the sheer amount of possible query types.
  // So it should be moved into an own file if it grows beyond a few 100 lines.
  MySQLParserContextImpl *impl = dynamic_cast<MySQLParserContextImpl *>(context.get());
  if (impl == nullptr)
  {
    grt::DictRef result(true);
    result.gset("error", "Invalid parser context");
    return result;
  }

  impl->parse(sql, MySQLParseUnit::PuGeneric, nullptr);
  if (!impl->errors.empty())
  {
    // Return the error message in case of syntax errors.
    grt::DictRef result(true);
    result.gset("error", impl->errors[0].message);
    return result;
  }

  MySQLQueryType queryType = impl->determineQueryType(sql);

  switch (queryType) {
    case QtGrant:
    case QtGrantProxy:
      // XXX: return collectGrantDetails(recognizer);

    default:
    {
      grt::DictRef result(true);
      result.gset("error", "Unsupported query type (" + base::to_string(queryType) + ")");
      return result;
    }
  }
}

//--------------------------------------------------------------------------------------------------

static bool doParseType(const std::string &type, GrtVersionRef targetVersion,SimpleDatatypeListRef,
  db_SimpleDatatypeRef &simpleType, int &precision, int &scale, int &length, std::string &explicitParams)
{/* XXX:
  // No char sets necessary for parsing data types as there's no repertoire necessary/allowed in any
  // data type part. Neither do we need an sql mode (string lists in enum defs only allow
  // single quoted text). Hence we don't require to pass in a parsing context but create a local parser.
  //
  // Note: we parse here more than the pure data type name + precision/scale (namely additional parameters
  // like charsets etc.). That doesn't affect the main task here, however. Additionally stuff
  // is simply ignored for now (but it must be a valid definition).
  MySQLRecognizer recognizer(bec::version_to_int(targetVersion), "", std::set<std::string>());
  recognizer.parse(type.c_str(), type.size(), true, MySQLParseUnit::PuDataType);
  if (!recognizer.error_info().empty())
    return false;

  Scanner scanner = recognizer.tree_scanner();

  // A type name can consist of up to 3 parts (e.g. "national char varying").
  std::string type_name = scanner.tokenText();

  switch (scanner.tokenType())
  {
    case MySQLLexer::DOUBLE_SYMBOL:
      scanner.next();
      if (scanner.tokenType() == PRECISION_SYMBOL)
        scanner.next(); // Simply ignore syntactic sugar.
      break;

    case MySQLLexer::NATIONAL_SYMBOL:
      scanner.next();
      type_name += " " + scanner.tokenText();
      scanner.next();
      if (scanner.tokenType() == VARYING_SYMBOL)
      {
        type_name += " " + scanner.tokenText();
        scanner.next();
      }
      break;

    case MySQLLexer::NCHAR_SYMBOL:
      scanner.next();
      if (scanner.tokenType() == VARCHAR_SYMBOL || scanner.tokenType() == VARYING_SYMBOL)
      {
        type_name += " " + scanner.tokenText();
        scanner.next();
      }
      break;

    case MySQLLexer::CHAR_SYMBOL:
      scanner.next();
      if (scanner.tokenType() == VARYING_SYMBOL)
      {
        type_name += " " + scanner.tokenText();
        scanner.next();
      }
      break;

    case MySQLLexer::LONG_SYMBOL:
      scanner.next();
      switch (scanner.tokenType())
    {
      case MySQLLexer::CHAR_SYMBOL: // LONG CHAR VARYING
        if (scanner.lookAhead(true) == VARYING_SYMBOL) // Otherwise we may get e.g. LONG CHAR SET...
        {
          type_name += " " + scanner.tokenText();
          scanner.next();
          type_name += " " + scanner.tokenText();
          scanner.next();
        }
        break;

      case MySQLLexer::VARBINARY_SYMBOL:
      case MySQLLexer::VARCHAR_SYMBOL:
        type_name += " " + scanner.tokenText();
        scanner.next();
    }
      break;

    default:
      scanner.next();
  }

  simpleType = findDataType(typeList, targetVersion, type_name);

  if (!simpleType.is_valid()) // Should always be valid at this point or we have messed up our type list.
    return false;

  if (!scanner.is(MySQLLexer::OPEN_PAR_SYMBOL))
    return true;

  scanner.next();

  // We use the simple type properties for char length to learn if we have a length here or a precision.
  // We could indicate that in the grammar instead, however the handling in WB is a bit different
  // than what the server grammar would suggest (e.g. the length is also used for integer types, in the grammar).
  if (simpleType->characterMaximumLength() != bec::EMPTY_TYPE_MAXIMUM_LENGTH
      || simpleType->characterOctetLength() != bec::EMPTY_TYPE_OCTET_LENGTH)
  {
    if (scanner.tokenType() != INT_NUMBER)
      return false;
    length = base::atoi<int>(scanner.tokenText().c_str());
    return true;
  }

  if (!scanner.is(MySQLLexer::INT_NUMBER))
  {
    // ENUM or SET.
    do
    {
      if (!explicitParams.empty())
        explicitParams += ", ";
      explicitParams += scanner.tokenText(true);
      scanner.next();
      scanner.skipIf(MySQLLexer::COMMA_SYMBOL); // We normalize the whitespace around the commas.
    } while (!scanner.is(MySQLLexer::CLOSE_PAR_SYMBOL));
    explicitParams = "(" + explicitParams + ")";

    return true;
  }

  // Finally all cases with either precision, scale or both.
  precision = base::atoi<int>(scanner.tokenText().c_str());
  scanner.next();
  if (scanner.tokenType() != COMMA_SYMBOL)
    return true;
  scanner.next();
  scale = base::atoi<int>(scanner.tokenText().c_str());
  */
  return true;
}

//--------------------------------------------------------------------------------------------------

bool MySQLParserServicesImpl::parseTypeDefinition(const std::string &typeDefinition, GrtVersionRef targetVersion,
  SimpleDatatypeListRef typeList, UserDatatypeListRef userTypes, SimpleDatatypeListRef defaultTypeList,
  db_SimpleDatatypeRef &simpleType, db_UserDatatypeRef &userType, int &precision, int &scale, int &length,
  std::string &datatypeExplicitParams)
{/* XXX:
  if (user_types.is_valid())
  {
    std::string::size_type argp = type.find('(');
    std::string typeName = type;

    if (argp != std::string::npos)
      typeName = type.substr(0, argp);

    // 1st check if this is a user defined type
    for (size_t c = user_types.count(), i = 0; i < c; i++)
    {
      db_UserDatatypeRef utype(user_types[i]);

      if (base::string_compare(utype->name(), typeName, false) == 0)
      {
        userType = utype;
        break;
      }
    }
  }

  if (userType.is_valid())
  {
    // If the type spec has an argument, we replace the arguments from the type definition
    // with the one provided by the user.
    std::string finalType = userType->sqlDefinition();
    std::string::size_type tp;
    bool overriden_args = false;

    if ((tp = type.find('(')) != std::string::npos) // Are there user specified args?
    {
      std::string::size_type p = finalType.find('(');
      if (p != std::string::npos) // Strip the original args.
        finalType = finalType.substr(0, p);

      // Extract the user specified args and append to the specification.
      finalType.append(type.substr(tp));

      overriden_args = true;
    }

    // Parse user type definition.
    if (!parse_type(finalType, targetVersion, typeList.is_valid() ? typeList : default_type_list,
                    simpleType, precision, scale, length, datatypeExplicitParams))
      return false;

    simpleType = db_SimpleDatatypeRef();
    if (!overriden_args)
    {
      precision = bec::MySQLLexer::EMPTY_COLUMN_PRECISION;
      scale = bec::MySQLLexer::EMPTY_COLUMN_SCALE;
      length = bec::MySQLLexer::EMPTY_COLUMN_LENGTH;
      datatypeExplicitParams = "";
    }
  }
  else
  {
    if (!parse_type(type, targetVersion, typeList.is_valid() ? typeList : default_type_list,
                    simpleType, precision, scale, length, datatypeExplicitParams))
      return false;
    
    userType = db_UserDatatypeRef();
  }*/
  return true;
}

//--------------------------------------------------------------------------------------------------

std::string MySQLParserServicesImpl::replaceTokenSequence(parser_ContextReferenceRef context_ref,
  const std::string &sql, size_t start_token, size_t count, grt::StringListRef replacements)
{
  MySQLParserContext::Ref context = parser_context_from_grt(context_ref);

  std::vector<std::string> list;
  list.reserve(replacements->count());
  std::copy(replacements.begin(), replacements.end(), std::back_inserter(list));
  return replaceTokenSequenceWithText(context, sql, start_token, count, list);
}

//--------------------------------------------------------------------------------------------------

std::string MySQLParserServicesImpl::replaceTokenSequenceWithText(MySQLParserContext::Ref context,
  const std::string &sql, size_t start_token, size_t count, const std::vector<std::string> replacements)
{
  std::string result;
  /* XXX:
  context->recognizer()->parse(sql.c_str(), sql.size(), true, MySQLParseUnit::PuGeneric);
  size_t error_count = context->recognizer()->error_info().size();
  if (error_count > 0)
    return "";

  Scanner scanner = context->recognizer()->tree_scanner();
  if (!scanner.advanceToType((unsigned)start_token, true))
    return sql;

  // Get the index of each token in the input stream and use that in the input lexer to find
  // tokens to replace. Don't touch any other (including whitespaces).
  // The given start_token can only be a visible token.

  // First find the range of the text before the start token and copy that unchanged.
  ANTLR3_MARKER current_index = scanner.tokenIndex();
  if (current_index > 0)
  {
    ParserToken token = context->recognizer()->token_at_index(current_index - 1);
    result = sql.substr(0, token.stop - sql.c_str() + 1);
  }

  // Next replace all tokens we have replacements for. Remember tokens are separated by hidden tokens
  // which must be added to the result unchanged.
  size_t c = std::min(count, replacements.size());
  size_t i = 0;
  for (; i < c; ++i)
  {
    ++current_index; // Ignore the token.
    result += replacements[i];

    // Append the following separator token.
    ParserToken token = context->recognizer()->token_at_index(current_index++);
    if (token.type == ANTLR3_TOKEN_INVALID)
      return result; // Premature end. Too many values given to replace.
    result += token.text;

    --count;
  }

  if (i < replacements.size())
  {
    // Something left to add before continuing with the rest of the SQL.
    // In order to separate replacements properly they must include necessary whitespaces.
    // We don't add any here.
    while (i < replacements.size())
      result += replacements[i++];
  }

  if (count > 0)
  {
    // Some tokens left without replacement. Skip them and add the rest of the query unchanged.
    // Can be a large number to indicate the removal of anything left.
    current_index += count;
  }

  // Finally add the remaining text.
 ParserToken token = context->recognizer()->token_at_index(current_index);
  if (token.type != ANTLR3_TOKEN_INVALID)
    result += token.start; // Implicitly zero-terminated.
*/
  return result;
}

//--------------------------------------------------------------------------------------------------
