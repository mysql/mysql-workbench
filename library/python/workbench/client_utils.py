# Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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


import platform
import sys
import os

import tempfile
import subprocess
try:
    import _subprocess
except ImportError:
    pass

from Queue import Queue, Empty
from threading import Thread
import mforms
import grt

from workbench.log import log_info, log_error, log_debug

from db_utils import ConnectionTunnel


def get_path_to_mysql():
    # get path to mysql client from options
    try:
        path = grt.root.wb.options.options["mysqlclient"]
        if path:
            if os.path.exists(path):
                return path
            if any(os.path.exists(os.path.join(p,path)) for p in os.getenv("PATH").split(os.pathsep)):
                return path
            return None
    except:
        return None
    
    if sys.platform.lower() == "darwin":
        # if path is not specified, use bundled one
        return mforms.App.get().get_executable_path("mysql").encode("utf8")
    elif sys.platform.lower() == "win32":
        return mforms.App.get().get_executable_path("mysql.exe").encode("utf8")
    else:
        # if path is not specified, use bundled one
        path = mforms.App.get().get_executable_path("mysql").encode("utf8")
        if path:
            return path
        # just pick default
        if any(os.path.exists(os.path.join(p,"mysql")) for p in os.getenv("PATH").split(os.pathsep)):
            return "mysql"
        return None


def start_reading_from(file):
    """Create a thread to read from the file object and feed a queue. Use for non-blocking reads from file objects."""

    def reader(file, q):
        while True:
            l = file.readline()
            if not l:
                break
            q.put(l)
        q.put(None)

    q = Queue()
    thr = Thread(target=reader, args=(file, q))
    thr.start()

    return q, thr


class MySQLScriptImporter(object):
    """Import a SQL script using the MySQL command line tool"""

    def __init__(self, connection_params):
        self._extra_params = []
        self._password = ""
    
        self._tool_path = get_path_to_mysql()
        self._upgrade_tool_path = ""
    
        self._tunnel = ConnectionTunnel(connection_params)

        conn = connection_params.parameterValues
        params = []
        if connection_params.driver.name == "MysqlNativeSocket":
            params.append("--protocol="+("pipe" if sys.platform == "win32" else "socket"))
            if conn["socket"]:
                params.append("--socket=" + conn["socket"])
        else:
            params.append("--protocol=tcp")
            if self._tunnel.port or conn["port"]:
                params.append("--port=" + str(self._tunnel.port or conn["port"]))
            if (self._tunnel.port and ["localhost"] or [conn["hostName"]])[0]:
                params.append("--host=" + (self._tunnel.port and ["localhost"] or [conn["hostName"]])[0])
        
        if conn.get("useSSL", 0):
            if conn.get("sslCert", ""):
                params.append("--ssl-cert=%s" % conn["sslCert"])
            if conn.get("sslCA", ""):
                params.append("--ssl-ca=%s" % conn["sslCA"])
            if conn.get("sslKey", ""):
                params.append("--ssl-key=%s" % conn["sslKey"])
            if conn.get("sslCipher", ""):
                params.append("--ssl-cipher=%s" % conn["sslCipher"])
                
        if conn.get("OPT_ENABLE_CLEARTEXT_PLUGIN", ""):
            params.append("--enable-cleartext-plugin")                
            
        params += ["--user=" + conn["userName"]]
        self._connection_params = params


    def set_extra_params(self, param_list):
        self._extra_params = param_list


    def set_password(self, password):
        self._password = password


    def report_progress(self, message, current, total):
        pass


    def report_output(self, text):
        print text

    def add_command_parameter(self, param_list, parameter, data = None):
        is_windows = platform.system() == 'Windows'

        if parameter == "command":
            param_list.append(data)
        elif parameter == "defaults-extra-file":
            if is_windows:
                pwdfile = tempfile.NamedTemporaryFile(delete=False, suffix=".cnf")
                pwdfilename = pwdfile.name
                tmpdir = None
            else:
                # use a pipe to feed the password to the client
                tmpdir = tempfile.mkdtemp()
                pwdfilename = os.path.join(tmpdir, 'extraparams.cnf')
                os.mkfifo(pwdfilename)
            param_list.append("--defaults-extra-file=%s" % pwdfilename)
            return tmpdir, pwdfilename
        elif parameter == "default-character-set" and data:
            param_list.append("--default-character-set=%s" % data)
        elif parameter == "connection-params":
            param_list += self._connection_params
        elif parameter == "extra-params":
            param_list += self._extra_params
        elif parameter == "default-schema" and data:
            param_list.append(data)

    def launch_process(self, command, working_directory):
        is_windows = platform.system() == 'Windows'
        real_command = ""
        info = None
        if is_windows:
            info = subprocess.STARTUPINFO()
            info.dwFlags |= _subprocess.STARTF_USESHOWWINDOW
            info.wShowWindow = _subprocess.SW_HIDE
            # Command line can contain object names in case of export and filename in case of import
            # Object names must be in utf-8 but filename must be encoded in the filesystem encoding,
            # which probably isn't utf-8 in windows.
            fse = sys.getfilesystemencoding()
            real_command = command.encode(fse) if isinstance(command, unicode) else command
        else:
            real_command = command
        
        try:
            log_debug("Executing command: %s\n" % real_command)
            proc = subprocess.Popen(real_command, cwd=working_directory, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT,startupinfo=info)
        except OSError, exc:
            log_error("Error executing command %s\n%s\n" % (real_command, exc))
            import traceback
            traceback.print_exc()
            raise RuntimeError("Error executing %s:\n%s" % (real_command, str(exc)))
        return proc


    def reset_schemas(self, default_schema=None, default_charset="utf8"):
        if not self._upgrade_tool_path:
            raise RuntimeError("You need to specify a valid location to mysql_upgrade tool.")

        command = []
        
        self.add_command_parameter(command, "command", self._upgrade_tool_path)
        tmpdir, pwdfilename = self.add_command_parameter(command, "defaults-extra-file")
        self.add_command_parameter(command, "default-character-set", default_charset)
        self.add_command_parameter(command, "connection-params")
        self.add_command_parameter(command, "extra-params")
        self.add_command_parameter(command, "default-schema", default_schema)
        command.append("--force")

        command_string = " ".join(command)
        
        proc = None
        try:
            self.report_progress("Preparing...", 0, 2)
            proc = self.launch_process(command, None)

            if self._password is None:
                self._password = ''
            pwdfile = open(pwdfilename, 'w')
            pwdfile.write('[client]\npassword=%s\n' % self._password.replace("\\", "\\\\"))
            pwdfile.close()

            line_count = 0
            for line in iter(proc.stdout.readline,''):
                if line.startswith('Checking'):
                    line_count += 1
                    self.report_progress(line, line_count, 2)

            proc.wait()
            self.report_progress("Finished...", 2, 2)
        except Exception, e:
            log_error("There was an exception running a process: %s\n%s" % (command, str(e)))
        finally:
            if pwdfilename:
                os.remove(pwdfilename)
            if tmpdir:
                os.rmdir(tmpdir)


                
                
    def import_script(self, path, default_schema=None, default_charset="utf8"):
        if not self._tool_path:
            raise RuntimeError("mysql command line client not found. Please fix its path in Preferences -> Administration")

        is_windows = platform.system() == 'Windows'
        
        if is_windows:
            params = ['"%s"' % self._tool_path]
            pwdfile = tempfile.NamedTemporaryFile(delete=False, suffix=".cnf")
            pwdfilename = pwdfile.name
            tmpdir = None
        else:
            params = [self._tool_path]
            # use a pipe to feed the password to the client
            tmpdir = tempfile.mkdtemp()
            pwdfilename = os.path.join(tmpdir, 'extraparams.cnf')
            os.mkfifo(pwdfilename)
        params.append('--defaults-extra-file=' + pwdfilename)

        if default_charset:
            params.append("--default-character-set=%s" % default_charset)
        params += self._connection_params
        params += self._extra_params

        if default_schema:
            params.append(default_schema)

        cmdstr = " ".join(params)

        workdir = os.path.dirname(path)

        log_info("Feeding data from %s to %s (cwd=%s)\n" % (path, cmdstr, workdir))
        p1 = None
        try:
            self.report_progress("Preparing...", None, None)
            if not is_windows:
                try:
                    p1 = subprocess.Popen(params, cwd=workdir, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                except OSError, exc:
                    log_error("Error executing command %s\n%s\n" % (" ".join(params), exc))
                    raise RuntimeError("Error executing %s:\n%s" % (" ".join(params), str(exc)))

            # in !Windows feed password to client after it's started (otherwise the fifo would block on open for writing)
            pwdfile = open(pwdfilename, 'w')
            pwdfile.write('[client]\npassword=')
            if self._password is None:
                self._password = ''
            pwdfile.write(self._password.replace("\\", "\\\\"))
            pwdfile.write('\n')
            pwdfile.close()

            if is_windows:
                try:
                    info = subprocess.STARTUPINFO()
                    info.dwFlags |= _subprocess.STARTF_USESHOWWINDOW
                    info.wShowWindow = _subprocess.SW_HIDE
                    # Command line can contain object names in case of export and filename in case of import
                    # Object names must be in utf-8 but filename must be encoded in the filesystem encoding,
                    # which probably isn't utf-8 in windows.
                    fse = sys.getfilesystemencoding()
                    cmd = cmdstr.encode(fse) if isinstance(cmdstr, unicode) else cmdstr
                    log_debug("Executing command: %s\n" % cmdstr)
                    p1 = subprocess.Popen(cmd, cwd=workdir, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT,startupinfo=info, shell=cmdstr[0] != '"')
                except OSError, exc:
                    log_error("Error executing command %s\n%s\n" % (cmdstr, exc))
                    import traceback
                    traceback.print_exc()
                    raise RuntimeError("Error executing %s:\n%s" % (cmdstr, str(exc)))

            # do the import
            total_size = os.stat(path).st_size
            processed = 0
            
            self.report_progress("Importing %s..." % os.path.basename(path), 0, total_size)
            stdout_q, thr = start_reading_from(p1.stdout)
           
            input_file = open(path, "r")
            while p1 and p1.poll() == None:
                try:
                    if stdout_q:
                        text = stdout_q.get_nowait()
                        if text:
                            log_info("Task stdout: %s\n" % text)
                            if 'Access denied for user' in text:
                                raise grt.DBLoginError(text)
                            elif "Can't open named pipe to host" in text and sys.platform.lower() == "win32":
                                text = "%s\n%s" % (text, "Please check if the server started with the --enabled-named-pipe parameter. The parameter can also be set in the config file.")
                            self.report_output(text.strip())
                        elif text is None:
                              stdout_q = None
                except Empty:
                    pass

                line = input_file.readline()
                if not line:
                    break
                processed += len(line)
                try:
                    p1.stdin.write(line)
                except IOError, e:
                    log_error("Exception writing to stdin from cmdline client: %s\n" % e)
                    if e.errno == 32: # broken pipe
                        log_error("Broken pipe from child process\n")
                        break
                    elif e.errno == 22: # invalid argument (happens in Windows, when child process just dies)
                      log_error("Broken pipe from child process\n")
                      break
                    raise e
                self.report_progress(None, processed, total_size)
                
            input_file.close()

            # close the writer end of the client's pipe, so it can exit
            p1.stdin.close()

            self.report_progress("Finished executing script", processed, total_size)
            
            # flush queue from reader
            if stdout_q:
                while True:
                    text = stdout_q.get()
                    if text is None:
                        break
                    log_info("Task stdout: %s\n" % text)
                    if 'Access denied for user' in text:
                        raise grt.DBLoginError(text)
                    elif "Can't open named pipe to host" in text and sys.platform.lower() == "win32":
                        text = "%s\n%s" % (text, "Please check if the server started with the --enabled-named-pipe parameter. The parameter can also be set in the config file.")
                    self.report_output(text.strip())

            # let reader thread die
            thr.join()

            p1.wait()

            exitcode = p1.returncode
            
            log_info("mysql tool exited with code %s\n" % exitcode)

            if exitcode != 0:
                self.report_progress("Operation failed with exitcode " + str(exitcode), None, None)
            else:
                self.report_progress("Operation completed successfully", None, None)
            
            return exitcode
        finally:
            if pwdfilename:
                os.remove(pwdfilename)
            if tmpdir:
                os.rmdir(tmpdir)





