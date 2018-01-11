/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include "grt.h"
#include "base/string_utilities.h"
#include "base/util_functions.h"

#include "grtpp_shell_python_help.h"

#define NL "\n"

struct HelpTopic {
  const char *keyword;
  const char *text;
};

static HelpTopic help_topics[] = {
  {"grt", "    GRT (Generic RunTime) is internal system used by Workbench to hold model" NL
          "document data and allow plugins and modules to interface between each other," NL
          "with the Workbench application and the model." NL
          "    Workbench model data (diagrams, schemas, tables etc) is stored in a hierarchy" NL
          "of objects that can be accessed by any plugin. To allow that, the data is" NL
          "represented in 6 different datatypes that can be accessed from any language" NL
          "supported: integers, doubles, strings, dicts, lists and objects." NL
          "    When working from Python, simple data types (integers, doubles and strings) are" NL
          "seen as normal Python values of the corresponding type." NL
          "    Lists and dicts are kept in their internal representation but can be mostly" NL
          "treated as Python lists and dicts, meaning you can index lists and get items from" NL
          "dicts with the usual Python syntax (ie aList[0], len(aList), aDict['key'] etc)." NL
          "    Objects contain data fields and methods, but the GRT only recognizes objects" NL
          "from a pre-registered class hierarchy. You can see the list of existing classes in" NL
          "the 'grt' module in the 'classes' list or in the Classes tab in the Scripting Shell" NL "window." NL
          "    Workbench document data is kept in a tree-like structure that you can access and" NL
          "modify from Python. However, it is easy to corrupt a document beyond repair, so be" NL
          "careful when modifying the GRT tree for models and always make backups. Read-only" NL
          "access to the tree is safe and sufficient for many purposes." NL NL
          "    In Python, the GRT tree root node can be accessed as grt.root" NL
          "    Some examples of nodes that can be of interest:" NL NL
          "    grt.root.wb.doc.physicalModels[0].catalog.schemata" NL "        list of schemas in the open model" NL
          "    grt.root.wb.doc.physicalModels[0].diagrams" NL "        list of diagrams in the model" NL
          "    grt.root.wb.options" NL "        Workbench options and other state data" NL NL
          "    While you cannot create new classes in the GRT from Python, you can create" NL
          "modules and plugins that are usable from other Workbench parts, including from" NL
          "other languages. See the corresponding help topics for more information about" NL
          "calling and writing your own modules." NL NL
          "    For practical tips and examples on how to write scripts, see 'scripting'." NL
          "    For more information about GRT Objects, see 'objects'" NL NL},
  {"objects", ""},
  {"scripting",
   "    To execute a Python script against the loaded model, you can simply input it" NL
   "in the Scripting Shell entry box. For doing quick tests and exploring it will be" NL
   "the quickest way. If you have a script file you want to execute, you can execute" NL
   "it through the 'Scripting -> Run Workbench Script File' menu item." NL NL
   "    If you have a script that is used often or that you want to distribute to other" NL
   "people, you may consider turning it into a plugin. A plugin will be accessible from" NL
   "the Workbench UI in the main menu or in certain context menus for model objects," NL
   "depending on how you register it. See the 'plugins' section for more info about this." NL
   "    You can also put your scripts in a library of plain Python functions, and invoke" NL
   "them from the Scripting Shell. By placing the library in the libraries folder in your" NL
   "user's Workbench data folder (you can see its path printed in the Workbench Shell) it" NL
   "will automatically loaded when Workbench starts. This is a more flexible approach as" NL
   "you can easily pass any kind of argument to your scripts with little coding effort." NL NL "    Samples" NL
   "    -------" NL "    The following are some sample snippets for things you can do in Workbench," NL
   "to view them type '? keyword'" NL NL "    iter.figures - iterate through figures in the diagram" NL
   "    iter.tables - iterate through tables in the 1st schema in the model" NL
   "    chcolor - change color of all figures matching some criteria" NL
   "    lowercase - rename titles of all tables and columns to lower case only" NL "    "},
  {"iter.figures", "# iterate over figures from a diagram and prints some information about them" NL
                   "def iter_figures(diagram):" NL "    for figure in diagram.figures:" NL
                   "        print '%s %s (%i,%i)'%(figure.__grtclassname__, figure.name, figure.left, figure.top)" NL NL
                   "iter_figures(grt.root.wb.doc.physicalModels[0].diagrams[0])" NL},
  {"iter.tables",
   "# iterate over database tables in a given schema and print columns and foreign keys" NL
   "def iter_tables(schema):" NL "    for table in schema.tables:" NL
   "        print '%s <%s>' % (table.name, table.tableEngine)" NL "        print 'Columns:'" NL
   "        for column in table.columns:" NL
   "            print '%s%s : %s' % (column.name, table.isPrimaryKeyColumn(column) and 'PK' or '', "
   "column.formattedType())" NL "        print 'Foreign Keys:'" NL "        for fk in table.foreignKeys:" NL
   "            print '%s' % fk.name" NL NL "iter_tables(grt.root.wb.doc.physicalModels[0].catalog.schemata[0])" NL},
  {"chcolor",
   "# change color of all figures in diagram of a given class" NL "def change_color(diagram, klass, new_color):" NL
   "    for figure in diagram.figures:" NL "        if figure.__grtclassname__ == klass:" NL
   "            figure.color = new_color" NL NL
   "change_color(grt.root.wb.doc.physicalModels[0].diagrams[0], 'workbench.physical.TableFigure', '#aa8844')" NL},
  {"lowercase",
   "# change name of all tables to lowercase" NL "def change_table_names(function, schema=None):" NL
   "    if schema is None:" NL "       schema = grt.root.wb.doc.physicalModels[0].catalog.schemata[0]" NL
   "    for table in schema.tables:" NL "        s = function(table.name)" NL "        if s != table.name:" NL
   "            table.name = s" NL NL "change_table_names(lambda name: name.lower())" NL},
  {"modules",
   "    In the GRT, modules are libraries with a list of functions that are exported for use" NL
   "by code in other modules, scripts or Workbench itself. Modules can be currently written" NL
   "in C++ and Python but the datatypes used in arguments and return value must be GRT" NL "types." NL
   "    To export certain functions from a Python source file as a module, you need to do the" NL "following:" NL
   "- the file must be placed in the user modules folder. You can see its path in the" NL
   "scripting shell window as 'Looking for user plugins in ...'. You can also install it" NL
   "using the Scripting -> Install Plugin/Module File... menu command." NL
   "- the filename must have the _grt.py suffix (ex: my_modules_grt.py)" NL
   "- define some module metadata using the DefineModule function in the 'wb' module:" NL "from wb import *" NL
   "ModuleInfo = DefineModule(name='MyStuff', author='me', version='1.0')" NL
   "- functions to be exported must have their signature declared with the export decorator" NL
   "in the previously created ModuleInfo object:" NL "@ModuleInfo.export(grt.INT, grt.STRING)" NL
   "def checkString(s):" NL "..."
   "    The arguments to export is a list of GRT type names, starting with the" NL
   "return type of the function, followed by 0 or more argument types. The allowed type" NL
   "names are (note that they are in the grt module, which must be imported):" NL
   " - grt.INT integer values; also used for boolean values" NL " - grt.DOUBLE floating point numeric values" NL
   " - grt.STRING UTF-8 or ASCII string data" NL " - grt.DICT a key/value dictionary value; keys must be strings" NL
   " - grt.LIST a list of other values. You may specify the type of the contents as" NL
   "   a tuple in the form (grt.LIST, <type-or-class>), ex: (grt.LIST, grt.STRING) for a list" NL
   "of strings or (grt.LIST, grt.classes.db_Table) for a list of db.Table objects" NL
   " - grt.OBJECT an instance of a GRT object or a GRT class object, from grt.classes" NL},
  {"plugins", ""},
  {NULL, NULL}};

void grt_shell_show_python_help(const char *command) {
  if (!command || !*command)
    grt::GRT::get()->send_output(
      "Help Topics" NL "-----------" NL "grt        General information about the Workbench runtime" NL
      "scripting  Practical information when working on scripts and modules for Workbench" NL
      "wbdata     Summary about Workbench model data organization" NL
      "modules    Information about Workbench module usage" NL
      "plugins    Information about writing Plugins and Modules for Workbench" NL
      "Type '? <topic>' to get help on the topic." NL NL "Custom Python Modules" NL "---------------------" NL
      "grt        Module to work with Workbench runtime (grt) objects" NL
      "   grt.root    The root object in the internal Workbench object hierarchy" NL
      "   grt.modules Location where Workbench modules are available" NL
      "   grt.classes List of classes known to the GRT system" NL
      "mforms     A Module to access the cross-platform UI toolkit used in some Workbench features" NL
      "wb         Utility module for creating Workbench plugins" NL NL
      "Type 'help(<module/object/function>)' to get information about a module, object or function." NL
      "'dir(<object>)' will give a quick list of methods an object has." NL
      "For an introductory tutorial on the Python language, visit http://docs.python.org/tutorial/" NL
      "For general Python and library reference documentation, visit http://python.org/doc/" NL);
  else {
    bool found = false;
    for (int i = 0; help_topics[i].keyword; i++) {
      if (strcmp(command, help_topics[i].keyword) == 0) {
        found = true;
        grt::GRT::get()->send_output(help_topics[i].text);
        grt::GRT::get()->send_output(NL);
        break;
      }
    }
    if (!found)
      grt::GRT::get()->send_output("Unknown help topic\n");
  }
}
