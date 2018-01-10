# Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

import random
import mforms
import raw_opts
import options_layout
import wba_config_file_be

class NameSource(object):
    def get_typed_value(self, name):
        return (name, mforms.CellText, False)

class ValueSource(object):
    def get_typed_value(self, name):
        if name[1] in ['a','b','c','d']:
            lst = ["1", "2", "3", "4", "5", "6"]
            return (lst[random.randint(0, len(lst) - 1)], mforms.CellEnum, lst)
        else:
            return (name, mforms.CellText, True)

frm = mforms.Form(None, mforms.FormResizable)

grid = mforms.newSimpleGrid()

layout = wba_config_file_be.WBAOptionsLayout(grid, options_layout.layout, ["Option name", "Option value"])

layout.add_source(NameSource(), "Option name")
layout.add_source(ValueSource(), "Option value")

layout.rebuild_ui(options_layout.layout)
layout.update_ui()

frm.set_content(grid)
frm.set_size(700,600)
frm.show()
