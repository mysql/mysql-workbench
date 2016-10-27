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

import re
from grt import log_error

def get_exe_path(cmd):
    import os
    import mforms

    filepath = mforms.App.get().get_executable_path(cmd).encode("utf8")
    if len(filepath) != 0:
        return filepath;
    
    filepath, filename = os.path.split(cmd)
    
    def is_executable(filepath):
        return os.path.isfile(filepath) and os.access(filepath, os.X_OK)
    
    if filepath:
        if is_executable(filepath):
            return cmd
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe = os.path.join(path, cmd)
            if is_executable(exe):
                return exe
    return None

def human_size(num):
    for x in ['bytes','KiB','MiB','GiB']:
        if num < 1024.0:
            return "%3.1f %s" % (num, x)
        num /= 1024.0
    return "%3.1f %s" % (num, 'TiB')


def format_duration(t, skip_seconds=False):
    s = int(t % 60)
    m = int((t / 60) % 60)
    h = int((t / 3600) % 24)
    d = int(t / (3600*24))

    res = []
    if d > 0:
        if d == 1:
            res.append("%i day" % d)
        else:
            res.append("%i days" % d)

    if skip_seconds:
        res.append("%i:%02i" % (h, m))
    else:
        res.append("%i:%02i:%02i" % (h, m, s))

    return " ".join(res)



def find_object_with_name(list, name):
    """Finds an object with the given name within a list of objects (such as grt.List).

    Returns the found object or None if there was no object with the given name in the collection.
    """
    for obj in list:
        if obj.name == name:
            return obj
    return None

def find_object_with_old_name(list, name):
    """Finds an object with the given oldName within a list of objects (such as grt.List).

    Returns the found object or None if there was no object with the given name in the collection.
    """
    for obj in list:
        if obj.oldName == name:
            return obj
    return None


def replace_string_parameters(template_string, params):
    if isinstance(params, dict):
        params = list( params.iteritems() )
    return reduce( lambda partial_template_string, rep_tuple: partial_template_string.replace('%'+rep_tuple[0]+'%', str(rep_tuple[1])),
                                [ template_string ] + params
                  )

def parameters_from_dsn(dsn):
    chunks = dsn.split(';')
    params = ( (name, value) for name, value in 
                    ( chunk.split('=', 1) for chunk in chunks if '=' in chunk )
                    if not (value.startswith('%') and value.endswith('%'))
             )
    return dict(params)

def dsn_parameters_to_connection_parameters(dsn_params):
    param_mapping = { 'DRIVER'  : 'driver',
                      'SERVER'  : 'hostName',
                      'UID'     : 'userName',
                      'PWD'     : 'password',
                      'PORT'    : 'port',
                      'DATABASE': 'schema',
                      'DSN'     : 'dsn',
                    }
    return dict( (param_mapping.get(dsn_key.upper(), dsn_key), dsn_value) for dsn_key, dsn_value in dsn_params.iteritems() )
    
    
def check_grt_subtree_consistency(value):
    pass

def server_version_str2tuple(version_str):
    match = re.match(r'^(\d+\.\d+(\.\d+)*).*$', version_str.strip())
    if match:
        return tuple(int(x) for x in match.group(1).split('.'))
    return tuple()

def server_os_path(server_profile):
    """Returns an os.path module specific for the server OS."""
    if server_profile.target_is_windows:
        return __import__('ntpath')
    else:
        return __import__('posixpath')



class Version:
    def __init__(self, major, minor, release=0):
        self.majorNumber = major
        self.minorNumber = minor
        self.releaseNumber = release

    def __str__(self):
        if self.releaseNumber >= 0:
            return "%i.%i.%i" % (self.majorNumber, self.minorNumber, self.releaseNumber)
        else:
            return "%i.%i" % (self.majorNumber, self.minorNumber)

    @classmethod
    def fromgrt(cls, v):
        return Version(v.majorNumber, v.minorNumber, v.releaseNumber)

    @classmethod
    def fromstr(cls, s):
        match = re.match(r'^(\d+\.\d+(\.\d+)*).*$', s.strip())
        if match:
            v = tuple(int(x) for x in match.group(1).split('.'))
        else:
            v = []

        if len(v) == 1:
            return Version(v[0])
        elif len(v) == 2:
            return Version(v[0], v[1])
        elif len(v) == 3:
            return Version(v[0], v[1], v[2])
        else:
            raise ValueError("Invalid version string %s" % s)
    
    def compare(self, other):
        other_version = None
        if isinstance(other, Version):
            other_version = other
        elif isinstance(other, basestring):
            other_version = Version.fromstr(other)
        else:
            raise TypeError("Unexpected type")

        this_version_number = self.majorNumber * 10000 + self.minorNumber * 100 + max(0, self.releaseNumber)
        other_version_number = other_version.majorNumber * 10000 + other_version.minorNumber * 100 + max(0, other_version.releaseNumber)
        
        if this_version_number < other_version_number:
            return -1
        elif this_version_number > other_version_number:
            return 1
        return 0
    
    def __lt__(self, other):
        return self.compare(other) < 0

    def __eq__(self, other):
        return self.compare(other) == 0

    def __ne__(self, other):
        return self.compare(other) != 0

    def __gt__(self, other):
        return self.compare(other) > 0

    def __ge__(self, other):
        return self.compare(other) >= 0

    def __le__(self, other):
        return self.compare(other) <= 0

    def is_supported_mysql_version(self):
        if (self.majorNumber == 5 and self.minorNumber in (1, 5, 6, 7)) or (self.majorNumber == 8 and self.minorNumber == 0):
            return True
        return False

    def is_supported_mysql_version_at_least(self, major, minor = None, release=-1):
        assert type(major) == int or isinstance(major, Version)
        if isinstance(major, Version):
            v = major
            major = v.majorNumber
            minor = v.minorNumber
            release = v.releaseNumber

        # if the version required is older (<) than 5.6, then any server that matches is fine
        # if the version required is newer (>=) than 5.6, then we can only guarantee that known servers versions have some specific feature
        
        if (major == 5 and minor >= 6) or (major == 8 and minor == 0):
            return self.is_supported_mysql_version() and self >= Version(major, minor, release)
        else:
            return self > Version(major, minor, release)

import threading
import Queue

class QueueFile:
    def __init__(self):
        self._cond = threading.Condition()
        self.data = ""
        self._write_done = False

    def write(self, data):
        self._cond.acquire()
        self.data += data
        self._cond.notify()
        self._cond.release()

    def close(self):
        self._cond.acquire()
        self._write_done = True
        self._cond.notify()
        self._cond.release()

    def peek(self, size):
        data = ""
        self._cond.acquire()
        while size > len(self.data) and not self._write_done:
            self._cond.wait()
        if self._write_done:
            data = self.data
        else:
            data = self.data[:size]
        self._cond.release()
        return data

    def read(self, size):
        data = ""
        self._cond.acquire()
        while size > len(self.data) and not self._write_done:
            self._cond.wait()
        if self._write_done:
            data = self.data
            self.data = ""
        else:
            data = self.data[:size]
            self.data = self.data[size:]
        self._cond.release()
        return data

    def readline(self):
        data = ""
        self._cond.acquire()
        find_start = 0
        # in case of VERY long lines (several MBs) this loop can slow down things a lot, so we take a longer break every once in a while to
        # give the thread more time to feed data
        while self.data.find('\n', find_start) < 0 and not self._write_done:
            find_start = len(self.data)
            self._cond.wait()
        pos = self.data.find('\n')
        if self._write_done:
            if pos >= 0:
                pos += 1
                data = self.data[:pos]
                self.data = self.data[pos:]
            else:
                data = self.data
                self.data = ""
        else:
            pos += 1
            data = self.data[:pos]
            self.data = self.data[pos:]
        self._cond.release()
        return data


import multiprocessing
class QueueFileMP:
    def __init__(self, pipe):
        self._queue = pipe
        self._write_done = False
        self._data = ""

    def write(self, data):
        self._queue.put(data)

    def close(self):
        self._queue.put(None)

    def _readup(self, maxloops=4):
        tmp = self._queue.get()
        if tmp is None:
            self._write_done = True
        else:
            l = [tmp]
            # flush the queue
            for i in range(maxloops):
                try:
                    tmp = self._queue.get()
                    if tmp is None:
                        self._write_done = True
                        break
                    l.append(tmp)
                except multiprocessing.Queue.Empty:
                    break
            self._data += "".join(l)

    def peek(self, size):
        while size > len(self._data) and not self._write_done:
            self._readup(0)
        if self._write_done:
            data = self._data
        else:
            data = self._data[:size]
        return data

    def read(self, size):
        data = ""
        while size > len(self._data) and not self._write_done:
            self._readup()
        if self._write_done:
            data = self._data
            self._data = ""
        else:
            data = self._data[:size]
            self._data = self._data[size:]
        return data

    def readline(self):
        data = ""
        find_start = 0
        # in case of VERY long lines (several MBs) this loop can slow down things a lot, so we take a longer break every once in a while to
        # give the thread more time to feed data
        while self._data.find('\n', find_start) < 0 and not self._write_done:
            find_start = len(self._data)
            self._readup()
        pos = self._data.find('\n')
        if self._write_done:
            if pos >= 0:
                pos += 1
                data = self._data[:pos]
                self._data = self._data[pos:]
            else:
                data = self._data
                self._data = ""
        else:
            pos += 1
            data = self._data[:pos]
            self._data = self._data[pos:]
        return data



class WorkerThreadHelper:
    """Worker thread that executes a task and sends messages/updates to a message handler 
        that keeps running from the main thread until it's done."""
    def __init__(self, worker_func, message_handler_func):
        self.worker = worker_func
        self.message_handler = message_handler_func
        self.thread = threading.Thread()
        self.queue = Queue.Queue()
        self.thread.run = self._run
        self._timeout_handle = None
        self._running = False


    def start(self, update_interval = 0.5):
        self.thread.start()
        self._running = True
        import mforms
        self._timeout_handle = mforms.Utilities.add_timeout(update_interval, self._timeout)

    def add_message(self, message):
        self.queue.put(message)

    def _timeout(self):
        while True:
            try:
                message = self.queue.get_nowait()
            except Queue.Empty:
                break
            self.message_handler(message)
        return self._running and self.queue.empty()

    def _run(self):
        try:
            self.worker(self.add_message)
        except Exception, e:
            import traceback
            log_error("WorkerThreadHelper", "An exception occurred in the worker thread:\n%s\n" % traceback.format_exc())
            self.add_message(e)
        self._running = False

