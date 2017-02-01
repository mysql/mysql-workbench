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
import grt
import threading

import os

from mforms import newTreeView
from mforms import FileChooser
from sqlide_power_import_export_be import create_module
from workbench.ui import WizardForm, WizardPage, WizardProgressPage
from datetime import datetime
import operator


from workbench.log import log_error

last_location = None

def showPowerExport(editor, selection):
    exporter = PowerExportWizard(editor, mforms.Form.main_form(), selection, "Table Data Export")
    exporter.run()

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
        else:
            return

    if user_selection:
        if user_selection['table']:
            item = mforms.newMenuItem("Table Data Export Wizard")
            item.add_clicked_callback(lambda sender=sender : showPowerExport(sender, user_selection))
            menu.insert_item(4, item)


class SimpleTabExport(mforms.Box):
    def __init__(self, editor, owner):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        self.editor = editor
        self.caption = "Simple"
        self.owner = owner
        
        self.columns = []

        self.content = mforms.newBox(False)
        self.add(self.content, True, True)

        self.create_ui()
        
    def create_ui(self):
        self.suspend_layout()
        self.set_spacing(16)
        self.content.set_spacing(16)
        
        colbox = mforms.newBox(False)
        colbox.set_spacing(8)
        colbox.add(mforms.newLabel("Select columns you'd like to export"), False, True)
        
        self.column_list = newTreeView(mforms.TreeFlatList)
        self.column_list.add_column(mforms.CheckColumnType, "Export", 50, True)
        self.column_list.add_column(mforms.StringColumnType, "Column name", self.owner.main.get_width(), False)
        self.column_list.end_columns()
        self.column_list.set_allow_sorting(True)
        self.column_list.set_size(200, -1)
        colbox.add(self.column_list, True, True)
        
        limit_box = mforms.newBox(True)
        limit_box.set_spacing(8)
        limit_box.add(mforms.newLabel("Count: "), False, False)
        self.limit_entry = mforms.newTextEntry()
        self.limit_entry.set_size(50, -1)
        self.limit_entry.add_changed_callback(lambda entry=self.limit_entry: self.entry_changed(entry))
        limit_box.add(self.limit_entry, False, False)
        
        offset_box = mforms.newBox(True)
        offset_box.set_spacing(8)
        offset_box.add(mforms.newLabel("Row Offset: "), False, False)
        self.offset_entry = mforms.newTextEntry()
        self.offset_entry.set_size(50, -1)
        self.offset_entry.add_changed_callback(lambda entry=self.offset_entry: self.entry_changed(entry))

        offset_box.add(self.offset_entry, False, False)
        
        
        sellall_cb = mforms.newCheckBox()
        sellall_cb.set_text("Select / Deselect all entries")
        sellall_cb.set_active(True)
        sellall_cb.add_clicked_callback(lambda cb = sellall_cb: self.sell_all(cb))
        
        limit_offset = mforms.newBox(True)
        limit_offset.set_spacing(8)
        limit_offset.add(sellall_cb, False, True)
        limit_offset.add_end(limit_box, False, True)
        limit_offset.add_end(offset_box, False, True)

        colbox.add(limit_offset, False, True)
        self.content.add(colbox, True, True)
        self.resume_layout()
    
    def sell_all(self, checkbox):
        for i in range(self.column_list.count()):
            self.column_list.node_at_row(i).set_bool(0, checkbox.get_active())
        
    
    def entry_changed(self, control):
        txt = str(control.get_string_value())
        if len(txt) and not txt.isdigit():
            mforms.Utilities.show_warning("Table Data Export", "Offset and Limit field can contain only numbers", "OK", "", "")
            control.set_value("".join([s for s in list(txt) if s.isdigit()])) 
 
    def set_columns(self, cols):
        self.column_list.freeze_refresh()
        self.column_list.clear()
        self.columns = cols
        for col in self.columns:
            node = self.column_list.add_node()
            node.set_bool(0, True)
            node.set_string(1, col['name'])     
        self.column_list.thaw_refresh()
        
    def get_query(self):
        selected_columns = []
        for i in range(self.column_list.count()):
            if self.column_list.node_at_row(i).get_bool(0):
                selected_columns.append(self.column_list.node_at_row(i).get_string(1))

        limit = ""
        if self.limit_entry.get_string_value():
            limit = "LIMIT %d" % int(self.limit_entry.get_string_value())
            if self.offset_entry.get_string_value():
                limit = "LIMIT %d,%d" % (int(self.offset_entry.get_string_value()), int(self.limit_entry.get_string_value()))
        table_w_prefix = "%s.%s" % (self.owner.main.source_table['schema'], self.owner.main.source_table['table'])
        return """SELECT %s FROM %s %s""" % (",".join(selected_columns), table_w_prefix, limit)
        
class AdvancedTabExport(mforms.Box):
    def __init__(self, editor, owner):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        self.editor = editor
        self.caption = "Advanced"
        self.owner = owner
        
        self.content = mforms.newBox(False)
        self.add(self.content, True, True)
        
        self.create_ui()
        
    def create_ui(self):
        box = mforms.newBox(False)
        box.set_spacing(8)
        lbl_box = mforms.newBox(False)
        lbl_box.set_spacing(8)
        lbl_box.add(mforms.newLabel("Type query that will be used as a base for export."), False, True)
        box.add(lbl_box, False, True)
        self.code_editor = mforms.CodeEditor()
        self.code_editor.set_language(mforms.LanguageMySQL)
        self.code_editor.set_managed()
        self.code_editor.set_release_on_add()
        box.add(self.code_editor, True, True)
        self.content.add(box, True, True)
        
    def set_query(self, query):
        self.code_editor.set_text(query)
    
    def get_query(self):
        return self.code_editor.get_text(False) 
    
    @property
    def is_dirty(self):
        return self.code_editor.is_dirty()
    
    def reset_dirty(self):
        self.code_editor.reset_dirty()

class ResultsPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Export Results")
        self.next_button.set_text('Finish')
            
    def go_next(self):
        self.main.close()
        
    def get_path(self):
        return self.main.select_file_page.exportfile_path.get_string_value()

    def create_ui(self):
        if self.main.export_progress_page.export_time:
            itime = float("%d.%d" % (self.main.export_progress_page.export_time.seconds, self.main.export_progress_page.export_time.microseconds))
            self.content.add(mforms.newLabel(str("File %s was exported in %.3f s" % (self.get_path(), itime))), False, True)
            
        self.content.add(mforms.newLabel(str("Exported %d records" % self.main.export_progress_page.module.item_count)), False, True)
        
class ExportProgressPage(WizardProgressPage):
    def __init__(self, owner):
        WizardProgressPage.__init__(self, owner, "Export Data")
        
        self.add_task(self.prepare_export, "Prepare Export")
        self.add_threaded_task(self.start_export, "Export data to file")
        self.module = None
        self.stop = None
        self.export_time = None
        
    def prepare_export(self):
        self.module = self.main.select_file_page.active_module
        self.stop = threading.Event()
        self.module.set_table(self.main.source_table['schema'], self.main.source_table['table'])
        self.module.set_filepath(self.main.select_file_page.output_file)
        if not self.main.data_input_page._showing_simple:
            self.module.set_columns([])
            query = self.main.data_input_page.get_query()
            if len(query):
                self.module.set_user_query(query)
        else:
            self.module.set_user_query(None)
            self.module.set_limit(self.main.data_input_page.get_limit())
            self.module.set_offset(self.main.data_input_page.get_offset())
            self.module.set_columns(self.main.data_input_page.get_columns())
            self.module.set_local(self.main.select_file_page.export_local)

        self.module.progress_info = self.progress_notify
        return True
    
    def progress_notify(self, pct, msg):
        self.send_progress(pct, msg)
        
    def start_export(self):
        self.export_time = None
        start = datetime.now()
        retval = self.module.start(self.stop)
        self.export_time = datetime.now() - start
        return retval
        
    def page_activated(self, advancing):
        self.reset(True)
        self.module = None
        self.stop = None
        super(ExportProgressPage, self).page_activated(advancing)
        
        
    def go_cancel(self):
        if self.on_close():
            self.main.close()
                
    def on_close(self):
        if self.module and self.module.is_running:
            if mforms.ResultOk == mforms.Utilities.show_message("Confirmation", "Do you wish to stop the export process?", "Yes", "No", ""):
                self.stop.set()
                return True
            return False
        return True
        
class SelectFilePage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Select output file location")
        if self.main.formats:
            self.active_module = self.main.formats[0] # We set first module as the active one
        else:
            self.active_module = None
        self.unsupported_output_format = False
        self.confirm_file_overwrite = False
        self.radio_opts = []
        self.optbox = None
        self.destination_file_checked = False
        
    def page_activated(self, advancing):
        super(SelectFilePage, self).page_activated(advancing)
        self.export_local_box.show(bool(self.main.data_input_page._showing_simple))
            
    def create_ui(self):
        self.suspend_layout()
        self.set_spacing(16)
        
        label = mforms.newLabel("Table Data Export allows you to easily export data into CSV, JSON datafiles.\n")
        label.set_style(mforms.BoldInfoCaptionStyle)

        self.content.add(label, False, False)

        entry_box = mforms.newBox(True)
        entry_box.set_spacing(5)

        entry_box.add(mforms.newLabel("File Path:"), False, True)

        self.exportfile_path = mforms.newTextEntry()
        self.exportfile_path.add_changed_callback(lambda entry=self.exportfile_path: self.entry_changed(entry))
        entry_box.add(self.exportfile_path, True, True)
        if last_location != None:
            self.exportfile_path.set_value(last_location)
            self.confirm_file_overwrite = True
            self.get_module(True)

        browse_btn = mforms.newButton()
        browse_btn.set_text("Browse...")
        browse_btn.add_clicked_callback(self.browse)
        entry_box.add(browse_btn, False, False)
        self.content.add(entry_box, False, True)
        
        radio_box = mforms.newBox(True)
        radio_box.set_spacing(8)
        for format in self.main.formats:
            fradio = mforms.newRadioButton(1)
            fradio.set_text(format.title)
            fradio.set_active(bool(self.active_module and self.active_module.name == format.name))
            fradio.add_clicked_callback(lambda f = format: self.output_type_changed(f))
            radio_box.add(fradio, False, True)
            self.radio_opts.append({'radio':fradio, 'name': format.name})
        self.content.add(radio_box, False, True)

        self.optpanel = mforms.newPanel(mforms.TitledBoxPanel)
        self.optpanel.set_title("Options:")

        self.content.add(self.optpanel, False, True)
        self.optpanel.show(False)
        
        self.export_local_box = mforms.newBox(False)
        self.export_local_cb = mforms.newCheckBox()
        self.export_local_cb.set_text("Export to local machine")
        self.export_local_cb.set_active(True)
        self.export_local_box.add(self.export_local_cb, False, True)
        l = mforms.newLabel("""If checked, rows will be exported on the location that started Workbench.\nIf not checked, rows will be exported on the server.\nIf server and computer that started Workbench are different machines, import of that file can be done manual way only.""")
        l.set_style(mforms.SmallHelpTextStyle)
        self.export_local_box.add(l, False, True)
        
        self.content.add(self.export_local_box, False, True)
        self.resume_layout()
        
        self.load_module_options()
    
    def entry_changed(self, entry):
        if len(entry.get_string_value()) > 1:
            self.destination_file_checked = False

    @property
    def export_local(self):
        return self.main.data_input_page._showing_simple and self.export_local_cb.get_active()
    
    @property
    def output_file(self):
        return str(self.exportfile_path.get_string_value())
        
    def output_type_changed(self, format):
        self.active_module = format
        self.load_module_options()
        
    def validate(self):
        if not self.check_is_supported_format():
            mforms.Utilities.show_error(self.main.title, "This file format is not supported, please select CSV or JSON.", "Ok", "", "")
            return False            
        file_path = self.exportfile_path.get_string_value()
        if not os.path.isdir(os.path.dirname(file_path)):
            mforms.Utilities.show_error(self.main.title, "Please specify a valid file path.", "OK", "", "")
            return False
        if self.confirm_file_overwrite and os.path.isfile(file_path):
            self.destination_file_checked = True
            if mforms.Utilities.show_warning(self.main.title, "The output file already exists. Do you want to overwrite it?", "Yes", "No", "") != mforms.ResultOk:
                return False
            self.confirm_file_overwrite = False
        if self.main.data_input_page._showing_simple and not self.export_local_cb.get_active():
           if mforms.Utilities.show_warning(self.main.title, "You've specified to export file on the server. Export may fail if the file already exists. Please remove the file manually before continuing.", "Continue", "Cancel", "") != mforms.ResultOk:
               return False
        elif not self.destination_file_checked and os.path.isfile(file_path):
            self.destination_file_checked = True
            if mforms.Utilities.show_warning(self.main.title, "The output file already exists, do you want to overwrite it?", "Yes", "No", "") != mforms.ResultOk:
                return False
        global last_location
        last_location = file_path
        return True
    
    def check_is_supported_format(self):
        file_name, file_ext = os.path.splitext(os.path.basename(self.exportfile_path.get_string_value()))
        self.input_file_type = file_ext[1:]
        for format in self.main.formats:
            if format.name == self.input_file_type:
                return format
        return None

    def get_module(self, silent = False):
        self.unsupported_output_format = False
        format = self.check_is_supported_format()
        if format:
            self.active_module = format 
        else:
            self.unsupported_output_format = True
            self.active_module = self.main.formats[0] # we use first format as default one
            if not silent:
                mforms.Utilities.show_error(self.main.title, "This file format is not supported. Please select CSV or JSON.", "Ok", "", "")
    
    def load_module_options(self):
        self.suspend_layout()
        
        self.optpanel.show(False)
        if self.optbox:
            self.optpanel.remove(self.optbox)
        if self.active_module and len(self.active_module.options) != 0:
            def set_text_entry(field, output):
                txt = field.get_string_value().encode('utf-8').strip()
                if len(txt) == 0:
                    operator.setitem(output, 'value', None)
                elif len(txt) == 1:
                    operator.setitem(output, 'value', txt)
                else:
                    field.set_value("")
                    mforms.Utilities.show_error(self.main.title, "Due to the nature of this wizard, you can't use unicode characters in this place, as only one character is allowed.","Ok","","")
                    

            def set_selector_entry(selector, output):
                operator.setitem(output, 'value', output['opts'][str(selector.get_string_value())])
            
            self.optbox = mforms.newBox(False)
            self.optbox.set_spacing(8)
            self.optbox.set_padding(8)
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
                if opts['type'] == 'select':
                    opt_val = mforms.newSelector()
                    opt_val.set_size(75, -1)
                    opt_val.add_items([v for v in opts['opts']])
                    opt_val.set_selected(opts['opts'].values().index(opts['value']))
                    opt_val.add_changed_callback(lambda selector = opt_val, output = opts: set_selector_entry(selector, output))
                    label_box.add_end(opt_val, False, True)
                self.optbox.add(label_box, False, True)
            self.optpanel.add(self.optbox)
            self.optpanel.show(True)        
        self.resume_layout()
        
    def go_cancel(self):
        self.main.close()
        
        
    def browse(self):
        filechooser = FileChooser(mforms.SaveFile)
        filechooser.set_directory(os.path.dirname(self.exportfile_path.get_string_value()))
        extensions = []
        for module in self.main.formats:
            extensions.append(module.get_file_extension()[0])
 
        filechooser.set_extensions("|".join(extensions), self.active_module.get_file_extension()[1], False)
        
        if filechooser.run_modal():
            file_path = filechooser.get_path()
            self.exportfile_path.set_value(file_path)
            self.destination_file_checked = True
            global last_location
            last_location = file_path
            self.confirm_file_overwrite = False
            self.get_module()
            for opt in self.radio_opts:
                if self.active_module and opt['name'] == self.active_module.name:
                    opt['radio'].set_active(True) 
            self.load_module_options()

        
class DataInputPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Select data for export")
        self.simple_export = None
        self.advanced_export = None
        self._showing_simple = True
        self.table_list = {}
        
    def create_ui(self):
        self.suspend_layout()
        self.set_spacing(16)
        
        headingBox = mforms.newBox(True)
        headingBox.set_spacing(16)
        
        self.simple_export_box = mforms.newBox(False)
        self.simple_export_box.set_spacing(16)
        
        label = mforms.newLabel("Select source table for export:")
        label.set_style(mforms.BoldInfoCaptionStyle)
        headingBox.add(label, False, True)
        
        self.source_table_sel = mforms.newSelector()
        self.source_table_sel.set_size(self.get_width(), -1)
        self.preload_existing_tables()
        sorted_keys = self.table_list.keys()
        sorted_keys.sort()
        self.source_table_sel.add_items(sorted_keys)
        table_name = "%s.%s" % (self.main.source_table['schema'], self.main.source_table['table'])
        if table_name in self.table_list.keys():
            self.source_table_sel.set_selected(sorted_keys.index(table_name))
        self.source_table_sel.add_changed_callback(lambda selector = self.source_table_sel: self.source_table_changed(selector.get_string_value()))
        headingBox.add(self.source_table_sel, False, True)
        
        self.simple_export_box.add(headingBox, False, True)
        
        self.simple_export = SimpleTabExport(self.main.editor, self)
        self.simple_export_box.add(self.simple_export, True, True)
        self.content.add(self.simple_export_box, True, True)
        
        self.advanced_export = AdvancedTabExport(self.main.editor, self)
        self.advanced_export.show(False)
        self.content.add(self.advanced_export, True, True)
        self.resume_layout()
        
        self.preload_table_info()
        
        
    def get_table_columns(self, table):
        cols = []
        try:
            rset = self.main.editor.executeManagementQuery("SHOW COLUMNS FROM `%s`.`%s`" % (table['schema'], table['table']), 1)
        except grt.DBError, e:
            log_error("SHOW COLUMNS FROM `%s`.`%s` : %s" % (table['schema'], table['table'], e))
            rset = None
            
        if rset:
            ok = rset.goToFirstRow()
            while ok:
                col = {'name': None, 'type': None, 'is_string': None, 'is_geometry':None, 'is_bignumber':None, 'is_number': None, 'is_date_or_time': None, 'is_bin': None, 'value': None}
                col['name'] = rset.stringFieldValueByName("Field")
                col['type'] = rset.stringFieldValueByName("Type")
                col['is_number'] = any(x in col['type'] for x in ['int', 'integer'])
                col['is_geometry'] = any(x in col['type'] for x in ['geometry','geometrycollection', 'linestring', 'multilinestring', 'multipoint', 'multipolygon', 'point' , 'polygon'])
                col['is_bignumber'] = any(x in col['type'] for x in ['bigint'])
                col['is_float'] = any(x in col['type'] for x in ['decimal', 'float', 'double'])  
                col['is_string'] = any(x in col['type'] for x in ['char', 'text', 'set', 'enum', 'json'])
                col['is_bin'] = any(x in col['type'] for x in ['blob', 'binary'])  
                col['is_date_or_time'] = any(x in col['type'] for x in ['timestamp', 'datetime', 'date', 'time'])
                cols.append(col)
                ok = rset.nextRow()
        return cols
    
    def source_table_changed(self, table):
        self.main.source_table = self.table_list[table]
        self.preload_table_info()
        
    def preload_table_info(self):
        self.simple_export.set_columns(self.get_table_columns(self.main.source_table))
    
    def preload_existing_tables(self):
        self.table_list = {}
       
        rset = self.main.editor.executeManagementQuery("SHOW TABLES FROM `%s`" % self.main.source_table['schema'], 0)
        if rset:
            ok = rset.goToFirstRow()
            while ok:
                self.table_list["%s.%s" % (self.main.source_table['schema'], rset.stringFieldValue(0))] = {'schema': self.main.source_table['schema'], 'table': rset.stringFieldValue(0)}
                ok = rset.nextRow()
        
    def go_advanced(self):
        if not self._showing_simple and self.advanced_export.is_dirty:
            if mforms.Utilities.show_warning("Table Data Export", "Code editor was modified. If you continue, your changes will be lost. Do you want to continue?", "Continue", "Cancel", "") != mforms.ResultOk:
                return
         
        self._showing_simple = not self._showing_simple
        if self._showing_simple:
            self.advanced_button.set_text("Advanced >>")
        else:
            self.advanced_button.set_text("Simple >>")
        
        self.simple_export_box.show(bool(self._showing_simple))
        self.advanced_export.show(not self._showing_simple)
        
        if not self._showing_simple:
            self.advanced_export.set_query(str(self.simple_export.get_query()))
            self.advanced_export.reset_dirty()
    
    def go_cancel(self):
        self.main.close()

    def get_query(self):
        return self.advanced_export.get_query()
    
    def get_limit(self):
        return int(self.simple_export.limit_entry.get_string_value()) if self.simple_export.limit_entry.get_string_value() else 0        

    def get_offset(self):
        return int(self.simple_export.offset_entry.get_string_value()) if self.simple_export.offset_entry.get_string_value() else 0
    
    def get_columns(self):
        cols = []
        for r in range(self.simple_export.column_list.count()):
            node = self.simple_export.column_list.node_at_row(r)
            if node.get_bool(0):
                for col in self.simple_export.columns:
                    if col['name'] == node.get_string(1):
                        cols.append(col)
        return cols
    
    def validate(self):
        if len(self.get_columns()) == 0:
            mforms.Utilities.show_message(self.main.title, "You need to specify at least one column", "Ok", "","")
            return False
        return True

class PowerExportWizard(WizardForm):
    def __init__(self, editor, owner, selection = {}, title = None):
        WizardForm.__init__(self, mforms.Form.main_form())

        self.title = title
        self.set_title(title)

        self.editor = editor
        
        self.formats = []
        self.formats.append(create_module("csv", editor, False))
        self.formats.append(create_module("json", editor, False))
        
        self.source_table = selection
        
        self.center()

        self.data_input_page = DataInputPage(self)
        self.add_page(self.data_input_page)
        
        self.select_file_page = SelectFilePage(self)
        self.add_page(self.select_file_page)
        
        self.export_progress_page = ExportProgressPage(self)
        self.add_page(self.export_progress_page)
        
        self.result_page = ResultsPage(self)
        self.add_page(self.result_page)
        

