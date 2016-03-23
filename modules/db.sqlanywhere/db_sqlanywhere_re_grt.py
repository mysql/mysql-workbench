# Copyright (c) 2013, 2016, Oracle and/or its affiliates. All rights reserved.
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
from workbench.utils import find_object_with_name, server_version_str2tuple, replace_string_parameters
from workbench import db_driver
from workbench.exceptions import NotConnectedError

import grt

ModuleInfo = DefineModule(name= "DbSQLAnywhereRE", author= "Oracle Corp.", version="1.0")

def release_cursors(method):
    """Deletes the available cursors in the connection once the wrapped method finishes.

    This is needed because in sqlanydb the cursors are not deleted once they go out of scope,
    because the connection keeps a set with each instantiated cursor. To make things worse,
    the number of available cursors is limited so the rev eng process may fail at some point
    when this limit is reached.

    Any method of the Rev Eng class that executes a query should be decorated with this function.
    """
    def wrapped_method(cls, connection, *args):
        # Call the original method:
        res = method(cls, connection, *args)
        c = cls.get_connection(connection)
        if connection.driver.driverLibraryName == 'sqlanydb':
            for cursor in c.conn.cursors:
                cursor.close(remove=False)
            c.conn.cursors = set()
        return res
    return wrapped_method

class SQLAnywhereReverseEngineering(GenericReverseEngineering):
    @classmethod
    def getTargetDBMSName(cls):
        return 'SQLAnywhere'

    @classmethod
    def serverVersion(cls, connection):
        return cls._connections[connection.__id__]["version"]

    @classmethod 
    @release_cursors
    def connect(cls, connection, password):
        '''Establishes a connection to the server and stores the connection object in the connections pool.

        It first looks for a connection with the given connection parameters in the connections pool to
        reuse existent connections. If such connection is found it queries the server to ensure that the
        connection is alive and reestablishes it if is dead. If no suitable connection is found in the
        connections pool, a new one is created and stored in the pool.

        Parameters:
        ===========
            connection:  an object of the class db_mgmt_Connection storing the parameters
                         for the connection.
            password:    a string with the password to use for the connection (ignored for SQLite).
        '''
        con = None
        try:
            con = cls.get_connection(connection)
            try:
                if not con.cursor().execute('SELECT 1'):
                    raise Exception('connection error')
            except Exception, exc:
                grt.send_info('Connection to %s apparently lost, reconnecting...' % connection.hostIdentifier)
                raise NotConnectedError('Connection error')
        except NotConnectedError, exc:
            grt.send_info('Connecting to %s...' % connection.hostIdentifier)
            if connection.driver.driverLibraryName == 'sqlanydb':
                import sqlanydbwrapper as sqlanydb  # Replace this to a direct sqlanydb import when it complies with PEP 249
                connstr = replace_string_parameters(connection.driver.connectionStringTemplate,
                                                    dict(connection.parameterValues))
                import ast
                try:
                    all_params_dict = ast.literal_eval(connstr)
                except Exception, exc:
                    grt.send_error('The given connection string is not a valid python dict: %s' % connstr)
                    raise
                # Remove unreplaced parameters:
                params = dict( (key, value) for key, value in all_params_dict.iteritems()
                                            if not (value.startswith('%') and value.endswith('%'))
                             )
                params['password'] = password
                conn_params = dict(params)
                conn_params['password'] = '%password%'
                connection.parameterValues['wbcopytables_connection_string'] = repr(conn_params)
                
                con = sqlanydb.connect(**params)
            else:
                con = db_driver.connect(connection, password)
            if not con:
                grt.send_error('Connection failed', str(exc))
                raise
            grt.send_info('Connected')
            cls._connections[connection.__id__] = {'connection': con}
        if con:
            ver = cls.execute_query(connection, "SELECT @@version").fetchone()[0]
            grt.log_info("SQLAnywhere RE", "Connected to %s, %s\n" % (connection.name, ver))
            ver_parts = server_version_str2tuple(ver) + (0, 0, 0, 0)
            version = grt.classes.GrtVersion()
            version.majorNumber, version.minorNumber, version.releaseNumber, version.buildNumber = ver_parts[:4]
            cls._connections[connection.__id__]["version"] = version

        return 1

    @classmethod
    @release_cursors
    def getCatalogNames(cls, connection):
        """Returns a list of the available catalogs.

        [NOTE] This will in fact return the name of the database we are connected to.
        """
        return [cls.execute_query(connection, "SELECT DB_PROPERTY('Name')").fetchone()[0]]

    @classmethod
    @release_cursors
    def getSchemaNames(cls, connection, catalog_name):
        """Returns a list of schemata for the given connection object."""

        return sorted(set(row[1] for row in cls.execute_query(connection, 'sp_tables')))

    @classmethod
    @release_cursors
    def getTableNames(cls, connection, catalog_name, schema_name):
        query = """SELECT st.table_name
FROM SYSTAB st LEFT JOIN SYSUSER su ON st.creator=su.user_id
WHERE su.user_name = '%s' AND st.table_type = 1""" % schema_name
        return [row[0] for row in cls.execute_query(connection, query)]

    @classmethod
    @release_cursors
    def getViewNames(cls, connection, catalog_name, schema_name):
        query = """SELECT st.table_name
FROM SYSTAB st LEFT JOIN SYSUSER su ON st.creator=su.user_id
WHERE su.user_name = '%s' AND st.table_type IN (2, 21)""" % schema_name
        return [row[0] for row in cls.execute_query(connection, query)]

    @classmethod
    @release_cursors
    def getProcedureNames(cls, connection, catalog_name, schema_name):
        query = """SELECT sp.proc_name
FROM SYSPROCEDURE sp LEFT JOIN SYSUSER su ON sp.creator=su.user_id
WHERE su.user_name = '%s'""" % schema_name
        return [row[0] for row in cls.execute_query(connection, query)]


    #########  Reverse Engineering functions #########


    @classmethod
    def reverseEngineer(cls, connection, catalog_name, schemata_list, context):
        catalog = super(SQLAnywhereReverseEngineering, cls).reverseEngineer(connection, '', schemata_list, context)
        catalog.name = catalog_name
        return catalog


    @classmethod
    @release_cursors
    def getCommentForTable(cls, connection, table):
        query = """SELECT sr.remarks
FROM SYSTAB st JOIN SYSUSER su ON st.creator=su.user_id
JOIN SYSREMARK sr ON st.object_id=sr.object_id
WHERE st.table_name='%s' AND su.user_name='%s'""" % (table.name, table.owner.name)
        return ''.join([row[0] for row in cls.execute_query(connection, query)])


    @classmethod
    @release_cursors
    def reverseEngineerUserDatatypes(cls, connection, catalog):
        catalog.userDatatypes.remove_all()

        query = """SELECT UPPER(st.type_name), UPPER(base_type_str), UPPER(sd.domain_name )
        FROM SYSUSERTYPE st LEFT JOIN SYSDOMAIN sd ON st.domain_id=sd.domain_id"""

        if cls.serverVersion(connection).majorNumber < 12:
            query = """SELECT
            UPPER(st.type_name),
            CASE
                WHEN st.type_name = 'money' THEN UPPER(sd.domain_name+'(19,4)')
                WHEN st.type_name = 'smallmoney' THEN UPPER(sd.domain_name+'(10,4)')
                WHEN st.type_name = 'sysname' THEN UPPER(sd.domain_name+'(30)')
                WHEN st.type_name = 'uniqueidentifierstr' THEN 'CHAR(36)'
                WHEN st.type_name = 'uniqueidentifier' THEN 'BINARY(16)'
            ELSE
                UPPER(sd.domain_name)
            END AS base_type_str,
            UPPER(sd.domain_name )
            FROM
                SYSUSERTYPE st
                    LEFT JOIN
                SYSDOMAIN sd ON st.domain_id=sd.domain_id"""

        simple_datatypes = set()
        for datatype in cls._rdbms.simpleDatatypes:
            simple_datatypes.update([datatype.name] + list(datatype.synonyms))
        for type_name, sql_definition, parent_type in cls.execute_query(connection, query):
            if type_name in simple_datatypes:  # Some standard types are defined as user types wrt a synonym type
                continue
            datatype = grt.classes.db_UserDatatype()
            datatype.name = type_name
            datatype.sqlDefinition = sql_definition
            
            for stype in cls._rdbms.simpleDatatypes:
                if stype.name == parent_type or parent_type in stype.synonyms:
                    datatype.actualType = stype
                    break
            datatype.owner = catalog
            catalog.userDatatypes.append(datatype)


    @classmethod
    @release_cursors
    def reverseEngineerTableColumns(cls, connection, table):
        query = """SELECT UPPER(sd.domain_name), sc.column_name, sc.nulls, sc.width, sc.scale, sc."default"
FROM SYSTABCOL sc JOIN SYSDOMAIN sd ON sc.domain_id=sd.domain_id
JOIN SYSTAB st ON sc.table_id=st.table_id
JOIN SYSUSER su ON st.creator=su.user_id
WHERE st.table_name='%s' AND su.user_name='%s'
ORDER BY sc.column_id""" % (table.name, table.owner.name)
        for datatype, col_name, nullable, width, scale, default_value in cls.execute_query(connection, query):
            column = grt.classes.db_Column()
            column.name = col_name or ''
            column.isNotNull = nullable in ['N', 'n']
            column.collationName = ''  # TODO: find a way to get the column's collation

            if datatype.startswith('UNSIGNED '):
                datatype = datatype[9:]
                column.flags.append('UNSIGNED')

            is_simple_datatype, datatype_object = cls.find_datatype_object(table.owner.owner, datatype)

            if not datatype_object:
                is_simple_datatype, datatype_object = cls.find_datatype_object(table.owner.owner, 'VARCHAR')
                width = 255
                msg = 'Column datatype "%s" for column "%s" in table "%s.%s" reverse engineered as VARCHAR(255)' % (datatype, column.name, table.owner.name, table.name)
                grt.send_warning('SQL Anywhere reverseEngineerTableColumns', msg)

            if is_simple_datatype:
                column.simpleType = datatype_object
            else:
                column.userType = datatype_object

            column.defaultValue = str(default_value) if default_value is not None else '' 

            group = datatype_object.group.name if is_simple_datatype else datatype_object.actualType.group.name

            width = int(width) if width is not None else -1
            if group.upper() == 'NUMERIC':
                column.length = -1
                column.precision = width
                column.scale = scale
            else:
                column.length = width
                column.precision = column.scale = -1

            table.addColumn(column)


    @classmethod
    @release_cursors
    def reverseEngineerTablePK(cls, connection, table):
        """Reverse engineers the primary key(s) for the given table."""

        schema = table.owner


        if len(table.columns) == 0:  # Table must have columns reverse engineered before we can rev eng its primary key(s)
            grt.send_error('Migration: reverseEngineerTablePK: Reverse engineering of table %s was attempted but the table has no columns attribute' % table.name)
            return 1
        
        # Primary keys and indices come together in the SYSIDX system view, so we'll rev eng them at once:
        query = """SELECT st.table_id, si.index_id, si.index_name, si.index_category, si."unique"
FROM SYSIDX si
JOIN SYSTAB st ON si.table_id=st.table_id
JOIN SYSUSER su ON st.creator=su.user_id
WHERE st.table_name='%s' AND su.user_name='%s'
ORDER BY si.index_id""" % (table.name, schema.name)
        idx_cursor = cls.get_connection(connection).cursor()
        for table_id, index_id, index_name, index_category, index_unique in idx_cursor.execute(query):
            index = grt.classes.db_Index()
            index.name = index_name
            index.isPrimary = 1 if index_category == 1 else 0
            index.unique = 1 if index_unique in (1, 2) else 0
            if index_category == 1:
                index.indexType = 'PRIMARY'
            elif index_category == 2:
                continue  # This is a foreign key, will be handled when reverse engineering them
            elif index_category == 3:  # Can be a regular index or a unique constraint
                if index_unique == 2:
                    index.indexType = 'UNIQUE'
                else:
                    index.indexType = 'INDEX'
            else:
                index.indexType = 'FULLTEXT'
            
    #        index.hasFilter = False  # TODO: Find out if there's a way to determine this

            # Get the columns for the index:
            idx_cols_query = """SELECT sc.column_name, sic."order"
FROM SYSIDXCOL sic
JOIN SYSTAB st ON sic.table_id=st.table_id
JOIN SYSTABCOL sc ON (sc.column_id = sic.column_id AND sc.table_id = sic.table_id)
WHERE st.table_id=%s AND sic.index_id=%s
ORDER BY sic.sequence""" % (table_id, index_id)
            idx_cols_cursor = cls.get_connection(connection).cursor()
            for column_name, order in idx_cols_cursor.execute(idx_cols_query):
                column = find_object_with_name(table.columns, column_name)
                if column:
                    index_column = grt.classes.db_IndexColumn()
                    index_column.name = index_name + '.' + column_name
                    index_column.referencedColumn = column
                    index_column.descend = 1 if order and order.upper() == 'D' else 0
                    index.columns.append(index_column)
            table.addIndex(index)

            if index.isPrimary:
                table.primaryKey = index
        return 0


    @classmethod
    @release_cursors
    def reverseEngineerTableFKs(cls, connection, table):
        """Reverse engineers the foreign keys for the given table."""

        schema = table.owner
        catalog = schema.owner

        if len(table.columns) == 0:  # Table must have columns reverse engineered before we can rev eng its foreign keys
            grt.send_error('Migration: reverseEngineerTableFKs: Reverse engineering of table %s was attempted but the table has no columns attribute' % table.name)
            return 1
        
        query = """SELECT si.index_name, sfk.foreign_table_id, sfk.foreign_index_id, sfk.primary_table_id, sfk.primary_index_id
FROM SYSFKEY sfk
JOIN SYSIDX si ON (sfk.foreign_index_id=si.index_id AND sfk.foreign_table_id=si.table_id)
JOIN SYSTAB st ON sfk.foreign_table_id=st.table_id
JOIN SYSUSER su ON st.creator=su.user_id
WHERE st.table_name='%s' AND su.user_name='%s'
ORDER BY sfk.primary_index_id""" % (table.name, schema.name)

        fk_cursor = cls.get_connection(connection).cursor()
        for fk_name, this_table_id, this_index_id, other_table_id, other_index_id in fk_cursor.execute(query):
            this_column_query = """SELECT stc.column_name
FROM SYSIDXCOL sic
JOIN SYSTABCOL stc ON (sic.table_id=stc.table_id AND sic.column_id=stc.column_id)
WHERE sic.table_id=%d AND sic.index_id=%d
ORDER BY sic.sequence""" % (this_table_id, this_index_id)
            other_column_query = """SELECT su.user_name, st.table_name, stc.column_name
FROM SYSIDXCOL sic
JOIN SYSTABCOL stc ON (sic.table_id=stc.table_id AND sic.column_id=stc.column_id)
JOIN SYSTAB st ON stc.table_id=st.table_id
JOIN SYSUSER su ON st.creator=su.user_id
WHERE sic.table_id=%d AND sic.index_id=%d
ORDER BY sic.sequence""" % (other_table_id, other_index_id)

            these_columns =  cls.execute_query(connection, this_column_query).fetchall()
            other_columns =  cls.execute_query(connection, other_column_query).fetchall()
            
            foreign_key = grt.classes.db_ForeignKey()
            foreign_key.owner = table
            foreign_key.name = fk_name
            
            # Find the referenced table:
            referenced_schema = find_object_with_name(catalog.schemata, other_columns[0][0]) if other_columns[0][0] else schema
            foreign_key.referencedTable = find_object_with_name(referenced_schema.tables, other_columns[0][1]) if other_columns[0][1] else table
            
            for (this_column_name,), (_, _, other_column_name) in zip(these_columns, other_columns):
                column = find_object_with_name(table.columns, this_column_name)
                if not column:
                    grt.send_error('Migration: reverseEngineerTableFKs: Column "%s" not found in table "%s"' % (this_column_name, table.name) )
                    continue

                ref_column = find_object_with_name(foreign_key.referencedTable.columns, other_column_name)
                if not ref_column:
                    grt.send_error('Migration: reverseEngineerTableFKs: Column "%s" not found in table "%s"' % (other_column_name, foreign_key.referencedTable.name) )
                    continue
                
                foreign_key.columns.append(column)
                foreign_key.referencedColumns.append(ref_column)

            table.foreignKeys.append(foreign_key)
        return 0
        
    @classmethod
    @release_cursors
    def getOS(cls, connection):
        _os = cls.execute_query(connection, "SELECT PROPERTY ('Platform')").fetchone()[0].lower()
        if 'unix' in _os:
            return 'linux'
        elif 'windows' in _os:
            return 'windows'
        else:
            return 'darwin'

        return None
###########################################################################################

@ModuleInfo.export(grt.classes.db_mgmt_Rdbms)
def initializeDBMSInfo():
    return SQLAnywhereReverseEngineering.initializeDBMSInfo('sqlanywhere_rdbms_info.xml')

@ModuleInfo.export((grt.LIST, grt.STRING))
def getDataSourceNames():
    return SQLAnywhereReverseEngineering.getDataSourceNames()


@ModuleInfo.export(grt.STRING, grt.STRING)
def quoteIdentifier(name):
    return SQLAnywhereReverseEngineering.quoteIdentifier(name)


@ModuleInfo.export(grt.STRING, grt.classes.GrtNamedObject)
def fullyQualifiedObjectName(obj):
    return SQLAnywhereReverseEngineering.fullyQualifiedObjectName(obj)


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.STRING)
def connect(connection, password):
    return SQLAnywhereReverseEngineering.connect(connection, password)


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def disconnect(connection):
    return SQLAnywhereReverseEngineering.disconnect(connection)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def isConnected(connection):
    return SQLAnywhereReverseEngineering.isConnected(connection)

@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return SQLAnywhereReverseEngineering.getTargetDBMSName()

@ModuleInfo.export(grt.LIST)
def getSupportedObjectTypes():
    return SQLAnywhereReverseEngineering.getSupportedObjectTypes()

@ModuleInfo.export(grt.classes.GrtVersion, grt.classes.db_mgmt_Connection)
def getServerVersion(connection):
    return SQLAnywhereReverseEngineering.getServerVersion(connection)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection)
def getCatalogNames(connection):
    return SQLAnywhereReverseEngineering.getCatalogNames(connection)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING)
def getSchemaNames(connection, catalog_name):
    return SQLAnywhereReverseEngineering.getSchemaNames(connection, catalog_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTableNames(connection, catalog_name, schema_name):
    return SQLAnywhereReverseEngineering.getTableNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getViewNames(connection, catalog_name, schema_name):
    return SQLAnywhereReverseEngineering.getViewNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTriggerNames(connection, catalog_name, schema_name):
    return SQLAnywhereReverseEngineering.getTriggerNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getProcedureNames(connection, catalog_name, schema_name):
    return SQLAnywhereReverseEngineering.getProcedureNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getFunctionNames(connection, catalog_name, schema_name):
    return SQLAnywhereReverseEngineering.getFunctionNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING, (grt.LIST, grt.STRING), grt.DICT)
def reverseEngineer(connection, catalog_name, schemata_list, context):
    return SQLAnywhereReverseEngineering.reverseEngineer(connection, catalog_name, schemata_list, context)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Catalog)
def reverseEngineerUserDatatypes(connection, catalog):
    return SQLAnywhereReverseEngineering.reverseEngineerUserDatatypes(connection, catalog)

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING)
def reverseEngineerCatalog(connection, catalog_name):
    return SQLAnywhereReverseEngineering.reverseEngineerCatalog(connection, catalog_name)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerTables(connection, schema):
    return SQLAnywhereReverseEngineering.reverseEngineerTables(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerViews(connection, schema):
    return SQLAnywhereReverseEngineering.reverseEngineerViews(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerProcedures(connection, schema):
    return SQLAnywhereReverseEngineering.reverseEngineerProcedures(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerFunctions(connection, schema):
    return SQLAnywhereReverseEngineering.reverseEngineerFunctions(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerTriggers(connection, schema):
    return SQLAnywhereReverseEngineering.reverseEngineerTriggers(connection, schema)

@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def getOS(connection):
    return SQLAnywhereReverseEngineering.getOS(connection)