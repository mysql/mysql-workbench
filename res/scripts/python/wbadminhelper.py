# Copyright (c) 2013, 2019, Oracle and/or its affiliates. All rights reserved.
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
import time
import stat
import shlex

from workbench.tcp_utils import SocketClient
from workbench.os_utils import FileUtils, OSUtils
import subprocess

# The script is executed with elevated privileges which means it is 
# being executed on a clean environment on the admin user and so it is
# not possible to send back the command output
# 
# It uses a socket client to send the output to a command listener
# Which requires authentication (done with the handshake logic)
# and also a way to tell the listener that the command has completed

# Retrieves the parameters
parsedArgs = []

for arg in sys.argv:
    if arg.find(' ') > -1:
        arg = '"' + arg + '"'
    parsedArgs.append(arg)

port = parsedArgs[1]
handshake = parsedArgs[2]
done_key = parsedArgs[3]
command = parsedArgs[4:]

class CommandProcessor:
    def __init__(self, command, client):
        self._command = command[0]
        self._args = ' '.join(command[1:])
        self._client = client
        self._result_code = 0
        self._result_message = ""
        
        self._commands = {}
        self._commands['LISTDIR'] = self._process_listdir
        self._commands['GETFILE'] = self._process_getfile
        self._commands['GETFILE_LINES'] = self._process_getfile_lines
        self._commands['GET_FREE_SPACE'] = self._process_get_free_space
        self._commands['CHECK_DIR_WRITABLE'] = self._process_check_dir_writable
        self._commands['CHECK_PATH_EXISTS'] = self._process_check_path_exists
        self._commands['CREATE_DIRECTORY'] = self._process_create_directory
        self._commands['CREATE_DIRECTORY_RECURSIVE'] = self._process_create_directory_recursive
        self._commands['REMOVE_DIRECTORY'] = self._process_remove_directory
        self._commands['REMOVE_DIRECTORY_RECURSIVE'] = self._process_remove_directory_recursive
        self._commands['DELETE_FILE'] = self._process_delete_file
        self._commands['COPY_FILE'] = self._process_copy_file
        self._commands['GET_FILE_OWNER'] = self._process_get_file_owner
        self._commands['EXEC'] = self._process_exec

        if self._command not in self._commands:
            raise RuntimeError("Command %s is not supported" % self._command)
    
    def execute(self):
        """
        Selects the proper method for the command execution,
        when an additional custom command (i.e. LISTDIR) is needed
        a method should be created on this class and called on this
        method based on the command name.

        When no specific method is defined for the given command it
        will be handled by the _execute_command method which assumes
        the command is valid for the operating system
        """
        self._commands[self._command]()
        
    def _process_listdir(self):
        """
        Lists the content of a directory and returns it to the command listener,
        either including or not the file sizes.
        Syntax:
                LISTDIR <size> <path>
                
                size:   Indicates if the returned list should containg the size
                        or not. 0 indicates no, 1 indicates yes.
                path:   The path to the file or folder that will be listed.
        """
        include_size, path = self._args.split(' ', 1)
        include_size = (include_size == '1')
        
        FileUtils.list_dir(path, include_size, self._send_to_listener)

    def _process_getfile(self):
        """
        Load data from a file and sends it back to the command listener in 64K chunks.
        Syntax:
                GETFILE <offset> <size> <path>
                
                offset: In bytes, indicates the position of the file where the
                        read operation will start. 0 indicates the beggining of
                        the file.
                size:   In bytes, indicates the amount of bytes to be read from
                        the file. 0 indicates the whole file should be read.
                path:   The path to the file that will be read.
        """
        offset, size, path = self._args.split(' ', 2)
        read_size = chunk_size = 64000
        
        offset = int(offset)
        size = int(size)
        remaining = size
        
        try:
            f = open(path, 'r')
        
            # If specifieds moves the read to the offset position
            if offset:
                f.seek(offset)
                
            # Starts the read process
            continue_reading = True
            while continue_reading:
                # Calculates the amount of data to be read, it will be the chunk size
                # most of the time, except when a specific amount of data is requested,
                # in such case the limit needs to be considered
                if size:
                    read_size = chunk_size if chunk_size < remaining else remaining
                
                data = f.read(read_size)
                
                # The read operation returns an empty string when no more data
                # is in the file
                if data:
                    self._client.send(data)
                    
                    # When a size is specified, the read will be done once that
                    # size is reached
                    if size:
                        remaining = remaining - read_size
                        
                        if remaining == 0:
                            continue_reading = False
                else:
                    continue_reading = False
            
            f.close()
                       
        except (IOError) as e:
            self._result_code = 1
            self._result_message = repr(e)

    def _process_getfile_lines(self):
        #GETFILE_LINES <skip> <path>
        skip, path = self._args.split(' ', 1)
        skip = int(skip)
        FileUtils.get_file_lines(path, skip, self._send_to_listener)
        
    def _process_get_free_space(self):
        free_space = FileUtils.get_free_space(self._args)
        self._client.send(str(free_space))
            
    def _process_check_dir_writable(self):
        self._client.send(str(FileUtils.check_dir_writable(self._args)))
        
    def _process_check_path_exists(self):
        self._client.send(str(FileUtils.check_path_exists(self._args)))
        
    def _process_create_directory(self):
        FileUtils.create_directory(self._args)
        
    def _process_create_directory_recursive(self):
        FileUtils.create_directory_recursive(self._args)
        
    def _process_remove_directory(self):
        FileUtils.remove_directory(self._args)
        
    def _process_remove_directory_recursive(self):
        FileUtils.remove_directory_recursive(self._args)
        
    def _process_delete_file(self):
        FileUtils.delete_file(self._args)
        
    def _process_copy_file(self):
        files = self._args.split('>')
        
        if len(files) < 2:
            raise RuntimeError('Invalid call to the COPY_FILE command')
        
        src = files[0]
        tgt = files[1]
        tgt_backup = ""
        if len(files) > 2:
            tgt_backup = files[2]
            
        src.strip()
        tgt.strip()
        tgt_backup.strip()
        FileUtils.copy_file(src, tgt, tgt_backup)
        
    def _process_get_file_owner(self):
        self._client.send(str(FileUtils.get_file_owner(self._args)))
        
    # Process any valid OS command that has not been handled by the other functions
    def _send_to_listener(self, data):
        self._client.send(data)
        
    def _process_exec(self):
        """
        Executes any OS valid command and sends the output to the command listener
        Syntax:
                EXEC <command>
                
                command: A valid OS command.
        """
        OSUtils.exec_command(self._args, self._send_to_listener)


# Creates the client to use it to send the output to the command listener
client = SocketClient('127.0.0.1', int(port), handshake, done_key)
            
try:
    if client.start():

        processor = CommandProcessor(command, client)

        processor.execute()
        
        client.close(processor._result_code, processor._result_message)
    
except Exception as e:
    if client._connected:
        client.close(1, repr(e))
    else:
        print(e)

            
