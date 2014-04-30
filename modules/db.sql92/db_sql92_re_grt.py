# Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
# License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301  USA

from db_generic_re_grt import GenericReverseEngineering

from wb import DefineModule
from workbench.utils import find_object_with_name

import grt

ModuleInfo = DefineModule(name= "DbSql92RE", author= "Oracle Corp.", version="1.0")

class Sql92ReverseEngineering(GenericReverseEngineering):
    @classmethod
    def getTargetDBMSName(cls):
        return 'Sql92'


    @classmethod
    def getCatalogNames(cls, connection):
        """Returns a list of the available catalogs.

        [NOTE] This will in fact return the name of the database we are connected to.
        """
        return sorted(list(set(row[0] for row in cls.execute_query(connection, 'SELECT TABLE_CATALOG FROM INFORMATION_SCHEMA.TABLES'))) )


    @classmethod
    def getSchemaNames(cls, connection, catalog_name):
        """Returns a list of schemata for the given connection object."""

        query = """SELECT TABLE_SCHEMA
        FROM INFORMATION_SCHEMA.TABLES
        WHERE TABLE_CATALOG = ?"""
        return sorted(list(set(row[0] for row in cls.execute_query(connection, query, catalog_name))) )


    @classmethod
    def getTableNames(cls, connection, catalog_name, schema_name):
        query = """SELECT TABLE_NAME
        FROM INFORMATION_SCHEMA.TABLES
        WHERE TABLE_CATALOG = ? AND TABLE_SCHEMA = ? AND TABLE_TYPE = 'BASE TABLE'"""
        return sorted(list(set(row[0] for row in cls.execute_query(connection, query, catalog_name, schema_name))) )


    @classmethod
    def getViewNames(cls, connection, catalog_name, schema_name):
        query = """SELECT TABLE_NAME
        FROM INFORMATION_SCHEMA.VIEWS
        WHERE TABLE_CATALOG = ? AND TABLE_SCHEMA = ?"""
        return sorted(list(set(row[0] for row in cls.execute_query(connection, query, catalog_name, schema_name))) )


    @classmethod
    def getTriggerNames(cls, connection, catalog_name, schema_name):
        query = """SELECT TRIGGER_NAME
        FROM INFORMATION_SCHEMA.TRIGGERS
        WHERE TRIGGER_CATALOG = ? AND TRIGGER_SCHEMA = ?"""
        return sorted(list(set(row[0] for row in cls.execute_query(connection, query, catalog_name, schema_name))) )


    @classmethod
    def getProcedureNames(cls, connection, catalog_name, schema_name):
        # SQL-92 standard does not include INFORMATION_SCHEMA.ROUTINES
        query = """SELECT ROUTINE_NAME FROM INFORMATION_SCHEMA.ROUTINES
        WHERE ROUTINE_CATALOG = ? AND ROUTINE_SCHEMA = ? AND ROUTINE_TYPE = 'PROCEDURE'"""
        try:
            return sorted(list(set(row[0] for row in cls.execute_query(connection, query, catalog_name, schema_name))) )
        except Exception:
            try:
                return super(Sql92ReverseEngineering, cls).getProcedureNames(connection, catalog_name, schema_name)
            except:
                return []


    @classmethod
    def getFunctionNames(cls, connection, catalog_name, schema_name):
        # SQL-92 standard does not include INFORMATION_SCHEMA.ROUTINES
        query = """SELECT ROUTINE_NAME FROM INFORMATION_SCHEMA.ROUTINES
        WHERE ROUTINE_CATALOG = ? AND ROUTINE_SCHEMA = ? AND ROUTINE_TYPE = 'FUNCTION'"""
        try:
            return sorted(list(set(row[0] for row in cls.execute_query(connection, query, catalog_name, schema_name))) )
        except Exception:
            try:
                return super(Sql92ReverseEngineering, cls).getFunctionNames(connection, catalog_name, schema_name)
            except:
                return []


    #########  Reverse Engineering functions #########

    @classmethod
    def reverseEngineerUserDatatypes(cls, connection, catalog):
        pass  # TODO: Implement the rev eng of user data types

    @classmethod
    def getColumnDatatype(cls, connection, table, column, type_name):
        catalog = table.owner.owner
        up_type_name = type_name.upper()
        for stype in cls._rdbms.simpleDatatypes:
            if stype.name.upper() == up_type_name or up_type_name in [s.upper() for s in stype.synonyms]:
                return stype

        for utype in catalog.userDatatypes:
            if utype.name.upper() == up_type_name:
                return utype

        return None

    @classmethod
    def reverseEngineerTableColumns(cls, connection, table):
        schema = table.owner
        catalog = schema.owner

        query = """SELECT COLUMN_NAME, COLUMN_DEFAULT,
        IS_NULLABLE, DATA_TYPE, CHARACTER_MAXIMUM_LENGTH,
        NUMERIC_PRECISION, NUMERIC_SCALE, DATETIME_PRECISION,
        CHARACTER_SET_NAME, COLLATION_NAME
    FROM INFORMATION_SCHEMA.COLUMNS
    WHERE TABLE_CATALOG='%s' AND TABLE_SCHEMA='%s' AND TABLE_NAME='%s'
    ORDER BY ORDINAL_POSITION"""  % (catalog.name, schema.name, table.name)

        table_columns = cls.execute_query(connection, query)
        for (column_name, column_default, is_nullable, type_name, char_max_length, precision, scale,
                datetime_precision, charset, collation) in table_columns:
            column = grt.classes.db_Column()
            column.name = column_name
            column.isNotNull = is_nullable == 'NO'
            column.length = char_max_length
            column.precision = precision if precision is not None else -1
            column.scale = scale if scale is not None else -1
            column.defaultValue = column_default if column_default is not None else ''

            datatype = cls.getColumnDatatype(connection, table, column, type_name)
            if isinstance(datatype, grt.classes.db_SimpleDatatype):
                column.simpleType = datatype
            elif isinstance(datatype, grt.classes.db_UserDatatype):
                column.userType = datatype
            else:
                column.simpleType = cls.getColumnDatatype(connection, table, column, 'VARCHAR')
                column.length = 255
                msg = 'Column datatype "%s" for column "%s" in table "%s.%s" is unknown, reverse engineering as VARCHAR(255)' % (type_name, column.name, schema.name, table.name)
                grt.send_warning('%s reverseEngineerTableColumns: ' %  cls.getTargetDBMSName() + msg)

            table.addColumn(column)

        return 0


    @classmethod
    def reverseEngineerTablePK(cls, connection, table):
        """Reverse engineers the primary key(s) for the given table."""

        schema = table.owner
        catalog = schema.owner

        query = """SELECT tc.CONSTRAINT_NAME, kcu.COLUMN_NAME
    FROM INFORMATION_SCHEMA.TABLE_CONSTRAINTS AS tc
      JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE AS kcu
        ON kcu.CONSTRAINT_SCHEMA = tc.CONSTRAINT_SCHEMA
       AND kcu.CONSTRAINT_NAME = tc.CONSTRAINT_NAME
       AND kcu.TABLE_SCHEMA = tc.TABLE_SCHEMA
       AND kcu.TABLE_NAME = tc.TABLE_NAME
    WHERE tc.CONSTRAINT_TYPE='PRIMARY KEY' AND tc.TABLE_CATALOG = '%s' AND tc.TABLE_SCHEMA = '%s' AND tc.TABLE_NAME = '%s'
    ORDER BY tc.CONSTRAINT_NAME, kcu.ORDINAL_POSITION""" % (catalog.name, schema.name, table.name)

        if len(table.columns) == 0:  # Table must have columns reverse engineered before we can rev eng its primary key(s)
            grt.send_error('%s reverseEngineerTablePK: Reverse engineer of table %s was attempted but the table has '
                           'no columns attribute' % (cls.getTargetDBMSName(), table.name))
            return 1

        fk_rows = cls.execute_query(connection, query).fetchall()
        if fk_rows:
            index = grt.classes.db_Index()
            index.name = fk_rows[0][0]
            index.isPrimary = 1
            index.unique = 1
            index.indexType = 'PRIMARY'

            for _, pk_col in fk_rows:
                table_column = find_object_with_name(table.columns, pk_col)
                if not table_column:
                    grt.send_warning('%s reverseEngineerTablePK: Could not find column "%s" in table "%s" referenced '
                                     'by primary key constraint "%s". The primary key will not be added.' % (cls.getTargetDBMSName(), pk_col, table.name, index.name) )
                    return 0

                index_column = grt.classes.db_IndexColumn()
                index_column.name = index.name + '.' + pk_col
                index_column.referencedColumn = table_column

                index.columns.append(index_column)

            table.primaryKey = index
            table.addIndex(index)
        return 0


    @classmethod
    def reverseEngineerTableFKs(cls, connection, table):
        """Reverse engineers the foreign keys for the given table."""

        catalog = table.owner.owner
        schema = table.owner

        query = """SELECT kcu1.COLUMN_NAME,
           rc.CONSTRAINT_NAME, kcu2.TABLE_SCHEMA, kcu2.TABLE_NAME,
           kcu2.COLUMN_NAME, rc.UPDATE_RULE, rc.DELETE_RULE
    FROM INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS rc
         JOIN INFORMATION_SCHEMA.TABLE_CONSTRAINTS AS tc
            ON rc.CONSTRAINT_NAME = tc.CONSTRAINT_NAME
         JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE kcu1
            ON  kcu1.CONSTRAINT_CATALOG = rc.CONSTRAINT_CATALOG
            AND kcu1.CONSTRAINT_SCHEMA  = rc.CONSTRAINT_SCHEMA
            AND kcu1.CONSTRAINT_NAME    = rc.CONSTRAINT_NAME
         JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE kcu2
            ON  kcu2.CONSTRAINT_CATALOG = rc.UNIQUE_CONSTRAINT_CATALOG
            AND kcu2.CONSTRAINT_SCHEMA  = rc.UNIQUE_CONSTRAINT_SCHEMA
            AND kcu2.CONSTRAINT_NAME    = rc.UNIQUE_CONSTRAINT_NAME
    WHERE tc.CONSTRAINT_TYPE = 'FOREIGN KEY' AND kcu1.ORDINAL_POSITION = kcu2.ORDINAL_POSITION
          AND kcu1.TABLE_CATALOG = ?
          AND kcu1.TABLE_SCHEMA = ?
          AND kcu1.TABLE_NAME = ?
    ORDER BY kcu1.CONSTRAINT_NAME, kcu1.ORDINAL_POSITION"""

        if len(table.columns) == 0:
            grt.send_error('%s reverseEngineerTableFKs: Reverse engineering of table '
                           '%s was attempted but the table has no columns attribute' % (cls.getTargetDBMSName(), table.name))
            return 1    # Table must have columns reverse engineered before we can rev eng its foreign keys

        cursor = cls.execute_query(connection, query, catalog.name, schema.name, table.name)
        current_fk = None
        table.foreignKeys.remove_all()
        for col_name, fk_name, ref_schema, ref_table, ref_col, upd_rule, del_rule in cursor:
            if not current_fk or fk_name != current_fk.name:
                if current_fk:
                    table.foreignKeys.append(current_fk)
                foreign_key = grt.classes.db_ForeignKey()
                foreign_key.name = fk_name
                foreign_key.owner = table
                foreign_key.deleteRule = del_rule.upper()
                foreign_key.updateRule = upd_rule.upper()
                foreign_key.modelOnly = 0
                referenced_schema = find_object_with_name(catalog.schemata, ref_schema)
                if not referenced_schema:
                    grt.send_warning('%s reverseEngineerTableFKs: Could not find referenced schema "%s" '
                                     'for foreign key constraint "%s"' % (cls.getTargetDBMSName(), ref_schema, fk_name))
                    continue
                referenced_table = find_object_with_name(referenced_schema.tables, ref_table)
                if not referenced_table:
                    grt.send_warning('%s reverseEngineerTableFKs: Could not find referenced table "%s.%s" '
                                     'for foreign key constraint "%s"' % (cls.getTargetDBMSName(), ref_schema, ref_table, fk_name))
                    continue
                if len(referenced_table.columns) == 0:
                    grt.send_error('%s reverseEngineerTableFKs: Reverse engineering of table '
                                   '%s was attempted but the table has no columns attribute' % (cls.getTargetDBMSName(), referenced_table.name))
                    return 1    # Table must have columns reverse engineered before we can rev eng its foreign keys

                foreign_key.referencedTable = referenced_table
                current_fk = foreign_key

            column = find_object_with_name(table.columns, col_name)
            if not column:
                grt.send_warning('%s reverseEngineerTableFKs: Could not find column "%s.%s.%s" '
                                 'for foreign key constraint "%s"' % (cls.getTargetDBMSName(), schema.name, table.name, col_name, fk_name))
                continue
            current_fk.columns.append(column)

            referenced_column = find_object_with_name(current_fk.referencedTable.columns, ref_col)
            if not referenced_column:
                grt.send_warning('%s reverseEngineerTableFKs: Could not find referenced column "%s.%s.%s" '
                                 'for foreign key constraint "%s"' % (cls.getTargetDBMSName(), ref_schema, ref_table, ref_col, fk_name))
                continue
            current_fk.referencedColumns.append(referenced_column)

        # Store the last fk:
        if current_fk:
            table.foreignKeys.append(current_fk)

        return 0


    @classmethod
    def reverseEngineerViews(cls, connection, schema):
        query = """SELECT TABLE_NAME, VIEW_DEFINITION
        FROM INFORMATION_SCHEMA.VIEWS
        WHERE TABLE_CATALOG ='%s' AND TABLE_SCHEMA = '%s'""" % (schema.owner.name, schema.name)
        schema.views.remove_all()
        view_count = len(cls.getViewNames(connection, schema.owner.name, schema.name))
        step = 1.0 / (view_count + 1e-10)
        idx = 0
        for view_name, view_definition in cls.execute_query(connection, query):
            grt.send_progress(idx * step, 'Reverse engineering view %s.%s' % (schema.name, view_name))
            view = grt.classes.db_View()
            view.name = view_name or ''
            view.owner = schema
            view.sqlDefinition = view_definition
            schema.views.append(view)
            idx += 1
        return 0


###########################################################################################

@ModuleInfo.export(grt.classes.db_mgmt_Rdbms)
def initializeDBMSInfo():
    return Sql92ReverseEngineering.initializeDBMSInfo('sql92_rdbms_info.xml')

@ModuleInfo.export((grt.LIST, grt.STRING))
def getDataSourceNames():
    return Sql92ReverseEngineering.getDataSourceNames()


@ModuleInfo.export(grt.STRING, grt.STRING)
def quoteIdentifier(name):
    return Sql92ReverseEngineering.quoteIdentifier(name)


@ModuleInfo.export(grt.STRING, grt.classes.GrtNamedObject)
def fullyQualifiedObjectName(obj):
    return Sql92ReverseEngineering.fullyQualifiedObjectName(obj)


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.STRING)
def connect(connection, password):
    return Sql92ReverseEngineering.connect(connection, password)


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def disconnect(connection):
    return Sql92ReverseEngineering.disconnect(connection)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def isConnected(connection):
    return Sql92ReverseEngineering.isConnected(connection)

@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return Sql92ReverseEngineering.getTargetDBMSName()

@ModuleInfo.export(grt.LIST)
def getSupportedObjectTypes():
    return Sql92ReverseEngineering.getSupportedObjectTypes()

@ModuleInfo.export(grt.classes.GrtVersion, grt.classes.db_mgmt_Connection)
def getServerVersion(connection):
    return Sql92ReverseEngineering.getServerVersion(connection)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection)
def getCatalogNames(connection):
    return Sql92ReverseEngineering.getCatalogNames(connection)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING)
def getSchemaNames(connection, catalog_name):
    return Sql92ReverseEngineering.getSchemaNames(connection, catalog_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTableNames(connection, catalog_name, schema_name):
    return Sql92ReverseEngineering.getTableNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getViewNames(connection, catalog_name, schema_name):
    return Sql92ReverseEngineering.getViewNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTriggerNames(connection, catalog_name, schema_name):
    return Sql92ReverseEngineering.getTriggerNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getProcedureNames(connection, catalog_name, schema_name):
    return Sql92ReverseEngineering.getProcedureNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getFunctionNames(connection, catalog_name, schema_name):
    return Sql92ReverseEngineering.getFunctionNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING, (grt.LIST, grt.STRING), grt.DICT)
def reverseEngineer(connection, catalog_name, schemata_list, context):
    return Sql92ReverseEngineering.reverseEngineer(connection, catalog_name, schemata_list, context)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Catalog)
def reverseEngineerUserDatatypes(connection, catalog):
    return Sql92ReverseEngineering.reverseEngineerUserDatatypes(connection, catalog)

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING)
def reverseEngineerCatalog(connection, catalog_name):
    return Sql92ReverseEngineering.reverseEngineerCatalog(connection, catalog_name)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerTables(connection, schema):
    return Sql92ReverseEngineering.reverseEngineerTables(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerViews(connection, schema):
    return Sql92ReverseEngineering.reverseEngineerViews(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerProcedures(connection, schema):
    return Sql92ReverseEngineering.reverseEngineerProcedures(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerFunctions(connection, schema):
    return Sql92ReverseEngineering.reverseEngineerFunctions(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerTriggers(connection, schema):
    return Sql92ReverseEngineering.reverseEngineerTriggers(connection, schema)
