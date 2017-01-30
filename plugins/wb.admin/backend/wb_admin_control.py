# Copyright (c) 2007, 2016, Oracle and/or its affiliates. All rights reserved.
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

import os
import socket
import threading
import time
import Queue
import StringIO
import traceback

from workbench.utils import Version
from wb_admin_ssh import SSHDownException
from workbench.db_utils import MySQLConnection, MySQLError, QueryError, strip_password, escape_sql_string

from wb_common import OperationCancelledError, Users, PermissionDeniedError, InvalidPasswordError, SSHFingerprintNewError

from wb_server_control import PasswordHandler, ServerControlShell, ServerControlWMI
from wb_server_management import ServerManagementHelper, SSH, wbaOS

from workbench.notifications import nc

import grt
import mforms

from workbench.log import log_info, log_warning, log_error, log_debug

MYSQL_ERR_ACCESS_DENIED = 1045
MYSQL_ERR_PASSWORD_EXPIRED = 1820
MYSQL_ERR_OFFLINE_MODE = 3032

#===============================================================================
#
#===============================================================================
# Handling object should have corresponding method if the object wants to receive
# events. For event shutdown method name must be shutdown_event
class EventManager(object):
    valid_events = ['server_offline_event', 'server_started_event', 'server_stopped_event', 'shutdown_event']

    def __init__(self):
        self.events = {}
        self.defer = False
        self.deferred_events = []

    def add_event_handler(self, event_name, handler):
        event_name += "_event"
        if hasattr(handler, event_name):
            handlers_list = None

            if event_name in self.events:
                handlers_list = self.events[event_name]
            else:
                handlers_list = []
                self.events[event_name] = handlers_list

            handlers_list.append(handler)
            log_debug("Added " + handler.__class__.__name__ + " for event " + event_name + '\n')
        else:
            log_error(handler.__class__.__name__ + " does not have method " + event_name + '\n')

    def defer_events(self):
        self.defer = True

    def continue_events(self):
        self.defer = False
        for ev_name in self.deferred_events:
            self.event(ev_name)
        self.deferred_events = []

    def event(self, name):
        if self.defer:
            self.deferred_events.append(name)
            return
        name += "_event"
        if name not in self.valid_events:
            log_error('EventManager: invalid event: ' + name + '\n')
        elif name in self.events:
            log_debug("Found event " + name + " in list" + '\n')
            for obj in self.events[name]:
                if hasattr(obj, name):
                    log_debug("Passing event " + name + " to " + obj.__class__.__name__ + '\n')
                    getattr(obj, name)()
        else:
            log_debug("Found valid but unrequested event " + name + " in list" + '\n')

#===============================================================================
#import thread
class SQLQueryExecutor(object):
    dbconn = None
    mtx     = None

    def __init__(self, dbconn):
        log_debug("Constructing SQL query runner, dbconn (" + repr(dbconn) + ')\n')
        self.mtx = threading.Lock()
        self.dbconn = dbconn

    def is_connected(self):
        return self.dbconn is not None and self.dbconn.is_connected

    def close(self):
        if self.is_connected():
            self.mtx.acquire()
            try:
                if self.dbconn:
                    self.dbconn.disconnect()
            finally:
                self.mtx.release()
        self.dbconn = None

    def exec_query(self, query):
        result = None
        self.mtx.acquire()
        try:
            if self.is_connected():
                log_debug("Executing query %s\n" % strip_password(query))
                result = self.dbconn.executeQuery(query)
        finally:
            self.mtx.release()
        return result

    def exec_query_multi_result(self, query):
        result = None
        self.mtx.acquire()
        try:
            if self.is_connected():
                log_debug("Executing query multi result %s\n" % strip_password(query))
                result = self.dbconn.executeQueryMultiResult(query)
        finally:
            self.mtx.release()
        return result

    def execute(self, query):
        result = None
        self.mtx.acquire()
        try:
            if self.is_connected():
                log_debug("Executing statement %s\n" % strip_password(query))
                result = self.dbconn.execute(query)
        finally:
            self.mtx.release()
        return result

    def updateCount(self):
        return self.dbconn.updateCount()

#===============================================================================

class WbAdminControl(object):
    server_helper = None # Instance of ServerManagementHelper for doing file and process operations on target server
    server_control = None # Instance of ServerControlBase to do start/stop/status of MySQL server

    ssh = None
    sql = None

    worker_thread = None

    target_version = None
    raw_version = "unknown"
    
    def __init__(self, server_profile, editor, connect_sql=True, test_only = False):
        self.test_only = test_only
        self.server_control = None
        self.events   = EventManager()
        self.defer_events() # continue events should be called when all listeners are registered, that happens later in the caller code
                          # Until then events are piled in the queue

        self.server_profile = server_profile
        self.server_control_output_handler = None

        self.task_queue = Queue.Queue(512)

        self.sql_enabled = connect_sql
        self.password_handler = PasswordHandler(server_profile)

        self.server_active_plugins = set()

        self.running = True

        self.add_me_for_event("server_started", self)
        self.add_me_for_event("server_stopped", self)
        self.add_me_for_event("server_offline", self)

        self.editor = editor
        if editor:
            if editor.isConnected == 1:
                self.last_known_server_running_status = ("running", None) # (status, timestamp)
            elif editor.isConnected == -1:
                self.last_known_server_running_status = ("offline", None) # (status, timestamp)
            else:
                self.last_known_server_running_status = ("stopped", None) # (status, timestamp)
        else:
            self.last_known_server_running_status = ("unknown", None)

        self.server_variables = {}
        self.status_variables = {} # 1st time is updated by us and then in a fixed interval by the monitoring thread
        self.status_variables_time = None
        self.status_variable_poll_interval = 3

        # Sets the default logging callback
        self.log_cb = self.raw_log


    def raw_log(self, data):
        log_info(data + "\n")


    @property
    def admin_access_available(self):
        return self.server_control is not None

    def _confirm_server_fingerprint(self, host, port, fingerprint_exception):
        msg = { 'msg': "The authenticity of host '%(0)s (%(0)s)' can't be established.\n%(1)s key fingerprint is %(2)s\nAre you sure you want to continue connecting?"  % {'0': "%s:%s" % (host, port), '1': fingerprint_exception.key.get_name(), '2': fingerprint_exception.fingerprint}, 'obj': fingerprint_exception}
        if mforms.Utilities.show_message("SSH Server Fingerprint Missing", msg['msg'], "Continue", "Cancel", "") == mforms.ResultOk:
            msg['obj'].client._host_keys.add(msg['obj'].hostname, msg['obj'].key.get_name(), msg['obj'].key)
            if msg['obj'].client._host_keys_filename is not None:
                try:
                    if os.path.isdir(os.path.dirname(msg['obj'].client._host_keys_filename)) == False:
                        log_warning("Host_keys directory is missing, recreating it\n")
                        os.makedirs(os.path.dirname(msg['obj'].client._host_keys_filename))
                    if os.path.exists(msg['obj'].client._host_keys_filename) == False:
                        log_warning("Host_keys file is missing, recreating it\n")
                        open(msg['obj'].client._host_keys_filename, 'a').close()
                    msg['obj'].client.save_host_keys(msg['obj'].client._host_keys_filename)
                    log_warning("Successfully saved host_keys file.\n")
                    return True
                except IOError, e:
                    error = str(e)
                    raise e
        else:
            return False

    def acquire_admin_access(self):
        """Make sure we have access to the instance for admin (for config file, start/stop etc)"""
        if self.server_control or not self.server_profile.admin_enabled:
            return
        if self.server_profile.uses_ssh:
            try:
                while True:
                    try:
                        self.ssh = SSH(self.server_profile, self.password_handler)
                        break
                    except SSHFingerprintNewError, exc:
                        if self._confirm_server_fingerprint(self.server_profile.ssh_hostname, self.server_profile.ssh_port, exc):
                            continue;
                        else:
                            raise OperationCancelledError("user cancel")
                    
            except OperationCancelledError:
                self.ssh = None
                raise OperationCancelledError("SSH connection cancelled")

            except SSHDownException, e:
                log_error("SSHDownException: %s\n" % traceback.format_exc())
                self.ssh = None
                if self.sql_enabled:
                    if mforms.Utilities.show_warning('SSH connection failed',
                                                     "Check your SSH connection settings and whether the SSH server is up.\nYou may continue anyway, but some functionality will be unavailable.\nError: %s" % e,
                                                     "Continue", "Cancel", "") != mforms.ResultOk:
                        raise OperationCancelledError("Could not connect to SSH server")
                else:
                    mforms.Utilities.show_warning('SSH connection failed',
                                                  "Check your SSH connection settings and whether the SSH server is up.\nError: %s" % e, "OK", "", "")
        else:
            self.ssh = None
        # init server management helper (for doing remote file operations, process management etc)
        self.server_helper = ServerManagementHelper(self.server_profile, self.ssh)
        if self.server_helper:
            self.server_profile.expanded_config_file_path = self.server_helper.shell.expand_path_variables(self.server_profile.config_file_path)
            self.query_server_installation_info()
        # detect the exact type of OS we're managing
        os_info = self.detect_operating_system_version()
        if os_info:
            os_type, os_name, os_variant, os_version = os_info
            log_info("Target OS detection returned: os_type=%s, os_name=%s, os_variant=%s, os_version=%s\n" % (os_type, os_name, os_variant, os_version))
            self.server_profile.set_os_version_info(os_type or "", os_name or "", os_variant or "", os_version or "")
        else:
            log_warning("Could not detect target OS details\n")
        # init server control instance appropriate for what we're connecting to
        if self.server_profile.uses_wmi:
            self.server_control = ServerControlWMI(self.server_profile, self.server_helper, self.password_handler)
        elif self.server_profile.uses_ssh or self.server_profile.is_local:
            self.server_control = ServerControlShell(self.server_profile, self.server_helper, self.password_handler)
        else:
            log_error("""Unknown management method selected. Server Profile is possibly inconsistent
uses_ssh: %i uses_wmi: %i\n""" % (self.server_profile.uses_ssh, self.server_profile.uses_wmi))
            raise Exception("Unknown management method selected. Server Profile is possibly inconsistent")

        # Sets the default logging callback
        if self.server_control:
            self.log_cb = self.server_control.info
            self.end_line = ""

    def init(self):
        # connect SQL
        while self.sql_enabled:
            try:
                self.connect_sql()
                break
            except MySQLError, err:
                log_error("Error connecting to MySQL: %s\n" % err)
                if err.code == MYSQL_ERR_ACCESS_DENIED:
                    # Invalid password, request password
                    r = mforms.Utilities.show_error("Could not connect to MySQL Server at %s" % err.location,
                            "Could not connect to MySQL server: %s\nClick Continue to proceed without functionality that requires a DB connection." % err,
                            "Retry", "Cancel", "Continue")
                    if r == mforms.ResultOk:
                        continue
                    elif r == mforms.ResultCancel:
                        raise OperationCancelledError("Connection cancelled")
                    else:
                        # continue without a db connection
                        self.sql_enabled = False
                elif err.code == MYSQL_ERR_PASSWORD_EXPIRED:
                    if not grt.modules.WbAdmin.handleExpiredPassword(self.server_profile.db_connection_params):
                        raise OperationCancelledError("Connection cancelled")
                else:
                    self.sql_enabled = False
                    if not self.server_profile.admin_enabled:  # We have neither sql connection nor management method
                        raise Exception('Could not connect to MySQL Server and no management method is available')

        if not self.worker_thread and not self.test_only:
            # start status variable check thread
            self.worker_thread = threading.Thread(target=self.server_polling_thread)
            self.worker_thread.start()


    def shutdown(self):
        self.events.event('shutdown')
        self.running = False
        self.disconnect_sql()
        if self.ssh:
            self.ssh.close()


    #---------------------------------------------------------------------------
    def force_check_server_state(self, verbose = False):
        # Check the current state of the server and cause the SQL Editor to reconnect/update if the state
        # changed. Returns None if no state change was detected or the new state if it did change.
        old_state = self.last_known_server_running_status[0]
        new_state = self.is_server_running(verbose=verbose, force_hard_check=True)
        log_debug("Force check server state returned %s\n" % new_state)
        if new_state != old_state:
            info = { "state" : -1, "connection" : self.server_profile.db_connection_params }
            if new_state == "running":
                info['state'] = 1 
            elif new_state == "offline":
                info['state'] = -1;
            else:
                info['state'] = 0;
            
            # this will notify the rest of the App that the server state has changed, giving them a chance
            # to reconnect or formally disconnect
            nc.send("GRNServerStateChanged", self.editor, info)
            
            if new_state == "stopped" or new_state == "offline":
                self.server_variables = {}
                self.status_variables_time = None
                self.status_variables = {}

            return new_state
        return None


    #---------------------------------------------------------------------------
    def is_server_running(self, verbose = 0, force_hard_check = False):
        # Check recent information from query runs. If last query (not older than 4 secs)
        # was performed successfully, then we imply that server is up.
        ret = "unknown"
        status, stime = self.last_known_server_running_status
        if not force_hard_check:
            ret = status
        else:
            self.log_cb("Checking server status...")
            if self.is_sql_connected():
                # do a ping on the connection to make sure it's still up
                if self.sql_ping():
                    self.log_cb("MySQL server is currently running")
                    self.query_server_info() #query_server_info do update server status
                    status, stime = self.last_known_server_running_status
                    ret = status
            if ret not in ("running", "offline"):
                self.log_cb("Trying to connect to MySQL...")

                conn = grt.modules.DbMySQLQuery.openConnection(self.server_profile.db_connection_params)
                code = grt.modules.DbMySQLQuery.lastErrorCode()
                err = grt.modules.DbMySQLQuery.lastError()
                if conn > 0:
                    grt.modules.DbMySQLQuery.closeConnection(conn)

                if code == 0:
                    self.log_cb("Connection succeeded")
                else:
                    self.log_cb("%s (%i)" % (err, code))

                if code in (2002, 2003, 2013):
                    ret = "stopped"
                    self.log_cb("Assuming server is not running")
                elif code == MYSQL_ERR_OFFLINE_MODE:
                    ret = "offline"
                    self.log_cb("Server is in offline mode")
                else:
                    self.query_server_info()
                    ret = "running"
                    self.log_cb("Assuming server is running")

        return ret

    #---------------------------------------------------------------------------
    def uitask(self, task, *args):
        self.task_queue.put((task, args))

    #---------------------------------------------------------------------------
    def process_ui_task_queue(self):
        while not self.task_queue.empty():
            func, args = self.task_queue.get()
            func(*args)
            self.task_queue.task_done()

    """
      Adds object for event named @event. In order for object to handle event
    it needs to conform to some requirements. For example if there is a need for
    some object to handle an event named 'server_started', the object's class must
    have method named server_started_event. Valid event names can be found in
    EventManager class.

    param event: name of the event, for example 'server_started'
    param obj: object which method named <event_name>_event will be called
    """
    def add_me_for_event(self, event, obj):
        self.events.add_event_handler(event, obj)

    #---------------------------------------------------------------------------
    """
      Tells to notify all listener objects that event occured.
    """
    def event(self, name):
        self.events.event(name)


    def event_from_main(self, event_name):
        self.uitask(self.event, event_name)

    #---------------------------------------------------------------------------
    """
    This method should not be used outside of MyCtrl. The aim of the method to
    queue coming events for later processing. This is used when events start tp
    appear at load stage, but all listeners may not be registered yet.
    """
    def defer_events(self):
        self.events.defer_events()

    #---------------------------------------------------------------------------
    """
    This method should not be used outside of MyCtrl. It indicate that deferred
    events can be delivered to listeners and resumes normal events flow processing.
    """
    def continue_events(self):
        self.events.continue_events()


    def expand_path_variables(self, path):
        ret = path
        if self.server_helper:
            ret = self.server_helper.shell.expand_path_variables(self.server_profile.config_file_path)
        return ret

    #---------------------------------------------------------------------------
    def server_started_event(self):
        try:
            if not self.is_sql_connected():
                self.connect_sql()
        except Exception, e:
            log_error("Error connecting to MySQL: %s\n" % e)
            mforms.Utilities.show_error("Connect Error", "Could not connect to MySQL: %s" % e, "OK", "", "")

        if not self.worker_thread.is_alive() and not self.test_only:
            # start status variable check thread
            self.worker_thread = threading.Thread(target=self.server_polling_thread)
            self.worker_thread.start()

    #---------------------------------------------------------------------------
    def server_stopped_event(self):
        self.disconnect_sql()
        self.last_known_server_running_status = ("stopped", time.time())
        
    #---------------------------------------------------------------------------
    def server_offline_event(self):
        self.last_known_server_running_status = ("offline", time.time())

    #---------------------------------------------------------------------------
    """
    This method is passed to MySQLConnection.__init__. MySQLConnection will call
    this method when connection created/destroyed, also on success or error when
    executing query
    """
    def sql_status_callback(self, code, error, connect_info):
        if code == 0:
            if "offline_mode" in self.server_variables and self.server_variables["offline_mode"] == "ON":
                self.last_known_server_running_status = ("offline", time.time()) 
            else:
                self.last_known_server_running_status = ("running", time.time())


    #---------------------------------------------------------------------------
    def connect_sql(self): # from GUI thread only, throws MySQLError
        if not self.is_sql_connected():
            password = self.get_mysql_password()

            connection = MySQLConnection(self.server_profile.db_connection_params, self.sql_status_callback,
                                        password=password)
            try:
                connection.connect()
            except grt.UserInterrupt:
                log_info("Cancelled connection\n")
                return

            self.sql = SQLQueryExecutor(connection)

            if self.is_sql_connected():
                # perform some server capabilities checking
                self.query_server_info()
            else:
                log_error("Failed to connect to MySQL server\n")
        else:
            log_info("Already connected to MySQL server\n")

    #---------------------------------------------------------------------------
    def disconnect_sql(self):
        if self.sql:
            self.sql.close()
        self.sql = None
        self.raw_version = None
        self.target_version = None

        self.server_version = "unknown"
        mforms.Utilities.driver_shutdown()

    #---------------------------------------------------------------------------
    def is_sql_connected(self):
        return self.sql and self.sql.is_connected()


    #---------------------------------------------------------------------------
    def server_polling_thread(self):
        try:
            password = self.get_mysql_password()
            self.poll_connection = MySQLConnection(self.server_profile.db_connection_params, password=password)
            self.poll_connection.connect()
        except MySQLError, err:
            log_error("Error creating SQL connection for monitoring: %r\n" % err)
            self.poll_connection = None
            mforms.Utilities.driver_shutdown()
            return None

        log_info("Monitoring thread running...\n")
        time.sleep(self.status_variable_poll_interval)
        try:
            # runs in a separate thread to fetch status variables
            while self.running:
                variables = {}
                result = self.poll_connection.executeQuery("SHOW GLOBAL STATUS")
                while result and result.nextRow():
                    variables[result.stringByName("Variable_name")] = result.stringByName("Value")

                self.status_variables, self.status_variables_time = variables, time.time()

                time.sleep(self.status_variable_poll_interval)
        except QueryError:
            log_error("Error in monitoring thread: %s\n" % traceback.format_exc())

        log_info("Monitoring thread done.\n")
        self.poll_connection.disconnect()
        self.poll_connection = None
        mforms.Utilities.driver_shutdown()

    #---------------------------------------------------------------------------
    def get_mysql_password(self):
        found, password = mforms.Utilities.find_cached_password(self.server_profile.db_connection_params.hostIdentifier, self.server_profile.mysql_username)
        if found:
            return password
        return None

    #---------------------------------------------------------------------------
    def sql_ping(self):
        ret = False
        if self.is_sql_connected():
            try:
                self.sql.exec_query("select 1")
                ret = True
            except QueryError, e:
                if not e.is_connection_error():
                    ret = True # Any other error except connection ones is from server
        else:
            try:
                self.connect_sql()
            except MySQLError, e:
                pass # ignore connection errors, since it likely means the server is down

            # Do not do anything for now, connection status check will be perfomed
            # on the next sql_ping call
        return ret

    #---------------------------------------------------------------------------
    def exec_query(self, q, auto_reconnect=True):
        ret = None
        if self.sql is not None:
            try:
                ret = self.sql.exec_query(q)
            except QueryError, e:
                log_warning("Error executing query %s: %s\n"%(q, strip_password(str(e))))
                if auto_reconnect and e.is_connection_error():
                    log_warning("exec_query: Loss of connection to mysql server was detected.\n")
                    self.handle_sql_disconnection(e)
                else: # if exception is not handled, give a chance to the caller do it
                    raise e
        else:
            log_info("sql connection is down\n")

        return ret

    def exec_query_multi_result(self, q, auto_reconnect=True):
        ret = None
        if self.sql is not None:
            try:
                ret = self.sql.exec_query_multi_result(q)
            except QueryError, e:
                log_warning("Error executing query multi result %s: %s\n"%(q, strip_password(str(e))))
                if auto_reconnect and e.is_connection_error():
                    log_warning("exec_query_multi_result: Loss of connection to mysql server was detected.\n")
                    self.handle_sql_disconnection(e)
                else: # if exception is not handled, give a chance to the caller do it
                    raise e
        else:
            log_info("sql connection is down\n")

        return ret

    def exec_sql(self, q, auto_reconnect=True):
        if self.sql is not None:
            try:
                ret = self.sql.execute(q)
                cnt = self.sql.updateCount()
                return ret, cnt
            except QueryError, e:
                log_warning("Error executing SQL %s: %s\n"%(strip_password(q), strip_password(str(e))))
                if auto_reconnect and e.is_connection_error():
                    log_warning("exec_sql: Loss of connection to mysql server was detected.\n")
                    self.handle_sql_disconnection(e)
                else:
                    raise e
        else:
            log_info("sql connection is down\n")

        return None, -1

    def handle_sql_disconnection(self, e):
        self.disconnect_sql()
        if e.is_error_recoverable():
            log_warning("Error is recoverable. Reconnecting to MySQL server.\n")
            try:
                self.connect_sql()
                if self.is_sql_connected():
                    return True
            except MySQLError, er:
                log_warning("Auto-reconnection failed: %s\n" % er)
            return False
        return False

    #---------------------------------------------------------------------------
    def get_server_variable(self, variable, default = None):
        return self.server_variables.get(variable, default)

    #---------------------------------------------------------------------------
    def open_ssh_session_for_monitoring(self):
        ssh = SSH(self.server_profile, self.password_handler)

        return ssh

    #---------------------------------------------------------------------------
    def is_ssh_connected(self):
        return self.ssh is not None


    #---------------------------------------------------------------------------
    def query_server_info(self):
        self.server_variables = {}
        result = self.exec_query("SHOW VARIABLES")
        
        if not result:
            # Didn't get the server variables. Assuming the server is stopped
            self.last_known_server_running_status = ("stopped", time.time())
            return
        
        while result and result.nextRow():
            self.server_variables[result.stringByName("Variable_name")] = result.stringByName("Value")

        self.status_variables_time = time.time()
        self.status_variables = {}
        result = self.exec_query("SHOW GLOBAL STATUS")
        while result and result.nextRow():
            self.status_variables[result.stringByName("Variable_name")] = result.stringByName("Value")
            
        # check version
        if self.server_variables:
            self.raw_version = self.server_variables["version"]
            self.target_version = Version.fromstr(self.raw_version)

            if self.server_profile.server_version != self.raw_version: # Update profile version with live data from server
                log_info('%s.connect_sql(): The server version stored in the server instance profile was "%s". '
                            'Changed to the version reported by the server: "%s"\n' % (self.__class__.__name__,
                            self.server_profile.server_version, self.raw_version) )
                self.server_profile.server_version = self.raw_version

        if self.target_version and self.target_version.is_supported_mysql_version_at_least(5, 1, 5):
            # The command to retrieve plugins was 'SHOW PLUGIN' for v. [5.1.5, 5.1.9)
            # and was changed to 'SHOW PLUGINS' from MySQL Server v. 5.1.9 onwards:
            plugin_var = 'PLUGINS' if self.target_version.is_supported_mysql_version_at_least(5, 1, 9) else 'PLUGIN'
            result = self.exec_query('SHOW %s' % plugin_var)
            # check whether Windows authentication plugin is available
            while result and result.nextRow():
                name = result.stringByName("Name")
                status = result.stringByName("Status")
                plugin_type = result.stringByName("Type")
                if status == "ACTIVE":
                    self.server_active_plugins.add((name, plugin_type))
        if "offline_mode" in self.server_variables and self.server_variables["offline_mode"] == "ON":
            #We're in offline mode, need to change server status
            self.last_known_server_running_status = ("offline", time.time())
        else:
            self.last_known_server_running_status = ("running", time.time())


    def query_server_installation_info(self):
        normpath = self.server_profile.path_module.normpath

        def get_config_options():
            if self.server_profile.config_file_path and self.server_helper:
                try:
                    cfg_file = StringIO.StringIO(self.server_helper.get_file_content(self.server_profile.config_file_path))
                except PermissionDeniedError:
                    log_debug('Could not open the file "%s" as the current user. Trying as admin\n' % self.server_profile.config_file_path)
                    while True:
                        try:
                            password = self.password_handler.get_password_for('file')
                            cfg_file = StringIO.StringIO(self.server_helper.get_file_content(self.server_profile.config_file_path,
                                              as_user=Users.ADMIN, user_password = password))
                            break
                        except InvalidPasswordError:
                            self.password_handler.reset_password_for('file')
                        except Exception, err:
                            log_error('Could not open the file "%s": %s\n' % (self.server_profile.config_file_path, str(err)))
                            return {}
                except Exception, err:
                    log_error('Could not open the file "%s": %s\n' % (self.server_profile.config_file_path, str(err)))
                    return {}

                opts = {}
                section = 'root'
                for line in cfg_file:
                    line = line.strip()
                    if line == '' or line.startswith('#'):
                        continue
                    elif line.startswith('[') and line.endswith(']'):
                        section = line[1:-1].strip()
                    else:
                        k, d, v = line.partition('=')
                        val = v.strip() or 'ON'
                        opts.setdefault(section, {})[k.strip()] = val
                return opts
            return {}

        opts = get_config_options()
        config_section = self.server_profile.config_file_section or 'mysqld'

        request_save_profile = False

        if self.server_variables:
            hostname = self.server_variables.get('hostname', '')
            if not hostname and self.server_profile.is_local:
                hostname = socket.gethostname()

            datadir = self.server_variables.get('datadir', '')
            if datadir and self.server_profile.datadir != datadir:
                self.server_profile.datadir = datadir
                request_save_profile = True

            basedir = self.server_variables.get('basedir', '')
            if basedir and self.server_profile.basedir != basedir:
                self.server_profile.basedir = basedir
                request_save_profile = True

            try:
                general_log_enabled = self.server_variables.get('general_log') in ('ON', '1')
                if self.server_profile.general_log_enabled != general_log_enabled:
                    self.server_profile.general_log_enabled = general_log_enabled
                    request_save_profile = True
            except ValueError:
                pass

            try:
                if self.target_version and self.target_version.is_supported_mysql_version_at_least(5, 1, 29):
                    slow_query_var = 'slow_query_log'
                else:
                    slow_query_var = 'log_slow_queries'
                
                slow_log_enabled = self.server_variables.get(slow_query_var) in ('ON', '1')
                if self.server_profile.slow_log_enabled != slow_log_enabled:
                    self.server_profile.slow_log_enabled = slow_log_enabled
                    request_save_profile = True
            except ValueError:
                pass

        if not self.target_version or not self.target_version.is_supported_mysql_version_at_least(5, 1, 29):
            general_log_file_path = opts[config_section].get('log', '').strip(' "') if opts.has_key(config_section) else ''
            general_log_file_path = normpath(general_log_file_path) if general_log_file_path else ''
            if general_log_file_path and self.server_profile.general_log_file_path != general_log_file_path:
                self.server_profile.general_log_file_path = general_log_file_path or os.path.join(self.server_profile.datadir, hostname + '.log')
                request_save_profile = True

            slow_query_log_file = opts[config_section].get('log-slow-queries', '').strip(' "') if opts.has_key(config_section) else ''
            slow_query_log_file = normpath(slow_query_log_file) if slow_query_log_file else ''
            if slow_query_log_file and self.server_profile.slow_log_file_path != slow_query_log_file:
                self.server_profile.slow_log_file_path = slow_query_log_file or os.path.join(self.server_profile.datadir, hostname + '.slow')
                request_save_profile = True

            error_log_file_path = opts[config_section].get('log-error', '').strip(' "') if opts.has_key(config_section) else ''
            error_log_file_path = normpath(error_log_file_path) if error_log_file_path else ''
            if error_log_file_path and self.server_profile.error_log_file_path != error_log_file_path:
                self.server_profile.error_log_file_path = error_log_file_path or os.path.join(self.server_profile.datadir, hostname + '.err')
                request_save_profile = True

        else:
            if self.server_variables:
                path = self.server_variables.get('general_log_file')
                general_log_file_path = normpath(path) if path and path != '0' else ''
                if self.server_profile.general_log_file_path != general_log_file_path:
                    self.server_profile.general_log_file_path = general_log_file_path
                    request_save_profile = True

                path = self.server_variables.get('slow_query_log_file')
                slow_query_log_file_path = normpath(path) if path and path != '0' else ''
                if self.server_profile.slow_log_file_path != slow_query_log_file_path:
                    self.server_profile.slow_log_file_path = slow_query_log_file_path
                    request_save_profile = True

                path = self.server_variables.get('log_error')
                log_error_path = normpath(path) if path and path != '0' else ''
                if self.server_profile.error_log_file_path != log_error_path:
                    self.server_profile.error_log_file_path = log_error_path
                    request_save_profile = True

        log_info("Currently connected to MySQL server version " + repr(self.raw_version) +
                 ", conn status = " + repr(self.is_sql_connected()) +
                 ", active plugins = " + str([x[0] for x in self.server_active_plugins]) + '\n')

        # Save the server profile if at least one of its values has changed:
        if request_save_profile:
            from grt.modules import Workbench
            Workbench.saveInstances()


    #---------------------------------------------------------------------------
    def detect_operating_system_version(self):
        """Try to detect OS information in the remote server, via SSH connection.
            
        The information returned is (os_type, os_name, os_variant, os_version)
            
            
            os_type: one of the main types of OS supported (one of wbaOS.windows, wbaOS.linux, wbaOS.darwin)
            os_name: the exact name of the OS (eg Windows, Linux, Mac OS X, Solaris)
            os_variant: the variant of the OS, esp for Linux distributions (eg Ubuntu, Fedora etc)
            os_version: the version of the OS (eg 12.04, XP etc)
        """
        if self.ssh or self.server_profile.is_local:
            o = StringIO.StringIO()
            # check if windows
            rc = self.server_helper.execute_command('ver', output_handler=o.write)
            if rc == 0 and o.getvalue().strip().startswith("Microsoft Windows"):
                os_type = wbaOS.windows
                os_name = "Windows"
                os_variant = "Windows"
                os_version = o.getvalue().strip()

                o = StringIO.StringIO()
                rc = self.server_helper.execute_command('reg query "HKLM\Software\Microsoft\Windows NT\CurrentVersion" /v "ProductName"', output_handler=o.write)
                if rc == 0:
                    os_name = " ".join(o.getvalue().strip().split("\n")[-1].split()[2:])
                return os_type, os_name, os_variant, os_version
            else:
                os_type, os_name, os_variant, os_version = (None, None, None, None)

                o = StringIO.StringIO()
                if self.server_helper.execute_command("uname", output_handler=o.write) == 0:
                    ostype = o.getvalue().strip()
                    log_debug("uname in remote system returned %s\n"%ostype)
                    if ostype == "Darwin":
                        os_type = wbaOS.darwin
                        o = StringIO.StringIO()
                        if self.server_helper.execute_command("sw_vers", output_handler=o.write) == 0:
                            for line in o.getvalue().strip().split("\n"):
                                line = line.strip()
                                if line:
                                    k, v = line.split(":", 1)
                                    if k == "ProductName":
                                        os_name = v.strip()
                                        os_variant = v.strip()
                                    elif k == "ProductVersion":
                                        os_version = v.strip()
                    else:
                        os_type = wbaOS.linux

                        o = StringIO.StringIO()
                        if self.server_helper.execute_command("lsb_release -a", output_handler=o.write) == 0:
                            os_name = "Linux"
                            for line in o.getvalue().strip().split("\n"):
                                line = line.strip()
                                if line:
                                    k, sep, v = line.partition(":")
                                    if k == "Distributor ID":
                                        os_variant = v.strip()
                                    elif k == "Release":
                                        os_version = v.strip()
                        else:
                            log_warning("lsb_release utility not found in target server. Consider installing its package if possible\n")
                            # some distros don't install lsb things by default
                            try:
                                info = self.server_helper.get_file_content("/etc/fedora-release")
                                if info:
                                    os_name = "Linux"
                                    os_variant = "Fedora"
                                    os_version = info[info.find("release")+1:].split()[0].strip()
                            except (IOError, OSError):
                                pass
                            try:
                                info = self.server_helper.get_file_content("/etc/debian_version")
                                if info:
                                    os_name = "Linux"
                                    os_variant = "Debian"
                                    os_version = info.strip()
                            except (IOError, OSError):
                                pass
                            try:
                                info = self.server_helper.get_file_content("/etc/oracle-release")
                                if info:
                                    os_name = "Linux"
                                    os_variant = "Oracle Linux"
                                    os_version = info[info.find("release")+1:].split()[0].strip()
                            except (IOError, OSError):
                                pass
                    return os_type, os_name, os_variant, os_version
                else:
                    log_error("Could not execute uname command on remote server, system type is unknown\n")
        return None
