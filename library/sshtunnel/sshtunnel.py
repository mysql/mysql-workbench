# Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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
import platform
import threading
import random
import Queue
import traceback
import socket
import select
import sys
import time
import os

import mforms

import paramiko
from workbench.log import log_warning, log_error, log_debug, log_debug2, log_debug3, log_info
from wb_common import SSHFingerprintNewError, format_bad_host_exception

import grt

SSH_PORT = 22
REMOTE_PORT = 3306

# timeout for closing an unused tunnel
TUNNEL_TIMEOUT = 3
SSH_CONNECTION_TIMEOUT = 10

# paramiko 1.6 didn't have this class
if hasattr(paramiko, "WarningPolicy"):
    WarningPolicy = paramiko.WarningPolicy
else:
    class WarningPolicy(paramiko.MissingHostKeyPolicy):
        def missing_host_key(self, client, hostname, key):
            import binascii
            log_warning('WARNING: Unknown %s host key for %s: %s\n' % (key.get_name(), hostname, binascii.hexlify(key.get_fingerprint())))

class StoreIfConfirmedPolicy(paramiko.MissingHostKeyPolicy):
    def missing_host_key(self, client, hostname, key):
        raise SSHFingerprintNewError("Key mismatched", client, hostname, key)

tunnel_serial=0
class Tunnel(threading.Thread):
    
    """This class is a threaded implementation of an SSH tunnel.
    
    You should not access the attributes that starts with an underscore outside this thread
    of execution (e.g. self._server) for this could run into race conditions. Even when
    accessing its public attributes (those that don't start with an underscore) you should
    be careful of acquiring the self.lock reentrant lock (and releasing it once done):
    
        with tunnel.lock:
            if tunnel.connecting:
                ... whatever...
                
    """

    def __init__(self, q, server, username, target, password, keyfile):
        super(Tunnel, self).__init__()
        self.daemon = True

        self._server = server
        self._username = username
        self._target = target
        self._password = password
        self._keyfile = keyfile
        
        # Acquire and release this lock while accessing the attributes of objects
        # of this class from the main thread: 
        self.lock = threading.RLock()
        
        # This event marks when a random port is selected to be used:
        self.port_is_set = threading.Event()

        self.local_port = None
        self._listen_sock = None
        self.q = q

        self._shutdown = False
        self.connecting = False
        
        self._client = paramiko.SSHClient()
        self._connections = []


    def is_connecting(self):
        with self.lock:
            return self.connecting

    def run(self):
        try:
            self.do_run()
        except Exception, e:
            log_error("Unhandled exception in SSH tunnel: %s\n" % e)


    def do_run(self):
        global tunnel_serial
        tunnel_serial += 1
        mforms.Utilities.set_thread_name("SSHTunnel%i"%tunnel_serial)

        sys.stdout.write('Thread started\n')  # sys.stdout.write is thread safe while print isn't
        log_debug2("SSH Tunel %i thread started\n" % tunnel_serial)
        # Create a socket and pick a random port number for it:
        self._listen_sock = socket.socket()
        while True:
                local_port = random.randint(1024, 65535)
                try:
                    self._listen_sock.bind(('127.0.0.1', local_port))
                    self._listen_sock.listen(2)
                    with self.lock:
                        self.local_port = local_port
                    break
                except socket.error, exc:
                    sys.stdout.write('Socket error: %s for port %d\n' % (exc, local_port) )
                    err, msg = exc.args
                    if err == 22:
                        continue # retry
                    self.notify_exception_error('ERROR',"Error initializing server end of tunnel", sys.exc_info())
                    raise exc
                finally:
                    with self.lock:
                        self.connecting = True

                    self.port_is_set.set()

        if self._keyfile:
            self.notify('INFO', 'Connecting to SSH server at %s:%s using key %s...' % (self._server[0], self._server[1], self._keyfile) )
        else:
            self.notify('INFO', 'Connecting to SSH server at %s:%s...' % (self._server[0], self._server[1]) )

        connected = self._connect_ssh()
        if not connected:
            self._listen_sock.close()
            self._shutdown = True

        with self.lock:
            self.connecting = False
            
        if connected:
            self.notify('INFO', 'Connection opened')

        del self._password
        last_activity = time.time()
        while not self._shutdown:
            try:
                socks = [self._listen_sock]
                for sock, chan in self._connections:
                    socks.append(sock)
                    socks.append(chan)
                r, w, x = select.select(socks, [], [], TUNNEL_TIMEOUT)
            except Exception, e:
                if not self._shutdown:
                    self.notify_exception_error('ERROR', 'Error while forwarding data: %r' % e, sys.exc_info())
                break

            if not r and len(socks) <= 1 and time.time() - last_activity > TUNNEL_TIMEOUT:
                self.notify('INFO', 'Closing tunnel to %s:%s for inactivity...' % (self._server[0], self._server[1]) )
                break
            last_activity = time.time()

            if self._listen_sock in r:
                self.notify('INFO', 'New client connection')
                self.accept_client()
            
            closed = []
            for sock, chan in self._connections:
                if sock in r:
                    data = sock.recv(1024)
                    if not data:
                        closed.append((sock, chan))
                    else:
                        chan.send(data)

                if chan in r:
                    data = chan.recv(1024)
                    if not data:
                        closed.append((sock, chan))
                    else:
                        sock.send(data)

            for item in set(closed):  # set() will remove duplicates from closed list
                sock, chan = item
                try:
                    sock.close()
                except:
                    pass
                try:
                    chan.close()
                except:
                    pass
                self.notify('INFO', 'Client for %s disconnected' % local_port)
                self._connections.remove(item)

            if closed and not self._connections and time.time() - last_activity > TUNNEL_TIMEOUT:
                self.notify('INFO', 'Closing tunnel to %s:%s for inactivity...' % (self._server[0], self._server[1]) )
                break
        # Time to shutdown:
        for sock, chan in self._connections:
            try:
                sock.close()
            except:
                pass
            try:
                chan.close()
            except:
                pass
        
        self._listen_sock.close()
        self._client.close()
        log_debug("Leaving tunnel thread %s\n" % self.local_port)
        
    def notify(self, msg_type, msg_object):
        log_debug2("tunnel_%i: %s %s\n" % (self.local_port, msg_type, msg_object))
        self.q.put((msg_type, msg_object))
        
    def notify_exception_error(self, msg_type, msg_txt, msg_obj = None):
        self.notify(msg_type, msg_txt)
        log_error("%s\n" % traceback.format_exc())

    def match(self, server, username, target):
        with self.lock:
            return self._server == server and self._username == username and self._target == target


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

    def _connect_ssh(self):
        """Create the SSH client and set up the connection.
        
        Any exception coming from paramiko will be notified as an error
        that would cause the failure of the connection. Some of these are:
        
        paramiko.AuthenticationException   --- raised when authentication failed for some reason
        paramiko.PasswordRequiredException --- raised when a password is needed to unlock a private key file;
                                               this is a subclass of paramiko.AuthenticationException
        """
        try:
            
            config = paramiko.config.SSHConfig()
            config_file_path = self._get_ssh_config_path()
            if config_file_path:
                with open(config_file_path) as f:
                    config.parse(f)
                
            opts = config.lookup(self._server[0])
            ssh_known_hosts_file = None
            if "userknownhostsfile" in opts:
                ssh_known_hosts_file = opts["userknownhostsfile"]
            else:
                self._client.get_host_keys().clear()
                ssh_known_hosts_file = '~/.ssh/known_hosts'
                
                if platform.system().lower() == "windows":
                    ssh_known_hosts_file = '%s\ssh\known_hosts' % mforms.App.get().get_user_data_folder()

            try:
                self._client.load_host_keys(os.path.expanduser(ssh_known_hosts_file))
            except IOError, e:
                log_warning("IOError, probably caused by file %s not found, the message was: %s\n" % (ssh_known_hosts_file, e))

            if "stricthostkeychecking" in opts and opts["stricthostkeychecking"].lower() == "no":
                self._client.set_missing_host_key_policy(WarningPolicy())
            else:
                self._client.set_missing_host_key_policy(StoreIfConfirmedPolicy())
                
            has_key = bool(self._keyfile)
            self._client.connect(self._server[0], self._server[1], username=self._username,
                                 key_filename=self._keyfile, password=self._password,
                                 look_for_keys=has_key, allow_agent=has_key, timeout=SSH_CONNECTION_TIMEOUT)
        except paramiko.BadHostKeyException, exc:
            self.notify_exception_error('ERROR',format_bad_host_exception(exc, '%s\ssh\known_hosts' % mforms.App.get().get_user_data_folder() if platform.system().lower() == "windows" else "~/.ssh/known_hosts file"))
            return False
        except paramiko.BadAuthenticationType, exc:
            self.notify_exception_error('ERROR', "Bad authentication type, the server is not accepting this type of authentication.\nAllowed ones are:\n %s" % exc.allowed_types, sys.exc_info());
            return False
        except paramiko.AuthenticationException, exc:
            self.notify_exception_error('ERROR', "Authentication failed, please check credentials.\nPlease refer to logs for details", sys.exc_info())
            return False
        except socket.gaierror, exc:
            self.notify_exception_error('ERROR', "Error connecting to SSH server: %s\nPlease refer to logs for details." % str(exc))
            return False
        except paramiko.ChannelException, exc:
            self.notify_exception_error('ERROR', "Error connecting SSH channel.\nPlease refer to logs for details: %s" % str(exc), sys.exc_info())
            return False
        except SSHFingerprintNewError, exc:
            self.notify_exception_error('KEY_ERROR', { 'msg': "The authenticity of host '%(0)s (%(0)s)' can't be established.\n%(1)s key fingerprint is %(2)s\nAre you sure you want to continue connecting?"  % {'0': "%s:%s" % (self._server[0], self._server[1]), '1': exc.key.get_name(), '2': exc.fingerprint}, 'obj': exc})
            return False
        except IOError, exc:
            #Io should be report to the user, so maybe he will be able to fix this issue
            self.notify_exception_error('IO_ERROR', "IO Error: %s.\n Please refer to logs for details." % str(exc), sys.exc_info())
            return False

        except Exception, exc:
            self.notify_exception_error('ERROR', "Authentication error, unhandled exception caught in tunnel manager, please refer to logs for details", sys.exc_info())
            return False
        else:
            log_debug("connect_ssh2 OK\n")
            return True

    def close(self):
        self.notify('INFO', 'Closing tunnel')
        self._listen_sock.close()
        self._shutdown = True

    def accept_client(self):
        try:
            local_sock, peeraddr = self._listen_sock.accept()
        except Exception, e:
            self.notify_exception_error('ERROR', 'Error accepting new tunnel client: %r' % e,sys.exc_info())
            return
        self.notify('INFO', 'Client connection established')

        transport = self._client.get_transport()

        try:
            sshchan = transport.open_channel('direct-tcpip', self._target, local_sock.getpeername())
        except paramiko.ChannelException, exc:
            self.notify_exception_error('ERROR', 'Could not open port forwarding SSH channel: %s' % exc)
            local_sock.close()
            return
        except Exception, e:
            self.notify_exception_error('ERROR', 'Remote connection to %s:%d failed: %r' % (self._target[0], self._target[1], e), sys.exc_info())
            local_sock.close()
            return

        if sshchan is None:
            self.notify_exception_error('ERROR', 'Remote connection to %s:%d was rejected by the SSH server.' % (self._target[0], self._target[1]), sys.exc_info())
            local_sock.close()
            return

        self.notify('INFO', 'Tunnel now open %r -> %r -> %r' % (local_sock.getsockname(), sshchan.getpeername(), self._target))

        self._connections.append((local_sock, sshchan))

class TunnelManager:
    def __init__(self):
        self.tunnel_by_port = {}

        self.inpipe = sys.stdin
        self.outpipe = sys.stdout

    def _address_port_tuple(self, raw_address, default_port):
        if type(raw_address) is str:
            if ':' in raw_address:
                address, port = raw_address.split(':', 1)
                try:
                    port = int(port)
                except:
                    port = default_port
                return (address, port)
            else:
                return (raw_address, default_port)
        else:
            return raw_address

    def lookup_tunnel(self, server, username, target):
        server = self._address_port_tuple(server, default_port=SSH_PORT)
        target = self._address_port_tuple(target, default_port=REMOTE_PORT)

        for port, tunnel in self.tunnel_by_port.iteritems():
            if tunnel.match(server, username, target) and tunnel.isAlive():
                with tunnel.lock:
                    return tunnel.local_port
        return None

    def open_tunnel(self, server, username, password, keyfile, target):
        try:
            port = self.open_ssh(server, username, password, keyfile, target)
        except Exception:
            traceback.print_exc()
            return (False, str(traceback.format_exc()))
        return (True, port)
   
    def open_ssh(self, server, username, password, keyfile, target):
        server = self._address_port_tuple(server, default_port=SSH_PORT)
        target = self._address_port_tuple(target, default_port=REMOTE_PORT)
        
        password = password or ''
        keyfile  = keyfile  or None

        if keyfile is not None:
            keyfile = keyfile.decode('utf-8')

        found = None
        for tunnel in self.tunnel_by_port.itervalues():
            if tunnel.match(server, username, target) and tunnel.isAlive():
                found = tunnel
                break

        if found:
            with tunnel.lock:
                log_debug('Reusing tunnel at port %d' % tunnel.local_port)
                return tunnel.local_port
        else:
            tunnel = Tunnel(Queue.Queue(), server, username, target, password, keyfile)
            tunnel.start()
            tunnel.port_is_set.wait()
            with tunnel.lock:
                port = tunnel.local_port
            self.tunnel_by_port[port] = tunnel
            return port


    def wait_connection(self, port):
        tunnel = self.tunnel_by_port.get(port)
        if not tunnel:
            return 'Could not find a tunnel for port %d' % port
        error = None
        close_tunnel = False
        tunnel.port_is_set.wait()
        if tunnel.isAlive():
            while True:
                # Process any message in queue. Every retrieved message is printed.
                # If an error is detected in the queue, exit returning its message:
                try:
                    msg_type, msg = tunnel.q.get_nowait()
                except Queue.Empty:
                    continue
                else:
                    if msg_type == 'KEY_ERROR':
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
                                except IOError, e:
                                    error = str(e)
                                    break;
                            error = "Server key has been stored"
                        else:
                            error = "User cancelled"
                        close_tunnel = True
                        break # Exit returning the error message
                    elif msg_type == 'IO_ERROR':
                        error = msg
                        break # Exit returning the error message
                    else:
                        time.sleep(0.3)
                    _msg = msg
                    if type(msg) is tuple:
                        msg = '\n' + ''.join(traceback.format_exception(*msg))
                        _msg = str(_msg[1])
                    log_debug("%s: %s\n" % (msg_type, msg))
                    if msg_type == 'ERROR':
                        error = _msg
                        break  # Exit returning the error message
                
                if (not tunnel.is_connecting() or not tunnel.isAlive()) and tunnel.q.empty():  
                    break
                time.sleep(0.3)
        log_debug("returning from wait_connection(%s): %s\n" % (port, error))
        # we need to close tunnel so it get opened again, without it we may have problems later
        if close_tunnel:
            tunnel.close()
            del self.tunnel_by_port[port]
        return error


    def get_message(self, port):
        if port not in self.tunnel_by_port:
            log_error("Looking up invalid port %s\n" % port)
            return None
        tunnel = self.tunnel_by_port[port]
        try:
            return tunnel.q.get_nowait()
        except Queue.Empty:
            return None


    def set_keepalive(self, port, keepalive):
        if keepalive == 0:
            log_info("SSH KeepAlive setting skipped.\n")
            return
        tunnel = self.tunnel_by_port.get(port)
        if not tunnel:
            log_error("Looking up invalid port %s\n" % port)
            return
        transport = tunnel._client.get_transport()
        if transport is None:
            log_error("SSHTransport not ready yet %d\n" % port)
            return

        transport.set_keepalive(keepalive)

    def close(self, port):
        pass
        # tunnels auto-close when inactive
        #tunnel = self.tunnel_by_port.get(port, None)
        #if tunnel:
        #    tunnel.num_clients -= 1
        #    if tunnel.num_clients == 0:
        #        tunnel.close()
        #        del self.tunnel_by_port[port]

    def send(self, code, arg=''):
        if arg:
            self.outpipe.write(code + ' ' + arg + '\n')
        else:
            self.outpipe.write(code + '\n')
        self.outpipe.flush()
    
    def shutdown(self):
        for tunnel in self.tunnel_by_port.itervalues():
            tunnel.close()
            tunnel.join()

    # FIXME: It seems that this function is never called. Should we remove it?
    def wait_requests(self):
        #print "SSH Tunnel Manager started, waiting for requests..."
        self.send("READY")
        while True:
            request = self.inpipe.readline()
            if not request:
                #print "Exiting tunnel manager..."
                break
            try:
                cmd, args = eval(request, {}, {})
            except:
                self.send("ERROR", "Invalid request")
                continue
            if cmd == "LOOKUP":
                try:
                    port = self.lookup_tunnel(*args)
                    if port is not None:
                        self.send("OK", str(port))
                    else:
                        self.send("ERROR", "not found")
                except Exception, exc:
                    self.send("ERROR", str(exc))
            elif cmd == "OPENSSH":
                try:
                    port = self.open_ssh(*args)
                    self.send("OK", str(port))
                except Exception, exc:
                    self.send("ERROR", str(exc))
            elif cmd == "CLOSE":
                #self.close(args[0])
                self.send("OK")
            elif cmd == "WAIT":
                # wait for the SSH connection to be established
                error = self.wait_connection(args)
                if not error:
                    self.send("OK")
                else:
                    self.send("ERROR "+error)
            elif cmd == "MESSAGE":
                msg = self.get_message(args)
                if msg:
                    self.send(msg)
                else:
                    self.send("NONE")
            else:
                log_error("Invalid request %s\n" % request)
                self.send("ERROR", "Invalid request")

"""
if "--single" in sys.argv:
    target = sys.argv[2]
    if "-pw" in sys.argv:
        password = sys.argv[sys.argv.index("-pw")+1]
    else:
        password = None
    if "-i" in sys.argv:
        keyfile = sys.argv[sys.argv.index("-i")+1]
    else:
        keyfile = None
    server = sys.argv[-1]

    tunnel = Tunnel(None, False)
    if "@" in server:
        username, server = server.split("@", 1)
    else:
        username = ""
    print "Starting tunnel..."
    if type(server) == str:
        if ':' in server:
            server = server.split(":", 1)
            server = (server[0], int(server[1]))
        else:
            server = (server, SSH_PORT)
    if type(target) == str:
        if ':' in target:
            target = target.split(":", 1)
            target = (target[0], int(target[1]))
        else:
            target = (target, REMOTE_PORT)
    
    tunnel.start(server, username, password or "", keyfile, target)

else:
    tm = TunnelManager()
    sys.stdout = sys.stderr
    try:
        tm.wait_requests()
    except KeyboardInterrupt, e:
	    pass

"""
