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

import functools
from fnmatch import fnmatch

import mforms

class DatabaseObjectSelector(mforms.Box):
    '''Enables the selection and filtering of database objects'''

    def __init__(self, types_to_display, database_objects, ui_settings={}):
        super(DatabaseObjectSelector, self).__init__(False)
        self.database_objects = database_objects
        self.supported_object_types = types_to_display
        
        self.ui_settings = {
            'tables' : { 'icon': 'db.Table.many.32x32.png',
                        'small_icon': 'db.Table.16x16.png',
                        'group_label': 'Migrate Table objects',
                        'group_selected': True,
                        'status_text': '%(total)d total, %(selected)d selected',
                        'show_details': False,
                      },
            'views'  : { 'icon': 'db.View.many.32x32.png',
                        'small_icon': 'db.View.16x16.png',
                        'group_label': 'Migrate View objects',
                        'group_selected': True,
                        'status_text': '%(total)d total, %(selected)d selected',
                        'show_details': False,
                      },
            'routines':{ 'icon': 'db.Routine.many.32x32.png',
                        'small_icon': 'db.Routine.16x16.png',
                        'group_label': 'Migrate Routine objects',
                        'group_selected': True,
                        'status_text': '%(total)d total, %(selected)d selected',
                        'show_details': False,
                      },
            'routineGroups':{ 'icon': 'db.RoutineGroup.48x48.png',
                        'small_icon': 'db.RoutineGroup.16x16.png',
                        'group_label': 'Migrate Routine Group/Package objects',
                        'group_selected': True,
                        'status_text': '%(total)d total, %(selected)d selected',
                        'show_details': False,
                      },
            'synonyms':{ 'icon': 'grt_object.png',
                        'small_icon': 'grt_object.png',
                        'group_label': 'Migrate Synonym objects',
                        'group_selected': True,
                        'status_text': '%(total)d total, %(selected)d selected',
                        'show_details': False,
                      },
            'structuredTypes':{ 'icon': 'grt_object.png',
                        'small_icon': 'grt_object.png',
                        'group_label': 'Migrate Structured Type objects',
                        'group_selected': True,
                        'status_text': '%(total)d total, %(selected)d selected',
                        'show_details': False,
                      },
            'sequences':{ 'icon': 'grt_object.png',
                        'small_icon': 'grt_object.png',
                        'group_label': 'Migrate Sequence objects',
                        'group_selected': True,
                        'status_text': '%(total)d total, %(selected)d selected',
                        'show_details': False,
                      },
        }
        
        # Update the ui settings dict with the custom settings supplied by the user (if any):
        if isinstance(ui_settings, dict):
            for key, value in ui_settings.iteritems():
                if key not in self.ui_settings or not isinstance(value, dict):
                    continue
                self.ui_settings[key].update(value)
        
        # Create UI:
        self.set_padding(8)
        self.set_spacing(8)
        
        self.ui = {}
        for group in self.supported_object_types:
            if group not in self.database_objects or group not in self.ui_settings:
                continue
            self.ui[group] = {}
            group_objects = self.database_objects[group]
            
            group_panel = mforms.newPanel(mforms.BorderedPanel)
            group_box = mforms.newBox(False)
            group_box.set_padding(8)
            group_box.set_spacing(8)
            
            header_box = mforms.Box(True)
            header_box.set_spacing(8)

            icon = mforms.newImageBox()
            icon.set_image(self.ui_settings[group]['icon'])
            header_box.add(icon, False, True)
            
            text_box = mforms.Box(False)
            group_selector = mforms.newCheckBox()
            group_selector.set_text(self.ui_settings[group]['group_label'])
            group_selector.set_active(bool(self.ui_settings[group]['group_selected']))
            group_selector.add_clicked_callback(functools.partial(self.group_checkbox_clicked, group=group))
            text_box.add(group_selector, False, True)
            info_label = mforms.newLabel(self.ui_settings[group]['status_text'] % {'total': len(group_objects), 
                                       'selected': len(group_objects) if self.ui_settings[group]['group_selected'] else 0 })
            info_label.set_style(mforms.SmallHelpTextStyle)
            text_box.add(info_label, False, True)
            header_box.add(text_box, False, True)
            
            show_details = self.ui_settings[group]['show_details']
            self.ui_settings[group]['_showing_details'] = show_details
            filter_button = mforms.newButton()
            filter_button.set_text('Hide Selection' if show_details else 'Show Selection')
            filter_button.set_enabled(bool(self.ui_settings[group]['group_selected']))
            filter_button.add_clicked_callback(functools.partial(self.filter_button_clicked, group=group))
            header_box.add_end(filter_button, False, True)
            
            group_box.add(header_box, False, True)
            
            # The invisible stuff:
            if len(group_objects) > 0:
                box = mforms.newBox(True)
                search_entry = mforms.newTextEntry(mforms.SearchEntry)
                search_entry.set_placeholder_text("Filter objects (wildcards chars * and ? are allowed)")
                search_entry.add_changed_callback(functools.partial(self.search_entry_changed, group=group))
                box.add(search_entry, False, True)
                group_box.add(box, True, True)
                search_entry.set_size(350, -1)

                filter_container = mforms.newBox(True)
                filter_container.set_spacing(8)

                available_list = mforms.newTreeView(mforms.TreeFlatList)
                available_list.add_column(mforms.IconColumnType, 'Available Objects', 300, False)
                available_list.end_columns()
                available_list.set_selection_mode(mforms.TreeSelectMultiple)
                available_list.set_allow_sorting(False)
                filter_container.add(available_list, True, True)

                control_box = mforms.newBox(False)
                control_box.set_padding(0, 30, 0, 30)
                control_box.set_spacing(4)
                add_button = mforms.newButton()
                add_button.set_text('>')
                add_button.enable_internal_padding(False)
                add_button.add_clicked_callback(functools.partial(self.move_button_clicked, group=group, operation='add'))
                control_box.add(add_button, False)
                remove_button = mforms.newButton()
                remove_button.set_text('<')
                remove_button.enable_internal_padding(False)
                remove_button.add_clicked_callback(functools.partial(self.move_button_clicked, group=group, operation='remove'))
                control_box.add(remove_button, False, True)
                add_all_button = mforms.newButton()
                add_all_button.set_text('>>')
                add_all_button.enable_internal_padding(False)
                add_all_button.add_clicked_callback(functools.partial(self.move_button_clicked, group=group, operation='add_all'))
                control_box.add(add_all_button, False, True)
                remove_all_button = mforms.newButton()
                remove_all_button.set_text('<<')
                remove_all_button.enable_internal_padding(False)
                remove_all_button.add_clicked_callback(functools.partial(self.move_button_clicked, group=group, operation='remove_all'))
                control_box.add(remove_all_button, False)
                filter_container.add(control_box, False, True)


                selected_list = mforms.newTreeView(mforms.TreeFlatList)
                selected_list.add_column(mforms.IconColumnType, 'Objects to Migrate', 300, False)
                selected_list.end_columns()
                selected_list.set_selection_mode(mforms.TreeSelectMultiple)
                selected_list.set_allow_sorting(False)
                for item in sorted(group_objects):
                    node = selected_list.add_node()
                    node.set_icon_path(0, self.ui_settings[group]['small_icon'])
                    node.set_string(0, item)
                filter_container.add(selected_list, True, True)
                
                group_box.add(filter_container, True, True)
                
                filter_container.show(bool(show_details))
            
                self.ui[group].update( {
                    'filter_container': filter_container,
                    'available_list': available_list,
                    'selected_list': selected_list,
                    'search_entry': search_entry,
                    } )
            else:  # Empty object list
                filter_button.set_enabled(False)

            self.ui[group].update ( {
                'icon': icon,
                'group_selector': group_selector,
                'group_panel': group_panel,
                'info_label': info_label,
                'filter_button': filter_button,
                'all_objects': set(group_objects),
                'has_elements': bool(len(group_objects)),
                'available': set(),
                'selected': set(group_objects),
                'objects_passing_filter': set(group_objects),
                }  )
            group_panel.add(group_box)
            self.add(group_panel, False, True)

    def _show_selection(self, group, show_details):
        '''Shows/hides the details of the selections for the group'''

        if 'filter_container' in self.ui[group]:  # The group is not empty
            self.ui[group]['filter_container'].show(bool(show_details))
        self.ui[group]['filter_button'].set_text('Hide Selection' if show_details else 'Show Selection')
        self.ui_settings[group]['_showing_details'] = show_details
        self.relayout()

    def filter_button_clicked(self, group):
        self._show_selection(group, not self.ui_settings[group]['_showing_details'])
        
    def search_entry_changed(self, group):
        search_entry = self.ui[group]['search_entry']
        search_pattern = search_entry.get_string_value()

        available_list = self.ui[group]['available_list']
        selected_list = self.ui[group]['selected_list']
        all_objects = self.ui[group]['all_objects']
        available = self.ui[group]['available']
        selected = self.ui[group]['selected']
        
        objects_passing_filter = set(object_name for object_name in all_objects if fnmatch(object_name, search_pattern)) if search_pattern else all_objects
        self.ui[group]['objects_passing_filter'] = objects_passing_filter
        
        selected_list.clear()
        selected_list.freeze_refresh()
        for item in sorted(list(selected & objects_passing_filter)):
            node = selected_list.add_node()
            node.set_icon_path(0, self.ui_settings[group]['small_icon'])
            node.set_string(0, item)
        selected_list.thaw_refresh()

        available_list.clear()
        available_list.freeze_refresh()
        for item in sorted(list(available & objects_passing_filter)):
            node = available_list.add_node()
            node.set_icon_path(0, self.ui_settings[group]['small_icon'])
            node.set_string(0, item)
        available_list.thaw_refresh()

    def group_checkbox_clicked(self, group):
        is_active = bool(self.ui[group]['group_selector'].get_active())
        has_elements = bool(self.ui[group]['has_elements'])
        should_expand = is_active and self.ui_settings[group]['show_details']
        self._show_selection(group, should_expand)
        self.ui[group]['filter_button'].set_enabled(is_active and has_elements)
        self.ui[group]['info_label'].set_text(self.ui_settings[group]['status_text'] % {'total': len(self.ui[group]['all_objects']), 
                                                           'selected': len(self.ui[group]['selected']) if is_active else 0 })

    def move_button_clicked(self, group, operation):
        available_list = self.ui[group]['available_list']
        selected_list = self.ui[group]['selected_list']
        all_objects = self.ui[group]['all_objects']
        available = self.ui[group]['available']
        selected = self.ui[group]['selected']
        objects_passing_filter = self.ui[group]['objects_passing_filter']
        
        if operation == 'add':
            involved = set(node.get_string(0) for node in available_list.get_selection() )
            selected |= involved
            available -= involved
        elif operation == 'remove':
            involved = set(node.get_string(0) for node in selected_list.get_selection() )
            available |= involved
            selected -= involved
        elif operation == 'add_all':
            selected |= (available & objects_passing_filter)
            available -= objects_passing_filter
        elif operation == 'remove_all':
            available |= (selected & objects_passing_filter)
            selected -= objects_passing_filter

        selected_list.clear()
        selected_list.freeze_refresh()
        for item in sorted(list(selected & objects_passing_filter)):
            node = selected_list.add_node()
            node.set_icon_path(0, self.ui_settings[group]['small_icon'])
            node.set_string(0, item)
        selected_list.thaw_refresh()

        available_list.clear()
        available_list.freeze_refresh()
        for item in sorted(list(available & objects_passing_filter)):
            node = available_list.add_node()
            node.set_icon_path(0, self.ui_settings[group]['small_icon'])
            node.set_string(0, item)
        available_list.thaw_refresh()
        
        self.ui[group]['info_label'].set_text(self.ui_settings[group]['status_text'] % {'total': len(all_objects), 
                                                                                   'selected': len(selected) })

    def _get_objects(self, list_type):
        relevant_objects = {}
        for group in self.supported_object_types:
            if group not in self.database_objects or group not in self.ui_settings:
                continue
            if self.ui[group]['group_selector'].get_active():  # the group checkbox is checked
                relevant_objects[group] = list(self.ui[group][list_type])
            else:
                relevant_objects[group] = list(self.ui[group]['all_objects']) if list_type == 'available' else []
        return relevant_objects
    
    def get_ignored_objects(self):
        return self._get_objects('available')

    def get_selected_objects(self):
        return self._get_objects('selected')
