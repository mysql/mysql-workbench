# Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

import sys, os, platform, subprocess
os_icon_suffix = ""
if sys.platform=="darwin":
    os_icon_suffix = "_mac"


from mforms import newButton, newCheckBox, newTreeNodeView
from mforms import FileChooser
from datetime import datetime

try:
    import _subprocess
except ImportError:
    pass

from workbench.log import log_error, log_debug, log_info

def showImporter(editor, schema):
    importer = SpatialImporterWizard(editor)
    importer.set_schema(schema)
    importer.run()

def handleContextMenu(name, sender, args):
    return #we hide it for now
    menu = mforms.fromgrt(args['menu'])

    selection = args['selection']

    # Add extra menu items to the SQL editor live schema tree context menu
    schemas_selected = None


    for s in selection:
        if s.type == 'db.Schema':
            schemas_selected = s.name
            break
        else:
            return

    item = mforms.newMenuItem("Load Spatial Data")
    item.add_clicked_callback(lambda sender=sender : showImporter(sender, schemas_selected))
    
    menu.insert_item(0, item)
    menu.add_separator()
    
    
def cmd_executor(cmd):
    p1 = None
    if platform.system() != "Windows":
        try:
            p1 = subprocess.Popen("exec " + cmd, stdout = subprocess.PIPE, stderr=subprocess.PIPE, shell = True)
        except OSError, exc:
            log_error("Error executing command %s\n%s\n" % (cmd, exc));
            import traceback
            traceback.print_ext()
    else:
        try:
            info = subprocess.STARTUPINFO()
            info.dwFlags |= _subprocess.STARTF_USESHOWWINDOW
            info.wShowWindow = _subprocess.SW_HIDE
            # Command line can contain object names in case of export and filename in case of import
            # Object names must be in utf-8 but filename must be encoded in the filesystem encoding,
            # which probably isn't utf-8 in windows.
            
            cmd = cmd.encode("utf8") if isinstance(cmd,unicode) else cmd
            log_debug("Executing command: %s\n" % cmd)
            p1 = subprocess.Popen(cmd, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE,startupinfo=info,shell=True)
        except OSError, exc:
            log_error("Error executing command %s\n%s\n" % (cmd, exc))
            import traceback
            traceback.print_exc()
            p1 = None
    return p1

class SpatialImporter:
    def __init__(self):
        self.import_table = None
        self.selected_fields = None
        self.process_handle = None
        self.import_overwrite = False
        self.import_append = False
        self.my_host = None
        self.my_pwd = None
        self.my_user = None
        self.my_port = 3306
        self.my_schema = None
        self.filepath = None
        self.skipfailures = False
        self.features_per_transcation = 200
        
        self.my_geo_column = "shape"
        self.abort_requested = False
        
        self.is_running = False
        self.returncode = None


    def execute_cmd(self, cmd, progress_notify):
        p1 = cmd_executor(cmd)

        self.process_handle = p1

        pct = 0.0
        progress_notify(pct, "0 %")
        while p1 and p1.returncode is None:
            p1.poll()
            char = p1.stdout.read(1)
             
            if char == "0":
                continue
            else:
                if char == ".":
                    pct = pct + 0.03
                else:
                    try:
                        num = float(char)
                    except ValueError:
                        #it means that child process pass away, we can do nothing with it
                        break

                    if num == 1.0 and pct > 3.0:
                        progress_notify(pct, "Finished")
                        break
                    pct = num/10.0
            progress_notify(pct, "%d %%" % int(pct * 100))

        sout, serr = p1.communicate()
        self.returncode = p1.returncode
        if self.returncode !=0:
            if serr != "":
                raise Exception(serr)
            raise grt.UserInterrupt()


    def print_log_message(self, msg):
        print msg
 
    def kill(self):
        self.abort_requested = True
        if self.process_handle:
            if platform.system() == 'Windows':
                cmd = "taskkill /F /T /PID %i" % self.process_handle.pid
                log_debug("Killing task: %s\n" % cmd)
                subprocess.Popen(cmd , shell=True)
            else:
                import signal
                try:
                    log_debug("Sending SIGTERM to task %s\n" % self.process_handle.pid)
                    os.kill(self.process_handle.pid, signal.SIGTERM)
                except OSError, exc:
                    log_error("Exception sending SIGTERM to task: %s\n" % exc)
                    self.print_log_message("kill task: %s" % str(exc))
                     
    def run(self, progress_notify):
        cmd_args = {}
        cmd_args['host'] = self.my_host
        cmd_args['schema'] = self.my_schema
        cmd_args['user'] = self.my_user
        cmd_args['pwd'] = self.my_pwd
        cmd_args['port'] = self.my_port
        cmd_args['features_per_transcation'] = self.features_per_transcation
        if self.import_table:
            cmd_args['table_name'] = " -nln %s" % self.import_table
        else:
            cmd_args['table_name'] = ""

        cmd_args['opts'] = "-lco GEOMETRY_NAME=%s" % self.my_geo_column
        if self.skipfailures:
            cmd_args['opts'] = cmd_args['opts'] + " -skipfailures"
            
        if self.import_append:
            cmd_args['opts'] = cmd_args['opts'] + " -append"

        if self.import_overwrite:
            cmd_args['opts'] = cmd_args['opts'] + " -overwrite"
        
        if self.selected_fields:
            cmd_args['opts'] = cmd_args['opts'] + " -select " + self.selected_fields
        
        cmd_args['filepath'] = self.filepath

        cmd = """ogr2ogr -f "MySQL" MySQL:"%(schema)s,host=%(host)s,user=%(user)s,password=%(pwd)s,port=%(port)d" %(filepath)s %(table_name)s %(opts)s -progress -gt %(features_per_transcation)d -lco ENGINE=InnoDb -lco SPATIAL_INDEX=NO""" % cmd_args
        self.is_running = True
        try:
            self.execute_cmd(cmd, progress_notify)
        except grt.UserInterrupt:
            log_info("User cancelled")
            raise
        except Exception, exc:
            import traceback
            log_error("An error occured during execution of ogr2ogr file import: %s, stack: %s\n" % (exc, traceback.format_exc()))
            raise
        self.is_running = False


from workbench.ui import WizardForm, WizardPage, WizardProgressPage

class SelectFileWizardPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Select File to Import", wide=True)

        self.schema_label = mforms.newLabel("Target schema: ")

        self.back_button.set_enabled(False)


    def create_ui(self):
        self.set_spacing(16)

        self.content.add(self.schema_label, False, True)

        entry_box = mforms.newBox(True)
        entry_box.set_spacing(5)

        entry_box.add(mforms.newLabel("File Path:"), False, True)

        self.shapefile_path = mforms.newTextEntry()
        entry_box.add(self.shapefile_path, True, True)

        self.shapefile_browse_btn = newButton()
        self.shapefile_browse_btn.set_text("Browse...")
        self.shapefile_browse_btn.add_clicked_callback(self.shapefile_browse)
        entry_box.add(self.shapefile_browse_btn, False, False)

        self.content.add(entry_box, False, True)

        label = mforms.newLabel("""Select a shapefile containing spatial data to load into MySQL.
A new table with the imported fields will be created in the selected schema,
unless the append or update options are specified.""")
        self.content.add(label, False, False)

        self.dbf_box = mforms.newBox(True)
        self.dbf_box.set_spacing(8)
        self.dbf_icon = mforms.newImageBox()
        self.dbf_icon.set_image("task_unchecked%s.png" % os_icon_suffix)
        self.dbf_box.add(self.dbf_icon, False, True)
        dbf_label = mforms.newLabel("Check if dbf file is present")
        self.dbf_box.add(dbf_label, True, True)
        self.dbf_box.show(True)
         
        self.content.add(self.dbf_box, False, True)
         
        self.proj_box = mforms.newBox(True)
        self.proj_box.set_spacing(8)
        self.proj_icon = mforms.newImageBox()
        self.proj_icon.set_image("task_unchecked%s.png" % os_icon_suffix)
        self.proj_box.add(self.proj_icon, False, True)
        proj_label = mforms.newLabel("Check if prj file is present")
        self.proj_box.add(proj_label, True, True)
        self.proj_box.show(True)
         
        self.content.add(self.proj_box, False, True)
        
        self.warning_srs = mforms.newLabel("")
        self.content.add(self.warning_srs, False, True)

    def go_cancel(self):
        self.main.cancel()


    def validate(self):
        filepath = self.shapefile_path.get_string_value()
        if not os.path.isfile(filepath):
            mforms.Utilities.show_error("Invalid Path", "Please specify a valid file path.", "OK", "", "")
            return False
        return True


    def shapefile_browse(self):
        filechooser = FileChooser(mforms.OpenFile)
        filechooser.set_directory(os.path.dirname(self.shapefile_path.get_string_value()))
        filechooser.set_extensions("Spatial Shape File (*.shp)|*.shp", "shp");
        if filechooser.run_modal():
            filepath = filechooser.get_path()
            filename = os.path.splitext(os.path.basename(filepath))[0]
            self.shapefile_path.set_value(filepath)
            if os.path.isfile("".join([os.path.dirname(filepath),"/",filename,".dbf"])):
                self.dbf_icon.set_image("task_checked%s.png" % os_icon_suffix)
            else:
                self.dbf_icon.set_image("task_warning%s.png" % os_icon_suffix)
            if os.path.isfile("".join([os.path.dirname(filepath),"/",filename,".prj"])):
                self.proj_icon.set_image("task_checked%s.png" % os_icon_suffix)
            else:
                self.proj_icon.set_image("task_warning%s.png" % os_icon_suffix)
                self.warning_srs.set_text("Projection file not found, assuming WGS84 Spatial Reference System")
                
            
def small_label(text):
    l = mforms.newLabel(text)
    l.set_style(mforms.SmallHelpTextStyle)
    return l


class ContentPreviewPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Import Options", wide=True)
        self.layer_name = None
        self.column_list = []

    def get_path(self):
        return self.main.select_file_page.shapefile_path.get_string_value()
    
    def get_info(self):
        cmd = "ogrinfo -al -so %s" % self.get_path()
        p1 = cmd_executor(cmd)
        sout, serr = p1.communicate(input)
        import re
        p = re.compile("^(\w+):\s(\w+)\s\(([0-9\.]+)\)", re.IGNORECASE)
        for line in sout.splitlines():
            if line.startswith("Layer name: "):
                self.layer_name_lbl.set_text(line.split(':')[1].strip())
                self.layer_name = line.split(':')[1].strip()
                self.table_name.set_value(line.split(':')[1].strip())
            else:
               m = p.match(line)
               if m is not None:
                    row = self.column_list.add_node()
                    row.set_bool(0, True)
                    row.set_string(1, m.group(1))

    def go_cancel(self):
        self.main.cancel()
        
    def validate(self):
        sfields = self.get_fields()
        if len(sfields) == 0:
            mforms.Utilities.show_error("Missing columns", "Please specify at least one column to import.", "OK")
            return False
        
        table_name = self.table_name.get_string_value()
        if len(table_name) < 1:
            if mforms.ResultOk == mforms.Utilities.show_error("Invalid Table Name ", "Please specify a destination table name.", "Use Layer Name", "Cancel", ""):
                self.table_name.set_value(self.layer_name)
                return True
            return False
        return True

    def get_fields(self):
        fields = []
        for i in range(self.column_list.count()):
            node = self.column_list.node_at_row(i)
            if node.get_bool(0):
                fields.append(node.get_string(1))
        return fields

    def create_ui(self):
        self.set_spacing(16)
        self.content.set_padding(16)
        
        layer_box = mforms.newBox(True)
        layer_box.set_spacing(16)
        layer_heading = mforms.newLabel("Layer name:")
        layer_box.add(layer_heading, False, False)
        self.layer_name_lbl = mforms.newLabel("")
        layer_box.add(self.layer_name_lbl, False, False)
        self.content.add(layer_box, False, False)

        entry_box = mforms.newBox(True)
        entry_box.set_spacing(12)

        entry_box.add(mforms.newLabel("Destination table:"), False, True)

        self.table_name = mforms.newTextEntry()
        entry_box.add(self.table_name, True, True)
        
        self.content.add(entry_box, False, True)
        entry_box.show(True)

        cbox = mforms.newBox(False)

        self.column_list = newTreeNodeView(mforms.TreeFlatList)
        self.column_list.add_column(mforms.CheckColumnType, "", 40, True)
        self.column_list.add_column(mforms.StringColumnType, "Column name", 300, False)
        self.column_list.end_columns()
        self.column_list.set_size(-1, 100)

        cbox.add(self.column_list, False, True)
        cbox.add(small_label("Please select columns you'd like to import"), False, True)

        self.content.add(cbox, False, True)
        cbox.show(True)
        
        options_layer = mforms.newPanel(mforms.TitledBoxPanel)
        options_layer.set_title("Additional options")
        
        options_box = mforms.newBox(False)
        options_box.set_spacing(12)
        options_box.set_padding(12)
        
        boxfailures = mforms.newBox(False)
        
        self.skipfailures_chb = newCheckBox()
        self.skipfailures_chb.set_text("Skip failures");
        self.skipfailures_chb.set_active(False)
        
        boxfailures.add(self.skipfailures_chb, False, False)
        boxfailures.add(small_label("if error occurs, skip it and continue processing the data"), False, False)
        options_box.add(boxfailures, False, False)
        
        boxappend = mforms.newBox(False)
        
        self.append_chb = newCheckBox()
        self.append_chb.set_text("Append to existing data");
        self.append_chb.set_active(False)
        boxappend.add(self.append_chb, False, False)
        boxappend.add(small_label("append to existing table instead of creating new one"), False, False)
        options_box.add(boxappend, False, False)
        
        boxoverwrite = mforms.newBox(False)
        
        self.overwrite_chb = newCheckBox()
        self.overwrite_chb.set_text("Overwrite existing data");
        self.overwrite_chb.set_active(False)
        boxoverwrite.add(self.overwrite_chb, False, False)
        boxoverwrite.add(small_label("delete current table and recreate it empty"), False, False)
        options_box.add(boxoverwrite, False, False)
        
        options_layer.add(options_box)
        options_layer.show(True)
        
        self.content.add(options_layer, False, False)

        boxconvert = mforms.newBox(False)
        self.cartesian_convert_chb = newCheckBox()
        self.cartesian_convert_chb.set_text("Convert data to cartesian coordinate system");
        self.cartesian_convert_chb.set_active(True)

        boxconvert.add(self.cartesian_convert_chb, False, True)
        boxconvert.add(small_label("MySQL support only Cartesian format, leaving this checkbox in it's initial state will convert the data which may lead to data loss"), False, False)
        
        self.content.add(boxconvert, False, True)
        self.get_info()



class ImportProgressPage(WizardProgressPage):
    
    def __init__(self, owner):
        WizardProgressPage.__init__(self, owner, "Import Data")

        self.add_task(self.prepare_import, "Prepare import")
        self.add_threaded_task(self.start_import, "Import data file")
        self.importer_time = None
        self.importer = None

    def get_mysql_password(self, connection):
        parameterValues = connection.parameterValues
        pwd = parameterValues["password"]
        if not pwd:
            username = parameterValues["userName"]
            host = connection.hostIdentifier
            title = "MySQL password for File Import"
            accepted, pwd = mforms.Utilities.find_or_ask_for_password(title, host, username, False)
            if not accepted:
                return None
        return pwd


    def get_path(self):
        return self.main.select_file_page.shapefile_path.get_string_value()


    def prepare_import(self):
        if self.importer and self.importer.is_running:
            mforms.Utilities.show_message("Importing...", "Import thread is already running.", "Ok", "", "")
            raise RuntimeError("Import is already running")

        self.importer = SpatialImporter()
        self.importer.filepath = self.get_path()
        if self.importer.filepath == None or not os.path.isfile(self.importer.filepath):
            raise RuntimeError("Unable to open specified file: %s" % self.importer.filepath)

        self.importer.my_pwd = self.get_mysql_password(self.main.editor.connection)
        if self.importer.my_pwd == None:
            log_error("Cancelled MySQL password input\n")
            raise RuntimeError("Cancelled MySQL password input")

        self.importer.my_host = self.main.editor.connection.parameterValues.hostName
        self.importer.my_port = self.main.editor.connection.parameterValues.port

        self.importer.my_user = self.main.editor.connection.parameterValues.userName
        self.importer.my_schema = self.main.selected_schema
        
        self.importer.skipfailures = self.main.content_preview_page.skipfailures_chb.get_active()
        self.importer.import_overwrite = self.main.content_preview_page.skipfailures_chb.get_active()
        self.importer.import_append = self.main.content_preview_page.append_chb.get_active()
        
        if self.main.content_preview_page.table_name.get_string_value() != "":
            self.importer.import_table = self.main.content_preview_page.table_name.get_string_value()

        self.importer.selected_fields = ",".join(self.main.content_preview_page.get_fields())
        return True

    def progress_notify(self, pct, msg):
        self.send_progress(pct, msg)

    def start_import(self):
        self.importer_time = None
        start = datetime.now()
        self.importer.run(self.progress_notify)
        self.importer_time = datetime.now() - start
        if self.importer.returncode == 0:
            return True
        return False
    
    def go_cancel(self):
        if self.importer and self.importer.is_running:
            if mforms.ResultOk == mforms.Utilities.show_message("Confirmation", "Do you wish to stop import process?", "Yes", "No", "Cancel"):
                if self.importer:
                    self.importer.kill()


class ResultsPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Import Results")

    def get_path(self):
        return self.main.select_file_page.shapefile_path.get_string_value()

    def create_ui(self):
        if self.main.import_page.importer_time:
            itime = float("%d.%d" % (self.main.import_page.importer_time.seconds, self.main.import_page.importer_time.microseconds))
            self.content.add(mforms.newLabel(str("File %s was imported in %.3f s" % (self.get_path(), itime))), False, True)
        
        
        self.content.add(mforms.newLabel(str("Table %s was created" % self.main.content_preview_page.table_name.get_string_value())), False, True)
        


class SpatialImporterWizard(WizardForm):
    def __init__(self, editor):
        WizardForm.__init__(self, mforms.Form.main_form())

        self.editor = editor
        
        self.selected_schema = None

        self.set_title("Load Spatial Data")

        self.center()

        self.select_file_page = SelectFileWizardPage(self)
        self.add_page(self.select_file_page)
        
        self.content_preview_page = ContentPreviewPage(self)
        self.add_page(self.content_preview_page)

        self.import_page = ImportProgressPage(self)
        self.add_page(self.import_page)

        self.results_page = ResultsPage(self)
        self.add_page(self.results_page)


    def set_schema(self, schema_name):
        self.select_file_page.schema_label.set_text("Tables will be imported to schema: %s" % schema_name)
        self.selected_schema = schema_name
        

