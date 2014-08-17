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

import grt
import mforms
import re

from workbench.ui import WizardPage, WizardProgressPage, DatabaseSchemaSelector

class SchemaMainView(WizardPage):

    def __init__(self, main):
        WizardPage.__init__(self, main, 'Schemas Selection')

        self._ui_created = False
        self.main.add_wizard_page(self,  'SourceTarget', 'Schemas Selection')

        optionspanel = mforms.newPanel(mforms.TitledBoxPanel)
        optionspanel.set_title('Schema Name Mapping Method')
        optionsbox = mforms.newBox(False)
        optionsbox.set_padding(8)
        optionsbox.set_spacing(8)
        
        optionsbox.add(mforms.newLabel('Choose how the reverse engineered schemas and objects should be mapped.\n'), False)

        options = [ 'Keep schemas as they are: Catalog.Schema.Table -> Schema.Table',
                    'Only one schema: Catalog.Schema.Table -> Catalog.Table',
                    'Only one schema, keep current schema names as a prefix: Catalog.Schema.Table -> Catalog.Schema_Table',
                  ]
        rid = mforms.RadioButton.new_id()
        self.options = []
        for opt in options:
            radio_button = mforms.newRadioButton(rid)
            radio_button.set_text(opt)
            optionsbox.add(radio_button, False)
            self.options.append(radio_button)
        self.options[1].set_active(True)

        optionspanel.add(optionsbox)
        self._optionspanel = optionspanel
        #self._advanced_shown = False
        #self._optionspanel.show(False)
        self.content.add_end(optionspanel, False)


    def page_activated(self, advancing):
        WizardPage.page_activated(self, advancing)

        if advancing:
            self.doesSupportCatalogs = self.main.plan.migrationSource.rdbms.doesSupportCatalogs

            match_str = r"\%s\.\%s" % (self.main.plan.migrationSource._db_module.quoteIdentifier('(.+)\\'), self.main.plan.migrationSource._db_module.quoteIdentifier('(.+)\\'))
            if self.doesSupportCatalogs > 0:
                catalog_schemata_list = [ (catalog_name, schema_name) for catalog_name, schema_name in (re.match(match_str, full_name).groups() 
                                            for full_name in self.main.plan.migrationSource.schemaNames) ]
                self.catalog_schemata = {}
                for catalog_name, schema_name in catalog_schemata_list:
                    self.catalog_schemata.setdefault(catalog_name, []).append(schema_name)
                self.catalog_schemata = self.catalog_schemata.items()
                
                self._optionspanel.show(True)
                #self.advanced_button.show(True)
            else:
                self.catalog_schemata = [ schema_name for catalog_name, schema_name in (re.match(match_str, full_name).groups() 
                                            for full_name in self.main.plan.migrationSource.schemaNames) ]
                self._optionspanel.show(False)
                #self.advanced_button.show(False)

            if self.schema_selector:
                self.content.remove(self.schema_selector)
      
            self.schema_selector = DatabaseSchemaSelector(self.catalog_schemata, tree_checked_callback=self.update_next_button)
            self.content.add(self.schema_selector, True, True)
      
            self.next_button.set_enabled(False)


    def create_ui(self):
        label = mforms.newLabel('Select the schemata you want to migrate:')
        label.set_style(mforms.BoldStyle)
        self.content.add(label, False)
        self.schema_selector = None


    def should_skip(self):
        return self.main.plan.migrationSource.rdbms.doesSupportCatalogs < 0 and len(self.main.plan.migrationSource.schemaNames) == 1

    def page_skipped(self):
        # called when the page is not activated, because should_skip returned True
        match_re = self.main.plan.migrationSource._db_module.quoteIdentifier('(.+)\\')
        names = [re.match(match_re, s).groups()[0] for s in self.main.plan.migrationSource.schemaNames]
        self.main.plan.migrationSource.selectedCatalogName, self.main.plan.migrationSource.selectedSchemataNames = ("def", names)
        self.main.plan.state.applicationData["schemaMappingMethod"] = "drop_catalog"


    def update_next_button(self, count):
        self.next_button.set_enabled( bool(count) )

    def schemata_to_migrate(self):
        selected = self.schema_selector.get_selected()
        if self.doesSupportCatalogs > 0:
            if len(selected) > 1:
                raise Exception('Cannot select multiple schemas from different catalogs')
            catalog = selected.keys()[0]
            return catalog, selected[catalog]
        else:
            return "def", selected

    #def go_advanced(self):
    #    self._advanced_shown = not self._advanced_shown
    #    self._optionspanel.show(self._advanced_shown)

    def go_next(self):
        try:
            self.main.plan.migrationSource.selectedCatalogName, self.main.plan.migrationSource.selectedSchemataNames = self.schemata_to_migrate()
        except Exception, e:
            mforms.Utilities.show_error("Invalid Selection", str(e), "OK", "", "")
            return

        def find_selected_option():  #TODO: When we finally drop py2.5 support substitute this with self.options.index(next(opt for opt in self.options if opt.get_active()))
            for idx, option_radio in enumerate(self.options):
                if option_radio.get_active():
                    return idx
            return None

        if self.doesSupportCatalogs:
            self.main.plan.state.applicationData["schemaMappingMethod"] = ["drop_catalog", "drop_schema", "merge_schema"][find_selected_option()]
        else:
            self.main.plan.state.applicationData["schemaMappingMethod"] = "drop_catalog"

        WizardPage.go_next(self)



class ReverseEngineerProgressView(WizardProgressPage):
    def __init__(self, main):
        WizardProgressPage.__init__(self, main, "Reverse Engineer Source", description="""Selected schema metadata will now be fetched from the source RDBMS and reverse engineered
so that its structure can be determined.""")
        
        self._autostart = True
        self.add_task(self.task_connect, "Connect to source DBMS")
        self.add_threaded_task(self.task_reveng, "Reverse engineer selected schemas")
        self.add_task(self.task_post_processing, "Post-processing of reverse engineered schemas")

        self.main.add_wizard_page(self, "SourceTarget", "Reverse Engineer Source")
 
 
    def task_connect(self):
        grt.send_progress(-1, "Connecting...")
        if not self.main.plan.migrationSource.connect():
            raise Exception("Could not connect to source RDBMS")
        return True

    def go_back(self):
        self.main.plan.migrationSource.resetProgressFlags()
        self.reset(True)
        WizardProgressPage.go_back(self)


    def task_reveng(self):
        self.main.plan.migrationSource.reverseEngineer()
        return True
    
    
    def task_post_processing(self):
        selected_option = self.main.plan.state.applicationData.get("schemaMappingMethod")
        # nothing needs to be done for drop_catalog
        if selected_option == "drop_schema":
            grt.send_info("Merging reverse engineered schema objects into a single schema...")
            self._merge_schemata()
        elif selected_option == "merge_schema":
            grt.send_info("Merging and renaming reverse engineered schema objects into a single schema...")
            self._merge_schemata(prefix='schema_name')


    def _merge_schemata(self, prefix=''):
        catalog = self.main.plan.migrationSource.catalog
        schema = catalog.schemata[0]
        # preserve the original name of the catalog
        schema.oldName = schema.name

        module_db = self.main.plan.migrationSource.module_db()

        # otypes is something like ['tables', 'views', 'routines']:
        otypes = [ suptype[0] for suptype in self.main.plan.migrationSource.supportedObjectTypes ]

        # Update names for the objects of this first schema:
        if prefix:
            actual_prefix = (schema.name if prefix == 'schema_name' else schema.__id__) + '_'
            for otype in otypes:
                for obj in getattr(schema, otype):
                    # this will be used later during datacopy to refer to the original object to copy from
                    obj.oldName = module_db.quoteIdentifier(schema.oldName)+"."+module_db.quoteIdentifier(obj.name)
                    oname = obj.name
                    obj.name = actual_prefix + obj.name
                    grt.send_info("Object %s was renamed to %s" % (oname, obj.name))
        else:
            for otype in otypes:
                for obj in getattr(schema, otype):
                    # this will be used later during datacopy to refer to the original object to copy from
                    obj.oldName = module_db.quoteIdentifier(schema.name)+"."+module_db.quoteIdentifier(obj.name)

        schema.name = catalog.name
        if not prefix:
            known_names = dict( (otype, set(obj.name for obj in getattr(schema, otype))) for otype in otypes)

        for other_schema in list(catalog.schemata)[1:]:
            if other_schema.defaultCharacterSetName != schema.defaultCharacterSetName:
                grt.send_warning('While merging schema %s into %s: Default charset for schemas differs (%s vs %s). Setting default charset to %s' % (other_schema.name, schema.name, other_schema.defaultCharacterSetName, schema.defaultCharacterSetName, schema.defaultCharacterSetName))
                self.main.plan.state.addMigrationLogEntry(0, other_schema, None,
                      'While merging schema %s into %s: Default charset for schemas differs (%s vs %s). Setting default charset to %s' % (other_schema.name, schema.name, other_schema.defaultCharacterSetName, schema.defaultCharacterSetName, schema.defaultCharacterSetName))

            if other_schema.defaultCollationName != schema.defaultCollationName:
                grt.send_warning('While merging schema %s into %s: Default collation for schemas differs (%s vs %s). Setting default collation to %s' % (other_schema.name, schema.name, other_schema.defaultCollationName, schema.defaultCollationName, schema.defaultCollationName))
                self.main.plan.state.addMigrationLogEntry(0, other_schema, None,
                      'While merging schema %s into %s: Default collation for schemas differs (%s vs %s). Setting default collation to %s' % (other_schema.name, schema.name, other_schema.defaultCollationName, schema.defaultCollationName, schema.defaultCollationName))

            for otype in otypes:
                other_objects = getattr(other_schema, otype)
                if not prefix:
                    repeated_object_names = known_names[otype].intersection(obj.name for obj in other_objects)
                    if repeated_object_names:
                        objects_dict = dict( (obj.name, obj) for obj in other_objects )
                        for repeated_object_name in repeated_object_names:
                            objects_dict[repeated_object_name].name += '_' + other_schema.name
                            grt.send_warning('The name of the %(otype)s "%(oname)s" conflicts with other %(otype)s names: renamed to "%(onewname)s"' % { 'otype':otype[:-1],
                                                                  'oname':repeated_object_name,
                                                                  'onewname':objects_dict[repeated_object_name].name })
        
                            self.main.plan.state.addMigrationLogEntry(0, other_schema, None,
                                  'The name of the %(otype)s "%(oname)s" conflicts with other %(otype)s names: renamed to "%(onewname)s"' % { 'otype':otype[:-1],
                                                                                                                                              'oname':repeated_object_name,
                                                                                                                                              'onewname':objects_dict[repeated_object_name].name }
                                                                      )
                        known_names[otype].update(other_objects)
                else:
                    actual_prefix = (other_schema.name if prefix == 'schema_name' else schema.__id__) + '_'

                getattr(schema, otype).extend(other_objects)
                for obj in other_objects:
                    # this will be used later during datacopy to refer to the original object to copy from
                    obj.oldName = module_db.quoteIdentifier(obj.owner.name)+"."+module_db.quoteIdentifier(obj.name)
                    
                    obj.owner = schema
                    if prefix:
                        oname = obj.name
                        obj.name = actual_prefix + obj.name
                        grt.send_info("Object %s was renamed to %s" % (oname, obj.name))

        # Keep only the merged schema:
        catalog.schemata.remove_all()
        catalog.schemata.append(schema)
