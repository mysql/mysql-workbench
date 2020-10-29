# Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

# FIXME: This tests are now broken after the redesign of the log reader classes

import sys
import os

import re
import unittest
import io

import wb_admin_test_main
import wb_log_reader

base_str = ("""Preamble<field1><field2><field3>
extra line""" +
"""<field1><field2><field3>extra text"""*4
)

slow_query_str = ("""This is some dummy text
to see that it can ignore the first part
# Time: 110901 12:20:56
# User@Host: root[root] @ localhost []
# Query_time: 3.000833  Lock_time: 0.000000 Rows_sent: 1  Rows_examined: 0
SET timestamp=1314897656;
SELECT sleep(3);
/usr/local/mysql/bin/mysqld, Version: 5.5.15-log (MySQL Community Server (GPL)). started with:
Tcp port: 3306  Unix socket: /tmp/mysql.sock
Time                 Id Command    Argument
/usr/local/mysql/bin/mysqld, Version: 5.5.15-log (MySQL Community Server (GPL)). started with:
Tcp port: 3306  Unix socket: /tmp/mysql.sock
Time                 Id Command    Argument
""" +
"""# Time: 110901 12:20:56
# User@Host: root[root] @ localhost []
# Query_time: 3.000833  Lock_time: 0.000000 Rows_sent: 1  Rows_examined: 0
SET timestamp=1314897656;
SELECT <some large query>;
<that spans several>
<lines>
"""*8)

@unittest.skip('Skipped until more relevant tests are added')
class TestQueryLogReader(wb_admin_test_main.AdminTestCase):
    @classmethod
    def setUpClass(cls):
        cls.local_db_fixtures.load_sample_database(sample_db='world', db_name='testdb')
        cls.local_db_fixtures.execute_script(os.path.join(os.path.dirname(__file__), 'fixtures/sample_general_log.sql'))

    def test_general_log_creation(self):
        general_log = wb_log_reader.GeneralQueryLogReader(self.local_ctrl_be, 'test.general_log')
        self.assertIsInstance(general_log, wb_log_reader.GeneralQueryLogReader)

#class TestBaseReaderClass(unittest.TestCase):
#    def setUp(self):
#        self.regex = re.compile(r'<field(1)><field(2)><field(3)>')
#        self.logFile1 = wb_log_file_reader.BaseLogFileReader(StringIO.StringIO(base_str),
#                        self.regex,
#                        40,
#                        False,
#                        True)
#        self.logFile2 = wb_log_file_reader.BaseLogFileReader(StringIO.StringIO(base_str),
#                        self.regex,
#                        40,
#                        False,
#                        False)
#
#
#    def test_start(self):
#        self.assertEqual(self.logFile1.first_record_pos, 8)
#        self.assertEqual(self.logFile2.first_record_pos, 8)
#
#    def test_size(self):
#        self.assertEqual(self.logFile1.file_size, len(base_str), 'file size mismatch')
#
#    def test_shorten_query_field(self):
#        data = '#'*1024
#        result = self.logFile1._shorten_query_field(data)
#        self.assertTrue(result.startswith('#'*256))
#        self.assertTrue('1.0 KB' in result)
#
#    def test_chunk_parsing1(self):
#        records = self.logFile2._parse_chunk(base_str)
#        self.assertEqual(records, [ ['1', '2', '3'] ]*5)
#
#    def test_chunk_parsing2(self):
#        records = self.logFile1._parse_chunk(base_str)
#        result = [ ['1', '2', '3\nextra line'] ]
#        result.extend([ ['1', '2', '3extra text'] ]*4)
#        self.assertEqual(records, result)
#
#    def test_read_next_chunk(self):
#        log_reader = wb_log_file_reader.BaseLogFileReader(StringIO.StringIO(base_str),
#                        self.regex,
#                        40,
#                        False,
#                        True)
#        # If positioned at the EOF (this is the initial state) a call to
#        # _read_next_chunk should raise an exception:
#        self.assertRaises(IndexError, log_reader._read_next_chunk)
#        log_reader.chunk_end = log_reader.first_record_pos
#        log_reader.chunk_size = 5
#        self.assertEqual(log_reader._read_next_chunk(), '<field1><field2><field3>\nextra line')
#
#
#    def test_read_previous_chunk(self):
#        log_reader = wb_log_file_reader.BaseLogFileReader(StringIO.StringIO(base_str),
#                        self.regex,
#                        40000,
#                        False,
#                        True)
#        self.assertEqual(log_reader._read_previous_chunk(), base_str[log_reader.first_record_pos:])
#        # When the file is smaller than chunk_size, once the only existent chunk is
#        # read, there are neither previous nor next chunks:
#        self.assertRaises(IndexError, log_reader._read_next_chunk)
#        self.assertRaises(IndexError, log_reader._read_previous_chunk)
#
#    def test_adjust_chunk_start(self):
#        log_reader = wb_log_file_reader.BaseLogFileReader(StringIO.StringIO(base_str),
#                        self.regex,
#                        5,
#                        False,
#                        True)
#        log_reader.chunk_start = log_reader.file_size - 3
#        log_reader._adjust_chunk_start()
#        self.assertEqual(log_reader.chunk_start,
#                         len(base_str) - 40 + log_reader.pat.search(base_str[-40:]).start())
#        log_reader.chunk_start = 0
#        log_reader._adjust_chunk_start()
#        self.assertEqual(log_reader.chunk_start, log_reader.first_record_pos)
#
#    def test_adjust_chunk_end(self):
#        self.logFile2.chunk_end = self.logFile2.file_size - 2
#        self.logFile2._adjust_chunk_end()
#        self.assertEqual(self.logFile2.chunk_end, self.logFile2.file_size)
#        self.logFile2.chunk_end = self.logFile2.first_record_pos
#        self.logFile2._adjust_chunk_end()
#        self.assertEqual(self.logFile2.chunk_end, self.logFile2.first_record_pos)
#
#    @unittest.skip('to be done')  # remove this line when ready to run this test
#    def test_updated_log_file(self):
#        pass
#
##=============================================================================
#class TestSlowReaderClass(unittest.TestCase):
#    def setUp(self):
#        self.logFile1 = wb_log_file_reader.SlowLogFileReader(StringIO.StringIO(slow_query_str))
#
#    @unittest.skip('to be done')  # remove this line when ready to run this test
#    def test_start(self):
#        self.assertEqual(self.logFile1.previous(), [])

#if __name__ == '__main__':
#    unittest.main()
