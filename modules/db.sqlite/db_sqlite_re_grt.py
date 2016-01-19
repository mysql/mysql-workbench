# Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

import sqlite3
import os.path

from db_generic_re_grt import GenericReverseEngineering

from wb import DefineModule
from workbench.utils import server_version_str2tuple
from workbench.exceptions import NotConnectedError

import grt

ModuleInfo = DefineModule(name= "DbSQLiteRE", author= "Oracle Corp.", version="1.0")

class SQLiteReverseEngineering(GenericReverseEngineering):
    @classmethod
    def getTargetDBMSName(cls):
        return 'SQLite'

    @classmethod
    def serverVersion(cls, connection):
        return cls._connections[connection.__id__]["version"]

    #########  Connection related functions #########
    @classmethod
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
            con = sqlite3.connect(connection.parameterValues['dbfile'])
            if not con:
                grt.send_error('Connection failed', str(exc))
                raise
            connection.parameterValues['wbcopytables_connection_string'] = "'" + connection.parameterValues['dbfile'] + "'"
            grt.send_info('Connected')
            cls._connections[connection.__id__] = {'connection': con}
        if con:
            ver = cls.execute_query(connection, "SELECT sqlite_version()").fetchone()[0]
            grt.log_info('SQLite RE', 'Connected to %s, %s' % (connection.name, ver))
            ver_parts = server_version_str2tuple(ver) + (0, 0, 0, 0)
            version = grt.classes.GrtVersion()
            version.majorNumber, version.minorNumber, version.releaseNumber, version.buildNumber = ver_parts[:4]
            cls._connections[connection.__id__]['version'] = version
        return 1


    @classmethod
    def getCatalogNames(cls, connection):
        """Returns a list of the available catalogs.
        """
        return ['def']

    @classmethod
    def getSchemaNames(cls, connection, catalog_name):
        """Returns a list of schemata for the given connection object."""

        return [os.path.splitext(os.path.basename(connection.parameterValues['dbfile']))[0]]

    @classmethod
    def getTableNames(cls, connection, catalog_name, schema_name):
        query = """SELECT name
FROM sqlite_master
WHERE type='table' AND NOT name LIKE 'sqlite_%'"""
        return [row[0] for row in cls.execute_query(connection, query)]

    @classmethod
    def getViewNames(cls, connection, catalog_name, schema_name):
        return []

    @classmethod
    def getProcedureNames(cls, connection, catalog_name, schema_name):
        return []


    #########  Reverse Engineering functions #########


    @classmethod
    def reverseEngineer(cls, connection, catalog_name, schemata_list, context):
        from grt.modules import MysqlSqlFacade
        grt.send_progress(0, "Reverse engineering catalog information")
        cls.check_interruption()
        catalog = cls.reverseEngineerCatalog(connection, catalog_name)

        # calculate total workload 1st
        grt.send_progress(0.1, 'Preparing...')

        get_tables = context.get("reverseEngineerTables", True)

        # 10% of the progress is for preparation
        total = 1e-10  # total should not be zero to avoid DivisionByZero exceptions
        total += len(cls.getTableNames(connection, catalog_name, '')) if get_tables else 0

        grt.send_progress(0.1, "Gathered stats")

        # Now the second pass for reverse engineering tables:
        if get_tables:
            idx = 0
            for object_type, name, tbl_name, _, sql in cls.execute_query(connection, "SELECT * FROM sqlite_master"):
                if type in ('view', 'trigger') or not sql or tbl_name.startswith('sqlite_'):
                    continue

                sql = sql.replace('[', '').replace(']', '')

                grt.log_debug('SQLiteReverseEngineering', 'Processing this sql:\n%s;' % sql)

                MysqlSqlFacade.parseSqlScriptString(catalog, sql)

                cls.check_interruption()
                grt.send_progress(0.1 + idx / total, 'Object %s reverse engineered!' % name)
                idx += 1

        grt.send_progress(1.0, 'Reverse engineering completed!')
        return catalog


    @classmethod
    def reverseEngineerCatalog(cls, connection, catalog_name):
        catalog = grt.classes.db_mysql_Catalog()
        catalog.name = catalog_name
        catalog.simpleDatatypes.extend(cls._rdbms.simpleDatatypes)
        
        schemata_names = cls.getSchemaNames(connection, catalog_name) or ['']
        catalog.schemata.remove_all()
        for schema_name in schemata_names:
            schema = grt.classes.db_mysql_Schema()
            schema.name = schema_name
            schema.owner = catalog
            if hasattr(cls, 'reverseEngineerSequences'):
                cls.reverseEngineerSequences(connection, schema)
            catalog.schemata.append(schema)

        return catalog


    @classmethod
    def getCommentForTable(cls, connection, table):
        pass



    @classmethod
    def reverseEngineerUserDatatypes(cls, connection, catalog):
        pass



###########################################################################################

@ModuleInfo.export(grt.classes.db_mgmt_Rdbms)
def initializeDBMSInfo():
    return SQLiteReverseEngineering.initializeDBMSInfo('sqlite_rdbms_info.xml')

@ModuleInfo.export((grt.LIST, grt.STRING))
def getDataSourceNames():
    return SQLiteReverseEngineering.getDataSourceNames()


@ModuleInfo.export(grt.STRING, grt.STRING)
def quoteIdentifier(name):
    return SQLiteReverseEngineering.quoteIdentifier(name)


@ModuleInfo.export(grt.STRING, grt.classes.GrtNamedObject)
def fullyQualifiedObjectName(obj):
    return SQLiteReverseEngineering.fullyQualifiedObjectName(obj)


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.STRING)
def connect(connection, password):
    return SQLiteReverseEngineering.connect(connection, password)


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def disconnect(connection):
    return SQLiteReverseEngineering.disconnect(connection)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def isConnected(connection):
    return SQLiteReverseEngineering.isConnected(connection)

@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return SQLiteReverseEngineering.getTargetDBMSName()

@ModuleInfo.export(grt.LIST)
def getSupportedObjectTypes():
    return SQLiteReverseEngineering.getSupportedObjectTypes()

@ModuleInfo.export(grt.classes.GrtVersion, grt.classes.db_mgmt_Connection)
def getServerVersion(connection):
    return SQLiteReverseEngineering.getServerVersion(connection)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection)
def getCatalogNames(connection):
    return SQLiteReverseEngineering.getCatalogNames(connection)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING)
def getSchemaNames(connection, catalog_name):
    return SQLiteReverseEngineering.getSchemaNames(connection, catalog_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTableNames(connection, catalog_name, schema_name):
    return SQLiteReverseEngineering.getTableNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getViewNames(connection, catalog_name, schema_name):
    return SQLiteReverseEngineering.getViewNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTriggerNames(connection, catalog_name, schema_name):
    return SQLiteReverseEngineering.getTriggerNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getProcedureNames(connection, catalog_name, schema_name):
    return SQLiteReverseEngineering.getProcedureNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getFunctionNames(connection, catalog_name, schema_name):
    return SQLiteReverseEngineering.getFunctionNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING, (grt.LIST, grt.STRING), grt.DICT)
def reverseEngineer(connection, catalog_name, schemata_list, context):
    return SQLiteReverseEngineering.reverseEngineer(connection, catalog_name, schemata_list, context)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Catalog)
def reverseEngineerUserDatatypes(connection, catalog):
    return SQLiteReverseEngineering.reverseEngineerUserDatatypes(connection, catalog)

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING)
def reverseEngineerCatalog(connection, catalog_name):
    return SQLiteReverseEngineering.reverseEngineerCatalog(connection, catalog_name)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerTables(connection, schema):
    return SQLiteReverseEngineering.reverseEngineerTables(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerViews(connection, schema):
    return SQLiteReverseEngineering.reverseEngineerViews(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerProcedures(connection, schema):
    return SQLiteReverseEngineering.reverseEngineerProcedures(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerFunctions(connection, schema):
    return SQLiteReverseEngineering.reverseEngineerFunctions(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerTriggers(connection, schema):
    return SQLiteReverseEngineering.reverseEngineerTriggers(connection, schema)
