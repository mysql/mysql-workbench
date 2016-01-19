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
import posixpath
import ntpath

write_log = False
logfile = "wbadebug.log"
debug_level = os.getenv("DEBUG_ADMIN")
if debug_level is not None:
    debug_level = int(debug_level)
    import inspect
else:
    debug_level = 0

if debug_level:
    print "Debug level -", debug_level

def dprint_ex(level, *args):
    if level <= debug_level:
        fr = inspect.currentframe().f_back
        cls = ""
        slf = fr.f_locals.get('self')
        if slf:
            cls = str(slf.__class__) + '.'
        ctx = inspect.getframeinfo(fr)
        # In Python 2.5, ctx is a tuple
        #method = cls + ctx.function + ':' + str(ctx.lineno)
        method = cls + ctx[2] + ":" + str(ctx[1])

        msg = method + " : " + " ".join([type(s) is str and s or str(s) for s in args])

        print msg
        if write_log:
            f = open(logfile, "a")
            f.write(msg)
            f.write("\n")
            f.close()



def splitpath(path):
    return posixpath.split(path) if '/' in path else ntpath.split(path)


def parentdir(path):
    return posixpath.dirname(path) if '/' in path else ntpath.dirname(path)


def stripdir(path):
    return posixpath.basename(path) if '/' in path else ntpath.basename(path)


def joinpath(path, *comps):
    return posixpath.join(path, *comps) if '/' in path else ntpath.join(path, *comps)


def sanitize_sudo_output(output):
    # in Mac OS X, XCode sets some DYLD_ environment variables when debugging which dyld
    # doesn't like and will print a warning to stderr, so we must filter that out
    if output.startswith("dyld: "):
        warning, _, output = output.partition("\n")
    return output

#===============================================================================
class Users:
  ADMIN = "root"
  CURRENT = ""

class OperationCancelledError(Exception):
    pass

# Put what is the wrong password in the exception message
class InvalidPasswordError(RuntimeError):
    pass

class PermissionDeniedError(RuntimeError):
    pass

class LogFileAccessError(RuntimeError):
    pass

class ServerIOError(RuntimeError):
    pass

class NoDriverInConnection(RuntimeError):
    pass

class SSHFingerprintNewError(Exception):
      def __init__(self, message, client, hostname, key):
          self.message = message
          self.key = key
          self.hostname = hostname
          self.client = client
          import binascii
          self.fingerprint = binascii.hexlify(self.key.get_fingerprint())
      def __str__(self):
         return self.message


def format_bad_host_exception(exc, known_hosts_filepath):
    import binascii
    return "The host %s fingerprints mismatch.\nExpected key: %s\nServer sent: %s\nPlease verify if it's correct.\nTo continue, delete entries for the host from the %s file." % (exc.hostname, binascii.hexlify(exc.expected_key.get_fingerprint()), binascii.hexlify(exc.key.get_fingerprint()), known_hosts_filepath)

# Decorator to log an exception
def log_error_decorator(method):
    def wrapper(self, error):
        import grt
        grt.log_error(self.__class__.__name__, str(error) + '\n')
        return method(self, error)
    return wrapper


class CmdOptions(object):
    CMD_WAIT_OUTPUT = 0
    CMD_HOME = 1
    
    def __setattr__(self, name, value):
        raise NotImplementedError

class CmdOutput(object):
    WAIT_ALWAYS = 1
    WAIT_NEVER = 2
    WAIT_IF_OK = 3
    WAIT_IF_FAIL = 4
    
    def __setattr__(self, name, value):
        raise NotImplementedError
