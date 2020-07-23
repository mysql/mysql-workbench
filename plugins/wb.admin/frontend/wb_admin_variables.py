# Copyright (c) 2007, 2020, Oracle and/or its affiliates.
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

from wb_admin_utils import weakcb


from mforms import newBox, newTreeView, newButton, newTabView, newTextEntry
import mforms

import wb_admin_variable_list

from wb_admin_utils import weakcb, WbAdminTabBase, WbAdminValidationConnection
from workbench.log import log_error

class VariablesViewer(mforms.Box):
    def __init__(self, ctrl_be, variables, command, viewer_type):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()

        self.user_groups = VariablesGroupContainer(viewer_type)
        self.user_groups.load()
        self.viewer_type = viewer_type
        self.variables = variables

        self.suspend_layout()

        self.command = command
        self.ctrl_be = ctrl_be
        self.support_variable_persistence = self.viewer_type == "system" and self.ctrl_be.target_version.is_supported_mysql_version_at_least(8, 0, 0)

        box = newBox(True)
        box.set_spacing(12)
        self.add(box, True, True)
        self.tree = newTreeView(mforms.TreeFlatList)
        self.tree.set_selection_mode(mforms.TreeSelectMultiple)

        sidebox = newBox(False)
        box.add(sidebox, False, True)

        self.searchEntry = newTextEntry(mforms.SearchEntry)

        sidebox.set_spacing(12)
        sidebox.add(self.searchEntry, False, True)
        sidebox.add(self.tree, True, True)
        self.tree.set_size(210, -1)

        self.searchEntry.add_changed_callback(self.filterOutput)

        self.tree.add_column(mforms.StringColumnType, "Category", 200, False)
        self.tree.end_columns()
        self.tree.add_changed_callback(weakcb(self, "refresh"))
        self.cat_menu = mforms.newContextMenu()
        self.cat_menu.add_will_show_callback(self.cat_menu_will_show)
        self.cat_menu.add_item_with_title("Delete Category", self.delete_category, "Delete", "Delete")
        self.tree.set_context_menu(self.cat_menu)

        self.values = newTreeView(mforms.TreeFlatList)
        self.values.set_selection_mode(mforms.TreeSelectMultiple)
        box.add(self.values, True, True)
        if self.support_variable_persistence:
            self.values.add_column(mforms.CheckColumnType, "Persist", 50, True)
        self.values.add_column(mforms.StringColumnType, "Name", 200, False)
        self.values.add_column(mforms.StringColumnType, "Value", 120, True)
        if self.support_variable_persistence:
            self.values.add_column(mforms.StringColumnType, "Persist Value", 120, True)
        self.values.add_column(mforms.StringColumnType, "Description", 1000, False)
        self.values.end_columns()
        self.values.set_allow_sorting(True)
        self.values.set_cell_edited_callback(self.edit_variable)
        self.values.add_changed_callback(weakcb(self, "value_selection_changed"))
        self.menu = mforms.newContextMenu()
        self.menu.add_will_show_callback(self.menu_will_show)
        self.values.set_context_menu(self.menu)

        box = newBox(True)
        box.set_spacing(8)
        copy_all_button = newButton()
        copy_all_button.set_text('Copy Global Status and Variables to Clipboard')
        copy_all_button.add_clicked_callback(self.copy_status_to_clipboard)
        box.add(copy_all_button, False, False)
        copy_shown_button = newButton()
        copy_shown_button.set_text('Copy Selected to Clipboard')
        copy_shown_button.add_clicked_callback(self.copy_selected_to_clipboard)
        box.add(copy_shown_button, False, False)

        if self.support_variable_persistence:
            reset_persist = newButton()
            reset_persist.set_text('Reset persist values')
            reset_persist.add_clicked_callback(weakcb(self, "reset_persist_cb"))
            box.add(reset_persist, False, False)

        self.copy_selected_to_clipboard_button = copy_shown_button
        button = newButton()
        box.add_end(button, False, True)
        button.set_text("Refresh")
        box.set_padding(0, 8, 0, 8)

        button.add_clicked_callback(weakcb(self, "refresh"))

        self.add(box, False, True)

        row = self.tree.add_node()
        row.set_string(0, "All")
        row = self.tree.add_node()
        row.set_string(0, "Filtered")

        self.resume_layout()

        self.variable_info = {}
        self.variables_in_group = {"Other": [], "Persisted": []}

        self._delayed_init_tm = mforms.Utilities.add_timeout(0.1, lambda: self.delayed_init(self.variables))

    def reset_persist_cb(self):
        if mforms.Utilities.show_message('Confirmation', 'Do you really want to reset ALL persistent variables?',
                                         'Ok', 'Cancel', '') == mforms.ResultOk:
            self.ctrl_be.exec_query("RESET PERSIST")
            self.refresh()

    def refresh_groups(self):
        self.tree.clear()
        row = self.tree.add_node()
        row.set_string(0, "All")
        row = self.tree.add_node()
        row.set_string(0, "Filtered")

        self.variable_info = {}
        self.variables_in_group = {"Other": []}
        self.delayed_init(self.variables)

    def shutdown(self):
        if self._delayed_init_tm:
            mforms.Utilities.cancel_timeout(self._delayed_init_tm)
            self._delayed_init_tm = None

    def load_persist_variables(self):
        self.persist_variables = []
        if self.ctrl_be.target_version and self.ctrl_be.target_version.is_supported_mysql_version_at_least(8, 0, 1):
            result = self.ctrl_be.exec_query("select @@performance_schema")
            if result:
                if result.nextRow():
                    if result.intByIndex(1) == 1:
                        result_variables = self.ctrl_be.exec_query("select * from performance_schema.persisted_variables")
                        if result_variables is not None:
                            while result_variables.nextRow():
                                self.persist_variables.append({'name': result_variables.stringByName("VARIABLE_NAME"),
                                                               'value': result_variables.stringByName("VARIABLE_VALUE")})

    def delayed_init(self, variables):
        self._delayed_init_tm = None

        variables_in_server = []
        result = self.ctrl_be.exec_query(self.command)
        if result is not None:
            while result.nextRow():
                name = result.stringByName("Variable_name")
                variables_in_server.append(name)

        if self.support_variable_persistence:
            self.load_persist_variables()

        existing_groups = set()
        for name, description, editable, groups in variables:
            self.variable_info[name.replace("-", "_")] = (description, editable)
            existing_groups = existing_groups.union(set(groups))
            for group in groups:
                if group not in self.variables_in_group:
                    self.variables_in_group[group] = []
                self.variables_in_group[group].append(name.replace("-", "_"))
            if not groups:
                self.variables_in_group["Other"].append(name.replace("-", "_"))

        for group_name in sorted(existing_groups):
            row = self.tree.add_node()
            row.set_string(0, group_name)
            row.set_tag(group_name)

        if self.variables_in_group["Other"]:
            row = self.tree.add_node()
            row.set_string(0, "Other")
            row.set_tag("Other")

        if self.support_variable_persistence:
            row = self.tree.add_node()
            row.set_string(0, "Persisted")
            row.set_tag("Persisted")
            for elem in self.persist_variables:
                self.variables_in_group["Persisted"].append(elem['name'])

        self.copy_selected_to_clipboard_button.set_enabled(len(self.values.get_selection()) > 0)
        for group in self.user_groups.content:
            row = self.tree.add_node()
            row.set_string(0, group)
            row.set_tag("Custom: %s" % group)
            if "Custom: %s" % group not in self.variables_in_group:
                self.variables_in_group["Custom: %s" % group] = self.user_groups.content[group]

    def edit_variable(self, node, column, value):
        name = ""
        if self.support_variable_persistence:
            if column == 0:
                if node.get_bool(0) is False:
                    if self.set_persist_value(node.get_string(1), node.get_string(2)):
                        node.set_bool(0, True)
                        node.set_string(3, node.get_string(2))
                else:
                    if self.reset_persist_single_value(node.get_string(1)):
                        node.set_bool(0, False)
                        sel = self.tree.get_selection()
                        if len(sel) == 1 and sel[0].get_string(0) == "Persisted":
                            self.refresh()
                        else:
                            node.set_string(3, "")
                return
            if column == 3 and node.get_bool(0) is True and self.set_persist_value(node.get_string(1), value):
                node.set_string(3, value)

            name = node.get_string(1)
        else:
            name = node.get_string(0)

        if name and name in self.variable_info and self.variable_info[name][1]:
            try:
                self.ctrl_be.exec_sql("SET GLOBAL %s=%s" % (name, int(value)))
            except ValueError:
                self.ctrl_be.exec_sql("SET GLOBAL %s='%s'" % (name, value.replace("'", "''")))

            value = self.ctrl_be.exec_query("%s LIKE '%s'" % (self.command, name))
            if value.firstRow():
                node.set_string(column, value.stringByIndex(2))

    def value_selection_changed(self):
        self.copy_selected_to_clipboard_button.set_enabled(len(self.values.get_selection()) > 0)

    def refresh_all(self):
        sel_row_node = 0
        sel = self.tree.get_selection()
        if sel:
            sel_row_node = self.tree.row_for_node(sel[0])

        self.refresh_groups()
        self.tree.set_node_selected(self.tree.node_at_row(sel_row_node), True)
        self.refresh()

    def refresh(self):
        if not self.ctrl_be.is_sql_connected():
            return

        if self.support_variable_persistence:
            self.load_persist_variables()
            self.variables_in_group["Persisted"] = []
            for elem in self.persist_variables:
                self.variables_in_group["Persisted"].append(elem['name'])

        rows = self.tree.get_selection()
        if not rows:
            self.values.clear()
            return

        filter_value = []
        search = None
        for row in rows:
            if self.tree.row_for_node(row) == 0:
                filter_value = None
                search = None
                break
            elif self.tree.row_for_node(row) == 1:
                filter_value = None
                search = self.searchEntry.get_string_value().replace("-", "_")
            else:
                tag = row.get_tag()
                if tag and filter_value is not None:
                    filter_value.extend(self.variables_in_group.get(tag, []))
        if filter_value:
            filter_value = set(filter_value)

        result = self.ctrl_be.exec_query(self.command)

        self.values.freeze_refresh()
        self.values.clear()
        
        if result is not None:
            while result.nextRow():
                name = result.stringByName("Variable_name")

                if filter_value is not None and name.replace("-", "_") not in filter_value:
                    continue

                if search is not None and search.lower() not in name.lower():
                    continue

                value = result.stringByName("Value")
                r = self.values.add_node()
                if self.support_variable_persistence:
                    r.set_bool(0, False)
                    r.set_string(1, name)
                    r.set_string(2, value)
                    for elem in self.persist_variables:
                        if elem['name'] == name:
                            r.set_string(3, elem['value'])
                            r.set_bool(0, True)

                    if name.replace("-", "_") not in self.variable_info:
                        r.set_string(4, "")
                    else:
                        editable = self.variable_info[name.replace("-", "_")][1] and "[rw] " or ""
                        r.set_string(4, editable + self.variable_info[name.replace("-", "_")][0])
                else:
                    r.set_string(0, name)
                    r.set_string(1, value)
                    if name.replace("-", "_") not in self.variable_info:
                        r.set_string(2, "")
                    else:
                        editable = self.variable_info[name.replace("-", "_")][1] and "[rw] " or ""
                        r.set_string(2, editable + self.variable_info[name.replace("-", "_")][0])

        if self.values.count() == 0:
            if len(rows) == 1 and row.get_string(0) == "Custom":
                r = self.values.add_node()
                if self.support_variable_persistence:
                    r.set_string(4, "Right click on a variable to add them to this category.")
                else:
                    r.set_string(2, "Right click on a variable to add them to this category.")

            if len(rows) == 1 and row.get_string(0) == "Persisted":
                r = self.values.add_node()
                if self.support_variable_persistence:
                    r.set_string(4, "Right click on a variable to add them to this category.")

        self.values.thaw_refresh()

    def filterOutput(self):
        self.tree.select_node(self.tree.node_at_row(1))
        self.refresh()

    def copy_status_to_clipboard(self):
        if not self.ctrl_be.is_sql_connected():
            mforms.Utilities.show_error('Connection error',
                                        'Cannot query the server for variables',
                                        'OK', '', '')
            return

        global_status = []
        result = self.ctrl_be.exec_query('SHOW GLOBAL STATUS')
        if result:
            while result.nextRow():
                global_status.append((result.stringByName('Variable_name'), result.stringByName('Value')))

        global_variables = []
        result = self.ctrl_be.exec_query('SHOW GLOBAL VARIABLES')
        if result:
            while result.nextRow():
                global_variables.append((result.stringByName('Variable_name'), result.stringByName('Value')))

        max_length = max(len(name) for name, val in global_status + global_variables) + 5
        status = 'GLOBAL STATUS:\n'
        status += '\n'.join([var_name.ljust(max_length, '.') + ' ' + var_value for var_name, var_value in global_status])
        status += '\n\nGLOBAL VARIABLES:\n'
        status += '\n'.join([var_name.ljust(max_length, '.') + ' ' + var_value for var_name, var_value in global_variables])
        mforms.Utilities.set_clipboard_text(status)

    def copy_selected_to_clipboard(self):
        selection = []
        selected_vars = self.values.get_selection()
        if not selected_vars:
            return
        for node in selected_vars:
            selection.append((node.get_string(0), node.get_string(1)) if not self.support_variable_persistence else (node.get_string(1), node.get_string(2)))
        max_length = max(len(name) for name, val in selection) + 5
        status = '\n'.join([var_name.ljust(max_length, '.') + ' ' + var_value for var_name, var_value in selection])
        mforms.Utilities.set_clipboard_text(status)

    def delete_category(self):
        node = self.tree.get_selected_node()
        if node:
            name = node.get_string(0)
            if name in self.user_groups.content:
                self.user_groups.delete([name])
                self.user_groups.save()
                node.remove_from_parent()

    def cat_menu_will_show(self, item):
        node = self.tree.get_selected_node()
        self.cat_menu.find_item("Delete").set_enabled(True if node and node.get_string(0) in self.user_groups.content and node.get_tag().startswith("Custom: ") else False)

    def menu_will_show(self, item):
        self.menu.remove_all()
        selected_vars = self.values.get_selection()
        if not selected_vars:
            return

        if len(selected_vars) == 1 and selected_vars[0].get_string(2).startswith('Right click on'):
            return

        if item is None:
            self.menu.add_item_with_title("Add to Custom Category...", self.var_to_custom_group, "Variable to Custome Group", "var_to_custom_group")
            sel_group = self.tree.get_selection()
            for node in sel_group:
                group = node.get_string(0)
                tag = node.get_tag()
                if tag.startswith("Custom: "):
                    self.menu.add_item_with_title("Remove from %s" % group, lambda self=self, x=group: self.remove_from_group(x), "Remove From Group %s" % group, "remove_from_group_%s" % group)

            if len(self.user_groups.content):
                self.menu.add_separator()

            for group in self.user_groups.content:
                self.menu.add_item_with_title("Add to %s" % group, lambda self=self, x=group: self.var_to_group(x), "Variable to Group %s" % group, "var_to_group_%s" % group)

    def remove_from_group(self, grp):
        selected_vars = self.values.get_selection()
        if not selected_vars:
            return

        selection = []
        for node in selected_vars:
            selection.append(node.get_string(0) if not self.support_variable_persistence else node.get_string(1))

        self.user_groups.remove_from_group(grp, selection)
        self.user_groups.save()
        self.refresh_all()

    def var_to_group(self, grp):
        selection = []
        selected_vars = self.values.get_selection()
        if not selected_vars:
            return

        for node in selected_vars:
            selection.append(node.get_string(0) if not self.support_variable_persistence else node.get_string(1))

        self.user_groups.assign({grp: selection})
        self.user_groups.save()
        self.refresh_all()

    def reset_persist_single_value(self, value_name):
            if mforms.Utilities.show_message('Confirmation', 'Do you really want to reset selected persistent variable: %s?' % value_name,
                                             'Ok', 'Cancel', '') == mforms.ResultOk:
                try:
                    self.ctrl_be.exec_query("RESET PERSIST %s" % value_name)
                    return True
                except Exception as e:
                    log_error("Error occured while unsetting persisting variables: %s" % e)
                    mforms.Utilities.show_error('Access denied', 'You need (at least one of) the SUPER or SYSTEM_VARIABLES_ADMIN privilege(s) for this operation.', 'Ok', '', '')
            return False

    def set_persist_value(self, name, value):
        try:
            if name in wb_admin_variable_list.ro_persistable:
                mforms.Utilities.show_message('Information', 'This value can\'t be persisted: %s' % name, 'Ok', '', '')
                return False

            if self.variable_info and name in self.variable_info and self.variable_info[name][1]:
                if not self.store_persist_value(name, value):
                    mforms.Utilities.show_message('Information', 'Unable to store status of the value: %s' % name, 'Ok', '', '')
                    return False
                return True
            else:
                if not self.store_persist_value(name, value, True):
                    mforms.Utilities.show_message('Information', 'Unable to store status of the value: %s' % name, 'Ok', '', '')
                    return False
                return True
        except Exception as e:
            log_error("Error occured while persisting variables: %s" % e)
            mforms.Utilities.show_error('Access denied', 'You need (at least one of) the SUPER or SYSTEM_VARIABLES_ADMIN privilege(s) for this operation.', 'Ok', '', '')
            return False

    def store_persist_value(self, value_name, value, only=False):
        if value_name in self.variable_info:
            persist_type = "PERSIST" if not only else "PERSIST_ONLY"
            try:
                self.ctrl_be.exec_sql("SET %s %s=%s" % (persist_type, value_name, int(value)))
            except ValueError:
                self.ctrl_be.exec_sql("SET %s %s='%s'" % (persist_type, value_name, value.replace("'", "''")))

            return True

        return False

    def var_to_custom_group(self):
        selection = []
        selected_vars = self.values.get_selection()
        if not selected_vars:
            return

        for node in selected_vars:
            selection.append(node.get_string(0) if not self.support_variable_persistence else node.get_string(1))

        grp_select = VariablesGroupSelector(self.user_groups, selection)
        grp_select.run()
        # VariablesGroupSelector is modal
        self.refresh_all()


def _decode_list(data):
    rv = []
    for item in data:
        if isinstance(item, str):
            item = item
        elif isinstance(item, list):
            item = _decode_list(item)
        elif isinstance(item, dict):
            item = _decode_dict(item)
        rv.append(item)
    return rv


def _decode_dict(data):
    rv = {}
    for key, value in data.items():
        if isinstance(key, str):
            key = key
        if isinstance(value, str):
            value = value
        elif isinstance(value, list):
            value = _decode_list(value)
        elif isinstance(value, dict):
            value = _decode_dict(value)
        rv[key] = value
    return rv


class VariablesGroupContainer:
    def __init__(self, container_type):
        self.content = {}
        self.type = container_type
        self.group_file = "%s/custom_%s_group.json" % (mforms.App.get().get_user_data_folder(), self.type)

    def load(self):
        self.content = {}
        try:
            with open(self.group_file, 'r') as fp:
                import json
                self.content = json.load(fp, object_hook=_decode_dict)
        except Exception:
            # Group file doesn't exists, probably user never created any,
            # create then the default custom group and save it
            self.add("Custom")
            self.save()

    def save(self):
        try:
            with open(self.group_file, 'wb') as fp:
                import json
                json.dump(self.content, fp)
        except Exception:
            pass

    def add(self, name):
        name = name.strip()
        if name in self.content:
            raise Exception("Already exists", "Category already exists, please specify a different category name")

        self.content[name] = []

    def delete(self, group_names):
        for name in group_names:
            self.content.pop(name, None)

    def remove_from_group(self, group_name, vals):
        for val in vals:
            if val in self.content[group_name]:
                self.content[group_name].remove(val)

    def assign(self, assign_info):
        for grp in assign_info:
            self.content[grp] = list(set(self.content[grp] + assign_info[grp]))


class VariablesGroupSelector(mforms.Form):
    def __init__(self, group_container, variables):
        mforms.Form.__init__(self, None)
        self.pending_changes = False
        self.sel_vars = variables
        self.group_container = group_container
        self.suspend_layout()

        self.set_title("Custom Variable Categories")

        content = mforms.newBox(False)
        self.set_content(content)
        content.set_padding(20)
        content.set_spacing(12)

        lbl = mforms.newLabel("Select or create new category for custom variable categories.")
        content.add(lbl, False, False)

        self.groups = newTreeView(mforms.TreeFlatList)
        self.groups.set_selection_mode(mforms.TreeSelectMultiple)
        self.groups.add_column(mforms.StringColumnType, "Category name", 100, False)
        self.groups.end_columns()
        self.groups.set_size(200, 200)
        self.menu = mforms.newContextMenu()
        self.menu.add_item_with_title("Delete Category", self.group_delete, "Delete Group", "group_delete")
        self.groups.set_context_menu(self.menu)

        content.add(self.groups, True, True)

        entry_box = mforms.newBox(True)
        entry_box.set_spacing(5)

        lbl = mforms.newLabel("Category name:")
        entry_box.add(lbl, False, False)
        self.name = mforms.newTextEntry()
        self.name.add_action_callback(self.group_name_action)
        entry_box.add(self.name, True, True)
        self.add_btn = newButton()
        self.add_btn.set_text("Add")
        self.add_btn.add_clicked_callback(self.group_add)
        entry_box.add_end(self.add_btn, False, False)

        content.add(entry_box, False, True)

        self.cancel = newButton()
        self.cancel.set_text("Cancel")
        self.cancel.add_clicked_callback(self.cancel_click)
        self.ok = newButton()
        self.ok.set_text("OK")
        self.ok.add_clicked_callback(self.ok_click)

        self.delete = newButton()
        self.delete.set_text("Delete")
        self.delete.add_clicked_callback(self.group_delete)

        bbox = mforms.newBox(True)
        bbox.set_spacing(12)

        okcancel_box = mforms.newBox(True)
        okcancel_box.set_spacing(12)

        bbox.add_end(okcancel_box, False, True)
        bbox.add(self.delete, False, True)

        mforms.Utilities.add_end_ok_cancel_buttons(okcancel_box, self.ok, self.cancel)
        content.add_end(bbox, False, True)

        self.set_size(550, 350)
        self.center()
        self.load_groups()

        self.resume_layout()

    def load_groups(self, soft_load=False, grp_name=None):
        self.groups.clear()
        if not soft_load:
            self.group_container.load()

        matched_node = None
        for item in self.group_container.content:
            node = self.groups.add_node()
            node.set_string(0, item)
            if grp_name is not None and item == grp_name:
                matched_node = node
        if self.groups.count() > 0:
            self.groups.select_node(self.groups.node_at_row(0))

        return matched_node

    def group_name_action(self, action):
        if action == mforms.EntryActivate:
            self.group_add()

    def group_add(self):
        val = self.name.get_string_value().strip()
        if len(val) == 0:
            mforms.Utilities.show_error("Missing value", "You must supply a category name.", "OK", "", "")
            return

        try:
            self.group_container.add(val)
            self.pending_changes = True
        except Exception as e:
            mforms.Utilities.show_error(e.args[0], e.args[1], "OK", "", "")
            return

        self.groups.set_node_selected(self.load_groups(True, val), True)
        self.name.set_value("")

    def group_delete(self):
        selected_vars = self.groups.get_selection()
        if not selected_vars:
            return
        selection = []
        for node in selected_vars:
            selection.append(node.get_string(0))

        groups_for_delete = []
        for node in selected_vars:
            groups_for_delete.append(node.get_string(0))
        self.group_container.delete(groups_for_delete)

        self.pending_changes = True

        self.load_groups(soft_load=True)

    def cancel_click(self):
        self.group_container.load()
        self.close()

    def ok_click(self):
        selected_vars = self.groups.get_selection()
        if selected_vars:
            group_assign = {}
            for node in selected_vars:
                group_assign[node.get_string(0)] = self.sel_vars

            self.group_container.assign(group_assign)
            self.group_container.save()
        self.close()

    def run(self):
        self.run_modal(None, self.cancel)


class WbAdminVariables(WbAdminTabBase):
    def __init__(self, ctrl_be, instance_info, main_view):
        WbAdminTabBase.__init__(self, ctrl_be, instance_info, main_view)
        
        self.add_validation(WbAdminValidationConnection(ctrl_be))
        self.set_standard_header("title_variables.png", self.instance_info.name, "Server Variables")

    @classmethod
    def wba_register(cls, admin_context):
        admin_context.register_page(cls, "Management", "Status and System Variables", "Status and System Variables")

    @classmethod
    def identifier(cls):
        return "admin_status_vars"

    def create_ui(self):
        self.tab = newTabView(False)

        self.status = VariablesViewer(self.ctrl_be, wb_admin_variable_list.status_variable_list, "SHOW GLOBAL STATUS", 'status')
        self.status.set_padding(6)

        self.server = VariablesViewer(self.ctrl_be, wb_admin_variable_list.system_variable_list, "SHOW GLOBAL VARIABLES", 'system')
        self.server.set_padding(6)

        self.tab.add_page(self.status, "Status Variables")
        self.tab.add_page(self.server, "System Variables")
        
        return self.tab
  

    def update_ui(self):
        self.status.refresh()
        self.server.refresh()
