# Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

# import the wb module, where various utilities for working with plugins are defined
from wb import *
from mforms import App
import mforms

import table_templates

# import module for working with Workbench data structures
import grt

# create a module information descriptor. The variable name must be ModuleInfo
ModuleInfo = DefineModule(name= "WbTableUtils", author= "MySQL Team", version="1.0")


# export a function from this module, declaring its return and parameter types and then
# tell WB that it is a plugin to be shown in the context menu for the selected table
# and the Objects menu receiving the current selected table as input
@ModuleInfo.plugin("wb.table.util.copyInsertToClipboard", caption= "Copy Inserts to Clipboard", input= [wbinputs.objectOfClass("db.Table")], groups=["Catalog/Utilities","Menu/Objects"])
@ModuleInfo.export(grt.INT, grt.classes.db_Table)
def copyInsertToClipboard(table):

  inserts = table.inserts()
  if inserts != "":
     grt.modules.Workbench.copyToClipboard(table.inserts())
     App.get().set_status_text("Ready")
  else:
     App.get().set_status_text("The table " + table.owner.name + "." + table.name + " has no records for insert statements")
  return 0

# export a function from this module, declaring its return and parameter types and then
# tell WB that it is a plugin to be shown in the context menu for the selected table
# and the Objects menu receiving the current selected table as input
@ModuleInfo.plugin("wb.table.util.copyInsertTemplateToClipboard", caption= "Copy Insert Template to Clipboard", input= [wbinputs.objectOfClass("db.Table")], groups=["Catalog/Utilities","Menu/Objects"])
@ModuleInfo.export(grt.INT, grt.classes.db_Table)
def copyInsertTemplateToClipboard(table):
  code = "INSERT INTO `" + table.owner.name + "`.`" + table.name + "` ("
  first = 1
  for col in table.columns:
    if first == 0:
      code += "`, `" + col.name
    else:
      code += "`" + col.name
    first = 0
    
  code += "`) VALUES ("

  first = 1
  for col in table.columns:
    if first == 0:
      code += ", " + "NULL"
    else:
      code += "NULL"
    first = 0
    
  code += ");"

  grt.modules.Workbench.copyToClipboard(code)
  App.get().set_status_text("Ready")
  return 0


def _parse_table_definitions(code):
    defs= []

    tables= code.strip().split("\n")
    
    for t in tables:
        name, sep, columns = t.strip().partition(" ")
        column_defs = []
        for column in columns.strip().split(","):
            cn, sep, ct = column.strip().partition(" ")
            if cn:
                column_defs.append((cn.strip(), ct.strip()))
        defs.append((name, column_defs))
    return defs


def _create_table(catalog, schema, name, columns):
    types= catalog.simpleDatatypes

    tbl= schema.addNewTable("db.mysql")
    tbl.name= name
    
    for cname, ctype in columns:
        column= grt.classes.db_mysql_Column()
        column.owner = tbl
        column.name = cname[1:] if cname.startswith("*") else cname
        if ctype:
            column.setParseType(ctype, types)
        tbl.addColumn(column)
        if cname.startswith("*"):
            tbl.addPrimaryKeyColumn(column)

    return tbl

@ModuleInfo.plugin("wb.table.util.quickTables", caption= "Create Multiple Tables", input= [wbinputs.currentCatalog()], groups=["Catalog/Utilities","Menu/Objects"])
@ModuleInfo.export(grt.INT, grt.classes.db_Catalog)
def quickTablesInCatalog(catalog):

    form= mforms.Form(None, mforms.FormDialogFrame)
    label = mforms.newLabel("""You can create multiple tables by giving their outline in the following format:
    table1 *column1 int,column2 varchar(32),column3
    table2 column1,column2
    table3
    ... 

* denotes a primary key column. Column type is optional.""")

    box = mforms.newBox(False)
    box.set_spacing(12)
    box.set_padding(12)
    box.add(label, False, False)
    text = mforms.newTextBox(mforms.VerticalScrollBar)
    box.add(text, True, True)

    form.set_content(box)
    form.set_title("Create Multiple Tables")

    ok = mforms.newButton()
    ok.set_text("OK")
    cancel = mforms.newButton()
    cancel.set_text("Cancel")

    bbox = mforms.newBox(True)
    mforms.Utilities.add_end_ok_cancel_buttons(bbox, ok, cancel)
    bbox.set_spacing(8)
    box.add_end(bbox, False, True)

    form.set_size(600, 350)
    form.center()

    schema = catalog.schemata[0]
    ok.add_clicked_callback(lambda: form.end_modal(True))
    if form.run_modal(None, cancel):
        tabledefs= _parse_table_definitions(text.get_string_value())
        for name, columns in tabledefs:
            if name:
                _create_table(catalog, schema, name, columns)
        return 1
    return 0


@ModuleInfo.plugin("wb.table.util.openTableTemplateEditor", caption= "Open Table Templates Editor")
@ModuleInfo.export(grt.INT)
def openTableTemplateEditor():
    table_templates.TableTemplateManager().edit_templates()
    return 0


@ModuleInfo.export(grt.INT, grt.STRING)
def openTableTemplateEditorFor(name):
  table_templates.TableTemplateManager().edit_template(name)
  return 0

@ModuleInfo.export(grt.classes.db_Table, grt.classes.db_Schema, grt.STRING)
def createTableFromTemplate(schema, template_name):
    return table_templates.TableTemplateManager().create_table_like_template_in_schema(schema, template_name)

