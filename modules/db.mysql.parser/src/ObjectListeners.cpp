/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation. The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "base/string_utilities.h"
#include "base/util_functions.h"

#include "grtpp_util.h"
#include "grtdb/charset_utils.h"
#include "grtdb/db_object_helpers.h"

#include "mysql/MySQLLexer.h"
#include "mysql/MySQLParserBaseListener.h"

#include "ObjectListeners.h"

#include "grtsqlparser/mysql_parser_services.h"

using namespace grt;
using namespace parsers;
using namespace antlr4;

//----------------------------------------------------------------------------------------------------------------------

static std::string getIdentifierList(MySQLParser::IdentifierListContext *ctx) {
  std::string result;
  for (auto &identifier : ctx->identifier()) {
    if (!result.empty())
      result += ", ";
    result += base::unquote(identifier->getText());
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

static size_t numberValue(std::string value) {
  // Value can have a suffix. And it can be a hex number.
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

  return factor * std::stoull(value);
}

//----------------------------------------------------------------------------------------------------------------------

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

      // Clear collation if it's the default collation or belongs to another character set.
      if ((result.second == defaultCollationForCharset(result.first)) ||
          (result.first != charsetForCollation(result.second)))
        result.second = "";
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

static std::pair<std::string, std::string> detailsForCollation(const std::string &collation,
                                                               const std::string &defaultCollation) {
  std::pair<std::string, std::string> result;
  if (!collation.empty()) {
    result.second = base::tolower(collation);
    if (result.second == "default")
      result.second = base::tolower(defaultCollation);

    // Clear collation if it's the default collation.
    result.first = charsetForCollation(result.second);
    if (defaultCollationForCharset(result.first) == result.second)
      result.second = "";
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

static void parseReferences(MySQLParser::ReferencesContext *ctx, const std::string &schemaName,
                            DbObjectReferences &references) {
  IdentifierListener listener(ctx->tableRef());
  Identifier identifier;
  if (listener.parts.size() == 1) {
    identifier.first = schemaName;
    identifier.second = listener.parts[0];
  } else {
    identifier.first = listener.parts[0];
    identifier.second = listener.parts[1];
  }

  references.targetIdentifier = identifier;
  if (ctx->identifierListWithParentheses() != nullptr) {
    for (auto &column : ctx->identifierListWithParentheses()->identifierList()->identifier())
      references.columnNames.push_back(base::unquote(column->getText()));
  }

  // MATCH is ignored by MySQL. We do the same for now.

  if (ctx->option != nullptr) {
    if (ctx->option->getType() == MySQLLexer::UPDATE_SYMBOL) {
      references.foreignKey->updateRule(MySQLBaseLexer::sourceTextForContext(ctx->deleteOption(0)));
      if (ctx->deleteOption().size() > 1) // 2 rules actually: UPDATE DELETE
        references.foreignKey->deleteRule(MySQLBaseLexer::sourceTextForContext(ctx->deleteOption(1)));
    } else {
      references.foreignKey->deleteRule(MySQLBaseLexer::sourceTextForContext(ctx->deleteOption(0)));
      if (ctx->deleteOption().size() > 1) // 2 rules actually: DELETE UPDATE
        references.foreignKey->updateRule(MySQLBaseLexer::sourceTextForContext(ctx->deleteOption(1)));
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Collects only the names of each entry in a key list into the references entry.
 */
static void columnNamesFromKeyList(MySQLParser::KeyListContext *ctx, DbObjectReferences &references) {
  for (auto &part : ctx->keyPart())
    references.columnNames.push_back(base::unquote(part->identifier()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

static void parseKeyList(ParserRuleContext *ctx, db_mysql_TableRef table, db_mysql_IndexRef index,
                         DbObjectsRefsCache &refCache) {
  DbObjectReferences references(index);
  references.table = table;
  index->columns().remove_all();

  auto variantExpression = dynamic_cast<MySQLParser::KeyListVariantsContext *>(ctx);
  auto keyList = variantExpression != nullptr ? variantExpression->keyList()
    : dynamic_cast<MySQLParser::KeyListContext *>(ctx);
  if (keyList != nullptr) {
    for (auto &part : keyList->keyPart()) {
      db_mysql_IndexColumnRef indexColumn(grt::Initialized);
      indexColumn->owner(index);
      indexColumn->name(base::unquote(part->identifier()->getText()));
      references.index->columns().insert(indexColumn);

      if (part->fieldLength() != nullptr) {
        auto child = dynamic_cast<tree::ParseTree *>(part->fieldLength()->children[1]);
        indexColumn->columnLength((size_t)std::stoull(child->getText()));
      }

      if (part->direction() != nullptr)
        indexColumn->descend(part->direction()->DESC_SYMBOL() != nullptr);
    }
  } else {
    for (auto &keyPartOrExpression : variantExpression->keyListWithExpression()->keyPartOrExpression()) {
      db_mysql_IndexColumnRef indexColumn(grt::Initialized);
      indexColumn->owner(index);
      if (keyPartOrExpression->keyPart() != nullptr) {
        auto part = keyPartOrExpression->keyPart();
        indexColumn->name(base::unquote(part->identifier()->getText()));

        if (part->fieldLength() != nullptr) {
          auto child = dynamic_cast<tree::ParseTree *>(part->fieldLength()->children[1]);
          indexColumn->columnLength((size_t)std::stoull(child->getText()));
        }

        if (part->direction() != nullptr)
          indexColumn->descend(part->direction()->DESC_SYMBOL() != nullptr);
      } else {
        auto expression = MySQLBaseLexer::sourceTextForContext(keyPartOrExpression->exprWithParentheses()->expr());
        indexColumn->expression(expression);
        if (keyPartOrExpression->direction() != nullptr)
          indexColumn->descend(keyPartOrExpression->direction()->DESC_SYMBOL() != nullptr);
      }

      references.index->columns().insert(indexColumn);
    }
  }

  refCache.push_back(references);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 *	Helper to bring the index type string into a commonly used form.
 */
static std::string formatIndexType(std::string indexType) {
  indexType = indexType.substr(0, indexType.find(' ')); // Only first word is meaningful.
  indexType = base::toupper(indexType);
  if (indexType == "KEY")
    indexType = "INDEX";
  return indexType;
}

//----------------------------------------------------------------------------------------------------------------------

// We have a number of partial listeners for parts that need to be evaluated in more than one object listener.

//----------------- IdentifierListener ---------------------------------------------------------------------------------

IdentifierListener::IdentifierListener(tree::ParseTree *tree) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void IdentifierListener::enterIdentifier(MySQLParser::IdentifierContext *ctx) {
  parts.push_back(base::unquote(ctx->getText()));
}

//----------------- DetailsListener ------------------------------------------------------------------------------------

// A mixed use listener for various details that are required in multiple object listeners, but where it is too much
// effort to create an own listener for.

DetailsListener::DetailsListener(db_mysql_CatalogRef catalog, bool caseSensitive)
  : _catalog(catalog), _caseSensitive(caseSensitive) {
}

//----------------- ColumnDefinitionListener ---------------------------------------------------------------------------

class ColumnDefinitionListener : public DetailsListener {
public:
  db_mysql_ColumnRef column;

  ColumnDefinitionListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog, const std::string &schemaName,
                           db_mysql_TableRef table, DbObjectsRefsCache &refCache)
    : DetailsListener(catalog, false),
      column(grt::Initialized),
      _table(table),
      _schemaName(schemaName),
      _refCache(refCache) {
    column->owner(_table);
    column->userType(db_UserDatatypeRef()); // We always have normal data types here.
    column->scale(bec::EMPTY_COLUMN_SCALE);
    column->precision(bec::EMPTY_COLUMN_PRECISION);
    column->length(bec::EMPTY_COLUMN_LENGTH);

    tree::ParseTreeWalker::DEFAULT.walk(this, tree);
  }

  virtual void exitColumnDefinition(MySQLParser::ColumnDefinitionContext *ctx) override {
    // For servers < 8 the column name can be qualified. With 8+ this changed to a simple identifier.
    if (ctx->columnName()->fieldIdentifier()) {
      IdentifierListener listener(ctx->columnName()->fieldIdentifier());
      column->name(listener.parts.back());
      column->oldName(listener.parts.back());
    } else {
      IdentifierListener listener(ctx->columnName()->identifier());
      column->name(listener.parts.back());
      column->oldName(listener.parts.back());
    }

    DataTypeListener typeListener(ctx->fieldDefinition()->dataType(), _catalog->version(), _catalog->simpleDatatypes(),
                                  column->flags(), _table->defaultCharacterSetName());
    column->simpleType(typeListener.dataType);
    column->scale(typeListener.scale);
    column->precision(typeListener.precision);
    column->length(typeListener.length);
    column->datatypeExplicitParams(typeListener.explicitParams);
    column->characterSetName(
      typeListener.charsetName); // Charset comes from data type, collation from column attributes.

    if (column->simpleType().is_valid() && base::same_string(column->simpleType()->name(), "TIMESTAMP", false)) {
      if (!_explicitNullValue)
        column->isNotNull(1);
    }

    if (!column->isNotNull() && !_explicitDefaultValue)
      bec::ColumnHelper::set_default_value(column, "NULL");

    // The CHECK expression is ignored by the server. And so do we.
    _table->columns().insert(column);
  }

  virtual void exitFieldDefinition(MySQLParser::FieldDefinitionContext *ctx) override {
    if (ctx->AS_SYMBOL() != nullptr) {
      // Only there for generated columns.
      column->generated(1);
      column->expression(MySQLBaseLexer::sourceTextForContext(ctx->exprWithParentheses()->expr()));

      if (ctx->VIRTUAL_SYMBOL() != nullptr)
        column->generatedStorage("VIRTUAL");
      if (ctx->STORED_SYMBOL() != nullptr)
        column->generatedStorage("STORED");

      if (ctx->collate() != nullptr) { // Collation clause for generated columns.
        auto info = detailsForCollation(ctx->collate()->collationName()->getText(), _table->defaultCollationName());
        column->characterSetName(info.first);
        column->collationName(info.second);
      }
    }
  }

  virtual void exitColumnAttribute(MySQLParser::ColumnAttributeContext *ctx) override {
    if (ctx->nullLiteral() != nullptr) {
      column->isNotNull(ctx->NOT_SYMBOL() != nullptr);
      _explicitNullValue = true;
      return;
    }

    if (ctx->collate() != nullptr) {
      auto info = detailsForCollation(ctx->collate()->collationName()->getText(), _table->defaultCollationName());
      column->characterSetName(info.first);
      column->collationName(info.second);
      return;
    }

    switch (ctx->value->getType()) {
      case MySQLLexer::DEFAULT_SYMBOL: {
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
        if (ctx->NOW_SYMBOL() != nullptr) {
          // As written above, convert all synonyms. This can cause trouble with additional
          // precision, which we may have to handle later.
          std::string newDefault = "CURRENT_TIMESTAMP";
          if (ctx->timeFunctionParameters() != nullptr) // Additional precision.
            newDefault += MySQLBaseLexer::sourceTextForContext(ctx->timeFunctionParameters());
          if (!existingDefault.empty())
            newDefault += " " + existingDefault;
          column->defaultValue(newDefault);
        } else if (ctx->signedLiteral() != nullptr) {
          std::string newDefault = MySQLBaseLexer::sourceTextForContext(ctx->signedLiteral(), true);
          column->defaultValue(newDefault);

          if (base::same_string(newDefault, "NULL", false))
            column->defaultValueIsNull(true);
        } else {
          // Expressions in parentheses.
          auto expression = ctx->exprWithParentheses()->expr();
          column->defaultValue(MySQLBaseLexer::sourceTextForContext(expression, true));
        }

        _explicitDefaultValue = true;
        break;
      }

      case MySQLLexer::ON_SYMBOL: {
        // As mentioned above we combine DEFAULT NOW and ON UPDATE NOW into a common default value.
        std::string newDefault = column->defaultValue();
        if (base::hasPrefix(newDefault, "CURRENT_TIMESTAMP"))
          newDefault += " ON UPDATE CURRENT_TIMESTAMP";
        else
          newDefault = "ON UPDATE CURRENT_TIMESTAMP";

        if (ctx->timeFunctionParameters() != nullptr) // Additional precision.
          newDefault += MySQLBaseLexer::sourceTextForContext(ctx->timeFunctionParameters());

        column->defaultValue(newDefault);
        _explicitDefaultValue = true;

        break;
      }

      case MySQLLexer::AUTO_INCREMENT_SYMBOL:
        column->autoIncrement(1);
        break;

      case MySQLLexer::SERIAL_SYMBOL: // SERIAL DEFAULT VALUE is an alias for NOT NULL AUTO_INCREMENT UNIQUE.
      case MySQLLexer::UNIQUE_SYMBOL: {
        if (ctx->DEFAULT_SYMBOL() != nullptr) {
          column->isNotNull(1);
          column->autoIncrement(1);
        }

        // Add new unique index for that column.
        db_mysql_IndexRef index(grt::Initialized);
        index->owner(_table);
        index->unique(1);
        index->indexType("UNIQUE");

        db_mysql_IndexColumnRef indexColumn(grt::Initialized);
        indexColumn->owner(index);
        indexColumn->referencedColumn(column);

        index->columns().insert(indexColumn);
        _table->indices().insert(index);

        break;
      }

      case MySQLLexer::PRIMARY_SYMBOL:
      case MySQLLexer::KEY_SYMBOL: {
        db_mysql_IndexRef index(grt::Initialized);
        index->owner(_table);

        index->isPrimary(1);
        _table->primaryKey(index);
        index->indexType("PRIMARY");
        index->name("PRIMARY");
        index->oldName("PRIMARY");

        db_mysql_IndexColumnRef indexColumn(grt::Initialized);
        indexColumn->owner(index);
        indexColumn->referencedColumn(column);

        index->columns().insert(indexColumn);
        _table->indices().insert(index);

        break;
      }

      case MySQLLexer::COMMENT_SYMBOL:
        column->comment(MySQLBaseLexer::sourceTextForContext(ctx->textLiteral()));
        break;

      case MySQLLexer::COLUMN_FORMAT_SYMBOL: // Ignored by the server, so we ignore it here too.
        break;

      case MySQLLexer::STORAGE_SYMBOL: // No info available, might later become important.
        break;

      default:
        break;
    }
  }

  virtual void exitReferences(MySQLParser::ReferencesContext *ctx) override {
    // This is a so called "inline references specification", which is not supported by
    // MySQL. We parse it nonetheless as it may require to create stub tables and
    // the old parser created foreign key entries for these.
    db_mysql_ForeignKeyRef fk(grt::Initialized);
    fk->owner(_table);
    fk->columns().insert(column);
    fk->many(true);
    fk->referencedMandatory(column->isNotNull());
    _table->foreignKeys().insert(fk);

    DbObjectReferences references(fk, DbObjectReferences::Referenced);
    references.table = _table;
    parseReferences(ctx, _schemaName, references);

    _refCache.push_back(references);
  }

private:
  db_mysql_TableRef _table;
  std::string _schemaName;
  DbObjectsRefsCache &_refCache;

  bool _explicitNullValue = false;
  bool _explicitDefaultValue = false;
};

class KeyDefinitionListener : public DetailsListener {
public:
  KeyDefinitionListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog, const std::string &schemaName,
                        db_mysql_TableRef table, DbObjectsRefsCache &refCache, bool autoGenerateFkNames)
    : DetailsListener(catalog, false),
      _table(table),
      _schemaName(schemaName),
      _refCache(refCache),
      _autoGenerateFkNames(autoGenerateFkNames),
      _currentIndex(grt::Initialized) {
    _currentIndex->owner(_table);
    _currentIndex->visible(1); // By default all indexes are visible.
    tree::ParseTreeWalker::DEFAULT.walk(this, tree);
  }

  virtual void exitTableConstraintDef(MySQLParser::TableConstraintDefContext *ctx) override {
    std::string constraintName;

    if (ctx->indexNameAndType() != nullptr) {
      IdentifierListener listener(ctx->indexNameAndType()->indexName());
      constraintName = listener.parts.back();
    } else if (ctx->indexName() != nullptr) {
      IdentifierListener listener(ctx->indexName());
      constraintName = listener.parts.back();
    } if (ctx->constraintName() != nullptr && ctx->constraintName()->identifier() != nullptr) {
      // Use the constraint symbol as name if given and no other name is available.
      IdentifierListener listener(ctx->constraintName()->identifier());
      constraintName = listener.parts.back();
    }

    bool isForeignKey = false; // Need the new index only for non-FK constraints.
    if (ctx->type == nullptr)  // Currently null for check constraints (which we ignore).
      return;

    switch (ctx->type->getType()) {
      case MySQLLexer::PRIMARY_SYMBOL:
        _currentIndex->isPrimary(1);
        _table->primaryKey(_currentIndex);
        constraintName = "PRIMARY";
        _currentIndex->indexType("PRIMARY");
        break;

      case MySQLLexer::FOREIGN_SYMBOL: {
        isForeignKey = true;

        db_mysql_ForeignKeyRef fk(grt::Initialized);
        fk->owner(_table);
        fk->name(constraintName);
        fk->oldName(constraintName);

        if (fk->name().empty() && _autoGenerateFkNames) {
          std::string name = bec::TableHelper::generate_foreign_key_name();
          fk->name(name);
          fk->oldName(name);
        }

        // List of columns in the FK.
        {
          DbObjectReferences references(fk, DbObjectReferences::Referencing);

          // Columns used in the FK might not have been parsed yet, so add the column refs
          // to our cache as well and resolve them when we are done with the overall structure.
          references.targetIdentifier.first = _schemaName;
          references.targetIdentifier.second = _table->name();
          references.table = _table;

          columnNamesFromKeyList(ctx->keyList(), references);
          _refCache.push_back(references);
        }

        DbObjectReferences references(fk, DbObjectReferences::Referenced);
        references.table = _table;
        parseReferences(ctx->references(), _schemaName, references);
        _table->foreignKeys().insert(fk);
        _refCache.push_back(references);

        break;
      }

      case MySQLLexer::UNIQUE_SYMBOL:
      case MySQLLexer::INDEX_SYMBOL:
      case MySQLLexer::KEY_SYMBOL:
      case MySQLLexer::FULLTEXT_SYMBOL:
      case MySQLLexer::SPATIAL_SYMBOL:
        if (ctx->type->getType() == MySQLLexer::UNIQUE_SYMBOL) {
          _currentIndex->unique(1);
          _currentIndex->indexType("UNIQUE");
        } else
          _currentIndex->indexType(formatIndexType(ctx->type->getText()));

        break;

      default: // Anything else we ignore for now.
        return;
    }

    if (!isForeignKey) {
      parseKeyList(ctx->keyListVariants(), _table, _currentIndex, _refCache);
      _currentIndex->name(constraintName);
      _currentIndex->oldName(constraintName);
      _table->indices().insert(_currentIndex);
    }
  }

  virtual void exitIndexType(MySQLParser::IndexTypeContext *ctx) override {
    _currentIndex->indexKind(base::toupper(ctx->algorithm->getText()));
  }

  virtual void exitCommonIndexOption(MySQLParser::CommonIndexOptionContext *ctx) override {
    if (ctx->KEY_BLOCK_SIZE_SYMBOL() != nullptr)
      _currentIndex->keyBlockSize((size_t)std::stoull(ctx->ulong_number()->getText()));
    else if (ctx->COMMENT_SYMBOL() != nullptr)
      _currentIndex->comment(ctx->textLiteral()->getText());

    if (ctx->visibility() != nullptr) {
        _currentIndex->visible(ctx->visibility()->VISIBLE_SYMBOL() != nullptr);
    }
  }

  virtual void exitFulltextIndexOption(MySQLParser::FulltextIndexOptionContext *ctx) override {
    if (ctx->WITH_SYMBOL() != nullptr)
      _currentIndex->withParser(ctx->identifier()->getText());
  }

private:
  db_mysql_TableRef _table;
  std::string _schemaName;
  DbObjectsRefsCache &_refCache;
  bool _autoGenerateFkNames;

  db_mysql_IndexRef _currentIndex;
};

//----------------- ObjectListener -------------------------------------------------------------------------------------

ObjectListener::ObjectListener(db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject, bool caseSensitive)
  : DetailsListener(catalog, caseSensitive), _object(anObject) {
}

//----------------------------------------------------------------------------------------------------------------------

/**
 *	Returns the schema with the given name. If it doesn't exist it will be created.
 */
db_mysql_SchemaRef ObjectListener::ensureSchemaExists(const std::string &name) {
  return ensureSchemaExists(_catalog, name, _caseSensitive);
}

//----------------------------------------------------------------------------------------------------------------------

db_mysql_SchemaRef ObjectListener::ensureSchemaExists(db_CatalogRef catalog, const std::string &name,
                                                      bool caseSensitive) {
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

//----------------- DataTypeListener -----------------------------------------------------------------------------------

DataTypeListener::DataTypeListener(tree::ParseTree *tree, GrtVersionRef version,
                                   const grt::ListRef<db_SimpleDatatype> &typeList, StringListRef flags,
                                   const std::string &defaultCharsetName)
  : _version(version), _typeList(typeList), _flags(flags), _defaultCharsetName(defaultCharsetName) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void DataTypeListener::exitDataType(MySQLParser::DataTypeContext *ctx) {
  // A type name can consist of up to 3 parts. Most however just have a single part.
  size_t type = (ctx->nchar() != nullptr) ? ctx->nchar()->type->getType() : ctx->type->getType();
  std::string typeName = (ctx->nchar() != nullptr) ? "NCHAR" : base::toupper(ctx->type->getText());
  switch (type) {
    case MySQLLexer::NATIONAL_SYMBOL:
      if (ctx->CHAR_SYMBOL() != nullptr)
        typeName += " CHAR";
      if (ctx->VARYING_SYMBOL() != nullptr)
        typeName += " VARYING";
      if (ctx->VARCHAR_SYMBOL() != nullptr)
        typeName += " VARCHAR";
      break;

    case MySQLLexer::CHAR_SYMBOL:
      if (ctx->VARYING_SYMBOL() != nullptr)
        typeName += " VARYING";
      break;

    case MySQLLexer::NCHAR_SYMBOL:
      if (ctx->VARCHAR_SYMBOL() != nullptr)
        typeName += " VARCHAR";
      if (ctx->VARYING_SYMBOL() != nullptr)
        typeName += " VARYING";
      break;

    case MySQLLexer::LONG_SYMBOL:
      if (ctx->VARBINARY_SYMBOL() != nullptr)
        typeName += " VARBINARY";
      if (ctx->CHAR_SYMBOL() != nullptr)
        typeName += " CHAR VARYING";
      if (ctx->VARCHAR_SYMBOL() != nullptr)
        typeName += " VARCHAR";
      break;

    default:
      break;
  }

  dataType = MySQLParserServices::findDataType(_typeList, _version, typeName);
  if (dataType.is_valid()) { // Should always be valid at this point.
    // Unfortunately, the length + precision handling in WB is a bit crude and we cannot simply use what the grammar
    // dictates. So we have to inspect the associated simple data type to know where to store the parsed
    // length/precision values.
    if (dataType->characterMaximumLength() != bec::EMPTY_TYPE_MAXIMUM_LENGTH ||
        dataType->characterOctetLength() != bec::EMPTY_TYPE_OCTET_LENGTH ||
        dataType->dateTimePrecision() != bec::EMPTY_TYPE_MAXIMUM_LENGTH) {
      // Move a potential precision value into the length field.
      if (precision != bec::EMPTY_COLUMN_PRECISION) {
        length = precision;
        precision = bec::EMPTY_COLUMN_PRECISION;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void DataTypeListener::exitFieldLength(MySQLParser::FieldLengthContext *ctx) {
  // Value should be stored in the length field, but as commented above WB's handling is a bit crude.
  if (ctx->DECIMAL_NUMBER() != nullptr)
    precision = std::stoull(ctx->DECIMAL_NUMBER()->getText());
  else
    precision = std::stoull(ctx->real_ulonglong_number()->getText());
}

//----------------------------------------------------------------------------------------------------------------------

void DataTypeListener::exitPrecision(MySQLParser::PrecisionContext *ctx) {
  precision = std::stoull(ctx->INT_NUMBER(0)->getText());
  scale = std::stoull(ctx->INT_NUMBER(1)->getText());
}

//----------------------------------------------------------------------------------------------------------------------

void DataTypeListener::exitFieldOptions(MySQLParser::FieldOptionsContext *ctx) {
  if (!ctx->UNSIGNED_SYMBOL().empty()) {
    if (_flags.get_index("UNSIGNED") == BaseListRef::npos)
      _flags.insert("UNSIGNED");
  }
  if (!ctx->SIGNED_SYMBOL().empty()) {
    if (_flags.get_index("SIGNED") == BaseListRef::npos)
      _flags.insert("SIGNED");
  }
  if (!ctx->ZEROFILL_SYMBOL().empty()) {
    if (_flags.get_index("ZEROFILL") == BaseListRef::npos)
      _flags.insert("ZEROFILL");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void DataTypeListener::exitCharsetWithOptBinary(MySQLParser::CharsetWithOptBinaryContext *ctx) {
  std::string flag;
  bool insertBinary = false;

  if (ctx->ascii() != nullptr) {
    flag = "ASCII";
    insertBinary = ctx->ascii()->BINARY_SYMBOL() != nullptr;
  } else if (ctx->unicode() != nullptr) {
    flag = "UNICODE";
    insertBinary = ctx->unicode()->BINARY_SYMBOL() != nullptr;
  } else if (ctx->BYTE_SYMBOL() != nullptr)
    flag = "BYTE";
  else if (ctx->BINARY_SYMBOL() != nullptr || ctx->charset() != nullptr)
    insertBinary = ctx->BINARY_SYMBOL() != nullptr;

  if (!flag.empty() && _flags.get_index(flag) == BaseListRef::npos)
    _flags.insert(flag);
  if (insertBinary && _flags.get_index("BINARY") == BaseListRef::npos)
    _flags.insert("BINARY");
}

//----------------------------------------------------------------------------------------------------------------------

void DataTypeListener::exitCharsetName(MySQLParser::CharsetNameContext *ctx) {
  auto info = detailsForCharset(base::unquote(ctx->getText()), "", _defaultCharsetName);
  charsetName = info.first;
}

//----------------------------------------------------------------------------------------------------------------------

void DataTypeListener::exitTypeDatetimePrecision(MySQLParser::TypeDatetimePrecisionContext *ctx) {
  precision = std::stoull(ctx->INT_NUMBER()->getText());
}

//----------------------------------------------------------------------------------------------------------------------

void DataTypeListener::exitStringList(MySQLParser::StringListContext *ctx) {
  std::string params;
  for (auto &entry : ctx->textString()) {
    if (!params.empty())
      params += ", ";
    params += entry->getText();
  }
  explicitParams = "(" + params + ")";
}

//----------------- SchemaListener -------------------------------------------------------------------------------------

SchemaListener::SchemaListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject,
                               bool caseSensitive)
  : ObjectListener(catalog, anObject, caseSensitive) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void SchemaListener::enterCreateDatabase(MySQLParser::CreateDatabaseContext *ctx) {
  auto info = detailsForCharset(_catalog->defaultCharacterSetName(), _catalog->defaultCollationName(),
                                _catalog->defaultCharacterSetName());

  db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(_object);
  schema->defaultCharacterSetName(info.first);
  schema->defaultCollationName(info.second);
}

//----------------------------------------------------------------------------------------------------------------------

void SchemaListener::exitCreateDatabase(MySQLParser::CreateDatabaseContext *ctx) {
  db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(_object);
  schema->name(MySQLBaseLexer::sourceTextForContext(ctx->schemaName()));
  ignoreIfExists = ctx->ifNotExists() != nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

void SchemaListener::exitCharsetName(MySQLParser::CharsetNameContext *ctx) {
  db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(_object);
  std::string charsetName;
  if (ctx->DEFAULT_SYMBOL() != nullptr)
    charsetName = "default";
  else
    charsetName = base::tolower(MySQLBaseLexer::sourceTextForContext(ctx));

  auto info = detailsForCharset(charsetName, schema->defaultCollationName(), _catalog->defaultCharacterSetName());
  schema->defaultCharacterSetName(info.first);
  schema->defaultCollationName(info.second);
}

//----------------------------------------------------------------------------------------------------------------------

void SchemaListener::exitCollationName(MySQLParser::CollationNameContext *ctx) {
  db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(_object);
  std::string collationName;
  if (ctx->DEFAULT_SYMBOL() == nullptr)
    collationName = base::tolower(MySQLBaseLexer::sourceTextForContext(ctx));
  else
    collationName = "default";

  auto info = detailsForCollation(collationName, _catalog->defaultCollationName());
  schema->defaultCharacterSetName(info.first);
  schema->defaultCollationName(info.second);
}

//----------------------------------------------------------------------------------------------------------------------

void SchemaListener::exitDefaultEncryption(MySQLParser::DefaultEncryptionContext *ctx) {
  //TODO: implement
}

//----------------- TableListener --------------------------------------------------------------------------------------

TableListener::TableListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_mysql_SchemaRef schema,
                             db_mysql_TableRef &table, bool caseSensitive, bool autoGenerateFkNames,
                             DbObjectsRefsCache &refCache)
  : ObjectListener(catalog, table, caseSensitive), _refCache(refCache) {
  _schema = schema;
  _autoGenerateFkNames = autoGenerateFkNames;

  table->primaryKey(db_mysql_IndexRef());
  table->indices().remove_all();
  table->columns().remove_all();
  table->foreignKeys().remove_all();

  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void TableListener::exitTableName(MySQLParser::TableNameContext *ctx) {
  db_mysql_TableRef table = db_mysql_TableRef::cast_from(_object);

  IdentifierListener listener(ctx);

  table->name(listener.parts.back());
  if (listener.parts.size() > 1) {
    if (!listener.parts[0].empty())
      _schema = ensureSchemaExists(listener.parts[0]);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void TableListener::exitCreateTable(MySQLParser::CreateTableContext *ctx) {
  db_mysql_TableRef table = db_mysql_TableRef::cast_from(_object);

  table->isTemporary(ctx->TEMPORARY_SYMBOL() != nullptr);
  ignoreIfExists = ctx->ifNotExists() != nullptr;

  std::string schemaName = _schema.is_valid() ? _schema->name() : "";
  for (auto item : ctx->tableElementList()->tableElement()) {
    if (item->columnDefinition() != nullptr)
      ColumnDefinitionListener(item->columnDefinition(), _catalog, schemaName, table, _refCache);
    else
      KeyDefinitionListener(item->tableConstraintDef(), _catalog, schemaName, table, _refCache, _autoGenerateFkNames);
  }
  table->owner(_schema);
}

//----------------------------------------------------------------------------------------------------------------------

void TableListener::exitTableRef(MySQLParser::TableRefContext *ctx) {
  // CREATE TABLE LIKE...
  IdentifierListener listener(ctx);

  db_SchemaRef schema = _schema;
  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    schema = find_named_object_in_list(_catalog->schemata(), listener.parts[0]);

  if (schema.is_valid()) {
    db_TableRef otherTable = find_named_object_in_list(schema->tables(), listener.parts.back());
    if (otherTable.is_valid()) {
      db_mysql_TableRef table = db_mysql_TableRef::cast_from(_object);
      bool isTemporary = table->isTemporary() != 0; // Ensure this value stays as is.
      table = grt::copy_object(db_mysql_TableRef::cast_from(otherTable));
      table->isTemporary(isTemporary);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void TableListener::exitPartitionClause(MySQLParser::PartitionClauseContext *ctx) {
  db_mysql_TableRef table = db_mysql_TableRef::cast_from(_object);
  if (ctx->PARTITIONS_SYMBOL() != nullptr)
    table->partitionCount((size_t)std::stoull(ctx->real_ulong_number()->getText()));

  // If no partition count was given use the number of definitions we found.
  if (table->partitionCount() == 0)
    table->partitionCount(table->partitionDefinitions().count());

  // Similar for sub partitions. Code taken from old parser, but it looks strange.
  // Do all partitions must have the same number of sub partitions?
  if (table->partitionDefinitions().count() > 0)
    table->subpartitionCount(table->partitionDefinitions()[0]->subpartitionDefinitions().count());
}

//----------------------------------------------------------------------------------------------------------------------

void TableListener::exitPartitionDefKey(MySQLParser::PartitionDefKeyContext *ctx) {
  db_mysql_TableRef table = db_mysql_TableRef::cast_from(_object);
  if (ctx->LINEAR_SYMBOL() != nullptr)
    table->partitionType("LINEAR KEY");
  else
    table->partitionType("KEY");

  if (ctx->partitionKeyAlgorithm() != nullptr)
    table->partitionKeyAlgorithm((size_t)std::stoull(ctx->partitionKeyAlgorithm()->real_ulong_number()->getText()));

  auto list = ctx->identifierList();
  if (list != nullptr)
    table->partitionExpression(getIdentifierList(list));
}

//----------------------------------------------------------------------------------------------------------------------

void TableListener::exitPartitionDefHash(MySQLParser::PartitionDefHashContext *ctx) {
  db_mysql_TableRef table = db_mysql_TableRef::cast_from(_object);
  if (ctx->LINEAR_SYMBOL() != nullptr)
    table->partitionType("LINEAR HASH");
  else
    table->partitionType("HASH");

  table->partitionExpression(MySQLBaseLexer::sourceTextForContext(ctx->bitExpr()));
}

//----------------------------------------------------------------------------------------------------------------------

void TableListener::exitPartitionDefRangeList(MySQLParser::PartitionDefRangeListContext *ctx) {
  db_mysql_TableRef table = db_mysql_TableRef::cast_from(_object);
  table->partitionType(ctx->RANGE_SYMBOL() != nullptr ? "RANGE" : "LISTE");

  if (ctx->COLUMNS_SYMBOL() != nullptr) {
    auto list = ctx->identifierList();
    if (list != nullptr)
      table->partitionExpression(getIdentifierList(list));
  } else
    table->partitionExpression(MySQLBaseLexer::sourceTextForContext(ctx->bitExpr()));
}

//----------------------------------------------------------------------------------------------------------------------

void TableListener::exitSubPartitions(MySQLParser::SubPartitionsContext *ctx) {
  db_mysql_TableRef table = db_mysql_TableRef::cast_from(_object);

  std::string linearPrefix;
  if (ctx->LINEAR_SYMBOL() != nullptr)
    linearPrefix = "LINEAR ";
  if (ctx->HASH_SYMBOL() != nullptr) {
    table->partitionType(linearPrefix + "HASH");
    table->partitionExpression(MySQLBaseLexer::sourceTextForContext(ctx->bitExpr()));
  } else {
    table->partitionType(linearPrefix + "KEY");

    if (ctx->partitionKeyAlgorithm() != nullptr)
      table->partitionKeyAlgorithm((size_t)std::stoull(ctx->partitionKeyAlgorithm()->real_ulong_number()->getText()));

    auto list = ctx->identifierListWithParentheses()->identifierList();
    table->partitionExpression(getIdentifierList(list));
  }

  MySQLParser::Real_ulong_numberContext* numberContext = ctx->real_ulong_number();
  if (ctx->SUBPARTITION_SYMBOL() != nullptr && numberContext != nullptr)
    table->subpartitionCount(static_cast<size_t>(std::stoull(numberContext->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

static void evaluatePartitionOption(db_mysql_PartitionDefinitionRef definition,
                                    MySQLParser::PartitionOptionContext *ctx) {
  switch (ctx->option->getType()) {
    case MySQLLexer::TABLESPACE_SYMBOL:
      definition->tableSpace(ctx->identifier()->getText());
      break;

    case MySQLLexer::ENGINE_SYMBOL:
      definition->engine(ctx->engineRef()->getText());
      break;

    case MySQLLexer::NODEGROUP_SYMBOL:
      definition->nodeGroupId((size_t)std::stoull(ctx->real_ulong_number()->getText()));
      break;

    case MySQLLexer::MAX_ROWS_SYMBOL:
      definition->maxRows(ctx->textLiteral()->getText());
      break;

    case MySQLLexer::MIN_ROWS_SYMBOL:
      definition->minRows(ctx->textLiteral()->getText());
      break;

    case MySQLLexer::DATA_SYMBOL:
      definition->dataDirectory(ctx->textLiteral()->getText());
      break;

    case MySQLLexer::INDEX_SYMBOL:
      definition->indexDirectory(ctx->textLiteral()->getText());
      break;

    case MySQLLexer::COMMENT_SYMBOL:
      definition->comment(ctx->textLiteral()->getText());
      break;

    default:
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void TableListener::exitPartitionDefinition(MySQLParser::PartitionDefinitionContext *ctx) {
  db_mysql_TableRef table = db_mysql_TableRef::cast_from(_object);

  db_mysql_PartitionDefinitionRef definition(grt::Initialized);
  definition->owner(table);
  definition->name(ctx->identifier()->getText());

  // Collect the expression text of the partition VALUES part if there is any. Don't include parentheses.
  if (ctx->VALUES_SYMBOL() != nullptr) {
    std::string expression;
    if (ctx->MAXVALUE_SYMBOL() != nullptr)
      expression = "MAX_VALUE";
    else if (ctx->partitionValueItemListParen() != nullptr)
      expression = MySQLBaseLexer::sourceTextForRange(ctx->partitionValueItemListParen()->partitionValueItem().front(),
                                                      ctx->partitionValueItemListParen()->partitionValueItem().back());
    else
      /*
       TODO: At the moment WB (in particular code generation) is not able to cope with lists of lists here.
       So for now we pretend there is only a single list.
      expression = MySQLBaseLexer::sourceTextForRange(ctx->partitionValuesIn()->partitionValueItemListParen().front(),
                                                      ctx->partitionValuesIn()->partitionValueItemListParen().back());
       */
      expression = MySQLBaseLexer::sourceTextForRange(
        ctx->partitionValuesIn()->partitionValueItemListParen(0)->partitionValueItem().front(),
        ctx->partitionValuesIn()->partitionValueItemListParen(0)->partitionValueItem().back());
    definition->value(expression);
  }

  for (auto &option : ctx->partitionOption())
    evaluatePartitionOption(definition, option);

  for (auto &subPartition : ctx->subpartitionDefinition()) {
    db_mysql_PartitionDefinitionRef subDefinition(grt::Initialized);
    subDefinition->name(subPartition->textOrIdentifier()->getText());
    for (auto &option : subPartition->partitionOption())
      evaluatePartitionOption(subDefinition, option);
    definition->subpartitionDefinitions().insert(subDefinition);
  }

  table->partitionDefinitions().insert(definition);
}

//----------------------------------------------------------------------------------------------------------------------

void TableListener::exitDuplicateAsQueryExpression(MySQLParser::DuplicateAsQueryExpressionContext *ctx) {
  // This is a creation-only part, i.e. it is not returned by the server when asking for the creation SQL code.
  // Similar for createSelect, which is used either in this context or createTable.
  // Implementing that is tricky, because we would essentially have to simulate a SELECT run to determine the actual
  // columns to return.
}

//----------------------------------------------------------------------------------------------------------------------

void TableListener::exitCreateTableOptions(MySQLParser::CreateTableOptionsContext *ctx) {
  db_mysql_TableRef table = db_mysql_TableRef::cast_from(_object);

  std::string schemaName = _schema.is_valid() ? _schema->name() : "";
  std::string defaultCharset = _schema.is_valid() ? _schema->defaultCharacterSetName() : "";
  std::string defaultCollation = _schema.is_valid() ? _schema->defaultCollationName() : "";
  if (defaultCollation.empty() && !defaultCharset.empty())
    defaultCollation = defaultCollationForCharset(defaultCharset);

  for (auto &option : ctx->createTableOption()) {
    if (option->option == nullptr) {
      // Collation/Charset handling.
      if (option->defaultCollation() != nullptr) {
        auto info = detailsForCollation(
          MySQLBaseLexer::sourceTextForContext(option->defaultCollation()->collationName()), defaultCollation);
        table->defaultCharacterSetName(info.first);
        table->defaultCollationName(info.second);
      } else {
        auto info =
          detailsForCharset(MySQLBaseLexer::sourceTextForContext(option->defaultCharset()->charsetName()),
                            defaultCollation, defaultCharset);
        table->defaultCharacterSetName(info.first);
        table->defaultCollationName(info.second); // Collation name or DEFAULT.
      }
      continue;
    }

    switch (option->option->getType()) {
      case MySQLLexer::ENGINE_SYMBOL:
      case MySQLLexer::TYPE_SYMBOL:
        table->tableEngine(option->engineRef()->getText());
        break;

      case MySQLLexer::MAX_ROWS_SYMBOL:
        table->maxRows(option->ulonglong_number()->getText());
        break;

      case MySQLLexer::MIN_ROWS_SYMBOL:
        table->minRows(option->ulonglong_number()->getText());
        break;

      case MySQLLexer::AVG_ROW_LENGTH_SYMBOL:
        table->avgRowLength(option->ulong_number()->getText());
        break;

      case MySQLLexer::PASSWORD_SYMBOL:
        table->password(MySQLBaseLexer::sourceTextForContext(option->textStringLiteral()));
        break;

      case MySQLLexer::COMMENT_SYMBOL:
        table->comment(MySQLBaseLexer::sourceTextForContext(option->textStringLiteral()));
        break;

      case MySQLLexer::COMPRESSION_SYMBOL:
        // TODO: table->compression(option->textString()->getText());
        break;

      case MySQLLexer::ENCRYPTION_SYMBOL: {
        // TODO: std::string value = base::tolower(option->textString()->getText());
        // TODO: table->encryption(value.size() == 1 && value = "n" ? false : true); // Only 'Y' or 'N', case
        // insensitive.
        break;
      }

      case MySQLLexer::AUTO_INCREMENT_SYMBOL:
        table->nextAutoInc(option->ulonglong_number()->getText());
        break;

      case MySQLLexer::PACK_KEYS_SYMBOL:
        if (option->ternaryOption()->DEFAULT_SYMBOL() != nullptr)
          table->packKeys(option->ternaryOption()->DEFAULT_SYMBOL()->getText());
        else
          table->packKeys(option->ternaryOption()->ulong_number()->getText());
        break;

      case MySQLLexer::STATS_AUTO_RECALC_SYMBOL:
        if (option->ternaryOption()->DEFAULT_SYMBOL() != nullptr)
          table->statsAutoRecalc(option->ternaryOption()->DEFAULT_SYMBOL()->getText());
        else
          table->statsAutoRecalc(option->ternaryOption()->ulong_number()->getText());
        break;

      case MySQLLexer::STATS_PERSISTENT_SYMBOL:
        if (option->ternaryOption()->DEFAULT_SYMBOL() != nullptr)
          table->statsPersistent(option->ternaryOption()->DEFAULT_SYMBOL()->getText());
        else
          table->statsPersistent(option->ternaryOption()->ulong_number()->getText());
        break;

      case MySQLLexer::STATS_SAMPLE_PAGES_SYMBOL:
        // Note: we denote the default as value 0 (like it is done in the server).
        if (option->ternaryOption()->DEFAULT_SYMBOL() != nullptr)
          table->statsSamplePages(0);
        else
          table->statsSamplePages((size_t)std::stoull(option->ternaryOption()->ulong_number()->getText()));
        break;

      case MySQLLexer::CHECKSUM_SYMBOL:
      case MySQLLexer::TABLE_CHECKSUM_SYMBOL:
        table->checksum((size_t)std::stoull(option->ulong_number()->getText()));
        break;

      case MySQLLexer::DELAY_KEY_WRITE_SYMBOL:
        table->delayKeyWrite((size_t)std::stoull(option->ulong_number()->getText()));
        break;

      case MySQLLexer::ROW_FORMAT_SYMBOL:
        table->rowFormat(option->format->getText());
        break;

      case MySQLLexer::UNION_SYMBOL: { // Only for merge engine.
        std::string value;
        for (auto &tableRef : option->tableRefList()->tableRef()) {
          IdentifierListener listener(tableRef);

          if (!value.empty())
            value += ", ";

          if (listener.parts.size() < 2 || listener.parts[0].empty()) {
            size_t nameIndex = 0;
            if (listener.parts[0].empty())
              ++nameIndex;

            // In order to avoid diff problems explicitly qualify unqualified tables
            // with the current schema name.
            value += schemaName + '.' + listener.parts[nameIndex];
          } else {
            ensureSchemaExists(listener.parts[0]);
            value += listener.parts[0] + "." + listener.parts[1];
          }
        }
        table->mergeUnion(value);
        break;
      }

      case MySQLLexer::INSERT_METHOD_SYMBOL:
        table->mergeInsert(option->method->getText());
        break;

      case MySQLLexer::DATA_SYMBOL:
        table->tableDataDir(MySQLBaseLexer::sourceTextForContext(option->textString()));
        break;

      case MySQLLexer::INDEX_SYMBOL:
        table->tableIndexDir(MySQLBaseLexer::sourceTextForContext(option->textString()));
        break;

      case MySQLLexer::TABLESPACE_SYMBOL:
        table->tableSpace(option->identifier()->getText());
        break;

      case MySQLLexer::STORAGE_SYMBOL:
        //(DISK_SYMBOL | MEMORY_SYMBOL) ignored for now, as not documented.
        break;

      case MySQLLexer::CONNECTION_SYMBOL:
        table->connectionString(MySQLBaseLexer::sourceTextForContext(option->textString()));
        break;

      case MySQLLexer::KEY_BLOCK_SIZE_SYMBOL:
        table->keyBlockSize(option->ulong_number()->getText());
        break;

      default:
        break;
    }
  }
}

//----------------- TableAlterListener ---------------------------------------------------------------------------------

TableAlterListener::TableAlterListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog,
                                       db_DatabaseObjectRef tableOrView, bool caseSensitive, bool autoGenerateFkNames,
                                       DbObjectsRefsCache &refCache)
  : ObjectListener(catalog, tableOrView, caseSensitive),
    _autoGenerateFkNames(autoGenerateFkNames),
    _refCache(refCache) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void TableAlterListener::exitAlterListItem(MySQLParser::AlterListItemContext *ctx) {
  db_mysql_SchemaRef schema = db_mysql_SchemaRef::cast_from(_object->owner());

  db_mysql_TableRef table;
  db_mysql_ViewRef view;
  std::string name;
  if (db_mysql_TableRef::can_wrap(_object)) {
    table = db_mysql_TableRef::cast_from(_object);
    name = table->name();
  } else {
    view = db_mysql_ViewRef::cast_from(_object);
    name = view->name();
  }

  // Many changes can go here, but we add them as needed.
  if (ctx->tableConstraintDef() != nullptr && table.is_valid())
    KeyDefinitionListener(ctx->tableConstraintDef(), _catalog, schema->name(), table, _refCache, _autoGenerateFkNames);

  if (ctx->tableName() != nullptr) {
    // Rename table or view. Can be a move as well (but not for views).
    IdentifierListener listener(ctx->tableName());

    db_mysql_SchemaRef targetSchema = schema;
    if (listener.parts.size() > 1 && !listener.parts[0].empty())
      targetSchema = ensureSchemaExists(_catalog, listener.parts[0], _caseSensitive);

    // TODO: should we add sanity checks here (e.g. duplicate names)?
    if (view.is_valid()) {
      // Cannot move between schemas.
      if (schema == targetSchema)
        view->name(listener.parts.back());
    } else {
      // Renaming a table.
      if (schema != targetSchema) {
        schema->tables()->remove(table);
        targetSchema->tables().insert(table);
      }
      table->name(listener.parts.back());
    }
  }
}

//----------------- LogfileGroupListener -------------------------------------------------------------------------------

LogfileGroupListener::LogfileGroupListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog,
                                           db_DatabaseObjectRef anObject, bool caseSensitive)
  : ObjectListener(catalog, anObject, caseSensitive) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void LogfileGroupListener::exitCreateLogfileGroup(MySQLParser::CreateLogfileGroupContext *ctx) {
  IdentifierListener listener(ctx->logfileGroupName());

  db_mysql_LogFileGroupRef group = db_mysql_LogFileGroupRef::cast_from(_object);
  group->name(listener.parts[0]);

  // TODO: no info is stored about UNDO or REDO.
  group->undoFile(MySQLBaseLexer::sourceTextForContext(ctx->textLiteral()));
}

//----------------------------------------------------------------------------------------------------------------------

void LogfileGroupListener::exitTsOptionInitialSize(MySQLParser::TsOptionInitialSizeContext *ctx) {
  db_mysql_LogFileGroupRef group = db_mysql_LogFileGroupRef::cast_from(_object);
  group->initialSize(numberValue(ctx->sizeNumber()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void LogfileGroupListener::exitTsOptionUndoRedoBufferSize(MySQLParser::TsOptionUndoRedoBufferSizeContext *ctx) {
  db_mysql_LogFileGroupRef group = db_mysql_LogFileGroupRef::cast_from(_object);
  if (ctx->UNDO_BUFFER_SIZE_SYMBOL() != nullptr)
    group->undoBufferSize(numberValue(ctx->sizeNumber()->getText()));
  else
    group->redoBufferSize(numberValue(ctx->sizeNumber()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void LogfileGroupListener::exitTsOptionNodegroup(MySQLParser::TsOptionNodegroupContext *ctx) {
  db_mysql_LogFileGroupRef group = db_mysql_LogFileGroupRef::cast_from(_object);
  group->nodeGroupId(static_cast<size_t>(std::stoull(ctx->real_ulong_number()->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

void LogfileGroupListener::exitTsOptionEngine(MySQLParser::TsOptionEngineContext *ctx) {
  db_mysql_LogFileGroupRef group = db_mysql_LogFileGroupRef::cast_from(_object);
  group->engine(base::unquote(ctx->engineRef()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void LogfileGroupListener::exitTsOptionWait(MySQLParser::TsOptionWaitContext *ctx) {
  db_mysql_LogFileGroupRef group = db_mysql_LogFileGroupRef::cast_from(_object);
  group->wait(ctx->WAIT_SYMBOL() != nullptr);
}

//----------------------------------------------------------------------------------------------------------------------

void LogfileGroupListener::exitTsOptionComment(MySQLParser::TsOptionCommentContext *ctx) {
  db_mysql_LogFileGroupRef group = db_mysql_LogFileGroupRef::cast_from(_object);
  group->comment(base::unquote(ctx->textLiteral()->getText()));
}

//----------------- RoutineListener ------------------------------------------------------------------------------------

RoutineListener::RoutineListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_mysql_RoutineRef routine,
                                 bool caseSensitive)
  : ObjectListener(catalog, routine, caseSensitive) {
  routine->params().remove_all();
  routine->modelOnly(0);

  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void RoutineListener::exitDefinerClause(MySQLParser::DefinerClauseContext *ctx) {
  db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(_object);
  routine->definer(MySQLBaseLexer::sourceTextForContext(ctx->user(), true));
}

//----------------------------------------------------------------------------------------------------------------------

void RoutineListener::exitCreateProcedure(MySQLParser::CreateProcedureContext *ctx) {
  db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(_object);

  routine->routineType("procedure");
  readRoutineName(ctx->procedureName());
}

//----------------------------------------------------------------------------------------------------------------------

void RoutineListener::exitCreateFunction(MySQLParser::CreateFunctionContext *ctx) {
  db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(_object);

  routine->returnDatatype(MySQLBaseLexer::sourceTextForContext(ctx->typeWithOptCollate()));

  routine->routineType("function");
  readRoutineName(ctx->functionName());
}

//----------------------------------------------------------------------------------------------------------------------

void RoutineListener::exitCreateUdf(MySQLParser::CreateUdfContext *ctx) {
  db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(_object);

  routine->routineType("udf");
  readRoutineName(ctx->udfName());

  routine->returnDatatype(ctx->type->getText());

  // SONAME is currently ignored.
}

//----------------------------------------------------------------------------------------------------------------------

void RoutineListener::exitProcedureParameter(MySQLParser::ProcedureParameterContext *ctx) {
  if (ctx->type != nullptr)
    _currentParameter->paramType(ctx->type->getText());
  else
    _currentParameter->paramType("IN");
}

//----------------------------------------------------------------------------------------------------------------------

void RoutineListener::enterFunctionParameter(MySQLParser::FunctionParameterContext *ctx) {
  db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(_object);

  _currentParameter = db_mysql_RoutineParamRef(grt::Initialized);
  _currentParameter->owner(routine);
  routine->params().insert(_currentParameter);
}

//----------------------------------------------------------------------------------------------------------------------

void RoutineListener::exitFunctionParameter(MySQLParser::FunctionParameterContext *ctx) {
  // Called for both functions and procedures.
  _currentParameter->name(MySQLBaseLexer::sourceTextForContext(ctx->parameterName()));
  _currentParameter->datatype(MySQLBaseLexer::sourceTextForContext(ctx->typeWithOptCollate()));
}

//----------------------------------------------------------------------------------------------------------------------

void RoutineListener::exitRoutineOption(MySQLParser::RoutineOptionContext *ctx) {
  db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(_object);

  // For now we only store comments and security settings.
  switch (ctx->option->getType()) {
    case MySQLLexer::SQL_SYMBOL:
      routine->security(ctx->security->getText());
      break;

    case MySQLLexer::COMMENT_SYMBOL:
      routine->comment(MySQLBaseLexer::sourceTextForContext(ctx->textLiteral()));
      break;

    default:
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void RoutineListener::readRoutineName(ParserRuleContext *ctx) {
  db_mysql_RoutineRef routine = db_mysql_RoutineRef::cast_from(_object);

  IdentifierListener listener(ctx);
  routine->name(listener.parts.back());

  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    routine->owner(ensureSchemaExists(listener.parts[0]));
}

//----------------- IndexListener --------------------------------------------------------------------------------------

IndexListener::IndexListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_mysql_SchemaRef schema,
                             db_mysql_IndexRef index, bool caseSensitive, DbObjectsRefsCache &refCache)
  : ObjectListener(catalog, index, caseSensitive), _schema(schema), _refCache(refCache) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void IndexListener::exitCreateIndex(MySQLParser::CreateIndexContext *ctx) {
  db_mysql_IndexRef index = db_mysql_IndexRef::cast_from(_object);

  switch (ctx->type->getType()) {
    case MySQLLexer::INDEX_SYMBOL:
      if (ctx->UNIQUE_SYMBOL() != nullptr) {
        index->unique(1);
        index->indexType("UNIQUE");
      } else
        index->indexType(formatIndexType(ctx->type->getText()));
      break;

    case MySQLLexer::FULLTEXT_SYMBOL:
    case MySQLLexer::SPATIAL_SYMBOL:
      index->indexType(formatIndexType(ctx->type->getText()));
      break;

    default:
      break;
  }

  if (ctx->indexNameAndType() != nullptr) {
    index->name(base::unquote(ctx->indexNameAndType()->indexName()->getText()));
  } else {
    index->name(base::unquote(ctx->indexName()->getText()));
  }
}

//----------------------------------------------------------------------------------------------------------------------

void IndexListener::exitIndexType(MySQLParser::IndexTypeContext *ctx) {
  db_mysql_IndexRef index = db_mysql_IndexRef::cast_from(_object);
  index->indexKind(ctx->algorithm->getText());
}

//----------------------------------------------------------------------------------------------------------------------

void IndexListener::exitCreateIndexTarget(MySQLParser::CreateIndexTargetContext *ctx) {
  db_mysql_IndexRef index = db_mysql_IndexRef::cast_from(_object);

  IdentifierListener listener(ctx->tableRef());

  db_mysql_TableRef table;
  db_mysql_SchemaRef schema = _schema;
  if (_catalog.is_valid()) {
    if (listener.parts.size() > 1 && !listener.parts[0].empty())
      schema = ensureSchemaExists(listener.parts[0]);
    table = find_named_object_in_list(schema->tables(), listener.parts.back(), _caseSensitive);
    if (table.is_valid()) {
      index->owner(table);
      parseKeyList(ctx->keyListVariants(), table, index, _refCache);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void IndexListener::exitCommonIndexOption(MySQLParser::CommonIndexOptionContext *ctx) {
  db_mysql_IndexRef index = db_mysql_IndexRef::cast_from(_object);

  if (ctx->KEY_BLOCK_SIZE_SYMBOL() != nullptr)
    index->keyBlockSize((size_t)std::stoull(ctx->ulong_number()->getText()));
  if (ctx->COMMENT_SYMBOL() != nullptr)
    index->comment(ctx->textLiteral()->getText());
}

//----------------------------------------------------------------------------------------------------------------------

void IndexListener::exitFulltextIndexOption(MySQLParser::FulltextIndexOptionContext *ctx) {
  db_mysql_IndexRef index = db_mysql_IndexRef::cast_from(_object);

  if (ctx->WITH_SYMBOL() != nullptr)
    index->withParser(ctx->identifier()->getText());
}

//----------------------------------------------------------------------------------------------------------------------

void IndexListener::exitAlterAlgorithmOption(MySQLParser::AlterAlgorithmOptionContext *ctx) {
  db_mysql_IndexRef index = db_mysql_IndexRef::cast_from(_object);

  if (ctx->DEFAULT_SYMBOL() != nullptr)
    index->algorithm("DEFAULT");
  else {
    // The algorithm can be any text, but allowed are only a small number of values.
    std::string algorithm = base::toupper(ctx->identifier()->getText());
    if (algorithm == "INPLACE" || algorithm == "COPY")
      index->algorithm(algorithm);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void IndexListener::exitAlterLockOption(MySQLParser::AlterLockOptionContext *ctx) {
  db_mysql_IndexRef index = db_mysql_IndexRef::cast_from(_object);

  if (ctx->DEFAULT_SYMBOL() != nullptr)
    index->lockOption("DEFAULT");
  else {
    // The lock type can be any text, but allowed are only a small number of values.
    std::string lock = base::toupper(ctx->identifier()->getText());
    if (lock == "NONE" || lock == "SHARED" || lock == "EXCLUSIVE")
      index->lockOption(lock);
  }
}

//----------------- TriggerListener ------------------------------------------------------------------------------------

TriggerListener::TriggerListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_mysql_SchemaRef schema,
                                 db_mysql_TriggerRef trigger, bool caseSensitive)
  : ObjectListener(catalog, trigger, caseSensitive), _schema(schema) {
  trigger->enabled(1);

  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void TriggerListener::exitDefinerClause(MySQLParser::DefinerClauseContext *ctx) {
  db_mysql_TriggerRef trigger = db_mysql_TriggerRef::cast_from(_object);

  trigger->definer(MySQLBaseLexer::sourceTextForContext(ctx->user(), true));
}

//----------------------------------------------------------------------------------------------------------------------

void TriggerListener::exitCreateTrigger(MySQLParser::CreateTriggerContext *ctx) {
  db_mysql_TriggerRef trigger = db_mysql_TriggerRef::cast_from(_object);

  IdentifierListener listener(ctx->triggerName());

  // We store triggers relative to the tables they act on, so we ignore here any qualifying schema.
  trigger->name(listener.parts.back());

  trigger->timing(ctx->timing->getText());
  trigger->event(ctx->event->getText());

  // Trigger table referencing is a bit different than for other objects because we need
  // the table now to add the trigger to it. We cannot defer that to the resolveReferences() call.
  // This has the implication that we can only work with tables we have found so far.
  listener.parts.clear();
  tree::ParseTreeWalker::DEFAULT.walk(&listener, ctx->tableRef());

  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    _schema = ensureSchemaExists(_catalog, listener.parts[0], _caseSensitive);
  db_mysql_TableRef table = find_named_object_in_list(_schema->tables(), listener.parts.back(), _caseSensitive);
  if (!table.is_valid()) {
    // If we don't find a table with the given name we create a stub object to be used instead.
    table = db_mysql_TableRef(grt::Initialized);
    table->owner(_schema);
    table->isStub(1);
    table->name(listener.parts.back());
    table->oldName(listener.parts.back());
    _schema->tables().insert(table);
  }

  trigger->owner(table);
}

//----------------------------------------------------------------------------------------------------------------------

void TriggerListener::exitTriggerFollowsPrecedesClause(MySQLParser::TriggerFollowsPrecedesClauseContext *ctx) {
  db_mysql_TriggerRef trigger = db_mysql_TriggerRef::cast_from(_object);

  trigger->ordering(ctx->ordering->getText());
  trigger->otherTrigger(MySQLBaseLexer::sourceTextForContext(ctx->textOrIdentifier()));

  // Note: ignoreIfExists cannot be derived from the existance of OR REPLACE, which has the opposite meaning of IF NOT
  // EXISTS.
}

//----------------- ViewListener ---------------------------------------------------------------------------------------

ViewListener::ViewListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject,
                           bool caseSensitive)
  : ObjectListener(catalog, anObject, caseSensitive) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void ViewListener::exitCreateView(MySQLParser::CreateViewContext *ctx) {
  db_mysql_ViewRef view = db_mysql_ViewRef::cast_from(_object);
  view->modelOnly(0);

  IdentifierListener listener(ctx->viewName());
  view->name(listener.parts.back());

  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    view->owner(ensureSchemaExists(listener.parts[0]));
}

//----------------------------------------------------------------------------------------------------------------------

void ViewListener::exitViewCheckOption(MySQLParser::ViewCheckOptionContext *ctx) {
  db_mysql_ViewRef view = db_mysql_ViewRef::cast_from(_object);
  view->withCheckCondition(true);
}

//----------------------------------------------------------------------------------------------------------------------

void ViewListener::exitViewAlgorithm(MySQLParser::ViewAlgorithmContext *ctx) {
  db_mysql_ViewRef view = db_mysql_ViewRef::cast_from(_object);

  switch (ctx->algorithm->getType()) {
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
}

//----------------------------------------------------------------------------------------------------------------------

void ViewListener::exitDefinerClause(MySQLParser::DefinerClauseContext *ctx) {
  db_mysql_ViewRef view = db_mysql_ViewRef::cast_from(_object);
  view->definer(MySQLBaseLexer::sourceTextForContext(ctx->user(), true));
}

//----------------- ServerListener -------------------------------------------------------------------------------------

ServerListener::ServerListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject,
                               bool caseSensitive)
  : ObjectListener(catalog, anObject, caseSensitive) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void ServerListener::exitCreateServer(MySQLParser::CreateServerContext *ctx) {
  db_mysql_ServerLinkRef server = db_mysql_ServerLinkRef::cast_from(_object);
  server->modelOnly(0);

  IdentifierListener listener(ctx->serverName());
  server->name(listener.parts.back());

  server->wrapperName(base::unquote(ctx->textOrIdentifier()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void ServerListener::exitServerOption(MySQLParser::ServerOptionContext *ctx) {
  db_mysql_ServerLinkRef server = db_mysql_ServerLinkRef::cast_from(_object);

  switch (ctx->option->getType()) {
    case MySQLLexer::HOST_SYMBOL:
      server->host(base::unquote(ctx->textLiteral()->getText()));
      break;
    case MySQLLexer::DATABASE_SYMBOL:
      server->schema(base::unquote(ctx->textLiteral()->getText()));
      break;
    case MySQLLexer::USER_SYMBOL:
      server->user(base::unquote(ctx->textLiteral()->getText()));
      break;
    case MySQLLexer::PASSWORD_SYMBOL:
      server->password(base::unquote(ctx->textLiteral()->getText()));
      break;
    case MySQLLexer::SOCKET_SYMBOL:
      server->socket(base::unquote(ctx->textLiteral()->getText()));
      break;
    case MySQLLexer::OWNER_SYMBOL:
      server->ownerUser(base::unquote(ctx->textLiteral()->getText()));
      break;
    case MySQLLexer::PORT_SYMBOL:
      server->port(ctx->ulong_number()->getText()); // TODO: the grt definition should be int not string.
      break;
  }
}

//----------------- TablespaceListener ---------------------------------------------------------------------------------

TablespaceListener::TablespaceListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog,
                                       db_DatabaseObjectRef anObject, bool caseSensitive)
  : ObjectListener(catalog, anObject, caseSensitive) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitCreateTablespace(MySQLParser::CreateTablespaceContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);
  tablespace->modelOnly(0);

  IdentifierListener listener(ctx->tablespaceName());
  tablespace->name(listener.parts.back());
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitLogfileGroupRef(MySQLParser::LogfileGroupRefContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);

  db_LogFileGroupRef logfileGroup = find_named_object_in_list(_catalog->logFileGroups(), base::unquote(ctx->getText()));
  if (logfileGroup.is_valid())
    tablespace->logFileGroup(logfileGroup);
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitTsDataFile(MySQLParser::TsDataFileContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);
  tablespace->dataFile(base::unquote(ctx->textLiteral()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitTsOptionInitialSize(MySQLParser::TsOptionInitialSizeContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);
  tablespace->initialSize(numberValue(ctx->sizeNumber()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitTsOptionAutoextendSize(MySQLParser::TsOptionAutoextendSizeContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);
  tablespace->autoExtendSize(numberValue(ctx->sizeNumber()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitTsOptionMaxSize(MySQLParser::TsOptionMaxSizeContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);
  tablespace->maxSize(numberValue(ctx->sizeNumber()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitTsOptionExtentSize(MySQLParser::TsOptionExtentSizeContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);
  tablespace->extentSize(numberValue(ctx->sizeNumber()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitTsOptionNodegroup(MySQLParser::TsOptionNodegroupContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);

  // An integer or hex number (no suffix).
  tablespace->nodeGroupId(numberValue(ctx->real_ulong_number()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitTsOptionEngine(MySQLParser::TsOptionEngineContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);
  tablespace->engine(base::unquote(ctx->engineRef()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitTsOptionWait(MySQLParser::TsOptionWaitContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);
  tablespace->wait(ctx->WAIT_SYMBOL() != nullptr);
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitTsOptionComment(MySQLParser::TsOptionCommentContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);
  tablespace->comment(base::unquote(ctx->textLiteral()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitTsOptionFileblockSize(MySQLParser::TsOptionFileblockSizeContext *ctx) {
  db_mysql_TablespaceRef tablespace = db_mysql_TablespaceRef::cast_from(_object);
  tablespace->fileBlockSize(static_cast<size_t>(std::stoull(ctx->sizeNumber()->getText())));
}

//----------------------------------------------------------------------------------------------------------------------

void TablespaceListener::exitTsOptionEncryption(MySQLParser::TsOptionEncryptionContext *ctx) {

}

//----------------- EventListener --------------------------------------------------------------------------------------

EventListener::EventListener(tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject,
                             bool caseSensitive)
  : ObjectListener(catalog, anObject, caseSensitive) {
  tree::ParseTreeWalker::DEFAULT.walk(this, tree);
}

//----------------------------------------------------------------------------------------------------------------------

void EventListener::exitDefinerClause(MySQLParser::DefinerClauseContext *ctx) {
  db_mysql_EventRef event = db_mysql_EventRef::cast_from(_object);
  event->definer(MySQLBaseLexer::sourceTextForContext(ctx->user(), true));
}

//----------------------------------------------------------------------------------------------------------------------

void EventListener::exitCreateEvent(MySQLParser::CreateEventContext *ctx) {
  db_mysql_EventRef event = db_mysql_EventRef::cast_from(_object);

  ignoreIfExists = ctx->ifNotExists() != nullptr;

  IdentifierListener listener(ctx->eventName());
  event->name(listener.parts.back());

  if (listener.parts.size() > 1 && !listener.parts[0].empty())
    event->owner(ensureSchemaExists(listener.parts[0]));

  if (ctx->PRESERVE_SYMBOL() != nullptr)
    event->preserved(ctx->NOT_SYMBOL() == nullptr);

  if (ctx->ENABLE_SYMBOL() != nullptr || ctx->DISABLE_SYMBOL() != nullptr)
    event->enabled(ctx->ENABLE_SYMBOL() != nullptr);

  if (ctx->COMMENT_SYMBOL() != nullptr)
    event->comment(base::unquote(ctx->textLiteral()->getText()));
}

//----------------------------------------------------------------------------------------------------------------------

void EventListener::exitSchedule(MySQLParser::ScheduleContext *ctx) {
  db_mysql_EventRef event = db_mysql_EventRef::cast_from(_object);

  event->at(MySQLBaseLexer::sourceTextForContext(ctx->expr(0)));
  event->useInterval(ctx->EVERY_SYMBOL() != nullptr);
  if (event->useInterval()) {
    event->intervalUnit(MySQLBaseLexer::sourceTextForContext(ctx->interval()));

    size_t expressionIndex = 1;
    if (ctx->STARTS_SYMBOL() != nullptr)
      event->intervalStart(MySQLBaseLexer::sourceTextForContext(ctx->expr(expressionIndex++)));

    if (ctx->ENDS_SYMBOL() != nullptr)
      event->intervalEnd(MySQLBaseLexer::sourceTextForContext(ctx->expr(expressionIndex)));
  }
}

//----------------------------------------------------------------------------------------------------------------------

