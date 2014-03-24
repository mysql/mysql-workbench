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

import threading

import mforms
from workbench.ui import DatabaseObjectSelector, WizardPage

class ObjectMainView(WizardPage):

    def __init__(self, main):
        WizardPage.__init__(self, main, "Source Objects")

        self.main.add_wizard_page(self, "ObjectMigration", "Source Objects")
        self._scrollpanel = None
        
        label = mforms.newLabel("You may select the objects to be migrated in the lists below.\nAll tables will be migrated by default.")
        self.content.add(label, False, True)
        

    def page_activated(self, advancing):
        WizardPage.page_activated(self, advancing)

        if advancing:
            supported_types = tuple(otype[0] for otype in self.main.plan.migrationSource.supportedObjectTypes)
            database_objects = {}
            ui_settings = {}
            for otype in supported_types:
                database_objects[otype] = self.main.plan.migrationSource.selectedObjectsOfType(otype)
                if otype != 'tables' and self.main.plan.migrationSource.rdbms.__id__ != 'com.mysql.rdbms.mysql':
                    ui_settings[otype] = { 'group_selected' : False }

            if self._scrollpanel:
                self.content.remove(self._scrollpanel)

            self._scrollpanel = mforms.newScrollPanel()
            self.db_selector = DatabaseObjectSelector(supported_types, database_objects, ui_settings)
            self._scrollpanel.add(self.db_selector)
            self.content.add(self._scrollpanel, True, True)


    def go_next(self):
        ignored_objects = self.db_selector.get_ignored_objects()

        for otype in ignored_objects:
            self.main.plan.migrationSource.setIgnoredObjectsOfType(otype, ignored_objects[otype])

        super(ObjectMainView, self).go_next()

