# Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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
import grt

from workbench.ui import WizardPage, WizardProgressPage


class MigrationOptionsView(WizardPage):
    def __init__(self, main):
        WizardPage.__init__(self, main, "Migration Options")

        self.main.add_wizard_page(self, "ObjectMigration", "Migration Options")

        label = mforms.newLabel("Select options for the migration of the source schema/schemas to MySQL.")
        self.content.add(label, False, True)

        panel = mforms.newPanel(mforms.TitledBoxPanel)
        panel.set_title("DBMS Specific Options")
        self.content.add(panel, False, True)

        self._db_options_box = mforms.newBox(False)
        panel.add(self._db_options_box)
        self._db_options_box.set_padding(12)
        self._db_options_box.set_spacing(8)
        self._db_options = []


    def page_advanced(self, advancing):
        if advancing:
            for item, name, getter in self._db_options:
                self._db_options_box.remove(item)

            if hasattr(self.main.plan.migrationSource.module_re(), "migrationOptions"):
                options = self.main.plan.migrationSource.module_re().migrationOptions(self.main.plan.state)
  
                self._db_options = self.create_options(self._db_options_box, options)
                if not self._db_options:
                    self._db_options_box.show(False)
                else:
                    self._db_options_box.show(True)
    
        return WizardPage.page_advanced(self, advancing)


    def create_options(self, box, options):
        optlist = []
        for option in options:
            cont = None
            if option.paramType == "boolean":
                opt = mforms.newCheckBox()
                opt.set_active(self.defaultValue == "1")
                box.add(opt, False, True)
                getter = opt.get_string_value
            elif option.paramType == "string":
                hbox = mforms.newBox(True)
                hbox.set_spacing(8)
                hbox.add(mforms.newLabel(option.caption), False, True)
                opt = mforms.newTextEntry()
                opt.set_value(option.defaultValue)
                hbox.add(opt, True, True)
                l = mforms.newLabel(option.description)
                l.set_style(mforms.SmallHelpTextStyle)
                hbox.add(l, False, True)
                box.add(hbox, False, True)
                cont = hbox
                getter = opt.get_string_value
            else:
                grt.send_error("MigrationWizard", "migrationOption() for source has an invalid parameter of type %s (%s)" % (option.paramType, option.name))
                continue
            optlist.append((cont or opt, option.name, getter))
        return optlist


    def go_next(self):
        dic = self.main.plan.state.objectMigrationParams
        for item, name, getter in self._db_options:
            dic[name] = getter()
        WizardPage.go_next(self)



class MigrationProgressView(WizardProgressPage):
    def __init__(self, main):
        WizardProgressPage.__init__(self, main, "Migration", description="""Reverse engineered objects from the source RDBMS will now be automatically 
converted into MySQL compatible objects. Default datatype and default column value
mappings will be used. You will be able to review and edit generated objects and column
definitions in the Manual Editing step.""")
        
        self._autostart = True
        self.add_threaded_task(self.task_migrate, "Migrate Selected Objects")
        self.add_threaded_task(self.task_generate_sql, "Generate SQL CREATE Statements")

        self.main.add_wizard_page(self, "ObjectMigration", "Migration")
 
 
    def task_migrate(self):
        self.main.plan.migrate()
        
        
    def task_generate_sql(self):
        grt.send_progress(0, "Generating SQL...")
        self.main.plan.generateSQL()


    def go_back(self):
        self.reset()
        WizardProgressPage.go_back(self)

