# Copyright (c) 2012, 2019, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

import random
import os.path

from wb import DefineModule
from workbench import db_driver
from workbench.utils import find_object_with_name
from workbench.exceptions import NotConnectedError

from db_generic_re_grt import GenericReverseEngineering

import SQLEXT as constant

import grt


ModuleInfo = DefineModule(name= "DbMsAccessRE", author= "Oracle Corp.", version="1.0")


def normalize_schema_name(path):
    """Turns an Access DB file path into something resembling a schema name"""
    return os.path.splitext(os.path.basename(path))[0]


class MsAccessReverseEngineering(GenericReverseEngineering):
    _connections = {}


    @classmethod
    def check_interruption(cls):
        if grt.query_status():
            raise grt.UserInterrupt()


    @classmethod
    def find_datatype_object(cls, catalog, datatype_name):
        ''' Finds the datatype object corresponding to the given datatype name.

        Returns: a tuple of the form (is_simple_datatype, datatype) where:
            is_simple_datatype: True if the datatype was found among the simple datatypes for
                                its corresponding RDBMS
            datatype:           The actual datatype object. None if not found
        '''
        simple_types = cls._rdbms.simpleDatatypes
        user_types = catalog.userDatatypes
        for simple_type in simple_types:
            if datatype_name == simple_type.name or datatype_name in simple_type.synonyms:
                return (True, simple_type)
        for user_type in user_types:
            if datatype_name == user_type.name:
                return (False, user_type)
        return (False, None)

    @classmethod
    def get_connection(cls, connection_object):
        if connection_object.__id__ in cls._connections:
            return cls._connections[connection_object.__id__]["connection"]
        else:
            raise NotConnectedError("No open connection to %s" % connection_object.hostIdentifier)

    # Note: try to avoid executing SQL code within this module
    @classmethod
    def execute_query(cls, connection_object, query, *args, **kwargs):
        """Retrieves a connection and executes the given query returning a cursor to iterate over results.

        The remaining positional and keyword arguments are passed with the query to the execute function
        """
        return cls.get_connection(connection_object).cursor().execute(query, *args, **kwargs)


    @classmethod
    def initializeDBMSInfo(cls, xml_data_path):
        cls._rdbms = grt.unserialize(os.path.join(ModuleInfo.moduleDataDirectory, xml_data_path))
        grt.root.wb.rdbmsMgmt.rdbms.append(cls._rdbms)
        return cls._rdbms

    
    @classmethod
    def getDataSourceNames(cls):
        result = grt.List(grt.STRING)
        import pyodbc
        sources = pyodbc.dataSources()
        for key, value in list(sources.items()):
            result.append("%s|%s (%s)" % (key, key, value))
        return result


    @classmethod
    def getSupportedObjectTypes(cls):
        return [("tables", "db.Table", "Tables"), 
               ]


    @classmethod
    def quoteIdentifier(cls, name):
        return '"%s"' % name.replace('"', '\"')

    @classmethod
    def fullyQualifiedObjectName(cls, obj):
        owner = obj.owner
        if owner and isinstance(owner, grt.classes.db_Schema):
            return cls.quoteIdentifier(owner.name)+"."+cls.quoteIdentifier(obj.name)
        return cls.quoteIdentifier(obj.name)


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
            password:    a string with the password to use for the connection.
        '''
        try:
            con = cls.get_connection(connection)
            try:
                if not con.cursor().execute('SELECT 1'):
                    raise Exception("connection error")
            except Exception as exc:
                grt.send_info("Connection to %s apparently lost, reconnecting..." % connection.hostIdentifier)
                raise NotConnectedError("Connection error")
        except NotConnectedError as exc:
            grt.send_info("Connecting to %s..." % connection.hostIdentifier)
            con = db_driver.connect(connection, password)
            if not con:
                grt.send_error('Connection failed', str(exc))
                raise
            grt.send_info("Connected")
            cls._connections[connection.__id__] = {"connection": con}
        return 1


    @classmethod
    def disconnect(cls, connection):
        if connection.__id__ in cls._connections:
            cls._connections[connection.__id__]['connection'].close()
            del cls._connections[connection.__id__]
        return 0

    @classmethod
    def isConnected(cls, connection):
        return 1 if connection.__id__ in cls._connections else 0


    #########  Exploratory functions (these only return useful info without reverse engineering) #########

        
    @classmethod
    def getDriverDBMSName(cls, connection):
        if connection.driver.driverLibraryName != 'pyodbc':
            return ''
        import pyodbc
        return cls.get_connection(connection).getinfo(pyodbc.SQL_DBMS_NAME)
    
    @classmethod
    def getTargetDBMSName(cls):
        return 'MsAccess'

    @classmethod
    def getServerVersion(cls, connection):
        """Returns a GrtVersion instance containing information about the server version."""
        
        # Note: Not implemented. This returns a predefined default server version for compatibility sake.
        version = grt.classes.GrtVersion()
        version.majorNumber, version.minorNumber, version.releaseNumber, version.buildNumber = 1, 0, 0, 0
        return version

    @classmethod
    def getCatalogNames(cls, connection):
        return ["def"]

    @classmethod
    def getSchemaNames(cls, connection, catalog_name):
        """Returns a list of the available schemas.

        [NOTE] This will in fact return the name of the database file we are connected to.
        """
        res = list(set(normalize_schema_name(row[0]) for row in cls.get_connection(connection).cursor().tables())) 
        assert len(res) == 1
        return res


    @classmethod
    def getTableNames(cls, connection, catalog_name, schema_name):
        return list(set(row.table_name for row in 
                    cls.get_connection(connection).cursor().tables() if row.table_type=='TABLE'))


    @classmethod
    def getViewNames(cls, connection, catalog_name, schema_name):
        return list(set(row.table_name for row in 
                    cls.get_connection(connection).cursor().tables() if row.table_type=='VIEW'))


    @classmethod
    def getTriggerNames(cls, connection, catalog_name, schema_name):
        return []


    @classmethod
    def getProcedureNames(cls, connection, catalog_name, schema_name):
        return list(set(row.procedure_name for row in 
                    cls.get_connection(connection).cursor().procedures() ))


    @classmethod
    def getFunctionNames(cls, connection, catalog_name, schema_name):
        return []


    #########  Reverse Engineering functions #########

    @classmethod
    def reverseEngineer(cls, connection, catalog_name, schemata_list, context):
        grt.send_progress(0, "Reverse engineering catalog information")
        cls.check_interruption()
        catalog = cls.reverseEngineerCatalog(connection, catalog_name)

        # calculate total workload 1st
        grt.send_progress(0.1, 'Preparing...')
        table_count_per_schema = {}
        view_count_per_schema = {}
        routine_count_per_schema = {}
        trigger_count_per_schema = {}
        total_count_per_schema = {}

        get_tables = context.get("reverseEngineerTables", True)
        get_triggers = context.get("reverseEngineerTriggers", True)
        get_views = context.get("reverseEngineerViews", True)
        get_routines = context.get("reverseEngineerRoutines", True)

        # 10% of the progress is for preparation
        total = 1e-10  # total should not be zero to avoid DivisionByZero exceptions
        i = 0.0
        accumulated_progress = 0.1
        for schema_name in schemata_list:
            cls.check_interruption()
            table_count_per_schema[schema_name] = len(cls.getTableNames(connection, catalog_name, schema_name)) if get_tables else 0
            view_count_per_schema[schema_name] = len(cls.getViewNames(connection, catalog_name, schema_name)) if get_views else 0
            cls.check_interruption()
            routine_count_per_schema[schema_name] = len(cls.getProcedureNames(connection, catalog_name, schema_name)) + len(cls.getFunctionNames(connection, catalog_name, schema_name)) if get_routines else 0
            trigger_count_per_schema[schema_name] = len(cls.getTriggerNames(connection, catalog_name, schema_name)) if get_triggers else 0

            total_count_per_schema[schema_name] = (table_count_per_schema[schema_name] + view_count_per_schema[schema_name] +
                                                   routine_count_per_schema[schema_name] + trigger_count_per_schema[schema_name] + 1e-10)
            total += total_count_per_schema[schema_name]

            grt.send_progress(accumulated_progress + 0.1 * (i / (len(schemata_list) + 1e-10) ), "Gathered stats for %s" % schema_name)
            i += 1.0

        # Now take 60% in the first pass of reverse engineering:
        accumulated_progress = 0.2
        for schema_name in schemata_list:
            schema_progress_share = 0.6 * (total_count_per_schema.get(schema_name, 0.0) / total)
            schema = find_object_with_name(catalog.schemata, schema_name) 

            if schema:
                # Reverse engineer tables:
                step_progress_share = schema_progress_share * (table_count_per_schema[schema_name] / (total_count_per_schema[schema_name] + 1e-10))
                if get_tables:
                    cls.check_interruption()
                    grt.send_info('Reverse engineering tables from %s' % schema_name)
                    grt.begin_progress_step(accumulated_progress, accumulated_progress + step_progress_share)
                    # Remove previous first pass marks that may exist if the user goes back and attempt rev eng again:
                    progress_flags = cls._connections[connection.__id__].setdefault('_rev_eng_progress_flags', set())
                    progress_flags.discard('%s_tables_first_pass' % schema_name)
                    cls.reverseEngineerTables(connection, schema)
                    grt.end_progress_step()
        
                accumulated_progress += step_progress_share
                grt.send_progress(accumulated_progress, 'First pass of table reverse engineering for schema %s completed!' % schema_name)
        
                # Reverse engineer views:
                step_progress_share = schema_progress_share * (view_count_per_schema[schema_name] / (total_count_per_schema[schema_name] + 1e-10))
                if get_views:
                    cls.check_interruption()
                    grt.send_info('Reverse engineering views from %s' % schema_name)
                    grt.begin_progress_step(accumulated_progress, accumulated_progress + step_progress_share)
                    cls.reverseEngineerViews(connection, schema)
                    grt.end_progress_step()
        
                accumulated_progress += step_progress_share
                grt.send_progress(accumulated_progress, 'Reverse engineering of views for schema %s completed!' % schema_name)
        
                # Reverse engineer routines:
                step_progress_share = schema_progress_share * (routine_count_per_schema[schema_name] / (total_count_per_schema[schema_name] + 1e-10))
                if get_routines:
                    cls.check_interruption()
                    grt.send_info('Reverse engineering routines from %s' % schema_name)
                    grt.begin_progress_step(accumulated_progress, accumulated_progress + step_progress_share)
                    grt.begin_progress_step(0.0, 0.5)
                    cls.reverseEngineerProcedures(connection, schema)
                    cls.check_interruption()
                    grt.end_progress_step()
                    grt.begin_progress_step(0.5, 1.0)
                    reverseEngineerFunctions(connection, schema)
                    grt.end_progress_step()
                    grt.end_progress_step()
        
                accumulated_progress += step_progress_share
                grt.send_progress(accumulated_progress, 'Reverse engineering of routines for schema %s completed!' % schema_name)
        
                # Reverse engineer triggers:
                step_progress_share = schema_progress_share * (trigger_count_per_schema[schema_name] / (total_count_per_schema[schema_name] + 1e-10))
                if get_triggers:
                    cls.check_interruption()
                    grt.send_info('Reverse engineering triggers from %s' % schema_name)
                    grt.begin_progress_step(accumulated_progress, accumulated_progress + step_progress_share)
                    cls.reverseEngineerTriggers(connection, schema)
                    grt.end_progress_step()
        
                accumulated_progress = 0.8
                grt.send_progress(accumulated_progress, 'Reverse engineering of triggers for schema %s completed!' % schema_name)
            else:  # No schema with the given name was found
                grt.send_warning('The schema %s was not found in the catalog %s. Skipping it.' % (schema_name, catalog_name) )
                
        # Now the second pass for reverse engineering tables:
        if get_tables:
            total_tables = sum(table_count_per_schema[schema.name] for schema in catalog.schemata if schema.name in schemata_list)
            for schema in catalog.schemata:
                if schema.name not in schemata_list:
                    continue
                cls.check_interruption()
                step_progress_share = 0.2 * (table_count_per_schema[schema.name] / (total_tables + 1e-10))
                grt.send_info('Reverse engineering foreign keys for tables in schema %s' % schema.name)
                grt.begin_progress_step(accumulated_progress, accumulated_progress + step_progress_share)
                cls.reverseEngineerTables(connection, schema)
                grt.end_progress_step()
        
                accumulated_progress += step_progress_share
                grt.send_progress(accumulated_progress, 'Second pass of table reverse engineering for schema %s completed!' % schema_name)
            

        grt.send_progress(1.0, 'Reverse engineering completed!')
        return catalog


    @classmethod
    def reverseEngineerUserDatatypes(cls, connection, catalog):
        catalog.simpleDatatypes.remove_all()
        for type_row in cls.get_connection(connection).cursor().getTypeInfo(catalog=catalog.name):  # FIXME: there are duplicated names in this resultset
            simple_datatype = grt.classes.db_SimpleDatatype()
            simple_datatype.name = type_row[0]
            simple_datatype.characterMaximumLength = simple_datatype.characterOctetLength = simple_datatype.numericPrecision = simple_datatype.dateTimePrecision = type_row[2] if isinstance(type_row[2], int) else -1
            if isinstance(type_row[17], int):
                simple_datatype.numericPrecisionRadix = type_row[17]
            if isinstance(type_row[14], int):
                simple_datatype.numericScale = type_row[14]
            parameter_format_type_mapping = { 0: 0, # none
                                              1: 2, # [(n)]
                                              2: 6, # [(m[, n])]
                                            }
            if type_row[5] is not None:  # parameter format
                simple_datatype.parameterFormatType = parameter_format_type_mapping.get(len(type_row[5].split(',')), 0)
            else:
                simple_datatype.parameterFormatType = 0

            simple_datatype.needsQuotes = type_row[3] in ["N'", "'"]
            
            catalog.simpleDatatypes.append(simple_datatype)


    @classmethod
    def reverseEngineerCatalog(cls, connection, catalog_name):
        catalog = grt.classes.db_Catalog()
        catalog.name = catalog_name
        
        cls.reverseEngineerUserDatatypes(connection, catalog)

        schemata_names = cls.getSchemaNames(connection, catalog_name) or ['']
        catalog.schemata.remove_all()
        for schema_name in schemata_names:
            schema = grt.classes.db_Schema()
            schema.name = schema_name
            schema.owner = catalog
            if hasattr(cls, 'reverseEngineerSequences'):
                cls.reverseEngineerSequences(connection, schema)
            catalog.schemata.append(schema)
        return catalog

    @classmethod
    def reverseEngineerTables(cls, connection, schema):
        # Since there are some reverse engineering stages that requires all table names and table columns
        # in the database to be set, these should be done after a first pass that rev engs their requirements
        progress_flags = cls._connections[connection.__id__].setdefault('_rev_eng_progress_flags', set())
        is_first_pass = not ('%s_tables_first_pass' % schema.name) in progress_flags

        if is_first_pass:
            catalog = schema.owner
            schema.tables.remove_all()
            table_names = cls.getTableNames(connection, catalog.name, schema.name)
            getCommentForTable = cls.getCommentForTable if hasattr(cls, 'getCommentForTable') else lambda conn, tbl:''
            total = len(table_names) + 1e-10
            i = 0.0
            for table_name in table_names:
                grt.send_progress(i / total, 'Retrieving table %s.%s...' % (schema.name, table_name))
                table = grt.classes.db_Table()
                table.name = table_name
                schema.tables.append(table)
                table.owner = schema
                table.comment = getCommentForTable(connection, table)
        
                cls.reverseEngineerTableColumns(connection, table)
                cls.reverseEngineerTablePK(connection, table)
                cls.reverseEngineerTableIndices(connection, table)
        
                i += 1.0
            progress_flags.add('%s_tables_first_pass' % schema.name)
        else:  # Second pass
            i = 0.0
            total = len(schema.tables) + 1e-10
            cls._connections[connection.__id__]['fk_names'] = {}
            for table in schema.tables:
                cls.reverseEngineerTableFKs(connection, table)
                grt.send_progress(i / total, 'Reverse engineering of foreign keys in table %s.%s completed' % (schema.name, table.name))
                i += 1.0

        return 0

    @classmethod
    def reverseEngineerTableColumns(cls, connection, table):
        schema = table.owner
        catalog = schema.owner

        simple_datatypes_list = [ datatype.name.upper() for datatype in catalog.simpleDatatypes ]
        user_datatypes_list   = [ datatype.name.upper() for datatype in catalog.userDatatypes ]

        odbc_datatypes = dict( (dtype.data_type, dtype.type_name) for dtype in cls.get_connection(connection).cursor().getTypeInfo() )

        table_columns = cls.get_connection(connection).cursor().columns(table=table.name)
        for column_info in table_columns:
            column = grt.classes.db_Column()
            column.name = column_info[3]  # column_name
            column.isNotNull = column_info[17] != 'YES'  # is_nullable
            column.length = column_info[6]  # column_size
            column.scale = column_info[8]  # decimal_digits
            column.precision = column_info[6]  # column_size

            datatype = None
            try:
                type_name = odbc_datatypes[column_info[4]].upper()  # data_type
                datatype = simple_datatypes_list.index(type_name)
            except (KeyError, ValueError):
                try:
                    user_datatype = catalog.userDatatypes[user_datatypes_list.index(type_name)]
                except (ValueError, TypeError, NameError):
                    user_datatype = None
                    datatype = simple_datatypes_list.index('VARCHAR')
                    column.length = 255
                    msg = 'Column datatype "%s" for column "%s" in table "%s.%s" reverse engineered as VARCHAR(255)' % (type_name, column.name, schema.name, table.name)
                    grt.send_warning('%s reverseEngineerTableColumns: ' % cls.getTargetDBMSName() + msg)
                else:
                    datatype = None
                    column.userType = user_datatype

            if isinstance(datatype, int):
                column.simpleType = catalog.simpleDatatypes[datatype]

            table.addColumn(column)

        return 0


    @classmethod
    def reverseEngineerTablePK(cls, connection, table):
        """Reverse engineers the primary key for the given table."""

        if len(table.columns) == 0:  # Table must have columns reverse engineered before we can rev eng its primary key
            grt.send_error('Migration: reverseEngineerTablePKAndIndices: Reverse engineer of table %s was attempted but the table has no columns attribute' % table.name)
            return 1
        
        pk_index_name = 'PrimaryKey'

        indices_dict = {}  # Map the indices names to their respective columns:
        for row in cls.get_connection(connection).cursor().statistics(table=table.name):
            if row.type == constant.SQL_TABLE_STAT:  # this entry is not an index
                continue
            indices_dict.setdefault(row.index_name, []).append(row)

        for index_name, row_list in list(indices_dict.items()):
            index = grt.classes.db_Index()
            index.name = index_name
            index.isPrimary = 1 if index_name == pk_index_name else 0
            index.unique = not row_list[0].non_unique
            index.indexType = 'UNIQUE' if index.unique else 'INDEX'
    #        index.hasFilter = False  # TODO: Find out if there's a way to determine this

            skip = False
            # Get the columns for the index:
            for row in sorted(row_list, key=lambda elem: elem[7]):  # Sorted by ordinal_position
                column = find_object_with_name(table.columns, row.column_name)
                if column:
                    # skip indexes on LONGCHAR columns
                    if column.simpleType.name in ["LONGCHAR"]:
                        grt.send_warning("Migration: reverseEngineerTable: Skipping index %s.%s on a %s column\n" % (table.name, column.name, column.simpleType.name)) 
                        skip = True
                    else:
                        index_column = grt.classes.db_IndexColumn()
                        index_column.name = index_name + '.' + row.column_name
                        index_column.referencedColumn = column
                        index.columns.append(index_column)
                        if not column.isNotNull and index.isPrimary:
                            column.isNotNull = 1
                            grt.send_warning("Migration: reverseEngineerTablePK: column %s.%s was changed to NOT NULL because it's a Primary Key column\n" % (column.owner.name, column.name))
                else:
                    grt.send_warning("Migration: reverseEngineerTablePK: could not find column %s, belonging to key %s. Key will be skipped\n" % (row.column_name, index_name))
                    skip = True
            if not skip:
                table.addIndex(index)
                if index.isPrimary:
                    table.primaryKey = index

        return 0


    @classmethod
    def reverseEngineerTableIndices(cls, connection, table):
        pass  # Indices already reverse engineered in reverseEngineerTablePK


    @classmethod
    def reverseEngineerTableFKs(cls, connection, table):
        """Reverse engineers the foreign keys for the given table."""
        
        def get_update_action(grbit):
            if grbit & 256:
                return "CASCADE"
            else:
                return "RESTRICT"

        def get_delete_action(grbit):
            if grbit & 4352:
                return "CASCADE"
            else:
                return "RESTRICT"
            
        def process_fk(catalog, table, fk_name, fk_rows):
            foreign_key = grt.classes.db_ForeignKey()
            if fk_name in cls._connections[connection.__id__]['fk_names']:
                while True:
                    suffix = '_%06d' % random.randint(0, 999999)
                    if fk_name + suffix not in cls._connections[connection.__id__]['fk_names']:
                        break
                fk_name += suffix
            foreign_key.name = fk_name
            foreign_key.owner = table
            foreign_key.deleteRule = get_delete_action(fk_rows[0].grbit)
            foreign_key.updateRule = get_update_action(fk_rows[0].grbit)
            foreign_key.modelOnly = 0
            
            # Find the referenced table:
            foreign_key.referencedTable = find_object_with_name(catalog.schemata[0].tables, fk_rows[0].szReferencedObject)
            if not foreign_key.referencedTable:
                grt.send_error('Migration: reverseEngineerTableFKs: Table "%s" not found in schemata "%s"' % (fk_rows[0].szReferencedObject, catalog.schemata[0].name) )
                return 1
            
            for fk_row in fk_rows:
                column = find_object_with_name(table.columns, fk_row.szColumn)
                if not column:
                    grt.send_error('Migration: reverseEngineerTableFKs: Column "%s" not found in table "%s"' % (fk_row.szColumn, table.name) )
                    continue

                ref_column = find_object_with_name(foreign_key.referencedTable.columns, fk_row.szReferencedColumn)
                if not ref_column:
                    grt.send_error('Migration: reverseEngineerTableFKs: Column "%s" not found in table "%s"' % (fk_row.szReferencedColumn, foreign_key.referencedTable.name) )
                    continue
                
                foreign_key.columns.append(column)
                foreign_key.referencedColumns.append(ref_column)

            # Find and delete indexes that are identical to FKs
            for index in reversed(table.indices):
                if table.primaryKey != index and len(index.columns) == len(foreign_key.columns):
                    match = True
                    for i, col in enumerate(index.columns):
                        if foreign_key.columns[i] != col.referencedColumn:
                            match = False
                            break
                    if match:
                        grt.send_warning("Migration: reverseEngineerTable: Skipping duplicate index %s from table %s\n" % (col.name, table.name))
                        table.indices.remove(index)

            cls._connections[connection.__id__]['fk_names'][foreign_key.name] = table 
            table.foreignKeys.append(foreign_key)
                

        if len(table.columns) == 0:
            grt.send_error('Migration: reverseEngineerTableFKs: Reverse engineer of table %s was attempted but the table has no columns attribute' % table.name)
            return 1    # Table must have columns reverse engineered before we can rev eng its indices

        catalog = table.owner.owner
        table.foreignKeys.remove_all()
        fk_dict = {}  # Map the foreign key names to their respective columns:

        import pyodbc
        try:
            for row in cls.get_connection(connection).cursor().execute("SELECT * FROM MSysRelationships WHERE szObject = ?", (table.name,)):
                fk_dict.setdefault(row.szRelationship, []).append(row)
        except pyodbc.ProgrammingError as e:
            if e.args[0] == '42000':
                grt.send_error("\n\nMigration: Could not read from System Tables. You must grant SELECT access on all system tables for the database.")
                return 1
            raise

        for fk_name, fk_columns in list(fk_dict.items()):
            process_fk(catalog, table, fk_name, fk_columns)
        return 0


    @classmethod
    def reverseEngineerViews(cls, connection, schema):
        for view_name in cls.getViewNames(connection, schema.owner.name, schema.name):
            grt.send_info('%s reverseEngineerViews: Cannot reverse engineer view "%s"' % (cls.getTargetDBMSName(), view_name))
        return 0


    @classmethod
    def reverseEngineerProcedures(cls, connection, schema):
        # Unfortunately it seems that there's no way to get the SQL definition of a store procedure/function with ODBC
        for procedure_name in cls.getProcedureNames(connection, schema.owner.name, schema.name):
            grt.send_info('%s reverseEngineerProcedures: Cannot reverse engineer procedure "%s"' % (cls.getTargetDBMSName(), procedure_name))
        return 0


    @classmethod
    def reverseEngineerFunctions(cls, connection, schema):
        # Unfortunately it seems that there's no way to get the SQL definition of a store procedure/function with ODBC
        for function_name in cls.getFunctionNames(connection, schema.owner.name, schema.name):
            grt.send_info('%s reverseEngineerFunctions: Cannot reverse engineer function "%s"' % (cls.getTargetDBMSName(), function_name))
        return 0


    @classmethod
    def reverseEngineerTriggers(cls, connection, schema):
        # Unfortunately it seems that there's no way to get the SQL definition of a trigger with ODBC
        for trigger_name in cls.getTriggerNames(connection, schema.owner.name, schema.name):
            grt.send_info('%s reverseEngineerTriggers: Cannot reverse engineer trigger "%s"' % (cls.getTargetDBMSName(), trigger_name))
        return 0


    @classmethod
    def resetProgressFlags(cls, connection):
        cls._connections[connection.__id__]['_rev_eng_progress_flags'] = set()
        return 0
###############################################################################################################

@ModuleInfo.export(grt.classes.db_mgmt_Rdbms)
def initializeDBMSInfo():
    return MsAccessReverseEngineering.initializeDBMSInfo('msaccess_rdbms_info.xml')

@ModuleInfo.export((grt.LIST, grt.STRING))
def getDataSourceNames():
    return MsAccessReverseEngineering.getDataSourceNames()


@ModuleInfo.export(grt.LIST)
def getSupportedObjectTypes():
    return MsAccessReverseEngineering.getSupportedObjectTypes()


@ModuleInfo.export(grt.STRING, grt.STRING)
def quoteIdentifier(name):
    return MsAccessReverseEngineering.quoteIdentifier(name)


@ModuleInfo.export(grt.STRING, grt.classes.GrtNamedObject)
def fullyQualifiedObjectName(obj):
    return MsAccessReverseEngineering.fullyQualifiedObjectName(obj)


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.STRING)
def connect(connection, password):
    return MsAccessReverseEngineering.connect(connection, password)


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def disconnect(connection):
    return MsAccessReverseEngineering.disconnect(connection)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def isConnected(connection):
    return MsAccessReverseEngineering.isConnected(connection)


@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def getDriverDBMSName(connection):
    return MsAccessReverseEngineering.getDriverDBMSName(connection)
    
@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return MsAccessReverseEngineering.getTargetDBMSName()

@ModuleInfo.export(grt.classes.GrtVersion, grt.classes.db_mgmt_Connection)
def getServerVersion(connection):
    return MsAccessReverseEngineering.getServerVersion(connection)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection)
def getCatalogNames(connection):
    return MsAccessReverseEngineering.getCatalogNames(connection)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING)
def getSchemaNames(connection, catalog_name):
    return MsAccessReverseEngineering.getSchemaNames(connection, catalog_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTableNames(connection, catalog_name, schema_name):
    return MsAccessReverseEngineering.getTableNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getViewNames(connection, catalog_name, schema_name):
    return MsAccessReverseEngineering.getViewNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTriggerNames(connection, catalog_name, schema_name):
    return MsAccessReverseEngineering.getTriggerNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getProcedureNames(connection, catalog_name, schema_name):
    return MsAccessReverseEngineering.getProcedureNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getFunctionNames(connection, catalog_name, schema_name):
    return MsAccessReverseEngineering.getFunctionNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING, (grt.LIST, grt.STRING), grt.DICT)
def reverseEngineer(connection, catalog_name, schemata_list, context):
    return MsAccessReverseEngineering.reverseEngineer(connection, catalog_name, schemata_list, context)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Catalog)
def reverseEngineerUserDatatypes(connection, catalog):
    return MsAccessReverseEngineering.reverseEngineerUserDatatypes(connection, catalog)

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING)
def reverseEngineerCatalog(connection, catalog_name):
    return MsAccessReverseEngineering.reverseEngineerCatalog(connection, catalog_name)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerTables(connection, schema):
    return MsAccessReverseEngineering.reverseEngineerTables(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerViews(connection, schema):
    return MsAccessReverseEngineering.reverseEngineerViews(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerProcedures(connection, schema):
    return MsAccessReverseEngineering.reverseEngineerProcedures(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerFunctions(connection, schema):
    return MsAccessReverseEngineering.reverseEngineerFunctions(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerTriggers(connection, schema):
    return MsAccessReverseEngineering.reverseEngineerTriggers(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def resetProgressFlags(connection):
    return MsAccessReverseEngineering.resetProgressFlags(connection)

