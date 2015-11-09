# Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.
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



class DatatypeMappingEditor(mforms.Form):
    def __init__(self):
        mforms.Form.__init__(self, None, mforms.FormDialogFrame|mforms.FormResizable|mforms.FormMinimizable)
        
        self.set_title("Data Type Mapping for Generic Migration")
        
        content = mforms.newBox(False)
        self.set_content(content)
        content.set_padding(12)
        content.set_spacing(12)
        
        hbox = mforms.newBox(True)
        content.add(hbox, True, True)
        hbox.set_spacing(12)
        
        self._type_list = mforms.newTreeView(mforms.TreeFlatList)#|mforms.TreeAllowReorderRows)
        self._type_list.set_size(200, -1)
        self._type_list.add_column(mforms.StringColumnType, "Type", 100, False)
        self._type_list.add_column(mforms.StringColumnType, "Target Type", 100, False)
        self._type_list.end_columns()
        self._type_list.add_changed_callback(self.selection_changed)
        hbox.add(self._type_list, False, True)

        detail_box = mforms.newBox(False)
        self.detail_box = detail_box
        hbox.add(detail_box, True, True)
        detail_box.set_spacing(12)
        
        ##
        
        spanel = mforms.newPanel(mforms.TitledBoxPanel)
        spanel.set_title("Source Data Type")
        
        stable = mforms.newTable()
        stable.set_padding(12)
        spanel.add(stable)
        stable.set_row_count(5)
        stable.set_column_count(2)
        stable.set_row_spacing(8)
        stable.set_column_spacing(4)

        stable.add(mforms.newLabel("Type Name:", True), 0, 1, 0, 1, mforms.HFillFlag)
        self._stype_entry = mforms.newTextEntry()
        self._stype_entry.add_changed_callback(self.save_changes)
        stable.add(self._stype_entry, 1, 2, 0, 1, mforms.HFillFlag|mforms.HExpandFlag) 

        #self._stype_user_check = mforms.newCheckBox()
        #self._stype_user_check.set_text("Is user datatype")
        #stable.add(self._stype_user_check, 1, 2, 1, 2, mforms.HFillFlag|mforms.HExpandFlag) 
        
        stable.add(mforms.newLabel("Type Category:", True), 0, 1, 2, 3, mforms.HFillFlag)
        self._sgroup_selector = mforms.newSelector()
        stable.add(self._sgroup_selector, 1, 2, 2, 3, mforms.HFillFlag|mforms.HExpandFlag) 
        self._sgroup_selector.add_changed_callback(self.source_group_selected)

        stable.add(mforms.newLabel("Min. Length:", True), 0, 1, 3, 4, mforms.HFillFlag)
        self._sminlen_entry = mforms.newTextEntry()
        self._sminlen_entry.add_changed_callback(self.save_changes)
        stable.add(self._sminlen_entry, 1, 2, 3, 4, mforms.HFillFlag|mforms.HExpandFlag) 
        stable.add(mforms.newLabel("Max. Length:", True), 0, 1, 4, 5, mforms.HFillFlag)
        self._smaxlen_entry = mforms.newTextEntry()
        self._smaxlen_entry.add_changed_callback(self.save_changes)
        stable.add(self._smaxlen_entry, 1, 2, 4, 5, mforms.HFillFlag|mforms.HExpandFlag) 

        detail_box.add(spanel, False, True)
        
        ##
        
        tpanel = mforms.newPanel(mforms.TitledBoxPanel)
        tpanel.set_title("Target MySQL Data Type")
        
        ttable = mforms.newTable()
        ttable.set_padding(12)
        tpanel.add(ttable)
        ttable.set_row_count(4)
        ttable.set_column_count(2)
        ttable.set_row_spacing(8)
        ttable.set_column_spacing(4)

        ttable.add(mforms.newLabel("Target Type:", True), 0, 1, 0, 1, mforms.HFillFlag)
        self._ttype_selector = mforms.newSelector()
        self._ttype_selector.add_changed_callback(self.save_changes)
        ttable.add(self._ttype_selector, 1, 2, 0, 1, mforms.HFillFlag|mforms.HExpandFlag) 
        
        def add_check_entry_row(table, row, label, name):
            check = mforms.newCheckBox()
            check.set_text(label)
            table.add(check, 0, 1, row, row+1, mforms.HFillFlag)
            entry = mforms.newTextEntry()
            entry.add_changed_callback(self.save_changes)
            table.add(entry, 1, 2, row, row+1, mforms.HFillFlag|mforms.HExpandFlag) 
            setattr(self, name+"_check", check)
            setattr(self, name+"_entry", entry)
            entry.set_enabled(False)
            def callback(entry, check):
                if not check.get_active():
                    entry.set_value("-2")
                entry.set_enabled(check.get_active())
                self.save_changes()
            check.add_clicked_callback(lambda: callback(entry, check))

        add_check_entry_row(ttable, 1, "Override Length:", "_target_length")
        add_check_entry_row(ttable, 2, "Override Precision:", "_target_precision")
        add_check_entry_row(ttable, 3, "Override Scale:", "_target_scale")

        detail_box.add(tpanel, False, True)
        
        ##
        
        bbox = mforms.newBox(True)
        
        self._add_button = mforms.newButton()
        self._add_button.set_text("Add")
        bbox.add(self._add_button, False, True)
        self._add_button.add_clicked_callback(self.add_clicked)
        self._del_button = mforms.newButton()
        self._del_button.set_text("Delete")
        self._del_button.add_clicked_callback(self.del_clicked)
        bbox.add(self._del_button, False, True)

        self._ok_button = mforms.newButton()
        self._ok_button.set_text("OK")
        self._ok_button.add_clicked_callback(self.ok_clicked)
        self._cancel_button = mforms.newButton()
        self._cancel_button.set_text("Cancel")
        self._ok_button.add_clicked_callback(self.cancel_clicked)
        mforms.Utilities.add_end_ok_cancel_buttons(bbox, self._ok_button, self._cancel_button)
        content.add_end(bbox, False, True)
        bbox.set_spacing(12)
        self._del_button.set_enabled(False)

        self.set_size(700, 500)
    

    def run(self, type_mapping):        
        self.type_mapping = type_mapping

        for g in grt.root.wb.rdbmsMgmt.datatypeGroups: 
            if g.name not in ("structured", "userdefined"):
                self._sgroup_selector.add_item(g.name)

        self.refresh_type_list()
        return self.run_modal(self._ok_button, self._cancel_button)


    def add_clicked(self):
        tmap = grt.classes.db_migration_DatatypeMapping()
        tmap.sourceDatatypeName = "string"
        tmap.targetDatatypeName = "VARCHAR"
        self.type_mapping.append(tmap)
        node = self._type_list.add_node()
        node.set_string(0, tmap.sourceDatatypeName)
        node.set_string(1, tmap.targetDatatypeName)
        node.set_tag(tmap.__id__)
        
        self._type_list.select_node(node)
        self.selection_changed()

 
    def del_clicked(self):
        node = self._type_list.get_selected_node()
        if node:
            tag = node.get_tag()
            for o in self.type_mapping:
                if o.__id__ == tag:
                    self.type_mapping.remove(o)
                    break
            node.remove_from_parent()


    def refresh_type_list(self):
        self._type_list.clear()
        for tmap in self.type_mapping:
            node = self._type_list.add_node()
            node.set_string(0, tmap.sourceDatatypeName)
            node.set_string(1, tmap.targetDatatypeName)
            node.set_tag(tmap.__id__)


    def selection_changed(self):
        node = self._type_list.get_selected_node()
        if node:
            self.detail_box.set_enabled(True)
            tag = node.get_tag()
            tmap = None
            for o in self.type_mapping:
                if o.__id__ == tag:
                    tmap = o
                    break
            if not tmap:
                return

            self.show_type(tmap)

            self._del_button.set_enabled(True)
        else:
            self.detail_box.set_enabled(False)
            self._del_button.set_enabled(False)


    def ok_clicked(self):
        self.end_modal(True)


    def cancel_clicked(self):
        self.end_modal(False)


    def save_changes(self):
        node = self._type_list.get_selected_node()
        if node:
            tag = node.get_tag()
            tmap = None
            for o in self.type_mapping:
                if o.__id__ == tag:
                    tmap = o
                    break
            if not tmap:
                return

            tmap.sourceDatatypeName = self._stype_entry.get_string_value()
            tmap.targetDatatypeName = self._ttype_selector.get_string_value()
            
            node.set_string(0, tmap.sourceDatatypeName)
            node.set_string(1, tmap.targetDatatypeName)
    
            try:
                tmap.length = int(self._target_length_entry.get_string_value())
            except:
                pass
            try:
                tmap.precision = int(self._target_precision_entry.get_string_value())
            except:
                pass
            try:
                tmap.scale = int(self._target_scale_entry.get_string_value())
            except:
                pass
            try:
                tmap.lengthConditionTo = int(self._smaxlen_entry.get_string_value())
            except:
                pass
            try:
                tmap.lengthConditionFrom = int(self._sminlen_entry.get_string_value())
            except:
                pass

    def show_type(self, tmap):
        # the MySQL rdbms
        rdbms = grt.root.wb.rdbmsMgmt.rdbms[0]

        self._stype_entry.set_value(tmap.sourceDatatypeName)
        
        dtype = rdbms.simpleDatatypes[0]
        for dt in rdbms.simpleDatatypes:
            if dt.name == tmap.targetDatatypeName:
                dtype = dt
                break

        i = self._sgroup_selector.index_of_item_with_title(dtype.group.name)
        if i >= 0:
            self._sgroup_selector.set_selected(i)
            self._ttype_selector.clear()
            for t in rdbms.simpleDatatypes:
                if t.group == dtype.group:
                    self._ttype_selector.add_item(t.name)

            i = self._ttype_selector.index_of_item_with_title(dtype.name)
            if i >= 0:
                self._ttype_selector.set_selected(i)

        
        self._sminlen_entry.set_value(str(tmap.lengthConditionFrom))
        self._smaxlen_entry.set_value(str(tmap.lengthConditionTo))

        if tmap.length != -2:
            self._target_length_entry.set_enabled(True)
            self._target_length_check.set_active(True)
        else:
            self._target_length_entry.set_enabled(False)
            self._target_length_check.set_active(False)
        self._target_length_entry.set_value(str(tmap.length))
        if tmap.scale != -2:
            self._target_scale_entry.set_enabled(True)
            self._target_scale_check.set_active(True)
        else:
            self._target_scale_entry.set_enabled(False)
            self._target_scale_check.set_active(False)
        self._target_scale_entry.set_value(str(tmap.scale))
        if tmap.precision != -2:
            self._target_precision_entry.set_enabled(True)
            self._target_precision_check.set_active(True)
        else:
            self._target_precision_entry.set_enabled(False)
            self._target_precision_check.set_active(False)
        self._target_precision_entry.set_value(str(tmap.precision))


    def source_group_selected(self):
        # the MySQL rdbms
        rdbms = grt.root.wb.rdbmsMgmt.rdbms[0]

        gname = self._sgroup_selector.get_string_value()

        group = [g for g in grt.root.wb.rdbmsMgmt.datatypeGroups if g.name == gname][0]

        self._ttype_selector.clear()
        for t in rdbms.simpleDatatypes:
            if t.group == group:
                self._ttype_selector.add_item(t.name)

        has_length = gname in ["text", "blob", "string", "various"]
        has_scale = gname in ["numeric"]

        self._target_length_check.set_enabled(has_length)
        self._target_length_entry.set_enabled(has_length)

        self._target_scale_check.set_enabled(has_scale)
        self._target_scale_entry.set_enabled(has_scale)

        self._target_precision_check.set_enabled(has_scale)
        self._target_precision_entry.set_enabled(has_scale)
        
        self.save_changes()

        
def run():
    import grt
    import os
    
    datadir = mforms.App.get().get_user_data_folder()
    path = datadir + "/migration_generic_typemap.xml"
    if grt.root.wb.migration:
        typemap = grt.root.wb.migration.genericDatatypeMappings
    else:        
        if os.path.exists(path):
            typemap = grt.unserialize(path)
        else:
            global_path = mforms.App.get().get_resource_path("")
            global_path += "/modules/data/migration_generic_typemap.xml"
            if os.path.exists(global_path):
                typemap = grt.unserialize(global_path)
            else:
                typemap = grt.List(grt.OBJECT, "db.migration.DatatypeMapping")
    form = DatatypeMappingEditor()
    if form.run(typemap):
        grt.serialize(typemap, path)
        return 1
    return 0
