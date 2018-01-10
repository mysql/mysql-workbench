# Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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
