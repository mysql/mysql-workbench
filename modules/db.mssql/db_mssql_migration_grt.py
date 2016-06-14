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

import re
import string

from wb import DefineModule
import grt

from db_generic_migration_grt import GenericMigration
from workbench.utils import find_object_with_name, Version

ModuleInfo = DefineModule(name= "DbMssqlMigration", author= "Oracle Corp.", version="1.0")

# unfinished
MSSQL_COLLATION_MAP= [
('SQL_Latin1_General_Cp437_BIN', 'utf8_general_ci', False),
('SQL_Latin1_General_Cp437_CS_AS', 'utf8_general_ci', False),
('SQL_Latin1_General_Cp437_CI_AS', 'utf8_general_ci', False),
('SQL_Latin1_General_Pref_CP437_CI_AS', 'utf8_general_ci', False),
('SQL_Latin1_General_Cp437_CI_AI', 'utf8_general_ci', False),
('SQL_Latin1_General_Cp850_BIN', 'utf8_bin', True),
('SQL_Latin1_General_Cp850_CS_AS', 'utf8_general_ci', False),
('SQL_Latin1_General_Cp850_CI_AS', 'utf8_general_ci', False),
('SQL_Latin1_General_Pref_CP850_CI_AS', 'utf8_general_ci', False),
('SQL_Latin1_General_Cp850_CI_AI', 'utf8_general_ci', False),
('SQL_1Xcompat_CP850_CI_AS', 'utf8_general_ci', False),
('Latin1_General_BIN', 'latin1_bin', True),
('SQL_Latin1_General_Cp1_CS_AS', 'utf8_general_ci', False),
('SQL_Latin1_General_Cp1_CI_AS', 'utf8_general_ci', True),
('SQL_Latin1_General_Pref_CP1_CI_AS', 'utf8_general_ci', False),
('SQL_Latin1_General_Cp1_CI_AI', 'utf8_general_ci', True),
('SQL_AltDiction_Cp850_CS_AS', 'utf8_general_ci', False),
('SQL_AltDiction_Pref_CP850_CI_AS', 'utf8_general_ci', False),
('SQL_AltDiction_Cp850_CI_AI', 'utf8_general_ci', False),
('SQL_Scandinavian_Pref_Cp850_CI_AS', 'utf8_swedish_ci', False),
('SQL_Scandinavian_Cp850_CS_AS', 'utf8_swedish_ci', False),
('SQL_Scandinavian_Cp850_CI_AS', 'utf8_swedish_ci', False),
('SQL_AltDiction_Cp850_CI_AS', 'utf8_general_ci', False),
('Latin1_General_CS_AS', 'utf8_general_ci', False),
('Latin1_General_CI_AS', 'utf8_general_ci', False),
('Danish_Norwegian_CS_AS', 'utf8_danish_ci', False),
('Finnish_Swedish_CS_AS', 'utf8_swedish_ci', False),
('Icelandic_CS_AS', 'utf8_icelandic_ci', False),
('Hungarian_BIN', 'utf8_bin', True),
('SQL_Latin1_General_Cp1250_CS_AS', 'cp1250_general_ci', False),
('SQL_Latin1_General_Cp1250_CI_AS', 'cp1250_general_ci', True),
('SQL_Czech_Cp1250_CS_AS', 'cp1250_czech_cs', True),
('SQL_Czech_Cp1250_CI_AS', 'cp1250_czech_cs', False),
('SQL_Hungarian_Cp1250_CS_AS', 'cp1250_general_ci', False),
('SQL_Hungarian_Cp1250_CI_AS', 'cp1250_general_ci', False),
('SQL_Polish_Cp1250_CS_AS', 'sql_polish_cp1250_cs_as', False),
('SQL_Polish_Cp1250_CI_AS', 'sql_polish_cp1250_ci_as', False),
('SQL_Romanian_Cp1250_CS_AS', 'sql_romanian_cp1250_cs_as', False),
('SQL_Romanian_Cp1250_CI_AS', 'sql_romanian_cp1250_ci_as', False),
('SQL_Croatian_Cp1250_CS_AS', 'sql_croatian_cp1250_cs_as', False),
('SQL_Croatian_Cp1250_CI_AS', 'sql_croatian_cp1250_ci_as', False),
('SQL_Slovak_Cp1250_CS_AS', 'sql_slovak_cp1250_cs_as', False),
('SQL_Slovak_Cp1250_CI_AS', 'sql_slovak_cp1250_ci_as', False),
('SQL_Slovenian_Cp1250_CS_AS', 'sql_slovenian_cp1250_cs_as', False),
('SQL_Slovenian_Cp1250_CI_AS', 'sql_slovenian_cp1250_ci_as', False),
('Cyrillic_General_BIN', 'cyrillic_general_bin', False),
('SQL_Latin1_General_Cp1251_CS_AS', 'sql_latin1_general_cp1251_cs_as', False),
('SQL_Latin1_General_Cp1251_CI_AS', 'sql_latin1_general_cp1251_ci_as', False),
('SQL_Ukrainian_Cp1251_CS_AS', 'sql_ukrainian_cp1251_cs_as', False),
('SQL_Ukrainian_Cp1251_CI_AS', 'sql_ukrainian_cp1251_ci_as', False),
('Greek_BIN', 'greek_bin', False),
('SQL_Latin1_General_Cp1253_CS_AS', 'sql_latin1_general_cp1253_cs_as', False),
('SQL_Latin1_General_Cp1253_CI_AS', 'sql_latin1_general_cp1253_ci_as', False),
('SQL_MixDiction_Cp1253_CS_AS', 'sql_mixdiction_cp1253_cs_as', False),
('SQL_AltDiction_Cp1253_CS_AS', 'sql_altdiction_cp1253_cs_as', False),
('SQL_Latin1_General_Cp1253_CI_AI', 'sql_latin1_general_cp1253_ci_ai', False),
('Turkish_BIN', 'turkish_bin', False),
('SQL_Latin1_General_Cp1254_CS_AS', 'sql_latin1_general_cp1254_cs_as', False),
('SQL_Latin1_General_Cp1254_CI_AS', 'sql_latin1_general_cp1254_ci_as', False),
('Hebrew_BIN', 'hebrew_bin', False),
('SQL_Latin1_General_Cp1255_CS_AS', 'sql_latin1_general_cp1255_cs_as', False),
('SQL_Latin1_General_Cp1255_CI_AS', 'sql_latin1_general_cp1255_ci_as', False),
('Arabic_BIN', 'arabic_bin', False),
('SQL_Latin1_General_Cp1256_CS_AS', 'sql_latin1_general_cp1256_cs_as', False),
('SQL_Latin1_General_Cp1256_CI_AS', 'sql_latin1_general_cp1256_ci_as', False),
('SQL_Latin1_General_Cp1257_CS_AS', 'sql_latin1_general_cp1257_cs_as', False),
('SQL_Latin1_General_Cp1257_CI_AS', 'sql_latin1_general_cp1257_ci_as', False),
('SQL_Estonian_Cp1257_CS_AS', 'sql_estonian_cp1257_cs_as', False),
('SQL_Estonian_Cp1257_CI_AS', 'sql_estonian_cp1257_ci_as', False),
('SQL_Latvian_Cp1257_CS_AS', 'sql_latvian_cp1257_cs_as', False),
('SQL_Latvian_Cp1257_CI_AS', 'sql_latvian_cp1257_ci_as', False),
('SQL_Lithuanian_Cp1257_CS_AS', 'sql_lithuanian_cp1257_cs_as', False),
('SQL_Lithuanian_Cp1257_CI_AS', 'sql_lithuanian_cp1257_ci_as', False),
('SQL_Danish_Pref_Cp1_CI_AS', 'sql_danish_pref_cp1_ci_as', False),
('SQL_SwedishPhone_Pref_Cp1_CI_AS', 'sql_swedishphone_pref_cp1_ci_as', False),
('SQL_SwedishStd_Pref_Cp1_CI_AS', 'sql_swedishstd_pref_cp1_ci_as', False),
('SQL_Icelandic_Pref_Cp1_CI_AS', 'sql_icelandic_pref_cp1_ci_as', False),
('Japanese_BIN', 'japanese_bin', False),
('Japanese_CI_AS', 'japanese_ci_as', False),
('Korean_Wansung_BIN', 'korean_wansung_bin', False),
('Korean_Wansung_CI_AS', 'korean_wansung_ci_as', False),
('Chinese_Taiwan_Stroke_BIN', 'chinese_taiwan_stroke_bin', False),
('Chinese_Taiwan_Stroke_CI_AS', 'chinese_taiwan_stroke_ci_as', False),
('Chinese_PRC_BIN', 'chinese_prc_bin', False),
('Chinese_PRC_CI_AS', 'chinese_prc_ci_as', False),
('Japanese_CS_AS', 'japanese_cs_as', False),
('Korean_Wansung_CS_AS', 'korean_wansung_cs_as', False),
('Chinese_Taiwan_Stroke_CS_AS', 'chinese_taiwan_stroke_cs_as', False),
('Chinese_PRC_CS_AS', 'chinese_prc_cs_as', False),
('Thai_BIN', 'thai_bin', False),
('Thai_CI_AS', 'thai_ci_as', False),
('Thai_CS_AS', 'thai_cs_as', False),
('SQL_EBCDIC037_CP1_CS_AS', 'sql_ebcdic037_cp1_cs_as', False),
('SQL_EBCDIC273_CP1_CS_AS', 'sql_ebcdic273_cp1_cs_as', False),
('SQL_EBCDIC277_CP1_CS_AS', 'sql_ebcdic277_cp1_cs_as', False),
('SQL_EBCDIC278_CP1_CS_AS', 'sql_ebcdic278_cp1_cs_as', False),
('SQL_EBCDIC280_CP1_CS_AS', 'sql_ebcdic280_cp1_cs_as', False),
('SQL_EBCDIC284_CP1_CS_AS', 'sql_ebcdic284_cp1_cs_as', False),
('SQL_EBCDIC285_CP1_CS_AS', 'sql_ebcdic285_cp1_cs_as', False),
('SQL_EBCDIC297_CP1_CS_AS', 'sql_ebcdic297_cp1_cs_as', False)
]

truncated_identifier_serial = 0
class MSSQLMigration(GenericMigration):
    def migrateIdentifier(self, mssql_name, log, dots_allowed=False):
        mysql_valid_regex = re.compile(r'^[^/\\.]+$', re.U)  # Database and table names cannot contain "/", "\", ".", or characters that are not permitted in file names
        mysql_name = mssql_name
        log_message = ''
        # Remove quoting chars from the identifier if present
        if ( mssql_name.startswith('"') and mssql_name.endswith('"') or
             mssql_name.startswith('[') and mssql_name.endswith(']') ):
            mysql_name = mssql_name[1:-1]
        if not dots_allowed and not mysql_valid_regex.match(mysql_name):
            mysql_name = mysql_name.translate(string.maketrans(r'/\.', '___'))
            log_message = (u'Schema and table names cannot contain "/", "\\", ".", or characters that are '
                           u'not permitted in file names. The identifier [%s] was changed to `%s`. ' % (mssql_name, mysql_name))
        
        # truncate too long identifiers
        if len(mysql_name) > 64:
            global truncated_identifier_serial
            truncated_identifier_serial += 1
            original_mysql_name = mysql_name
            mysql_name = mysql_name[:62]+str(truncated_identifier_serial)
            log_message += u'Identifier `%s` is too long, truncated to `%s`' % (original_mysql_name, mysql_name)

        if log and log_message:
            entry = grt.classes.GrtLogEntry()
            entry.entryType = 1
            entry.name = log_message
            log.entries.append(entry)
        
        return mysql_name

    def migrateTableToMySQL(self, state, sourceTable, targetSchema):
        targetTable = super(MSSQLMigration, self).migrateTableToMySQL(state, sourceTable, targetSchema)
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
                    target_column.length = -1
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
            target_version = Version.fromgrt(targetCatalog.version)
            # Decide which mysql datatype corresponds to the column datatype:
            source_datatype = source_type.name.upper()
            grt.log_debug3("Migration", "Migrating source column '%s' - type: %s, length: %s\n" % (source_column.name, source_datatype,source_column.length))
            # string data types:
            target_datatype = ''
            #NCHAR and NVARCHAR in Microsoft SQL Server is always encoded as UCS-2 (UTF-16)
            if source_datatype in ['NCHAR', 'NVARCHAR'] and target_version.is_supported_mysql_version_at_least(5,5,0):
                target_column.characterSetName = 'utf8mb4'
            if source_datatype in ['VARCHAR', 'NVARCHAR']:
                if source_column.length == -1:  # VARCHAR(MAX) or NVARCHAR(MAX)
                    target_datatype = 'LONGTEXT'  #TODO: Give the user the choice for this target datatype
                elif 0 < source_column.length < 256:
                    target_datatype = 'VARCHAR'
                else:  # MySQL versions > 5.0 can hold up to 65535 chars in a VARCHAR column
                    target_datatype = 'TEXT' if targetCatalog.version.majorNumber < 5 else 'VARCHAR'
            elif source_datatype in ['TEXT', 'NTEXT']:
                target_datatype = 'LONGTEXT'
            elif source_datatype in ['CHAR', 'NCHAR']:  # MSSQL CHAR's (also VARCHAR's) max length is 8000 non Unicode characters
                if 0 < source_column.length < 256:
                    target_datatype = 'CHAR'
                else:
                    target_datatype = 'TEXT' 
            # integer data types:
            elif source_datatype in ['BIGINT', 'INT', 'SMALLINT']:
                target_datatype = source_datatype
                target_column.precision = -1
            elif source_datatype == 'TINYINT':
                target_datatype = source_datatype
                target_column.precision = -1
                if 'UNSIGNED' not in target_column.flags:
                    target_column.flags.append('UNSIGNED')  # In MSSQL TINYINT is unsigned
            elif source_datatype == 'UNIQUEIDENTIFIER':
                target_datatype = 'VARCHAR'
                target_column.length = 64
                if 'UNIQUE' not in target_column.flags:
                    target_column.flags.append('UNIQUE') # uniqueid must be UNIQUE... bug #43098
                state.addMigrationLogEntry(0, source_column, target_column,
                        "Source column type %s was migrated to %s(%s)" % (source_datatype, target_datatype, target_column.length))
            elif source_datatype == 'SYSNAME':  # the relevant info is in http://msdn.microsoft.com/en-us/library/ms191240(v=sql.105).aspx
                target_datatype = 'VARCHAR'
                target_column.length = 160
                state.addMigrationLogEntry(0, source_column, target_column,
                        "Source column type %s was migrated to %s(%s)" % (source_datatype, target_datatype, target_column.length))
            # floating point datatypes:
            elif source_datatype in ['DECIMAL', 'NUMERIC']:
                if source_column.scale == 0:
                    target_datatype = 'BIGINT'
                    if source_column.precision < 5:
                        target_datatype = 'SMALLINT'
                    elif source_column.precision < 7:
                        target_datatype = 'MEDIUMINT'
                    elif source_column.precision < 10:
                        target_datatype = 'INT'
                    target_column.precision = -1
                else:
                    target_datatype = 'DECIMAL'
            elif source_datatype == 'REAL':
                target_datatype = 'FLOAT'
            elif source_datatype == 'FLOAT':
                if source_column.precision > 24:
                    target_datatype = 'DOUBLE'
                    target_column.precision = -1
            elif source_datatype in ['MONEY', 'SMALLMONEY']:
                target_datatype = 'DECIMAL'
                target_column.precision = source_column.simpleType.numericPrecision
                target_column.scale = source_column.simpleType.numericScale
            # binary datatypes:
            elif source_datatype == 'IMAGE':
                target_datatype = 'LONGBLOB'
            elif source_datatype == 'VARBINARY' and source_column.length == -1:  # VARBINARY(MAX):
                target_datatype = 'LONGBLOB'
            # datetime datatypes:
            elif source_datatype in ['DATETIME', 'SMALLDATETIME', 'DATETIME2', 'DATETIMEOFFSET']:
                target_datatype = 'DATETIME'
                target_column.precision = -1
                if target_version.is_supported_mysql_version_at_least(5,6,4) and source_datatype != 'SMALLDATETIME':
                    target_column.precision = source_column.precision if source_column.precision < 7 else 6
            # timestamp datatypes
            # In MS SQL Server a nonnullable timestamp column is semantically equivalent to a binary(8) column, 
            # and a nullable timestamp column is semantically equivalent to a varbinary(8) column.
            elif source_datatype in ['TIMESTAMP', 'ROWVERSION']:
                target_datatype = 'BINARY' if source_column.isNotNull else 'VARBINARY'
            elif source_datatype == 'DATE':
                target_datatype = 'DATE'
                target_column.precision = -1
            elif source_datatype == 'TIME':
                target_datatype = 'TIME'
                target_column.precision = -1
                if target_version.is_supported_mysql_version_at_least(5,6,4):
                    target_column.precision = source_column.precision if source_column.precision < 7 else 6
            elif source_datatype == 'BIT':
                target_datatype = 'TINYINT'
                target_column.length = 1
                state.addMigrationLogEntry(0, source_column, target_column,
                      "Source column type BIT was migrated to TINYINT(1)")
            elif source_datatype == 'XML':
                target_datatype = 'TEXT'
                state.addMigrationLogEntry(0, source_column, target_column,
                      "Source column type XML was migrated to TEXT")
            elif source_datatype in ['GEOMETRY', 'GEOGRAPHY']:
                target_datatype = 'GEOMETRY'
            elif source_datatype == 'HIERARCHYID':
                target_datatype = 'VARCHAR'
                target_column.length = 255
                state.addMigrationLogEntry(1, source_column, target_column,
                        "Source column type HIERARCHYID was migrated to VARCHAR(255)")
            elif source_datatype == 'SQL_VARIANT':
                target_datatype = 'TEXT'
                state.addMigrationLogEntry(1, source_column, target_column,
                        "Source column type %s was migrated to %s(%s)" % (source_datatype, target_datatype, target_column.length))
            else:
                # just fall back to same type name and hope for the best
                target_datatype = source_datatype

            if mysql_simpleTypes.has_key(target_datatype):
                target_column.simpleType = mysql_simpleTypes[target_datatype]
            else:
                grt.log_warning("Migration", "MSSQL migrateTableColumnsToMySQL", "Can't find datatype %s for type %s\n" % (target_datatype, source_datatype))
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
                        target_datatype = target_column.simpleType.name.upper()
                        if source_datatype in ['VARCHAR', 'NVARCHAR']:
                            if source_datatype == 'VARCHAR':
                                if target_column.length > 4000 or target_column.length == -1: # NVARCHAR is limited to 4000 - if you need more, you must use MAX insted BUG #18167872
                                    type_cast_expression = "CAST(? as NVARCHAR(MAX))"         # If in source table is VARCHAR(MAX) column lnegth is 0 - so must be casted to NVARCHAR(MAX) BUG #18105486
                                else:
                                    type_cast_expression = "CAST(? as NVARCHAR(%d))" % target_column.length
                        elif source_datatype in ['TEXT', 'NTEXT']:
                            if source_datatype == 'TEXT':
                                type_cast_expression = "CAST(? as NTEXT)"
                        elif source_datatype in ['CHAR', 'NCHAR']:  # MSSQL CHAR's (also VARCHAR's) max length is 8000 non Unicode characters
                            if source_datatype == 'CHAR':
                                type_cast_expression = "CAST(? as NCHAR(%d))" % target_column.length
                        elif source_datatype == 'UNIQUEIDENTIFIER':
                            type_cast_expression = "CAST(? as VARCHAR(64))"
                        elif source_datatype == 'SYSNAME':  # the relevant info is in http://msdn.microsoft.com/en-us/library/ms191240(v=sql.105).aspx
                            type_cast_expression = "CAST(? as VARCHAR(128))"
                        # floating point datatypes:
                        elif source_datatype in ['DECIMAL', 'NUMERIC']:
                            if source_column.scale == 0:
                                type_cast_expression = "CAST(? as %s)" % ('INT' if target_datatype == 'MEDIUMINT' else target_datatype)
                        elif source_datatype == 'XML':
                            type_cast_expression = "CAST(? as NVARCHAR(max))"
                        elif source_datatype in ['GEOMETRY', 'GEOGRAPHY']:
                            type_cast_expression = '?.STAsText()'
                        elif source_datatype == 'HIERARCHYID':
                            type_cast_expression = "CAST(? as VARCHAR(max))"
                        elif source_datatype == 'SQL_VARIANT':
                            type_cast_expression = "CAST(? as NVARCHAR(max))"
                        elif source_datatype in ['BINARY', 'VARBINARY', 'TIMESTAMP', 'ROWVERSION']:
                            type_cast_expression = 'CONVERT(VARBINARY(MAX), ?, 0)'

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



instance = MSSQLMigration()
@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return 'Mssql'

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

    #param = grt.classes.db_migration_MigrationParameter()
    #param.name = "mssql:mergeSchemaNameToTableName"
    #param.caption = "Merge source schema and table names into a single target schema\nex.: AdventureWorks.Person.Contact -> AdventureWorks.Person_Contact"
    #list.append(param)

    return list


