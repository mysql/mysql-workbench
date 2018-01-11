# Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

import os
import sys
import glob
import unittest

import grt

try:
    import coverage
except ImportError:
    has_coverage = False
else:
    has_coverage = True

CURRENT_DIR = os.path.abspath(os.path.dirname(__file__))
SRC_DIR = os.path.abspath(os.path.join(CURRENT_DIR, '../../../'))

test_params = { 'hostName'      :'172.16.0.1',
               'port'           :1433,
               'database'       :'AdventureWorks',
               'user'           :'testuser',
               'password'       :'c3poLATxce090'
               }

def create_connection(hostName, port, database, user, password=''):
    driver = None
    for i in grt.root.wb.rdbmsMgmt.rdbms:
        if i.name == 'Mssql':
            driver = i.defaultDriver
            break

    conn = None
    if driver:
        conn = grt.classes.db_mgmt_Connection()
        conn.driver = driver
        conn.name = hostName + ":" + str(port)
        conn.hostIdentifier = "Mssql@" + conn.name
        params = conn.parameterValues
        params['hostName'] = hostName
        params['port'] = port
        params['database'] = database
        params['userName'] = user
        params['%userName%::Mssql@%hostName%:%port%'] = '%userName%::Mssql@%hostName%:%port%'
        params['password'] = password

    return conn

class MigrationTestCase(unittest.TestCase):
    connection = create_connection(**test_params)

if __name__ == '__main__':
    if has_coverage:
        cov = coverage.coverage()
        cov.start()

    os.chdir(CURRENT_DIR)
    sys.path[:0] = [ CURRENT_DIR, os.path.join(SRC_DIR, 'library/python') ]

    suite = unittest.TestSuite()
    names = [ os.path.splitext(fname)[0] for fname in glob.glob("test_*.py") ]
    suite.addTest(unittest.defaultTestLoader.loadTestsFromNames(names))

    result = unittest.TextTestRunner(stream=open(os.path.join(SRC_DIR, 'testing/python/test_results_migration.txt'), 'w'), verbosity=2).run(suite)

    if has_coverage:
        cov.stop()
        report_file = open(os.path.join(SRC_DIR, 'testing/python/coverage_migration_report.txt'), 'a')
        cov.report(file=report_file)
        report_file.write('\n\n')

    if not result.wasSuccessful():
        sys.exit(1)
