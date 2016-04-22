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

import grt
import mforms
import migration

from grt import DBLoginError
from workbench.ui import WizardPage, WizardProgressPage
from workbench.utils import replace_string_parameters
from workbench.db_driver import get_connection_parameters, get_odbc_connection_string, is_odbc_connection
from workbench.log import log_error

def ping_host(hostname):
    import sys
    import subprocess
    try:
        if sys.platform == "win32":
            return subprocess.call(["ping", "-n", "1", "-w", "3000", hostname]) == 0
        elif sys.platform == "darwin":
            return subprocess.call(["ping", "-c", "1", "-t", "3", hostname]) == 0
        else:
            return subprocess.call(["ping", "-c", "1", "-t", "3", hostname]) == 0
    except:
        return False

def request_password(connection, forget_old = False):
    # Get the password:
    for param in connection.driver.parameters:
        if param.paramType in ('keychain', 'password'):
            if param.paramType == 'keychain':
                connection_params = get_connection_parameters(connection, do_not_transform=True)
                storage_key_format = param.paramTypeDetails['storageKeyFormat']
                username, storage_string = replace_string_parameters(storage_key_format, connection_params).split('::', 1)
                accepted, passwd = mforms.Utilities.find_or_ask_for_password('Enter password for user ' + username, storage_string, username, forget_old)
                if accepted:
                    return passwd
            else: # Plain password field
                return connection.parameterValues['password']
            break
    return None


def test_connectivity(connection, error_title):
    # get the hostname and port
    hostname = None
    port = None
    if connection.driver.owner.name == "Mysql":
        if connection.driver.name == "MysqlNative":
            hostname = connection.parameterValues["hostName"]
            port = connection.parameterValues.get("port", 3306)
    else:
        hostname = connection.parameterValues.get("hostName", None)
        port = connection.parameterValues.get("port", None)

    if hostname and port:
        import socket
        # try connecting to the port
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(10) # 10s timeout
        try:
            s.connect((hostname, port))
        except socket.gaierror, (errno, e):
            if errno == 8: # cannot resolve
                mforms.Utilities.show_message(error_title,
                        "Unable to connect to the provided host and port combination.\n\n"+
                        "Could not resolve %s\n" % hostname+
                        "Make sure that the provided hostname or IP address is correct.", "OK", "", "")
                return False
            else:
                mforms.Utilities.show_message(error_title,
                        "Unable to connect to the provided host and port combination.\n\n%s\n\n" % e+
                        "Make sure that:\n"+
                        "- the provided hostname or IP address is correct\n"+
                        "- the database server is running and listening in the provided port number\n"+
                        "- the machine hosting the database server allows external connections to the database port\n"+
                        "- your network connection is properly functioning",
                        "OK", "", "")
                return False
        except socket.timeout, e:
            if ping_host(hostname):
                mforms.Utilities.show_message(error_title,
                        "Timed out connecting to %s:%s, although the host could be pinged.\n\n%s\n\n" % (hostname, port, e)+
                        "Make sure that:\n"+
                        "- the database server is running and listening in the provided port number\n"+
                        "- the machine hosting the database server allows external connections to the database port\n"+
                        "- the provided hostname or IP address is correct",
                        "OK", "", "")
            else:
                # if ping fails, we don't know what it could be, because some servers just disable ICMP ping
                mforms.Utilities.show_message(error_title,
                        "Timed out connecting to %s:%s.\n\n" % (hostname, port)+
                        "Make sure that:\n"+
                        "- the provided hostname or IP address is correct\n"+
                        "- the database server is running and listening in the provided port number\n"+
                        "- the machine hosting the database server allows external connections to the database port\n"+
                        "- your network connection is properly functioning",
                        "OK", "", "")
            return False
        except socket.error, (errno, e):
            if errno == 61: # connection refused
                if ping_host(hostname):
                    mforms.Utilities.show_message(error_title,
                            "Connection refused at %s:%s, although the host could be pinged.\n\n" % (hostname, port)+
                            "Make sure that:\n"+
                            "- the database server is running and listening in the provided port number\n"+
                            "- the machine hosting the database server allows external connections to the database port\n"+
                            "- the provided hostname or IP address is correct",
                            "OK", "", "")
                else:
                    # if ping fails, we don't know what it could be, because some servers just disable ICMP ping
                    mforms.Utilities.show_message(error_title,
                            "Error connecting to %s:%s.\n\n%s\n\n" % (hostname, port, e)+
                            "Make sure that:\n"+
                            "- the provided hostname or IP address is correct\n"+
                            "- the database server is running and listening in the provided port number\n"+
                            "- the machine hosting the database server allows external connections to the database port\n"+
                            "- your network connection is properly functioning",
                            "OK", "", "")
            else:
                if ping_host(hostname):
                    mforms.Utilities.show_message(error_title,
                            "Timed out connecting to the provided host and port combination, although the host could be pinged.\n\n%s\n\n" % e+
                            "Make sure that:\n"+
                            "- the database server is running and listening in the provided port number\n"+
                            "- the machine hosting the database server allows external connections to the database port\n"+
                            "- the provided hostname or IP address is correct",
                            "OK", "", "")
                else:
                    # if ping fails, we don't know what it could be, because some servers just disable ICMP ping
                    mforms.Utilities.show_message(error_title,
                            "Timed out connecting to the provided host and port combination.\n\n%s\n\n" % e+
                            "Make sure that:\n"+
                            "- the provided hostname or IP address is correct\n"+
                            "- the database server is running and listening in the provided port number\n"+
                            "- the machine hosting the database server allows external connections to the database port\n"+
                            "- your network connection is properly functioning",
                            "OK", "", "")
            return False
        s.close()
        return True

    return None


def show_missing_driver_error(e):
    mforms.Utilities.show_error("Connect to ODBC Source",
            "The ODBC driver that was selected for the source connection was not found by the ODBC manager.\n"
            "Ensure that you have the proper ODBC driver installed and retry.\n\n"
            "Error Text: %s" % e, "OK", "", "")
    

class SourceWizardPage(WizardPage):
    def _toggle_store_connection(self):
        if self._store_connection_check.get_active():
            self._store_connection_entry.set_enabled(True)
        else:
            self._store_connection_entry.set_enabled(False)


    def save_connection(self):
        name = self._store_connection_entry.get_string_value()
        if not name:
            if mforms.Utilities.show_message("Store Connection", "A name must be provided for saving the connection.", "OK", "Skip", "") != mforms.ResultOk:
                return False
        
        # check for duplicates
        dup = False
        for con in grt.root.wb.rdbmsMgmt.storedConns:
            if con.name == name:
                dup = True
                break
        for con in grt.root.wb.rdbmsMgmt.otherStoredConns:
            if con.name == name:
                dup = True
                break
        if dup:
            if mforms.Utilities.show_message("Store Connection", "There already is a connection named '%s'. Do you want to replace it?" % name,
                                    "Replace Connection", "Cancel", "") != mforms.ResultOk:
                return False
        
        try:
            self.panel.saveConnectionAs(name)
        except Exception, e:
            mforms.Utilities.show_error("Store Connection", str(e), "OK", "", "")
            return False
        return True


    def test_connection(self, source, caption):
        # check host 1st
        set_status_text = mforms.App.get().set_status_text
        set_status_text("Testing network connectivity to %s Server..." % caption)
        if test_connectivity(source.connection, "Test %s DBMS Connection" % caption) == False:
            set_status_text("%s server could not be contacted" % caption)
            return
        set_status_text("Testing connection to %s DBMS..." % caption)
 
        force_password = False
        attempt = 0
        extra = ""
        is_odbc = is_odbc_connection(source.connection)
        while True:
            try:
                if not source.connect():
                    raise Exception("Could not connect to DBMS")
                source.disconnect()
                set_status_text("%s DBMS connection is OK" % caption)
                mforms.Utilities.show_message("Test %s DBMS Connection" % caption, "Connection succeeded.", "OK", "", "")
                if source.password is None:
                    source.password = "" # connection succeeded with no password, so it must be blank
                break
            except (DBLoginError, SystemError), e:
                if attempt == 0 and "[Driver Manager]" in e.message and "image not found" in e.message:
                    set_status_text("Specified ODBC driver not found")
                    show_missing_driver_error(e)
                    return
                elif attempt > 0:
                    if isinstance(e, DBLoginError) and not force_password:
                        force_password = True
                    else:
                        set_status_text("Could not connect to DBMS")
                        if is_odbc:
                            extra = "\n\nODBC connection string: %s" % get_odbc_connection_string(source.connection, '<your password>')
                        etext = str(e)
                        if etext.startswith("Error(") and ": error calling " in etext:
                            try:
                                etext = eval(etext[7:etext.rfind("):")-1], {}, {})[1]
                            except:
                                pass
                        mforms.Utilities.show_message("Test %s DBMS Connection" % caption, "Could not connect to %s DBMS.\n%s%s" % (caption, etext, extra), "OK", "", "")
                        return

                attempt += 1
                source.password = request_password(source.connection, force_password)
                
                # Avoid asking the password a second time when the user cancels the password request
                if source.password == None:
                    mforms.Utilities.show_error("Test %s DBMS Connection" % caption, "Operation cancelled", "OK", "", "")
                    break
                    
            except migration.NotSupportedError, e:
                mforms.Utilities.show_message('Unsupported Connection Method', e.message, 'OK', '', '')
                return
            except Exception, e:
                log_error("Exception testing connection: %s\n" % e)
                set_status_text("Could not connect to DBMS: %s" % e)
                if is_odbc:
                    extra = "\n\nODBC connection string: %s" % get_odbc_connection_string(source.connection, '<your password>')
                etext = str(e)
                if etext.startswith("Error(") and etext.endswith(")"):
                    etext = eval(etext[6:-1], {}, {})[1]
                mforms.Utilities.show_message("Test %s DBMS Connection" % caption, "Could not connect to the DBMS.\n%s%s" % (etext, extra), "OK", "", "")


class SourceMainView(SourceWizardPage):
    def __init__(self, main):
        SourceWizardPage.__init__(self, main, "Source Selection")

        self.supported_sources_instances = grt.List(grt.OBJECT, grt.classes.db_mgmt_Rdbms.__grtclassname__)
        for rdbms in migration.MigrationPlan.supportedSources():
            self.supported_sources_instances.append(rdbms)
        self.main.add_wizard_page(self, "SourceTarget", "Source Selection")

    def create_ui(self):
        self.back_button.set_enabled(False)

        # Main layout structure
        self.server_instance_box = mforms.newBox(False)
        self.server_instance_box.set_spacing(8)
        instance_label = mforms.newLabel('Source RDBMS Connection Parameters')
        instance_label.set_style(mforms.BoldStyle)
        self.server_instance_box.add(instance_label, False, True)

        # Add the view that corresponds to the selected RDBMS:
        self.panel = grt.classes.ui_db_ConnectPanel()
        self.panel.initializeWithRDBMSSelector(grt.root.wb.rdbmsMgmt, self.supported_sources_instances)
        if not self.panel.view:
            raise Exception("NO PANEL!!!")
        view = mforms.fromgrt(self.panel.view)
        self.server_instance_box.add(view, True, True)
        
        box = mforms.newBox(True)
        self._store_connection_check = mforms.newCheckBox()
        self._store_connection_check.set_text("Store connection for future usage as ")
        self._store_connection_check.add_clicked_callback(self._toggle_store_connection)
        self._store_connection_check.set_size(270, -1)
        box.add(self._store_connection_check, False, True)
        self._store_connection_entry = mforms.newTextEntry()
        box.add(self._store_connection_entry, True, True)
        self._store_connection_entry.set_enabled(False)

        self.server_instance_box.add(box, False, True)
        self.content.add(self.server_instance_box, True, True)

        self.advanced_button.set_text("Test Connection")

        self.odbc_button = mforms.newButton()
        self.odbc_button.set_text("Open ODBC Administrator")
        self.odbc_button.add_clicked_callback(self.open_odbc)
        self.button_box.add(self.odbc_button, False, True)



    def go_advanced(self):
        self.main.plan.setSourceConnection(self.panel.connection)
        self.test_connection(self.main.plan.migrationSource, "Source")

    def go_next(self):
        if self._store_connection_check.get_active():
            if not self.save_connection():
                return
            self._store_connection_check.set_active(False)

        self.main.plan.setSourceConnection(self.panel.connection)

        SourceWizardPage.go_next(self)

    def open_odbc(self):
        if not grt.modules.PyWbUtils.startODBCAdmin():
            mforms.Utilities.show_error("Open ODBC Administrator", "ODBC Administrator utility could not be found.", "OK", "", "")



class TargetMainView(SourceWizardPage):
    def __init__(self, main):
        SourceWizardPage.__init__(self, main, "Target Selection")

        self.main.add_wizard_page(self, "SourceTarget", "Target Selection")


    def just_script_toggled(self):
        pass

    def create_ui(self):
        # Main layout structure
        self.server_instance_box = mforms.newBox(False)
        self.server_instance_box.set_spacing(8)
        instance_label = mforms.newLabel('Target RDBMS Connection Parameters')
        instance_label.set_style(mforms.BoldStyle)
        self.server_instance_box.add(instance_label, False, True)

        # TODO: Enable the export to script option in future versions:
#        self.just_script_choice = mforms.newCheckBox()
#        self.just_script_choice.set_text('Do not use a live instance (SQL script output)')
#        self.just_script_choice.set_tooltip('Check this if you just want an output script to execute it later')
#        self.just_script_choice.add_clicked_callback(self.just_script_toggled)
#        self.server_instance_box.add(self.just_script_choice, False)

        # Add the view that corresponds to the selected RDBMS:
        self.panel = grt.classes.ui_db_ConnectPanel()
        self.panel.initialize(grt.root.wb.rdbmsMgmt)
        view = mforms.fromgrt(self.panel.view)
        self.server_instance_box.add(view, True, True)

        box = mforms.newBox(True)
        self._store_connection_check = mforms.newCheckBox()
        self._store_connection_check.set_text("Store connection for future usage as ")
        self._store_connection_check.add_clicked_callback(self._toggle_store_connection)
        box.add(self._store_connection_check, False, True)
        self._store_connection_entry = mforms.newTextEntry()
        box.add(self._store_connection_entry, True, True)
        self._store_connection_entry.set_enabled(False)

        self.server_instance_box.add(box, False, True)
        self.content.add(self.server_instance_box, True, True)

        self.advanced_button.set_text("Test Connection")

    def go_next(self):
        if self._store_connection_check.get_active():
            if not self.save_connection():
                return
            self._store_connection_check.set_active(False)

        self.main.plan.setTargetConnection(self.panel.connection)
        
        SourceWizardPage.go_next(self)

    def go_advanced(self):
        self.main.plan.setTargetConnection(self.panel.connection)
        self.test_connection(self.main.plan.migrationTarget, "Target")


PAGE_DESCRIPTION="""The following tasks will now be performed. Please monitor the execution.

The names of available schemas will be retrieved from the source RDBMS. The account used for 
the connection will need to have appropriate privileges for listing and reading the schemas you 
want to migrate. Target RDBMS connection settings will also be checked for validity.
"""

class FetchProgressView(WizardProgressPage):
    def __init__(self, main):
        WizardProgressPage.__init__(self, main, "Fetch Schema List", description=PAGE_DESCRIPTION)
        
        self._autostart = True
        self.add_task(self.task_connect, "Connect to source DBMS")
        self.add_task(self.task_test_target, "Check target DBMS connection")
        self.add_threaded_task(self.task_fetch_schemata, "Retrieve schema list from source")

        self.main.add_wizard_page(self, "SourceTarget", "Fetch Schemas List")


    def format_exception_text(self, e):
        return str(e).replace(";", ";\n")

 
    def task_connect(self):
        grt.send_progress(-1, "Connecting to source...")
        force_password = False
        attempt = 0
        while True:
            try:
                if not self.main.plan.migrationSource.connect():
                    raise Exception("Could not connect to source RDBMS")
                if self.main.plan.migrationSource.password is None: # no password and succeeded, assume blank
                    self.main.plan.migrationSource.password = ""
                self.main.plan.migrationSource.checkVersion()
                break
            except (DBLoginError, SystemError), e:
                if attempt == 0:
                    if "[Driver Manager]" in e.message and "image not found" in e.message:
                        show_missing_driver_error(e)
                        return
                if attempt > 0:
                    if isinstance(e, DBLoginError) and not force_password:
                        force_password = True
                    else:
                        #if mforms.Utilities.show_error("Connect to Source RDBMS", str(e), "Retry", "Cancel", "") != mforms.ResultOk:
                        raise e
                attempt += 1
                self.main.plan.migrationSource.password = request_password(self.main.plan.migrationSource.connection, force_password)

    def go_back(self):
        self.reset(True)
        WizardProgressPage.go_back(self)


    def task_fetch_schemata(self):
        connection = self.main.plan.migrationSource.connection
        only_these_catalogs = ( [connection.parameterValues['schema']] if (connection.parameterValues.has_key('schema') and connection.parameterValues['schema'])
                                                                       else []  )
        self.main.plan.migrationSource.doFetchSchemaNames(only_these_catalogs)
        self.main.plan.migrationSource.disconnect()


    def task_test_target(self):
        grt.send_progress(-1, "Connecting to target...")
        attempt = 0
        if self.main.plan.migrationTarget.connection.hostIdentifier == self.main.plan.migrationSource.connection.hostIdentifier:
            if self.main.plan.migrationTarget.connection.parameterValues['userName'] == self.main.plan.migrationSource.connection.parameterValues['userName']:
                self.main.plan.migrationTarget.password = self.main.plan.migrationSource.password
        force_password = False
        while True:
            try:
                if not self.main.plan.migrationTarget.checkConnection():
                    raise Exception("Could not connect to target RDBMS")
                if self.main.plan.migrationTarget.password is None: # no password and succeeded, assume blank
                    self.main.plan.migrationTarget.password = ""
                self.main.plan.migrationTarget.checkVersion()
                break
            except (DBLoginError, SystemError), e:
                if attempt > 0:
                    if isinstance(e, DBLoginError) and not force_password:
                        force_password = True
                    else:
                        raise e
                attempt += 1
                self.main.plan.migrationTarget.password = request_password(self.main.plan.migrationTarget.connection)
