# Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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


import re
import grt
import mforms
from workbench.db_utils import escape_sql_identifier
from workbench.plugins import insert_item_to_plugin_context_menu

from table_templates import TableTemplateManager

from sqlide_tableman_ext import CreateIndexForm
from sqlide_catalogman_ext import show_schema_manager

def esc_ident(s):
    # escape SQL identifier and add backtics only if needed
    if not re.match("^([a-z][A-Z][0-9]_)*$", s):
        return "`%s`" % escape_sql_identifier(s)
    return s


def _(s):
    return s

template_manager = None
def init():
    global template_manager

    template_manager = TableTemplateManager()


def handleLiveTreeContextMenu(name, sender, args):
    menu = mforms.fromgrt(args['menu'])
    selection = args['selection']

    # Add extra menu items to the SQL editor live schema tree context menu

    node_type = None
    mixed = False
    selection_type = None
    object_selected = False
    object_selected_count = 0
    column_selected = False
    if len(selection) == 1:
        object_type_caption = {
            'db.Schema' : "Schema",
            'db.Table' : "Table",
            'db.View' : "View",
            'db.Function' : "Function",
            'db.StoredProcedure' : "Stored Procedure",
            'db.Index' : "Index",
            'db.Trigger' : "Trigger",
            'db.ForeignKey' : "Foreign Key",
        }
    else:
        object_type_caption = {
            'db.Schema' : "Schemas",
            'db.Table' : "Tables",
            'db.View' : "Views",
            'db.Function' : "Functions",
            'db.StoredProcedure' : "Stored Procedures",
            'db.Index' : "Indexes",
            'db.Trigger' : "Triggers",
            'db.ForeignKey' : "Foreign Keys",
        }
    unique_tables = set()
    selection_db_type = None
    for s in selection:
        if s.type != node_type:
            if node_type is None:
                node_type = s.type
            else:
                mixed = True
        if s.type == 'columns':
            selection_type = s.type
            column_selected = True
            unique_tables.add(s.name)
        elif s.type in ['tables', 'views', 'functions', 'storedProcedures']:
            selection_type = s.type
            object_selected = False
            if s.type == 'tables':
                selection_db_type = 'db.Table'
            elif s.type == 'views':
                selection_db_type = 'db.View'
            elif s.type == 'functions':
                selection_db_type = 'db.Function'
            elif s.type == 'storedProcedures':
                selection_db_type = 'db.StoredProcedure'
        elif s.type in ('db.Schema', 'db.Table', 'db.View', 'db.Function', 'db.StoredProcedure'):
            selection_type = s.type
            selection_db_type = s.type
            object_selected = True
            object_selected_count += 1
            if s.type == 'db.Table':
                unique_tables.add(s.name)
        elif s.type in ('db.Index', 'db.Trigger'):
            selection_type = s.type
            object_selected = False
        elif s.type == ('db.Column'):
            selection_type = s.type
            if s.owner:
                unique_tables.add(s.owner.name)
                selection_type = s.owner.type+':'+s.type
            column_selected = True
        else:
            print "Unhandled type", s.type

    if mixed:
        selection_type = None
        pass
    else:
        item = menu.find_item('builtins_separator')
        if item:
            index = menu.get_item_index(item)+1
        else:
            index = 0
        needs_separator = False

        if False and object_selected and selection_type == 'db.Table':
            item = mforms.newMenuItem(_("Export Data to CSV File..."))
            item.set_name("export_table_csv")
            menu.insert_item(index-1, item)
            index += 1
            item = mforms.newMenuItem(_("Import Data from CSV File..."))
            item.set_name("import_table_csv")
            menu.insert_item(index-1, item)
            index += 1
            item = mforms.newMenuItem(_("Dump Data to SQL File..."))
            item.set_name("dump_table")
            menu.insert_item(index-1, item)
            index += 1

        if object_selected or column_selected:
            # Add menu items for generating code
            code_items = [
                          # Caption, callback name, supported node types, enable condition, mixed node types allowed
                          (_("Name"), 'name_short', ['db.Schema'], len(selection) > 0, False),
                          (_("Name (short)"), 'name_short', ['tables', 'views', 'functions', 'storedProcedures', 'db.Table', 'db.Table:db.Column', 'db.View:db.Column', 'db.View', 'db.StoredProcedure', 'db.Function'], len(selection) > 0, True),
                          (_("Name (long)"), 'name_long', ['tables', 'views', 'functions', 'storedProcedures', 'db.Table', 'db.Table:db.Column', 'db.View:db.Column', 'db.View', 'db.StoredProcedure', 'db.Function'], len(selection) > 0, True),
                          (_("Select All Statement"), 'select_all_statement', ['db.Table', 'db.View'], len(selection) > 0, False),
                          (_("Select All Statement"), 'select_all_statement', ['columns'], len(selection) == 1 and selection[0].type == 'columns', False),
                          (_("Select Columns Statement"), 'select_columns_statement', ['db.Table:db.Column', 'db.View:db.Column'], len(unique_tables) == 1, False),
                          (_("Insert Statement"), 'insert_all_statement', ['db.Table', 'columns'], len(selection) > 0, False),
                          (_("Insert Statement"), 'insert_columns_statement', ['db.Table:db.Column'], len(selection) > 0, False),
                          (_("Update Statement"), 'update_all_statement', ['db.Table', 'columns'], len(selection) > 0, False),
                          (_("Update Statement"), 'update_columns_statement', ['db.Table:db.Column'], len(selection) > 0, False),
                          (_("Delete Statement"), 'delete_statement', ['db.Table'], len(selection) > 0, False),
                          (_("Create Statement"), 'create_statement', ['db.Schema', 'db.Table', 'db.View', 'db.StoredProcedure', 'db.Function'], len(selection) > 0, False),
                          (_("Procedure Call"), 'call_procedure', ['db.StoredProcedure'], len(selection) > 0, False),
                          (_("Function Call"), 'call_function', ['db.Function'], len(selection) > 0, False),
                          (None, None, ['db.Table'], None, False),
                          (_("Join Selected Tables"), 'build_joined_select', ['db.Table'], len(selection) == 2, False),
                          (_("Delete with References"), 'build_cascaded_delete', ['db.Table'], len(selection) == 1, False),
                          (_("Select Row References"), 'build_cascaded_select', ['db.Table'], len(selection) == 1, False),
                          ]

            copy_submenu = mforms.newMenuItem("Copy to Clipboard")
            copy_submenu.set_name("copy_to_clipboard")
            menu.insert_item(index, copy_submenu)
            index += 1

            send_submenu = mforms.newMenuItem("Send to SQL Editor")
            send_submenu.set_name("send_to_editor")
            menu.insert_item(index, send_submenu)
            index += 1

            gencopy = CodeGenerator(sender, selection, False)
            gensend = CodeGenerator(sender, selection, True)

            for caption, name, types, enabled, allow_mixed in code_items:
                if not allow_mixed and mixed:
                    continue
                if types and selection_type not in types:
                    continue
                if caption is None:
                    copy_submenu.add_separator()
                    send_submenu.add_separator()
                else:
                    item = copy_submenu.add_item_with_title(caption, getattr(gencopy, name), name)
                    item.set_enabled(bool(enabled))

                    item = send_submenu.add_item_with_title(caption, getattr(gensend, name), name)
                    item.set_enabled(bool(enabled))
            needs_separator = True
        else:
            needs_separator = False

        if selection_type in ('tables', 'views', 'functions', 'storedProcedures', 'db.Schema', 'db.Table', 'db.View', 'db.Function', 'db.StoredProcedure') and len(selection) == 1:
            if needs_separator:
                menu.insert_item(index, mforms.newMenuItem("", mforms.SeparatorMenuItem))
                index += 1
                needs_separator = False

            item = mforms.newMenuItem("Create %s..." % object_type_caption.get(selection_db_type, selection_db_type))
            item.add_clicked_callback(lambda: do_create_object(sender, selection[0].schemaName, selection_db_type))
            menu.insert_item(index, item)
            index+= 1

            if selection_type in ('db.Table', 'tables'):
                item = mforms.newMenuItem("Create Table Like...")
                menu.insert_item(index, item)
                index += 1
                #        it = item.add_item_with_title("Selected Table", lambda: template_manager.create_table_like(sender, selection[0].schemaName, selection[0].name))
                #        if selection[0].type == 'tables':
                #            it.set_enabled(False)
                for templ in template_manager.templates:
                    item.add_item_with_title(templ.name, lambda templ=templ, schema=selection[0].schemaName: template_manager.create_table_like_template(sender, schema, templ))
                item.add_separator()
                item.add_item_with_title("Edit Templates...", template_manager.edit_templates)
                
        if selection_type is None:
            if needs_separator:
                menu.insert_item(index, mforms.newMenuItem("", mforms.SeparatorMenuItem))
                index += 1
                needs_separator = False

            item = mforms.newMenuItem("Create Schema...")
            item.add_clicked_callback(lambda: do_create_object(sender, None, "db.Schema"))
            menu.insert_item(index, item)
            index+= 1

        if object_selected:
            if needs_separator:
                menu.insert_item(index, mforms.newMenuItem("", mforms.SeparatorMenuItem))
                index += 1
                needs_separator = False

            if object_selected_count == 1:
                item = mforms.newMenuItem("Alter %s..." % object_type_caption.get(selection_type, selection_type))
            else:
                item = mforms.newMenuItem("Alter %i %s..." % (object_selected_count, object_type_caption.get(selection_type, selection_type)))
            item.add_clicked_callback(lambda: do_alter_object(sender, selection))
            menu.insert_item(index, item)
            index+= 1
            needs_separator = True

        if selection_type == 'db.Table' and object_selected:
            item = mforms.newMenuItem("Table Maintenance...")
            item.add_clicked_callback(lambda: show_schema_manager(sender, set([obj.schemaName for obj in selection]), True))
            menu.insert_item(index, item)
            index += 1

        if object_selected: # or selection_type in ('db.Index', 'db.Trigger'):  <-- enable this once we support selective refresh of LST on drop of these objects
            if needs_separator:
                menu.insert_item(index, mforms.newMenuItem("", mforms.SeparatorMenuItem))
                index += 1
                needs_separator = False

            if object_selected_count == 1:
                item = mforms.newMenuItem("Drop %s..." % object_type_caption.get(selection_type, selection_type))
            else:
                item = mforms.newMenuItem("Drop %i %s..." % (object_selected_count, object_type_caption.get(selection_type, selection_type)))
            item.add_clicked_callback(lambda: do_drop_object(sender, selection))
            menu.insert_item(index, item)
            index+= 1

        if selection_type == 'db.Table' and object_selected:
            item = mforms.newMenuItem("Truncate %s..." % ("Table" if object_selected_count == 1 else "%i Tables" % object_selected_count))
            item.add_clicked_callback(lambda: do_truncate_table(sender, selection))
            menu.insert_item(index, item)
            index += 1

        if selection_type in ('db.Table', 'db.Schema', 'tables'):
            item = mforms.newMenuItem("Search Table Data...")
            item.add_clicked_callback(lambda: open_search(sender))
            insert_item_to_plugin_context_menu(menu, item)

        if selection_type == 'db.Table:db.Column' and column_selected:
            if needs_separator:
                menu.insert_item(index, mforms.newMenuItem("", mforms.SeparatorMenuItem))
                index += 1
                needs_separator = False
            item = mforms.newMenuItem("Create Index...")
            item.add_clicked_callback(lambda: do_create_index(sender, selection))
            menu.insert_item(index, item)
            index += 1


def open_search(editor):
    grt.modules.MySQLDBSearchModule.showSearchPanel(editor)


def do_create_object(editor, schema_name, db_type):
    if db_type == 'db.Schema':
        editor.alterLiveObject(db_type, "", "")
    else:
        editor.alterLiveObject(db_type, schema_name, "")

def do_alter_object(editor, selection):
    for obj in selection:
        editor.alterLiveObject(obj.type, obj.schemaName, obj.name)



def review_sql_code(code):
    form = mforms.Form(mforms.Form.main_form(), mforms.FormNormal)
    form.set_title("Review SQL Code to Execute")
    box = mforms.newBox(False)
    box.set_padding(12)
    form.set_content(box)

    box.set_spacing(8)
    box.add(mforms.newLabel("Review the SQL code to be executed."), False, True)

    editor = mforms.CodeEditor()
    box.add(editor, True, True)
    editor.set_language(mforms.LanguageMySQL)
    editor.set_text(code)
    editor.set_features(mforms.FeatureReadOnly, True)

    ok = mforms.newButton()
    ok.set_text("Execute")
    cancel = mforms.newButton()
    cancel.set_text("Cancel")

    bbox = mforms.newBox(True)
    bbox.set_spacing(8)
    bbox.add_end(ok, False, True)
    bbox.add_end(cancel, False, True)
    box.add_end(bbox, False, True)

    form.set_size(500, 360)
    return form.run_modal(ok, cancel)



def do_truncate_table(editor, selection):
    tables = []
    for obj in selection:
        if obj.type == 'db.Table':
            tables.append("%s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name)))

    if len(tables) == 1:
        res = mforms.Utilities.show_message("Truncate Table", "Please confirm permanent deletion of all rows from table `%s`." % tables[0],
                                         "Review SQL", "Cancel", "Truncate")
    elif len(tables) > 1:
        res = mforms.Utilities.show_message("Truncate Tables", "Please confirm permanent deletion of all rows from %s selected tables." % len(tables),
                                         "Review SQL", "Cancel", "Truncate")
    else:
        return
    if res == mforms.ResultCancel:
        return
    elif res == mforms.ResultOk:
        if not review_sql_code("\n".join([("TRUNCATE %s;" % t) for t in tables])):
            return

    count = 0
    for table in tables:
        try:
            stmt = "TRUNCATE %s" % table
            editor.executeManagementCommand(stmt, 1)
            count += 1
        except Exception, exc:
            if count < len(tables)-1:
                if mforms.Utilities.show_error("Could not Truncate Table", str(exc)+"\nClick Cancel to stop truncating other tables.\n\n"+stmt, "OK", "Cancel", "") == mforms.ResultCancel:
                    break
            else:
                if mforms.Utilities.show_error("Could not Truncate Table", str(exc)+"\n\n"+stmt, "OK", "", "") == mforms.ResultCancel:
                    break
    mforms.App.get().set_status_text("%i tables truncated" % count)


def do_create_index(editor, selection):
    cols = []
    schema = None
    table = None
    for node in selection:
        if schema and schema != node.schemaName:
            mforms.Utilities.show_error("Create Index", "Please select one or more columns from the same table.", "OK", "", "")
            return
        if table and (not node.owner or table != node.owner.name):
            mforms.Utilities.show_error("Create Index", "Please select one or more columns from the same table.", "OK", "", "")
            return
        schema = node.schemaName
        table = node.owner.name
        cols.append(node.name)
    if cols:
        form = CreateIndexForm(mforms.Form.main_form(), editor, schema, table, cols, None)
        if form.run():
            pass



def do_drop_object(editor, selection):
    statements = []
    table_names = []
    object_types = set()
    for obj in selection:
        if obj.type == 'db.Schema':
            stmt = "DROP DATABASE %s" % esc_ident(obj.name)
            object_types.add("Schema")
        elif obj.type == 'db.Table':
            table_names.append("%s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name)))
            object_types.add("Table")
            continue
        elif obj.type == 'db.Trigger':
            stmt = "DROP TRIGGER %s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name))
            object_types.add("Trigger")
        elif obj.type == 'db.StoredProcedure':
            stmt = "DROP PROCEDURE %s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name))
            object_types.add("Stored Procedure")
        elif obj.type == 'db.Function':
            stmt = "DROP FUNCTION %s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name))
            object_types.add("Function")
        elif obj.type == 'db.View':
            stmt = "DROP VIEW %s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name))
            object_types.add("View")
        elif obj.type == 'db.Index':
            stmt = "DROP INDEX %s ON %s.%s" % (esc_ident(obj.name), esc_ident(obj.owner.schemaName), esc_ident(obj.owner.name))
            object_types.add("Index")
        else:
            print "Unsupported type for drop", obj.type
        statements.append((1, stmt))

    # multiple tables can be dropped in the same command (probably so that we dont get ref constraint errors)
    if table_names:
        stmt = "DROP TABLE %s" % ", ".join(table_names)
        count = len(statements) + len(table_names)
        statements.append((len(table_names), stmt))
    else:
        count = len(statements)

    if not count:
        return

    # don't let dropping tables or schemas mixed with other objects to make the confirmation message clearer and reduce risk of accident
    if len(object_types) > 1:
        mforms.Utilities.show_message("Drop Objects", "Multiple objects of different types are selected. To avoid accidents, please drop one type of object at a time.", "OK", "", "")
        return

    res = None
    if "Table" in object_types:
        if count == 1:
            res = mforms.Utilities.show_message("Drop Table",
                                             "Please confirm permanent deletion of table `%s` and its data." % (selection[0].name),
                                             "Review SQL", "Cancel", "Drop Now")
        else:
            res = mforms.Utilities.show_message("Drop %i Tables" % len(table_names),
                                             "Please confirm permanent deletion of tables %s and their data." % ", ".join(table_names),
                                             "Review SQL", "Cancel", "Drop Now")
    elif "Schema" in object_types:
        if count == 1:
            res = mforms.Utilities.show_message("Drop Schema",
                                             "Please confirm permanent deletion of schema `%s` and all its data." % (selection[0].name),
                                             "Review SQL", "Cancel", "Drop Now")
        else:
            res = mforms.Utilities.show_message("Drop %i Schemas" % count,
                                             "Please confirm permanent deletion of schemas %s and all their data." % (", ".join([s.name for s in selection])),
                                             "Review SQL", "Cancel", "Drop Now")
    else:
        if count == 1:
            res = mforms.Utilities.show_message("Drop %s" % list(object_types)[0],
                                             "Please confirm permanent deletion of %s `%s`." % (list(object_types)[0].lower(), selection[0].name),
                                             "Review SQL", "Cancel", "Drop Now")
        else:
            res = mforms.Utilities.show_message("Drop Objects", "Please confirm permanent deletion of %s %s objects: %s" % (count, ", ".join(list(object_types)), ", ".join([s.name for s in selection])),
                                             "Review SQL", "Cancel", "Drop Now")
    if res is None or res == mforms.ResultCancel:
        return

    if res == mforms.ResultOk:
        if not review_sql_code("\n".join([s+";" for c, s in statements])):
            return

    drop_count = 0
    for i, (c, stmt) in enumerate(statements):
        try:
            #This should be run in background because of the bug: #71327
            editor.executeCommand(stmt,1 ,1)
            drop_count += c
        except Exception, exc:
            if i < len(statements)-1:
                if mforms.Utilities.show_error("Could not Drop Object", str(exc)+"\nClick Cancel to stop dropping other objects.\n\n"+stmt, "OK", "Cancel", "") == mforms.ResultCancel:
                    break
            else:
                if mforms.Utilities.show_error("Could not Drop Object", str(exc)+"\n\n"+stmt, "OK", "", "") == mforms.ResultCancel:
                    break
    mforms.App.get().set_status_text("%i objects dropped" % drop_count)


class DependencyAnalyzer:
    def __init__(self, catalog, schema_name):
        self.schema_name = schema_name
        self.catalog = catalog
        self.tables_by_name = {}
        self.referencing_foreign_keys = {}


    def load_data(self, connection):
        m = grt.modules.DbMySQLRE
        grt.log_info("sqlide_grt", "Connecting...")
        ok, password = mforms.Utilities.find_or_ask_for_password("Connect to MySQL", connection.hostIdentifier, "root", False)
        if not ok:
            return None
        m.connect(connection, password)

        grt.log_info("sqlide_grt", "Reverse engineering schema %s...\n" % self.schema_name)
        options = {"reverseEngineerTables" : True,
            "reverseEngineerTriggers" : False,
            "reverseEngineerViews" : False,
            "reverseEngineerRoutines" : False}
        self.catalog = m.reverseEngineer(connection, "", [self.schema_name], options)

        m.disconnect(connection)
        grt.log_info("sqlide_grt", "Reverse engineer schema %s finished\n" % self.schema_name)

        return self.catalog


    def scan_tables(self):
        # build a map of all tables and the FKs that reference them
        for s in self.catalog.schemata:
            for t in s.tables:
                self.tables_by_name[t.name] = t

                for f in t.foreignKeys:
                    refs = self.referencing_foreign_keys.get(f.referencedTable, [])
                    refs.append(f)
                    self.referencing_foreign_keys[f.referencedTable] = refs


    def table_with_name(self, schema, name):
        if schema == self.schema_name:
            return self.tables_by_name.get(name, None)
        return None


    def get_foreign_keys_to_table(self, table):
        return self.referencing_foreign_keys.get(table, [])


    def get_referenced_tables(self, table):
        return [fk.referencedTable for fk in table.foreignKeys]


    def get_foreign_key_between(self, table1, table2):
        for fk in table1.foreignKeys:
            if fk.referencedTable == table2:
                return fk
        for fk in table2.foreignKeys:
            if fk.referencedTable == table1:
                return fk
        return None


    def get_foreign_key_from(self, table1, table2):
        for fk in table1.foreignKeys:
            if fk.referencedTable == table2:
                return fk
        return None


    def find_foreign_key_path_between(self, from_table, to_table):
        # find the shortest path between the 2 given tables using Dijkstra's algorithm
        # table = vertex, FK = edge

        predecessor = {}
        distance = {}

        # initialization
        for table in self.tables_by_name.values():
            predecessor[table] = None
            distance[table] = 99999999
        distance[from_table] = 0

        def relax(u, v):
            if u not in distance or v not in distance:
                return
            if distance[v] > distance[u] + 1:
                distance[v] = distance[u] + 1
                predecessor[v] = u

        def get_cheapest(remaining):
            v = reduce(lambda a, b: a if distance[a] < distance[b] else b, remaining)
            remaining.remove(v)
            return v

        # main loop
        remaining = self.tables_by_name.values()
        while remaining:
            u = get_cheapest(remaining)
            for v in self.get_referenced_tables(u):
                relax(u, v)

        # get the path
        foreign_keys = []
        t = to_table
        while predecessor[t]:
            pred = predecessor[t]
            foreign_keys.append(self.get_foreign_key_from(pred, t))
            t = pred

        return foreign_keys



def dependencyInfoForSchemaInEditor(editor, schema):
    catalog = editor.customData.get("sqlide_grt:Catalog:%s" % schema, None)
    if not catalog:
        mforms.App.get().set_status_text("Reverse engineering schema for %s..." % schema)

        info = DependencyAnalyzer(None, schema)
        catalog = info.load_data(editor.connection)
        if not catalog:
            mforms.App.get().set_status_text("Cancelled.")
            return

        mforms.App.get().set_status_text("%s reverse engineered" % schema)

        editor.customData["sqlide_grt:Catalog:%s" % schema] = catalog
    else:
        mforms.App.get().set_status_text("Reusing reverse engineered schema for %s..." % schema)
        info = DependencyAnalyzer(catalog, schema)
    return info


def format_fk_join(fk):
    joins = []
    for i in range(len(fk.referencedColumns)):
        joins.append("%s.%s = %s.%s" % (esc_ident(fk.referencedTable.name), esc_ident(fk.referencedColumns[i].name), esc_ident(fk.owner.name), esc_ident(fk.columns[i].name)))
    return " AND ".join(joins)


def join_tables(info, tables, auto_add_missing):
    joins = set()
    for i, t1 in enumerate(tables):
        for t2 in tables[i+1:]:
            fk = info.get_foreign_key_between(t1, t2)
            if fk:
                joins.add(fk)
                break
            else:
                if auto_add_missing:
                    fks = info.find_foreign_key_path_between(t1, t2) or info.find_foreign_key_path_between(t2, t1)
                    if not fks:
                        print "Could not find path from %s to %s" % (t1.name, t2.name)
                        return None
                    joins.update(fks)
                else:
                    return None

    if not joins:
        return None

    return "SELECT *\n    FROM %s\n    WHERE %s" % (", ".join(set([esc_ident(t.name) for t in tables] + [esc_ident(fk.owner.name) for fk in joins])), " AND ".join(format_fk_join(fk) for fk in joins))


table_column_cache = {}

def tokenize_argument_list(text):
    count = 1
    token = ""
    tokens = []
    for i in range(1, len(text)):
        if text[i] == '(':
            count += 1
        elif text[i] == ')':
            count -= 1
        if count == 0:
            if token:
                tokens.append(token)
            return tokens
        elif count == 1 and text[i] == ',':
            tokens.append(token)
            token = ""
        else:
            token += text[i]
    return None

class CodeGenerator:
    def __init__(self, editor, selection, to_editor):
        self.editor = editor
        self.selection = selection
        self.to_editor = to_editor


    def get_table_columns(self, schema, table):
        info_key = "%s:%s.%s" % (self.editor.__id__, schema, table)
        if table_column_cache.has_key(info_key):
            return table_column_cache[info_key]

        rs = self.editor.executeManagementQuery("SHOW COLUMNS FROM %s.%s" % (esc_ident(schema), esc_ident(table)), 0)
        if rs:
            columns = []
            ok = rs.goToFirstRow()
            while ok:
                name = rs.stringFieldValueByName("Field")
                is_key = rs.stringFieldValueByName("Key") == "PRI"
                default = rs.stringFieldValueByName("Default")
                columns.append((name, is_key, default))
                ok = rs.nextRow()
            table_column_cache[info_key] = columns
            print columns
            return columns
        else:
            return None

    def copy_to_clipboard(self, text):
        mforms.Utilities.set_clipboard_text(text)

    def send_to_editor(self, text):
        ed = self.editor.activeQueryEditor
        if not ed:
            ed = self.editor.addQueryEditor()
        if ed:
            ed.replaceSelection(text)

    def send(self, text):
        if self.to_editor:
            self.send_to_editor(text)
        else:
            self.copy_to_clipboard(text)


    def name_short(self):
        # assumes only real object nodes or Columns node
        parts = []
        for obj in self.selection:
            parts.append("%s" % esc_ident(obj.name))
        self.send(", ".join(parts))

    def name_long(self):
        # assumes only real object nodes of Columns node
        parts = []
        for obj in self.selection:
            parts.append("%s.%s" % (esc_ident(obj.owner.name) if obj.owner else esc_ident(obj.schemaName), esc_ident(obj.name)))
        self.send(", ".join(parts))

    def select_all_statement(self):
        # assumes only table nodes (or the Columns node of a table)
        parts = []
        for obj in self.selection:
            if obj.type == 'columns':
                obj = obj.owner
            parts.append("SELECT %s\nFROM %s.%s;\n" % (",\n    ".join("%s.%s" % (esc_ident(obj.name), esc_ident(c[0])) for c in self.get_table_columns(obj.schemaName, obj.name)), esc_ident(obj.schemaName), esc_ident(obj.name)))
        self.send("\n".join(parts))

    def select_columns_statement(self):
        # assumes only column nodes or Columns parent node, all of a single unique table
        columns = []
        table = None
        for obj in self.selection:
            if obj.type == 'db.Column':
                columns.append(obj.name)
                if obj.owner:
                    table = obj.owner
        if table:
            self.send("SELECT %s FROM %s.%s;\n" % (", ".join("%s" % esc_ident(c) for c in columns), esc_ident(table.schemaName), esc_ident(table.name)))

    def insert_all_statement(self):
        # assumes only table nodes (or the Columns node of a table)
        parts = []
        for obj in self.selection:
            if obj.type == 'columns':
                obj = obj.owner
            columns = self.get_table_columns(obj.schemaName, obj.name)
            parts.append("INSERT INTO %s.%s\n(%s)\nVALUES\n(%s);\n" % (esc_ident(obj.schemaName), esc_ident(obj.name),
                                                                             ",\n".join("%s" % esc_ident(c[0]) for c in columns),
                                                                             ",\n".join("<{%s: %s}>" % (c[0], c[2]) for c in columns)))
        self.send("\n".join(parts))

    def insert_columns_statement(self):
        # assumes only table column nodes, from the same table
        table = self.selection[0].owner
        self.send("INSERT INTO %s.%s\n(%s)\nVALUES\n(%s);\n" % (esc_ident(table.schemaName), esc_ident(table.name),
                                                                    ",\n".join("%s" % esc_ident(obj.name) for obj in self.selection),
                                                                    ",\n".join("<{%s}>" % c.name for c in self.selection)))

    def update_all_statement(self):
        # assumes only table nodes (or the Columns node of a table)
        parts = []
        for obj in self.selection:
            if obj.type == 'columns':
                obj = obj.owner
            columns = self.get_table_columns(obj.schemaName, obj.name)
            parts.append("UPDATE %s.%s\nSET\n%s\nWHERE %s;\n" % (esc_ident(obj.schemaName), esc_ident(obj.name),
                                                                  ",\n".join("%s = <{%s: %s}>" % (esc_ident(c[0]), c[0], c[2]) for c in columns),
                                                                  " AND ".join(["%s = <{expr}>" % esc_ident(c[0]) for c in columns if c[1]])))
        self.send("\n".join(parts))

    def update_columns_statement(self):
        # assumes only table nodes (or the Columns node of a table)
        table = self.selection[0].owner
        self.send("UPDATE %s.%s\nSET\n%s\nWHERE <{where_expression}>;\n" % (esc_ident(table.schemaName), esc_ident(table.name),
                                                                    ",\n".join("%s = <{%s}>" % (esc_ident(obj.name), obj.name) for obj in self.selection)))

    def delete_statement(self):
        parts = []
        for table in self.selection:
            parts.append("DELETE FROM %s.%s\nWHERE <{where_expression}>;\n" % (esc_ident(table.schemaName), esc_ident(table.name)))
        self.send("\n".join(parts))

    def create_statement(self):
        parts = []
        for obj in self.selection:
            wrapper = "%s;\n"
            if obj.type == 'db.Schema':
                rs = self.editor.executeManagementQuery("SHOW CREATE SCHEMA %s" % (esc_ident(obj.name)), 0)
                field_name = 'Create Database'
            elif obj.type == 'db.Table':
                rs = self.editor.executeManagementQuery("SHOW CREATE TABLE %s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name)), 0)
                field_name = 'Create Table'
            elif obj.type == 'db.Trigger':
                rs = self.editor.executeManagementQuery("SHOW CREATE TRIGGER %s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name)), 0)
                field_name = 'Statement'
                wrapper = "DELIMITER $$\n%s$$\nDELIMITER ;\n"
            elif obj.type == 'db.StoredProcedure':
                rs = self.editor.executeManagementQuery("SHOW CREATE PROCEDURE %s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name)), 0)
                field_name = 'Create Procedure'
                wrapper = "DELIMITER $$\n%s$$\nDELIMITER ;\n"
            elif obj.type == 'db.Function':
                rs = self.editor.executeManagementQuery("SHOW CREATE FUNCTION %s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name)), 0)
                field_name = 'Create Function'
                wrapper = "DELIMITER $$\n%s$$\nDELIMITER ;\n"
            elif obj.type == 'db.View':
                rs = self.editor.executeManagementQuery("SHOW CREATE VIEW %s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name)), 0)
                field_name = 'Create View'
            else:
                print "Unsupported type", obj.type
                continue
            if rs and rs.goToFirstRow():
                sql = rs.stringFieldValueByName(field_name)
                if not sql:
                    print "No field %s for %s" % (field_name, obj.name)
                else:
                    parts.append(wrapper % sql)
            else:
                print "Couldn't fetch create code for %s" % obj.name

        self.send("\n".join(parts))


    def call_procedure(self):
        parts = []
        for obj in self.selection:
            rs = self.editor.executeManagementQuery("SHOW CREATE PROCEDURE %s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name)), 0)
            if rs and rs.goToFirstRow():
                sql = rs.stringFieldValueByName('Create Procedure')
                if sql:
                    s = re.match("(CREATE .* PROCEDURE [^(]*\()", sql)
                    if s:
                        args = tokenize_argument_list(sql[len(s.groups()[0])-1:])
                        parts.append("CALL %s.%s(%s);\n" % (esc_ident(obj.schemaName), esc_ident(obj.name),
                                                                ", ".join(["<{%s}>" % a.strip() for a in args])))
        self.send("".join(parts))

    def call_function(self):
        parts = []
        for obj in self.selection:
            rs = self.editor.executeManagementQuery("SHOW CREATE FUNCTION %s.%s" % (esc_ident(obj.schemaName), esc_ident(obj.name)), 0)
            if rs and rs.goToFirstRow():
                sql = rs.stringFieldValueByName('Create Function')
                if sql:
                    s = re.match("(CREATE .* FUNCTION [^(]*\()", sql)
                    if s:
                        args = tokenize_argument_list(sql[len(s.groups()[0])-1:])
                        parts.append("%s.%s(%s)" % (esc_ident(obj.schemaName), esc_ident(obj.name),
                                                        ", ".join(["<{%s}>" % a.strip() for a in args])))
        self.send("\n".join(parts))


    def build_joined_select(self):
        editor = self.editor
        selection = self.selection

        info = dependencyInfoForSchemaInEditor(editor, selection[0].schemaName)
        if not info:
            return
        info.scan_tables()

        tables = []
        for object in selection:
            table = info.table_with_name(object.schemaName, object.name)
            if table:
                tables.append(table)

        if len(tables) != len(selection):
            mforms.Utilities.show_error("Join Selected Tables", "Could reverse engineer some of the selected tables.", "OK", "", "")
            return

        query = join_tables(info, tables, False)
        if query:
            self.send(query)
        else:
            if mforms.Utilities.show_error("Join Selected Tables", "Some of the tables could not be joined together.\nShould missing intermediate tables be searched and added?", "Add Missing", "Cancel", "") == mforms.ResultOk:
                query = join_tables(info, tables, True)
                if query:
                    self.send(query)
                else:
                    mforms.Utilities.show_error("Join Selected Tables", "Could not join selected tables.", "OK", "", "")
                    return
            else:
                return

        if not self.to_editor:
          mforms.App.get().set_status_text("Select statement for joined selection was copied to clipboard")

    def build_cascaded_select(self):
        editor = self.editor
        object = self.selection[0]

        info = dependencyInfoForSchemaInEditor(editor, object.schemaName)
        if not info:
            return
        info.scan_tables()
        the_table = info.table_with_name(object.schemaName, object.name)

        if not the_table:
            mforms.Utilities.show_error("Select Row References", "Could not find reverse engineered %s" % object.name, "OK", "", "")
            return

        if not the_table.primaryKey:
            mforms.Utilities.show_error("Select Row References", "Unable to find referencing tables for %s because the table has no primary key." % object.name,
                                      "OK", "", "")
            return

        # expression to match the row
        pk = []
        for c in the_table.primaryKey.columns:
            pk.append("%s.%s = @%s_to_select" % (the_table.name, c.referencedColumn.name, c.referencedColumn.name))
        pk = " AND ".join(pk)


        def create_select(table, info, selects, reference_chain, visited):
            if table in visited:
                return
            visited.add(table)

            def add_to_list(list, item):
                if item not in list:
                    return list + [item]
                return list

            for fk in info.get_foreign_keys_to_table(table):
                create_select(fk.owner, info, selects, reference_chain + [fk], visited)

            query = "SELECT %s.*\n" % table.name
            query += "    FROM %s\n" % ", ".join(add_to_list([fk.owner.name for fk in reference_chain], the_table.name))
            if reference_chain:
                query += "    WHERE %s\n" % "\n          AND ".join([format_fk_join(fk) for fk in reference_chain])
                query += "          AND %s;" % pk
            else:
                query += "    WHERE %s;" % pk
            selects.append(query)

        selects = []
        create_select(the_table, info, selects, [], set())
        varsetup = []
        for c in the_table.primaryKey.columns:
            varsetup.append("SET @%s_to_select = <{row_id}>;" % c.referencedColumn.name)
        query = "\n".join(varsetup) + "\n" + "\n".join(selects)
        self.send(query)
        
        if not self.to_editor:
          mforms.App.get().set_status_text("Select statements for %s references was copied to clipboard" % the_table.name)


    def build_cascaded_delete(self):
        """
            To delete a row from a table all references to that row from other tables must be deleted first,
            otherwise the FK constraint won't let it be done.
            So this function will build the DELETE statements needed to do that, by deleting
            references to the given row all the way up to the wanted table.
        """
        editor = self.editor
        object = self.selection[0]

        info = dependencyInfoForSchemaInEditor(editor, object.schemaName)
        if not info:
            return
        info.scan_tables()
        the_table = info.table_with_name(object.schemaName, object.name)

        if not the_table:
            mforms.Utilities.show_error("Cascading DELETE", "Could not find reverse engineered %s" % object.name, "OK", "", "")
            return

        if not the_table.primaryKey:
            mforms.Utilities.show_error("Cascading DELETE", "Unable to create a DELETE statement for %s because the table has no primary key." % object.name,
                                        "OK", "", "")
            return

        # expression to match the row to be deleted
        pk = []
        for c in the_table.primaryKey.columns:
            pk.append("%s.%s = @%s_to_delete" % (the_table.name, c.referencedColumn.name, c.referencedColumn.name))
        pk = " AND ".join(pk)


        def create_delete(table, info, deletes, reference_chain, visited):
            if table in visited:
                return
            visited.add(table)

            def add_to_list(list, item):
                if item not in list:
                    return list + [item]
                return list

            for fk in info.get_foreign_keys_to_table(table):
                create_delete(fk.owner, info, deletes, reference_chain + [fk], visited)

            query = "DELETE FROM %s\n" % table.name
            query += "    USING %s\n" % ", ".join(add_to_list([fk.owner.name for fk in reference_chain], the_table.name))
            if reference_chain:
                query += "    WHERE %s\n" % "\n          AND ".join([format_fk_join(fk) for fk in reference_chain])
                query += "          AND %s;" % pk
            else:
                query += "    WHERE %s;" % pk
            deletes.append(query)

        deletes = []
        create_delete(the_table, info, deletes, [], set())
        varsetup = []
        for c in the_table.primaryKey.columns:
            varsetup.append("SET @%s_to_delete = <{row_id}>;" % c.referencedColumn.name)
        query = """
-- All objects that reference that row (directly or indirectly) will be deleted when this snippet is executed.
-- To preview the rows to be deleted, use Select Row Dependencies
START TRANSACTION;
-- Provide the values of the primary key of the row to delete.
%s

%s
COMMIT;
""" % ("\n".join(varsetup), "\n".join(deletes))

        self.send(query)

        if not self.to_editor:
          mforms.App.get().set_status_text("DELETE statement for %s was copied to clipboard" % the_table.name)
        
