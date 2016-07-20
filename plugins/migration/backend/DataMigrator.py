# Copyright (c) 2012, 2016 Oracle and/or its affiliates. All rights reserved.
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
import os
import sys
import subprocess
import Queue
import grt
import re
import tempfile
from threading import Thread
from workbench.db_driver import get_connection_parameters
from workbench.utils import replace_string_parameters
from workbench.exceptions import NotConnectedError
from workbench import db_utils
from migration_source_selection import request_password


class HelperExited(Exception):
    def __init__(self, what, returncode):
        Exception.__init__(self, what)
        self.returncode = returncode


def mysql_conn_string(conn):
    param = conn.parameterValues
    if conn.driver.name == "MysqlNative":
        return "%(userName)s@%(hostName)s:%(port)s" % param
    elif conn.driver.name == "MysqlNativeSocket":
        if not param.get('socket', False):
            try:
                connection = db_utils.MySQLConnection(conn, password=request_password(conn))
                connection.connect()
            except (NotConnectedError, db_utils.MySQLError):
                raise Exception('There is no connection to the target MySQL server and the socket parameter in your '
                                'target connection settings is blank. Please check that your target server is running '
                                'or go back to the Target Selection page and set the socket parameter there.')

            result = connection.executeQuery("SHOW VARIABLES LIKE 'socket';")
            if result and result.nextRow():
                socket = result.stringByName('Value')
                param = { 'userName':param['userName'], 'socket':socket }
                connection.disconnect()
            else:
                raise Exception('Failed while querying the socket server variable and the socket parameter in your '
                                'target connection settings is blank. Please go back to the Target Selection page and '
                                'make sure that you have the socket parameter set.')
            
        return "%(userName)s@::%(socket)s" % param
    else:
        raise Exception("Connection method type %s is not supported for migration" % conn.driver.name)

def odbc_conn_string(conn, strip_password = False):
    conn_params = dict(conn.parameterValues)
    conn_params.update(get_connection_parameters(conn))

    connection_string_template = conn.driver.connectionStringTemplate or 'DRIVER={%driver%};SERVER=%host%;PORT=%port%;DATABASE={%database%};UID=%username%;PWD={%password%}'
    connstring = replace_string_parameters(connection_string_template, conn_params)
    if strip_password:
        connstring = re.sub("(PWD={[^;]}*|PWD=[^;]*)", "", connstring).rstrip(";")
    return connstring

def python_conn_string(conn):
    return conn.driver.driverLibraryName + '://' + conn.parameterValues['wbcopytables_connection_string']


class TableCopyWorker(Thread):
    def __init__(self, owner, args, result_queue):
        Thread.__init__(self)
        self._owner = owner
        self.result_queue = result_queue
        self._process_args = args
        grt.log_debug3("Migration", "Spawning copy worker task: %s" % args)
        self._owner.send_info(" ".join(args))
        if sys.platform == "win32":
            # shell=True causes a created window to be hidden by default, this prevents a popup to be shown
            # on the migration wizard
            self.process = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            universal_newlines=True, shell=True)
        else:
            self.process = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            universal_newlines=True, close_fds=True)

    def feed_input(self, text):
        if self.process.poll() is None:
            if type(text) is unicode:
                text = text.encode("utf-8")
            self.process.stdin.write(text)
            self.process.stdin.flush()
        else:
            raise HelperExited("Table copy task finished unexpectedly", self.process.returncode)


    def run(self):
        try:
            while self.process.poll() is None:
                if self._owner.query_cancel_status():
                    raise grt.UserInterrupt("Canceled by user")
                line = self.process.stdout.readline()
                if line is not None:
                    type, _, msg = line.strip().partition(":")
                    if type in ("PROGRESS", "ERROR", "BEGIN", "END"):
                        self.result_queue.put((type, msg))
                    else:
                        self.result_queue.put(("LOG", line))

            # Processes any remaining output
            output = self.process.stdout.read()
            lines = output.split("\n")
            for line in lines:
                if line is not None:
                    type, _, msg = line.strip().partition(":")
                    if type in ("PROGRESS", "ERROR", "BEGIN", "END"):
                        self.result_queue.put((type, msg))
                    else:
                        self.result_queue.put(("LOG", msg))

            if self.process.returncode != 0:
                self.result_queue.put(("DONE", "Worker exited with status %s" % self.process.returncode))
            else:
                self.result_queue.put(("DONE", None))
        except grt.UserInterrupt, e:
            self._owner.send_info("Copy task interrupted by user, terminating %s..." % self.name)
            self.terminate()
            self.result_queue.put(("INTERRUPTED", None))
        except Exception, e:
            import traceback
            traceback.print_exc()
            self.result_queue.put(("DONE", str(e)))

    def terminate(self):
        if self.process.poll() is None:
            o, e = self.process.communicate()
            if o:
                last_progress = None
                for line in o.split("\n"):
                    if line.startswith("PROGRESS:"):
                        type, _, last_progress = line.strip().partition(":")
                    else:
                        self._owner.send_info(line)
                if last_progress:
                    self.result_queue.put((type, last_progress))
            if e:
                self._owner.send_info(e)

            # The process should be killed ONLY if it has not yet finished
            try:
                if hasattr(self.process, "terminate"):
                    self.process.terminate()
                else:
                    import signal
                    os.kill(self.process.pid, signal.SIGTERM)
            except OSError, e:
                if e.errno == 3:
                    pass
                else:
                    # can't kill process
                    self._owner.send_error("Unable to kill worker task %s: %s" % (self.process.id, e))

        self.process.wait()


class DataMigrator(object):
    copytable_path = "wbcopytables-bin"

    def __init__(self, message_target, options, srcconnobj, srcpassword, tgtconnobj, tgtpassword):
        assert hasattr(message_target, "send_info") and hasattr(message_target, "send_error") and hasattr(message_target, "send_progress")
        self._owner = message_target
        self._options = options
        self._src_conn_object = srcconnobj
        self._src_password = srcpassword or ''
        self._tgt_conn_object = tgtconnobj
        self._tgt_password = tgtpassword or ''
        self._resume = False

        # Container for the tasks...
        self._tasks = []
        self._processes = []
        self._error = None

    def count_table_rows(self, working_set):
        table_param = []
        if sys.platform == "win32":
            try:
                with tempfile.NamedTemporaryFile("w", delete=False) as table_file:
                    for task in working_set.values():
                        fields = []
                        fields.append(task["source_schema"])
                        fields.append(task["source_table"])
                        if self._resume:
                            fields.append(task["target_schema"])
                            fields.append(task["target_table"])
                            fields.append(task["source_primary_key"])
                            fields.append(task["target_primary_key"])
                            if task.get("select_expression", None):
                                fields.append(task["select_expression"])
                            else:
                                fields.append("*")
                        line = "\t".join(fields)
                        table_file.write(line + "\n")

                    table_param.append("--table-file=%s" % table_file.name)

            except IOError, e:
                raise Exception ("Error creating table file: %s" % e.strerror)
        else:
            for task in working_set.values():
                if self._resume:
                    table_param += ["--table", task["source_schema"], task["source_table"], task["target_schema"], task["target_table"], task["source_primary_key"], task["target_primary_key"]]
                    if task.get("select_expression", None):
                        table_param.append(task["select_expression"])
                    else:
                        table_param.append("*")
                else:
                    table_param += ["--table", task["source_schema"], task["source_table"]]

        stdout = ""

        if not self.copytable_path:
            raise RuntimeError("Path to wbcopytables not found")

        args = self.helper_basic_arglist(self._resume)

        if 'stimeout' in task:
            args.append('--source-timeout=%s' % task['stimeout'])
        if 'ttimeout' in task:
            args.append('--target-timeout=%s' % task['ttimeout'])
        if self._resume:
            args.append("--resume")

        argv = [self.copytable_path, "--count-only", "--passwords-from-stdin"] + args + table_param
        self._owner.send_info(" ".join(argv))

        if sys.platform == "win32":
            # shell=True causes a created window to be hidden by default, this prevents a popup to be shown
            # on the migration wizard
            out = subprocess.Popen(argv, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        else:
            out = subprocess.Popen(argv, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        if self._resume:
            passwords= (self._src_password+"\t"+self._tgt_password+"\n").encode("utf8") 
        else:               
            passwords= (self._src_password+"\n").encode("utf8")
        while out.poll() is None:
            o, e = out.communicate(passwords)
            passwords = None
            if o:
                stdout += o
            if e:
                for l in e.split("\n"):
                    self._owner.send_info(l)
            total = 0
        if out.returncode == 0:
            for schema, table, count in [l.split(":")[1:] for l in stdout.split("\n") if l.startswith("ROW_COUNT")]:
                count = int(count.strip())
                total += count
                working_set[schema+"."+table]["row_count"] = count
            return total
        else:
            self._owner.send_info(stdout)
            raise Exception("Error getting row count from source tables, wbcopytables exited with code %s" % out.returncode)

    def migrate_data(self, num_processes, working_set):
        table_param = []
        if sys.platform == "win32":
            try:
                with tempfile.NamedTemporaryFile("w", delete=False) as table_file:
                    for task in working_set.values():
                        fields = []
                        fields.append(task["source_schema"])
                        fields.append(task["source_table"])
                        fields.append(task["target_schema"])
                        fields.append(task["target_table"])
                        fields.append(task["source_primary_key"])
                        fields.append(task["target_primary_key"])
                        if task.get("select_expression", None):
                            fields.append(task["select_expression"])
                        else:
                            fields.append("*")
                        line = "\t".join(fields)
                        table_file.write(line + "\n")

                    table_param.append("--table-file=%s" % table_file.name)

            except IOError, e:
                raise Exception ("Error creating table file: %s" % e.strerror)
        else:
            for task in working_set.values():
                table_param += ["--table", task["source_schema"], task["source_table"], task["target_schema"], task["target_table"], task["source_primary_key"], task["target_primary_key"]]
                if task.get("select_expression", None):
                    table_param.append(task["select_expression"])
                else:
                    table_param.append("*")

        if len(working_set) < num_processes:
            num_processes = len(working_set)

        args = self.helper_basic_arglist(True)
        args += ["--progress", "--passwords-from-stdin"]

        if self._options.get("TruncateTargetTables", False):
            args.append("--truncate-target")
        if self._options.get("DebugTableCopy", False):
            args.append("--log-level=debug3")
        if self._options.get("DriverSendsDataAsUTF8", False):
            args.append("--force-utf8-for-source")

        args.append("--thread-count=" + str(num_processes));
        args.append('--source-rdbms-type=%s' % self._src_conn_object.driver.owner.name)
        if 'stimeout' in task:
            args.append('--source-timeout=%s' % task['stimeout'])
        if 'ttimeout' in task:
            args.append('--target-timeout=%s' % task['ttimeout'])

        if 'defaultCharSet' in self._src_conn_object.parameterValues.keys():
            default_charset = self._src_conn_object.parameterValues.get("defaultCharSet")
            if default_charset:
                args.append('--source-charset=%s' % default_charset)

        if self._resume:
            args.append("--resume")

        argv = [self.copytable_path] + args + table_param

        self._working_set = working_set
        self._result_queue = Queue.Queue(len(working_set))

        worker = TableCopyWorker(self._owner, argv, self._result_queue)
        worker.feed_input(self._src_password+"\t"+self._tgt_password+"\n")
        worker.start()
        results = self.process_until_done()
        worker.terminate()
        return results


    def helper_basic_arglist(self, include_target_conn):
        args = []
        if self._src_conn_object.driver.owner.name == "Mysql":
            args.append('--mysql-source="%s"' % mysql_conn_string(self._src_conn_object))
            if self._src_conn_object.parameterValues.get("OPT_ENABLE_CLEARTEXT_PLUGIN", False):
                args.append("--source-use-cleartext")
        elif (isinstance(self._src_conn_object.driver, grt.classes.db_mgmt_PythonDBAPIDriver) and
              self._src_conn_object.driver.driverLibraryName != 'pyodbc'):
            args.append('--pythondbapi-source="%s"' % python_conn_string(self._src_conn_object))
        else:
            args.append('--odbc-source="%s"' % odbc_conn_string(self._src_conn_object, True))

        if include_target_conn:
            args.append('--target="%s"' % mysql_conn_string(self._tgt_conn_object))
            if self._tgt_conn_object.parameterValues.get("OPT_ENABLE_CLEARTEXT_PLUGIN", False):
                args.append("--target-use-cleartext")

        return args


    def helper_connections_arglist(self):
        conn_args = { 'source_user': self._src_conn_object.parameterValues.get("userName", 'root'),
                      'source_instance': '',
                      'source_port': self._src_conn_object.parameterValues.get("port", 3306),
                      'target_port': self._tgt_conn_object.parameterValues.get("port", 3306),
                      'target_user': self._tgt_conn_object.parameterValues.get("userName", 'root'),
                      'source_rdbms':self._src_conn_object.driver.owner.name.lower()}

        return conn_args


    def process_until_done(self):
        total_row_count = 0
        for table in self._working_set.values():
            total_row_count += table["row_count"]

        progress_row_count = {}

        self.interrupted = False

        active_job_names = set()
        self._resume = False
        done = False

        while True:
            if done:
                # flush pending messages
                try:
                    _update_resume_status = getattr(self._owner, "_update_resume_status", None)
                    if callable(_update_resume_status):
                        _update_resume_status(self._resume)
                    msgtype, message = self._result_queue.get_nowait()
                except Queue.Empty:
                    break
            else:
                msgtype, message = self._result_queue.get()

            if msgtype == "BEGIN":
                target_table = message.split(":")[0]
                active_job_names.add(target_table)
                self._owner.send_info(message)
            elif msgtype == "END":
                target_table = message.split(":")[0]
                if target_table in active_job_names:
                    active_job_names.remove(target_table)
                self._owner.send_info(message)
                progress_row_count[target_table] = (True, progress_row_count.get(target_table, (False, 0))[1])

            elif msgtype == "ERROR":
                target_table = message.split(":")[0]
                if target_table in active_job_names:
                    active_job_names.remove(target_table)
                self._owner.send_error(message)

                self._owner.add_log_entry(2, target_table, message)
                grt.log_error("Migration", "%s\n"%message)
                self._resume = True

            elif msgtype == "PROGRESS":
                target_table, current, total = message.split(":")
                progress_row_count[target_table] = (False, int(current))
                self._owner.send_progress(float(sum([x[1] for x in progress_row_count.values()])) / total_row_count, "Copying %s" % ", ".join(active_job_names))
            elif msgtype == "LOG":
                self._owner.send_info(message)
            elif msgtype == "DONE":
                done = True
                if message:
                    self._resume = True
                    self._owner.send_error("Copy helper exited with an error: %s" % message)
                else:
                    self._owner.send_info("Copy helper has finished")
            elif msgtype == "INTERRUPTED":
                done = True
                self.interrupted = True
                self._resume = True
                self._owner.send_info("Copy helper was aborted by user")
            else:
                self._owner.send_info(msgtype + ": " + message)

        return progress_row_count
