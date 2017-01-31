# Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

import sys, os, csv

import datetime
import json
from workbench.utils import Version

from workbench.log import log_debug3, log_debug2, log_error, log_warning

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

    if user_selection:
        if user_selection['table']:
            item = mforms.newMenuItem("Table Data Export")
            item.add_clicked_callback(lambda sender=sender : showPowerExport(sender, user_selection))
            menu.insert_item(3, item)
    
        item = mforms.newMenuItem("Table Data Import")
        item.add_clicked_callback(lambda sender=sender : showPowerImport(sender, user_selection))
        menu.insert_item(4, item)
        
        menu.insert_item(5, mforms.newMenuItem("", mforms.SeparatorMenuItem))
    
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
        self._targetVersion = Version.fromgrt(editor.serverVersion)
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
        self._truncate_table = False
        self._type_map = {'text':'is_string', 'bigint': 'is_bignumber', 'geometry': 'is_geometry', 'int': 'is_number', 'double':'is_float', 'json': 'is_json'}
        self.is_running = False
        self.progress_info = None
        self.item_count = 0

    def guess_type(self, vals):
        def is_json(v):
            return True if type(v) in [dict, list] else False
        
        def is_float(v):
            if hasattr(v, 'encode'):
                v = v.encode('utf8')

            v = str(v)
            try:
                if "%s" % float(v) == v:
                    return True
                return False
            except:
                return False
        
        def is_int(v):
            if hasattr(v, 'encode'):
                v = v.encode('utf8')

            v = str(v)
            try:
                if "%s" % int(v) == v:
                    return True
                return False
            except:
                return False
        
        cur_type = None
        for v in vals:
            if is_json(v):
                return "json" # If JSON then we can return immediately. 
            if is_int(v):
                if not cur_type:
                    #We know that it's int, but let's see if it's maybe big int
                    val = int(v)
                    if val > 2147483647 or val < -2147483647:
                        cur_type = "bigint"
                    else:
                        cur_type = "int"
                continue
    
            if is_float(v):
                if cur_type in [None, "int"]:
                    cur_type = "double"
                continue
            cur_type = "text"
            break
        return cur_type

        
    def update_progress(self, pct, msg):
        if self.progress_info:
            self.progress_info(pct, msg)

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
        
    def truncate_table(self, trunc):
        self._truncate_table = trunc

    def set_table(self, schema, table):
        if schema:
            self._table_w_prefix = "`%s`.`%s`" % (schema, table)
        else:
            self._table_w_prefix = "`%s`" % str(table)
        
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
                   'is_string': any(x in c.columnType for x in ['char', 'text', 'set', 'enum']),
                   'is_json': any(x in c.columnType for x in ['json']), 
                   'is_number': any(x in c.columnType for x in ['int', 'integer']),
                   'is_bignumber':  any(x in c.columnType for x in ['bigint']),
                   'is_geometry':  any(x in c.columnType for x in ['geometry','geometrycollection', 'linestring', 'multilinestring', 'multipoint', 'multipolygon', 'point' , 'polygon']),
                   'is_date_or_time': any(x in c.columnType for x in ['timestamp', 'time', 'datetime', 'date']), 
                   'is_bin': any(x in c.columnType for x in ['geo', 'blob', 'binary']),
                   'is_float': any(x in c.columnType for x in ['decimal', 'float', 'double', 'real']),
                   'value': None})
    
    def prepare_new_table(self):
        try:
            
            self._editor.executeManagementCommand(""" CREATE TABLE %s (%s)""" % (self._table_w_prefix, ", ".join(["`%s` %s" % (col['name'], col["type"]) for col in self._mapping])), 1)
            self.update_progress(0.0, "Prepared new table")
            # wee need to setup dest_col for each row, as the mapping is empty if we're creating new table
            for col in self._mapping:
                col['dest_col'] = col['name']
            return True
        except Exception, e:
            log_error("Error creating table for import: %s" % e)
            if len(e.args) == 2 and e.args[1] == 1050 and self._force_drop_table:
                try:
                    self.update_progress(0.0, "Drop existing table")
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
        try:
            self.is_running = True
            if self._is_import:
                ret = self.start_import()
            else:
                ret = self.start_export()
            self.is_running = False
            return ret
        except:
            self.is_running = False
            raise

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
        self.options = {'filedseparator': {'description':'Field Separator', 'type':'select', 'opts':{'TAB':'\t',';':';', ':':':', ',':','}, 'value':';', 'entry': None},
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
            return """SELECT %s FROM %s %s""" % (",".join(["`%s`" % value['name'] for value in self._columns]), self._table_w_prefix, limit)
        else:
            limit = ""
            if self._limit:
                limit = "LIMIT %d" % int(self._limit)
                if self._offset:
                    limit = "LIMIT %d,%d" % (int(self._offset), int(self._limit))
            fpath = self._filepath
            if sys.platform.lower() == "win32":
                fpath = fpath.replace("\\","\\\\")

            return """SELECT %s FROM %s INTO OUTFILE '%s' 
                        FIELDS TERMINATED BY '%s' 
                        ENCLOSED BY '%s' 
                        LINES TERMINATED BY %s %s""" % (",".join(["`%s`" % value['name'] for value in self._columns]), self._table_w_prefix, fpath,
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
                self.update_progress(0.0, "Begin Export")
                with open(self._filepath, 'wb') as csvfile:
                    output = csv.writer(csvfile, delimiter = self.options['filedseparator']['value'], 
                                        lineterminator = self.options['lineseparator']['value'], 
                                        quotechar = self.options['encolsestring']['value'], quoting = csv.QUOTE_NONNUMERIC if self.options['encolsestring']['value'] else csv.QUOTE_NONE)
                    output.writerow([value['name'].encode('utf-8') for value in self._columns])
                    ok = rset.goToFirstRow()
                    
                    # Because there's no realiable way to use offset only, we'll do this here.
                    offset = 0
                    if self._offset and not self._limit:
                        offset = self._offset
                    i = 0
                    while ok:
                        if self._thread_event and self._thread_event.is_set():
                            log_debug2("Worker thread was stopped by user")
                            self.update_progress(round(self._current_row / self._max_rows, 2), "Data export stopped by user request")
                            return False

                        i += 1
                        if offset > 0 and i <= offset:
                            ok = rset.nextRow()
                            continue
                        self.item_count = self.item_count + 1 
                        self._current_row = float(rset.currentRow + 1)
                        self.update_progress(round(self._current_row / self._max_rows, 2), "Data export")
                        row = []
                        for col in self._columns:
                            if col['is_number'] or col['is_bignumber']:
                                row.append(rset.intFieldValueByName(col['name']))
                            elif col['is_float']:
                                row.append(rset.floatFieldValueByName(col['name']))
                            elif col['is_geometry']:
                                row.append(rset.geoStringFieldValueByName(col['name']))
                            else:
                                row.append(rset.stringFieldValueByName(col['name']))
                        output.writerow(row)
                        csvfile.flush()
                        ok = rset.nextRow()
                self.update_progress(1.0, "Export finished")
        else:
            self._editor.executeManagementCommand(query, 1)

        return True
    
    def start_import(self):
        if not self._last_analyze:
            return False
        
        if self._new_table:
            if not self.prepare_new_table():
                return False
            
        if self._truncate_table:
            self.update_progress(0.0, "Truncate table")
            self._editor.executeManagementCommand("TRUNCATE TABLE %s" % self._table_w_prefix, 1)
            
        result = True
        
        with open(self._filepath, 'rb') as csvfile:
            self.update_progress(0.0, "Prepare Import")
            dest_col_order = list(set([i['dest_col'] for i in self._mapping if i['active']]))
            query = """PREPARE stmt FROM 'INSERT INTO %s (%s) VALUES(%s)'""" % (self._table_w_prefix, ",".join(["`%s`" % col for col in dest_col_order]), ",".join(["?" for i in dest_col_order]))
            col_order = dict([(i['dest_col'], i['col_no']) for i in self._mapping if i['active']])
            col_type = dict([(i['dest_col'], i['type']) for i in self._mapping if i['active']])

            is_server_5_7 = self._targetVersion.is_supported_mysql_version_at_least(Version.fromstr("5.7.5"))
            
            self._editor.executeManagementCommand(query, 1)
            try:
                is_header = self.has_header
                reader = UniReader(csvfile, self.dialect, encoding=self._encoding)
                self._max_rows = os.path.getsize(self._filepath)
                self.update_progress(0.0, "Begin Import")
                for row in reader:
                    if self._thread_event and self._thread_event.is_set():
                        self._editor.executeManagementCommand("DEALLOCATE PREPARE stmt", 1)
                        log_debug2("Worker thread was stopped by user")
                        self.update_progress(round(self._current_row / self._max_rows, 2), "Import stopped by user request")
                        return False

                    self._current_row = float(csvfile.tell())
                    
                    if is_header:
                        is_header = False
                        continue

                    
                    

                    for i, col in enumerate(col_order):
                        if col_order[col] >= len(row):
                            log_error("Can't find col: %s in row: %s" % (col_order[col], row))
                            result = False
                            break
                        val = row[col_order[col]]
                        col_name = col_order[col]

                        if col_type[col] == "geometry":
                            if is_server_5_7:
                                val = """ST_GeomFromText("%s")""" % row[col_name]
                            else:
                                val = """GeomFromText("%s")""" % row[col_name]
                                
                            self._editor.executeManagementCommand("""SET @a%d = %s """ % (i, val), 0)
                        else:
                            if col_type[col] == 'double':
                                val = row[col_name].replace(self._decimal_separator, '.')
                            elif col_type[col] == 'datetime':
                                val = datetime.datetime.strptime(row[col_name], self._date_format).strftime("%Y-%m-%d %H:%M:%S")
                            if hasattr(val, "replace"):
                                val = val.replace("\\", "\\\\").replace("'", "\\'")
                            self._editor.executeManagementCommand("""SET @a%d = '%s' """ % (i, val), 0)
                    else:
                        try:
                            self._editor.executeManagementCommand("EXECUTE stmt USING %s" % ", ".join(['@a%d' % i for i, col in enumerate(col_order)]), 0)
                            self.item_count = self.item_count + 1
                            self.update_progress(round(self._current_row / self._max_rows, 2), "Data import")
                        except Exception, e:
                            log_error("Row import failed with error: %s" % e)
                            self.update_progress(round(self._current_row / self._max_rows, 2), "Row import failed with error: %s" % e)
                            result = False

                self.update_progress(1.0, "Import finished")
            except Exception, e:
                import traceback
                log_debug3("Import failed traceback: %s" % traceback.format_exc())
                log_error("Import failed: %s" % e)
            self._editor.executeManagementCommand("DEALLOCATE PREPARE stmt", 1)

        return result

    def analyze_file(self):
        with open(self._filepath, 'rb') as csvfile:
            if self.dialect is None:
                csvsample = []
                for i in range(0,2): #read two lines as a sample
                    line = csvfile.readline()
                    if len(line) > 0:
                        csvsample.append(line)
                
                csvsample_len = len(csvsample)
                csvsample = "".join(csvsample)
                self.dialect = csv.Sniffer().sniff(csvsample)
                self.has_header = csv.Sniffer().has_header(csvsample)
                if self.has_header and csvsample_len == 1:
                    self.has_header = False
                    
                csvfile.seek(0)
                self.options['filedseparator']['value'] = self.dialect.delimiter 
                self.options['lineseparator']['value'] = self.dialect.lineterminator 
                self.options['encolsestring']['value'] = self.dialect.quotechar 
            else:
                self.dialect.delimiter = self.options['filedseparator']['value']
                self.dialect.lineterminator = self.options['lineseparator']['value']
                self.dialect.quotechar = self.options['encolsestring']['value']
                csvfile.seek(0)
                
            try:
                reader = UniReader(csvfile, self.dialect, encoding=self._encoding)
                self._columns = []
                row_line = None
                try:
                    row_line = reader.next()
                except StopIteration, e:
                    pass
                
                
                if row_line:
                    for col_value in row_line:
                        self._columns.append({'name': col_value, 'type': 'text', 'is_string': True, 'is_geometry': False, 'is_bignumber': False, 'is_number': False, 'is_date_or_time': False, 'is_bin': False, 'is_float':False, 'is_json':False,'value': []})
                        
                    for i, row in enumerate(reader): #we will read only first few rows
                        if i < 5:
                            for j, col_value in enumerate(row):
                                try:
                                    json_value = json.loads(col_value)
                                    self._columns[j]['is_string'] = False
                                    self._columns[j]['is_json'] = True
                                    self._columns[j]['value'].append(json_value)
                                except Exception as e:
                                    self._columns[j]['value'].append(col_value)
                        else:
                            break
                        
                    if not self.has_header and i == 1: # This means there were one line which was consumed as a header we need to copy it to use as values
                        log_warning("File: %s, probably has got only one line, using it as a header and data\n" % self._filepath)
                        for j, col_value in enumerate(row_line):
                            try:
                                json_value = json.loads(col_value)
                                self._columns[j]['is_string'] = False
                                self._columns[j]['is_json'] = True
                                self._columns[j]['value'].append(json_value)
                            except Exception as e:
                                self._columns[j]['value'].append(col_value)

                    for col in self._columns:
                        # Means the file is missing some data or is mallformed
                        if len(col['value']) == 0:
                            return False

                        gtype = self.guess_type(col['value'])
                        if gtype not in self._type_map:
                            raise Exception("Unhandled type: %s in %s" % (gtype, self._type_map))
                        else:
                            col['type'] = gtype
                            for attrib in col:
                                if attrib.startswith("is_"):
                                    if attrib == self._type_map[gtype]:
                                        col[attrib] = True
                                    else:
                                        col[attrib] = False
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
        return """SELECT %s FROM %s %s""" % (",".join(["`%s`" % value['name'] for value in self._columns]), self._table_w_prefix, limit)                
    
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
                
                # Because there's no realiable way to use offset only, we'll do this here.
                offset = 0
                if self._offset and not self._limit:
                    offset = self._offset
                i = 0
                while ok:
                    if self._thread_event and self._thread_event.is_set():
                        log_debug2("Worker thread was stopped by user")
                        return False

                    i += 1
                    if offset > 0 and i <= offset:
                        ok = rset.nextRow()
                        continue

                    self.item_count = self.item_count + 1
                    self._current_row = rset.currentRow + 1
                    row = []
                    for col in self._columns:
                        if col['is_number'] or col['is_bignumber']:
                            row.append("\"%s\":%s" % (col['name'], json.dumps(rset.intFieldValueByName(col['name']))))
                        elif col['is_float']:
                            row.append("\"%s\":%s" % (col['name'], json.dumps(rset.floatFieldValueByName(col['name']))))
                        elif col['is_geometry']:
                            row.append("\"%s\":%s" % (col['name'], rset.geoJsonFieldValueByName(col['name'])))
                        else:
                            if col['type'] == "json":
                                row.append("\"%s\":%s" % (col['name'], rset.stringFieldValueByName(col['name'])))
                            else:
                                row.append("\"%s\":%s" % (col['name'], json.dumps(rset.stringFieldValueByName(col['name']))))
                    ok = rset.nextRow()
                    jsonfile.write("{%s}%s" % (', '.join(row), ",\n " if ok else ""))
                    jsonfile.flush()
                jsonfile.write(']')

        return True

    def start_import(self):
        if not self._last_analyze:
            return False

        if self._new_table:
            if not self.prepare_new_table():
                return False
        
        if self._truncate_table:
            self.update_progress(0.0, "Truncate table")
            self._editor.executeManagementCommand("TRUNCATE TABLE %s" % self._table_w_prefix, 1)
        
        result = True
        with open(self._filepath, 'rb') as jsonfile:
            data = json.load(jsonfile)
            dest_col_order = list(set([i['dest_col'] for i in self._mapping if i['active']]))
            query = """PREPARE stmt FROM 'INSERT INTO %s (%s) VALUES(%s)'""" % (self._table_w_prefix, ",".join(["`%s`" % col for col in dest_col_order]), ",".join(["?" for i in dest_col_order]))
            col_order = dict([(i['dest_col'], i['name']) for i in self._mapping if i['active']])
            col_type = dict([(i['name'], i['type']) for i in self._mapping if i['active']])
            
            self._editor.executeManagementCommand(query, 1)
            try:
                self._max_rows = len(data)
                for row in data:
                    if self._thread_event and self._thread_event.is_set():
                        log_debug2("Worker thread was stopped by user")
                        self._editor.executeManagementCommand("DEALLOCATE PREPARE stmt", 1)
                        return False
                    self._current_row = self._current_row + 1
                    for i, col in enumerate(col_order):
                        if col_order[col] not in row:
                            log_error("Can't find col: %s in row: %s" % (col_order[col], row))
                            result = False
                            break

                        val = row[col_order[col]]
                        col_name = col_order[col]

                        if col_type[col] == "geometry":
                            val = """ ST_GeomFromGeoJSON('%s')""" % json.dumps(val).replace("\\", "\\\\").replace("'", "\\'")
                                 
                            self._editor.executeManagementCommand("""SET @a%d = %s """ % (i, val), 0)
                        else:
                            if col_type[col_name] != "json" and hasattr(val, "replace"):
                                val = val.replace("\\", "\\\\").replace("'", "\\'")
                                
                            if col_type[col_name] == 'double':
                                val = val(str).replace(self._decimal_separator, '.')
                            elif col_type[col_name] == 'datetime':
                                val = datetime.datetime.strptime(val, self._date_format).strftime("%Y-%m-%d %H:%M:%S")
                            elif col_type[col_name] == "json":
                                val = json.dumps(val).replace("\\", "\\\\").replace("'", "\\'")                                
    
                            if col_type[col_name] == "int":
                                self._editor.executeManagementCommand("""SET @a%d = %d """ % (i, int(val)), 0)
                            else:
                                self._editor.executeManagementCommand("""SET @a%d = '%s' """ % (i, val), 0)

                    else:
                        try:
                            self._editor.executeManagementCommand("EXECUTE stmt USING %s" % ", ".join(['@a%d' % i for i, col in enumerate(col_order)]), 0)
                            self.item_count = self.item_count + 1
                        except Exception, e:
                            log_error("Row import failed with error: %s" % e)
                        
            except Exception, e:
                import traceback
                log_debug3("Import failed traceback: %s" % traceback.format_exc())
                log_error("Import failed: %s" % e)
            self._editor.executeManagementCommand("DEALLOCATE PREPARE stmt", 1)
            
        return result

    def analyze_file(self):
        data = []
        with open(self._filepath, 'rb') as f:
            prevchar = None
            stropen = False
            inside = 0
            datachunk = []
            rowcount = 0 
            while True:
                if f.tell() >= 20480:
                    log_error("JSON file contains data that's in unknown structure: %s" % (self._filepath))
                    return False
                c = f.read(1)
                if c == "":
                    break
            
                if c == '"' and prevchar != '\\':
                    stropen = True if stropen == False else False
            
                if stropen == False:
                    if c == '{' and prevchar != '\\':
                        inside = inside + 1
                    if c == '}' and prevchar != '\\':
                        inside = inside - 1
                        if inside == 0:
                            if rowcount >= 4:
                                datachunk.append(c)
                                datachunk.append(']')
                                break
                            else:
                                rowcount = rowcount + 1 
                datachunk.append(c)
                prevchar = c
            try:
                data = json.loads("".join(datachunk))
            except Exception, e:
                log_error("Unable to parse JSON file: %s,%s " % (self._filepath, e))
                self._last_analyze = False
                return False
        if len(data) == 0:
            log_error("JSON file contains no data, or data is invalid: %s" % (self._filepath))
            self._last_analyze = False
            return False
        
        self._columns = []
        for elem in data[0]:
            self._columns.append({'name': elem, 'type': 'text', 'is_string': True, 'is_geometry': False, 'is_bignumber': False, 'is_number': False, 'is_date_or_time': False, 'is_bin': False, 'is_float':False, 'is_json':False, 'value': []})

        for row in data:
            for i, elem in enumerate(row):
                if type(row[elem]) in [dict, list]:
                    self._columns[i]['is_string'] = False
                    self._columns[i]['is_json'] = True
                self._columns[i]['value'].append(row[elem])
                
        for col in self._columns:
            gtype = self.guess_type(col['value'])
            if gtype not in self._type_map:
                raise Exception("Unhandled type: %s in %s" % (gtype, self._type_map))
            else:
                col['type'] = gtype
                for attrib in col:
                    if attrib.startswith("is_"):
                        if attrib == self._type_map[gtype]:
                            col[attrib] = True
                        else:
                            col[attrib] = False
        self._last_analyze = True
        return True
        

def create_module(type, editor, is_import):
    if type == "csv":
        return csv_module(editor, is_import);
    if type == "json":
        return json_module(editor, is_import);
