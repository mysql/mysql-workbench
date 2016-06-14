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
from workbench.utils import find_object_with_old_name

import grt

MYSQL_MAX_INDEX_KEY_LENGTH_INNODB_UTF8 = 767/3
MYSQL_MAX_INDEX_KEY_LENGTH_INNODB_LATIN1 = 767
MYSQL_MAX_INDEX_KEY_LENGTH_NDBCLUSTER = 0

ModuleInfo = DefineModule(name= "DbGenericMigration", author= "Oracle Corp.", version="1.0")

truncated_identifier_serial = 0
key_names = {}
class GenericMigration(object):
    ## Note: do not add member variables in this class or subclasses

    def findMatchingTargetObject(self, state, sourceObject):
        """Finds the matching target object for a given source object, by searching in the migrationLog"""
        for log in state.migrationLog:
            if type(log.logObject) == type(sourceObject)\
               and type(log.logObject.owner) == type(sourceObject.owner)\
               and log.logObject.owner.name == sourceObject.owner.name\
               and log.logObject.name == sourceObject.name:
                return log.refObject
        return None
        
    def findDatatypeMapping(self, state, column, datatype):
        if state.genericDatatypeMappings:
            for typemap in state.genericDatatypeMappings:
                if typemap.sourceDatatypeName.upper() == datatype.upper():
                    if typemap.lengthConditionFrom > 0 and column.length > 0 and not (typemap.lengthConditionFrom <= column.length):
                        continue
                    if typemap.lengthConditionTo > 0 and column.length > 0 and not (typemap.lengthConditionTo >= column.length):
                        continue
                    #if typemap.precisionConditionFrom > 0 and column.precision > 0 and not (typemap.precisionConditionFrom <= column.precision):
                    #    continue
                    #if typemap.precisionConditionTo > 0 and column.precision > 0 and not (typemap.precisionConditionTo >= column.precision):
                    #    continue
                    #if typemap.scaleConditionFrom > 0 and column.scale > 0 and not (typemap.scaleConditionFrom <= column.scale):
                    #    continue
                    #if typemap.scaleConditionTo > 0 and column.scale > 0 and not (typemap.scaleConditionTo >= column.scale):
                    #    continue
                    return typemap        
        return None

    def shouldMigrate(self, state, otype, object):
        if "%s:*" % otype in state.ignoreList or "%s:%s.%s" % (otype, object.owner.name, object.name) in state.ignoreList:
            return False
        return True

    def migrateIdentifier(self, name, log, dots_allowed=False):
        mysql_name = name
        # truncate too long identifiers
        if len(mysql_name) > 64:
            global truncated_identifier_serial
            truncated_identifier_serial += 1
            mysql_name = mysql_name[:62]+str(truncated_identifier_serial)
            if log:
                entry = grt.classes.GrtLogEntry()
                entry.entryType = 1
                entry.name = 'Identifier `%s` is too long and was truncated to `%s`' % (name, mysql_name)
                log.entries.append(entry)
        return mysql_name

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
            source_datatype = source_type.name
            target_datatype = source_datatype
            
            # check the type mapping table
            typemap = self.findDatatypeMapping(state, source_column, source_datatype)
            if typemap:
                if not mysql_simpleTypes.has_key(typemap.targetDatatypeName.upper()):
                    grt.log_warning("migrateTableColumnsToMySQL", "Can't find mapped datatype %s for type %s\n" % (typemap.targetDatatypeName, source_datatype))
                    state.addMigrationLogEntry(2, source_column, target_column, 
                        'Unknown mapped datatype "%s" for source type "%s" (check type mapping table)' % (typemap.targetDatatypeName, source_datatype) )
                    return False

                target_column.simpleType = mysql_simpleTypes[typemap.targetDatatypeName.upper()]
                if typemap.length > -2:
                    target_column.length = typemap.length
                if typemap.scale > -2:
                    target_column.scale = typemap.scale
                if typemap.precision > -2:
                    target_column.precision = typemap.precision
                if typemap.isUnsigned > 0:
                    if "UNSIGNED" not in target_column.flags:
                        target_column.flags.append("UNSIGNED")

            # try a direct mapping to mysql types
            elif mysql_simpleTypes.has_key(target_datatype.upper()):
                target_column.simpleType = mysql_simpleTypes[target_datatype.upper()]
            else:
                grt.log_warning("migrateTableColumnsToMySQL", "Can't find datatype %s for type %s\n" % (target_datatype, source_datatype))
                state.addMigrationLogEntry(2, source_column, target_column, 
                    'Unknown datatype "%s"' % (source_datatype) )
                return False

            return True
        else:
            state.addMigrationLogEntry(2, source_column, target_column, 
                    'Could not migrate type of column "%s" in "%s" (%s)' % (target_column.name, source_column.owner.name, source_column.formattedRawType) )
            return False

        return True

    def getColumnDataType(self, column):
        source_type = column.simpleType
        if not source_type and column.userType:
            source_type = column.userType.actualType
        return source_type.name.upper() if source_type else None 

    def migrateUpdateForChanges(self, state, target_catalog):
        """
        Create datatype cast expression for target column based on source datatype.
        """
        return target_catalog

    def migrateColumnDefaultValue(self, state, default_value, source_column, target_column):
        return default_value
        
    def migrateCharsetCollation(self, state, charset, collation, source_object, target_object):
        return charset, collation

    def migrateCatalog(self, state, source_catalog):
        target_catalog = grt.classes.db_mysql_Catalog()
        log = state.addMigrationLogEntry(0, source_catalog, target_catalog, "")
        target_catalog.name = self.migrateIdentifier(source_catalog.name, log)
        target_catalog.oldName = source_catalog.name
        targetRdbms = state.targetConnection.driver.owner
        target_catalog.simpleDatatypes.extend(targetRdbms.simpleDatatypes)
        state.targetCatalog = target_catalog

        # set the version of the target database
        if state.targetVersion:
            aTargetVersion = state.targetVersion
            targetVersion = grt.classes.GrtVersion()
            targetVersion.owner = target_catalog
            targetVersion.majorNumber, targetVersion.minorNumber, targetVersion.releaseNumber, targetVersion.buildNumber = aTargetVersion.majorNumber, aTargetVersion.minorNumber, aTargetVersion.releaseNumber, aTargetVersion.buildNumber
            targetVersion.name = aTargetVersion.name
            target_catalog.version = targetVersion
        else:
            targetVersion = grt.classes.GrtVersion()
            targetVersion.owner = target_catalog
            targetVersion.majorNumber, targetVersion.minorNumber, targetVersion.releaseNumber, targetVersion.buildNumber = (5, 5, 0, 0)
            targetVersion.name = "5.5.0"
            target_catalog.version = targetVersion

        if True:
            grt.send_progress(0.0, "Migrating...")
            
            i = 0.0
            # migrate all source schemata to target schemata
            for sourceSchema in source_catalog.schemata:
                grt.begin_progress_step(0.9*i / (len(source_catalog.schemata)+1e-10), 0.9 *(i+1) / (len(source_catalog.schemata)+1e-10))
                grt.send_progress(0.9 * i / (len(source_catalog.schemata)+1e-10), "Migrating schema %s..." % sourceSchema.name)
                # migrate schema
                targetSchema = self.migrateSchema(state, sourceSchema, target_catalog)
                if targetSchema:
                    # add generated schema to target_catalog
                    target_catalog.schemata.append(targetSchema)
                grt.end_progress_step()
                i += 1
            
            grt.send_progress(0.9, "Finalizing foreign key migration...")

            # migrate foreign keys last, as they need the referenced objects which can be from different schemas to be ready
            for sourceSchema in source_catalog.schemata:
                global key_names
                key_names[sourceSchema.name] = set()
                for sourceTable in sourceSchema.tables:
                    if not self.shouldMigrate(state, 'tables', sourceTable):
                        continue
                    targetTable = self.findMatchingTargetObject(state, sourceTable)
                    self.migrateTableToMySQL2ndPass(state, sourceTable, targetTable)

            grt.send_progress(1.0, "Migration finished")
        
        return target_catalog


    def migrateSchema(self, state, sourceSchema, targetCatalog):
        targetSchema = grt.classes.db_mysql_Schema()
        targetSchema.owner = targetCatalog
        log = state.addMigrationLogEntry(0, sourceSchema, targetSchema, "")
        targetSchema.defaultCharacterSetName, targetSchema.defaultCollationName = self.migrateCharsetCollation(state, sourceSchema.defaultCharacterSetName, sourceSchema.defaultCollationName, sourceSchema, targetSchema)
        targetSchema.name = self.migrateIdentifier(sourceSchema.name, log)
        targetSchema.oldName = sourceSchema.name
        targetSchema.comment = sourceSchema.comment
        
        grt.send_progress(0.2, 'Migrating schema contents for schema %s' % sourceSchema.name)
        if True:
            grt.begin_progress_step(0.2, 1.0)
            self.migrateSchemaContents(state, targetSchema, sourceSchema)
            grt.end_progress_step()

        return targetSchema


    def migrateSchemaContents(self, state, targetSchema, sourceSchema):
        total = ( len([table for table in sourceSchema.tables if self.shouldMigrate(state, "tables", table)]) + 
                  len([view for view in sourceSchema.views if self.shouldMigrate(state, "views", view)]) + 
                  len([routine for routine in sourceSchema.routines if self.shouldMigrate(state, "routines", routine)]) +
                  1e-10
                )

        i = 0.0
        for source_table in sourceSchema.tables:
            if self.shouldMigrate(state, "tables", source_table):
                target_table = self.migrateTableToMySQL(state, source_table, targetSchema)
                if target_table:
                    targetSchema.tables.append(target_table)
                grt.send_progress(i/total, 'Table %s.%s migrated' % (sourceSchema.name, source_table.name))
                i += 1

        for source_view in sourceSchema.views:
            if self.shouldMigrate(state, "views", source_view):
                target_view = self.migrateViewToMySQL(state, source_view, targetSchema)
                if target_view:
                    targetSchema.views.append(target_view)
                grt.send_progress(i/total, 'View %s.%s migrated' % (sourceSchema.name, source_view.name))
                i += 1

        for source_routine in sourceSchema.routines:
            if self.shouldMigrate(state, "routines", source_routine):
                target_routine = self.migrateRoutineToMySQL(state, source_routine, targetSchema)
                if target_routine:
                    targetSchema.routines.append(target_routine)
                grt.send_progress(i/total, 'Routine %s.%s migrated' % (sourceSchema.name, source_routine.name))
                i += 1


    def migrateTableToMySQL(self, state, sourceTable, targetSchema):
        targetTable = grt.classes.db_mysql_Table()
        log = state.addMigrationLogEntry(0, sourceTable, targetTable, "")
        targetTable.owner = targetSchema
        targetTable.name = self.migrateIdentifier(sourceTable.name, log)
        targetTable.oldName = sourceTable.name
        targetTable.comment = sourceTable.comment
        # base attributes
        targetTable.isStub = sourceTable.isStub
        
        if True:
            # migrate columns 1st, because everything else depend on them
            column_collations = set()
            for sourceColumn in sourceTable.columns:
                targetColumn = self.migrateTableColumnToMySQL(state, sourceColumn, targetTable)
                if targetColumn:
                    targetTable.columns.append(targetColumn)
                    if targetColumn.collationName:
                        column_collations.add(targetColumn.collationName)

            # check if all columns are of the same collation and if so, just make them inherit from table default
            if len(column_collations) == 1 and targetTable.defaultCollationName == "":
                for column in targetTable.columns:
                    column.collationName = ""
                targetTable.defaultCollationName = column_collations.pop()
            
            # indexes next, because FKs and PK depend on them
            for sourceIndex in sourceTable.indices:
                targetIndex = self.migrateTableIndexToMySQL(state, sourceIndex, targetTable)
                if targetIndex:
                    targetTable.indices.append(targetIndex)

            # PK
            self.migrateTablePrimaryKeyToMySQL(state, sourceTable, targetTable)

            # Only one column in PK can have autoIncrement set and it should be the first column mentioned in the PK:
            if targetTable.primaryKey and len(targetTable.primaryKey.columns) > 1:
                first_autoinc_column = None
                for idx, column in enumerate(targetTable.primaryKey.columns):
                    if column.referencedColumn.autoIncrement:
                        if first_autoinc_column is None:
                            first_autoinc_column = idx
                        else:
                            column.referencedColumn.autoIncrement = 0
                # Only the 1st column in the PK can be auto_increment, so if the auto_inc column in a composite key
                # is not the 1st, reorder it in the index
                if first_autoinc_column is not None:
                    targetTable.primaryKey.columns.reorder(first_autoinc_column, 0)

            # FIXME: Only unique (not PK!) columns can have AI
            # Only PK columns can have autoIncrement set:
            if targetTable.primaryKey:
                pk_cols = set(column.referencedColumn.name for column in targetTable.primaryKey.columns)
            else:
                pk_cols = []
            for column in targetTable.columns:
                if column.autoIncrement and column.name not in pk_cols:
                    column.autoIncrement = 0
                    state.addMigrationLogEntry(1, sourceTable, targetTable,
                    'Autoincrement unset for column %s: Autoincrement for non primary key columns is not allowed in MySQL' % column.name)

            # triggers
            for sourceTrigger in sourceTable.triggers:
                if self.shouldMigrate(state, "triggers", sourceTrigger):
                    targetTrigger = self.migrateTriggerToMySQL(state, sourceTrigger, targetTable)
                    if targetTrigger:
                        targetTable.triggers.append(targetTrigger)

        return targetTable


    def migrateTableToMySQL2ndPass(self, state, sourceTable, targetTable):
        for sourceFK in sourceTable.foreignKeys:
            targetFK = self.migrateTableForeignKeyToMySQL(state, sourceFK, targetTable)
            if targetFK:
                targetTable.foreignKeys.append(targetFK)
        return 0

    def secondary_default_value_validation(self, state, source_column, target_column):
        # Only 1 CURRENT_TIMESTAMP column can exist and it has to be the 1st TIMESTAMP column in the table
        if target_column.simpleType:
            if target_column.simpleType.name == 'TIMESTAMP' and target_column.defaultValue == 'CURRENT_TIMESTAMP':
                for column in target_column.owner.columns:
                    if column == target_column:
                        break
                    if column.simpleType and column.simpleType.name == 'TIMESTAMP':
                        # column.defaultValue == 'CURRENT_TIMESTAMP':
                        state.addMigrationLogEntry(1, source_column, target_column,
                              'DEFAULT CURRENT_TIMESTAMP can only be used in the first TIMESTAMP column of the table. '
                                                   'Default value removed.')
                        target_column.defaultValue = ''
        else:
            grt.send_warning('Could not migrate datatype of column %s in table %s.%s' % (target_column.name, target_column.owner.owner.name, target_column.owner.name))


    def migrateTableColumnToMySQL(self, state, source_column, targetTable):
        target_catalog = targetTable.owner.owner
        target_column = grt.classes.db_mysql_Column()
        target_column.owner = targetTable
        log = state.addMigrationLogEntry(0, source_column, target_column, "")
        target_column.name = self.migrateIdentifier(source_column.name, log, dots_allowed=True)
        target_column.oldName = source_column.name

        for flag in source_column.flags:
            if flag not in target_column.flags:
                target_column.flags.append(flag)

        target_column.defaultValueIsNull = source_column.defaultValueIsNull
        target_column.isNotNull = source_column.isNotNull
        target_column.length = source_column.length
        target_column.scale = source_column.scale
        target_column.precision = source_column.precision
        target_column.datatypeExplicitParams = source_column.datatypeExplicitParams
        target_column.characterSetName, target_column.collationName = self.migrateCharsetCollation(state, source_column.characterSetName, source_column.collationName, source_column, target_column)
        target_column.comment = source_column.comment
        # ignores checks

        if not self.migrateDatatypeForColumn(state, source_column, target_column):
            #return None
            # ignore error and let rest of column to be migrated, an error should've been logged already
            pass

        target_column.defaultValue = self.migrateColumnDefaultValue(state, source_column.defaultValue, source_column, target_column)
        if (target_catalog.version.majorNumber, target_catalog.version.minorNumber, target_catalog.version.releaseNumber) < (5, 6, 5):
            self.secondary_default_value_validation(state, source_column, target_column)

        return target_column


    def migrateTablePrimaryKeyToMySQL(self, state, sourceTable, targetTable):
        if sourceTable.primaryKey:
            # we're searching for an index here, not a PK, so search it by name
            targetTable.primaryKey = find_object_with_old_name(targetTable.indices, sourceTable.primaryKey.name)
        return 0


    def migrateTableIndexToMySQL(self, state, source_index, targetTable):
        sourceTable = source_index.owner
        if len(sourceTable.columns) == 0 or len(targetTable.columns) == 0:
            state.addMigrationLogEntry(2, source_index, None,
                    'The migration of table %s indices was attempted but either the source or the target table has no columns attribute' % sourceTable.name)
            return None

        target_index = grt.classes.db_mysql_Index()
        log = state.addMigrationLogEntry(0, source_index, target_index, "")
        target_index.owner = targetTable
        target_index.name = self.migrateIdentifier(source_index.name, log, dots_allowed=True)
        target_index.oldName = source_index.name
            
        target_index.isPrimary = source_index.isPrimary
        target_index.deferability = source_index.deferability
        target_index.unique = source_index.unique
        target_index.indexType = source_index.indexType
        target_index.comment = source_index.comment
        for source_index_column in source_index.columns:
            referenced_index_col = find_object_with_old_name(targetTable.columns, source_index_column.referencedColumn.name)
            if not referenced_index_col:
                state.addMigrationLogEntry(2, source_index, target_index,
                      'The column "%s" is part of source table "%s" index "%s" but there is no such column in the target table' % (source_index_column.name, sourceTable.name, source_index.name) )
                #return None
                ##XXXXX
            target_index_column = grt.classes.db_mysql_IndexColumn()
            target_index_column.owner = target_index
            target_index_column.referencedColumn = referenced_index_col
            target_index_column.name = source_index_column.name
            target_index_column.columnLength = source_index_column.columnLength
            # if this is a text/string column, make sure that the key length is inside limit
            if referenced_index_col.simpleType and referenced_index_col.simpleType.group.name in ('string', 'text', 'blob'):
                prefix_length_limit = min(referenced_index_col.length, MYSQL_MAX_INDEX_KEY_LENGTH_INNODB_UTF8) if referenced_index_col.length > 0 else MYSQL_MAX_INDEX_KEY_LENGTH_INNODB_UTF8

                if target_index_column.columnLength > 0:
                    target_index_column.columnLength = min(prefix_length_limit, target_index_column.columnLength)
                elif target_index_column.columnLength == 0:
                    # if there's no index key length limit and the length of the column is bigger than allowed, then limit it
                    if referenced_index_col.length > 0 and prefix_length_limit < referenced_index_col.length:
                        target_index_column.columnLength = prefix_length_limit

                    # if the column type is blob/text, then we always need a key length limit
                    elif referenced_index_col.simpleType.group.name in ('text', 'blob'):
                        target_index_column.columnLength = prefix_length_limit
                else:
                    target_index_column.columnLength = min(prefix_length_limit, referenced_index_col.length)

                if target_index_column.columnLength != source_index_column.columnLength:
                    state.addMigrationLogEntry(1, source_index, target_index,
                          'Truncated key column length for column %s from %s to %s' % (source_index_column.name, source_index_column.columnLength, target_index_column.columnLength))

            target_index_column.descend = source_index_column.descend
            target_index_column.comment = source_index_column.comment

            target_index.columns.append(target_index_column)

        return target_index


    def migrateTableForeignKeyToMySQL(self, state, source_fk, targetTable):
        sourceTable = source_fk.owner
        sourceSchema = sourceTable.owner
        if len(sourceTable.columns) == 0 or len(targetTable.columns) == 0:
            state.addMigrationLogEntry(2, source_fk, None,
                    'The migration of table %s foreign keys was attempted but either the source or the target table has no columns attribute' % sourceTable.name)
            return None

        if source_fk.modelOnly == 1:
            return None
        target_fk = grt.classes.db_mysql_ForeignKey()
        target_fk.owner = targetTable
        log = state.addMigrationLogEntry(0, source_fk, target_fk, "")

        global key_names
        fk_name = source_fk.name
        if fk_name in key_names[sourceSchema.name]:  # Found duplicated fk name in this schema
            idx = 1
            while True:
                fk_name = source_fk.name + '_%d' % idx
                if not fk_name in key_names[sourceSchema.name]:
                    break
                idx += 1
            state.addMigrationLogEntry(1, source_fk, target_fk,
                'The foreign key constraint name "%s" is duplicated. Changed to "%s"' % (source_fk.name, fk_name))

        target_fk.name = self.migrateIdentifier(fk_name, log, dots_allowed=True)
        key_names[sourceSchema.name].add(fk_name)
        target_fk.oldName = source_fk.name

        target_fk.deleteRule = source_fk.deleteRule.replace('_', ' ')
        target_fk.updateRule = source_fk.updateRule.replace('_', ' ')
        target_fk.deferability = source_fk.deferability
        target_fk.modelOnly = 0
        target_fk.mandatory = source_fk.mandatory
        target_fk.referencedMandatory = source_fk.referencedMandatory
        target_fk.many = source_fk.many
        target_fk.comment = source_fk.comment

        #Find the referenced schema to find the referenced table among its tables:
        try:
            referenced_schema = self.findMatchingTargetObject(state, source_fk.referencedTable.owner)
            if not referenced_schema:
                raise ValueError, 'The referenced schema does not refer to a valid schema object'
        except Exception, err:
            state.addMigrationLogEntry(2, source_fk, target_fk, 
                        '"%s" while trying to get the schema for the table "%s"' % (str(err), source_fk.referencedTable.name) )
            return target_fk

        try:
            referenced_table = self.findMatchingTargetObject(state, source_fk.referencedTable)
            if not referenced_table:
                raise ValueError, 'The referenced table does not refer to a valid table object'
            target_fk.referencedTable = referenced_table
        except Exception, err:
            state.addMigrationLogEntry(2, source_fk, target_fk,
                      '"%s" while trying to get the referenced table for the foreign key "%s"' % (str(err), source_fk.name) )
            return target_fk

        # Migrate the columns of the foreign key
        column_errors = False
        for source_fk_column in source_fk.columns:
            try:
                target_fk_col = self.findMatchingTargetObject(state, source_fk_column)
                if not target_fk_col:
                    raise ValueError, 'The column "%s" was not found in table "%s"' % (self.migrateIdentifier(source_fk_column.name, None, dots_allowed=True), targetTable.name)
                target_fk.columns.append(target_fk_col)
            except Exception, err:
                state.addMigrationLogEntry(2, source_fk, target_fk,
                          '"%s" while trying to get the target columns for the foreign key "%s"' % (str(err), source_fk.name) )
                column_errors = True

        if column_errors:
            return target_fk

        # Migrate the referenced columns
        column_errors = False
        for source_fk_column in source_fk.referencedColumns:
            try:
                target_fk_col = self.findMatchingTargetObject(state, source_fk_column)
                if not target_fk_col:
                    raise ValueError, 'The column "%s" was not found in table "%s"' % (self.migrateIdentifier(source_fk_column.name, None, dots_allowed=True), target_fk.referencedTable.name)
                target_fk.referencedColumns.append(target_fk_col)
            except Exception, err:
                state.addMigrationLogEntry(2, source_fk, target_fk, 
                            'migrateTableForeignKeysToMySQL: "%s" while trying to get the referenced target columns for the foreign key "%s"' % (str(err), source_fk.name) )
                column_errors = True

        if column_errors:
            return target_fk

        if source_fk.index:
            target_fk.index = self.findMatchingTargetObject(state, source_fk.index)

        return target_fk


    def migrateTriggerToMySQL(self, state, source_trigger, target_table):
        copy_members = ["name", "oldName", "definer", "event", "timing", "enabled", "sqlDefinition"]
        target_trigger = grt.classes.db_mysql_Trigger()
        state.addMigrationLogEntry(0, source_trigger, target_trigger, "")
        target_trigger.owner = target_table
        for m in copy_members:
            setattr(target_trigger, m, getattr(source_trigger, m))
        # comment out triggers by default
        target_trigger.commentedOut = 1
        return target_trigger


    def migrateViewToMySQL(self, state, source_view, target_schema):
        copy_members = ["name", "oldName", "withCheckCondition", "isReadOnly", 
             "sqlDefinition"]
        target_view = grt.classes.db_mysql_View()
        target_view.owner = target_schema
        state.addMigrationLogEntry(0, source_view, target_view, "")

        for m in copy_members:
            setattr(target_view, m, getattr(source_view, m))
        # comment out view by default
        target_view.commentedOut = 1
        return target_view


    def migrateRoutineToMySQL(self, state, source_routine, target_schema):
        copy_members = ["name", "oldName", "routineType", "sqlDefinition"]
        target_routine = grt.classes.db_mysql_Routine()
        target_routine.owner = target_schema
        state.addMigrationLogEntry(0, source_routine, target_routine, "")

        for m in copy_members:
            setattr(target_routine, m, getattr(source_routine, m))
        # comment out routines by default
        target_routine.commentedOut = 1
        return target_routine


instance = GenericMigration()

@ModuleInfo.export(grt.STRING)
def getTargetDBMSName():
    return 'Generic'

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
