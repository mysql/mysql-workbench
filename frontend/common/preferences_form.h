/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */


#ifndef _PREFERENCES_FORM_H_
#define _PREFERENCES_FORM_H_

#include "workbench/wb_backend_public_interface.h"

#include "mforms/form.h"
#include "mforms/tabview.h"
#include "mforms/box.h"
#include "mforms/button.h"
#include "mforms/checkbox.h"
#include "mforms/treenodeview.h"
#include "mforms/selector.h"
#include "mforms/fs_object_selector.h"

namespace mforms {
  class RadioButton;
}

class MYSQLWBBACKEND_PUBLIC_FUNC PreferencesForm : public mforms::Form 
{
public:
  struct Option
  {
    mforms::View *view;
    boost::function<void ()> show_value;
    boost::function<void ()> update_value;
  };
  
private:
  friend class OptionTable;
  std::list<Option*> _options;

  mforms::Box _top_box;
  mforms::Box _bottom_box;
  mforms::TabView _tabview;

  mforms::Box _button_box;
  mforms::Button _ok_button;
  mforms::Button _cancel_button;
  
  mforms::CheckBox _use_global;

  mforms::Selector _font_preset;
  mforms::TreeNodeView _font_list;
  std::vector<std::string> _font_options;

  workbench_physical_ModelRef _model; // nil unless we're showing model specific options
  wb::WBContextUI *_wbui;

  boost::function<std::string (std::string,std::string)> _edit_font;

  void change_font_option(const std::string &option, const std::string &value);
  void font_preset_changed();
  
  mforms::TextEntry *new_entry_option(const std::string &option, bool numeric);
  mforms::FsObjectSelector *new_path_option(const std::string &option, bool file);  
  mforms::TextEntry *new_numeric_entry_option(const std::string &option, int minrange, int maxrange);
  mforms::CheckBox *new_checkbox_option(const std::string &option);
  mforms::Selector *new_selector_option(const std::string &option, std::string choices_string="", bool numeric=false);

  void ok_clicked();
  void cancel_clicked();

  void code_completion_changed(mforms::CheckBox *cc_box, mforms::Box *subsettings_box);

  void show_values();
  void update_values();

  void show_colors_and_fonts();
  void update_colors_and_fonts();

  void create_admin_page();
  void create_sqlide_page();
  void create_query_page();
  void create_general_page();
  void create_model_page();
  void create_mysql_page();
  void create_diagram_page();
  void create_appearance_page();
#if _WIN32
  void create_color_scheme_page();
#endif

  grt::DictRef get_options(bool global= false);
  
  void toggle_use_global();

  void show_path_option(const std::string &option_name, mforms::FsObjectSelector *entry);
  void update_path_option(const std::string &option_name, mforms::FsObjectSelector *entry);

  void show_entry_option(const std::string &option_name, mforms::TextEntry *entry, bool numeric);
  void update_entry_option(const std::string &option_name, mforms::TextEntry *entry, bool numeric);
  void update_entry_option_numeric(const std::string &option_name, mforms::TextEntry *entry, int minrange, int maxrange);
  
  void show_checkbox_option(const std::string &option_name, mforms::CheckBox *checkbox);
  void update_checkbox_option(const std::string &option_name, mforms::CheckBox *checkbox);
  void show_selector_option(const std::string &option_name, mforms::Selector *selector,
                            const std::vector<std::string> &choices);
  void update_selector_option(const std::string &option_name, mforms::Selector *selector,
                              const std::vector<std::string> &choices, const std::string &default_value, bool as_number);

public:
  PreferencesForm(wb::WBContextUI *wbui, const workbench_physical_ModelRef &model = workbench_physical_ModelRef());
  virtual ~PreferencesForm();
  
  // returned font, title, current font
  void set_font_panel_function(const boost::function<std::string (std::string,std::string)> &edit_font);

  void show();
};


#endif /* _PREFERENCES_FORM_H_ */
