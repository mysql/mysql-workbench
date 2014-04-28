# Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

import re

from wb import DefineModule
import grt

from db_generic_migration_grt import GenericMigration
from workbench.utils import find_object_with_name

ModuleInfo = DefineModule(name= "DbSybaseMigration", author= "Oracle Corp.", version="1.0")


truncated_identifier_serial = 0
class SybaseMigration(GenericMigration):
    def migrateIdentifier(self, sybase_name, log, dots_allowed=True):
        mysql_valid_regex = re.compile(r'^[^/\\.]+$', re.U)  # Database and table names cannot contain "/", "\", ".", or characters that are not permitted in file names
        mysql_name = sybase_name
        # Remove quoting chars from the identifier if present
        if ( sybase_name.startswith('"') and sybase_name.endswith('"') or
             sybase_name.startswith('[') and sybase_name.endswith(']') ):
            mysql_name = sybase_name[1:-1]
        if not mysql_valid_regex.match(mysql_name) and log:
            entry = grt.classes.GrtLogEntry()
            entry.entryType = 2
            entry.name = u'Sybase migrateIdentifier: Could not migrate identifier %s' % sybase_name
            log.entries.append(entry)
        
        # truncate too long identifiers
        if len(mysql_name) > 64:
            global truncated_identifier_serial
            truncated_identifier_serial += 1
            mysql_name = mysql_name[:62]+str(truncated_identifier_serial)
            if log:
                entry = grt.classes.GrtLogEntry()
                entry.entryType = 1
                entry.name = u'Identifier `%s` is too long, truncated to `%s`' % (sybase_name, mysql_name)
                log.entries.append(entry)
        
        return mysql_name

    def migrateTableToMySQL(self, state, sourceTable, targetSchema):
        targetTable = super(SybaseMigration, self).migrateTableToMySQL(state, sourceTable, targetSchema)
        targetTable.defaultCharacterSetName, targetTable.defaultCollationName =  self.migrateCharsetCollation(state, sourceTable.owner.defaultCharacterSetName, sourceTable.owner.defaultCollationName, sourceTable, targetTable)
        return targetTable

    def migrateColumnDefaultValue(self, state, default_value, source_column, target_column):
        target_default_value = default_value
        if source_column.simpleType:
            source_datatype = source_column.simpleType.name
            if source_datatype == 'TIMESTAMP':
                if default_value == 'getdate()':
                    target_default_value = 'CURRENT_TIMESTAMP'
            elif source_datatype in ['DATETIME', 'SMALLDATETIME']:
                if source_column.defaultValue == 'getdate()':
                    target_default_value = 'CURRENT_TIMESTAMP'

                    # Only timestamp supports CURRENT_TIMESTAMP, so force the target type to it
                    target_column.simpleType = find_object_with_name(state.targetCatalog.simpleDatatypes, 'TIMESTAMP')
                    state.addMigrationLogEntry(0, source_column, target_column, 
                              'Default value is %s, so type was changed from %s to TIMESTAMP' % (default_value, source_datatype))

            if default_value and not default_value.startswith("'") and target_default_value == default_value:
                # not a string, check for numeric literals
                try:
                    float(default_value)
                except:
                    # not a numeric literal
                    target_default_value = ''
                    state.addMigrationLogEntry(1, source_column, target_column, 
                              'Default value %s is not supported' % default_value)
                    
        return target_default_value


    def migrateDatatypeForColumn(self, state, source_column, target_column):
        targetCatalog = state.targetCatalog
    
        mysql_simpleTypes = dict( (datatype.name.upper(), datatype) for datatype in targetCatalog.simpleDatatypes )
        
        source_type = source_column.simpleType
        if not source_type and source_column.userType:
            # evaluate user type
            source_type = source_column.userType.actualType

            target_column.flags.extend(source_column.userType.flags)

        if source_type:
            # Decide which mysql datatype corresponds to the column datatype:
            source_datatype = source_type.name.upper()
            # string data types:
            target_datatype = ''
            if source_datatype in ['VARCHAR', 'NVARCHAR', 'UNIVARCHAR']:
                if 0 <= source_column.length < 256:
                    target_datatype = 'VARCHAR'
                elif 0 <= source_column.length < 65536:  # MySQL versions > 5.0 can hold up to 65535 chars in a VARCHAR column
                    if targetCatalog.version.majorNumber < 5:
                        target_datatype = 'MEDIUMTEXT'
                    else:
                        target_datatype = 'VARCHAR'
                else:
                    target_datatype = 'LONGTEXT'
            elif source_datatype in ['CHAR', 'NCHAR', 'UNICHAR']:
                if source_column.length < 256:
                    target_datatype = 'CHAR'
                else:
                    target_datatype = 'LONGTEXT'
            elif source_datatype in ['TEXT', 'NTEXT', 'UNITEXT']:
                target_datatype = 'LONGTEXT'
                target_column.length = -1
            # integer data types:
            elif source_datatype in ['BIGINT', 'INT', 'SMALLINT']:
                target_datatype = source_datatype
                target_column.precision = -1
            elif source_datatype == 'TINYINT':
                target_datatype = source_datatype
                target_column.precision = -1
                if 'UNSIGNED' not in target_column.flags:
                    target_column.flags.append('UNSIGNED')  # In Sybase TINYINT is unsigned
            elif source_datatype == 'SYSNAME':
                target_datatype = 'VARCHAR'
                target_column.length = 30
                state.addMigrationLogEntry(0, source_column, target_column,
                        "Source column type %s was migrated to %s(%s)" % (source_datatype, target_datatype, target_column.length))
            elif source_datatype == 'LONGSYSNAME':
                target_datatype = 'VARCHAR'
                target_column.length = 255
                state.addMigrationLogEntry(0, source_column, target_column,
                        "Source column type %s was migrated to %s(%s)" % (source_datatype, target_datatype, target_column.length))
            # floating point datatypes:
            elif source_datatype in ['DECIMAL', 'NUMERIC']:
                target_datatype = 'DECIMAL'
            elif source_datatype == 'REAL':
                target_datatype = 'FLOAT'
                target_column.precision = -1
            elif source_datatype == 'FLOAT':
                target_datatype = 'FLOAT' if source_column.length == 4 else 'DOUBLE'
                target_column.precision = -1
            elif source_datatype in ['MONEY', 'SMALLMONEY']:
                target_datatype = 'DECIMAL'
                target_column.precision = source_column.simpleType.numericPrecision
                target_column.scale = source_column.simpleType.numericScale
            # binary datatypes:
            elif source_datatype == 'IMAGE':
                target_datatype = 'LONGBLOB'
                target_column.length = -1
            elif source_datatype in ['BINARY', 'VARBINARY']:
                if 0 <= source_column.length < 256:
                    if source_datatype == 'IMAGE':
                        target_datatype = 'TINYBLOB'
                    else:
                        target_datatype = source_datatype
                elif 0 <= source_column.length < 65536:
                    target_datatype = 'MEDIUMBLOB'
                else:
                    target_datatype = 'LONGBLOB'
            # datetime datatypes:
            elif source_datatype in ['DATETIME', 'SMALLDATETIME']:
                target_datatype = 'DATETIME'
            # timestamp datatypes
            elif source_datatype == 'DATE':
                target_datatype = 'DATE'
            elif source_datatype == 'TIME':
                target_datatype = 'TIME'
            elif source_datatype == 'BIT':
                target_datatype = 'TINYINT'
                target_column.length = 1
                state.addMigrationLogEntry(0, source_column, target_column,
                      "Source column type BIT was migrated to TINYINT(1)")
            else:
                # just fall back to same type name and hope for the best
                target_datatype = source_datatype

            if mysql_simpleTypes.has_key(target_datatype):
                target_column.simpleType = mysql_simpleTypes[target_datatype]
            else:
                grt.log_warning("Sybase migrateTableColumnsToMySQL", "Can't find datatype %s for type %s\n" % (target_datatype, source_datatype))
                state.addMigrationLogEntry(2, source_column, target_column, 
                    'Could not migrate column "%s" in "%s": Unknown datatype "%s"' % (target_column.name, source_column.owner.name, source_datatype) )
                return False

            return True
        else:
            state.addMigrationLogEntry(2, source_column, target_column, 
                    'Could not migrate type of column "%s" in "%s" (%s)' % (target_column.name, source_column.owner.name, source_column.formattedRawType) )
            return False

        return True


    def migrateUpdateForChanges(self, state, target_catalog):
        """
        Create datatype cast expression for target column based on source datatype.
        """   
        for targetSchema in target_catalog.schemata:
            for targetTable in targetSchema.tables:
                for target_column in targetTable.columns:                    
                    # SQL expression to use for converting the column data to the target type
                    # eg.: CAST(? as NVARCHAR(max))
                    type_cast_expression = None
                    source_datatype = None
                    source_column = state.lookupSourceObject(target_column)
                    if source_column:
                        source_datatype = GenericMigration.getColumnDataType(self, source_column)
                    if source_column and source_datatype:
                        if source_datatype == 'SYSNAME':
                            type_cast_expression = "CONVERT(VARCHAR(30), ?)"
                        elif source_datatype == 'LONGSYSNAME':
                            type_cast_expression = "CONVERT(VARCHAR(255), ?)"

                        if type_cast_expression:
                            target_column.owner.customData["columnTypeCastExpression:%s" % target_column.name] = "%s as ?" % type_cast_expression

        return target_catalog


    def migrateCharsetCollation(self, state, charset, collation, source_object, target_object):
        if collation:
            state.addMigrationLogEntry(0, source_object, target_object, 
                    'Collation %s migrated to utf8_general_ci' % (collation))
            return '', 'utf8_general_ci'

        return charset, collation
    

    def migrateTableColumnToMySQL(self, state, source_column, targetTable):
        target_column = GenericMigration.migrateTableColumnToMySQL(self, state, source_column, targetTable)
        if target_column:
            # Autoincrement for integer datatypes:
            if source_column.simpleType:
                source_datatype = source_column.simpleType.name
                if source_datatype in ['INT', 'TINYINT', 'SMALLINT', 'BIGINT']:
                    target_column.autoIncrement = source_column.identity

            # TODO set charset/collation
            #target_column.characterSetName = 
            
        return target_column

    #def migrateTableIndexToMySQL(self, state, source_index, targetTable):
    #    index = GenericMigration.migrateTableIndexToMySQL(self, state, source_index, targetTable)
    #    return index

    def migrateTableForeignKeyToMySQL(self, state, source_fk, targetTable):
        target_fk = GenericMigration.migrateTableForeignKeyToMySQL(self, state, source_fk, targetTable)
        ### TODO: migrate constraints
        return target_fk



instance = SybaseMigration()
@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return 'Sybase'

@ModuleInfo.export(grt.STRING, grt.STRING, grt.classes.GrtLogObject)
def migrateIdentifier(name, log):
    return instance.migrateIdentifier(name, log)

@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_migration_Migration, grt.classes.db_Catalog)
def migrateCatalog(state, sourceCatalog):
    return instance.migrateCatalog(state, sourceCatalog)


@ModuleInfo.export(grt.classes.db_Schema, grt.classes.db_migration_Migration, grt.classes.db_Schema, grt.classes.db_Catalog)
def migrateSchema(state, sourceSchema, targetCatalog):
    return instance.migrateSchema(state, sourceSchema, targetCatalog)


@ModuleInfo.export(grt.classes.db_Table, grt.classes.db_migration_Migration, grt.classes.db_Table, grt.classes.db_Schema)
def migrateTableToMySQL(state, sourceTable, target_schema):
    return instance.migrateTableToMySQL(state, sourceTable, target_schema)


@ModuleInfo.export(grt.INT, grt.classes.db_migration_Migration, grt.classes.db_Table, grt.classes.db_Table)
def migrateTableToMySQL2ndPass(state, sourceTable, targetTable):
    return instance.migrateTableToMySQL2ndPass(state, sourceTable, targetTable)


@ModuleInfo.export(grt.classes.db_mysql_ForeignKey, grt.classes.db_migration_Migration, grt.classes.db_ForeignKey, grt.classes.db_Table)
def migrateTableForeignKeyToMySQL(state, source_fk, targetTable):
    return instance.migrateTableForeignKeyToMySQL(state, source_fk, targetTable)


@ModuleInfo.export(grt.classes.db_mysql_Trigger, grt.classes.db_migration_Migration, grt.classes.db_Trigger, grt.classes.db_Table)
def migrateTriggerToMySQL(state, source_trigger, target_table):
    return instance.migrateTriggerToMySQL(state, source_trigger, target_table)
  

@ModuleInfo.export(grt.classes.db_mysql_View, grt.classes.db_migration_Migration, grt.classes.db_View, grt.classes.db_Schema)
def migrateViewToMySQL(state, source_view, target_schema):
    return instance.migrateViewToMySQL(state, source_view, target_schema)


@ModuleInfo.export(grt.classes.db_mysql_Routine, grt.classes.db_migration_Migration, grt.classes.db_Routine, grt.classes.db_Schema)
def migrateRoutineToMySQL(state, source_routine, target_schema):
    return instance.migrateRoutineToMySQL(state, source_routine, target_schema)


@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_migration_Migration, grt.classes.db_Catalog)
def migrateUpdateForChanges(state, target_catalog):
    return instance.migrateUpdateForChanges(state, target_catalog)


@ModuleInfo.export((grt.LIST, grt.classes.db_migration_MigrationParameter), grt.classes.db_migration_Migration)
def getMigrationOptions(state):
    list = grt.List(grt.OBJECT, grt.classes.db_migration_MigrationParameter.__grtclassname__)

    return list


