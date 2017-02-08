# Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
import os
import posixpath
import ntpath
import errno
import threading
import tempfile
import StringIO
import pipes
import subprocess
import time
import inspect
import random
import string
import functools

# Declares the global variable for sudo prefix
default_sudo_prefix = ''

def reset_sudo_prefix():
    global default_sudo_prefix
    default_sudo_prefix       = '/usr/bin/sudo -k -S -p EnterPasswordHere'

reset_sudo_prefix()

from mforms import App
from workbench.utils import QueueFileMP
from wb_common import InvalidPasswordError, PermissionDeniedError, Users, sanitize_sudo_output, splitpath
from wb_admin_ssh import WbAdminSSH, ConnectionError
from wb_common import CmdOptions, CmdOutput

from workbench.log import log_info, log_warning, log_error, log_debug, log_debug2, log_debug3

from workbench.tcp_utils import CustomCommandListener
from workbench.os_utils import FileUtils, OSUtils, FunctionType

class wbaOS(object):
    unknown = "unknown"
    windows = "windows"
    linux   = "linux"
    darwin  = "darwin"

    def __setattr__(self, name, value):
        raise NotImplementedError

def quote_path(path):
    if path.startswith("~/"):
        # be careful to not quote shell special chars
        return '~/"%s"' % path[2:]
    else:
        return '"%s"' % path.replace('"', r'\"')

def quote_path_win(path):
    return '"%s"' % path.replace("/", "\\").replace('"', r'\"')


def wrap_for_sudo(command, sudo_prefix, as_user = Users.ADMIN, to_spawn = False):
    if not command:
        raise Exception("Empty command passed to execution routine")
        
    if not sudo_prefix:
        sudo_prefix = default_sudo_prefix

    if to_spawn:
        command += ' &'
        sudo_prefix += ' /usr/bin/nohup'
      
    # If as_user is the CURRENT then there's no need to sudo
    if as_user != Users.CURRENT:
        #sudo needs to use -u <user> for non admin
        if as_user != Users.ADMIN:
            sudo_user = "sudo -k -u %s" % as_user
            sudo_prefix = sudo_prefix.replace('sudo', sudo_user)
        if '/bin/sh' in sudo_prefix or '/bin/bash' in sudo_prefix:
            command = sudo_prefix + " \"" + command.replace('\\', '\\\\').replace('"', r'\"').replace('$','\\$') + "\""
        else:
            command = sudo_prefix + " /bin/bash -c \"" + command.replace('\\', '\\\\').replace('"', r'\"').replace('$','\\$') + "\""

    return command

###

# Function decorator for absolute path validation
def useAbsPath(param):
    def abs_path_validator(function):
        @functools.wraps(function)
        def decorated_function(*args, **kw):
            path_index = None
            try:
                path_index = function.__code__.co_varnames.index(param)
            except ValueError:
                # this implies an internal coding error and will just be logged
                # It means the programmer indicated to validate an unexisting parameter
                log_warning ('Error on path validation for function %s, using invalid parameter "%s"\n' % (function.__name__, param))
            
            # Only performs the actual validation when a valid parameter is set
            if path_index is not None:
                path_value = args[path_index]
                
                if type(path_value) not in [str, unicode] or not os.path.isabs(path_value):
                    raise ValueError('Error on path validation for function "%s", parameter "%s" must be an absolute path: %s' % (function.__name__, param, path_value))
                
            return function(*args, **kw)
        return decorated_function
    return abs_path_validator


class SSH(WbAdminSSH):
    def __init__(self, profile, password_delegate):
        self.mtx = threading.Lock()
        self.wrapped_connect(profile, password_delegate)

    def __del__(self):
        log_debug("Closing SSH connection\n")
        self.close()

    def get_contents(self, filename):
        self.mtx.acquire()
        try:
            ret = WbAdminSSH.get_contents(self, filename)
        finally:
            self.mtx.release()
        return ret

    def set_contents(self, filename, data, mode="w"):
        self.mtx.acquire()
        try:
            ret = WbAdminSSH.set_contents(self, filename, data, mode)
        finally:
            self.mtx.release()
        return ret

    def exec_cmd(self, cmd, as_user = Users.CURRENT, user_password = None, output_handler = None, read_size = 128, get_channel_cb = None, options = None):
        output   = None
        retcode  = None

        self.mtx.acquire()
        log_debug3('%s:exec_cmd(cmd="%s", sudo=%s)\n' % (self.__class__.__name__, cmd, str(as_user)) )
        try:
            (output, retcode) = WbAdminSSH.exec_cmd(self, cmd,
                                            as_user=as_user,
                                            user_password=user_password,
                                            output_handler=output_handler,
                                            read_size = read_size,
                                            get_channel_cb = get_channel_cb,
                                            options = options)
            log_debug3('%s:exec_cmd(): Done cmd="%s"\n' % (self.__class__.__name__, cmd) )
        finally:
            self.mtx.release()

        return (output, retcode)

##===================================================================================================
## Local command execution
def local_run_cmd_linux(command, as_user = Users.CURRENT, user_password=None, sudo_prefix=default_sudo_prefix, output_handler=None, output_timeout=15, options=None):
    # wrap cmd
    if as_user != Users.CURRENT:
        command = wrap_for_sudo(command, sudo_prefix, as_user)

    debug_run_cmd = False
    if debug_run_cmd:
        log_debug2("local_run_cmd_linux: %s (as %s)\n" % (command, as_user))

    script = command.strip(" ")
    if not script:
        return None
    script_to_log = script

    script = "cd; " + script + " ; exit $?"
    result = None

    def read_nonblocking(fd, size, timeout=0, return_on_newline=False):
        import select
        data = []
        t = time.time()
        while len(data) < size:
            try:
                r, _, _ = select.select([fd], [], [], timeout)
            except select.error, e:
                if e.args[0] == 4:
                    timeout -= time.time() - t
                    if timeout < 0:
                        break
                    continue
                raise
            if not r:
                break
            data.append(fd.read(1))
            if return_on_newline and data[-1] == "\n":
                break
        return "".join(data)

    def read_nonblocking_until_nl_or(proc, fd, text, timeout=0):
        import select
        data = ""
        t = time.time()
        while time.time() - t < timeout:
            try:
                r, _, _ = select.select([fd], [], [], timeout - (time.time() - t))
            except select.error, e:
                if e.args[0] == 4:
                    continue
                raise
            if not r:
                break
        
            new_byte = fd.read(1)
            
            if new_byte:
                data += new_byte
                
                if data.endswith(text) or "\n" in data:
                    ndata = sanitize_sudo_output(data)
                    if ndata != data:
                        data = ndata
                        continue
                    break
            elif proc.poll() is not None:
                break
                
        return data

    # script should already have sudo
    my_env = os.environ.copy()
    my_env['LANG'] = "C" # Force english locale 
    child = subprocess.Popen(["/bin/sh", "-c", script], bufsize=0,
                             stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             close_fds=True, env = my_env)

    expect_sudo_failure = False

    if as_user != Users.CURRENT: 
        # If sudo is being used, we need to input the password
        data = read_nonblocking_until_nl_or(child, child.stdout, "EnterPasswordHere", timeout=output_timeout)
        if data.endswith("EnterPasswordHere"):
            
            if not user_password:
                log_debug2("Password required for sudo, but user_password is empty, throwing exception.")
                child.terminate() # sudo need pw, but pw is empty, throw exception, 
                                  # it will be handled upper in the chain and proper pw dialog will be shown.
                raise InvalidPasswordError("Incorrect password for sudo")
            # feed the password
            if debug_run_cmd:
                log_debug2("local_run_cmd_linux: sending password to child...\n")
            child.stdin.write((user_password or " ")+"\n"+'\x1a')
            expect_sudo_failure = True # we could get a Sorry or the password prompt again
        else:
            # If the prompt didn't come in, it could mean that password is not required
            # or it could also mean that sudo printed some junk/banner message before sending over the prompt
            # Problem is, there's no way to tell whether the text coming in is the output from the program being executed
            # or stuff from sudo itself, until either the sudo finishes or the prompt is seen.
            # So to account for this case, we will buffer everything that comes until:
            # - until the sudo prompt is seen
            # - a certain number of lines are read from the pipe
            # - until a timeout occurs (a short one, since the banner shouldn't take very long to get printed)
            # - until the sudo terminates
            log_debug2("sudo prompt available, but it's not standard. Trying to parse.")
            max_lines_to_read_until_giving_up_waiting_for_sudo_prompt = 10
            num_seconds_to_wait_for_sudo_greeting_message_until_we_assume_prompt_wont_come = 1

            buffered_output = [data] # init with the data that came in initially
            start_time = time.time()
            while child.poll() is None:
                data = read_nonblocking_until_nl_or(child, child.stdout, "EnterPasswordHere", timeout=1)
                if data.endswith("EnterPasswordHere"):
                    if not user_password:
                        log_debug2("Password required for sudo, but user_password is empty, throwing exception.")
                        child.terminate() # sudo need pw, but pw is empty, throw exception, 
                                          # it will be handled upper in the chain and proper pw dialog will be shown.
                        raise InvalidPasswordError("Incorrect password for sudo")
                    log_info("Banner message from sudo for command %s:\n%s\n" % (script, "".join(buffered_output)))
                    buffered_output = None
                    # ok, so the stuff that came in until now is all garbage and the sudo prompt finally arrived
                    if debug_run_cmd:
                        log_debug2("local_run_cmd_linux: sending password to child, after receiving greeting from sudo...\n")
                    child.stdin.write((user_password or "")+"\n")
                    expect_sudo_failure = True # we could get a Sorry or the password prompt again
                    break
                else:
                    buffered_output.append(data)
                    if len(buffered_output) > max_lines_to_read_until_giving_up_waiting_for_sudo_prompt or \
                        time.time() - start_time < num_seconds_to_wait_for_sudo_greeting_message_until_we_assume_prompt_wont_come:
                        log_debug("local_run_cmd_linux: was expecting sudo password prompt, but it never came\n")
                        # ok, we assume the output that came in until now is all from the program and there's no sudo prompt
                        break

            if output_handler and buffered_output:
                for line in buffered_output:
                    output_handler(line)

    if debug_run_cmd:
        log_debug2("local_run_cmd_linux: waiting for data...\n")
    read_size = 40
    return_on_newline = True
    while child.poll() is None:
        #t = time.time()
        # read 1KB from stdout, timeout in 15s.. at first time, read just enough to see if password prompt is there again
        current_text = read_nonblocking(child.stdout, read_size, output_timeout, return_on_newline)
        return_on_newline = False
        read_size = 1024
        if debug_run_cmd:
            log_debug2("local_run_cmd_linux: %s: read %i bytes from child [%s...]\n" % (script_to_log, len(current_text), current_text[:50]))

        # If Password prompt shows up again, it means the password we tried earlier was wrong.. so raise an exception
        if expect_sudo_failure and (current_text.find("EnterPasswordHere") >= 0 or current_text.find("Sorry, try again") >= 0):
            child.terminate()
            raise InvalidPasswordError("Incorrect password for sudo")
        else:
            # Not the password prompt
            expect_sudo_failure = False
            if output_handler and current_text:
                output_handler(current_text)

    # Try to read anything left, wait exit
    try:
        current_text, _ = child.communicate()
        if current_text and output_handler:
            output_handler(current_text)
    except:
        pass
    result = child.returncode
    if debug_run_cmd:
        log_debug2("local_run_cmd_linux: child returned %s\n" % result)
    log_debug3('local_run_cmd_linux(): script="%s", ret=%s\n' % (script_to_log, result))
    return result


def local_run_cmd_windows(command, as_user=Users.CURRENT, user_password=None, sudo_prefix=None, output_handler=None, options=None):
    # wrap cmd
    retcode = 1

    if as_user != Users.CURRENT:

        # Starts the command execution listener
        listener = None

        # When the command output is needed, the command will be executed with the
        # helper script and the output listened on a socket connection
        if output_handler:
            # The TCPCommandListener is missing one thing: the way to turn it off
            # so can't be used yet
            #listener = TCPCommandListener(output_handler)
            listener = CustomCommandListener(output_handler)
            listener.start()
            
            helper_path = ntpath.join(os.getcwd(),App.get().get_resource_path("wbadminhelper.exe"))

            # Creates the command as a call to the bundled python 
            # - The path to the python helper which will receive additionally:
            #   - The port where the command listener is waiting for the helper response
            #   - The handshake to allow it to connect to the listener
            #   - The close_key so it can notify the listener when processing is done
            #   - The original command to be executed

            # Patch to ensure old OS calls get executed properly
            actual_command = command.split(' ')[0]
            if not actual_command in ['LISTDIR','GETFILE','GET_FREE_SPACE', 'CHECK_FILE_READABLE', 'CHECK_DIR_WRITABLE', 'CHECK_PATH_EXISTS', 'CREATE_DIRECTORY', 'CREATE_DIRECTORY_RECURSIVE', 'REMOVE_DIRECTORY', 'REMOVE_DIRECTORY_RECURSIVE','DELETE_FILE', 'COPY_FILE', 'GET_FILE_OWNER', 'GETFILE_LINES', 'EXEC']:
                command = "EXEC " + command
            cmdname = helper_path
            cmdparams = '%d %s %s %s' % (listener.port, listener.handshake, listener.close_key, command)
        else:
            cmdname = "cmd.exe"
            cmdparams = "/C" + command

        command = "%s %s" % (cmdname, cmdparams)

        try:
            from ctypes import c_int, WINFUNCTYPE, windll
            from ctypes.wintypes import HWND, LPCSTR, UINT
            prototype = WINFUNCTYPE(c_int, HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, UINT)

            paramflags = (1, "hwnd", 0), (1, "operation", "runas"), (1, "file", cmdname), (1, "params", cmdparams), (1, "dir", None), (1, "showcmd", 0)
            SHellExecute = prototype(("ShellExecuteA", windll.shell32), paramflags)
            ret = SHellExecute()

            # If the user chooses to not allow privilege elevation for the operation
            # a PermissionDeniedError is launched
            if ret == 5:
                raise PermissionDeniedError("User did not accept privilege elevation")

            # Waits till everything has been received from the command
            if listener and ret != 5:
                listener.join()

                if listener.exit_status:
                    try:
                        helper_exception = eval(listener.exit_message)
                    except Exception, e:
                        # Some networking exceptions can't be evaluated
                        # So a runtime exception will be created on those cases
                        helper_exception = RuntimeError(listener.exit_message)
                    log_error("Exception received from Windows command helper executing %s %s: %s\n" % (cmdname, cmdparams, helper_exception))
                    raise helper_exception

            # > 32 is OK, < 32 is error code
            
            retcode = 1
            if ret > 32:
                retcode = 0
            else:
                if ret == 0:
                    log_error('local_run_cmd_windows(): Out of memory executing "%s"\n' % command)
                else:
                    log_error('local_run_cmd_windows(): Error %i executing "%s"\n' % (ret, command) )
            return retcode
        except Exception, e:
          # These errors will contain information probably sent by the helper so
          # they will be rethrow so they get properly displayed
          raise
    else:
        try:
            retcode = OSUtils.exec_command(command, output_handler)
        except Exception, e:
            import traceback
            log_error("Exception executing local command: %s: %s\n%s\n" % (command, e, traceback.format_exc()))
            retcode = 1
            #out_str = "Internal error: %s" % e

    return retcode


if platform.system() == "Windows":
    local_run_cmd = local_run_cmd_windows
else:
    local_run_cmd = local_run_cmd_linux

def local_get_cmd_output(command, as_user=Users.CURRENT, user_password=None):
    output = []
    output_handler = lambda line, l=output: l.append(line)
    rc = local_run_cmd(command=command, as_user=as_user, user_password=user_password, sudo_prefix=None, output_handler=output_handler)
    return ("\n".join(output), rc)

##===================================================================================================
## Process Execution


_process_ops_classes = []


class ProcessOpsBase(object):
    cmd_output_encoding = ""

    def __init__(self, **kwargs):
        pass

    def post_init(self):
        pass

    def expand_path_variables(self, path):
        return path

    def get_cmd_output(self, command, as_user=Users.CURRENT, user_password=None):
        output = []
        output_handler = lambda line, l=output: l.append(line)
        rc = self.exec_cmd(command, as_user, user_password, output_handler)
        return ("\n".join(output), rc)

    def list2cmdline(self, args):
        return None


class ProcessOpsNope(ProcessOpsBase):
    @classmethod
    def match(cls, (host, target, connect)):
        return connect == 'none'

    def expand_path_variables(self, path):
        return path

    def exec_cmd(self, command, as_user=Users.CURRENT, user_password=None, output_handler=None, options=None):
        return None

    def spawn_process(self, command, as_user=Users.CURRENT, user_password=None, output_handler=None, options=None):
        raise NotImplementedError("%s must implement spawn_process" % self.__class__.__name__)

    def get_cmd_output(self, command, as_user=Users.CURRENT, user_password=None):
        return ("", None)

_process_ops_classes.append(ProcessOpsNope)


class ProcessOpsLinuxLocal(ProcessOpsBase):
    @classmethod
    def match(cls, (host, target, connect)):
        return connect == 'local' and (host in (wbaOS.linux, wbaOS.darwin) and target in (wbaOS.linux, wbaOS.darwin))

    def __init__(self, **kwargs):
        ProcessOpsBase.__init__(self, **kwargs)
        self.sudo_prefix= kwargs.get("sudo_prefix", default_sudo_prefix)

    def exec_cmd(self, command, as_user=Users.CURRENT, user_password=None, output_handler=None, options = None):
        return local_run_cmd_linux(command, as_user, user_password, self.sudo_prefix, output_handler, 15, options)

    def spawn_process(self, command, as_user=Users.CURRENT, user_password=None, output_handler=None, options=None):
        
        sudo_prefix = self.sudo_prefix
        if options and options.has_key(CmdOptions.CMD_HOME):
            sudo_prefix = "%s HOME=%s" % (sudo_prefix, options[CmdOptions.CMD_HOME])
        
        # wrap cmd
        if as_user != Users.CURRENT:
            command = wrap_for_sudo(command, sudo_prefix, as_user, True)
      
        script = command.strip(" ")
        if script is None or len(script) == 0:
            return None

        # Creates the process
        process = subprocess.Popen(script, stdin=subprocess.PIPE, shell=True, close_fds=True)
  
        # Passes the password if needed
        if as_user != Users.CURRENT:
            process.stdin.write(user_password + "\n")
            process.stdin.flush()
  
        return 0

    def list2cmdline(self, args):
        return " ".join([pipes.quote(a) or "''" for a in args])


_process_ops_classes.append(ProcessOpsLinuxLocal)


class ProcessOpsLinuxRemote(ProcessOpsBase):
    @classmethod
    def match(cls, (host, target, connect)):
        # host doesn't matter
        return connect == 'ssh' and target in (wbaOS.linux, wbaOS.darwin)

    def __init__(self, **kwargs): # Here should be at least commented list of args
        ProcessOpsBase.__init__(self, **kwargs)

        self.sudo_prefix= kwargs.get("sudo_prefix", default_sudo_prefix)
        self.ssh = kwargs["ssh"]

    def exec_cmd(self, command, as_user=Users.CURRENT, user_password=None, output_handler=None, options=None):
        #if not self.ssh:
        #    raise Exception("No SSH session active")

        if as_user != Users.CURRENT:
            command = wrap_for_sudo(command, self.sudo_prefix, as_user)

        def ssh_output_handler(chunk, handler):
            if "EnterPasswordHere" in chunk and as_user != Users.CURRENT:
                raise InvalidPasswordError("Invalid password for sudo")
            if chunk is not None and chunk != "":
                handler(chunk)
            
        if output_handler:
            handler = lambda chunk, h=output_handler: ssh_output_handler(chunk, h)
        else:
            handler = None

        if self.ssh:
            # output_handler taken by ssh.exec_cmd is different from the one used elsewhere
            dummy_text, ret = self.ssh.exec_cmd(command,
                    as_user=as_user, user_password=user_password,
                    output_handler=handler, options=options)
        else:
            ret = 1
            if output_handler:
                output_handler("No SSH connection is active")
            else:
                print("No SSH connection is active")
                log_info('No SSH connection is active\n')

        return ret

    def spawn_process(self, command, as_user=Users.CURRENT, user_password=None, output_handler=None, options=None):
        if as_user != Users.CURRENT:
            # options has been introduced to allow customizing the HOME folder for the
            # user spawning the process.
            # This is required to prevent nohup raising a Permission Denied error trying
            # to write the nohup.out on a non writable folder
            sudo_prefix = self.sudo_prefix
            if options and options.has_key(CmdOptions.CMD_HOME):
                sudo_prefix = "%s HOME=%s" % (sudo_prefix, options[CmdOptions.CMD_HOME])
            
            command = wrap_for_sudo(command, sudo_prefix, as_user, True)
        
        def ssh_output_handler(chunk, handler):
            if "EnterPasswordHere" in chunk and as_user != Users.CURRENT:
              raise InvalidPasswordError("Invalid password for sudo")
            if chunk is not None and chunk != "":
              handler(chunk)
        
        if output_handler:
          handler = lambda chunk, h=output_handler: ssh_output_handler(chunk, h)
        else:
          handler = None
        
        if self.ssh:
          # output_handler taken by ssh.exec_cmd is different from the one used elsewhere
          dummy_text, ret = self.ssh.exec_cmd(command,
                                              as_user=as_user, user_password=user_password,
                                              output_handler=handler, options = options)
        else:
            ret = 1
            if output_handler:
                output_handler("No SSH connection is active")
            else:
                print("No SSH connection is active")
                log_info('No SSH connection is active\n')
        
        return ret


    def list2cmdline(self, args):
        return " ".join([pipes.quote(a) or "''" for a in args])

_process_ops_classes.append(ProcessOpsLinuxRemote)




WIN_REG_QUERY_PROGRAMFILES = 'reg query HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion /v "ProgramFilesDir"'
WIN_REG_QUERY_PROGRAMFILES_x86 = 'reg query HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion /v "ProgramFilesDir (x86)"'

WIN_PROGRAM_FILES_VAR = "%ProgramFiles%"
WIN_PROGRAM_FILES_X86_VAR = "%ProgramFiles(x86)%"
WIN_PROGRAM_FILES_X64_VAR = "%ProgramW6432%"
WIN_PROGRAM_DATA_VAR = "%ProgramData%"


class ProcessOpsWindowsLocal(ProcessOpsBase):
    @classmethod
    def match(cls, (host, target, connect)):
        return (host == wbaOS.windows and target == wbaOS.windows and connect in ('wmi', 'local'))

    def __init__(self, **kwargs):
        ProcessOpsBase.__init__(self, **kwargs)
        self.target_shell_variables = {}

    def post_init(self):
        self.fetch_windows_shell_info()

    def exec_cmd(self, command, as_user, user_password, output_handler=None, options = None):
        return local_run_cmd_windows(command, as_user, user_password, None, output_handler, options)

    def spawn_process(self, command, as_user=Users.CURRENT, user_password=None, output_handler=None, options=None):
        try:
            DETACHED_PROCESS = 0x00000008
            subprocess.Popen(command, shell=True, close_fds = True, creationflags=DETACHED_PROCESS)
            #process = subprocess.Popen(command, stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.STDOUT, shell=True)
        except Exception, exc:
            import traceback
            log_error("Error executing local Windows command: %s: %s\n%s\n" % (command, exc, traceback.format_exc()))
            #out_str = "Internal error: %s"%exc

    def expand_path_variables(self, path):
        """
        Expand some special variables in the path, such as %ProgramFiles% and %ProgramFiles(x86)% in
        Windows. Uses self.target_shell_variables for the substitutions, which should have been
        filled when the ssh connection to the remote host was made.
        """
        for k, v in self.target_shell_variables.iteritems():
            path = path.replace(k, v)
        return path

    def fetch_windows_shell_info(self):
        # get some info from the remote shell
        result, code = self.get_cmd_output("chcp.com")
        if code == 0:
            result = result.strip(" .\r\n").split()
            if len(result) > 0:
                  self.cmd_output_encoding = "cp" + result[-1]
        else:
            log_warning('%s.fetch_windows_shell_info(): WARNING: Unable to determine codepage from shell: "%s"\n' % (self.__class__.__name__, str(result)) )

            # some ppl don't have the system32 dir in PATH for whatever reason, check if that's the case
            _, code = self.get_cmd_output("ver")
            if code != 0:
                # assume this is not Windows
                raise RuntimeError("Target host is configured as Windows, but seems to be a different OS. Please review the connection settings.")
            raise RuntimeError("Unable to execute command chcp. Please make sure that the C:\\Windows\\System32 directory is in your PATH environment variable.")

        result, code = self.get_cmd_output("echo %PROCESSOR_ARCHITECTURE%")
        if result:
            result = result.strip()

        ProgramFilesVar = None
        x86var = None
        if result != "x86":#we are on x64 win in x64 mode
            x86var = WIN_PROGRAM_FILES_X86_VAR
            ProgramFilesVar = WIN_PROGRAM_FILES_VAR
        else:
            result, code = self.get_cmd_output("echo %PROCESSOR_ARCHITEW6432%")
            if result:
                result = result.strip()
            if result == "%PROCESSOR_ARCHITEW6432%":#we are on win 32
                x86var = WIN_PROGRAM_FILES_VAR
                ProgramFilesVar = WIN_PROGRAM_FILES_VAR
            else:#32bit app on x64 win
                x86var = WIN_PROGRAM_FILES_VAR
                ProgramFilesVar = WIN_PROGRAM_FILES_X64_VAR

        result, code = self.get_cmd_output("echo "+ ProgramFilesVar)
        if code == 0:
            self.target_shell_variables["%ProgramFiles%"] = result.strip("\r\n")
            if ProgramFilesVar != "%ProgramFiles%":
                self.target_shell_variables[ProgramFilesVar] = result.strip("\r\n")
        else:
            print "WARNING: Unable to fetch ProgramFiles value in Windows machine: %s"%result
            log_warning('%s.fetch_windows_shell_info(): WARNING: Unable to fetch ProgramFiles value in Windows machine: "%s"\n' % (self.__class__.__name__, str(result)) )

        # this one only exists in 64bit windows
        result, code = self.get_cmd_output("echo "+ x86var)
        if code == 0:
            self.target_shell_variables["%ProgramFiles(x86)%"] = result.strip("\r\n")
        else:
            print "WARNING: Unable to fetch ProgramFiles(x86) value in local Windows machine: %s"%result
            log_warning('%s.fetch_windows_shell_info(): WARNING: Unable to fetch ProgramFiles(x86) value in local Windows machine: "%s"\n' % (self.__class__.__name__, str(result)) )

        # Fetches the ProgramData path
        result, code = self.get_cmd_output("echo "+ WIN_PROGRAM_DATA_VAR)
        if code == 0:
            self.target_shell_variables[WIN_PROGRAM_DATA_VAR] = result.strip("\r\n")
        else:
            # If not found, it will use the %ProgramFiles% variable value
            self.target_shell_variables[WIN_PROGRAM_DATA_VAR] = self.target_shell_variables[ProgramFilesVar]
            print "WARNING: Unable to fetch ProgramData value in local Windows machine: %s, using ProgramFiles path instead: %s" % (result, self.target_shell_variables[WIN_PROGRAM_DATA_VAR])
            log_warning('%s.fetch_windows_shell_info(): WARNING: Unable to fetch ProgramData value in local Windows machine: "%s"\n' % (self.__class__.__name__, str(result)) )

        log_debug('%s.fetch_windows_shell_info(): Encoding: "%s", Shell Variables: "%s"\n' % (self.__class__.__name__, self.cmd_output_encoding, str(self.target_shell_variables)))

    def list2cmdline(self, args):
          return subprocess.list2cmdline(args)


_process_ops_classes.append(ProcessOpsWindowsLocal)


class ProcessOpsWindowsRemoteSSH(ProcessOpsWindowsLocal):
    @classmethod
    def match(cls, (host, target, connect)):
        # host doesn't matter
        return (target == wbaOS.windows and connect == 'ssh')

    def __init__(self, **kwargs):
        ProcessOpsWindowsLocal.__init__(self, **kwargs)

        self.ssh = kwargs["ssh"]


    def post_init(self):
        if self.ssh:
            self.fetch_windows_shell_info()


    def exec_cmd(self, command, as_user=Users.CURRENT, user_password=None, output_handler=None, options=None):
        command = "cmd.exe /c " + command

        if not self.ssh:
            raise Exception("No SSH session active")

        def ssh_output_handler(chunk, handler):
            if chunk is not None and chunk != "":
                handler(chunk)
            #else:
            #    loop = False

        if output_handler:
            handler = lambda chunk, h=output_handler: ssh_output_handler(chunk, h)
        else:
            handler = None

        # output_handler taken by ssh.exec_cmd is different from the one used elsewhere
        dummy_text, ret = self.ssh.exec_cmd(command,
                as_user=as_user, user_password=user_password,
                output_handler=handler, options=options)
        return ret


    def spawn_process(self, command, as_user=Users.CURRENT, user_password=None, output_handler=None, options=None):
        raise NotImplementedError("%s must implement spawn_process" % self.__class__.__name__)


    def list2cmdline(self, args):
          return subprocess.list2cmdline(args)

_process_ops_classes.append(ProcessOpsWindowsRemoteSSH)



##===================================================================================================
## File Operations

_file_ops_classes = []

class FileOpsNope(object):
    @classmethod
    def match(cls, target_os, connection_method):
        return connection_method == "none"

    def __init__(self, process_ops, ssh = None, target_os = None):
        pass

    def save_file_content(self, filename, content, as_user = Users.CURRENT, user_password = None, mode = None):
        pass

    def save_file_content_and_backup(self, filename, content, backup_extension, as_user = Users.CURRENT, user_password = None, mode = None):
        pass

    def get_file_content(self, filename, as_user = Users.CURRENT, user_password = None, skip_lines=0):
        return ""

    def _copy_file(self, source, dest, as_user = Users.CURRENT, user_password = None): # not used externally
        pass

    def check_file_readable(self, path, as_user=Users.CURRENT, user_password=None):
        return False

    def check_path_exists(self, path, as_user=Users.CURRENT, user_password=None):
        return False

    def check_dir_writable(self, path, as_user=Users.CURRENT, user_password=None):
        return False

    def file_exists(self, path, as_user = Users.CURRENT, user_password = None):
        return False

    def get_file_owner(self, path, as_user = Users.CURRENT, user_password = None):
        return False
        
    def create_directory(self, path, as_user = Users.CURRENT, user_password = None, with_owner=None):
        pass

    def create_directory_recursive(self, path, as_user = Users.CURRENT, user_password = None, with_owner=None):
        pass

    def get_available_space(self, path, as_user = Users.CURRENT, user_password = None):
        return False

    # Return format is list of entries in dir (directories go first, each dir name is follwoed by /)
    def listdir(self, path, as_user = Users.CURRENT, user_password = None, include_size=False): # base operation to build file_exists and remote file selector
        return []

    def get_owner(self, path): # base operation to build file_exists and remote file selector
        return []
      
    def join_paths(self, path, *paths):
        pass
_file_ops_classes.append(FileOpsNope)

class FileOpsLinuxBase(object):
    def __init__(self, process_ops, ssh=None, target_os = None):
        self.process_ops = process_ops
        self.ssh = ssh
        self.target_os = target_os
        
    # Exception Handling will vary on local and remote
    def raise_exception(self, message, custom_messages = {}):
        raise Exception(message)

    @useAbsPath("path")
    def check_path_exists(self, path, as_user=Users.CURRENT, user_password=None):
        res = self.process_ops.exec_cmd('test -d ' + quote_path(path),
                            as_user,
                            user_password,
                            output_handler = lambda line:None,
                            options={CmdOptions.CMD_WAIT_OUTPUT:CmdOutput.WAIT_NEVER})
        return res == 0

    @useAbsPath("filename")
    def file_exists(self, filename, as_user=Users.CURRENT, user_password=None):
        res = self.process_ops.exec_cmd('test -e ' + quote_path(filename),
                            as_user,
                            user_password,
                            output_handler = lambda line:None,
                            options={CmdOptions.CMD_WAIT_OUTPUT:CmdOutput.WAIT_NEVER})
        return res == 0
    
    @useAbsPath("path")
    def get_available_space(self, path, as_user=Users.CURRENT, user_password=None):
        output = StringIO.StringIO()
        res = self.process_ops.exec_cmd("LC_ALL=C df -Ph %s" % quote_path(path),
                            as_user,
                            user_password,
                            output_handler = output.write)

        output = sanitize_sudo_output(output.getvalue()).strip()
        
        available = "Could not determine"
        if res == 0:
            tokens = output.split("\n")[-1].strip().split()
            available = "%s of %s available" % (tokens[3], tokens[1])
        
        return available
        
    @useAbsPath("path")
    def get_file_owner(self, path, as_user = Users.CURRENT, user_password = None):
        if self.target_os == wbaOS.linux:
          command = 'LC_ALL=C stat -c %U '
        else:
          command = 'LC_ALL=C /usr/bin/stat -f "%Su" '
      
        output = StringIO.StringIO()
        command = command + quote_path(path)
        
        res = self.process_ops.exec_cmd(command,
                            as_user,
                            user_password,
                            output_handler= output.write)
            
        output = sanitize_sudo_output(output.getvalue()).strip()
        if res != 0:
            self.raise_exception(output)
        
        return output      
    
    @useAbsPath("path")
    def create_directory(self, path, as_user = Users.CURRENT, user_password = None, with_owner=None):
        output = StringIO.StringIO()
        if with_owner:
            # Chown is usually restricted to the root user
            if as_user == Users.CURRENT:
                raise PermissionDeniedError("Cannot set owner of directory %s" % path)        
            else:
                command = "/bin/mkdir %s && chown %s %s" % (quote_path(path), with_owner, quote_path(path))
        else:
            command = "/bin/mkdir %s" % (quote_path(path))
            
        res = self.process_ops.exec_cmd(command,
                                        as_user   = as_user,
                                        user_password = user_password,
                                        output_handler = output.write,
                                        options={CmdOptions.CMD_WAIT_OUTPUT:CmdOutput.WAIT_IF_FAIL})

        if res != 0:
            output = sanitize_sudo_output(output.getvalue()).strip()
            self.raise_exception(output)      
    
    @useAbsPath("path")
    def create_directory_recursive(self, path, as_user = Users.CURRENT, user_password = None, with_owner=None):
        head, tail = splitpath(path)
        if not tail:
            head, tail = splitpath(head)
        if head and tail and not self.file_exists(head):
            try:
                self.create_directory_recursive(head, as_user, user_password, with_owner)
            except OSError, e:
                if e.errno != errno.EEXIST:
                    raise

        self.create_directory(path, as_user, user_password, with_owner)

    @useAbsPath("path")
    def remove_directory(self, path, as_user = Users.CURRENT, user_password = None):
        output = StringIO.StringIO()
        res = self.process_ops.exec_cmd('/bin/rmdir ' + quote_path(path),
                                        as_user   = as_user,
                                        user_password = user_password,
                                        output_handler = output.write,
                                        options={CmdOptions.CMD_WAIT_OUTPUT:CmdOutput.WAIT_IF_FAIL})

        if res != 0:
            output = sanitize_sudo_output(output.getvalue()).strip()
            self.raise_exception(output)

    @useAbsPath("path")
    def remove_directory_recursive(self, path, as_user = Users.CURRENT, user_password = None):
        output = StringIO.StringIO()
        res = self.process_ops.exec_cmd('/bin/rm -R ' + quote_path(path),
                                        as_user   = as_user,
                                        user_password = user_password,
                                        output_handler = output.write,
                                        options={CmdOptions.CMD_WAIT_OUTPUT:CmdOutput.WAIT_IF_FAIL})

        if res != 0:
            output = sanitize_sudo_output(output.getvalue()).strip()
            self.raise_exception(output)

    @useAbsPath("path")
    def delete_file(self, path, as_user = Users.CURRENT, user_password = None):
        output = StringIO.StringIO()
        res = self.process_ops.exec_cmd("/bin/rm " + quote_path(path),
                                        as_user   = as_user,
                                        user_password = user_password,
                                        output_handler = output.write,
                                        options = {CmdOptions.CMD_WAIT_OUTPUT:CmdOutput.WAIT_IF_FAIL})

        if res != 0:
            output = sanitize_sudo_output(output.getvalue()).strip()
            self.raise_exception(output)

    @useAbsPath("filename")
    def get_file_content(self, filename, as_user = Users.CURRENT, user_password = None, skip_lines=0): # may raise IOError
        command = ''
        output = StringIO.StringIO()

        if skip_lines == 0:
            command = 'LC_ALL=C cat %s' % quote_path(filename)
        else:
            command = 'LC_ALL=C tail -n+%d %s' % (skip_lines+1, quote_path(filename))

        res = self.process_ops.exec_cmd(command,
                                        as_user   = as_user,
                                        user_password = user_password,
                                        output_handler = output.write)

                                        
        output = sanitize_sudo_output(output.getvalue()).strip()
        
        if res != 0:
            self.raise_exception(output)

        return output
        
    @useAbsPath("path")
    def _create_file(self, path, content):
        try:
            f = open(path, 'w')
            f.write(content)
            f.close()
        except (IOError, OSError), err:
            if err.errno == errno.EACCES:
                raise PermissionDeniedError("Could not open file %s for writing" % path)
            raise err
        
        
    def _copy_file(self, source, dest, as_user = Users.CURRENT, user_password = None):
        output = StringIO.StringIO()
        
        res = self.process_ops.exec_cmd("LC_ALL=C /bin/cp " + quote_path(source) + " " + quote_path(dest),
                      as_user   = as_user,
                      user_password = user_password,
                      output_handler = output.write,
                      options = {CmdOptions.CMD_WAIT_OUTPUT:CmdOutput.WAIT_IF_FAIL})

        if res != 0:
            output = sanitize_sudo_output(output.getvalue()).strip()
            self.raise_exception(output)
    
    @useAbsPath("path")
    def check_file_readable(self, path, as_user=Users.CURRENT, user_password=None):
        ret_val = True
        
        output = StringIO.StringIO()
        
        path = quote_path(path)
        command = "test -e %s;_fe=$?;test -f %s;_fd=$?;test -r %s;echo $_fe$_fd$?" % (path, path, path)
        self.process_ops.exec_cmd(command,
                                  as_user,
                                  user_password,
                                  output_handler = output.write)

        # The validation will depend on the output and not the returned value
        output = sanitize_sudo_output(output.getvalue()).strip()
      
        log_debug2('check_file_readable :%s %s\n' % (len(output), output))
        
        if len(output) == 3:
            if output[0] == '1':
                raise OSError(errno.ENOENT, 'The path "%s" does not exist' % path)
            elif output[1] == '1':
                raise OSError(errno.ENOTDIR, 'The path "%s" is not a regular file' % path)
            elif output[2] == '1':
                ret_val = False
        else:
          raise Exception('Unable to verify file is readable : %s' % output)

        return ret_val    
    @useAbsPath("path")
    def check_dir_writable(self, path, as_user=Users.CURRENT, user_password=None):
        ret_val = True
        
        output = StringIO.StringIO()
        
        path = quote_path(path)
        command = "test -e %s;_fe=$?;test -d %s;_fd=$?;test -w %s;echo $_fe$_fd$?" % (path, path, path)
        self.process_ops.exec_cmd(command,
                                  as_user,
                                  user_password,
                                  output_handler = output.write)

        # The validation will depend on the output and not the returned value
        output = sanitize_sudo_output(output.getvalue()).strip()
      
        log_debug('check_dir_writable :%s %s\n' % (len(output), output))
        
        if len(output) == 3:
            if output[0] == '1':
                raise OSError(errno.ENOENT, 'The path "%s" does not exist' % path)
            elif output[1] == '1':
                raise OSError(errno.ENOTDIR, 'The path "%s" is not a directory' % path)
            elif output[2] == '1':
                ret_val = False
        else:
          raise Exception('Unable to verify directory is writable : %s' % output)

        return ret_val        
    
    @useAbsPath("path")
    def listdir(self, path, as_user = Users.CURRENT, user_password = None, include_size=False): 
        file_list = []
        if include_size:
            # for ls -l, the output format changes depending on stdout being a terminal or not
            # since both cases are possible, we need to handle both at the same time (1st line being total <nnnn> or not)
            # the good news is that if the line is there, then it will always start with total, regardless of the locale
            command = 'LC_ALL=C /bin/ls -l -p %s' % quote_path(path)
        else:
            command = 'LC_ALL=C /bin/ls -1 -p %s' % quote_path(path)
            
        output = StringIO.StringIO()
        res = self.process_ops.exec_cmd(command,
                                        as_user,
                                        user_password,
                                        output_handler = output.write)
        output = sanitize_sudo_output(output.getvalue().strip())
        
        if res != 0:
            custom_messages = {
                                errno.ENOENT:"The path \"%s\" does not exist" % path,
                                errno.ENOTDIR:"The path \"%s\" is not a directory" % path,
                                errno.EACCES:"Permission denied accessing %s" % path
                              }
        
            self.raise_exception(output, custom_messages)
        else:
            try:
                if include_size:
                    file_list = [(f, int(s)) for _, _, _, _, s, _, _, _, f in [s.strip().split(None, 8) for s in output.split("\n") if not s.startswith('total')]]
                else:
                    file_list = [s.strip() for s in output.split("\n")]
                    
            except Exception, e:
                log_error("%s: Could not parse output of remote ls %s command: '%s'\n"% (e, path, output))
    
        return file_list        
        
    def save_file_content(self, filename, content, as_user = Users.CURRENT, user_password = None, mode = None):        
        pass
        
    def _set_file_content(self, path, content):
        pass
        
    def _create_temp_file(self, content):
        tmp = tempfile.NamedTemporaryFile(delete = False)
        tmp_name = tmp.name

        try:
            log_debug('%s: Writing file contents to tmp file "%s"\n' %  (self.__class__.__name__, tmp_name) )
            tmp.write(content)
            tmp.flush()
        except Exception, exc:
            log_error('%s: Exception caught: %s\n' % (self.__class__.__name__, str(exc)) )
            if tmp:
                tmp.close()
            raise
            
        return tmp_name
        

    @useAbsPath("filename")
    def save_file_content_and_backup(self, filename, content, backup_extension, as_user = Users.CURRENT, user_password = None, mode = None):
        log_debug('%s: Saving file "%s" with backup (sudo="%s")\n' % (self.__class__.__name__, filename, str(as_user)) )
        # Checks if the target folder is writable
        target_dir = posixpath.split(filename)[0]
        
        if self.check_dir_writable(target_dir, as_user, user_password):
            # Creates a backup of the existing file... if any
            if backup_extension and self.file_exists(filename, as_user, user_password):
                log_debug('%s: Creating backup of "%s" to "%s"\n' %  (self.__class__.__name__, filename, filename+backup_extension))
                
                self._copy_file(source = filename, dest = filename + backup_extension,
                                as_user = as_user, user_password = user_password)
                                    
            if as_user != Users.CURRENT:
                temp_file = self._create_temp_file(content)
                log_debug('%s: Wrote file contents to tmp file "%s"\n' %  (self.__class__.__name__, temp_file) )
                
                         
                log_debug('%s: Copying over tmp file to final filename using sudo: %s -> %s\n' % (self.__class__.__name__, temp_file, filename) )
                
                self._copy_file(source = temp_file, dest = filename, 
                                as_user = Users.ADMIN, user_password = user_password)
                    
                log_debug('%s: Copying file done\n' % self.__class__.__name__)
                
                # If needed changes the ownership of the new file to the requested user
                if as_user != Users.ADMIN:
                
                    # TODO: Does this need any validation being executed by root??
                    self.process_ops.exec_cmd("chown %s %s" % (as_user, quote_path(filename)),
                                  as_user   = Users.ADMIN,
                                  user_password = user_password,
                                  output_handler = None,
                                  options = {CmdOptions.CMD_WAIT_OUTPUT:CmdOutput.WAIT_IF_FAIL})
                                  
                self.delete_file(temp_file)
            else:
                log_debug('%s: Saving file...\n' % self.__class__.__name__)
                self._create_file(filename, content)

            if mode:
                self.process_ops.exec_cmd("chmod %s %s" % (mode, quote_path(filename)),
                                as_user   = Users.ADMIN,
                                user_password = user_password,
                                output_handler = None,
                                options = {CmdOptions.CMD_WAIT_OUTPUT:CmdOutput.WAIT_IF_FAIL})
                    
        else:
            raise PermissionDeniedError("Cannot write to target folder: %s" % target_dir)

    def join_paths(self, path, *paths):
        result = posixpath.join(path, *paths)
        return result
        
        
            
#===============================================================================
# The local file ops are context free, meaning that they
# do not need active shell session to work on
# local  all  plain
#   save_file_content  - python
#   get_file_content   - python
#   copy_file          - python
#   get_dir_access     - python (returns either rw or ro or none)
#   listdir            - python
# local  all  sudo derives from local-all-plain
#   save_file_content  - shell
#   get_file_content   - python (maybe sudo if file is 0600)
#   copy_file          - shell
#   get_dir_access     - python (returns either rw or ro or none)
#   listdir            - python/shell(for ro-dirs)
class FileOpsLocalUnix(FileOpsLinuxBase):
    @classmethod
    def match(cls, target_os, connection_method):
        return connection_method == "local" and target_os in (wbaOS.linux, wbaOS.darwin)

    process_ops = None
    def __init__(self, process_ops, ssh=None, target_os = None):
        FileOpsLinuxBase.__init__(self, process_ops, ssh, target_os)

    # Still need to differentiate whether it is an OSError or an IOError
    def raise_exception(self, message, custom_messages = {}):
        for code, name in errno.errorcode.iteritems():
            if os.strerror(code) in message:
                if code == errno.EACCES:
                    raise PermissionDeniedError(custom_messages.get(code, message))
                
                raise OSError(code, custom_messages.get(code, message))
    
        raise Exception(custom_messages.get(None, message))
        
    # content must be a string
    @useAbsPath("filename")
    def save_file_content(self, filename, content, as_user = Users.CURRENT, user_password = None, mode = None):
        self.save_file_content_and_backup(filename, content, None, as_user, user_password, mode)

    # UseCase: If get_file_content fails with exception of access, try sudo
    @useAbsPath("filename")
    def get_file_content(self, filename, as_user = Users.CURRENT, user_password = None, skip_lines=0):
        if as_user == Users.CURRENT:
            try:
                f = open(filename, 'r')
            except (IOError, OSError), e:
                if e.errno == errno.EACCES:
                    raise PermissionDeniedError("Can't open file '%s'" % filename)
                raise e

            if skip_lines > 0:
                #unused last_skipped_line = ""
                skipped = 0
                lines = []
                for line in f:
                    if skipped < skip_lines:
                        #unused last_skipped_line = "%d - %s\n" % (skipped, line)
                        pass
                    else:
                        lines.append(line.rstrip())

                    skipped = skipped + 1
                 
                cont = "\n".join(lines)
            else:
                cont = f.read()

            f.close()
        else:
            cont = FileOpsLinuxBase.get_file_content(self, filename, as_user, user_password, skip_lines)

        return cont

  
    @useAbsPath("path")
    def delete_file(self, path, as_user = Users.CURRENT, user_password = None):
        if as_user == Users.CURRENT:
            FileUtils.delete_file(path)                   
        else:
            FileOpsLinuxBase.delete_file(self, path, as_user, user_password)


    def _copy_file(self, source, dest, as_user = Users.CURRENT, user_password = None):
        if as_user == Users.CURRENT:
            FileUtils.copy_file(source, dest)            
        else:
            FileOpsLinuxBase._copy_file(self, source, dest, as_user, user_password)

    # Return format is list of entries in dir (directories go first, each dir name is followed by /)
    @useAbsPath("path")
    def listdir(self, path, as_user = Users.CURRENT, user_password = None, include_size=False): # base operation to build file_exists and remote file selector
        file_list = []
        if as_user == Users.CURRENT: 
            FileUtils.list_dir(path, include_size, lambda l, list = file_list: file_list.append(l))
            
            if include_size:
              file_list = [(f, int(s)) for s, f in [s.strip().split(" ", 1) for s in file_list]]
            else:
              file_list = [s.strip() for s in file_list]
        else:
            file_list = FileOpsLinuxBase.listdir(self, path, as_user, user_password, include_size)
                    
        return file_list

    def join_paths(self, path, *paths):
        result = posixpath.join(path, *paths)
        return result

_file_ops_classes.append(FileOpsLocalUnix)


#===============================================================================
class FileOpsLocalWindows(object): # Used for remote as well, if not using sftp
    @classmethod
    def match(cls, target_os, connection_method):
        return connection_method in ("local", "wmi") and target_os == wbaOS.windows


    def __init__(self, process_ops, ssh=None, target_os = None):
        self.process_ops = process_ops
        self.ssh = ssh
        self.target_os = target_os
    
        tempdir, rc= self.process_ops.get_cmd_output("echo %temp%")
        if tempdir and tempdir.strip():
            self.tempdir = tempdir.strip()

    def exec_helper_command(self, command, result_mode, as_user, user_password):
        """
        This function is in charge of executing a command through the admin helper
        and processes the result depending on the result_mode parameter

        It was created to avoid having a result parsing on each function called
        through the admin helper and simplify code
        """

        ret_val = None
        out = []
        res = self.process_ops.exec_cmd(command,
                            as_user,
                            user_password,
                            output_handler = lambda line, l = out:l.append(line))

        if res == 0:
            if result_mode == FunctionType.Boolean:
                # Only booleans are expected,  if an error 
                # occurred it has been already raised on an exception
                if out[0] == 'True':
                    ret_val = True
                elif out[0] == 'False':
                    ret_val = False

            elif result_mode == FunctionType.Success:
                # nothing to do, is expected to succeed, if an error
                # happened, it has been already raised on an exception
                pass

            elif result_mode == FunctionType.String:
                ret_val = out[0]

            elif result_mode == FunctionType.Data:
                ret_val = out
                    
        else:
            raise Exception('Error executing helper command : %s' % command)

        return ret_val

        
    def get_file_owner(self, path, as_user = Users.CURRENT, user_password = None):
        if as_user == Users.CURRENT:
            ret_val = FileUtils.get_file_owner(path)
        else:
            ret_val = self.exec_helper_command('GET_FILE_OWNER %s' % path, FunctionType.String, as_user, user_password)

        return ret_val

    def check_file_readable(self, path, as_user=Users.CURRENT, user_password=None):
        if as_user == Users.CURRENT:
            ret_val = FileUtils.check_file_readable(path)
        else:
            ret_val = self.exec_helper_command('CHECK_FILE_READABLE %s' % path, FunctionType.Boolean, as_user, user_password)
          
        return ret_val

    def check_path_exists(self, path, as_user=Users.CURRENT, user_password=None):
        if as_user == Users.CURRENT:
            ret_val = FileUtils.check_path_exists(path)
        else:
            ret_val = self.exec_helper_command('CHECK_PATH_EXISTS %s' % path, FunctionType.Boolean, as_user, user_password)
        return ret_val;

    def check_dir_writable(self, path, as_user=Users.CURRENT, user_password=None):
        if as_user == Users.CURRENT:
            ret_val = FileUtils.check_dir_writable(path)
        else:
            ret_val = self.exec_helper_command('CHECK_DIR_WRITABLE %s' % path, FunctionType.Boolean, as_user, user_password)
          
        return ret_val

    def file_exists(self, filename, as_user = Users.CURRENT, user_password=None):
        if as_user == Users.CURRENT:
            ret_val = FileUtils.check_path_exists(filename)
        else:
            ret_val = self.exec_helper_command('CHECK_PATH_EXISTS %s' % filename, FunctionType.Boolean, as_user, user_password)

        return ret_val

    def get_available_space(self, path, as_user=Users.CURRENT, user_password=None):
        if as_user == Users.CURRENT:
            ret_val = FileUtils.get_free_space(path)
        else:
            try:
                ret_val = self.exec_helper_command('GET_FREE_SPACE %s' % path, FunctionType.String, as_user, user_password)
            except Exception:
                ret_val = 'Could not determine'

        return ret_val

    # content must be a string
    def save_file_content(self, filename, content, as_user = Users.CURRENT, user_password = None, mode = None):
        self.save_file_content_and_backup(filename, content, None, as_user, user_password, mode)


    def save_file_content_and_backup(self, filename, content, backup_extension, as_user = Users.CURRENT, user_password = None, mode = None):
        log_debug('%s: Saving file "%s" with backup (sudo="%s")\n' % (self.__class__.__name__, filename, str(as_user)) )

        # First saves the content to a temporary file
        try:
            tmp = tempfile.NamedTemporaryFile("w+b", delete = False)
            tmp_name = tmp.name
            log_debug('%s: Writing file contents to tmp file "%s" as %s\n' % (self.__class__.__name__, tmp_name, as_user) )
            tmp.write(content)
            tmp.close()

            backup_file = ""
            if backup_extension and ntpath.exists(filename):
                backup_file = filename + backup_extension

            if as_user != Users.CURRENT:
                if backup_file:
                    copy_command = 'COPY_FILE %s>%s>%s' % (tmp_name, filename, backup_file)
                else:
                    copy_command = 'COPY_FILE %s>%s' % (tmp_name, filename)

                self.exec_helper_command(copy_command, FunctionType.Success, as_user, user_password)
            else:
                FileUtils.copy_file(tmp_name, filename, backup_file)

        except Exception, exc:
            log_error('%s: Exception caught: %s\n' % (self.__class__.__name__, str(exc)) )
            raise

    # UseCase: If get_file_content fails with exception of access, try sudo
    def get_file_content(self, filename, as_user = Users.CURRENT, user_password = None, skip_lines=0):

        lines = []
        if as_user == Users.CURRENT: 
            FileUtils.get_file_lines(filename, skip_lines, lambda l, list = lines: list.append(l))
        else:
            lines = self.exec_helper_command('GETFILE_LINES %d %s' % (skip_lines, filename), FunctionType.Data, as_user, user_password)

        return ''.join(lines)


    def _copy_file(self, source, dest, as_user = Users.CURRENT, user_password = None): # not used externally, but in superclass
        if as_user == Users.CURRENT:
            error = FileUtils.copy_file(source, dest)
            if error:
                raise Exception(error)
        else:
            self.exec_helper_command('COPY_FILE %s>%s' % (source, dest), FunctionType.Success, as_user, user_password)


    def create_directory(self, path, as_user = Users.CURRENT, user_password = None, with_owner=None):
        if with_owner is not None:
            raise PermissionDeniedError("Changing owner of directory not supported in Windows" % path)

        if as_user == Users.CURRENT:
            FileUtils.create_directory(path)
        else:
            self.exec_helper_command('CREATE_DIRECTORY %s' % path, FunctionType.Success, as_user, user_password)


    def create_directory_recursive(self, path, as_user = Users.CURRENT, user_password = None, with_owner=None):
        if with_owner is not None:
            raise PermissionDeniedError("Changing owner of directory not supported in Windows" % path)

        if as_user == Users.CURRENT:
            FileUtils.create_directory_recursive(path)
        else:
            self.exec_helper_command('CREATE_DIRECTORY_RECURSIVE %s' % path, FunctionType.Success, as_user, user_password)

    def remove_directory(self, path, as_user = Users.CURRENT, user_password = None):
        if as_user == Users.CURRENT:
            FileUtils.remove_directory(path)
        else:
            self.exec_helper_command('REMOVE_DIRECTORY %s' % path, FunctionType.Success, as_user, user_password)

    def remove_directory_recursive(self, path, as_user = Users.CURRENT, user_password = None):
        if as_user == Users.CURRENT:
            FileUtils.remove_directory_recursive(path)
        else:
            self.exec_helper_command('REMOVE_DIRECTORY_RECURSIVE %s' % path, FunctionType.Success, as_user, user_password)

    def delete_file(self, path, as_user = Users.CURRENT, user_password = None):
        if as_user == Users.CURRENT:
            FileUtils.delete_file(path)
        else:
            self.exec_helper_command('DELETE_FILE %s' % path, FunctionType.Success, as_user, user_password)


    def listdir(self, path, as_user = Users.CURRENT, user_password = None, include_size=False):
        file_list = []
        if as_user == Users.CURRENT: 
            FileUtils.list_dir(path, include_size, lambda l, list = file_list: file_list.append(l))
        else:
            file_list = self.exec_helper_command('LISTDIR %s %s' % ("1" if include_size else "0", path), FunctionType.Data, as_user, user_password)

        if include_size:
          file_list = [(f, int(s)) for s, f in [s.strip().split(" ", 1) for s in file_list]]
        else:
          file_list = [s.strip() for s in file_list]

        return file_list

    def join_paths(self, path, *paths):
        result = ntpath.join(path, *paths)
        return result


_file_ops_classes.append(FileOpsLocalWindows)

#===============================================================================
# This remote file ops are shell dependent, they must be
# given active ssh connection, possibly, as argument
# remote unix sudo/non-sudo
#   save_file_content  - shell
#   get_file_content   - shell
#   copy_file          - shell
#   get_dir_access     - shell(returns either rw or ro or none)
#   listdir            - shell(for ro-dirs)
class FileOpsRemoteUnix(FileOpsLinuxBase):
    @classmethod
    def match(cls, target_os, connection_method):
        return connection_method == "ssh" and target_os in (wbaOS.linux, wbaOS.darwin)

    def __init__(self, process_ops, ssh, target_os = None):
        FileOpsLinuxBase.__init__(self, process_ops, ssh, target_os)

    # Still need to differentiate whether it is an OSError or an IOError
    def raise_exception(self, message, custom_messages = {}):
        if 'Permission denied' in message:
            raise PermissionDeniedError(custom_messages.get(errno.EACCES, message))
        elif 'No such file or directory' in message:
            raise OSError(errno.ENOENT, custom_messages.get(errno.ENOENT, message))
        elif 'Not a directory' in message:
            raise OSError(errno.ENOTDIR, custom_messages.get(errno.ENOTDIR, message))
        elif 'Directory not empty' in message:
            raise OSError(errno.ENOTEMPTY, custom_messages.get(errno.ENOTEMPTY, message))
        elif 'No SSH connection is active' in message:
            stack = inspect.stack()
            if len(stack) > 1:
                function = stack[1][3]
                message = "Unable to perform function %s. %s" % (function, message)
            raise Exception(message)
        else:        
            raise Exception(custom_messages.get(None, message))

       
    def create_directory(self, path, as_user = Users.CURRENT, user_password = None, with_owner=None):
        if as_user == Users.CURRENT:
            if with_owner is not None:
                raise PermissionDeniedError("Cannot set owner of directory %s" % path)
                
            try:
                self.ssh.mkdir(path)
            except (IOError, OSError), err:
                if err.errno == errno.EACCES:
                    raise PermissionDeniedError("Could not create directory %s" % path)
                raise err
        else:
            FileOpsLinuxBase.create_directory(self, path, as_user, user_password, with_owner)

    def delete_file(self, path, as_user = Users.CURRENT, user_password = None):
        if as_user == Users.CURRENT:
            try:
                self.ssh.remove(path)
            except (IOError, OSError), err:
                if err.errno == errno.EACCES:
                    raise PermissionDeniedError("Could not delete file %s" % path)
                raise err
        else:
            FileOpsLinuxBase.delete_file(self, path, as_user, user_password)
    
    #-----------------------------------------------------------------------------
    def save_file_content(self, filename, content, as_user = Users.CURRENT, user_password = None, mode = None):
        self.save_file_content_and_backup(filename, content, None, as_user, user_password, mode)

    #-----------------------------------------------------------------------------
    
    def _create_file(self, path, content):
        
        try:
            self.ssh.set_contents(path, content)
        except (IOError, OSError), err:
            if err.errno == errno.EACCES:
                raise PermissionDeniedError("Could not open file %s for writing" % path)
            raise err
    
    def _create_temp_file(self, content):

        tmpfilename = ''

        if self.ssh is not None:
            done = False
            attempts = 0
            while not done:
                tmpfilename = '/tmp/' + ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(16))
                try:
                    # This uses file open mode as wx to make sure the temporary has been created
                    # on this open attempt to avoid writing to an existing file.
                    self.ssh.set_contents(tmpfilename, content, "wx")
                    log_debug2('Created temp file: "%s".\n' % tmpfilename)
                    done = True
                except IOError, exc:
                    # This is the only hting reported on a failure due to an attempt to
                    # create a file that already exists
                    if exc.message == "Failure":
                        log_warning('WARNING: Unable to create temp file: "%s", trying a different name.\n' % tmpfilename)

                        if attempts < 10:
                            attempts += 1
                        else:
                            log_warning('ERROR: Unable to create temp file max number of attempts reached.\n')
                            raise IOError('Unable to create temp file max number of attempts reached.')
                    else:
                        log_warning('ERROR: Unable to create temp file: "%s" : %s.\n' % (tmpfilename, exc))
                        raise exc
                except Exception, exc:
                    log_warning('ERROR: Unable to create temp file: "%s" : %s.\n' % (tmpfilename, exc))
                    raise exc
        else:
            raise Exception("No SSH session active, cannot save file remotely")

        return tmpfilename
        
    def join_paths(self, path, *paths):
        result = posixpath.join(path, *paths)
        return result

_file_ops_classes.append(FileOpsRemoteUnix)


#===============================================================================
# remote win sudo/non-sudo
#   save_file_content  - sftp
#   get_file_content   - sftp
#   copy_file          - sftp
#   get_dir_access     - sftp(returns either rw or ro or none)
#   listdir            - sftp(for ro-dirs)
class FileOpsRemoteWindows(object):
    @classmethod
    def match(cls, target_os, connection_method):
        print "AAAAAAAAA - TARGETOS: %s - %s" % (target_os, wbaOS.windows)
        return connection_method == "ssh" and target_os == wbaOS.windows

    def __init__(self, process_ops, ssh, target_os):
        self.process_ops = process_ops
        self.ssh = ssh

    def file_exists(self, filename, as_user=Users.CURRENT, user_password=None):
        if self.ssh:
            try:
                return self.ssh.file_exists(filename)
            except IOError:
                raise
        else:
            print "Attempt to read remote file with no ssh session"
            log_error('%s: Attempt to read remote file with no ssh session\n' % self.__class__.__name__)
            raise Exception("Cannot read remote file without an SSH session")
        return False

    def get_available_space(self, path, as_user=Users.CURRENT, user_password=None):
        out = []        

        res = self.process_ops.exec_cmd('dir %s' % quote_path(path),
                            as_user,
                            user_password,
                            output_handler = lambda line, l = out:l.append(line))

        available = "Could not determine"
        if res == 0 and len(out):
            measures = ['B', 'KB', 'MB', 'GB', 'TB']
            tokens = out[0].strip().split("\n")[-1].strip().split()
            
            total = float(tokens[2].replace(",",""))
            index = 0
            while index < len(measures) and total > 1024:
                total = float(total / 1024)
                index = index + 1


            available = "%.2f %s available" % (total, measures[index])
        
        return available

    def check_path_exists(self, path, as_user=Users.CURRENT, user_password=None):
        if self.ssh:
            out, ret = self.ssh.exec_cmd('if exist %s exit /b 0' % quote_path(path), as_user, user_password)
            if ret != 0:
                raise RuntimeError(out)
            return True
        else:
            log_error('%s: Attempt to read remote file with no ssh session\n' % self.__class__.__name__)
            raise Exception("Cannot read remote file without an SSH session")
        return False

    def create_directory(self, path, as_user = Users.CURRENT, user_password = None, with_owner=None):
        if with_owner is not None:
            raise PermissionDeniedError("Changing owner of directory not supported in Windows" % path)

        if as_user == Users.CURRENT:
            try:
                self.ssh.mkdir(path)
            except OSError, err:
                if err.errno == errno.EACCES:
                    raise PermissionDeniedError("Could not create directory %s" % path)
                raise err
        else:
            command = wrap_for_sudo('mkdir ' + quote_path_win(path), self.process_ops.sudo_prefix, as_user)
            out, ret = self.ssh.exec_cmd(command, as_user, user_password)
            if ret != 0:
                raise RuntimeError(out)

    def create_directory_recursive(self, path, as_user = Users.CURRENT, user_password = None, with_owner=None):
        head, tail = splitpath(path)
        if not tail:
            head, tail = splitpath(head)
        if head and tail and not self.file_exists(head):
            try:
                self.create_directory_recursive(head, as_user, user_password, with_owner)
            except OSError, e:
                if e.errno != errno.EEXIST:
                    raise

        self.create_directory(path, as_user, user_password, with_owner)

    def remove_directory(self, path, as_user = Users.CURRENT, user_password = None):
        if as_user == Users.CURRENT:
            try:
                self.ssh.rmdir(path)
            except OSError, err:
                if err.errno == errno.EACCES:
                    raise PermissionDeniedError("Could not remove directory %s" % path)
                raise err
        else:
            command = wrap_for_sudo('rmdir ' + quote_path_win(path), self.process_ops.sudo_prefix, as_user)

            out, ret = self.ssh.exec_cmd(command, as_user, user_password)
            if ret != 0:
                raise RuntimeError(out)

    def remove_directory_recursive(self, path, as_user = Users.CURRENT, user_password = None):
        command = wrap_for_sudo('rmdir %s /s /q' % quote_path_win(path), self.process_ops.sudo_prefix, as_user)

        out, ret = self.ssh.exec_cmd(command, as_user, user_password)
        if ret != 0:
            raise RuntimeError(out)

    def delete_file(self, path, as_user = Users.CURRENT, user_password = None):
        if as_user == Users.CURRENT:
            try:
                self.ssh.remove(path)
            except OSError, err:
                if err.errno == errno.EACCES:
                    raise PermissionDeniedError("Could not delete file %s" % path)
                raise err
        else:
            command = wrap_for_sudo('del ' + quote_path_win(path), self.process_ops.sudo_prefix, as_user)
            out, ret = self.ssh.exec_cmd(command, as_user, user_password)
            if ret != 0:
                raise RuntimeError(out)

    def save_file_content_and_backup(self, path, content, backup_extension, as_user = Users.CURRENT, user_password = None, mode = None):
        # Check if dir, where config file will be stored is writable
        dirname, filename = ntpath.split(path)

        if as_user == Users.CURRENT:
            if not self.check_dir_writable(dirname.strip(" \r\t\n")):
                raise PermissionDeniedError("Cannot write to directory %s" % dirname)

        if self.ssh is not None:
            ## Get temp dir for using as tmpdir
            tmpdir, status = self.process_ops.get_cmd_output("echo %temp%")
            if type(tmpdir) is unicode:
                tmpdir = tmpdir.encode("utf8")
            if type(tmpdir) is str:
                tmpdir = tmpdir.strip(" \r\t\n")
                if tmpdir[1] == ":":
                    tmpdir = tmpdir[2:]
                else:
                    log_debug('%s: Temp directory path "%s" is not in expected form. The expected form is something like "C:\\Windows\\Temp"\n' % (self.__class__.__name__, tmpdir) )
                    tmpdir = None
                log_debug2('%s: Got temp dir: "%s"\n' % (self.__class__.__name__, tmpdir) )
            else:
                tmpdir = None

            if not tmpdir:
                tmpdir = dirname

            tmpfilename = tmpdir + r"\workbench-temp-file.ini"

            log_debug('%s: Remotely writing contents to temporary file "%s"\n' % (self.__class__.__name__, tmpfilename) )
            log_debug3('%s: %s\n' % (self.__class__.__name__, content) )
            self.ssh.set_contents(tmpfilename, content)

            if backup_extension:
                log_debug('%s: Backing up "%s"\n' % (self.__class__.__name__, path) )
                backup_cmd = "copy /y " + quote_path_win(path) + " " + quote_path_win(path+backup_extension)
                msg, code = self.process_ops.get_cmd_output(backup_cmd)
                if code != 0:
                    print backup_cmd, "->", msg
                    log_error('%s: Error backing up file: %s\n' % (self.__class__.__name__, backup_cmd+'->'+msg) )
                    raise RuntimeError("Error backing up file: %s" % msg)

            copy_to_dest = "copy /y " + quote_path_win(tmpfilename) + " " + quote_path_win(path)
            delete_tmp = "del " + quote_path_win(tmpfilename)
            log_debug('%s: Copying file to final destination: "%s"\n' % (self.__class__.__name__, copy_to_dest) )
            msg, code = self.process_ops.get_cmd_output(copy_to_dest)
            if code != 0:
                print copy_to_dest, "->", msg
                log_error('%s: Error copying temporary file over destination file: %s\n%s to %s\n' % (self.__class__.__name__, msg, tmpfilename, path) )
                raise RuntimeError("Error copying temporary file over destination file: %s\n%s to %s" % (msg, tmpfilename, path))
            log_debug('%s: Deleting tmp file: "%s"\n' % (self.__class__.__name__, delete_tmp) )
            msg, code = self.process_ops.get_cmd_output(delete_tmp)
            if code != 0:
                print "Could not delete temporary file %s: %s" % (tmpfilename, msg)
                log_info('%s: Could not delete temporary file "%s": %s\n' % (self.__class__.__name__, tmpfilename, msg) )
        else:
            raise Exception("No SSH session active, cannot save file remotely")

    # UseCase: If get_file_content fails with exception of access, try sudo
    def get_file_content(self, filename, as_user = Users.CURRENT, user_password = None, skip_lines=0):
        if self.ssh:
            # Supposedly in Windows, sshd account has admin privileges, so Users.ADMIN can be ignored
            try:
                return self.ssh.get_contents(filename)
            except IOError, exc:
                if exc.errno == errno.EACCES:
                    raise PermissionDeniedError("Permission denied attempting to read file %s" % filename)
        else:
            print "Attempt to read remote file with no ssh session"
            raise Exception("Cannot read remote file without an SSH session")

    #TODO fix for windows
    def check_file_readable(self, path, as_user=Users.CURRENT, user_password=None):
        msg, code = self.process_ops.get_cmd_output('if exist ' + quote_path(path) + ' ( type '+ quote_path(path) +'>NUL && echo 0 ) else ( echo 1 )')
        ret = (code == 0)
        return ret

    def check_dir_writable(self, path, as_user=Users.CURRENT, user_password=None):
        msg, code = self.process_ops.get_cmd_output('echo 1 > ' + quote_path(path + "/wba_tmp_file.bak"))
        ret = (code == 0)
        if ret:
            msg, code = self.process_ops.get_cmd_output('del ' + quote_path(path + "/wba_tmp_file.bak"))
        return ret

    def listdir(self, path, as_user = Users.CURRENT, user_password = None, include_size=False): # base operation to build file_exists and remote file selector
        # TODO: user elevation
        sftp = self.ssh.getftp()
        (dirs, files) = sftp.ls(path, include_size=include_size)
        ret = []
        for d in dirs:
            ret.append((d[0] + "/", d[1]) if include_size else d + "/")
        return ret + list(files)

    def join_paths(self, path, *paths):
        result = os.path.join(path, *paths)
        return result
        
_file_ops_classes.append(FileOpsRemoteWindows)


#===============================================================================
#
#===============================================================================
class ServerManagementHelper(object):
    def __init__(self, profile, ssh):
      
        settings = profile.get_settings_object()
        serverInfo = settings.serverInfo
        
        # Resets the sudo prefix accordingly
        reset_sudo_prefix()
        
        if serverInfo.has_key('sys.mysqld.sudo_override'):
            sudo_override = serverInfo['sys.mysqld.sudo_override']
        
            if sudo_override.strip():
                global default_sudo_prefix
                default_sudo_prefix = sudo_override
                log_warning('Overriding default sudo prefix to : %s\n' % default_sudo_prefix)
    
        self.tmp_files = [] # TODO: make sure the files will be deleted on exit

        self.profile = profile

        klass = None
        
        match_tuple = (profile.host_os, profile.target_os, profile.connect_method)
        for k in _process_ops_classes:
            if k.match(match_tuple):
                klass = k
                break
        if klass:
            sudo_prefix=profile.sudo_prefix
            
            if not sudo_prefix:
                sudo_prefix = default_sudo_prefix

            self.shell = klass(sudo_prefix=sudo_prefix, ssh=ssh)
            self.shell.post_init()
        else:
            raise Exception("Unsupported administration target type: %s"%str(match_tuple))

        klass = None
        for k in _file_ops_classes:
            if k.match(profile.target_os, profile.connect_method):
                klass = k
                break

        if klass:
            self.file = klass(self.shell, ssh=ssh, target_os = profile.target_os)
        else:
            raise Exception("Unsupported administration target type: %s:%s" % (str(profile.target_os), str(profile.connect_method)))


    @property
    def cmd_output_encoding(self):
        if self.shell:
            return self.shell.cmd_output_encoding
        return ""

    #-----------------------------------------------------------------------------
    def check_file_readable(self, path, as_user=Users.CURRENT, user_password=None):
        return self.file.check_file_readable(path, as_user, user_password)
    
    #-----------------------------------------------------------------------------
    def check_path_exists(self, path, as_user=Users.CURRENT, user_password=None):
        return self.file.check_path_exists(path, as_user, user_password)

    #-----------------------------------------------------------------------------
    def check_dir_writable(self, path, as_user=Users.CURRENT, user_password=None):
        return self.file.check_dir_writable(path, as_user, user_password)

    #-----------------------------------------------------------------------------
    def file_exists(self, path, as_user = Users.CURRENT, user_password=None):
        return self.file.file_exists(path, as_user, user_password)

    #-----------------------------------------------------------------------------
    def get_available_space(self, path, as_user = Users.CURRENT, user_password=None):
        return self.file.get_available_space(path, as_user, user_password)

    #-----------------------------------------------------------------------------
    def get_file_owner(self, path, as_user = Users.CURRENT, user_password=None):
        return self.file.get_file_owner(path, as_user, user_password)

    #-----------------------------------------------------------------------------
    def create_directory(self, path, as_user = Users.CURRENT, user_password=None, with_owner=None):
        return self.file.create_directory(path, as_user, user_password, with_owner)

    #-----------------------------------------------------------------------------
    def create_directory_recursive(self, path, as_user = Users.CURRENT, user_password=None, with_owner=None):
        return self.file.create_directory_recursive(path, as_user, user_password, with_owner)

    #-----------------------------------------------------------------------------
    def remove_directory(self, path, as_user = Users.CURRENT, user_password=None):
        return self.file.remove_directory(path, as_user, user_password)

    #-----------------------------------------------------------------------------
    def remove_directory_recursive(self, path, as_user = Users.CURRENT, user_password=None):
        return self.file.remove_directory_recursive(path, as_user, user_password)

    #-----------------------------------------------------------------------------
    def delete_file(self, path, as_user = Users.CURRENT, user_password=None):
        return self.file.delete_file(path, as_user, user_password)

    #-----------------------------------------------------------------------------
    # Make sure that file is readable only by user!
    def make_local_tmpfile(self):
        # Here we create that file name blah-blah-blah
        # if total_success:
        #   self.tmp_files.append(filename)
        raise NotImplementedError

    #-----------------------------------------------------------------------------
    def get_file_content(self, path, as_user = Users.CURRENT, user_password = None, skip_lines=0):
        return self.file.get_file_content(path, as_user=as_user, user_password=user_password, skip_lines=skip_lines)

    #-----------------------------------------------------------------------------
    def set_file_content(self, path, contents, as_user = Users.CURRENT, user_password = None, mode = None):
        return self.file.save_file_content(path, contents, as_user=as_user, user_password=user_password, mode = mode)

    #-----------------------------------------------------------------------------
    def listdir(self, path, as_user = Users.CURRENT, user_password = None, include_size=False):
        return self.file.listdir(path, as_user, user_password, include_size=include_size)

    #-----------------------------------------------------------------------------
    def set_file_content_and_backup(self, path, contents, backup_extension, as_user = Users.CURRENT, user_password = None, mode = None):
        if type(contents) is unicode:
            contents = contents.encode("utf8")
        return self.file.save_file_content_and_backup(path, contents, backup_extension, as_user=as_user, user_password=user_password, mode = mode)

    #-----------------------------------------------------------------------------
    # Returns Status Code
    # Text Output is given to output_handler, if there is any
    def execute_command(self, command, as_user = Users.CURRENT, user_password=None, output_handler=None, options=None):
        return self.shell.exec_cmd(command, as_user, user_password, output_handler, options)

    def spawn_process(self, command, as_user=Users.CURRENT, user_password=None, output_handler=None, options=None):
        return self.shell.spawn_process(command, as_user, user_password, output_handler, options)


    def list2cmdline(self, args):
        return self.shell.list2cmdline(args)


    def join_paths(self, path, *paths):
        result = self.file.join_paths(path, *paths)
        return result
#===============================================================================

class LocalInputFile(object):
    def __init__(self, path):
        self.path = path
        self._f = open(path)

    def tell(self):
        return self._f.tell()

    @property
    def size(self):
        return os.stat(self.path).st_size

    def get_range(self, start, end):
        self._f.seek(start)
        return self._f.read(end-start)

    def start_read_from(self, offset):
        self._f.seek(offset)

    def read(self, count):
        return self._f.read(count)

    def readline(self):
        return self._f.readline()


class SFTPInputFile(object):
    def __init__(self, ctrl_be, path):
        self.ctrl_be = ctrl_be
        self.ssh = WbAdminSSH()
        self.path = path
        while True:
            try:  # Repeat get password while password is misspelled. Re-raise other exceptions
                self.ssh.wrapped_connect(self.ctrl_be.server_profile, self.ctrl_be.password_handler)
            except ConnectionError, error:
                if not str(error).startswith('Could not establish SSH connection: Authentication failed'):
                    raise
            else:
                break
        if not self.ssh.is_connected():
            raise RuntimeError('Could not connect to remote server')

        self.sftp = self.ssh.client.open_sftp()
        try:
            self._f = self.sftp.open(path)
        except IOError:
            raise RuntimeError('Could not read file %s in remote server. Please verify path and permissions' % path)

    def tell(self):
        return self._f.tell()

    @property
    def size(self):
        return self.sftp.stat(self.path).st_size

    def get_range(self, start, end):
        self._f.seek(start)
        return self._f.read(end-start)

    def start_read_from(self, offset):
        self._f.seek(offset)

    def read(self, count):
        return self._f.read(count)

    def readline(self):
        return self._f.readline()


import multiprocessing
class SudoTailInputFile(object):
    def __init__(self, server_helper, path, password = None, password_cb = None):
        self.path = path
        self.server_helper = server_helper # ServerManagementHelper
        self.data = None
        self._password = password
        self._password_cb = password_cb
        self.skip_first_newline = True
        self._pos = 0
        self._proc = None
        self._queue = None
        # multiprocessing + paramiko doesn't work, so fallback to threading for remote access
        self._is_local = self.server_helper.profile.is_local
        # check if we can read the file as normall user, if not we need to gather password if callback is provided
        self._need_sudo = False
        while True:
            if not self._need_sudo:
                try:
                    if not self.server_helper.check_file_readable(self.path):
                        self._need_sudo = True 
                    break
                except OSError, e:
                    log_debug3("check_file_readable returned OSError, we will try with sudo then")
                    self._need_sudo = True
            else:
                self._password = self.get_password
                self.server_helper.check_file_readable(self.path, as_user = Users.ADMIN, user_password=self._password)
                break
            
    def __del__(self):
        if self._proc:
            self._proc.join()

    def tell(self):
        return self._pos
    
    @property
    def get_password(self):
        return self._password if not self._password_cb else self._password_cb()

    @property
    def size(self):
        if not self._need_sudo:
            files = self.server_helper.listdir(self.path, as_user = Users.CURRENT, user_password=None, include_size=True)
        else:
            files = self.server_helper.listdir(self.path, as_user = Users.ADMIN, user_password=self._password, include_size=True)
        if not files:
            raise RuntimeError("Could not get size of file %s" % self.path)
        return files[0][1]

    def get_range(self, start, end):
        f = StringIO.StringIO()
        if not self._need_sudo:
            ret = self.server_helper.execute_command("/bin/dd if=%s ibs=1 skip=%i count=%i 2> /dev/null" % (quote_path(self.path), start, end-start), as_user = Users.CURRENT, user_password=None, output_handler=f.write)
        else:
            ret = self.server_helper.execute_command("/bin/dd if=%s ibs=1 skip=%i count=%i 2> /dev/null" % (quote_path(self.path), start, end-start), as_user = Users.ADMIN, user_password=self.get_password, output_handler=f.write)

        if ret != 0:
            raise RuntimeError("Could not get data from file %s" % self.path)
        return f.getvalue()

    def read_task(self, offset, file):
        if not self._need_sudo:
            self.server_helper.execute_command("/bin/dd if=%s ibs=1 skip=%i 2> /dev/null" % (quote_path(self.path), offset), as_user = Users.CURRENT, user_password=None, output_handler=file.write)
        else:
            self.server_helper.execute_command("/bin/dd if=%s ibs=1 skip=%i 2> /dev/null" % (quote_path(self.path), offset), as_user = Users.ADMIN, user_password=self.get_password, output_handler=file.write)
        # this will signal the reader end that there's no more data
        file.close()

    def start_read_task_from(self, offset):
        self._pos = offset
        self._queue = multiprocessing.Queue()
        self.data = QueueFileMP(self._queue)
        self._proc = multiprocessing.Process(target=self.read_task, args=(offset, self.data))
        import sys
        # restore original stdout file descriptors before forking
        stdo, stde, stdi = sys.stdout, sys.stderr, sys.stdin
        sys.stdout, sys.stderr, sys.stdin = sys.real_stdout, sys.real_stderr, sys.real_stdin
        self._proc.start()
        sys.stdout, sys.stderr, sys.stdin = stdo, stde, stdi
        if self.skip_first_newline:
            # for some reason (in Mac OS X) the output is prefixed with a newline, which breaks the XML parsing.. so check for that and skip it if its the case
            if self.data.peek(1) == '\n':
                self.data.read(1)

    def start_read_from(self, offset):
        if self._is_local:
            return self.start_read_task_from(offset)
        self._pos = offset
        f = StringIO.StringIO()
        if not self._need_sudo:
            self.server_helper.execute_command("/bin/dd if=%s ibs=1 skip=%i 2> /dev/null" % (quote_path(self.path), offset), as_user = Users.CURRENT, user_password=None, output_handler=f.write)
        else:
            self.server_helper.execute_command("/bin/dd if=%s ibs=1 skip=%i 2> /dev/null" % (quote_path(self.path), offset), as_user = Users.ADMIN, user_password=self._password, output_handler=f.write)
        self.data = f
        self.data.seek(0)
        if self.skip_first_newline:
            # for some reason (in Mac OS X) the output is prefixed with a newline, which breaks the XML parsing.. so check for that and skip it if its the case
            if self.data.read(1) != '\n':
                self.data.seek(0)

    def read(self, count=None):
        assert self.data is not None
        d = self.data.read(count)
        self._pos += len(d)
        return d

    def readline(self):
        assert self.data is not None
        d = self.data.readline()
        self._pos += len(d)
        return d


# Previous class but for windows
class AdminTailInputFile(object):
    """
    This class is the windows like implementation for the tail as admin command
    It is aided with a command in the admin helper which has the next syntax

    GETFILE <offset> <size> <filename>
       <offset> : position of the file where the read starts
       <size>   : number of bytes to be read, 0 indicates the rest of the file from the <offset> position
    """
    def __init__(self, server_helper, path, password):
        self.path = path
        self.server_helper = server_helper # ServerManagementHelper
        self.data = None
        self._password = password
        self._pos = 0
        self._proc = None
        self._queue = None

    def __del__(self):
        if self._proc:
            self._proc.join()

    def tell(self):
        return self._pos

    @property
    def size(self):
        files = self.server_helper.listdir(self.path, as_user = Users.ADMIN, user_password=self._password, include_size=True)
        if not files:
            raise RuntimeError("Could not get size of file %s" % self.path)
        return files[0][1]

    def get_range(self, start, end):
        f = StringIO.StringIO()
        ret = self.server_helper.execute_command("GETFILE %i %i file=%s" % (start, end-start, quote_path(self.path)), as_user = Users.ADMIN, user_password=self._password, output_handler=f.write)
        if ret != 0:
            raise RuntimeError("Could not get data from file %s" % self.path)
        return f.getvalue()

    def read_task(self, offset, file):
        self.server_helper.execute_command("GETFILE %i 0 file=%s" % (offset, quote_path(self.path)), as_user = Users.ADMIN, user_password=self._password, output_handler=file.write)
        # this will signal the reader end that there's no more data
        file.close()

    def start_read_task_from(self, offset):
        self._pos = offset
        self._queue = multiprocessing.Queue()
        self.data = QueueFileMP(self._queue)
        self._proc = multiprocessing.Process(target=self.read_task, args=(offset, self.data))
        import sys
        # restore original stdout file descriptors before forking
        stdo, stde, stdi = sys.stdout, sys.stderr, sys.stdin
        sys.stdout, sys.stderr, sys.stdin = sys.real_stdout, sys.real_stderr, sys.real_stdin
        self._proc.start()
        sys.stdout, sys.stderr, sys.stdin = stdo, stde, stdi

    def start_read_from(self, offset):
        self._pos = offset
        f = StringIO.StringIO()
        self.server_helper.execute_command("GETFILE %i 0 %s" % (offset, quote_path(self.path)), as_user = Users.ADMIN, user_password=self._password, output_handler=f.write)
        self.data = f
        self.data.seek(0)

    def read(self, count=None):
        assert self.data is not None
        d = self.data.read(count)
        self._pos += len(d)
        return d

    def readline(self):
        assert self.data is not None
        d = self.data.readline()
        self._pos += len(d)
        return d
