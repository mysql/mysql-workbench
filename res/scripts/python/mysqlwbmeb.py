#!/usr/bin/env python
# Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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
from time import strptime, strftime
import re
import subprocess
import logging
import json

is_library = True



def call_system(command, spawn, output_handler = None):
    result = 0

    logging.info('Executing command: %s' % command)

    if spawn or output_handler is None:
        try:
            if os.fork() != 0:
                return result
            
            os.setpgrp()
            
            for i in range(0,100):
                try:
                    os.close(i)
                except:
                    pass
                      
            os.execvp("/bin/sh", ["/bin/sh", "-c", command])
        except OSError, e:
            logging.error('Error on command execution: %s' % str(e))
            result = 1

    else:
        child = subprocess.Popen(command, bufsize=0, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, close_fds=True, preexec_fn=os.setpgrp)
        
    if not spawn:
        if output_handler:
            for line in iter(child.stdout.readline, ""):
                output_handler(line)

        child.wait()
        
        result = child.returncode

    return result


def check_version_at_least(command, major, minor, revno):
  output = StringIO.StringIO()
  
  ret_val = call_system("%s --version" % command, False, output.write)
  
  if ret_val == 0:
    tokens = output.getvalue().strip().split()
    
    version = ""
    found_major = 0
    found_minor = 0
    found_revno = 0
    
    for token in tokens:
      if token == 'version':
        version = 'found'
      else:
        if version == 'found':
          version = token
          version_tokens = version.split('.')
          found_major = int(version_tokens[0])
          found_minor = int(version_tokens[1])
          found_revno = int(version_tokens[2])
    
    return  (found_major, found_minor, found_revno) >= (major, minor, revno)


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

class ConfigUpdater(object):
        def __init__(self):
            self.config = None
           
        def load_config(self, file_name):
            self.config = ConfigReader(file_name)
            
        def update(self, file_name):
            my_source_handler = open(file_name, 'r')
            my_target_handler = open(file_name + ".tmp", 'w')
            
            current_section = None
            for line in my_source_handler:
                line = line.strip()
                
                if line.startswith('[') and line.endswith(']'):
                    current_section = line[1:-1]
                elif not line.startswith('#') and line.find('=') != -1:
                    att, val = line.split('=', 2)
                    
                    att = att.strip()
                    val = val.strip()
                    
                    if current_section:
                        new_val = self.config.read_value(current_section, att, False, None)
                        
                        if new_val is not None:
                            line = '%s = %s' % (att, new_val)
                
                my_target_handler.write('%s\n' % line)
                
            my_source_handler.close()
            my_target_handler.close()
            
            os.remove(file_name)
            os.rename(file_name + ".tmp", file_name)
            

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
        self._commands['VERSION'] = MEBHelperVersion
        self._commands['GET_PROFILES'] = MEBGetProfiles
        self._commands['UPDATE_SCHEDULING'] = MEBUpdateScheduling
        self._commands['PROPAGATE_DATA'] = MEBPropagateSettings


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
        self.backup_ok_re   = re.compile("^mysqlbackup completed OK!")  # according to http://dev.mysql.com/doc/mysql-enterprise-backup/3.10/en/mysqlbackup.html, we can rely on this appearing at the end of the backup log when the backup succeeds

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
        self.compress_method = profile.read_value('meb_manager', 'compress_method', False, 'lz4')
        self.compress_level = profile.read_value('meb_manager', 'compress_level', False, '1')
        self.skip_unused_pages = profile.read_value('meb_manager', 'skip_unused_pages', False, False) == 'True'

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
        
            if check_version_at_least(self.command, 3, 10, 0):
                # lz4 is the default so id it is selected only sets the --compress option
                if self.compress_method == 'lz4':
                    self.command_call += " --compress"
                 # Otherwise using --compress-method makes --compress to be not needed
                else:
                    self.command_call += " --compress-method=%s" % self.compress_method

                    # Level is only specified if not using the default value
                    if self.compress_level != '1':
                        self.command_call += " --compress-level=%s" % self.compress_level
            else:
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
        elif self.skip_unused_pages:
            self.command_call += " --skip-unused-pages"
                
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
        sub_folders.sort(reverse=True)    # sort backups in newest-to-oldest order
        lastest_time = base_time

        # find the most-recent valid backup
        for folder in sub_folders:
            if self.time_format_re.match(folder):

                if self.is_backup_dir_valid(os.path.join(path,folder)):
                    lastest_time = strptime(folder, self.time_format)
                    break

        return lastest_time

    def is_backup_dir_valid(self, folder):

        log_filename = folder + '.log'  # each /path/to/backup/dir has a corresponding /path/to/backup/dir.log

        # grep log file for the flag message
        f = open(log_filename, 'r')
        for line in f:
            if self.backup_ok_re.match(line):
                return True
        return False

class MEBUpdateScheduling(MEBCommand):
    def __init__(self, params = None, output_handler = None):
        super(MEBUpdateScheduling, self).__init__(params, output_handler)

        # Initializes these to False so they serve their purpose
        # in the case of a DELETE
        self.new_fb_schedule = "False"
        self.new_ib_schedule = "False"        

    def print_usage(self):
        self.write_output("UPDATE_SCHEDULING <profile> <old_label> <old_full> <old_incremental>")
        self.write_output("WHERE : <change>           : Indicates the operation being done with the profile: NEW, UPDATE, DELETE.")
        self.write_output("        <profile>          : is the UUID of the profile to be used for the scheduling.")
        self.write_output("        [<old_label>]      : indicates the label under which the jobs were scheduled. Applies on UPDATE and DELETE changes.")
        self.write_output("        [<old_full>]       : indicates if the profile was scheduled for full backup. (0 or 1). Applies on UPDATE and DELETE changes.")
        self.write_output("        [<old_incremental>]: indicates if the profile was scheduled for incremental backup. (0 or 1). Applies on UPDATE and DELETE changes.\n\n")

    def read_params(self):
        ret_val = False

        if self.param_count() >= 2:
            self.change = self.get_param(0)
            if self.change == "NEW":
                param_count = 2
            else:
                param_count = 5
            
            if self.param_count() == param_count:

                self.uuid = self.get_param(1)
                
                self.profile_file = '%s.cnf' % self.uuid
                
                if self.change != "NEW":
                    self.old_label = self.get_param(2)
                    self.old_fb_schedule = self.get_param(3)
                    self.old_ib_schedule = self.get_param(4)
                
                ret_val = True

        return ret_val

    def init_profile(self):
        # Gets the path to this file which should be on the
        # backups home directory
        this_file_path = os.path.realpath(__file__)
        backups_home = os.path.dirname(this_file_path)

        # Creates the full path to the profile file
        self.profile_file = os.path.join(backups_home, self.profile_file)

        # Loads the information to be used to create the command call
        self.profile = ConfigReader(self.profile_file)
        self.new_label = self.profile.read_value("meb_manager", "label", "")
        self.uuid = self.profile.read_value("meb_manager", "uuid", "")
        self.new_fb_schedule = self.profile.read_value("meb_manager", "full_backups_enabled", "")
        self.new_ib_schedule = self.profile.read_value("meb_manager", "inc_backups_enabled", "")
        self.compress = self.profile.read_value("meb_manager", "compress", "")
        self.apply_log = self.profile.read_value("meb_manager", "apply_log", "")
        self.backup_dir = self.profile.read_value('mysqlbackup', 'backup_dir', True, "")




    def read_profile_data(self, backup_type):
        self.frequency = self.profile.read_value("meb_manager", backup_type + "_backups_frequency", "")
        self.month_day = self.profile.read_value("meb_manager", backup_type + "_backups_month_day", "")
        self.week_days = self.profile.read_value("meb_manager", backup_type + "_backups_week_days", "")
        self.hour = self.profile.read_value("meb_manager", backup_type + "_backups_hour", "")
        self.minute = self.profile.read_value("meb_manager", backup_type + "_backups_minute", "")

        print "Loaded data for type : %s" % backup_type
        print "Frequency: %s" % self.frequency
        print "Month Day: %s" % self.month_day
        print "Week Days: %s" % self.week_days
        print "Hour: %s" % self.hour
        print "Minute: %s" % self.minute

    def get_unschedule_command(self, backup_type):
        cron_file = "%s/wb_cron_file" % os.path.dirname(__file__)

        if backup_type == 'full':
            command = "crontab -l | grep -P -v '%s.*%s\.cnf\s\d\s0' > '%s'; crontab '%s';rm '%s'" % (__file__, self.uuid, cron_file, cron_file, cron_file)
        else:
            command = "crontab -l | grep -P -v '%s.*%s\.cnf\s\d\s1' > '%s'; crontab '%s';rm '%s'" % (__file__, self.uuid, cron_file, cron_file, cron_file)
            
        return command

    def create_backup_command_call(self, backup_type):
        cmd_data = []

        cmd_data.append('"%s"' % __file__)

        # Appends the command to be handled by the helper
        cmd_data.append('BACKUP')

        # Uses the configuration file for the backup (MUST be the 1st option)
        cmd_data.append('%s.cnf' % self.uuid)

        real_compress = "0"
        real_incremental = "0"

        if backup_type == 'full':
            # The compress option is set only for full backups when apply-log is not
            # enabled, this is because --compress will be ignored anyway
            if self.compress and not self.apply_log:
                # 1 to indicate compress, 0 to indicate NOT incremental
                real_compress = "1"

        elif backup_type == 'inc':
            real_incremental = "1"

        cmd_data.append(real_compress)
        cmd_data.append(real_incremental)
        cmd_data.append("0")
        cmd_data.append("1")

        if self.apply_log == "True" and backup_type == 'full':
            cmd_data.append('backup-and-apply-log')
        else:
            cmd_data.append('backup')

        return " ".join(cmd_data)        

    def get_schedule_command(self, backup_type):
        # Configures the backup and log target paths
        log_path = self.backup_dir
        target_path = "\$BACKUP_NAME.log" 

        if backup_type == "inc":
            target_path = "inc/\$BACKUP_NAME.log"

        log_path = os.path.join(log_path, target_path)

        # Creates the mysqlbackup command call
        command = self.create_backup_command_call(backup_type)

        self.read_profile_data(backup_type)

        schedule = []
        schedule.append(str(self.minute))
        schedule.append('*' if self.frequency == "0" else str(self.hour))
        schedule.append('*' if self.frequency in ["0", "1", "2"] else str(self.month_day))
        schedule.append('*')
        schedule.append('*' if self.frequency != "2" else self.week_days)
        schedule.append("BACKUP_NAME=\$(date +\%Y-\%m-\%d_\%H-\%M-\%S); " + command)

        schedule.append('> \\"%s\\" 2>&1' % log_path)

        # A temporary file to store the crontab
        cron_file = "'%s/wb_cron_file'" % self.backup_dir

        cron_entry = " ".join(schedule)
        schedule_command = 'crontab -l > %s; echo "%s" >> %s; crontab %s; rm %s' % (cron_file, cron_entry, cron_file, cron_file, cron_file)

        return schedule_command

    def execute(self):
        output = StringIO.StringIO()
        ret_val = 0
        if self.read_params():
        
            if self.change != "DELETE":
                self.init_profile()
            
            # Unscheduling would NOT occur on NEW profiles
            command = ""
            if self.change != "NEW":
                label = self.new_label if hasattr(self, 'new_label') else self.old_label
                message = ", renamed from %s" % self.old_label if self.old_label != label else "."
                
                if self.old_fb_schedule == "1" or self.old_label != label:
                    command = self.get_unschedule_command("full")
                    logging.debug("Unscheduling full backup for profile: %s%s" % (label, message))
                    ret_val = ret_val + call_system(command, False, output.write)

                if self.old_ib_schedule == "1" or self.old_label != label:
                    command = self.get_unschedule_command("inc")
                    logging.debug("Unscheduling incremental backup for profile: %s%s" % (label, message))
                    ret_val = ret_val + call_system(command, False, output.write)
            
            if self.new_fb_schedule == "True":
                self.read_profile_data("full")
                command = self.get_schedule_command("full")
                logging.debug("Scheduling full backup for profile: %s." % self.new_label)
                ret_val = ret_val + call_system(command, False, output.write)
            
            if self.new_ib_schedule == "True":
                self.read_profile_data("inc")
                command = self.get_schedule_command("inc")
                logging.debug("Scheduling incremental backup for profile: %s." % self.new_label)
                ret_val = ret_val + call_system(command, False, output.write)
        else:
            self.print_usage()

        # In case of error prints the output
        if ret_val:
            self.write_output(output.getvalue().strip())
        
        return ret_val

class MEBPropagateSettings(MEBCommand):
    def __init__(self, params = None, output_handler = None):
        super(MEBPropagateSettings, self).__init__(params, output_handler)
        
        self.datafile = ""
        self.datadir = ""
        this_file_path = os.path.realpath(__file__)
        self.backups_home = os.path.dirname(this_file_path)
    
    def read_params(self):
        ret_val = False
        if self.param_count() == 1:
            self.datafile = self.get_param(0)
            ret_val = True

        return ret_val

    def print_usage(self):
        self.write_output("PROPAGATE_DATA <datadir>")
        self.write_output("WHERE : <datadir> : is the path to the datadir of the server instance for which the profiles are")
        self.write_output("                    being loaded. (There could be more than one instance on the same box).")
        

    def execute(self):
        ret_val = 1
        if self.read_params():
            ret_val = 0
            
            # Loads the information to be propagated
            updater = ConfigUpdater()
            source_config_file = os.path.join(self.backups_home, self.datafile)
            updater.load_config(source_config_file)
            self.datadir = updater.config.read_value('target', 'datadir', None, None)
        
            if self.datadir is not None:
                # Creates the string to be used on the file search
                search_string = os.path.join(self.backups_home, '*.cnf')

                # The glob module will be used to list only the required files
                import glob
                for filename in glob.glob(search_string):
                    # Creates a config reader for each profile
                    profile = ConfigReader(filename)

                    # Verifies the datadir to ensure it belongs to the requested instance
                    profile_datadir = profile.read_value('mysqlbackup', 'datadir', False, "")
                    if profile_datadir == self.datadir:
                        updater.update(filename)
            else:
                self.write_output('Data propagation file is missing the target datadir.')
                
            # Deletes the temporary file
            os.remove(source_config_file)
        else:
            self.print_usage()

        return ret_val

            
class MEBGetProfiles(MEBCommand):
    def __init__(self, params = None, output_handler = None):
        super(MEBGetProfiles, self).__init__(params, output_handler)
        self.datadir = ''
        self.meb_version=0

    def read_params(self):
        ret_val = False

        if self.param_count() == 2:
            self.meb_command = self.get_param(0)
            self.datadir = self.get_param(1)
            ret_val = True

        return ret_val

    def print_usage(self):
        self.write_output("GET_PROFILES <meb_command> <datadir>")
        self.write_output("WHERE : <meb_command> : is the path to a valid MEB executable.")
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
                
                # Verifies the profile configured command is the same as the received as parameter
                profile_command = profile.read_value('meb_manager', 'command', False, "")
                if profile_command != self.meb_command:
                    profile_issues |= 16

                # Verifies the datadir to ensure it belongs to the requested instance
                profile_datadir = profile.read_value('mysqlbackup', 'datadir', False, "")
                if profile_datadir == self.datadir:
                    data = {}
                    data['LABEL'] = profile.read_value('meb_manager', 'label', False, "")
                    data['PARTIAL'] = profile.read_value('meb_manager', 'partial', False, "")
                    data['USING_TTS'] = profile.read_value('meb_manager', 'using_tts', False, "0")
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
                    
                    # Profiles with version 0 could have a UUID as the value for the "includes" field
                    # It was used to make all of the InnoDB tables NOT matching the selection criteria on initial
                    # versions of MEB.
                    # On MEB 3.9.0 this became invalid as an error would be generated so
                    # If found in a profile, we need to report the invalid configuration on the UI.
                    profile_version = int(profile.read_value('meb_manager', 'version', False, "0"))
                    if profile_version == '0' and check_version_at_least(self.meb_command, 3, 9, 0):
                        include = profile.read_value('mysqlbackup', 'include', False, "")
                        if include:
                            expression='^[\dA-Fa-f]{8}-([\dA-Fa-f]{4}-){3}[\dA-Fa-f]{12}$'
                            compiled=re.compile(expression)
                            match=compiled.match(include)
                            
                            if match:
                                profile_issues |= 2
                
                    command = profile.read_value('meb_manager', 'command', False, "")
                    if check_version_at_least(command, 3, 10, 0):
                        include   = profile.read_value('mysqlbackup', 'include', False, "")
                        databases = profile.read_value('mysqlbackup', 'databases', False, "")
                    
                        if include or databases:
                            profile_issues |= 8
                    else:
                        include = profile.read_value('mysqlbackup', 'include_tables', False, "")
                        exclude = profile.read_value('mysqlbackup', 'exclude_tables', False, "")
                        
                        if include or exclude:
                          profile_issues |= 4

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

class MEBHelperVersion(MEBCommand):
    def __init__(self, params = None, output_handler = None):
        super(MEBHelperVersion, self).__init__(params, output_handler)
        
    def execute(self):
        try:
          import hashlib

          file = open (__file__, 'r')
          data = file.read()
          md5 = hashlib.md5(data)

          self.write_output(md5.hexdigest())
        except Exception, e:
            logging.error('MEBHelperVersion error ' % str(e))
            return 1
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











