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

is_library = True



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
    def __init__(self, params = None, output_handler = None):
        self.params = params
        self.output_handler = output_handler

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

    # The parameter count increases in 2 when run as a script
    # The first two parameters are the script name and the command identifier
    def param_count(self):
        return len(self.params) if is_library else len(self.params) - 2

    def get_param(self, index):

        real_index = index
        if not is_library:
            real_index += 2

        return self.params[real_index]

    def write_output(self, output):
        if self.output_handler:
            self.output_handler(output + '\n')
        else:
            print output


class MEBCommandProcessor(MEBCommand):
    def __init__(self, params = None, output_handler = None):
        super(MEBCommandProcessor, self).__init__(params, output_handler)
        self.help_needed = False

        self._commands = {}
        self._commands['BACKUP'] = MEBBackup
        self._commands['VERSION'] = MEBVersion
        self._commands['GET_PROFILES'] = MEBGetProfiles


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
        self.write_output("mysqlwbmeb <command> <parameters>")
        self.write_output("WHERE : <command>        : is one of %s" % ','.join(self._commands.keys()))
        self.write_output("        <parameters>     : are the parameters needed for each command.")
        self.write_output("\nYou can also use as follows to get each command parameters:\n")
        self.write_output("mysqlwbmeb help <command>")
        self.write_output("WHERE : <command>        : is one of %s\n" % self._commands.keys())

    def execute(self):
        processed = False
        ret_val = 1
        if self.read_params():
            command = self.command.upper()
            if self._commands.has_key(command):
                klass = self._commands[command]
                instance = klass(self.params, self.output_handler)
                processed = True
                if self.help_needed:
                    instance.print_usage()
                    ret_val = 0
                else:
                    ret_val = instance.execute()


        if not processed:
            self.write_output("\nERROR Executing mysqlwbmeb\n")
            self.print_usage()
            
        return ret_val

class MEBBackup(MEBCommand):
    def __init__(self, params = None, output_handler = None):
        super(MEBBackup, self).__init__(params, output_handler)
        self.file_name=''
        self.time_format = '%Y-%m-%d_%H-%M-%S'
        self.time_format_re = re.compile("\\b[1-9]\\d\\d\\d\\b-[0-1]\\d-[0-3]\\d_[0-2]\\d-[0-5]\\d-[0-5]\\d")

    def print_usage(self):
        self.write_output("BACKUP <profile> <compress> <incremental> <to_single_file> <report_progress> <command>")
        self.write_output("WHERE : <profile>        : is the UUID of the profile to be used for the backup process.")
        self.write_output("        <compress>       : indicates if the backup will be compressed. (1 or 0)")
        self.write_output("        <incremental>    : indicates the backup should be incremental.  (1 or 0)")
        self.write_output("        <to_single_file> : indicates the backup will be done to an image file.  (1 or 0)")
        self.write_output("        <report_progress>: indicates the backup should be compressed.  (1 or 0)")
        self.write_output("        <command>        : indicates the backup operation to be done, could be backup or backup-and-apply-log")
        self.write_output("        [file_name]      : indicates the target name for the backup file or folder\n\n")

    def read_bool_param(self, param):
        true_param = ['1', 'TRUE', 'YES']

        return param.upper() in true_param;

    def read_params(self):
        ret_val = False
        if self.param_count() >= 6:
            self.profile_file = self.get_param(0)
            self.compress = self.read_bool_param(self.get_param(1))
            self.incremental = self.read_bool_param(self.get_param(2))
            self.to_single_file = self.read_bool_param(self.get_param(3))
            self.report_progress = self.read_bool_param(self.get_param(4))
            self.bkcommand = self.get_param(5)
            
            if self.param_count() > 6:
                self.file_name = self.get_param(6)

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
        self.use_tts = profile.read_value('meb_manager', 'using_tts', False, "0")

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
                self.write_output("ERROR: Unable to run incremental backup without a base folder.")
                ret_val = False
                
        # Sets the needed backup paths
        self.set_backup_paths()

        # Adds the backup folder to the command
        self.command_call += ' %s="%s"' % (path_param, self.backup_dir)

        if self.to_single_file:
            self.command_call += ' --backup-image=%s' % self.file_name
            
        if self.use_tts != "0":
            tts_value="with-minimum-locking"
            if self.use_tts=="2":
                tts_value="with-full-locking"
                
            self.command_call += " --use-tts=%s" % tts_value
            
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
                self.write_output(json.dumps({'LOG_PATH':self.log_path}))

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

class MEBGetProfiles(MEBCommand):
    def __init__(self, params = None, output_handler = None):
        super(MEBGetProfiles, self).__init__(params, output_handler)
        self.datadir = ''
        self.meb_version=0

    def read_params(self):
        ret_val = False

        if self.param_count() == 2:
            self.meb_version = int(self.get_param(0))
            self.datadir = self.get_param(1)
            ret_val = True

        return ret_val

    def print_usage(self):
        self.write_output("GET_PROFILES <meb_version> <datadir>")
        self.write_output("WHERE : <meb_version> : is the profile version required by the meb being used at the server for backups")
        self.write_output("WHERE :               : is the path to the datadir of the server instance for which the profiles are")
        self.write_output("                        being loaded. (There could be more than one instance on the same box).")

        if is_library:
            self.write_output("\nWhen executed from python code the WBMEB_BACKUPSHOME environment variable must be set.")
        

    def execute(self):
        # Gets the backups home path
        backups_home = ''
        if os.environ.has_key('WBMEB_BACKUPSHOME'):
            backups_home = os.environ['WBMEB_BACKUPSHOME']

        if backups_home and self.read_params():
            master_data = {}


            # Creates the string to be used on the file search
            search_string = os.path.join(backups_home, '*.cnf')

            # The glob module will be used to list only the required files
            import glob
            for filename in glob.glob(search_string):
                profile_issues = 0
                
                # Creates a config reader for each profile
                profile = ConfigReader(filename)

                # Verifies the datadir to ensure it belongs to the requested instance
                profile_datadir = profile.read_value('mysqlbackup', 'datadir', False, "")
                if profile_datadir == self.datadir:
                    data = {}
                    data['LABEL'] = profile.read_value('meb_manager', 'label', False, "")
                    data['PARTIAL'] = profile.read_value('meb_manager', 'partial', False, "")
                    data['USING_TTS'] = profile.read_value('meb_manager', 'using_tts', False, "")
                    data['BACKUP_DIR'] = profile.read_value('mysqlbackup', 'backup_dir', False, "")

                    # Gets the folder stats to calculate available space
                    fs = os.statvfs(data['BACKUP_DIR'])
                    total = fs.f_frsize * fs.f_blocks
                    free = fs.f_frsize * fs.f_bfree

                    # Formats to a human readable format
                    data['AVAILABLE'] = self.get_available_space(total, free)

                    # Validates the backups folder for write permission
                    if not os.access(data['BACKUP_DIR'], os.W_OK):
                        profile_issues |= 1

                    # Gets the full schedule data
                    e = profile.read_value('meb_manager', 'full_backups_enabled', False, "")
                    f = profile.read_value('meb_manager', 'full_backups_frequency', False, "")
                    md = profile.read_value('meb_manager', 'full_backups_month_day', False, "")
                    wd = profile.read_value('meb_manager', 'full_backups_week_days', False, "")
                    h = profile.read_value('meb_manager', 'full_backups_hour', False, "")
                    m = profile.read_value('meb_manager', 'full_backups_minute', False, "")
                    data['FSCHEDULE'] = '-'.join([e, f, md, wd, h, m])

                    # Gets the incremental schedule data
                    e = profile.read_value('meb_manager', 'inc_backups_enabled', False, "")
                    f = profile.read_value('meb_manager', 'inc_backups_frequency', False, "")
                    md = profile.read_value('meb_manager', 'inc_backups_month_day', False, "")
                    wd = profile.read_value('meb_manager', 'inc_backups_week_days', False, "")
                    h = profile.read_value('meb_manager', 'inc_backups_hour', False, "")
                    m = profile.read_value('meb_manager', 'inc_backups_minute', False, "")
                    data['ISCHEDULE'] = '-'.join([e, f, md, wd, h, m])
                    
                    # Gets the profile version
                    p_version = int(profile.read_value('meb_manager', 'version', False, "0"))
                    if p_version == 0 and self.meb_version > 0:
                        include = profile.read_value('mysqlbackup', 'include', False, "")
                        if include:
                            expression='^[\dA-Fa-f]{8}-([\dA-Fa-f]{4}-){3}[\dA-Fa-f]{12}$'
                            compiled=re.compile(expression)
                            match=compiled.match(include)
                            
                            if match:
                                profile_issues |= 2

                    # The VALID item will cintain a numeric valid describing the issues encountered on the profile
                    # Validation. Each issue should be assigned a value of 2^x so the different issues can be joined
                    # using bitwise operations
                    # 1 : Indicates the backup folder is not valid to store the backups.
                    # 2 : Indicates a partial backup profile using a regular expression on the include parameter.
                    data['VALID'] = profile_issues
                    

                    master_data[filename] = data

            # If any, prints the profile data
            if master_data:
                self.write_output(json.dumps(master_data))

            return 0
        else:
            self.print_usage()
            return 1

    def get_available_space(self, total, available):
        suffixes = ['B', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB']
        limit = float(1024)
        index = 0
        while total > limit:
            total /= limit
            available /= limit
            index +=1

        return "%s%s of %s%s available" % ('{0:.2f}'.format(available), suffixes[index], '{0:.2f}'.format(total), suffixes[index])
            
        return 0

class MEBVersion(MEBCommand):
    current = "1"

    def __init__(self, params = None, output_handler = None):
        super(MEBVersion, self).__init__(params, output_handler)
        
    def execute(self):
        self.write_output(self.current)
        return 0
    

if __name__ == "__main__":
    # The parameters passed to the command will have an offset when called
    # form the script, this is the script name and the command name
    is_library = False

    # Setups some environment variables
    this_file_path = os.path.realpath(__file__)
    os.environ['WBMEB_BACKUPSHOME'] = os.path.dirname(this_file_path)

    # Initializes the logging module
    log_file = '%s.log' % __file__[:-3]
    logging.basicConfig(filename=log_file, level=logging.DEBUG)
    logging.basicConfig(format='[%(asctime)s] %(message)s', datefmt='%Y-%m-%d %I:%M:%S %p')

    processor = MEBCommandProcessor(sys.argv)
    sys.exit(processor.execute())











