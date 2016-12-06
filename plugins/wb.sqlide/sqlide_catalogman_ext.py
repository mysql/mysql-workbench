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

from workbench.log import log_error, log_warning
from mforms import IconStringColumnType, StringColumnType, LongIntegerColumnType, IntegerColumnType, NumberWithUnitColumnType
from workbench.notifications import NotificationCenter
from wb_admin_utils import make_panel_header
from workbench.utils import human_size, Version

def show_schema_manager(editor, selection, table_maintenance=False):
    try:
        editor.executeManagementQuery("select 1", 0)
    except grt.DBError, e:
        mforms.Utilities.show_error("Schema Inspector", "Can not launch the Schema Inspector because the server is unreacheble.", "OK", "", "")
        log_error("Can not launch the Schema Inspector because the server is unreacheble.\n")
        return False
    
    for schema_name in selection:
        sman = SchemaManager(editor, schema_name)
        dpoint = mforms.fromgrt(editor.dockingPoint)
        dpoint.dock_view(sman, "", 0)
        dpoint.select_view(sman)
        sman.set_title(schema_name)

        if table_maintenance:
            sman.show_table_maintenance()


def handleLiveTreeContextMenu(name, sender, args):
    menu = mforms.fromgrt(args['menu'])
    selection = args['selection']

    # Add extra menu items to the SQL editor live schema tree context menu
    schemas_selected = []


    for s in selection:
        if s.type == 'db.Schema':
            schemas_selected.append(s.name)
        else:
            return

    if schemas_selected:
        item = mforms.newMenuItem("Schema Inspector")
        item.add_clicked_callback(lambda: show_schema_manager(sender, schemas_selected))
        menu.insert_item(2, item)

class MaintenanceResultForm(mforms.Form):
    def __init__(self, results, checksum=False):
        mforms.Form.__init__(self, mforms.Form.main_form(), mforms.FormNormal)

        self.box = mforms.newBox(False)
        self.box.set_padding(12)
        self.box.set_spacing(8)
        self.set_content(self.box)

        self.tree = mforms.newTreeView(mforms.TreeFlatList|mforms.TreeAltRowColors|mforms.TreeShowColumnLines)
        self.tree.set_selection_mode(mforms.TreeSelectMultiple)
        self.box.add(self.tree, True, True)
        self.tree.add_column(mforms.StringColumnType, "Table", 200, False)
        if checksum:
            self.tree.add_column(mforms.StringColumnType, "Checksum", 100, False)
        else:
            self.tree.add_column(mforms.StringColumnType, "Operation", 80, False)
            self.tree.add_column(mforms.IconStringColumnType, "Message", 400, False)
        self.tree.end_columns()

        self._checksum = checksum

        app = mforms.App.get()
        icon_path = {
            'status': app.get_resource_path('mini_ok.png'),
            'error': app.get_resource_path('mini_error.png'),
            'info': '',
            'note': app.get_resource_path('mini_notice.png'),
            'warning': app.get_resource_path('mini_warning.png'),
        }

        ok = results.goToFirstRow()
        while ok:
            node = self.tree.add_node()
            node.set_string(0, results.stringFieldValue(0))
            if not checksum:
                node.set_string(1, results.stringFieldValue(1))
                node.set_icon_path(2, icon_path.get(results.stringFieldValue(2), ''))
                node.set_string(2, results.stringFieldValue(3))
                node.set_tag(results.stringFieldValue(2))
            else:
                node.set_string(1, results.stringFieldValue(1))
            ok = results.nextRow()

        bbox = mforms.newBox(True)
        self.box.add(bbox, False, True)

        copy = mforms.newButton()
        copy.add_clicked_callback(self.copy_to_clipboard)
        copy.set_text("Copy to Clipboard")
        bbox.add(copy, False, True)

        self.ok = mforms.newButton()
        self.ok.set_text("OK")
        self.ok.add_clicked_callback(self.close)
        bbox.add_end(self.ok, False, True)

        self.set_size(500, 400)


    def copy_to_clipboard(self):
        rows = []
        for node in self.tree.get_selection() or [self.tree.node_at_row(i) for i in range(self.tree.count())]:
            row = [node.get_string(0), node.get_string(1)]
            if not self._checksum:
                row.append(node.get_tag())
                row.append(node.get_string(2))
            rows.append("\t".join(row))
        mforms.Utilities.set_clipboard_text("\n".join(rows))

    def run(self):
        self.run_modal(self.ok, None)


def do_table_maintenance(editor, action, selection):
    query = action % ", ".join(selection)
    result = editor.executeManagementQuery(query, 1)

    form = MaintenanceResultForm(result, action.startswith("CHECKSUM"))
    form.set_title((action % "").strip())
    form.run()


class ObjectManager(mforms.Box):
    filter = None
    icon_column = None
    bad_icon_path = "task_error.png"
    node_name = None
    actions = []
    def __init__(self, editor, schema):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()

        self.schema = schema
        self.editor = editor
        
        self.target_version = Version.fromgrt(editor.serverVersion)

        self.main = mforms.newBox(False)
        self.add(self.main, True, True)

        self.error_heading = mforms.newLabel("")
        self.error_heading.set_style(mforms.BoldStyle)
        self.error_body = mforms.newLabel("")
        self.error_box = mforms.newBox(False)
        self.error_box.set_spacing(8)
        self.error_box.set_padding(8)
        self.error_box.add(self.error_heading, True, False)
        self.error_box.add(self.error_body, True, False)
        self.add(self.error_box, True, False)
        self.error_box.show(False)

        self.main.set_padding(8)
        self.main.set_spacing(8)

        self.tree = mforms.newTreeView(mforms.TreeFlatList|mforms.TreeAltRowColors|mforms.TreeShowColumnLines)
        self.tree.set_selection_mode(mforms.TreeSelectMultiple)
        
        #Check if there is method to load the columns, if not, skip.
        if hasattr(self, "preload_columns") and callable(getattr(self, "preload_columns")):
            self.preload_columns()
        
        for field, type, caption, width, min_version in self.columns:
            if min_version and not self.target_version.is_supported_mysql_version_at_least(Version.fromstr(min_version)):
                continue
            self.tree.add_column(type, caption, width, False)
        self.tree.end_columns()
        self.tree.set_allow_sorting(True)
        self.main.add(self.tree, True, True)

        self.menu = mforms.newContextMenu()
        self.menu.add_will_show_callback(self.menu_will_show)
        self.tree.add_activated_callback(self.on_activate)
        self.tree.set_context_menu(self.menu)

        self.icon_path = mforms.App.get().get_resource_path(self.klass+".16x16.png")
        self.bad_icon_path = mforms.App.get().get_resource_path(self.bad_icon_path)

        self.row_count = mforms.newLabel("")
        self.row_count.set_text("");
        
        self.refresh_btn = mforms.newButton()
        self.refresh_btn.set_text("Refresh")
        self.refresh_btn.add_clicked_callback(self.refresh)

        self.bbox = mforms.newBox(True)
        self.bbox.set_spacing(8)
        self.main.add(self.bbox, False, True)

        self.bbox.add(self.row_count, False, True)
        self.bbox.add_end(self.refresh_btn, False, True)

        for caption, callback_name in self.actions:
            if not caption:
                self.bbox.add(mforms.newLabel(" "), False, True)
                continue
            btn = mforms.newButton()
            btn.set_text(caption)
            btn.add_clicked_callback(getattr(self, callback_name))
            self.bbox.add(btn, False, True)
          
    def show_error(self, title, msg):
        self.main.show(False)
        self.error_box.show(True)
        self.error_heading.set_text(title)
        self.error_body.set_text(msg)

    def refresh_row_count(self):
        self.row_count.set_text("Count: %d" % self.tree.count())

    def on_activate(self, node, col):
        from sqlide_tableman_ext import show_table_inspector

        if show_table_inspector is not None:
            if self.klass == 'db.Table': 
                show_table_inspector(self.editor, [(self.schema, node.get_string(self.name_column).encode("utf8"))])
            elif self.klass == 'db.Index' and hasattr(self, 'parent_name_column'):
                show_table_inspector(self.editor, [(self.schema, node.get_string(self.parent_name_column).encode("utf8"))], "indexes")

    def menu_will_show(self, item):
        # item is the parent node which will be None when the
        # context menu has just being opened.
        # So when the call is done for a sub-menu, no reset is needed.
        if item is None:
            self.menu.remove_all()

            if item is None:
                parent_nodes = {}
                selection = grt.List()
                pobj = None
                for node in self.tree.get_selection():
                    name = node.get_string(self.name_column)
                    obj = grt.classes.db_query_LiveDBObject()
                    obj.name = name
                    obj.schemaName = self.schema
                    obj.type = self.klass
                    if hasattr(self, 'parent_name_column'):
                        if hasattr(self, 'table'):
                            parent_name = self.table
                        else:
                            parent_name = node.get_string(self.parent_name_column)
                        
                        if parent_nodes.has_key(parent_name):
                            obj.owner = parent_nodes[parent_name]
                        else:
                            pobj = grt.classes.db_query_LiveDBObject()
                            obj.owner = pobj
                            pobj.type = 'db.Table'
                            pobj.name = parent_name
                            pobj.schemaName = self.schema
                            parent_nodes[parent_name]= pobj
                    selection.append(obj)

                if not selection and self.node_name:
                    obj = grt.classes.db_query_LiveDBObject()
                    obj.schemaName = self.schema
                    obj.type = self.node_name
                    selection.append(obj)

                    sobj = grt.classes.db_query_LiveDBObject()
                    sobj.schemaName = self.schema
                    sobj.name = self.schema
                    sobj.type = "db.Schema"
                    obj.owner = sobj


                separator = mforms.newMenuItem("", mforms.SeparatorMenuItem)
                separator.set_name("bottom_plugins_separator")
                self.menu.add_item(separator)
                self.menu.add_item_with_title("Refresh", self.refresh, "refresh")

                args = grt.Dict()
                args["selection"] = selection
                args["menu"] = mforms.togrt(self.menu, "ContextMenu")
                args['schema_inspector'] = True
                NotificationCenter().send("GRNLiveDBObjectMenuWillShow", self.editor, args)
                
    def get_query(self):
        cols = []
        for field_obj, ctype, caption, width, min_version in self.columns:
            if min_version and not self.target_version.is_supported_mysql_version_at_least(Version.fromstr(min_version)):
                continue
            try:
                cols.append("`%s`" % field_obj['field'])
            except:
                cols.append("`%s`" % field_obj)
        return self.show_query % {'schema' : self.schema, 'columns' : ", ".join(cols)}

    def preload_data(self,query):
        try:
            rset = self.editor.executeManagementQuery(query, 0)
        except grt.DBError, e:
            if e.args[1] == 1044 or e.args[1] == 1142:
                mforms.Utilities.show_error("Access Error", "The current user does not have enough privileges to execute %s.\n\n%s"%(query, e.args[0]), "OK", "", "")
            else:
                mforms.Utilities.show_error("MySQL Error", "An error occurred retrieving information about the schema.\nQuery: %s\nError: %s"%(query, e.args[0]), "OK", "", "")
            return
        ok = rset.goToFirstRow()
        while ok:
            if not self.filter or self.filter(rset):
                node = self.tree.add_node()
                if self.is_row_corrupted(rset):
                    print rset.stringFieldValueByName("Name"), "IS CORRUPTED"
                    node.set_icon_path(self.icon_column, self.bad_icon_path)
                elif self.icon_column is not None:
                    node.set_icon_path(self.icon_column, self.icon_path)
                i = 0
                for field_obj, ctype, caption, width, min_version in self.columns:
                    if min_version and not self.target_version.is_supported_mysql_version_at_least(Version.fromstr(min_version)):
                        continue
                    format_func = None
                    field = None
                    try:
                        format_func = field_obj['format_func']
                        field = field_obj['field']
                    except:
                        field = field_obj
                    if ctype is mforms.IntegerColumnType:
                        if type(field) is int:
                            node.set_int(i, rset.intFieldValue(field) or 0)
                        else:
                            node.set_int(i, rset.intFieldValueByName(field) or 0)
                    elif ctype is mforms.LongIntegerColumnType:
                        if type(field) is int:
                            node.set_long(i, long(rset.stringFieldValue(field) or 0))
                        else:
                            node.set_long(i, long(rset.stringFieldValueByName(field) or 0))
                    else:
                        if type(field) is int:
                            node.set_string(i, rset.stringFieldValue(field) or "" if format_func is None else format_func(rset.stringFieldValue(field)))
                        else:
                            node.set_string(i, rset.stringFieldValueByName(field) or "" if format_func is None else format_func(rset.stringFieldValueByName(field)))
                    i += 1
            ok = rset.nextRow()

    def refresh(self):
        self.tree.clear()
        self.preload_data(self.get_query())
        self.refresh_row_count()


    def inspect_table(self):
        node = self.tree.get_selected_node()
        if node:
            self.on_activate(node, 0)

    def is_row_corrupted(self, rset):
        return False


class TableManager(ObjectManager):
    caption = "Tables"
    klass = "db.Table"
    node_name = "tables"
    icon_column = 0
    
    table_names = None

    show_query = "show table status from `%(schema)s` where Comment <> 'VIEW'"
    name_column = 0
    columns = [("Name", IconStringColumnType, "Name", 180, None),
               ("Engine", StringColumnType, "Engine", 60, None),
               ("Version", StringColumnType, "Version", 50, None),
               ("Row_format", StringColumnType, "Row Format", 100, None),
               ("Rows", LongIntegerColumnType, "Rows", 80, None),
               ("Avg_row_length", LongIntegerColumnType, "Avg Row Length", 100, None),
               ({'field' : "Data_length", 'format_func' : lambda x: human_size(long(x)) if x else ""}, NumberWithUnitColumnType, "Data Length", 100, None),
               ({'field' : "Max_data_length", 'format_func' : lambda x: human_size(long(x)) if x else ""}, NumberWithUnitColumnType, "Max Data Length", 100, None),
               ({'field' : "Index_length", 'format_func' : lambda x: human_size(long(x)) if x else ""}, NumberWithUnitColumnType, "Index Length", 100, None),
               ({'field' : "Data_free", 'format_func' : lambda x: human_size(long(x)) if x else ""}, NumberWithUnitColumnType, "Data Free", 80, None),
               ("Auto_increment", LongIntegerColumnType, "Auto Increment", 80, None),
               ("Create_time", StringColumnType, "Create Time", 150, None),
               ("Update_time", StringColumnType, "Update Time", 150, None),
               ("Check_time", StringColumnType, "Check Time", 150, None),
               ("Collation", StringColumnType, "Collation", 100, None),
               ("Checksum", StringColumnType, "Checksum", 80, None),
               ("Comment", StringColumnType, "Comment", 500, None)
               ]

    def is_row_corrupted(self, rset):
        return not rset.stringFieldValueByName("Engine")


    def refresh(self):
        self.table_names = []
        ObjectManager.refresh(self)


    def filter(self, rset):
        self.table_names.append(rset.stringFieldValueByName("Name"))
        return True

    def get_selection(self):
        selection = []
        for node in self.tree.get_selection():
            obj = grt.classes.db_query_LiveDBObject()
            obj.name = node.get_string(self.name_column)
            obj.schemaName = self.schema
            obj.type = self.klass
            selection.append(obj)
        return selection



class TableMaintenancePanel(mforms.Box):
    def __init__(self, editor):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()

        self.editor = editor

        self.set_spacing(8)
        self.set_padding(12)

        heading = mforms.newLabel("Table Maintenance Operations")
        heading.set_style(mforms.BigStyle)
        self.add(heading, False, True)

        help = mforms.newLabel("Select tables and click the operation you want to perform.\nNOTE: Some commands may require locking tables until completion,\nwhich may take a long time for large tables.")
        self.add(help, False, True)

        self.scroll = mforms.newScrollPanel(0)
        self.add(self.scroll, True, True)
        self.content = mforms.newBox(False)
        self.content.set_spacing(8)
        self.content.set_padding(12)
        self.scroll.add(self.content)

        self._buttons = []

        def make_command_box(callable, title, desc, tooltip, options = None, extra_options = None):
            l = mforms.newLabel(title)
            l.set_style(mforms.BoldStyle)
            self.content.add(l, False, True)

            l = mforms.newLabel(desc)
            self.content.add(l, False, True)

            if extra_options:
                self.content.add(extra_options, False, True)

            hb = mforms.newBox(True)
            hb.set_spacing(12)

            l = mforms.newImageBox()
            l.set_image(mforms.App.get().get_resource_path("mini_notice.png"))
            l.set_tooltip(tooltip)
            hb.add(l, False, True)

            for o in options:
                hb.add(o, False, True)

            btn = mforms.newButton()
            btn.add_clicked_callback(callable)
            btn.set_text(title.strip())
            hb.add_end(btn, False, True)

            self._buttons.append(btn)

            self.content.add(hb, False, True)

        self.analyze_local = mforms.newCheckBox()
        self.analyze_local.set_text("Don't write to BINLOG (local)")

        make_command_box(self.analyze_table, "Analyze Table",
                         """Analyzes and stores the key distribution for a table.
During the analysis, the table is locked with a read lock for InnoDB and MyISAM.""",
                         """With InnoDB tables, when you enable the innodb_stats_persistent option, you must run ANALYZE TABLE after loading substantial data into an InnoDB table, or creating a new index for one.

MySQL uses the stored key distribution to decide the order in which tables should be joined when you perform a join on something other than a constant. In addition, key distributions can be used when deciding which indexes to use for a specific table within a query.""",
                         [self.analyze_local])


        if editor.serverVersion.majorNumber > 5 or (editor.serverVersion.majorNumber == 5 and editor.serverVersion.minorNumber >= 6):
            extra_options = mforms.newBox(True)
            extra_options.set_spacing(4)

            self.optimize_ft_only = mforms.newCheckBox()
            self.optimize_ft_only.set_text("Optimize FULLTEXT only")
            self.optimize_ft_only.add_clicked_callback(self.optimize_ft_only_toggled)
            extra_options.add(self.optimize_ft_only, False, True)

            l = mforms.newLabel("")
            l.set_size(20, -1)
            extra_options.add(l, False, True)

            extra_options.add(mforms.newLabel("Number of words to optimize per run:"), False, True)
            self.optimize_ft_numwords = mforms.newTextEntry()
            self.optimize_ft_numwords.set_size(50, -1)
            self.optimize_ft_numwords.set_enabled(False)
            extra_options.add(self.optimize_ft_numwords, False, True)

            result = editor.executeManagementQuery("SHOW VARIABLES LIKE 'innodb_ft_num_word_optimize'", 0)
            if result and result.goToFirstRow():
                self.optimize_ft_numwords.set_value(result.stringFieldValue(1))
        else:
            extra_options = None
            self.optimize_ft_only = None

        self.optimize_local = mforms.newCheckBox()
        self.optimize_local.set_text("Don't write to BINLOG (local)")

        make_command_box(self.optimize_table, "\n\n"+"Optimize Table",
                         """Reorganizes the physical storage of table data and associated index data, 
to reduce storage space and improve I/O efficiency when accessing the table.""",
                         """The exact changes made to each table depend on the storage engine used by that table.

Use OPTIMIZE TABLE in these cases, depending on the type of table:

* After doing substantial insert, update, or delete operations on an InnoDB table that has its own .ibd file. The table and indexes are reorganized, and disk space can be reclaimed for use by the operating system.

* After doing substantial insert, update, or delete operations on columns that are part of a FULLTEXT index in an InnoDB table. Set the configuration option innodb_optimize_fulltext_only=1 first. To keep the index maintenance period to a reasonable time, set the innodb_ft_num_word_optimize option to specify how many words to update in the search index, and run Optimize Table until the search index is fully updated.""",
                         [self.optimize_local], extra_options)


        self.check_scan_opt = mforms.newSelector()
        self.check_scan_opt.add_items(["", "Quick", "Medium", "Extended", "For Upgrade"])

        self.check_fast_opt = mforms.newCheckBox()
        self.check_fast_opt.set_text("Fast")

        self.check_changed_opt = mforms.newCheckBox()
        self.check_changed_opt.set_text("Changed")

        options = [self.check_scan_opt, self.check_fast_opt, self.check_changed_opt]

        make_command_box(self.check_table, "\n\n"+"Check Table",
                         """CHECK TABLE checks a table or tables for errors. 
For MyISAM tables, the key statistics are updated as well.""",
                         """The FOR UPGRADE option checks whether the named tables are compatible with the current version of MySQL. With FOR UPGRADE, the server checks each table to determine whether there have been any incompatible changes in any of the table's data types or indexes since the table was created.
                             
Check Options:
- QUICK    Do not scan the rows to check for incorrect links. Applies to InnoDB and MyISAM tables and views.
- FAST    Check only tables that have not been closed properly. Applies only to MyISAM tables and views; ignored for InnoDB.
- CHANGED    Check only tables that have been changed since the last check or that have not been closed properly. Applies only to MyISAM tables and views; ignored for InnoDB.
- MEDIUM    Scan rows to verify that deleted links are valid. This also calculates a key checksum for the rows and verifies this with a calculated checksum for the keys. Applies only to MyISAM tables and views; ignored for InnoDB.
- EXTENDED    Do a full key lookup for all keys for each row. This ensures that the table is 100% consistent, but takes a long time. Applies only to MyISAM tables and views; ignored for InnoDB.""",
                         options)

        # The manual is full of scary warnings about this one, maybe better just leave user perform it by hand
#        make_command_box("\n"+"Repair Table",
#                         """""",
#                         """
#                             """)

        self.checksum_quick = mforms.newCheckBox()
        self.checksum_quick.set_text("Quick (if supported)")
        options = [self.checksum_quick]
        make_command_box(self.checksum_table, "\n\n"+"Checksum Table",
                         """CHECKSUM TABLE reports a checksum for the contents of a table.""",
                         """You can use this statement to verify that the contents are the same before and after a backup, rollback, or other operation that is intended to put the data back to a known state.""", options)

    def optimize_ft_only_toggled(self):
        self.optimize_ft_numwords.set_enabled(self.optimize_ft_only.get_active())
    

    def analyze_table(self):
        local = " NO_WRITE_TO_BINLOG" if self.analyze_local.get_active() else ""
        do_table_maintenance(self.editor, "ANALYZE%s TABLE %%s" % local, self.table_list)

    def optimize_table(self):
        if self.optimize_ft_only:
            old_value = None
            res = self.editor.executeManagementQuery("SELECT @@innodb_optimize_fulltext_only", 0)
            if res and res.goToFirstRow():
                old_value = res.stringFieldValue(0) != "OFF"
            command = "SET GLOBAL innodb_optimize_fulltext_only=%s, innodb_ft_num_word_optimize=%s" % ("1" if self.optimize_ft_only.get_active() else "0", self.optimize_ft_numwords.get_string_value())
            self.editor.executeManagementCommand(command, 1)

        local = " NO_WRITE_TO_BINLOG" if self.optimize_local.get_active() else ""
        do_table_maintenance(self.editor, "OPTIMIZE%s TABLE %%s" % local, self.table_list)

        if self.optimize_ft_only and old_value is not None:
            command = "SET GLOBAL innodb_optimize_fulltext_only=%s" % old_value
            self.editor.executeManagementCommand(command, 1)


    def check_table(self):
        option = self.check_scan_opt.get_string_value()
        option += " FAST" if self.check_fast_opt.get_active() else ""
        option += " CHANGED" if self.check_changed_opt.get_active() else ""
        do_table_maintenance(self.editor, "CHECK TABLE %%s %s" % option, self.table_list)

    def checksum_table(self):
        option = " QUICK" if self.checksum_quick.get_active() else ""
        do_table_maintenance(self.editor, "CHECKSUM TABLE %%s%s" % option, self.table_list)


    def show_tables(self, schema, tables):
        self.table_list = ["`%s`.`%s`" % (schema, table) for table, engine in tables]
        for b in self._buttons:
            b.set_enabled(len(self.table_list) > 0)


class TableManagerParent(mforms.Splitter):
    def __init__(self, editor, schema):
        mforms.Splitter.__init__(self, True, False)
        self.set_managed()
        self.set_release_on_add()

        self.schema = schema
        self.editor = editor
        self.summary_view = TableManager(editor, schema)
        self.add(self.summary_view, 100, True)

        self.node_name = self.summary_view.node_name
        self.caption = self.summary_view.caption

        self.summary_view.tree.add_changed_callback(self.table_selection_changed)

        self.goback_btn = mforms.newButton()
        self.goback_btn.set_text("< Summary List")
        self.goback_btn.add_clicked_callback(self.switch_back)
        self.summary_view.bbox.add(self.goback_btn, False, True)
        self.goback_btn.show(False)

        self.inspect_btn = mforms.newButton()
        self.inspect_btn.set_text("Inspect Table")
        self.inspect_btn.add_clicked_callback(self.summary_view.inspect_table)
        self.summary_view.bbox.add_end(self.inspect_btn, False, True)

        self.maintenance_btn = mforms.newButton()
        self.maintenance_btn.set_text("Maintenance >")
        self.maintenance_btn.add_clicked_callback(self.switch_maintenance)
        self.summary_view.bbox.add(self.maintenance_btn, False, True)

        self.right_view = None

    @property
    def table_names(self):
        return self.summary_view.table_names

    def refresh(self):
        self.summary_view.refresh()


    def switch_back(self):
        if self.right_view:
            self.remove(self.right_view)
            self.right_view = None

        self.summary_view.refresh_btn.show(True)
        self.maintenance_btn.show(True)
        self.goback_btn.show(False)
        self.inspect_btn.show(True)


    def switch_maintenance(self):
        self.right_view = TableMaintenancePanel(self.editor)
        self.add(self.right_view)
        self.relayout()
        self.right_view.relayout()
        self.set_divider_position(250)

        self.right_view.show_tables(self.schema, self.selected_tables())

        self.summary_view.refresh_btn.show(False)
        self.maintenance_btn.show(False)
        self.goback_btn.show(True)
        self.inspect_btn.show(False)


    def selected_tables(self):
        tables = []
        for node in self.summary_view.tree.get_selection():
            tables.append((node.get_string(0), node.get_string(1)))
        return tables


    def table_selection_changed(self):
        if self.right_view:
            self.right_view.show_tables(self.schema, self.selected_tables())

class ColumnManager(ObjectManager):
    caption = "Columns"
    klass = "db.Column"
    node_name = "columns"
    filter = None
    show_query = "select * from information_schema.columns where table_schema = '%(schema)s'"
    icon_column = 1
    name_column = 1
    parent_name_column = 0
    columns = [("TABLE_NAME", StringColumnType, "Table", 150, None),
               ("COLUMN_NAME", IconStringColumnType, "Column", 150, None),
               ("COLUMN_TYPE", StringColumnType, "Type", 120, None),
               ("COLUMN_DEFAULT", StringColumnType, "Default Value", 120, None),
               ("IS_NULLABLE", StringColumnType, "Nullable", 50, None),
               ("CHARACTER_SET_NAME", StringColumnType, "Character Set", 80, None),
               ("COLLATION_NAME", StringColumnType, "Collation", 100, None),
               ("PRIVILEGES", StringColumnType, "Privileges", 200, None),
               ("EXTRA", StringColumnType, "Extra", 100, None),
               ("COLUMN_COMMENT", StringColumnType, "Comments", 200, None)]
    #actions = [("Manage Indexes", "manage")]


class IndexManager(ObjectManager):
    caption = "Indexes"
    klass = "db.Index"
    node_name = "indexes"
    filter = None
    show_query = "show index from `%(table)s` from `%(schema)s`"
    parent_name_column = 0
    name_column = 1
    icon_column = 0
    columns = [("Table", IconStringColumnType, "Table", 200, None),
               ("Key_name", IconStringColumnType, "Name", 200, None),
               ({'field' : "Non_unique", 'format_func' : lambda val: "Yes" if val != "1" else "No"}, StringColumnType, "Unique", 50, None),
               ("Index_type", StringColumnType, "Index Type", 50, None),
               ("Index_comment", StringColumnType, "Index Comment", 200, "5.5"),
               ("Column_name", StringColumnType, "Column", 100, None),
               ("Seq_in_index", IntegerColumnType, "Seq in Index", 100, None),
               ("Packed", StringColumnType, "Packed", 50, None),
               ("Collation", StringColumnType, "Collation", 50, None),
               ("Cardinality", StringColumnType, "Cardinality", 50, None),
               ("Sub_part", StringColumnType, "Sub part", 50, None),
               ("Null", StringColumnType, "NULL", 50, None),
               ("Comment", StringColumnType, "Comment", 200, None)]
    #actions = [("Manage Indexes", "manage")]

    def __init__(self, editor, schema, owner):
        ObjectManager.__init__(self, editor, schema)

        self.owner= owner
        self.table_manager = owner.tab_tables


    def manage(self):
        self.owner.show_index_manager()


    def refresh(self):
        self.tree.clear()
        for table in self.table_manager.table_names or []:
            try:
                rset = self.editor.executeManagementQuery(self.show_query % {'table' : table.replace("`", "``"), 'schema' : self.schema.replace("`", "``")}, 0)
            except grt.DBError, err:
                log_warning("Error querying index info for %s.%s: %s\n" % (self.schema, table, err[0]))
                continue
            ok = rset.goToFirstRow()
            while ok:
                if not self.filter or self.filter(rset):
                    node = self.tree.add_node()
                    node.set_icon_path(0, self.icon_path)
                    i = 0
                    for field_obj, ctype, caption, width, min_version in self.columns:
                        if min_version and not self.target_version.is_supported_mysql_version_at_least(Version.fromstr(min_version)):
                            continue
                        format_func = None
                        field = None
                        try:
                            format_func = field_obj['format_func']
                            field = field_obj['field']
                        except:
                            field = field_obj

                        if ctype == mforms.IntegerColumnType:
                            if type(field) is int:
                                node.set_int(i, rset.intFieldValue(field) or 0)
                            else:
                                node.set_int(i, rset.intFieldValueByName(field) or 0)
                        else:
                            if type(field) is int:
                                node.set_string(i, rset.stringFieldValue(field) or "" if format_func is None else format_func(rset.stringFieldValue(field)))
                            else:
                                node.set_string(i, rset.stringFieldValueByName(field) or "" if format_func is None else format_func(rset.stringFieldValueByName(field)))
                        i += 1
                ok = rset.nextRow()
        self.refresh_row_count()


class GrantsManager(ObjectManager):
    caption = "Grants"
    klass = "db.Grants"
    node_name = "table_privileges"
    name_column = 0
    columns = [("Host", IconStringColumnType, "Host", 100, None),
           ("User", StringColumnType, "User", 100, None),
           ("Db", StringColumnType, "Scope", 100, None),
           ("Select_priv", StringColumnType, "Select", 50, None),
           ("Insert_priv", StringColumnType, "Insert", 50, None),
           ("Update_priv", StringColumnType, "Update", 50, None),
           ("Delete_priv", StringColumnType, "Delete", 50, None),
           ("Create_priv", StringColumnType, "Create", 50, None),
           ("Drop_priv", StringColumnType, "Drop", 50, None),
           ("Grant_priv", StringColumnType, "Grant", 50, None),
           ("References_priv", StringColumnType, "References", 50, None),
           ("Index_priv", StringColumnType, "Index", 50, None),
           ("Alter_priv", StringColumnType, "Alter", 50, None),
           ("Create_tmp_table_priv", StringColumnType, "Create tmp table", 50, None),
           ("Lock_tables_priv", StringColumnType, "Lock Tables", 50, None),
           ("Create_view_priv", StringColumnType, "Create View", 50, None),
           ("Create_routine_priv", StringColumnType, "Create Routine", 50, None),
           ("Alter_routine_priv", StringColumnType, "Alter Routine", 50, None),
           ("Execute_priv", StringColumnType, "Execute", 50, None),
           ("Event_priv", StringColumnType, "Event", 50, None),
           ("Trigger_priv", StringColumnType, "Trigger", 50, None),
           ]

    def get_query(self):
        if len(self.columns) == 0: # Probably user doesn't have privileges to list the privilege tables.
            return []

        output = []
        fields = []
        fields_where = []
        for field_obj, ctype, caption, width, min_version in self.columns:
            if min_version and not self.target_version.is_supported_mysql_version_at_least(Version.fromstr(min_version)):
                continue
            field = None
            try:
                field = field_obj['field']
            except:
                field = field_obj

            if field == "scope":
                continue
            
            if field not in ['Db']:
                fields.append(field)
            if field not in ['User','Host','Db']:
                fields_where.append("%s = 'Y'" % field.replace(" ","_"))

        output.append("SELECT '<global>' as Db,%(sel_fields)s FROM mysql.user WHERE %(where_fields)s" % {'sel_fields' : ",".join(fields), 'where_fields' : " OR ".join(fields_where)})

        output.append("SELECT Db,%(sel_fields)s FROM mysql.db WHERE '%(schema)s' like db" % {'sel_fields' : ",".join(fields), 'schema' : self.schema})

        return output
    
    def refresh(self):
        self.tree.clear()
        for query in self.get_query():
            self.preload_data(query)
        self.refresh_row_count()


class ViewManager(ObjectManager):
    caption = "Views"
    klass = "db.View"
    node_name = "views"
    filter = None
    show_query = "show table status from `%(schema)s` where Comment = 'VIEW'"
    name_column = 0
    icon_column = 0
    columns = [(0, IconStringColumnType, "Name", 400, None)]


class TriggerManager(ObjectManager):
    caption = "Triggers"
    klass = "db.Trigger"
    node_name = "triggers"
    show_query = "show triggers from `%(schema)s`"
    name_column = 0
    icon_column = 0
    columns = [("Trigger", IconStringColumnType, "Name", 200, None),
               ("Event", StringColumnType, "Event", 100, None),
               ("Table", StringColumnType, "Table", 100, None),
               ("Timing", StringColumnType, "Timing", 100, None),
               ("Created", StringColumnType, "Created", 100, None),
               ("sql_mode", StringColumnType, "SQL Mode", 100, None),
               ("Definer", StringColumnType, "Definer", 100, None),
               ("character_set_client", StringColumnType, "Client Character Set", 100, None),
               ("collation_connection", StringColumnType, "Connection Collation", 100, None),
               ("Database Collation", StringColumnType, "Database Collation", 100, None),
               ]


class ProcedureManager(ObjectManager):
    caption = "Stored Procedures"
    klass = "db.StoredProcedure"
    node_name = "storedProcedures"
    show_query = "show procedure status where Db = '%(schema)s'"
    name_column = 0
    icon_column = 0
    columns = [("Name", IconStringColumnType, "Name", 200, None),
               ("Type", StringColumnType, "Type", 100, None),
               ("Definer", StringColumnType, "Definer", 100, None),
               ("Modified", StringColumnType, "Modified", 100, None),
               ("Created", StringColumnType, "Created", 100, None),
               ("Security_type", StringColumnType, "Security Type", 100, None),
               ("character_set_client", StringColumnType, "Client Character Set", 100, None),
               ("collation_connection", StringColumnType, "Connection Collation", 100, None),
               ("Database Collation", StringColumnType, "Database Collation", 100, None),
               ("Comment", StringColumnType, "Comment", 400, None),
               ]


class FunctionManager(ObjectManager):
    caption = "Functions"
    klass = "db.Function"
    node_name = "functions"
    show_query = "show function status where Db = '%(schema)s'"
    name_column = 0
    icon_column = 0
    columns = ProcedureManager.columns


class EventManager(ObjectManager):
    caption = "Events"
    klass = "GrtObject"
    node_name = "events"
    show_query = "show events from `%(schema)s`"
    name_column = 0
    icon_column = 0
    columns = [("Name", IconStringColumnType, "Name", 200, None),
               ("Definer", StringColumnType, "Row Format", 100, None),
               ("Time zone", StringColumnType, "Time Zone", 100, None),
               ("Type", StringColumnType, "Type", 100, None),
               ("Execute at", StringColumnType, "Execute at", 100, None),
               ("Interval value", StringColumnType, "Interval value", 100, None),
               ("Interval field", StringColumnType, "Interval field", 100, None),
               ("Starts", StringColumnType, "Starts", 100, None),
               ("Ends", StringColumnType, "Ends", 100, None),
               ("Status", StringColumnType, "Status", 100, None),
               ("Originator", StringColumnType, "Originator", 100, None),
               ("character_set_client", StringColumnType, "Client Character Set", 100, None),
               ("collation_connection", StringColumnType, "Connection Collation", 100, None),
               ("Database Collation", StringColumnType, "Database Collation", 100, None)
               ]

class SchemaInfoPanel(mforms.Box):
    caption = "Info"
    node_name = "informations"
    def __init__(self, editor, schema):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        
        self.editor = editor
        self._schema = schema
        
        self.table = mforms.newTable()
        self.table.set_row_count(7)
        self.table.set_column_count(2)
        self.table.set_row_spacing(8)
        self.table.set_column_spacing(8)

        def make_title(t):
            l = mforms.newLabel(t)
            l.set_style(mforms.BoldStyle)
            return l

        self.panel_header_box = mforms.newBox(True)
        self.panel_header_box.set_padding(10)
        self.panel_header_box.add(make_panel_header("db.Schema.32x32.png", self.editor.connection.name, "%s" % (schema)), False, True)
        
        self.add(self.panel_header_box, False, True)

        self.table.add(make_title("Schema Details"), 0, 2, 0, 1, mforms.HFillFlag)

        self.table.add(mforms.newLabel("Default collation:"),              0, 1, 3, 4, mforms.HFillFlag)
        self.col_default_collation_name = mforms.newLabel("")
        self.col_default_collation_name.set_style(mforms.BoldStyle)
        self.table.add(self.col_default_collation_name,                    1, 2, 3, 4, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Default characterset:"),           0, 1, 4, 5, mforms.HFillFlag)
        self.col_default_character_set_name = mforms.newLabel("")
        self.col_default_character_set_name.set_style(mforms.BoldStyle)
        self.table.add(self.col_default_character_set_name,                1, 2, 4, 5, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Table count:"),                    0, 1, 5, 6, mforms.HFillFlag)
        self.table_count = mforms.newLabel("")
        self.table_count.set_style(mforms.BoldStyle)
        self.table.add(self.table_count,                                   1, 2, 5, 6, mforms.HFillFlag|mforms.HExpandFlag)
        
        self.table.add(mforms.newLabel("Database size (rough estimate):"), 0, 1, 6, 7, mforms.HFillFlag)
        self.database_size = mforms.newLabel("")
        self.database_size.set_style(mforms.BoldStyle)
        self.table.add(self.database_size,                                 1, 2, 6, 7, mforms.HFillFlag|mforms.HExpandFlag)

        tbox = mforms.Box(False)
        tbox.set_spacing(15)
        tbox.set_padding(15)
        tbox.add(self.table, True, True)
        
        self.add(tbox, False, True)
    
    def refresh(self):
        try:
            rset = self.editor.executeManagementQuery("select * from information_schema.schemata WHERE schema_name = '%s'" % self._schema, 0)
        except grt.DBError, e:
            log_error("select * from information_schema.schemata WHERE schema_name = '%s' : %s\n" % (self._schema, e))
            rset = None
            
        if rset:
            ok = rset.goToFirstRow()
            
            if ok:
                for col in rset.columns:
                    if hasattr(self, "col_%s" % col.name.lower()):
                        getattr(self, "col_%s" % col.name.replace(" ", "_").lower()).set_text(rset.stringFieldValueByName(col.name))
                        
        try:
            rset = self.editor.executeManagementQuery("select sum(data_length) + sum(index_length) database_size,count(*) table_count from information_schema.tables WHERE table_schema = '%s'" % self._schema, 0)
        except grt.DBError, e:
            log_error("select sum(data_length) + sum(index_length) database_size,count(*) table_count from information_schema.tables WHERE table_schema = '%s' : %s\n" % (self._schema, e))
            rset = None
            
        if rset:
            ok = rset.goToFirstRow()
            if ok:
                self.database_size.set_text(human_size(rset.floatFieldValueByName("database_size")))
                self.table_count.set_text(rset.stringFieldValueByName("table_count"))

class SchemaManager(mforms.AppView):
    def __init__(self, editor, schema_name):
        mforms.AppView.__init__(self, False, "schema_inspector", False)

        self.editor = editor

        self.tabview = mforms.newTabView()
        self.add(self.tabview, True, True)

        self.pages = []

        tabs = [SchemaInfoPanel, TableManagerParent, ColumnManager, IndexManager, TriggerManager, ViewManager, ProcedureManager, FunctionManager, GrantsManager]
        if self.editor.serverVersion.majorNumber > 5 or (self.editor.serverVersion.majorNumber == 5 and self.editor.serverVersion.minorNumber >= 1):
            tabs.append(EventManager)
        # tabs.append(AccessManager)
        for Tab in tabs:
            try:
                if Tab is IndexManager:
                    tab = Tab(editor, schema_name, self)
                else:
                    tab = Tab(editor, schema_name)
                setattr(self, "tab_"+tab.node_name, tab)
                self.pages.append(tab)
                self.tabview.add_page(tab, tab.caption)
            except Exception:
                import traceback
                log_error("Error initializing tab %s: %s\n" % (tab.node_name, traceback.format_exc()))
        self.refresh()


    def show_table_maintenance(self):
        self.tab_tables.switch_maintenance()


    def refresh(self):
        for tab in self.pages:
            tab.refresh()


