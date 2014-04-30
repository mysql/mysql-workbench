# Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
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


def insert_item_to_plugin_context_menu(menu, item):
    bottom = menu.find_item("bottom_plugins_separator")
    if bottom:
        index = menu.get_item_index(bottom)
        if index > 0 and not menu.find_item("top_plugins_separator"):
            sep = mforms.newMenuItem("", mforms.SeparatorMenuItem)
            sep.set_name("top_plugins_separator")
            menu.insert_item(index, sep)
            index += 1
        menu.insert_item(index, item)
