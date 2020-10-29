# Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

import mforms
from mforms import newBox, newButton, newTreeView, newTextEntry, newTabView

import os
import sys


class VariablesGrouper(mforms.Box):
    def __init__(self, variables, all_opts, output_path):
        mforms.Box.__init__(self, False)

        self.set_managed()
        self.set_release_on_add()

        self.suspend_layout()
        self.output_path = output_path
        self.all_options = all_opts

        self.filter = newTextEntry()
        self.filter.add_changed_callback(self.show_filtered_options)
        self.add(self.filter, False, True)

        box = newBox(True)
        box.set_spacing(12)

        sidebox = newBox(False)
        sidebox.set_spacing(12)

        self.groups_tree = newTreeView(mforms.TreeFlatList)
        self.groups_tree.set_size(220, -1)
        self.groups_tree.add_column(mforms.CheckColumnType, "", 20, True)
        self.groups_tree.add_column(mforms.StringColumnType, "", 160, False)
        self.groups_tree.end_columns()
        self.groups_tree.set_cell_edited_callback(self.toggled_group)

        sidebox.add(self.groups_tree, True, True)
        box.add_end(sidebox, False, True)

        self.values_tree = newTreeView(mforms.TreeFlatList)
        self.values_tree.add_column(mforms.StringColumnType, "Name", 200, False)
        self.values_tree.add_column(mforms.StringColumnType, "Groups", 1000, False)
        self.values_tree.set_selection_mode(mforms.TreeSelectMultiple)
        self.values_tree.end_columns()
        self.values_tree.set_allow_sorting(True)
        self.values_tree.add_changed_callback(self.value_selected)
        box.add(self.values_tree, True, True)

        self.add(box, True, True)

        box = newBox(True)
        box.set_spacing(8)
        box.set_padding(12)

        self.button_save = newButton()
        self.button_save.set_text("Save")
        self.button_save.add_clicked_callback(self.save)
        box.add_end(self.button_save, False, True)

        self.button_add_group = newButton()
        self.button_add_group.set_text("Add Group")
        self.button_add_group.add_clicked_callback(self.add_group)
        self.button_add_group.set_enabled(False)
        box.add_end(self.button_add_group, False, True)

        self.group_entry = newTextEntry()
        self.group_entry.set_size(300, -1)
        self.group_entry.set_enabled(False)
        box.add_end(self.group_entry, False, True)

        self.add(box, False, True)

        self.resume_layout()

        self.all_groups = set()
        self.value_group_pair = dict()

        for name, groups in variables:
            self.all_groups = self.all_groups.union(set(groups))
            self.value_group_pair[name] = groups

        for g in sorted(self.all_groups):
            node = self.groups_tree.add_node()
            node.set_string(1, g)

        self.groups = self.all_groups

        self.show_filtered_options()

    def show_filtered_options(self):
        self.values_tree.clear()
        filter_text = self.filter.get_string_value()

        for opt in self.all_options:
            if len(filter_text) == 0 or opt['name'].lower().find(filter_text.lower()) >= 0:
                node = self.values_tree.add_node()
                node.set_string(0, opt['name'])
                node.set_string(1, ", ".join(self.value_group_pair.get(opt['name'], [])))

    def value_selected(self):
        self.group_entry.set_enabled(False)
        self.button_add_group.set_enabled(False)
        node = self.values_tree.get_selected_node()
        if node:
            self.groups_tree.clear()
            groups = self.split_groups(node.get_string(1))
            for group in sorted(self.groups):
                group = group.strip()
                node = self.groups_tree.add_node()
                node.set_string(1, group)
                node.set_bool(0, group in groups)
            self.group_entry.set_enabled(True)
            self.button_add_group.set_enabled(True)

    def split_groups(self, group_string):
        return [x.encode("utf-8").strip() for x in group_string.strip().split(",") if x]

    def join_groups(self, group1_list, group2_list):
        return ",".join(sorted(group1_list + group2_list))

    def toggled_group(self, node, column, value):
        changed_group = node.get_string(1)
        for sel in self.values_tree.get_selection():
            var = self.split_groups(sel.get_string(1))
            node.set_bool(0, int(value) != 0)
            if int(value):
                sel.set_string(1, self.join_groups(var, [changed_group]))
            else:
                var.remove(changed_group)
                sel.set_string(1, ",".join(sorted(var)))
            self.value_group_pair[sel.get_string(0)] = self.split_groups(sel.get_string(1))

    def add_group(self):
        print("Adding new group: %s" % self.group_entry.get_string_value())
        v = self.group_entry.get_string_value()
        self.groups.add(v)
        self.value_selected()

    def save(self):
        l = []
        for x in range(self.values_tree.count()):
            node = self.values_tree.node_at_row(x)
            l.append((node.get_string(0).encode("utf-8"), self.split_groups(node.get_string(1))))

        import pprint
        f = open(self.output_path, "w+")
        f.write("variable_groups = ")
        pp = pprint.PrettyPrinter(indent=2, stream=f)
        pp.pprint(l)
        f.close()


script_dir = os.getcwd()

form = mforms.Form(None, 0)
form.set_size(800,600)

file_chooser = mforms.newFileChooser(form, mforms.OpenDirectory)

file_chooser.set_title('Choose the base directory')
file_chooser.set_path(script_dir)
file_chooser_result = file_chooser.run_modal()
print("File chooser result: %s" % file_chooser_result)
if not file_chooser_result:
    sys.exit()

temp_path = file_chooser.get_path()
sys.path.append(temp_path)

import variable_groups
import status_groups
import raw_opts
import raw_vars

tabs = newTabView(mforms.TabViewSystemStandard)

system_variables = VariablesGrouper(variable_groups.variable_groups, raw_opts.ropts, os.path.join(temp_path, "variable_groups.py"))
status_variables = VariablesGrouper(status_groups.variable_groups, raw_vars.status_vars_list, os.path.join(temp_path, "status_groups.py"))

tabs.add_page(system_variables, "System Variables")
tabs.add_page(status_variables, "Status Variables")

form.set_content(tabs)

form.run_modal(None, None)
