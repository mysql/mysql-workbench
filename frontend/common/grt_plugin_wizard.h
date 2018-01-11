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

#ifndef __GRT_PLUGIN_WIZARD_H__
#define __GRT_PLUGIN_WIZARD_H__

#include "base/string_utilities.h"
#include "base/file_functions.h"
#include "workbench/wb_context.h"
#include "grt/common.h"

#include <glib/gstdio.h>

#include "mforms/radiobutton.h"
#include "mforms/selector.h"
#include "mforms/table.h"
#include "mforms/form.h"
#include "mforms/label.h"
#include "mforms/textentry.h"
#include "mforms/tabview.h"
#include "mforms/box.h"

using namespace mforms;

class NewPluginDialog : public Form {
  Label *newLabel(const std::string &text);
  Label *newDescr(const std::string &text);

public:
  struct PluginTemplate {
    std::string name;
    std::string code;

    bool operator<(const PluginTemplate &other) const {
      return name < other.name;
    }
  };

  NewPluginDialog(Form *owner, const std::string &template_dir);

  virtual bool advance();
  bool run(std::string &filename, std::string &code, bool &is_script, std::string &language);

private:
  std::vector<PluginTemplate> _plugin_template_list;

  RadioButton _python_script;
  RadioButton _python_module;
  RadioButton _python_plugin;
  Selector _plugin_template;
  Table _script_args;
  TextEntry _filename;

  TabView _tab;

  Table _module_args;
  TextEntry _module_name;
  TextEntry _function_name;
  TextEntry _module_file;

  Table _plugin_args;
  TextEntry _plugin_name;
  TextEntry _plugin_file;
  Label _error_label;
  Button _ok;
  Button _cancel;

  void changed_type();
  inline bool is_valid_id(const std::string &s);
  inline bool is_valid_filename(const std::string &file, const std::string &ext);
  void validate();
  void name_changed(TextEntry *entry);
  void load_plugin_templates(const std::string &template_dir);
};

#endif
