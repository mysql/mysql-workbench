# Copyright (c) 2007, 2015, Oracle and/or its affiliates. All rights reserved.
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

import re
import time
import getpass
import threading
import functools
import random
import string
import os

import mforms

from mforms import newBox, newLabel, newButton, newTextEntry, newTreeNodeView, newTable, newRadioButton, newListBox, newSelector, newPanel, newTabView, Utilities, newCheckBox, newImageBox, App
from wb_admin_utils import not_running_warning_label, make_panel_header
from wb_admin_security_be import AdminSecurity, PrivilegeInfo, PrivilegeReverseDict, SecurityAdminRoles, WBSecurityValidationError
from wb_common import PermissionDeniedError
from workbench.log import log_error


SCHEMA_OBJECT_RIGHTS = [
"Select_priv",
"Insert_priv",
"Update_priv",
"Delete_priv",
"Execute_priv",
"Show_view_priv",
]

SCHEMA_DDL_RIGHTS = [
"Create_priv",
"Alter_priv",
"References_priv",
"Index_priv",
"Create_view_priv",
"Create_routine_priv",
"Alter_routine_priv",
"Event_priv",
"Drop_priv",
"Trigger_priv"
]

SCHEMA_OTHER_RIGHTS = [
"Grant_priv",
"Create_tmp_table_priv",
"Lock_tables_priv",
]


DEFAULT_AUTH_PLUGIN = "mysql_native_password"
AUTHENTICATION_PLUGIN_TYPES = {
"mysql_native_password" : {"name" : "Standard",
        "auth_string_label": "",
        "enable_host": True,
        "enable_password" : True,
        },
None : {"name" : "Standard",
        "auth_string_label": "",
        "enable_host": True,
        "enable_password" : True,
        },
"mysql_old_password" : {"name" : "Standard (old)",
        "auth_string_label": "",
        "enable_host": True,
        "enable_password" : True,
        },

"authentication_windows" : { "name": "Windows Native",
        "auth_string_label": "Supply the Windows username and/or group names that are allowed to use this account, separated by a comma (,)\nThis account will be usable by these users whenever they are logged into Windows, without additional passwords.\nExample: Administrator, %s" % (getpass.getuser() or "joe"),
        "enable_host": True,
        "enable_password" : False
        },
"authentication_pam" : {"name": "PAM",
        "auth_string_label": "Syntax: <pam_service_name>[,<group_name1>=<sql_user_name1>  [,<group_name2>=<sql_user_name2> ...]]\nSee documentation for details.",
        "enable_host": True,
        "enable_password" : False
        },
"sha256_password" : {"name": "SHA256 Password",
        "auth_string_label": "",
        "enable_host": True,
        "enable_password" : True
},
"*" : {"name": None,
        "auth_string_label": "See the plugin documentation for valid values and details.",
        "enable_host": True,
        "enable_password" : True
        }
}

def rLabel(text):
    l = newLabel(text)
    l.set_text_align(mforms.MiddleRight)
    return l

def dLabel(text):
    l = newLabel(text)
    l.set_style(mforms.SmallHelpTextStyle)
    return l



class ThreadedInputValidator(object):
    """This class validates the changes in the associated text entry widget displaying the result in a validation label

    Usage:
        Instanciate this class and set is callback method to be the callback of the associated text entry.
    """
    def __init__(self, owner, text_entry, validation_label, ctrl_be, delay=1, colors=('#33aa33', '#aa3333')):
        self.owner = owner
        self.text_entry = text_entry
        self.validation_label = validation_label
        self.ctrl_be = ctrl_be
        self.delay = delay
        self.colors = colors

        self.last_keypress_time = None
        self.text_to_validate = None
        self.lock = threading.Lock()
        self.timer = None
        self.strength = ['Blank','Weak', 'Medium strength', 'Strong']
        self.message = '%s password.'

    def update_validation_label(self, estimate):  # Call this from the main thread
        color = self.colors[0 if estimate > 1 else 1]
        message = self.strength[0] if estimate == 0 else (self.message % self.strength[estimate])
        self.validation_label.set_color(color)
        self.validation_label.set_text(message)
        self.owner.set_dirty()

    def callback(self):
        with self.lock:
            self.last_keypress_time = time.time()
            self.text_to_validate = self.text_entry.get_string_value()

        if not self.timer or not self.timer.is_alive():
            self.timer = threading.Timer(self.delay, self.validate)
            self.timer.start()
    def shutdown(self):
        if self.timer:
          self.timer.cancel()

    def validate(self):
        def compute_password_strength(passwd):
            if passwd == '':
                return 0
            if len(passwd) < 8:
                return 1

            estimate = 1
            if len(passwd) >= 12:
                estimate += 1
            if re.search('\d+', passwd):
                estimate += 1
            if re.search('[a-z]', passwd) and re.search('[A-Z]', passwd):
                estimate += 1
            if re.search('(\W|_)+', passwd):
                estimate += 1

            return  1 if estimate < 3 else (
                    2 if estimate < 5 else
                    3                      ) 

        def query_password_strength(passwd):
            if passwd == '':
                return 0

            result = self.ctrl_be.exec_query("SELECT VALIDATE_PASSWORD_STRENGTH('%s')" % passwd)
            if result and result.nextRow():
                estimate = result.intByIndex(1)
            else:
                return compute_password_strength(passwd)

            return 1 if estimate < 50 else (
                   2 if estimate < 75 else
                   3                       ) 

        query_strength = self.ctrl_be.is_sql_connected() and ('validate_password', 'VALIDATE PASSWORD') in self.ctrl_be.server_active_plugins
        strength_function = query_password_strength if query_strength else compute_password_strength
        while True:  # If after processing the current text a new text to validate is present, then process it
            with self.lock:
                last_keypress_time = self.last_keypress_time
                text_to_validate = self.text_to_validate

            if time.time() - last_keypress_time >= self.delay:
                if text_to_validate is None:  # delay have passed and there is no text to validate
                    break  # Leave this function thus stopping this thread
                with self.lock:
                    self.text_to_validate = None
                estimate = strength_function(text_to_validate)
                mforms.Utilities.perform_from_main_thread(functools.partial(self.update_validation_label, estimate=estimate),
                                                          False)
            time.sleep(0.2)
        mforms.Utilities.driver_shutdown()


class AddSchemaPrivilegeForm(mforms.Form):
    def __init__(self, secman, user=""):
        mforms.Form.__init__(self, None, mforms.FormResizable | mforms.FormMinimizable)

        self.set_title("New Schema Privilege Definition")

        self.secman = secman

        box = newBox(False)
        box.set_padding(12)
        box.set_spacing(8)
        self.set_content(box)

        label = newLabel(("Select the Schema for which the user '%s' will have the privileges you want to define." % user).encode('utf-8'))
        box.add(label, False, True)

        panel = newPanel(mforms.TitledBoxPanel)
        panel.set_title("Schema")
        box.add(panel, True, True)
        table = newTable()
        panel.add(table)
        table.set_padding(8)
        table.set_row_count(3)
        table.set_column_count(3)
        table.set_row_spacing(8)
        table.set_column_spacing(4)

        self.schema1 = newRadioButton(mforms.RadioButton.new_id())
        self.schema1.set_active(True)
        self.schema1.add_clicked_callback(self.schema_radio_changed)
        self.schema1.set_text("Any Schema (%)")
        table.add(self.schema1, 0, 1, 0, 1, mforms.HFillFlag)
        table.add(dLabel("This rule will apply to any schema name."), 2, 3, 0, 1, mforms.HFillFlag)

        self.schema2 = newRadioButton(self.schema1.group_id())
        self.schema2.add_clicked_callback(self.schema_radio_changed)
        self.schema2.set_text("Schemas matching pattern or name:")
        table.add(self.schema2, 0, 1, 1, 2, mforms.HFillFlag)
        table.add(dLabel("This rule will apply to schemas that match the given name or pattern.\nYou may use _ and % as wildcards in a pattern.\nEscape these characters with \\ in case you want their literal value."), 2, 3, 1, 2, mforms.HFillFlag)

        self.schema2entry = newTextEntry()
        table.add(self.schema2entry, 1, 2, 1, 2, mforms.HFillFlag|mforms.HExpandFlag)

        self.schema3 = newRadioButton(self.schema1.group_id())
        self.schema3.add_clicked_callback(self.schema_radio_changed)
        self.schema3.set_text("Selected schema:")
        alignbox = mforms.newBox(False)
        alignbox.add(self.schema3, False, False)
        table.add(alignbox, 0, 1, 2, 3, mforms.HFillFlag)
        label = dLabel("Select a specific schema name for the rule to apply to.")
        label.set_text_align(mforms.TopLeft)
        table.add(label, 2, 3, 2, 3, mforms.HFillFlag)

        self.schema3sel = newListBox(False)
        table.add(self.schema3sel, 1, 2, 2, 3, mforms.HFillFlag|mforms.HExpandFlag|mforms.VFillFlag|mforms.VExpandFlag)
        self.schema3sel.add_items(self.secman.escaped_schema_names)

        bbox = newBox(True)
        box.add(bbox, False, True)

        bbox.set_spacing(8)

        self.ok = newButton()
        self.ok.set_text("OK")
        bbox.add_end(self.ok, False, True)

        self.cancel = newButton()
        self.cancel.set_text("Cancel")
        bbox.add_end(self.cancel, False, True)

        self.set_size(850, 500)

        self.schema_radio_changed()

        self.center()

    def schema_radio_changed(self):
        self.schema2entry.set_enabled(self.schema2.get_active())
        self.schema3sel.set_enabled(self.schema3.get_active())


    def run(self):
        if self.run_modal(self.ok, self.cancel):
            if self.schema1.get_active():
                schema = "%"
            elif self.schema2.get_active():
                schema = self.schema2entry.get_string_value()
            else:
                schema = self.schema3sel.get_string_value()

            return schema

        return None

##############################

class SecuritySchemaPrivileges(mforms.Box):
    def __init__(self, owner):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        self._owner = owner
        self.dirty = False

        self.suspend_layout()
        self.set_spacing(8)
        self.set_padding(8)

        self.schema_rights_checks = {}

        self.privs_list = newTreeNodeView(mforms.TreeFlatList)
        self.privs_list.add_column(mforms.StringColumnType, "Schema", 150, True)
        self.privs_list.add_column(mforms.StringColumnType, "Privileges", 800, False)
        self.privs_list.end_columns()
        self.privs_list.add_changed_callback(self.schema_priv_selected)

        self.add(self.privs_list, True, True)

        bbox = newBox(True)
        bbox.set_spacing(8)

        bbox.add(dLabel("Schema and Host fields may use % and _ wildcards.\nThe server will match specific entries before wildcarded ones."), False, True)

        self.add_entry_button = newButton()
        self.add_entry_button.set_text("Add Entry...")
        bbox.add_end(self.add_entry_button, False, True)
        self.add_entry_button.add_clicked_callback(self.add_entry)

        self.del_entry_button = newButton()
        self.del_entry_button.set_text("Delete Entry")
        bbox.add_end(self.del_entry_button, False, True)
        self.del_entry_button.add_clicked_callback(self.del_entry)

        self.add(bbox, False, True)

        self.schema_priv_label = newLabel("")
        self.add(self.schema_priv_label, False, True)

        hbox = newBox(True)
        hbox.set_spacing(8)
        hbox.set_homogeneous(True)

        self.schema_object_privs_panel = panel = newPanel(mforms.TitledBoxPanel)
        panel.set_title("Object Rights")
        box = newBox(False)
        box.set_padding(8)
        self.object_rights_box = box
        panel.add(box)
        hbox.add(panel, True, True)

        self.schema_ddl_privs_panel = panel = newPanel(mforms.TitledBoxPanel)
        panel.set_title("DDL Rights")
        box = newBox(False)
        box.set_padding(8)
        self.ddl_rights_box = box
        panel.add(box)
        hbox.add(panel, True, True)

        self.schema_other_privs_panel = panel = newPanel(mforms.TitledBoxPanel)
        panel.set_title("Other Rights")
        box = newBox(False)
        box.set_padding(8)
        self.other_rights_box = box
        panel.add(box)
        hbox.add(panel, False, True)
        self.add(hbox, False, True)

        bottom_box = newBox(True)
        bottom_box.set_spacing(8)

        if 0:
            img = newImageBox()
            if App.get().get_resource_path("task_warning_mac.png"):
                img.set_image("task_warning_mac.png")
            else:
                img.set_image("task_warning.png")
            bottom_box.add(img, False, True)
            bottom_box.add(dLabel("There are %i schema privilege entries for accounts that don't exist"), False, True)
            purge = newButton()
            purge.set_text("Purge")
            bottom_box.add(purge, False, True)


        self.grant_all = newButton()
        self.grant_all.set_text('Select "ALL"')
        bottom_box.add_end(self.grant_all, False, True)
        self.grant_all.add_clicked_callback(self.grant_all_schema_privs)

        self.revoke_all = newButton()
        self.revoke_all.set_text("Unselect All")
        bottom_box.add_end(self.revoke_all, False, True)
        self.revoke_all.add_clicked_callback(self.revoke_all_schema_privs)

        self.add(bottom_box, False, True)

        self.resume_layout()


    ####

    def update(self):
        if self.schema_rights_checks:
            return

        global_privilege_names = self._owner.owner.secman.global_privilege_names
        for name in SCHEMA_OBJECT_RIGHTS:
            if name not in global_privilege_names:
                continue
            cb = newCheckBox()
            label, desc = PrivilegeInfo.get(name, ("", None))
            cb.set_text(label)
            if desc:
                cb.set_tooltip(desc)
            cb.add_clicked_callback(self.schema_priv_checked)
            self.object_rights_box.add(cb, False, False)
            self.schema_rights_checks[name] = cb

        for name in SCHEMA_DDL_RIGHTS:
            if name not in global_privilege_names:
                continue
            cb = newCheckBox()
            label, desc = PrivilegeInfo.get(name, ("", None))
            cb.set_text(label)
            if desc:
                cb.set_tooltip(desc)
            cb.add_clicked_callback(self.schema_priv_checked)
            self.ddl_rights_box.add(cb, False, False)
            self.schema_rights_checks[name] = cb

        for name in SCHEMA_OTHER_RIGHTS:
            if name not in global_privilege_names:
                continue
            cb = newCheckBox()
            label, desc = PrivilegeInfo.get(name, ("", None))
            cb.set_text(label)
            if desc:
                cb.set_tooltip(desc)
            cb.add_clicked_callback(self.schema_priv_checked)
            self.other_rights_box.add(cb, False, False)
            self.schema_rights_checks[name] = cb


    def schema_priv_checked(self):
        privs = []
        for name, cb in self.schema_rights_checks.iteritems():
            if cb.get_active():
                privs.append(name)
        sel = self.privs_list.get_selected_row()
        if sel >= 0:
            self._owner._selected_user.schema_privs.entries[sel].privileges = set(privs)

            plist = [PrivilegeInfo.get(p, " ")[0] for p in privs]
            plist.sort()
            self.privs_list.node_at_row(sel).set_string(1, ", ".join(plist) or "none")
        self._owner.set_dirty()


    def add_entry(self):
        addform = AddSchemaPrivilegeForm(self._owner.owner.secman, self._owner._selected_user.username)

        schema = addform.run()
        if schema is not None:
            entry = self._owner._selected_user.schema_privs.add_entry(schema, set())

            self.refresh_entry_list(self._owner._selected_user.schema_privs)
            self.show_privileges(entry)

            self.privs_list.select_node(self.privs_list.node_at_row(len(self._owner._selected_user.schema_privs.entries)-1))
            self.schema_priv_selected()

            self._owner.set_dirty()


    def del_entry(self):
        sel = self.privs_list.get_selected_node()
        if not sel:
            return
        self._owner._selected_user.schema_privs.del_entry(self.privs_list.row_for_node(sel))
        sel.remove_from_parent()
        self.schema_priv_selected()
        self._owner.set_dirty()
        
        self.add_entry_button.set_enabled(True)
        self.del_entry_button.set_enabled(True)
        self._owner.refresh_button.set_enabled(True)


    def schema_priv_selected(self):
        sel = self.privs_list.get_selected_node()
        if not sel or not self._owner._selected_user:
            self.show_privileges(None)
        else:
            entry = self._owner._selected_user.schema_privs.entries[self.privs_list.row_for_node(sel)]
            self.show_privileges(entry)


    def grant_all_schema_privs(self):
        for name, cb in self.schema_rights_checks.iteritems():
            if name != "Grant_priv":
                cb.set_active(True)
            else:
                cb.set_active(False)
        self.schema_priv_checked()
        self._owner.set_dirty()


    def revoke_all_schema_privs(self):
        for cb in self.schema_rights_checks.itervalues():
            cb.set_active(False)
        self.schema_priv_checked()
        self._owner.set_dirty()


    def show_user(self, user):
        # make sure that the info is not outdated
        self.set_enabled(user != None)

        self._owner.unset_dirty()

        self.refresh_entry_list(user)

        if user is not None:
            self.add_entry_button.set_enabled(True)
            self.del_entry_button.set_enabled(False)

        self.show_privileges(None)


    def refresh_entry_list(self, user):
        self.privs_list.clear()
        for entry in user and user.entries or []:
            row = self.privs_list.add_node()
            row.set_string(0, entry.db)
            plist = [PrivilegeInfo.get(p, " ")[0] for p in entry.privileges]
            plist.sort()
            row.set_string(1, ", ".join(plist) or "none")



    def show_privileges(self, entry):
        self.schema_object_privs_panel.set_enabled(entry != None)
        self.schema_ddl_privs_panel.set_enabled(entry != None)
        self.schema_other_privs_panel.set_enabled(entry != None)

        if entry:
            db, privs = entry.db, entry.privileges

            text = "The user '%s'@'%s' " % (self._owner._selected_user.username, self._owner._selected_user.host)
            if '_' in db or '%' in db:
                if db == '%':
                    text += "will have the following access rights to any schema:"
                else:
                    text += "will have the following access rights to schemas matching '%s':" % db
            else:
                text += "will have the following access rights to the schema '%s':" % db
            self.schema_priv_label.set_text(text)

            self.del_entry_button.set_enabled(True)

            self.grant_all.set_enabled(True)
            self.revoke_all.set_enabled(True)

            for priv, check in self.schema_rights_checks.iteritems():
                check.set_active(priv in privs)
        else:
            self.schema_priv_label.set_text("")
            self.del_entry_button.set_enabled(False)

            self.grant_all.set_enabled(False)
            self.revoke_all.set_enabled(False)

            for priv, check in self.schema_rights_checks.iteritems():
                check.set_active(False)


    def refresh(self):
        self.show_user(None)



#############################

class SecurityAccount(mforms.Box):
    def __init__(self, owner):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        self.owner = owner
        self.dirty = False

        self._selected_user = None
        self._selected_user_original = None

        self.suspend_layout()
        self.set_padding(8)
        self.set_spacing(8)

        self.splitter = mforms.newSplitter(True)

        bottom_box = newBox(True)
        bottom_box.set_spacing(8)

        self.add_button = newButton()
        self.add_button.set_text("Add Account")
        bottom_box.add(self.add_button, False, True)
        self.add_button.add_clicked_callback(self.add_account)

        #self.dup_button = newButton()
        #self.dup_button.set_text("Duplicate")
        #bottom_box.add(self.dup_button, False, True)
        #self.dup_button.add_clicked_callback(self.dup_account)

        self.del_button = newButton()
        self.del_button.set_text("Delete")
        bottom_box.add(self.del_button, False, True)
        self.del_button.add_clicked_callback(self.del_account)

        self.refresh_button = newButton()
        self.refresh_button.set_text("Refresh")
        self.refresh_button.add_clicked_callback(self.owner.refresh)
        bottom_box.add(self.refresh_button, False, True)

        self.save_button = newButton()
        self.save_button.set_text("Apply")
        bottom_box.add_end(self.save_button, False, True)
        self.save_button.add_clicked_callback(self.commit)

        self.revert_button = newButton()
        self.revert_button.set_text("Revert")
        bottom_box.add_end(self.revert_button, False, True)
        self.revert_button.add_clicked_callback(self.revert)

        if self.owner.ctrl_be.target_version and self.owner.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6, 7):
            self.expire_button = newButton()
            self.expire_button.set_text("Expire Password")
            self.expire_button.set_tooltip("Force user to change password after next login. The user will be unable to issue any command other than SET PASSWORD.")
            bottom_box.add_end(self.expire_button, False, True)
            self.expire_button.add_clicked_callback(self.expire)
        else:
            self.expire_button = None

        self.revoke_all_button = newButton()
        self.revoke_all_button.set_text("Revoke All Privileges")
        bottom_box.add_end(self.revoke_all_button, False, True)
        self.revoke_all_button.add_clicked_callback(self.revoke_all)
        self.revoke_all_button.set_tooltip("Immediately remove all privileges from the account, from every object at all levels.\nThe account itself will be left untouched and logins will still be possible.")

        account_list_box = newBox(False)
        account_list_box.set_spacing(8)
        account_list_box.set_size(220, -1)

        label = newLabel("User Accounts")
        account_list_box.add(label, False, True)

        #searchbox = TextEntry(SearchEntry)
        #account_list_box.add(searchbox, False, True)

        self.user_list = newTreeNodeView(mforms.TreeFlatList)
        self.user_list.add_column(mforms.StringColumnType, "User", 120, False)
        self.user_list.add_column(mforms.StringColumnType, "From Host", 200, False)
        self.user_list.end_columns()
        self.user_list.add_changed_callback(self.user_selected)
        self.user_list.set_allow_sorting(True)
        account_list_box.add(self.user_list, True, True)
        self.splitter.add(account_list_box, 100, True)

        # Right part

        self.content_box = abox = newBox(False)
        abox.set_spacing(8)

        self.account_label = newLabel("Select an account to edit or click Add Account to create a new one")
        self.account_label.set_style(mforms.BoldStyle)

        abox.add(self.account_label, False, True)

        tabView = newTabView(False)
        self.inner_tabview = tabView

        # Login Tab
        vbox = newBox(False)
        vbox.set_spacing(12)
        vbox.set_padding(12)

        table = newTable()
        vbox.add(table, False, True)

        table.set_row_count(8)
        table.set_column_count(3)
        table.set_row_spacing(8)
        table.set_column_spacing(8)

        self.username = newTextEntry()
        self.username.set_size(180, -1)
        self.username.add_changed_callback(self.set_dirty)
        self.username.set_max_length(16) # max username length for mysql
        self.password = newTextEntry(mforms.PasswordEntry)
        self.password.set_size(180, -1)
        self.password_advice = 'Consider using a password with 8 or more characters with\nmixed case letters, numbers and punctuation marks.'
        self.password_label = dLabel(self.password_advice)
        self.password_validator = ThreadedInputValidator(self, self.password, self.password_label, self.owner.ctrl_be)
        self.password.add_changed_callback(self.password_validator.callback)
        self.confirm = newTextEntry(mforms.PasswordEntry)
        self.confirm.set_size(180, -1)
        self.confirm.add_changed_callback(self.set_dirty)

        self.hostlimithost = newTextEntry()
        self.hostlimithost.set_size(180, -1)
        self.hostlimithost.add_changed_callback(self.hostlimithost_changed)

        table.add(rLabel("Login Name:"), 0, 1, 0, 1, mforms.HFillFlag)
        table.add(self.username, 1, 2, 0, 1, mforms.HFillFlag)
        table.add(dLabel("You may create multiple accounts with the same name\nto connect from different hosts."), 2, 3, 0, 1, mforms.HFillFlag|mforms.HExpandFlag)

        self.has_extra_plugins = len([p for p in self.active_plugins]) > 0

        if self.has_extra_plugins:
            self.auth_type_label = rLabel("Authentication Type:")
            table.add(self.auth_type_label, 0, 1, 1, 2, mforms.HFillFlag)
            self.auth_type_sel = newSelector()
            self.auth_type_sel.set_size(180, -1)
            table.add(self.auth_type_sel, 1, 2, 1, 2, mforms.HFillFlag)
            table.add(dLabel("\nFor the standard password and/or host based authentication,\nselect 'Standard'.\n"), 2, 3, 1, 2, mforms.HFillFlag)
            self.auth_type_sel.add_changed_callback(self.auth_type_changed)
            self.auth_type_list = []
            for plugin in self.active_plugins:
                if AUTHENTICATION_PLUGIN_TYPES.has_key(plugin):
                    self.auth_type_sel.add_item(AUTHENTICATION_PLUGIN_TYPES[plugin]["name"])
                else:
                    self.auth_type_sel.add_item(plugin)
                self.auth_type_list.append(plugin)
        else:
            self.auth_type_list = None
            self.auth_string_param = None

        self.hostlimithost_caption = rLabel("Limit to Hosts Matching:")
        table.add(self.hostlimithost_caption, 0, 1, 2, 3, mforms.HFillFlag)
        table.add(self.hostlimithost, 1, 2, 2, 3, mforms.HFillFlag)
        self.hostlimit_box = newBox(True)
        self.hostlimithost_valid_icon = newImageBox()
        self.hostlimit_box.add(self.hostlimithost_valid_icon, False, False)
        self.password_caption2 = dLabel("% and _ wildcards may be used")
        self.hostlimit_box.add(self.password_caption2, True, True)
        table.add(self.hostlimit_box, 2, 3, 2, 3, mforms.HFillFlag|mforms.HExpandFlag)

        self.password_caption = rLabel("Password:")
        table.add(self.password_caption, 0, 1, 3, 4, mforms.HFillFlag)
        table.add(self.password, 1, 2, 3, 4, mforms.HFillFlag)
        self.password_caption2 = dLabel("Type a password to reset it.")
        table.add(self.password_caption2, 2, 3, 3, 4, mforms.HFillFlag|mforms.HExpandFlag)
        table.add(self.password_label, 1, 3, 4, 5, mforms.HFillFlag)
        self.confirm_caption = rLabel("Confirm Password:")
        table.add(self.confirm_caption, 0, 1, 5, 6, mforms.HFillFlag)
        table.add(self.confirm, 1, 2, 5, 6, mforms.HFillFlag)
        self.confirm_caption2 = dLabel("Enter password again to confirm.")
        table.add(self.confirm_caption2, 2, 3, 5, 6, mforms.HFillFlag|mforms.HExpandFlag)

        table.add(newLabel(""), 1, 2, 6, 7, mforms.HFillFlag)

        if self.has_extra_plugins:
            self.auth_string_param = newTextEntry()
            self.auth_string_param.add_changed_callback(self.set_dirty)
            self.auth_string_label = rLabel("Authentication String:")
            self.auth_string_desc = dLabel("Authentication plugin specific parameters.")
            table.add(self.auth_string_label, 0, 1, 7, 8, mforms.HFillFlag)
            table.add(self.auth_string_param, 1, 2, 7, 8, mforms.HFillFlag)
            table.add(self.auth_string_desc, 2, 3, 7, 8, mforms.HFillFlag|mforms.HExpandFlag)
            self.auth_string_help = newLabel("")
            vbox.add(self.auth_string_help, False, True)
            
        self.bottom_message_hbox = newBox(True)
        self.bottom_message_hbox.set_spacing(8)        
        
        self.bottom_message_caption = newLabel('')
        self.bottom_message_caption.set_color('#aa3333')
        
        self.upgrade_account_button = newButton()
        self.upgrade_account_button.set_text("Upgrade")
        self.upgrade_account_button.set_tooltip("Upgrade the authentication type of the account, from the obsolete format to the newer, safer one. You must set the password for the account before clicking this button.")
        self.upgrade_account_button.add_clicked_callback(self.upgrade_account)
        
        self.bottom_message_hbox.add(self.bottom_message_caption, True, True)
        self.bottom_message_hbox.add(self.upgrade_account_button, False, True)        
        vbox.add_end(self.bottom_message_hbox, False)
        self.bottom_message_hbox.show(False)

        tabView.add_page(vbox, "Login")

        # Account Limits Tab
        table = newTable()

        table.set_padding(12)
        table.set_column_spacing(8)
        table.set_row_spacing(8)
        table.set_row_count(4)
        table.set_column_count(3)

        table.add(rLabel("Max. Queries:"), 0, 1, 0, 1, mforms.HFillFlag)
        self.max_questions = newTextEntry()
        self.max_questions.set_size(60, -1)
        self.max_questions.add_changed_callback(self.set_dirty)
        table.add(self.max_questions, 1, 2, 0, 1, mforms.HFillFlag)
        table.add(dLabel("Number of queries the account can execute within one hour."), 2, 3, 0, 1, mforms.HFillFlag|mforms.HExpandFlag)

        table.add(rLabel("Max. Updates:"), 0, 1, 1, 2, mforms.HFillFlag)
        self.max_updates = newTextEntry()
        self.max_updates.set_size(60, -1)
        self.max_updates.add_changed_callback(self.set_dirty)
        table.add(self.max_updates, 1, 2, 1, 2, mforms.HFillFlag)
        table.add(dLabel("Number of updates the account can execute within one hour."), 2, 3, 1, 2, mforms.HFillFlag|mforms.HExpandFlag)

        table.add(rLabel("Max. Connections:"), 0, 1, 2, 3, mforms.HFillFlag)
        self.max_connections = newTextEntry()
        self.max_connections.set_size(60, -1)
        self.max_connections.add_changed_callback(self.set_dirty)
        table.add(self.max_connections, 1, 2, 2, 3, mforms.HFillFlag)
        table.add(dLabel("The number of times the account can connect to the server per hour."), 2, 3, 2, 3, mforms.HFillFlag|mforms.HExpandFlag)

        table.add(rLabel("Concurrent Connections:"), 0, 1, 3, 4, mforms.HFillFlag)
        self.max_uconnections = newTextEntry()
        self.max_uconnections.set_size(60, -1)
        self.max_uconnections.add_changed_callback(self.set_dirty)
        table.add(self.max_uconnections, 1, 2, 3, 4, mforms.HFillFlag)
        table.add(dLabel("The number of simultaneous connections to the server the account can have."), 2, 3, 3, 4, mforms.HFillFlag|mforms.HExpandFlag)
        
        tabView.add_page(table, "Account Limits")

        # Administrative Roles Tab

        box = newBox(False)

        lbox = newBox(True)
        box.add(lbox, True, True)
        lbox.set_spacing(12)
        lbox.set_padding(12)

        self.role_list = newTreeNodeView(mforms.TreeFlatList)
        self.role_list.add_column(mforms.CheckColumnType, "", 30, True)
        self.role_list.add_column(mforms.StringColumnType, "Role", 150, False)
        self.role_list.add_column(mforms.StringColumnType, "Description", 300, False)
        self.role_list.end_columns()
        lbox.add(self.role_list, True, True)
        self.role_list.set_cell_edited_callback(self.role_list_toggled)

        self.role_priv_list = newTreeNodeView(mforms.TreeFlatList)
        self.role_priv_list.add_column(mforms.CheckColumnType, '', 30, True)
        self.role_priv_list.add_column(mforms.StringColumnType, "Global Privileges", 180, False)
        self.role_priv_list.end_columns()
        self.role_priv_list.set_size(220, -1)
        self.role_priv_list.set_cell_edited_callback(self.role_priv_list_toggled)
        lbox.add(self.role_priv_list, False, True)
        tabView.add_page(box, "Administrative Roles")


        self.schema_privs = SecuritySchemaPrivileges(self)
        tabView.add_page(self.schema_privs, "Schema Privileges")

        abox.add(tabView, True, True)
        self.splitter.add(abox, 200)
        self.add(self.splitter, True, True)
        self.add(bottom_box, False, True)

        self.resume_layout()

        mforms.Utilities.add_timeout(0.1, lambda self=self: self.splitter.set_position(240))

        self.user_selected()

    def shutdown(self):
       self.password_validator.shutdown() 

    @property
    def active_plugins(self):
        return [name for name, ptype in self.owner.ctrl_be.server_active_plugins if ptype == "AUTHENTICATION"]

    def selected_plugin_type(self):
        if self.auth_type_list:
            return self.auth_type_list[max(self.auth_type_sel.get_selected_index(), 0)]
        return None

    def update_enable_state_for_auth_type(self, auth_type, is_new_user):
        info = AUTHENTICATION_PLUGIN_TYPES.get(auth_type)
        if not info:
            info = AUTHENTICATION_PLUGIN_TYPES["*"]

        if info:
            self.password_caption.set_enabled(info["enable_password"])
            self.password_caption2.set_enabled(info["enable_password"])
            self.password_label.set_enabled(info["enable_password"])
            self.password.set_enabled(info["enable_password"])
            self.confirm.set_enabled(info["enable_password"])
            self.confirm_caption.set_enabled(info["enable_password"])
            self.confirm_caption2.set_enabled(info["enable_password"])
            self.hostlimithost.set_enabled(info["enable_host"])
            self.hostlimithost_caption.set_enabled(info["enable_host"])

            if self.auth_string_param:
                if info["auth_string_label"]:
                    self.auth_string_help.show(True)
                    self.auth_string_help.set_text(info["auth_string_label"])
                    self.auth_string_label.show(True)
                    self.auth_string_param.show(True)
                    self.auth_string_desc.show(True)
                    if is_new_user:
                        self.auth_string_label.set_enabled(True)
                        self.auth_string_param.set_enabled(True)
                    else:
                        self.auth_string_label.set_enabled(False)
                        self.auth_string_param.set_enabled(False)
                else:
                    self.auth_string_help.show(False)
                    self.auth_string_label.show(False)
                    self.auth_string_param.show(False)
                    self.auth_string_desc.show(False)

    def set_hostlimithost(self, value):
        self.hostlimithost.set_value(value)
        self.validate_hostlimithost()

    def hostlimithost_changed(self):
        self.validate_hostlimithost()
        self.set_dirty()


    def auth_type_changed(self):
        if not self.has_extra_plugins:
            return

        auth_type = self.selected_plugin_type()
        # this method is only called for new users (because auth-type can't be changed for existing users)
        self.update_enable_state_for_auth_type(auth_type, True)

        info = AUTHENTICATION_PLUGIN_TYPES.get(auth_type)
        if not info:
            info = AUTHENTICATION_PLUGIN_TYPES["*"]

        if info:
            if not info["enable_password"]:
                self.password.set_value("")
                self.confirm.set_value("")

            if not info["enable_host"]:
                self.set_hostlimithost("%")

        self.set_dirty()

    def refresh_role_list(self):
        if self._selected_user:
            self.role_list.clear()
            roles = self._selected_user.admin_roles

            TheRoles = SecurityAdminRoles if not self._selected_user.is_custom_role_needed else SecurityAdminRoles + [ ('Custom', 'custom role', []) ]

            for name, desc, privs in TheRoles:
                row = self.role_list.add_node()
                row.set_bool(0, name in roles)
                row.set_string(1, name)
                row.set_string(2, desc)

    def role_priv_list_toggled(self, node, col, value):
        if self._selected_user:
            node.set_bool(col, value == '1')
            priv = node.get_string(1)
            self._selected_user.toggle_priv(PrivilegeReverseDict[priv], value == '1')

            self.refresh_role_list()

            self.set_dirty()

    def role_list_toggled(self, node, col, value):
        if self._selected_user:
            node.set_int(col, int(value))

            role = node.get_string(1)
            self._selected_user.toggle_role(role, value == "1")

            self.refresh_role_list()

            self.set_dirty()

            self.refresh_priv_list()


    def host_limit_clicked(self):
        self.hostlimithost.set_enabled(self.hostlimit.get_active())
        self.set_dirty()


    def user_selected(self):
        sel = self.user_list.get_selected_node()

        self._selected_user = None
        self._selected_user_original = None
        self.show_user(None)

        #self.dup_button.set_enabled(False)
        self.del_button.set_enabled(False)

        if sel:
            user, host = eval(sel.get_tag())
            self.account_label.set_text("Details for account %s@%s" % (user, host))

            if self.owner.secman.is_zombie(user, host):
                self.show_zombie_user(user, host)
            else:
                self.owner.secman.async_get_account(self.show_user, user, host)
        else:
            self.account_label.set_text("Select an account to edit or click [Add Account] to create a new one")


    def show_zombie_user(self, user, host):
        self.content_box.set_enabled(True)
        self.inner_tabview.show(False)
        if self.expire_button:
            self.expire_button.set_enabled(False)
        #self.dup_button.set_enabled(False)
        self.del_button.set_enabled(True)

        privs = self.owner.secman.get_zombie_privs(user, host)

        self.account_label.set_text("Account %s@%s does not exist but it still has privileges defined for the following objects:\n    %s\n\nClick the [Delete] button to completely remove the account." % (user, host, "\n    ".join(privs)))


    def show_user(self, user):
        self.inner_tabview.show(True)

        sel = self.user_list.get_selected_node()
        if not sel and user:
            return
        if user and eval(sel.get_tag())[0] != user.username:
            return

        self.content_box.set_enabled(user != None)
        self.revoke_all_button.set_enabled(user != None and user.is_commited)
        self.unset_dirty()

        self._selected_user = user
        self._selected_user_original = user and user.snapshot_for_revert()

        if user:
            #self.dup_button.set_enabled(True)
            self.del_button.set_enabled(True)
            if self.expire_button:
                self.expire_button.set_enabled(user.is_commited and not user.password_expired)

            self.username.set_value(user.username)
            self.password.set_value(user.password or "")
            self.confirm.set_value(user.password or "")

            self.set_hostlimithost(user.host)

            if self.auth_type_list:
                try:
                    index = self.auth_type_list.index(user.auth_plugin or DEFAULT_AUTH_PLUGIN)
                except ValueError:
                    mforms.Utilities.show_warning("Invalid Authentication Plugin",
                              "User %s has plugin type %s, which is not listed as a known authentication plugin by the server." % (user.formatted_name(), user.auth_plugin),
                              "OK", "", "")
                    return

                self.auth_type_sel.set_selected(index)
                # changing auth type not supported atm
                self.auth_type_sel.set_enabled(not user.is_commited)
                self.auth_string_param.set_value(user.auth_string or "")
                self.update_enable_state_for_auth_type(user.auth_plugin or None, not user.is_commited)

            self.max_questions.set_value(str(user.max_questions))
            self.max_updates.set_value(str(user.max_updates))
            self.max_connections.set_value(str(user.max_connections))
            self.max_uconnections.set_value(str(user.max_user_connections))

            self.refresh_priv_list()
            self.refresh_role_list()

            self.schema_privs.show_user(user.schema_privs)
        else:
            self.username.set_value("")
            self.password.set_value("")
            self.confirm.set_value("")
            if self.expire_button:
                self.expire_button.set_enabled(False)

            self.set_hostlimithost("")

            if self.auth_string_param:
                self.auth_string_param.set_value("")

            self.max_questions.set_value("")
            self.max_updates.set_value("")
            self.max_connections.set_value("")
            self.max_uconnections.set_value("")

            for i in range(self.role_list.count()):
                role_list_node = self.role_list.node_at_row(i)
                if role_list_node:
                    self.role_list.node_at_row(i).set_bool(0, False)

            self.schema_privs.show_user(None)

        self.password_label.set_text(self.password_advice)
        self.password_label.set_color('#000000')
        
        self.setup_bottom_message_box(user)           


    def _find_user_position(self, user, host):
        users_count = len(self.owner.secman.account_names)
        for row in range(users_count):
            node = self.user_list.node_at_row(row)
            if (user, host) == ( node.get_string(0), node.get_string(1) ):
                return row
        return users_count - 1  # Control shouldn't get here

    def add_account(self):
        account = self.owner.secman.create_account()
        self.refresh()
        self.inner_tabview.set_active_tab(0)
        pos = self._find_user_position(account.username, account.host)
        if pos is not None and pos >= 0:
            self.user_list.select_node(self.user_list.node_at_row(pos))
        self.user_selected()
        self.set_dirty()
        self.add_button.set_enabled(False)
        self.del_button.set_enabled(False)
        self.refresh_button.set_enabled(False)
        
    def upgrade_account(self):
        def generate_password(length = 8):    
            chars = string.ascii_letters + string.digits + '!@#$%^&*()'
            random.seed = (os.urandom(1024))
            return ''.join(random.choice(chars) for i in range(length))
        
        def change_password(password, confirm_password):
            self._selected_user.password = password
            self._selected_user.confirm_password = confirm_password
            try:
                self._selected_user.upgrade_password_format()
            except WBSecurityValidationError, exc:
                Utilities.show_error("Upgrade", str(exc), "OK", "", "")
                return
            except Exception, exc:
                import traceback
                log_error("Exception while upgrading account auth type: %s\n" % traceback.format_exc())
                Utilities.show_error("Error Upgrading Authentication Method", str(exc), "OK", "", "")
                return

        if self.password.get_string_value() == self._selected_user._orig_password:
            upgrade_account_msg = ["A password must be provided for the account (either the original or a new one) before it can be upgraded."]
            reset_to_expire_caption = ""
            if self.owner.ctrl_be.target_version and self.owner.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6, 0):
                upgrade_account_msg.append("Click [Reset to Expired] to set a random password and expire it, so the user will have to reset it next time they login.")
                reset_to_expire_caption = "Reset to Expired"
            result = Utilities.show_warning("Upgrade Authentication Method", "\n".join(upgrade_account_msg), "OK", "", reset_to_expire_caption)
            if result == mforms.ResultOther:
                gen_password = generate_password()                
                change_password(gen_password, gen_password)
                Utilities.show_message("Random Generated Password", 'Password changed to: %s' % gen_password, "OK", "", "")
                self.expire()
            else:
                return
        else:
            change_password(self.password.get_string_value(), self.confirm.get_string_value())
        
        self.reload_user(False)


    def dup_account(self):
        if self._selected_user:
            account = self.owner.secman.copy_account(self._selected_user)
            self.refresh()
            pos = self._find_user_position(account.username, account.host)
            if pos and pos >= 0:
                self.user_list.select_node(self.user_list.node_at_row(pos))
            self.user_selected()


    def del_account(self):
        if self._selected_user:
            if not self._selected_user.is_commited or Utilities.show_message("Delete Account",
                  "The account '%s' will be permanently removed. Please confirm."%(self._selected_user.formatted_name()),
                  "Delete", "Cancel", "") == mforms.ResultOk:
                the_name = self._selected_user.formatted_name()
                try:
                    self.owner.secman.delete_account(self._selected_user)
                except Exception, e:
                    log_error("Exception while removing account: %s\n" % str(e))
                    title, message = e.args[:2] if len(e.args) > 1 else ('Error:', str(e))
                    Utilities.show_error("Delete account", '%s\n%s' % (title, message), 'OK', '', '')
                    return
                self._selected_user = None
                self._selected_user_original = None
                self.owner.do_refresh()
                self.user_selected()
                mforms.App.get().set_status_text("Account '%s' was deleted" % the_name)
        else:
            user = self.user_list.get_selected_node()
            if user:
                username, host = eval(user.get_tag())
                try:
                    self.owner.secman.do_delete_account(username, host)
                except Exception, e:
                    log_error("Exception while removing zombi account: %s\n" % str(e))
                    title, message = e.args[:2] if len(e.args) > 1 else ('Error:', str(e))
                    Utilities.show_error("Delete account", '%s\n%s' % (title, message), 'OK', '', '')
                    return
                user.remove_from_parent()

        self.add_button.set_enabled(True)
        self.del_button.set_enabled(True)
        self.refresh_button.set_enabled(True)


    def refresh(self):
        selected_row = None

        su = self._selected_user
        suo = self._selected_user_original
        self.user_list.freeze_refresh()
        self.user_list.clear()
        for user, host in self.owner.secman.account_names:
            row = self.user_list.add_node()
            if self.owner.secman.is_zombie(user, host):
                row.set_string(0, "(!) "+(user or "<anonymous>"))
            else:
                row.set_string(0, user or "<anonymous>")
            row.set_string(1, host)
            row.set_tag(repr((user, host)))
            if su and (user == su.username and host == su.host):
                selected_row = row

        self.user_list.thaw_refresh()

        self._selected_user = su
        self._selected_user_original = suo

        if selected_row is not None:
            self.user_list.select_node(selected_row)
            self.user_selected()

        self.password_label.set_text(self.password_advice)
        self.password_label.set_color('#000000')
        
        self.setup_bottom_message_box(su) 


    def refresh_priv_list(self):
        self.role_priv_list.clear()
        if self._selected_user:
            all_supported_privs = sorted( [val[0] for key, val in PrivilegeInfo.iteritems() if key in self.owner.secman.global_privilege_names] )
            privs = self._selected_user.raw_privilege_names
            for priv in all_supported_privs:
                row = self.role_priv_list.add_node()
                row.set_bool(0, priv in privs)
                row.set_string(1, priv)

    def validate_hostlimithost(self):
        host = self.hostlimithost.get_string_value()
        self.valid_name = True
        self.valid_ipv6 = False
        if len(host) > 255:
            self.valid_name = False

        subnet_mask = ''
        if host[-1:] == ".":
            host = host[:-1]
        elif '/' in host:
            host, _, subnet_mask = host.rpartition('/')
            if not subnet_mask:
                self.valid_name = False

        if self.valid_name:
            allowed = re.compile(r"^(?!-)[\.A-Z%_-]{1,63}(?<!-)$", re.IGNORECASE)
            allowed_ipv4 = re.compile(r"^(((%|_)?|25[0-5%_]|(%|_)?|2[0-4%_][0-9%_]|[01%_]?[0-9%_][0-9%_]?)\.){0,3}((%|_)?|25[0-5%_]|2[0-4%_][0-9%_]|[01%_]?[0-9%_][0-9%_]?)$")
            allowed_ipv6 = re.compile(r"^\s*(?!.*::.*::)(?:(?!:)|:(?=:))(?:[0-9a-f%_]{0,4}(?:(?<=::)|(?<!::):)){6}(?:[0-9a-f%_]{0,4}(?:(?<=::)|(?<!::):)[0-9a-f%_]{0,4}(?:(?<=::)|(?<!:)|(?<=:)(?<!::):)|(?:25[0-4%_]|2[0-4%_]\d|1\d\d|[1-9%_]?\d)(?:\.(?:25[0-4%_]|2[0-4%_]\d|1\d\d|[1-9%_]?\d)){3})\s*$"
                                      , re.IGNORECASE)
            self.valid_ipv6 = allowed_ipv6.match(host)
            self.valid_name = allowed.match(host) or allowed_ipv4.match(host) or self.valid_ipv6

        if self.valid_name and subnet_mask:
            self.valid_name = ( (subnet_mask.isdigit() and 0 <= int(subnet_mask) <= 32) or
                                (subnet_mask.isdigit() and 0 <= int(subnet_mask) <= 128 and self.valid_ipv6) or
                                (re.match(r'\d{1,3}(\.\d{1,3}){3}', subnet_mask) and            # 255.255.254.0 is a valid netmask
                                 all( int(item) <= 255 for item in subnet_mask.split('.') ) )
                              )

        if self.valid_name: 
            self.hostlimithost_valid_icon.show(False)
        else:
            #self.unset_dirty()
            self.hostlimithost_valid_icon.set_image("mini_error.png")
            self.hostlimithost_valid_icon.show()
            self.hostlimithost_valid_icon.set_tooltip("Host name contains incorrect characters")

    def set_dirty(self):
        self.save_button.set_enabled(True)
        self.revert_button.set_enabled(True)
        self.user_list.set_enabled(False)
        self.dirty = True


    def unset_dirty(self):
        self.save_button.set_enabled(False)
        self.revert_button.set_enabled(False)
        self.user_list.set_enabled(True)
        self.dirty = False

  
    def expire(self):
        if self._selected_user:
            try:
                self._selected_user.expire_password()
            except Exception, e:
                    title, message = e.args[:2] if len(e.args) > 1 else ('Error:', str(e))
                    Utilities.show_error(title, message, 'OK', '', '')
        self.refresh()

  
    def revoke_all(self):
        if self._selected_user:
            if Utilities.show_message("Revoke All Privileges",
                  "Please confirm revokation of all privileges for the account '%s'@'%s'.\nNote: the account itself will be maintained."%(self._selected_user.username, self._selected_user.host),
                  "Revoke", "Cancel", "") == mforms.ResultOk:
                try:
                    self._selected_user.revoke_all()
                    self._selected_user.load(self._selected_user.username, self._selected_user.host)
                    self.show_user(self._selected_user)
                except Exception, e:
                    title, message = e.args[:2] if len(e.args) > 1 else ('Error:', str(e))
                    Utilities.show_error(title, message, 'OK', '', '')


    def revert(self):
        if self._selected_user_original:
            self.show_user(self.owner.secman.revert_account(self._selected_user, self._selected_user_original))
        if not self._selected_user.is_commited:
            self.owner.secman.delete_account(self._selected_user)
        self.refresh()
        if not self.user_list.get_selected_node():
            self.user_selected()

        self.add_button.set_enabled(True)
        self.del_button.set_enabled(True)
        self.refresh_button.set_enabled(True)

    def commit(self):
        if self._selected_user:
            username = self.username.get_string_value()
            host = self.hostlimithost.get_string_value()
            if not self.valid_name:
                Utilities.show_error('Invalid host specification',
                    'The host specification "%s" is not valid. Please correct it and try again.' % host,
                                    'OK', '', '' )

            is_new_user = not self._selected_user.is_commited

            password_unneeded = False
            self.password_label.set_text("Password is expired. User must change password to use the account." if self._selected_user.password_expired else self.password_advice)
            plugin_info = AUTHENTICATION_PLUGIN_TYPES.get(self.selected_plugin_type(), {})
            if self.has_extra_plugins and not plugin_info.get("enable_password", True):
                password_unneeded = True
            if is_new_user and not self.password.get_string_value() and not password_unneeded:
                if Utilities.show_warning("Save Account Changes",
                        "It is a security hazard to create an account with no password.\nPlease confirm creation of '%s'@'%s' with no password."%(username, host),
                        "Create", "Cancel", "") != mforms.ResultOk:
                    return
            
            lcase_host = host.lower()
            if lcase_host != host:
                if Utilities.show_message_and_remember("Save Account Changes",
                                          "MySQL only allows lowercase characters for hostnames, the account host will be updated accordingly.",
                                          "Ok", "", "","wb.admin.warn_ucase_hostnames", "Don't show this message again"):
                    self.hostlimithost.set_value(lcase_host)

            self._selected_user.username = self.username.get_string_value()
            self._selected_user.password = self.password.get_string_value()
            self._selected_user.confirm_password = self.confirm.get_string_value()
            #if self.hostlimit.get_active():
            self._selected_user.host = self.hostlimithost.get_string_value()
           # else:
           #     self._selected_user.host = "%"

            try:
                self._selected_user.max_questions = int(self.max_questions.get_string_value())
                if self._selected_user.max_questions < 0: raise ValueError
            except ValueError:
                Utilities.show_error('Wrong Value for Max. Queries',
                        'Cannot convert "%s" to a valid non-negative integer.\nPlease correct this value and try again.' % self.max_questions.get_string_value(),
                        'OK', '', '')
                return

            try:
                self._selected_user.max_updates = int(self.max_updates.get_string_value())
                if self._selected_user.max_updates < 0: raise ValueError
            except ValueError:
                Utilities.show_error('Wrong Value for Max. Updates',
                        'Cannot convert "%s" to a valid non-negative integer.\nPlease correct this value and try again.' % self.max_updates.get_string_value(),
                        'OK', '', '')
                return

            try:
                self._selected_user.max_connections = int(self.max_connections.get_string_value())
                if self._selected_user.max_connections < 0: raise ValueError
            except ValueError:
                Utilities.show_error('Wrong Value for Max. Connections',
                        'Cannot convert "%s" to a valid non-negative integer.\nPlease correct this value and try again.' % self.max_connections.get_string_value(),
                        'OK', '', '')
                return
            try:
                self._selected_user.max_user_connections = int(self.max_uconnections.get_string_value())
                if self._selected_user.max_user_connections < 0: raise ValueError
            except ValueError:
                Utilities.show_error('Wrong Value for Concurrent Connections',
                        'Cannot convert "%s" to a valid non-negative integer.\nPlease correct this value and try again.' % self.max_uconnections.get_string_value(),
                        'OK', '', '')
                return
            
            

            if is_new_user and self.has_extra_plugins:
                self._selected_user.auth_plugin = self.selected_plugin_type()

            self._selected_user.auth_string = None
            if self._selected_user.auth_plugin and AUTHENTICATION_PLUGIN_TYPES[self._selected_user.auth_plugin]["auth_string_label"]:
                self._selected_user.auth_string = self.auth_string_param.get_string_value()

            try:
                self._selected_user.save()
            except WBSecurityValidationError, exc:
                Utilities.show_error("Save Account Changes",
                      str(exc), "OK", "", "")
                return
            except PermissionDeniedError, exc:
                Utilities.show_error("Permission Errors",
                      str(exc), "OK", "", "")
                return
            except Exception, exc:
                import traceback
                log_error("Exception while saving account: %s\n" % traceback.format_exc())
                Utilities.show_error("Error Saving Account",
                      str(exc), "OK", "", "")
                return

            self.reload_user(is_new_user)

        self.add_button.set_enabled(True)
        self.del_button.set_enabled(True)
        self.refresh_button.set_enabled(True)


    def update(self):
        self.schema_privs.update()

    def setup_bottom_message_box(self, user):
        if not user:
            return               

        caption = ''
        if user.old_authentication:
            caption = '''This account is using the pre-mysql-4.1.1 password hashing type.
The user will not be able to login if the secure_auth option is enabled.
Please click [Upgrade Account] to fix that.
Either the account password must be provided to reset it
or a new password must be supplied.'''
        elif user.password_expired:
            caption = 'Password is expired. User must change password to use the account.'
        elif not user.username:
            caption = 'This is an anonymous account. It is usually advisable to delete this account.'
        elif user.blank_password :
            caption = 'No password is set for this account.'
        
        self.bottom_message_caption.set_text(caption)
        self.bottom_message_hbox.show(user.old_authentication or user.password_expired or user.blank_password or not user.username)
        self.upgrade_account_button.show(user.old_authentication) 


    def reload_user(self, is_new_user):
        the_name = self._selected_user.formatted_name()
        self._selected_user.load(self._selected_user.username, self._selected_user.host)
        self.unset_dirty()
        self.owner.refresh()
        if is_new_user:
            mforms.App.get().set_status_text("Created account '%s'" % the_name)
        else:
            mforms.App.get().set_status_text("Updated account '%s'" % the_name)




#############################


class WbAdminSecurity(mforms.Box):
    _schema_priv_entries = []
    ui_created = False

    @classmethod
    def wba_register(cls, admin_context):
        admin_context.register_page(cls, "wba_management", "Users and Privileges")

    @classmethod
    def identifier(cls):
        return "admin_manage_privs"

    def shutdown(self):
        self.account_tab.shutdown()

    def __init__(self, ctrl_be, server_profile, main_view):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        self.ctrl_be = ctrl_be
        self.secman  = None
        self.server_profile = server_profile
        self.main_view = main_view


    def create_ui(self):
        self.suspend_layout()

        self.set_padding(12)
        self.set_spacing(8)

        self.heading = make_panel_header("title_users.png", self.server_profile.name, "Users and Privileges")
        self.add(self.heading, False, True)

        self.warning = not_running_warning_label()
        self.add(self.warning, False, True)

        self.account_tab = SecurityAccount(self)
        self.add(self.account_tab, True, True)

        self.resume_layout()


    def show_no_permission(self):
        self.warning.set_text("\n\n\n\nThe account you are currently using does not have sufficient privileges to make changes to MySQL users and privileges.")
        self.warning.show(True)
        self.account_tab.show(False)


    def update_ui(self):
        if self.ctrl_be.is_sql_connected():
            self.secman = AdminSecurity(self.ctrl_be)

            self.warning.show(False)
            self.account_tab.show(True)
        else:
            self.secman = None

            self.warning.show(True)
            self.account_tab.show(False)



    def page_activated(self):
        if not self.ui_created:
            self.create_ui()
            self.ui_created = True
        self.update_ui()
        self.refresh()

        # Ask the user to remove anonymous accounts if present
        if self.ctrl_be.is_sql_connected():
            try:
                anon_accounts = [ (user, host) for user, host in self.secman.account_names if user=='']
                if anon_accounts and not self.__dict__.get('already_asked_for_anon_accounts', False):
                    logged_username = self.ctrl_be.server_profile.db_connection_params.parameterValues['userName']
                    logged_servername = self.ctrl_be.server_profile.db_connection_params.hostIdentifier
                    privs = self.secman.get_valid_privileges()
            except:
                privs = []

                if 'DELETE' in privs or 'CREATE USER' in privs:
                    if Utilities.show_message_and_remember('Anonymous accounts detected',
                        'Anonymous accounts were detected in the server %s.\nAnonymous accounts can cause great confusion and are also a potential security issue and are advised to be removed. Would you like Workbench to delete them now?.' % logged_servername,
                        'Delete', 'Leave Accounts', '',
                        'wb.admin.delete_anonymous_accounts:' + logged_username + '@' + logged_servername,
                        "Don't show this message again") == mforms.ResultOk:
                        for name, host in anon_accounts:
                            self.secman.async_get_account(self.secman.delete_account, name, host)
                else:
                    Utilities.show_message_and_remember('Anonymous accounts detected',
                        'Anonymous accounts were detected in the server %s.\nAnonymous accounts can cause great confusion and are also a potential security issue and are advised to be removed. Please ask a DBA to delete them.' % logged_servername,
                        'OK', '', '',
                        'wb.admin.delete_anonymous_accounts:' + logged_username + '|no_privileges|' + '@' + logged_servername,
                        "Don't show this message again")
                self.already_asked_for_anon_accounts = True



    def refresh(self):
        if self.ctrl_be.is_sql_connected():
            try:
                self.secman.async_refresh(self.do_refresh)
            except PermissionDeniedError:
                self.show_no_permission()
            mforms.Utilities.driver_shutdown()



    def do_refresh(self):
        if not self.account_tab.dirty:
            self.account_tab.refresh()
            self.account_tab.update()
