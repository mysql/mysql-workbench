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

ModuleInfo = DefineModule(name= "DbSql92Migration", author= "Oracle Corp.", version="1.0")

truncated_identifier_serial = 0

class Sql92Migration(GenericMigration):
    def migrateTableToMySQL(self, state, sourceTable, targetSchema):
        targetTable = super(Sql92Migration, self).migrateTableToMySQL(state, sourceTable, targetSchema)
        targetTable.defaultCharacterSetName, targetTable.defaultCollationName =  self.migrateCharsetCollation(state, sourceTable.owner.defaultCharacterSetName, sourceTable.owner.defaultCollationName, sourceTable, targetTable)
        return targetTable

    def migrateColumnDefaultValue(self, state, default_value, source_column, target_column):
        target_default_value = default_value

        def raise_on_no_match(re_str, target):
            if re.match(re_str, target) is None:
                raise ValueError('"%s" does not match the regular expression "%s"' % (target, re_str))
            return True

        value_validators = [
            (['SMALLINT', 'INT', 'BIGINT'], int),
            (['NUMERIC', 'DECIMAL', 'FLOAT', 'REAL', 'DOUBLE PRECISION'], float),
            (['CHAR', 'VARCHAR', 'NCHAR', 'NVARCHAR', 'BLOB', 'CLOB', 'XML'], lambda _:True),
            (['BIT', 'BIT VARYING'], lambda val: raise_on_no_match(r"[Bb]?'?[10]+'?", val) ),  # E.g. B'101001'
            (['DATE'], lambda val: raise_on_no_match(r"(\d{4}|\d{2})-\d{1,2}-\d{1,2}", val) ),
            (['TIME'], lambda val: raise_on_no_match(r"(\d{1,2} )?\d{1,2}:\d{0,2}:\d{0,2}", val) ),
            (['TIMESTAMP'], lambda val: raise_on_no_match(
                r"((\d{4}|\d{2})-\d{1,2}-\d{1,2}( (\d{1,2} )?\d{1,2}:\d{0,2}:\d{0,2})?|CURRENT_TIMESTAMP|now\(\))", val) ),
            (['BOOLEAN'], lambda val: raise_on_no_match(r'(TRUE|FALSE)', val) ),
        ]

        if source_column.simpleType:
            source_datatype = source_column.simpleType.name
            if default_value:
                for value_validator in value_validators:
                    if source_datatype in value_validator[0]:
                        try:
                            value_validator[1](default_value)
                        except Exception:
                            target_default_value = ''
                            state.addMigrationLogEntry(1, source_column, target_column, 
                                      'Default value %s is not supported. Removed!' % default_value)
                        else:
                            if source_datatype == 'TIMESTAMP' and default_value.upper() == 'NOW()':
                                target_default_value = 'CURRENT_TIMESTAMP'
                    
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
            if source_datatype in ['VARCHAR', 'NVARCHAR']:
                if 0 <= source_column.length < 256:
                    target_datatype = 'VARCHAR'
                elif 0 <= source_column.length < 65536:  # MySQL versions > 5.0 can hold up to 65535 chars in a VARCHAR column
                    if targetCatalog.version.majorNumber < 5:
                        target_datatype = 'MEDIUMTEXT'
                    else:
                        target_datatype = 'VARCHAR'
                else:
                    target_datatype = 'LONGTEXT'
            elif source_datatype in ['CHAR', 'NCHAR']:
                if source_column.length < 256:
                    target_datatype = 'CHAR'
                else:
                    target_datatype = 'LONGTEXT'
            # integer data types:
            elif source_datatype in ['BIGINT', 'INT', 'SMALLINT']:
                target_datatype = source_datatype
                target_column.precision = -1
            # floating point datatypes:
            elif source_datatype in ['DECIMAL', 'NUMERIC']:
                target_datatype = 'DECIMAL'
            elif source_datatype in ['REAL', 'FLOAT']:
                target_datatype = 'FLOAT'
            elif source_datatype == 'DOUBLE PRECISION':
                target_datatype = 'DOUBLE'
            # binary datatypes:
            elif source_datatype == 'BLOB':
                if 0 <= source_column.length < 2**8:
                    target_datatype = 'TINYBLOB'
                if 0 <= source_column.length < 2**16:
                    target_datatype = 'BLOB'
                elif 0 <= source_column.length < 2**24:
                    target_datatype = 'MEDIUMBLOB'
                else:
                    target_datatype = 'LONGBLOB'
            elif source_datatype == 'CLOB':
                if 0 <= source_column.length < 2**8:
                    target_datatype = 'TINYTEXT'
                if 0 <= source_column.length < 2**16:
                    target_datatype = 'TEXT'
                elif 0 <= source_column.length < 2**24:
                    target_datatype = 'MEDIUMTEXT'
                else:
                    target_datatype = 'LONGTEXT'
            # datetime datatypes:
            elif source_datatype == 'TIMESTAMP':
                target_datatype = 'TIMESTAMP'
            elif source_datatype == 'DATE':
                target_datatype = 'DATE'
            elif source_datatype == 'TIME':
                target_datatype = 'TIME'
            elif source_datatype in ['BIT', 'BIT VARYING']:
                target_datatype = 'BIT'
            elif source_datatype == 'BOOLEAN':
                target_datatype = 'TINYINT'
                target_column.length = 1
                state.addMigrationLogEntry(0, source_column, target_column,
                      "Source column type BOOLEAN was migrated to TINYINT(1)")
            elif source_datatype == 'XML':
                target_datatype = 'TEXT'
                state.addMigrationLogEntry(0, source_column, target_column,
                      "Source column type XML was migrated to TEXT")
            else:
                # just fall back to same type name and hope for the best
                target_datatype = source_datatype

            if mysql_simpleTypes.has_key(target_datatype):
                target_column.simpleType = mysql_simpleTypes[target_datatype]
            else:
                grt.log_warning("SQL-92 migrateTableColumnsToMySQL", "Can't find datatype %s for type %s\n" % (target_datatype, source_datatype))
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
                        if source_datatype == 'XML':
                            type_cast_expression = "CAST(? as NVARCHAR(max))"

                        if type_cast_expression:
                            target_column.owner.customData["columnTypeCastExpression:%s" % target_column.name] = "%s as ?" % type_cast_expression

        return target_catalog


    def migrateCharsetCollation(self, state, charset, collation, source_object, target_object):
        if collation:
            state.addMigrationLogEntry(0, source_object, target_object, 
                    'Collation %s migrated to utf8_general_ci' % (collation))
            return '', 'utf8_general_ci'

        return charset, collation
    

instance = Sql92Migration()

@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return 'Sql92'

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

    #param = grt.classes.db_migration_MigrationParameter()
    #param.name = "generic:mergeSchemaNameToTableName"
    #param.caption = "Treat source catalogs as schemas in MySQL and merge schema and table names\nex.: MyCatalog.MySchema.MyTable -> MyCatalog.MySchema_MyTable"
    #list.append(param)

    return list
