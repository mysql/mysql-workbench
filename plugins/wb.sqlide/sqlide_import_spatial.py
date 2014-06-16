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

import os, platform, subprocess


from mforms import newButton
from mforms import FileChooser

try:
    import _subprocess
except ImportError:
    pass

from workbench.log import log_error, log_debug

def showImporter(editor, schema):
    importer = SpatialImporterWizard(editor)
    importer.set_schema(schema)
    importer.run()

def handleContextMenu(name, sender, args):
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
    
    
class SpatialImporter:
    def __init__(self):
        self.import_table = None
        self.process_handle = None
        self.import_overwrite = False
        self.my_host = None
        self.my_pwd = None
        self.my_user = None
        self.my_port = 3306
        self.my_schema = None
        self.filepath = None
        self.skipfailures = False
        
        self.my_geo_column = "shape"
        self.abort_requested = False
        
        self.is_running = False


    def execute_cmd(self, cmd):
        p1 = None
        if platform.system() != "Windows":
            try:
                p1 = subprocess.Popen(cmd, stdout = subprocess.PIPE, stderr=subprocess.PIPE, shell = True)
            except OSError, exc:
                log_error("Error executing command %s\n%s\n" % (cmd, exc));
                import traceback
                traceback.print_ext()
                self.print_log_message("Error executing task: %s" % exc)
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
                self.print_log_message("Error executing task: %s" % str(exc))
                p1 = None


        self.process_handle = p1

#         while p1 and p1.poll() == None and not self.abort_requested:

#             err = read_nonblocking(p1.stderr, timeout = 1)
#             print "Error is: %s\n" % err
#             out = read_nonblocking(p1.stdout, timeout = 1)
#             print "Output is: %s\n" % out
#             print "err"
#             if err != "":
#                 print "The error is: %s\n" % err
#             print p1.stdout.read()

                
                
#             log_error("Error from task: %s\n" % err)
#             self.print_log_message(err)
#             if 'Access denied for user' in err:
#                 self.e = wb_common.InvalidPasswordError('Wrong username/password!')
    
        
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
                     
    def run(self):
        cmd_args = {}
        cmd_args['host'] = self.my_host
        cmd_args['schema'] = self.my_schema
        cmd_args['user'] = self.my_user
        cmd_args['pwd'] = self.my_pwd
        cmd_args['port'] = self.my_port
        if self.import_table:
            cmd_args['table_name'] = " -nln %s" % self.import_table
        else:
            cmd_args['table_name'] = ""
  
        if self.import_overwrite:
            cmd_args['overwrite'] = " -overwrite"
        else:
            cmd_args['overwrite'] = ""
        
            
  
        cmd_args['opts'] = "-lco GEOMETRY_NAME=%s" % self.my_geo_column
        if self.skipfailures:
            cmd_args['opts'] = cmd_args['opts'] + " -skipfailures"

        
        cmd_args['filepath'] = self.filepath

        cmd = """ogr2ogr -f "MySQL" MySQL:"%(schema)s,host=%(host)s,user=%(user)s,password=%(pwd)s,port=%(port)d" %(filepath)s %(table_name)s %(overwrite)s %(opts)s -progress -lco ENGINE=InnoDb -lco SPATIAL_INDEX=NO %(opts)s""" % cmd_args
        self.is_running = True
        print cmd
        try:
            self.execute_cmd(cmd)
        except Exception, exc:
            log_error("An error occured during execution of ogr2ogr file import: %s\n", exc)
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
            self.shapefile_path.set_value(filechooser.get_path())



class ContentPreviewPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Select Layers to Import", wide=True)


    def create_ui(self):
        self.set_spacing(16)



class ImportProgressPage(WizardProgressPage):
    importer = None

    def __init__(self, owner):
        WizardProgressPage.__init__(self, owner, "Import Data")

        self.add_task(self.prepare_import, "Prepare import")
        self.add_threaded_task(self.start_import, "Import data file")


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
        return True


    def start_import(self):
        self.importer.run()
        return True


class ResultsPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Import Results")


    def create_ui(self):
        self.content.add(mforms.newLabel("File %s was imported in %i:%02is"))
        self.content.add(mforms.newLabel("Table %s was created and %i records were inserted"))
        # etc etc


class SpatialImporterWizard(WizardForm):
    def __init__(self, editor):
        WizardForm.__init__(self, mforms.Form.main_form())

        self.editor = editor
        
        self.selected_schema = None

        self.set_title("Load Spatial Data")

        self.center()

        self.select_file_page = SelectFileWizardPage(self)
        self.add_page(self.select_file_page)

        self.import_page = ImportProgressPage(self)
        self.add_page(self.import_page)

        self.results_page = ResultsPage(self)
        self.add_page(self.results_page)


    def set_schema(self, schema_name):
        self.select_file_page.schema_label.set_text("Tables will be imported to schema: %s" % schema_name)
        self.selected_schema = schema_name

