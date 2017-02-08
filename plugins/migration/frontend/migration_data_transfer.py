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

import sys
import grt
import mforms
import os
from workbench.ui import WizardPage, WizardProgressPage
from DataMigrator import DataMigrator
from migration_source_selection import request_password
from workbench.utils import Version
from migration_bulk_copy_data import DataCopyFactory

class SetupMainView(WizardPage):
    def _browse_files(self, option, title):
        form = mforms.newFileChooser(mforms.Form.main_form(), mforms.SaveFile)
        form.set_title(title)
        form.set_path(getattr(self, option+"_entry").get_string_value())
        if form.run_modal():
            getattr(self, option+"_entry").set_value(form.get_path())
            setattr(self, option+"_check_duplicate", False)

    def _add_script_radiobutton_option(self, box, name, caption, path_caption, browser_caption, label_caption, rid):
        holder = mforms.newBox(False)
        holder.set_spacing(4)
        radio = mforms.newRadioButton(rid)
        radio.set_text(caption)
        holder.add(radio, False, True)
        vbox = mforms.newBox(False)
        vbox.set_spacing(4)
        file_box = mforms.newBox(True)
        file_box.set_spacing(4)
        file_box.add(mforms.newLabel(path_caption), False, True)
        file_entry = mforms.newTextEntry()
        file_entry.add_changed_callback(lambda self=self, option=name: setattr(self, name+"_check_duplicate", True))
        file_box.add(file_entry, True, True)
        radio.add_clicked_callback(self._script_radio_option_callback)
        button = mforms.newButton()
        button.set_text("Browse...")
        button.add_clicked_callback(lambda option=name, title=browser_caption: self._browse_files(option, title))
        file_box.add(button, False, True)
        vbox.add(file_box, False, True)
        label = mforms.newLabel(label_caption)
        label.set_style(mforms.SmallHelpTextStyle)
        vbox.add(label, False, True)
        vbox.set_enabled(False)
        holder.add(vbox, False, True)
        box.add(holder, False, True)

        setattr(self, name+"_check_duplicate", False)
        setattr(self, name+"_radiobutton", radio)
        setattr(self, name+"_entry", file_entry)
        setattr(self, name+"_vbox", vbox)

    def _script_radio_option_callback(self):
        self.copy_script_vbox.set_enabled(self.copy_script_radiobutton.get_active())
        self.bulk_copy_script_vbox.set_enabled(self.bulk_copy_script_radiobutton.get_active())

    def __init__(self, main):
        WizardPage.__init__(self, main, "Data Transfer Setup")

        self.main.add_wizard_page(self, "DataMigration", "Data Transfer Setup")

        label = mforms.newLabel("Select options for the copy of the migrated schema tables in the target MySQL server and click [Next >] to execute.")
        self.content.add(label, False, True)

        panel = mforms.newPanel(mforms.TitledBoxPanel)
        panel.set_title("Data Copy")
        self.content.add(panel, False, True)

        box = mforms.newBox(False)
        panel.add(box)
        box.set_padding(16)
        box.set_spacing(16)

        rid = mforms.RadioButton.new_id()

        self._copy_db = mforms.newRadioButton(rid)
        self._copy_db.set_text("Online copy of table data to target RDBMS")
        self._copy_db.add_clicked_callback(self._script_radio_option_callback)
        box.add(self._copy_db, False, True)

        # XXX TODO
        #box.add(mforms.newLabel(""), False, True)
        #self._add_script_checkbox_option(box, "dump_to_file", "Create a dump file with the data", "Dump File:", "Save As")

        if sys.platform == "win32":
            self._add_script_radiobutton_option(box, "copy_script", "Create a batch file to copy the data at another time", "Batch File:", "Save As", "You should edit this file to add the source and target server passwords before running it.", rid)
        else:
            self._add_script_radiobutton_option(box, "copy_script", "Create a shell script to copy the data from outside Workbench", "Shell Script File:", "Save As", "You should edit this file to add the source and target server passwords before running it.", rid)

        self._add_script_radiobutton_option(box, "bulk_copy_script", "Create a shell script to use native server dump and load abilities for fast migration", "Bulk Data Copy Script:", "Save As", "Edit the generated file and change passwords at the top of the generated script.\nRun it on the source server to create a zip package containing a data dump as well as a load script.\nCopy this to the target server, extract it, and run the import script. See the script output for further details.", rid)

        panel = mforms.newPanel(mforms.TitledBoxPanel)
        panel.set_title("Options")
        self.content.add(panel, False, True)

        self.options_box = mforms.newBox(False)
        self.options_box.set_padding(12)
        self.options_box.set_spacing(8)
        panel.add(self.options_box)

        self._truncate_db = mforms.newCheckBox()
        self._truncate_db.set_text("Truncate target tables (i.e. delete contents) before copying data")
        self.options_box.add(self._truncate_db, False, True)

        hbox = mforms.newBox(True)
        hbox.set_spacing(16)
        hbox.add(mforms.newLabel("Worker tasks"), False, True)
        self._worker_count = mforms.newTextEntry()
        self._worker_count.set_value("2")
        self._worker_count.set_size(30, -1)
        hbox.add(self._worker_count, False, True)
        l = mforms.newImageBox()
        l.set_image(mforms.App.get().get_resource_path("mini_notice.png"))
        l.set_tooltip("Number of tasks to use for data transfer. Each task will open a "+
          "connection to both source and target RDBMSes to copy table rows.\nDefault value 2.")
        hbox.add(l, False, True)
        self.options_box.add(hbox, False, True)

        self._debug_copy = mforms.newCheckBox()
        self._debug_copy.set_text("Enable debug output for table copy")
        self.options_box.add(self._debug_copy, False, True)
        
        self._driver_sends_utf8 = mforms.newCheckBox()
        self._driver_sends_utf8.set_text("Driver sends data already encoded as UTF-8.")
        self.options_box.add(self._driver_sends_utf8, False, True)


        ###

        self._advanced_panel = mforms.newPanel(mforms.TitledBoxPanel)
        self._advanced_panel.set_title("Tables to Copy")
        self.content.add(self._advanced_panel, True, True)

        box = mforms.newBox(False)
        box.set_padding(12)
        box.set_spacing(8)

        l = mforms.newLabel("""You can limit the number of rows to be copied for certain tables. Tables that are referenced by
foreign keys from other tables cannot be limited, unless data copy from the referencing tables is also disabled.
All tables are copied by default.""")
        l.set_style(mforms.SmallHelpTextStyle)
        box.add(l, False, True)

        self._tree = mforms.newTreeView(mforms.TreeDefault)
        self._tree.add_column(mforms.IconStringColumnType, "Table", 200, False)
        self._tree.add_column(mforms.StringColumnType, "Limit Copy", 100, True)
        self._tree.add_column(mforms.StringColumnType, "Referencing Tables", 500, False)
        self._tree.end_columns()
        box.add(self._tree, True, True)
        self._advanced_panel.add(box)
        self._tree.set_cell_edited_callback(self._cell_edited)
        self._advbox_shown = False
        self._advanced_panel.show(False)
        #self.go_advanced() # toggle to hide initially


    def go_next(self):
        i = self._worker_count.get_string_value()
        try:
            count = int(i)
            if count < 1:
                raise Exception("Bad value")
        except Exception:
            mforms.Utilities.show_error("Invalid Value", "Worker thread count must be a number larger than 0.", "OK", "", "")
            return
        self.main.plan.state.dataBulkTransferParams["workerCount"] = count
        #if self.dump_to_file.get_active():
        #   self.main.plan.state.dataBulkTransferParams["GenerateDumpScript"] = self.dump_to_file_entry.get_string_value()
        #else:
        #    if "GenerateDumpScript" in self.main.plan.state.dataBulkTransferParams:
        #       del self.main.plan.state.dataBulkTransferParams["GenerateDumpScript"]

        if self.copy_script_radiobutton.get_active():
            self.main.plan.state.dataBulkTransferParams["GenerateCopyScript"] = self.copy_script_entry.get_string_value()
        else:
            if self.main.plan.state.dataBulkTransferParams.has_key("GenerateCopyScript"):
                del self.main.plan.state.dataBulkTransferParams["GenerateCopyScript"]

        if self.bulk_copy_script_radiobutton.get_active():
            self.main.plan.state.dataBulkTransferParams["GenerateBulkCopyScript"] = self.bulk_copy_script_entry.get_string_value()
        else:
            if self.main.plan.state.dataBulkTransferParams.has_key("GenerateBulkCopyScript"):
                del self.main.plan.state.dataBulkTransferParams["GenerateBulkCopyScript"]

        self.main.plan.state.dataBulkTransferParams["LiveDataCopy"] = 1 if self._copy_db.get_active() else 0
        self.main.plan.state.dataBulkTransferParams["DebugTableCopy"] = 1 if self._debug_copy.get_active() else 0
        self.main.plan.state.dataBulkTransferParams["DriverSendsDataAsUTF8"] = 1 if self._driver_sends_utf8.get_active() else 0
        self.main.plan.state.dataBulkTransferParams["TruncateTargetTables"] = 1 if self._truncate_db.get_active() else 0

        for key in self.main.plan.state.dataBulkTransferParams.keys():
            if key.endswith(":rangeKey"):
                del self.main.plan.state.dataBulkTransferParams[key]
            if key.endswith(":rangeStart"):
                del self.main.plan.state.dataBulkTransferParams[key]
            if key.endswith(":rangeEnd"):
                del self.main.plan.state.dataBulkTransferParams[key]
            if key.endswith(":rowCount"):
                del self.main.plan.state.dataBulkTransferParams[key]

        tables_to_copy = []
        for row in range(self._tree.count()):
            n = self._tree.node_at_row(row)
            table = self._tables_by_id[n.get_tag()]

            count = n.get_string(1)
            if not count:
                tables_to_copy.append(table)
            else:
                try:
                    count = int(count)
                    if count > 0:
                        # tables_to_copy.append(table)
                        self.main.plan.state.dataBulkTransferParams["%s:rowCount" % table.__id__] = count
                except:
                    grt.log_error("Invalid value in Migration DataCopy tree: %s"%count)

        self.main.plan.state.dataBulkTransferParams["tableList"] = tables_to_copy

        if self.main.plan.state.dataBulkTransferParams["tableList"]:
            return WizardPage.go_next(self)
        else:
            self.main.go_next_page(2)
            return

    # not enabled for now
    #def go_advanced(self):
    #    self._advbox_shown = not self._advbox_shown
    #    if self._advbox_shown:
    #        self.advanced_button.set_text("Advanced <<")
    #    else:
    #        self.advanced_button.set_text("Advanced >>")
    #    self._advanced_panel.show(self._advbox_shown)


    def page_activated(self, advancing):
        if advancing:
            if self.main.plan.state.objectCreationParams.get("CreateInDB", True):
                self._copy_db.set_active(True)
            else:
                self._copy_db.set_active(False)

            self.refresh_table_list()
            for k in self.main.plan.state.dataBulkTransferParams.keys():
                del self.main.plan.state.dataBulkTransferParams[k]


            if sys.platform == "win32":
                filename = mforms.Utilities.get_special_folder(mforms.Desktop)+"\\copy_migrated_tables.cmd"
            else:
                filename = mforms.Utilities.get_special_folder(mforms.Desktop)+"/copy_migrated_tables.sh"
            self.copy_script_entry.set_value(filename)
            self.copy_script_check_duplicate = True

            source_os = self.main.plan.migrationSource.get_os()
            if not source_os:
                self.bulk_copy_script_radiobutton.set_enabled(False)
                bulk_copy_filename = ''
                grt.send_warning('Cannot get operating system of source server.')
            elif source_os == "windows":
                bulk_copy_filename = os.path.join(mforms.Utilities.get_special_folder(mforms.Desktop), 'bulk_copy_tables.cmd')
            else:
                bulk_copy_filename = os.path.join(mforms.Utilities.get_special_folder(mforms.Desktop), 'bulk_copy_tables.sh')
            self.bulk_copy_script_entry.set_value(bulk_copy_filename)
            self.bulk_copy_script_check_duplicate = True


        WizardPage.page_activated(self, advancing)


    def refresh_table_list(self):
        self._tree.clear()
        target_catalog = self.main.plan.migrationTarget.catalog

        self._tables_by_id = {}
        self._table_references = {}
        for schema in target_catalog.schemata:
            for table in schema.tables:
                self._tables_by_id[table.__id__] = table
                for fk in table.foreignKeys:
                    if fk.referencedTable:
                        l = self._table_references.get(fk.referencedTable.__id__, [])
                        l.append(table)
                        self._table_references[fk.referencedTable.__id__] = l

        prepend_schema = len(target_catalog.schemata) > 1
        for schema in target_catalog.schemata:
            for table in schema.tables:
                node = self._tree.add_node()
                node.set_icon_path(0, "db.Table.16x16.png")
                if prepend_schema:
                    node.set_string(0, "%s.%s" % (schema.name, table.name))
                else:
                    node.set_string(0, table.name)
                if table.commentedOut:
                    node.set_string(1, "0")
                else:
                    node.set_string(1, "")
                refs = self._table_references.get(table.__id__, [])
                if prepend_schema:
                    node.set_string(2, ", ".join(set([tbl.owner.name+"."+tbl.name for tbl in refs])))
                else:
                    node.set_string(2, ", ".join(set([tbl.name for tbl in refs])))
                node.set_tag(table.__id__)


    def _cell_edited(self, node, column, value):
        if column == 1:
            count = None
            if value:
                try:
                    count = int(value)
                    if count < 0:
                        raise ValueError("Can't be negative")
                except ValueError:
                    mforms.Utilities.show_error("Invalid Value", "Row limit must be an integer value larger than or equal to 0. To copy all rows, leave the value blank.", "OK", "", "")
                    return
            if count is not None:
                # is this table referenced by other tables?
                refs = self._table_references.get(node.get_tag(), [])
                # check if refercences are referenced by more tables
                cur_size = 0
                # keep repeating until the set of referencing tables doesn't get anything added to it
                while cur_size != len(set(refs)):
                    cur_size = len(set(refs))
                    for r in refs:
                        refs.extend(self._table_references.get(r.__id__, []))

                if refs:
                    result = mforms.Utilities.show_warning("Limit Table Copy",
                            ("Table %s is directly or indirectly referenced by %s.\n" % (node.get_string(0), [r.name for r in refs]))+
                            "Limiting the number of rows to be copied will require also disabling copy of the referencing tables.",
                            "Disable Copy of References", "Cancel", "")
                    if result == mforms.ResultCancel:
                        return
                    tables_to_disable = [r.__id__ for r in set(refs)]
                    for table in refs:
                        for row in range(self._tree.count()):
                            n = self._tree.node_at_row(row)
                            if n.get_tag() in tables_to_disable:
                                n.set_string(1, "0")

                node.set_string(1, value)



class TransferMainView(WizardProgressPage):
    def __init__(self, main):
        WizardProgressPage.__init__(self, main, "Bulk Data Transfer", use_private_message_handling=True)
        self._autostart = True
        self._resume = False
        self.retry_button = mforms.newButton()
        self.retry_button.set_text('Retry')
        self.retry_button.add_clicked_callback(self.go_retry)

        self.retry_box = mforms.newBox(True)
        self.content.remove(self._detail_label)
        self.retry_box.add(self._detail_label, False, True)
        self.retry_box.add(self.retry_button, False, True)
        self.content.add(self.retry_box, False, True)
        self.retry_button.show(False)

        self.main.add_wizard_page(self, "DataMigration", "Bulk Data Transfer")

        self._tables_to_exclude = list()

    def page_activated(self, advancing):
        if advancing and self.main.plan.state.dataBulkTransferParams["tableList"]:
            options = self.main.plan.state.dataBulkTransferParams
            copy_script = options.get("GenerateCopyScript", None)
            bulk_copy_script = options.get("GenerateBulkCopyScript", None)

            self.add_task(self._prepare_copy, "Prepare information for data copy")
            if copy_script != None:
                self._copy_script_task = self.add_task(self._create_copy_script, "Create shell script for data copy")

            if bulk_copy_script != None:
                self._bulk_copy_script_task = self.add_task(self._create_bulk_copy_script, "Create shell script for bulk data copy")

            if options.get("LiveDataCopy", False) or options.get("GenerateDumpScript", False):
                self._migrate_task1 = self.add_threaded_task(self._count_rows, "Determine number of rows to copy")
                self._migrate_task2 = self.add_threaded_task(self._migrate_data, "Copy data to target RDBMS")

            self._migrating_data = False
            self._progress_per_table = {}

            if options.get("LiveDataCopy", False):
                source_password = self.main.plan.migrationSource.password
                if source_password is None:
                    source_password = request_password(self.main.plan.migrationSource.connection)
                target_password = self.main.plan.migrationTarget.password
                if target_password is None:
                    if self.main.plan.migrationTarget.connection.hostIdentifier == self.main.plan.migrationSource.connection.hostIdentifier:
                        if self.main.plan.migrationTarget.connection.parameterValues['userName'] == self.main.plan.migrationSource.connection.parameterValues['userName']:
                            target_password = source_password
                if target_password is None:
                    target_password = request_password(self.main.plan.migrationTarget.connection)
            else:
                source_password = None
                target_password = None

            self._transferer = DataMigrator(self, self.main.plan.state.dataBulkTransferParams,
                          self.main.plan.migrationSource.connection, source_password,
                          self.main.plan.migrationTarget.connection, target_password)

            self._transferer.copytable_path = self.main.plan.wbcopytables_path_bin
        WizardProgressPage.page_activated(self, advancing)


    def go_back(self):
        self.clear_tasks()
        self.reset(True)
        WizardProgressPage.go_back(self)


    def update_status(self):
        return WizardProgressPage.update_status(self)


    def _prepare_copy(self):
        # create work list
        source_catalog = self.main.plan.migrationSource.catalog
        tables = self.main.plan.state.dataBulkTransferParams["tableList"]
        has_catalogs = self.main.plan.migrationSource.connection.driver.owner.doesSupportCatalogs > 0
        has_schema = self.main.plan.migrationSource.connection.driver.owner.doesSupportCatalogs >= 0

        source_db_module = self.main.plan.migrationSource.module_db()
        target_db_module = self.main.plan.migrationTarget.module_db()
        ttimeout = str(self.main.plan.migrationTarget.connection.parameterValues['timeout'])
        stimeout = ''
        if self.main.plan.migrationSource.connection.parameterValues.has_key('timeout'):
            stimeout = str(self.main.plan.migrationSource.connection.parameterValues['timeout'])

        self._working_set = {}
        for table in tables:
            # find the source table
            stable = None
            for sschema in source_catalog.schemata:
                if sschema.name == table.owner.oldName:
                    for t in sschema.tables:
                        if t.name == table.oldName:
                            stable = t
                            break
                    break
            if not stable:
                self.send_error("Source table for %s (%s) not found, skipping...\n" % (table.name, table.oldName))
                continue

            if table.name in self._tables_to_exclude:
                continue

            if has_catalogs:
                schema_name = source_db_module.quoteIdentifier(stable.owner.owner.name)
                if stable.oldName:
                    # oldName already comes pre-quoted from the reveng stage
                    table_name = stable.oldName
                else:
                    table_name = source_db_module.quoteIdentifier(stable.owner.name) + "." + source_db_module.quoteIdentifier(stable.name)
            else:
                if has_schema:
                    schema_name = source_db_module.quoteIdentifier(stable.owner.name)
                else:
                    schema_name = ''
                table_name = source_db_module.quoteIdentifier(stable.name)

            targ_schema_name = target_db_module.quoteIdentifier(table.owner.name)
            targ_table_name = target_db_module.quoteIdentifier(table.name)

            self._working_set[schema_name+"."+table_name] = {"table" : table,
                        "source_schema":schema_name, "source_table":table_name,
                        "target_schema":targ_schema_name, "target_table":targ_table_name,
                        "target_table_object":table, "ttimeout":ttimeout, "stimeout":stimeout}
            select_expression = []
            source_pk_list = []
            target_pk_list = []
            for column in table.columns:
                if column.generated:
                    continue
                if table.isPrimaryKeyColumn(column):
                    source_pk_list.append(source_db_module.quoteIdentifier(column.oldName))
                    target_pk_list.append(target_db_module.quoteIdentifier(column.name))
                cast = table.customData.get("columnTypeCastExpression:%s" % column.name, None)
                if cast:
                    select_expression.append(cast.replace("?", source_db_module.quoteIdentifier(column.oldName)))
                else:
                    select_expression.append(source_db_module.quoteIdentifier(column.oldName))

            self._working_set[schema_name+"."+table_name]["source_primary_key"] = ",".join(source_pk_list) if len(source_pk_list) > 0 else "-"
            self._working_set[schema_name+"."+table_name]["target_primary_key"] = ",".join(target_pk_list) if len(target_pk_list) > 0 else "-"
            self._working_set[schema_name+"."+table_name]["select_expression"] = ", ".join(select_expression)
#            source_db_module = self.main.plan.migrationSource.module_db()
 #           source_table = source_db_module.fullyQualifiedObjectName(stable)



    def _create_copy_script(self):
        path = self.main.plan.state.dataBulkTransferParams["GenerateCopyScript"]
        debug_table_copy = self.main.plan.state.dataBulkTransferParams["DebugTableCopy"]
        truncate_target_tables = self.main.plan.state.dataBulkTransferParams["TruncateTargetTables"]
        worker_count = self.main.plan.state.dataBulkTransferParams["workerCount"]
        f = open(path, "w+")

        if sys.platform == "win32":
            def cmt(s):
                return "REM "+s+"\n"
        else:
            os.chmod(path, 0700)
            def cmt(s):
                return "# "+s+"\n"
            f.write("#!/bin/sh\n")

        f.write(cmt("Workbench Table Data copy script"))
        f.write(cmt("Workbench Version: %s" % Version.fromgrt(grt.root.wb.info.version)))
        f.write(cmt(""))
        f.write(cmt("Execute this to copy table data from a source RDBMS to MySQL."))
        f.write(cmt("Edit the options below to customize it. You will need to provide passwords, at least."))
        f.write(cmt(""))
        f.write(cmt("Source DB: %s (%s)" % (self.main.plan.migrationSource.connection.hostIdentifier,
                                          self.main.plan.migrationSource.connection.driver.owner.caption)))
        f.write(cmt("Target DB: %s" % self.main.plan.migrationTarget.connection.hostIdentifier))
        f.write("\n\n")

        if sys.platform == "win32":
            f.write("@ECHO OFF\n")
            f.write("REM Source and target DB passwords\n")
            f.write("set arg_source_password=\n")
            f.write("set arg_target_password=\n")
            f.write("""
IF [%arg_source_password%] == [] (
    IF [%arg_target_password%] == [] (
        ECHO WARNING: Both source and target RDBMSes passwords are empty. You should edit this file to set them.
    )
)
""")
            f.write("set arg_worker_count=%d\n" % worker_count)
            f.write("REM Uncomment the following options according to your needs\n")
            f.write("\n")
            f.write("REM Whether target tables should be truncated before copy\n")
            f.write( ("" if truncate_target_tables else "REM ") + "set arg_truncate_target=--truncate-target\n")
            #f.write("REM Copy tables incrementally. Useful for updating table contents after an initial migration\n")
            #f.write("REM set arg_incremental_copy=--incremental-copy\n")
            f.write("REM Enable debugging output\n")
            f.write( ("" if debug_table_copy else "REM ") + "set arg_debug_output=--log-level=debug3\n")
            f.write("\n\n")
            f.write("REM Creation of file with table definitions for copytable\n\n")

            # Creates a temporary file name with the tables to be migrated
            filename = '"%TMP%\wb_tables_to_migrate.txt"'
            f.write("set table_file=%s\n" % filename)
            f.write("TYPE NUL > %s\n" % filename)

            for table in self._working_set.values():
                fields = []
                fields.append(table["source_schema"])
                fields.append(table["source_table"])
                fields.append(table["target_schema"])
                fields.append(table["target_table"])
                fields.append(table["source_primary_key"].replace("'", r"\'"))
                fields.append(table["target_primary_key"].replace("'", r"\'"))
                fields.append(table["select_expression"].replace("'", r"\'"))

                line = "ECHO %s >> %s" % ("\t".join(fields), filename)
                f.write(line + "\n")

            f.write("\n\n")
            f.write(self.main.plan.wbcopytables_path)
            f.write(" ^\n")
            for arg in self._transferer.helper_basic_arglist(True):
                f.write(' %s ^\n' % arg)
            f.write(' --source-password="%arg_source_password%" ^\n --target-password="%arg_target_password%" ^\n --table-file="%table_file%"')
            f.write(' --thread-count=%arg_worker_count% ^\n %arg_truncate_target% ^\n %arg_debug_output%')
            f.write("\n\n")
            f.write("REM Removes the file with the table definitions\n")
            f.write("DEL %s\n" % filename)
        else:
            f.write("# Source and target DB passwords\n")
            f.write("arg_source_password=\n")
            f.write("arg_target_password=\n")
            f.write("""
if [ -z "$arg_source_password" ] && [ -z "$arg_target_password" ] ; then
    echo WARNING: Both source and target RDBMSes passwords are empty. You should edit this file to set them.
fi
""")
            f.write("arg_worker_count=%d\n" % worker_count)
            f.write("# Uncomment the following options according to your needs\n")
            f.write("\n")
            f.write("# Whether target tables should be truncated before copy\n")
            f.write( ("" if truncate_target_tables else "# ") + "arg_truncate_target=--truncate-target\n")
            #f.write("# Copy tables incrementally. Useful for updating table contents after an initial migration\n")
            #f.write("#arg_incremental_copy=--incremental-copy\n")
            f.write("# Enable debugging output\n")
            f.write( ("" if debug_table_copy else "# ") + "arg_debug_output=--log-level=debug3\n")
            f.write("\n")
            f.write(self.main.plan.wbcopytables_path)
            f.write(" \\\n")
            for arg in self._transferer.helper_basic_arglist(True):
                f.write(' %s \\\n' % arg)
            f.write(' --source-password="$arg_source_password" \\\n --target-password="$arg_target_password" \\\n')
            f.write(' --thread-count=$arg_worker_count \\\n $arg_truncate_target \\\n $arg_debug_output \\\n')

            for table in self._working_set.values():
                opt = "--table '%s' '%s' '%s' '%s' '%s' '%s' '%s'" % (table["source_schema"], 
                                                                      table["source_table"], 
                                                                      table["target_schema"], 
                                                                      table["target_table"], 
                                                                      table["source_primary_key"].replace("'", "\'"), 
                                                                      table["target_primary_key"].replace("'", "\'"), 
                                                                      table["select_expression"].replace("'", "\'"))
                f.write(" "+opt)

        f.write("\n\n")
        f.close()
        self.send_info("Table copy script written to %s" % path)



    def _create_bulk_copy_script(self):
        script_path = self.main.plan.state.dataBulkTransferParams["GenerateBulkCopyScript"]
        conn_args = self._transferer.helper_connections_arglist()

        if conn_args['source_rdbms'] == 'mssql':
            conn_args['source_instance'] = self.main.plan.migrationSource.get_source_instance()

        source_os = self.main.plan.migrationSource.get_os() 
        target_os = self.main.plan.migrationTarget.get_os()

        script = DataCopyFactory(source_os, target_os, conn_args['source_rdbms'])
        script.generate(self._working_set.values(), conn_args, script_path)



    def _count_rows(self):
        self.send_info("Counting number of rows in tables...")
        total = self._transferer.count_table_rows(self._working_set)

        self.send_info("%i total rows in %i tables need to be copied:" % (total, len(self._working_set)))
        for task in self._working_set.values():
            self.send_info("- %s.%s: %s" % (task["source_schema"], task["source_table"], task.get("row_count", "error")))


    def _migrate_data(self):
        # update the label with the number of rows to copy here, since this is in the main thread
        total = 0
        table_count = len (self._working_set);
        for task in self._working_set.values():
            total += task.get("row_count", 0)
            self.create_transfer_log(task["target_table_object"])

        self.send_info("") # newline

        if self._working_set:
            thread_count = self.main.plan.state.dataBulkTransferParams.get("workerCount", 2)
            self.send_info("Migrating data...")
            self._log_progress_text = False
            self._migrating_data = True
            try:
                succeeded_tasks = self._transferer.migrate_data(thread_count, self._working_set)
            finally:
                self._log_progress_text = True
                self._migrating_data = False

            self.send_info("") # newline

            self.send_info("Data copy results:")
            fully_copied = 0
            self._tables_to_exclude = list()
            self._count_of_failed_tables = 0
            for task in self._working_set.values():
                info = succeeded_tasks.get(task["target_schema"]+"."+task["target_table"], None)
                row_count = task.get("row_count", 0)
                if info:
                    ok, count = info
                else:
                    count = 0
                    ok = False
                if ok and count == row_count:
                    fully_copied = fully_copied + 1

                    target_table = "%s.%s" % (task["target_schema"], task["target_table"])
                    message = "Succeeded : copied %s of %s rows from %s.%s" % (count, row_count,task["source_schema"], task["source_table"])
                    self.add_log_entry(0, target_table, message)


                    self.send_info("- %s.%s has succeeded (%s of %s rows copied)" % (task["target_schema"], task["target_table"], count, row_count))
                    self._tables_to_exclude.append(task["target_table"])
                else:
                    self.send_info("- %s.%s has FAILED (%s of %s rows copied)" % (task["target_schema"], task["target_table"], count, row_count))
                    self._count_of_failed_tables = self._count_of_failed_tables + 1

            self.send_info("%i tables of %i were fully copied" % (fully_copied, table_count))

            if self._transferer.interrupted:
                raise grt.UserInterrupt("Canceled by user")

            if self._resume:
                self.send_info("Click [Retry] to retry copying remaining data from tables")
        else:
            self.send_info("Nothing to be done")


    def _verify_copy(self):
        self.send_info("Checking if number of rows copied to target tables matches source tables...")


    def create_transfer_log(self, target_table):
        log = grt.classes.GrtLogObject()
        log.logObject = target_table

        target_db_module = self.main.plan.migrationTarget.module_db()
        logSchema = target_db_module.quoteIdentifier(log.logObject.owner.name)
        logTable = target_db_module.quoteIdentifier(log.logObject.name)

        log.name = "%s.%s" % (logSchema, logTable)
        log.logObject = target_table

        self.main.plan.state.dataTransferLog.append(log)

    def get_log_object(self, target_table):
        for log in self.main.plan.state.dataTransferLog:
            if target_table == log.name:
                return log

    def add_log_entry(self, type, target_table, message):
        logObject = self.get_log_object(target_table)

        entry = grt.classes.GrtLogEntry()
        entry.entryType = type
        entry.name = message

        logObject.entries.append(entry)

    def tasks_finished(self):
        self.show_retry_button(False)


    def tasks_failed(self, canceled):
        if self._resume:
            self.show_retry_button(True)
            mforms.Utilities.show_message("Copying Tables", "Table data copy failed for %i tables. Please review the logs for details.\nIf you'd like to retry copying from the last successful point, click [Retry]."
                                           %self._count_of_failed_tables,
                                           "OK", "", "")
        else:
            self.show_retry_button(False)

    def go_retry(self):
        self._resume = False
        self.retry_button.show(False)
        self.reset()
        self.start()

    def show_retry_button(self, _show):
        self.retry_button.show(bool(_show))
        self.next_button.set_enabled(not _show)

    def _update_resume_status(self, _resume):
        self._resume = _resume

