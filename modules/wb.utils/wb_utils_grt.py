# Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

import re
import sys
import subprocess
import os
import threading
import zipfile
import tempfile
import shlex
from workbench.utils import get_exe_path

# import the wb module
from wb import DefineModule, wbinputs
# import the grt module
import grt
import mforms

from grt import log_warning
from workbench.log import log_info, log_error, log_debug2
import traceback

from workbench.ui import WizardForm, WizardPage
from mforms import newButton, newCheckBox

# define this Python module as a GRT module
ModuleInfo = DefineModule(name= "PyWbUtils", author= "Sun Microsystems Inc.", version="1.0")


# wb_model_utils contains a few more plugins that will get registered when imported
import wb_model_utils # noqa
import wb_catalog_utils # noqa

def get_linux_terminal_program():
    paths = os.getenv("PATH").split(":")
    if not paths:
        paths = ['/usr/bin', '/usr/local/bin', '/bin']

    for term in ["gnome-terminal", "konsole", "xterm", "rxvt"]:
        for d in paths:
            full_path = os.path.join(d, term)
            if os.path.exists(full_path):
                return full_path

    return None


@ModuleInfo.plugin('wb.tools.backupConnections', caption='Backup existing connections')
@ModuleInfo.export(grt.INT)
def backupConnections():
    user_data_dir = mforms.App.get().get_user_data_folder()
    connections_path = os.path.join(user_data_dir, 'connections.xml')
    instances_path = os.path.join(user_data_dir, 'server_instances.xml')
 
    file_chooser = mforms.newFileChooser(mforms.Form.main_form(), mforms.SaveFile)
    file_chooser.set_title('Export Connections As')
    file_chooser.set_extensions('ZIP Files (*.zip)|*.zip', 'import')
    if file_chooser.run_modal() == mforms.ResultOk:
        backup_path = file_chooser.get_path()
        if isinstance(backup_path, unicode):
            backup_path = backup_path.encode('utf-8')
        try:
            backup_file = zipfile.ZipFile(backup_path, 'w', zipfile.ZIP_DEFLATED)
        except Exception:
            mforms.Utilities.show_error('Backup file creation error',
                                       'Could not create the backup file. Please check path and permissions and try '
                                       'again.', 'OK', '', '')
            return 1

        backup_file.write(connections_path, 'connections.xml')
        backup_file.write(instances_path, 'server_instances.xml')

        if mforms.Utilities.show_message('Connections saved', 'Your connections were successfully backed up to '
                                      + backup_path,
                                      'OK', '', 'Show File') == mforms.ResultOther:
            mforms.Utilities.reveal_file(backup_path)
    return 0


@ModuleInfo.plugin('wb.tools.restoreConnections', caption='Restore connections from a backup file')
@ModuleInfo.export(grt.INT)
def restoreConnections():
    def generate_unique_name(name, name_set):
        new_name = name
        idx = 1
        while True:
            if not new_name in name_set:
                return new_name
            new_name = name + ' (%d)' % idx
            idx += 1

    file_chooser = mforms.newFileChooser(mforms.Form.main_form(), mforms.OpenFile)
    file_chooser.set_title('Select a Connections Backup File')
    file_chooser.set_extensions('ZIP Files (*.zip)|*.zip', 'import')
    if file_chooser.run_modal():
        backup_path = file_chooser.get_path()
        try:
            backup_file = zipfile.ZipFile(backup_path, 'r')
            try:
                instances_file = tempfile.NamedTemporaryFile(delete=False)
                instances_file.write(backup_file.read('server_instances.xml'))
                instances_file.close()

                connections_file = tempfile.NamedTemporaryFile(delete=False)
                connections_file.write(backup_file.read('connections.xml'))
                connections_file.close()
            except KeyError, error:
                mforms.Utilities.show_error('Restore Connections Error', 'The selected file is not a valid backup file '
                                            'or the file is corrupted: %s.' % error.message,
                                            'OK', '', '')
                grt.log_error('restoreConnections', 'The selected file is not a valid backup file '
                              'or the file is corrupted: %s.' % error.message)
                return

            connections = grt.unserialize(connections_file.name)

            if not isinstance(connections, grt.List):
                mforms.Utilities.show_error('Restore Connections Error', 'The selected file is not a valid backup file '
                                            'or the file is corrupted.',
                                            'OK', '', '')
                grt.log_error('restoreConnections', 'The selected archive does not have a valid connection backup file.\n')
                return

            inserted_connections = {}
            existent_connection_names = set(conn.name for conn in grt.root.wb.rdbmsMgmt.storedConns)
            existent_connection_ids = set(conn.__id__ for conn in grt.root.wb.rdbmsMgmt.storedConns)
            duplicate_connection_count = 0
            for candidate_connection in connections:
                if candidate_connection.__id__ in existent_connection_ids:
                    duplicate_connection_count = duplicate_connection_count + 1
                    continue
                    
                candidate_connection.name = generate_unique_name(candidate_connection.name, existent_connection_names)

                existent_connection_names.add(candidate_connection.name)
                candidate_connection.owner = grt.root.wb.rdbmsMgmt
                inserted_connections[candidate_connection.__id__] = candidate_connection
                grt.root.wb.rdbmsMgmt.storedConns.append(candidate_connection)

            instances   = grt.unserialize(instances_file.name)
            
            if not isinstance(instances, grt.List):
                mforms.Utilities.show_error('Restore Connections Error', 'The selected file is not a valid backup file '
                                            'or the file is corrupted.',
                                            'OK', '', '')
                grt.log_error('restoreConnections', 'Workbench restored %i valid connections but server configuration data could not be found or is not valid.\n' % len(connections))
                return
     
            existent_instance_names = set(instance.name for instance in grt.root.wb.rdbmsMgmt.storedInstances)
            previous_instances_conns = set()
            duplicated_instance_count = 0
            for candidate_instance in instances:
                if candidate_instance.connection.__id__ in previous_instances_conns:
                    duplicated_instance_count = duplicated_instance_count + 1
                    continue  # Skip instances whose connections are associated to previously processed instances
                previous_instances_conns.add(candidate_instance.connection.__id__)
                candidate_instance.name = generate_unique_name(candidate_instance.name, existent_instance_names)

                existent_instance_names.add(candidate_instance.name)
                new_conn = inserted_connections.get(candidate_instance.connection.__id__, None)
                candidate_instance = candidate_instance.shallow_copy()
                candidate_instance.connection = new_conn
                grt.root.wb.rdbmsMgmt.storedInstances.append(candidate_instance)

            grt.modules.Workbench.refreshHomeConnections()
            grt.modules.Workbench.saveConnections()
            grt.modules.Workbench.saveInstances()
            
            if duplicate_connection_count > 0 or duplicated_instance_count > 0:
                message = []
                message.append('Workbench detected ')
                if duplicate_connection_count > 0:
                    message.append('%i duplicated connections' % duplicate_connection_count)
                if duplicated_instance_count > 0:
                    if duplicate_connection_count > 0:
                        message.append(' and ')
                    message.append('%i duplicated instances' % duplicated_instance_count)
                message.append(', which were not restored.')
                mforms.Utilities.show_warning('Restore Connections', ''.join(message), 'OK', '', '')
            
        except zipfile.BadZipfile, error:
            mforms.Utilities.show_error('Restore Connections Error', 'The selected file is not a valid backup file '
                                        'or the file is corrupted.',
                                        'OK', '', '')
            grt.log_error('restoreConnections', 'The selected file is not a valid backup file or the file is corrupted: %s\n' % error)
        except IOError, error:
            mforms.Utilities.show_error('Restore Connections Error', 'Cannot read from file. Please check this file '
                                        'permissions and try again.',
                                        'OK', '', '')
            grt.log_error('restoreConnections', '%s\n' % str(error))
    return 0


@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def connectionStringFromConnection(conn):
    #<user>[:<password>]@<host>[:<port>][:<socket>]
    connstr = ""
    if conn.driver.name == "MysqlNative":
        connstr = "%s@%s:%s" % (conn.parameterValues["userName"], conn.parameterValues["hostName"], conn.parameterValues["port"])
    elif conn.driver.name == "MysqlNativeSocket":
        connstr = "%s@%s::%s" % (conn.parameterValues["userName"], conn.parameterValues["hostName"], conn.parameterValues["socket"])
    elif conn.driver.name == "MysqlNativeSSH":
        #XXX this is incomplete, need some way to encode the ssh params
        connstr = "%s@%s::%s" % (conn.parameterValues["userName"], conn.parameterValues["hostName"], conn.parameterValues["port"])
    return connstr


@ModuleInfo.export(grt.classes.db_mgmt_Connection, grt.STRING)
def connectionFromString(connstr):
    valid = False
    
    def get_driver(name):
        for d in grt.root.wb.rdbmsMgmt.rdbms[0].drivers:
            if d.name == name:
                return d
        return None
    
    # parse as a one of our connection strings
    g = re.match("(.*?)(?::(.*))?@(.*?)(?::([0-9]+|)(?::(.+|))?)?$", connstr)
    if g:
        user, password, host, port, socket = g.groups()
        valid = True
    else:
        user, password, host, port, socket = None, None, None, None, None
        # check if this is a mysql cmdline client command
        tokens = shlex.split(connstr.strip())
        if tokens:
            if tokens[0].endswith("mysql") or tokens[0].endswith("mysql.exe"):
                i = 1
                valid = True
                while i < len(tokens):
                    if tokens[i] == "-u":
                        i += 1
                        user = tokens[i]
                    elif tokens[i].startswith("-u"):
                        user = tokens[i][2:]
                    elif tokens[i] == "-h":
                        i += 1
                        host = tokens[i]
                    elif tokens[i].startswith("-h"):
                        host = tokens[i][2:]
                    elif tokens[i] == "-p":
                        i += 1
                        password = tokens[i]
                    elif tokens[i].startswith("-p"):
                        password = tokens[i][2:] # noqa
                    elif tokens[i] == "-P":
                        i += 1
                        port = tokens[i]
                    elif tokens[i].startswith("-P"):
                        port = tokens[i][2:]
                    elif tokens[i] == "-S":
                        i += 1
                        socket = tokens[i]
                    elif tokens[i].startswith("-S"):
                        socket = tokens[i][2:]
                    i += 1
    if valid:
        if port:
            try:
                port = int(port)
            except:
                log_warning("wb_utils", "Error parsing connstring; port value '%s' should be a number\n" % port)
                port = None
        if not port:
            port = 3306
        conn = grt.classes.db_mgmt_Connection()
        conn.owner = grt.root.wb.rdbmsMgmt
        conn.name = connstr
        if socket:
            conn.driver = get_driver("MysqlNativeSocket")
        else:
            conn.driver = get_driver("MysqlNative")

        if user:
            conn.parameterValues["userName"] = user
        if host:
            conn.parameterValues["hostName"] = host
        if port:
            conn.parameterValues["port"] = port
        if socket:
            conn.parameterValues["socket"] = socket

        hostIdentifier = conn.driver.hostIdentifierTemplate
        for key, value in conn.parameterValues.items():
            hostIdentifier = hostIdentifier.replace("%"+key+"%", str(value))
        conn.hostIdentifier = hostIdentifier

        return conn

    return None


@ModuleInfo.plugin("wb.tools.copyConnectionString", caption="Copy Connection String to Clipboard", input= [wbinputs.selectedConnection()], pluginMenu="Home/Connections")
@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def copyConnectionString(conn):
    connstr = connectionStringFromConnection(conn)
    mforms.Utilities.set_clipboard_text(connstr)


@ModuleInfo.plugin("wb.tools.copyJDBCConnectionString", caption="Copy JDBC Connection String to Clipboard", input= [wbinputs.selectedConnection()], pluginMenu="Home/Connections")
@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def copyJDBCConnectionString(conn):
    if conn.parameterValues.has_key("schema"):
        params = "/"+conn.parameterValues["schema"]
    else:
        params = "/"
    params += "?user=%s" % conn.parameterValues["userName"]

    if conn.driver.name == "MysqlNative":
        connstr = "jdbc:mysql://%s:%s" % (conn.parameterValues["hostName"], conn.parameterValues["port"])
    elif conn.driver.name == "MysqlNativeSocket":
        connstr = "jdbc:mysql://%s:%s" % (conn.parameterValues["hostName"], conn.parameterValues["socket"])
    elif conn.driver.name == "MysqlNativeSSH":
        mforms.Utilities.show_error("Copy JDBC Connection String",
            "Cannot create JDBC connection string for %s. The connection uses a SSH tunnel." % conn.name,
            "OK", "", "")
        return
    mforms.Utilities.set_clipboard_text(connstr+params)


@ModuleInfo.plugin("wb.tools.createMissingLocalConnections", caption="Rescan for Local MySQL Instances", input= [], pluginMenu="Home/Connections")
@ModuleInfo.export(grt.INT)
def createMissingLocalConnections():

    found_instances = grt.modules.Workbench.createInstancesFromLocalServers()
    
    grt.modules.Workbench.refreshHomeConnections()
    
    if found_instances < 0:
        mforms.Utilities.show_error("Rescan for Local MySQL Servers", "Rescan for local MySQL servers failed", "OK", "", "")
    elif found_instances == 0:
        mforms.Utilities.show_message('Rescan for Local MySQL Servers', 'No servers were found.', 'OK', '', '')
    else:
        mforms.Utilities.show_message('Rescan for Local MySQL Servers', 'Found %s servers.' % found_instances, 'OK', '', '')
    
    return 1

@ModuleInfo.plugin("wb.tools.connectionFromClipboard", caption="Add Connection(s) from Clipboard", input= [], pluginMenu="Home/Connections")
@ModuleInfo.export(grt.INT)
def newConnectionFromClipboard():
    text = mforms.Utilities.get_clipboard_text()
    if not text:
        return 0

    existing = set()
    # make a normalized set of the connections that already exist
    for con in grt.root.wb.rdbmsMgmt.storedConns:
        existing.add(connectionStringFromConnection(con))

    parse_errors = False
    found_instances = 0
    for line in text.encode("utf8").split("\n"):
        conn = connectionFromString(line)
        if not conn and not parse_errors:
            mforms.Utilities.show_error("Add Connection(s) from Clipboard", "Could not parse connection parameters from string '%s'" % line, "OK", "", "")
            parse_errors = True
            continue
        
        if connectionStringFromConnection(conn) in existing:
            mforms.Utilities.show_error("Add Connection(s) from Clipboard", "The connection %s already exists and was not added." % line,
                                        "OK", "", "")
            continue
        i = 1
        name = conn.parameterValues.get("hostName", "local")
        prefix = name
        while any(conn.name == name for conn in grt.root.wb.rdbmsMgmt.storedConns):
            name = "%s (%i)" % (prefix, i)
            i += 1
        conn.name = name
        log_info("Added connection %s from clipboard\n" % conn.name)
        found_instances = found_instances + 1
        grt.root.wb.rdbmsMgmt.storedConns.append(conn)

    grt.modules.Workbench.refreshHomeConnections()

    if found_instances > 0:
        mforms.Utilities.show_message('Add Connection(s) from Clipboard', 'Found %s servers.' % found_instances, 'OK', '', '')

    return 1


@ModuleInfo.plugin("wb.tools.cmdlineClient", caption="Start Command Line Client", input= [wbinputs.selectedConnection()], pluginMenu="Home/Connections")
@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def startCommandLineClientForConnection(conn):
    import platform
    import os
    if "ssh" in conn.driver.name.lower():
        host = "127.0.0.1"
        tun = grt.modules.DbMySQLQuery.openTunnel(conn)
        if tun < 0:
            mforms.Utilities.show_error("Start Command Line Client", "Could not open SSH tunnel to host.", "OK", "", "")
            return
        port = grt.modules.DbMySQLQuery.getTunnelPort(tun)
        socket = ""
    elif "socket" in conn.driver.name.lower():
        if platform.system() == "Windows":
            host = "."
        else:
            host = "localhost"
        port = None
        socketName = conn.parameterValues["socket"]
        if socketName is None:
            socketName = "MySQL"
        socket = "--socket=" + socketName
    else:
        host = conn.parameterValues["hostName"].replace("\\", "\\\\").replace('"', '\\"')
        port = conn.parameterValues["port"]
        socket = ""

    user = conn.parameterValues["userName"].replace("\\", "\\\\").replace('"', '\\"')
    if port is None:
        port = 3306
    schema = conn.parameterValues["schema"]
    if schema:
        schema = schema.replace("\\", "\\\\").replace('"', '\\"')
    else:
        schema = ""

    bundled_client_path = grt.root.wb.options.options.get("mysqlclient", None)
    if platform.system().lower() == "darwin":
        if not bundled_client_path:
            bundled_client_path = mforms.App.get().get_executable_path("mysql")
        command = """\\"%s\\" \\"-u%s\\" \\"-h%s\\" -P%i %s -p %s""" % (os.path.expanduser(bundled_client_path), user, host, port, socket, schema)
        os.system("""osascript -e 'tell application "Terminal" to do script "%s"'""" % command)
    elif platform.system().lower() == "windows":
        if not bundled_client_path:
            bundled_client_path = mforms.App.get().get_executable_path("mysql.exe")

        # call mysql client, and if it exits with error, pause so that the user can see what went wrong, before closing the window
        # (the ideal way would have been to do what we do for Linux, but Windows shell is too limited)
        command = """start cmd /C "%s -u%s -h%s -P%i %s -p %s || pause" """ % (bundled_client_path.replace(" ", "\\ "), user, host, port, socket, schema)
        subprocess.Popen(command, shell = True)
    else:
        if not bundled_client_path:
            bundled_client_path = mforms.App.get().get_executable_path("mysql")
        if not bundled_client_path:
            bundled_client_path = "mysql"
        command = """\\"%s\\" \\"-u%s\\" \\"-h%s\\" -P%i %s -p %s""" % (bundled_client_path, user, host, port, socket, schema)

        # call mysql client in a loop until either: 1. it exits with no error, or 2. user exits with Ctrl+C.
        # This is necessary because if the user enters wrong password, the window closes too quick for the user to see what's going on.
        subprocess.call([ "/bin/sh", "-c",  # <--- having extra /bin/sh wrapper allows GUI terminal to be launched in background (thus run parallel with workbench)
            get_linux_terminal_program() + " -e '" +
                "sh -c \"while :; do %s && break || read -p \\\"Press Enter to retry or Ctrl+C to quit\\\" DUMMY_VAR; done\" " % command
            + "' &"   # <--- launch GUI terminal and exit (returns to Workbench immediately rather than blocking)
        ])
        
        my_env = os.environ.copy()
        if (("XDG_SESSION_TYPE" in my_env and my_env["XDG_SESSION_TYPE"] == "wayland") 
          or "WAYLAND_DISPLAY" in my_env) and my_env["GDK_BACKEND"]:
            my_env["GDK_BACKEND"] = "wayland"
        subprocess.Popen(["/bin/sh", "-c",
                          get_linux_terminal_program() + " -e '" +
                "sh -c \"while :; do %s && break || read -p \\\"Press Enter to retry or Ctrl+C to quit\\\" DUMMY_VAR; done\" " % command
                + "' &"   # <--- launch GUI terminal and exit (returns to Workbench immediately rather than blocking) 
        ], shell=False, env=my_env)

if sys.platform == "linux2":
    @ModuleInfo.export(grt.INT)
    def startODBCAdmin():
        path = get_exe_path('iodbcadm-gtk')
        if not path:
            path = get_exe_path('ODBCManageDataSourcesQ4')

        if path:
            subprocess.Popen(path, shell=True, close_fds=True)
            return 1
        else:
            return 0
elif sys.platform == "darwin":
    @ModuleInfo.export(grt.INT)
    def startODBCAdmin():
        ret = subprocess.call("open -a 'ODBC Administrator'", shell=True)
        if ret == 1:
            ret = subprocess.call("open -a 'ODBC Manager'", shell=True)
        return 0 if ret == 1 else 1
elif sys.platform == "win32":
    @ModuleInfo.export(grt.INT)
    def startODBCAdmin():
        # WTF alert:
        # In 64bit windows, there are 2 versions of odbcad32.exe. One is 64bits and the other 32.
        # The 64bit version is in \Windows\System32
        # The 32bit version is in \Windows\SysWOW64

        # so if we're a 64bit WB, then we run the 64bit odbc tool (since we can't use 32bit drivers anyway)

        if sys.maxint > 2**31:
            subprocess.Popen(r"%SYSTEMROOT%\SysWOW64\odbcad32.exe", shell=True, creationflags=subprocess.CREATE_NEW_PROCESS_GROUP, close_fds=True)
        else:
            subprocess.Popen(r"%SYSTEMROOT%\System32\odbcad32.exe", shell=True, creationflags=subprocess.CREATE_NEW_PROCESS_GROUP, close_fds=True)
            return 1


def process_not_found_utils():
    utilities_url = ("http://dev.mysql.com/downloads/utilities/" if grt.root.wb.info.edition == "Community" else
                        "https://edelivery.oracle.com/EPD/Search/get_form?product=18251")

    source_description = "www.mysql.com" if grt.root.wb.info.edition == "Community" else "eDelivery"

    if mforms.Utilities.show_message("MySQL Utilities", "The command line MySQL Utilities could not be "
                                        "located.\n\nTo use them, you must download and install the utilities "
                                        "package for your system from %s.\n\n"
                                        "Click on the Download button to proceed." % source_description,
                                "Download...", "Cancel", "") == mforms.ResultOk:

        mforms.Utilities.open_url(utilities_url)

@ModuleInfo.plugin("wb.tools.utilitiesShell", caption="Start Shell for MySQL Utilities", groups=["Others/Menu/Ungrouped"])
@ModuleInfo.export(grt.INT)
def startUtilitiesShell():
    import platform
    import os

    if platform.system() == "Windows":
        guessed_path = None
        for varname in ["ProgramFiles(x86)", "ProgramFiles"]:
            if not os.getenv(varname):
                continue
            path = os.path.join(os.getenv(varname), "MySQL", "MySQL Utilities", "mysqluc.exe")
            if os.path.exists(path):
                guessed_path = path
                break
        if any(os.path.exists(os.path.join(f, "mysqluc.exe")) for f in os.getenv("PATH").split(";")):
            # Utils path is in PATH already
            command = r'start cmd /K "mysqluc"'
            subprocess.Popen(command, shell = True)
        elif guessed_path:
            command = r'start cmd /K "%s"' % guessed_path
            subprocess.Popen(command, shell = True)
        else:
            process_not_found_utils()

    elif platform.system() == "Darwin":
        # PATH seems to be stripped down when WB is started from a binary .app
        if any(os.path.exists(f+"/mysqluc") for f in os.getenv("PATH").split(":") + ["/usr/local/bin"]):
            os.system(r"""osascript -e 'tell application "Terminal" to do script "mysqluc -e \"help utilities\""' -e 'tell front window of application "Terminal" to set custom title to "MySQL Utilities"'""")
        else:
            process_not_found_utils()
    else:
        if not any(os.path.exists(f+"/mysqluc") for f in os.getenv("PATH").split(":")):
            process_not_found_utils()
        else:
            term = get_linux_terminal_program()
            if term:
                import tempfile
                fd, setup_script = tempfile.mkstemp(prefix="delme.", dir=mforms.App.get().get_user_data_folder())
                f = os.fdopen(fd, "w+")
                f.write('echo "The following MySQL Utilities are available:"\n')
                f.write('mysqluc -e "help utilities"\n')
                f.write('rm -f "%s"\n' % setup_script)
                f.write('sh -i\n')
                f.close()
                os.chmod(setup_script, 0700)

                if 'konsole' in term:
                    subprocess.call([term, "-e", "/bin/sh", setup_script])
                else:
                    my_env = os.environ.copy()
                    if (("XDG_SESSION_TYPE" in my_env and my_env["XDG_SESSION_TYPE"] == "wayland") 
                      or "WAYLAND_DISPLAY" in my_env) and my_env["GDK_BACKEND"]:
                        my_env["GDK_BACKEND"] = "wayland"
                    subprocess.Popen(["/bin/sh", "-c", "%s -e %s &" % (term, setup_script)], shell=False, env=my_env)
            else:
                raise RuntimeError("Terminal program could not be found")


class CheckForUpdateThread(threading.Thread):
    def __init__(self):
        self.is_running = False
        self.finished = False
        super(CheckForUpdateThread, self).__init__()
    
    def run(self):
        if self.is_running:
            return
        
        self.is_running = True
        try:
            import urllib2
            import json
            self.json = json.load(urllib2.urlopen("http://workbench.mysql.com/current-release")) 
        except Exception, error:

            self.json = None
            self.error = "%s\n\nPlease verify your internet connection is available." % str(error)        
    
    def checkForUpdatesCallback(self):
        if self.isAlive():
            return True  # Don't do anything until the dom is built
        
        if not self.json:
            if hasattr(self, 'error'):
                mforms.Utilities.show_error("Check for updates failed", str(self.error), "OK", "", "")
        else:
            try:
                current_version = (grt.root.wb.info.version.majorNumber, grt.root.wb.info.version.minorNumber, grt.root.wb.info.version.releaseNumber)
                newest_version = tuple(int(i) for i in self.json['fullversion'].split("."))

                if newest_version > current_version:
                    if mforms.Utilities.show_message('New Version Available', 'The new MySQL Workbench %s has been released.\nYou can download the latest version from\nhttp://www.mysql.com/downloads/workbench.' % '.'.join( [str(num) for num in newest_version] ),
                                                  'Get it Now', 'Maybe Later', "") == mforms.ResultOk:
                        mforms.Utilities.open_url('http://www.mysql.com/downloads/workbench')
                else:
                    mforms.Utilities.show_message('MySQL Workbench is Up to Date', 'You are already using the latest version of MySQL Workbench.', 'OK', '', '')
        
            except Exception, error:
                mforms.Utilities.show_error("Check for updates failed", str(error), "OK", "", "")

        mforms.App.get().set_status_text('Ready.')
        self.is_running = False
        self.finished = True
        return False


# Global variable:
thread = CheckForUpdateThread()

@ModuleInfo.plugin("wb.tools.checkForUpdates", caption="Check for Updates")
@ModuleInfo.export(grt.INT)
def checkForUpdates():
    global thread
    
    if thread.is_running:
        return 0
    
    if thread.finished:
        thread = CheckForUpdateThread()
    thread.start()
    mforms.App.get().set_status_text('Checking for updates...')
    ignore = mforms.Utilities.add_timeout(1.0, thread.checkForUpdatesCallback) # noqa



class SSLWizard_GenerationTask:
    def __init__(self, main, path):
        self.main = main
        self.path = path
        self.config_file = {}

    def display_error(self, title, message):
        log_error("%s\n%s\n" % (title, message))
        mforms.Utilities.show_error(title, message, "OK", "", "")
    
    def verify_preconditions(self):
        try:
            if not os.path.exists(self.main.certificates_root) or not os.path.isdir(self.main.certificates_root):
                log_info("Creating certificates toor directory[%s]" % self.main.certificates_root)
                os.mkdir(self.main.certificates_root, 0700)
            
            if os.path.exists(self.path) and not os.path.isdir(self.path):
                self.display_error("Checking requirements", "The selected path is a file. You should select a directory.")
                return False
            if not os.path.exists(self.path):
                if mforms.Utilities.show_message("Create directory", "The directory you selected does not exists. Do you want to create it?", "Create", "Cancel", "") == mforms.ResultCancel:
                    self.display_error("Create directory", "The operation was canceled.")
                    return False
                os.mkdir(self.path, 0700)
                
            return True
        except OSError, e:
            self.display_error("Create directory", "There was an error (%d) - %s\n%s" % (e.errno, str(e), str(traceback.format_exc())))
            if e.errno == 17:
                return True
                #raise
            return False
      
    def generate_config_file(self, target):
        self.config_file[target] = os.path.join(self.path, "attribs-%s.txt" % target)
        f = open(self.config_file[target], "w+")
        f.write("[req]\ndistinguished_name=distinguished_name\nprompt=no\n")
        f.write("\n".join(["[distinguished_name]"] + self.main.generate_page.get_attributes(target))+"\n")
        f.close()
      

    def generate_certificate(self, tool, out_path, out_name, ca_cert, ca_key, config_file, days=3600):
        key = os.path.join(out_path, out_name+"-key.pem")
        req = os.path.join(out_path, out_name+"-req.pem")
        cert = os.path.join(out_path, out_name+"-cert.pem")

        serial_file = os.path.join(out_path, out_name+".serial")

        req_cmd = [tool, "req", "-newkey", "rsa:2048", "-days", str(days), "-nodes", "-keyout", key, "-out", req, "-config", config_file]
        if not self.run_command(req_cmd):
            log_error("Unable to generate key.\n")
            return False, key, req, cert

        rsa_cmd = [tool, "rsa", "-in", key, "-out", key]
        if not self.run_command(rsa_cmd):
            log_error("Unable to generate key.\n")
            return False, key, req, cert

        rsa_cmd = [tool, "x509", "-req", "-in", req, "-days", str(days), "-CA", ca_cert, "-CAkey", ca_key,
                         "-CAserial", serial_file, "-CAcreateserial",
                         "-out", cert]
        if not self.run_command(rsa_cmd):
            log_error("Unable to generate certificate serial.\n")
            return False, key, req, cert

        return True, key, req, cert

    def run_command(self, command, output_to = subprocess.PIPE):

        try:
            set_shell = True if sys.platform == "win32" else False
            p = subprocess.Popen(command, stdout=output_to, stderr=subprocess.PIPE, shell=set_shell)
            out = p.communicate()

            if p.returncode != 0:
                log_error("Running command: %s\nOutput(retcode: %d):\n%s\n" % (str(command), p.returncode, str(out)))
                return False

            return True
        except ValueError, e:
            log_error("Running command: %s\nValueError exception\n" % (str(e.cmd)))
            return False
        except OSError, e:
            log_error("Running command: %s\nException:\n%s\n" % (str(command), str(e)))
            return False

    def generate(self, path, config_file):
        days = 3600

        tool = "openssl"
        ca_key = os.path.join(path, "ca-key.pem")
        ca_cert = os.path.join(path, "ca-cert.pem")
        
        # Check if the tool exists
        log_debug2("Checking tool availability(%s)\n" % tool)
        if not self.run_command([tool, "version"]):
            self.display_error("Checking requirements", "The SSL tool (%s) is not available. Please verify if it's installed and the installation directory is in the PATH environment variable" % tool)
            return False, None, None, None, None, None

        # Check if path exists
        if not os.path.exists(self.path):
            self.display_error("Checking requirements", "The specified directory does not exist.")
            return False, None, None, None, None, None

        log_debug2("Creating CA certificate...\n")
        
        f = open(ca_key, "w")
        if not self.run_command([tool, "genrsa", "2048"], f):
            self.display_error("Creating CA certificate...", "Could not generate RSA certificate")
            return False, None, None, None, None, None

        log_debug2("Creating CA key...\n")
        
        req_cmd = [tool, "req", "-new", "-x509", "-nodes", "-days", str(days), "-key", ca_key, "-out", ca_cert, "-config", self.config_file["CA"]]
        if not self.run_command(req_cmd):
            self.display_error("Creating CA certificate...", "Could not generate keys")
            return False, None, None, None, None, None

        log_debug2("Create server certificate and self-sign\n")
        result, server_key, server_req, server_cert = self.generate_certificate(tool, path, "server", ca_cert, ca_key, self.config_file["Server"])
        if not result:
            self.display_error("Create server certificate and self-sign", "Could not generate keys")
            return False, server_key, server_req, server_cert

        log_debug2("Create client certificates and self-sign\n")
        result, client_key, client_req, client_cert = self.generate_certificate(tool, path, "client", ca_cert, ca_key, self.config_file["Client"])
        if not result:
            self.display_error("Create client certificates and self-sign", "Could not generate keys")
            return False, server_key, server_req, server_cert

        return True, ca_cert, server_cert, server_key, client_cert, client_key

      
    def run(self):
        self.result = False
        if not self.verify_preconditions():
            return False
        
        self.generate_config_file("CA")
        self.generate_config_file("Server")
        self.generate_config_file("Client")
        
        self.result, self.ca_cert, self.server_cert, self.server_key, self.client_cert, self.client_key = self.generate(self.path, self.config_file)
        
        return True

class SSLWizard_IntroPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Welcome to MySQL Workbench SSL Wizard")

    def go_cancel(self):
        self.main.finish()

    def create_ui(self):
        box = mforms.newBox(False)
        box.set_padding(20)
        box.set_spacing(20)

        message = "This wizard will assist you to generate a set of SSL certificates and self-signed keys that are required \n"
        message += "by the MySQL server to enable SSL. Other files will also be generated so that you can check how to \n"
        message += "configure your server and clients as well as the attributes used to generate them."

        label = mforms.newLabel(message)
        box.add(label, False, True)
        
        self.content.add(box, False, True)
        box.show(True)

class SSLWizard_OptionsPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Options")
        
        self.generate_files = newCheckBox()
        self.generate_files.set_text("Generate new certificates and self-signed keys");
        self.generate_files.set_active(not self.check_all_files_availability())
        self.generate_files.set_enabled(self.check_all_files_availability())

        self.update_connection = newCheckBox()
        self.update_connection.set_text("Update the connection");
        self.update_connection.set_active(True)

        self.use_default_parameters = newCheckBox()
        self.use_default_parameters.set_text("Use default parameters");
        self.use_default_parameters.set_active(False)
        
        self.clear_button = newButton()
        self.clear_button.set_text("Clear Files")
        self.clear_button.add_clicked_callback(self.clear_button_clicked)
        self.clear_button.set_enabled(os.path.isdir(self.main.results_path))

    def go_cancel(self):
        self.main.finish()

    def check_all_files_availability(self):
        if not os.path.isdir(self.main.results_path):
            return False
        if not os.path.isfile(os.path.join(self.main.results_path, "ca-cert.pem")):
            return False
        if not os.path.isfile(os.path.join(self.main.results_path, "client-cert.pem")):
            return False
        if not os.path.isfile(os.path.join(self.main.results_path, "client-key.pem")):
            return False

        return True

    def clear_button_clicked(self):
        for filename in os.listdir(self.main.results_path):
            filepath = os.path.join(self.main.results_path, filename)
            try:
                if os.path.isfile(filepath):
                    os.unlink(filepath)
            except Exception, e:
                log_error("SSL Wizard: Unable to remove file %s\n%s" % (filepath, str(e)))
                return
                
        self.generate_files.set_active(True)
        self.generate_files.set_enabled(False)
        self.main.generate_files_changed()
        
    def create_ui(self):
        box = mforms.newBox(False)
        box.set_spacing(12)
        box.set_padding(12)

        message = "These options allow you to configure the process. You can use default parameters\n"
        message += "instead of providing your own, allow the generation of the certificates and determine\n"
        message += "whether to update the connection settings or not."

        label = mforms.newLabel(message)

        box.add(label, False, True)
        box.add(self.use_default_parameters, False, True)
        box.add(self.generate_files, False, True)
        box.add(self.update_connection, False, True)
        
        button_box = mforms.newBox(True)
        button_box.set_spacing(12)
        button_box.set_padding(12)
        
        button_box.add(self.clear_button, False, True)
        
        self.content.add(box, False, True)
        self.content.add(button_box, False, True)
        box.show(True)

class SSLWizard_GeneratePage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Generate certificates and self-signed keys")

        self.ca_cert = os.path.join(self.main.results_path, "ca-cert.pem").replace('\\', '/')
        self.server_cert = os.path.join(self.main.results_path, "server-cert.pem").replace('\\', '/')
        self.server_key = os.path.join(self.main.results_path, "server-key.pem").replace('\\', '/')
        self.client_cert = os.path.join(self.main.results_path, "client-cert.pem").replace('\\', '/')
        self.client_key = os.path.join(self.main.results_path, "client-key.pem").replace('\\', '/')

        self.table = mforms.newTable()
        self.table.set_padding(12)
        self.table.set_column_count(3)
        self.table.set_row_count(7)

        self.table.set_row_spacing(8)
        self.table.set_column_spacing(4)

        row, self.country_code = self.add_label_row(0, "Country:", "2 letter country code (eg, US)")
        row, self.state_name = self.add_label_row(row, "State or Province:", "Full state or province name")
        row, self.locality_name = self.add_label_row(row, "Locality:", "eg, city")
        row, self.org_name = self.add_label_row(row, "Organization:", "eg, company")
        row, self.org_unit = self.add_label_row(row, "Org. Unit:", "eg, section, department")
        row, self.email_address = self.add_label_row(row, "Email Address:", "")
        row, self.common_name = self.add_label_row(row, "Common:", "eg, put the FQDN of the server\nto allow server address validation")

        message = "Now you must specify the parameters to use in the certificates and self-signed key generation.\n"
        message += "This may include some data refering to youself and/or the company you work for. All fields are optional."
        
        self.parameters_box = mforms.newBox(False)
        self.parameters_box.set_padding(20)
        self.parameters_box.set_spacing(20)

        self.parameters_label = mforms.newLabel(message)
        
        self.parameters_panel = mforms.newPanel(mforms.TitledBoxPanel)
        self.parameters_panel.set_title("Optional Parameters")
        self.parameters_panel.add(self.table)
        
        self.parameters_box.add(self.parameters_label, False, True)
        self.parameters_box.add(self.parameters_panel, False, True)

        self.default_label = mforms.newLabel("The wizard is ready to generate the files for you. Click 'Next >' to generate \nthe certificates and self-signed key files...")

    def add_label_row(self, row, label, help):
        control = mforms.newTextEntry()
        self.table.add(mforms.newLabel(label, True), 0, 1, row, row+1, mforms.HFillFlag)
        self.table.add(control, 1, 2, row, row+1, mforms.HFillFlag|mforms.HExpandFlag)
        l = mforms.newLabel(help)
        l.set_style(mforms.SmallHelpTextStyle)
        self.table.add(l, 2, 3, row, row+1, mforms.HFillFlag)
        control.set_size(100, -1)
        return row+1, control

    def set_show_parameters(self, value):
        self.parameters_box.show(bool(value))
        self.default_label.show(not value)

    def get_attributes(self, target):
        l = []
        l.append("C=%s"%self.country_code.get_string_value().encode('utf-8'))
        l.append("ST=%s"%self.state_name.get_string_value().encode('utf-8'))
        l.append("L=%s"%self.locality_name.get_string_value().encode('utf-8'))
        l.append("O=%s"%self.org_name.get_string_value().encode('utf-8'))
        l.append("OU=%s"%self.org_unit.get_string_value().encode('utf-8'))
        l.append("CN=%s-%s"%(self.common_name.get_string_value().encode('utf-8'), target))
        l.append("emailAddress=%s"%self.email_address.get_string_value().encode('utf-8'))
        # filter out blank values
        l = [s for s in l if s.partition("=")[-1]]
        if not l:
            l.append("C=US")
        return l

    def create_ui(self):
        self.content.add(self.parameters_box, False, True)
        self.content.add(self.default_label, False, True)

    def go_cancel(self):
        self.main.finish()

    def go_next(self):
        log_debug2("Setting up in path %s\n" % self.main.results_path)
        
        task = SSLWizard_GenerationTask(self.main, self.main.results_path)
        task.run()
        
        if task.result == False:
            return
          
        self.ca_cert = task.ca_cert
        self.server_cert = task.server_cert
        self.server_key = task.server_key
        self.client_cert = task.client_cert
        self.client_key = task.client_key
        f = open(os.path.join(self.main.results_path, "my.cnf.sample"), "w+")
        f.write("""# Copy this to your my.cnf file. Please change <directory> to the corresponding 
# directory where the files were copied.
[client]
ssl-ca=%(ca_cert)s
ssl-cert=%(client_cert)s
ssl-key=%(client_key)s

[mysqld]
ssl-ca=%(ca_cert)s
ssl-cert=%(server_cert)s
ssl-key=%(server_key)s
        """ % {"ca_cert"     : os.path.join("<directory>", os.path.basename(self.ca_cert)).replace('\\', '/'), 
               "server_cert" : os.path.join("<directory>", os.path.basename(self.server_cert)).replace('\\', '/'), 
               "server_key"  : os.path.join("<directory>", os.path.basename(self.server_key)).replace('\\', '/'), 
               "client_cert" : os.path.join("<directory>", os.path.basename(self.client_cert)).replace('\\', '/'), 
               "client_key"  : os.path.join("<directory>", os.path.basename(self.client_key)).replace('\\', '/')
              })
        f.close()
        log_debug2("SSL Wizard generation task result: %s\n" % str(task.result))
        
        self.main.go_next_page()


class SSLWizard_ResultsPage(WizardPage):
    def __init__(self, owner):
        WizardPage.__init__(self, owner, "Results")
        self.update_connection = True
        
    def set_update_connection(self, value):
        self.update_connection = value

    def go_next(self):
        if self.update_connection:
            self.main.conn.parameterValues['sslCA'] = self.main.generate_page.ca_cert.replace('\\', '/')
            self.main.conn.parameterValues['sslCert'] = self.main.generate_page.client_cert.replace('\\', '/')
            self.main.conn.parameterValues['sslKey'] = self.main.generate_page.client_key.replace('\\', '/')
            self.main.conn.parameterValues['useSSL'] = 4
            
        self.main.go_next_page()

    def create_ui(self):
      
        message = "The wizard was successful. "
        
        if self.update_connection:
            message += "Click on the finish button to update the connection. "
            
        message += "To setup the server, you should \ncopy the following files to a <directory> inside %s:\n\n" % self.main.conn.parameterValues['hostName']
        message += " - %s\n" % str(os.path.join(self.main.results_path, "ca-cert.pem")).replace('\\', '/')
        message += " - %s\n" % str(os.path.join(self.main.results_path, "server-cert.pem")).replace('\\', '/')
        message += " - %s\n" % str(os.path.join(self.main.results_path, "server-key.pem")).replace('\\', '/')
        message += "\n\nand edit the config file to use the following parameters:"
        
        label = mforms.newLabel(message)
        self.content.add(label, False, True)

        f = open(os.path.join(self.main.results_path, "my.cnf.sample"), "r")
        config_file = mforms.newTextBox(mforms.VerticalScrollBar)
        config_file.set_value(f.read())
        config_file.set_size(-1, 150)
        self.content.add(config_file, False, True)
        f.close()
        
        label = mforms.newLabel("A copy of this file can be found in:\n%s" % str(os.path.join(self.main.results_path, "my.cnf.sample").replace('\\', '/')))
        self.content.add(label, False, True)
        
        return

class SSLWizard(WizardForm):
    def __init__(self, parent, conn, conn_id):
        WizardForm.__init__(self, parent)

        self.conn = conn
        self.conn_id = conn_id
        self.certificates_root = os.path.join(mforms.App.get().get_user_data_folder(), "certificates")
        self.results_path = os.path.join(self.certificates_root, self.conn_id)
        
        self.set_title("SSL Wizard")

        self.intro_page = SSLWizard_IntroPage(self)
        self.add_page(self.intro_page)

        self.options_page = SSLWizard_OptionsPage(self)
        self.add_page(self.options_page)
        
        self.generate_page = SSLWizard_GeneratePage(self)
        self.add_page(self.generate_page)

        self.results_page = SSLWizard_ResultsPage(self)
        self.add_page(self.results_page)
        
        # Set the default selection values
        self.generate_page.set_show_parameters(not self.options_page.use_default_parameters.get_active())
        self.results_page.set_update_connection(self.options_page.update_connection.get_active())
        self.generate_page.skip_page(not self.options_page.generate_files.get_active())
        
        # Setup up the callbacks for the options
        self.options_page.use_default_parameters.add_clicked_callback(lambda: self.generate_page.set_show_parameters(not self.options_page.use_default_parameters.get_active()))
        self.options_page.update_connection.add_clicked_callback(lambda: self.results_page.set_update_connection(self.options_page.update_connection.get_active()))
        self.options_page.generate_files.add_clicked_callback(lambda: self.generate_files_changed())

    def generate_files_changed(self):
        self.generate_page.skip_page(not self.options_page.generate_files.get_active())


@ModuleInfo.export(grt.INT, mforms.Form, grt.classes.db_mgmt_Connection, grt.STRING)
def generateCertificates(parent, conn, conn_id):
    try:
        log_info("Running SSL Wizard\nParent: %s\nUser Folder: %s\nConn Parameters: %s\nConn ID: %s\n" % (str(parent), mforms.App.get().get_user_data_folder(), str(conn.parameterValues), conn_id))
        p = mforms.fromgrt(parent)
        log_info("Running SSL Wizard\n%s\n" % str(p))
        r = SSLWizard(p, conn, conn_id)
        r.run(True)
    except Exception, e:
        log_error("There was an exception running SSL Wizard.\n%s\n\n%s" % (str(e), traceback.format_exc()))


