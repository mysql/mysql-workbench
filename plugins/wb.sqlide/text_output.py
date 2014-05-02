# Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.
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

class TextOutputTab(mforms.AppView):
    def __init__(self, output):
        super(TextOutputTab, self).__init__(False, "TextOutput", False)

        self.textbox = mforms.newTextBox(mforms.BothScrollBars)
        self.textbox.set_monospaced(True)
        self.textbox.set_value(output)
        self.textbox.set_read_only(True)
            
        self.add(self.textbox, True, True)
