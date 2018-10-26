# Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

import mforms
import grt

# Attach some plugins items in the resultset context menus
def handleResultsetContextMenu(name, sender, args):
    menu = mforms.fromgrt(args['menu'])
    selection = args['selected-rows']
    column = args.get('selected-column')
    if column is not None and column >= 0 and selection:
        row = selection[0]
    else:
        row = None

    menu.add_separator()

    item = menu.add_item_with_title("Capitalize Text", lambda: capitalizeCell(sender, row, column), "Capitalize Text", "")
    item.set_enabled(row is not None and isinstance(sender, grt.classes.db_query_EditableResultset))

    item = menu.add_item_with_title("lowercase Text", lambda: lowerCaseCell(sender, row, column), "Lowercase Text", "")
    item.set_enabled(row is not None and isinstance(sender, grt.classes.db_query_EditableResultset))

    item = menu.add_item_with_title("UPPERCASE Text", lambda: upperCaseCell(sender, row, column), "Uppercase Text", "")
    item.set_enabled(row is not None and isinstance(sender, grt.classes.db_query_EditableResultset))



def capitalizeCell(rs, row, column):
    rs.goToRow(row)
    s= rs.stringFieldValue(column)
    if s:
        s=" ".join([ss.capitalize() for ss in s.split()])
        rs.setStringFieldValue(column, s)


def lowerCaseCell(rs, row, column):
    rs.goToRow(row)
    s= rs.stringFieldValue(column)
    if s:
        s= s.lower()
        rs.setStringFieldValue(column, s)


def upperCaseCell(rs, row, column):
    rs.goToRow(row)
    s= rs.stringFieldValue(column)
    if s:
        s= s.upper()
        rs.setStringFieldValue(column, s)
