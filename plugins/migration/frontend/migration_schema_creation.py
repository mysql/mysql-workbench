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
import os

from workbench.ui import WizardPage
from workbench.ui import WizardProgressPage


class MainView(WizardPage):
    def __init__(self, main):
        WizardPage.__init__(self, main, "Target Creation Options")

        self.main.add_wizard_page(self, "ObjectMigration", "Target Creation Options")

        label = mforms.newLabel("Select options for the creation of the migrated schema in the target\nMySQL server and click [Next >] to execute.")
        self.content.add(label, False, True)

        panel = mforms.newPanel(mforms.TitledBoxPanel)
        panel.set_title("Schema Creation")
        self.content.add(panel, False, True)

        box = mforms.newBox(False)
        panel.add(box)
        box.set_padding(12)

        self._create_db = mforms.newCheckBox()
        self._create_db.set_text("Create schema in target RDBMS")
        box.add(self._create_db, False, True)

        # spacer
        box.add(mforms.newLabel(""), False, True)

        self._create_script = mforms.newCheckBox()
        self._create_script.set_text("Create a SQL script file")
        self._create_script.add_clicked_callback(self._toggle_sql_script)
        box.add(self._create_script, False, True)
        self._file_hbox = mforms.newBox(True)
        self._file_hbox.set_spacing(4)
        self._file_hbox.add(mforms.newLabel("Script File:"), False, True)
        self._create_script_file = mforms.newTextEntry()
        self._create_script_file.set_value(os.path.join(os.path.expanduser('~'), 'migration_script.sql'))
        self._file_hbox.add(self._create_script_file, True, True)
        button = mforms.newButton()
        button.set_text("Browse...")
        button.add_clicked_callback(self._browse_files)
        self._file_hbox.add(button, False, True)
        box.add(self._file_hbox, False, True)

        panel = mforms.newPanel(mforms.TitledBoxPanel)
        panel.set_title("Options")
        self.content.add(panel, False, True)

        box = mforms.newBox(False)
        panel.add(box)
        box.set_padding(12)
        box.set_spacing(8)

        self._keep_schema = mforms.newCheckBox()
        self._keep_schema.set_text("Keep schemas if they already exist. Objects that already exist will not be recreated or updated.")
        box.add(self._keep_schema, False, True)

        self._create_db.set_active(True)
        self._toggle_sql_script()
        self._check_file_duplicate = True


    def _toggle_sql_script(self):
        self._file_hbox.set_enabled(self._create_script.get_active())

    def _filename_changed(self):
        self._check_file_duplicate = True

    def _browse_files(self):
        form = mforms.newFileChooser(mforms.Form.main_form(), mforms.SaveFile)
        form.set_title("Save SQL Script As")
        default_path = os.path.abspath(self._create_script_file.get_string_value())
        # Use as starting directory the directory corresponding to the path stored in the text box if it exists
        # or use the home directory if it doesn't
        form.set_path(default_path if os.path.exists(default_path if os.path.isdir(default_path)
                                                                  else os.path.dirname(default_path))
                                   else os.path.expanduser('~')
                     )
        if form.run_modal():
            self._create_script_file.set_value(form.get_path())
            self._check_file_duplicate = False

    def go_next(self):
        self.main.plan.state.objectCreationParams["KeepSchemata"] = self._keep_schema.get_active()

        self.main.plan.state.objectCreationParams["CreateInDB"] = self._create_db.get_active()    

        if self._create_script.get_active():
            path = self._create_script_file.get_string_value()
            if not path or not os.path.isdir(os.path.dirname(path)):
                mforms.Utilities.show_error("Create Script File", "Create Script File option was enabled, but the provided path is invalid.\nPlease correct and retry.",
                                            "OK", "", "")
                return

            if os.path.isdir(path):
                mforms.Utilities.show_error("Create Script File", "'%s' is a directory name. Please provide a file name for saving the script as and retry." % path,
                                            "OK", "", "")
                return

            if os.path.exists(path) and self._check_file_duplicate:
                if mforms.Utilities.show_error("Create Script File", "The file '%s' provided for the SQL script already exists. Do you want to replace it?" % path,
                                            "Replace", "Cancel", "") == mforms.ResultCancel:
                    return
            self.main.plan.state.objectCreationParams["CreateSQLFile"] = path
        elif self.main.plan.state.objectCreationParams.has_key("CreateSQLFile"):
            del self.main.plan.state.objectCreationParams["CreateSQLFile"]
        
        WizardPage.go_next(self)



class CreationProgressView(WizardProgressPage):
    def __init__(self, main):
        WizardProgressPage.__init__(self, main, "Create Schemas", description="""The SQL scripts generated for the migrated schema objects will now be executed
in the target database. You can monitor execution in the logs. If there are errors 
you may correct them in the next step. Table data will be migrated at a later step.""")

        self._autostart = True
        main.add_wizard_page(self, "ObjectMigration", "Create Schemas")


    def create_ui(self):
        self.clear_tasks()
        self._script_task = self.add_task(self._create_script_task, "Create Script File")
        self._db_task1 = self.add_task(self._connect_task, "Connect to Target Database")
        self._db_task2 = self.add_task(self._validate_existing_schemata, "Perform Checks in Target")
        self._db_task3 = self.add_threaded_task(self._create_task, "Create Schemas and Objects")

    def page_activated(self, advancing):
        WizardProgressPage.page_activated(self, advancing)
        if advancing:
            if self.main.plan.state.objectCreationParams.get("CreateSQLFile", ""):
                self._script_task.set_enabled(True)
            else:
                self._script_task.set_enabled(False)

            if self.main.plan.state.objectCreationParams.get("CreateInDB", True):
                self._db_task1.set_enabled(True)
                self._db_task2.set_enabled(True)
                self._db_task3.set_enabled(True)
            else:
                self._db_task1.set_enabled(False)
                self._db_task2.set_enabled(False)
                self._db_task3.set_enabled(False)
    
    
    def go_back(self):
        self.reset()
        WizardProgressPage.go_back(self)
        

    def _create_script_task(self):
        self.main.plan.createTargetScript(self.main.plan.state.objectCreationParams["CreateSQLFile"])

        
    def _connect_task(self):
        self.main.plan.migrationTarget.connect()

    def _validate_existing_schemata(self):
        
        grt.send_progress(0.0, "Validating for existing schemas on target MySQL Server...")
        schema_set = set(schema.name.upper() for schema in self.main.plan.migrationTarget.catalog.schemata)
        target_schema_set = set(schema.upper() for schema in grt.modules.DbMySQLFE.getSchemaNames(self.main.plan.migrationTarget.connection))

        existing_schemas = list(schema_set.intersection(target_schema_set))

        continue_migration = True
        if len(existing_schemas) > 0:
            if self.main.plan.state.objectCreationParams.get("KeepSchemata", False):
                message = ''
                for schema in self.main.plan.migrationTarget.catalog.schemata:
                    if not schema.name.upper() in existing_schemas:
                        continue
                    target_schema_tables = set(table_name.upper() for table_name in
                                               grt.modules.DbMySQLFE.getTableNames(self.main.plan.migrationTarget.connection, schema.name) )
                    existing_tables = [table.name for table in schema.tables if table.name.upper() in target_schema_tables]
                    if existing_tables:
                        message += 'In schema ' + schema.name + ':\n    '
                        message += ', '.join(existing_tables) + '\n'

                if message:
                    if mforms.Utilities.show_message("Existing Tables", "The following tables already exist in their "
                                                     "target schemas:\n%sThey won't be recreated. Delete those "
                                                     "tables before continuing if you want them to be recreated. "
                                                     "Do you want to continue?" % message , "Yes", "No", "") == mforms.ResultCancel:
                        continue_migration = False

                    
            elif mforms.Utilities.show_message("Existing Schemas", "The %s %s " % ( 'schema' if len(existing_schemas) == 1 else 'schemata', ", ".join(existing_schemas)) +
                    "will be dropped in the target MySQL Server and all the existing data will be" +
                    " lost. Do you want to continue?" , "Yes", "No", "") == mforms.ResultCancel:
                continue_migration = False

        self._db_task3.set_enabled(continue_migration)
        

    def _create_task(self):
        grt.send_output("="*80+"\n\n")

        if self.main.plan.state.objectCreationParams.get("KeepSchemata", False):
            # Regenerate the creation code to take into account this option
            self.main.plan.generateSQL()
        self.main.plan.createTarget()
        self.main.plan.migrationTarget.disconnect()
        grt.send_output("="*80+"\n\n")



class CreationReportView(WizardPage):
    def __init__(self, main):
        WizardPage.__init__(self, main, "Create Target Results", wide=True)

        self.main.add_wizard_page(self, "ObjectMigration", "Create Target Results")

        text = """Scripts to create the target schema were executed. No data has been migrated yet. Review the creation report below 
for errors or warnings. If there are any errors, you can manually fix the scripts and click [Recreate Objects] to retry 
the schema creation or return to the Manual Editing page to correct them there and retry the target creation."""
        self.content.add(mforms.newLabel(text), False, True)

        hbox = mforms.newBox(True)
        hbox.set_spacing(12)
        hbox.set_homogeneous(True)
        self._tree = mforms.newTreeView(mforms.TreeFlatList)
        self._tree.add_column(mforms.IconStringColumnType, "Object", 200, False)
        self._tree.add_column(mforms.IconStringColumnType, "Result", 600, False)
        self._tree.end_columns()
        self._tree.add_changed_callback(self._selection_changed)
        hbox.add(self._tree, True, True)
        #self.content.add(self._tree, True, True)

        self._advbox = mforms.newPanel(mforms.TitledBoxPanel)
        self._advbox.set_title("SQL CREATE Script for Selected Object")
        box = mforms.newBox(False)
        self._code = mforms.newCodeEditor()
        self._code.set_language(mforms.LanguageMySQL)
        self._code.add_changed_callback(self._code_changed)
        box.add(self._code, True, True)
        vbox = mforms.newBox(True)
        vbox.set_padding(12)
        vbox.set_spacing(8)
        self._comment_check = mforms.newCheckBox()
        self._comment_check.set_text("Comment out")
        self._comment_check.add_clicked_callback(self._comment_clicked)
        vbox.add(self._comment_check, False, True)

        self._revert_btn = mforms.newButton()
        self._revert_btn.set_text("Discard")
        vbox.add_end(self._revert_btn, False, True)
        self._revert_btn.add_clicked_callback(self._discard_clicked)

        self._apply_btn = mforms.newButton()
        self._apply_btn.set_text("Apply")
        vbox.add_end(self._apply_btn, False, True)
        self._apply_btn.add_clicked_callback(self._apply_clicked)
        box.add(vbox, False, True)

        self._advbox.add(box)
        #self._advbox.set_size(-1, 200)
        #self._advbox_shown = True
        #self.go_advanced() # toggle to hide
        
        self.advanced_button.set_text("Recreate Objects")
        
        #self.content.add(self._advbox, False, True)
        hbox.add(self._advbox, True, True)
        self.content.add(hbox, True, True)

        self._msgbox = mforms.newPanel(mforms.TitledBoxPanel)
        self._msgbox.set_title("Output Messages")
        box = mforms.newBox(False)
        box.set_padding(8)
        self._msgs = mforms.newTextBox(mforms.VerticalScrollBar)
        box.add(self._msgs, True, True)
        self._msgbox.add(box)
        self.content.add(self._msgbox, False, True)
        self._msgbox.set_size(-1, 200)

        self._error_tables = []


    def go_next(self):
        if self._error_tables:
            r = mforms.Utilities.show_warning("Table Creation Errors",
                    "Some tables could not be created in the target database.\nWould you like to flag them to be skipped and copy the data for the remaining tables only?",
                    "Skip Failed Tables", "Cancel", "")
            if r == mforms.ResultOk:
                for table in self._error_tables:
                    table.commentedOut = 1
            else:
                return
        
        WizardPage.go_next(self)

    
    def _selected_log_object(self):
        selected = self._tree.get_selected_node()
        if selected and selected.get_tag():
            oid = selected.get_tag()
            object = self._object_dict.get(oid, None)
            # if a column is selected, show the table SQL
            if isinstance(object, grt.classes.db_Column):
                object = object.owner
            return object
        return None
    
    def _selected_object(self):
        log = self._selected_log_object()
        if log:
            return log.logObject
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
            self._apply_btn.set_enabled(True)
            self._revert_btn.set_enabled(True)


    def _selection_changed(self):
        log = self._selected_log_object()
        if log:
            text = []
            for entry in log.entries:
                if entry.entryType == 0:
                    text.append("\n    ".join(entry.name.split("\n")))
                elif entry.entryType == 1:
                    text.append("WARNING: %s" % "\n    ".join(entry.name.split("\n")))
                elif entry.entryType == 2:
                    text.append("ERROR: %s" % "\n    ".join(entry.name.split("\n")))
                else:
                    text.append("\n    ".join(entry.name.split("\n")))
            self._msgs.set_value("\n\n".join(text))

        object = log.logObject if log else None
        if object and hasattr(object, "temp_sql"):
            self._code.set_text(object.temp_sql)
            self._code.set_enabled(not object.commentedOut)
            self._comment_check.set_active(object.commentedOut != 0)
            self._advbox.set_enabled(True)
            self._apply_btn.set_enabled(False)
            self._revert_btn.set_enabled(False)
            return
            
        self._code.set_text("")
        self._code.set_enabled(False)
        self._comment_check.set_active(False)
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
        self._apply_btn.set_enabled(False)
        self._revert_btn.set_enabled(False)


    def _discard_clicked(self):
        object = self._selected_object()
        if object:
            self._code.set_text(object.temp_sql)
            self._comment_check.set_active(object.commentedOut != 0)
        self._apply_btn.set_enabled(False)
        self._revert_btn.set_enabled(False)
  
    
    def refresh(self):
        self._object_dict = {}
        self._error_tables = []
        self._tree.clear()
        for log in self.main.plan.state.creationLog:
            node = self._tree.add_node()
            obj = log.logObject
            icon = "GrtObject.16x16.png"
            for c in [grt.classes.db_Schema, grt.classes.db_Table, grt.classes.db_View, grt.classes.db_Routine, grt.classes.db_Trigger]:
                if isinstance(obj, c):
                    icon = c.__grtclassname__+".16x16.png"
                    break
            if not obj:
                grt.log_warning("Migration", "Object creation log '%s' referenced no object" % log.name)
                continue
            full_name = obj.name
            o = obj.owner
            while o:
                full_name = o.name + "." + full_name
                if isinstance(o, grt.classes.db_Schema):
                    break
                o = o.owner
            node.set_string(0, full_name)
            node.set_icon_path(0, icon)
            node.set_tag(log.__id__)
            self._object_dict[log.__id__] = log
            text = []
            worst = None
            for entry in log.entries:
                worst = max(worst, entry.entryType)
                if entry.entryType == 1:
                    text.append("WARNING: %s" % entry.name)
                elif entry.entryType == 2:
                    text.insert(0, "ERROR: %s" % entry.name)
                    if isinstance(obj, grt.classes.db_Table):
                        self._error_tables.append(obj)
                else:
                    text.append("Script executed successfully")

            if worst == 0:
                node.set_icon_path(1, "task_checked.png")
            elif worst == 1:
                node.set_icon_path(1, "task_warning.png")
            elif worst == 2:
                node.set_icon_path(1, "task_error.png")
            node.set_string(1, ",\n".join(text))


    def page_activated(self, advancing):
        if advancing:
            self.refresh()
        WizardPage.page_activated(self, advancing)


    def go_advanced(self):
        page = self.main.go_previous_page()
        page.reset()
        
  
    #def go_advanced(self):
    #    self._advbox_shown = not self._advbox_shown
    #    if self._advbox_shown:
    #        self.advanced_button.set_text("Hide Code")
    #    else:
    #        self.advanced_button.set_text("Show Code")
    #    self._advbox.show(self._advbox_shown)


