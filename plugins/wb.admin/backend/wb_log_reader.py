# Copyright (c) 2011, 2016, Oracle and/or its affiliates. All rights reserved.
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

"""
.. module:: wb_log_reader
   :synopsis: Reads and parses a log source to retrieve sets of records from it.

This module defines several classes to handle MySQL server logs. It supports logs
stored in the database as well as logs stored in files.

All of the defined classes adhere to a common interface defining and implementing
these public attributes and methods:

Attributes:

    column_specs (tuple):  Specifies each field in the log entries. The elements
            of this tuple are also tuples having the form
            (column_name, column_widh, [column_table_name])
            where:
                column_name (str):  A human readable name for the column. Frontend
                                    code should use this name wherever a column
                                    title is needed.
                column_width (int): The recommended with of the column
                column_table_name (str):  (Optional) the name of the field referred
                                          by this column in the log table for DB logs

    partial_support:        False if the log source is fully supported or a
                            string explaining the limitations regarding the implemented
                            log source reader class otherwise.

Methods:

    has_previous():    Returns True if there are older entries that can be
                       retrieved and False otherwise.

    has_next():        Returns True if there are newer entries that can be
                       retrieved and False otherwise.

    first():           Returns a list of the first (oldest) records in the log.
                       Each element in this list represents a single log entry
                       and is also a list whose elements are the values for the
                       columns defined in `column_specs`.

    last():            The same as `first()` but the records returned are the
                       newest ones.

    previous():        Returns the records that precede the last retrieved
                       records. Before calling it you should verify that
                       `has_previous()` returns True.

    next():            Returns the records that follow the last retrieved
                       records. Before calling it you should verify that
                       `has_next()` returns True.

    current():         Returns the last retrieved records.

    range_text():      Returns a string that gives an indication of the position
                       of the current records in the existent log set (if
                       available). E.g. 'Records 1..50 of 145'


    refresh():         After calling this function the log reader should be able
                       to manage new log entries that were added since the last
                       call to this function or since the creation of the log
                       reader object. This function doesn't return anything.

If it is not possible to read the log entries, the class should raise an
exception with a descriptive message to let the user know the reasons of
the failure.

Current limitations:
----------------------

* No remote server support for logs stored in files.

* Cannot read files that aren't readable by the user running Workbench.

"""

import re

from workbench.log import log_info, log_error, log_warning

from wb_server_management import SudoTailInputFile, LocalInputFile, SFTPInputFile
from wb_common import LogFileAccessError, ServerIOError, InvalidPasswordError
from workbench.utils import server_os_path

import time
import datetime
import calendar

def ts_iso_to_local(ts, fmt):
    if ts[-1] == "Z":
        ts = ts[:-1]
    if "." in ts: # strip the millisecond part
        ts, _, ms = ts.partition(".")
        ms = "."+ms
    else:
        ms = ""
    try:
        local_time = calendar.timegm(datetime.datetime.strptime(ts, "%Y-%m-%dT%H:%M:%S").timetuple())
        return time.strftime(fmt, time.localtime(local_time))+ms
    except Exception, e:
        log_warning("Error parsing timestamp %s: %s\n" % (ts, e))
        return ts



#========================= Query Based Readers ================================

class BaseQueryLogReader(object):
    """
    The base class for logs stored in a database.

    **This is not intended for direct instantiation.**
    """
    def __init__(self, ctrl_be, log_table, column_specs, ordering_column):
        """Constructor

        :param ctrl_be:  Control backend instance to make queries
        :param log_table: The name of the table where the log entries are stored
        :type log_table: str
        :param column_specs: Column definitions as explained in the module docstring
        :type column_specs: tuple
        :param ordering_column: The index for the column in `column_specs` that stores
                                the timestamp of the log entries
        :type ordering_column: int
        """
        self.log_table = log_table
        self.log_file = None
        self.ctrl_be = ctrl_be
        self.column_specs = column_specs
        self.ordering_column = ordering_column

        self.partial_support = False

        self.total_count = 0
        self.refresh()  # Updates self.total_count
        self.show_count = 50
        self.show_start = max(self.total_count - self.show_count, 0)
        self.colnames = [ colspec[2] for colspec in column_specs ]

    def has_previous(self):
        return self.show_start > 0

    def has_next(self):
        return self.show_start < self.total_count - 1

    def current(self):
        return self._query_records()

    def previous(self):
        self.show_start = max(self.show_start - self.show_count, 0)
        return self._query_records()

    def next(self):
        self.show_start = min(self.show_start + self.show_count, self.total_count)
        return self._query_records()

    def first(self):
        self.show_start = 0
        return self._query_records()

    def last(self):
        self.show_start = max(self.total_count - self.show_count, 0)
        return self._query_records()

    def size_text(self):
        return '%d records' % self.total_count

    def range_text(self):
        return 'Records %d..%d of %d' % (self.show_start,
                                         min(self.show_start + self.show_count, self.total_count),
                                         self.total_count)

    def refresh(self):
        try:
            result = self.ctrl_be.exec_query("SELECT count(*) AS count FROM %s" % self.log_table)
        except Exception, e:
            raise ServerIOError('Error fetching log contents: %s' % e)
        if not result or not result.nextRow():
            raise ServerIOError('Error fetching log contents')
        self.total_count = result.intByName('count')

    def _query_records(self):
        query = "SELECT * FROM %s ORDER BY %s DESC LIMIT %i, %i"  % (
                        self.log_table,
                        self.colnames[self.ordering_column],
                        self.show_start,
                        self.show_count)
        try:
            result = self.ctrl_be.exec_query(query)
        except Exception, e:
            raise ServerIOError('Error fetching log contents: %s' % e)

        records = []
        if result:
            while result.nextRow():
                row = [ result.stringByName(colname) for colname in self.colnames ]
                records.append(row)
        elif self.total_count:
            raise IOError('There were problems querying the server table %s.' % self.log_table)

        return records


class GeneralQueryLogReader(BaseQueryLogReader):
    def __init__(self, ctrl_be, table_name='mysql.general_log'):
        column_specs = (
            ('Time', 150, 'event_time'),
            ('From', 120, 'user_host'),
            ('Thread', 80, 'thread_id'),
            ('Server', 80, 'server_id'),
            ('Command Type', 80, 'command_type'),
            ('Detail', 500, 'argument')
                        )
        self.detail_column = 5
        super(GeneralQueryLogReader, self).__init__(ctrl_be, table_name, column_specs, 0)

class SlowQueryLogReader(BaseQueryLogReader):
    def __init__(self, ctrl_be, table_name='mysql.slow_log'):
        column_specs = (
            ('Start Time', 150, 'start_time'),
            ('From', 120, 'user_host'),
            ('Query Time', 150, 'query_time'),
            ('Lock Time', 150, 'lock_time'),
            ('Rows Sent', 50, 'rows_sent'),
            ('Rows Examined', 50, 'rows_examined'),
            ('DB', 80, 'db'),
            ('Last Insert ID', 50, 'last_insert_id'),
            ('Insert ID', 50, 'insert_id'),
            ('Server ID', 50, 'server_id'),
            ('SQL', 500, 'sql_text'),
                        )
        self.detail_column = 10
        super(SlowQueryLogReader, self).__init__(ctrl_be, table_name, column_specs, 0)


#========================= File Based Readers =================================

class EventLogInput(object):
    def __init__(self, ctrl_be, path):
        self.ctrl_be = ctrl_be
        self.path = "stderr"

    def tell(self):
        return 0

    @property
    def size(self):
        return 0

    def get_range(self, start, end):
        return ""

    def start_read_from(self, offset):
        return ""

    def read(self, count):
        return ""

    def readline(self):
        return ""


class BaseLogFileReader(object):
    '''
        The base class for logs stored in files unreadable to the current user.

        **This is not intended for direct instantiation.**
        '''
    def __init__(self, ctrl_be, log_file_param, pat, chunk_size, truncate_long_lines, append_gaps=True):
        """Constructor

            :param ctrl_be:  Control backend instance to retrieve root password if needed
            :param log_file_param: The path to the log file to read from or a file like instance to
            read log entries from
            :type log_file_param: str/file
            :param pat: A regular expression pattern that matches a log entry
            :type pat: regular expression object
            :param chunk_size: The size in bytes of the chunks that are read from the log file
            :type chunk_size: int
            :param truncate_long_lines: Whether the log entries that are long should be abbreviated
            :type truncate_long_lines: bool
            :param append_gaps: Whether the data between the end of the record regex and the start
            of the next record regex should be added to the previous entry
            :type append_gaps: (not used)
            """

        self.pat = pat  # the regular expression that identifies a record
        self.pat2 = pat # subclasses may override this if they want a different pattern for parsing chunks
        self.append_gaps = append_gaps
        # regex are considered to belong to the last field of the record
        self.truncate_long_lines = truncate_long_lines
        self.ctrl_be = ctrl_be

        self.partial_support = False

        # If there isn't a directory component in the path, use @@datadir as the directory for the log file:
        log_file_param = log_file_param.strip(' "')
        ospath = server_os_path(self.ctrl_be.server_profile)
        datadir = self.ctrl_be.server_profile.datadir
        if not datadir and ctrl_be.is_sql_connected():
            datadir = self.ctrl_be.get_server_variable('datadir')
        self.log_file_name = log_file_param if ospath.isabs(log_file_param) else ospath.join(datadir, log_file_param)

        use_sftp = False
        use_event_viewer = False
        if self.ctrl_be.server_profile.target_is_windows:
            use_sudo = False
            # In Windows we can either access the file locally as a plain file or remotely with sftp
            # Ther is no upport for reading log files that are only readable by the admin
            if log_file_param.lower() == "stderr":
                use_event_viewer = True
            if not self.ctrl_be.server_profile.is_local:
                if not self.ctrl_be.server_profile.remote_admin_enabled:
                    raise LogFileAccessError('''You have not enabled remote administration for this server. Without it this log file cannot be shown.
                        Please enable remote administration in this server instance and try again.''')
                if not self.ctrl_be.server_profile.uses_ssh:
                    raise LogFileAccessError('''Remote log files are only supported for SSH connection.
                        Please configure an SSH connection and try again.''')
                use_sftp = True
        else:
            use_sudo = self.ctrl_be.server_profile.use_sudo
            # In Mac/Linux, we can access local files with sudo and remote files with sudo too
            if not self.ctrl_be.server_profile.is_local:
                if not self.ctrl_be.server_profile.remote_admin_enabled:
                    raise LogFileAccessError('''You have not enabled remote administration for this server. Without it this log file cannot be shown.
                        Please enable remote administration in this server instance and try again.''')

                if not self.ctrl_be.server_profile.uses_ssh:
                    raise LogFileAccessError('''Remote log files are only supported for SSH connection.
                        Please configure an SSH connection and try again.''')
                use_sftp = True

        if use_sudo:
            log_info("Will use sudo and dd to get contents of log file %s\n" % self.log_file_name)
            password = ctrl_be.password_handler.get_password_for('file', cached_only=True)

            retry = False
            try:
                self.log_file = SudoTailInputFile(self.ctrl_be.server_helper, self.log_file_name, password)
                self.file_size = self.log_file.size
            except InvalidPasswordError, error:
                if password is None:
                    retry = True
                else:
                    log_error("Invalid password to sudo %s\n" % error)
                    ctrl_be.password_handler.reset_password_for('file')
                    raise
            if retry: # either there was no password cached or the cached password was wrong, so retry interactively
                password = ctrl_be.password_handler.get_password_for('file', cached_only=False)
                try:
                    self.log_file = SudoTailInputFile(self.ctrl_be.server_helper, self.log_file_name, password)
                    self.file_size = self.log_file.size
                except InvalidPasswordError, error:
                    log_error("Invalid password to sudo %s\n" % error)
                    ctrl_be.password_handler.reset_password_for('file')
                    raise
        elif use_event_viewer:
            if not self.ctrl_be.server_profile.is_local:
                raise LogFileAccessError('''An attempt to read events from the remote server failed. Events can only be read from the local machine, 
                    hence installed MySQL Workbench on the remote machine to allow showing the server's event log.''')
            self.log_file = EventLogInput(self.ctrl_be, self.log_file_name)
            self.file_size = self.log_file.size
        elif use_sftp:
            log_info("Will use sftp to get contents of log file %s\n" % self.log_file_name)
            self.log_file = SFTPInputFile(self.ctrl_be, self.log_file_name)
            self.file_size = self.log_file.size
        else:
            log_info("Will use plain file access to get contents of log file %s\n" % self.log_file_name)
            self.log_file = LocalInputFile(self.log_file_name)
            self.file_size = self.log_file.size

        self.chunk_size = chunk_size
        # chunk_start is the start of the chunk to be read (ie, we start reading from the end of the file, so we get the last page starting from there)
        self.chunk_start = max(0, self.file_size - chunk_size)
        self.chunk_end = self.file_size

        self.record_count = 0



    def has_previous(self):
        '''
            If there is a previous chunk that can be read.
            '''
        return self.chunk_start > 0

    def has_next(self):
        '''
            If there is a next chunk that can be read.
            '''
        return self.chunk_end != self.file_size

    def range_text(self):
        return '%s records starting at byte offset %s' % (self.record_count, self.chunk_start)

    def size_text(self):
        return self._format_size(self.file_size)

    def _format_size(self, bytes):
        '''
            Returns a string with a human friendly representation of a file size
            '''
        if bytes <= 0:
            return '0 B'
        units = (
                 (1., 'B'),
                 (1024., 'kB'),
                 (1024*1024., 'MB'),
                 (1024*1024*1024., 'GB'),
                 )
        for idx, unit in enumerate(units):
            if bytes < unit[0]:
                return '%.1f %s' % (bytes/units[idx-1][0], units[idx-1][1])
        return '%.1f %s' % (bytes/units[-1][0], units[-1][1])

    def _get_offset_to_first_record(self, data):
        match = self.pat.search(data)
        if match:
            return match.start()
        return 0

    def _extract_record(self, found):
        return list(found.groups())

    def _parse_chunk(self, data):
        '''
            Extracts the records from a chunk of data.
            '''
        records = []
        found = self.pat2.search(data)
        if found:
            end = found.start()
            if self.chunk_start > 0:  # optional?
                self.chunk_start += end
        while found:
            start = found.start()
            if start-end > 1 and self.append_gaps:  # there's a gap between occurrences of the pattern
                records[-1][-1] += data[end:start]  # append the gap to previous record
            records.append( self._extract_record(found) )
            end = found.end()
            found = self.pat2.search(data, end)
            if found and self.truncate_long_lines:  # shorten all but the last record
                records[-1][-1] = self._shorten_query_field(records[-1][-1])
        if records:
            if self.append_gaps:
                records[-1][-1] += data[end:]  # add what remains in data
            if self.truncate_long_lines:
                records[-1][-1] = self._shorten_query_field(records[-1][-1])  # now shorten the last record

        self.record_count = len(records)
        return records

    def _shorten_query_field(self, data):
        '''
            Receives a query stored in a log file and prepares it for the output in
            the log viewer shortening to 256 characters and taking care of encoding issues
            '''
        l = len(data)
        try:
            abbr = data[:256].encode('utf-8')
        except ValueError:
            abbr = data[:256].decode('latin1').encode('utf-8')
        size = '%d bytes' % l if l < 1024 else '%.1f KB' % (l / 1024.0)
        return abbr if l <= 256 else abbr + ' [truncated, %s total]' % size


    def current(self):
        '''
            Returns a list with the records in the current chunk.
            Each record is a list with the values for each column of
            the corresponding log entry.
            '''
        data = self.log_file.get_range(self.chunk_start, self.chunk_end)
        if self.chunk_start > self.chunk_size / 10:
            # adjust the start of the current chunk to the start of the 1st record (if we're not too close to the top)
            offset = self._get_offset_to_first_record(data)
            self.chunk_start += offset
        else:
            offset = 0
        return self._parse_chunk(data[offset:])

    def previous(self):
        '''
            Returns a list with the records in the previous chunk.
            Each record is a list with the values for each column of
            the corresponding log entry.
            '''
        if self.chunk_start == 0:
            return []
        self.chunk_end = self.chunk_start
        self.chunk_start = max(0, self.chunk_start - self.chunk_size)
        return self.current()
    
    def next(self):
        '''
            Returns a list with the records in the next chunk.
            Each record is a list with the values for each column of
            the corresponding log entry.
            '''
        if self.chunk_end == self.file_size:
            return []
        self.chunk_start = self.chunk_end
        self.chunk_end = min(self.chunk_start + self.chunk_size, self.file_size)
        return self.current()
    
    def first(self):
        '''
            Returns a list with the records in the first chunk
            '''
        self.chunk_start = 0
        self.chunk_end = self.chunk_size
        return self.current()
    
    def last(self):
        '''
            Returns a list with the records in the first chunk
            '''
        self.chunk_start = max(0, self.file_size - self.chunk_size)
        self.chunk_end = self.file_size
        return self.current()

    def refresh(self):
        '''
            Checks if the log file has been updated since it was opened and if so
            reopen the file again to keep going with the changes.
            Warning: this function only supports appending to the log file.
            '''
        if self.log_file.path == "stderr":
            return
        else:
            new_size = self.log_file.size
            if new_size != self.file_size:
                self.file_size = new_size
                self.chunk_start = max(0, self.file_size - self.chunk_size)
                self.chunk_end = self.file_size


#==============================================================================
class ErrorLogFileReader(BaseLogFileReader):
    '''
    This class enables the retrieval of log entries in a MySQL error
    log file.
    '''
    def __init__(self, ctrl_be, file_name, chunk_size=64 * 1024, truncate_long_lines=True):
        # The error log is a mess, there are several different formats for each entry and a new one comes up every version
        mysql_56 = r'^(?P<v56>(\d{2,4}-\d{1,2}-\d{2} {1,2}\d{1,2}:\d{2}:\d{2}) (\d+) \[(.*)\] (.*?))$'
        # this is also the format used by mysqld_safe
        mysql_55 = r'^(?P<v55>(\d{6} {1,2}\d{1,2}:\d{2}:\d{2}) {1,2}([^ ]*) (.*?))$'
        mysql_pre55 = r'^(?P<old>(\d{2})(\d{2})(\d{2}) {1,2}(\d{1,2}:\d{2}:\d{2}) ([a-zA-Z0-9_]*?) (.*?))$'
        mysql_57 = r'^(?P<v57>(\d{2,4}-\d{1,2}-\d{2}T{1,2}\d{1,2}:\d{2}:\d{2}.\d+Z) (\d+) \[(.*)\] (.*?))$'

        # add new formats to the end, or you'll have a hard time adjusting indexes
        partial_re = '|'.join([mysql_56, mysql_55, mysql_pre55, mysql_57])

        pat = re.compile(partial_re, re.M)
        super(ErrorLogFileReader, self).__init__(ctrl_be, file_name, pat, chunk_size, truncate_long_lines, append_gaps=False)
        if self.log_file.path == "stderr":
            self.column_specs = (
                ('Timestamp', 150),
                ('Thread', 100),
                ('Level', 100),
                ('Event message', 500),
                            )
            self.column_keys = ('timecreated', 'threadid', 'level', 'message')
        else:
            self.column_specs = (
                    ('Timestamp', 150),
                    ('Thread', 100),
                    ('Type', 100),
                    ('Details', 500),
                                )
        # regex that matches all known entry formats + anything else
        self.pat2 = re.compile(partial_re + r'|^(?P<any>.+)$', re.M)

        self.detail_column = 3

    def _extract_record(self, found):
        gdict = found.groupdict()
        g = found.groups()
        if gdict['v56']:
            return list(g[1:5])
        elif gdict['v55']:
            ts = g[6]
            return ["20%s-%s-%s %s" % (ts[:2], ts[2:4], ts[4:6], ts[6:].lstrip()), "", g[7], g[8]]
        elif gdict['old']:
            return ["20%s-%s-%s %s" % (g[10], g[11], g[12], g[13]), "", g[14], g[15]]
        elif gdict['v57']:
            return [ts_iso_to_local(g[17], "%F %T"), g[18], g[19], g[20]]
        else:
            return ["", "", "", g[-1]]

    def current(self):
        records = super(ErrorLogFileReader, self).current()
        if self.chunk_end < self.file_size:
            # check if the last record is truncated
            rec = records[-1]
            if not any(f != "" for f in rec[:-1]):
                # the last record wasn't recognized, it's probably truncated so leave it out of this chunk
                self.chunk_end -= len(rec[-1])
                del records[-1]
        return records


#==============================================================================
class GeneralLogFileReader(BaseLogFileReader):
    '''
    This class enables the retrieval of log entries in a MySQL general query
    log file.
    '''
    def __init__(self, ctrl_be, file_name, chunk_size=64 * 1024, truncate_long_lines=True):
        pat = re.compile(r'^(?P<v57>(\d{2,4}-\d{1,2}-\d{2}T{1,2}\d{1,2}:\d{2}:\d{2}.\d+Z)[\t ]*(\d+)\s*(.*?)(?:\t+| {2,})(.*?))$|^(?P<v56>(\d{6} {1,2}\d{1,2}:\d{2}:\d{2}[\t ]+|[\t ]+)(\s*\d+)(\s*.*?)(?:\t+| {2,})(.*?))$', re.M)

        super(GeneralLogFileReader, self).__init__(ctrl_be, file_name, pat, chunk_size, truncate_long_lines)
        self.column_specs = (
                ('Timestamp', 150),
                ('Thread', 80),
                ('Command Type', 80),
                ('Detail', 500),
                            )
        self.detail_column = 3


    def _extract_record(self, found):
        gdict = found.groupdict()
        g = found.groups()
        if gdict['v57']:
            return [ts_iso_to_local(g[1], "%F %T"), g[2], g[3], g[4]]
        else: # v56
            return list(g[6:10])


#==============================================================================
class SlowLogFileReader(BaseLogFileReader):
    '''
    This class enables the retrieval of log entries in a MySQL slow query
    log file.
    '''
    def __init__(self, ctrl_be, file_name, chunk_size=64 * 1024, truncate_long_lines=True, append_gaps=False):
        mysql_57 = r'(?:^|\n)(?P<v57># Time: (\d{2,4}-\d{1,2}-\d{2}T{1,2}\d{1,2}:\d{2}:\d{2}.\d+Z).*?\n# User@Host: (.*?)\n# Query_time: +([0-9.]+) +Lock_time: +([\d.]+) +Rows_sent: +(\d+) +Rows_examined: +(\d+)\s*\n(.*?)(?=\n# |\n[^\n]+, Version: |$))'
        mysql_56 = r'(?:^|\n)(?P<v56># Time: (\d{6} {1,2}\d{1,2}:\d{2}:\d{2}).*?\n# User@Host: (.*?)\n# Query_time: +([0-9.]+) +Lock_time: +([\d.]+) +Rows_sent: +(\d+) +Rows_examined: +(\d+)\s*\n(.*?)(?=\n# |\n[^\n]+, Version: |$))'
        pat = re.compile('|'.join([mysql_57, mysql_56]), re.S)
        super(SlowLogFileReader, self).__init__(ctrl_be, file_name, pat, chunk_size, truncate_long_lines, append_gaps)
        self.column_specs = (
                ('Start Time', 150),
                ('User@Host', 80),
                ('Query Time', 80),
                ('Lock Time', 80),
                ('Rows Sent', 80),
                ('Rows Examined', 80),
                ('Detail', 500),
                            )
        self.detail_column = 6

    def _extract_record(self, found):
        gdict = found.groupdict()
        g = found.groups()
        if gdict['v57']:
            # convert timezone from UTC to local
            return [ts_iso_to_local(g[1], "%F %T")]+list(g[2:8])
        else: # v56
            return list(g[9:9+7])
