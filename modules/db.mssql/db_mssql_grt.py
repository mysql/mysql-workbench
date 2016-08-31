# Copyright (c) 2012, 2016, Oracle and/or its affiliates. All rights reserved.
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

from wb import DefineModule

import grt

from workbench import db_driver
from workbench.exceptions import NotConnectedError


ModuleInfo = DefineModule(name= "DbMssqlRE", author= "Oracle Corp.", version="1.0")

######################### Non exposed functions and variables #################

def check_interruption():
    if grt.query_status():
        raise grt.UserInterrupt()

def get_mssql_rdbms_instance():
    mssql_rdbms_instance = None
    for rdbms in grt.root.wb.rdbmsMgmt.rdbms:
        if rdbms.name == 'Mssql':
            mssql_rdbms_instance = rdbms
            break
    return mssql_rdbms_instance

_connections = {}

def get_connection(connection_object):
    if connection_object.__id__ in _connections:
        return _connections[connection_object.__id__]["connection"]
    else:
        raise NotConnectedError("No open connection to %s" % connection_object.hostIdentifier)

def connected_server_version(connection_object):
    if connection_object.__id__ in _connections:
        return _connections[connection_object.__id__]["version"]
    else:
        raise NotConnectedError("No open connection to %s" % connection_object.hostIdentifier)

def execute_query(connection_object, query, *args, **kwargs):
    """Retrieves a connection and executes the given query returning a cursor to iterate over results.

    The remaining positional and keyword arguments are passed with the query to the execute function
    """
    grt.log_debug3("db.mssql", "execute %s %s %s\n" % (query, args, kwargs))
    return get_connection(connection_object).cursor().execute(query, *args, **kwargs)


def find_grt_object(name, collection):
    """Finds an object with the given name within a collection of GRT objects (such as grt.List).

    Returns the found object or None if there was no object with the given name in the collection.
    """
    for obj in collection:
        if obj.name == name:
            return obj
    return None

###############################################################################

@ModuleInfo.export(grt.STRING, grt.STRING)
def quoteIdentifier(name):
    return "[%s]" % name

@ModuleInfo.export(grt.STRING, grt.classes.GrtNamedObject)
def fullyQualifiedObjectName(obj):
    owner = obj.owner
    if owner and isinstance(owner, grt.classes.db_Schema):
        if owner.owner and isinstance(owner.owner, grt.classes.db_Catalog):
            return quoteIdentifier(owner.owner.name)+"."+quoteIdentifier(owner.name)+"."+quoteIdentifier(obj.name)
    elif owner and isinstance(owner, grt.classes.db_Catalog):
        return quoteIdentifier(owner.name)+"."+quoteIdentifier(obj.name)
    return quoteIdentifier(obj.name)
    

#########  Connection related functions #########

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.STRING)
def connect(connection, password):
    '''Establishes a connection to the server and stores the connection object in the connections pool.

    It first looks for a connection with the given connection parameters in the connections pool to
    reuse existent connections. If such connection is found it queries the server to ensure that the
    connection is alive and reestablishes it if is dead. If no suitable connection is found in the
    connections pool, a new one is created and stored in the pool.

    Parameters:
    ===========
        connection:  an object of the class db_mgmt_Connection storing the parameters
                     for the connection.
        password:    a string with the password to use for the connection.
    '''
    con = None
    try:
        con = get_connection(connection)
        try:
            if not con.cursor().execute('SELECT 1'):
                raise Exception("connection error")
        except Exception, exc:
            grt.send_info("Connection to %s apparently lost, reconnecting..." % connection.hostIdentifier)
            raise NotConnectedError("Connection error")
    except NotConnectedError, exc:
        host_identifier = connection.hostIdentifier
        version = ""
        grt.send_info("Connecting to %s..." % host_identifier)        
        import pyodbc
        try:
            con = db_driver.connect(connection, password)
            
            # Adds data type conversion functions for pyodbc
            if connection.driver.driverLibraryName == 'pyodbc':
                version = con.execute("SELECT CAST(SERVERPROPERTY('ProductVersion') AS VARCHAR)").fetchone()[0]
                majorVersion = int(version.split('.', 1)[0])
                if majorVersion >= 9:
                    con.add_output_converter(-150, lambda value: value if value is None else value.decode('utf-16'))
                    con.add_output_converter(0, lambda value: value if value is None else value.decode('utf-16'))
                else:
                    con.add_output_converter(-150, lambda value: value if value is None else str(value))
                    con.add_output_converter(0, lambda value: value if value is None else str(value))

        except pyodbc.Error, odbc_err:
            # 28000 is from native SQL Server driver... 42000 seems to be from FreeTDS
            if len(odbc_err.args) == 2 and odbc_err.args[0] in ('28000', '42000') and "(18456)" in odbc_err.args[1]:
                raise grt.DBLoginError(odbc_err.args[1])

        if not con:
            grt.send_error('Connection failed', str(exc))
            raise
        grt.send_info("Connected to %s, %s" % (host_identifier, version))
        
        _connections[connection.__id__] = {"connection" : con }
        _connections[connection.__id__]["version"] = getServerVersion(connection)
    return 1


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def disconnect(connection):
    if connection.__id__ in _connections:
        del _connections[connection.__id__]  # pyodbc cursors are automatically closed when deleted
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def isConnected(connection):
    return 1 if connection.__id__ in _connections else 0


#########  Exploratory functions (these only return useful info without reverse engineering) #########

@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return 'Mssql'

@ModuleInfo.export(grt.classes.GrtVersion, grt.classes.db_mgmt_Connection)
def getServerVersion(connection):
    """Returns a GrtVersion instance containing information about the server version."""
    version = grt.classes.GrtVersion()
    ver_string = execute_query(connection, "SELECT CAST(SERVERPROPERTY('ProductVersion') AS VARCHAR)").fetchone()[0]
    ver_parts = [ int(part) for part in ver_string.split('.') ] + 4*[ 0 ]
    version.majorNumber, version.minorNumber, version.releaseNumber, version.buildNumber = ver_parts[:4]
    return version

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection)
def getCatalogNames(connection):
    """Returns a list of the available catalogs.

    [NOTE] From MSDN: [A catalog] is equivalent to a databases in SQL Server.
    """
    query_pre_90 = 'SELECT name FROM master.dbo.sysdatabases'
    query_post_90 = 'exec sys.sp_databases'
    
    serverVersion = connected_server_version(connection)

    query = query_pre_90 if serverVersion.majorNumber < 9 else query_post_90

    return [ row[0] for row in execute_query(connection, query) ]


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING)
def getSchemaNames(connection, catalog_name):
    """Returns a list of schemata for the given connection object."""

    execute_query(connection, 'USE %s' % quoteIdentifier(catalog_name))
    query_post70 = """SELECT TABLE_SCHEMA AS SCHEMANAME, max(TABLE_CATALOG) AS CATALOGNAME
    FROM INFORMATION_SCHEMA.TABLES
    WHERE TABLE_CATALOG = '%s'
    GROUP BY TABLE_SCHEMA
UNION
    SELECT ROUTINE_SCHEMA AS SCHEMANAME, max(ROUTINE_CATALOG) AS CATALOGNAME
    FROM INFORMATION_SCHEMA.ROUTINES
    GROUP BY ROUTINE_SCHEMA;""" % catalog_name
    query_pre70 = """SELECT TABLE_SCHEMA AS SCHEMANAME, max(TABLE_CATALOG) AS CATALOGNAME
FROM INFORMATION_SCHEMA.TABLES
    WHERE TABLE_CATALOG = '%s'
GROUP BY TABLE_SCHEMA;""" % catalog_name

    serverVersion = connected_server_version(connection)

    query = query_pre70 if serverVersion.majorNumber <= 7 else query_post70
    #return [ row[1]+'.'+row[0] for row in execute_query(connection, query) ]
    return [ row[0] for row in execute_query(connection, query) ]


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTableNames(connection, catalog_name, schema_name):
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog_name))
    query_post_90 = """SELECT name
FROM sys.tables
WHERE schema_id = SCHEMA_ID('%s')""" % (schema_name)
    query_pre_90 = """SELECT TABLE_NAME
FROM INFORMATION_SCHEMA.TABLES
WHERE TABLE_TYPE='BASE TABLE' AND TABLE_SCHEMA='%s'""" % schema_name

    serverVersion = connected_server_version(connection)

    query = query_pre_90 if serverVersion.majorNumber < 9 else query_post_90

    return [ table_name[0] for table_name in execute_query(connection, query) ]


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getViewNames(connection, catalog_name, schema_name):
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog_name))
    query_post_90 = """SELECT name
FROM sys.views
WHERE SCHEMA_NAME(schema_id) = '%s';""" % schema_name
    query_pre_90 = """SELECT TABLE_NAME
FROM INFORMATION_SCHEMA.VIEWS
WHERE TABLE_CATALOG='%s' AND TABLE_SCHEMA='%s'""" % (catalog_name, schema_name)

    serverVersion = connected_server_version(connection)
    if serverVersion.majorNumber <= 8:
        return []
    query = query_pre_90 if serverVersion.majorNumber < 9 else query_post_90

    return [ views_info[0] for views_info in execute_query(connection, query) ]


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTriggerNames(connection, catalog_name, schema_name):
    # Query from http://msdn.microsoft.com/en-us/library/ms188746.aspx  (post 90)
    #            http://www.sharpdeveloper.net/content/archive/2007/06/26/search-trigger-text-sql-server-2005.aspx  (pre 90)
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog_name))
    query_post_90 = """SELECT name
FROM sys.triggers
WHERE OBJECT_SCHEMA_NAME(object_id) = '%s'"""
    query_pre_90 = """SELECT OBJECT_NAME(id)
FROM sysobjects
WHERE OBJECTPROPERTY(id, 'IsTrigger') = 1 AND uid = SCHEMA_ID('%s')
GROUP BY OBJECT_NAME(id)"""
    serverVersion = connected_server_version(connection)

    # SQL Server 2000 and before had y.a. different way of fetching triggers
    if serverVersion.majorNumber <= 8:
        return []
    query = query_pre_90 if serverVersion.majorNumber <= 9 else query_post_90

    return [ trigger_info[0] for trigger_info in execute_query(connection, query % schema_name) ]


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getProcedureNames(connection, catalog_name, schema_name):
    # Query from http://msdn.microsoft.com/en-us/library/ms345522.aspx#_FAQ9
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog_name))
    query_post_90 = """SELECT name
FROM sys.procedures
WHERE schema_id = SCHEMA_ID('%s')"""
    query_pre_90 = """SELECT ROUTINE_NAME as name
FROM INFORMATION_SCHEMA.Routines
WHERE ROUTINE_SCHEMA='%s' AND ROUTINE_TYPE='PROCEDURE'"""

    serverVersion = connected_server_version(connection)
    if serverVersion.majorNumber <= 8:
        return []
    query = query_pre_90 if serverVersion.majorNumber < 9 else query_post_90

    return [ proc_info[0] for proc_info in execute_query(connection, query % schema_name) ]


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getFunctionNames(connection, catalog_name, schema_name):
    # Query from http://msdn.microsoft.com/en-us/library/ms345522.aspx#_FAQ12
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog_name))
    query_post_90 = """SELECT name
FROM sys.objects
WHERE type_desc LIKE '%FUNCTION%' AND schema_id = SCHEMA_ID(?)"""
    query_pre_90 = """SELECT ROUTINE_NAME as name
FROM INFORMATION_SCHEMA.Routines
WHERE ROUTINE_SCHEMA=? AND ROUTINE_TYPE='FUNCTION'"""

    serverVersion = connected_server_version(connection)
    if serverVersion.majorNumber <= 8:
        return []
    query = query_pre_90 if serverVersion.majorNumber < 9 else query_post_90

    return [ func_info[0] for func_info in execute_query(connection, query, schema_name) ]


#########  Reverse Engineering functions #########

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING, (grt.LIST, grt.STRING), grt.DICT)
def reverseEngineer(connection, catalog_name, schemata_list, options):

    #catalog = reverseEngineerCatalog(connection, catalog_name)
    grt.send_progress(0, "Reverse engineering catalog information")
    catalog = grt.classes.db_mssql_Catalog()
    catalog.name = catalog_name
    catalog.simpleDatatypes.remove_all()
    catalog.simpleDatatypes.extend(connection.driver.owner.simpleDatatypes)

    collation_row = execute_query(connection, "SELECT DATABASEPROPERTYEX(?, 'Collation')", catalog_name).fetchone()
    if collation_row:
        catalog.defaultCollationName = collation_row[0] or ''  # Avoid None in defaultCollationName
    
    grt.send_progress(0.05, "Reverse engineering User Data Types...")
    check_interruption()
    reverseEngineerUserDatatypes(connection, catalog)

    # calculate total workload 1st
    grt.send_progress(0.1, 'Preparing...')
    table_count_per_schema = {}
    view_count_per_schema = {}
    routine_count_per_schema = {}
    trigger_count_per_schema = {}
    total_count_per_schema = {}

    get_tables = options.get("reverseEngineerTables", True)
    get_triggers = options.get("reverseEngineerTriggers", True)
    get_views = options.get("reverseEngineerViews", True)
    get_routines = options.get("reverseEngineerRoutines", True)

    # 10% of the progress is for preparation
    total = 1e-10  # total should not be zero to avoid DivisionByZero exceptions
    i = 1.0
    accumulated_progress = 0.1
    for schema_name in schemata_list:
        check_interruption()
        table_count_per_schema[schema_name] = len(getTableNames(connection, catalog_name, schema_name)) if get_tables else 0
        view_count_per_schema[schema_name] = len(getViewNames(connection, catalog_name, schema_name)) if get_views else 0
        check_interruption()
        routine_count_per_schema[schema_name] = len(getProcedureNames(connection, catalog_name, schema_name)) + len(getFunctionNames(connection, catalog_name, schema_name)) if get_routines else 0
        trigger_count_per_schema[schema_name] = len(getTriggerNames(connection, catalog_name, schema_name)) if get_triggers else 0

        total_count_per_schema[schema_name] = (table_count_per_schema[schema_name] + view_count_per_schema[schema_name] +
                                               routine_count_per_schema[schema_name] + trigger_count_per_schema[schema_name] + 1e-10)
        total += total_count_per_schema[schema_name]

        grt.send_progress(accumulated_progress + 0.1 * (i / (len(schemata_list) + 1e-10) ), "Gathered stats for %s" % schema_name)
        i += 1.0

    # Now take 60% in the first pass of reverse engineering:
    accumulated_progress = 0.2
    grt.reset_progress_steps()
    grt.begin_progress_step(accumulated_progress, accumulated_progress + 0.6)
    accumulated_schema_progress = 0.0
    for schema_name in schemata_list:
        schema_progress_share = total_count_per_schema.get(schema_name, 0.0) / total

        grt.begin_progress_step(accumulated_schema_progress, accumulated_schema_progress + schema_progress_share)
        
        this_schema_progress = 0.0

        schema = grt.classes.db_mssql_Schema()
        schema.owner = catalog
        schema.name = schema_name
        schema.defaultCollationName = catalog.defaultCollationName
        catalog.schemata.append(schema)

        # Reverse engineer tables:
        step_progress_share = table_count_per_schema[schema_name] / (total_count_per_schema[schema_name] + 1e-10)
        if get_tables:
            check_interruption()
            grt.send_info('Reverse engineering %i tables from %s' % (table_count_per_schema[schema_name], schema_name))
            grt.begin_progress_step(this_schema_progress, this_schema_progress + step_progress_share)
            reverseEngineerTables(connection, schema)
            grt.end_progress_step()

        this_schema_progress += step_progress_share
        grt.send_progress(this_schema_progress, 'First pass of table reverse engineering for schema %s completed!' % schema_name)

        # Reverse engineer views:
        step_progress_share = view_count_per_schema[schema_name] / (total_count_per_schema[schema_name] + 1e-10)
        if get_views:
            check_interruption()
            grt.send_info('Reverse engineering %i views from %s' % (view_count_per_schema[schema_name], schema_name))
            grt.begin_progress_step(this_schema_progress, this_schema_progress + step_progress_share)
            reverseEngineerViews(connection, schema)
            grt.end_progress_step()

        this_schema_progress += step_progress_share
        grt.send_progress(this_schema_progress, 'Reverse engineering of views for schema %s completed!' % schema_name)

        # Reverse engineer routines:
        step_progress_share = routine_count_per_schema[schema_name] / (total_count_per_schema[schema_name] + 1e-10)
        if get_routines:
            check_interruption()
            grt.send_info('Reverse engineering %i routines from %s' % (routine_count_per_schema[schema_name], schema_name))
            grt.begin_progress_step(this_schema_progress, this_schema_progress + step_progress_share/2)
            schema.routines.remove_all()
            reverseEngineerProcedures(connection, schema)
            grt.end_progress_step()
            check_interruption()
            grt.begin_progress_step(this_schema_progress + step_progress_share/2, this_schema_progress + step_progress_share)
            reverseEngineerFunctions(connection, schema)
            grt.end_progress_step()

        this_schema_progress += step_progress_share
        grt.send_progress(this_schema_progress, 'Reverse engineering of routines for schema %s completed!' % schema_name)

        # Reverse engineer triggers:
        step_progress_share = trigger_count_per_schema[schema_name] / (total_count_per_schema[schema_name] + 1e-10)
        if get_triggers:
            check_interruption()
            grt.send_info('Reverse engineering %i triggers from %s' % (trigger_count_per_schema[schema_name], schema_name))
            grt.begin_progress_step(this_schema_progress, this_schema_progress + step_progress_share)
            reverseEngineerTriggers(connection, schema)
            grt.end_progress_step()

        this_schema_progress += step_progress_share
        grt.send_progress(this_schema_progress, 'Reverse engineering of triggers for schema %s completed!' % schema_name)
    
        accumulated_schema_progress += schema_progress_share
        grt.end_progress_step()

    grt.end_progress_step()

    # Now the second pass for reverse engineering tables:
    accumulated_progress = 0.8
    if get_tables:
        total_tables = sum(table_count_per_schema[schema.name] for schema in catalog.schemata)
        for schema in catalog.schemata:
            check_interruption()
            step_progress_share = 0.2 * (table_count_per_schema[schema.name] / (total_tables + 1e-10))
            grt.send_info('Reverse engineering foreign keys for tables in schema %s' % schema.name)
            grt.begin_progress_step(accumulated_progress, accumulated_progress + step_progress_share)
            reverseEngineerTables(connection, schema)
            grt.end_progress_step()
    
            accumulated_progress += step_progress_share
            grt.send_progress(accumulated_progress, 'Second pass of table reverse engineering for schema %s completed!' % schema_name)
        

    grt.send_progress(1.0, 'Reverse engineering completed!')
    return catalog


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Catalog)
def reverseEngineerUserDatatypes(connection, catalog):
    # TODO: Migrate user datatypes...
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog.name))
    query = """SELECT name, TYPE_NAME(system_type_id), max_length, precision, scale, is_nullable
FROM sys.types
WHERE is_user_defined = 1"""

    serverVersion = connected_server_version(connection)
    if serverVersion.majorNumber >= 9:  # Alias can be used as table column datatypes from SQL Server 2005 onwards
        mssql_rdbms_instance = get_mssql_rdbms_instance()
        catalog.userDatatypes.remove_all()
        for name, base_type, length, precision, scale, is_nullable in execute_query(connection, query):
            datatype = grt.classes.db_mssql_UserDatatype()
            datatype.name = name.upper()
            datatype.characterMaximumLength = length
            datatype.numericPrecision = precision
            datatype.numericScale = scale
            datatype.isNullable = is_nullable
            actual_type_found = False
            for simple_type in mssql_rdbms_instance.simpleDatatypes:
                if base_type and simple_type.name == base_type.upper():
                    datatype.actualType = simple_type
                    actual_type_found = True
                    break
            if not actual_type_found:
                grt.send_warning('MSSQL reverseEngineerUserDatatypes', 'Could not find base type "%s" for user defined type "%s"' % (base_type, name) )
            datatype.owner = catalog
            catalog.userDatatypes.append(datatype)
    return 0

#@# make this call reverseEngineerSchema for each schema in the catalog
#@# make this return a new catalog object
@ModuleInfo.export(grt.classes.db_mssql_Catalog, grt.classes.db_mgmt_Connection, grt.STRING)
def reverseEngineerCatalog(connection, catalog_name):
    catalog = grt.classes.db_mssql_Catalog()
    catalog.name = catalog_name
    catalog.simpleDatatypes.remove_all()
    catalog.simpleDatatypes.extend(connection.driver.owner.simpleDatatypes)

    reverseEngineerUserDatatypes(connection, catalog)

    schemata_names = getSchemaNames(connection, catalog_name)
    catalog.schemata.remove_all()
    for schema_name in schemata_names:
        schema = grt.classes.db_mssql_Schema()
        schema.name = schema_name#.split('.')[-1] if '.' in schema_name else schema_name
        schema.owner = catalog
        catalog.schemata.append(schema)
    return catalog

#@# uncomment this
#@# make this return a new schema object and remove from the param list
#@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Schema)
#def reverseEngineerSchema(connection, schema):
#    table_names = getSchemaNames(connection, password)
#    catalog.schemata.remove_all()
#    for schema_name in schemata_names:
#        schema = grt.classes.db_mssql_Schema()
#        schema.name = schema_name
#        catalog.schemata.append(schema)
#    return 0

#@# remove redundant params
#@# make it return a list of table objects
#@# params should be the schema object and a list of table names to be revenged. only tables in that list should be fetched
@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Schema)
def reverseEngineerTables(connection, schema):
    # Since there are some reverse engineering stages that requires all table names and table columns
    # in the database to be set, these should be done after a first pass that rev engs their requirements
    progress_flags = _connections[connection.__id__].setdefault('_rev_eng_progress_flags', [])
    is_first_pass = not ('%s_tables_first_pass' % schema.name) in progress_flags

    if is_first_pass:
        catalog = schema.owner
        execute_query(connection, 'USE %s' % quoteIdentifier(catalog.name))
        query_post_90 = """SELECT t.name, p.value
FROM sys.tables t LEFT JOIN sys.extended_properties p ON p.major_id = t.object_id AND p.minor_id = 0 AND p.name = 'MS_Description' AND p.class_desc = 'OBJECT_OR_COLUMN'
WHERE schema_id = SCHEMA_ID('%s')"""

        query_pre_90 = """SELECT TABLE_NAME, ''
                          FROM INFORMATION_SCHEMA.TABLES
                          WHERE TABLE_TYPE='BASE TABLE' AND TABLE_SCHEMA='%s'"""

        serverVersion = connected_server_version(connection)
    
        query = query_pre_90 if serverVersion.majorNumber < 9 else query_post_90
    
        schema.tables.remove_all()
        table_names = [(row[0], row[1]) for row in execute_query(connection, query % schema.name) ]
        total = len(table_names) + 1e-10
        i = 0.0
        for table_name, table_comment in table_names:
            grt.send_progress(i / total, 'Retrieving table %s.%s...' % (schema.name, table_name))
            table = grt.classes.db_mssql_Table()
            table.name = table_name
            schema.tables.append(table)
            table.owner = schema
            table.comment = table_comment or ''  # table_comment can be None
    
            reverseEngineerTableColumns(connection, table)
            reverseEngineerTablePK(connection, table)
            reverseEngineerTableIndices(connection, table)
    
            i += 1.0
            
#        grt.send_progress(1.0, 'Reverse engineering completed!')
        progress_flags.append('%s_tables_first_pass' % schema.name)
    else:  # Second pass
        i = 1.0
        total = len(schema.tables) + 1e-10
        for table in schema.tables:
            reverseEngineerTableFKs(connection, table)
            grt.send_progress(i / total, 'Reverse engineering of foreign keys in table %s.%s completed' % (schema.name, table.name))
            i += 1.0

    return 0

#@# no need to export this. Remove the redundant params. catalog = schema.owner, schema = table.owner
@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Table)
def reverseEngineerTableColumns(connection, table):
    schema = table.owner
    catalog = schema.owner
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog.name))
    query_post90 = """SELECT sys.columns.name AS COLUMN_NAME,
    sys.columns.is_nullable AS IS_NULLABLE, sys.types.name AS DATA_TYPE, sys.columns.max_length AS CHARACTER_MAXIMUM_LENGTH,
    sys.columns.precision AS NUMERIC_PRECISION, sys.columns.scale AS NUMERIC_SCALE,
    sys.columns.collation_name AS COLLATION_NAME, is_identity AS IS_IDENTITY_COLUMN,
    CAST (sys.default_constraints.definition as NVARCHAR(max)) as COLUMN_DEFAULT,
    sys.extended_properties.value as COLUMN_COMMENT
FROM sys.columns JOIN sys.types ON sys.columns.user_type_id=sys.types.user_type_id JOIN sys.objects ON sys.columns.object_id = sys.objects.object_id
     LEFT JOIN sys.default_constraints ON (sys.columns.column_id=sys.default_constraints.parent_column_id AND sys.columns.object_id=sys.default_constraints.parent_object_id)
     LEFT JOIN sys.extended_properties ON sys.extended_properties.major_id = sys.columns.object_id and sys.extended_properties.minor_id = sys.columns.column_id and sys.extended_properties.name = 'MS_Description' and sys.extended_properties.class_desc = 'OBJECT_OR_COLUMN'
WHERE sys.objects.schema_id=SCHEMA_ID(?) AND sys.objects.name=?
ORDER BY sys.columns.column_id"""

    query_pre90 = """SELECT COLUMN_NAME, COLUMN_DEFAULT,
        IS_NULLABLE, DATA_TYPE, CHARACTER_MAXIMUM_LENGTH,
        NUMERIC_PRECISION, NUMERIC_SCALE, DATETIME_PRECISION,
        CHARACTER_SET_CATALOG, CHARACTER_SET_SCHEMA, CHARACTER_SET_NAME,
        COLLATION_NAME, DOMAIN_CATALOG, DOMAIN_SCHEMA, DOMAIN_NAME,
        (c.status & 128) / 128 AS IS_IDENTITY_COLUMN,
        '' AS COLUMN_COMMENT
FROM INFORMATION_SCHEMA.COLUMNS, sysobjects t, sysusers u, syscolumns c
WHERE TABLE_SCHEMA=? AND TABLE_NAME=? AND
        u.name=TABLE_SCHEMA AND t.name=TABLE_NAME AND
        u.uid=t.uid AND c.id=t.id AND
        c.name=COLUMN_NAME
ORDER BY ORDINAL_POSITION"""
    
    serverVersion = connected_server_version(connection)

    query = query_pre90 if serverVersion.majorNumber < 9 else query_post90
    rows = execute_query(connection, query, (schema.name, table.name) )

    mssql_rdbms_instance = get_mssql_rdbms_instance()
    mssql_simple_datatypes_list = [ datatype.name for datatype in mssql_rdbms_instance.simpleDatatypes ]
    user_datatypes_list = [ datatype.name for datatype in catalog.userDatatypes ]

    col_names = [ col_description[0] for col_description in rows.description ]
    for row in rows:
        row_values = dict( nameval for nameval in zip(col_names, row) )
        column = grt.classes.db_mssql_Column()
        column.name = row_values['COLUMN_NAME'] or ""
        column.isNotNull = not ( row_values['IS_NULLABLE']=='YES' if serverVersion.majorNumber < 9 else row_values['IS_NULLABLE'] )
        column.collationName = row_values['COLLATION_NAME'] or ""
        column.length = row_values['CHARACTER_MAXIMUM_LENGTH'] or 0
        column.scale = row_values['NUMERIC_SCALE'] or 0
        column.precision = row_values['NUMERIC_PRECISION'] or 0
        column.comment = row_values['COLUMN_COMMENT'] or ""
        column.identity = row_values['IS_IDENTITY_COLUMN']
        user_datatype = None
        try:
            datatype = mssql_simple_datatypes_list.index( row_values['DATA_TYPE'].upper() )
        except ValueError:
            try:
                user_datatype = catalog.userDatatypes[user_datatypes_list.index( row_values['DATA_TYPE'].upper() )]
            except (ValueError, TypeError):
                user_datatype = None
                datatype = mssql_simple_datatypes_list.index('VARCHAR')
                column.length = 255
                msg = 'Column datatype "%s" for column "%s" in table "%s.%s" reverse engineered as VARCHAR(255)' % (row_values['DATA_TYPE'].upper(), column.name, schema.name, table.name)
                grt.send_warning('MSSQL reverseEngineerTableColumns', msg)
            else:
                datatype = None
                column.userType = user_datatype

        if datatype is not None:
            column.simpleType = mssql_rdbms_instance.simpleDatatypes[datatype]

        # When information about the columns are retrieved from  sys.columns (server version is 9 or greater) - 
        # field CHARACTER_MAXIMUM_LENGTH return length in bytes, so length in characters for type NCHAR and NVARCHAR
        # sholud be divided by two. 
        if serverVersion.majorNumber >= 9 and column.simpleType is not None and column.simpleType.name in ['NCHAR', 'NVARCHAR']:
            column.length = column.length/2

        default_value = row_values['COLUMN_DEFAULT']
        
        if default_value is not None:
            def remove_parenthesis(data):
                if data.startswith('(') and data.endswith(')'):
                    data = remove_parenthesis(data[1:-1])
                return data

            # remove () and (N) from (N'xxxx') but not from (NULL)
            #group_name = column.simpleType.group.name if column.simpleType else ( column.userType.actualType.group.name if column.userType else '' )
            #if group_name == 'string' and default_value.startswith('(N') and default_value.endswith(')'):
            #    default_value = default_value[2:-1]
            
            if default_value == '(NULL)':
                column.defaultValueIsNull = True
            else:
                # Remove now any pair of parenthesis that may remain
                column.defaultValue = remove_parenthesis(default_value)
                #column.defaultValue = default_value.strip("'")

        table.addColumn(column)

        # TODO: charset name

    return 0

#@# don't export, remove reundant params
@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Table)
def reverseEngineerTablePK(connection, table):
    """Reverse engineers the primary key(s) for the given table."""

    schema = table.owner
    catalog = schema.owner

    execute_query(connection, 'USE %s' % quoteIdentifier(catalog.name))
    query = "exec sp_pkeys '%s', '%s'" % (table.name, schema.name)

    if len(table.columns) == 0:
        grt.send_error('Migration: reverseEngineerTablePK', 'Reverse engineer of table %s.%s was attempted but the table has no columns attribute' % (schema.name, table.name) )
        return 1    # Table must have columns reverse engineered before we can rev eng its primary key(s)

    pk_col_names = [ row[3] for row in execute_query(connection, query) ]

    for pk_column in pk_col_names:
        column = find_grt_object(pk_column, table.columns)
        if column:
            table.addPrimaryKeyColumn(column)
    return 0

#@# don't export, remove reundant params
@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Table)
def reverseEngineerTableIndices(connection, table):
    """Reverse engineers the indices for the given table."""

    schema = table.owner
    catalog = schema.owner
    
    if len(table.columns) == 0:
        grt.send_error('Migration: reverseEngineerTableIndices', 'Reverse engineer of table %s.%s was attempted but the table has no columns attribute' % (schema.name, table.name) )
        return 1    # Table must have columns reverse engineered before we can rev eng its indices

    version = connected_server_version(connection)

    execute_query(connection, 'USE %s' % quoteIdentifier(catalog.name))
    if version.majorNumber <= 9:
        get_indices_query_pre90 = """SELECT u.name as TABLE_SCHEMA, 
                 o.name as TABLE_NAME, i.name AS INDEX_NAME, c.name AS COLUMN_NAME, 
                 (i.status & 1) AS IGNORE_DUPLICATE_KEYS, 
                 (i.status & 2) / 2 AS IS_UNIQUE, 
                 (i.status & 4) / 4 AS IGNORE_DUPLICATE_ROWS, 
                 (i.status & 16) / 16 AS IS_CLUSTERED, 
                 (i.status & 2048) / 2048 AS IS_PRIMARY_KEY, 
                 (i.status & 4096) / 4096 AS IS_UNIQUE_KEY 
                FROM sysindexes i, sysobjects o, sysusers u, sysindexkeys k, syscolumns c 
                WHERE u.uid=o.uid AND i.id = o.id AND k.indid=i.indid AND k.id=i.id AND 
                 c.id=i.id AND c.colid=k.colid AND 
                 i.indid > 0 AND i.indid < 255 AND o.type = 'U' AND 
                 (i.status & 64)=0 AND (i.status & 8388608)=0 AND 
                 (i.status & 2048)=0 AND  u.name='%s' AND o.name='%s'
               ORDER BY i.name, k.keyno""" % (schema.name, table.name)
        index_rows = execute_query(connection, get_indices_query_pre90).fetchall()
        index = None
        for table_schema, table_name, index_name, column_name, ignore_dup_keys, is_unique, \
            ignore_dup_rows, is_clustered, is_primary_key, is_unique in index_rows:
            if not is_primary_key:
                if not index or index.name != index_name:
                    index = grt.classes.db_mssql_Index()
                    index.name = index_name
                    index.isPrimary = is_primary_key
                    index.unique = is_unique
                    # 'SPATIAL' if type_desc == 'SPATIAL' else 
                    index.indexType = ('UNIQUE' if is_unique else 'INDEX')
                    index.clustered = is_clustered
                    #index.hasFilter = has_filter
                    #index.filterDefinition = filter_definition if has_filter else ''
                    index.ignoreDuplicateRows = ignore_dup_rows
                    table.addIndex(index)

                # Get the columns for the index:
                column = find_grt_object(column_name, table.columns)
                if column:
                    index_column = grt.classes.db_mssql_IndexColumn()
                    index_column.name = index_name + '.' + column_name
                    #index_column.descend = is_descending_key
                    index_column.referencedColumn = column
                    index.columns.append(index_column)

    else:
        get_indices_query = """SELECT object_id, name, index_id, CAST (type_desc AS NVARCHAR) AS type_desc, is_unique,
    is_primary_key, is_disabled, has_filter, CAST (filter_definition AS NVARCHAR) AS filter_definition
    FROM sys.indexes
    WHERE sys.indexes.object_id = OBJECT_ID('%s.%s')""" % (schema.name, table.name)

        get_index_columns_query = """SELECT c.name, ic.is_descending_key
    FROM sys.index_columns ic JOIN sys.columns c on (ic.column_id=c.column_id and ic.object_id=c.object_id)
    WHERE ic.object_id=%i and ic.index_id=%i"""

        index_rows = execute_query(connection, get_indices_query).fetchall()
        for table_id, index_name, index_id, type_desc, is_unique, is_primary_key, is_disabled, has_filter, filter_definition in index_rows:
            if index_name is None:
                # this is not a real index, but a HEAP thingmajig...
                continue
            if not is_disabled and not is_primary_key:    # Do not create the index if it is disabled or if it is part of the primary key
                index = grt.classes.db_mssql_Index()
                index.name = index_name
                index.isPrimary = is_primary_key
                index.unique = is_unique
                index.indexType = 'SPATIAL' if type_desc == 'SPATIAL' else ('UNIQUE' if is_unique else 'INDEX')
                index.clustered = int( type_desc == 'CLUSTERED')
                index.hasFilter = has_filter
                index.filterDefinition = filter_definition if has_filter else ''
                # TODO: set values for fields deferability and ignoreDuplicateRows

                # Get the columns for the index:
                for column_name, is_descending_key in execute_query(connection, get_index_columns_query % (table_id, index_id) ):
                    column = find_grt_object(column_name, table.columns)
                    if column:
                        index_column = grt.classes.db_mssql_IndexColumn()
                        index_column.name = index_name + '.' + column_name
                        index_column.descend = is_descending_key
                        index_column.referencedColumn = column
                        index.columns.append(index_column)
                table.addIndex(index)
    return 0

#@# don't export, remove reundant params
@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Table)
def reverseEngineerTableFKs(connection, table):
    """Reverse engineers the foreign keys for the given table."""

    schema = table.owner
    catalog = schema.owner

    execute_query(connection, 'USE %s' % quoteIdentifier(catalog.name))

    version = connected_server_version(connection)
    if version.majorNumber <= 9:
        get_fks_query_pre9 = """SELECT fk.name AS CONSTRAINT_NAME, 
 c.name AS COLUMN_NAME, ref_u.name AS REF_SCHEMA_NAME, 
 ref_tbl.name AS REF_TABLE_NAME, ref_c.name AS REF_COLUMN_NAME, 
 CASE WHEN (ObjectProperty(sfk.constid, 'CnstIsUpdateCascade')=1) THEN 
  'CASCADE' ELSE 'NO ACTION' END as UPDATE_RULE, 
 CASE WHEN (ObjectProperty(sfk.constid, 'CnstIsDeleteCascade')=1) THEN 
  'CASCADE' ELSE 'NO ACTION' END as DELETE_RULE 
FROM sysusers u, sysobjects t, sysobjects fk, sysforeignkeys sfk, 
 syscolumns c, sysobjects ref_tbl, sysusers ref_u, syscolumns ref_c 
WHERE u.name=? AND t.name=? AND 
 t.uid=u.uid AND t.xtype='U' AND 
 sfk.fkeyid=t.id AND fk.id=sfk.constid AND 
 c.id=t.id AND c.colid=sfk.fkey AND 
 ref_tbl.id=sfk.rkeyid AND ref_tbl.uid=ref_u.uid AND 
 ref_c.id=ref_tbl.id AND ref_c.colid=sfk.rkey 
ORDER BY sfk.constid, sfk.keyno"""
        fk_rows = execute_query(connection, get_fks_query_pre9, (schema.name, table.name)).fetchall()
        foreign_key = None
        for fk_name, column_name, referenced_schema_name, referenced_table_name, referenced_column_name, update_rule, delete_rule in fk_rows:
            if foreign_key is None or foreign_key.name != fk_name:
                if foreign_key:
                    table.foreignKeys.append(foreign_key)
                foreign_key = grt.classes.db_mssql_ForeignKey()
                foreign_key.name = fk_name or ""
                foreign_key.owner = table
                foreign_key.deleteRule = delete_rule or ""
                foreign_key.updateRule = update_rule or ""
                foreign_key.modelOnly = 0

                # Get the details for the foreign key:
                referenced_schema = find_grt_object(referenced_schema_name, catalog.schemata)
                if referenced_schema:
                    referenced_table = find_grt_object(referenced_table_name, referenced_schema.tables)
                else:
                    grt.send_error('Migration: reverseEngineerTableFKs', 'While reverse engineering table %s.%s: Referenced schema %s in foreign key %s not found in catalog schemata' % (schema.name, table.name, referenced_schema_name, fk_name) )
                    return 1
                if not referenced_table:
                    grt.send_error('Migration: reverseEngineerTableFKs', 'While reverse engineering table %s.%s: Referenced table %s.%s in foreign key %s not found' % (schema.name, table.name, referenced_schema_name, referenced_table_name, fk_name) )
                    return 1
                foreign_key.referencedTable = referenced_table

            column = find_grt_object(column_name, table.columns)
            if not column:
                grt.send_error('Migration: reverseEngineerTableFKs', 'While reverse engineering table %s.%s: Foreign key column %s in foreign key %s not found in this table' % (schema.name, table.name, column_name, fk_name) )
                return 1

            referenced_column = find_grt_object(referenced_column_name, referenced_table.columns)
            if not referenced_column:
                grt.send_error('Migration: reverseEngineerTableFKs', 'While reverse engineering table %s.%s: Foreign key column %s in foreign key %s not found in table %s.%s' % (schema.name, table.name, referenced_column_name, fk_name, referenced_schema_name, referenced_table_name) )
                return 1

            foreign_key.columns.append(column)
            foreign_key.referencedColumns.append(referenced_column)
        if foreign_key:
            table.foreignKeys.append(foreign_key)
    else:
        get_fks_query = """SELECT object_id as fk_id, name, delete_referential_action_desc, update_referential_action_desc, is_disabled
    FROM sys.foreign_keys
    WHERE parent_object_id=OBJECT_ID('%s.%s')""" % (schema.name, table.name)

        get_fk_mapping_query = """SELECT COL_NAME(fkc.parent_object_id, fkc.parent_column_id) as parent_column,
         OBJECT_SCHEMA_NAME(fkc.referenced_object_id) as referenced_schema,
         OBJECT_NAME(fkc.referenced_object_id) as referenced_table,
         COL_NAME(fkc.referenced_object_id, fkc.referenced_column_id) as referenced_column
    FROM sys.foreign_key_columns fkc JOIN sys.foreign_keys fk ON fkc.constraint_object_id = fk.object_id
    WHERE fk.object_id=?;
        """

        if len(table.columns) == 0:
            grt.send_error('Migration: reverseEngineerTableFKs', 'Reverse engineer of table %s.%s was attempted but the table has no columns attribute' % (schema.name, table.name) )
            return 1    # Table must have columns reverse engineered before we can rev eng its foreign keys

        fk_rows = execute_query(connection, get_fks_query).fetchall()
        table.foreignKeys.remove_all()
        for fk_id, fk_name, delete_referential_action, update_referential_action, is_disabled in fk_rows:
            if not is_disabled:    # Do not create the foreign key if it is disabled
                foreign_key = grt.classes.db_mssql_ForeignKey()
                foreign_key.name = fk_name or ""
                foreign_key.owner = table
                foreign_key.deleteRule = delete_referential_action or ""
                foreign_key.updateRule = update_referential_action or ""
                foreign_key.modelOnly = 0

                # Get the details for the foreign key:
                referenced_table = None
                for column_name, referenced_schema_name, referenced_table_name, referenced_column_name in execute_query(connection, get_fk_mapping_query, fk_id):
                    if not referenced_table:  # Find referenced table only in the first iteration
                        referenced_schema = find_grt_object(referenced_schema_name, catalog.schemata)
                        if referenced_schema:
                            referenced_table = find_grt_object(referenced_table_name, referenced_schema.tables)
                        else:
                            grt.send_error('Migration: reverseEngineerTableFKs', 'While reverse engineering table %s.%s: The foreign key %s references the schema %s, which was not found in the list of schemas to be migrated' % (schema.name, table.name, fk_name, referenced_schema_name) )
                            return 1

                    if not referenced_table:
                        grt.send_error('Migration: reverseEngineerTableFKs', 'While reverse engineering table %s.%s: The foreign key %s references the table %s.%s, which was not found in the source schema' % (schema.name, table.name, referenced_schema_name, fk_name, referenced_table_name) )
                        return 1

                    column = find_grt_object(column_name, table.columns)
                    if not column:
                        grt.send_error('Migration: reverseEngineerTableFKs', 'While reverse engineering table %s.%s: Foreign key column %s in foreign key %s not found in this table' % (schema.name, table.name, column_name, fk_name) )
                        return 1

                    referenced_column = find_grt_object(referenced_column_name, referenced_table.columns)
                    if not referenced_column:
                        grt.send_error('Migration: reverseEngineerTableFKs', 'While reverse engineering table %s.%s: Foreign key column %s in foreign key %s not found in table %s.%s' % (schema.name, table.name, referenced_column_name, fk_name, referenced_schema_name, referenced_table_name) )
                        return 1

                    foreign_key.columns.append(column)
                    foreign_key.referencedColumns.append(referenced_column)

                foreign_key.referencedTable = referenced_table

                #TODO: Reverse engineer the index associated with this FK.

                table.foreignKeys.append(foreign_key)
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Schema)
def reverseEngineerViews(connection, schema):
    execute_query(connection, 'USE %s' % quoteIdentifier(schema.owner.name))  # catalog
    query_post_90 = """SELECT COUNT(*) OVER () AS count, OBJECT_NAME(object_id) as name, definition
FROM sys.sql_modules
WHERE OBJECT_SCHEMA_NAME(object_id) = '%(schema)s'"""
    query_pre_90 = """SELECT (SELECT COUNT(*)
FROM INFORMATION_SCHEMA.VIEWS
WHERE TABLE_SCHEMA='%(schema)s'), TABLE_NAME as name, VIEW_DEFINITION AS definition
FROM INFORMATION_SCHEMA.VIEWS
WHERE TABLE_SCHEMA='%(schema)s'"""

    serverVersion = connected_server_version(connection)

    query = query_pre_90 if serverVersion.majorNumber <= 9 else query_post_90

    cursor = execute_query(connection, query % {'schema':schema.name})
    if cursor:
        schema.views.remove_all()
        step = 0.0
        for idx, (view_count, view_name, view_definition) in enumerate(cursor):
            if idx == 0:
                step = 1.0 / (view_count + 1e-10)
            grt.send_progress(idx * step, 'Reverse engineering view %s.%s' % (schema.name, view_name))
            view = grt.classes.db_mssql_View()
            view.name = view_name or ""
            view.owner = schema
            view.sqlDefinition = view_definition
            schema.views.append(view)
#    grt.send_progress(1.0, 'Finished reverse engineering of views for the %s schema.' % schema.name)
    return 0


@ModuleInfo.export(grt.classes.db_mssql_View, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Schema, grt.STRING)
def reverseEngineerView(connection, schema, view_name):
    """Reverse engineers a view for the given schema.

    Parameters:
    ===========
        connection:      an object of the class :class:`db_mgmt_Connection` storing the parameters
                         for the connection.
        schema:          a schema object (an instance of :class:`grt.classes.db_mssql_Schema`). This
                         object must have its ``owner`` and ``name`` attributes properly set.
        view_name:       the name of the view to reverse engineer.

    Return value:
    =============
        view:            an object of the class :class:`grt.classes.db_mssql_View` with the retrieved information.
    """
    # The queries are taken from http://msdn.microsoft.com/en-us/library/ms345522.aspx#_FAQ35

    execute_query(connection, 'USE %s' % quoteIdentifier(schema.owner.name))  # catalog
    query_post_90 = """SELECT definition
FROM sys.sql_modules
WHERE object_id = OBJECT_ID('%s.%s')
"""
    query_pre_90 = """SELECT VIEW_DEFINITION AS definition
FROM INFORMATION_SCHEMA.VIEWS
WHERE TABLE_SCHEMA='%s' AND TABLE_NAME='%s'"""

    serverVersion = connected_server_version(connection)

    query = query_pre_90 if serverVersion.majorNumber < 9 else query_post_90

    resultset = execute_query(connection, query % (schema.name, view_name) )
    if resultset:
        view = grt.classes.db_mssql_View()
        view.name = view_name or ""
        view.owner = schema
        view.sqlDefinition = resultset[0][0]

    return view


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Schema)
def reverseEngineerProcedures(connection, schema):
    # http://msdn.microsoft.com/en-us/library/ms345443.aspx#TsqlProcedure
    execute_query(connection, 'USE %s' % quoteIdentifier(schema.owner.name))  # catalog
    query_post_90 = """SELECT COUNT(*) OVER () AS count, OBJECT_NAME(M.object_id) as name, definition
FROM sys.sql_modules M JOIN sys.procedures P ON M.object_id=P.object_id
WHERE OBJECT_SCHEMA_NAME(M.object_id) = '%(schema)s'"""
    query_pre_90 = """SELECT (SELECT COUNT(*)
FROM INFORMATION_SCHEMA.ROUTINES
WHERE ROUTINE_SCHEMA='%(schema)s' AND ROUTINE_TYPE='PROCEDURE'), ROUTINE_NAME as name, ROUTINE_DEFINITION as definition
FROM INFORMATION_SCHEMA.ROUTINES
WHERE ROUTINE_SCHEMA='%(schema)s' AND ROUTINE_TYPE='PROCEDURE'"""

    serverVersion = connected_server_version(connection)

    query = query_pre_90 if serverVersion.majorNumber <= 9 else query_post_90

    cursor = execute_query(connection, query % {'schema':schema.name})
    step = 0.0
    if cursor:
        for idx, (proc_count, proc_name, proc_definition) in enumerate(cursor):
            if idx == 0:
                step = 1.0 / (proc_count + 1e-10)
            grt.send_progress(idx * step, 'Reverse engineering procedure %s.%s' % (schema.name, proc_name))
            proc = grt.classes.db_mssql_Routine()
            proc.name = proc_name or ""
            proc.owner = schema
            proc.routineType = 'PROCEDURE'
            proc.sqlDefinition = proc_definition
            schema.routines.append(proc)
    grt.send_progress(1.0, 'Finished reverse engineering of procedures for the %s schema.' % schema.name)
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Schema)
def reverseEngineerFunctions(connection, schema):
    execute_query(connection, 'USE %s' % quoteIdentifier(schema.owner.name))  # catalog
    query_post_90 = """SELECT COUNT(*) OVER () AS count, OBJECT_NAME(M.object_id) as name, definition
FROM sys.sql_modules M JOIN sys.objects O ON M.object_id=O.object_id
WHERE type_desc LIKE '%%FUNCTION%%' AND schema_id = SCHEMA_ID('%(schema)s')"""
    query_pre_90 = """SELECT (SELECT COUNT(*)
FROM INFORMATION_SCHEMA.ROUTINES
WHERE ROUTINE_SCHEMA='%(schema)s' AND ROUTINE_TYPE='FUNCTION'), ROUTINE_NAME as name, ROUTINE_DEFINITION as definition
FROM INFORMATION_SCHEMA.ROUTINES
WHERE ROUTINE_SCHEMA='%(schema)s' AND ROUTINE_TYPE='FUNCTION'"""

    serverVersion = connected_server_version(connection)

    query = query_pre_90 if serverVersion.majorNumber < 9 else query_post_90

    cursor = execute_query(connection, query % {'schema':schema.name})
    step = 0.0
    if cursor:
        for idx, (func_count, func_name, func_definition) in enumerate(cursor):
            if idx == 0:
                step = 1.0 / (func_count + 1e-10)
            grt.send_progress(idx * step, 'Reverse engineering function %s.%s' % (schema.name, func_name))
            proc = grt.classes.db_mssql_Routine()
            proc.name = func_name or ""
            proc.owner = schema
            proc.routineType = 'FUNCTION'
            proc.sqlDefinition = func_definition
            schema.routines.append(proc)
    grt.send_progress(1.0, 'Finished reverse engineering of functions for the %s schema.' % schema.name)
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mssql_Schema)
def reverseEngineerTriggers(connection, schema):
    execute_query(connection, 'USE %s' % quoteIdentifier(schema.owner.name))  # catalog
    query_post_90 = """SELECT st.object_id,
    st.name AS trigger_name,
    OBJECT_NAME(st.parent_id) AS table_name,
    sm.definition as trigger_code,
    is_disabled, is_instead_of_trigger, STUFF((
          SELECT ';' + ste.type_desc
          FROM sys.trigger_events ste
          WHERE st.object_id = ste.object_id
          FOR XML PATH('')), 1, 1, '') as type_desc_detail
FROM sys.triggers st JOIN sys.sql_modules sm ON st.object_id = sm.object_id
WHERE st.parent_class = 1 AND OBJECT_SCHEMA_NAME(st.object_id) = '%s'
ORDER BY st.parent_id"""
    query_pre_90 = """SELECT so.id,
so.name AS trigger_name,
OBJECT_NAME(so.parent_obj) AS table_name,
sc.text AS sql_code,
OBJECTPROPERTY(so.id, 'ExecIsTriggerDisabled') AS is_disabled,
so.instrig as is_instead_of_trigger,
event = ( CASE WHEN OBJECTPROPERTY(so.id, 'ExecIsInsertTrigger')=1 THEN 'INSERT;' ELSE '' END +
    CASE WHEN OBJECTPROPERTY(so.id, 'ExecIsDeleteTrigger')=1 THEN 'DELETE;' ELSE '' END + 
    CASE WHEN OBJECTPROPERTY(so.id, 'ExecIsUpdateTrigger')=1 THEN 'UPDATE' ELSE '' END)
FROM sysobjects so LEFT JOIN syscomments sc ON so.id = sc.id LEFT JOIN sysusers su ON so.uid = su.uid
WHERE so.type = 'TR' AND su.name='%s'"""

    serverVersion = connected_server_version(connection)

    query = query_pre_90 if serverVersion.majorNumber <= 9 else query_post_90

    cursor = execute_query(connection, query % schema.name)
    if cursor:
        for table in schema.tables:
            table.triggers.remove_all()
        resultset = cursor.fetchall()
        step = 1.0 / (len(resultset) + 1e-10)
        trigger_table = None
        for idx, (trigger_id, trigger_name, table_name, trigger_code, is_disabled, is_instead_of_trigger, event) in enumerate(resultset):
            grt.send_progress(idx * step, 'Reverse engineering trigger %s.%s' % (schema.name, trigger_name))
            trigger = grt.classes.db_mssql_Trigger()
            trigger.name = trigger_name
            trigger.sqlDefinition = trigger_code
            trigger.enabled = not is_disabled
            if serverVersion.majorNumber < 9:
                trigger.event = event.strip(';')  # It would take values as 'INSERT;UPDATE'
#            else:
#                trigger.event = ';'.join(row[0] for row in execute_query(connection, 
#                                     'SELECT type_desc FROM sys.trigger_events WHERE sys.trigger_events.object_id = %d' % trigger_id))
#            trigger.orientation = 'ROW'  # TODO: This needs extra analysis
            trigger.timing = 'INSTEADOF' if is_instead_of_trigger else 'AFTER'  # NOTE: There's no equivalent for mysql BEFORE in mssql

            if not trigger_table or trigger_table.name != table_name:
                trigger_table = find_grt_object(table_name, schema.tables)

            if trigger_table:
                trigger.owner = trigger_table
                trigger_table.triggers.append(trigger)

    grt.send_progress(1.0, 'Finished reverse engineering of triggers for the %s schema.' % schema.name)
    return 0


@ModuleInfo.export(grt.classes.db_mgmt_Rdbms)
def initializeDBMSInfo():
    rdbms = grt.unserialize(ModuleInfo.moduleDataDirectory + "/mssql_rdbms_info.xml")
#    print "unserialized", rdbms
    grt.root.wb.rdbmsMgmt.rdbms.append(rdbms)
    return rdbms



@ModuleInfo.export((grt.LIST, grt.STRING))
def getDataSourceNames():
    result = grt.List(grt.STRING)
    import pyodbc
    sources = pyodbc.dataSources()
    for key, value in sources.items():
        result.append("%s|%s (%s)" % (key, key, value))
    return result

    
@ModuleInfo.export((grt.LIST, grt.STRING))
def getTDSProtocolVersionChoices():
    result = grt.List(grt.STRING)

    # not tested yet
    #result.append("7.0|Microsoft SQL Server 7")
    result.append("7.1|Microsoft SQL Server 2000 (7.1)")
    result.append("7.2|Microsoft SQL Server 2005 (7.2)")
    result.append("7.2|Microsoft SQL Server 2008 (7.2)")
    result.append("7.2|Microsoft SQL Server 2012 (7.2)")

    return result 

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def resetProgressFlags(connection):
    _connections[connection.__id__]['_rev_eng_progress_flags'] = []
    return 0

@ModuleInfo.export(grt.INT)
def cleanup():
    global _connections
    _connections = {}
    return 0

@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def getOS(connection):
    return 'windows'

@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def getSourceInstance(connection):
    return execute_query(connection, "SELECT @@servicename ").fetchone()[0]

