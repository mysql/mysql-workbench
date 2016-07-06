# Copyright (c) 2012, 2016 Oracle and/or its affiliates. All rights reserved.
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
from workbench.utils import find_object_with_name


ModuleInfo = DefineModule(name= "DbSybaseRE", author= "Oracle Corp.", version="1.0")

######################### Non exposed functions and variables #################

def check_interruption():
    """Checks if the user is requesting to cancel an operation in progress.
    
    Call this from time to time so that the actual cancel requests can be handled
    """
    if grt.query_status():
        raise grt.UserInterrupt()


def get_sybase_rdbms_instance():
    sybase_rdbms_instance = None
    for rdbms in grt.root.wb.rdbmsMgmt.rdbms:
        if rdbms.name == 'Sybase':
            sybase_rdbms_instance = rdbms
            break
    return sybase_rdbms_instance


_connections = {}


def get_connection(connection_object):
    if connection_object.__id__ in _connections:
        return _connections[connection_object.__id__]["connection"]
    else:
        raise NotConnectedError("No open connection to %s" % connection_object.hostIdentifier)


def execute_query(connection_object, query, *args, **kwargs):
    """Retrieves a connection and executes the given query returning a cursor to iterate over results.

    The remaining positional and keyword arguments are passed with the query to the execute function
    """
    grt.log_debug3("db.sybase", "execute %s %s %s\n" % (query, args, kwargs))
    return get_connection(connection_object).cursor().execute(query, *args, **kwargs)


# There's no real use for this at the moment, but may be needed later
def allow_ddl_in_tran(func):
    """(Decorator). Needed because a Sybase issue with DDL statements inside transactions.

    More info here: http://manuals.sybase.com/onlinebooks/group-as/asg1250e/sqlug/@Generic__BookTextView/53037
    """
    def wrapper_function(*args, **kwargs):
        conn = None
        # Find the connection in the function params:
        for arg in args:
            if isinstance(arg,  grt.classes.db_mgmt_Connection):
                conn = arg
                break
        if not conn:
            grt.log_error('db.sybase', 'Cannot find a connection object to apply the allow-ddl-in-tran fix')
            return func
        cursor = get_connection(conn).cursor()
        try:
            current_db = cursor.execute('SELECT db_name()').fetchone()[0]  # Will restore it later
        except Exception:
            current_db = 'master'
        cursor.execute('USE master')  # This is required for the next query to work
        cursor.execute('sp_dboption tempdb,"ddl in tran", true')
        cursor.execute('CHECKPOINT tempdb')  # Like FLUSH in mysql for options
        if current_db != 'master':
            cursor.execute('USE ?', current_db)
        del cursor  # Needed to use just one connection to the DB (Sybase Developer Edition allows only one connection)
        res = func(*args, **kwargs)
        # Once the function is executed, restore False to 'ddl in tran':
        cursor = get_connection(conn).cursor()
        cursor.execute('USE master')  # This is required for the next query to work
        cursor.execute('sp_dboption tempdb,"ddl in tran", false')
        cursor.execute('CHECKPOINT tempdb')  # Like FLUSH in mysql for options
        if current_db != 'master':
            cursor.execute('USE ?', current_db)
        # Restore the originally active database
        return res
    return wrapper_function

def join_multiline_content(name_column, split_column, cursor, callback=lambda _:None):
    """Generator to join definition columns that are split across several rows

    In Sybase, object definitions in syscomments can be split into several rows. For example,
    a stored procedure can have several row entries in syscomments each having the same object
    name and containing the chunks of the procedure's code in the "text" column. The order of
    the rows is determined by the "colid" value (a sequence of integers: 1, 2, 3, etc.).

    Arguments: name_column  -- string with the name of the column that has the object name
               split_column -- string with the name of the column that has the fragments
               cursor       -- the resultset to iterate through
               callback     -- an optional callable that will be called with the row corresponding
                               to the first row of each distinct object

    Returns: idx, name, definition where idx tracks the count of different objects as they are
    found, name is the name of the current object and definition is the joint definition for the
    object.

    Note: This functions assumes that the rows are ordered properly, i.e. sorted by name, colid.
    """
    idx = 0
    current_object = None
    current_object_chunks = []
    column_index = dict( (col_description[0], pos) for pos, col_description in enumerate(cursor.description) )
    for row in cursor:
        object_name, object_definition = row[column_index[name_column]], row[column_index[split_column]]
        if not current_object or object_name != current_object:
            callback(row)
            if current_object:
                yield idx, current_object, ''.join(current_object_chunks)
            current_object = object_name
            current_object_chunks = [object_definition]
            idx += 1
            continue
        current_object_chunks.append(object_definition)
    # Add the code of the last object:
    if current_object:
        yield idx, current_object, ''.join(current_object_chunks)

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
    host_identifier = connection.hostIdentifier
    try:
        con = get_connection(connection)
        try:
            if not con.cursor().execute('SELECT 1'):
                raise Exception("connection error")
        except Exception, exc:
            grt.send_info("Connection to %s apparently lost, reconnecting..." % connection.hostIdentifier)
            raise NotConnectedError("Connection error")
    except NotConnectedError, exc:
        grt.send_info("Connecting to %s..." % host_identifier)
        import pyodbc
        try:
            con = db_driver.connect(connection, password)
            # Sybase metadata query SPs use things that don't work inside transactions, so enable autocommit
            con.autocommit = True

            # Adds data type conversion functions for pyodbc
#            if connection.driver.driverLibraryName == 'pyodbc':
#                cursor = con.cursor()
#                version = con.execute("SELECT CAST(SERVERPROPERTY('ProductVersion') AS VARCHAR)").fetchone()[0]
#                majorVersion = int(version.split('.', 1)[0])
#                if majorVersion >= 9:
#                    con.add_output_converter(-150, lambda value: value if value is None else value.decode('utf-16'))
#                    con.add_output_converter(0, lambda value: value if value is None else value.decode('utf-16'))
#                else:
#                    con.add_output_converter(-150, lambda value: value if value is None else str(value))
#                    con.add_output_converter(0, lambda value: value if value is None else str(value))

        except pyodbc.Error, odbc_err:
            # 28000 is from native SQL Server driver... 42000 seems to be from FreeTDS
            # FIXME: This should be tuned for Sybase
            if len(odbc_err.args) == 2 and odbc_err.args[0] in ('28000', '42000') and "(18456)" in odbc_err.args[1]:
                raise grt.DBLoginError(odbc_err.args[1])

        if not con:
            grt.send_error('Connection failed', str(exc))
            raise
        
        _connections[connection.__id__] = {"connection" : con }
        _connections[connection.__id__]["version"] = getServerVersion(connection)
        version  = execute_query(connection, "SELECT @@version").fetchone()[0]
        grt.send_info("Connected to %s, %s", (host_identifier, version))
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
    return 'Sybase'

@ModuleInfo.export(grt.classes.GrtVersion, grt.classes.db_mgmt_Connection)
def getServerVersion(connection):
    """Returns a GrtVersion instance containing information about the server version."""
    version = grt.classes.GrtVersion()
    ver_string = execute_query(connection, "SELECT @@version").fetchone()[0]
    try:
        ver_string = ver_string.split('/', 2)[1]
    except IndexError:
        ver_string = '15'
    ver_parts = [ int(part) for part in ver_string.split('.') ] + 4*[ 0 ]
    version.majorNumber, version.minorNumber, version.releaseNumber, version.buildNumber = ver_parts[:4]
    return version


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection)
def getCatalogNames(connection):
    """Returns a list of the available catalogs.

    [NOTE] A catalog is equivalent to a databases in Sybase.
    """
    query = 'exec sp_databases'
    return [ row[0] for row in execute_query(connection, query) ]


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING)
def getSchemaNames(connection, catalog_name):
    """Returns a list of schemata for the given connection object."""

    execute_query(connection, 'USE %s' % quoteIdentifier(catalog_name))
    return sorted(set(row[1] for row in execute_query(connection, 'sp_tables')))


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTableNames(connection, catalog_name, schema_name):
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog_name))
    return [row[2] for row in execute_query(connection, '''sp_tables @table_type="'TABLE'"''') if row[1] == schema_name] # Use "'TABLE' 'SYSTEM TABLE'" for all tables


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getViewNames(connection, catalog_name, schema_name):
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog_name))
    return [row[2] for row in execute_query(connection, '''sp_tables @table_type="'VIEW'"''') if row[1] == schema_name]

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTriggerNames(connection, catalog_name, schema_name):
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog_name))
    query = """SELECT name
FROM sysobjects
WHERE type='TR' AND uid = user_id(?)"""
    return [ row[0] for row in execute_query(connection, query, schema_name) ]


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getProcedureNames(connection, catalog_name, schema_name):
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog_name))
    query = """SELECT name
FROM sysobjects
WHERE type='P' AND uid = user_id(?)"""
    return [ row[0] for row in execute_query(connection, query, schema_name) ]


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getFunctionNames(connection, catalog_name, schema_name):
    return []  # TODO: Figure out a way to tell apart function from procedures


#########  Reverse Engineering functions #########

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING, (grt.LIST, grt.STRING), grt.DICT)
def reverseEngineer(connection, catalog_name, schemata_list, options):
    """Reverse engineers a Sybase ASE database.

    This is the function that will be called by the Migration Wizard to reverse engineer
    a Sybase database. All the other reverseEngineer* functions are not actually required
    and should not be considered part of this module API even though they are currently
    being exposed. This function calls the other reverseEngineer* functions to complete
    the full reverse engineer process.
    """
    grt.send_progress(0, "Reverse engineering catalog information")
    catalog = grt.classes.db_sybase_Catalog()
    catalog.name = catalog_name
    catalog.simpleDatatypes.remove_all()
    catalog.simpleDatatypes.extend(connection.driver.owner.simpleDatatypes)
    catalog.defaultCollationName = '' #   FIXME: Find out the right collation for the catalog
    
    grt.send_progress(0.05, "Reverse engineering User Data Types...")
    check_interruption()  #
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

        schema = grt.classes.db_sybase_Schema()
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
            # Remove previous first pass marks that may exist if the user goes back and attempt rev eng again:
            progress_flags = _connections[connection.__id__].setdefault('_rev_eng_progress_flags', set())
            progress_flags.discard('%s_tables_first_pass' % schema_name)
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


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_sybase_Catalog)
def reverseEngineerUserDatatypes(connection, catalog):
    base_types = dict( (
            (34,'IMAGE'),
            (35,'TEXT'),
            (36,'EXTENDED TYPE'),
            (37,'TIMESTAMP'),
            (38,'INTN'),
            (39,'VARCHAR'),
            (45,'BINARY'),
            (47,'CHAR'),
            (48,'TINYINT'),
            (49,'DATE'),
            (50,'BIT'),
            (51,'TIME'),
            (52,'SMALLINT'),
            (55,'DECIMAL'),
            (56,'INT'),
            (58,'SMALLDATETIME'),
            (59,'REAL'),
            (60,'MONEY'),
            (61,'DATETIME'),
            (62,'FLOAT'),
            (63,'NUMERIC'),
            (65,'USMALLINT'),
            (66,'UINT'),
            (67,'UBIGINT'),
            (68,'UINTN'),
            (106,'DECIMALN'),
            (108,'NUMERICN'),
            (109,'FLOATN'),
            (110,'MONEYN'),
            (111,'DATETIMN'),
            (122,'SMALLMONEY'),
            (123,'DATEN'),
            (135,'UNICHAR'),
            (147,'TIMEN'),
            (155,'UNIVARCHAR'),
            (169,'TEXT_LOCATOR'),
            (170,'IMAGE_LOCATOR'),
            (171,'UNITEXT_LOCATOR'),
            (174,'UNITEXT'),
            (187,'BIGDATETIMEN'),
            (188,'BIGTIMEN'),
            (189,'BIGDATETIME'),
            (190,'BIGTIME'),
            (191,'BIGINT'),
        )  )   

    query = """SELECT name, length, prec, scale, allownulls, type
FROM systypes
WHERE accessrule != NULL"""

    execute_query(connection, 'USE %s' % quoteIdentifier(catalog.name))
    sybase_rdbms_instance = get_sybase_rdbms_instance()
    catalog.userDatatypes.remove_all()
    for name, length, precision, scale, is_nullable, base_type in execute_query(connection, query):
        datatype = grt.classes.db_sybase_UserDatatype()
        datatype.name = name.upper()
        datatype.characterMaximumLength = length
        datatype.numericPrecision = precision
        datatype.numericScale = scale
        datatype.isNullable = is_nullable
        simple_type = find_object_with_name(sybase_rdbms_instance.simpleDatatypes, base_types[base_type])
        if simple_type:
            datatype.actualType = simple_type
        else:
            grt.send_warning('Sybase reverseEngineerUserDatatypes', 'Could not found base type "%s" for user defined type "%s"' % (base_type, name) )
        catalog.userDatatypes.append(datatype)
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_sybase_Schema)
def reverseEngineerTables(connection, schema):
    # Since there are some reverse engineering stages that requires all table names and table columns
    # in the database to be set, these should be done after a first pass that rev engs their requirements
    progress_flags = _connections[connection.__id__].setdefault('_rev_eng_progress_flags', set())
    is_first_pass = not ('%s_tables_first_pass' % schema.name) in progress_flags

    if is_first_pass:
        catalog = schema.owner
        execute_query(connection, 'USE %s' % quoteIdentifier(catalog.name))
    
        schema.tables.remove_all()
        # TODO: Add real create table comments instead of empty string in the following line:
        table_names = [(table_name, '') for table_name in getTableNames(connection, catalog.name, schema.name) ]
        total = len(table_names) + 1e-10
        i = 0.0
        for table_name, table_comment in table_names:
            grt.send_progress(i / total, 'Retrieving table %s.%s...' % (schema.name, table_name))
            table = grt.classes.db_sybase_Table()
            table.name = table_name
            schema.tables.append(table)
            table.owner = schema
            table.comment = table_comment or ''  # table_comment can be None

            reverseEngineerTableColumns(connection, table)
            reverseEngineerTablePK(connection, table)
            reverseEngineerTableIndices(connection, table)
    
            i += 1.0
        progress_flags.add('%s_tables_first_pass' % schema.name)
    else:  # Second pass
        i = 1.0
        total = len(schema.tables) + 1e-10
        for table in schema.tables:
            reverseEngineerTableFKs(connection, table)
            grt.send_progress(i / total, 'Reverse engineering of foreign keys in table %s.%s completed' % (schema.name, table.name))
            i += 1.0

    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_sybase_Table)
def reverseEngineerTableColumns(connection, table):
    schema = table.owner
    catalog = schema.owner
    execute_query(connection, 'USE %s' % quoteIdentifier(catalog.name))
    query = """SELECT  ISNULL(C.name, '') AS COLUMN_NAME, T.name AS DATA_TYPE,
        C.length AS CHARACTER_MAXIMUM_LENGTH, C.prec AS NUMERIC_PRECISION,
        C.scale AS NUMERIC_SCALE, CONVERT(BIT, (C.status & 0x08)) AS IS_NULLABLE,
        CONVERT(BIT, (C.status & 0x80)) AS IS_IDENTITY_COLUMN, K.text AS COLUMN_DEFAULT
        FROM syscolumns C, systypes T, sysobjects A, syscomments K
        WHERE USER_NAME(A.uid) = ? AND
        A.id = C.id AND C.id = OBJECT_ID(?) AND
        C.usertype *= T.usertype AND
        C.cdefault *= K.id
        ORDER BY C.colid"""
    
    rows = execute_query(connection, query, schema.name, table.name)

    sybase_rdbms_instance = get_sybase_rdbms_instance()
    sybase_simple_datatypes_list = [ datatype.name for datatype in sybase_rdbms_instance.simpleDatatypes ]
    user_datatypes_list = [ datatype.name for datatype in catalog.userDatatypes ]

    col_names = [ col_description[0] for col_description in rows.description ]
    for row in rows:
        row_values = dict( nameval for nameval in zip(col_names, row) )
        column = grt.classes.db_sybase_Column()
        column.name = row_values['COLUMN_NAME'] or ''
        column.isNotNull = not row_values['IS_NULLABLE']
        column.collationName = row_values.get('COLLATION_NAME', '')  # TODO: find a way to get the column's collation
        column.length = row_values['CHARACTER_MAXIMUM_LENGTH'] or 0
        column.precision = row_values['NUMERIC_PRECISION'] if row_values['NUMERIC_PRECISION'] is not None else -1
        column.scale = row_values['NUMERIC_SCALE'] if row_values['NUMERIC_SCALE'] is not None else -1
        column.comment = row_values.get('COLUMN_COMMENT', '')  # TODO: find a way to get the column's comments
        column.identity = row_values['IS_IDENTITY_COLUMN'] or 0
        user_datatype = None
        try:
            datatype = sybase_simple_datatypes_list.index( row_values['DATA_TYPE'].upper() )
        except ValueError:
            try:
                user_datatype = catalog.userDatatypes[user_datatypes_list.index( row_values['DATA_TYPE'].upper() )]
            except (ValueError, TypeError):
                user_datatype = None
                datatype = sybase_simple_datatypes_list.index('VARCHAR')
                column.length = 255
                msg = 'Column datatype "%s" for column "%s" in table "%s.%s" reverse engineered as VARCHAR(255)' % (row_values['DATA_TYPE'].upper(), column.name, schema.name, table.name)
                grt.send_warning('Sybase reverseEngineerTableColumns', msg)
            else:
                datatype = None
                column.userType = user_datatype

        if datatype is not None:
            column.simpleType = sybase_rdbms_instance.simpleDatatypes[datatype]

        default_value = row_values['COLUMN_DEFAULT']
        
        if default_value is not None and default_value.startswith('DEFAULT '):
            column.defaultValue = default_value[8:]

        table.addColumn(column)

        # TODO: charset name

    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_sybase_Table)
def reverseEngineerTablePK(connection, table):
    """Reverse engineers the primary key for the given table."""

    schema = table.owner
    catalog = schema.owner
    execute_query(connection, 'USE %s' % catalog.name)
    query ="""SELECT sc.name
FROM sysobjects so JOIN syskeys sk ON so.id=sk.id
     JOIN syscolumns sc ON sc.id=sk.id AND sc.colid IN (sk.key1, sk.key2, sk.key3, sk.key4, sk.key5, sk.key6, sk.key7, sk.key8)
WHERE so.uid=USER_ID(?) AND sk.id=OBJECT_ID(?) AND sk.type=1"""

    if len(table.columns) == 0:
        grt.send_error('Sybase reverseEngineerTablePK', "Reverse engineer of table's %s.%s primary key was attempted but the table has no columns attribute" % (schema.name, table.name) )
        return 1    # Table must have columns reverse engineered before we can rev eng its primary key

    pk_col_names = [ row[0] for row in execute_query(connection, query, schema.name, table.name) ]

    for pk_column in pk_col_names:
        column = find_object_with_name(table.columns, pk_column)
        if column:
            table.addPrimaryKeyColumn(column)
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_sybase_Table)
def reverseEngineerTableIndices(connection, table):
    """Reverse engineers the indices for the given table."""

    schema = table.owner
    catalog = schema.owner
    
    if len(table.columns) == 0:
        grt.send_error('Sybase reverseEngineerTableIndices', 'Reverse engineer of table %s.%s was attempted but the table has no columns attribute' % (schema.name, table.name) )
        return 1    # Table must have columns reverse engineered before we can rev eng its indices

    execute_query(connection, 'USE %s' % catalog.name)

    query = """SELECT INDEX_NAME = A.name,
IS_CLUSTERED = CASE
                WHEN ((A.status&16) = 16 OR (A.status2&512) = 512) THEN 1
                ELSE 0
               END,
IS_PRIMARY = CASE
                WHEN ((A.status&0x800) = 0x800) THEN 1
                ELSE 0
             END,
IS_UNIQUE = CASE
                WHEN ((A.status&2) = 2) THEN 1
                ELSE 0
            END,
IGNORE_DUP = CASE
                WHEN ((A.status&4) = 4) THEN 1
                ELSE 0
             END,
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 1),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 2),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 3),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 4),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 5),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 6),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 7),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 8),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 9),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 10),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 11),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 12),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 13),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 14),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 15),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 16),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 17),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 18),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 19),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 20),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 21),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 22),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 23),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 24),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 25),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 26),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 27),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 28),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 29),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 30),
INDEX_COL(USER_NAME(B.uid) + '.' + B.name, indid, 31)
FROM sysindexes A,  sysobjects B
WHERE A.indid > 0 AND A.indid < 255 AND A.status2 & 2 != 2 AND
B.id = A.id AND B.type = 'U' AND
USER_NAME(B.uid) = ? AND B.name=? ORDER BY 1, 2, 3"""

    for index_row in execute_query(connection, query, schema.name, table.name):
        index = grt.classes.db_sybase_Index()
        index.name = index_row[0]
        index.clustered = index_row[1]
        index.isPrimary = index_row[2]
        index.unique = index_row[3]
        index.indexType = 'UNIQUE' if index.unique else 'INDEX'
        index.ignoreDuplicateRows = index_row[4]
        table.addIndex(index)

        # Get the columns for the index:
        index_column_names = [colname for colname in index_row[5:] if colname is not None]
        for column_name in index_column_names:
            column = find_object_with_name(table.columns, column_name)
            if column:
                index_column = grt.classes.db_sybase_IndexColumn()
                index_column.name = index.name + '.' + column_name
                index_column.referencedColumn = column
                index.columns.append(index_column)
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_sybase_Table)
def reverseEngineerTableFKs(connection, table):
    """Reverse engineers the foreign keys for the given table."""

    schema = table.owner
    catalog = schema.owner

    execute_query(connection, 'USE %s' % catalog.name)

    query = """SELECT so.name, USER_NAME(so.uid),
COL_NAME(sk.id, key1),
COL_NAME(sk.id, key2),
COL_NAME(sk.id, key3),
COL_NAME(sk.id, key4),
COL_NAME(sk.id, key5),
COL_NAME(sk.id, key6),
COL_NAME(sk.id, key7),
COL_NAME(sk.id, key8),
COL_NAME(sk.depid, depkey1),
COL_NAME(sk.depid, depkey2),
COL_NAME(sk.depid, depkey3),
COL_NAME(sk.depid, depkey4),
COL_NAME(sk.depid, depkey5),
COL_NAME(sk.depid, depkey6),
COL_NAME(sk.depid, depkey7),
COL_NAME(sk.depid, depkey8)
FROM syskeys sk JOIN sysobjects so ON sk.depid = so.id
WHERE sk.type = 2 AND sk.id = OBJECT_ID('%s.%s')""" % (schema.name, table.name)

    if len(table.columns) == 0:
        grt.send_error('Sybase reverseEngineerTableFKs', 'Reverse engineer of foreign keys for table %s.%s was attempted but the table has no columns attribute' % (schema.name, table.name) )
        return 1    # Table must have columns reverse engineered before we can rev eng its foreign keys

    table.foreignKeys.remove_all()
    for row in execute_query(connection, query):
        fk_columns = [col_name for col_name in row[2:10] if col_name]
        fk_ref_columns = [col_name for col_name in row[10:] if col_name]
        foreign_key = grt.classes.db_sybase_ForeignKey()
        foreign_key.name = '%s_%s_%s_fk' % (schema.name, table.name, '_'.join(fk_columns))
        foreign_key.owner = table
        foreign_key.deleteRule = foreign_key.updateRule = 'RESTRICT'
        foreign_key.modelOnly = 0
        referenced_schema = find_object_with_name(catalog.schemata, row[1])
        if not referenced_schema:
            grt.send_error('Sybase reverseEngineerTableFKs', 'Could not find schema "%s" in catalog "%s"' %
                           (schema.name, catalog.name) )
            return 1

        foreign_key.referencedTable = find_object_with_name(referenced_schema.tables, row[0])
        if not foreign_key.referencedTable:
            grt.send_error('Sybase reverseEngineerTableFKs', 'Could not find referenced table "%s" in schema "%s"' %
                           (row[0], schema.name) )
            return 1

        for column_name, referenced_column_name in zip(fk_columns, fk_ref_columns):
            column = find_object_with_name(table.columns, column_name)
            if not column:
                grt.send_error('Sybase reverseEngineerTableFKs', 'Could not find column "%s" in table "%s.%s"' %
                               (column_name, schema.name, table.name) )
                return 1
            referenced_column = find_object_with_name(foreign_key.referencedTable.columns, referenced_column_name)
            if not referenced_column:
                grt.send_error('Sybase reverseEngineerTableFKs', 'Could not find column "%s" in table "%s.%s"' %
                               (referenced_column_name, referenced_schema.name, foreign_key.referencedTable.name) )
                return 1
            foreign_key.columns.append(column)
            foreign_key.referencedColumns.append(referenced_column)

        table.foreignKeys.append(foreign_key)
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_sybase_Schema)
def reverseEngineerViews(connection, schema):
    execute_query(connection, 'USE %s' % schema.owner.name)  # catalog

    query = """SELECT so.name AS view_name, sc.text AS view_definition
FROM sysobjects so JOIN syscomments sc on so.id=sc.id
WHERE so.uid=USER_ID(?) AND so.type='V'
ORDER BY so.name, sc.colid""" 

    schema.views.remove_all()
    step = 1.0 / (len(getViewNames(connection, schema.owner.name, schema.name)) + 1e-10)
    cursor = execute_query(connection, query, schema.name)
    if cursor:
        for idx, view_name, view_definition in join_multiline_content('view_name', 'view_definition', cursor):
            grt.send_progress(idx * step, 'Reverse engineering view %s.%s' % (schema.name, view_name))
            view = grt.classes.db_sybase_View()
            view.owner = schema
            view.name = view_name or ''
            view.sqlDefinition = view_definition
            schema.views.append(view)
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_sybase_Schema)
def reverseEngineerProcedures(connection, schema):
    execute_query(connection, 'USE %s' % schema.owner.name)  # catalog

    query = """SELECT so.name AS procedure_name, sc.text as procedure_definition
FROM sysobjects so INNER JOIN syscomments sc ON so.id=sc.id
WHERE so.uid = USER_ID(?) AND so.type = 'P'
ORDER BY so.name, sc.colid""" 

    step = 1.0 / (len(getProcedureNames(connection, schema.owner.name, schema.name)) + 1e-10)
    cursor = execute_query(connection, query, schema.name)
    if cursor:
        for idx, procedure_name, procedure_definition in join_multiline_content('procedure_name', 'procedure_definition', cursor):
            grt.send_progress(idx * step, 'Reverse engineering procedure %s.%s' % (schema.name, procedure_name))
            procedure = grt.classes.db_sybase_Routine()
            procedure.owner = schema
            procedure.name = procedure_name or ''
            procedure.routineType = 'PROCEDURE'
            procedure.sqlDefinition = procedure_definition
            schema.routines.append(procedure)
    grt.send_progress(1.0, 'Finished reverse engineering of procedures for the %s schema.' % schema.name)
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_sybase_Schema)
def reverseEngineerFunctions(connection, schema):
    # TODO: Find a way to reverse engineer functions in Sybase ASE
    grt.send_progress(1.0, 'Finished reverse engineering of functions for the %s schema.' % schema.name)
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_sybase_Schema)
def reverseEngineerTriggers(connection, schema):
    execute_query(connection, 'USE %s' % schema.owner.name)  # catalog

    tables_with_triggers_query = """SELECT name, deltrig, instrig, updtrig
FROM sysobjects
WHERE uid = USER_ID(?) AND type='U'
AND(deltrig != 0 OR instrig != 0 OR updtrig != 0)"""

    trigger_details_query = """SELECT so.name AS trigger_name, sc.id AS trigger_id, sc.text AS trigger_definition
FROM syscomments sc JOIN sysobjects so ON sc.id=so.id
WHERE sc.id IN (%s)
ORDER BY so.name, sc.colid"""

    triggers = {}
    for row in execute_query(connection, tables_with_triggers_query, schema.name):
        if row[1] != 0:
            triggers.setdefault(row[1], [row[0], ''])[1] += ';DELETE'
        if row[2] != 0:
            triggers.setdefault(row[2], [row[0], ''])[1] += ';INSERT'
        if row[3] != 0:
            triggers.setdefault(row[3], [row[0], ''])[1] += ';UPDATE'

    step = 1.0 / (len(getTriggerNames(connection, schema.owner.name, schema.name)) + 1e-10)
    all_triggers = execute_query(connection, trigger_details_query % ', '.join(str(trig_id) for trig_id in triggers)) if triggers else None
    trigger_name2id = {}
    def register_trigger_name(row):
        trigger_name2id[row[0]] = row[1]
    if all_triggers:
        for idx, trigger_name, trigger_definition in join_multiline_content('trigger_name', 'trigger_definition',
                                                                            all_triggers, register_trigger_name):
            grt.send_progress(idx * step, 'Reverse engineering trigger %s.%s' % (schema.name, trigger_name))
            trigger = grt.classes.db_sybase_Trigger()
            trigger.name = trigger_name or ''
            trigger.sqlDefinition = trigger_definition
            trigger.timing = 'AFTER'  # All Sybase ASE triggers are fired after the data is changed
#            trigger.orientation = 'ROW'  # TODO: This needs extra analysis
            trigger.enabled = 1  # TODO: Figure out how to tell the actual value
            trigger_table, trigger_events = triggers[trigger_name2id[trigger_name]]
            trigger.event = trigger_events.strip(';')  # It would take values as 'INSERT;UPDATE'
            trigger.owner = find_object_with_name(schema.tables, trigger_table)

            if trigger.owner:
                trigger.owner.triggers.append(trigger)
            else:
                grt.send_warning('Sybase reverseEngineerTriggers', 'Parent table not found for trigger "%s"' % trigger.name)

    grt.send_progress(1.0, 'Finished reverse engineering of triggers for the %s schema.' % schema.name)
    return 0


@ModuleInfo.export(grt.classes.db_mgmt_Rdbms)
def initializeDBMSInfo():
    rdbms = grt.unserialize(ModuleInfo.moduleDataDirectory + "/sybase_rdbms_info.xml")
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
    result.append("4.1|Sybase Adaptive Server Enterprise 15 (4.1)")

    return result 


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def resetProgressFlags(connection):
    _connections[connection.__id__]['_rev_eng_progress_flags'] = []
    return 0