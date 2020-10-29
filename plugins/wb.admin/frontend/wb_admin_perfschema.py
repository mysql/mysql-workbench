# Copyright (c) 2013, 2020, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

import mforms
import grt

import re
import os

from threading import Thread
from queue import Queue, Empty
from workbench.client_utils import MySQLScriptImporter

from wb_admin_utils import weakcb, MessageButtonPanel, WbAdminTabBase, WbAdminValidationBase, WbAdminValidationConnection

from workbench.utils import Version

from workbench.log import log_info, log_error, log_warning
from mforms import FileChooser




#===============================================================================
# WB SYS Schema Deployment

def download_server_install_script(ctrl_be):
    server_helper = ctrl_be.server_helper
    profile = server_helper.profile
    install_script_path = server_helper.join_paths(profile.basedir, "share", "mysql_sys_schema.sql")

    try:

        if not server_helper.file_exists(install_script_path):
            log_warning("The server does not supply the sys schema install script\n")
            return None
          
        if not server_helper.check_file_readable(install_script_path):
            log_warning("The server supplies the sys schema install script, but it's not readable\n")
            return None

        install_script_content = server_helper.get_file_content(install_script_path)

    except Exception:
        import traceback
        log_error("There was an exception when making validations:\n%s\n" % traceback.format_exc())
        return None

    # Import the file to the local user space
    local_install_script_path = os.path.join(mforms.App.get().get_user_data_folder(), "install_sys_script.sql")
    f = open(local_install_script_path, "w")
    
    for line in install_script_content.split('\n'):
        content = ""
        if line.startswith("CREATE DEFINER='root'@'localhost'"):
            # Set delimiters for functions/procedures/triggers, so that we can run them on the server properly
            delimiter = delimiter = grt.root.wb.options.options['SqlDelimiter']
            line = re.sub(r'(.*);', r'\1' + delimiter, line)
            content = "\nDELIMITER " + delimiter + "\n%s\nDELIMITER ;\n" % line
        else:
            content = line
        f.write("%s\n" % content)
    f.close()
    
    return local_install_script_path


def get_sys_version_from_script(file_path):
    """Gets the version of the sys schema that's shipped with Workbench."""
    if not os.path.exists(file_path):
        log_info("No sys script found\n")
        return None
    for line in open(file_path):
        if line.startswith("CREATE OR REPLACE"):
            m = re.findall("SELECT '(.*)' AS sys_version", line)
            if m:
                return m[0]
    return None

def get_current_sys_version(server_version):
    """Gets the version of the sys schema that's shipped with Workbench."""
    syspath = mforms.App.get().get_resource_path("sys")
    path = os.path.join(syspath, "before_setup.sql")
    return get_sys_version_from_script(path)


def get_installed_sys_version(sql_editor):
    """Checks whether the sys schema is installed and up to date."""
    try:
        res = sql_editor.executeManagementQuery("SELECT sys_version FROM sys.version", 0)
        if res.goToFirstRow():
            return res.stringFieldValue(0)
    except grt.DBError as e:
        log_error("MySQL error getting sys schema version: %s\n" % e)
        if e.args[1] == 1146: # table doesn't exist
            return None
        if e.args[1] == 1142: # user does not have sufficient privileges
            return "access_denied";
        raise


class HelperInstallPanel(mforms.Table):
    def __init__(self, owner, editor, ctrl_be):
        mforms.Table.__init__(self)
        self.set_managed()
        self.set_release_on_add()

        self.set_row_count(2)
        self.set_column_count(1)

        self.set_padding(-1)

        self.label = mforms.newLabel("Installing...")
        self.add(self.label, 0, 1, 0, 1, mforms.HFillFlag)
        
        self.progress = mforms.newProgressBar()
        self.add(self.progress, 0, 1, 1, 2, mforms.HFillFlag)
        
        self.progress.set_size(400, -1)
        
        self.owner = owner
        self.ctrl_be = ctrl_be
        
        self.importer = MySQLScriptImporter(editor.connection)
        self.importer.report_progress = self.report_progress
        self.importer.report_output = self.report_output
        
        self._worker_queue = Queue()
        self._worker = None
        self._progress_status = None
        self._progress_value = 0

        self._update_timer = None
        self._messages = []

    def __del__(self):
        if self._update_timer:
            mforms.Utilities.cancel_timeout(self._update_timer)

    @property
    def is_busy(self):
        return self._worker != None


    def report_progress(self, status, current, total):
        self._progress_status = status
        if total > 0:
            self._progress_value = float(current) / total


    def report_output(self, message):
        log_info("%s\n" % message)
        self._messages.append(message)


    def update_ui(self):
        try:
            data = self._worker_queue.get_nowait()
            if data is None:
                self._worker.join()
                self._worker = None
                self.owner.page_activated()
                self._update_timer = None
                if self._messages:
                    mforms.Utilities.show_message("Install sys Schema", "Import output:\n%s" % "\n".join(self._messages), "OK", "", "")
                return False

            if isinstance(data, Exception):
                if isinstance(data, grt.DBError) and data.args[1] == 1044:
                    mforms.Utilities.show_error("Install sys Schema",
                                                "The current MySQL account does not have enough privileges to create the sys schema.\nPlease use an account with schema creation privileges or ask an administrator to install sys.", "OK", "", "")
                elif isinstance(data, grt.DBLoginError):
                    mforms.Utilities.show_error("Install sys Schema",
                                                "Error installing sys Schema.\n"+str(data), "OK", "", "")
                else:
                    mforms.Utilities.show_error("Install sys Schema",
                                                "Error installing sys Schema.\n"+str(data), "OK", "", "")
        except Empty:
            pass

        if self._progress_status is not None:
            self.label.set_text(self._progress_status)
        self.progress.set_value(self._progress_value)

        return True

    def install_scripts(self, files, message):
        try:
            for f, db in files:
                self._progress_status = "%s %s..." % (message, f)
                self._progress_value = 0
                self.importer.import_script(f, db)

            log_info("sys schema installation finished\n")
        except grt.DBLoginError as e:
            log_error("MySQL login error installing sys schema: %s\n" % e)
            self._worker_queue.put(e)
        except grt.DBError as e:
            log_error("MySQL error installing sys schema: %s\n" % e)
            self._worker_queue.put(e)
        except Exception as e:
            import traceback
            log_error("Unexpected exception installing sys schema: %s\n%s\n" % (e, traceback.format_exc()))
            self._worker_queue.put(e)
        self._worker_queue.put(None)
            
            
    def work(self, files):
        try:
            if self.ctrl_be.target_version >= Version(5, 7, 10):
                self.importer.reset_schemas()
            else:
                location = download_server_install_script(self.ctrl_be)
              
                if location:
                    workbench_version_string = get_current_sys_version(None)
                    server_version_string = get_sys_version_from_script(location)
                    
                    maj, min, rel = [int(i) for i in workbench_version_string.split(".")]
                    workbench_version = Version(maj, min, rel)
                    maj, min, rel = [int(i) for i in server_version_string.split(".")]
                    server_version = Version(maj, min, rel)

                    if server_version >= workbench_version:
                        log_info("Installing sys schema supplied by the server: %s\n" % str(location))
                        self.install_scripts([(location, None)], "Installing server script")
                        return
                    else:
                        log_info("Server sys schema install script exists but it's outdated compared to the one supplied by Workbench...\n")
                        
                        
                log_info("Installing sys schema supplied by workbench\n")
                self.install_scripts(files, "Installing Workbench script")
        except Exception as e:
              log_error("Runtime error when installing the sys schema: %s\n" % str(e))
              self._worker_queue.put(e)
        
        # This makes the screen refresh
        self._worker_queue.put(None)      
              
    def start(self):
        server_profile = self.owner.ctrl_be.server_profile
        parameterValues = server_profile.db_connection_params.parameterValues
        username = parameterValues["userName"]
        host = server_profile.db_connection_params.hostIdentifier
        accepted, pwd = mforms.Utilities.find_or_ask_for_password("Install sys Schema", host, username, False)
        if not accepted:
            return
        self.importer.set_password(pwd)
      
        syspath = mforms.App.get().get_resource_path("sys")

        server_version = self.owner.ctrl_be.target_version
        main_sys = "sys_%i%i.sql" % (server_version.majorNumber, server_version.minorNumber)

        self._worker = Thread(target = self.work, args = ([(os.path.join(syspath, main_sys), None)],))
        self._worker.start()
    
        self._update_timer = mforms.Utilities.add_timeout(0.2, self.update_ui)



    def install(self, what, default_db):
        self.importer.import_script(what, default_db)


class WbAdminValidationPSUsable(WbAdminValidationBase):
    def __init__(self, main_view):
        super().__init__("Performance Schema is either unavailable or disabled on this server.\nYou need a MySQL server version 5.6 or newer, with the performance_schema feature enabled.")
        self._main_view = main_view
        
    def validate(self):
        try:
            res = self._main_view.editor.executeManagementQuery("select @@performance_schema", 0)
            if res.goToFirstRow():
                return res.stringFieldValue(0) == "1"
        except grt.DBError as e:
            log_error("MySQL error retrieving the performance_schema variable: %s\n" % e)
        return False


class WbAdminValidationNeedsInstallation(WbAdminValidationBase):
    def __init__(self, main_view, ctrl_be, owner):
        super().__init__("Performance Schema is either unavailable or disabled on this server.\nYou need a MySQL server version 5.6 or newer, with the performance_schema feature enabled.")
        self._main_view = main_view
        self._ctrl_be = ctrl_be
        self._error_title = ""
        self._install_button = None
        self._owner = owner

    def add_install_button(self):
        self._install_button = ("Install Helper", self.install_helper)

    def get_missing_grants(self):
        required_grants = ['SELECT', 'INSERT', 'CREATE', 'DROP', 'ALTER', 'SUPER', 'CREATE VIEW', 'CREATE ROUTINE', 'ALTER ROUTINE', 'TRIGGER']
        missing_grants = []
        current_user_grants = ""
        try:
            res = self._main_view.editor.executeManagementQuery("show grants", 0)
            if res.goToFirstRow():
                current_user_grants = res.stringFieldValue(0)
        except grt.DBError as e:
            log_error("MySQL error retrieving user grants: %s\n" % e)

        # First we check if there's full grant set,
        if current_user_grants.find("ALL") != -1:
            return missing_grants
    
        # only if not, we will check for the other
        for grant in required_grants:
            if current_user_grants.find(grant) == -1:
                missing_grants.append(grant)

        return missing_grants

    def validate(self):
        try:
            self._ctrl_be.acquire_admin_access()
            installed_version = get_installed_sys_version(self._main_view.editor)
            install_text = ""
            missing_grants = self.get_missing_grants()
            install_text = """\n\nTo install the performance schema helper, you need the following privileges:
    - SELECT, INSERT, CREATE, DROP, ALTER, SUPER, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE and TRIGGER"""
            can_install = True
            if len(missing_grants) > 0:
                can_install = False
                if installed_version == "access_denied":
                    install_text = ""
                install_text = "%s\n\nThe following grants are missing:\n  - %s" % (install_text, str(missing_grants))
    
            if not installed_version:
        
                self.set_error_message("""The Performance Schema helper schema (sys) is not installed.
    Click the [Install Helper] button to install it.
    You must have at least the following privileges to use Performance Schema functionality:
      - SELECT on performance_schema.*
      - UPDATE on performance_schema.setup* for configuring instrumentation
      %s""" % install_text)
                self.add_install_button()
                return False

            elif installed_version == "access_denied":
                self.set_error_message("""The Performance Schema helper schema (sys) is not accesible.
    You must have at least the following privileges to use Performance Schema functionality:
      - SELECT on performance_schema.*
      - UPDATE on performance_schema.setup* for configuring instrumentation
      %s""" % install_text)
                return False
            else:
                curversion = get_current_sys_version(self._ctrl_be.target_version)
                x, y, z = [int(i) for i in curversion.split(".")]
                ix, iy, iz = [int(i) for i in installed_version.split(".")]
                
                # major number is incremented on backwards incompatible changes
                # minor number is incremented when new things are added
                # release number is incremented on any other kind of change
                version_ok = False
                if ix > x:
                    version_ok = True
                elif ix == x:
                    if iy > y:
                        version_ok = True
                    elif iy == y and iz >= z:
                        version_ok = True
                
                if not version_ok:
                    self.set_error_message("Performance Schema helper schema (sys) is outdated\n\nMySQL Workbench needs to upgrade it.\n(current version is %s, server has %s%s)." % (curversion, installed_version, install_text))
                    self.add_install_button()
                    return False
        except grt.DBError as e:
            self.set_error_message("Unable to access Performance Schema helper schema (sys)\n\n%s (error %s)\n\nIf the sys schema is already installed, make sure you have SELECT privileges on it.\nIf not, you will need privileges to create the `sys` schema and populate it with views and stored procedures for PERFORMANCE_SCHEMA." % e.args)
            return False

        except Exception as e:
            import traceback
            log_error("Error checking for PS helper: %s\n" % traceback.format_exc())
            self.set_error_message("Unable to access Performance Schema helper (sys) \n\n%s\n\nIf the sys schema is already installed, make sure you have SELECT privileges on it.\nIf not, you will need privileges to create the `sys` schema and populate it with views and stored procedures for PERFORMANCE_SCHEMA." % str(e))
            return False

        return True

    def errorScreen(self):
        self._content = MessageButtonPanel("", self._error_title, self._error_message, self._install_button)
      
        return self._content
      
    def install_helper(self):
        self.installer_panel = HelperInstallPanel(self._owner, self._main_view.editor, self._ctrl_be)
        self._content.add(self.installer_panel, 1, 2, 2, 3, 0)
        self._owner.relayout() # needed b/c of layout bug in Mac

        if self._ctrl_be.target_version >= Version(5, 7, 10):
            filechooser = FileChooser(mforms.OpenFile)
            filechooser.set_title("Specify the location of mysql_upgrade")
            if filechooser.run_modal():
                self.installer_panel.importer._upgrade_tool_path = filechooser.get_path()

        self.installer_panel.importer.set_password(self._ctrl_be.get_mysql_password())
        self.installer_panel.start()


class WbAdminPSBaseTab(WbAdminTabBase):
    sys = "sys"
    ui_created = False

    def __init__(self, ctrl_be, instance_info, main_view):
        WbAdminTabBase.__init__(self, ctrl_be, instance_info, main_view)
        self.content = None

        self.add_validation(WbAdminValidationConnection(ctrl_be))
        self.add_validation(WbAdminValidationPSUsable(main_view))
        self.add_validation(WbAdminValidationNeedsInstallation(main_view, ctrl_be, self))

    def get_select_int_result(self, query):
        res = self.main_view.editor.executeManagementQuery(query, 0)
        if res and res.goToFirstRow():
            return res.intFieldValue(0)
        return None



