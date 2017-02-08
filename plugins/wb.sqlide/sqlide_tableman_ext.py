# Copyright (c) 2013, 2016, Oracle and/or its affiliates. All rights reserved.
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
import sys
from workbench.log import log_error

from sqlide_catalogman_ext import MaintenanceResultForm
from sqlide_catalogman_ext import ObjectManager
from sqlide_catalogman_ext import TriggerManager
from sqlide_catalogman_ext import ColumnManager
from mforms import IconStringColumnType, StringColumnType, IntegerColumnType
from wb_admin_utils import make_panel_header
from workbench.utils import human_size, Version

def make_title(t):
    l = mforms.newLabel(t)
    l.set_style(mforms.BoldStyle)
    return l

def show_table_inspector(editor, selection, page = None):
    for schema, table in selection:
        tinspect = TableInspector(editor)
        tinspect.show_table(schema, table)
        dpoint = mforms.fromgrt(editor.dockingPoint)
        dpoint.dock_view(tinspect, "", 0)
        dpoint.select_view(tinspect)
        tinspect.set_title("%s.%s" % (schema, table))
        if page is not None:
            tinspect.switch_to_page(page)

def handleLiveTreeContextMenu(name, sender, args):
    menu = mforms.fromgrt(args['menu'])
    selection = args['selection']
    from_schema_inspector = args.get('schema_inspector', False)

    # Add extra menu items to the SQL editor live schema tree context menu
    tables_selected = []
    show_index_page = False
    for s in selection:
        if s.type == 'db.Table':
            tables_selected.append((s.schemaName, s.name))
        elif s.type == 'db.Index':
            show_index_page = True
            tables_selected.append((s.schemaName, s.owner.name))
        else:
            return
    if selection:
        item = mforms.newMenuItem("Create Index" if show_index_page else "Table Inspector")
        item.add_clicked_callback(lambda sender=sender, tables_selected=tables_selected: show_table_inspector(sender, tables_selected , 'indexes' if show_index_page else None))
        if from_schema_inspector:
            menu.insert_item(0, item)
            menu.insert_item(1, mforms.newMenuItem("", mforms.SeparatorMenuItem))
        else:
            menu.insert_item(0 if show_index_page else 1, item)
            #menu.insert_item(1 if show_index_page else 2, mforms.newMenuItem("", mforms.SeparatorMenuItem))

class TableInfoPanel(mforms.Box):
    caption = "Info"
    node_name = "informations"
    def __init__(self, editor):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        
        self.editor = editor
        
        self.table = mforms.newTable()
        offset = 2 if self.i_s_innodb_available() else 0
        self.table.set_row_count(19 + offset)
        self.table.set_column_count(2)
        self.table.set_row_spacing(8)
        self.table.set_column_spacing(8)

        self.panel_header_box = mforms.newBox(True)
        self.panel_header_box.set_padding(10)

        self._table_engine = None
        
        self.add(self.panel_header_box,False, True)


        self.table.add(make_title("Table Details"), 0, 2, 0, 1, mforms.HFillFlag)

        self.table.add(mforms.newLabel("Engine:"),              0, 1, 3, 4, mforms.HFillFlag)
        self.engine = mforms.newLabel("")
        self.engine.set_style(mforms.BoldStyle)
        self.table.add(self.engine,                             1, 2, 3, 4, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Row format:"),          0, 1, 4, 5, mforms.HFillFlag)
        self.row_format = mforms.newLabel("")
        self.row_format.set_style(mforms.BoldStyle)
        self.table.add(self.row_format,                         1, 2, 4, 5, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Column count:"),        0, 1, 5, 6, mforms.HFillFlag)
        self.column_count = mforms.newLabel("")
        self.column_count.set_style(mforms.BoldStyle)
        self.table.add(self.column_count,                       1, 2, 5, 6, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Table rows:"),          0, 1, 6, 7, mforms.HFillFlag)
        self.table_rows = mforms.newLabel("")
        self.table_rows.set_style(mforms.BoldStyle)
        self.table.add(self.table_rows,                         1, 2, 6, 7, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("AVG row length:"),      0, 1, 7, 8, mforms.HFillFlag)
        self.avg_row_length = mforms.newLabel("")
        self.avg_row_length.set_style(mforms.BoldStyle)
        self.table.add(self.avg_row_length,                     1, 2, 7, 8, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Data length:"),         0, 1, 8, 9, mforms.HFillFlag)
        self.data_length = mforms.newLabel("")
        self.data_length.set_style(mforms.BoldStyle)
        self.table.add(self.data_length,                        1, 2, 8, 9, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Index length:"),        0, 1, 9, 10, mforms.HFillFlag)
        self.index_length = mforms.newLabel("")
        self.index_length.set_style(mforms.BoldStyle)
        self.table.add(self.index_length,                       1, 2, 9, 10, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Max data length:"),     0, 1, 10, 11, mforms.HFillFlag)
        self.max_data_length = mforms.newLabel("")
        self.max_data_length.set_style(mforms.BoldStyle)
        self.table.add(self.max_data_length,                    1, 2, 10, 11, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Data free:"),           0, 1, 11, 12, mforms.HFillFlag)
        self.data_free = mforms.newLabel("")
        self.data_free.set_style(mforms.BoldStyle)
        self.table.add(self.data_free,                          1, 2, 11, 12, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Table size (estimate):"),          0, 1, 12, 13, mforms.HFillFlag)
        self.table_size = mforms.newLabel("")
        self.table_size.set_style(mforms.BoldStyle)
        self.table.add(self.table_size,                         1, 2, 12, 13, mforms.HFillFlag|mforms.HExpandFlag)
        
        if self.i_s_innodb_available():
            self.table.add(mforms.newLabel("File format:"),        0, 1, 13, 14, mforms.HFillFlag)
            self.file_format = mforms.newLabel("")
            self.file_format.set_style(mforms.BoldStyle)
            self.table.add(self.file_format,                       1, 2, 13, 14, mforms.HFillFlag|mforms.HExpandFlag)
            
            self.table.add(mforms.newLabel("Data path:"),          0, 1, 14, 15, mforms.HFillFlag)
            self.data_path = mforms.newLabel("")
            self.data_path.set_style(mforms.BoldStyle)
            self.table.add(self.data_path,                         1, 2, 14, 15, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Update time:"),         0, 1, 13 + offset, 14 + offset, mforms.HFillFlag)
        self.update_time = mforms.newLabel("")
        self.update_time.set_style(mforms.BoldStyle)
        self.table.add(self.update_time,                        1, 2, 13 + offset, 14 + offset, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Create time:"),         0, 1, 14 + offset, 15 + offset, mforms.HFillFlag)
        self.create_time = mforms.newLabel("")
        self.create_time.set_style(mforms.BoldStyle)
        self.table.add(self.create_time,                        1, 2, 14 + offset, 15 + offset, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Auto increment:"),      0, 1, 15 + offset, 16 + offset, mforms.HFillFlag)
        self.auto_increment = mforms.newLabel("")
        self.auto_increment.set_style(mforms.BoldStyle)
        self.table.add(self.auto_increment,                     1, 2, 15 + offset, 16 + offset, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Table collation:"),     0, 1, 16 + offset, 17 + offset, mforms.HFillFlag)
        self.table_collation = mforms.newLabel("")
        self.table_collation.set_style(mforms.BoldStyle)
        self.table.add(self.table_collation,                    1, 2, 16 + offset, 17 + offset, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Create options:"),      0, 1, 17 + offset, 18 + offset, mforms.HFillFlag)
        self.create_options = mforms.newLabel("")
        self.create_options.set_style(mforms.BoldStyle)
        self.table.add(self.create_options,                     1, 2, 17 + offset, 18 + offset, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Comment:"),             0, 1, 18 + offset, 19 + offset, mforms.HFillFlag)
        self.table_comment = mforms.newLabel("")
        self.table_comment.set_style(mforms.BoldStyle)
        self.table_comment.set_wrap_text(True)
        if sys.platform.lower() == "win32": # We need this so Win will recompute the dimensions.
            self.table_comment.set_size(1, -1)

        self.table.add(self.table_comment,                      1, 2, 18 + offset, 19 + offset, mforms.HFillFlag|mforms.HExpandFlag)
        
        scroll = mforms.ScrollPanel()
        scroll.set_visible_scrollers(True, False)
        scroll.set_autohide_scrollers(True)

        tbox = mforms.Box(False)
        tbox.set_spacing(15)
        tbox.set_padding(15)
        tbox.add(self.table, True, True)
        
        scroll.add(tbox)
        self.add(scroll, True, True)
        
        bbox = mforms.Box(True)
        bbox.set_spacing(15)
        bbox.set_padding(15)
        
        table = mforms.newTable()
        table.set_row_count(1)
        table.set_column_count(3)

        self.analyze_btn = mforms.newButton()
        self.analyze_btn.set_text("Analyze Table")
        self.analyze_btn.add_clicked_callback(self.analyze_table)
        self.analyze_btn.enable_internal_padding(False)
        
        table.add(mforms.newLabel("Information on this page may be outdated. Click "), 0, 1, 0, 1, mforms.HFillFlag)
        table.add(self.analyze_btn,                                                     1, 2, 0, 1, mforms.HFillFlag)
        table.add(mforms.newLabel(" to update it."),                              2, 3, 0, 1, mforms.HFillFlag)
        
        bbox.add(table, False, True)
        
        self.add(bbox, False, True)
        
    def get_table_engine(self):
        return self._table_engine

    def analyze_table(self):
        result = self.editor.executeManagementQuery("ANALYZE TABLE `%s`.`%s`" % (self._schema, self._table), 1)
        form = MaintenanceResultForm(result, False)
        form.set_title("ANALYZE TABLE")
        form.run()
        self.refresh()
        
    def i_s_innodb_available(self):
        return self.editor.serverVersion.majorNumber == 5 and self.editor.serverVersion.minorNumber == 6

    def refresh(self):
        try:
            rset = self.editor.executeManagementQuery("select * from information_schema.tables WHERE table_schema = '%s' AND table_name = '%s'" % (self._schema, self._table), 0)
        except grt.DBError, e:
            log_error("select * from information_schema.tables WHERE table_schema = '%s' AND table_name = '%s': %s\n" % (self._schema, self._table, e))
            rset = None
            
        if rset:
            ok = rset.goToFirstRow()
            columns = []
            column_values = {}
            for col in rset.columns:
                if hasattr(self, col.name.lower()):
                    columns.append(col.name)
            while ok:
                for col in columns:
                    if col.replace(" ", "_").lower() == "engine":
                        self._table_engine = rset.stringFieldValueByName(col).lower()
                    getattr(self, col.replace(" ", "_").lower()).set_text(rset.stringFieldValueByName(col))
                    column_values[col.lower()] = rset.stringFieldValueByName(col)
                    
#                     setattr(columnsVal, col.lower(), rset.stringFieldValueByName(col))

                ok = rset.nextRow()

            try:
                self.table_size.set_text(human_size(int(column_values['data_length']) + int(column_values['index_length'])))
                self.data_length.set_text(human_size(int(column_values['data_length'])))
                self.index_length.set_text(human_size(int(column_values['index_length'])))
                self.data_free.set_text(human_size(int(column_values['data_free'])))
                self.max_data_length.set_text(human_size(int(column_values['max_data_length'])))
            except Exception, e:
                log_error("Error displaying table info for %s.%s: %s\n" % (self._schema, self._table, e))

        try:
            rset = self.editor.executeManagementQuery("select count(*) column_count from information_schema.columns WHERE table_schema = '%s' and table_name = '%s'" % (self._schema, self._table), 0)
        except grt.DBError, e:
            log_error("select count(*) column_count from information_schema.columns WHERE table_schema = '%s' and table_name = '%s': %s\n" % (self._schema, self._table, e))
            rset = None

        if rset:
            ok = rset.goToFirstRow()
            if ok:
                self.column_count.set_text(rset.stringFieldValueByName("column_count"));
            
        if self.i_s_innodb_available():
            try:
                rset = self.editor.executeManagementQuery("SELECT @@datadir datadir,st.FILE_FORMAT,sd.path FROM information_schema.INNODB_SYS_TABLES st JOIN information_schema.innodb_sys_datafiles sd USING(space) WHERE st.name = '%s/%s'" % (self._schema, self._table), 0)
            except grt.DBError, e:
                log_error("SELECT @@datadir datadir,st.FILE_FORMAT,sd.path FROM information_schema.INNODB_SYS_TABLES st JOIN information_schema.innodb_sys_datafiles sd USING(space) WHERE st.name = '%s/%s'': %s\n" % (self._schema, self._table, e))
                rset = None

            if rset:
                ok = rset.goToFirstRow()
                if ok:
                    self.file_format.set_text(rset.stringFieldValueByName("FILE_FORMAT"))
                    data_path = rset.stringFieldValueByName("path")
                    if data_path[:1] == ".":
                        self.data_path.set_text("%s%s" %(rset.stringFieldValueByName("datadir"), data_path[2:]))
                    else:
                        self.data_path.set_text(data_path)
        
    def show_table(self, schema, table):
        self._schema = schema
        self._table = table

        self.panel_header_box.add(make_panel_header("db.Table.32x32.png", self.editor.connection.name, "%s.%s" % (schema, table)), False, True)

        self.refresh()

class TableDDL(mforms.Box):
    caption = "DDL"
    node_name = "structure"
    def __init__(self, editor):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()

        self.editor = editor
        self.set_spacing(15)
        self.set_padding(15)
        self.title_box = mforms.newBox(False)
        self.title_box.set_spacing(1)

        self.add(self.title_box, False, True)

        self.code_editor = mforms.CodeEditor()
        self.code_editor.set_language(mforms.LanguageMySQL)
        self.code_editor.set_managed()
        self.code_editor.set_read_only(True)
        self.code_editor.set_release_on_add()
        self.add(self.code_editor, True, True)

    def refresh(self):
        try:
            rset = self.editor.executeManagementQuery("show create table `%s`.`%s`" % (self._schema, self._table), 0)
        except grt.DBError, e:
            log_error("show create table `%s`.`%s` : %s\n" % (self._schema, self._table, e))
            rset = None

        if rset:
            ok = rset.goToFirstRow()
            if ok:
                self.code_editor.set_read_only(False)
                self.code_editor.set_value(rset.stringFieldValue(1));
                self.code_editor.set_read_only(True)

    def show_table(self, schema, table):
        self._schema = schema
        self._table = table
        self.title_box.add(make_title("DDL for %s.%s" % (self._schema, self._table)), False, True)
        self.refresh()

class CreateIndexForm(mforms.Form):
    def __init__(self, owner, editor, schema, table, columns, engine):
        mforms.Form.__init__(self, mforms.Form.main_form(), mforms.FormNormal)

        self._owner = owner
        self._editor = editor
        self._schema = schema
        self._table = table
        self._columns = columns
        self._engine = engine

        self.target_version = Version.fromgrt(editor.serverVersion)
        
        self.set_title("Create Index for Table %s.%s" % (schema, table))

        content = mforms.newBox(False)
        self.set_content(content)
        content.set_padding(20)
        content.set_spacing(12)

        table = mforms.newTable()
        table.set_row_count(7)
        table.set_column_count(2)
        table.set_row_spacing(8)
        table.set_column_spacing(8)
        content.add(table, False, True)

        table.add(mforms.newLabel("Index Name:", True), 0, 1, 0, 1, mforms.HFillFlag)
        hbox = mforms.newBox(True)
        hbox.set_spacing(12)
        self.name = mforms.newTextEntry()
        hbox.add(self.name, True, True)
        self.kind = mforms.newSelector()
        self.kind.add_items(["Non-Unique", "Unique", "FullText", "Spatial"])
        hbox.add(self.kind, False, True)
        table.add(hbox, 1, 2, 0, 1, mforms.VFillFlag|mforms.HFillFlag|mforms.HExpandFlag)

        if self._engine in ["memory", "heap","ndb"]:
            table.add(mforms.newLabel("Type:", True), 0, 1, 1, 2, mforms.HFillFlag)
            self.type = mforms.newSelector()
            self.type.add_items(["BTREE", "HASH"])
            table.add(self.type, 1, 2, 1, 2, mforms.HFillFlag)

        l = mforms.newLabel("Columns:")
        l.set_text_align(mforms.TopRight)
        table.add(l, 0, 1, 2, 3, mforms.HFillFlag|mforms.VFillFlag)
        self.columns = mforms.newTreeView(mforms.TreeFlatList|mforms.TreeAltRowColors|mforms.TreeShowColumnLines)
        self.columns.add_column(mforms.StringColumnType, "Column", 200, False)
        self.columns.add_column(mforms.StringColumnType, "Length", 60, True)
        #        self.columns.add_column(mforms.CheckColumnType, "Order", 50, False) # ignored by server
        self.columns.end_columns()
        self.columns.set_size(-1, 80)
        tbl = mforms.newTable()
        tbl.set_row_count(3)
        tbl.set_column_count(2)
        tbl.set_row_spacing(2)
        tbl.set_column_spacing(4)
        tbl.add(self.columns, 0, 1, 0, 3, mforms.HFillFlag|mforms.VFillFlag|mforms.HExpandFlag|mforms.VExpandFlag)
        self.move_up = mforms.newButton()
        self.move_up.set_text("\xe2\x96\xb2")
        self.move_up.add_clicked_callback(self.move_row_up)
        self.move_up.enable_internal_padding(False)
        self.move_down = mforms.newButton()
        self.move_down.set_text("\xe2\x96\xbc")
        self.move_down.add_clicked_callback(self.move_row_down)
        self.move_down.enable_internal_padding(False)
        tbl.add(self.move_up, 1, 2, 0, 1, mforms.VFillFlag|mforms.HFillFlag)
        tbl.add(self.move_down, 1, 2, 1, 2, mforms.VFillFlag|mforms.HFillFlag)
        tbl.add(mforms.newLabel(""), 1, 2, 2, 3, mforms.VFillFlag|mforms.HFillFlag|mforms.VExpandFlag)
        table.add(tbl, 1, 2, 2, 3, mforms.VFillFlag|mforms.HFillFlag)

        l = mforms.newLabel("Comments:")
        l.set_text_align(mforms.TopRight)
        table.add(l, 0, 1, 3, 4, mforms.HFillFlag|mforms.VFillFlag)
        self.comments = mforms.newTextBox(0)
        self.comments.set_size(-1, 60)
        if self._editor.serverVersion.majorNumber > 5 or (self._editor.serverVersion.majorNumber == 5 and self._editor.serverVersion.minorNumber >= 5):
            pass
        else:
            self.comments.set_enabled(False)
        table.add(self.comments, 1, 2, 3, 4, mforms.HFillFlag|mforms.VFillFlag)

        online_ddl_ok = self._editor.serverVersion.majorNumber > 5 or (self._editor.serverVersion.majorNumber == 5 and self._editor.serverVersion.minorNumber >= 6)

        if online_ddl_ok:
            l = mforms.newLabel("\nCreate/Online Options")
        else:
            l = mforms.newLabel("\nCreate/Online Options (requires MySQL 5.6+)")
            l.set_enabled(False)
        l.set_style(mforms.BoldStyle)
        table.add(l, 1, 2, 4, 5, mforms.HFillFlag)
        table.add(mforms.newLabel("Algorithm:", True), 0, 1, 5, 6, mforms.HFillFlag)
        self.algorithm = mforms.newSelector()
        self.algorithm.add_items(["Default", "Copy", "InPlace"])
        self.algorithm.set_enabled(online_ddl_ok)
        table.add(self.algorithm, 1, 2, 5, 6, mforms.HFillFlag)

        table.add(mforms.newLabel("Locking:", True), 0, 1, 6, 7, mforms.HFillFlag)
        self.lock = mforms.newSelector()
        self.lock.add_items(["Default (allow as much concurrency as possible)", "Exclusive (totally block access to table)", "Shared (allow queries but not DML)", "None (allow queries and DML)"])
        self.lock.set_enabled(online_ddl_ok)
        table.add(self.lock, 1, 2, 6, 7, mforms.HFillFlag)

        bbox = mforms.newBox(True)
        bbox.set_spacing(12)
        self.ok = mforms.newButton()
        self.ok.set_text("Create")

        self.cancel = mforms.newButton()
        self.cancel.set_text("Cancel")

        mforms.Utilities.add_end_ok_cancel_buttons(bbox, self.ok, self.cancel)
        content.add_end(bbox, False, True)

        self.set_size(550, 400)
        self.center()

    def move_row_up(self):
        node = self.columns.get_selected_node()
        if node:
            row = self.columns.row_for_node(node)
            if row > 0:
                name, length = node.get_string(0), node.get_string(1)
                new_node = self.columns.root_node().insert_child(row-1)
                new_node.set_string(0, name)
                new_node.set_string(1, length)
                node.remove_from_parent()
                self.columns.select_node(new_node)

    def move_row_down(self):
        node = self.columns.get_selected_node()
        if node:
            row = self.columns.row_for_node(node)
            if row < self.columns.count()-1:
                name, length = node.get_string(0), node.get_string(1)
                node.remove_from_parent()
                new_node = self.columns.root_node().insert_child(row+1)
                new_node.set_string(0, name)
                new_node.set_string(1, length)
                self.columns.select_node(new_node)

    def run(self):
        name = "idx_%s_%s" % (self._table, "_".join(self._columns))
        self.name.set_value(name)

        for c in self._columns:
            node = self.columns.add_node()
            node.set_string(0, c)

        if self.run_modal(self.ok, self.cancel):
            columns = []
            for i in range(self.columns.count()):
                node = self.columns.node_at_row(i)
                c = node.get_string(0)
                l = node.get_string(1)
                if l:
                    columns.append("%s(%s)" % (c, l))
                else:
                    columns.append(c)
            kind = self.kind.get_string_value().upper()
            if self.kind.get_selected_index() == 0:
                kind = ""
            else:
                kind = " "+kind
            sql = "CREATE%s INDEX `%s` %s ON `%s`.`%s` (%s)" % (kind, self.name.get_string_value(), "USING %s" % self.type.get_string_value().upper() if hasattr(self, "type") else "", self._schema, self._table, ", ".join(columns))
            if self._editor.serverVersion.majorNumber > 5 or (self._editor.serverVersion.majorNumber == 5 and self._editor.serverVersion.minorNumber >= 5):
                sql += " COMMENT '%s'" % self.comments.get_string_value().replace("'", "''") # 5.5
            if self._editor.serverVersion.majorNumber > 5 or (self._editor.serverVersion.majorNumber == 5 and self._editor.serverVersion.minorNumber >= 6):
                sql += " ALGORITHM %s" % self.algorithm.get_string_value().upper() # 5.6
                sql += " LOCK %s" % self.lock.get_string_value().upper().split()[0]
            try:
                self._editor.executeManagementCommand(sql, 1)
                return True
            except grt.DBError, e:
                mforms.Utilities.show_error("Create Index",
                                            "Error creating index.\n%s" % e.args[0],
                                            "OK", "", "")
        return True


class TableIndexInfoPanel(mforms.Box):
    caption = "Indexes"
    node_name = "indexes"
    def __init__(self, editor):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()

        self.set_padding(12)
        self.set_spacing(12)

        self.editor = editor
        self.target_version = Version.fromgrt(self.editor.serverVersion)
        
        self.get_table_engine = None

        # Upper Row
        table = mforms.newTable()
        table.set_row_count(2)
        table.set_column_count(3)
        table.set_row_spacing(4)
        table.set_column_spacing(8)
        self.add(table, False, True)

        table.add(make_title("Indexes in Table"), 0, 1, 0, 1, mforms.HFillFlag)
        self.index_list = mforms.newTreeView(mforms.TreeFlatList|mforms.TreeAltRowColors|mforms.TreeShowColumnLines)
        self.index_list.add_column(mforms.IconStringColumnType, "Key", 140, False)
        self.index_list.add_column(mforms.StringColumnType, "Type", 80, False)
        self.index_list.add_column(mforms.StringColumnType, "Unique", 40, False)
        self.index_list.add_column(mforms.StringColumnType, "Columns", 200, False)
        self.index_list.end_columns()
        self.index_list.add_changed_callback(self.index_selected)
        self.index_list.set_size(450, -1)
        table.add(self.index_list, 0, 1, 1, 2, mforms.HFillFlag|mforms.VFillFlag)

        dhbox = mforms.newBox(True)
        dhbox.add(make_title("Index Details"), False, True)
        self.drop_index = mforms.newButton()
        self.drop_index.set_text("Drop Index")
        self.drop_index.set_enabled(False)
        self.drop_index.add_clicked_callback(self.do_drop_index)
        dhbox.add_end(self.drop_index, False, True)
        table.add(dhbox, 1, 3, 0, 1, mforms.HFillFlag|mforms.HExpandFlag|mforms.VFillFlag)
        
        self.info = mforms.newTable()
        table.add(self.info, 1, 3, 1, 2, mforms.HFillFlag|mforms.HExpandFlag|mforms.VFillFlag)

        self.info.set_padding(8)
        self.info.set_row_count(7)
        self.info.set_column_count(4)
        self.info.set_row_spacing(4)
        self.info.set_column_spacing(4)

        self.info.add(mforms.newLabel("Key Name:"),        0, 1, 0, 1, mforms.HFillFlag)
        self.key_name = mforms.newLabel("")
        self.key_name.set_style(mforms.BoldStyle)
        self.info.add(self.key_name,                       1, 4, 0, 1, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.info.add(mforms.newLabel("Index Type:"),      0, 1, 1, 2, mforms.HFillFlag)
        self.index_type = mforms.newLabel("")
        self.index_type.set_style(mforms.BoldStyle)
        self.info.add(self.index_type,                     1, 2, 1, 2, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.info.add(mforms.newLabel("Packed:"),          2, 3, 1, 2, mforms.HFillFlag)
        self.packed = mforms.newLabel("")
        self.packed.set_style(mforms.BoldStyle)
        self.info.add(self.packed,                         3, 4, 1, 2, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.info.add(mforms.newLabel("Allows NULL:"),     0, 1, 2, 3, mforms.HFillFlag)
        self.allows_null = mforms.newLabel("")
        self.allows_null.set_style(mforms.BoldStyle)
        self.info.add(self.allows_null,                    1, 2, 2, 3, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.info.add(mforms.newLabel("Unique:"),          2, 3, 2, 3, mforms.HFillFlag)
        self.unique_values = mforms.newLabel("")
        self.unique_values.set_style(mforms.BoldStyle)
        self.info.add(self.unique_values,                  3, 4, 2, 3, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.info.add(mforms.newLabel("Cardinality:"),     0, 1, 3, 4, mforms.HFillFlag)
        self.cardinality = mforms.newLabel("")
        self.cardinality.set_style(mforms.BoldStyle)
        self.info.add(self.cardinality,                    1, 4, 3, 4, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.info.add(mforms.newLabel("Comment:"),         0, 1, 4, 5, mforms.HFillFlag)
        self.comment = mforms.newLabel("")
        self.comment.set_style(mforms.BoldStyle)
        self.info.add(self.comment,                        1, 4, 4, 5, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.info.add(mforms.newLabel("User Comment:"),    0, 1, 5, 6, mforms.HFillFlag)
        self.user_comment = mforms.newLabel("")
        self.user_comment.set_style(mforms.BoldStyle)
        self.info.add(self.user_comment,                   1, 4, 5, 6, mforms.HFillFlag|mforms.HExpandFlag)

        # Lower Row
        table = mforms.newTable()
        table.set_row_count(2)
        table.set_column_count(2)
        table.set_row_spacing(4)
        table.set_column_spacing(8)
        
        self.add(table, True, True)

        table.add(make_title("Columns in table"), 0, 2, 0, 1, mforms.HFillFlag|mforms.HExpandFlag)

        self.column_list = mforms.newTreeView(mforms.TreeFlatList|mforms.TreeAltRowColors|mforms.TreeShowColumnLines)
        self.column_list.add_column(mforms.IconStringColumnType, "Column", 150, False)
        self.column_list.add_column(mforms.StringColumnType, "Type", 150, False)
        self.column_list.add_column(mforms.StringColumnType, "Nullable", 50, False)
        self.column_list.add_column(mforms.StringColumnType, "Indexes", 300, False)
        self.column_list.end_columns()
        self.column_list.set_selection_mode(mforms.TreeSelectMultiple)
        table.add(self.column_list, 0, 2, 1, 2, mforms.HFillFlag|mforms.HExpandFlag|mforms.VFillFlag|mforms.VExpandFlag)

        hbox = mforms.newBox(True)
        self.create_index = mforms.newButton()
        self.create_index.set_text("Create Index for Selected Columns...")
        self.create_index.add_clicked_callback(self.do_create_index)
        hbox.add_end(self.create_index, False, True)
        self.add(hbox, False, True)


    def do_drop_index(self):
        node = self.index_list.get_selected_node()
        if node:
            index = node.get_string(0)
            if mforms.Utilities.show_message("Drop Index", "Drop index `%s` from table `%s`.`%s`?" % (index, self._schema, self._table), "Drop", "Cancel", "") == mforms.ResultOk:
                try:
                    self.editor.executeManagementCommand("DROP INDEX `%s` ON `%s`.`%s`" % (index, self._schema, self._table), 1)
                    self.refresh()
                except grt.DBError, e:
                    mforms.Utilities.show_error("Drop Index", "Error dropping index.\n%s" % e.args[0], "OK", "", "")


    def do_create_index(self):
        cols = []
        for node in self.column_list.get_selection():
            cols.append(node.get_string(0))
        if cols:
            form = CreateIndexForm(self, self.editor, self._schema, self._table, cols, self.get_table_engine() if self.get_table_engine else None)
            if form.run():
                self.refresh()
        else:
            mforms.Utilities.show_warning("Create Index","You have to select at least one column.\n", "OK", "", "")

    def index_selected(self):
        node = self.index_list.get_selected_node()
        if node:
            idx = self.index_info[self.index_list.row_for_node(node)]

            info, columns = idx
            self.key_name.set_text(info[2])
            self.unique_values.set_text("NO" if info[1] == "1" else "YES")
            if info[2] == "PRIMARY" and self.get_table_engine and self.get_table_engine() == "innodb":
                self.index_type.set_text("%s (clustered)" % info[6])
            else:
                self.index_type.set_text(info[6])
            self.cardinality.set_text(info[3])
            self.packed.set_text(info[4])
            self.allows_null.set_text(info[5])
            self.comment.set_text(info[7])
            if self.target_version.is_supported_mysql_version_at_least(5, 6):
                self.user_comment.set_text(info[8])

            self.drop_index.set_enabled(True)
        else:
            self.drop_index.set_enabled(False)
            self.key_name.set_text("")
            self.unique_values.set_text("")
            self.index_type.set_text("")
            self.cardinality.set_text("")
            self.packed.set_text("")
            self.allows_null.set_text("")
            self.comment.set_text("")
            self.user_comment.set_text("")

    def refresh(self):
        self.show_table(self._schema, self._table)


    def show_table(self, schema, table):
        self._schema = schema
        self._table = table
        self.index_list.clear()
        self.index_info = []
        self.column_list.clear()
        column_icon = mforms.App.get().get_resource_path("db.Column.16x16.png")
        index_icon = mforms.App.get().get_resource_path("db.Index.16x16.png")
        if table:
            try:
                rset = self.editor.executeManagementQuery("SHOW INDEX FROM `%s`.`%s`" % (schema, table), 0)
            except grt.DBError, e:
                log_error("Cannot execute SHOW INDEX FROM `%s`.`%s`: %s\n" % (schema, table, e))
                rset = None

            if self.target_version.is_supported_mysql_version_at_least(Version.fromstr("5.6")):
                index_rs_columns = range(13)
            else:
                index_rs_columns = range(12)
            column_rs_columns = [3, 4, 5, 7]
            for i in column_rs_columns:
                index_rs_columns.remove(i)
            column_to_index = {}
            if rset:
                ok = rset.goToFirstRow()
                curname = None
                itype = None
                non_unique = None
                columns = []
                while ok:
                    name = rset.stringFieldValue(2)
                    if name != curname:
                        if columns:
                            node = self.index_list.add_node()
                            node.set_icon_path(0, index_icon)
                            node.set_string(0, curname)
                            node.set_string(1, itype)
                            node.set_string(2, "YES" if non_unique != "1" else "NO")
                            node.set_string(3, ", ".join([c[1] for c in columns]))
                        columns = []
                        self.index_info.append(([rset.stringFieldValue(i) for i in index_rs_columns], columns))
                        curname = name
                        itype = rset.stringFieldValue(10)
                        non_unique = rset.stringFieldValue(1)

                    cname = rset.stringFieldValue(4)
                    if cname not in column_to_index:
                        column_to_index[cname] = [name]
                    else:
                        column_to_index[cname].append(name)
                    columns.append([rset.stringFieldValue(i) for i in column_rs_columns])
                    ok = rset.nextRow()
                if columns:
                    node = self.index_list.add_node()
                    node.set_icon_path(0, index_icon)
                    node.set_string(0, curname)
                    node.set_string(1, itype)
                    node.set_string(2, "YES" if non_unique != "1" else "NO")
                    node.set_string(3, ", ".join([c[1] for c in columns]))
            try:
                rset = self.editor.executeManagementQuery("SHOW COLUMNS FROM `%s`.`%s`" % (schema, table), 0)
            except grt.DBError, e:
                log_error("Cannot execute SHOW COLUMNS FROM `%s`.`%s`: %s\n" % (schema, table, e))
                rset = None

            if rset:
                ok = rset.goToFirstRow()
                while ok:
                    node = self.column_list.add_node()
                    node.set_icon_path(0, column_icon)
                    node.set_string(0, rset.stringFieldValue(0))
                    node.set_string(1, rset.stringFieldValue(1))
                    node.set_string(2, rset.stringFieldValue(2))
                    node.set_string(3, ", ".join(column_to_index.get(rset.stringFieldValue(0), [])))
                    ok = rset.nextRow()
        else:
            self.index_list.clear()
            self.column_list.clear()
            self.index_selected()

class TableManDefs:
    def get_query(self):
        cols = []
        for i, (field_obj, ctype, caption, width, min_version) in enumerate(self.columns):
            if min_version and not self.target_version.is_supported_mysql_version_at_least(Version.fromstr(min_version)):
                continue
            try:
                cols.append("`%s`" % field_obj['field'])
            except:
                cols.append("`%s`" % field_obj)
        return self.show_query % {'schema' : self.schema, 'table' : self.table, 'columns' : ", ".join(cols)}
    
    def show_table(self, schema, table):
        self.schema = schema
        self.table = table
        self.refresh()

class TableTriggerManager(TableManDefs, TriggerManager):
    icon_column = 0
    show_query = "show triggers from `%(schema)s` like '%(table)s'"
    columns = [("Trigger", IconStringColumnType, "Name", 200, None),
           ("Event", StringColumnType, "Event", 100, None),
           ("Timing", StringColumnType, "Timing", 100, None),
           ("Created", StringColumnType, "Created", 100, None),
           ("sql_mode", StringColumnType, "SQL Mode", 100, None),
           ("Definer", StringColumnType, "Definer", 100, None),
           ("character_set_client", StringColumnType, "Client Character Set", 100, None),
           ("collation_connection", StringColumnType, "Connection Collation", 100, None),
           ("Database Collation", StringColumnType, "Database Collation", 100, None),
           ]
    def __init__(self, editor):
        TriggerManager.__init__(self, editor, None)
        self.set_managed()
        self.set_release_on_add()
    
        
class TableColumnManager(TableManDefs, ColumnManager):
    show_query = "select %(columns)s from information_schema.columns where table_schema = '%(schema)s' and table_name = '%(table)s'"
    icon_column = 0
    name_column = 0
    columns = [("COLUMN_NAME", IconStringColumnType, "Column", 150, None),
               ("COLUMN_TYPE", StringColumnType, "Type", 120, None),
               ("COLUMN_DEFAULT", StringColumnType, "Default Value", 120, None),
               ("IS_NULLABLE", StringColumnType, "Nullable", 50, None),
               ("CHARACTER_SET_NAME", StringColumnType, "Character Set", 80, None),
               ("COLLATION_NAME", StringColumnType, "Collation", 100, None),
               ("PRIVILEGES", StringColumnType, "Privileges", 200, None),
               ("EXTRA", StringColumnType, "Extra", 100, None),
               ("COLUMN_COMMENT", StringColumnType, "Comments", 200, None)]
    def __init__(self, editor):
        ColumnManager.__init__(self, editor, None)
        self.set_managed()
        self.set_release_on_add()
    
class ConstraintManager(TableManDefs, ObjectManager):
    icon_column = 0
    caption = "Foreign keys"
    klass = "db.Constraints"
    node_name = "constraints"
    show_query = "select %(columns)s \
                    FROM information_schema.KEY_COLUMN_USAGE \
                    WHERE ((REFERENCED_TABLE_SCHEMA = '%(schema)s' AND referenced_table_name = '%(table)s') OR (TABLE_SCHEMA='%(schema)s' and table_name = '%(table)s')) AND \
                    REFERENCED_TABLE_NAME is not null ORDER BY TABLE_NAME, COLUMN_NAME;"
    name_column = 0
    columns = [("constraint_name", IconStringColumnType, "Name", 200, None),
               ("constraint_schema", StringColumnType, "Schema", 100, None),
               ("table_name", StringColumnType, "Table", 100, None),
               ("column_name", StringColumnType, "Column", 100, None),
               ("referenced_table_schema", StringColumnType, "Referenced Schema", 100, None),
               ("referenced_table_name", StringColumnType, "Referenced Table", 100, None),
               ("referenced_column_name", StringColumnType, "Referenced Column", 100, None)]
    def __init__(self, editor):
        ObjectManager.__init__(self, editor, None)
        self.set_managed()
        self.set_release_on_add()
    
class PartitionManager(TableManDefs, ObjectManager):
    caption = "Partitions"
    klass = "db.Partitions"
    node_name = "partitions"
    show_query = "select %(columns)s \
                    FROM information_schema.partitions \
                    WHERE TABLE_SCHEMA = '%(schema)s' AND TABLE_NAME = '%(table)s' AND PARTITION_NAME IS NOT NULL ORDER BY PARTITION_NAME"
    name_column = 0
    icon_column = 0
    columns = [("partition_name", IconStringColumnType, "Name", 200, None),
               ("subpartition_name", StringColumnType, "Subpartiton Name", 100, None),
               ("partition_ordinal_position", IntegerColumnType, "Ordinal Pos", 100, None),
               ("subpartition_ordinal_position", IntegerColumnType, "Subpartiton Ordinal Pos", 100, None),
               ("partition_method", StringColumnType, "Partition Method", 100, None),
               ("subpartition_method", StringColumnType, "Subpartition Method", 100, None),
               ("partition_expression", StringColumnType, "Partition expression", 100, None),
               ("subpartition_expression", StringColumnType, "Subpartition expression", 100, None),
               ("partition_description", StringColumnType, "Partition description", 100, None),
               ("table_rows", IntegerColumnType, "Table rows", 100, None),
               ("avg_row_length", IntegerColumnType, "AVG row length", 100, None),
               ("data_length", IntegerColumnType, "Data length", 100, None),
               ("max_data_length", IntegerColumnType, "max data length", 100, None),
               ("index_length", IntegerColumnType, "Index length", 100, None),
               ("create_time", StringColumnType, "Create time", 150, None),
               ("update_time", StringColumnType, "Update time", 150, None),
               ("check_time", StringColumnType, "Check time", 150, None)]
    def __init__(self, editor):
        ObjectManager.__init__(self, editor, None)
        self.set_managed()
        self.set_release_on_add()
    
    
class GrantsTableManager(TableManDefs, ObjectManager):
    caption = "Table privileges"
    klass = "db.Grants"
    node_name = "table_privileges"
    name_column = 0
    def __init__(self, editor):
        self.columns = []
        ObjectManager.__init__(self, editor, None)
       
        self.set_managed()
        self.set_release_on_add()
        
        # We need to hide the refresh buttons, cause we will create one general refresh for  grants.
        self.refresh_btn.show(False)
        
    def preload_columns(self):
        if len(self.columns) > 0:
            return;
        
        query = "show columns from `mysql`.`tables_priv` like 'table_priv'"
        try:
            rset = self.editor.executeManagementQuery(query, 0)
        except grt.DBError, e:
            if e.args[1] == 1044 or e.args[1] == 1142:
                
                self.show_error("Access Error", "The current user does not have enough privileges to execute %s.\n\n%s" % (query, e.args[0]))
            else:
                self.show_error("MySQL Error", "An error occurred retrieving information about the schema.\nQuery: %s\nError: %s" % (query, e.args[0]))
            return
        
        self.columns = [('user', StringColumnType, 'User', 100, None),
                        ('host', StringColumnType, 'Host', 100, None),
                        ('scope', StringColumnType, 'Scope', 100, None)]
        
        ok = rset.goToFirstRow()
        if ok:
            for col in rset.stringFieldValueByName("Type")[5:][:-2].split("','"):
                self.columns.append((col, StringColumnType, col, 50, None));
    
    def get_query(self):
        if len(self.columns) == 0: # Probably user doesn't have privileges to list the privilege tables.
            return []

        output = []
        fields = []
        fields_where = []
        for i, (field_obj, ctype, caption, width, min_version) in enumerate(self.columns):
            field = None
            try:
                field = field_obj['field']
            except:
                field = field_obj
            if field == "scope":
                continue
            if field not in ['user','host']:
                fields.append("%s_priv AS '%s'" % (field.replace(" ","_"), field))
                fields_where.append("%s_priv = 'Y'" % field.replace(" ","_"))
            else:
                fields.append(field)
        
        output.append("SELECT '<global>' as scope,%(sel_fields)s FROM mysql.user WHERE %(where_fields)s" % {'sel_fields' : ",".join(fields), 'where_fields' : " OR ".join(fields_where)})

        output.append("SELECT Db as scope,%(sel_fields)s FROM mysql.db WHERE '%(schema)s' like db" % {'sel_fields' : ",".join(fields), 'schema' : self.schema})
        
        #mysql.tables_priv is little different, it holds priv info in set type column
        fields = []
        fields_where = []
        for i, (field_obj, ctype, caption, width, min_version) in enumerate(self.columns):
            field = None
            try:
                field = field_obj['field']
            except:
                field = field_obj
            if field == "scope":
                continue
            if field not in ['user','host']:
                fields.append("IF(FIND_IN_SET('%s',Table_priv) = 0, 'N', 'Y') AS '%s'" % (field.replace(" ","_"), field))
            else:
                fields.append(field)
         
        output.append("SELECT CONCAT(Db,'.',Table_name) as scope,%(sel_fields)s FROM mysql.tables_priv WHERE '%(schema)s' LIKE Db AND '%(table)s' LIKE Table_name" % {'sel_fields' : ",".join(fields), 'schema' : self.schema, 'table' : self.table})
#         
        return output
    
    def refresh(self):
        self.tree.clear()
        for query in self.get_query():
            self.preload_data(query)
        self.refresh_row_count()

class GrantsColumnManager(TableManDefs, ObjectManager):
    caption = "Column privileges"
    klass = "db.Grants"
    node_name = "column_privileges"
    name_column = 0
    def __init__(self, editor):
        self.columns = []
        ObjectManager.__init__(self, editor, None)
        self.set_managed()
        self.set_release_on_add()
        
        # We need to hide the refresh buttons, cause we will create one general refresh for  grants.
        self.refresh_btn.show(False)
        
    def preload_columns(self):
        if len(self.columns) > 0:
            return;

        query = "show columns from `mysql`.`columns_priv` like 'Column_priv'"
        try:
            rset = self.editor.executeManagementQuery(query, 0)
        except grt.DBError, e:
            if e.args[1] == 1044 or e.args[1] == 1142:
                self.show_error("Access Error", "The current user does not have enough privileges to execute %s.\n\n%s" % (query, e.args[0]))
            else:
                self.show_error("MySQL Error", "An error occurred retrieving information about the schema.\nQuery: %s\nError: %s" % (query, e.args[0]))
            return

        self.columns = [('user', StringColumnType, 'User', 100, None),
                        ('host', StringColumnType, 'Host', 100, None),
                        ('scope', StringColumnType, 'Scope', 100, None)]

        ok = rset.goToFirstRow()
        if ok:
            for col in rset.stringFieldValueByName("Type")[5:][:-2].split("','"):
                self.columns.append((col, StringColumnType, col, 50, None));

    def get_query(self):
        if len(self.columns) == 0: # Probably user doesn't have privileges to list the privilege tables.
            return []

        output = []
        fields = []
        for i, (field_obj, ctype, caption, width, min_version) in enumerate(self.columns):
            field = None
            try:
                field = field_obj['field']
            except:
                field = field_obj
            if field == "scope":
                continue
            if field not in ['user','host']:
                fields.append("IF(FIND_IN_SET('%s',Column_priv) = 0, 'N', 'Y') AS '%s'" % (field.replace(" ","_"), field))
            else:
                fields.append(field)
        
        output.append("SELECT CONCAT(Db,'.',Table_name,'.',Column_name) as scope,%(sel_fields)s FROM mysql.columns_priv WHERE Db LIKE '%(schema)s' AND Table_name LIKE '%(table)s'" % {'sel_fields' : ",".join(fields), 'schema' : self.schema, 'table' : self.table})
        return output

    def refresh(self):
        self.tree.clear()
        for query in self.get_query():
            self.preload_data(query)
        self.refresh_row_count()

    
class GrantsManager(TableManDefs, mforms.Box):
    caption = "Grants"
    klass = "db.Grants"
    node_name = "grants"
    name_column = 0
    def __init__(self, editor):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()

        self.editor = editor

        self.set_padding(8)
        self.set_spacing(8)
        
        tbl_box = mforms.newBox(False)
        tbl_box.set_spacing(1)
        
        self.grants_table = GrantsTableManager(self.editor)
        tbl_box.add(make_title(self.grants_table.caption), False, True)
        tbl_box.add(self.grants_table, True, True)
        
        splitter = mforms.newSplitter(False, False)
        splitter.add(tbl_box, 200);

        col_box = mforms.newBox(False)
        col_box.set_spacing(1)
         
        self.grants_column = GrantsColumnManager(self.editor)
        col_box.add(make_title(self.grants_column.caption), False, True)
        col_box.add(self.grants_column, True, True)

        splitter.add(col_box, 220);
        self.add(splitter, True, True)
        
        self.bbox = mforms.newBox(True)
        self.bbox.set_spacing(8)
        self.add(self.bbox, False, True)
        self.refresh_btn = mforms.newButton()
        self.refresh_btn.set_text("Refresh")
        self.refresh_btn.add_clicked_callback(self.refresh)
        self.bbox.add_end(self.refresh_btn, False, True)
        
    def refresh(self):
        self.grants_table.show_table(self.schema, self.table);
        self.grants_column.show_table(self.schema, self.table);
    

class TableInspector(mforms.AppView):
    def __init__(self, editor):
        self.tab_list = {}
        self.pages = []

        mforms.AppView.__init__(self, False, "TableInspector", False)

        self.set_managed()
        self.set_release_on_add()

        self.tab = mforms.newTabView()
        self.add(self.tab, True, True)

        tabs = [TableInfoPanel, TableColumnManager, TableIndexInfoPanel, TableTriggerManager, ConstraintManager, PartitionManager, GrantsManager, TableDDL]

        i =0
        for Tab in tabs:
            try:
                tab = Tab(editor)
                setattr(self, "tab_"+tab.node_name, tab)
                self.tab_list[tab.node_name] = i
                self.tab.add_page(tab, tab.caption)
                self.pages.append(tab)
                i += 1
            except Exception:
                import traceback
                log_error("Error initializing tab %s: %s\n" % (tab.node_name, traceback.format_exc()))


    def show_table(self, schema, table):
        for tab in self.pages:
#             try:
            tab.show_table(schema, table)
#             except:
#                 print "some error"
#                 continue # If user will launch table inspector on Information or Performance Schema, there can be some problems, let's catch those.
        #After each tab is loaded we can make a ptr to the get_table_engine method
        self.pages[self.tab_list["indexes"]].get_table_engine = self.pages[self.tab_list["informations"]].get_table_engine

    def switch_to_page(self, page):
        idx = self.tab_list.get(page)
        if idx is not None:
            self.tab.set_active_tab(idx)
