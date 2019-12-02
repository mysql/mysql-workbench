# Copyright (c) 2012, 2019, Oracle and/or its affiliates. All rights reserved.
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
import re

from workbench.ui import WizardPage, DatabaseSchemaSelector


SYSTEM_SCHEMAS = ['`def`.`mysql`', '`def`.`information_schema`', '`def`.`performance_schema`']

class SchemaMainView(WizardPage):

    def __init__(self, main):
        super(SchemaMainView, self).__init__(main, 'Schema Selection')


    def create_ui(self):
        self.content.set_padding(20)

        self.content.add(mforms.newLabel('Select the schemas to copy to the destination server and click [Start Copy >] to start the process.'), False, True)

        match_str = r"\%s\.\%s" % (self.main.plan.migrationSource._db_module.quoteIdentifier('(.+)\\'), self.main.plan.migrationSource._db_module.quoteIdentifier('(.+)\\'))

        self.catalog_schemata = [ schema_name for catalog_name, schema_name in (re.match(match_str, full_name).groups()
                                            for full_name in [x for x in self.main.plan.migrationSource.schemaNames if x not in SYSTEM_SCHEMAS])]

        self.schema_selector = DatabaseSchemaSelector(self.catalog_schemata, tree_checked_callback=self.update_next_button)
        self.content.add(self.schema_selector, True, True)
  
        self.innodb_switch = mforms.newCheckBox()
        self.innodb_switch.set_text('Migrate MyISAM tables to InnoDB')
        self.innodb_switch.set_active(True)
        self.content.add_end(self.innodb_switch, False)

        self.content.add_end(mforms.newLabel(''), False)
        
        self.next_button.set_text('Start Copy >')
        self.next_button.set_enabled(False)
            

    def update_next_button(self, count):
        self.next_button.set_enabled( bool(count) )


    def go_next(self):
        self.main.plan.migrationSource.selectedCatalogName = 'def'
        try:
            self.main.plan.migrationSource.selectedSchemataNames = self.schema_selector.get_selected()
        except Exception as e:
            mforms.Utilities.show_error("Invalid Selection", str(e), "OK", "", "")
            return
            
        self.main.plan.state.applicationData["schemaMappingMethod"] = "drop_catalog"

        schema_set = set(schema.upper() for schema in self.schema_selector.get_selected())
        target_schema_set = set(schema.upper() for schema in grt.modules.DbMySQLFE.getSchemaNames(self.main.plan.migrationTarget.connection))

        existing_schemas = list(schema_set.intersection(target_schema_set))

        if len(existing_schemas) > 0:
            if mforms.Utilities.show_message("Existing Schemas", "The %s %s " % ( 'schema' if len(existing_schemas) == 1 else 'schemas', ", ".join(existing_schemas)) +
                    "will be dropped in the target MySQL Server and all the existing data will be" +
                    " lost. Do you want to continue?" , "Yes", "No", "") == mforms.ResultCancel:
                return

        super(SchemaMainView, self).go_next()
