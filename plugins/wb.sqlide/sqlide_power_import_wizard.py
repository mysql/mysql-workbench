# Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

from __future__ import with_statement

# import the mforms module for GUI stuff
import mforms
import threading

import os

from mforms import newTreeView
from mforms import FileChooser
from sqlide_power_import_export_be import create_module
from workbench.ui import WizardForm, WizardPage, WizardProgressPage
from datetime import datetime
import operator
from workbench.utils import Version


from workbench.log import log_error, log_info

last_location = ""
drop_table = False
truncate_table = False

def showPowerImport(editor, selection):
    importer = PowerImportWizard(editor, mforms.Form.main_form(), selection)
    importer.set_title("Table Data Import")
    importer.run()

def handleContextMenu(name, sender, args):
    menu = mforms.fromgrt(args['menu'])

    selection = args['selection']

    # Add extra menu items to the SQL editor live schema tree context menu
    user_selection = None
    
    for s in selection:
        if s.type == 'db.Schema':
            user_selection = {'schema': s.name, 'table': None}
            break
        elif s.type == 'db.Table':
            user_selection = {'table': s.name, 'schema': s.schemaName}
            break
        elif s.type == 'tables':
            user_selection = {'table': None, 'schema': s.schemaName}
            break
        else:
            return

    if user_selection:
        item = mforms.newMenuItem("Table Data Import Wizard")
        item.add_clicked_callback(lambda sender=sender : showPowerImport(sender, user_selection))
        menu.insert_item(4, item)
        if user_selection['table']:
            menu.insert_item(5, mforms.newMenuItem("", mforms.SeparatorMenuItem))

class ResultsPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Import Results")
        self.next_button.set_text('Finish')
            
    def go_next(self):
        self.main.close()
        
    def get_path(self):
        return self.main.select_file_page.importfile_path.get_string_value()

    def create_ui(self):
        if self.main.import_progress_page.import_time:
            itime = float("%d.%d" % (self.main.import_progress_page.import_time.seconds, self.main.import_progress_page.import_time.microseconds))
            self.content.add(mforms.newLabel(str("File %s was imported in %.3f s" % (self.get_path(), itime))), False, True)
        
        
        self.content.add(mforms.newLabel(str("Table %s.%s %s" % (self.main.destination_table['schema'], 
                                                                 self.main.destination_table['table'], 
                                                                 "has been used" if self.main.destination_page.existing_table_radio.get_active() else "was created"))), False, True)
        self.content.add(mforms.newLabel(str("%d records imported" % self.main.import_progress_page.module.item_count)), False, True)

class ImportProgressPage(WizardProgressPage):
    def __init__(self, owner):
        WizardProgressPage.__init__(self, owner, "Import Data")

        self.add_task(self.prepare_import, "Prepare Import")
        self.add_threaded_task(self.start_import, "Import data file")
        self.module = None
        self.stop = None
        self.import_time = None
        
    def prepare_import(self):
        self.module = self.main.configuration_page.active_module
        self.stop = threading.Event() 
        self.module.create_new_table(self.main.destination_page.new_table_radio.get_active())
        self.module.force_drop_table(self.main.destination_page.drop_table_cb.get_active())
        self.module.truncate_table(self.main.destination_page.truncate_table_cb.get_active())
        self.module.set_table(self.main.destination_table['schema'], self.main.destination_table['table'])
        self.module.set_decimal_separator(str(self.main.configuration_page.ds_entry.get_string_value()))
        self.module.set_date_format(str(self.main.configuration_page.df_entry.get_string_value()))
        self.module.set_mapping(self.main.configuration_page.column_mapping)
        self.module.progress_info = self.progress_notify
        return True
        
    def progress_notify(self, pct, msg):
        self.send_progress(pct, msg)
        
    def start_import(self):
        self.import_time = None
        start = datetime.now()
        retval = self.module.start(self.stop)
        self.import_time = datetime.now() - start
        return retval
        
    def page_activated(self, advancing):
        self.reset(True)
        self.module = None
        self.stop = None
        super(ImportProgressPage, self).page_activated(advancing)
        
    def go_cancel(self):
        if self.on_close():
            self.main.close()
                
    def on_close(self):
        if self.module and self.module.is_running:
            if mforms.ResultOk == mforms.Utilities.show_message("Confirmation", "Do you wish to stop the import process?", "Yes", "No",""):
                self.stop.set()
                return True
            return False
        return True

class ConfigurationPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Configure Import Settings", wide=True)
        
        self.last_analyze_status = False
        self.input_file_type = 'csv'
        self.active_module = self.main.formats[0] # csv
        self.encoding_list = {'cp1250 (windows-1250)':'cp1250', 
                              'latin2 (iso8859-2)':'iso8859_2', 
                              'latin1 (iso8859-1)':'latin_1', 
                              'utf-8':'utf-8', 
                              'utf-16':'utf-16'}
        self.dest_cols = []
        self.column_mapping = []
        self.ds_show_count = 0
        self.df_show_count = 0
        self.opts_mapping = {}
        self.is_server_5_7 = Version.fromgrt(self.main.editor.serverVersion).is_supported_mysql_version_at_least(Version.fromstr("5.7.5"))


    def go_cancel(self):
        self.main.close()
    
    def page_activated(self, advancing):
        if advancing:
            self.get_module()
            
        if advancing and not self.main.destination_page.new_table_radio.get_active():
            self.load_dest_columns()
        super(ConfigurationPage, self).page_activated(advancing)
        
        if advancing:
            self.call_create_preview_table()
        
    
    def create_ui(self):
        self.set_spacing(16)
        format_box = mforms.newBox(True)
        format_box.set_spacing(8)
        format_box.add(mforms.newLabel("Detected file format: %s" % self.input_file_type), False, True)
        if len(self.active_module.options) != 0:
            advanced_opts_btn = mforms.newButton(mforms.ToolButton)
            advanced_opts_btn.set_icon(mforms.App.get().get_resource_path("admin_option_file.png"))
            advanced_opts_btn.add_clicked_callback(lambda: self.optpanel.show(False) if self.optpanel.is_shown() else self.optpanel.show(True) )
            format_box.add(advanced_opts_btn, False, True)
        
        self.content.add(format_box, False, True)

        if len(self.active_module.options) != 0:
            self.optpanel = mforms.newPanel(mforms.TitledBoxPanel)
            self.optpanel.set_title("Options:")
            def set_text_entry(field, output):
                txt = field.get_string_value().encode('utf-8').strip()
                if len(txt) == 0:
                    operator.setitem(output, 'value', None)
                    mforms.Utilities.add_timeout(0.1, self.call_create_preview_table)
                elif len(txt) == 1:
                    operator.setitem(output, 'value', txt)
                    mforms.Utilities.add_timeout(0.1, self.call_create_preview_table)
                else:
                    field.set_value("")
                    mforms.Utilities.show_error("Import Wizard", "Due to the nature of this wizard, you can't use unicode characters in this place, as only one character is allowed.","Ok","","")


            def set_selector_entry(selector, output):
                operator.setitem(output, 'value', output['opts'][str(selector.get_string_value())])
                mforms.Utilities.add_timeout(0.1, self.call_create_preview_table)
            
            box = mforms.newBox(False)
            box.set_spacing(8)
            box.set_padding(8)
            for name, opts in self.active_module.options.iteritems():
                label_box = mforms.newBox(True)
                label_box.set_spacing(8)
                label_box.add(mforms.newLabel(opts['description']), False, True)
                if opts['type'] == 'text':
                    opt_val = mforms.newTextEntry()
                    opt_val.set_size(35, -1)
                    opt_val.set_value(opts['value'])
                    opt_val.add_changed_callback(lambda field = opt_val, output = opts: set_text_entry(field, output))
                    label_box.add_end(opt_val, False, True)
                    self.opts_mapping[name] = lambda val: opt_val.set_value(val)
                if opts['type'] == 'select':
                    opt_val = mforms.newSelector()
                    opt_val.set_size(75, -1)
                    opt_val.add_items([v for v in opts['opts']])
                    opt_val.set_selected(opts['opts'].values().index(opts['value']))
                    opt_val.add_changed_callback(lambda selector = opt_val, output = opts: set_selector_entry(selector, output))
                    self.opts_mapping[name] = lambda input, values =  opts['opts'].values(): opt_val.set_selected(values.index(input) if input in values else 0)
                    label_box.add_end(opt_val, False, True)
                box.add(label_box, False, True)
            self.optpanel.add(box)
            self.content.add(self.optpanel, False, True)
            self.optpanel.show(False)

        if self.input_file_type == 'csv':
            # We show encoding box only for csv as json can be only utf-8, utf-16 according to rfc
            self.encoding_box = mforms.newBox(True)
            self.encoding_box.set_spacing(16)
            self.encoding_box.add(mforms.newLabel("Encoding: "), False, True)
            self.encoding_sel = mforms.newSelector()
            self.encoding_sel.set_size(250, -1)
            self.encoding_box.add(self.encoding_sel, False, True)
        
            for i,e in enumerate(self.encoding_list):
                self.encoding_sel.add_item(e)
                if self.encoding_list[e] == 'utf-8':
                    self.encoding_sel.set_selected(i)
            self.encoding_sel.add_changed_callback(self.encoding_changed)
            self.content.add(self.encoding_box, False, True)
        
        self.table_preview_box = mforms.newBox(False)
        self.table_preview_box.set_spacing(16)
        self.preview_table = None
        self.content.add(self.table_preview_box, True, True)
        
        self.column_caption = mforms.newPanel(mforms.BorderedPanel)
        self.column_caption.set_title("Columns:")
        self.column_caption.set_size(-1, 100)
        self.column_scroll = mforms.newScrollPanel(0)
        self.column_caption.add(self.column_scroll)
        self.table_preview_box.add(self.column_caption, True, True)

        extra_opts = mforms.newBox(False)
        extra_opts.set_spacing(16)
        
        self.ds_box = mforms.newBox(True)
        self.ds_box.set_spacing(8)
        extra_opts.add(self.ds_box, False, True)
        
        self.df_box = mforms.newBox(True)
        self.df_box.set_spacing(8)
        extra_opts.add_end(self.df_box, False, True)
        
        self.ds_box.add(mforms.newLabel("Decimal Separator:"), False, True)
        self.ds_entry = mforms.newTextEntry()
        self.ds_entry.set_value('.')
        self.ds_entry.set_size(30, -1)
        self.ds_box.add(self.ds_entry, False, True)
        self.ds_box.show(False)
        
        self.df_box.add(self.make_label_with_tooltip("Date format: ", "Expects string pattern with the date format.\n"
                                                    "Default format is: %Y-%m-%d %H:%M:%S\n"
                                                    "\nCommon used options:\n"
                                                    "\t%d is the day number\n"
                                                    "\t%m is the month number\n"
                                                    "\t%y is the four digits year number\n"
                                                    "\t%H is the hour number\n"
                                                    "\t%M is the minute number\n"
                                                    "\t%S is the second number\n\n"
                                                    "More formats can be found under the following location:\n" 
                                                    "https://docs.python.org/2/library/datetime.html#strftime-and-strptime-behavior"), False, True)
        self.df_entry = mforms.newTextEntry()
        self.df_entry.set_value("%Y-%m-%d %H:%M:%S")
        self.df_entry.set_size(200, -1)
        self.df_box.add(self.df_entry, False, True)
        self.df_box.show(False)
        
        self.content.add_end(extra_opts, False, True)
        
    def make_label_with_tooltip(self, lbl, tooltip):
        box = mforms.newBox(True)
        box.add(mforms.newLabel(lbl), False, True)
        l = mforms.newImageBox()
        l.set_image(mforms.App.get().get_resource_path("mini_notice.png"))
        l.set_tooltip(tooltip)
        box.add(l, False, True)
        return box
        
    def load_dest_columns(self):
        try:
            rset = self.main.editor.executeManagementQuery("SHOW COLUMNS FROM `%s`.`%s`" % (self.main.destination_table['schema'], self.main.destination_table['table']), 1)
        except Exception, e:
            log_error("SHOW COLUMNS FROM `%s`.`%s` : %s" % (self.main.destination_table['schema'], self.main.destination_table['table'], e))
            rset = None
            
        if rset:
            self.dest_cols = []
            ok = rset.goToFirstRow()
            while ok:
                self.dest_cols.append(rset.stringFieldValueByName("Field"))
                ok = rset.nextRow()    
    
    def call_create_preview_table(self):
        self.df_box.show(False)
        self.ds_box.show(False)
        self.ds_show_count = 0
        self.df_show_count = 0
        
        self.create_preview_table(self.call_analyze())
        if self.input_file_type == 'csv' and self.active_module.dialect:
            for name, opts in self.active_module.options.items():
                self.opts_mapping[name](opts['value'])
    
    def call_analyze(self):
        self.active_module.set_filepath(self.main.select_file_page.importfile_path.get_string_value())
        if self.input_file_type == 'csv':
            self.active_module.set_encoding(self.encoding_list[self.encoding_sel.get_string_value()])
        if not self.active_module.analyze_file():
            mforms.Utilities.show_warning("Table Data Import", "Can't analyze file. Please try to change encoding type. If that doesn't help, maybe the file is not: %s, or the file is empty." % self.active_module.title, "Ok", "", "")
            self.last_analyze_status = False
            return False
        self.last_analyze_status = True
        return True 

    def show_df_box(self, show = True):
        if show:
            self.df_show_count = self.df_show_count + 1
            if self.df_show_count == 1:
                self.df_box.show(True)
        else:
            if self.df_show_count > 0:
                self.df_show_count = self.df_show_count - 1
                if self.df_show_count == 0:
                    self.df_box.show(False)
    
    def show_ds_box(self, show = True):
        if show:
            self.ds_show_count = self.ds_show_count + 1
            if self.ds_show_count == 1:
                self.ds_box.show(True)
        else:
            if self.ds_show_count > 0:
                self.ds_show_count = self.ds_show_count - 1
                if self.ds_show_count == 0:
                    self.ds_box.show(False)
        
        
    def create_preview_table(self, clean_up = False):
        
        def create_chkbox(row):
            chk =  mforms.newCheckBox()
            chk.set_active(True)
            chk.add_clicked_callback(lambda checkbox = chk, output = row: operator.setitem(output, 'active', True if checkbox.get_active() else False))
            return chk
        
        type_items = {'is_string':'text','is_bignumber':'bigint', "is_geometry":'geometry', 'is_number':'int', 'is_float':'double', 'is_bin':'binary', 'is_date_or_time': 'datetime', 'is_json':'json'}
        def create_select_type(row):
            def sel_changed(sel, output):
                selection = sel.get_string_value()
                for v in type_items:
                    if selection in type_items[v]:
                        if output['type'] == 'double' and type_items[v] != 'double':
                            self.show_ds_box(False)
                        
                        if output['type'] == 'datetime' and type_items[v] != 'datetime':
                            self.show_df_box(False)
                            
                        if type_items[v] == 'double':
                            self.show_ds_box(True)
                        if type_items[v] == 'datetime':
                            self.show_df_box(True)
                            
                        output['type'] = type_items[v]
                        
                        break  
                
            sel = mforms.newSelector()
            sel.set_size(120, -1)

            items = [type for type in type_items.values() if not ((type == "geometry" or type == "json") and self.input_file_type == "json" and not self.is_server_5_7) ]
            sel.add_items(items)
            if not self.is_server_5_7 and (row['type'] == "geometry" or row["type"] == "json") and self.input_file_type == "json":
                row['type'] = "text" # If it's server older than 5.7 we don't have support for geojson so we can't properly import this file, instead we fallback to text
                log_info("Column %s is of type GeoJso but server doesn't support this, importing as text instead." % row['name'])
                
            for i, v in enumerate(items):
                if row['type'] == v:
                    sel.set_selected(i)
                    break
            
            sel.add_changed_callback(lambda: sel_changed(sel, row))
            return sel
        
        if self.preview_table is not None:
            self.column_scroll.remove()
            self.table_preview_box.set_spacing(16)
            if self.treeview_preview is not None:
                self.table_preview_box.remove(self.treeview_preview)
                self.treeview_preview = None
            self.preview_table = None
            self.dest_column_table_col = []
            self.field_type_table_col = []
            
        def create_select_dest_col(row, cols):
            sel = mforms.newSelector()
            sel.set_size(120, -1)
            sel.add_items(cols)
            for i, c in enumerate(cols):
                if c == row['dest_col']:
                    sel.set_selected(cols.index(c))
                    break
            sel.add_changed_callback(lambda output = row: operator.setitem(output, 'dest_col', sel.get_string_value()))
            return sel

        self.preview_table = mforms.newTable()
        self.preview_table.suspend_layout()
        self.column_scroll.add(self.preview_table)

        self.preview_table.set_column_count(5)
        self.preview_table.set_row_count(len(self.active_module._columns) + 1)
        self.preview_table.set_row_spacing(8)
        self.preview_table.set_column_spacing(8)
        
        if len(self.active_module._columns) >= 3:
            self.column_caption.set_size(-1, 200)
        else:
            self.column_caption.set_size(-1, 100)
        
        self.checkbox_list = []

        def sell_all(cols, active):
            for checkbox in self.checkbox_list:
                checkbox.set_active(bool(active))
            for row in self.column_mapping:
                row['active'] = active
        
        def find_column(col_name, index):
            if col_name in self.dest_cols:
                return col_name
            else:
                return self.dest_cols[index] if i < len(self.dest_cols) else None
        
        chk = mforms.newCheckBox()
        chk.set_active(True)
        chk.add_clicked_callback(lambda checkbox = chk, columns = self.active_module._columns: sell_all(columns, checkbox.get_active()))
        
        self.preview_table.add(chk, 0, 1, 0, 1, mforms.HFillFlag)
        self.preview_table.add(mforms.newLabel("Source Column"), 1, 2, 0, 1, mforms.HFillFlag)
        if not self.main.destination_page.new_table_radio.get_active():
            self.preview_table.add(mforms.newLabel("Dest Column"), 2, 3, 0, 1, mforms.HFillFlag)
        else:
            self.preview_table.add(mforms.newLabel("Field Type"), 3, 4, 0, 1, mforms.HFillFlag)
        self.column_mapping = []
        for i, col in enumerate(self.active_module._columns):
            row = {'active': True, 'name': col['name'], 'type' : None, 'col_no': i, 'dest_col': find_column(col['name'], i)}
            for c in col:
                if c.startswith('is_') and col[c]:
                    row['type'] = type_items[c]
                    break
            chk_box = create_chkbox(row)
            self.checkbox_list.append(chk_box)
            self.preview_table.add(chk_box, 0, 1, i+1, i+2, mforms.HFillFlag)
            self.preview_table.add(mforms.newLabel(str(col['name'].encode('utf8'))), 1, 2, i+1, i+2, mforms.HFillFlag)
            if not self.main.destination_page.new_table_radio.get_active():
                self.preview_table.add(create_select_dest_col(row, self.dest_cols), 2, 3, i+1, i+2, mforms.HFillFlag)
            else:
                self.preview_table.add(create_select_type(row), 3, 4, i+1, i+2, mforms.HFillFlag)
            self.column_mapping.append(row)
            
        self.treeview_preview = newTreeView(mforms.TreeFlatList)
        for i, col in enumerate(self.active_module._columns):
            self.treeview_preview.add_column(mforms.StringColumnType, str(col['name'].encode('utf8')), 75, True)
        self.treeview_preview.end_columns()
        
        
        if len(self.active_module._columns):
            col_values = []
            val_len = 0
            for col in self.active_module._columns:
                val_len = len(col['value']) if len(col['value']) > val_len else val_len

            col_len = len(self.active_module._columns)
            for i in range(0, val_len):
                row = []
                for j in range(0, col_len):
                    
                    if len(self.active_module._columns[j]['value']) > i:
                        row.append(self.active_module._columns[j]['value'][i])
                    else:
                        row.append("")
                col_values.append(row)

            for row in col_values:
                node = self.treeview_preview.add_node()
                for i, col in enumerate(row):
                    if hasattr(col, 'encode'):
                        node.set_string(i, str(col.encode('utf8')))
                    else:
                        node.set_string(i, str(col))

        self.treeview_preview.set_allow_sorting(True)
        self.treeview_preview.set_size(200, 100)
        self.table_preview_box.add(self.treeview_preview, False, True)
        self.preview_table.resume_layout()
        
    def encoding_changed(self):
        self.call_create_preview_table()
        
    def get_module(self):
        file_name, file_ext = os.path.splitext(os.path.basename(self.main.select_file_page.importfile_path.get_string_value()))
        self.input_file_type = str(file_ext[1:])
        for format in self.main.formats:
            if format.name == self.input_file_type:
                self.active_module = format
                break 
        else:
            raise Exception("Unsupported file type.")
        
    def validate(self):
        if not self.last_analyze_status:
            mforms.Utilities.show_message("Table Data Import", "File not loaded properly, please check the file and try again.", "Ok", "","")
            return False
        
        for row in self.column_mapping:
            if row['active']:
                return True
        else:
            mforms.Utilities.show_message("Table Data Import", "You need to specify at least one column", "Ok", "","")
            return False

class SelectDestinationPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Select Destination", wide=True)
        self.back_button.set_enabled(False)
        self.table_list = {}
        
    def go_cancel(self):
        self.main.close()
        
    def page_activated(self, advancing):
        super(SelectDestinationPage, self).page_activated(advancing)

        if advancing:
            file_name, file_ext = os.path.splitext(os.path.basename(self.main.select_file_page.importfile_path.get_string_value()))
            self.new_table_name.set_value(file_name)
            self.preload_existing_tables()
        
    def preload_existing_tables(self):
        compare_in_lowercase = self.check_server_lower_case_table_names()
        
        rset = self.main.editor.executeManagementQuery("SHOW DATABASES", 1)
        if rset:
            ok = rset.goToFirstRow()
            self.table_list = {}
            db_list = []
            while ok:
                dbname = rset.stringFieldValue(0)
                if dbname.strip() in ["mysql", "sys", "information_schema", "fabric", "performance_schema"]:
                    ok = rset.nextRow()
                    continue
                db_list.append(dbname)
                ok = rset.nextRow()
            
            rset = self.main.editor.executeManagementQuery("SHOW FULL TABLES FROM `%s`" % self.main.destination_table['schema'], 0)
            if rset:
                ok = rset.goToFirstRow()
                while ok:
                    if rset.stringFieldValue(1) == "BASE TABLE":
                        table_name = rset.stringFieldValue(0) if not compare_in_lowercase else rset.stringFieldValue(0).lower() 
                        self.table_list["%s.%s" % (self.main.destination_table['schema'], table_name)] = {'schema': self.main.destination_table['schema'], 'table': table_name} 
                        
                    ok = rset.nextRow()
            
            self.destination_table_sel.clear()
            self.destination_table_sel.add_items(self.table_list.keys())
            if self.main.destination_table['schema'] and self.main.destination_table['table']:
                table_name = "%s.%s" % (self.main.destination_table['schema'], self.main.destination_table['table'])
                if table_name in self.table_list.keys():
                    self.destination_table_sel.set_selected(self.table_list.keys().index(table_name))
            self.destination_database_sel.clear()
            self.destination_database_sel.add_items(db_list)
            if self.main.destination_table['schema']:
                self.destination_database_sel.set_selected(db_list.index(self.main.destination_table['schema']))
        
    def create_ui(self):
        self.set_spacing(16)
        
        label = mforms.newLabel("Select destination table and additional options.")
        label.set_style(mforms.BoldInfoCaptionStyle)
        self.content.add(label, False, True)

        
        table_destination_box = mforms.newBox(False)
        table_destination_box.set_spacing(8)

        existing_table_box = mforms.newBox(True)
        existing_table_box.set_spacing(8)
        self.existing_table_radio = mforms.newRadioButton(1)
        self.existing_table_radio.set_text("Use existing table:")
        self.existing_table_radio.add_clicked_callback(self.radio_click)
        if 'table' in self.main.destination_table and self.main.destination_table['table'] is not None:
            self.existing_table_radio.set_active(True)
        existing_table_box.add(self.existing_table_radio, False, True)
        
        self.destination_table_sel = mforms.newSelector()
        self.destination_table_sel.set_size(75, -1)
        existing_table_box.add(self.destination_table_sel, True, True)
        table_destination_box.add(existing_table_box, False, True)
            
        new_table_box = mforms.newBox(True)
        new_table_box.set_spacing(8)
        self.new_table_radio = mforms.newRadioButton(1)
        self.new_table_radio.set_text("Create new table: ")
        self.new_table_radio.add_clicked_callback(self.radio_click)
        if 'table' not in self.main.destination_table or self.main.destination_table['table'] is None:
            self.new_table_radio.set_active(True)
            
        new_table_box.add(self.new_table_radio, False, True)
        self.destination_database_sel = mforms.newSelector()
        self.destination_database_sel.set_size(120, -1)
        new_table_box.add(self.destination_database_sel, False, True)
        new_table_box.add(mforms.newLabel("."), False, True)
        self.new_table_name = mforms.newTextEntry()
        new_table_box.add_end(self.new_table_name, True, True)
        table_destination_box.add(new_table_box, True, True)
        
        def set_trunc(sender):
            global truncate_table
            truncate_table = sender.get_active()
 
        self.truncate_table_cb = mforms.newCheckBox()
        self.truncate_table_cb.set_text("Truncate table before import")
        self.truncate_table_cb.set_active(truncate_table)
        self.truncate_table_cb.add_clicked_callback(lambda sender = self.truncate_table_cb: set_trunc(sender))
        table_destination_box.add(self.truncate_table_cb, False, True)
        
        def set_drop(sender):
            global drop_table
            drop_table = sender.get_active()
            
        self.drop_table_cb = mforms.newCheckBox()
        self.drop_table_cb.set_text("Drop table if exists")
        self.drop_table_cb.set_active(drop_table)
        self.drop_table_cb.add_clicked_callback(lambda sender = self.drop_table_cb: set_drop(sender))
        if self.existing_table_radio.get_active():
            self.drop_table_cb.show(False)
            self.truncate_table_cb.show(True)
        else:
            self.drop_table_cb.show(True)
            self.truncate_table_cb.show(False)
        table_destination_box.add(self.drop_table_cb, False, True)
        
        self.content.add(table_destination_box, False, True)
        
    def radio_click(self):
        if self.new_table_radio.get_active():
            self.drop_table_cb.show(True)
            self.truncate_table_cb.show(False)
        elif self.existing_table_radio.get_active():
            self.drop_table_cb.show(False)
            self.truncate_table_cb.show(True)
            
    def check_server_lower_case_table_names(self):
        rset = self.main.editor.executeManagementQuery("SHOW SESSION VARIABLES LIKE 'lower_case_table_names'", 1)
        if rset and rset.goToFirstRow():
            return rset.intFieldValueByName("Value") != 0 
        return False
    
    def check_if_table_exists(self, schema, table):
        rset = self.main.editor.executeManagementQuery("SHOW TABLES FROM `%s` like '%s'" % (schema, table), 1)
        if rset and rset.goToFirstRow():
            return True
        return False

    def validate(self):
        compare_in_lowercase = self.check_server_lower_case_table_names()
        if self.existing_table_radio.get_active():
            self.main.destination_table = self.table_list[self.destination_table_sel.get_string_value()]
        else:
            self.main.destination_table['schema'] = self.destination_database_sel.get_string_value()
            self.main.destination_table['table'] = self.new_table_name.get_string_value().strip()
            if len(self.main.destination_table['table']) == 0:
                mforms.Utilities.show_error("Table Import", "You need to specify new table name", "Ok", "", "")
                return False 
            
            if compare_in_lowercase:
                self.main.destination_table['table'] = self.main.destination_table['table'].lower()
            
            table_name = "%s.%s" % (self.main.destination_table['schema'], self.main.destination_table['table'])

            if not self.drop_table_cb.get_active() and (table_name in self.table_list or self.check_if_table_exists(self.main.destination_table['schema'], self.main.destination_table['table'])):
                res = mforms.Utilities.show_message("Table Import", "You specified to create a new table, but a table with the same name already exists in the selected schema. Would you like to drop it, or use the existing one and truncate?", "Drop the table", "Use Existing One and Truncate it", "Cancel")
                if res == mforms.ResultOk: 
                    self.drop_table_cb.set_active(True)
                elif res == mforms.ResultCancel:
                    self.truncate_table_cb.set_active(True)
                    self.existing_table_radio.set_active(True)
                    self.destination_table_sel.set_selected(self.table_list.keys().index(table_name))
                else:
                    return False 
        return True

class SelectFileWizardPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Select File to Import", wide=True)

        self.back_button.set_enabled(False)

    def create_ui(self):
        self.set_spacing(16)

        label = mforms.newLabel("Table Data Import allows you to easily import CSV, JSON datafiles.\nYou can also create destination table on the fly.")
        label.set_style(mforms.BoldInfoCaptionStyle)

        self.content.add(label, False, False)

        entry_box = mforms.newBox(True)
        entry_box.set_spacing(5)

        entry_box.add(mforms.newLabel("File Path:"), False, True)

        self.importfile_path = mforms.newTextEntry()
        entry_box.add(self.importfile_path, True, True)
        self.importfile_path.set_value(last_location)

        self.importfile_browse_btn = mforms.newButton()
        self.importfile_browse_btn.set_text("Browse...")
        self.importfile_browse_btn.add_clicked_callback(self.importfile_browse)
        entry_box.add(self.importfile_browse_btn, False, False)

        self.content.add(entry_box, False, True)

    def go_cancel(self):
        self.main.close()


    def validate(self):
        file_path = self.importfile_path.get_string_value()
        if not os.path.isfile(file_path):
            mforms.Utilities.show_error("Invalid Path", "Please specify a valid file path.", "OK", "", "")
            return False
        
        fileName, fileExt = os.path.splitext(os.path.basename(file_path))
        
        for format in self.main.formats:
            if format.name == fileExt[1:]:
                break
        else:
            mforms.Utilities.show_warning("Table Data Import", "This file type is not supported, valid options are: CSV, JSON", "Ok", "", "")
            return False
        
        if fileExt[1:] == "json":
            if os.path.getsize(file_path) > 104857600:
                if mforms.Utilities.show_warning("Table Data Import", "This file appears to be a JSON filetype, and the size is over 100MB, which will take a long time to import. If possible, obtain the data in CSV format instead. Do you wish to continue with this file anyway?", "Continue", "Cancel", "") != mforms.ResultOk:
                    self.importfile_path.set_value("")
                    return False

        return True


    def importfile_browse(self):
        filechooser = FileChooser(mforms.OpenFile)
        filechooser.set_directory(os.path.dirname(self.importfile_path.get_string_value()))
        extensions = []
        for module in self.main.formats:
            extensions.append(module.get_file_extension()[0])
 
        filechooser.set_extensions("|".join(extensions), self.main.formats[0].get_file_extension()[1], False)
        
        if filechooser.run_modal():
            file_path = filechooser.get_path()
            self.importfile_path.set_value(file_path)
            global last_location
            last_location = file_path

class PowerImportWizard(WizardForm):
    def __init__(self, editor, owner, selection = {}):
        WizardForm.__init__(self, mforms.Form.main_form())

        self.editor = editor
        
        self.formats = []
        self.formats.append(create_module("csv", editor, True))
        self.formats.append(create_module("json", editor, True))
        
        self.destination_table = selection
        
        self.center()

        self.select_file_page = SelectFileWizardPage(self)
        self.add_page(self.select_file_page)
        
        self.destination_page = SelectDestinationPage(self)
        self.add_page(self.destination_page)
        
        self.configuration_page = ConfigurationPage(self)
        self.add_page(self.configuration_page)
        
        self.import_progress_page = ImportProgressPage(self)
        self.add_page(self.import_progress_page)
        
        self.result_page = ResultsPage(self)
        self.add_page(self.result_page)
