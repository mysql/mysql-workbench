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

import os
import sys

old_test_case = False

if sys.version_info < (2, 7, 0):
    from unittest2 import TestCase
    old_test_case = True
else:
    from unittest import TestCase

import wb_server_management as target_module
from wb_common import Users, InvalidPasswordError, PermissionDeniedError

class TestGlobalModuleCode(TestCase):
    def __init__(self, *args, **kwargs):
        TestCase.__init__(self, *args, **kwargs)
        
        if old_test_case:
            self.setUpClass()

    @classmethod
    def setUpClass(self):
        self.sudo_pwd = None
        if 'WB_SUDO_PASSWD' in os.environ:
            self.sudo_pwd = os.environ['WB_SUDO_PASSWD']
        else:
            print('---> WB_SUDO_PASSWD is not defined, skipping TestGlobalModuleCode Admin Tests')
    
    def test_reset_sudo_prefix(self):
        target_module.reset_sudo_prefix()
        self.assertEqual(target_module.default_sudo_prefix, '/usr/bin/sudo -S -p EnterPasswordHere')
    
    def test_quote_path(self):
        self.assertEqual(target_module.quote_path("/hello/world"), '"/hello/world"')
        self.assertEqual(target_module.quote_path("~/hello/world"), '~/"hello/world"')
        self.assertEqual(target_module.quote_path("/hello world/file.txt"), '"/hello world/file.txt"')
        self.assertEqual(target_module.quote_path('/tmp/"hello world".txt'), '"/tmp/\\"hello world\\".txt"')
        self.assertEqual(target_module.quote_path("/tmp/'hello world'.txt"), "\"/tmp/'hello world'.txt\"")
        self.assertEqual(target_module.quote_path("/tmp/hello world.txt"), '"/tmp/hello world.txt"')
        self.assertEqual(target_module.quote_path("/tmp/foo\bar"), '"/tmp/foo\bar"')
        self.assertEqual(target_module.quote_path("/tmp/$hello.txt"), '"/tmp/$hello.txt"')
        
        
    def test_wrap_for_sudo(self):
    
        #1) Test passing no command
        with self.assertRaisesRegex(Exception, 'Empty command passed to execution routine'):
            target_module.wrap_for_sudo("", "")
        
        #2) Test wrapping command for current user, no change should be done
        self.assertEqual(target_module.wrap_for_sudo('ls -l', '', Users.CURRENT), 'ls -l')
        
        #3) Test wrapping command for admin user, the default sudo prefix should be added
        self.assertEqual(target_module.wrap_for_sudo('ls -l', ''), '/usr/bin/sudo -S -p EnterPasswordHere /bin/bash -c "ls -l"')
        
        #4) Test wrapping command for admin user, passing a custom sudo prefix
        self.assertEqual(target_module.wrap_for_sudo('ls -l', 'sudo'), 'sudo /bin/bash -c "ls -l"')

        #5) Repeast previous 2 test cases but using a user different than admin or the current user
        self.assertEqual(target_module.wrap_for_sudo('ls -l', '', 'guest'), '/usr/bin/sudo -u guest -S -p EnterPasswordHere /bin/bash -c "ls -l"')
        self.assertEqual(target_module.wrap_for_sudo('ls -l', 'sudo', 'guest'), 'sudo -u guest /bin/bash -c "ls -l"')
        
        #6) Test sudo for a spawned command
        self.assertEqual(target_module.wrap_for_sudo('ls -l', '', to_spawn=True), '/usr/bin/sudo -S -p EnterPasswordHere /usr/bin/nohup /bin/bash -c "ls -l &"')

        
    def test_local_run_cmd_linux(self):
        custom_sudo = '/usr/bin/sudo -S -k -p EnterPasswordHere'
        # Creates a folder as the current user
        self.assertEqual(target_module.local_run_cmd_linux('mkdir __testing_folder'), 0)
        
        self.assertEqual(target_module.local_run_cmd_linux('rmdir __testing_folder'), 0)

        # Attempts creating a folder using sudo with the incorrect password
        with self.assertRaisesRegex(InvalidPasswordError, 'Incorrect password for sudo'):
            self.assertEqual(target_module.local_run_cmd_linux('mkdir __testing_admin', Users.ADMIN, sudo_prefix = custom_sudo), 1)
            
        if self.sudo_pwd:
            # Creates a folder as the root user
            self.assertEqual(target_module.local_run_cmd_linux('mkdir __testing_admin', Users.ADMIN, self.sudo_pwd, custom_sudo), 0)
            
            # Deletes both folders
            self.assertEqual(target_module.local_run_cmd_linux('rmdir __testing_admin', Users.ADMIN, self.sudo_pwd, custom_sudo), 0)
        

class TestProcessOpsLinuxLocal(TestCase):
    def __init__(self, *args, **kwargs):
        TestCase.__init__(self, *args, **kwargs)
        
        if old_test_case:
            self.setUpClass()

    @classmethod
    def setUpClass(self):
        self.sudo_pwd = None
        if 'WB_SUDO_PASSWD' in os.environ:
            self.sudo_pwd = os.environ['WB_SUDO_PASSWD']
        else:
            print('---> WB_SUDO_PASSWD is not defined, skipping TestFileOpsLinuxBase Admin Tests')
    
        self.target_class = target_module.ProcessOpsLinuxLocal(sudo_prefix = '/usr/bin/sudo -S -k -p EnterPasswordHere', ssh=None)
        
    def test_exec_cmd(self):
        # Creates a folder as the current user
        self.assertEqual(self.target_class.exec_cmd('mkdir __testing_folder'), 0)
        self.assertEqual(self.target_class.exec_cmd('rmdir __testing_folder'), 0)
        
        # Attempts creating a folder using sudo with the incorrect password
        with self.assertRaisesRegex(InvalidPasswordError, 'Incorrect password for sudo'):
            self.assertEqual(self.target_class.exec_cmd('mkdir __testing_admin', Users.ADMIN), 1)
                        
        # Creates a folder as the root user
        if self.sudo_pwd:
            self.assertEqual(self.target_class.exec_cmd('mkdir __testing_admin', Users.ADMIN, self.sudo_pwd), 0)
            
            # Deletes both folders
            self.assertEqual(self.target_class.exec_cmd('rmdir __testing_admin', Users.ADMIN, self.sudo_pwd), 0)


class TestFileOpsLinuxBase(TestCase):
    def __init__(self, *args, **kwargs):
        TestCase.__init__(self, *args, **kwargs)
        
        if old_test_case:
            self.setUpClass()

    @classmethod
    def setUpClass(self):
        self.sudo_pwd = None
        if 'WB_SUDO_PASSWD' in os.environ:
            self.sudo_pwd = os.environ['WB_SUDO_PASSWD']
        else:
            print('---> WB_SUDO_PASSWD is not defined, skipping TestFileOpsLinuxBase Admin Tests')
    
        self.process_ops = target_module.ProcessOpsLinuxLocal(sudo_prefix = '/usr/bin/sudo -S -k -p EnterPasswordHere', ssh=None)
        self.target_class = target_module.FileOpsLinuxBase(self.process_ops, None, 'linux')

        self.user_home = os.path.expanduser('~')
        
        self.test_files = []
        self.test_files.append(os.path.join(self.user_home, '__testing_file.txt'))
        self.test_files.append(os.path.join(self.user_home, '__testing file.txt'))
        
        self.test_folders = []
        self.test_folders.append(os.path.join(self.user_home, '__testing_folder'))
        self.test_folders.append(os.path.join(self.user_home, '__testing folder'))
        
    def test_file_exists(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "file_exists", parameter "filename" must be an absolute path'):
            self.target_class.file_exists('sample.txt')
        
        for file in self.test_files:
            # Verifies the non existence of a file
            self.assertFalse(self.target_class.file_exists(file))

            # Creates a temporary file
            self.process_ops.exec_cmd('touch "%s"' % file)
            
            # Ensures the file_exists only accepts absolute paths
            self.assertTrue(self.target_class.file_exists(file))

            # Removes the temporary file
            self.process_ops.exec_cmd('rm "%s"' % file)
        

    def test_get_available_space(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "get_available_space", parameter "path" must be an absolute path'):
            self.target_class.get_available_space('folder_name')
        
        for folder in self.test_folders:
            # Gets availalbe space of unexisting folder
            self.assertEqual(self.target_class.get_available_space(folder), 'Could not determine')
            
            # Gets availalbe space of existing folder
            self.process_ops.exec_cmd('mkdir "%s"' % folder)
            self.assertRegex(self.target_class.get_available_space(folder), '.*of.*available')
            self.process_ops.exec_cmd('rmdir "%s"' % folder)
        
    def test_get_file_owner(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "get_file_owner", parameter "path" must be an absolute path'):
            self.target_class.get_file_owner('test.txt')
        
        
        for file in self.test_files:
            # Gets availalbe space of unexisting file
            with self.assertRaisesRegex(Exception, 'No such file or directory'):
                self.target_class.get_file_owner(file)


            if self.sudo_pwd:
                # Creates a temporary file
                self.process_ops.exec_cmd('touch "%s"' % file, Users.ADMIN, self.sudo_pwd)
                
                # Ensures the file_exists
                self.assertEqual(self.target_class.get_file_owner(file), 'root')

                # Removes the temporary file
                self.process_ops.exec_cmd('rm "%s"' % file, Users.ADMIN, self.sudo_pwd)


    def test_create_directory(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "create_directory", parameter "path" must be an absolute path'):
            self.target_class.create_directory('test_folder')
        
        for folder in self.test_folders:
            # Creates a directory
            self.target_class.create_directory(folder + '_1')
            self.assertTrue(self.target_class.file_exists(folder + '_1'))
            
            # Attempts creating an existing directory
            with self.assertRaisesRegex(Exception, 'cannot create directory.*File exists'):
                self.target_class.create_directory(folder + '_1')        
                
            self.process_ops.exec_cmd('rmdir "%s_1"' % folder)

            # Attempts creating a directory with owned but using the current user
            with self.assertRaisesRegex(PermissionDeniedError, 'Cannot set owner of directory'):
                self.target_class.create_directory(folder + '_2', with_owner = 'other')        

            if self.sudo_pwd:
                # Attempts creating a directory with the admin user
                self.target_class.create_directory(folder + '_2', Users.ADMIN, self.sudo_pwd, with_owner = 'root')
                self.assertTrue(self.target_class.file_exists(folder + '_2'))
                
                # Removes folder
                self.process_ops.exec_cmd('rmdir "%s_2"' % folder, Users.ADMIN, self.sudo_pwd)


    def test_create_directory_recursive(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "create_directory_recursive", parameter "path" must be an absolute path'):
            self.target_class.create_directory_recursive('test_folder')
            
        for folder in self.test_folders:
            # Creates a directory
            self.target_class.create_directory_recursive(os.path.join(folder, 'one', 'two', 'three'))
            
            self.assertTrue(self.target_class.file_exists(os.path.join(folder, 'one', 'two', 'three')))
            self.assertTrue(self.target_class.file_exists(os.path.join(folder, 'one', 'two')))
            self.assertTrue(self.target_class.file_exists(os.path.join(folder, 'one')))
            self.assertTrue(self.target_class.file_exists(os.path.join(folder)))
            
            # Creates a directory tree that aalready exists, nothing is expected to happen
            with self.assertRaisesRegex(Exception, 'cannot create directory.*File exists'):
                self.target_class.create_directory_recursive(os.path.join(folder, 'one', 'two', 'three'))

            self.process_ops.exec_cmd('rm -R "%s"' % folder)
        
        
    def test_remove_directory(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "remove_directory", parameter "path" must be an absolute path'):
            self.target_class.remove_directory('test_folder')
        
        for folder in self.test_folders:
            # Creates a directory
            self.process_ops.exec_cmd('mkdir "%s"' % folder)
            self.assertTrue(self.target_class.file_exists(folder))
            
            self.target_class.remove_directory(folder)
            self.assertFalse(self.target_class.file_exists(folder))
            
            # Attempts creating an existing directory
            with self.assertRaisesRegex(Exception, 'No such file or directory'):
                self.target_class.remove_directory(folder)
        
    def test_remove_directory_recursive(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "remove_directory_recursive", parameter "path" must be an absolute path'):
            self.target_class.remove_directory_recursive('test_folder')
        
        for folder in self.test_folders:
            # Creates a directory
            self.process_ops.exec_cmd('mkdir "%s"' % folder)
            self.process_ops.exec_cmd('mkdir "%s"' % os.path.join(folder, 'uno'))
            self.process_ops.exec_cmd('mkdir "%s"' % os.path.join(folder, 'uno', 'dos'))
            self.process_ops.exec_cmd('mkdir "%s"' % os.path.join(folder, 'uno', 'dos', 'tres'))
            self.assertTrue(self.target_class.file_exists(os.path.join(folder, 'uno', 'dos', 'tres')))
            
            self.target_class.remove_directory_recursive(folder)
            self.assertFalse(self.target_class.file_exists(folder))
            
            # Attempts creating an existing directory
            with self.assertRaisesRegex(Exception, 'No such file or directory'):
                self.target_class.remove_directory_recursive(folder)
            
    def test_delete_file(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "delete_file", parameter "path" must be an absolute path'):
            self.target_class.delete_file('test.txt')
        
        for file in self.test_files:
            # Creates a temporary file
            self.process_ops.exec_cmd('touch "%s"' % file)
            self.assertTrue(self.target_class.file_exists(file))
            
            # Deletes the file
            self.target_class.delete_file(file)
            self.assertFalse(self.target_class.file_exists(file))
            
            # Attempts to delete the file that no longer exists
            with self.assertRaisesRegex(Exception, 'No such file or directory'):
                self.target_class.delete_file(file)


    def test_get_file_content(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "get_file_content", parameter "filename" must be an absolute path'):
            self.target_class.get_file_content('test.txt')
        
        for file in self.test_files:
            # Creates a temporary file
            self.process_ops.exec_cmd('echo "1234\n6789" >  "%s"' % file)
            
            # gets the file content
            self.assertEqual(self.target_class.get_file_content(file), '1234\n6789')
            
            # gets the file content skipping the first line
            self.assertEqual(self.target_class.get_file_content(file, skip_lines = 1), '6789')

            # Deletes the file
            self.process_ops.exec_cmd('rm "%s"' % file)

            # Attempts to get the content of a file that no longer exists
            with self.assertRaisesRegex(Exception, 'No such file or directory'):
                self.target_class.get_file_content(file)


    def test_check_dir_writable(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "check_dir_writable", parameter "path" must be an absolute path'):
            self.target_class.check_dir_writable('test_folder')
        
        for folder in self.test_folders:
            # Creates temporary folders
            self.process_ops.exec_cmd('mkdir "%s"' % folder + '_1')
            
            if self.sudo_pwd:
                self.process_ops.exec_cmd('mkdir "%s"' % folder + '_2', Users.ADMIN, self.sudo_pwd)
            
            # Checks writability
            self.assertTrue(self.target_class.check_dir_writable(folder + '_1'))
            if self.sudo_pwd:
                self.assertFalse(self.target_class.check_dir_writable(folder + '_2'))
            
            self.process_ops.exec_cmd('rmdir "%s_1"' % folder)
            
            if self.sudo_pwd:
                self.assertTrue(self.target_class.check_dir_writable(folder + '_2', Users.ADMIN, self.sudo_pwd))
                self.process_ops.exec_cmd('rmdir "%s_2"' % folder, Users.ADMIN, self.sudo_pwd)
            
            # Attempts to check dir writability on unexisting folder
            with self.assertRaisesRegex(Exception, 'The path.*does not exist'):
                self.target_class.check_dir_writable(folder)

            # Attempts to check dir writability on a file
            self.process_ops.exec_cmd('touch "%s"' % folder)
            with self.assertRaisesRegex(Exception, 'The path.*is not a directory'):
                self.target_class.check_dir_writable(folder)
            self.process_ops.exec_cmd('rm "%s"' % folder)


    def test_listdir(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "listdir", parameter "path" must be an absolute path'):
            self.target_class.listdir('test_folder')
        
        for folder in self.test_folders:
            # Attempts listing unexisting folder
            with self.assertRaisesRegex(Exception, 'cannot access.*No such file or directory'):
                self.target_class.listdir(folder)
                
                
            # Creates temporary folders
            self.process_ops.exec_cmd('mkdir "%s"' % folder)
            self.process_ops.exec_cmd('touch "%s"' % os.path.join(folder, 'test.txt'))
            
            directory = self.target_class.listdir(folder)
            self.assertIn('test.txt', directory)

            directory = self.target_class.listdir(folder, include_size=True)
            self.assertIn(('test.txt', 0), directory)
            
            self.process_ops.exec_cmd('rm -R "%s"' % folder)
        
    def test_save_file_content_and_backup(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "save_file_content_and_backup", parameter "filename" must be an absolute path'):
            self.target_class.save_file_content_and_backup('test.txt', '12345678')
        
        for folder in self.test_folders:
            # Tries to save a file on an unexisting folder
            with self.assertRaisesRegex(OSError, 'The path.*does not exist'):
                self.target_class.save_file_content_and_backup(os.path.join(folder, 'test.txt'), '12345678', '.bak')
                
            # Tries to save a file on a folder with no write permissions
            if self.sudo_pwd:
                self.process_ops.exec_cmd('mkdir "%s"' % folder, Users.ADMIN, self.sudo_pwd)
                with self.assertRaisesRegex(PermissionDeniedError, 'Cannot write to target folder'):
                    self.target_class.save_file_content_and_backup(os.path.join(folder, 'test.txt'), '12345678', '.bak')
                    
                self.target_class.save_file_content_and_backup(os.path.join(folder, 'test.txt'), '12345678', '.bak', Users.ADMIN, self.sudo_pwd)
                
                self.process_ops.exec_cmd('rm -R "%s"' % folder, Users.ADMIN, self.sudo_pwd)
            
            # Saves the file on a folder, first time
            self.process_ops.exec_cmd('mkdir "%s"' % folder)
            self.target_class.save_file_content_and_backup(os.path.join(folder, 'test.txt'), '1', '.bak')
            self.assertEqual(self.target_class.get_file_content(os.path.join(folder, 'test.txt')), '1')
            self.assertFalse(self.target_class.file_exists(os.path.join(folder, 'test.txt.bak')))
            

            # Saves the file on a folder, second time
            self.target_class.save_file_content_and_backup(os.path.join(folder, 'test.txt'), '2', '.bak')
            self.assertEqual(self.target_class.get_file_content(os.path.join(folder, 'test.txt')), '2')
            self.assertEqual(self.target_class.get_file_content(os.path.join(folder, 'test.txt.bak')), '1')

            self.process_ops.exec_cmd('rm -R "%s"' % folder)
        
class TestFileOpsLocalUnix(TestCase):
    def __init__(self, *args, **kwargs):
        TestCase.__init__(self, *args, **kwargs)
        
        if old_test_case:
            self.setUpClass()
    # These test cases also exercise the variants of the methods inherited from
    # from FileOpsLinuxBase using a ProcessOpeLinuxLocal instance
    
    @classmethod
    def setUpClass(self):
        self.sudo_pwd = None
        if 'WB_SUDO_PASSWD' in os.environ:
            self.sudo_pwd = os.environ['WB_SUDO_PASSWD']
        else:
            print('---> WB_SUDO_PASSWD is not defined, skipping TestFileOpsLinuxBase Admin Tests')
    
        self.process_ops = target_module.ProcessOpsLinuxLocal(sudo_prefix = '/usr/bin/sudo -S -k -p EnterPasswordHere', ssh=None)
        self.target_class = target_module.FileOpsLocalUnix(self.process_ops, None, 'linux')
        
        self.user_home = os.path.expanduser('~')
        
        self.test_files = []
        self.test_files.append(os.path.join(self.user_home, '__testing_file.txt'))
        self.test_files.append(os.path.join(self.user_home, '__testing file.txt'))
        
        self.test_folders = []
        self.test_folders.append(os.path.join(self.user_home, '__testing_folder'))
        self.test_folders.append(os.path.join(self.user_home, '__testing folder'))
        
    
    def test_save_file_content(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "save_file_content", parameter "filename" must be an absolute path'):
            self.target_class.save_file_content('test.txt', '12345678')
    
        for file in self.test_files:
            self.target_class.save_file_content(file, "1")
            
            self.process_ops.exec_cmd('rm "%s"' % file)
        
    def test_get_file_content(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "get_file_content", parameter "filename" must be an absolute path'):
            self.target_class.get_file_content('test.txt')

        for file in self.test_files:
            # Creates a temporary file
            self.target_class.save_file_content(file, "1234\n6789")
            
            # Tests loading the full file
            self.assertEqual(self.target_class.get_file_content(file), "1234\n6789")
            
            # Tests skipping the first line
            self.assertEqual(self.target_class.get_file_content(file, skip_lines = 1), "6789")
            
            # Tests loading the file as ADMIN
            if self.sudo_pwd:
                self.assertEqual(self.target_class.get_file_content(file, Users.ADMIN, self.sudo_pwd), "1234\n6789")
            
            self.process_ops.exec_cmd('rm "%s"' % file)

    def test_delete_file(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "delete_file", parameter "path" must be an absolute path'):
            self.target_class.delete_file('test.txt')
        
        for file in self.test_files:
            self.process_ops.exec_cmd('touch "%s"' % file)
            self.assertTrue(self.target_class.file_exists(file))
            self.target_class.delete_file(file)
            self.assertFalse(self.target_class.file_exists(file))
            
            if self.sudo_pwd:
                self.process_ops.exec_cmd('touch "%s"' % file, Users.ADMIN, self.sudo_pwd)
                self.assertTrue(self.target_class.file_exists(file))
                self.target_class.delete_file(file, Users.ADMIN, self.sudo_pwd)
                self.assertFalse(self.target_class.file_exists(file))

        
    def test_listdir(self):
        # Ensures the function requires an absolute path
        with self.assertRaisesRegex(ValueError, 'Error on path validation for function "listdir", parameter "path" must be an absolute path'):
            self.target_class.listdir('test_folder')
        
        for folder in self.test_folders:
            # Attempts listing unexisting folder
            with self.assertRaisesRegex(Exception, 'No such file or directory'):
                self.target_class.listdir(folder)
                
            # Attempts listing unexisting folder
            with self.assertRaisesRegex(Exception, 'No such file or directory'):
                self.target_class.listdir(folder)
                
            # Creates temporary folders
            self.process_ops.exec_cmd('mkdir "%s"' % folder)
            self.process_ops.exec_cmd('touch "%s"' % os.path.join(folder, 'test.txt'))
            
            directory = self.target_class.listdir(folder)
            self.assertIn('test.txt', directory)

            directory = self.target_class.listdir(folder, include_size=True)
            self.assertIn(('test.txt', 0), directory)

            if self.sudo_pwd:
                directory = self.target_class.listdir(folder, Users.ADMIN, self.sudo_pwd, include_size=True)
                self.assertIn(('test.txt', 0), directory)

            self.process_ops.exec_cmd('rm -R "%s"' % folder)        
        
    
