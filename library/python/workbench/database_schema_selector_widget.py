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

import functools

import mforms

class DatabaseSchemaSelector(mforms.Box):
    '''Widget to select database schemata within a collection of catalogs (optional) and schemata

    This wigdet will help you to display a set of schemata in a nice tree structure, grouping them
    by catalogs (if the catalog information is supported/available). Next to each listed schema is
    a checkbox for the selection/unselection of the schema. The user can quickly select all of the
    schemata for a given catalog by clicking on the checkbox next to the catalog name.

    .. image:: ../../../library/python/workbench/doc/images/schema_selection_widget.png


    :param database_objects:      Pass a list/tuple of schema names if you don't use catalogs.
                                  If you do use catalogs then pass a list of tuples of the form
                                  ``(catalog_name, schema_list)`` where :attr:`schema_list` is a
                                  list/tuple with the schemata that belongs to the given :attr:`catalog_name`.
    :type database_objects:       list/tuple

    :param tree_checked_callback: A function object that will be called by this widget whenever
                                  a schema is checked/unchecked. It should accept an integer
                                  argument with the number of selected schemata.
    :type tree_checked_callback:  callable

    :param ui_settings:           A dictionary to customize the appearance of the widget. In
                                  case you need to customize something, just pass the relevant
                                  modifications in this dict. The other (default) settings are
                                  kept. Check the definition of :attr:`self.ui_settings` in the
                                  class implementation to see the configurable settings. 
    :type ui_settings:            dict

    .. rubric:: Sample usage:
    ::

        db_objects = [ ('catalog1', ('schema1', 'schema2')),
                       ('catalog2', ('schema3', 'schema4'))
                     ] 

        content = mforms.newBox()
        object_selector = DatabaseObjectSelector(db_objects)
        content.add(object_selector, True)
    '''

    def __init__(self, database_objects, tree_checked_callback=lambda n:None, ui_settings={}):
        '''Constructor.
        '''
        super(DatabaseSchemaSelector, self).__init__(False)
        self.database_objects = database_objects
        self.tree_checked_callback = tree_checked_callback
      
        # Check if we should use catalogs:
        self.use_catalogs = ( isinstance(database_objects, (list, tuple)) and
                           len(database_objects) > 0 and
                           all( isinstance(item, (list, tuple)) and
                               len(item) == 2 and
                               isinstance(item[0], (str, unicode)) and
                               isinstance(item[1], (list, tuple))
                               for item in database_objects
                               )
                           )
      
      
        self.ui_settings = {
            'catalogs' : { 'icon': 'workbench.physical.Layer.16x16.png',
                           'should_expand': True,
                         },
            'schemas'  : { 'icon': 'db.Schema.16x16.png',
                         },
            'general'  : { 'summary_text': '%(nSchemata)d schemata selected',
                           'show_select_all_button': not self.use_catalogs,
                           'show_unselect_all_button': True,
                         },
        }
        
        # Update the ui settings dict with the custom settings supplied by the user (if any):
        if isinstance(ui_settings, dict):
            for key, value in ui_settings.iteritems():
                if key not in self.ui_settings or not isinstance(value, dict):
                    continue
                self.ui_settings[key].update(value)
        

        if self.use_catalogs:
            self._catalogs = tuple(item[0] for item in database_objects)
            self._schemata = dict(database_objects)
        else:
            self._catalogs = None
            self._schemata = database_objects

        self._schema_nodes = []
        self._catalog_nodes = []

        # Create UI:
        self.set_spacing(8)
        
        self.schema_list_tree = mforms.newTreeView(mforms.TreeDefault)
        self.schema_list_tree.add_column(mforms.CheckColumnType, 'Include', 60, True)
        self.schema_list_tree.add_column(mforms.IconColumnType, 'Catalog/Schema' if self.use_catalogs else 'Schema', 300, False)
        self.schema_list_tree.end_columns()
        self.schema_list_tree.set_cell_edited_callback(self._schema_tree_checked)
        self.schema_list_tree.set_allow_sorting(False)
        self.add(self.schema_list_tree, True, True)

        helper_buttons_box = mforms.newBox(True)
        helper_buttons_box.set_spacing(12)

        self.select_summary_label = mforms.newLabel('')
        helper_buttons_box.add(self.select_summary_label, False, True)

        if self.ui_settings['general']['show_unselect_all_button']:
            self.unselect_all_btn = mforms.newButton()
            self.unselect_all_btn.set_text('Unselect All')
            self.unselect_all_btn.add_clicked_callback(functools.partial(self._mark_all_schemata, checked=False))
            helper_buttons_box.add_end(self.unselect_all_btn, False)

        if self.ui_settings['general']['show_select_all_button']:
            self.select_all_btn = mforms.newButton()
            self.select_all_btn.set_text('Select All')
            self.select_all_btn.add_clicked_callback(functools.partial(self._mark_all_schemata, checked=True))
            helper_buttons_box.add_end(self.select_all_btn, False)


        self.add(helper_buttons_box, False, True)
        
        self._fill_schema_tree()
        
    def _fill_schema_tree(self):
        self._schema_nodes = []
        self._catalog_nodes = []
        if self.use_catalogs:
            for catalog_name in self._catalogs:
                catalog_node = self.schema_list_tree.add_node()
                catalog_node.set_bool(0, False)
                catalog_node.set_icon_path(1, self.ui_settings['catalogs']['icon'])
                catalog_node.set_string(1, catalog_name)
                catalog_node.set_tag(catalog_name)
                self._catalog_nodes.append(catalog_node)
                
                for schema_name in self._schemata[catalog_name]:
                    schema_node = catalog_node.add_child()
                    schema_node.set_bool(0, False)
                    schema_node.set_icon_path(1, self.ui_settings['schemas']['icon'])
                    schema_node.set_string(1, schema_name)
                    schema_node.set_tag(catalog_name + '.' + schema_name)
                    self._schema_nodes.append(schema_node)
                
                if self.ui_settings['catalogs']['should_expand']:
                    catalog_node.expand()
        else:  # Not using catalogs
            for schema_name in self._schemata:
                schema_node = self.schema_list_tree.add_node()
                schema_node.set_bool(0, False)
                schema_node.set_icon_path(1, self.ui_settings['schemas']['icon'])
                schema_node.set_string(1, schema_name)
                schema_node.set_tag(schema_name)
                self._schema_nodes.append(schema_node)
            if len(self._schemata) == 1:
                self.schema_list_tree.select_node(schema_node)
            
        self.select_summary_label.set_text(self.ui_settings['general']['summary_text'] % {'nSchemata':0})
        
    def _schema_tree_checked(self, node_list, col, data):
        # This allows to change the status of several nodes at once too:
        if not isinstance(node_list, list):
            node_list = [node_list]
        for node in node_list:
            if col == 0:
                checked = data == '1'
                node.set_bool(0, checked)
                if self.use_catalogs:
                    parent_node = node.get_parent()
                    is_catalog = parent_node == self.schema_list_tree.root_node()
                    catalog_name = node.get_string(1) if is_catalog else parent_node.get_string(1)
                    schema_count = len(self._schemata[catalog_name])
                    if is_catalog:
                        for idx in range(schema_count):
                            node.get_child(idx).set_bool(0, checked)
                    else:
                        # Check/uncheck parent catalog if all of its schemata are checked/unchecked:
                        parent_node.set_bool(0, checked and all( schema_node.get_bool(0) for schema_node in ( parent_node.get_child(idx) for idx in range(schema_count) ) ) )

        selected_schema_count = len(tuple(schema_node for schema_node in self._schema_nodes if schema_node.get_bool(0)))
        self.select_summary_label.set_text(self.ui_settings['general']['summary_text'] % {'nSchemata':selected_schema_count})
        self.tree_checked_callback(selected_schema_count)
        
    def _select_objects(self, checked):
        if self.use_catalogs:
            catalog_schemata = {}
            for schema_node in self._schema_nodes:
                if schema_node.get_bool(0) != checked:
                    continue
                catalog_name, dot, schema_name = schema_node.get_tag().rpartition('.')
                catalog_schemata.setdefault(catalog_name, []).append(schema_name)
            return catalog_schemata
        else:
            return [ schema_node.get_string(1) for schema_node in self._schema_nodes if schema_node.get_bool(0) == checked ]
    
    def get_selected(self):
        return self._select_objects(True)
    
    def get_ignored(self):
        return self._select_objects(False)
    
    def _mark_all_schemata(self, checked):
        nodes = self._catalog_nodes if self.use_catalogs else self._schema_nodes
        self._schema_tree_checked(nodes, 0, '1' if checked else '0')
