# Copyright (c) 2012, 2016, Oracle and/or its affiliates. All rights reserved.
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
from workbench.ui import WizardPage

class MainView(WizardPage):
    def __init__(self, main):
        WizardPage.__init__(self, main, 'Overview', no_buttons=True)

        self.main = main

    def create_ui(self):
        label = mforms.newLabel('Welcome to the MySQL Workbench Schema Transfer Wizard')
        label.set_style(mforms.BigBoldStyle)
        self.content.add(label, False, True)
        self.content.set_spacing(12)
        self.content.set_padding(20)

        label = mforms.newLabel('The MySQL Schema Transfer Wizard helps you to move your data from an older MySQL server to the latest MySQL GA\n'+
                                '(General Availability) version. It is meant for developer machines to get you working with the latest\n'+ 
                                'MySQL Server quickly. The data is transferred on the fly and is not based on a consistent snapshot. This\n'+
                                'works well for local instances that are used for development purposes. Please note that you should not\n'+
                                'use this tool on production MySQL instances. Production databases require a more complex data migration\n'+
                                'scenario in most cases.')
        self.content.add(label, False, True)

        box = mforms.newBox(True)
        box.add(mforms.newLabel(''), True, True)
        button_start = mforms.newButton()
        button_start.set_size(150, -1)
        button_start.set_text('Start the Wizard')
        button_start.add_clicked_callback(self.main.go_next_page)
        box.add(button_start, False, True)
        box.add(mforms.newLabel(''), True, True)
        self.content.add_end(box, False, True)
