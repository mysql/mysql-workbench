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
from workbench.ui import WizardPage

class SchemaMappingsOptionsView(WizardPage):

    def __init__(self, main):
        WizardPage.__init__(self, main, 'Schema Mappings')

        self.main.add_wizard_page(self, 'OBJECT MIGRATION', 'Schema Mappings')

        self.schemata = None
        self.rid = mforms.RadioButton.new_id()
        self.options = []

    def _merge_schemata(self, prefix=''):
        catalog = self.main.plan.migrationSource.catalog
        schema = catalog.schemata[0]

        # otypes is something like ['tables', 'views', 'routines']:
        otypes = [ suptype[0] for suptype in self.main.plan.migrationSource.supportedObjectTypes ]

        # Update names for the objects of this first schema:
        if prefix:
            actual_prefix = (schema.name if prefix == 'schema_name' else schema.__id__) + '_'
            for otype in otypes:
                for obj in getattr(schema, otype):
                    obj.name = actual_prefix + obj.name

        schema.name = catalog.name
        if not prefix:
            known_names = dict( (otype, set(obj.name for obj in getattr(schema, otype))) for otype in otypes)

        for other_schema in list(catalog.schemata)[1:]:
            if other_schema.defaultCharacterSetName != schema.defaultCharacterSetName:
                self.main.plan.state.addMigrationLogEntry(0, 'Schema Mappings', other_schema,
                      'While merging schema %s into %s: Default charset for schemas differs (%s vs %s). Setting default charset to %s' % (other_schema.name, schema.name, other_schema.defaultCharacterSetName, schema.defaultCharacterSetName, schema.defaultCharacterSetName))

            if other_schema.defaultCollationName != schema.defaultCollationName:
                self.main.plan.state.addMigrationLogEntry(0, 'Schema Mappings', other_schema,
                      'While merging schema %s into %s: Default collation for schemas differs (%s vs %s). Setting default collation to %s' % (other_schema.name, schema.name, other_schema.defaultCollationName, schema.defaultCollationName, schema.defaultCollationName))

            for otype in otypes:
                other_objects = getattr(other_schema, otype)
                if not prefix:
                    repeated_object_names = known_names[otype].intersection(obj.name for obj in other_objects)
                    if repeated_object_names:
                        objects_dict = dict( (obj.name, obj) for obj in other_objects )
                        for repeated_object_name in repeated_object_names:
                            objects_dict[repeated_object_name].name += '_' + other_schema.name
                            self.main.plan.state.addMigrationLogEntry(0, 'Schema Mappings', other_schema,
                                  'The name of the %(otype)s "%(oname)s" conflicts with other %(otype)s names: renamed to "%(onewname)s"' % { 'otype':otype[:-1],
                                                                                                                                              'oname':repeated_object_name,
                                                                                                                                              'onewname':objects_dict[repeated_object_name].name }
                                                                      )
                        known_names[otype].update(other_objects)
                else:
                    actual_prefix = (other_schema.name if prefix == 'schema_name' else schema.__id__) + '_'

                getattr(schema, otype).extend(other_objects)
                for obj in other_objects:
                    obj.owner = schema
                    if prefix:
                        obj.name = actual_prefix + obj.name

        # Keep only the merged schema:
        catalog.schemata.remove_all()
        catalog.schemata.append(schema)


    def create_ui(self):
        if self.main.plan.migrationSource and self.main.plan.migrationSource.catalog and self.main.plan.migrationSource.catalog.schemata:
            optionspanel = mforms.newPanel(mforms.TitledBoxPanel)
            optionspanel.set_title('Choose how your schemas will be mapped')
            optionsbox = mforms.newBox(False)
            optionsbox.set_padding(8)
            optionsbox.set_spacing(6)

            options = [ 'Keep schemas as they are: Catalog.Schema.Table -> Schema.Table',
                        'Only one schema: Catalog.Schema.Table -> Catalog.Table',
                        'Only one schema, keep current schema names as a prefix: Catalog.Schema.Table -> Catalog.Schema__Table',
                      ]
            self.options = []
            for opt in options:
                radio_button = mforms.newRadioButton(self.rid)
                radio_button.set_text(opt)
                optionsbox.add(radio_button, False)
                self.options.append(radio_button)


            optionspanel.add(optionsbox)
            self.content.add(optionspanel, False)
        else:
            self.go_next()


    def go_next(self):

        def find_selected_option():  #TODO: When we finally drop py2.5 support substitute this with self.options.index(next(opt for opt in self.options if opt.get_active()))
            for idx, option_radio in enumerate(self.options):
                if option_radio.get_active():
                    return idx
            return None

        selected_option = find_selected_option()

        # If selected option == 0 we don't do anything
        if selected_option == 1:
            self._merge_schemata()
        elif selected_option == 2:
            self._merge_schemata(prefix='schema_name')

        super(SchemaMappingsOptionsView, self).go_next()
