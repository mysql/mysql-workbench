/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "grt_plugin_wizard.h"

#include "base/string_utilities.h"
#include "base/file_functions.h"
#include "workbench/wb_context.h"

#include "mforms/radiobutton.h"
#include "mforms/selector.h"
#include "mforms/table.h"
#include "mforms/textentry.h"
#include "mforms/label.h"
#include "mforms/tabview.h"
#include "mforms/box.h"
#include "workbench/wb_version.h"

#define PYTHON_SCRIPT_TEMPLATE                 \
  "# -*- coding: utf-8 -*-\n"                  \
  "# MySQL Workbench Python script\n"          \
  "# <description>\n"                          \
  "# Written in MySQL Workbench %wbversion%\n" \
  "\n"                                         \
  "import grt\n"                               \
  "#import mforms\n"

#define PYTHON_MODULE_TEMPLATE                 \
  "# -*- coding: utf-8 -*-\n"                  \
  "# MySQL Workbench module\n"                 \
  "# <description>\n"                          \
  "# Written in MySQL Workbench %wbversion%\n" \
  "\n"                                         \
  "from wb import *\n"                         \
  "import grt\n"                               \
  "#import mforms\n"                           \
  "\n\n"                                       \
  "ModuleInfo = DefineModule(%modulename%)\n"  \
  "\n\n"                                       \
  "@ModuleInfo.export()\n"                     \
  "def %functionname%():\n"                    \
  "    pass\n"

using namespace mforms;
NewPluginDialog::NewPluginDialog(Form* owner, const std::string& template_dir)
  : Form(owner),
    _python_script(RadioButton::new_id()),
    _python_module(_python_script.group_id()),
    _python_plugin(_python_script.group_id()),
    _tab(mforms::TabViewTabless) {
  set_title(_("New Script File"));
  set_name("New Plugin Dialog");
  setInternalName("new_plugin_dialog");

  load_plugin_templates(template_dir);

  Box* vbox = manage(new Box(false));

  vbox->set_spacing(12);
  vbox->set_padding(24);

  Label* l = manage(newLabel(_("Select the type of file to create")));
  l->set_style(WizardHeadingStyle);
  vbox->add(l, false, false);

  Box* box = manage(new Box(false));
  vbox->add(box, false, true);

  Table* table = manage(new Table());
  box->add(table, false, true);

  table->set_row_count(4);
  table->set_column_count(2);
  table->set_row_spacing(12);
  table->set_column_spacing(8);
  table->set_padding(12);

  _python_script.set_text(_("Python Script"));
  table->add(&_python_script, 0, 1, 0, 1, HFillFlag);
  table->add(
    manage(newDescr(_("Python Scripts that can be manually executed in the Scripting IDE or from the command line."))),
    1, 2, 0, 1, HExpandFlag | HFillFlag);
  _python_script.set_active(true);
  _python_plugin.set_text(_("Python Plugin"));
  table->add(&_python_plugin, 0, 1, 2, 3, HFillFlag);
  table->add(manage(newDescr(_("Plugins accessible by the user from various places in Workbench:"))), 1, 2, 2, 3,
             HExpandFlag | HFillFlag);
  _python_module.set_text(_("Python Module"));
  table->add(&_python_module, 0, 1, 3, 4, HFillFlag);
  table->add(
    manage(newDescr(
      _("Python modules containing functions exported to be used by other scripts, modules or plugins in Workbench."))),
    1, 2, 3, 4, HExpandFlag | HFillFlag);

  scoped_connect(_python_script.signal_clicked(), std::bind(&NewPluginDialog::changed_type, this));
  scoped_connect(_python_module.signal_clicked(), std::bind(&NewPluginDialog::changed_type, this));
  scoped_connect(_python_plugin.signal_clicked(), std::bind(&NewPluginDialog::changed_type, this));

  for (std::vector<PluginTemplate>::const_iterator tpl = _plugin_template_list.begin();
       tpl != _plugin_template_list.end(); ++tpl)
    _plugin_template.add_item(tpl->name);

  l = manage(newLabel(_("Details")));
  l->set_style(WizardHeadingStyle);
  vbox->add(l, false, false);

  //
  _script_args.set_padding(12);
  _script_args.set_row_count(1);
  _script_args.set_column_count(3);
  _script_args.set_row_spacing(8);
  _script_args.set_column_spacing(8);
  _script_args.add(manage(new Label(_("Script File Name:"))), 0, 1, 0, 1, HFillFlag);
  _script_args.add(&_filename, 1, 2, 0, 1, HExpandFlag | HFillFlag);
  scoped_connect(_filename.signal_changed(), std::bind(&NewPluginDialog::validate, this));
  _script_args.add(
    manage(newDescr(
      _("The default location for scripts is in the scripts folder\nof your Workbench configuration folder."))),
    2, 3, 0, 1, HFillFlag);
  _tab.add_page(&_script_args, "");

  //
  _plugin_args.set_padding(12);
  _plugin_args.set_row_count(3);
  _plugin_args.set_column_count(3);
  _plugin_args.set_row_spacing(8);
  _plugin_args.set_column_spacing(8);
  _plugin_args.add(manage(new Label(_("Template:"))), 0, 1, 0, 1, HFillFlag);
  _plugin_args.add(&_plugin_template, 1, 2, 0, 1, HFillFlag | HExpandFlag);
  _plugin_args.add(manage(newDescr(_("Select the template for the type of plugin you want to write.\nA file may "
                                     "contain as many plugin functions as you want."))),
                   2, 3, 0, 1, HFillFlag);

  _plugin_args.add(manage(new Label(_("Plugin Name:"))), 0, 1, 1, 2, HFillFlag);
  _plugin_args.add(&_plugin_name, 1, 2, 1, 2, HFillFlag | HExpandFlag);
  scoped_connect(_plugin_name.signal_changed(), std::bind(&NewPluginDialog::name_changed, this, &_plugin_name));
  _plugin_args.add(
    manage(newDescr(_("Provide a unique name for the plugin. Use alphanumeric\ncharacters and _ only."))), 2, 3, 1, 2,
    HFillFlag);

  _plugin_args.add(manage(new Label(_("File Name:"))), 0, 1, 2, 3, HFillFlag);
  _plugin_args.add(&_plugin_file, 1, 2, 2, 3, HExpandFlag | HFillFlag);
  scoped_connect(_plugin_file.signal_changed(), std::bind(&NewPluginDialog::validate, this));
  _plugin_args.add(manage(newDescr(_("Name of the plugin file, must be unique and contain only\nalphanumeric chars and "
                                     "_. Specify name only, without a path."))),
                   2, 3, 2, 3, HFillFlag);

  _plugin_name.set_value("myplugin");
  name_changed(&_plugin_name);
  _tab.add_page(&_plugin_args, "");

  //
  _module_args.set_padding(12);
  _module_args.set_row_count(3);
  _module_args.set_column_count(3);
  _module_args.set_row_spacing(8);
  _module_args.set_column_spacing(8);
  _module_args.add(manage(new Label(_("Module Name:"))), 0, 1, 0, 1, HFillFlag);
  _module_args.add(&_module_name, 1, 2, 0, 1, HFillFlag | HExpandFlag);
  _module_args.add(manage(newDescr(_("Provide a name for the module. Use alphanumeric\ncharacters and _ only."))), 2, 3,
                   0, 1, HFillFlag);
  scoped_connect(_module_name.signal_changed(), std::bind(&NewPluginDialog::name_changed, this, &_module_name));
  _module_args.add(manage(new Label(_("Function Name:"))), 0, 1, 1, 2, HFillFlag);
  _module_args.add(&_function_name, 1, 2, 1, 2, HFillFlag | HExpandFlag);
  _module_args.add(
    manage(newDescr(_("Provide a name for the module function. Use alphanumeric\ncharacters and _ only."))), 2, 3, 1, 2,
    HFillFlag);
  scoped_connect(_function_name.signal_changed(), std::bind(&NewPluginDialog::validate, this));

  _module_args.add(manage(new Label(_("File Name:"))), 0, 1, 2, 3, HFillFlag);
  _module_args.add(&_module_file, 1, 2, 2, 3, HExpandFlag | HFillFlag);
  _module_args.add(manage(newDescr(_("Name of the module file, must be unique and contain only\nalphanumeric chars and "
                                     "_. Specify name only, without a path."))),
                   2, 3, 2, 3, HFillFlag);
  scoped_connect(_module_file.signal_changed(), std::bind(&NewPluginDialog::validate, this));

  _module_name.set_value("mymodule");
  name_changed(&_module_name);
  _tab.add_page(&_module_args, "");

  vbox->add(&_tab, true, true);

  _ok.set_text(_("Create"));
  _cancel.set_text(_("Cancel"));

  box = manage(new Box(true));
  box->add(&_error_label, false, true);
  box->set_spacing(12);
  Utilities::add_end_ok_cancel_buttons(box, &_ok, &_cancel);
  vbox->add_end(box, false, true);

  set_content(vbox);
  set_size(-1, 450);
  changed_type();
}
bool NewPluginDialog::advance() {
  if (_python_script.get_active()) {
    std::string value = _filename.get_string_value();

    if (value.empty()) {
      Utilities::show_error(_("Invalid Name"), _("Please fill in a name for the script/module."), "OK");
      return false;
    }

    for (std::string::const_iterator c = value.begin(); c != value.end(); ++c) {
      if (!isalnum(*c) && *c != '_') {
        Utilities::show_error("Invalid Name", _("Please use only alpha-numeric characters and _ for the name."), "OK");

        return false;
      }
    }
  }
  return true;
}
bool NewPluginDialog::run(std::string& filename, std::string& code, bool& is_script, std::string& language) {
  if (!run_modal(&_ok, &_cancel))
    return false;

  if (_python_script.get_active()) {
    filename = _filename.get_string_value();
    code = PYTHON_SCRIPT_TEMPLATE;
    is_script = true;
    language = "python";

    if (!filename.empty() && !g_str_has_suffix(filename.c_str(), ".py"))
      filename.append(".py");
  } else if (_python_module.get_active()) {
    filename = _module_file.get_string_value();
    code = PYTHON_MODULE_TEMPLATE;
    base::replaceStringInplace(code, "%modulename%", _module_name.get_string_value());
    base::replaceStringInplace(code, "%functionname%", _function_name.get_string_value());
    is_script = false;
    language = "python";
  } else if (_python_plugin.get_active()) {
    int index = _plugin_template.get_selected_index();
    filename = _plugin_file.get_string_value();
    if (index < 0)
      index = 0;
    code = _plugin_template_list[index].code;
    base::replaceStringInplace(code, "%modulename%", _plugin_name.get_string_value());
    base::replaceStringInplace(code, "%functionname%", _plugin_name.get_string_value());
    base::replaceStringInplace(code, "%pluginname%", _plugin_name.get_string_value());
    is_script = false;
    language = "python";
  }

  base::replaceStringInplace(code, "%wbversion%",
                             base::strfmt("%i.%i.%i", APP_MAJOR_NUMBER, APP_MINOR_NUMBER, APP_RELEASE_NUMBER));

  return true;
}
void NewPluginDialog::changed_type() {
  if (_python_script.get_active()) {
    _tab.set_active_tab(0);
  } else if (_python_module.get_active()) {
    _tab.set_active_tab(2);
  } else if (_python_plugin.get_active()) {
    _tab.set_active_tab(1);
  }
  validate();
}
bool NewPluginDialog::is_valid_id(const std::string& s) {
  for (std::string::const_iterator i = s.begin(); i != s.end(); ++i) {
    if (!isalnum(*i) && *i != '_')
      return false;
  }
  return !s.empty();
}
bool NewPluginDialog::is_valid_filename(const std::string& file, const std::string& ext) {
  if (!g_str_has_suffix(file.c_str(), ext.c_str()))
    return is_valid_id(file);

  return is_valid_id(file.substr(0, file.size() - ext.size()));
}
void NewPluginDialog::validate() {
  bool ok = true;

  _error_label.set_text("");
  if (_python_script.get_active()) {
    // python scripts must have a filename (otherwise we can't set breakpoints)
    // if (!_filename.get_string_value().empty())
    {
      std::string fn = _filename.get_string_value();
      if (!g_str_has_suffix(fn.c_str(), ".py"))
        fn.append(".py");
      if (!is_valid_filename(fn, ".py")) {
        ok = false;
        _error_label.set_text("Invalid filename");
      }
    }
  } else if (_python_module.get_active()) {
    ok = false;
    if (!is_valid_id(_module_name.get_string_value()))
      _error_label.set_text("Module name is invalid");
    else if (!is_valid_id(_function_name.get_string_value()))
      _error_label.set_text("Function name is invalid");
    else if (!is_valid_filename(_module_file.get_string_value(), "_grt.py")) {
      std::string path = _module_file.get_string_value();
      if (g_str_has_suffix(path.c_str(), "_grt.py"))
        _error_label.set_text("File name must end with _grt.py");
      else
        _error_label.set_text("File name is invalid");
    } else
      ok = true;
  } else if (_python_plugin.get_active()) {
    ok = false;
    if (!is_valid_id(_plugin_name.get_string_value()))
      _error_label.set_text("Plugin name is invalid");
    else if (!is_valid_filename(_plugin_file.get_string_value(), "_grt.py")) {
      std::string path = _plugin_file.get_string_value();
      if (g_str_has_suffix(path.c_str(), "_grt.py"))
        _error_label.set_text("File name must end with _grt.py");
      else
        _error_label.set_text("Plugin file name is invalid");
    } else
      ok = true;
  }
  _ok.set_enabled(ok);
}
void NewPluginDialog::name_changed(TextEntry* entry) {
  std::string name = entry->get_string_value();
  std::string filename;

  if (entry == &_filename) {
    ;
  } else if (entry == &_module_name) {
    char* tmp = g_utf8_strdown(name.data(), (int)name.length());
    filename.append(tmp).append("_grt.py");
    g_free(tmp);
    _module_file.set_value(filename);
  } else if (entry == &_plugin_name) {
    char* tmp = g_utf8_strdown(name.data(), (int)name.length());
    filename.append(name).append("_grt.py");
    g_free(tmp);
    _plugin_file.set_value(filename);
  }

  validate();
}
void NewPluginDialog::load_plugin_templates(const std::string& template_dir) {
  _plugin_template_list.clear();

  GDir* dir = g_dir_open(template_dir.c_str(), 0, NULL);
  if (dir) {
    const gchar* entry;
    while ((entry = g_dir_read_name(dir))) {
      FILE* f = base_fopen((template_dir + "/" + entry).c_str(), "r");
      if (f) {
        char line[1024];

        PluginTemplate t;

        if (fgets(line, sizeof(line), f)) {
          if (strncmp(line, "##", 2) == 0) {
            t.name = g_strstrip(line + 2);

            while (fgets(line, sizeof(line), f)) {
              char* end = line + (strlen(line) - 1);
              while (*end == '\n' || *end == '\r') {
                *end = 0;
                if (end == line)
                  break;
                --end;
              }
              t.code.append(line).append("\n");
            }
          }
        }
        if (!t.name.empty())
          _plugin_template_list.push_back(t);
        fclose(f);
      }
    }
    g_dir_close(dir);
  }

  std::sort(_plugin_template_list.begin(), _plugin_template_list.end());
  PluginTemplate t;
  t.name = "Empty File";
  _plugin_template_list.insert(_plugin_template_list.begin(), t);
}
Label* NewPluginDialog::newLabel(const std::string& text) {
  Label* l = new Label(text);
  l->set_wrap_text(true);
  return l;
}
Label* NewPluginDialog::newDescr(const std::string& text) {
  Label* l = new Label(text);
  l->set_style(SmallHelpTextStyle);
  return l;
}

#define PYTHON_SCRIPT_TEMPLATE                 \
  "# -*- coding: utf-8 -*-\n"                  \
  "# MySQL Workbench Python script\n"          \
  "# <description>\n"                          \
  "# Written in MySQL Workbench %wbversion%\n" \
  "\n"                                         \
  "import grt\n"                               \
  "#import mforms\n"

#define PYTHON_MODULE_TEMPLATE                 \
  "# -*- coding: utf-8 -*-\n"                  \
  "# MySQL Workbench module\n"                 \
  "# <description>\n"                          \
  "# Written in MySQL Workbench %wbversion%\n" \
  "\n"                                         \
  "from wb import *\n"                         \
  "import grt\n"                               \
  "#import mforms\n"                           \
  "\n\n"                                       \
  "ModuleInfo = DefineModule(%modulename%)\n"  \
  "\n\n"                                       \
  "@ModuleInfo.export()\n"                     \
  "def %functionname%():\n"                    \
  "    pass\n"
