# Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

ModuleInfo = DefineModule(name= "DbSQLAnywhereMigration", author= "Oracle Corp.", version="1.0")


class SQLAnywhereMigration(GenericMigration):
    def migrateColumnDefaultValue(self, state, default_value, source_column, target_column):
        target_catalog = target_column.owner.owner.owner
        target_default_value = default_value

        def raise_on_no_match(re_str, target):
            if re.match(re_str, target) is None:
                raise ValueError('"%s" does not match the regular expression "%s"' % (target, re_str))
            return True

        value_validators = [
            (['TINYINT', 'SMALLINT', 'INTEGER', 'BIGINT'], int),
            (['NUMERIC', 'DECIMAL', 'FLOAT', 'REAL', 'DOUBLE'], float),
            (['CHAR', 'VARCHAR', 'NCHAR', 'NVARCHAR', 'TEXT', 'NTEXT' 'UNIQUEIDENTIFIERSTR', 'BINARY', 'LONG BINARY', 'XML'], lambda _:True),
            (['VARBIT', 'LONG VARBIT'], lambda val: raise_on_no_match(r"[Bb]?'?[10]+'?", val) ),  # E.g. B'101001'
            (['DATE'], lambda val: raise_on_no_match(r"(\d{4}|\d{2})-\d{1,2}-\d{1,2}", val) ),
            (['TIME'], lambda val: raise_on_no_match(r"(\d{1,2} )?\d{1,2}:\d{0,2}:\d{0,2}", val) ),
            (['TIMESTAMP', 'DATETIME'], lambda val: raise_on_no_match(
                r"((\d{4}|\d{2})-\d{1,2}-\d{1,2}( (\d{1,2} )?\d{1,2}:\d{0,2}:\d{0,2})?|NULL|NOW\(\))", val.upper()) ),
        ]

        source_datatype = None
        if source_column.simpleType:
            source_datatype = source_column.simpleType.name
            if default_value:
                group = source_column.simpleType.group if source_column.simpleType else source_column.userType.actualType.group
                if group.name == 'numeric':
                    default_value = default_value.strip("' ")

        if default_value:
            target_default_value = default_value

            if default_value.upper() == 'AUTOINCREMENT' and source_datatype in ['TINYINT', 'SMALLINT', 'INTEGER', 'BIGINT']:
                target_column.flags.append('AUTO_INCREMENT')
                return ''

            for value_validator in value_validators:
                if source_datatype in value_validator[0]:
                    try:
                        value_validator[1](default_value)
                    except Exception:
                        target_default_value = ''
                        state.addMigrationLogEntry(1, source_column, target_column,
                                  'Default value %s is not supported. Removed!' % default_value)
                    else:
                        target_datatype = target_column.simpleType and target_column.simpleType.name or ''
                        if source_datatype in ['TIMESTAMP', 'DATETIME'] and default_value.upper() in ['NOW()', 'CURRENT TIMESTAMP', 'CURRENT_TIMESTAMP', 'TIMESTAMP']:
                            if (target_catalog.version.majorNumber, target_catalog.version.minorNumber, target_catalog.version.releaseNumber) < (5, 6, 5):
                                if  target_datatype == 'TIMESTAMP':
                                    target_default_value = 'CURRENT_TIMESTAMP'  # now() => CURRENT_TIMESTAMP for TIMESTAMP columns in server v<5.6.5
                                else:
                                    target_default_value = ''
                                    state.addMigrationLogEntry(1, source_column, target_column,
                                              'Default value %s is not supported for a MySQL column of type "%s".Removed!' % (default_value, target_datatype))
                            else:  # Server version from 5.6.5 and newer
                                target_default_value = 'CURRENT_TIMESTAMP'  # CURRENT_TIMESTAMP freely allowed for DATETIME & TIMESTAMP columns



        return target_default_value


    def migrateDatatypeForColumn(self, state, source_column, target_column):
        targetCatalog = state.targetCatalog

        mysql_simpleTypes = dict( (datatype.name.upper(), datatype) for datatype in targetCatalog.simpleDatatypes )

        source_type = source_column.simpleType
        if not source_type and source_column.userType:
            # evaluate user type
            source_type = source_column.userType.actualType

            if not source_type and source_column.userType.sqlDefinition.startswith('enum('):
                target_column.simpleType = mysql_simpleTypes['ENUM']
                target_column.datatypeExplicitParams = source_column.userType.sqlDefinition[4:]
                return True

            target_column.flags.extend(source_column.userType.flags)

        if source_type:
            # Decide which mysql datatype corresponds to the column datatype:
            source_datatype = source_type.name.upper()
            # string data types:
            target_datatype = ''
            if source_datatype == 'VARCHAR':
                if 0 <= source_column.length < 256:
                    target_datatype = 'VARCHAR'
                elif 0 <= source_column.length < 65536:  # MySQL versions > 5.0 can hold up to 65535 chars in a VARCHAR column
                    if targetCatalog.version.majorNumber < 5:
                        target_datatype = 'MEDIUMTEXT'
                    else:
                        target_datatype = 'VARCHAR'
                else:
                    target_datatype = 'LONGTEXT'
            elif source_datatype == 'CHAR':
                if source_column.length < 256:
                    target_datatype = 'CHAR'
                else:
                    target_datatype = 'LONGTEXT'
            # integer data types:
            elif source_datatype == 'BIT':
                target_datatype = 'TINYINT'
            elif source_datatype == 'INTEGER':
                target_datatype = 'INT'
            elif source_datatype in ['SMALLINT', 'INT', 'BIGINT']:
                target_datatype = source_datatype
                target_column.precision = -1
            # numeric
            elif source_datatype in ['DECIMAL', 'NUMERIC']:
                target_datatype = 'DECIMAL'
            elif source_datatype == 'SMALLMONEY':
                target_datatype = 'DECIMAL'
                target_column.precision = 10
                target_column.scale = 4
            elif source_datatype == 'MONEY':
                target_datatype = 'DECIMAL'
                target_column.precision = 19
                target_column.scale = 4
            # floating point datatypes:
            elif source_datatype == 'REAL':
                target_datatype = 'FLOAT'
            elif source_datatype == 'DOUBLE PRECISION':
                target_datatype = 'DOUBLE'
            # binary datatypes:
            elif source_datatype == 'VARBIT':
                if 1 <= source_column.length <= 64:
                    target_datatype = 'BIT'
                else:
                    target_datatype = 'LONGBLOB'
            elif source_datatype == 'LONG VARBIT':
                target_datatype = 'LONGBLOB'
            elif source_datatype in ['XML', 'TEXT', 'NTEXT']:
                target_datatype = 'LONGTEXT'
            elif source_datatype == 'BINARY':
                if 1 <= source_column.length <= 8:
                    target_datatype = 'TINYBLOB'
                elif 8 < source_column.length <= 16:
                    target_datatype = 'BLOB'
                elif 16 < source_column.length <= 24:
                    target_datatype = 'MEDIUMBLOB'
                else:
                    target_datatype = 'LONGBLOB'
            elif source_datatype == 'LONG BINARY':
                target_datatype = 'LONGBLOB'
            # datetime datatypes:
            elif source_datatype == 'TIMESTAMP':
                target_datatype = 'DATETIME'
            elif source_datatype == 'DATE':
                target_datatype = 'DATE'
            elif source_datatype == 'TIME':
                target_datatype = 'TIME'
            elif source_datatype == 'DATETIMEOFFSET':
                target_datatype = 'TIME'
                state.addMigrationLogEntry(0, source_column, target_column,
                      "Source column type DATETIMEOFFSET was migrated to TIME")
            # others
            elif source_datatype == 'UNIQUEIDENTIFIERSTR':
                target_datatype = 'VARCHAR'
                target_column.length = 36
            elif source_datatype.startswith('ST_'):  # Spatial datatypes
                target_datatype = 'VARCHAR'
            else:
                # just fall back to same type name and hope for the best
                target_datatype = source_datatype

            if mysql_simpleTypes.has_key(target_datatype):
                target_column.simpleType = mysql_simpleTypes[target_datatype]
            else:
                grt.log_warning("SQLAnywhere migrateTableColumnsToMySQL", "Can't find datatype %s for type %s\n" % (target_datatype, source_datatype))
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
        return target_catalog


    def migrateTableForeignKeyToMySQL(self, state, source_fk, targetTable):
        target_fk = super(SQLAnywhereMigration, self).migrateTableForeignKeyToMySQL(state, source_fk, targetTable)
        if target_fk:
            for column, referenced_column in zip(target_fk.columns, target_fk.referencedColumns):
                if column.simpleType != referenced_column.simpleType or column.length != referenced_column.length:
                    state.addMigrationLogEntry(1, source_fk, target_fk,
                          'The column %s.%s references %s.%s but its data type is %s instead of %s. '
                          'Data type changed to %s.' % (column.owner.name, column.name,
                                                       referenced_column.owner.name, referenced_column.name,
                                                       column.formattedType, referenced_column.formattedType, referenced_column.formattedType
                                                      )
                                              )
                    if column.simpleType != referenced_column.simpleType:
                        column.owner.customData['columnTypeCastExpression:%s' % column.name] = '?::%s as ?' % referenced_column.simpleType.name
                    column.simpleType = referenced_column.simpleType
                    column.length = referenced_column.length
                if column.isNotNull:
                    if target_fk.updateRule == 'SET NULL':
                        target_fk.updateRule = 'NO ACTION'
                        state.addMigrationLogEntry(1, source_fk, target_fk,
                            'Cannot have a SET NULL update rule: referencing column %s.%s does not allow nulls. '
                            'Update rule changed to NO ACTION' % (column.owner.name, column.name)
                                                  )
                    if target_fk.deleteRule == 'SET NULL':
                        target_fk.deleteRule = 'NO ACTION'
                        state.addMigrationLogEntry(1, source_fk, target_fk,
                            'Cannot have a SET NULL delete rule: referencing column %s.%s does not allow nulls. '
                            'Delete rule changed to NO ACTION' % (column.owner.name, column.name)
                                                  )
        return target_fk

instance = SQLAnywhereMigration()

@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return 'SQLAnywhere'

@ModuleInfo.export(grt.STRING, grt.STRING, grt.classes.GrtLogObject)
def migrateIdentifier(name, log):
    return instance.migrateIdentifier(name, log)


@ModuleInfo.export(grt.classes.db_Catalog, grt.classes.db_migration_Migration, grt.classes.db_Catalog)
def migrateCatalog(state, source_catalog):
    return instance.migrateCatalog(state, source_catalog)


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

    param = grt.classes.db_migration_MigrationParameter()
    param.name = "sqlanywhere:migrateTimestampAsDatetime"
    param.caption = "Migrate TIMESTAMP values as DATETIME by default. TIMESTAMP values in MySQL have a limited time range."
    param.paramType = "boolean"
    list.append(param)

    return list
