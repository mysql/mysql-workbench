# Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

"""
How to run the tests in this file
==================================

1. Copy the settings.py.in and rename it to settings.py. Addapt the
   settings.py file you have just created to match the settings of 
   your test environmnent.
2. Open a console and cd to the directory where these files live
   (plugins/migration/unit-tests).
3. Execute this command to run the tests:
    python -m unittest test_wbcopytables

"""


import unittest
import types
import os
import sys
import fnmatch
import subprocess
import hashlib
import functools
import logging
import re
import platform

import settings

_this_dir = os.path.abspath(os.path.dirname(__file__))


# FIXME: password containing spaces will only be scrambled up to the first space:
_pwd_regex = re.compile(r"""(?<=\s-p).*?(?=\s|$)|"""
                        r"""(?<=\s--source-password=).*?(?=\s|$)|"""
                        r"""(?<=\s--target-password=).*?(?=\s|$)|"""
                        r"""(?<=\s--password=).*?(?=\s|$)""")

def scramble_pwd(cmd, replacement='***'):
    return _pwd_regex.sub(replacement, cmd)

if settings.log_file:
    logging.basicConfig(filename=settings.log_file,level=logging.DEBUG)
else:
    logging.basicConfig(level=logging.DEBUG)


class CopyTablesTestCase(unittest.TestCase):
    thread_count = 1

    @classmethod
    def setUpClass(cls):
        """Preparations before running all tests in this class.

        Sets the environment variable for the linker to find the wb base lib needed by wbcopytables' executable.
        """
        OS = platform.system()
        cls._env_var_original = None
        if OS == 'Linux':
            env_var = 'LD_LIBRARY_PATH'
            # set lib_dir to dir(wbcopytables executable)/../lib/mysql-workbench
            lib_dir = os.path.join(os.path.dirname(os.path.dirname(settings.copytables_path)), 'lib', 'mysql-workbench')
            sep = ':'
        elif OS == 'Windows':
            return  # In our current build setup, base.dll is in the same dir than wbcopytables.exe, so no need to tweak
                    # the path. Remove this return if it should be set in the future
            env_var = 'PATH'
            lib_dir = os.path.dirname(settings.copytables_path)
            sep = ';'
        else:  # Mac OS X
            return  # TODO
            env_var = 'DYLD_LIBRARY_PATH'
            lib_dir = ''  # TODO
            sep = ':'
        cls._env_var_original = (env_var, os.getenv(env_var))
        os.putenv(env_var, (cls._env_var_original[1] + sep + lib_dir) if cls._env_var_original[1] else lib_dir)

    def setUp(self):
        """Preparations before running each test in this class.

        Creates test database in the MySQL test servers.
        """
        for target_name, target_info in settings.mysql_instances:
            mysql_call = (settings.mysql_client + 
                          " -u %(user)s -p%(password)s -h %(host)s -P %(port)d -e 'CREATE DATABASE %(database)s'" % target_info
                         )
            logging.debug('Calling the MySQL Client with command: %s' % scramble_pwd(mysql_call))
            subprocess.Popen(mysql_call, shell=True).wait()

    def _param_test(self, test_info, source_instance, source_info, target_instance, target_info):
        """Parameterized test function.
        
        The individual tests in this class call this functions with their specific data. This test
        does the following:
            1. Connect to the given source instance and run there the associated .sql code that sets
               up the test data in the source server.
            2. Connect to the given mysql instance and run there the associated .sql code that creates
               the target table that is to be loaded with data with wbcopytables.
            3. Call wbcopytables to move the data from the source to the target server.
            4. Run mysqldump in the target server and compare its output with the expected output for
               the test. The test fails if they differ.

        Args:
            test_info: A dictionary containing information about the test (test name, test files, etc.)
                like the one that returns the available_tests function below.
            source_instance: A string with the name of a source server instance. This name is also the
                key to that instance in the settings.source_instances dict.
            source_info: A dictionary with information about the source server instance. Is the dict
                associated to the source_instance key in settings.source_instances.
            target_instance: A string with the name of a mysql target server instance that will be used
                by the test. Like with source_instance, this is also the key to that instance settings
                dict in settings.mysql_instances.
            target_info: A dictionary with information about the mysql target server instance. This is
                the dict associated to the target_instance key in settings.mysql_instances.
        """
        # Run the source script in the source RDBMS instance:
        logging.debug('Importing python module %s' % source_info['module'])
        __import__(source_info['module'])
        module = sys.modules[source_info['module']]
        try:
            source_conn_str = source_info['connection_string'] % source_info
            logging.debug('source_conn_str = %s' % scramble_pwd(source_conn_str))
            conn = module.connect(source_conn_str)
        except Exception:
            self.fail('Could not connect to the %s instance' % source_instance)
        else:
            logging.debug('Connected to source instance')
            cursor = conn.cursor()
            script = open(test_info['source'], 'rb').read()
            logging.debug('Executing this script in source instance: \n%s' % '\t'.join(line + '\n' for line in script.split('\n')))
            if hasattr(cursor, 'executescript'):
                logging.debug('Running the whole script in one call')
                cursor.executescript(script)
            else:
                logging.debug('Running the whole script sentence by sentence')
                for stmt in script.split(';'):
                    cursor.execute(stmt)
            conn.commit()

        # Run the target script in the target MySQL instance:
        mysql_call = (settings.mysql_client + ' -u %(user)s -p%(password)s -h %(host)s -P %(port)d %(database)s < ' % target_info
                      + test_info['target'] )
        logging.debug('Calling the MySQL Client with command: %s' % scramble_pwd(mysql_call))
        subprocess.Popen(mysql_call, shell=True).wait()

        # Call copytables to transfer the data from source to target:
        copytables_params = (' --pythondbapi-source="%(module)s' % source_info + '''://'%s'"''' % source_conn_str + 
                             ' --source-password="%(password)s"' % source_info +
                             ' --target="%(user)s@%(host)s:%(port)d" --target-password="%(password)s"' % target_info +
                             ' --table-file="%(table_file)s"' % test_info +
                             ' --thread-count=%u' % self.thread_count
                            )
        logging.debug('Calling copytables with command: %s' % settings.copytables_path + scramble_pwd(copytables_params))
        subprocess.Popen(settings.copytables_path + copytables_params, shell=True).wait()

        # Dump the MySQL data and compare it with the expected data:
        mysqldump_call = settings.mysql_dump + ' -u %(user)s -p%(password)s -h %(host)s -P %(port)d --compact %(database)s' % target_info
        logging.debug('Calling the MySQL Dump with command: %s' % scramble_pwd(mysqldump_call))
        p = subprocess.Popen(mysqldump_call, shell=True, stdout=subprocess.PIPE)
        dumped_data = p.communicate()[0]
        dumped_hash = hashlib.md5(dumped_data).hexdigest()
        expected_data = open(test_info['expected'][target_instance], 'rb').read()
        expected_hash = hashlib.md5(expected_data).hexdigest()
        if dumped_hash != expected_hash:
            logging.error('The dumped SQL file is different from the expected one.\n' +
                          60*'-' + '\nExpected file:\n' + 60*'-' +
                          '\n%s\n' % expected_data +
                          60*'-' + '\nDumped file:\n' + 60*'-' +
                          '\n%s\n' % dumped_data + 60*'-'
                         )
        self.assertEqual(dumped_hash, expected_hash)

    def tearDown(self):
        """Clean up after running each test in this class.
        
        Drops the test database in the MySQL test servers.
        """
        for target_name, target_info in settings.mysql_instances:
            mysql_call = (settings.mysql_client + 
                          " -u %(user)s -p%(password)s -h %(host)s -P %(port)d %(database)s -e 'DROP DATABASE %(database)s'" % target_info
                         )
            logging.debug('Calling the MySQL Client with command: %s' % scramble_pwd(mysql_call))
            subprocess.Popen(mysql_call, shell=True).wait()
       
    @classmethod
    def tearDownClass(cls):
        """Clean up after running all tests in this class.
        
        Restore original value of modified environment variables.
        """
        if cls._env_var_original:
            os.putenv(*cls._env_var_original)


def available_tests(path):
    """Iterates over available tests in a given path.
    
    Walks through a directory structure yielding information about the tests it founds.
    A test is identified by the presence of the files <test_name>_source.sql,
    <test_name>_target.sql and either a <test_name>_expected_target_data.sql or a
    subset of <test_name>_<mysql_server_name>_expected_target_data.sql files for each
    defined mysql server in settings.py.

    Args:
        path: A string containing the path to the directory where the tests are stored.
            The tests can be in separate directories inside <path> but all the test files
            for a given test must reside in the same directory.

    Yields:
        A dictionary with information for each individual test. The dictionary contains
        the following info:
            'test_name' : The name of the test.
            'source' : The .sql file to be run in the source RDBMS to set up the test
                table(s) that wbcopytables will use to extract the data from.
            'target' : The .sql file to be run in each target MySQL server. This file
                usually sets up the table(s) that will contain the migrated data.
            'table_file' : The table file that wbcopytables will use.
            'expected' : A dictionary mapping a MySQL server name (obtained from iterating
                over the MySQL servers defined in settings.py) to the expected result
                file, obtained from dumping the target table that contains the expected
                data using "mysqldump --compact".
    """
    if os.path.isdir(path):
        servers = [server[0] for server in settings.mysql_instances]
        for root, dirs, files in os.walk(path):
            for candidate_test in fnmatch.filter(files, '*_source.sql'):
                test_name = candidate_test[:-11]
                permitted_targets = (  [test_name+'_expected_target_data.sql'] +
                             [test_name+'_%s_expected_target_data.sql' % server for server in servers]
                          )
                if (test_name+'_target.sql' in files and
                    test_name+'_table_file.txt' in files and
                    any(expected_file in files for expected_file in permitted_targets)
                   ):
                    expected = {}
                    for sname in servers:
                        if (test_name+'_%s_expected_target_data.sql' % sname) in files:
                            expected[sname] = os.path.join(root, test_name + '_%s_expected_target_data.sql' % sname)
                        elif (test_name + '_expected_target_data.sql') in files:
                            expected[sname] = os.path.join(root, test_name + '_expected_target_data.sql')
                        else:
                            expected[sname] = None

                    yield( { 'test_name' : test_name,
                             'source'    : os.path.join(root, candidate_test),
                             'target'    : os.path.join(root, test_name + '_target.sql'),
                             'table_file': os.path.join(root, test_name + '_table_file.txt'),
                             'expected'  : expected
                           }
                         )



# Generate the tests based on the directory structure and the files on disk:
for source_instance, source_info in settings.source_instances:
    for test_info in available_tests(os.path.join(_this_dir, 'fixtures', source_instance)):
        for target_instance, target_info in settings.mysql_instances:
            # Dynamically add a meaningful test function to the test case class defined above:
            setattr(CopyTablesTestCase, 'test_%s_%s_%s' % (test_info['test_name'], source_instance, target_instance),
                    types.MethodType(functools.partial(CopyTablesTestCase._param_test,
                                                       test_info=test_info,
                                                       source_instance = source_instance,
                                                       source_info = source_info,
                                                       target_instance = target_instance,
                                                       target_info = target_info
                                                      ),
                                     None,
                                     CopyTablesTestCase)
                   )


if __name__ == '__main__':
    unittest.main()
