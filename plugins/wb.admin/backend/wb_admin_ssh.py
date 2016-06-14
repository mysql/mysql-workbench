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

 # import platform before paramiko to avoid random import error in osx 10.6
import platform
try:
    platform.system()
except IOError, err:
    if err.errno == 4:
        print "platform.system() exception detected, trying workaround..."
        # random bizarre platform.system() bug detected, try again after 3s
        import time
        time.sleep(3)
        platform.system()

import os
import errno
import stat
import mforms
import time
import traceback

from wb_common import PermissionDeniedError, InvalidPasswordError, OperationCancelledError, Users
from workbench.utils import server_version_str2tuple
from wb_common import CmdOptions, CmdOutput, SSHFingerprintNewError, format_bad_host_exception

from workbench.log import log_info, log_warning, log_error, log_debug, log_debug2, log_debug3

import grt

try:
    import paramiko
    import socket
    
    class StoreIfConfirmedPolicy(paramiko.MissingHostKeyPolicy):
        def missing_host_key(self, client, hostname, key):
            raise SSHFingerprintNewError("Key mismatched", client, hostname, key)
        
    # paramiko 1.6 didn't have this class
    if hasattr(paramiko, "WarningPolicy"):
        WarningPolicy = paramiko.WarningPolicy
    else:
        class WarningPolicy(paramiko.MissingHostKeyPolicy):
            def missing_host_key(self, client, hostname, key):
                import binascii
                log_warning('WARNING: Unknown %s host key for %s: %s\n' % (key.get_name(), hostname, binascii.hexlify(key.get_fingerprint())))

    
except:
    traceback.print_exc()
    # temporary workaround
    paramiko = None
    socket = None


def normalize_windows_path_for_ftp(path):
    idx = -1
    if platform.system().lower() != 'windows':
        idx = path.find(':')

    # Gets rid of the unit from the path
    if idx != -1:
        path = path[idx + 1 :]

    # Replaces the \\ to //
    path = path.replace('\\', '/')

    return path


if paramiko and server_version_str2tuple(paramiko.__version__) >= (1, 7, 4):
    from paramiko.message import Message
    from paramiko.common import MSG_CHANNEL_OPEN
    from paramiko.channel import Channel
    from paramiko.ssh_exception import SSHException 
    import threading
    OPEN_CHANNEL_TIMEOUT = 15

    def wba_open_channel(self, kind, dest_addr=None, src_addr=None, timeout = None, window_size=None, max_packet_size=None):
        chan = None
        if not self.active:
            # don't bother trying to allocate a channel
            return None
        self.lock.acquire()
        try:
            
            chanid = self._next_channel()
            m = Message()
            m.add_byte(chr(MSG_CHANNEL_OPEN))
            m.add_string(kind)
            m.add_int(chanid)
            local_window_size = None
            local_max_packet_size = None
            if (server_version_str2tuple(paramiko.__version__) < (1, 15, 0)):
                local_window_size = self.window_size
                local_max_packet_size = self.max_packet_size
            else:
                if window_size is not None:
                    local_window_size = self._sanitize_window_size(window_size)
                else:
                    local_window_size = self.default_window_size
                
                if max_packet_size is not None:
                    local_max_packet_size = self._sanitize_packet_size(max_packet_size)
                else:
                    local_max_packet_size = self.default_max_packet_size

            m.add_int(local_window_size)
            m.add_int(local_max_packet_size)
            if (kind == 'forwarded-tcpip') or (kind == 'direct-tcpip'):
                m.add_string(dest_addr[0])
                m.add_int(dest_addr[1])
                m.add_string(src_addr[0])
                m.add_int(src_addr[1])
            elif kind == 'x11':
                m.add_string(src_addr[0])
                m.add_int(src_addr[1])
            chan = Channel(chanid)
            self._channels.put(chanid, chan)
            self.channel_events[chanid] = event = threading.Event()
            self.channels_seen[chanid] = True
            chan._set_transport(self)
            chan._set_window(local_window_size, local_max_packet_size)
        finally:
            self.lock.release()
        self._send_user_message(m)
        ts = time.time() + OPEN_CHANNEL_TIMEOUT if (timeout is None) else timeout
        while True:
            event.wait(0.1);
            if not self.active:
                e = self.get_exception()
                if e is None:
                    e = SSHException('Unable to open SSH channel.')
                raise e
            if event.isSet():
                break
            if time.time() > ts:
                chan.close()
                chan = None  # TODO: Check if clean up from self._channels is needed
                event.clear()
                self._channels.delete(chanid)
                if chanid in self.channel_events:
                    del self.channel_events[chanid]
                raise IOError("open SSH channel timeout")
        chan = self._channels.get(chanid)
        if chan is not None:
            return chan
        e = self.get_exception()
        if e is None:
            e = SSHException('Unable to open SSH channel.')
        raise e

    paramiko.Transport.open_channel = wba_open_channel
else:
    print "Warning! Can't use connect with timeout in paramiko", paramiko and paramiko.__version__
    if paramiko:
        log_warning('Cannot use connect with timeout in paramiko version %s\n' % paramiko.__version__)
    else:
        log_warning('Paramiko unavailable.\n')

#===============================================================================
class ConnectionError(Exception):
    pass


#===============================================================================
class SSHDownException(Exception):
    pass

#===============================================================================
#
#===============================================================================
class WbAdminSFTP(object):
    def __init__(self, sftp):
        self.sftp = sftp

    #-----------------------------------------------------------------------------
    def pwd(self):
        ret = None
        if self.sftp:
            try:
                ret = self.sftp.getcwd()

                if ret is None:
                    self.sftp.chdir(".")
                    ret = self.sftp.getcwd()

                if ret is None:
                    ret = ''
            except IOError, e:
                print e

        return ret

    #-----------------------------------------------------------------------------
    def ls(self, path, include_size=False):
        path = normalize_windows_path_for_ftp(path)
        ret = ((),())
        if self.sftp:
            fnames = ()
            fattrs = ()
            try:
                fnames = self.sftp.listdir(path)
                fattrs = self.sftp.listdir_attr(path)
            except IOError, e:
                if e.errno == errno.ENOENT and path.strip(" \r\t\n") == ".":
                    raise
                ret = (('Failed to read directory content',),())

            if len(fnames) > 0 and len(fnames) == len(fattrs):
                dirs = []
                rest = []
                for i in range(0, len(fnames)):
                    attr = fattrs[i]
                    if stat.S_ISDIR(attr.st_mode):
                        dirs.append((attr.filename, 0) if include_size else attr.filename)
                    elif stat.S_ISREG(attr.st_mode):
                        rest.append((attr.filename, attr.st_size) if include_size else attr.filename)
                    else:
                        rest.append((attr.filename, attr.st_size) if include_size else attr.filename)
                if include_size:
                    dirs.sort(lambda a,b: cmp(a[0], b[0]))
                    rest.sort(lambda a,b: cmp(a[0], b[0]))
                else:
                    dirs.sort()
                    rest.sort()
                ret = (tuple(dirs), tuple(rest))
        return ret

    #-----------------------------------------------------------------------------
    def cd(self, path):
        path = normalize_windows_path_for_ftp(path)
        ret = False
        if self.sftp:
            try:
                self.sftp.chdir(path)
                ret = True
            except IOError:
                ret = False

        return ret

    #-----------------------------------------------------------------------------
    def close(self):
        if self.sftp:
            self.sftp.close()

#===============================================================================
#
#===============================================================================
class WbAdminSSH(object):
    client = None
    def wrapped_connect(self, settings, password_delegate):
        assert hasattr(password_delegate, "get_password_for")
        assert hasattr(password_delegate, "reset_password_for")

        log_debug2("%s: starting connect\n" % self.__class__.__name__)
        self.client = None

        #loginInfo = settings.loginInfo
        #serverInfo = settings.serverInfo

        host = settings.ssh_hostname #loginInfo['ssh.hostName']
        usekey = settings.ssh_usekey #int(loginInfo['ssh.useKey'])
        pwd = None # It is ok to keep pwd set to None even if we have it in server settings
                   # it will be retrived later
        port = settings.ssh_port#loginInfo['ssh.port']
        self.keepalive = settings.ssh_keepalive
        self.ssh_timeout = settings.ssh_timeout
        if usekey == 1:
            # We need to check if keyfile needs password. For some reason paramiko does not always
            # throw exception to request password
            key_filename = settings.ssh_key #loginInfo['ssh.key']
            if key_filename.startswith('~'):
                key_filename = os.path.expanduser(key_filename)
            f = None
            try:
                f = open(key_filename, 'r')
            except IOError:
                f = None # set file handle to None indicating that open failed

            keycont = None # Will hold contents of the keyfile
            if f is not None:
                keycont = f.read()
                f.close()
            else:
                usekey = 0 # Reset usekey to 0 so paramiko will not use non-existent key file
                key_filename = None

            if usekey == 0:
                # We need password for password ssh auth as keyfile is missing. Retrieve password or ask for it
                pwd = password_delegate.get_password_for("ssh")
                #accepted, pwd = mforms.Utilities.find_or_ask_for_password("SSH key file missing. Remote SSH Login (%s)" % serverInfo['sys.system'], "ssh@%s:%s" % (host,port or 22), loginInfo['ssh.userName'], False)
            elif keycont is not None:
                if 'ENCRYPTED' in keycont:
                    # Retrieve password or ask for it
                    try:
                        pwd = password_delegate.get_password_for("sshkey")
                    except OperationCancelledError, exc:
                        # SSH key usage cancelled, just login normally
                        usekey = 0
                        pwd = password_delegate.get_password_for("ssh")
                    #accepted, pwd = mforms.Utilities.find_or_ask_for_password("Unlock SSH Private Key", "ssh_keyfile@%s"%key_filename, loginInfo['ssh.userName'], False)
                #else:
                #  accepted = True

            #if not accepted:
            #  raise OperationCancelledError("Cancelled key password input")

            try:
                self.connect(host, port, settings.ssh_username, pwd, usekey, key_filename)
            except InvalidPasswordError, exc:
                if usekey:
                    password_delegate.reset_password_for("sshkey")
                else:
                    password_delegate.reset_password_for("ssh")
                raise exc
            except paramiko.PasswordRequiredException:
                pwd = password_delegate.get_password_for("sshkey")
                self.connect(host, port, settings.ssh_username, pwd, usekey, key_filename)
                # Retry key pwd
        else:
            #if pwd is None:
            #  accepted, pwd = mforms.Utilities.find_or_ask_for_password("Remote SSH Login (%s)" % serverInfo['sys.system'], "ssh@%s:%s" % (host,port or 22), loginInfo['ssh.userName'], False)
            pwd = password_delegate.get_password_for("ssh")

            #if accepted:
            try:
                self.connect(host, port, settings.ssh_username, pwd, None, None)
            except InvalidPasswordError, exc:
                password_delegate.reset_password_for("ssh")
                raise exc
            #else:
            #  raise OperationCancelledError("Cancelled login")
        log_debug2("%s: Leave\n" % self.__class__.__name__)

    def _get_ssh_config_path(self):
        paths = []
        user_path = grt.root.wb.options.options['pathtosshconfig'] if grt.root.wb.options.options['pathtosshconfig'] is not None else None
        if user_path:
            paths.append(user_path)
        if platform.system().lower() == "windows":
            paths.append("%s\ssh\config" % mforms.App.get().get_user_data_folder())
            paths.append("%s\ssh\ssh_config" % mforms.App.get().get_user_data_folder())
        else:
            paths.append("~/.ssh/config")
            paths.append("~/.ssh/ssh_config")
            
        for path in paths:
            if os.path.isfile(os.path.expanduser(path)):
                return os.path.expanduser(path)
        else:
            log_debug3("ssh config file not found")
            return None

    def connect(self, host, port, user, pwd, usekey = 0, key = None):
        if port == None or port == 0:
            port = 22

        if not paramiko:
            raise Exception("One of the modules necessary for SSH base administration could not be imported.")

        if key and key.startswith("~"):
            key = os.path.expanduser(key)

        config = paramiko.config.SSHConfig()
        config_file_path = self._get_ssh_config_path()
        if config_file_path:
            with open(config_file_path) as f:
                config.parse(f)

        opts = config.lookup(host)
        

        client = paramiko.SSHClient()
        ssh_known_hosts_file = None
        if "userknownhostsfile" in opts:
            ssh_known_hosts_file = opts["userknownhostsfile"]
        else:
            client.get_host_keys().clear()
            ssh_known_hosts_file = '~/.ssh/known_hosts'
            
            if platform.system().lower() == "windows":
                ssh_known_hosts_file = '%s\ssh\known_hosts' % mforms.App.get().get_user_data_folder()
                
        try:
            client.load_host_keys(os.path.expanduser(ssh_known_hosts_file))
        except IOError, e:
            log_warning("IOError, probably caused by file %s not found, the message was: %s\n" % (ssh_known_hosts_file, e))
        
        if "stricthostkeychecking" in opts and opts["stricthostkeychecking"].lower() == "no":
            client.set_missing_host_key_policy(WarningPolicy())
        else:
            client.set_missing_host_key_policy(StoreIfConfirmedPolicy())

        # TODO: Check if file with
        #client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        #client.set_missing_host_key_policy(paramiko.WarningPolicy()) # This is a temp workaround, later we need to have UI with buttons accept

           
        try:
            if 'timeout' in paramiko.SSHClient.connect.func_code.co_varnames:
                client.connect(hostname = host, port = int(port), username = user, password = pwd, #pkey = None,
                                    key_filename = key, timeout = 10, look_for_keys=bool(usekey), allow_agent=bool(usekey) )
            else:
                client.connect(hostname = host, port = int(port), username = user, password = pwd, #pkey = None,
                                    key_filename = key, look_for_keys=bool(usekey), allow_agent=bool(usekey) )
            log_info("%s: Connected via ssh to %s\n" % (self.__class__.__name__, host) )
            if self.keepalive != 0:
                client.get_transport().set_keepalive(self.keepalive)

            self.client = client
        except socket.error, exc:
            import traceback
            log_error("Error opening SSH connection to %s: %s\n" % (host, traceback.format_exc()))

            if exc.args[0] == 8: # [Errno 8] nodename nor servname provided, or not known
                raise ConnectionError("Unable to resolve host name '%s'" % host)
            else:
                raise SSHDownException(str(exc))
            #if exc.args[0] == 61: # connection refused
            #  raise ConnectionError("Could not establish SSH connection: %r.\nMake sure the SSH daemon is running and is accessible." % exc)
            #else:
            #  raise ConnectionError("Could not establish SSH connection: %r.\nMake sure the SSH daemon is running and is accessible." % exc)
        except paramiko.BadHostKeyException, exc:
            raise Exception(format_bad_host_exception(exc, '%s\ssh\known_hosts' % mforms.App.get().get_user_data_folder() if platform.system().lower() == "windows" else "~/.ssh/known_hosts file"))

        except paramiko.PasswordRequiredException, exc:
            if pwd is not None:
                raise ConnectionError("Could not unlock private keys. %s" % exc)
            else:
                raise exc
        except paramiko.BadAuthenticationType, exc:
            import traceback
            log_error("Error opening SSH connection: %s\n" % traceback.format_exc())
            if 'keyboard-interactive' in exc.allowed_types and pwd is not None:
                # wrong password
                raise InvalidPasswordError("Authentication failed for SSH user %s" % user)
            raise ConnectionError("Could not establish SSH connection: %s." % exc)
        except paramiko.AuthenticationException, exc:
            import traceback
            log_error("Error opening SSH connection: %s\n" % traceback.format_exc())
            if usekey:
                raise InvalidPasswordError("Invalid ssh key %s for SSH user %s" % (key ,user))
            else:
                raise InvalidPasswordError("Invalid password for SSH user %s" % user)
        except paramiko.SSHException, exc:
            import traceback
            log_error("Error opening SSH connection: %s\n" % traceback.format_exc())
            raise ConnectionError("Could not establish SSH connection: %s." % exc)
        except SSHFingerprintNewError, exc: # We need to handle this upper in the chain
            raise exc
        except Exception, exc:
            import traceback
            log_error("Error opening SSH connection: %s\n" % traceback.format_exc())
            raise ConnectionError("Could not establish SSH connection. %s." % exc)

    def is_connected(self):
        return self.client is not None

    def mkdir(self, path):
        path = normalize_windows_path_for_ftp(path)
        sftp = self.client.open_sftp()
        try:
            sftp.mkdir(path)
            sftp.close()
        except:
            import traceback
            log_error("Error creating remote dir via ssh: %s\n" % traceback.format_exc())
            sftp.close()
            raise

    def rmdir(self, path):
        sftp = self.client.open_sftp()
        try:
            sftp.rmdir(path)
            sftp.close()
        except:
            import traceback
            log_error("Error removing remote dir via ssh: %s\n" % traceback.format_exc())
            sftp.close()
            raise


    def remove(self, path):
        sftp = self.client.open_sftp()
        try:
            sftp.remove(path)
            sftp.close()
        except:
            import traceback
            log_error("Error removing remote file via ssh: %s\n" % traceback.format_exc())
            sftp.close()
            raise


    def file_exists(self, path):
        path = normalize_windows_path_for_ftp(path)
        ret = False
        if self.client is None:
            raise Exception("wb_admin_ssh: SSH client not connected. file_exists failed")

        sftp = self.client.open_sftp()
        try:
            sftp.stat(path)
            ret = True
            sftp.close()
        except IOError, e:
            sftp.close()
            if e.errno != errno.ENOENT:
              raise
        except:
            import traceback
            log_error("Error checking remote file via ssh: %s\n" % traceback.format_exc())
            sftp.close()
            raise

        return ret

    def stat(self, path):
        path = normalize_windows_path_for_ftp(path)
        ret = None
        sftp = self.client.open_sftp()
        try:
            ret = sftp.stat(path)
            sftp.close()
        except IOError:
            ret = None
            sftp.close()
        except:
            import traceback
            log_error("Error stating remote file via ssh: %s\n" % traceback.format_exc())
            ret = None
            sftp.close()

        return ret
        

    def get(self, source, dest):
        source = normalize_windows_path_for_ftp(source)
        if source is not None:
            source = source.strip("'\"")

        ret = False
        try:
            sftp = self.client.open_sftp()
            sftp.get(source, dest)
            ret = True
            sftp.close()
        except IOError, e:
            log_error('%s: Retrieval of file "%s" failed: %s\n' % (self.__class__.__name__, source, str(e)) )
            raise

        return ret

    def get_contents(self, path): # raises IOError
        path = normalize_windows_path_for_ftp(path)
        
        sftp = self.client.open_sftp()

        # test if sftp session is ok. I get a EACCES doing anything after sftp session is open with freesshd
        try:
            sftp.chdir(".")
        except IOError, exc:
            sftp.close()
            if exc.errno == errno.EACCES:
                raise PermissionDeniedError("Permission denied opening SFTP session. Please ensure the ssh account is correctly setup.")
            raise exc

        try:
            f = sftp.file(path, "r")
        except IOError, exc:
            sftp.close()
            if exc.errno == errno.EACCES:
                raise PermissionDeniedError("Permission denied opening remote file %s for reading: %s" % (path, exc))
            raise exc
        ret = f.read()
        f.close()
        sftp.close()
        return ret

    def set_contents(self, path, data, mode = "w"): # raises IOError
        path = normalize_windows_path_for_ftp(path)
        
        sftp = self.client.open_sftp()
        try:
            f = sftp.file(path, mode)
            f.chmod(stat.S_IREAD | stat.S_IWRITE)
        except IOError, exc:
            sftp.close()
            if exc.errno == errno.EACCES:
                raise PermissionDeniedError("Permission denied opening remote file %s for writing: %s" % (path, exc))
            raise exc
        ret = f.write(data)
        f.close()
        sftp.close()
        return ret

    def put(self, source, dest):
        ret = False
        try:
            sftp = self.client.open_sftp()
            sftp.put(source, dest)
            ret = True
            sftp.close()
        except Exception, e:
            log_error('%s: Sending of file "%s" to the server failed: %s\n' % (self.__class__.__name__, source, str(e)) )
            raise

        return ret
    
    def _read_streams(self, chan, stdout, stderr, read_size, end_text = None, timeout=0, spawn_process = False, wait_output = CmdOutput.WAIT_ALWAYS):
        """
          Will read data from the channel streams with different exit criteria:
          - end_text is specified and found on the read data.
          - timeout is specified and it has completed.
          - If the server indicated the command completed and there's nothing left to read
          
          Rturns a tuple containing:
          - Command return code if completed on the server else None
          - A boolean value indicating if the end_text was found
          - The data read from the stdout stream
          - The data read from the stderr stream
          
          Expected Output
          - Used to determine whether output should be read or not from the executed
            command, possible values include:
            - YES : Will wait for output all the time
            - NO  : Will not wait for output all the time
            - ON_OK : Will only wait for output if the command succeeds
            - ON_ERROR : Will only wait for output if the command fails
          """
        t = time.time()
        all_data = ''
        all_error= ''
        
        srt_timeout = 2
        status_ready_time = None
        
        
        
        # Initializes the exit criteria
        cmd_ret = None
        found_text = False
        read_done = False
        time_complete = False
        
        
        #while not found_text and cmd_ret is None and not time_complete:
        while not found_text and not read_done and not time_complete:
            data = ''
            error = ''
            
            # Reads from the stdout if available
            while chan.recv_ready() and len(data) < read_size:
                data += stdout.read(1)
            if data:
                log_debug2("ssh session stdout [%d]: plain>>>%s<<<   hex>>>%s<<<x\n" % (len(data), data, ' '.join(x.encode('hex') for x in data)))

            # Reads from the stderr if available
            while chan.recv_stderr_ready() and len(error) < read_size:
                error += stderr.read(1)
            if error:
                log_debug2("ssh session stderr [%d]: plain>>>%s<<<   hex>>>%s<<<x\n" % (len(error), error, ' '.join(x.encode('hex') for x in error)))

            # Appends any read data on stdout and stderr
            if data or error:
                all_data += data
                all_error += error
            
            # If nothing was read, checks if the command completed and
            # Reads the exist status
            elif chan.exit_status_ready():
                if cmd_ret is None:
                    cmd_ret = chan.recv_exit_status()
                
                # No need to read output...
                if ((all_data or all_error) and chan.closed) or\
                   (wait_output == CmdOutput.WAIT_NEVER) or\
                   (wait_output == CmdOutput.WAIT_IF_OK and cmd_ret != 0) or\
                   (wait_output == CmdOutput.WAIT_IF_FAIL and cmd_ret == 0):
                   read_done = True
                
                # Needs to read output : YES, ON_OK and RC is 0 and ON_ERROR and RC != 0
                elif status_ready_time:
                    new_time = time.time()
                    diff = new_time - status_ready_time
                    read_done = diff >= srt_timeout
                else:
                    status_ready_time = time.time()
                      

                
            # If end_text was specified, checks if it was found on the
            # read data to exit properly
            if end_text:
                if all_data and all_data.find(end_text) != -1:
                    found_text = True
                
                # Same check on the stderr data if needed
                if spawn_process and all_error and all_error.find(end_text) != -1:
                    found_text = True
            
            # If timeout was specified checks wether it has completed
            if timeout:
                time_complete = time.time() - t >= timeout
        
        return (cmd_ret, found_text, all_data, all_error)


    def _prepare_channel(self, transport):
          channel = None
          stdin = None
          stdout = None
          stderr = None

          channel = transport.open_session()

          if channel:
              channel.setblocking(True)
              channel.settimeout(10)
              channel.get_pty()
              
              bufsize = -1
              stdin = channel.makefile('wb', bufsize)
              stdout = channel.makefile('rb', bufsize)
              stderr = channel.makefile_stderr('rb', bufsize)

          return channel, stdin, stdout, stderr



    def exec_cmd(self, cmd, as_user = Users.CURRENT, user_password = None, output_handler = None, read_size = 128, get_channel_cb = None, options = None):
        out = ""
        error = ""
        ret_code = None
        

        # For some reason we have bool in pwd
        if type(user_password) is unicode:
            user_password = user_password.encode("utf8")

        if type(user_password) is not str:
            user_password = None

        expect_sudo_failure = False

        read_timeout = self.ssh_timeout

        if self.client is not None:
            transport = self.client.get_transport()
            try:
                spawn_process = False
                if 'nohup' in cmd:
                    spawn_process = True
                
                chan = None
                chan, stdin, stdout, stderr = self._prepare_channel(transport)
                wait_output = CmdOutput.WAIT_ALWAYS
                if options and options.has_key(CmdOptions.CMD_WAIT_OUTPUT):
                    wait_output = options[CmdOptions.CMD_WAIT_OUTPUT]

                if chan:
                    ret_code = None
                    initial_data = ""
                    error = ""
                    more_data = True

                    log_debug2("About to execute command through ssh session: %s\n" % cmd)
                    chan.exec_command(cmd)
                    pass_prompt_count = 0
                  
                    if (as_user != Users.CURRENT and user_password is not None):
                      
                        ret_code, prompted, initial_data, error = self._read_streams(chan, stdout, stderr, 1, 'EnterPasswordHere', read_timeout+5, spawn_process, wait_output)
                        log_debug2("%s.exec_cmd initial read for command [%s]:\nRetCode : [%s]\nPrompted : [%s]\nData : [%s]\nError : [%s]\n" % (self.__class__.__name__, cmd, ret_code, prompted, initial_data, error) )
                        
                        if prompted:
                            initial_data = ''
                            
                            # feed the password
                            log_debug2("exec_cmd: sending password to child...\n")
                            
                            try:
                                stdin.write(user_password+"\n")
                                stdin.flush()
                                stdin.close()
                            except socket.error, e:
                                es = str(e)
                                if not('ocket' in es and 'closed' in es):
                                    raise
                            
                            expect_sudo_failure = True # we could get a Sorry or the password prompt again
                        else:
                            if ret_code:
                                log_error("exec_cmd: error on sudo operation : %s\n" % error)
                                more_data = False
                            else:
                                log_debug("exec_cmd: was expecting sudo password prompt, but it never came\n")
            
                                if initial_data:
                                    wait_output = CmdOutput.WAIT_NEVER

                        # Outputs the initial read
                        if output_handler:
                            output_handler(initial_data)
                        else:
                            if prompted:
                                pass_prompt_count=1

                            out = initial_data


                    # TODO: Is this really needed????
                    #       The call with channel get_channel_cb is only done in two places:
                    #       - On the unit tests at the bottom of this file where it is only used to ensure the channel
                    #         is returned. (So the channel is not used)
                    #       - On wba_monitor_be.WinRemoteStats where it is only stored and not used (pretty much the same as the Test class)
                    if get_channel_cb is not None:
                        log_debug3("%s.exec_cmd: Getting channel via passed cb (%s)\n" % (self.__class__.__name__, get_channel_cb.__name__) )
                        get_channel_cb(chan)

                    # Now reads the command output and sends it to the handler if needed.
                    # Note that is is needed to enter this loop even if there's no output 
                    # handler to prevent ending the channel before the command is completed.
                    while more_data:
                        ret_code, prompted, chunk, error = self._read_streams(chan, stdout, stderr, read_size, None, read_timeout, False, wait_output)
                        
                        if chunk or error:
                            out += chunk
                            # Detects an additional password prompt which could come in 2 formats
                            # EnterPasswordHere is checked against pass_prompt_count as out could may or may not contain
                            # the initial password prompt, the Sorry.... 
                            if expect_sudo_failure and (out.count("EnterPasswordHere") > pass_prompt_count or out.count("Sorry, try again") > 0):
                                raise InvalidPasswordError("Incorrect password for sudo")
                            elif output_handler:
                                output_handler(chunk)
                                
                            if error:
                                log_warning("%s.exec_cmd error : %s\n" % (self.__class__.__name__, error))

                        # Quits reading when the exit code is read
                        if ret_code != None:
                            more_data = False

                    if ret_code is None:
                        log_warning("%s: Read from the peer is done, but status code is not available\n" % self.__class__.__name__)

            except paramiko.SSHException, e:
                if chan and chan.recv_ready():
                    out = chan.recv(128)
                log_error("SSHException in SSH: %s\n" % str(e))
                traceback.print_exc()
            except InvalidPasswordError, e:
                raise
            except Exception, e:
                if chan and chan.recv_ready():
                    out = chan.recv(128)
                # A "socket closed" exception happens sometimes when sending the password, for some reason.
                # Not sure, but it could be that the command was accepted without a password (maye sudo doing internal
                # password caching for some time). Until that is investigated, exceptions shouldn't bubble up
                log_error("Exception in SSH: %s\n%s\n" % (str(e), traceback.format_exc()))
            except:
                log_error("Unknown exception in ssh\n")

            if chan is not None:
                chan.close()

        # If the password was sent removes the first occurrence of it from the output
        # This is to prevent the sent password to be included but to not affect the
        # rest of the output
        if out is not None and user_password is not None and not initial_data:
            out = out.replace(user_password, "", 1)

        try:
            log_debug3("%s.exec_cmd(cmd=%s, output=%s. Retcode = %s)\n" % (self.__class__.__name__.encode('utf8'), cmd.encode('utf8'), out.decode('utf8'), ret_code) )
        except (UnicodeDecodeError, UnicodeEncodeError), e:
            log_error("Unexpected output received (byte-string <-> UTF-8 conversion failed).  Below is the sanitised version of output:\n")
            log_error("  %s.exec_cmd(cmd=%s, output=%s. Retcode = %s)\n" % (self.__class__.__name__.encode('utf8','replace'), cmd.encode('utf8','replace'), out.decode('utf8','replace'), ret_code))

        return (out, ret_code)

    def getftp(self):
        self.sftp = WbAdminSFTP(self.client.open_sftp())
        return self.sftp

    def close(self):
        if self.client is not None:
            self.client.close()





            


# === Unit tests ===
if __name__ == "__main__":
    #import threading

    class Settings:
        def __init__(self):
            self.loginInfo = {}
            self.serverInfo = {}

            self.loginInfo['ssh.hostName'] = ''
            self.loginInfo['ssh.useKey']   = 0
            self.loginInfo['ssh.userName'] = ''
            self.loginInfo['ssh.port'] = ''

    settings = Settings()

    class Delegate:
        def get_password_for(self, what):
            return ""

        def reset_password_for(self, what):
            return ""

    wbassh = WbAdminSSH()
    wbassh.wrapped_connect(settings, Delegate())
    ftp = wbassh.getftp()

    print ftp.pwd()
    print ftp.ls('.')
    ftp.cd('OpenVPN')
    print ftp.pwd()
    print ftp.ls('.')

    wbassh.close()
    quit()

    class Test:
        def __init__(self):
            self.chan = None
            self.running = [True]

        def save_channel(self, c):
            print "Saving channel", c
            self.chan = c

        def cpu(self, text):
            text = text.strip(" \r\t\n")
            value = None
            try:
                value = int(text)
            except ValueError:
                value = None
            if value is not None:
                print "CPU", value

        def mem(self, text):
            text = text.strip(" \r\t\n")
            value = None
            try:
                value = int(text)
            except ValueError:
                value = None
            if value is not None:
                print "Mem", value

        def reader(self, ssh_session):
            what = ""
            out = ""
            timeouts = 12
            while self.running[0]: # running is passed in list via "refernce"
                try:
                    ch = ssh_session.recv(1)
                    timeouts = 12
                    if ch == "C":
                        what = self.cpu
                    elif ch == "M":
                        what = self.mem
                    elif ch == "\r" or ch == "\n":
                        if what is not None:
                            what(out)
                        what = None
                        out = ""
                        pass
                    elif ch in "0123456789. ":
                        out += ch
                    else:
                        what = None
                        out = ""
                except socket.timeout:
                    timeouts -= 1
                    if timeouts <= 0:
                        ssh_session.close()
                        raise Exception("Can't read from remote Windows script")




    ts = Test()

    t = threading.Thread(target = wbassh.exec_cmd, args=("cmd /C cscript //NoLogo \"C:\Program Files\MySQL\MySQL Server 5.1\mysql_system_status.vbs\" /DoStdIn", Users.CURRENT, False, ts.reader, 1, ts.save_channel))
    t.setDaemon(True)
    t.start()

    time.sleep(10)

    t.join()
    wbassh.close()
