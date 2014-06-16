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

import os, platform, threading, subprocess


from mforms import newButton
from mforms import FileChooser

try:
    import _subprocess
except ImportError:
    pass

from workbench.log import log_error, log_debug


def showImporter(editor, schema):
    importer = SpatialImporterGui(editor)
    dpoint = mforms.fromgrt(editor.dockingPoint)
    dpoint.dock_view(importer, "", 0)
    dpoint.select_view(importer)
    importer.set_title("Spatial Importer")
    importer.set_schema(schema)

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
    
    
class SpatialImporter(threading.Thread):
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
        
        threading.Thread.__init__(self)

    def execute_cmd(self, cmd):
        print cmd
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
        try:
            self.execute_cmd(cmd)
        except Exception, exc:
            log_error("An error occured during execution of ogr2ogr file import: %s\n", exc)
        self.is_running = False


class SpatialImporterGui(mforms.AppView):
    def __init__(self, editor):
        mforms.AppView.__init__(self, False, "spatial_importer", False)

        self.editor = editor
        
        self.selected_schema = None
        
        self.importer = None
        
        self.content = mforms.newBox(False)
        self.content.set_padding(12)
        self.content.set_spacing(8)
        self.add(self.content, True, True)
        
        self.schema_label = mforms.newLabel("Schema: ")
        
        self.content.add(self.schema_label, False, False)
        
        entry_box = mforms.newBox(True)
        entry_box.set_spacing(5)

        entry_box.add(mforms.newLabel("Shapefile path:"), False, False)
        self.shapefile_path = mforms.newTextEntry()
        entry_box.add(self.shapefile_path, True, True)
        
        self.shapefile_browse_btn = newButton()
        self.shapefile_browse_btn.set_text("Browse")
        self.shapefile_browse_btn.add_clicked_callback(self.shapefile_browse)
        entry_box.add_end(self.shapefile_browse_btn, False, False)

        self.content.add(entry_box, False, True)

        self.start_btn = mforms.newButton()
        self.start_btn.set_text("Start")
        self.start_btn.add_clicked_callback(self.start_import)
        
        self.content.add(self.start_btn, False, True)
        
    def get_mysql_password(self, connection):
        parameterValues = connection.parameterValues
        pwd = parameterValues["password"]
        if not pwd:
            username = parameterValues["userName"]
            host = connection.hostIdentifier
            title = "Import file (type the correct password)"
            accepted, pwd = mforms.Utilities.find_or_ask_for_password(title, host, username, False)
            if not accepted:
                return None
        return pwd
        
    def notify_finished(self):
        mforms.Utilities.show_message("Import", "Import finished", "Ok", "", "")
        
    def start_import(self):
        if self.importer and self.importer.is_running:
            mforms.Utilities.show_message("Importing...", "Import thread is already running.", "Ok", "", "")
            return;

        self.importer = SpatialImporter()
        self.importer.filepath = self.shapefile_path.get_string_value()
        if self.importer.filepath == None or os.path.isfile(self.importer.filepath) == False:
            log_error("Unable to open specified file: %s\n" % self.importer.filepath)
            return

        self.importer.my_pwd = self.get_mysql_password(self.editor.connection)
        if self.importer.my_pwd == None:
            log_error("Unknown MySQL password\n")
            return
        self.importer.my_host = self.editor.connection.parameterValues.hostName
        self.importer.my_port = self.editor.connection.parameterValues.port
         
        self.importer.my_user = self.editor.connection.parameterValues.userName
        self.importer.my_schema = self.selected_schema

        self.importer.start()
        
    def set_schema(self, schema_name):
        self.schema_label.set_text("Schema: %s" % schema_name)
        self.selected_schema = schema_name

        
    def shapefile_browse(self):
        filechooser = FileChooser(mforms.OpenFile)
        filechooser.set_directory(os.path.dirname(self.shapefile_path.get_string_value()))
        filechooser.set_extensions("Spatial Shape File (*.shp)|*.shp", "shp");
        if filechooser.run_modal():
            self.shapefile_path.set_value(filechooser.get_path())
        
