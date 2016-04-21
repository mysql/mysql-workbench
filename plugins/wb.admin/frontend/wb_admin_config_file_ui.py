# Copyright (c) 2007, 2016, Oracle and/or its affiliates. All rights reserved.
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
import copy

from mforms import newTabView, newBox, newTable, newPanel, TitledBoxPanel, newScrollPanel, newCheckBox, newTextEntry, newTextBox
from mforms import newLabel, HFillFlag, VFillFlag, HExpandFlag, Utilities, newSelector
from mforms import newButton, SmallHelpTextStyle, FileChooser, OpenDirectory, OpenFile, SaveFile
from mforms import SelectorPopup, SelectorCombobox, Form, newImageBox
import mforms

import wb_admin_config_file_be
from wb_common import dprint_ex, debug_level, PermissionDeniedError, InvalidPasswordError, OperationCancelledError, Users
from wb_admin_config_file_be import multi_separator
from wb_admin_utils import make_panel_header

from workbench.utils import server_os_path

#from grt.modules.WBAdmin import openRemoteFileSelector

CATOPTS = os.getenv("WB_CATOPTS")
if CATOPTS is not None:
    cat_sec = ('General', 'Advanced', 'MyISAM Parameters', 'Performance', 'Logging', 'Security', 'InnoDB Parameters', 'Networking', 'Replication')
    cat_grp = ('Networking', 'Advanced log options', 'Slave replication objects', 'Slave default connection values', 'Activate Logging', 'Memory', 'Fulltext search', 'Data / Memory size', 'Datafiles', 'Localization', 'Thread specific settings', 'Advanced', 'Advanced Settings', 'Various', 'Binlog Options', 'Memory usage', 'Directories', 'Logfiles', 'Relay Log', 'Master', 'General slave', 'Security', 'Activate InnoDB', 'Slave Identification', 'Query cache', 'General', 'Insert delayed settings', 'Slow query log options', 'Naming', 'Timeout Settings')

    def handle_cat_opt(cat, grp, enabled):
        print "CATOPT", "'" + enabled + "', '" + cat.get_string_value() + "', '" + grp.get_string_value()

# The list of versions to show to user when detected/given version is not supported
supported_versions = [5.0, 5.1, 5.5, 5.6, 5.7]

#===============================================================================
def verify_selected_version(version_selector, set_back):
    ver = str(version_selector.get_string_value())
    new_ver = ""
    if type(ver) is str:
        for vch in ver:
            if vch.isdigit() or vch == '.':
                new_ver += vch

    if set_back and new_ver != ver:
        version_selector.set_value(new_ver)

    return new_ver

#===============================================================================
def run_version_select_form(version):
    form = Form(Form.main_form())
    top_vbox = newBox(False)
    top_vbox.set_padding(16)
    top_vbox.set_spacing(16)

    info_hbox = newBox(True)
    info_hbox.set_spacing(16)

    img_box = newImageBox()
    img_box.set_image("warning_icon.png")

    right_vbox = newBox(False)
    right_vbox.set_spacing(12)

    warn_label = newLabel("Server version %s is not supported by Workbench\nconfiguration file management tool." % ".".join(map(lambda x: str(x), version)))
    right_vbox.add(warn_label, False, True)

    warn_label = newLabel("Although, you can select different server version\nfor the tool to use. Suggested version "
                          "is given\nbelow. You can either pick version or type one."
                         )
    right_vbox.add(warn_label, False, True)

    warn_label = newLabel("Valid version formats are X.Y.ZZ or X.Y.\nAll other variants will resort to default - 5.1.")
    right_vbox.add(warn_label, False, True)

    if (type(version) is not tuple):
        version = (5,1)
        dprint_ex(1, "Given version is not a valid tuple object")

    try:
        version_maj = int(version[0]) + int(version[1]) / 10.0
    except (ValueError, IndexError):
        version_maj = 5.1


    guessed_version = 5.1

    versions = copy.copy(supported_versions)

    for v in versions:
        if version_maj >= v:
            guessed_version = v
        else:
            break

    version_selector = newSelector(SelectorCombobox)
    versions.reverse()
    version_selector.add_items(map(lambda x: str(x), versions))
    version_selector.set_value(str(guessed_version))
    right_vbox.add(version_selector, False, True)
    version_selector.add_changed_callback(lambda: verify_selected_version(version_selector, True))

    info_hbox.add(img_box, False, True)
    info_hbox.add(right_vbox, True, True)
    top_vbox.add(info_hbox, True, True)

    ok = newButton()
    ok.set_text("Ok")

    button_box = newBox(True)
    button_box.add_end(ok, False, True)

    top_vbox.add(button_box, False, True)

    form.set_content(top_vbox)
    form.run_modal(ok, None)

    guessed_version = verify_selected_version(version_selector, False) # False - do not set value back to selector
    if guessed_version == '' or len(guessed_version) == 1:
        guessed_version = "5.1"

    try:
        newver = []
        splitted = guessed_version.split(".")
        for vpart in splitted:
            newver.append(int(vpart))
        guessed_version = tuple(newver)
    except ValueError:
        guessed_version = (5,1)

    return guessed_version


#===============================================================================
class Page(object):
    def __init__(self, page_name, page_content, var_column_width):
        self.page_name = page_name
        self.page_content = page_content
        self.var_column_width = var_column_width
        self.panel = None
        self.created = False
        self.update_cb = None

    def set_update_cb(self, update_cb):
        self.update_cb = update_cb



#===============================================================================
class WbAdminConfigFileUI(mforms.Box):
    @classmethod
    def wba_register(cls, admin_context):
        admin_context.register_page(cls, "wba_instance", "Options File", True)

    @classmethod
    def identifier(cls):
        return "admin_option_file"

        #---------------------------------------------------------------------------
    def page_activated(self):
        def error_page(text):
            import mforms
            box = mforms.newBox(False)

            label = mforms.newLabel(text)
            label.set_style(mforms.BoldStyle)
            label.set_text_align(mforms.MiddleCenter)
            box.add(label, True, True)

            self.tab_view.add_page(box, "")
            self.search_panel.show(False)
            self.bottom_box.show(False)

        if not self.server_profile.is_local and not self.server_profile.remote_admin_enabled:
            error_page("MySQL Workbench requires an SSH connection to support managing Option File remotely.")
            return

        if not self.server_profile.config_file_path:
            error_page("Location of MySQL configuration file (ie: my.cnf) not specified")
            return

        if not self.ui_created:
            self.ui_created = True
            #self.suspend_layout()
            self.create_ui()
            #self.resume_layout()
        else:
            on = bool(self.server_profile.admin_enabled)
            self.file_name_ctrl.set_enabled(on)
            self.section_ctrl.set_enabled(on)
            self.bottom_box.set_enabled(on)
            self.tab_view.set_enabled(on)
            self.search_panel.set_enabled(on)

    #---------------------------------------------------------------------------
    def __init__(self, ctrl_be, server_profile, main_view, version="5.1"):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()

        self.set_padding(12)
        self.set_spacing(8)

        self.search_panel = self.create_search_panel()

        self.heading = make_panel_header("title_options.png", server_profile.name, "Options File", self.search_panel)
        self.add(self.heading, False, True)

        self.tab_view = newTabView()
        self.main_view = main_view

        self.myopts = None
        self.opt2ctrl_map = {}
        self.not_multiline_options = set()
        self.loading = True
        self.section = None
        self.ui_created = False

        self.version = version
        self.ctrl_be = ctrl_be
        self.server_profile = server_profile
        self.cfg_be = None # Will be created later in self.create_ui
        self.version = version
        self.dir_dict = {}  # Will be filled later when loading default values

        self.pack_to_top()

    #---------------------------------------------------------------------------
    def create_search_panel(self):
        search_label = newLabel("Locate option:")
        self.option_lookup_entry  = newTextEntry()
        self.option_lookup_entry.set_size(200,-1)
        search_btn = newButton()
        search_btn.set_text("Find")
        search_btn.set_size(70, -1)

        search_box = newBox(True)
        search_box.set_padding(2)
        search_box.set_spacing(4)
        #search_box.set_size(300, -1)
        search_box.add(search_label, False, True)
        search_box.add(self.option_lookup_entry, False, True)
        search_box.add(search_btn, False, True)
        search_panel = newPanel(mforms.FilledPanel)
        search_panel.add(search_box)

        def lookup_option_wrapper():
            self.locate_option(self.option_lookup_entry.get_string_value())

        search_btn.add_clicked_callback(lookup_option_wrapper)

        return search_panel

    #---------------------------------------------------------------------------
    def create_page(self, page_number):
        self.loading = True
        if page_number < 0 or page_number == 0:
            self.loading = False
            return

        if page_number not in self.pages:
            print "Unknown page number ", page_number
            self.loading = False
            return

        # page is a stored page data stored in self.pages in create_ui
        page = self.pages[page_number]
        if page.created == True:
            self.loading = False
            return

        page_content = page.page_content

        box = newBox(False)
        box.set_spacing(8)

        # Actual option values from config file parsed by cfg_be, in form of list of tuples (<name>, <value>)
        options = self.cfg_be.get_options(self.section_ctrl.get_string_value())
        opts_map = dict(options)

        # Fill the dict with dirs to be used in defaults values:
        self.dir_dict = {'datadir'   : self._get_datadir(), 
                         'basedir'   : self._get_basedir(),
                         'configdir' : os.path.dirname(self.server_profile.config_file_path),
                        }

        for group in page_content['groups']:
            controls = group['controls']

            number_of_controls = len(controls)

            if number_of_controls == 0:
                continue

            table = newTable()
            table.set_row_spacing(10)
            table.set_column_spacing(20)
            table.set_padding(5)
            table.set_homogeneous(False)
            
            table.set_row_count(number_of_controls)
            table.set_column_count(3)

            table_row = -1 # Counter to address table rows, as we may skip some control_idx.
            table.suspend_layout()
            for control_idx in range(0, number_of_controls):
                ctrl_def = controls[control_idx]

                table_row += 1

                name = ctrl_def['name']
                # Handle aliases like server_id == server-id. We have only one form in
                # opts.opts, and that form comes from documentaion team's xml file.
                # However if user config file contains alias, we substitute the alias
                # instead of official option name to use for this WBA session. From now
                # on all operation with the option are done through alias.
                names = self.cfg_be.option_alt_names(name) #(name, name.replace("-","_"), name.replace("_","-"))
                right_name = filter(lambda x: x in opts_map, names)
                if len(right_name) > 0:
                    right_name = right_name[0]
                    caption = ctrl_def.get('caption')
                    if caption and name in caption:
                        ctrl_def['caption'] = caption.replace(name, right_name)
                    name = right_name
                    ctrl_def['name'] = name

                ctrl_tuple = self.place_control(ctrl_def, table, table_row, page.var_column_width)

                label = newLabel(ctrl_def['description'])
                label.set_size(500, -1)
                label.set_wrap_text(True)
                label.set_style(SmallHelpTextStyle)
                table.add(label, 2, 3, table_row, table_row + 1, HFillFlag | VFillFlag)

                ctrl     = ctrl_tuple[1]
                ctrl_def = ctrl_tuple[2]

                #load default value into control
                if ctrl is not None and ctrl_def is not None:
                    ctrl[0].set_active(False)
                    self.enabled_checkbox_click(name, False)
                    if ctrl_def.has_key('default'):
                        default = ctrl_def['default']
                        if default is not None:
                            self.set_string_value_to_control(ctrl_tuple, str(default))
                    else:
                        self.set_string_value_to_control(ctrl_tuple, "")

                #load control with values from config
                if name in opts_map:
                    value = opts_map[name]
                    self.enabled_checkbox_click(name, True)
                    self.set_string_value_to_control(ctrl_tuple, value)

            table.resume_layout()
            # Remove empty rows
            table.set_row_count(table_row+1)#number_of_controls - (number_of_controls - table_row))

            # Important for good performance: add the table *after* all content was added to its container. So we have only one layout call!
            panel = newPanel(TitledBoxPanel)
            panel.add(table)
            panel.set_title(group['caption'])
            box.add(panel, False, True)


        
        page.panel.add(box)
        page.created = True
        self.loading = False

    #---------------------------------------------------------------------------
    def locate_option(self, option_name_fragment):
        for opt in self.cfg_be.get_options_containing(option_name_fragment):
            (tab_name, group_name) = self.cfg_be.get_option_location(opt)
            if tab_name is not None:
                for page_idx, page in self.pages.iteritems():
                    if page.page_name == tab_name:
                        self.create_page(page_idx)
                        self.tab_view.set_active_tab(page_idx - 1)
                        page.panel.flush_events()
                        ctrl_tuple = self.opt2ctrl_map.get(opt)
                        # ctrl_tuple is in for of ('<type>', <ui controls tuple>, <opt def from xml file>)
                        if ctrl_tuple is not None:
                            ctrl = ctrl_tuple[1] # in this ctrl we have format of (enabled_checkbox, ...)
                            page.panel.scroll_to_view(ctrl[0]) # scroll to enabled checkbox
                            return

    #---------------------------------------------------------------------------
    def tab_changed(self):
        page_number = self.tab_view.get_active_tab() + 1
        page = self.pages.get(page_number)
        if page is not None and page.update_cb is not None:
            page.update_cb(page)
        else:
            self.create_page(page_number)

    #---------------------------------------------------------------------------
    def create_ui(self):
        self.loading = True

        self.cfg_be = wb_admin_config_file_be.WbAdminConfigFileBE(self.server_profile, self.ctrl_be)

        sys_config_path = self.server_profile.config_file_path
        if sys_config_path is None:
            sys_config_path = ""
        self.file_name_ctrl.set_text(sys_config_path or "not configured")
        self.section_ctrl.add_changed_callback(self.clear_and_load)
        try:
            self.myopts = self.cfg_be.get_possible_options()
            option_stats = self.cfg_be.get_option_set_stats()
            dprint_ex(1, "Options stats: '%s'" % str(option_stats))
            if option_stats and type(option_stats) is dict:
                added = option_stats.get("added", None)
                if added is not None and added < 10:
                    user_selected_version = run_version_select_form(option_stats["version"])
                    self.server_profile.set_server_version(".".join(map(lambda x: str(x), user_selected_version)))
                    self.cfg_be.reload_possible_options()
                    self.myopts = self.cfg_be.get_possible_options()
                    option_stats = self.cfg_be.get_option_set_stats()
                    dprint_ex(1, "Options stats after version correction: '%s'" % str(option_stats))
        except KeyError:
            Utilities.show_error("Error", "Wrong version '" + self.version + "'given to admin plugin", "Close", None, None)

        self.load_options_from_cfg()

        #build ordered list of pages. Initially only skeleton pages are created, means only names.
        # Values into pages will be load as soon as page is switched to.
        self.pages = {}

        for page_name, page_content in self.myopts.iteritems():
            self.pages[int(page_content['position'])] = Page(page_name, page_content, page_content['width']) # False means page not created
        # page key is its position in UI. As we can have pages positions set like (1,2,4,5)
        # the position set needs to be sorted so pages appear in specified order
        page_positions = self.pages.keys()
        page_positions.sort()

        # Create dummy pages according to assigned position
        for page_pos in page_positions:
            page = self.pages[page_pos]
            page.panel = newScrollPanel(mforms.ScrollPanelNoFlags)
            self.tab_view.add_page(page.panel, page.page_name)

        if debug_level > 0:
            # Create file page
            page = Page("File", None)
            page.panel = newTextBox(mforms.BothScrollBars)
            page.set_update_cb(self.update_file_content_tab)
            self.pages[max(self.pages.keys()) + 1] = page
            self.tab_view.add_page(page.panel, page.page_name)

        # Create first page, so we display something from start
        self.create_page(1)
        self.loading = True # create_page resets loading flag

        self.tab_view.add_tab_changed_callback(self.tab_changed)

        self.loading = False

    #---------------------------------------------------------------------------
    def update_file_content_tab(self, page):
        page.panel.clear()
        for line in self.cfg_be.get_file_content():
            page.panel.append_text(line)

    #---------------------------------------------------------------------------
    def create_textedit(self, name, ctrl_def):
        te = newTextEntry()
        te.set_enabled(False)
        te.add_changed_callback(lambda: self.control_action(name))
        te.set_tooltip("To convert option to a multi-line one, use " + multi_separator + " to separate values. The symbol " + multi_separator + " should not be at the start or end of the value")
        return te

    #---------------------------------------------------------------------------
    def create_dir_file_edit(self, name, ctrl_def):
        dir_box = newBox(True)
        dir_box.set_spacing(4)

        te = newTextEntry()
        te.set_tooltip("To convert option to a multi-line one, use " + multi_separator + " to separate values. The symbol " + multi_separator + " should not be at the start or end of the value")
        btn = newButton()
        btn.set_text("...")
        btn.enable_internal_padding(False)
        btn.add_clicked_callback(lambda: self.open_file_chooser(OpenDirectory, te, name))

        dir_box.add(te, True, True)
        dir_box.add(btn, False, True)
        te.set_enabled(False)
        btn.set_enabled(False)

        return (dir_box, te, btn)

    #---------------------------------------------------------------------------
    def _get_datadir(self):
        if self.server_profile.datadir:
            return self.server_profile.datadir
        elif self.cfg_be.original_opts.get('datadir'):
            return self.cfg_be.original_opts['datadir'].val(0)
        return ''

    #---------------------------------------------------------------------------
    def _get_basedir(self):
        if self.server_profile.basedir:
            return self.server_profile.basedir
        elif self.cfg_be.original_opts.get('basedir'):
            return self.cfg_be.original_opts['basedir'].val(0)
        return ''

    #---------------------------------------------------------------------------
    def create_fileedit(self, name, ctrl_def):
        def download_file_from_server_cb(te):
            ospath = server_os_path(self.server_profile)
            remote_path = te.get_string_value().strip('" ')
            if not remote_path:
                res = mforms.Utilities.show_error('Specify remote location',
                    'No remote location available. Please specify the path to the file in the server using '
                    'the line edit control at the left of the download button and try again. Or you can click '
                    'on the "Use Suggested" button to let Workbench use an appropriate value for the remote '
                    'location',
                    'OK', 'Use Suggested', '')
                if res == mforms.ResultCancel:
                    # Use ctrl_def default if available before attempting to build a default value:
                    if ctrl_def.get('default'):
                        remote_path = ctrl_def['default'] % self.dir_dict 
                    else:
                        remote_path = self._get_datadir()
                        if remote_path:
                            remote_path = ospath.abspath(ospath.join(remote_path, '%s.txt' % name))
                        else:
                            mforms.Utilities.show_warning('Could not find server data directory',
                                'Please type the full path to the file in the server before attempting to upload it',
                                'OK', '', '')
                            return
                    te.set_value(remote_path)
                else:
                    return
            elif not ospath.isabs(remote_path):  # file is relative to datadir
                dirname = self._get_datadir()
                if dirname:
                    remote_path = ospath.abspath(ospath.join(dirname, remote_path))
            file_exists = False

            # Check if remote_path exists in the server before attempting to download it:
            as_user, user_password = Users.CURRENT, None
            try:
                file_exists = self.ctrl_be.server_helper.file_exists(remote_path)
            except Exception:  # Try again as admin
                user_password = None
                while True:
                    try:
                        user_password = self.ctrl_be.password_handler.get_password_for("file")
                        if user_password is not None:
                            as_user = Users.ADMIN
                        file_exists = self.ctrl_be.server_helper.file_exists(remote_path, as_user, user_password)
                    except InvalidPasswordError, e:  # Wrong pwd, retry:
                        self.ctrl_be.password_handler.reset_password_for("file")
                        pass
                    except OperationCancelledError:  # The user chose Cancel in pwd input dialog
                        return
                    else:
                        break
            if not file_exists:
                mforms.Utilities.show_error('File does not exist',
                    'The file "%s" does not exist in %s' % (remote_path, self.server_profile.host_name),
                    'OK', '', '')
                return
            filechooser = FileChooser(SaveFile)
            filechooser.set_directory(os.path.expanduser('~'))
            if filechooser.run_modal():
                content = None
                try:
                    content = self.ctrl_be.server_helper.get_file_content(remote_path, as_user, user_password)
                except PermissionDeniedError, e:
                    try:
                        user_password = self.ctrl_be.password_handler.get_password_for("file")
                    except Exception, e:
                        return
                    if user_password is not None:
                        as_user = Users.ADMIN
                    try:
                        content = self.ctrl_be.server_helper.get_file_content(remote_path, as_user, user_password)
                    except InvalidPasswordError, err:
                        self.ctrl_be.password_handler.reset_password_for("file")
                        raise err
                except Exception, e:
                    mforms.Utilities.show_error('Error while reading from "%s"' % remote_path,
                        str(e),
                        'OK', '', '')
                    return

                if content is not None:
                    local_path = filechooser.get_path()
                    try:
                        local_file = open(local_path, 'w')
                    except IOError, e:
                        mforms.Utilities.show_error('Error while opening "%s" for writing' % local_path,
                            str(e),
                            'OK', '', '')
                        return

                    local_file.write(content)
                    mforms.App.get().set_status_text('File %s in %s successfully downloaded to %s' % (remote_path,
                                                                                                      self.server_profile.host_name,
                                                                                                      local_path)
                                                    )
        def upload_file_to_server_cb(te):
            ospath = server_os_path(self.server_profile)
            remote_path = te.get_string_value().strip('" ')
            if not remote_path:
                res = mforms.Utilities.show_error('Specify remote location',
                    'No remote location available. Please specify the path to the file in the server using '
                    'the line edit control at the left of the upload button and try again. Or you can click '
                    'on the "Use Suggested" button to let Workbench use an appropriate value for the remote '
                    'location',
                    'OK', 'Use Suggested', '')
                if res == mforms.ResultCancel:
                    # Use ctrl_def default if available before attempting to build a default value:
                    if ctrl_def.get('default'):
                        remote_path = ctrl_def['default'] % self.dir_dict
                    else:
                        remote_path = self._get_datadir()
                        if remote_path:
                            remote_path = ospath.abspath(ospath.join(remote_path, '%s.txt' % name))
                            te.set_value(remote_path)
                        else:
                            mforms.Utilities.show_warning('Could not find server data directory',
                                'Please type the full path to the file in the server before attempting to upload it',
                                'OK', '', '')
                            return
                    te.set_value(remote_path)
                else:
                    return
            elif not ospath.isabs(remote_path):  # file is relative to datadir
                dirname = self._get_datadir()
                if dirname:
                    remote_path = ospath.abspath(ospath.join(dirname, remote_path))
            filechooser = FileChooser(OpenFile)
            filechooser.set_directory(os.path.expanduser('~'))
            if filechooser.run_modal():
                local_path = filechooser.get_path()
                try:
                    local_file = open(local_path)
                except IOError, e:
                    mforms.Utilities.show_error('Error while opening "%s"' % local_path,
                        str(e),
                        'OK', '', '')
                    return

                try:
                    content = local_file.read()
                    self.ctrl_be.server_helper.set_file_content(remote_path, content, as_user=Users.CURRENT, user_password=None)
                except PermissionDeniedError, e:
                    try:
                        user_password = self.ctrl_be.password_handler.get_password_for("file")
                    except Exception, e:
                        return
                    if user_password is not None:
                        as_user = Users.ADMIN
                    try:
                        self.ctrl_be.server_helper.set_file_content(remote_path, content, as_user, user_password)
                    except InvalidPasswordError, err:
                        self.ctrl_be.password_handler.reset_password_for("file")
                        raise err
                except Exception, e:
                    mforms.Utilities.show_error('Error while uploading the file to "%s"' % remote_path,
                        str(e),
                        'OK', '', '')
                    return
                mforms.App.get().set_status_text('File %s successfully uploaded to %s as %s' % (local_path,
                                                                                                self.server_profile.host_name,
                                                                                                remote_path)
                                                )

        dir_box = newBox(True)
        dir_box.set_spacing(4)

        te = newTextEntry()
        te.set_tooltip('The path to the file in your server')
        dir_box.add(te, True, True)
        te.set_enabled(False)
        
        btn = newButton()
        btn.set_text("...")
        btn.set_tooltip('Browse a file in your server...')
        btn.enable_internal_padding(False)
        btn.add_clicked_callback(lambda: self.open_file_chooser(OpenFile, te, name))
        dir_box.add(btn, False, True)

        btn_dwn = None
        if ctrl_def['type'] in ('filedownload', 'fileedit'):
            btn_dwn = newButton(mforms.ToolButton)
            btn_dwn.set_icon(mforms.App.get().get_resource_path('record_export.png'))
            btn_dwn.set_tooltip('Download this file to your local computer')
            btn_dwn.enable_internal_padding(False)
            btn_dwn.add_clicked_callback(lambda: download_file_from_server_cb(te))
            dir_box.add(btn_dwn, False, True)
            btn_dwn.set_enabled(False)

        btn_upl = None
        if ctrl_def['type'] == 'fileedit':
            btn_upl = newButton(mforms.ToolButton)
            btn_upl.set_icon(mforms.App.get().get_resource_path('record_import.png'))
            btn_upl.set_tooltip('Upload a file from your local computer to the server')
            btn_upl.enable_internal_padding(False)
            btn_upl.add_clicked_callback(lambda: upload_file_to_server_cb(te))
            dir_box.add(btn_upl, False, True)
            btn_upl.set_enabled(False)


        return (dir_box, te, btn, btn_dwn, btn_upl)

    #---------------------------------------------------------------------------
    def create_numeric(self, name, ctrl_def):
        te = newTextEntry()
        #spin_box.add(te, True, True)
        te.set_enabled(False)
        te.set_tooltip("For numeric values, you may specify a K, M or G size suffix, if appropriate.")

        te.add_changed_callback(lambda: self.control_action(name))

        #unit = None
        #if ctrl_def.has_key('unitcontrol'):
        #  unit = ctrl_def['unitcontrol']

        #unitcontrol = None
        #unit_items = None

        #if unit is not None:
        #  unitcontrol = newSelector()
        #  unit_items = unit.split(";")
        #  for item in unit_items:
        #    unitcontrol.add_item(item)
        #
        #  spin_box.add(unitcontrol, False, False)
        #  unitcontrol.set_enabled(False)
        #  unitcontrol.add_changed_callback(lambda: self.control_action(name))

        #return (spin_box, te, unitcontrol, unit_items)
        return te

    #---------------------------------------------------------------------------
    def create_dropdownbox(self, name, ctrl_def, ctype):
        items = None
        if 'choice' in ctrl_def:
            items = ctrl_def['choice']

        style = SelectorPopup
        if ctype == 'dropdownboxentry':
            style = SelectorCombobox
        dropbox = newSelector(style)

        if type(items) is str:
            if items in wb_admin_config_file_be.pysource:
                code = wb_admin_config_file_be.pysource[items]
                result = eval(code)
                items = []
                for item in result.split(','):
                    item = item.strip(" \t")
                    items.append(item)

        for i in items:
            dropbox.add_item(i)
        dropbox.set_enabled(False)

        if ctrl_def.has_key('default'):
            default = ctrl_def['default']
            idx = 0
            for i in items:
                if i == default:
                    dropbox.set_selected(idx)
                idx += 1

        dropbox.add_changed_callback(lambda: self.control_action(name))

        return (dropbox, items)

    #---------------------------------------------------------------------------
    def place_control(self, ctrl_def, table, row, val_column_width):
        ctrl = None
        ctype = ctrl_def['type']
        name = ctrl_def['name']

        enabled = newCheckBox()
        enabled.set_text(ctrl_def['caption'])
        enabled.set_size(val_column_width, -1) # Use a fixed fix to make all tables align their columns properly. Must be larger than the largest text, to make it work.
        enabled.set_tooltip("%s\n\n%s" % (ctrl_def['name'], ctrl_def['description']))

        # place_control creates control as ctrl_def describes. Reference to a created control is placed
        # to map of controls. That is done in order to access controls via option name
        if ctype == "checkbox" or ctype == "boolean":
            ctrl = ('chk', (enabled, enabled), ctrl_def)
            self.opt2ctrl_map[name] = ctrl
            enabled.set_active(False)
            #label = newLabel(" ")
            #table.add(label, 1, 2, row, row+1, HExpandFlag | HFillFlag)
        elif ctype == 'textedit' or ctype == 'string' or ctype == 'set':
            te = self.create_textedit(name, ctrl_def)
            table.add(te, 1, 2, row, row+1, VFillFlag | HExpandFlag | HFillFlag)
            ctrl = ('txt', (enabled, te), ctrl_def)
            self.opt2ctrl_map[name] = ctrl
        elif ctype in ['filename', 'fileedit', 'filebrowse', 'filedownload']:
            if name.endswith('dir'):
                (dir_box, te, btn) = self.create_dir_file_edit(name, ctrl_def)
                ctrl = ('dir', (enabled, te, btn), ctrl_def)
            else:
                (dir_box, te, btn, btn_dwn, btn_upl) = self.create_fileedit(name, ctrl_def)
                ctrl = ('fed', (enabled, te, btn, btn_dwn, btn_upl), ctrl_def)
            table.add(dir_box, 1, 2, row, row + 1, VFillFlag | HExpandFlag | HFillFlag)
            te.add_changed_callback(lambda: self.control_action(name))
            self.opt2ctrl_map[name] = ctrl
        elif ctype in ['directory', 'dirname']:
            (dir_box, te, btn) = self.create_dir_file_edit(name, ctrl_def)
            ctrl = ('dir', (enabled, te, btn), ctrl_def)
            table.add(dir_box, 1, 2, row, row + 1, VFillFlag | HExpandFlag | HFillFlag)
            te.add_changed_callback(lambda: self.control_action(name))
            self.opt2ctrl_map[name] = ctrl
        elif ctype == "numeric" or ctype == "spinedit" or ctype == "integer":
            #(spin_box, te, unitcontrol, unit_items) = self.create_numeric(name, ctrl_def)
            #ctrl = ('spn', (enabled, te, unitcontrol, unit_items), ctrl_def)
            te = self.create_numeric(name, ctrl_def)
            #ctrl = ('spn', (enabled, te), ctrl_def)
            ctrl = ('txt', (enabled, te), ctrl_def)
            self.opt2ctrl_map[name] = ctrl
            #table.add(spin_box, 1, 2, row, row + 1, HExpandFlag | HFillFlag)
            table.add(te, 1, 2, row, row + 1, VFillFlag | HExpandFlag | HFillFlag)
        elif ctype == "dropdownbox" or ctype == 'dropdownboxentry' or ctype == "enum":
            if 'choice' not in ctrl_def:
                te = newTextEntry()
                te.set_enabled(False)
                te.add_changed_callback(lambda: self.control_action(name))
                table.add(te, 1, 2, row, row+1, VFillFlag | HExpandFlag | HFillFlag)
                ctrl = ('txt', (enabled, te), ctrl_def)
                self.opt2ctrl_map[name] = ctrl
            else:
                (dropbox, items) = self.create_dropdownbox(name, ctrl_def, ctype)
                table.add(dropbox, 1, 2, row, row + 1, VFillFlag | HExpandFlag | HFillFlag)
                ctrl = ('drp', (enabled, dropbox, items), ctrl_def)
                self.opt2ctrl_map[name] = ctrl
        else:
            raise NotImplementedError("Control type %s not implemented for config file editor"%ctype)

        if CATOPTS is None:
            table.add(enabled, 0, 1, row, row + 1, VFillFlag | HFillFlag)
            enabled.add_clicked_callback(lambda: self.enabled_checkbox_click(name))
        else:
            catbox = newBox(True)
            cat = newSelector(mforms.SelectorCombobox)
            for item in cat_sec:
                cat.add_item(item)
            grp = newSelector(mforms.SelectorCombobox)
            for item in cat_grp:
                grp.add_item(item)
            cat.add_changed_callback(lambda : handle_cat_opt(cat, grp, ctrl_def['name']))
            grp.add_changed_callback(lambda : handle_cat_opt(cat, grp, ctrl_def['name']))
            catbox.add(cat, True, True)
            catbox.add(grp, True, True)
            catbox.add(enabled, True, True)
            table.add(catbox, 0, 1, row, row + 1, HExpandFlag | HFillFlag)

        return ctrl

    #---------------------------------------------------------------------------
    def open_file_chooser(self, file_chooser_type, textfield, name):
        filename = None
        if self.server_profile.is_local:
            filechooser = FileChooser(file_chooser_type)
            filechooser.set_directory(textfield.get_string_value())
            if filechooser.run_modal():
                filename = filechooser.get_directory() if file_chooser_type is OpenDirectory else filechooser.get_path()
        else:
            from wba_ssh_ui import remote_file_selector
            filename = remote_file_selector(self.server_profile, self.ctrl_be.password_handler, self.ctrl_be.ssh)

        if filename and (type(filename) is str or type(filename) is unicode):
            filename = filename.replace("\\", "/") # TODO: Check for backslashed spaces and so on
            textfield.set_value('"' + filename + '"')
            self.control_action(name)

    #---------------------------------------------------------------------------
    def enabled_checkbox_click(self, name, force_enabled = None):
        if self.opt2ctrl_map.has_key(name):
            ctrl = self.opt2ctrl_map[name]

            def control(idx):
                return ctrl[1][idx]

            tag = ctrl[0] # tuple ctrl holds tag at index 0, the rest is control def. Exact format
                          # of control tuple(the one that goes after tag is defined by the type of control

            if force_enabled is not None:
                enabled = bool(force_enabled)
                control(0).set_active(enabled)
            else:
                enabled = control(0).get_active()

            if tag == "txt":
                control(1).set_enabled(enabled)
            elif tag == "spn":
                control(1).set_enabled(enabled)
                if control(2) is not None:
                    control(2).set_enabled(enabled)
            elif tag == "drp":
                control(1).set_enabled(enabled)
            elif tag == "dir":
                control(1).set_enabled(enabled)
                control(2).set_enabled(enabled)
            elif tag == "fed":
                control(1).set_enabled(enabled)
                control(2).set_enabled(enabled)
                try:
                    control(3).set_enabled(enabled)
                    control(4).set_enabled(enabled)
                except Exception:
                    pass

            if not self.loading:
                # Notify config BE about change
                if enabled:
                    self.cfg_be.option_added(name, self.get_string_value_from_control(name, ctrl), self.section)
                else:
                    self.cfg_be.option_removed(name, self.section)

    #---------------------------------------------------------------------------
    def control_action(self, name):
        if self.loading:
            return

        if self.opt2ctrl_map.has_key(name):
            ctrl = self.opt2ctrl_map[name]

            if not self.loading:
                self.cfg_be.option_changed(name, self.get_string_value_from_control(name, ctrl), self.section)

    #---------------------------------------------------------------------------
    def get_string_value_from_control(self, option_name, ctrl):
        value = "" # ctrl is a tuple from map

        tag = ctrl[0]
        def control(idx):
            return ctrl[1][idx]

        is_multiple = False
        control_name = control(1).get_name()
        if control_name == "Multiple":
            is_multiple = True

        if tag == "txt":
            value = (control(1).get_string_value(),)
        elif tag == "spn":
            # (enabled, te, unitcontrol, unit_items)). Note! unitcontrol and unit_items may be None
            value = control(1).get_string_value().strip(" \r\n\t")

            if control(2) is not None:
                value += control(2).get_string_value()
        elif tag == "drp":
            value = control(1).get_string_value()
        elif tag == "dir":
            value = control(1).get_string_value()
            if is_multiple:
                value = value.split(multi_separator)
        elif tag == "fed":
            value = control(1).get_string_value()
            if is_multiple:
                value = value.split(multi_separator)
        elif tag == "chk":
            value = (control(0).get_active(),)

        # Here we detect if value has signs of multi line option.
        # For example, user entered separator char.
        # It is enough to ensure that the first item in tuple is string
        is_string = False  # We only can detect multi-line in strings (rework is scheduled for 5.3)
        has_separator = False
        if type(value) is tuple:
            value_len = len(value)
            if value_len == 1: # Check only single item tuples
                is_string = type(value[0]) is str or type(value[0]) is unicode
                if is_string:
                    has_separator = value[0].find(multi_separator) > 0
        else:
            is_string = type(value) is str or type(value) is unicode
            has_separator = value.find(multi_separator) > 0

        if is_multiple == False and is_string and has_separator and not self.loading and option_name not in self.not_multiline_options:
            answer = Utilities.show_message("Multi Line Option"
                ,"The %s sequence was detected in the string, which is used for separating multi-line options. Would you like to split the value to multiple options?" % multi_separator
                , "Convert", "Keep as Is", "")
            if answer == mforms.ResultOk:
                control(1).set_name("Multiple")
            else:
                self.not_multiline_options.add(option_name)

        # some controls return values in form of one-item tuples
        # so we need to extract that item for processing below
        if has_separator and is_string and not option_name in self.not_multiline_options:
            if type(value) is tuple:
                if len(value) == 1:     # Only extract values from one-item tuples
                    value = value[0]      # If tuple has more items it already has been converted to multi-line

            # skip multi line values - no need to convert. Also skip non-string option values
            if type(value) is not tuple:
                value = map(lambda x: x.strip(multi_separator), value.split(multi_separator))

        if type(value) is not list and type(value) is not tuple:
            value = (value,)

        return value

    #---------------------------------------------------------------------------
    #Value is a an integer with
    def parse_spin_string(self, unit_items, value):
        value = value.strip(" \r\t\n")
        longest_suffix = 0
        if unit_items is not None:
            for item in unit_items:
                l = len(item)
                if (l > longest_suffix):
                    longest_suffix = l

        suffix = ""

        def get_unit(sfx):
            sfx2 = sfx.lower()
            ret_item = None
            for item in unit_items:
                if sfx2 == item.lower():
                    ret_item = item
                    break
            return ret_item

        if longest_suffix > 0:
            value_len = len(value)
            for suffix_length in range(1,longest_suffix + 1):
                if suffix_length < value_len:
                    cur_sfx = value[-suffix_length:]
                    cur_unit = get_unit(cur_sfx)
                    if cur_unit is not None:
                        suffix = cur_unit
                        value = value[:-suffix_length]

        return (filter(lambda x: x.isdigit() or x == '-', value), suffix)


    #---------------------------------------------------------------------------
    def set_string_value_to_control(self, ctrl, value):
        tag = ctrl[0] #ctrl is a tuple from map
        def control(idx):
            return ctrl[1][idx]

        if type(value) is tuple and len(value) > 1:
            control(1).set_name("Multiple")
        else:
            control(1).set_name("Single")

        if tag in ['txt', 'dir', 'fed']:
            if value is None or not isinstance(value, (str, unicode)):
                # TODO: Add config file error message
                value = ""
            value = value.strip(" \r\n\t")
            try:
                value = value.format(**self.dir_dict)
            except Exception:
                pass
            control(1).set_value(value)
        elif tag == "spn":
            if value is None:
                value = ""
            elif type(value) is not str:
                # TODO: Add Warning
                pass
            value = value.strip(" \r\n\t")
            (value,suffix) = self.parse_spin_string(control(3), value)
            control(1).set_value(value)

            if control(2) is not None and suffix is not None:
                try:
                    idx = control(3).index(suffix)
                    control(2).set_selected(idx)
                except ValueError:
                    pass
        elif tag == "drp":
            #search for value
            try:
                items = control(2)
                if items is not None:
                    lowcase_value = value.lower()
                    for (i, item) in enumerate(items):
                        if item.lower() == lowcase_value:
                            control(1).set_selected(i)
            except ValueError:
                pass
        elif tag == "chk":
            value = self.cfg_be.normalize_bool(value)
            control(1).set_active(value)
        else:
            print "Error"

    #---------------------------------------------------------------------------
    def config_apply_changes_clicked(self):
        errors = self.cfg_be.validate_changes(self.myopts)
        if errors:
            mforms.Utilities.show_warning("Configuration Error",
                        "The following errors were found in the changes you have made.\n"
                        "Please correct them before applying:\n"+errors, "OK", "", "")
        else:
            self.cfg_be.apply_changes()

    #---------------------------------------------------------------------------
    def config_discard_changes_clicked(self):
        self.clear_and_load()

    #---------------------------------------------------------------------------
    def clear_and_load(self):
        self.not_multiline_options = set()
        if self.loading == False:
            self.load_defaults()
            self.load_options_from_cfg(self.section_ctrl.get_string_value())

    #---------------------------------------------------------------------------
    def load_options_from_cfg(self, given_section = None):
        self.loading = True

        try:
            self.cfg_be.open_configuration_file(self.server_profile.config_file_path or "")
        except Exception, exc:
            import traceback
            traceback.print_exc()
            mforms.Utilities.show_error("Error opening configuration file",
                                        "%s: %s" % (type(exc).__name__, exc),
                                        "OK", "", "")

        self.section_ctrl.clear()

        idx = 0
        if given_section is None or given_section == "":
            given_section = self.server_profile.config_file_section

        # Fill section selector at the bottom of config file page
        section_ctrl_was_filled = False
        for i,sec in enumerate(self.cfg_be.get_sections()):
            self.section_ctrl.add_item(sec)
            section_ctrl_was_filled = True
            if sec == given_section:
                idx = i
                self.section = sec

        # If we have an empty file or file with no section, add user-specified default section to the selector
        if section_ctrl_was_filled == False and given_section is not None:
            self.section_ctrl.add_item(given_section)
            idx = 0

        self.section_ctrl.set_selected(idx)

        # each opt is (name, value)
        for name, value in self.cfg_be.get_options(self.section):
            ctrl = self.opt2ctrl_map.get(name)
            if not ctrl:
                # We are here because of either 1) alias, or 2) unknown option, or
                # 3) initial load when map is not created
                # Try to resolve alias and update UI and maps
                names = self.cfg_be.option_alt_names(name)
                for alt_name in names:
                    ctrl = self.opt2ctrl_map.get(alt_name)
                    if ctrl is not None:
                        # Fix UI and map
                        self.opt2ctrl_map[name] = ctrl
                        # Rename checkbox with name
                        ctrl[1][0].set_text(name)
                        del self.opt2ctrl_map[alt_name]
                        break

            if ctrl is None:
                continue

            self.enabled_checkbox_click(name, True)
            self.set_string_value_to_control(ctrl, value)

        self.loading = False

    #---------------------------------------------------------------------------
    def load_defaults(self):
        self.loading = True
        for name,ctrl_tuple in self.opt2ctrl_map.iteritems():
            if ctrl_tuple is not None:
                #tag      = ctrl_tupple[0]
                ctrl     = ctrl_tuple[1]
                ctrl_def = ctrl_tuple[2]

                if ctrl is not None and ctrl_def is not None:
                    ctrl[0].set_active(False)
                    self.enabled_checkbox_click(ctrl_def['name'], False)
                    if ctrl_def.has_key('default'):
                        default = ctrl_def['default']
                        if default is not None:
                            self.set_string_value_to_control(ctrl_tuple, str(default))
                    else:
                        self.set_string_value_to_control(ctrl_tuple, "")

        self.loading = False


    #-------------------------------------------------------------------------------
    def pack_to_top(self):
        self.file_name_ctrl = newLabel("")
        self.file_name_ctrl.set_tooltip("To change the path to the configuration file, edit the server profile in the Manage Server Instances dialog.")
        sys_config_path = self.server_profile.config_file_path
        if sys_config_path is None:
            sys_config_path = ""
        self.file_name_ctrl.set_text(sys_config_path)
        self.file_name_ctrl.set_size(300, -1)

        self.section_ctrl = newSelector()
        self.section_ctrl.set_size(150, -1)

        self.bottom_box = newBox(True)

        accept_btn = newButton()
        accept_btn.set_text("Apply...")
        accept_btn.add_clicked_callback(self.config_apply_changes_clicked)

        discard_btn = newButton()
        discard_btn.set_text("Discard")
        discard_btn.add_clicked_callback(self.config_discard_changes_clicked)

        self.add(self.tab_view, True, True)
        self.add(self.bottom_box, False, True)

        self.bottom_box.add(newLabel("Configuration File:"), False, True)
        self.bottom_box.add(self.file_name_ctrl, True, True)
        self.bottom_box.add(self.section_ctrl, False, True)

        Utilities.add_end_ok_cancel_buttons(self.bottom_box, accept_btn, discard_btn)

        self.bottom_box.set_spacing(8)
        self.bottom_box.set_padding(12)

    #-------------------------------------------------------------------------------
    def shutdown(self):
        if hasattr(self.cfg_be, 'changeset') and self.cfg_be.changeset:
            res = Utilities.show_warning('Configuration file', '''Configuration file from instance '%s' may contain unsaved changes.
Would you like to review them?''' % self.server_profile.name, 'Review', 'Cancel', 'Discard')
            if res == mforms.ResultOk:
                self.config_apply_changes_clicked()
            return res == mforms.ResultOther
