# Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved.
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


import mforms
import grt

import re
import os

from threading import Thread
from Queue import Queue, Empty
from workbench.client_utils import MySQLScriptImporter

from wb_admin_utils import make_panel_header, MessageButtonPanel

from workbench.log import log_info, log_error




#===============================================================================
# WB SYS Schema Deployment

def get_current_sys_version(server_version):
    """Gets the version of the sys schema that's shipped with Workbench."""
    syspath = mforms.App.get().get_resource_path("sys")
    path = os.path.join(syspath, "before_setup.sql")
    if not os.path.exists(path):
        log_info("No sys script found\n")
        return None
    for line in open(path):
        if line.startswith("CREATE OR REPLACE ALGORITHM"):
            m = re.findall("SELECT '(.*)' AS sys_version", line)
            if m:
                return m[0]
    return None


def get_installed_sys_version(sql_editor):
    """Checks whether the sys schema is installed and up to date."""
    try:
        res = sql_editor.executeManagementQuery("SELECT sys_version FROM sys.version", 0)
        if res.goToFirstRow():
            return res.stringFieldValue(0)
    except grt.DBError, e:
        log_error("MySQL error getting sys schema version: %s\n" % e)
        if e.args[1] == 1146: # table doesn't exist
            return None
        raise



class HelperInstallPanel(mforms.Table):
    def __init__(self, owner, editor):
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
            
            
    def work(self, files):
        try:
            for f, db in files:
                log_info("Installing %s...\n" % f)
                self._progress_status = "Installing %s..." % f
                self._progress_value = 0
                self.importer.import_script(f, db)

            log_info("sys schema installation finished\n")
        except grt.DBLoginError, e:
            log_error("MySQL login error installing sys schema: %s\n" % e)
            self._worker_queue.put(e)
        except grt.DBError, e:
            log_error("MySQL error installing sys schema: %s\n" % e)
            self._worker_queue.put(e)
        except Exception, e:
            import traceback
            log_error("Unexpected exception installing sys schema: %s\n%s\n" % (e, traceback.format_exc()))
            self._worker_queue.put(e)
        self._worker_queue.put(None)


    def start(self):
        server_profile = self.owner.ctrl_be.server_profile
        parameterValues = server_profile.db_connection_params.parameterValues
        pwd = parameterValues["password"]
        if not pwd:
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



class WbAdminPSBaseTab(mforms.Box):
    sys = "sys"
    ui_created = False

    def __init__(self, ctrl_be, instance_info, main_view):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        
        self.set_spacing(12)
        self.set_padding(12)

        self.instance_info = instance_info
        self.ctrl_be = ctrl_be
        self.main_view = main_view
    
        self.installer_panel = None
        self.warning_panel = None
        self.content = None


    def create_basic_ui(self, icon, title, button=None):
        self.heading = make_panel_header(icon, self.instance_info.name, title, button)
        self.heading.set_padding(8)
        self.add(self.heading, False, False)

        self.content = None


    def get_select_int_result(self, query):
        res = self.main_view.editor.executeManagementQuery(query, 0)
        if res and res.goToFirstRow():
            return res.intFieldValue(0)
        return None

    def ps_helper_needs_installation(self):
        try:
            installed_version = get_installed_sys_version(self.main_view.editor)
            if not installed_version:
                return "The Performance Schema helper schema (sys) is not installed", \
    """Click the [Install Helper] button to install it.
    You must have at least the following privileges to use Performance Schema functionality:
      - SELECT on performance_schema.*
      - UPDATE on performance_schema.setup* for configuring instrumentation
                    
    and
                    
      - ALL on sys to be able to install the performance schema helper"""
            else:
                curversion = get_current_sys_version(self.ctrl_be.target_version)
                x, y, z = [int(i) for i in curversion.split(".")]
                ix, iy, iz = [int(i) for i in installed_version.split(".")]
                
                # major number is incremented on backwards incompatible changes
                # minor number is incremented when new things are added
                # release number is incremented on any other kind of change
                
                if ix == x and (iy > y or (iy == y and iz >= z)):
                    pass # ok
                elif x < ix:
                    return "Installed Performance Schema helper (sys) version is newer than supported", "MySQL Workbench needs to downgrade it.\n(current version is %s, server has %s)." % (curversion, installed_version)
                else:
                    return "Performance Schema helper schema (sys) is outdated", "MySQL Workbench needs to upgrade it.\n(current version is %s, server has %s)." % (curversion, installed_version)
        except grt.DBError, e:
            return "Unable to access Performance Schema helper schema (sys)", "%s (error %s)\n\nIf the sys schema is already installed, make sure you have SELECT privileges on it.\nIf not, you will need privileges to create the `sys` schema and populate it with views and stored procedures for PERFORMANCE_SCHEMA." % e.args

        except Exception, e:
            import traceback
            log_error("Error checking for PS helper: %s\n" % traceback.format_exc())
            return "Unable to access Performance Schema helper (sys)", str(e)+"\n\nIf the sys schema is already installed, make sure you have SELECT privileges on it.\nIf not, you will need privileges to create the `sys` schema and populate it with views and stored procedures for PERFORMANCE_SCHEMA."

        return None


    def ps_usable(self):
        """Checks if performance is enabled."""
        ret_val = False
        try:
            res = self.main_view.editor.executeManagementQuery("select @@performance_schema", 0)
            if res.goToFirstRow():
                value = res.stringFieldValue(0)
                ret_val = value == "1"
        except grt.DBError, e:
            log_error("MySQL error retrieving the performance_schema variable: %s\n" % e)
        
        return ret_val
    
    
    def check_usable(self):
        return None, None

    def page_activated(self):
        if self.warning_panel:
            self.remove(self.warning_panel)
            self.warning_panel = None

        self.page_active = True

        button_data = None
        if not self.ctrl_be.is_sql_connected():
            text = ("There is no connection to the MySQL Server.", "This functionality requires a connection to a MySQL server to work.")
        elif not self.ps_usable():
            text = ("Performance Schema Unavailable", "Performance Schema is either unavailable or disabled on this server.\nYou need a MySQL server version 5.6 or newer, with the performance_schema feature enabled.")
        else:
            text = self.ps_helper_needs_installation()
            if self.installer_panel:
                if self.installer_panel.is_busy:
                    return
                self.remove(self.installer_panel)
                self.installer_panel = None

            if text:
                button_data = ("Install Helper", self.install_helper)

        if not text:
            text, button_data = self.check_usable()

        if text:
            title, text = text
            self.warning_panel = MessageButtonPanel("", title, text, button_data)
            self.add(self.warning_panel, True, True)
            
            if self.content:
                self.content.show(False)
        else:
            self.suspend_layout()
            if not self.ui_created:
                self.create_ui()
                self.ui_created = True
    
            self.content.show(True)
            self.relayout()
            self.resume_layout()


    def install_helper(self):
        self.remove(self.warning_panel)
        self.warning_panel = None

        self.installer_panel = HelperInstallPanel(self, self.main_view.editor)
        self.add(self.installer_panel, True, True)
        self.relayout() # needed b/c of layout bug in Mac

        self.installer_panel.importer.set_password(self.ctrl_be.get_mysql_password())
        self.installer_panel.start()


