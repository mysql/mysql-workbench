# Copyright (c) 2012, 2015, Oracle and/or its affiliates. All rights reserved.
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

from db_generic_migration_grt import GenericMigration

ModuleInfo = DefineModule(name= "DbMySQLMigration", author= "Oracle Corp.", version="1.0")

class MySQLMigration(GenericMigration):
    def migrateIdentifier(self, name, log, dots_allowed=True):
        return name

    def migrateDatatypeForColumn(self, state, source_column, target_column):
        def find_object_with_id(list, oid):
            for o in list:
                if o.__id__ == oid:
                    return o
            return None

        if source_column.simpleType:
            target_column.simpleType = find_object_with_id(source_column.owner.owner.owner.simpleDatatypes, source_column.simpleType.__id__)
            return target_column.simpleType is not None
        else:
            # migration error
            state.addMigrationLogEntry(2, source_column, target_column, 
                    'migrateTableColumnToMySQL: Cannot migrate column %s.%s because migration of its datatype is unsupported' % (source_column.owner.name, source_column.name))
            return False

            
    def migrateColumnDefaultValue(self, state, default_value, source_column, target_column):
        return default_value
        

    def migrateTableToMySQL(self, state, sourceTable, target_schema):
        targetTable = GenericMigration.migrateTableToMySQL(self, state, sourceTable, target_schema)

        # MySQL attributes
        for attr in ["tableEngine", "nextAutoInc", "password", "delayKeyWrite", "defaultCharacterSetName", "defaultCollationName", "mergeUnion", "mergeInsert",
                      "tableDataDir", "tableIndexDir", "packKeys", "raidType", "raidChunks", "raidChunkSize", "checksum", "rowFormat", "keyBlockSize", "avgRowLength", "minRows", "maxRows",
                      "partitionType", "partitionExpression", "partitionCount", "subpartitionType", "subpartitionExpression", "subpartitionCount"]:
            setattr(targetTable, attr, getattr(sourceTable, attr))

        if True:
            def copy_partitions(owner, part_list):
                l = []
                for src in part_list:
                    dst = grt.classes.db_mysql_PartitionDefinition()
                    for attr in ["name", "value", "comment", "dataDirectory", "indexDirectory", "maxRows", "minRows"]:
                        setattr(dst, attr, getattr(src, attr))
                    dst.owner = owner
                    dst.subpartitionDefinitions.extend(copy_partitions(dst, src.subpartitionDefinitions))
                    l.append(dst)
                return l
            # partition defs
            targetTable.partitionDefinitions.extend(copy_partitions(targetTable, sourceTable.partitionDefinitions))

        return targetTable


    def migrateTableColumnToMySQL(self, state, source_column, targetTable):
        target_column = GenericMigration.migrateTableColumnToMySQL(self, state, source_column, targetTable)
        # MySQL specific
        for attr in ["autoIncrement", "expression", "generated", "generatedStorage"]:
            setattr(target_column, attr, getattr(source_column, attr))

        return target_column


    def migrateTriggerToMySQL(self, state, source_trigger, target_table):
        target_trigger = GenericMigration.migrateTriggerToMySQL(self, state, source_trigger, target_table)
        target_trigger.commentedOut = 0
        return target_trigger


    def migrateViewToMySQL(self, state, source_view, target_schema):
        target_view = GenericMigration.migrateViewToMySQL(self, state, source_view, target_schema)
        target_view.commentedOut = 0
        return target_view


    def migrateRoutineToMySQL(self, state, source_routine, target_schema):
        target_routine = GenericMigration.migrateRoutineToMySQL(self, state, source_routine, target_schema)
        target_routine.commentedOut = 0
        return target_routine


instance = MySQLMigration()

@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return 'Mysql'

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
