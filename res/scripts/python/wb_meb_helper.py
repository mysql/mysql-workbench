#!/usr/bin/env python

# Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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
import StringIO
import ConfigParser
from time import strptime, strftime, localtime
import re
import subprocess
import logging
import time
import json
from multiprocessing import Process



def call_system(command, spawn):
    result = 0

    logging.info('Executing command: %s' % command)
    child = subprocess.Popen(command, bufsize=0, close_fds=True, shell=True, preexec_fn=os.setpgrp)

    if not spawn:
        child.wait()
        result = result = child.returncode

    return result



class ConfigReader(object):
    def __init__(self, file):
        self._file = file

        profile_file = open(file, 'r')
        data = profile_file.read()

        self.doc = ConfigParser.ConfigParser()
        self.doc.readfp(StringIO.StringIO(data))        

    def read_value(self, section, item, mandatory = False, default = None):
        value = default

        try:
            value = self.doc.get(section, item)
        except:
            if mandatory:
                raise
            else:
                pass

        return value


class MEBCommand(object):
    def __init__(self):
        pass

    def read_params(self):
        pass

    def print_usage(self):
        pass

    def execute(self):
        if self.read_params():
            print "ERROR: Missing command execution logic"
        else:
            self.print_usage()
            
        return 1


class MEBCommandProcessor(MEBCommand):
    def __init__(self, argv):
        self.help_needed = False

        self._commands = {}
        self._commands['BACKUP'] = MEBBackup()
        self._commands['VERSION'] = MEBVersion()


    def read_params(self):
        ret_val = True
        try:
            self.command = sys.argv[1]

            if self.command.upper() == 'HELP':
                self.command = sys.argv[2]
                self.help_needed = True
        except IndexError:
            print "Error executing helper, use it as follows:"
            logging.error("Unable to execute %s with no parameters." % __file__)
            ret_val = False

        return ret_val

    def print_usage(self):
        print "wb_meb_helper <command> <parameters>"
        print "WHERE : <command>        : is one of %s" % ','.join(self._commands.keys())
        print "        <parameters>     : are the parameters needed for each command."
        print "\nYou can also use as follows to get each command parameters:\n"
        print "wb_meb_helper help <command>"
        print "WHERE : <command>        : is one of %s\n" % self._commands.keys()

    def execute(self):
        processed = False
        ret_val = 1
        if self.read_params():
            command = self.command.upper()
            if self._commands.has_key(command):
                processed = True
                if self.help_needed:
                    self._commands[command].print_usage()
                    ret_val = 0
                else:
                    ret_val = self._commands[command].execute()


        if not processed:
            print "\nERROR Executing wb_meb_helper\n"
            self.print_usage()
            
        return ret_val



class MEBBackup(MEBCommand):
    def __init__(self):
        self.file_name=''
        self.time_format = '%Y-%m-%d_%H-%M-%S'
        self.time_format_re = re.compile("\\b[1-9]\\d\\d\\d\\b-[0-1]\\d-[0-3]\\d_[0-2]\\d-[0-5]\\d-[0-5]\\d")

    def print_usage(self):
        print "BACKUP <profile> <compress> <incremental> <to_single_file> <report_progress> <command>"
        print "WHERE : <profile>        : is the UUID of the profile to be used for the backup process."
        print "        <compress>       : indicates if the backup will be compressed. (1 or 0)"
        print "        <incremental>    : indicates the backup should be incremental.  (1 or 0)"
        print "        <to_single_file> : indicates the backup will be done to an image file.  (1 or 0)"
        print "        <report_progress>: indicates the backup should be compressed.  (1 or 0)"
        print "        <command>        : indicates the backup operation to be done, could be backup or backup-and-apply-log"
        print "        [file_name]      : indicates the target name for the backup file or folder\n\n"

    def read_bool_param(self, param):
        true_param = ['1', 'TRUE', 'YES']

        return param.upper() in true_param;

    def read_params(self):
        ret_val = False
        param_count = len(sys.argv)
        
        if param_count >= 8:
            self.profile_file = sys.argv[2]
            self.compress = self.read_bool_param(sys.argv[3])
            self.incremental = self.read_bool_param(sys.argv[4])
            self.to_single_file = self.read_bool_param(sys.argv[5])
            self.report_progress = self.read_bool_param(sys.argv[6])
            self.bkcommand = sys.argv[7]
            
            if param_count > 8:
                self.file_name = sys.argv[8]

            ret_val = True

        return ret_val

    def read_profile_data(self):
        # Gets the path to this file which should be on the
        # backups home directory
        this_file_path = os.path.realpath(__file__)
        backups_home = os.path.dirname(this_file_path)

        # Creates the full path to the profile file
        self.profile_file = os.path.join(backups_home, self.profile_file)

        # Loads the information to be used to create the command call
        profile = ConfigReader(self.profile_file)
        self.command = profile.read_value('meb_manager', 'command', True, "")
        self.backup_dir = profile.read_value('mysqlbackup', 'backup_dir', True, "")
        self.inc_backup_dir = profile.read_value('mysqlbackup', 'incremental_backup_dir', True, "")

    def set_backup_paths(self):
        target_folder = ''

        # Defines the target file/folder name for the backup
        if self.file_name != '':
            if self.to_single_file:
                if self.file_name.lower()[-4:] != '.mbi':
                    self.file_name += '.mbi'

                # On an image backup the backup dir will be the one
                # Received as a parameter
                self.backup_dir = self.file_name[:-4]
                self.log_path = self.file_name.replace('.mbi', '.log')
            else:
                # If a file name is passed it is used as the backup folder
                target_folder = self.file_name
        else:
            # If no file name is passed, uses the timestamp to create one
            target_folder = strftime(self.time_format)
            
        # The full path is the target folder under the backups home for
        # the profile
        if target_folder:
            self.backup_dir = os.path.join(self.backup_dir, target_folder)
            self.log_path = self.backup_dir + '.log'
        

    def prepare_command(self):
        ret_val = True
        self.command_call = '"%s" --defaults-file="%s"' % (self.command, self.profile_file)

        # Adds the compress parameter if needed
        if self.compress:
            self.command_call += " --compress"

        # Get the right path parameter, path and running type 
        path_param = " --backup-dir"

        # If the backup is incremental
        if self.incremental:
            base_folder = self.get_incremental_base_folder()

            if base_folder:
                self.command_call += ' --incremental --incremental-base=dir:"%s"' % base_folder
                self.backup_dir = self.inc_backup_dir
                path_param = '  --incremental-backup-dir'
            else:
                print "ERROR: Unable to run incremental backup without a base folder."
                ret_val = False
                
        # Sets the needed backup paths
        self.set_backup_paths()

        # Adds the backup folder to the command
        self.command_call += ' %s="%s"' % (path_param, self.backup_dir)

        if self.to_single_file:
            self.command_call += ' --backup-image=%s' % self.file_name
            
        if self.report_progress:
            self.command_call += ' --show-progress=stdout'

        self.command_call += ' %s > "%s" 2>&1' % (self.bkcommand, self.log_path)

        return ret_val

    def execute(self):
        ret_val = 1
        if self.read_params():
            # Loads the profile information to be used on the
            # explicitly on the command call
            self.read_profile_data()

            if self.prepare_command():
                # Spawns the backup command execution
                ret_val = call_system(self.command_call, True)
                
                # Prints the data to be returned back
                print json.dumps({'LOG_PATH':self.log_path})

        else:
            self.print_usage()
            
        return ret_val


    def get_incremental_base_folder(self):
        base_folder=''
        base_tstamp=strptime('1900-01-01_00-00-00', self.time_format)

        lastest_full = self.find_lastest_backup(self.backup_dir, base_tstamp)

        if lastest_full > base_tstamp:
            base_folder = self.backup_dir
            base_tstamp = lastest_full

            lastest_inc = self.find_lastest_backup(self.inc_backup_dir, base_tstamp)

            if lastest_inc > base_tstamp:
                base_tstamp = lastest_inc
                base_folder = self.inc_backup_dir

            if base_folder:
                base_folder = os.path.join(base_folder, strftime(self.time_format, base_tstamp))

        return base_folder


    def find_lastest_backup(self, path, base_time):

        sub_folders = [name for name in os.listdir(path) if os.path.isdir(os.path.join(path, name))]
        lastest_time = base_time

        for folder in sub_folders:
            if self.time_format_re.match(folder):
                folder_time=strptime(folder, self.time_format)

                if folder_time > lastest_time:
                    lastest_time = folder_time

        return lastest_time

class MEBVersion(MEBCommand):
    current = "1"
        
    def execute(self):
        print self.current
        return 0
    

if __name__ == "__main__":
    # Initializes the logging module
    log_file = '%s.log' % __file__[:-3]
    logging.basicConfig(filename=log_file, level=logging.DEBUG)
    logging.basicConfig(format='[%(asctime)s] %(message)s', datefmt='%Y-%m-%d %I:%M:%S %p')


    processor = MEBCommandProcessor(sys.argv)
    sys.exit(processor.execute())











