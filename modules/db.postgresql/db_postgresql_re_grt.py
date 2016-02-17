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

from db_sql92_re_grt import Sql92ReverseEngineering

from wb import DefineModule
from workbench.utils import find_object_with_name

import grt

ModuleInfo = DefineModule(name= "DbPostgresqlRE", author= "Oracle Corp.", version="1.0")

class PostgresqlReverseEngineering(Sql92ReverseEngineering):
    @classmethod
    def getTargetDBMSName(cls):
        return 'Postgresql'

    @classmethod
    def serverVersion(cls, connection):
        return cls._connections[connection.__id__]["version"]

    @classmethod 
    def connect(cls, connection, password):
        r = super(PostgresqlReverseEngineering, cls).connect(connection, password)
        if r:
            ver = cls.execute_query(connection, "select version()").fetchone()[0]
            grt.log_info("PostgreSQL RE", "Connected to %s, %s\n" % (connection.name, ver))
            ver_parts = [int(n) for n in ver.split()[1].rstrip(",").split(".")] + 4*[0]
            version = grt.classes.GrtVersion()
            version.majorNumber, version.minorNumber, version.releaseNumber, version.buildNumber = ver_parts[:4]
            cls._connections[connection.__id__]["version"] = version

            if version.majorNumber < 8:
                raise RuntimeError("PostgreSQL version %s is not a supported migration source.\nAt least version 8 is required." % ver)
        return r

    @classmethod
    def getSchemaNames(cls, connection, catalog_name):
        """Returns a list of schemata for the given connection object."""

        return [ schema_name for schema_name in super(PostgresqlReverseEngineering, cls).getSchemaNames(connection, catalog_name)
                 if schema_name.upper() not in ['INFORMATION_SCHEMA', 'PG_CATALOG'] ]


    #########  Reverse Engineering functions #########

    @classmethod
    def reverseEngineerSequences(cls, connection, schema):
        schema.sequences.remove_all()

        seq_names_query = """SELECT c.relname
FROM pg_catalog.pg_class c JOIN pg_catalog.pg_namespace n
     ON (c.relnamespace = n.oid)
WHERE n.nspname = '%s' AND c.relkind in ('S', 's')""" % schema.name

        seq_details_query = """SELECT min_value, max_value, start_value, 
increment_by, last_value, is_cycled, cache_value
FROM "%s"."%s" """

        sequence_names = cls.execute_query(connection, seq_names_query).fetchall()
        for (seq_name, ) in sequence_names:
            min_value, max_value, start_value, increment_by, last_value, is_cycled, ncache = cls.execute_query(connection, seq_details_query % (schema.name, seq_name)).fetchone()
            sequence = grt.classes.db_Sequence()
            sequence.name = seq_name
            sequence.owner = schema
            sequence.minValue = str(min_value)
            sequence.maxValue = str(max_value)
            sequence.startValue = str(start_value)
            sequence.incrementBy = str(increment_by)
            sequence.lastNumber = str(last_value)
            sequence.cycleFlag = int(is_cycled)
            sequence.cacheSize = str(ncache)

            schema.sequences.append(sequence)


    @classmethod
    def reverseEngineerTableIndices(cls, connection, table):
        schema = table.owner
        
        if len(table.columns) == 0:
            grt.send_error('%s: reverseEngineerTableIndices', 
                'Reverse engineer of table %s.%s was attempted but the table has no columns attribute' % (cls.getTargetDBMSName(), schema.name, table.name) )
            return 1    # Table must have columns reverse engineered before we can rev eng its indices

        all_indices_query = """SELECT c2.relname, i.indisunique::int, i.indisclustered::int, i.indnatts, i.indkey
FROM pg_catalog.pg_class c, pg_catalog.pg_class c2, pg_catalog.pg_namespace n, pg_catalog.pg_index i
WHERE c.oid = i.indrelid AND i.indexrelid = c2.oid AND c.relnamespace = n.oid AND 
n.nspname = '%s' AND c.relname = '%s' AND i.indisprimary = False 
ORDER BY c2.relname""" % (schema.name, table.name)

        index_columns_query = """SELECT a.attname
FROM unnest(ARRAY%r) attrid
JOIN pg_catalog.pg_attribute a ON attrid=a.attnum
JOIN pg_catalog.pg_class c ON c.oid = a.attrelid
JOIN pg_catalog.pg_namespace n ON c.relnamespace = n.oid
WHERE n.nspname = '%s' AND c.relname = '%s'"""

        index_rows = cls.execute_query(connection, all_indices_query).fetchall()
        for index_name, is_unique, is_clustered, column_count, column_refs in index_rows:
            index = grt.classes.db_Index()
            index.name = index_name
            index.isPrimary = 0
            index.unique = is_unique
            index.indexType = ('UNIQUE' if is_unique else 'INDEX')
            #index.clustered = is_clustered

            # Get the columns for the index:
            cols = [ int(col) for col in column_refs.split() ]
            if column_count != len(cols):
                grt.send_warning('%s: reverseEngineerTableIndices' % cls.getTargetDBMSName(), 
                    'Reverse engineer of index %s.%s was attempted but the referenced columns count differs '
                    'from the number of its referenced columns. Skipping index!' % (schema.name, index_name) )
                continue

            for (column_name, ) in cls.execute_query(connection, index_columns_query % (cols, schema.name, table.name)):
                column = find_object_with_name(table.columns, column_name)
                if column:
                    index_column = grt.classes.db_IndexColumn()
                    index_column.name = index_name + '.' + column_name
                    #index_column.descend = is_descending_key
                    index_column.referencedColumn = column
                    index.columns.append(index_column)
                else:
                    grt.send_warning('%s: reverseEngineerTableIndices' % cls.getTargetDBMSName(), 
                        'Reverse engineer of index %s.%s was attempted but the referenced column %s '
                        'could not be found on table %s. Skipping index!' % (schema.name, index_name, column_name, table.name) )
                    continue

            table.addIndex(index)
        return 0

    
    @classmethod
    def getColumnDatatype(cls, connection, table, column, type_name):
        if type_name == 'USER-DEFINED':
            query = """SELECT a.attname,
                            pg_catalog.format_type(a.atttypid, a.atttypmod)
                        FROM pg_catalog.pg_attribute a LEFT JOIN pg_catalog.pg_class c ON a.attrelid = c.oid
                            LEFT JOIN pg_catalog.pg_namespace n ON c.relnamespace = n.oid
                        WHERE n.nspname = '%s' AND c.relname = '%s' AND a.attname = '%s' AND NOT a.attisdropped;
                    """ % (table.owner.name, table.name, column.name)
            udtype = cls.execute_query(connection, query).fetchall()
            if udtype:
                type_name = udtype[0][1]
 
        return super(PostgresqlReverseEngineering, cls).getColumnDatatype(connection, table, column, type_name)


    @classmethod
    def reverseEngineerUserDatatypes(cls, connection, catalog):
        """
          There are several kinds of user datatypes in Postgres, including:
          - domains
          - tuples/composite (table like structure)
          - ranges (numeric ranges with fancy definition, only in 9.2+)
          - base types
          - enums
          - others
          
          As of now, we're only supporting domains and enums.
          Ranges can be migrated to their underlying type.
          Composite types should be migrated to StructuredTypes at some point.
        """
        version = cls.serverVersion(connection)

        catalog.userDatatypes.remove_all()
        
        #query_composite = """SELECT t.typname, at.attname, pg_catalog.format_type(at.atttypid, at.atttypmod)
        #          FROM pg_type t
        #          JOIN pg_class on (reltype = t.oid)
        #          JOIN pg_attribute at on (at.attrelid = pg_class.oid)
        #          JOIN pg_type a on (at.atttypid = a.oid)
        #          JOIN pg_namespace n on n.oid = t.typnamespace
        #          WHERE n.nspname NOT IN ('information_schema', 'pg_catalog') AND pg_class.relkind = 'c' """
        # TODO
            
        query_domains = """SELECT t.typname, pg_catalog.format_type(t.typbasetype, t.typtypmod)
            FROM pg_catalog.pg_type t
            LEFT JOIN pg_catalog.pg_namespace n ON n.oid = t.typnamespace
            WHERE n.nspname NOT IN ('information_schema', 'pg_catalog') AND t.typtype = 'd' """
        domain_types = cls.execute_query(connection, query_domains)
        for type_name, type_def in domain_types:
            if not type_def:
                continue
            datatype = grt.classes.db_UserDatatype()
            datatype.name = type_name
            datatype.sqlDefinition = type_def
            if '(' in type_def:
                base_type = type_def[:type_def.find('(')]
            else:
                base_type = type_def
            up_type_name = base_type.upper()
            for stype in cls._rdbms.simpleDatatypes:
                if stype.name.upper() == up_type_name or up_type_name in [s.upper() for s in stype.synonyms]:
                    datatype.actualType = stype
                    break
            datatype.owner = catalog
            catalog.userDatatypes.append(datatype)

        #query_ranges = """
        #    """
        # TODO

        # PotgreSQl 8.2 and lower doesn't have enum types
        if (version.majorNumber > 8) or (version.majorNumber == 8 and version.minorNumber > 2):
            query_enums = """SELECT t.typname, e.enumlabel
              FROM pg_catalog.pg_type t LEFT JOIN pg_catalog.pg_namespace n
              ON n.oid = t.typnamespace
              LEFT JOIN pg_catalog.pg_enum e ON e.enumtypid = t.oid
              WHERE t.typrelid = 0 AND t.typtype = 'e'
              AND n.nspname NOT IN ('information_schema', 'pg_catalog')
              AND NOT EXISTS(SELECT 1 FROM pg_catalog.pg_type el WHERE el.oid = t.typelem AND el.typarray = t.oid)
              ORDER BY e.enumsortorder"""
    
            query_enums_80 =  """SELECT t.typname, e.enumlabel
              FROM pg_catalog.pg_type t LEFT JOIN pg_catalog.pg_namespace n
              ON n.oid = t.typnamespace
              LEFT JOIN pg_catalog.pg_enum e ON e.enumtypid = t.oid
              WHERE t.typrelid = 0 AND t.typtype = 'e'
              AND n.nspname NOT IN ('information_schema', 'pg_catalog')
              AND NOT EXISTS(SELECT 1 FROM pg_catalog.pg_type el WHERE el.oid = t.typelem AND el.typarray = t.oid)
              """
    
            enum_types = cls.execute_query(connection, query_enums if version.majorNumber >= 9 and version.minorNumber >= 1 else query_enums_80)
    
            ltype = None
            types = []
            values = []
            for type_name, enum_label in enum_types:
                if type_name != ltype:
                    ltype = type_name
                    values = []
                    types.append((type_name, values))
                values.append(enum_label)
    
            enumType = None
            for type_name, enum_labels in types:
                datatype = grt.classes.db_UserDatatype()
                datatype.name = type_name
                datatype.sqlDefinition = 'enum(%s)' % (', '.join(["'%s'" % l.replace("'", "''") for l in enum_labels]))
                datatype.actualType = enumType
                datatype.owner = catalog
                catalog.userDatatypes.append(datatype)



    @classmethod
    def getOS(cls, connection):
        ver = cls.execute_query(connection, "select version()").fetchone()[0].lower()
        if 'linux' in ver:
            return 'linux'
        elif 'visual c++' in ver:
            return 'windows'
        elif 'darwin' in ver or 'apple' in ver:
            return 'darwin'

        return None



###########################################################################################

@ModuleInfo.export(grt.classes.db_mgmt_Rdbms)
def initializeDBMSInfo():
    return PostgresqlReverseEngineering.initializeDBMSInfo('postgresql_rdbms_info.xml')

@ModuleInfo.export((grt.LIST, grt.STRING))
def getDataSourceNames():
    return PostgresqlReverseEngineering.getDataSourceNames()


@ModuleInfo.export(grt.STRING, grt.STRING)
def quoteIdentifier(name):
    return PostgresqlReverseEngineering.quoteIdentifier(name)


@ModuleInfo.export(grt.STRING, grt.classes.GrtNamedObject)
def fullyQualifiedObjectName(obj):
    return PostgresqlReverseEngineering.fullyQualifiedObjectName(obj)


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.STRING)
def connect(connection, password):
    return PostgresqlReverseEngineering.connect(connection, password)


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def disconnect(connection):
    return PostgresqlReverseEngineering.disconnect(connection)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def isConnected(connection):
    return PostgresqlReverseEngineering.isConnected(connection)

@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return PostgresqlReverseEngineering.getTargetDBMSName()

@ModuleInfo.export(grt.LIST)
def getSupportedObjectTypes():
    return PostgresqlReverseEngineering.getSupportedObjectTypes()

@ModuleInfo.export(grt.classes.GrtVersion, grt.classes.db_mgmt_Connection)
def getServerVersion(connection):
    return PostgresqlReverseEngineering.getServerVersion(connection)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection)
def getCatalogNames(connection):
    return PostgresqlReverseEngineering.getCatalogNames(connection)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING)
def getSchemaNames(connection, catalog_name):
    return PostgresqlReverseEngineering.getSchemaNames(connection, catalog_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTableNames(connection, catalog_name, schema_name):
    return PostgresqlReverseEngineering.getTableNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getViewNames(connection, catalog_name, schema_name):
    return PostgresqlReverseEngineering.getViewNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getTriggerNames(connection, catalog_name, schema_name):
    return PostgresqlReverseEngineering.getTriggerNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getProcedureNames(connection, catalog_name, schema_name):
    return PostgresqlReverseEngineering.getProcedureNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING, grt.STRING)
def getFunctionNames(connection, catalog_name, schema_name):
    return PostgresqlReverseEngineering.getFunctionNames(connection, catalog_name, schema_name)

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING, (grt.LIST, grt.STRING), grt.DICT)
def reverseEngineer(connection, catalog_name, schemata_list, context):
    return PostgresqlReverseEngineering.reverseEngineer(connection, catalog_name, schemata_list, context)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Catalog)
def reverseEngineerUserDatatypes(connection, catalog):
    return PostgresqlReverseEngineering.reverseEngineerUserDatatypes(connection, catalog)

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_mgmt_Connection, grt.STRING)
def reverseEngineerCatalog(connection, catalog_name):
    return PostgresqlReverseEngineering.reverseEngineerCatalog(connection, catalog_name)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerTables(connection, schema):
    return PostgresqlReverseEngineering.reverseEngineerTables(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerViews(connection, schema):
    return PostgresqlReverseEngineering.reverseEngineerViews(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerProcedures(connection, schema):
    return PostgresqlReverseEngineering.reverseEngineerProcedures(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerFunctions(connection, schema):
    return PostgresqlReverseEngineering.reverseEngineerFunctions(connection, schema)

@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_Schema)
def reverseEngineerTriggers(connection, schema):
    return PostgresqlReverseEngineering.reverseEngineerTriggers(connection, schema)

@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def getOS(connection):
    return PostgresqlReverseEngineering.getOS(connection)
