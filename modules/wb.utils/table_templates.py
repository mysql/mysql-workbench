# Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

import os, sys
from xml.dom import minidom


class TemplateEditor(mforms.Form):
    def __init__(self, owner):
        mforms.Form.__init__(self, None, mforms.FormDialogFrame|mforms.FormResizable|mforms.FormMinimizable)
        self.owner = owner
        self.tables_by_id = {}

        self.set_title("Table Templates")

        box = mforms.newBox(False)
        box.set_padding(12)
        box.set_spacing(12)

        label = mforms.newLabel("Manage templates of tables with pre-defined columns, for frequently used table structures.")
        box.add(label, False, True)

        top = mforms.newBox(True)
        box.add(top, False, True)

        #top.set_padding(12)
        top.set_spacing(12)
        self.template_list = mforms.newTreeView(mforms.TreeFlatList)
        self.template_list.add_column(mforms.IconStringColumnType, "Table Template", 200, True)
        self.template_list.end_columns()
        self.template_list.add_changed_callback(self.table_selected)
        self.template_list.set_cell_edited_callback(self.table_edited)
        top.add(self.template_list, True, True)
        if sys.platform.lower() != "darwin":
            self.template_list.set_size(-1, 150)

        bbox = mforms.newBox(False)
        bbox.set_spacing(8)
        top.add(bbox, False, True)

        self.add = mforms.newButton()
        self.add.set_text("New Template")
        self.add.add_clicked_callback(self.add_templ)
        bbox.add(self.add, False, True)

        self.duplicate = mforms.newButton()
        self.duplicate.set_text("Duplicate")
        self.duplicate.add_clicked_callback(self.dup_templ)
        bbox.add(self.duplicate, False, True)

        self.delete = mforms.newButton()
        self.delete.set_text("Delete")
        self.delete.add_clicked_callback(self.del_templ)
        bbox.add(self.delete, False, True)

        hbox = mforms.newBox(True)
        hbox.set_spacing(12)
      
        self.column_list = mforms.newTreeView(mforms.TreeFlatList)
        self.column_list.add_column(mforms.IconStringColumnType, "Column", 100, True)
        self.column_list.add_column(mforms.StringColumnType, "Datatype", 100, True)
        self.column_list.add_column(mforms.StringColumnType, "Default", 150, True)
        self.column_list.add_column(mforms.CheckColumnType, "PK", 25, True)
        self.column_list.add_column(mforms.CheckColumnType, "NN", 25, True)
        self.column_list.add_column(mforms.CheckColumnType, "UQ", 25, True)
        self.column_list.add_column(mforms.CheckColumnType, "AI", 25, True)
        self.column_list.end_columns()
        self.column_list.set_cell_edited_callback(self.column_edited)
        self.column_list.add_changed_callback(self.column_selected)
        hbox.add(self.column_list, True, True)
      
        vbox = mforms.newBox(False)
        vbox.set_spacing(8)
      
        vbox.add(mforms.newLabel("Column Collation:"), False, True)
        self.charset = mforms.newSelector(mforms.SelectorPopup)
        self.charset.add_changed_callback(self.collation_changed)
        collations = ["Table Default"]
        for ch in grt.root.wb.rdbmsMgmt.rdbms[0].characterSets:
            collations += ch.collations
        self.charset.add_items(collations)
        vbox.add(self.charset, False, True)

        vbox.add(mforms.newLabel("Additional Flags:"), False, True)
        self.flag_checkboxes = []
        hbox.add(vbox, False, True)
        self.column_details = vbox
      
        box.add(hbox, True, True)

        self.column_menu = mforms.newContextMenu()
        self.column_menu.add_item_with_title("Delete", self.delete_column)
        self.column_list.set_context_menu(self.column_menu)

        bbox = mforms.newBox(True)
        self.ok = mforms.newButton()
        self.ok.set_text("Close")
        bbox.add_end(self.ok, False, True)

        box.add(bbox, False, True)

        self.set_content(box)
        self.set_size(800, 500)
        self.center()

        self.refresh_tables()


    def add_templ(self):
        table = grt.classes.db_mysql_Table()
        table.name = "template %i" % (len(self.owner.templates)+1)
        self.tables_by_id[table.__id__] = table
        self.owner.templates.append(table)

        node = self.template_list.add_node()
        node.set_icon_path(0, mforms.App.get().get_resource_path("db.Table.16x16.png"))
        node.set_string(0, table.name)
        node.set_tag(table.__id__)
        self.template_list.select_node(node)
        self.table_selected()


    def del_templ(self):
        node = self.template_list.get_selected_node()
        if node:
            table = self.tables_by_id[node.get_tag()]
            del self.tables_by_id[node.get_tag()]
            self.owner.templates.remove(table)
            node.remove_from_parent()


    def dup_templ(self):
        orig = self.selected_table()

        table = self.owner.copy_table(orig)

        self.tables_by_id[table.__id__] = table
        self.owner.templates.append(table)

        node = self.template_list.add_node()
        node.set_icon_path(0, mforms.App.get().get_resource_path("db.Table.16x16.png"))
        node.set_string(0, table.name)
        node.set_tag(table.__id__)
        self.template_list.select_node(node)
        self.table_selected()


    def delete_column(self):
        table = self.selected_table()
        node = self.column_list.get_selected_node()
        if node and table:
            i = self.column_list.row_for_node(node)
            if i < len(table.columns):
                del table.columns[i]
                node.remove_from_parent()


    def select_template(self, name):
        for i in range(self.template_list.count()):
            node = self.template_list.node_at_row(i)
            if node.get_string(0) == name:
                self.template_list.select_node(node)
                self.table_selected()
                break


    def table_edited(self, node, column, new_value):
        table = self.selected_table()
        if table:
            node.set_string(column, new_value)
            table.name = new_value


    def table_selected(self):
        self.refresh_columns()


    def refresh_tables(self):
        self.tables_by_id = {}
        icon = mforms.App.get().get_resource_path("db.Table.16x16.png")
        for table in self.owner.templates:
            node = self.template_list.add_node()
            node.set_icon_path(0, icon)
            node.set_string(0, table.name)
            node.set_tag(table.__id__)
            self.tables_by_id[table.__id__] = table


    def selected_table(self):
        node = self.template_list.get_selected_node()
        if node:
            return self.tables_by_id[node.get_tag()]
        return None
    
    
    def selected_column(self):
        node = self.column_list.get_selected_node()
        table = self.selected_table()
        if node and table:
            row = self.column_list.row_for_node(node)
            if row < len(table.columns):
                return table.columns[row]
        return None


    def show_column_node(self, node, column):
        node.set_icon_path(0, mforms.App.get().get_resource_path("db.Column.16x16.png"))
        node.set_string(0, column.name)
        node.set_string(1, column.formattedType)
        node.set_string(2, "NULL" if column.defaultValue is None else column.defaultValue)
        node.set_int(3, column.owner.isPrimaryKeyColumn(column))
        node.set_int(4, column.isNotNull)
        node.set_int(5, "UNIQUE" in column.flags)
        node.set_int(6, column.autoIncrement)
        return node


    def refresh_columns(self):
        self.column_list.clear()
        table = self.selected_table()
        if table:
            for column in table.columns:
                node = self.column_list.add_node()
                self.show_column_node(node, column)

            node = self.column_list.add_node()
            node.set_string(0, "Click to add")
            node.set_tag("placeholder")


    def column_selected(self):
        column = self.selected_column()
        if column:
            self.column_details.set_enabled(True)
            for c in self.flag_checkboxes:
                self.column_details.remove(c)
            self.flag_checkboxes = []

            if column.simpleType:
                for flag in column.simpleType.flags:
                    check = mforms.newCheckBox()
                    check.set_text(flag)
                    check.set_active(flag in column.flags)
                    self.column_details.add(check, False, True)
                    self.flag_checkboxes.append(check)
                    check.add_clicked_callback(lambda check=check, flag=flag:self.flag_checked(check, flag))
        
                if column.simpleType.group.name != "string" and not column.simpleType.name.lower().endswith("text"):
                    self.charset.set_selected(0)
                    self.charset.set_enabled(False)
                else:
                    self.charset.set_enabled(True)

            if not column.collationName:
                self.charset.set_selected(0)
            else:
                self.charset.set_value(column.collationName)
        else:
            self.charset.set_selected(0)
            self.column_details.set_enabled(False)
            for c in self.flag_checkboxes:
                self.column_details.remove(c)
            self.flag_checkboxes = []


    def flag_checked(self, check, flag):
        column = self.selected_column()
        if column:
            print check.get_active(), flag, column.flags
            if check.get_active():
                if flag not in column.flags:
                    column.flags.append(flag)
            else:
                if flag in column.flags:
                    column.flags.remove(flag)


    def collation_changed(self):
        column = self.selected_column()
        if column:
            if self.charset.get_selected_index() < 1:
                column.characterSetName = ""
                column.collationName = ""
            else:
                collation = self.charset.get_string_value()
                column.collationName = collation
                column.characterSetName = collation.partition("_")[0]


    def column_edited(self, node, tree_column, new_value):
        table = self.selected_table()
        if not table or node.get_string(0) == new_value:
            return
        if node.get_tag() == "placeholder":
            node.set_tag("")
            node.set_icon_path(0, mforms.App.get().get_resource_path("db.Column.16x16.png"))

            child = self.column_list.add_node()
            child.set_string(0, "Click to add")
            child.set_tag("placeholder")
            self.column_list.select_node(node)

            column = grt.classes.db_mysql_Column()
            column.owner = table
            table.columns.append(column)
            self.show_column_node(node, column)
        else:
            row = self.column_list.row_for_node(node)
            if row < len(table.columns):
                column = table.columns[row]
            else:
                return

        if tree_column == 0:
            column.name = new_value
        elif tree_column == 1:
            column.setParseType(new_value, grt.root.wb.rdbmsMgmt.rdbms[0].simpleDatatypes)
        elif tree_column == 2:
            if new_value == 'NULL':
                column.defaultValueIsNull = True
                column.defaultValue = None
            else:
                column.defaultValueIsNull = False
                column.defaultValue = new_value
        elif tree_column == 3:
            if new_value == '1':
                table.addPrimaryKeyColumn(column)
            else:
                table.removePrimaryKeyColumn(column)
        elif tree_column == 4:
            column.isNotNull = new_value == '1'
        elif tree_column == 5:
            if new_value == '1' and "UNIQUE" not in column.flags:
                column.flags.append("UNIQUE")
            elif new_value != '1' and "UNIQUE" in column.flags:
                column.flags.remove("UNIQUE")
        elif tree_column == 6:
            column.autoIncrement = new_value == '1'

        node.set_string(tree_column, new_value)

    def run(self):
        self.run_modal(None, self.ok)



class TableTemplateManager:
    @property
    def templates(self):
        tlist = grt.root.wb.options.options.get("TableTemplates", None)
        if not tlist:
            tlist = grt.List()
            grt.root.wb.options.options["TableTemplates"] = tlist
        return tlist


    def export_templates(self):
        dlg = mforms.FileChooser(mforms.SaveFile)
        dlg.set_title("Export Table Templates")
        if dlg.run_modal():
            print dlg.get_path()
            grt.serialize(self.templates, dlg.get_path())


    def copy_table(self, orig):
        table = grt.classes.db_mysql_Table()
        table.name = orig.name+"copy"
        for col in orig.columns:
            colcopy = col.shallow_copy()
            colcopy.owner = table
            table.columns.append(colcopy)
            if orig.isPrimaryKeyColumn(col):
                table.addPrimaryKeyColumn(colcopy)
        return table


    def edit_templates(self):
        ed = TemplateEditor(self)
        ed.run()

    def edit_template(self, name):
        ed = TemplateEditor(self)
        ed.select_template(name)
        ed.run()

    def create_table_like_template_in_schema(self, schema, template_name):
        template = None
        for t in self.templates:
            if t.name == template_name:
                template = t
                break
        if template:
            copy = self.copy_table(template)
            new_name = template_name
            i = 1
            while any(t.name == new_name for t in schema.tables):
                new_name = "%s_%i" % (template_name, i)
                i += 1
            copy.name = new_name
            copy.owner = schema
            schema.tables.append(copy)
            return copy
        return None


    def create_table_like_template(self, editor, schema_name, template):
        copy = self.copy_table(template)
        copy.name = template.name

        ocatalog = grt.classes.db_mysql_Catalog()
        ocatalog.name = 'default'
        ocatalog.oldName = ocatalog.name
        ocatalog.simpleDatatypes.extend(grt.root.wb.rdbmsMgmt.rdbms[0].simpleDatatypes)

        oschema = grt.classes.db_mysql_Schema()
        oschema.name = schema_name
        oschema.oldName = oschema.name
        oschema.owner = ocatalog
        ocatalog.schemata.append(oschema)

        catalog = grt.classes.db_mysql_Catalog()
        catalog.name = 'default'
        catalog.oldName = catalog.name
        catalog.simpleDatatypes.extend(grt.root.wb.rdbmsMgmt.rdbms[0].simpleDatatypes)

        schema = grt.classes.db_mysql_Schema()
        schema.name = schema_name
        schema.oldName = schema.name
        schema.owner = catalog
        catalog.schemata.append(schema)
        
        copy.owner = schema
        schema.tables.append(copy)
        
        if editor:
            editor.editLiveObject(copy, ocatalog)
    
    
    def create_table_like(self, editor, schema_name, table_name):
        pass

