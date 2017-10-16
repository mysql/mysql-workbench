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

import mforms
import grt
from workbench.ui import WizardPage


### remove when mforms version of newCodeEditor() is merged
def newCodeEditor(*args):
    c = mforms.CodeEditor()
    c.set_managed()
    c.set_release_on_add()
    return c
mforms.newCodeEditor = newCodeEditor

def find_object_with_old_name(list, name):
    for object in list:
        if not hasattr(object, "oldName"):
            return None
        if object.oldName == name:
            return object
    return None


class ReplaceForm(mforms.Form):
    def __init__(self, title, description):
        self._canceled = False
        mforms.Form.__init__(self, None, mforms.FormDialogFrame|mforms.FormResizable|mforms.FormMinimizable)
        self.set_title(title)

        content = mforms.newBox(False)
        self.set_content(content)
        content.set_padding(12)
        content.set_spacing(12)

        v_box = mforms.newBox(False)
        content.add(v_box, False, True)
        v_box.set_spacing(12)
        v_box.add(mforms.newLabel(description), False, True)

        table = mforms.newTable()
        table.set_padding(12)
        v_box.add(table, False, True)
        table.set_row_count(2)
        table.set_column_count(2)
        table.set_row_spacing(8)
        table.set_column_spacing(4)
        table.add(mforms.newLabel("Find:"), 0, 1, 0, 1)
        table.add(mforms.newLabel("Replace with:"), 0, 1, 1, 2)

        self.from_type_entry = mforms.newTextEntry()
        table.add(self.from_type_entry, 1, 2, 0, 1)

        self.to_type_entry = mforms.newTextEntry()
        table.add(self.to_type_entry, 1, 2, 1, 2)

        h_box = mforms.newBox(True)
        content.add_end(h_box, False, True)
        h_box.set_spacing(12)

        self.cancel_btn = mforms.newButton()
        self.cancel_btn.set_text("Cancel")
        h_box.add_end(self.cancel_btn, False, True)

        self.ok_btn = mforms.newButton()
        self.ok_btn.set_text("OK")
        h_box.add_end(self.ok_btn, False, True)
        self.set_size(600, 180)

    def show(self, type_to_replace):
        self.from_type_entry.set_value(type_to_replace)
        modal_result = self.run_modal(self.ok_btn, self.cancel_btn)
        return (modal_result, self.from_type_entry.get_string_value(), self.to_type_entry.get_string_value())


class MainView(WizardPage):
    def __init__(self, main):
        WizardPage.__init__(self, main, "Manual Editing", wide=True)

        self.main.add_wizard_page(self, "ObjectMigration", "Manual Editing")
        self._object_dict = {}
        self._source_objects_with_errors = set()
        self.error_count, self.warning_count = 0, 0


    def create_ui(self):
        self.content.set_spacing(4)
        self.content.add(mforms.newLabel("Review and edit migrated objects. You can manually edit the generated SQL before applying them to the target database."), False, True)
        
        hbox = mforms.newBox(True)
        self.tree_head_label = mforms.newLabel("Migrated Objects")
        hbox.add(self.tree_head_label, False, True)
        self._filter = mforms.newSelector()
        self._filter.add_items(["Migration Problems", "All Objects", "Column Mappings"])
        self._filter.add_changed_callback(self._filter_changed)
        hbox.add_end(self._filter, False, True)
        hbox.add_end(mforms.newLabel("View:"), False, True)
        self.content.add(hbox, False, True)
        
        self._no_errors_text = "No migration problems found. %d warning(s).\nUse the View pulldown menu to review all objects."
        self._no_errors = mforms.newLabel('')  # Label text will be set later when the warning count is calculated
        self._no_errors.set_style(mforms.BigStyle)
        self._no_errors.set_text_align(mforms.MiddleLeft)
        self.content.add(self._no_errors, True, True)
        
        self._tree = mforms.newTreeView(mforms.TreeDefault)
        self._tree.add_column(mforms.IconStringColumnType, "Source Object", 200, False)
        self._tree.add_column(mforms.IconStringColumnType, "Target Object", 200, True)
        self._tree.add_column(mforms.IconStringColumnType, "Migration Message", 300, False)
        self._tree.end_columns()
        self._tree.add_changed_callback(self._selection_changed)
        self.content.add(self._tree, True, True)
        self._tree.set_cell_edited_callback(self._cell_edited)

        self._all_menu = mforms.newContextMenu()
        self._all_menu.add_will_show_callback(self.all_menu_will_show)
        self._all_menu.add_check_item_with_title("Skip Object", self.skip_object, "skip_object")

        self._tree.set_context_menu(self._all_menu)

        
        self._columns = mforms.newTreeView(mforms.TreeShowColumnLines|mforms.TreeShowRowLines|mforms.TreeFlatList)
        self.COL_SOURCE_SCHEMA = self._columns.add_column(mforms.StringColumnType, "Source Schema", 100, False)
        self.COL_SOURCE_TABLE = self._columns.add_column(mforms.IconStringColumnType, "Source Table", 100, False)
        self.COL_SOURCE_COLUMN = self._columns.add_column(mforms.IconStringColumnType, "Source Column", 100, False)
        self.COL_SOURCE_TYPE = self._columns.add_column(mforms.StringColumnType, "Source Type", 100, False)
        self.COL_SOURCE_FLAGS = self._columns.add_column(mforms.StringColumnType, "Source Flags", 100, False)
        self.COL_SOURCE_NOTNULL = self._columns.add_column(mforms.CheckColumnType, "NN", 25, False)
        self.COL_SOURCE_DEFAULT = self._columns.add_column(mforms.StringColumnType, "Source Default Value", 100, False)
        self.COL_SOURCE_COLLATION = self._columns.add_column(mforms.StringColumnType, "Source Collation", 100, False)
        self.COL_TARGET_SCHEMA = self._columns.add_column(mforms.StringColumnType, "Target Schema", 100, False)
        self.COL_TARGET_TABLE = self._columns.add_column(mforms.IconStringColumnType, "Target Table", 100, False)
        self.COL_TARGET_COLUMN = self._columns.add_column(mforms.IconStringColumnType, "Target Column", 100, True)
        self.COL_TARGET_TYPE = self._columns.add_column(mforms.StringColumnType, "Target Type", 100, True)
        self.COL_TARGET_FLAGS = self._columns.add_column(mforms.StringColumnType, "Target Flags", 100, True)
        self.COL_TARGET_AI = self._columns.add_column(mforms.CheckColumnType, "AI", 25, True)
        self.COL_TARGET_NOTNULL = self._columns.add_column(mforms.CheckColumnType, "NN", 25, True)
        self.COL_TARGET_DEFAULT = self._columns.add_column(mforms.StringColumnType, "Target Default Value", 100, True)
        self.COL_TARGET_COLLATION = self._columns.add_column(mforms.StringColumnType, "Target Collation", 100, True)
        self.COL_MESSAGE = self._columns.add_column(mforms.IconStringColumnType, "Migration Message", 300, False)
        self._columns.end_columns()
        self._columns.set_allow_sorting(True)
        self._columns.set_selection_mode(mforms.TreeSelectMultiple)
        self._columns.add_changed_callback(self._selection_changed)
        self.content.add(self._columns, True, True)
        self._columns.set_cell_edited_callback(self._columns_cell_edited)
        self._columns.show(False)
        
        self._menu = mforms.newContextMenu()
        self._menu.add_will_show_callback(self.menu_will_show)
        self._menu.add_item_with_title("Set Target Type of Selected Columns...", self.set_target_type, "set_target_type")
        self._menu.add_item_with_title("Find and Replace Target Type...", self.replace_target_type, "replace_target_type")
        self._menu.add_item_with_title("Find and Replace Target Flags...", self.replace_target_flags, "replace_target_flags")
        self._menu.add_item_with_title("Find and Replace Target Default Value...", self.replace_target_default_value, "replace_target_default_value")
        self._menu.add_item_with_title("Find and Replace Target Collation...", self.replace_target_collation, "replace_target_collation")
        self._columns.set_context_menu(self._menu)
        
        self.help_label = mforms.newLabel("You can rename target schemas and tables, and change column definitions by clicking them once selected.")
        self.help_label.set_style(mforms.SmallStyle)
        self.content.add(self.help_label, False, True)
        
        self._advbox = mforms.newPanel(mforms.TitledBoxPanel)
        self._advbox.set_title("SQL CREATE Script for Selected Object")
        box = mforms.newBox(True)
        self._code = mforms.newCodeEditor()
        self._code.set_language(mforms.LanguageMySQL)
        self._code.add_changed_callback(self._code_changed)
        box.add(self._code, True, True)
        vbox = mforms.newBox(False)
        vbox.set_padding(12)
        vbox.set_spacing(8)

        self._lock_check = mforms.newCheckBox()
        self._lock_check.set_text("Lock edited SQL")
        self._lock_check.set_tooltip("Lock the SQL code to the edited one, preventing automatic regenerations from discarding changes made directly to the SQL script.")
        self._lock_check.add_clicked_callback(self._lock_clicked)
        vbox.add(self._lock_check, False, True)

        self._comment_check = mforms.newCheckBox()
        self._comment_check.set_text("Comment out")
        self._comment_check.set_tooltip("Mark the object to be commented out on the generated script, making it not get created in the target server.")
        self._comment_check.add_clicked_callback(self._comment_clicked)
        vbox.add(self._comment_check, False, True)
        
        self._sql_outdated_label = mforms.newLabel("Code is outdated")
        self._sql_outdated_label.show(False)
        self._sql_outdated_label.set_tooltip("The locked SQL code seems to be outdated compared to a newer, automatically generated one. Unlocking the object will update it, but your changes will be lost.")
        vbox.add(self._sql_outdated_label, False, True)

        self._revert_btn = mforms.newButton()
        self._revert_btn.set_text("Discard Changes")
        vbox.add_end(self._revert_btn, False, True)
        self._revert_btn.add_clicked_callback(self._discard_clicked)

        self._apply_btn = mforms.newButton()
        self._apply_btn.set_text("Apply Changes")
        vbox.add_end(self._apply_btn, False, True)
        self._apply_btn.add_clicked_callback(self._apply_clicked)
        box.add(vbox, False, True)

        self._advbox.add(box)
        self._advbox.set_size(-1, 200)
        self._advbox_shown = True
        self.go_advanced() # toggle to hide
                
        self.content.add(self._advbox, False, True)
        
        self._filter_errors = False
        self._filter_changed()
        
        
    def _regenerateSQL(self):
        # regenerate
        self.main.plan.generateSQL()
        # refresh the selected object
        self._selection_changed()
        
        
    def _filter_changed(self):
        i = self._filter.get_selected_index()
        self._showing_columns = False
        if i == 0: # errors only
            self._columns.show(False)
            if not self._filter_errors:
                self._filter_errors = True
                self._source_objects_with_errors = set()
                self.scan_objects_with_errors(self.main.plan.migrationSource.catalog, self.main.plan.migrationTarget.catalog)
                self.refresh_full_tree()
            if self.error_count == 0:
                self._tree.show(False)
                self.help_label.show(False)
                self.tree_head_label.show(False)
                self.advanced_button.show(False)
                if self._advbox_shown:
                    self.go_advanced()
                self._no_errors.set_text(self._no_errors_text % self.warning_count)
                self._no_errors.show(True)
            else:
                self._tree.show(True)
                self.help_label.show(True)
                self.tree_head_label.show(True)
                self.advanced_button.show(True)
                self._no_errors.show(False)
        elif i == 1: # everything
            self._no_errors.show(False)
            self._tree.show(True)
            self.help_label.show(True)
            self.tree_head_label.show(True)
            self.advanced_button.show(True)
            self._columns.show(False)
            if self._filter_errors:
                self._filter_errors = False
                self._source_objects_with_errors = set()
                self.scan_objects_with_errors(self.main.plan.migrationSource.catalog, self.main.plan.migrationTarget.catalog)
                self.refresh_full_tree()
        elif i == 2: # columns editor
            self._no_errors.show(False)
            self._showing_columns = True
            self._tree.show(False)
            self.help_label.show(True)
            self.tree_head_label.show(True)
            self.advanced_button.show(True)
            self._columns.show(True)
        else:
            self._no_errors.show(False)
            self._tree.show(True)
            self.help_label.show(True)
            self.tree_head_label.show(True)
            self.advanced_button.show(True)
            self._columns.show(False)
        
        
    def _cell_edited(self, node, column, value):
        if column == 1:
            oid = node.get_tag()
            if oid:
                object = self._object_dict.get(oid, None)
                if object:
                    if isinstance(object, grt.classes.db_Column):
                        grt.log_info("Migration", "User changed target column definition from '%s' to '%s'\n"%(node.get_string(column), value))
                        name, sep, type = value.partition("  ")
                        object.name = name
                        try:
                            if not object.setParseType(type, None):
                                raise Exception("Could not parse column type string '%s'" % type)
                        except Exception, exc:
                            mforms.Utilities.show_error("Change Column Type", "Error changing column type: %s" % exc, "OK", "", "")
                            return
                        node.set_string(1, name+"  "+object.formattedRawType)
                        self._regenerateSQL()
                    elif not isinstance(object, grt.classes.db_View) and not isinstance(object, grt.classes.db_Routine) and not isinstance(object, grt.classes.db_Trigger):
                        grt.log_info("Migration", "User renamed target object '%s' to '%s'\n"%(object.name, value))
                        node.set_string(1, value)
                        object.name = value
                        self._regenerateSQL()
                    else:
                        mforms.Utilities.show_message("Rename Object", "The object cannot be renamed from the tree.", "OK", "", "")


    def _columns_cell_edited(self, node, column, value):
        object = self._object_dict.get(node.get_tag(), None)
        if object and isinstance(object, grt.classes.db_Column):
            if object.owner.customData.get("migration:lock_temp_sql", False):
                if mforms.Utilities.show_message("Object is Locked", "The object was manually edited and is locked against updates. Would you like to unlock the object discard your edits to apply this change?",
                                          "Unlock Object", "Cancel", "") == mforms.ResultCancel:
                    return
                grt.log_info("Migration", "User unlocked object '%s' by changing columns tree" % object.name)
                object.owner.customData["migration:lock_temp_sql"] = False
        
            if column == self.COL_TARGET_COLUMN:
                object.name = value
                node.set_string(column, value)
                grt.log_info("Migration", "User renamed target column '%s' to '%s'\n"%(object.name, value))
                self._regenerateSQL()
            elif column == self.COL_TARGET_TYPE:
                try:
                    if not object.setParseType(value, None):
                        raise Exception("Could not parse column type string '%s'" % value)
                except Exception, exc:
                    mforms.Utilities.show_error("Change Column Type", "Error changing column type: %s" % exc, "OK", "", "")
                    return
                node.set_string(column, value)                
                grt.log_info("Migration", "User changed target column type of '%s' to '%s'\n"%(object.name, value))
                self._regenerateSQL()
            elif column == self.COL_TARGET_FLAGS:
                node.set_string(column, value)                
                object.flags.remove_all()
                object.flags.extend(value.split())
                grt.log_info("Migration", "User changed target column flags of '%s' to '%s'\n"%(object.name, value))
                self._regenerateSQL()
            elif column == self.COL_TARGET_AI:
                node.set_bool(column, int(value) != 0)
                object.autoIncrement = (int(value) != 0)
                grt.log_info("Migration", "User changed target column autoIncrement of '%s' to '%s'\n"%(object.name, value))
                self._regenerateSQL()
            elif column == self.COL_TARGET_NOTNULL:
                node.set_bool(column, int(value) != 0)
                object.isNotNull = (int(value) != 0)
                grt.log_info("Migration", "User changed target column isNotNull of '%s' to '%s'\n"%(object.name, value))
                self._regenerateSQL()
            elif column == self.COL_TARGET_DEFAULT:
                node.set_string(column, value)
                object.defaultValue = value
                object.defaultValueIsNull = value == "NULL"
                grt.log_info("Migration", "User changed target column default of '%s' to '%s'\n"%(object.name, value))
                self._regenerateSQL()
            elif column == self.COL_TARGET_COLLATION:
                node.set_string(column, value)
                object.collationName = value
                grt.log_info("Migration", "User changed target column collation of '%s' to '%s'\n"%(object.name, value))
                self._regenerateSQL()


    def _selected_object(self):
        if self._showing_columns:
            selected = self._columns.get_selected_node()
        else:
            selected = self._tree.get_selected_node()
        if selected and selected.get_tag():
            oid = selected.get_tag()
            object = self._object_dict.get(oid, None)
            # if a column is selected, show the table SQL
            if isinstance(object, grt.classes.db_Column):
                object = object.owner
            return object
        return None

    def _comment_clicked(self):
        object = self._selected_object()
        if object:
            text = self._code.get_text(False)
            
            if self._comment_check.get_active():
                self._code.set_text("-- "+text.replace("\n", "\n-- "))
            else:
                if text.startswith("-- "):
                    self._code.set_text(text.replace("\n-- ", "\n")[3:])

            self._code.set_enabled(not object.commentedOut)
            self._apply_clicked()
            #self._apply_btn.set_enabled(True)
            #self._revert_btn.set_enabled(True)

    def _lock_clicked(self):
        object = self._selected_object()
        if object:
            object.customData["migration:lock_temp_sql"] = self._lock_check.get_active()
            if not self._lock_check:
                if object.customData.get("migration:new_temp_sql"):
                    object.temp_sql = object.customData["migration:new_temp_sql"]
                    self._selection_changed()

    def _selection_changed(self):
        object = self._selected_object()
        if object and hasattr(object, "temp_sql"):
            self._code.set_text(object.temp_sql)
            self._code.set_enabled(not object.commentedOut)
            self._comment_check.set_active(object.commentedOut != 0)
            self._lock_check.set_active(bool(object.customData.get("migration:lock_temp_sql", False)))
            self._advbox.set_enabled(True)
            self._apply_btn.set_enabled(False)
            self._revert_btn.set_enabled(False)
            return

        self._code.set_text("")
        self._code.set_enabled(False)
        self._comment_check.set_active(False)
        self._lock_check.set_active(False)
        self._advbox.set_enabled(False)
        
        
    def _code_changed(self, x, y, z):
        self._apply_btn.set_enabled(True)
        self._revert_btn.set_enabled(True)
    
    
    def _apply_clicked(self):
        text = self._code.get_text(False)
        object = self._selected_object()
        if object:
            object.temp_sql = text
            object.commentedOut = self._comment_check.get_active()
            object.customData["migration:lock_temp_sql"] = True
        self._selection_changed()


    def _discard_clicked(self):
        self._selection_changed()
  
  
    def go_advanced(self):
        self._advbox_shown = not self._advbox_shown
        if self._advbox_shown:
            self.advanced_button.set_text("Hide Code and Messages")
        else:
            self.advanced_button.set_text("Show Code and Messages")
        self._advbox.show(self._advbox_shown)
    
    
    def go_next(self):
        if self.validate():
            self.main.plan.migrationUpdate()
            WizardPage.go_next(self)


    def page_activated(self, advance):
        WizardPage.page_activated(self, advance)
        if advance:
            self._filter_changed()
            self.refresh()


    def validate(self):
        # check if target and source catalogs are the same in the host
        plan = self.main.plan
        if plan.migrationSource.connection.hostIdentifier == plan.migrationTarget.connection.hostIdentifier:
            for schema in plan.migrationSource.catalog.schemata:
                for tschema in plan.migrationTarget.catalog.schemata:
                    if tschema.oldName == schema.name:
                        if tschema.name == schema.name:
                            mforms.Utilities.show_error("Validation Error", 
                                "The source and target are in the same server and a schema being migrated has identical names.\nPlease rename the target schema to allow the migration to continue.",
                                "OK", "", "")
                            idx = self._filter.index_of_item_with_title('All Objects')
                            if idx == -1:
                                grt.log_warning('Migration', 'Could not get the index of the "All Objects" view')
                            else:
                                self._filter.set_selected(idx)
                                self._filter.call_changed_callback()
                            return False
        return True


    def get_log_entry_for(self, source, target):
        log_entry = self.main.plan.state.findMigrationLogEntry(source, target)
        if not log_entry:
            # in case the error was not migrated, the target object will be NULL
            log_entry = self.main.plan.state.findMigrationLogEntry(source, None)
        if log_entry:
            errors = 0
            warnings = 0
            first_problem = None
            for entry in log_entry.entries:
                if entry.entryType != 0:
                    first_problem = entry.name
                    if entry.entryType == 2:
                        errors += 1
                    elif entry.entryType == 1:
                        warnings += 1
                else:
                    if entry.name:
                        first_problem = entry.name
            if errors+warnings == 0:
                return (errors, warnings, first_problem or "")#"Object migrated successfully")
            else:
                if errors+warnings > 1:
                    return (errors, warnings, first_problem+", ...")
                else:
                    return (errors, warnings, first_problem)
        else:
            # show as error if the obj is not in the ignoreList?
            return (0, 0, "Object was not migrated")

    def _set_log_entry_for(self, node, source, target, column = 2):
        err, war, text = self.get_log_entry_for(source, target)
        node.set_string(column, text)
        if err:
            node.set_icon_path(column, "task_error.png")
        if war:
            node.set_icon_path(column, "task_warning.png")
        return err, war, text

    def _add_node(self, parent, source, target, icon):
        node = parent.add_child() if parent else self._tree.add_node()
        sextra = ""
        textra = ""
        if isinstance(source, grt.classes.db_Column):
            sextra = "  " + source.formattedRawType
            if target:
                textra = "  " + target.formattedRawType
    
        if source:
            node.set_string(0, source.name+sextra)
            node.set_icon_path(0, icon)
        else:
            node.set_string(0, "n/a")
        if target:
            node.set_icon_path(1, icon)
            node.set_string(1, target.name+textra)
        else:
            node.set_string(1, "n/a")
            
        if target:
            node.set_tag(target.__id__)
            self._object_dict[target.__id__] = target
        return node


    def _add_table_node(self, tables_node, tschema, stable):
        ttable = find_object_with_old_name(tschema.tables, stable.name)
        table_node = self._add_node(tables_node, stable, ttable, "db.Table.16x16.png")
        total_errs, total_warns, text = self._set_log_entry_for(table_node, stable, ttable)
        if ttable:
            for caption, icon, list_name in [("Columns", "db.Column.nn.16x16.png", "columns"),
                                             ("Indices", "db.Index.16x16.png", "indices"),
                                             ("ForeignKeys", "db.ForeignKey.16x16.png", "foreignKeys"),
                                             ("Triggers", "db.Trigger.16x16.png", "triggers")]:
                sub_node_group = table_node.add_child()
                sub_node_group.set_string(0, caption)
                sub_node_group.set_icon_path(0, "folder")
                sub_node_group.set_string(1, caption)
                sub_node_group.set_icon_path(1, "folder")
                
                slist = getattr(stable, list_name)
                tlist = getattr(ttable, list_name)
                sub_errs = 0
                sub_warns = 0
                for sitem in slist:
                    if self._filter_errors and sitem.__id__ not in self._source_objects_with_errors:
                        continue

                    titem = find_object_with_old_name(tlist, sitem.name)
                    if titem and list_name == "columns" and ttable.isPrimaryKeyColumn(titem):
                        sub_node = self._add_node(sub_node_group, sitem, titem, "db.Column.pk.16x16.png")
                    else:
                        sub_node = self._add_node(sub_node_group, sitem, titem, icon)
                    errs, warns, text = self._set_log_entry_for(sub_node, sitem, titem)
                    sub_errs += errs
                    sub_warns += warns

                    if list_name == "indices":
                        for icolumn in sitem.columns:
                            if titem:
                                ticolumn = find_object_with_old_name(titem.columns, icolumn.name)
                            else:
                                ticolumn = None
                            self._add_node(sub_node, icolumn, ticolumn, "db.Column.16x16.png")
                    elif list_name == "foreignKeys":
                        for icolumn in sitem.columns:
                            if titem:
                                ticolumn = find_object_with_old_name(titem.columns, icolumn.name)
                            else:
                                ticolumn = None
                            self._add_node(sub_node, icolumn, ticolumn, "db.Column.16x16.png")
                    
                if sub_errs:
                    sub_node_group.set_icon_path(2, "task_error.png")
                    sub_node_group.set_string(2, "Migration errors, expand to view")
                elif sub_warns:
                    sub_node_group.set_icon_path(2, "task_warning.png")
                    sub_node_group.set_string(2, "Migration warnings, expand to view")
                else:
                    sub_node_group.set_string(2, "")#"Objects migrated successfully")
                total_errs += sub_errs
                total_warns += sub_warns
        if total_errs:
            table_node.set_icon_path(2, "task_error.png")
            table_node.set_string(2, "Migration errors, expand to view")
        elif total_warns:
            table_node.set_icon_path(2, "task_warning.png")
            table_node.set_string(2, "Migration warnings, expand to view")
        else:
            table_node.set_string(2, "")#"Objects migrated successfully")
        return total_errs, total_warns


 
    def scan_objects_with_errors(self, source_catalog, target_catalog):
        def check_object(obj, object_list):
            tobj = find_object_with_old_name(object_list, obj.name)
            err, war, txt = self.get_log_entry_for(obj, tobj)
            has_errors   = err > 0 or war > 0
            return tobj, has_errors
            
        for schema in source_catalog.schemata:
            tschema, has_errors = check_object(schema, target_catalog.schemata)
            if has_errors:
                self._source_objects_with_errors.add(schema.__id__)
            if not tschema:
                continue
            for table in schema.tables:
                if self.main.plan.migrationSource.isObjectIgnored('tables', table):
                    continue
                ttable, has_errors = check_object(table, tschema.tables)
                if has_errors:
                    self._source_objects_with_errors.add(table.__id__)
                    self._source_objects_with_errors.add(schema.__id__)
                if not ttable:
                    continue
                for column in table.columns:
                    tcolumn, has_errors = check_object(column, ttable.columns)
                    if has_errors:
                        self._source_objects_with_errors.add(column.__id__)
                        self._source_objects_with_errors.add(table.__id__)
                        self._source_objects_with_errors.add(schema.__id__)
                for index in table.indices:
                    tindex, has_errors = check_object(index, ttable.indices)
                    if has_errors:
                        self._source_objects_with_errors.add(index.__id__)
                        self._source_objects_with_errors.add(table.__id__)
                        self._source_objects_with_errors.add(schema.__id__)
                for fk in table.foreignKeys:
                    tfk, has_errors = check_object(fk, ttable.foreignKeys)
                    if has_errors:
                        self._source_objects_with_errors.add(fk.__id__)
                        self._source_objects_with_errors.add(table.__id__)
                        self._source_objects_with_errors.add(schema.__id__)
                for trigger in table.triggers:
                    ttrigger, has_errors = check_object(trigger, ttable.triggers)
                    if has_errors:
                        self._source_objects_with_errors.add(trigger.__id__)
                        self._source_objects_with_errors.add(table.__id__)
                        self._source_objects_with_errors.add(schema.__id__)
            for view in schema.views:
                if self.main.plan.migrationSource.isObjectIgnored('views', view):
                    continue
                tview, has_errors = check_object(view, tschema.views)
                if has_errors:
                    self._source_objects_with_errors.add(view.__id__)
                    self._source_objects_with_errors.add(schema.__id__)
            for routine in schema.routines:
                if self.main.plan.migrationSource.isObjectIgnored('routines', routine):
                    continue
                troutine, has_errors = check_object(routine, tschema.routines)
                if has_errors:
                    self._source_objects_with_errors.add(routine.__id__)
                    self._source_objects_with_errors.add(schema.__id__)


    def refresh(self):
        self._source_objects_with_errors = set()
        self.scan_objects_with_errors(self.main.plan.migrationSource.catalog, self.main.plan.migrationTarget.catalog)
        # the full tree will populate the _object_dict
        self.refresh_full_tree()
        self.refresh_columns_tree()
        
        
    def refresh_columns_tree(self):
        self._columns.clear()
        source_catalog = self.main.plan.migrationSource.catalog
        target_catalog = self.main.plan.migrationTarget.catalog
        for sschema in source_catalog.schemata:
            if target_catalog:
                tschema = find_object_with_old_name(target_catalog.schemata, sschema.name)
            else:
                tschema = None

            for stable in sschema.tables:
                ttable = find_object_with_old_name(tschema.tables, stable.name) if tschema else None
                if not ttable:
                    continue
                for scolumn in stable.columns:
                    tcolumn = find_object_with_old_name(ttable.columns, scolumn.name) if ttable else None

                    node = self._columns.add_node()

                    icon = "db.Column.nn.16x16.png"
                    if scolumn.owner.isPrimaryKeyColumn(scolumn):
                        icon = "db.Column.pk.16x16.png"
                    node.set_string(self.COL_SOURCE_SCHEMA, sschema.name)
                    node.set_icon_path(self.COL_SOURCE_TABLE, "db.Table.16x16.png")
                    node.set_string(self.COL_SOURCE_TABLE, stable.name)
                    node.set_icon_path(self.COL_SOURCE_COLUMN, icon)
                    node.set_string(self.COL_SOURCE_COLUMN, scolumn.name)
                    node.set_string(self.COL_SOURCE_TYPE, scolumn.formattedRawType)
                    node.set_string(self.COL_SOURCE_FLAGS, " ".join(scolumn.flags))
                    node.set_bool(self.COL_SOURCE_NOTNULL, scolumn.isNotNull != 0)
                    node.set_string(self.COL_SOURCE_DEFAULT, scolumn.defaultValue)
                    node.set_string(self.COL_SOURCE_COLLATION, scolumn.collationName)

                    if tschema:
                        node.set_string(self.COL_TARGET_SCHEMA, tschema.name)
                    if ttable:
                        node.set_icon_path(self.COL_TARGET_TABLE, "db.Table.16x16.png")
                        node.set_string(self.COL_TARGET_TABLE, ttable.name)
                    if tcolumn:
                        icon = "db.Column.nn.16x16.png"
                        if tcolumn.owner.isPrimaryKeyColumn(tcolumn):
                            icon = "db.Column.pk.16x16.png"
                        node.set_tag(tcolumn.__id__)
                        self._object_dict[tcolumn.__id__] = tcolumn
                        node.set_icon_path(self.COL_TARGET_COLUMN, icon)
                        node.set_string(self.COL_TARGET_COLUMN, tcolumn.name)
                        node.set_string(self.COL_TARGET_TYPE, tcolumn.formattedRawType)
                        node.set_string(self.COL_TARGET_FLAGS, " ".join(tcolumn.flags))
                        node.set_bool(self.COL_TARGET_AI, tcolumn.autoIncrement != 0)
                        node.set_bool(self.COL_TARGET_NOTNULL, tcolumn.isNotNull != 0)
                        node.set_string(self.COL_TARGET_DEFAULT, tcolumn.defaultValue)
                        node.set_string(self.COL_TARGET_COLLATION, tcolumn.collationName)
                        self._set_log_entry_for(node, scolumn, tcolumn, self.COL_MESSAGE)
                    else:
                        node.set_string(self.COL_MESSAGE, "The column was not migrated")


    def refresh_full_tree(self):
        self._tree.clear()
        source_catalog = self.main.plan.migrationSource.catalog
        target_catalog = self.main.plan.migrationTarget.catalog
        
        self._object_dict = {}
        self.error_count, self.warning_count = 0, 0

        if not self._filter_errors:
            obj = target_catalog.customData["migration:preamble"]
            if obj:
                self._add_node(None, None, obj, "GrtObject.16x16.png")

        for sschema in source_catalog.schemata:
            if self._filter_errors and sschema.__id__ not in self._source_objects_with_errors:
                continue
            
            if target_catalog:
                tschema = find_object_with_old_name(target_catalog.schemata, sschema.name)
            else:
                tschema = None

            schema_node = self._add_node(None, sschema, tschema, "db.Schema.16x16.png")
            self._set_log_entry_for(schema_node, sschema, tschema)

            tables_node = schema_node.add_child()
            tables_node.set_string(0, "Tables")
            tables_node.set_string(1, "Tables")
            tables_node.set_icon_path(0, "folder")
            tables_node.set_icon_path(1, "folder")

            sch_errs, sch_warns = (0, 0)
            if tschema:
                for stable in sschema.tables:
                    if self._filter_errors and stable.__id__ not in self._source_objects_with_errors:
                        continue

                    tab_errs, tab_warns = self._add_table_node(tables_node, tschema, stable)
                    sch_errs += tab_errs
                    sch_warns += tab_warns

                self.error_count += sch_errs
                self.warning_count += sch_warns
                if sch_errs:
                    tables_node.set_icon_path(2, "task_error.png")
                    tables_node.set_string(2, "Migration errors, expand to view")
                elif sch_warns:
                    tables_node.set_icon_path(2, "task_warning.png")
                    tables_node.set_string(2, "Migration warnings, expand to view")
                else:
                    tables_node.set_string(2, "")#"Objects migrated successfully")

                for item in self.main.plan.migrationSource.supportedObjectTypes[1:]:
                    list_name, struct_name, caption = item
                    icon = struct_name+".16x16.png"
    
                    sub_node_group = schema_node.add_child()
                    sub_node_group.set_string(0, caption)
                    sub_node_group.set_icon_path(0, "folder")
                    sub_node_group.set_string(1, caption)
                    sub_node_group.set_icon_path(1, "folder")
                    
                    slist = getattr(sschema, list_name)
                    tlist = getattr(tschema, list_name)
                    group_errs, group_warns = 0, 0
                    for sitem in slist:
                        if self._filter_errors and sitem.__id__ not in self._source_objects_with_errors:
                            continue

                        titem = find_object_with_old_name(tlist, sitem.name)
                        sub_node = self._add_node(sub_node_group, sitem, titem, icon)
                        it_errs, it_warns, text = self._set_log_entry_for(sub_node, sitem, titem)
                        group_errs += it_errs
                        group_warns += it_warns
                    self.error_count += group_errs
                    self.warning_count += group_warns
                    if group_errs:
                        sub_node_group.set_icon_path(2, "task_error.png")
                        sub_node_group.set_string(2, "Migration errors, expand to view")
                    elif group_warns:
                        sub_node_group.set_icon_path(2, "task_warning.png")
                        sub_node_group.set_string(2, "Migration warnings, expand to view")
                    else:
                        sub_node_group.set_string(2, "")#"Objects migrated successfully")

            schema_node.expand()
        
        if not self._filter_errors:
            obj = target_catalog.customData["migration:postamble"]
            if obj:
                self._add_node(None, None, obj, "GrtObject.16x16.png")

        self._selection_changed()


    def skip_object(self):
        node = self._tree.get_selected_node()
        tag = node.get_tag()
        if tag and tag in self._object_dict:
            obj = self._object_dict[tag]
            if hasattr(obj, 'commentedOut'):
                obj.commentedOut = not obj.commentedOut    

    def all_menu_will_show(self, item):
        node = self._tree.get_selected_node()
        tag = node.get_tag()
        self._all_menu.get_item(0).set_enabled(False)
        if tag and tag in self._object_dict:
            obj = self._object_dict[tag]
            if hasattr(obj, 'commentedOut'):
                self._all_menu.get_item(0).set_checked(obj.commentedOut)
                if obj.__grtclassname__ in ("db.Index", "db.ForeignKey"):
                    self._all_menu.get_item(0).set_enabled(True)

    def menu_will_show(self, item):
        self._menu.get_item(0).set_enabled(len(self._columns.get_selection()) > 1)

    def set_target_type(self):
        selected_nodes = self._columns.get_selection()
        if selected_nodes:
            ret, type = mforms.Utilities.request_input('Change target columns type', 'Please specify the target type', '')
            if ret:
                for n in selected_nodes:
                    self._columns_cell_edited(n, self.COL_TARGET_TYPE, type)

    def replace_target(self, title, description, type):
        selected_node = self._columns.get_selection()[0]
        if selected_node:
            to_replace = selected_node.get_string(type)
            repl_form = ReplaceForm(title, description)
            ret, to_replace, replace_with = repl_form.show(bool(to_replace))
            if ret:
                for i in range(self._columns.count()):
                    node = self._columns.node_at_row(i)
                    if node.get_string(type) == to_replace:
                        self._columns_cell_edited(node, type, replace_with)

    def replace_target_type(self):
        self.replace_target("Find and Replace Target Type", 
                            "Target/migrated data types matching the search term will be replaced for all columns of all tables.", 
                            self.COL_TARGET_TYPE)

    def replace_target_flags(self):
        self.replace_target("Find and Replace Target Flags", 
                            "Target/migrated flags matching the search term will be replaced for all columns of all tables.", 
                            self.COL_TARGET_FLAGS)

    def replace_target_default_value(self):
        self.replace_target("Find and Replace Target Default Value", 
                            "Target/migrated default values matching the search term will be replaced for all columns of all tables.", 
                            self.COL_TARGET_DEFAULT)

    def replace_target_collation(self):
        self.replace_target("Find and Replace Target Collation", 
                            "Target/migrated collations matching the search term will be replaced for all columns of all tables.", 
                            self.COL_TARGET_COLLATION)
