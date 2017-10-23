# Copyright (c) 2007, 2016, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
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

import copy
import errno
import pprint
import os
import tempfile
import difflib
import re
import sys

import opts
import mforms

from mforms import Utilities, Form, newBox, newLabel, newTextBox, newButton, App
from wb_common import OperationCancelledError, InvalidPasswordError, PermissionDeniedError, splitpath, Users

from workbench.log import log_info, log_debug, log_debug2, log_debug3


from wb_server_control import ServerProfile

from workbench.utils import server_version_str2tuple

pysource = {}
pysource['engine-list'] = "grt.root.wb.options.options[\"@db.mysql.Table:tableEngine/Items\"]"
multi_separator = r'\n'
default_section = 'mysqld'

#-------------------------------------------------------------------------------
def ver_cmp(v1, v2):
    minlen = min(len(v1), len(v2))
    return cmp(v1[0:minlen], v2[0:minlen])

#-------------------------------------------------------------------------------
def option_is_for_version(version, versions_list):
    (inver, vlist, outver) = versions_list

    ret = False

    matched_version = None
    # Check if we have x.y version in list which allows version
    for v in vlist:
        if ver_cmp(version, v) == 0:
            matched_version = v
            ret = True
            break

    # Check that version is within introduced and removed
    # If not drop ret to False
    if matched_version:
        if inver is not None:
            for iv in inver:
                if matched_version[:2] == iv[:2]:
                    if ver_cmp(version, iv) < 0:
                        ret = False
                    break

        if outver is not None:
            for ov in outver:
                if matched_version[:2] == ov[:2]:
                    if ver_cmp(version, ov) > 0:
                        ret = False
                    break

    return ret

#===============================================================================
# vd - version of server when option become deprecated
# vs - current server version
def is_opt_deprecated(vd, vs):
    skip = False

    if vd is not None and vs is not None:
        vdl = len(vd)
        vsl = len(vs)

        if vdl == vsl:
            if vd < vs:
                skip = True
        else:
            if vdl < vsl:
                if vd <= vs[:vdl]:
                    skip = True
                elif vs <= vd[:vsl]:
                    skip = True

    return skip

#-------------------------------------------------------------------------------
def parse_version_str(version_str):
    version = None
    try:
        res = re.match("([0-9]+)\.([0-9]+)\.([0-9]+)|([0-9]+)\.([0-9]+)", version_str)
        if res:
            tokens = res.groups()
            if tokens[0] is not None:
                version = (int(tokens[0]), int(tokens[1]), int(tokens[2]))
            else:
                version = (int(tokens[3]), int(tokens[4]))
    except ValueError:
        print "ERROR! Incorrect version attribute value '" + version_str + "', ", type(version_str)

    return version

#-------------------------------------------------------------------------------
def pick_value(opt, version, platform):
    value = None
    if 'values' in opt:
        # Walk all values and pick best match
        for i, cur_value in enumerate(opt['values']):
            if 'bitsize' in cur_value:
                is_64bit = sys.maxsize > 2**32
                if (is_64bit and cur_value['bitsize'] != '64') or (not is_64bit and cur_value['bitsize'] != '32'):
                    continue
            inversion  = cur_value.get('inversion')
            outversion = cur_value.get('outversion')

            if inversion is None:
                inversion = (0,0,0)
                # use the outversion of the previous item
                if i > 0:
                    tmp = opt['values'][i-1].get('outversion')
                    if not tmp:
                        log_debug2("Option %s has invalid version range for defaults\n" % opt['caption'])
                    else:
                        inversion = tmp

            if outversion is None:
                outversion = (99,0,0)
                # use the inversion of the next item
                if i < len(opt['values'])-1:
                    tmp = opt['values'][i+1].get('inversion')
                    if tmp == inversion and len(opt['values']) -2 > i:
                        tmp = opt['values'][i+2].get('inversion')    
                    if not tmp: # if no value for inversion, assume outversion is both
                        tmp = opt['values'][i+1].get('outversion')
                    if not tmp:
                        log_debug2("Option %s has invalid version range for defaults\n" % opt['caption'])
                    else:
                        outversion = tmp

            if version >= inversion and version <= outversion:
                platform_match = False
                if 'platform' in cur_value:
                    pl = cur_value['platform']
                    if pl == platform or pl == 'all' or platform == "all":
                        platform_match = True
                else:
                    platform_match = True

                if platform_match:
                    value = cur_value
                    break

    return value

#-------------------------------------------------------------------------------
# @server_version is a tuple, platform - "windows", "linux", "macos"


#===============================================================================
class Option(object):
    def __init__(self, section, line = None, value = None):
        self.section = section
        self.values = []
        if line is not None and value is not None:
            self.values.append((line, value))

    def append(self, line, value):
        self.values.append((line, value))

    def is_multiline(self):
        return len(self.values) > 1

    def is_switch_opt(self):
        ret = False
        if len(self.values) > 0:
            ret = type(self.values) == bool
        return ret

    def val(self, i):
        return self.values[i][1]
    
    # Return the last value from the file, if more then one exists
    def value(self):
        result_value = None
        result_line = 0
        for value in self.values:
            if value[0] > result_line:
                result_line = value[0]
                result_value = value[1]
        return result_value
      
    def line(self, i):
        return self.values[i][0]

    def __iter__(self):
        return iter(self.values)

    def __len__(self):
        return len(self.values)

    def __str__(self):
        return multi_separator.join([str(x[1]) for x in self.values])

    def __repr__(self):
        return pprint.pformat(self.values)

#===============================================================================

CHANGE = 1
DELETE = 2
ADD    = 3

#===============================================================================
# ApplyWizard class
class ApplyWizard(Form):
        #view = None     # Holds mforms view(currently TextBox) to display diff and file text
        #view_btn = None # We need ref to this button as pressing on it changes view mode
                    # from diff to file. So the button's label must be changed accordingly
        #cmds_view = None # That is the display widget of commands to execute to save file
        #accept_action = None # This is an accept_action passed from client via ApplyWizard::show

        #---------------------------------------------------------------------------
    def __init__(self, owner, ctrl_be, server_profile):
        Form.__init__(self, None)

        self.set_title("Apply Changes to MySQL Configuration File")

        self.server_profile = server_profile
        self.ctrl_be = ctrl_be
        self.is_win = server_profile.target_is_windows

        content = newBox(False)
        content.set_padding(12)
        content.set_spacing(12)

        file = self.server_profile.config_file_path
        msg = "The following changes were made to the configuration file \"%s\"\nand will be saved when you click [Apply]." % file
        msg += "\nYou may edit the File Preview if you wish to make more changes manually."
        msg += "\nPlease review carefully as some mistakes could prevent the MySQL server from starting."
        content.add(newLabel(msg), False, True)

        self.file_textbox = newTextBox(mforms.BothScrollBars)
        self.file_textbox.set_bordered(True)
        self.diff_view_textbox = newTextBox(mforms.BothScrollBars)
        self.diff_view_textbox.set_bordered(True)
        content.add(self.file_textbox, True, True)
        content.add(self.diff_view_textbox, True, True)

        button_box = newBox(True)
        button_box.set_spacing(12)
        apply_btn = newButton()
        apply_btn.set_text("Apply")

        self.cancel_btn = newButton()
        self.cancel_btn.set_text("Cancel")
        self.cancel_btn.add_clicked_callback(self.cancel_clicked)

        self.view_btn = newButton()
        self.view_btn.add_clicked_callback(self.switch_view)

        button_box.add(self.view_btn, False, True)

        button_box.add_end(apply_btn, False, True)
        button_box.add_end(self.cancel_btn, False, True)

        content.add(button_box, False, True)

        #panel = Panel(TitledBoxPanel)
        #panel.set_title("Commands which will be run to save config file")

        # unused???
        #self.cmds_view = newLabel()
        #self.cmds_view.set_wrap_text(True)

        #panel.add(self.cmds_view)
        #content.add(panel, False, False)

        self.set_content(content)
        self.center()
        self.set_size(640,480)
        apply_btn.add_clicked_callback(self.apply_clicked)

    def switch_view(self):
        if self.view_diff == False:
                #switch to view diff
            self.file_textbox.show(False)
            self.diff_view_textbox.show(True)
            self.view_btn.set_text("View File Preview")
            #self.save_from_preview_btn.set_enabled(False)
            self.view_diff = True
        else:
            self.view_btn.set_text("View Changes")
            self.file_textbox.show(True)
            self.diff_view_textbox.show(False)
            #self.save_from_preview_btn.set_enabled(True)
            self.view_diff = False

    def apply_clicked(self):
        if self.accept_action is not None:
            text_from_box = self.file_textbox.get_string_value()
            if text_from_box != self.cfgfile:
                answer = Utilities.show_message("Confirm Changes from Preview"
                  ,"You have made additional edits to the configuration file in preview.\n"
                   "If you wish to save these changes click [Yes], "
                   "or if you wish to ignore these changes click [No]."
                  ,"Yes", "No", "")
                if answer != mforms.ResultOk:
                    text_from_box = None

            if text_from_box:
                App.get().set_status_text("Saving Configuration File with Manual Edits...")
            else:
                App.get().set_status_text("Saving Configuration File...")

            while True:
                try:
                    self.accept_action(text_from_box) # call action passed from caller(client)
                    App.get().set_status_text("Configuration File Saved.")
                except InvalidPasswordError, exc:
                    r = Utilities.show_error("Could not Save Configuration File",
                                             "Invalid admin password while saving the configuration file.\nClick Retry to enter the password again.",
                                  "Retry", "Cancel", "")
                    if r == mforms.ResultOk:
                        continue
                except OperationCancelledError, exc:
                    r = Utilities.show_message("Could not Save Configuration File",
                                      "Password input was cancelled. The file was not saved.",
                                      "OK", "", "")
                    return
                except Exception, exc:
                    import traceback
                    traceback.print_exc()
                    App.get().set_status_text("Error Saving Configuration File.")
                    Utilities.show_error("Could not Save Configuration File", "There was an error saving the configuration file.\n%s: %s"%(type(exc).__name__, exc), "OK", "", "")
                break

        self.close()

    def cancel_clicked(self):
        App.get().set_status_text("Cancelled Save of Configuration File")
        self.accept_action = None
        self.close()

    def show(self, changes_text, temp_file_content, accept_action = None):
        self.accept_action = accept_action
        self.view_diff = False            # Set view mode to list changes (diff)
        self.cfgfile = temp_file_content  # Content of the file
        self.file_textbox.set_value(self.cfgfile)
        self.diff_view_textbox.set_value(changes_text)

        self.switch_view() # Make it show diff

        self.show_modal(None, self.cancel_btn)


#===============================================================================
class WbAdminConfigFileBE(object):

    class ChangesetItem:
        def __init__(self, m,s,n,v):
            self.mod = m
            self.section = s
            self.value = v
            self.name = n
            self.orig_opt = None

        def __repr__(self):
            if self.mod == ADD:
                s = "+"
            elif self.mod == DELETE:
                s = "-"
            elif self.mod == CHANGE:
                s = "*"
            s += self.section + ":" + self.name + "=" + str(self.value)
            if self.orig_opt:
                s += ";[" + str(self.orig_opt) + "]"
            return s + "   "

    #---------------------------------------------------------------------------
    def __init__(self, server_profile, ctrl_be):
        self.file_lines = []
        self.original_opts = {}
        self.file_name = ""
        self.needs_root_for_file_read = False
        self.changeset = {}
        self.sections = []
        self.apply_form = None
        self.server_profile = server_profile
        self.ctrl_be = ctrl_be
        self.is_win = server_profile.target_is_windows
        self.opt_rindex = {} # self.opt_rindex stores dict of the following format: <opt_name> -> (opt_def, tabname, groupname)
        self.option_set_stats = None

        self.reload_possible_options()

    #---------------------------------------------------------------------------
    def get_file_content(self):
        return self.file_lines

    #---------------------------------------------------------------------------
    def get_option_set_stats(self):
        return self.option_set_stats

    #---------------------------------------------------------------------------
    def reload_possible_options(self):
        server_version = self.server_profile.server_version

        if server_version and str(server_version) != '':
            if type(server_version) is not tuple:
                server_version = server_version_str2tuple(server_version)

        if (not server_version or str(server_version) == ''):
            if not self.ctrl_be.target_version:
                server_version = (5, 1, 0)
            else:
                server_version = self.ctrl_be.target_version.majorNumber, self.ctrl_be.target_version.minorNumber, self.ctrl_be.target_version.releaseNumber
            log_debug2('Got server version "%s" from the server\n' % str(server_version))
        else:
            log_info("Note! Workbench uses server version '%s' from the server instance profile. Make the entry empty to auto pick version from the server.\n" % (str(server_version)))

        self.possible_options = self.transform_opts_for(server_version, self.server_profile.target_os)

    #---------------------------------------------------------------------------
    def get_possible_options(self):
        return self.possible_options


    #---------------------------------------------------------------------------
    def transform_opts_for(self, server_version, platform):
        log_debug2('Filtering options for %s, "%s"\n' % (str(server_version), platform) )
        # filter by version using opt def versions
        # remove deprecated
        # pick value according to version/bitsize/platform
        pos = 1
        tabs = {}
        self.opt_rindex = {}
        (added, skipped, deprecated, no_value) = (0, 0, 0, 0)

        for (tabname, tabcont, width) in opts.opts_list:
            new_tab_cont = {}
            groups = []

            for (grpname, grpcont) in tabcont:
                grp = {}
                grp['caption'] = grpname
                controls = []
                for opt in grpcont:
                    if 'versions' in opt:
                        if not option_is_for_version(server_version, opt['versions']):
                            #print "skipping ", server_version,  opt,"\n------------------"
                            skipped += 1
                            continue
                    if 'deprecated' in opt:
                        if is_opt_deprecated(opt['deprecated'], server_version):
                            #print "skipping deprecated", opt, "\n------------------"
                            deprecated += 1
                            continue
                    value = pick_value(opt, server_version, platform)
                    #print "pick_value", value
                    if value:
                        copt = copy.copy(opt)
                        del copt['values']
                        copt.update(value) # we have removed all possible values for the option
                                          # and added picked value definition for the cur version and platform
                        controls.append(copt)
                        added += 1
                        if 'default' in copt and copt.get('type') == 'boolean':
                            copt['default'] = self.normalize_bool(copt['default'])
                        # Store same option content for all alt names
                        for name in self.option_alt_names(copt['name']):
                            self.opt_rindex[name] = (copt, tabname, grpname)
                    else:
                        log_debug('Option "%s" skipped because of missing value\n' % opt['name'])
                        no_value += 1

                # Keep options sorted in the UI
                controls.sort(cmp = lambda r1, r2: cmp(r1['name'], r2['name']))
                grp['controls'] = tuple(controls)
                groups.append(grp)

            new_tab_cont['groups'] = tuple(groups)
            new_tab_cont['position'] = pos
            new_tab_cont['width'] = width
            pos += 1
            tabs[tabname] = new_tab_cont

        self.option_set_stats = {"version": server_version, "added" : added, "skipped" : skipped, "skipped_no_value" : no_value, "deprecated" : deprecated}
        print "Prepared options set for server version '%s' on '%s' platform: added - %i, skipped - %i, skipped with no value - %i, deprecated - %i" % (server_version, platform, added, skipped, no_value, deprecated)
        return tabs

    #---------------------------------------------------------------------------
    def option_alt_names(self, name):
        names = [name.replace('-', '_'), name.replace('_', '-')]
        if name not in names:
            names.insert(0, name)
        return names

    #---------------------------------------------------------------------------
    def get_option_def(self, name):
        # self.opt_rindex stores dict of the following format: <opt_name> -> (opt_def, tabname, groupname)
        odef = None
        option = self.opt_rindex.get(name) # We're fine with alt names as all were stored
        if option is not None:
            odef = option[0]
        return odef

    #---------------------------------------------------------------------------
    def get_option_location(self, name):
        # self.opt_rindex stores dict of the following format: <opt_name> -> (opt_def, tabname, groupname)
        # We are fine here with alias names as opt_rindex has all possible names
        return (self.opt_rindex.get(name, (None, None, None)))[1:]

    #---------------------------------------------------------------------------
    def get_options_containing(self, fragment):
        # We are fine here with alias names as opt_rindex has all possible names
        return [name for name in  self.opt_rindex.keys() if fragment in name]

    #---------------------------------------------------------------------------
    def normalize_bool(self, value):
        ret = False

        if type(value) == bool:
            ret = value
        else:
            if value and (type(value) is str or type(value) is unicode):
                value = value.lower()

            if value == 'checked' or value == "on" or value == "true" or value == "1":
                ret = True
            elif value == 'unchecked' or value == "off" or value == "false" or value == "" or value == "0":
                ret = False

        return ret

    #---------------------------------------------------------------------------
    def get_option_type_and_default(self, name):
        option_def = self.get_option_def(name) # Safe to ignore alt names rindex has all
        option_type    = None
        option_default = None

        if option_def is not None:
            if 'type' in option_def:
                option_type    = option_def['type']
                if option_type:
                    option_type = option_type.lower()
            if 'default' in option_def:
                option_default = option_def['default']
                if option_default and type(option_default) is not bool:
                    option_default = option_default.lower()

            if option_type == 'boolean':
                option_default = self.normalize_bool(option_default)

        return (option_type, option_default)

    #---------------------------------------------------------------------------
    def open_configuration_file(self, file_name, warn_missing = True):
        self.file_name = file_name

        self.needs_root_for_file_read = False
        try:
            open(file_name)
        except IOError, error:
            if error.errno == errno.EACCES:
                self.needs_root_for_file_read = True
        except:
            pass

        content = []
        exception = None
        try:
            content = self.read_mysql_cfg_file(file_name)
        except (OSError, IOError), e:
            if e.errno != errno.ENOENT:
                exception = e
            else:
                if warn_missing:
                    Utilities.show_warning("Read Configuration File",
                                           "Configuration file '%s' can not be found. New file will be created when changes are applied."%file_name,
                                          "OK", "", "")
                else:
                    return

        self.parse_file_contents(content)

        if exception:
            raise e

    #---------------------------------------------------------------------------
    def read_mysql_cfg_file(self, file_name):
        # read_mysql_cfg_file fetches file in case of remote file
        log_debug('Reading config file "%s"\n' % file_name)
        content = None
        if not self.needs_root_for_file_read:
            try:
                log_debug('Trying to read without sudo\n')
                content = self.ctrl_be.server_helper.get_file_content(file_name, as_user=Users.CURRENT, user_password=None)
                log_debug('%i bytes read from file\n' % len(content or []) )
            except PermissionDeniedError, e:
                log_debug('Permissin denied; sudo needed to read config file: "%r"\n' % e)
                self.needs_root_for_file_read = True

        if self.needs_root_for_file_read:
            log_debug('Trying to read with sudo\n')
            user_password = self.ctrl_be.password_handler.get_password_for("file")
            if user_password is not None:
              as_user = Users.ADMIN
            try:
                log_debug('Reading...\n')
                content = self.ctrl_be.server_helper.get_file_content(file_name, as_user, user_password)
            except InvalidPasswordError, err:
                log_debug('Invalid password error: "%r"\n' % err)
                self.ctrl_be.password_handler.reset_password_for("file")
                raise err

        log_debug('Config file read\n')

        # break up the contents into lines
        if content:
            return [line.rstrip("\r\n") for line in content.split("\n")]
        else:
            return []

    #---------------------------------------------------------------------------
    def save_config_file(self, user_modified_file_content):
        if not user_modified_file_content:
            user_modified_file_content = ""

        # convert line endings for windows
        if self.server_profile.target_is_windows:
            user_modified_file_content = "\r\n".join(user_modified_file_content.split("\n"))

        helper = self.ctrl_be.server_helper

        # split the path for / and \\
        directory, filename = splitpath(self.file_name)

        first_try = True
        while True:
            if not helper.check_path_exists(directory):
                helper.create_directory(directory)
            if helper.check_dir_writable(directory):
                password = None
                as_user = Users.CURRENT
            else:
                password = self.ctrl_be.password_handler.get_password_for("file", cached_only=first_try)
                as_user = Users.ADMIN

            try:
                helper.set_file_content_and_backup(self.file_name, user_modified_file_content, ".wba.bak", as_user, password)
            except InvalidPasswordError, err:
                self.ctrl_be.password_handler.reset_password_for("file")
                if first_try:
                    first_try = False
                    continue
                raise err
            break

        # read back the saved file
        data = self.read_mysql_cfg_file(self.file_name)
        self.parse_file_contents(data)



    #---------------------------------------------------------------------------
    # unused code
    #def process_include_directive(self, opt_ln_with_include):
    #  #we can have either
    #  tokens = opt_ln_with_include.split(" ")
    #  directive = tokens[0].lower()
    #  tail = " ".join(tokens[1:])
    #  if directive == "!include":
    #    self.parse_file(tail)
    #  elif directive == "!includedir":
    #    files = os.listdir(tail)
    #    for file in files:
    #      if file[-4:] == ".cnf" or file[-4:] == ".ini":
    #        self.parse_file(os.path.join(tail,file))

    #---------------------------------------------------------------------------
    def parse_file_contents(self, file_data = None):
        # Reset changes which could be made to the previously loaded config
        self.changeset = {}

        if type(file_data) is list or type(file_data) is tuple:
            self.file_lines = file_data
        else:
            raise Exception("Internal error. File data passed is not in expected format. This is a bug, we would greatly appreciate if you file a bug report at http://bugs.mysql.com.")

        filter_by_section = self.server_profile.config_file_section
        if not filter_by_section:
            filter_by_section = default_section

        log_debug('Parsing options only from section "%s"\n' % filter_by_section)

        self.sections = []
        if self.file_name is not None:
            cur_file_original_opts = {}

            current_section = ""
            for i,line in enumerate(self.file_lines):
                sline = line.strip(" \r\n\t")
                # Skip empty and commented out lines
                if len(sline) > 0 and ((sline[0] is not '#') and (sline[0] is not ';')):
                # Got section start line
                    if sline[0] == '[':
                        current_section = sline.strip("[]")
                        # Add sections as we go, having list of section names and its start lines
                        # we can compute section lines range to add config values later
                        self.sections.append((i, current_section))
                    elif sline.lower().find("!include") == 0:
                        # Currently we skip include and includedir directives
                        pass
                    else:
                        # Split line into option name and option value
                        pos = sline.find("=")
                        has_value = True
                        if pos == -1:
                            pos = len(sline)
                            has_value = False

                        if current_section == filter_by_section:
                            option_name = sline[:pos].strip()

                            option = None
                            # Get existing option. We handle all options as multiline.
                            # TODO: If we ever have an indicator what options can be multiline
                            #       we can issue warnings about duplicated option values
                            #       until that moment all duplicated options will be displayed as
                            #       multiline
                            if option_name in cur_file_original_opts:
                                option = cur_file_original_opts[option_name]
                            else:
                                option = Option(current_section)
                                cur_file_original_opts[option_name] = option

                            # i is a line number at which the option is in the cfg file
                            # Form option tuple of form (section, line, value)
                            # At this stage we do not validate options against supported set.
                            # Some sort of validation is performed when loading options to UI,
                            # unsupported options will not be displayed and they are left
                            # unaltered in the file.
                            value = sline[pos+1:]
                            if has_value:
                                option.append(i, value)
                            else:
                                option.append(i, True)

                            if option_name.startswith("skip-"):
                                odef = self.get_option_def(option_name[5:])
                                if odef and odef.get('disabledby') == option_name:
                                    option = Option(current_section, i, 'disabledby')
                                    cur_file_original_opts[option_name[5:]] = option
                                    cur_file_original_opts[option_name] = option
                            else:
                                odef = self.get_option_def(option_name)
                                if odef is not None and odef.get('type') == 'boolean':
                                    # since we remove one of redundant option from pairs like 'option_name' and 'skip-option_name'
                                    # we need to take care of options that exists in config file but not in UI
                                    # so if option is bool and has no 'skip-' in name we put additional option 'skip-option_name'
                                    # to cur_file_original_opts list with opposite value to properly handle every options 
                                    if has_value:
                                        value = not self.normalize_bool(value)
                                    else:
                                        value = False
                                    option = Option(current_section, i, value)
                                    cur_file_original_opts['skip-'+option_name] = option

            self.original_opts = cur_file_original_opts
            self.sections = sorted(self.sections, lambda x,y: cmp(x[0], y[0]))

        section = self.server_profile.config_file_section

        # Sets a default section in case it is empty
        if section == "":
            section = default_section

        # Check if the wanted section is in the file, if not, add it
        if self.server_profile.admin_enabled and not any(_section == section for _line, _section in self.sections):
            Utilities.show_warning("Read Configuration File",
                "Configuration file did not contain section [%s], so a new one was added.\nIf that is not correct, please fix the section name in the Server Instance Editor and reopen the administrator."%section,
                "OK", "", "")

            #if len(self.sections) == 0 and len(self.file_lines) == 0:
            self.sections.append((len(self.file_lines), section))
            self.file_lines.append("[" + section + "]\n")



    #---------------------------------------------------------------------------
    # This is called from ui when option addition is detected.
    def option_added(self, name, value, section):
        log_debug('Adding option: "%s", "%r", "%s"\n' % (name, value, section) )
        if value is None:
            value = True

        if section is None:
            section = self.server_profile.config_file_section if self.server_profile.config_file_section is not '' else default_section

        odef = self.get_option_def(name)
        option_type    = odef.get('type')
        orig_opt = self.original_opts.get(name)
        disabledby = odef.get('disabledby')

        if option_type == 'boolean':
            option_default = odef.get('default')
            log_debug3('adding boolean option "%s" with value = "%r", default = "%r", orig_opt = "%s", disabledby = "%r", definition = "%s"\n' % (name, value, option_default, orig_opt, disabledby, odef) )

            if disabledby is not None:
                if disabledby in self.original_opts:
                    ci = WbAdminConfigFileBE.ChangesetItem(DELETE, section, name, None)
                    ci.orig_opt = self.original_opts[disabledby]
                    self.changeset[name] = ci
                    log_debug3('added delete ci for "%s"\n' % name)

            if orig_opt is None:
                if name in self.changeset:
                    del self.changeset[name]
                    log_debug3('Removing existing changeset for "%s"\n' % name)

                if option_default != True:
                    #on_value = odef.get('on')
                    #if on_value == 'name':
                    #on_value = None
                    #ci = WbAdminConfigFileBE.ChangesetItem(ADD, section, name, (None if on_value == 'name' else on_value, ))
                    ci = WbAdminConfigFileBE.ChangesetItem(ADD, section, name, (None,))
                    log_debug3('Adding ci=ADD for "%s" with value "%s"\n' % (name, str(ci.value)) )
                    self.changeset[name] = ci
            else:
                orig_opt_value = self.normalize_bool(orig_opt.val(0))
                if orig_opt_value == False:
                    #on_value = odef.get('on')
                    #ci = WbAdminConfigFileBE.ChangesetItem(CHANGE, section, name, (None if on_value == 'name' else on_value, ))
                    ci = WbAdminConfigFileBE.ChangesetItem(CHANGE, section, name, (None,))
                    ci.orig_opt = orig_opt
                    self.changeset[name] = ci
        else:
            # We may have user unclicked and clicked an existing option, so that
            # results in unneeded DELETE, ADD sequence of actions. So first we check
            # if the option being added was not previously deleted
            if name in self.changeset:
                existing_ci = self.changeset[name]

                if existing_ci.section == section:
                    if existing_ci.mod == DELETE:
                        del self.changeset[name]
            else:
                ci = WbAdminConfigFileBE.ChangesetItem(ADD, section, name, value)

                if self.original_opts.has_key(name) and self.original_opts[name].section == section:
                    opt = self.original_opts[name]
                    ci.mod = CHANGE
                    ci.orig_opt = opt

                self.changeset[name] = ci

    #---------------------------------------------------------------------------
    def option_removed(self, name, section):
        log_debug('opt removed "%s"\n' % name)
        # Get option default value to detect special actions to take if option is
        # bool and its default is true. That means that we must not remove option
        # from the config file, but rather change it to <name> = FALSE
        #(option_type, option_default) = self.get_option_type_and_default(name)

        eci = self.changeset.get(name)
        if eci and eci.mod == ADD:
            del self.changeset[name]
            return

        odef = self.get_option_def(name)
        option_type = odef.get('type')
        if option_type == 'boolean':
            disabledby = odef.get('disabledby')
            option_default = odef.get('default')

            ci = None
            orig_opt = self.original_opts.get(name)
            off_value = odef.get("off")

            log_debug3('removing "%s", orig_opt = "%s", off_value = "%r", odef = "%s"\n' % (name, orig_opt, off_value, odef) )

            if orig_opt is None:
                if off_value == 'disabledby':
                    if disabledby is None:
                        print "Error, option definition does not have disabledby"
                    else:
                        if disabledby not in self.original_opts:
                            ci = WbAdminConfigFileBE.ChangesetItem(ADD, section, disabledby, None)
                            self.changeset[disabledby] = ci
                else:
                    if option_default == True:
                        ci = WbAdminConfigFileBE.ChangesetItem(ADD, section, name, off_value)
                        self.changeset[name] = ci
            else:
                print "got orig opt"
                #orig_value = orig_opt.val(0)

                if off_value == 'disabledby':
                    if disabledby is None:
                        print "Error, option definition does not have disbledby"
                    else:
                        ci = WbAdminConfigFileBE.ChangesetItem(DELETE, section, name, "")
                        ci.orig_opt = self.original_opts.get(name)
                        self.changeset[name] = ci
                        if disabledby not in self.original_opts:
                            ci = WbAdminConfigFileBE.ChangesetItem(ADD, section, disabledby, None)
                            self.changeset[disabledby] = ci
                elif off_value == 'del':
                    ci = WbAdminConfigFileBE.ChangesetItem(DELETE, section, name, "")
                    ci.orig_opt = self.original_opts.get(name)
                    self.changeset[name] = ci
                else:
                    if option_default != True:
                        ci = WbAdminConfigFileBE.ChangesetItem(DELETE, section, name, off_value)
                    else:
                        ci = WbAdminConfigFileBE.ChangesetItem(CHANGE, section, name, off_value)

                    ci.orig_opt = self.original_opts.get(name)
                    self.changeset[name] = ci
                #print self.changeset
        else:
            if self.original_opts.has_key(name):
                value = ""
                ci = WbAdminConfigFileBE.ChangesetItem(DELETE, section, name, value)
                ci.orig_opt = self.original_opts[name]
                self.changeset[name] = ci
            else:
                # Here we handle a situation when boolean option has default value = true and is not present in config file
                # In this case option is ticked in the ui, so user unticking it must result in adding
                # <option_name> = FALSE to the config file.
                if self.changeset.has_key(name):
                    ci = self.changeset[name]
                    if ci.section == section:
                        del self.changeset[name]

    #---------------------------------------------------------------------------
    def option_changed(self, name, value, section):
        log_debug('Option changed: "%s", "%r"\n' % (name, value) )

        if type(value) is not tuple:
            print "Warning setting option", name, "from non-tuple value", value

        # If we have change for the option recorded in the changeset, we simply overwrite
        # changed value with a new one, otherwise we add a new value to changeset
        if self.original_opts.has_key(name):
            opt = self.original_opts[name]
            ci = WbAdminConfigFileBE.ChangesetItem(CHANGE, section, name, value)
            ci.orig_opt = opt
            self.changeset[name] = ci
        else:
            self.changeset[name] = WbAdminConfigFileBE.ChangesetItem(ADD, section, name, value)

    #---------------------------------------------------------------------------
    def get_options(self, section):
        options = []
        for (name, opt) in self.original_opts.iteritems():
            name = name.strip()
            if opt.section == section:
                if self.changeset.has_key(name):
                    options.append((name, self.changeset[name].value))
                else:
                    odef = self.get_option_def(name)
                    if odef is None and name.startswith("skip-"):
                        odef = self.get_option_def(name[5:])
                    if odef is not None and odef.get('type') == 'boolean':
                        ovalue = str(opt)
                        if ovalue == 'disabledby':
                            ovalue = "0"
                        options.append((name, ovalue))
                    else:
                        if hasattr(opt.value(), "strip"):
                            options.append((name, opt.value().strip()))
                        else:
                            options.append((name, opt.value()))

        return options

    #---------------------------------------------------------------------------
    def get_sections(self):
        return [x[1] for x in self.sections]

    #---------------------------------------------------------------------------
    def get_section_line_nr_range(self, section_name):
        ret = [-1,-1]
        sections_nr = len(self.sections) - 1
        for i,sec in enumerate(self.sections):
            if sec[1] == section_name:
                ret[0] = sec[0]
                if i < sections_nr:
                    ret[1] = self.sections[i + 1][0] - 1
                else:
                    ret[1] = len(self.file_lines)

        return ret

    #---------------------------------------------------------------------------
    def validate_changes(self, options):
        option_types = {}
        for item in options.itervalues():
            for group in item["groups"]:
                for control in group["controls"]:
                    option_types[control["name"]] = control["type"]

        errors = ""
        for change in self.changeset.itervalues():
            otype = option_types.get(change.name)
            if not otype: continue
            if change.mod in (CHANGE, ADD):
                if otype == "filename":
                    if change.value == "":
                        errors += "Option '%s' is blank, but should be a path\n" % change.name
        return errors


    #---------------------------------------------------------------------------
    def apply_changes(self):
        if self.file_name is None:
            return

        # Build sections map. self.sections holds tuples of form (section_name, section first line number)
        # self.sections is sorted by line numbers
        # This map of sections is needed to sort changeset items. We sort them in the following order:
        # CHANGE first as it does not require line to be added or deleted. Next is REMOVE action. REMOVE action
        # is replacement of the line in file with and empty one. And the last goes ADD action. As we need to
        # calculate line number to insert we need to insert from bottom to top, so we need an additional sort
        # criteria - sections order. That means that when sorting two ADD actions the first action in the sorted changelist
        # will be the one which is in the lower section
        sections_map = dict([(x[1],self.get_section_line_nr_range(x[1])) for x in self.sections])

        def map_bool(v):
            rv = v
            if type(v) is bool:
                rv = int(v)
            return rv

        def sort_fn(x,y):
            r = cmp(x.mod,y.mod)
            if x.mod == ADD and r == 0:
                r = cmp(sections_map[y.section][0], sections_map[x.section][0])
            return r

        change = sorted(self.changeset.itervalues(), sort_fn)
        second_pass_changes = []

        # Now we have sorted items (CHANGE, DEL, ADD). This is the order we will apply changes
        file_lines = copy.deepcopy(self.file_lines)
        for c in change:
            (option_type, option_default) = self.get_option_type_and_default(c.name)

            # Walk values and change boolean to 0/1
            if type(c.value) is list or type(c.value) is tuple:
                c.value = map(map_bool, c.value)
            else:
                c.value = map_bool(c.value)

            if c.mod == CHANGE:
                log_debug('Applying change "%r"\n' % c)
                # Here comes the hard part
                orig_values_len     = len(c.orig_opt)
                modified_values_len = len(c.value)
                # First we apply changes of multiple options, so we walk the least common part of both lists
                # say we have 3 orignal options and 4 modified, so we will apply changes to the first three
                # and ADD a new one after that
                for i in range(min(orig_values_len, modified_values_len)):
                    # Orig opt is list of tupples with a format of (linenr, old_value)
                    line_nr = c.orig_opt.line(i)
                    log_debug2('line_nr "%r", "%r"\n' % (line_nr, c.orig_opt) )
                    rvalue = "\n"
                    if (c.value[i] is not None):
                        rvalue = " = " + str(c.value[i]) + "\n"
                    file_lines[line_nr] = c.name + rvalue
                # Below are two branches which schedule items for second pass. At the current - first
                # pass we can not change line number via adding or removing lines
                if orig_values_len < modified_values_len:
                    # Add options here
                    for i in range(orig_values_len, modified_values_len):
                        ci = WbAdminConfigFileBE.ChangesetItem(ADD, c.section, c.name, c.value[i])
                        second_pass_changes.append(ci)
                elif orig_values_len > modified_values_len:
                    #Remove options here
                    for i in range(modified_values_len, orig_values_len):
                        file_lines[c.orig_opt.line(i)] = ""
                        ci = WbAdminConfigFileBE.ChangesetItem(DELETE, c.section, c.name, c.orig_opt.val(i))
                        second_pass_changes.append(ci)
            elif c.mod == DELETE:
                # In c.orig_opt[1] we have a line number for single-lien options, or a string 'Multiple' for multi-line ones
                for line, value in c.orig_opt:
                    if option_type == 'boolean' and option_default == True:
                        file_lines[line] = c.name + " = 0\n"
                    else:
                        file_lines[line] = ""
            elif c.mod == ADD:
                lines_range = sections_map.get(c.section)
                if lines_range and lines_range[1] >= 0:
                    if type(c.value) is list or type(c.value) is tuple:
                        lineno = lines_range[1]
                        for v in c.value:
                            if v is not None:
                                file_lines.insert(lineno, c.name + " = " + v.strip(" ") + "\n")
                            else:
                                file_lines.insert(lineno, c.name + "\n")
                            lineno += 1
                    else:
                        if c.value is not None:
                            file_lines.insert(lines_range[1], c.name + " = " + c.value + "\n")
                        else:
                            file_lines.insert(lines_range[1], c.name + "\n")
                else:
                    print "Can't add option"

        # handle only addition for now, as change and delete can be done in place earlier
        for c in second_pass_changes:
            if c.mod == ADD:
                lines_range = sections_map[c.section]
                if lines_range[1] >= 0:
                    vtype = type(c.value)
                    if vtype is str or vtype is unicode:
                        file_lines.insert(lines_range[1], c.name + " = " + c.value + "\n")
                    else:
                        file_lines.insert(lines_range[1], c.name + " = " + c.value + "\n")
                else:
                    print "Can't add option"

        tempdir = tempfile.gettempdir()
        self.temp_file_name = os.path.join(tempdir, "mysql_workbench_config.temp")
        outf = open(self.temp_file_name, "w+b")
        eol = "\n"
        if self.server_profile.target_is_windows:
            eol = "\r\n"

        for line in file_lines:
            l = line.rstrip("\r\n") + eol
            outf.write(l)
        outf.close()

        # Prepare data for ApplyWizard
        self.apply_form = ApplyWizard(self, self.ctrl_be, self.server_profile)

        changes_for_apply = [(ci.mod, ci.section, ci.name, ci.value) for ci in change]
        second_pass_changes_for_apply = [(ci.mod, ci.section, ci.name, ci.value) for ci in second_pass_changes]
        changes_for_apply += second_pass_changes_for_apply

        changes_text = ""
        for d in difflib.unified_diff(a = self.file_lines, b = file_lines, n = 0):
            changes_text += d if d.endswith('\n') else d + '\n'
        if not changes_text:
            changes_text = "There are no changes."

        temp_file_content = ""
        try:
            outf = open(self.temp_file_name, "r")
            temp_file_content = outf.read()
            outf.close()
        except BaseException, e:
            temp_file_content = "Can not read file " + self.temp_file_name + "\n" + str(e)

        self.apply_form.show(changes_text, temp_file_content, self.save_config_file)

    #---------------------------------------------------------------------------
    def revert(self):
        try:
            # re-read the file
            data = self.read_mysql_cfg_file(self.file_name)
            self.parse_file_contents(data)
        except Exception, exc:
            Utilities.show_error("Could not Re-read configuration file",
                    "An error occurred while reading %s:\n%s" % (self.file_name, exc),
                    "OK", "", "")


#===============================================================================
#-------------------------------------------------------------------------------
def recreate_cfg_with(ctx, lines):
    pass

#-------------------------------------------------------------------------------
def init_cfg_be(ctx):
    cfg_be = ctx['cfg_be']
    settings = ctx['settings']

    opts = None
    if 'opts' in ctx:
        opts = ctx['opts']
    else:
        opts = __import__('opts')
        if opts:
            ctx['opts'] = opts

    section = settings.serverInfo['sys.config.section']
    cfg_be.sections.append((0, section))
    cfg_be.file_lines.append("[" + section + "]\n")


#-------------------------------------------------------------------------------
def unit_test_0(ctx):
    def show(self, changes_text, temp_file_content, save_config_file):
        save_config_file(temp_file_content)
    ctx['saved_show'] = ApplyWizard.show
    ApplyWizard.show = show

    ctx['cfg_be'] = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])

#-------------------------------------------------------------------------------
def unit_test_1(ctx):
    name = (__name__, "Checking clean config be")
    msg = ""

    cfg_be = ctx['cfg_be']

    check = len(cfg_be.get_options('mysqld')) == 0
    if not check:
        msg = "Clean config be has non-zero number of stored options."

    return (name, check, msg)

#-------------------------------------------------------------------------------
def unit_test_2(ctx):
    name = (__name__, "Checking normalize_bool")
    status = True
    msg = ""
    cfg_be = ctx['cfg_be']

    values = ('checked', "on", "true")
    for v in values:
        nv = cfg_be.normalize_bool(v)
        if nv != True:
            msg += "Failed norm to True from '" + v + "'\n"
            status = False

    values = ('unchecked', "off", "false", "")
    for v in values:
        nv = cfg_be.normalize_bool(v)
        if nv != False:
            msg += "Failed norm to False from '" + v + "'\n"
            status = False

    cfg_be = ctx['cfg_be']
    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_3(ctx):
    name = (__name__, "Checking load of values from config file.")
    status = True
    msg = ""
    settings = ctx['settings']
    cfg_be = ctx['cfg_be']

    cfgfile = os.path.join(os.getcwd(), "test-files/wb_admin_config_file_be3.cnf")
    settings.serverInfo['sys.config.path'] = cfgfile

    cfg_be.parse_file(cfgfile)
    test_vector = {'data':"'/usr/lib'", 'port':'3306'}
    for (opt, value) in cfg_be.get_options('mysqld'):
        if opt in test_vector:
            if value != test_vector[opt]:
                status = False
            del test_vector[opt]

    if len(test_vector) > 0:
        status = False
        msg = "Values " + ",".join(test_vector.keys()) + " were not parsed from config " + cfgfile

    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_4(ctx):
    name = (__name__, "Working with simple options.")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be4.cnf")

    # recreate empty config be
    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    cfg_be.file_name = cfgfile
    init_cfg_be(ctx)

    # closure to verify written data
    def verify_option(name, value, message):
        status = True
        msg = ""
        # Test read and parse
        ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
        init_cfg_be(ctx)
        cfg_be.file_name = cfgfile
        cfg_be.parse_file()
        options = dict(cfg_be.get_options('mysqld'))
        if not (name in options and options[name] == value):
            status = False
            msg = message
        return (status, msg)

    # 1. Adding option
    cfg_be.option_added('port', ('3307',), 'mysqld')
    cfg_be.apply_changes()
    (curstatus, message) = verify_option('port', '3307', "Adding option failed\n")
    if not curstatus:
        status = False
        msg += message

    # TODO: Check that section was correct
    # 2. Changing option
    cfg_be.option_changed('port', ('3308',), 'mysqld')
    cfg_be.apply_changes()
    (curstatus, message) = verify_option('port', '3308', "Changing option failed\n")
    if not curstatus:
        status = False
        msg += message

    cfg_be.option_removed('port', 'mysqld')
    cfg_be.apply_changes()
    if len(cfg_be.get_options('mysqld')) > 0:
        status = False
        msg += "Option remove failed"

    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_5(ctx):
    name = (__name__, "Removing boolean with default true and disabledby (opt not in file)")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be5.cnf")

    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    init_cfg_be(ctx)
    cfg_be.file_name = cfgfile
    cfg_be.parse_file()
    cfg_be.option_removed('partition', 'mysqld')
    cfg_be.apply_changes()
    cfg_be.parse_file()
    if 'skip-partition' not in cfg_be.original_opts:
        status = False
        msg += "skip-partition was not added on disable"

    if 'partition' not in cfg_be.original_opts:
        status = False
        msg += "partition was not added. Check parser"
    else:
        val = cfg_be.original_opts.get('partition')
        value = val.val(0)
        if value != "disabledby":
            status = False
            msg += "Broken parser. Partition must have value 'disabledby' when skip-partition is used!"

    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_6(ctx):
    name = (__name__, "Removing boolean with default true (opt not in file)")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be6.cnf")

    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    init_cfg_be(ctx)
    cfg_be.file_name = cfgfile
    cfg_be.parse_file()
    cfg_be.option_removed('temp-pool', 'mysqld')
    cfg_be.apply_changes()
    cfg_be.parse_file()

    opt = cfg_be.original_opts.get('temp-pool')
    if opt is None:
        status = False
        msg += "temp-pool is not in file, or was not parsed."
    else:
        odef = cfg_be.get_option_def('temp-pool')
        if opt.val(0) != odef['off']:
            status = False
            msg += "Value written in file is not the same as the off value in definition. " + str(opt) + ", " + odef['off']

    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_7(ctx):
    name = (__name__, "Switching off boolean with default false (opt not in file)")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be7.cnf")

    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    init_cfg_be(ctx)
    cfg_be.file_name = cfgfile
    cfg_be.parse_file()
    cfg_be.option_removed('core-file', 'mysqld')
    cfg_be.apply_changes()
    cfg_be.parse_file()

    opt = cfg_be.original_opts.get('core-file')
    if opt is not None:
        status = False
        msg += "Bool option with default false was added to file on turn-off"

    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_8(ctx):
    name = (__name__, "Switching off boolean with default false (opt in file)")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be8.cnf")

    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    init_cfg_be(ctx)
    cfg_be.file_name = cfgfile
    cfg_be.parse_file()

    cfg_be.option_added('core-file', ("1",), 'mysqld')
    cfg_be.apply_changes()
    cfg_be.parse_file()
    opt = cfg_be.original_opts.get('core-file')
    if opt is None:
        status = False
        msg += "Failed to add option for further remove test"
    else:
        if cfg_be.normalize_bool(opt.val(0)) == False:
            status = False
            msg += "Value has no on value"
        else:
            cfg_be.option_removed('core-file', 'mysqld')
            ci = cfg_be.changeset.get('core-file')
            if ci.mod != DELETE:
                status = False
                msg += "Removing opt with default = false should remove option from file"
            cfg_be.apply_changes()
            cfg_be.parse_file()
            if len(cfg_be.original_opts) > 0:
                status = False
                msg += "Option was not removed from file"

    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_9(ctx):
    name = (__name__, "Removing boolean with default false and disabledby (opt in file)")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be9.cnf")

    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    init_cfg_be(ctx)
    cfg_be.file_name = cfgfile
    cfg_be.parse_file()
    cfg_be.option_added('new', ('1',), 'mysqld')
    cfg_be.apply_changes()
    cfg_be.parse_file()

    opt = cfg_be.original_opts.get('new')
    if opt is None:
        status = False
        msg += "Failed to add option for further remove test"
    else:
        if cfg_be.normalize_bool(opt.val(0)) == False:
            status = False
            msg += "Value has no on value"
        else:
            cfg_be.option_removed('new', 'mysqld')
            ci = cfg_be.changeset.get('new')
            if ci.mod != DELETE:
                status = False
                msg += "Removing opt 'new' with disabledby=skip-new should remove option from file"
            ci = cfg_be.changeset.get('skip-new')
            if ci.mod != ADD:
                status = False
                msg += "Removing opt 'new' with disabledby=skip-new should add skip-new option to file"

            cfg_be.apply_changes()
            cfg_be.parse_file()

            new = cfg_be.original_opts.get('new')
            skip_new = cfg_be.original_opts.get('skip-new')
            if new is None:
                status = False
                msg += "new is not added to original_opts to turn off checkbox in ui"
            else:
                if new.val(0) != 'disabledby':
                    status = False
                    msg += "new with value = disabledby was not added to original_opts to indicate presense of skip-new"
            if skip_new is None:
                status = False
                msg += "skip-new is not added to original_opts after apply_changes"

    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_10(ctx):
    name = (__name__, "Checking turn on of special skip option (skip-networking)")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be10.cnf")

    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    init_cfg_be(ctx)
    cfg_be.file_name = cfgfile
    cfg_be.parse_file()
    cfg_be.option_added('skip-networking', ('1',), 'mysqld')
    cfg_be.apply_changes()
    cfg_be.parse_file()

    found = False
    for line in cfg_be.file_lines:
        if "skip-networking" == line.strip(" \r\t\n"):
            found = True
            break

    if not found:
        status = False
        msg += "skip-networking was written wrongly"

    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_11(ctx):
    name = (__name__, "Checking turn on bool opt = false (def != false, on=name) (opt in file)")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be11.cnf")

    f = open(cfgfile, "w")
    f.write("[mysqld]\n")
    f.write("enable-named-pipe = 0\n")
    f.close()

    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    init_cfg_be(ctx)
    cfg_be.file_name = cfgfile
    cfg_be.parse_file()
    cfg_be.option_added('enable-named-pipe', ("1",), 'mysqld')
    cfg_be.apply_changes()

    cfg_be.parse_file()
    if True != cfg_be.original_opts['enable-named-pipe'].val(0):
        status = False
        msg += "Value is not True"

    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_12(ctx):
    name = (__name__, "Checking turn on bool opt with disabledby=skip-* (opt in file)")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be12.cnf")

    f = open(cfgfile, "w")
    f.write("[mysqld]\n")
    f.write("skip-new\n")
    f.close()

    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    init_cfg_be(ctx)
    cfg_be.file_name = cfgfile
    cfg_be.parse_file()
    cfg_be.option_added('new', ("1",), 'mysqld')
    cfg_be.apply_changes()

    cfg_be.parse_file()
    if True != cfg_be.original_opts['new'].val(0):
        status = False
        msg += "merge is not set."

    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_13(ctx):
    name = (__name__, "Disabling innodb")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be13.cnf")

    f = open(cfgfile, "w")
    f.write("[mysqld]\n")
    f.close()
    ctrl_be = ctx['ctrl_be']
    version = ctrl_be.version
    ctrl_be.version = (5,1,16)

    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    init_cfg_be(ctx)
    cfg_be.file_name = cfgfile
    cfg_be.parse_file()

    cfg_be.option_removed('innodb', 'mysqld')
    cfg_be.apply_changes()

    cfg_be.parse_file()
    if None != cfg_be.original_opts.get('innodb'):
        status = False
        msg += "innodb option appeared in file after removal"

    ctrl_be.version = version
    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_13_(ctx):
    name = (__name__, "Disabling innodb")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be13.cnf")

    f = open(cfgfile, "w")
    f.write("[mysqld]\n")
    f.close()
    ctrl_be = ctx['ctrl_be']
    version = ctrl_be.version
    ctrl_be.version = (5,1,16)

    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    init_cfg_be(ctx)
    cfg_be.file_name = cfgfile
    cfg_be.parse_file()

    cfg_be.option_removed('innodb', 'mysqld')
    cfg_be.apply_changes()

    cfg_be.parse_file()
    if None != cfg_be.original_opts.get('innodb'):
        status = False
        msg += "innodb option appeared in file after removal"

    ctrl_be.version = version
    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_14(ctx):
    name = (__name__, "Disabling innodb. File has bool innodb; WBA uses enum innodb")
    status = True
    msg = ""
    settings = ctx['settings']
    settings.serverInfo['sys.config.path'] = cfgfile = os.path.join(os.getcwd(), "tmp/wb_admin_config_file_be14.cnf")

    f = open(cfgfile, "w")
    f.write("[mysqld]\ninnodb\n")
    f.close()
    ctrl_be = ctx['ctrl_be']
    version = ctrl_be.version
    ctrl_be.version = (5,4,2)

    ctx['cfg_be'] = cfg_be = WbAdminConfigFileBE(ServerProfile(ctx['settings']), ctx['ctrl_be'])
    init_cfg_be(ctx)
    cfg_be.file_name = cfgfile
    cfg_be.parse_file()

    cfg_be.option_changed('innodb', ('OFF',), 'mysqld')
    cfg_be.apply_changes()

    cfg_be.parse_file()
    innodb = cfg_be.original_opts.get('innodb')
    print cfg_be.original_opts
    if None == innodb:
        status = False
        msg += "innodb option disappeared after change"
    else:
        if 'OFF' != innodb.val(0):
            status = False
            msg += "innodb option was not changed to OFF"

    ctrl_be.version = version
    return (name, status, msg)

#-------------------------------------------------------------------------------
def unit_test_10000(ctx):
    ApplyWizard.show = ctx['saved_show']
    if 'cfg_be' in ctx:
        del ctx['cfg_be']
