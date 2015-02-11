# Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

import sys, os, csv

from mforms import newTreeNodeView
from mforms import FileChooser
import datetime
import operator


from workbench.log import log_debug3, log_debug2, log_error

last_location = ""

def showPowerImport(editor, selection):
    importer = PowerImport(editor, mforms.Form.main_form(), selection)
    importer.set_title("Table Data Import")
    importer.run()

def showPowerExport(editor, selection):
    exporter = PowerExport(editor, mforms.Form.main_form())
    exporter.set_source(selection)
    exporter.set_title("Table Data Export")
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

    if user_selection['table']:
        item = mforms.newMenuItem("Table Data Export")
        item.add_clicked_callback(lambda sender=sender : showPowerExport(sender, user_selection))
        menu.insert_item(3, item)

    item = mforms.newMenuItem("Table Data Import")
    item.add_clicked_callback(lambda sender=sender : showPowerImport(sender, user_selection))
    menu.insert_item(4, item)
    
    menu.insert_item(5, mforms.newMenuItem("", mforms.SeparatorMenuItem))
    
class WorkerThread:
    def __init__(self, module):
        self.is_running = False
        self.stop = threading.Event()
        self.thread = threading.Thread()
        self.thread.run = self._run

        self.exception = None
        self.module = module
        self.finished_callback = None

    def call_finished_callback(self):
        if self.is_running:
            return True

        if self.finished_callback:
            self.finished_callback(True if not self.exception else False)

        return False

    def start(self):
		if not self.is_running:
			self.exception = None
			self._timeout_handle = mforms.Utilities.add_timeout(0.5, lambda: self.call_finished_callback())
 			self.thread.start()

    def _run(self):
        if self.is_running:
            return
        self.is_running = True
        
        try:
            self.module.start(self.stop)
        except Exception, e:
            import traceback
            log_error("WorkerThread caught exception: %s" % traceback.format_exc())
            self.exception = e

        self.is_running = False
        
    def join(self, timeout = 0):
        if self.thread and self.is_running:
            self.thread.join(timeout)
    
    def is_alive(self):
        if self.thread and self.is_running:
            return self.thread.is_alive()
        return False

class base_module:
    def __init__(self, editor, is_import):
        self.name = ""
        self.title = self.name
        self.options = {};
        self._offset = None
        self._limit = None
        self._table_w_prefix = None
        self._columns = []
        self._filepath = None
        self._extension = None
        self._allow_remote = False
        self._editor = editor
        self._local = True
        self._mapping = []
        self._new_table = False
        self._last_analyze = False
        self._is_import = is_import
        self._current_row = 0
        self._max_rows = 0
        self._thread_event = None
        self._user_query = None
        self._decimal_separator = ','
        self._date_format = '%Y-%m-%d %H:%M:%S'
        self._encoding = 'utf-8' #default encoding
        self._force_drop_table = False
        
    def force_drop_table(self, force):
        self._force_drop_table = force

    def set_encoding(self, encoding):
        self._encoding = encoding
        
    def get_current_row(self):    
        return self._current_row
    
    def set_user_query(self, query):
        self._user_query = query

    def get_max_row(self):
        return self._max_rows

    def create_new_table(self, create):
        self._new_table = create

    def set_table(self, schema, table):
        if schema:
            self._table_w_prefix = "%s.%s" % (schema, table)
        else:
            self._table_w_prefix = str(table)
        
    def set_mapping(self, mapping):
        self._mapping = mapping
    
    def allow_remote(self):
        return self._allow_remote;
    
    def get_file_extension(self):
        return self._extension
    
    def set_columns(self, cols):
        self._columns = cols        
    
    def set_filepath(self, filename):
        self._filepath = filename
    
    def set_limit(self, limit):
        self._limit = limit;
        
    def set_offset(self, offset):
        self._offset = offset;
    
    def set_local(self, local):
        if self._allow_remote:
            self._local = local
            
    def set_decimal_separator(self, separator):
        self._decimal_separator = separator
    
    def set_date_format(self, format):
        self._date_format = format
    
    def read_user_query_columns(self, result):
        self._columns = []
        for c in result.columns:
            self._columns.append({'name': c.name, 'type': c.columnType, 
                   'is_string': True if c.columnType == "string" else False, 
                   'is_number': True if c.columnType == "int" else False, 
                   'is_date_or_time': any(x in c.columnType for x in ['time', 'datetime', 'date']), 
                   'is_bin': any(x in c.columnType for x in ['geo', 'blob']),
                   'is_float': True if c.columnType == "real" else False,
                   'value': None})
    
    def prepare_new_table(self):
        try:
            self._editor.executeManagementCommand(""" CREATE TABLE %s (%s)""" % (self._table_w_prefix, ", ".join(["%s %s" % (col['name'], col['type']) for col in self._mapping])), 1)
            # wee need to setup dest_col for each row, as the mapping is empty if we're creating new table
            for col in self._mapping:
                col['dest_col'] = col['name']
            return True
        except Exception, e:
            log_error("Error creating table for import: %s" % e)
            if len(e.args) == 2 and e.args[1] == 1050 and self._force_drop_table:
                try:
                    self._editor.executeManagementCommand(""" DROP TABLE %s""" % self._table_w_prefix, 1)
                    self.prepare_new_table()
                    return True
                except:
                    log_error("Error dropping table for import: %s" % e)
                    raise
            raise
        
    
    def get_command(self):
        return False
    
    def start_export(self):
        return False
    
    def start_import(self):
        return False
    
    def start(self, event):
        self._thread_event = event
        if self._is_import:
            self.start_import()
        else:
            self.start_export()

class Utf8Reader:
    def __init__(self, f, enc):
        import codecs
        self.reader = codecs.getreader(enc)(f)

    def __iter__(self):
        return self

    def next(self):
        return self.reader.next().encode("utf-8")

class UniReader:
    def __init__(self, f, dialect=csv.excel, encoding="utf-8", **args):
        f = Utf8Reader(f, encoding)
        self.csvreader = csv.reader(f, dialect=dialect, **args)

    def next(self):
        row = self.csvreader.next()
        return [unicode(s, "utf-8") for s in row]
    
    @property
    def line_num(self):
        return self.csvreader.line_num

    def __iter__(self):
        return self

class csv_module(base_module):
    def __init__(self, editor, is_import):
        self.dialect = None
        self.has_header = False

        base_module.__init__(self, editor, is_import)
        self.name = "csv"
        self.title = self.name
        self.options = {'filedseparator': {'description':'Field Separator', 'type':'select', 'opts':{'TAB':'\t',';':';', ':':':'}, 'value':';', 'entry': None}, 
                'lineseparator': {'description':'Line Separator', 'type':'select','opts':{"CR":'\r', "CR LF":'\r\n', "LF":'\n'}, 'value':'\n', 'entry': None}, 
                'encolsestring': {'description':'Enclose Strings in', 'type':'text', 'value':'"', 'entry': None}};
        
        self._extension = ["Comma Separated Values (*.csv)|*.csv", "csv"]
        self._allow_remote = True 
    
    def get_query(self):
        if self._local:
            limit = ""
            if self._limit:
                limit = "LIMIT %d" % int(self._limit)
                if self._offset:
                    limit = "LIMIT %d,%d" % (int(self._offset), int(self._limit))
            return """SELECT %s FROM %s %s""" % (",".join([value['name'] for value in self._columns]), self._table_w_prefix, limit)
        else:
            limit = ""
            if self._limit:
                limit = "LIMIT %d" % int(self._limit)
                if self._offset:
                    limit = "LIMIT %d,%d" % (int(self._offset), int(self._limit))
            fpath = self._filepath
            if sys.platform.lower() == "win32":
                fpath.replace("\\","\\\\")

            return """SELECT %s FROM %s INTO OUTFILE '%s' 
                        FIELDS TERMINATED BY '%s' 
                        ENCLOSED BY '%s' 
                        LINES TERMINATED BY %s %s""" % (",".join([value['name'] for value in self._columns]), self._table_w_prefix, fpath,
                                                       self.options['filedseparator']['value'], self.options['encolsestring']['value'], repr(self.options['lineseparator']['value']), limit)

    def start_export(self):
        if self._user_query:
            query = self._user_query
        else:
            query = self.get_query()

        if self._local:
            rset = self._editor.executeManagementQuery(query, 1)
            if rset:
                if self._user_query: #We need to get columns info
                    self.read_user_query_columns(rset)
                    
                self._max_rows = rset.rowCount
                with open(self._filepath, 'wb') as csvfile:
                    output = csv.writer(csvfile, delimiter = self.options['filedseparator']['value'], 
                                        lineterminator = self.options['lineseparator']['value'], 
                                        quotechar = self.options['encolsestring']['value'], quoting = csv.QUOTE_NONNUMERIC)
                    output.writerow([value['name'].encode('utf-8') for value in self._columns])
                    ok = rset.goToFirstRow()
                    while ok:
                        if self._thread_event and self._thread_event.is_set():
                            log_debug2("Worker thread was stopped by user")
                            break

                        self._current_row = rset.currentRow + 1
                        row = []
                        for col in self._columns:
                            if col['is_number']:
                                row.append(rset.intFieldValueByName(col['name']))
                            elif col['is_float']:
                                row.append(rset.floatFieldValueByName(col['name']))
                            else:
                                row.append(rset.stringFieldValueByName(col['name']))
                        output.writerow(row)
                        csvfile.flush()
                        ok = rset.nextRow()
        else:
            self._editor.executeManagementCommand(query, 1)
    
    def start_import(self):
        if not self._last_analyze:
            return
        
        if self._new_table:
            if not self.prepare_new_table():
                return
            
        with open(self._filepath, 'rb') as csvfile:
            dest_col_order = list(set([i['dest_col'] for i in self._mapping if i['active']]))
            query = """PREPARE stmt FROM 'INSERT INTO %s (%s) VALUES(%s)'""" % (self._table_w_prefix, ",".join(dest_col_order), ",".join(["?" for i in dest_col_order]))
            col_order = dict([(i['dest_col'], i['col_no']) for i in self._mapping if i['active']])
            col_type = dict([(i['name'], i['type']) for i in self._mapping if i['active']])
            
            self._editor.executeManagementCommand(query, 1)
            try:
                is_header = self.has_header
                reader = UniReader(csvfile, self.dialect, encoding=self._encoding)
                
                for row in reader:
                    if self._thread_event and self._thread_event.is_set():
                        log_debug2("Worker thread was stopped by user")
                        break
                    
                    self._current_row = reader.line_num
                    if is_header:
                        is_header = False
                        continue
                    
                    for i, col in enumerate(col_order):
                        if col_order[col] not in row:
                            log_error("Can't find col: %s in row: %s" % (col_order[col], row))
                            break
                        val = row[col_order[col]]
                        if col_type[col] == 'double':
                            val = row[col_order[col]].replace(self._decimal_separator, ',')
                        elif col_type[col] == 'datetime':
                            val = datetime.datetime.strptime(row[col_order[col]], self._date_format).strftime("%Y-%m-%d %H:%M:%S")

                        self._editor.executeManagementCommand("""SET @a%d = "%s" """ % (i, val), 0)
                    else:
                        try:
                            self._editor.executeManagementCommand("EXECUTE stmt USING %s" % ", ".join(['@a%d' % i for i, col in enumerate(col_order)]), 0)
                        except Exception, e:
                            log_error("Row import failed with error: %s" % e)
                        
            except Exception, e:
                import traceback
                log_debug3("Import failed traceback: %s" % traceback.format_exc())
                log_error("Import failed: %s" % e)
            self._editor.executeManagementCommand("DEALLOCATE PREPARE stmt", 1)

    def analyze_file(self):
        with open(self._filepath, 'rb') as csvfile:
            if self.dialect is None:
                csvsample = []
                for i in range(0,2): #read two lines as a sample
                    csvsample.append(csvfile.readline())
                
                csvsample = "".join(csvsample)
                self.dialect = csv.Sniffer().sniff(csvsample)
                self.has_header = csv.Sniffer().has_header(csvsample)
                csvfile.seek(0)
            else:
                self.dialect.delimiter = self.options['filedseparator']['value']
                self.dialect.lineterminator = self.options['lineseparator']['value']
                self.dialect.quotechar = self.options['encolsestring']['value']
                
            try:
                reader = UniReader(csvfile, self.dialect, encoding=self._encoding)
                self._columns = []
                row = None
                try:
                    row = reader.next()
                except StopIteration, e:
                    pass
                
                if row:
                    for col_value in row:
                        col = {'name': None, 'type': None, 'is_string': None, 'is_number': None, 'is_date_or_time': None, 'is_bin': None, 'value': []}
                        col['name'] = col_value 
                        col['type'] = "varchar"
                        col['is_number'] = False
                        col['is_float'] = False 
                        col['is_string'] = True
                        col['is_bin'] = False  
                        self._columns.append(col)

                    for i, row in enumerate(reader): #we will read only first few rows
                        if i < 5:
                            for j, col_value in enumerate(row):
                                self._columns[j]['value'].append(col_value)
                        else:
                            break

            except (UnicodeError, UnicodeDecodeError), e:
                import traceback
                log_error("Error analyzing file, probably encoding issue: %s\n Traceback is: %s" % (e, traceback.format_exc()))
                self._last_analyze = False
                return False
                
        self._last_analyze = True
        return True
        
class json_module(base_module):
    def __init__(self, editor, is_import):
        base_module.__init__(self, editor, is_import)
        self.name = "json"
        self.title = self.name
        self._extension = ["JavaScript Object Notation (*.json)|*.json", "json"]
        self._allow_remote = False
        
    def get_query(self):
        limit = ""
        if self._limit:
            limit = "LIMIT %d" % int(self._limit)
            if self._offset:
                limit = "LIMIT %d,%d" % (int(self._offset), int(self._limit))
        return """SELECT %s FROM %s %s""" % (",".join([value['name'] for value in self._columns]), self._table_w_prefix, limit)                
    
    def start_export(self):
        if self._user_query:
            query = self._user_query
        else:
            query = self.get_query()

        rset = self._editor.executeManagementQuery(query, 1)
        if rset:
            if self._user_query: #We need to get columns info
                self.read_user_query_columns(rset)
                
            with open(self._filepath, 'wb') as jsonfile:
                jsonfile.write('[')
                ok = rset.goToFirstRow()
                self._max_rows = rset.rowCount
                while ok:
                    if self._thread_event and self._thread_event.is_set():
                        log_debug2("Worker thread was stopped by user")
                        break

                    self._current_row = rset.currentRow + 1
                    row = []
                    for col in self._columns:
                        if col['is_string'] or col['is_bin']:
                            row.append("\"%s\":\"%s\"" % (col['name'], rset.stringFieldValueByName(col['name'])))
                        elif col['is_number']:
                            row.append("\"%s\":\"%s\"" % (col['name'], rset.intFieldValueByName(col['name'])))
                        elif col['is_float']:
                            row.append("\"%s\":\"%s\"" % (col['name'], rset.floatFieldValueByName(col['name'])))
                    ok = rset.nextRow()
                    jsonfile.write("{%s}%s" % (', '.join(row), ",\n " if ok else ""))
                    jsonfile.flush()
                jsonfile.write(']')

    def start_import(self):
        if not self._last_analyze:
            return

        if self._new_table:
            if not self.prepare_new_table():
                return
        
        with open(self._filepath, 'rb') as jsonfile:
            import json
            data = json.load(jsonfile)
            dest_col_order = list(set([i['dest_col'] for i in self._mapping if i['active']]))
            query = """PREPARE stmt FROM 'INSERT INTO %s (%s) VALUES(%s)'""" % (self._table_w_prefix, ",".join(dest_col_order), ",".join(["?" for i in dest_col_order]))
            col_order = dict([(i['dest_col'], i['name']) for i in self._mapping if i['active']])
            col_type = dict([(i['name'], i['type']) for i in self._mapping if i['active']])
            self._editor.executeManagementCommand(query, 1)
            try:
                self._max_rows = len(data)
                for row in data:
                    if self._thread_event and self._thread_event.is_set():
                        log_debug2("Worker thread was stopped by user")
                        break

                    self._current_row = self._current_row + 1
                    for i, col in enumerate(col_order):
                        if col_order[col] not in row:
                            log_error("Can't find col: %s in row: %s" % (col_order[col], row))
                            break
                        val = row[col_order[col]]
                        if col_type[col] == 'double':
                            val = row[col_order[col]].replace(self._decimal_separator, ',')
                        elif col_type[col] == 'datetime':
                            val = datetime.datetime.strptime(row[col_order[col]], self._date_format).strftime("%Y-%m-%d %H:%M:%S")
                            
                        self._editor.executeManagementCommand("""SET @a%d = "%s" """ % (i, val), 0)
                    else:
                        try:
                            self._editor.executeManagementCommand("EXECUTE stmt USING %s" % ", ".join(['@a%d' % i for i, col in enumerate(col_order)]), 0)
                        except Exception, e:
                            log_error("Row import failed with error: %s" % e)
                        
            except Exception, e:
                log_error("Import failed: %s" % e)
            self._editor.executeManagementCommand("DEALLOCATE PREPARE stmt", 1)

    def analyze_file(self):
        import json
        data = []
        with open(self._filepath, 'r') as f:
            prevchar = None
            stropen = False
            inside = 0
            datachunk = []
            while True:
                c = f.read(1)
                if c == "":
                    break
            
                if c == '"' and prevchar != '\\':
                    stropen = True if stropen == False else False
            
                if stropen == False:
                    if c == '{' and prevchar != '\\':
                        inside = inside + 1
                    if c == '}' and prevchar != '\\':
                        if inside != 1:
                            inside = inside - 1
                        else:
                            datachunk.append(c)
                            datachunk.append(']')
                            break
            
                datachunk.append(c)
                prevchar = c
            try:
                data = json.loads("".join(datachunk))
            except Exception, e:
                log_error("Unable to parse json file: %s,%s " % (self._filepath, e))
                self._last_analyze = False
                return False
        if len(data) != 1:
            log_error("Json file contains no data, or data is inalivd: %s" % (self._filepath))
            self._last_analyze = False
            return False
        
        self._columns = []
        for elem in data[0]:
            self._columns.append({'name': elem, 'type': 'varchar', 'is_string': True, 'is_number': False, 'is_date_or_time': None, 'is_bin': False, 'is_float':False, 'value': [data[0][elem]]})

        self._last_analyze = True
        return True
        

def create_module(type, editor, is_import):
    if type == "csv":
        return csv_module(editor, is_import);
    if type == "json":
        return json_module(editor, is_import);

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
        self.set_spacing(16)
        self.content.set_padding(16)
        self.content.set_spacing(16)
        
        colbox = mforms.newBox(False)
        colbox.set_spacing(8)
        colbox.add(mforms.newLabel("Select columns you'd like to export"), False, True)
        
        self.column_list = newTreeNodeView(mforms.TreeFlatList)
        self.column_list.add_column(mforms.CheckColumnType, "Export", 50, True)
        self.column_list.add_column(mforms.StringColumnType, "Column name", self.owner.get_width() - 50, False)
        self.column_list.end_columns()
        self.column_list.set_allow_sorting(True)
        self.column_list.set_size(200, -1)
        colbox.add(self.column_list, True, True)
        
        limit_box = mforms.newBox(True)
        limit_box.set_spacing(8)
        limit_box.add(mforms.newLabel("Limit: "), False, False)
        self.limit_entry = mforms.newTextEntry()
        self.limit_entry.set_size(50, -1)
        self.limit_entry.add_changed_callback(lambda entry=self.limit_entry: self.entry_changed(entry))
        limit_box.add(self.limit_entry, False, False)
        
        offset_box = mforms.newBox(True)
        offset_box.set_spacing(8)
        offset_box.add(mforms.newLabel("Offset: "), False, False)
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
    
    def sell_all(self, checkbox):
        for i in range(self.column_list.count()):
            self.column_list.node_at_row(i).set_bool(0, checkbox.get_active())
        
    
    def entry_changed(self, control):
        txt = str(control.get_string_value())
        if len(txt) and not txt.isdigit():
            mforms.Utilities.show_warning("Table Data Export", "Offset and Limit field can contain only numbers", "OK", "", "")
            control.set_value("".join([s for s in list(txt) if s.isdigit()])) 
 
    def set_columns(self, cols):
        self.columns = cols
        for col in self.columns:
            node = self.column_list.add_node()
            node.set_bool(0, True)
            node.set_string(1, col['name'])     
        self.column_list.relayout()
       
    
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
        lbl_box.set_padding(8)
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

class PowerExport(mforms.Form):
    def __init__(self, editor, owner):
        mforms.Form.__init__(self, owner)
        self.editor = editor
        
        self.table = {}
        self.formats = []
        self.formats.append(create_module("csv", editor, False))
        self.formats.append(create_module("json", editor, False))
        
        self.content = mforms.newBox(False)
        self.content.set_spacing(12)
        
        self.tab_view = mforms.newTabView(mforms.TabViewDocument)
        self.content.add(self.tab_view, True, True)
        
        self.simpleTab = SimpleTabExport(self.editor, self)
        self.tab_view.add_page(self.simpleTab, self.simpleTab.caption)
        
        self.advancedTab = AdvancedTabExport(self.editor, self)
        self.tab_view.add_page(self.advancedTab, self.advancedTab.caption)
        
        self.tab_view.set_size(-1, 200)
        self.tab_view.relayout()
        
        self.set_content(self.content)
        if sys.platform.lower() == "darwin":
            self.set_size(600, 200)
        else:
            self.set_size(600, -1)
        self.center()
    
        self.set_on_close(self.on_close)
        
        self.tab_opt_map = {}
        self.export_thread = None
        self.export_timeout = None
        
        self.suspend_layout()
        self.create_ui()
        self.resume_layout()
     
    def on_close(self):
        if self.export_thread:
            if mforms.Utilities.show_message("Table Data Export", "Export thread is in progress, if you continue, results can be undefined. Do you wish to stop export and close this window?", "Stop Export", "Cancel", "") == mforms.ResultOk:
                if not self.stop_export():
                    mforms.Utilities.show_error("Table Data Export", "Can't stop export thread", "Ok", "", "")
                    return False
                
                return True
            else:
                return False
        return True
    
    def create_ui(self):
        optbox = mforms.newBox(False)
        optbox.set_spacing(8)
         
        lbl_format_box = mforms.newBox(True)
        lbl_format_box.set_spacing(8)
        lbl_format_box.add(mforms.newLabel("Please select the output format:"), False, True)
        self.btn_show_advanced_options = mforms.newButton(mforms.ToolButton)
        self.btn_show_advanced_options.set_icon(mforms.App.get().get_resource_path("admin_option_file.png"))
        def show_hide_opt_panel(show):
            self.optpanel.show(show)
            self.optpanel.relayout()
            self.content.relayout()
        
        self.btn_show_advanced_options.add_clicked_callback(lambda: show_hide_opt_panel(False) if self.optpanel.is_shown() else show_hide_opt_panel(True) )

        lbl_format_box.add_end(self.btn_show_advanced_options, False, True)
        optbox.add(lbl_format_box, False, True)
         
        self.formatbox = mforms.newBox(True)
        self.formatbox.set_spacing(8)
        optbox.add(self.formatbox, False, False)
                 
        self.optpanel = mforms.newPanel(mforms.TitledBoxPanel)
        self.optpanel.set_title("Options:")
        
        if sys.platform.lower() == "win32":
            self.optpanel.set_size(-1, 110)
        elif sys.platform.lower() == "darwin":
            self.optpanel.set_size(-1, 150)
        
        self.optpanel.show(False)

         
        tmpbox = mforms.newBox(False)
        tmpbox.set_spacing(8)
        tmpbox.set_padding(8)
         
        optbox.add(self.optpanel, True, True)
        self.formatopttabview = mforms.newTabView(mforms.TabViewTabless)
        tmpbox.add(self.formatopttabview, True, True)
        self.optpanel.add(tmpbox)        
         
        bottom_box = mforms.newBox(False)
        bottom_box.set_spacing(8)
        bottom_box.set_padding(16)
        bottom_box.add(optbox, False, True)
        self.content.add(bottom_box, False, True)
         
        self.add_options()
        self.optpanel.relayout()
        self.optpanel.flush_events()
         
        fileselector_box = mforms.newBox(True)
        fileselector_box.set_spacing(8)
        self.destinationfile_entry = mforms.newTextEntry()
        fileselector_box.add(self.destinationfile_entry, True, True)
        self.fileselectbox_btn = mforms.newButton()
        self.fileselectbox_btn.set_text("Browse...")
        self.fileselectbox_btn.set_size(120, -1)
        self.destinationfile_entry.set_value(last_location)
        self.fileselectbox_btn.add_clicked_callback(self.select_destination)
        fileselector_box.add_end(self.fileselectbox_btn, False, True)
        bottom_box.add(fileselector_box, False, True)
        
        start_box = mforms.newBox(True)
        start_box.set_spacing(8)
   
        box = mforms.newBox(True)
        self.export_local = mforms.newCheckBox()
        self.export_local.set_text("Export to local machine")
        self.export_local.set_active(True)
        self.export_local.add_clicked_callback(self.toggle_export_destination)
        box.add(self.export_local, False, True)
        l = mforms.newImageBox()
        l.set_image(mforms.App.get().get_resource_path("mini_notice.png"))
        l.set_tooltip("If checked rows will be exported on the location that started Workbench.\nIf not checked, rows will be exported on the server.")
        box.add(l, False, True)
        start_box.add(box, False, True)
        self.start_btn = mforms.newButton()
        self.start_btn.set_text("Start Export")
        self.start_btn.set_size(120, -1)
        self.start_btn.add_clicked_callback(self.start_export)
        start_box.add_end(self.start_btn, False, True)
        bottom_box.add(start_box, False, True)
        
        self.progress_bar = mforms.newProgressBar()
        bottom_box.add(self.progress_bar, True, True)
        self.progress_bar.show(False)
    
    def set_advanced_tab_and_query(self, query):
        self.tab_view.set_active_tab(1)
        self.advancedTab.set_query(query)
    
    def set_source(self, selection):
        self.table = selection
        self.advancedTab.set_query("SELECT * FROM %s.%s" % (self.table['schema'], self.table['table']))
        self.simpleTab.set_columns(self.get_table_columns())
    
    def get_table_columns(self):
        cols = []
        try:
            rset = self.editor.executeManagementQuery("SHOW COLUMNS FROM %s.%s" % (self.table['schema'], self.table['table']), 1)
        except grt.DBError, e:
            log_error("SHOW COLUMNS FROM %s.%s : %s" % (self.table['schema'], self.table['table'], e))
            rset = None
            
        if rset:
            ok = rset.goToFirstRow()
            while ok:
                col = {'name': None, 'type': None, 'is_string': None, 'is_number': None, 'is_date_or_time': None, 'is_bin': None, 'value': None}
                col['name'] = rset.stringFieldValueByName("Field")
                col['type'] = rset.stringFieldValueByName("Type")
                col['is_number'] = any(x in col['type'] for x in ['int', 'integer'])
                col['is_float'] = any(x in col['type'] for x in ['decimal', 'float', 'double'])  
                col['is_string'] = any(x in col['type'] for x in ['char', 'text', 'set', 'enum'])
                col['is_bin'] = any(x in col['type'] for x in ['blob', 'binary'])              
                cols.append(col)
                ok = rset.nextRow()
        return cols
    
    def toggle_export_destination(self):
        local_path = self.export_local.get_active()
        self.fileselectbox_btn.set_enabled(True) if local_path else self.fileselectbox_btn.set_enabled(False)
        for key, val in self.tab_opt_map.iteritems():
            if not local_path and not val['format'].allow_remote():
                val['radio'].set_enabled(False)
            else:
                val['radio'].set_enabled(True)

    
        
    def select_destination(self):
        filechooser = FileChooser(mforms.SaveFile)
        module = self.get_active_module()
        if module is not None:
            filechooser.set_extensions(module.get_file_extension()[0], module.get_file_extension()[1])

            if filechooser.run_modal():
                global last_location
                last_location = filechooser.get_path()
                self.destinationfile_entry.set_value(filechooser.get_path())
        
    def setup_module(self, mod):
        mod.set_table(self.table['schema'], self.table['table'])
        mod.set_filepath(str(self.destinationfile_entry.get_string_value()))
        mod.set_local(self.export_local.get_active())
        if self.tab_view.get_active_tab() == 1:
            mod.set_columns([])
            uquery = self.advancedTab.get_query()
            if len(uquery):
                mod.set_user_query(uquery)
        else:
            mod.set_user_query(None)
            mod.set_limit(self.simpleTab.limit_entry.get_string_value())
            mod.set_offset(self.simpleTab.offset_entry.get_string_value())
            cols = []
            for r in range(self.simpleTab.column_list.count()):
                node = self.simpleTab.column_list.node_at_row(r)
                if node.get_bool(0):
                    for col in self.simpleTab.columns:
                        if col['name'] == node.get_string(1):
                            cols.append(col)
            if len(cols) == 0:
                mforms.Utilities.show_message("Table Data Export", "You need to specify at least one column", "Ok", "","")
                return False
            mod.set_columns(cols)
        return True
    
    def export_finished(self, success):
        self.export_timeout = None
        self.progress_bar.show(False)
        self.start_btn.set_enabled(True)
        
        if self.export_thread.stop.is_set(): #it means it was cancelled by user
            mforms.Utilities.show_message("Table Data Export", "Export cancelled by user", "Ok", "","")
        else:
            if success:
                mforms.Utilities.show_message("Table Data Export", "Export finished succesfully", "Ok", "","")
                self.export_thread = None # we need to set it to None because can_close check this property to show confirmation dialog
                self.close()
            else:
                mforms.Utilities.show_message("Table Data Export", "Export didn't finish succesfully, error was: %s" % self.export_thread.exception, "Ok", "","")
            
        
        self.export_thread = None

    def update_progress(self):
        if self.export_thread:
            if self.export_thread.module.get_current_row() != 0 and self.export_thread.module.get_max_row() != 0:
                self.progress_bar.set_value(round(float(self.export_thread.module.get_current_row()) / float(self.export_thread.module.get_max_row()), 2))
            return True
        else:
            return False
        

    def start_export(self):
        filepath = str(self.destinationfile_entry.get_string_value())
        if len(filepath) < 3:
            mforms.Utilities.show_error("Table Data Export", "You must specify a destination file.", "Ok", "","")
            return

        if self.export_thread is None:
            mod = self.get_active_module()
            if self.setup_module(mod):
                self.start_btn.set_enabled(False)
                self.progress_bar.show(True)
                self.progress_bar.set_value(0)
                self.export_thread = WorkerThread(mod)
                self.export_thread.finished_callback = self.export_finished
                self.export_thread.start()
                self.export_timeout = mforms.Utilities.add_timeout(1, self.update_progress)
            
    def stop_export(self):
        ret = True
        if self.export_thread:
            self.export_thread.stop.set()
            self.export_thread.join(3.0)
            if self.export_thread and self.export_thread.is_alive():
                log_error("Can't stop worker thread")
                ret = False

        if self.export_timeout:
            mforms.Utilities.cancel_timeout(self.export_timeout)
            self.export_timeout = None
            
        return ret
        
    def get_active_module(self):
        idx = self.formatopttabview.get_active_tab()
        for key, val in self.tab_opt_map.iteritems():
            if val['index'] == idx:
                return val['format']
        return None
    
    def add_options(self):
        first = True
        for format in self.formats:
            fradio = mforms.newRadioButton(1)
            fradio.set_text(format.title)
            fradio.add_clicked_callback(lambda name=format.name: self.formatopttabview.set_active_tab(self.tab_opt_map[name]['index']))
            if first:
                first = False
                fradio.set_active(True)
            self.formatbox.add(fradio, False, False)
            
            box = self.add_optsbox(format.title)
            if len(format.options) == 0:
                label_box = mforms.newBox(True)
                label_box.set_spacing(8)
                label_box.add(mforms.newLabel("This output format has no additional options"), False, False)
                box.add(label_box, False, False)
            else:
                for name, opts in format.options.iteritems():
                    label_box = mforms.newBox(True)
                    label_box.set_spacing(8)
                    label_box.add(mforms.newLabel(opts['description']), False, False)
                    if opts['type'] == 'text':
                        opt_val = mforms.newTextEntry()
                        opt_val.set_size(35, -1)
                        opt_val.set_value(opts['value'])
                        opt_val.add_changed_callback(lambda field = opt_val, output = opts: operator.setitem(output, 'value', str(field.get_string_value())))
                        label_box.add_end(opt_val, False, False)
                    if opts['type'] == 'select':
                        opt_val = mforms.newSelector()
                        opt_val.set_size(75, -1)
                        opt_val.add_items([v for v in opts['opts']])
                        opt_val.set_selected(opts['opts'].values().index(opts['value']))
                        opt_val.add_changed_callback(lambda selector = opt_val, output = opts: operator.setitem(output, 'value', output['opts'][str(selector.get_string_value())]))
                        label_box.add_end(opt_val, False, False)
                    box.add(label_box, False, False)
            
            self.tab_opt_map[format.name] = {'index': self.formatopttabview.add_page(box, format.name), 'format':format, 'radio': fradio}        
        
    def add_optsbox(self, name):
        obox = mforms.newBox(False)
        obox.set_spacing(8)
        return obox;
    
    def run(self):
        self.show()
        

class PowerImport(mforms.Form):
    def __init__(self, editor, owner, selection = {}):
        mforms.Form.__init__(self, owner)
        self.editor = editor
        
        self.table = {}
        self.formats = []
        self.encoding_list = {'cp1250 (windows-1250)':'cp1250', 
                              'latin2 (iso8859-2)':'iso8859_2', 
                              'latin1 (iso8859-1)':'latin_1', 
                              'utf-8':'utf-8', 
                              'utf-16':'utf-16'}
        

        self.formats.append(create_module("csv", editor, True))
        self.formats.append(create_module("json", editor, True))
        
        self.content = mforms.newBox(False)
        self.content.set_spacing(16)

        self.set_content(self.content)
        if sys.platform.lower() in ["darwin", "win32"]:
            self.set_size(600, 100)
        else:
            self.set_size(600, -1)
        self.center()
        
        self.destination_table = selection
        
        self.tab_opt_map = {}
        self.dest_cols = []
        
        self.import_thread = None
        self.import_timeout = None
        
        self.create_ui()
        
        self.ds_show_count = 0
        self.df_show_count = 0
        
        

        self.set_on_close(self.on_close)
     
    def on_close(self):
        if self.import_thread:
            if mforms.Utilities.show_message("Table Data Import", "Import thread is in progress, if you continue, results can be undefined. Do you wish to stop import and close this window?", "Stop Import", "Cancel", "") == mforms.ResultOk:
                if not self.stop_import():
                    mforms.Utilities.show_error("Table Data Import", "Can't stop import thread", "Ok", "", "")
                    return False
                
                return True
            else:
                return False
        return True
        
    def select_source_file(self):
        filechooser = FileChooser(mforms.OpenFile)
        extensions = []
        for module in self.formats:
            extensions.append(module.get_file_extension()[0])

        filechooser.set_extensions("|".join(extensions), self.formats[0].get_file_extension()[1])

        if filechooser.run_modal():
            file_path = filechooser.get_path()
            fileName, fileExt = os.path.splitext(os.path.basename(file_path))
            self.sourcefile_entry.set_value(file_path)
            
            if fileExt[1:] == "json":
                if os.path.getsize(file_path) > 104857600:
                    if mforms.Utilities.show_warning("Table Data Import", "This file appears to be a json filetype, and the size is over 100MB. We suggest to get the data in csv format, do you wish to continue?", "Continue", "Cancel", "") != mforms.ResultOk:
                        self.sourcefile_entry.set_value("")
                        return;

            self.set_active_module(fileExt[1:])
            self.new_table_box.show(True)
            self.opts_preview_box.show(True)
            if self.new_table_radio.get_active() and len(self.new_table_name.get_string_value()) == 0:
                self.new_table_name.set_value(fileName)
            
    def call_analyze(self, mod):
        mod.set_encoding(self.get_encoding_type())
        if not mod.analyze_file():
            mforms.Utilities.show_warning("Table Data Import", "Can't analyze file, please try to change encoding type, or input format.", "Ok", "", "")
            return False
        return True 
    
    def radio_option_changed(self, name):
        if name == "csv":
            self.encoding_box.show(True)
        else:
            self.encoding_box.show(False)
        if len(self.sourcefile_entry.get_string_value()):
            mod = self.get_active_module()
            mod.set_filepath(self.sourcefile_entry.get_string_value())
            
            self.create_preview_table(not self.call_analyze(mod))
    
    def load_dest_columns(self):
        try:
            rset = self.editor.executeManagementQuery("SHOW COLUMNS FROM %s.%s" % (self.destination_table['schema'], self.destination_table['table']), 1)
        except Exception, e:
            log_error("SHOW COLUMNS FROM %s.%s : %s" % (self.destination_table['schema'], self.destination_table['table'], e))
            rset = None
            
        if rset:
            self.dest_cols = []
            ok = rset.goToFirstRow()
            while ok:
                self.dest_cols.append(rset.stringFieldValueByName("Field"))
                ok = rset.nextRow()
    
    def encoding_changed(self):
        mod = self.get_active_module()
        self.create_preview_table(not self.call_analyze(mod))

    def get_encoding_type(self):
        return self.encoding_list[self.encoding.get_string_value()]

    def create_ui(self):
        contentbox = mforms.newBox(False)
        contentbox.set_spacing(16)
        contentbox.set_padding(16)
        self.content.add(contentbox, True, True)
        
        box = mforms.newBox(False)
        l = mforms.newLabel("Table Data Import allows you to easily import csv, json datafiles.\nYou can also create destination table on the fly.")
        l.set_style(mforms.BoldInfoCaptionStyle)
        box.add(l, False, False)
        contentbox.add(box, False, True)
        
        box = mforms.newBox(False)
        box.set_spacing(8)
        l = mforms.newLabel("Select data file you'd like to import.")
        box.add(l, False, False)
     
        fileselector_box = mforms.newBox(True)
        fileselector_box.set_spacing(8)
        self.sourcefile_entry = mforms.newTextEntry()
        fileselector_box.add(self.sourcefile_entry, True, True)
        self.fileselectbox_btn = mforms.newButton()
        self.fileselectbox_btn.set_text("Browse...")
        self.fileselectbox_btn.set_size(120, -1)
        self.fileselectbox_btn.add_clicked_callback(self.select_source_file)
        fileselector_box.add_end(self.fileselectbox_btn, False, True)
        box.add(fileselector_box, True, True)
        contentbox.add(box, False, True)
        
        box = mforms.newBox(False)
        box.set_spacing(8)
        if 'table' in self.destination_table and self.destination_table['table'] is not None:
            fradio = mforms.newRadioButton(2)
            fradio.set_text("Use existing table: %s.%s" % (self.destination_table['schema'], self.destination_table['table']))
            fradio.set_active(True)
            fradio.add_clicked_callback(self.destination_table_radio_click)
            box.add(fradio, False, True)
            self.load_dest_columns()
                
            
        self.new_table_box = mforms.newBox(True)
        self.new_table_box.set_spacing(8)
        self.new_table_radio = mforms.newRadioButton(2)
        self.new_table_radio.set_text("Create new table: ")
        self.new_table_radio.add_clicked_callback(self.destination_table_radio_click)
        if 'table' not in self.destination_table or self.destination_table['table'] is None:
            self.new_table_radio.set_active(True)
        self.new_table_box.add(self.new_table_radio, False, True)
        self.new_table_name = mforms.newTextEntry()
        
        self.new_table_box.add(self.new_table_name, True, True)
        box.add(self.new_table_box, False, True)
        contentbox.add(box, False, True)
        
        # We show encoding box only for csv as json can be only utf-8, utf-16 according to rfc
        self.encoding_box = mforms.newBox(True)
        self.encoding_box.set_spacing(16)
        self.encoding_box.add(mforms.newLabel("Encoding: "), False, True)
        
        
        self.encoding = mforms.newSelector()
        self.encoding.set_size(250, -1)
        self.encoding_box.add(self.encoding, False, True)
        
        for i,e in enumerate(self.encoding_list):
            self.encoding.add_item(e)
            if self.encoding_list[e] == 'utf-8':
                self.encoding.set_selected(i)

        self.encoding.add_changed_callback(self.encoding_changed)
        self.encoding_box.show(False)

        contentbox.add(self.encoding_box, False, True)
        
        self.new_table_box.show(False)
        
        self.opts_preview_box = mforms.newBox(False)
        self.opts_preview_box.set_spacing(16)
        contentbox.add(self.opts_preview_box, False, True)
        
        optbox = mforms.newBox(False)
        optbox.set_spacing(8)
         
        lbl_format_box = mforms.newBox(True)
        lbl_format_box.set_spacing(8)
        lbl_format_box.add(mforms.newLabel("Please select the input format:"), False, True)
        self.btn_show_advanced_options = mforms.newButton(mforms.ToolButton)
        self.btn_show_advanced_options.set_icon(mforms.App.get().get_resource_path("admin_option_file.png"))
        
        def show_hide_opt_panel(show):
            self.optpanel.show(show)
            self.optpanel.relayout()
            self.content.relayout()
        
        self.btn_show_advanced_options.add_clicked_callback(lambda: show_hide_opt_panel(False) if self.optpanel.is_shown() else show_hide_opt_panel(True) )
        lbl_format_box.add_end(self.btn_show_advanced_options, False, True)
        optbox.add(lbl_format_box, False, True)
         
        self.formatbox = mforms.newBox(True)
        self.formatbox.set_spacing(8)
        optbox.add(self.formatbox, False, False)
                 
        self.optpanel = mforms.newPanel(mforms.TitledBoxPanel)
        self.optpanel.set_title("Options:")
        
        if sys.platform.lower() == "win32":
            self.optpanel.set_size(-1, 110)
        elif sys.platform.lower() == "darwin":
            self.optpanel.set_size(-1, 150)
         
        tmpbox = mforms.newBox(False)
        tmpbox.set_spacing(8)
        tmpbox.set_padding(8)
         
        optbox.add(self.optpanel, True, True)
        self.formatopttabview = mforms.newTabView(mforms.TabViewTabless)
        self.formatopttabview.set_size(-1, 100)
        tmpbox.add(self.formatopttabview, True, True)
        self.optpanel.add(tmpbox)    
       
        self.opts_preview_box.add(optbox, True, True)
        
        self.add_options()
        self.optpanel.show(False)
        self.opts_preview_box.show(False)

        self.table_preview_box = mforms.newBox(False)
        self.table_preview_box.set_spacing(16)
        self.preview_table = None
        self.opts_preview_box.add(self.table_preview_box, True, True)
        
        self.column_caption = mforms.newPanel(mforms.BorderedPanel)
        self.column_caption.set_title("Columns:")
        self.column_caption.set_size(-1, 100)
        self.column_scroll = mforms.newScrollPanel(0)
        self.column_caption.add(self.column_scroll)
        self.table_preview_box.add(self.column_caption, True, True)
        
        
        start_box = mforms.newBox(True)
        start_box.set_spacing(8)

        self.start_btn = mforms.newButton()
        self.start_btn.set_text("Start Import")
        self.start_btn.set_size(120, -1)
        self.start_btn.add_clicked_callback(self.start_btn_click)
        start_box.add_end(self.start_btn, False, True)
        
        self.progress_box = mforms.newBox(False)
        self.progress_lbl = mforms.newLabel("Current Row: 0/0")
        self.progress_box.add(self.progress_lbl, False, True)
        self.progress_box.show(False)

        start_box.add(self.progress_box, False, True)
        
        
        
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
        
        self.df_box.add(self.make_label_with_tooltip("Date format: ", "Tooltip"), False, True)
        self.df_entry = mforms.newTextEntry()
        self.df_entry.set_value("%Y-%m-%d %H:%M:%S")
        self.df_entry.set_size(200, -1)
        self.df_box.add(self.df_entry, False, True)
        self.df_box.show(False)
        
        
        bottom_box = mforms.newBox(False)
        bottom_box.set_spacing(16)
        bottom_box.add(extra_opts, False, True)
        bottom_box.add(start_box, False, True)
        self.opts_preview_box.add(bottom_box, False, True)

    def make_label_with_tooltip(self, lbl, tooltip):
        box = mforms.newBox(True)
        box.add(mforms.newLabel(lbl), False, True)
        l = mforms.newImageBox()
        l.set_image(mforms.App.get().get_resource_path("mini_notice.png"))
        l.set_tooltip(tooltip)
        box.add(l, False, True)
        return box
        

    def start_btn_click(self):
        self.start_import()
        
    
    def setup_module(self, mod):
        if self.new_table_radio.get_active():
            mod.create_new_table(True)
            mod.set_table(None, self.new_table_name.get_string_value())
        else: 
            mod.set_table(self.destination_table['schema'], self.destination_table['table'])
            
        mod.set_decimal_separator(str(self.ds_entry.get_string_value()))
        mod.set_date_format(str(self.df_entry.get_string_value()))
        mod.set_mapping(self.column_mapping)
        for row in self.column_mapping:
            if row['active']:
                break
        else:
            mforms.Utilities.show_message("Table Data Import", "You need to specify at least one column", "Ok", "","")
            return False
        
        return True
    
    def import_finished(self, success):
        self.import_timeout = None
        self.progress_box.show(False)
        self.start_btn.set_enabled(True)
        
        if self.import_thread.stop.is_set(): #it means it was cancelled by user
            mforms.Utilities.show_message("Table Data Export", "Import cancelled by user", "Ok", "","")
        else:
            if success:
                mforms.Utilities.show_message("Table Data Import", "Import finished succesfully", "Ok", "","")
                self.import_thread = None # we need to set it to None because can_close check this property to show confirmation dialog
                self.close()
            else:
                if len(self.import_thread.exception.args) == 2 and self.import_thread.exception.args[1] == 1050:
                    if mforms.Utilities.show_error("Table Data Import", "Can't create destination table as there's already a table with that name, what would you like to do?", "Recreate Table", "Abandon","") == mforms.ResultOk:
                        mforms.Utilities.add_timeout(0.5, self.restart_with_drop)
                else:
                    mforms.Utilities.show_message("Table Data Import", "Import didn't finish succesfully, error was: %s" % self.import_thread.exception, "Ok", "","")
             
        self.import_thread = None
    
    def update_progress(self):
        if self.import_thread:
            if self.import_thread.module.get_max_row() == 0:
                self.progress_lbl.set_text("Current Row: %d" % self.import_thread.module.get_current_row())
            else:
                self.progress_lbl.set_text("Current Row: %d/%d" % (self.import_thread.module.get_current_row(), self.import_thread.module.get_max_row()))
            return True
        else:
            return False
    
    def stop_import(self):
        ret = True
        if self.import_thread:
            self.import_thread.stop.set()
            self.import_thread.join(3.0)
            if self.import_thread and self.import_thread.is_alive():
                log_error("Can't stop worker thread")
                ret = False

        if self.import_timeout:
            mforms.Utilities.cancel_timeout(self.import_timeout)
            self.import_timeout = None
            
        return ret
    
    def restart_with_drop(self):
        self.import_thread = None
        mod = self.get_active_module()
        mod.force_drop_table(True)
        self.start_import()
    
    def start_import(self):
        if self.import_thread is None:
            mod = self.get_active_module()
            if self.setup_module(mod):
                self.start_btn.set_enabled(False)
                self.progress_box.show(True)
                self.import_thread = WorkerThread(mod)
                self.import_thread.finished_callback = self.import_finished
                self.import_thread.start()
                self.import_timeout = mforms.Utilities.add_timeout(1, self.update_progress)
        
    def destination_table_radio_click(self):
        if self.new_table_radio.get_active():
            fileName, fileExt = os.path.splitext(os.path.basename(self.sourcefile_entry.get_string_value()))
            if len(self.new_table_name.get_string_value()) == 0:
                self.new_table_name.set_value(fileName)
                
        self.create_preview_table()
        
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
        
        type_items = {'is_string':'text', 'is_number':'int', 'is_float':'double', 'is_bin':'binary', 'is_date_or_time': 'datetime'}
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
            
            sel.add_items(type_items.values())
            for i, v in enumerate(type_items):
                if row['type'] in v:
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
            if clean_up:
                return
            
        def create_select_dest_col(row, cols):
            sel = mforms.newSelector()
            sel.set_size(120, -1)
            sel.add_items(cols)
            for i, c in enumerate(cols):
                if c == row['dest_col']:
                    sel.set_selected(i)
                    break
            sel.add_changed_callback(lambda output = row: operator.setitem(output, 'dest_col', sel.get_string_value()))
            return sel

        self.preview_table = mforms.newTable()
        self.preview_table.suspend_layout()
        self.column_scroll.add(self.preview_table)
        
        
        
        mod = self.get_active_module()
        self.preview_table.set_column_count(5)
        self.preview_table.set_row_count(len(mod._columns) + 1)
        self.preview_table.set_row_spacing(8)
        self.preview_table.set_column_spacing(8)
        
        if len(mod._columns) >= 3:
            self.column_caption.set_size(-1, 200)
        else:
            self.column_caption.set_size(-1, 100)
        
        self.checkbox_list = []

        def sell_all(cols, active):
            for checkbox in self.checkbox_list:
                checkbox.set_active(active)
            for row in self.column_mapping:
                row['active'] = active
        
        chk = mforms.newCheckBox()
        chk.set_active(True)
        chk.add_clicked_callback(lambda checkbox = chk, columns = mod._columns: sell_all(columns, checkbox.get_active()))
        
        self.preview_table.add(chk, 0, 1, 0, 1, mforms.HFillFlag)
        self.preview_table.add(mforms.newLabel("Source Column"), 1, 2, 0, 1, mforms.HFillFlag)
        if not self.new_table_radio.get_active():
            self.preview_table.add(mforms.newLabel("Dest Column"), 2, 3, 0, 1, mforms.HFillFlag)
        else:
            self.preview_table.add(mforms.newLabel("Field Type"), 3, 4, 0, 1, mforms.HFillFlag)
        self.column_mapping = []
        for i, col in enumerate(mod._columns):
            row = {'active': True, 'name': col['name'], 'type' : None, 'col_no': i, 'dest_col': self.dest_cols[i] if i < len(self.dest_cols) else None}
            for c in col:
                if c.startswith('is_') and col[c]:
                    row['type'] = type_items[c]
                    break
            chk_box = create_chkbox(row)
            self.checkbox_list.append(chk_box)
            self.preview_table.add(chk_box, 0, 1, i+1, i+2, mforms.HFillFlag)
            self.preview_table.add(mforms.newLabel(str(col['name'])), 1, 2, i+1, i+2, mforms.HFillFlag)
            if not self.new_table_radio.get_active():
                self.preview_table.add(create_select_dest_col(row, self.dest_cols), 2, 3, i+1, i+2, mforms.HFillFlag)
            else:
                self.preview_table.add(create_select_type(row), 3, 4, i+1, i+2, mforms.HFillFlag)
            self.column_mapping.append(row)
            
        self.treeview_preview = newTreeNodeView(mforms.TreeFlatList)
        for i, col in enumerate(mod._columns):
            self.treeview_preview.add_column(mforms.StringColumnType, str(col['name']), 75, True)
        self.treeview_preview.end_columns()
        
        
        if len(mod._columns):
            col_values = []
            val_len = 0
            for col in mod._columns:
                val_len = len(col['value']) if len(col['value']) > val_len else val_len

            col_len = len(mod._columns)
            for i in range(0, val_len):
                row = []
                for j in range(0, col_len):
                    
                    if len(mod._columns[j]['value']) > i:
                        row.append(mod._columns[j]['value'][i])
                    else:
                        row.append("")
                col_values.append(row)

            for row in col_values:
                node = self.treeview_preview.add_node()
                for i, col in enumerate(row):
                    node.set_string(i, col)

        self.treeview_preview.set_allow_sorting(True)
        self.treeview_preview.set_size(200, 100)
        self.table_preview_box.add(self.treeview_preview, False, True)
        self.preview_table.resume_layout()
        
    def add_optsbox(self, name):
        obox = mforms.newBox(False)
        obox.set_spacing(8)
        obox.show()
        return obox;
    
    def get_active_module(self):
        idx = self.formatopttabview.get_active_tab()
        for key, val in self.tab_opt_map.iteritems():
            if val['index'] == idx:
                return val['format']
        return None
    
    def set_active_module(self, name):
        if name in self.tab_opt_map:
            for opt in self.tab_opt_map:
                if self.formatopttabview.get_active_tab() == self.tab_opt_map[name]['index']:
                    self.tab_opt_map[opt]['radio'].set_active(True)
                else:
                    self.tab_opt_map[opt]['radio'].set_active(False)
                
            if self.formatopttabview.get_active_tab() != self.tab_opt_map[name]['index']:
                self.tab_opt_map[name]['radio'].set_active(True) 
                self.formatopttabview.set_active_tab(self.tab_opt_map[name]['index'])
                
            self.radio_clicked(name)
            

    def radio_clicked(self, name):
        self.formatopttabview.set_active_tab(self.tab_opt_map[name]['index'])
        if self.radio_option_changed:
            self.radio_option_changed(name)

    def call_preview_table(self):
        self.create_preview_table(not self.call_analyze(self.get_active_module()))
        return False
    
    def add_options(self):
        first = True
        def set_text_entry(field, output):
            operator.setitem(output, 'value', str(field.get_string_value()))
            mforms.Utilities.add_timeout(0.1, self.call_preview_table)

        def set_selector_entry(selector, output):
            operator.setitem(output, 'value', output['opts'][str(selector.get_string_value())])
            mforms.Utilities.add_timeout(0.1, self.call_preview_table)
        
        for format in self.formats:
            fradio = mforms.newRadioButton(1)
            fradio.set_text(format.title)
            fradio.add_clicked_callback(lambda name=format.name: self.radio_clicked(name))
            if first:
                first = False
                fradio.set_active(True)
            self.formatbox.add(fradio, False, False)
            
            box = self.add_optsbox(format.title)
            if len(format.options) == 0:
                label_box = mforms.newBox(True)
                label_box.set_spacing(8)
                label_box.add(mforms.newLabel("This output format has no additional options"), False, False)
                box.add(label_box, False, False)
            else:
                for name, opts in format.options.iteritems():
                    label_box = mforms.newBox(True)
                    label_box.set_spacing(8)
                    label_box.add(mforms.newLabel(opts['description']), False, False)
                    if opts['type'] == 'text':
                        opt_val = mforms.newTextEntry()
                        opt_val.set_size(35, -1)
                        opt_val.set_value(opts['value'])
                        opt_val.add_changed_callback(lambda field = opt_val, output = opts: set_text_entry(field, output))
                        label_box.add_end(opt_val, False, False)
                    if opts['type'] == 'select':
                        opt_val = mforms.newSelector()
                        opt_val.set_size(75, -1)
                        opt_val.add_items([v for v in opts['opts']])
                        opt_val.set_selected(opts['opts'].values().index(opts['value']))
                        opt_val.add_changed_callback(lambda selector = opt_val, output = opts: set_selector_entry(selector, output))
                        label_box.add_end(opt_val, False, False)
                    box.add(label_box, False, False)
            self.tab_opt_map[format.name] = {'index': self.formatopttabview.add_page(box, format.name), 'format':format, 'radio': fradio}
    
    def run(self):
        self.show()
    
        
        
