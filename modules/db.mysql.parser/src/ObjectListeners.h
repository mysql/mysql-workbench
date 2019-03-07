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

#pragma once

namespace parsers {

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


  // Object listeners are used to implement the visitor pattern when walking a parse tree to get object specific
  // informations out. For easier handling and readability we use individual listeners for each schema object type.

  // Specialized listener for (qualified) identifiers.
  class IdentifierListener : public parsers::MySQLParserBaseListener {
  public:
    std::vector<std::string> parts; // 1 - 3 identifier parts.

    IdentifierListener(antlr4::tree::ParseTree *tree);

    virtual void enterIdentifier(MySQLParser::IdentifierContext *ctx) override;
  };
  
  // The DetailsListener class adds some support code needed by both the ObjectListener classes as well as the
  // support listeners used for individual subparts.
  class DetailsListener : public parsers::MySQLParserBaseListener {
  public:
    DetailsListener(db_mysql_CatalogRef catalog, bool caseSensitive);

  protected:
    db_mysql_CatalogRef _catalog;
    bool _caseSensitive;
  };

  class ObjectListener : public DetailsListener {
  protected:
    db_DatabaseObjectRef _object;

  public:
    bool ignoreIfExists = false;

    ObjectListener(db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject, bool caseSensitive);

    db_mysql_SchemaRef ensureSchemaExists(const std::string &name);
    static db_mysql_SchemaRef ensureSchemaExists(db_CatalogRef catalog, const std::string &name, bool caseSensitive);
  };

  class DataTypeListener : public parsers::MySQLParserBaseListener {
  public:
    db_SimpleDatatypeRef dataType;
    ssize_t scale = bec::EMPTY_COLUMN_SCALE;
    ssize_t precision = bec::EMPTY_COLUMN_PRECISION;
    ssize_t length = bec::EMPTY_COLUMN_LENGTH;
    std::string charsetName;
    std:: string explicitParams;

    DataTypeListener(antlr4::tree::ParseTree *tree, GrtVersionRef version, const grt::ListRef<db_SimpleDatatype> &typeList,
                     grt::StringListRef flags, const std::string &defaultCharsetName);

    virtual void exitDataType(MySQLParser::DataTypeContext *ctx) override;
    virtual void exitFieldLength(MySQLParser::FieldLengthContext *ctx) override;
    virtual void exitPrecision(MySQLParser::PrecisionContext *ctx) override;
    virtual void exitFieldOptions(MySQLParser::FieldOptionsContext *ctx) override;
    virtual void exitCharsetWithOptBinary(MySQLParser::CharsetWithOptBinaryContext *ctx) override;
    virtual void exitCharsetName(MySQLParser::CharsetNameContext *ctx) override;
    virtual void exitTypeDatetimePrecision(MySQLParser::TypeDatetimePrecisionContext *ctx) override;
    virtual void exitStringList(MySQLParser::StringListContext *ctx) override;
  private:
    GrtVersionRef _version;
    grt::ListRef<db_SimpleDatatype> _typeList;
    grt::StringListRef _flags;
    std::string _defaultCharsetName;
  };

  class SchemaListener : public ObjectListener {
  public:
    SchemaListener(antlr4::tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject, bool caseSensitive);

    virtual void enterCreateDatabase(MySQLParser::CreateDatabaseContext *ctx) override;
    virtual void exitCreateDatabase(MySQLParser::CreateDatabaseContext *ctx) override;
    virtual void exitCharsetName(MySQLParser::CharsetNameContext *ctx) override;
    virtual void exitCollationName(MySQLParser::CollationNameContext *ctx) override;
    virtual void exitDefaultEncryption(MySQLParser::DefaultEncryptionContext *ctx) override;
  };

  class TableListener : public ObjectListener {
  private:
    db_mysql_SchemaRef _schema;
    bool _autoGenerateFkNames;
    DbObjectsRefsCache &_refCache;

  public:
    TableListener(antlr4::tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_mysql_SchemaRef schema, db_mysql_TableRef &table,
                  bool caseSensitive, bool autoGenerateFkNames, DbObjectsRefsCache &refCache);
    
    virtual void exitTableName(MySQLParser::TableNameContext *ctx) override;
    virtual void exitCreateTable(MySQLParser::CreateTableContext *ctx) override;
    virtual void exitTableRef(MySQLParser::TableRefContext *ctx) override;
    virtual void exitPartitionClause(MySQLParser::PartitionClauseContext *ctx) override;
    virtual void exitPartitionDefKey(MySQLParser::PartitionDefKeyContext *ctx) override;
    virtual void exitPartitionDefHash(MySQLParser::PartitionDefHashContext *ctx) override;
    virtual void exitPartitionDefRangeList(MySQLParser::PartitionDefRangeListContext *ctx) override;
    virtual void exitSubPartitions(MySQLParser::SubPartitionsContext *ctx) override;
    virtual void exitPartitionDefinition(MySQLParser::PartitionDefinitionContext *ctx) override;
    virtual void exitDuplicateAsQueryExpression(MySQLParser::DuplicateAsQueryExpressionContext *ctx) override;
    virtual void exitCreateTableOptions(MySQLParser::CreateTableOptionsContext *ctx) override;
  };

  class TableAlterListener : public ObjectListener {
  private:
    bool _autoGenerateFkNames;
    DbObjectsRefsCache &_refCache;

  public:
    TableAlterListener(antlr4::tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_DatabaseObjectRef tableOrView,
                       bool caseSensitive, bool autoGenerateFkNames, DbObjectsRefsCache &refCache);

    virtual void exitAlterListItem(MySQLParser::AlterListItemContext *ctx) override;
  };
  
  class LogfileGroupListener : public ObjectListener {
  public:
    LogfileGroupListener(antlr4::tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject, bool caseSensitive);

    virtual void exitCreateLogfileGroup(MySQLParser::CreateLogfileGroupContext *ctx) override;
    virtual void exitTsOptionInitialSize(MySQLParser::TsOptionInitialSizeContext *ctx) override;
    virtual void exitTsOptionUndoRedoBufferSize(MySQLParser::TsOptionUndoRedoBufferSizeContext *ctx) override;
    virtual void exitTsOptionNodegroup(MySQLParser::TsOptionNodegroupContext *ctx) override;
    virtual void exitTsOptionEngine(MySQLParser::TsOptionEngineContext *ctx) override;
    virtual void exitTsOptionWait(MySQLParser::TsOptionWaitContext *ctx) override;
    virtual void exitTsOptionComment(MySQLParser::TsOptionCommentContext *ctx) override;
  };

  // Used for SF, SP and UDF.
  class RoutineListener : public ObjectListener {
  public:
    RoutineListener(antlr4::tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_mysql_RoutineRef routine, bool caseSensitive);

    virtual void exitDefinerClause(MySQLParser::DefinerClauseContext *ctx) override;

    virtual void exitCreateProcedure(MySQLParser::CreateProcedureContext *ctx) override;
    virtual void exitCreateFunction(MySQLParser::CreateFunctionContext *ctx) override;
    virtual void exitCreateUdf(MySQLParser::CreateUdfContext *ctx) override;

    virtual void exitProcedureParameter(MySQLParser::ProcedureParameterContext *ctx) override;
    virtual void enterFunctionParameter(MySQLParser::FunctionParameterContext *ctx) override;
    virtual void exitFunctionParameter(MySQLParser::FunctionParameterContext *ctx) override;

    virtual void exitRoutineOption(MySQLParser::RoutineOptionContext *ctx) override;
  private:
    db_mysql_RoutineParamRef _currentParameter;

    void readRoutineName(antlr4::ParserRuleContext *ctx);
  };

  class IndexListener : public ObjectListener {
  public:
    IndexListener(antlr4::tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_mysql_SchemaRef schema, db_mysql_IndexRef index,
                  bool caseSensitive, DbObjectsRefsCache &refCache);

    virtual void exitCreateIndex(MySQLParser::CreateIndexContext *ctx) override;
    virtual void exitIndexType(MySQLParser::IndexTypeContext *ctx) override;
    virtual void exitCreateIndexTarget(MySQLParser::CreateIndexTargetContext *ctx) override;
    virtual void exitCommonIndexOption(MySQLParser::CommonIndexOptionContext *ctx) override;
    virtual void exitFulltextIndexOption(MySQLParser::FulltextIndexOptionContext *ctx) override;
    virtual void exitAlterAlgorithmOption(MySQLParser::AlterAlgorithmOptionContext *ctx) override;
    virtual void exitAlterLockOption(MySQLParser::AlterLockOptionContext *ctx) override;

  private:
    db_mysql_SchemaRef _schema;
    DbObjectsRefsCache &_refCache;
  };

  class TriggerListener : public ObjectListener {
  public:
    TriggerListener(antlr4::tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_mysql_SchemaRef schema,
                    db_mysql_TriggerRef trigger, bool caseSensitive);

    virtual void exitDefinerClause(MySQLParser::DefinerClauseContext *ctx) override;
    virtual void exitCreateTrigger(MySQLParser::CreateTriggerContext *ctx) override;
    virtual void exitTriggerFollowsPrecedesClause(MySQLParser::TriggerFollowsPrecedesClauseContext *ctx) override;

  private:
    db_mysql_SchemaRef _schema;
  };
  
  class ViewListener : public ObjectListener {
  public:
    ViewListener(antlr4::tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject, bool caseSensitive);

    virtual void exitCreateView(MySQLParser::CreateViewContext *ctx) override;
    virtual void exitViewCheckOption(MySQLParser::ViewCheckOptionContext *ctx) override;
    virtual void exitViewAlgorithm(MySQLParser::ViewAlgorithmContext *ctx) override;
    virtual void exitDefinerClause(MySQLParser::DefinerClauseContext *ctx) override;
  };

  class ServerListener : public ObjectListener {
  public:
    ServerListener(antlr4::tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject, bool caseSensitive);

    virtual void exitCreateServer(MySQLParser::CreateServerContext *ctx) override;
    virtual void exitServerOption(MySQLParser::ServerOptionContext *ctx) override;
  };
  
  class TablespaceListener : public ObjectListener {
  public:
    TablespaceListener(antlr4::tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject, bool caseSensitive);

    virtual void exitCreateTablespace(MySQLParser::CreateTablespaceContext *ctx) override;
    virtual void exitLogfileGroupRef(MySQLParser::LogfileGroupRefContext *ctx) override;
    virtual void exitTsDataFile(MySQLParser::TsDataFileContext *ctx) override;
    virtual void exitTsOptionInitialSize(MySQLParser::TsOptionInitialSizeContext *ctx) override;
    virtual void exitTsOptionAutoextendSize(MySQLParser::TsOptionAutoextendSizeContext *ctx) override;
    virtual void exitTsOptionMaxSize(MySQLParser::TsOptionMaxSizeContext *ctx) override;
    virtual void exitTsOptionExtentSize(MySQLParser::TsOptionExtentSizeContext *ctx) override;
    virtual void exitTsOptionNodegroup(MySQLParser::TsOptionNodegroupContext *ctx) override;
    virtual void exitTsOptionEngine(MySQLParser::TsOptionEngineContext *ctx) override;
    virtual void exitTsOptionWait(MySQLParser::TsOptionWaitContext *ctx) override;
    virtual void exitTsOptionComment(MySQLParser::TsOptionCommentContext *ctx) override;
    virtual void exitTsOptionFileblockSize(MySQLParser::TsOptionFileblockSizeContext *ctx) override;
    virtual void exitTsOptionEncryption(MySQLParser::TsOptionEncryptionContext *ctx) override;
  };
  
  class EventListener : public ObjectListener {
  public:
    EventListener(antlr4::tree::ParseTree *tree, db_mysql_CatalogRef catalog, db_DatabaseObjectRef anObject, bool caseSensitive);

    virtual void exitDefinerClause(MySQLParser::DefinerClauseContext *ctx) override;
    virtual void exitCreateEvent(MySQLParser::CreateEventContext *ctx) override;
    virtual void exitSchedule(MySQLParser::ScheduleContext *ctx) override;
  };
  
} // namespace parsers
