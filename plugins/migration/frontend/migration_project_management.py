# Copyright (c) 2012, 2013, Oracle and/or its affiliates. All rights reserved.
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
from migration_ui_style import newHeaderLabel

class MainView(mforms.Box):
    def __init__(self, main):
        mforms.Box.__init__(self, False)
        self.set_managed()

        self.ui_created = False
        self.main = main

        label = newHeaderLabel("  Project Management")
        self.main.ui_profile.apply_style(label, 'content-label')
        self.add(label, False, True)

        self.main.add_content_page(self, "Projects", "Project Management", "admin_export")

    def page_activated(self, advancing):
        if not self.ui_created:
            self.create_ui()
            self.ui_created = True

    def create_ui(self):
        pass
