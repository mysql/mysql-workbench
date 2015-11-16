# Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved.
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
import sys
import ctypes
if sys.platform == 'win32':
    from ctypes import wintypes
import errno
import subprocess
import shutil
import stat
from wb_common import PermissionDeniedError

class FunctionType:
    Success = 0
    Boolean = 1
    String = 2
    Data = 3


class FileUtils(object):
    """
    Provides implementation of functions to perform operations on the
    Windows File System.
    
    Exception handling is expected to be done by the callers:
    - FileOpsLocalWindows class for operations done as the CURRENT user
    - The wbadminhelper for operations done as ADMIN
    
    Some exception handling is done on these functions but only to be able
    to generate the PermissionDeniedError as it is used on further validations
    on the admin code.
    
    The functions can be divided in the next groups:
    - Success functions: they are expected to succeed, they don't return any value
    - Boolean functions: on success execution return a boolean value
    - String functions: on success execution return a string
    
    The functions will return the proper value on success or
    generate an exception in case of failure.
    
    The function names are self descriptive, however the function type
    will be indicated on each of them using the criteria explained above
    """
    def __init__(self):
        pass
        
    @classmethod
    def get_free_space(self, path):
        """
        Function Type : String
        """
        def get_readable_format(total):
            measures = ['B', 'KB', 'MB', 'GB', 'TB']
            index = 0
            while index < len(measures) and total > 1024:
                total = float(total / 1024)
                index = index + 1
                
            return "%.2f %s" % (total, measures[index])
            
        total_bytes, free_bytes = ctypes.c_ulonglong(0), ctypes.c_ulonglong(0)
        if ctypes.windll.kernel32.GetDiskFreeSpaceExW(ctypes.c_wchar_p(path), None, ctypes.pointer(total_bytes), ctypes.pointer(free_bytes)):
            free = get_readable_format(free_bytes.value)
            total = get_readable_format(total_bytes.value)
            ret_val = "%s of %s available" % (free, total)
        else:
            ret_val = "Could not determine"
            
        return ret_val
        
    @classmethod
    def check_dir_writable(self, path):
        """
        Function Type : Boolean
        """
        ret_val = True
        
        if os.path.isdir(path):
            dummy_file = os.path.join(path, '.wb_write_test')
            try:
                f = open(dummy_file, 'w+')
                f.close()
                os.remove(dummy_file)
            except IOError, e:
                if e.errno == errno.EACCES:
                    ret_val = False
                else:
                    raise
        elif os.path.exists(path):
            raise IOError(errno.ENOTDIR, 'The indicated path is not a directory')
        else:
            raise IOError(errno.ENOENT, 'The indicated path does not exist')
            
        return ret_val
                
    @classmethod
    def check_path_exists(self, path):
        """
        Function Type : Boolean
        """
        return os.path.exists(path)
      
    @classmethod
    def check_file_readable(self, path):
        """
        Function Type : Boolean
        """
        return os.access(path, os.R_OK)
      
    @classmethod
    def create_directory(self, path):
        """
        Function Type : Success
        """
        try:
            os.mkdir(path)
        except (IOError, OSError), err:
            if err.errno == errno.EACCES:
                raise PermissionDeniedError("Could not create directory %s" % path)
            raise
        
    @classmethod
    def create_directory_recursive(self, path):
        """
        Function Type : Success
        """
        try:
            os.makedirs(path)
        except (IOError, OSError), err:
            if err.errno == errno.EACCES:
                raise PermissionDeniedError("Could not create directory %s" % path)
            raise

    @classmethod
    def remove_directory(self, path):
        """
        Function Type : Success
        """
        try:
            os.rmdir(path)
        except (IOError, OSError), err:
            if err.errno == errno.EACCES:
                raise PermissionDeniedError("Could not remove directory %s" % path)
            raise err
        
    @classmethod
    def remove_directory_recursive(self, path):
        """
        Function Type : Success
        """
        try:
            shutil.rmtree(path)
        except (IOError, OSError), err:
            if err.errno == errno.EACCES:
                raise PermissionDeniedError("Could not remove directory %s" % path)
            raise err

    @classmethod
    def copy_file(self, source, target, target_backup = ""):
        """
        Function Type : Success
        """
        try:
            # Does a backup if it was required
            if target_backup:
                shutil.copy(target, target_backup)
                
            shutil.copy(source, target)
        except (IOError, OSError), e:
            if e.errno == errno.EACCES:
                raise PermissionDeniedError("Can't copy %s to %s" % (source, target))
            raise
        
    @classmethod
    def delete_file(self, path):
        """
        Function Type : Success
        """

        try:
            os.remove(path)
        except (IOError, OSError), err:
            if err.errno == errno.EACCES:
                raise PermissionDeniedError("Could not delete file %s" % path)
            raise err
    
    @classmethod
    def get_file_owner(self, path):
        """
        Function Type : String
        """
        # Grabs the functions that will be used to locate the owner of a specific path
        GetFileSecurityW = ctypes.windll.advapi32.GetFileSecurityW
        GetSecurityDescriptorOwner = ctypes.windll.advapi32.GetSecurityDescriptorOwner
        LookupAccountSidW = ctypes.windll.advapi32.LookupAccountSidW
        
        lookup_account = False
        get_owner_descriptor = False
        ret_val = ""
        
        SECURITY_INFORMATION_OWNER = 0X00000001
        length = wintypes.DWORD()
        GetFileSecurityW.restype = wintypes.BOOL
        GetFileSecurityW.argtypes = [
                    wintypes.LPCWSTR,
                    wintypes.DWORD,
                    ctypes.POINTER(wintypes.BYTE), 
                    wintypes.DWORD,
                    ctypes.POINTER(wintypes.DWORD),
                    ]

        # First: retrieves the security descriptor for the requested path
        # This call is to retrieve the size needed to store this descriptor
        GetFileSecurityW(path, SECURITY_INFORMATION_OWNER, None, 0, ctypes.byref(length))
        
        if length.value:
            sd = (wintypes.BYTE * length.value)()
            
            # This call retrieves the security descriptor
            if GetFileSecurityW(path, SECURITY_INFORMATION_OWNER, sd, length, ctypes.byref(length)):
                get_owner_descriptor = True
                
        # Second: Now gets the owner information based on the security descriptor retrieved
        #         on the first step
        if get_owner_descriptor:
            sdo = ctypes.POINTER(wintypes.BYTE)()
            sdo_defaulted = wintypes.BOOL()
            if GetSecurityDescriptorOwner(sd, ctypes.byref(sdo), ctypes.byref(sdo_defaulted)):
                lookup_account = True

        # Finally: using the security descriptor owner retrieved above, queries
        #          the user naeme and the first domain where the user is found
        if lookup_account:
            SIZE = 256
            name = ctypes.create_unicode_buffer(SIZE)
            domain = ctypes.create_unicode_buffer(SIZE) 
            cch_name = wintypes.DWORD(SIZE)
            cch_domain = wintypes.DWORD(SIZE)
            sdo_type = wintypes.DWORD()

            if LookupAccountSidW(None, sdo, name, 
                                    ctypes.byref(cch_name),
                                    domain, ctypes.byref(cch_domain),
                                    ctypes.byref(sdo_type)):
                                    
                ret_val = "%s\\%s" % (domain.value, name.value)
            else:
                print 'Failed to lookup user'
                
        if not ret_val:
            raise IOError(errno.EINVAL, "The given path is not a file or directory")
            
        return ret_val
        
    @classmethod
    def list_dir(self, path, include_size, output_handler):
        try:
            # This is needed on the SudoTail class
            if os.path.isfile(path):
                if include_size:
                    stat_info = os.stat(path)
                    stat_info.st_size
                    
                    line = "%i %s" % (stat_info.st_size, path)
                    output_handler(line)
                else:
                    output_handler(path)
            else:
                dlist = os.listdir(path)
                
                for item in dlist:
                    line = ""
                    item_path = os.path.join(path, item)
                    stat_info = os.stat(item_path)
                    item_stat = stat_info.st_mode
                    if stat.S_ISDIR(item_stat):
                        item += '/'
                        if include_size:
                            line = "%s %s" % (str(stat_info.st_size), item)
                        else:
                            line = item
                    elif stat.S_ISREG(item_stat) or stat.S_ISLNK(item_stat):
                        if include_size:
                            line = "%s %s" % (str(stat_info.st_size), item)
                        else:
                            line = item
                    
                    if line:
                        output_handler(line)
        except (IOError, OSError), e:
            if e.errno == errno.EACCES:
                raise PermissionDeniedError("Permission denied accessing %s" % path)
            raise
    
    @classmethod
    def get_file_lines(self, path, skip, output_handler):
        try:
            f = open(path, 'r')

            skipped = 0
            for line in f:
                if not skip or skipped == skip:
                    output_handler(line)
                else:
                    skipped = skipped + 1

            f.close()
        except (IOError, OSError), e:
            if e.errno == errno.EACCES:
                raise PermissionDeniedError("Can't open file '%s'" % path)
            raise
        

            
class OSUtils(object):
    """
    Provides the exec_command functiono wich will execute whatever command 
    is passed as long as it is valid for the OS.
    
    In case of failure this function will also raise the proper exception
    """
    @classmethod    
    def exec_command(self, command, output_handler):
        """
        Executes any OS valid command and sends the output to the command listener
        Syntax:
                EXEC <command>
                
                command: A valida OS command.
        """
        retcode = 1
        try:
            # Note that self._command contains the internal command so EXEC
            # and self._args is the real OS command to be executed
            # Executes the command, reading the output from the PIPE
            process = subprocess.Popen(command, stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.STDOUT, shell=True)

            # Sends all the command output to the listener
            if output_handler:
                for line in iter(process.stdout.readline, ""):
                    output_handler(line)

            # Waits for the process completion
            process.wait()
            
            retcode = process.returncode
        except Exception, exc:
            raise
            
        return retcode
    
                
            
        
        
        
        
