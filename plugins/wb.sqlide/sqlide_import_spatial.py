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

from __future__ import with_statement

# import the mforms module for GUI stuff
import mforms
import grt
from workbench.utils import get_exe_path
from workbench.utils import Version


import sys, os, platform, subprocess
os_icon_suffix = ""
if sys.platform=="darwin":
    os_icon_suffix = "_mac"


from mforms import newButton, newCheckBox, newTreeView
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
            p1 = subprocess.Popen(cmd, stdout = subprocess.PIPE, stderr=subprocess.PIPE, shell = True)
        except OSError, exc:
            log_error("Error executing command %s\n%s\n" % (cmd, exc));
            import traceback
            traceback.print_exc()
    else:
        try:
            info = subprocess.STARTUPINFO()
            info.dwFlags |= _subprocess.STARTF_USESHOWWINDOW
            info.wShowWindow = _subprocess.SW_HIDE
            # Command line can contain object names in case of export and filename in case of import
            # Object names must be in utf-8 but filename must be encoded in the filesystem encoding,
            # which probably isn't utf-8 in windows.
            
            if isinstance(cmd, list):
                for idx,item in enumerate(cmd):
                    cmd[idx] = item.encode("utf8") if isinstance(item,unicode) else item 
                log_debug("Executing command: %s\n" % "".join(cmd))
            else:
                cmd = cmd.encode("utf8") if isinstance(cmd,unicode) else cmd
                log_debug("Executing command: %s\n" % cmd)
            p1 = subprocess.Popen(cmd, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE, startupinfo=info, shell = True)
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
        self.destination_EPSG = None
        
        self.my_geo_column = "shape"
        self.abort_requested = False
        
        self.is_running = False
        self.returncode = None
        self.user_cancel = False


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
            if self.user_cancel:
                log_info("Execute command failed with error: %s, the exit code was: %d.\n" % (serr, self.returncode))
                raise grt.UserInterrupt()
            else:
                log_error("Execute command failed with error: %s, the exit code was: %d.\n" % (serr, self.returncode))
                raise Exception(serr)
            
        log_info("Execute command succeeed.\n")
            


    def print_log_message(self, msg):
        print msg
 
    def kill(self):
        self.abort_requested = True
        if self.process_handle:
            if platform.system() == 'Windows':
                cmd = "taskkill /F /T /PID %i" % self.process_handle.pid
                log_debug("Killing task: %s\n" % cmd)
                subprocess.Popen(cmd , shell=True)
                self.user_cancel = True
            else:
                import signal
                try:
                    log_debug("Sending SIGTERM to task %s\n" % self.process_handle.pid)
                    os.kill(self.process_handle.pid, signal.SIGTERM)
                    self.user_cancel = True
                except OSError, exc:
                    log_error("Exception sending SIGTERM to task: %s\n" % exc)
                    self.print_log_message("kill task: %s" % str(exc))
                     
    def run(self, progress_notify):
        cmd_args = {}
        cmd_args['ogr2ogr'] = get_exe_path("ogr2ogr")
        cmd_args['host'] = self.my_host
        cmd_args['schema'] = self.my_schema
        cmd_args['user'] = self.my_user
        cmd_args['pwd'] = self.my_pwd
        cmd_args['port'] = self.my_port
        
        cmd = []
        cmd.append("exec")
        cmd.append(get_exe_path("ogr2ogr"))
        cmd.append("-f")
        cmd.append('"MySQL"')
        cmd.append('MySQL:"%(schema)s,host=%(host)s,user=%(user)s,password=%(pwd)s,port=%(port)d"' % cmd_args)
        cmd.append(self.filepath)

        if self.import_table:
            cmd.append('-nln')
            cmd.append('"%s"' % self.import_table)
            
        if self.destination_EPSG:
            cmd.append('-t_srs')
            cmd.append('EPSG:' % self.destination_EPSG)

        if self.skipfailures:
            cmd.append('-skipfailures')

        if self.import_append:
            cmd.append('-append')

        if self.import_overwrite:
            cmd.append('-overwrite')

        if self.selected_fields:
            cmd.append('-select')
            cmd.append(self.selected_fields)
        
        cmd.append('-progress')
        cmd.append('-gt')
        cmd.append(str(self.features_per_transcation))
        cmd.append('-lco')
        cmd.append('GEOMETRY_NAME="%s"' % self.my_geo_column)
        cmd.append('-lco')
        cmd.append('ENGINE=InnoDb')
        cmd.append('-lco')
        if self.spatial_index:
            cmd.append('SPATIAL_INDEX=YES')
        else:
            cmd.append('SPATIAL_INDEX=NO')

        self.is_running = True
        try:
            self.execute_cmd(" ".join(cmd), progress_notify)
        except grt.UserInterrupt:
            log_info("User cancelled")
            raise
        except Exception, exc:
            import traceback
            log_error("An error occured during execution of ogr2ogr file import: %s, stack: %s\n The command was: %s\n" % (exc, traceback.format_exc(), cmd))
            raise
        self.is_running = False


from workbench.ui import WizardForm, WizardPage, WizardProgressPage

class SelectFileWizardPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Select File to Import", wide=True)

        self.schema_label = mforms.newLabel("Target schema: ")

        self.back_button.set_enabled(False)
        self.ogrinfo_missing = True
        self.ogr2ogr_missing = True


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

        self.ogrinfo_box = mforms.newBox(True)
        self.ogrinfo_box.set_spacing(8)
        self.ogrinfo_icon = mforms.newImageBox()
        self.ogrinfo_icon.set_image("task_unchecked%s.png" % os_icon_suffix)
        self.ogrinfo_box.add(self.ogrinfo_icon, False, True)
        ogrinfo_label = mforms.newLabel("Check if ogrinfo tool is present.")
        self.ogrinfo_box.add(ogrinfo_label, True, True)
        self.content.add(self.ogrinfo_box, False, True)
        self.ogrinfo_missing_lbl = mforms.newLabel("ogrinfo executable wasn't found. Please check if it's available in the PATH.")
        self.ogrinfo_missing_lbl.show(False)
        self.content.add(self.ogrinfo_missing_lbl, False, True)
    
        self.ogr2ogr_box = mforms.newBox(True)
        self.ogr2ogr_box.set_spacing(8)
        self.ogr2ogr_icon = mforms.newImageBox()
        self.ogr2ogr_icon.set_image("task_unchecked%s.png" % os_icon_suffix)
        self.ogr2ogr_box.add(self.ogr2ogr_icon, False, True)
        ogr2ogr_label = mforms.newLabel("Check if ogr2ogr tool is present.")
        self.ogr2ogr_box.add(ogr2ogr_label, True, True)
        self.content.add(self.ogr2ogr_box, False, True)
        self.ogr2ogr_missing_lbl = mforms.newLabel("ogr2ogr executable wasn't find. Please check if it's available in the PATH.")
        self.ogr2ogr_missing_lbl.show(False)
        self.content.add(self.ogr2ogr_missing_lbl, False, True)
        
        
       
        
        self.dbf_box = mforms.newBox(True)
        self.dbf_box.set_spacing(8)
        self.dbf_icon = mforms.newImageBox()
        self.dbf_icon.set_image("task_unchecked%s.png" % os_icon_suffix)
        self.dbf_box.add(self.dbf_icon, False, True)
        dbf_label = mforms.newLabel("Check if dbf file is present.")
        self.dbf_box.add(dbf_label, True, True)
        self.dbf_box.show(True)
         
        self.content.add(self.dbf_box, False, True)
         
        self.proj_box = mforms.newBox(True)
        self.proj_box.set_spacing(8)
        self.proj_icon = mforms.newImageBox()
        self.proj_icon.set_image("task_unchecked%s.png" % os_icon_suffix)
        self.proj_box.add(self.proj_icon, False, True)
        proj_label = mforms.newLabel("Check if prj file is present.")
        self.proj_box.add(proj_label, True, True)
        self.proj_box.show(True)
         
        self.content.add(self.proj_box, False, True)
        
        self.warning_srs = mforms.newLabel("")
        self.content.add(self.warning_srs, False, True)

    def go_cancel(self):
        self.main.cancel()

    def check_ogr_executables(self):
        if get_exe_path("ogrinfo"):
            self.ogrinfo_missing = False
            self.ogrinfo_icon.set_image("task_checked%s.png" % os_icon_suffix)
        else:
            self.ogrinfo_icon.set_image("task_warning%s.png" % os_icon_suffix)
            self.ogrinfo_missing_lbl.show(True)

        if get_exe_path("ogr2ogr"):
            self.ogr2ogr_missing = False
            self.ogr2ogr_icon.set_image("task_checked%s.png" % os_icon_suffix)
        else:
            self.ogr2ogr_icon.set_image("task_warning%s.png" % os_icon_suffix)
            self.ogr2ogr_missing_lbl.show(True)

    def validate(self):
        self.check_ogr_executables()
        if self.ogrinfo_missing or self.ogr2ogr_missing:
            mforms.Utilities.show_error("Missing Executable", "One of the required executables is missing. Please correct this before continue.", "OK", "", "")
            return False
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
        self.support_spatial_index = Version.fromgrt(owner.editor.serverVersion).is_supported_mysql_version_at_least(5, 7, 5)

    def get_path(self):
        return self.main.select_file_page.shapefile_path.get_string_value()
    
    def get_info(self):
        cmd = "%s -al -so %s" % (get_exe_path("ogrinfo"), self.get_path())
        p1 = cmd_executor(cmd)
        sout, serr = p1.communicate(input)
        if serr:
            log_error("There was an error getting file information: %s" % serr)
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

        from grt.modules import Utilities
        res = Utilities.fetchAuthorityCodeFromFile("%s.prj" % os.path.splitext(self.get_path())[0])
        if res:
            self.epsg_lbl.set_text(res)
        else:
            self.epsg_lbl.set_text(0)
            log_info("Can't find EPSG fallback to 0")

    def go_cancel(self):
        self.main.cancel()
        
    def validate(self):
        result = 0
        try:
            result = int(self.convert_to_epsg.get_string_value())
        except:
            pass
        if len(self.convert_to_epsg.get_string_value()) and result == 0:
            mforms.Utilities.show_error("Destination ESPG", "Incorrect destination EPSG. Value should be a number.", "OK", "", "")
            return False
        
        sfields = self.get_fields()
        if len(sfields) == 0:
            mforms.Utilities.show_error("Missing columns", "Please specify at least one column to import.", "OK", "", "")
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
        layer_box = mforms.newBox(True)
        layer_box.set_spacing(8)
        layer_heading = mforms.newLabel("Layer name:")
        layer_box.add(layer_heading, False, False)
        self.layer_name_lbl = mforms.newLabel("")
        layer_box.add(self.layer_name_lbl, False, False)
        self.content.add(layer_box, False, False)

        epsg_box = mforms.newBox(True)
        epsg_box.set_spacing(8)
        epsg_box_heading = mforms.newLabel("EPSG:")
        epsg_box.add(epsg_box_heading, False, False)
        self.epsg_lbl = mforms.newLabel("")
        epsg_box.add(self.epsg_lbl, False, False)
        self.content.add(epsg_box, False, False)
        
        entry_box = mforms.newBox(True)
        entry_box.set_spacing(12)

        entry_box.add(mforms.newLabel("Destination table:"), False, True)

        self.table_name = mforms.newTextEntry()
        entry_box.add(self.table_name, True, True)
        
        self.content.add(entry_box, False, True)
        entry_box.show(True)

        cbox = mforms.newBox(False)

        self.column_list = newTreeView(mforms.TreeFlatList)
        self.column_list.add_column(mforms.CheckColumnType, "", 40, True)
        self.column_list.add_column(mforms.StringColumnType, "Column name", 300, False)
        self.column_list.end_columns()
        self.column_list.set_size(-1, 150)

        cbox.add(small_label("Please select the columns you want to import:"), False, True)
        cbox.add(self.column_list, False, True)

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
        boxfailures.add(small_label("If an error occurs ignore it and continue processing data."), False, False)
        options_box.add(boxfailures, False, False)
        
        boxappend = mforms.newBox(False)
        
        self.append_chb = newCheckBox()
        self.append_chb.set_text("Append to existing data");
        self.append_chb.set_active(False)
        boxappend.add(self.append_chb, False, False)
        boxappend.add(small_label("Append to existing table instead of creating a new one."), False, False)
        options_box.add(boxappend, False, False)
        
        boxoverwrite = mforms.newBox(False)
        
        self.overwrite_chb = newCheckBox()
        self.overwrite_chb.set_text("Overwrite existing data");
        self.overwrite_chb.set_active(False)
        
        self.append_chb.add_clicked_callback(lambda checkbox1 = self.append_chb, checkbox2 = self.overwrite_chb: self.one_check_only(checkbox1, checkbox2))
        self.overwrite_chb.add_clicked_callback(lambda checkbox2 = self.append_chb, checkbox1 = self.overwrite_chb: self.one_check_only(checkbox1, checkbox2))
        boxoverwrite.add(self.overwrite_chb, False, False)
        boxoverwrite.add(small_label("Drop the selected table and recreate it."), False, False)
        options_box.add(boxoverwrite, False, False)
        
        if self.support_spatial_index:
            boxspatial = mforms.newBox(False)
            self.spatial_index_chb = newCheckBox()
            self.spatial_index_chb.set_text("Create spatial index")
            self.spatial_index_chb.set_active(False)
            boxspatial.add(self.spatial_index_chb, False, False)
            boxspatial.add(small_label("import will make spatial index around geometry column"), False, False)
            options_box.add(boxspatial, False, False)
        
        options_layer.add(options_box)
        options_layer.show(True)
        
        self.content.add(options_layer, False, False)

        boxconvert = mforms.newBox(False)
        entry_box = mforms.newBox(True)
        entry_box.set_spacing(8)
        entry_box.add(mforms.newLabel("Convert data to the following EPSG:"), False, True)
        self.convert_to_epsg = mforms.newTextEntry()
        entry_box.add(self.convert_to_epsg, False, False)
        boxconvert.add(entry_box, True, True)
        boxconvert.add(small_label("leave empty to import the data with no conversion"), False, False)
        
        self.content.add(boxconvert, False, True)
        self.get_info()
        
    def one_check_only(self, chk1, chk2):
        if chk1.get_active():
            chk2.set_active(False)


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
    
    def page_activated(self, advancing):
        self.reset(True)
        self.importer = None
        super(ImportProgressPage, self).page_activated(advancing)

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
        self.importer.import_overwrite = self.main.content_preview_page.overwrite_chb.get_active()
        self.importer.import_append = self.main.content_preview_page.append_chb.get_active()
        if hasattr(self.main.content_preview_page, "spatial_index_chb"):
            self.importer.spatial_index = self.main.content_preview_page.spatial_index_chb.get_active()
        else:
            self.importer.spatial_index = False
        
        if self.main.content_preview_page.table_name.get_string_value() != "":
            self.importer.import_table = self.main.content_preview_page.table_name.get_string_value()
            
        if self.main.content_preview_page.convert_to_epsg.get_string_value() != "":
            self.importer.destination_EPSG = int(self.main.content_preview_page.convert_to_epsg.get_string_value())

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
            else:
                self.main.cancel()


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
        self.set_size(800, 650)
        
    def cancel(self):
        if self.on_close():
            self.finish()

    def set_schema(self, schema_name):
        self.select_file_page.schema_label.set_text("Tables will be imported to schema: %s" % schema_name)
        self.selected_schema = schema_name
        

