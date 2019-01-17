/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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


#include "base/ui_form.h"
#include "base/string_utilities.h"
#include "base/util_functions.h"
#include "base/log.h"

#include "mforms/widgets.h"
#include "mforms/sectionbox.h"
#include "mforms/textbox.h"
#include "mforms/panel.h"
#include "mforms/radiobutton.h"
#include "mforms/scrollpanel.h"

#include "grt.h"

#include "grts/structs.h"
#include "grts/structs.app.h"
#include "grts/structs.workbench.physical.h"

#include "grt/editor_base.h"

#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"

#include "preferences_form.h"

#include "grtdb/db_helpers.h"
#include "snippet_popover.h"

#include "grtpp_notifications.h"

#if defined(_MSC_VER) || defined(__APPLE__)
#define HAVE_BUNDLED_MYSQLDUMP
#endif

DEFAULT_LOG_DOMAIN("Preferences")

using namespace base;
using namespace mforms;

#include "base/drawing.h"

struct LangFontSet {
  const char *name;

  // TODO: there are a few fonts missing here.
  const char *object_title_font;
  const char *object_section_font;
  const char *object_item_font;
  const char *layer_title_font;
  const char *note_font;
};

static LangFontSet font_sets[] = {{
                                    "Default (Western)", DEFAULT_FONT_FAMILY " Bold 12", DEFAULT_FONT_FAMILY " Bold 11",
                                    DEFAULT_FONT_FAMILY " 11", DEFAULT_FONT_FAMILY " 11", DEFAULT_FONT_FAMILY " 11",
                                  },
#ifdef _MSC_VER
                                  {"Japanese", "Arial Unicode MS Bold 12", "Arial Unicode MS Bold 11",
                                   "Arial Unicode MS 11", "Arial Unicode MS 11", "Arial Unicode MS 11"},
                                  {"Korean", "Arial Unicode MS Bold 12", "Arial Unicode MS Bold 11",
                                   "Arial Unicode MS 11", "Arial Unicode MS 11", "Arial Unicode MS 11"},
                                  {"Simplified Chinese", "Arial Unicode MS Bold 12", "Arial Unicode MS Bold 11",
                                   "Arial Unicode MS 11", "Arial Unicode MS 11", "Arial Unicode MS 11"},
                                  {"Cyrillic", DEFAULT_FONT_FAMILY " Bold 12", DEFAULT_FONT_FAMILY " Bold 11",
                                   DEFAULT_FONT_FAMILY " 11", DEFAULT_FONT_FAMILY " 11", DEFAULT_FONT_FAMILY " 11"},
#elif defined(__APPLE__)
                                  {"Japanese", "Osaka Bold 12", "Osaka Bold 11", "Osaka 11", "Osaka 11", "Osaka 11"},
                                  {"Korean", "AppleGothic Bold 12", "AppleGothic Bold 11", "AppleGothic 11",
                                   "AppleGothic 11", "AppleGothic 11"},
                                  {"Simplified Chinese", "SimHei Bold 12", "SimHei Bold 11", "SimHei 11", "SimHei 11",
                                   "SimHei 11"},
                                  {"Cyrillic", DEFAULT_FONT_FAMILY " Bold 12", DEFAULT_FONT_FAMILY " Bold 11",
                                   DEFAULT_FONT_FAMILY " 11", DEFAULT_FONT_FAMILY " 11", DEFAULT_FONT_FAMILY " 11"},
#else
                                  {"Japanese", "VL Gothic Bold 12", "VL Gothic Bold 11", "VL Gothic 11", "VL Gothic 11",
                                   "VL Gothic 11"},
                                  {"Korean", "WenQuanYi Zen Hei Bold 12", "WenQuanYi Zen Hei Bold 11",
                                   "WenQuanYi Zen Hei 11", "WenQuanYi Zen Hei 11", "WenQuanYi Zen Hei 11"},
                                  {"Simplified Chinese", "WenQuanYi Zen Hei Bold 12", "WenQuanYi Zen Hei Bold 11",
                                   "WenQuanYi Zen Hei 11", "WenQuanYi Zen Hei 11", "WenQuanYi Zen Hei 11"},
                                  {"Cyrillic", DEFAULT_FONT_FAMILY " Bold 12", DEFAULT_FONT_FAMILY " Bold 11",
                                   DEFAULT_FONT_FAMILY " 11", DEFAULT_FONT_FAMILY " 11", DEFAULT_FONT_FAMILY " 11"},
#endif
                                  {NULL, NULL, NULL, NULL, NULL, NULL}};

const std::string VALID_VERSION_TOOLTIP =
  _("Specify default target MySQL version in format MAJOR.MINOR or MAJOR.MINOR.RELEASE");
const std::string INVALID_VERSION_TOOLTIP =
  _("This is not valid version of MySQL.\nSpecify default target MySQL version in format MAJOR.MINOR or "
    "MAJOR.MINOR.RELEASE");

static mforms::Label *new_label(const std::string &text, const std::string &name, bool right_align = false, bool help = false) {
  mforms::Label *label = mforms::manage(new mforms::Label());
  label->set_text(text);
  label->set_name(name);
  if (right_align)
    label->set_text_align(mforms::MiddleRight);
  if (help) {
    label->set_style(mforms::SmallHelpTextStyle);
    label->set_wrap_text(true);
    label->set_size(50, -1);
  }

  return label;
}

// ------------------------------------------------------------------------------------------------

class OptionTable : public mforms::Panel {
  PreferencesForm *_owner;
  mforms::Table _table;
  int _rows;
  bool _help_column;

public:
  OptionTable(PreferencesForm *owner, const std::string &title, bool help_column)
    : mforms::Panel(mforms::TitledBoxPanel), _owner(owner), _rows(0), _help_column(help_column) {
    set_title(title);

    add(&_table);

    _table.set_padding(8);
    _table.set_row_spacing(12);
    _table.set_column_spacing(8);

    _table.set_column_count(_help_column ? 3 : 2);
  }

  void add_option(mforms::View *control, const std::string &caption, const std::string &name, const std::string &help) {
    _table.set_row_count(++_rows);

#ifdef _MSC_VER
    bool right_aligned = false;
#else
    bool right_aligned = true;
#endif

    mforms::Label *label = new_label(caption, name, right_aligned);
    _table.add(label, 0, 1, _rows - 1, _rows, mforms::HFillFlag);
    label->set_size(170, -1);
    _table.add(control, 1, 2, _rows - 1, _rows, mforms::HFillFlag | mforms::HExpandFlag);
    control->set_size(150, -1);
    label = new_label(help, name + " Help", false, true);
    label->set_size(200, -1);
    _table.add(label, 2, 3, _rows - 1, _rows, mforms::VFillFlag | mforms::HFillFlag | mforms::HExpandFlag);
  }

  mforms::TextEntry *add_entry_option(const std::string &option, const std::string &caption,
                                      const std::string &name, const std::string &tooltip) {
    _table.set_row_count(++_rows);

    mforms::TextEntry *entry = _owner->new_entry_option(option, false);
    entry->set_tooltip(tooltip);
    entry->set_size(50, -1);

#ifdef _MSC_VER
    bool right_aligned = false;
#else
    bool right_aligned = true;
#endif

    mforms::Label *label = new_label(caption, name, right_aligned);

    _table.add(label, 0, 1, _rows - 1, _rows, mforms::HFillFlag);
    _table.add(entry, 1, 2, _rows - 1, _rows,
               _help_column ? mforms::HFillFlag : mforms::HFillFlag | mforms::HExpandFlag);
    if (_help_column) {
      label = new_label(tooltip, name + " Help", false, true);
      label->set_size(200, -1);
      _table.add(label, 2, 3, _rows - 1, _rows, mforms::VFillFlag | mforms::HFillFlag | mforms::HExpandFlag);
    }
    return entry;
  }

  mforms::CheckBox *add_checkbox_option(const std::string &option, const std::string &caption,
                                        const std::string &name, const std::string &tooltip) {
    _table.set_row_count(++_rows);

    mforms::CheckBox *cb = _owner->new_checkbox_option(option);
    cb->set_text(caption);
    cb->set_name(name);
    cb->set_tooltip(tooltip);

    _table.add(cb, 0, 3, _rows - 1, _rows, mforms::HFillFlag);

    return cb;
  }
};

// ------------------------------------------------------------------------------------------------

static void force_checkbox_on_toggle(mforms::CheckBox *value, mforms::CheckBox *target, bool same_value,
                                     bool disable_on_active) {
  if (value->get_active()) {
    target->set_active(!same_value);
    target->set_enabled(!disable_on_active);
  } else {
    //    target->set_active(same_value);
    target->set_enabled(disable_on_active);
  }
}

//----------------- PreferencesForm ----------------------------------------------------------------

PreferencesForm::PreferencesForm(const workbench_physical_ModelRef &model)
  : Form(NULL, mforms::FormResizable),
    _switcher(mforms::TreeNoHeader | mforms::TreeSidebar),
    _hbox(true),
    _top_box(false),
    _bottom_box(true),
    _tabview(mforms::TabViewTabless),
    _button_box(true),
    _font_list(mforms::TreeFlatList) {
  _model = model;

  set_name("Workbench Preferences");
  setInternalName("preferences");

  if (!model.is_valid())
    set_title(_("Workbench Preferences"));
  else
    set_title(_("Model Options"));

#ifdef _MSC_VER
  set_back_color(base::Color::getApplicationColorAsString(base::AppColorMainBackground, false));
#endif

  _switcher.add_column(mforms::StringColumnType, "", 150);
  _switcher.end_columns();
  _switcher.signal_changed()->connect(std::bind(&PreferencesForm::switch_page, this));

  _switcher.set_size(150, -1);
  _hbox.add(&_switcher, false, true);

  _top_box.set_padding(12);
  _top_box.set_spacing(8);

  _top_box.add(&_tabview, true, true);

  _top_box.add(&_bottom_box, false, true);

  _bottom_box.add_end(&_button_box, false, true);
  _button_box.set_spacing(8);
  _button_box.set_homogeneous(true);

  scoped_connect(_ok_button.signal_clicked(), std::bind(&PreferencesForm::ok_clicked, this));
  scoped_connect(_cancel_button.signal_clicked(), std::bind(&PreferencesForm::cancel_clicked, this));

  _cancel_button.set_text(_("Cancel"));
  _cancel_button.enable_internal_padding(true);

  _ok_button.set_text(_("OK"));
  _ok_button.enable_internal_padding(true);

  mforms::Utilities::add_end_ok_cancel_buttons(&_button_box, &_ok_button, &_cancel_button);

  if (_model.is_valid()) {
    _use_global.set_text(_("Use defaults from global settings"));
#ifdef _MSC_VER
    if (base::Color::get_active_scheme() == ColorSchemeStandardWin7)
      _use_global.set_front_color("#FFFFFF");
    else
      _use_global.set_front_color("#000000");
#endif
    _bottom_box.add(&_use_global, true, true);
    scoped_connect(_use_global.signal_clicked(), std::bind(&PreferencesForm::toggle_use_global, this));
  }

  mforms::TreeNodeRef node;

  if (!_model.is_valid()) {
    add_page(NULL, _("General Editors"), create_general_editor_page());

    node = add_page(NULL, _("SQL Editor"), create_sqlide_page());
    add_page(node, _("Query Editor"), create_editor_page());
    add_page(node, _("Object Editors"), create_object_editor_page());
    add_page(node, _("SQL Execution"), create_query_page());
    node->expand();

    add_page(NULL, _("Administration"), create_admin_page());
  }

  if (_model.is_valid())
    node = NULL;
  else
    node = add_page(NULL, _("Modeling"), create_model_page());
  add_page(node, _("Defaults"), create_model_defaults_page());
  add_page(node, _("MySQL"), create_mysql_page());
  add_page(node, _("Diagram"), create_diagram_page());
  if (!_model.is_valid()) {
    add_page(node, _("Appearance"), create_appearance_page());
    node->expand();
  }

  if (!_model.is_valid()) {
#ifdef _MSC_VER
    add_page(NULL, _("Fonts & Colors"), create_fonts_and_colors_page());
#else
    // Fonts only for now in Mac/Linux
    add_page(NULL, _("Fonts"), create_fonts_and_colors_page());
#endif
  }
  if (!_model.is_valid()) {
    add_page(NULL, _("SSH"), createSSHPage());

    add_page(NULL, _("Others"), create_others_page());
  }

  _hbox.add(&_top_box, true, true);
  set_content(&_hbox);

  grt::DictRef info(true);
  if (!_model.is_valid())
    info.set("options", wb::WBContextUI::get()->get_wb()->get_wb_options());
  else {
    info.set("model-options", wb::WBContextUI::get()->get_model_options(_model.id()));
    info.set("model", model);
  }
  grt::GRTNotificationCenter::get()->send_grt("GRNPreferencesDidCreate", grt::ObjectRef(), info);

  _switcher.select_node(_switcher.node_at_row(0));

  set_size(800, 600);
  center();

  show_values();
}

//--------------------------------------------------------------------------------------------------

PreferencesForm::~PreferencesForm() {
  for (std::list<Option *>::iterator iter = _options.begin(); iter != _options.end(); ++iter)
    delete *iter;
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef PreferencesForm::add_page(mforms::TreeNodeRef parent, const std::string &title,
                                              mforms::View *view) {
  mforms::TreeNodeRef node = parent ? parent->add_child() : _switcher.add_node();
  node->set_string(0, title);

  mforms::ScrollPanel *scroll = mforms::manage(new mforms::ScrollPanel());
  scroll->set_autohide_scrollers(true);
  scroll->set_visible_scrollers(true, false);
  scroll->add(view);
  _tabview.add_page(scroll, title);

  return node;
}

//--------------------------------------------------------------------------------------------------

bool PreferencesForm::versionIsValid(const std::string &text) {
  size_t dots_count = 0;
  for (size_t i = 0; i < text.size(); i++) {
    if (!(isdigit(text[i]) || text[i] == '.'))
      return false;
    if (text[i] == '.')
      dots_count++;
  }

  if (hasPrefix(text, ".") || hasSuffix(text, ".") || dots_count < 1 || dots_count > 2)
    return false;

  GrtVersionRef version = bec::parse_version(text);
  if (!version.is_valid() || version->majorNumber() < 5 || version->majorNumber() > 10 || version->minorNumber() > 20)
    return false;

  return true;
}

void PreferencesForm::version_changed(mforms::TextEntry *entry) {
  if (versionIsValid(entry->get_string_value())) {
    entry->set_back_color("#FFFFFF");
    entry->set_tooltip(VALID_VERSION_TOOLTIP);
  } else {
    entry->set_back_color("#FF5E5E");
    entry->set_tooltip(INVALID_VERSION_TOOLTIP);
  }
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::switch_page() {
  int row = _switcher.get_selected_row();
  if (row >= 0)
    _tabview.set_active_tab(row);
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::show() {
  grt::DictRef info(true);
  if (!_model.is_valid())
    info.set("options", wb::WBContextUI::get()->get_wb()->get_wb_options());
  else {
    info.set("model-options", wb::WBContextUI::get()->get_model_options(_model.id()));
    info.set("model", _model);
  }
  grt::GRTNotificationCenter::get()->send_grt("GRNPreferencesWillOpen", grt::ObjectRef(), info);

  if (run_modal(&_ok_button, &_cancel_button))
    info.set("saved", grt::IntegerRef(1));
  else
    info.set("saved", grt::IntegerRef(0));

  grt::GRTNotificationCenter::get()->send_grt("GRNPreferencesDidClose", grt::ObjectRef(), info);
}

void PreferencesForm::show_values() {
  for (std::list<Option *>::const_iterator iter = _options.begin(); iter != _options.end(); ++iter)
    (*iter)->show_value();

  if (!_model.is_valid()) {
    show_colors_and_fonts();
  }

  if (_model.is_valid()) {
    std::string value;
    wb::WBContextUI::get()->get_wb_options_value(_model.id(), "useglobal", value);
    if (value == "1") {
      _use_global.set_active(true);
      _tabview.set_enabled(false);
    }
  }
}

void PreferencesForm::update_values() {
  grt::AutoUndo undo(!_model.is_valid());

  if (_model.is_valid()) {
    wb::WBContextUI::get()->set_wb_options_value(_model.id(), "useglobal", _use_global.get_active() ? "1" : "0");
  }

  if (!_model.is_valid() || !_use_global.get_active()) {
    for (std::list<Option *>::const_iterator iter = _options.begin(); iter != _options.end(); ++iter) {
      (*iter)->update_value();
    }
  }

  if (!_model.is_valid())
    updateColorsAndFonts();

  undo.end(_("Change Options"));
}

grt::DictRef PreferencesForm::get_options(bool global) {
  if (!_model.is_valid() || global)
    return wb::WBContextUI::get()->get_wb()->get_wb_options();
  else
    return wb::WBContextUI::get()->get_model_options(_model.id());
}

void PreferencesForm::show_entry_option(const std::string &option_name, mforms::TextEntry *entry, bool numeric) {
  std::string value;

  wb::WBContextUI::get()->get_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, value);
  entry->set_value(value);
}

void PreferencesForm::update_entry_option(const std::string &option_name, mforms::TextEntry *entry, bool numeric) {
  if (numeric)
    wb::WBContextUI::get()->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name,
                                                 entry->get_string_value(), grt::IntegerType);
  else
    wb::WBContextUI::get()->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name,
                                                 entry->get_string_value(), grt::StringType);
}

void PreferencesForm::show_path_option(const std::string &option_name, mforms::FsObjectSelector *entry) {
  std::string value;

  wb::WBContextUI::get()->get_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, value);
  entry->set_filename(value);
}

void PreferencesForm::update_path_option(const std::string &option_name, mforms::FsObjectSelector *entry) {
  wb::WBContextUI::get()->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, entry->get_filename(),
                                               grt::StringType);
}

void PreferencesForm::update_entry_option_numeric(const std::string &option_name, mforms::TextEntry *entry,
                                                  int minrange, int maxrange) {
  long value = base::atoi<long>(entry->get_string_value(), 0l);
  if (value < minrange)
    value = minrange;
  else if (value > maxrange)
    value = maxrange;

  wb::WBContextUI::get()->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name,
                                               strfmt("%li", (long)value));
}

void PreferencesForm::show_checkbox_option(const std::string &option_name, mforms::CheckBox *checkbox) {
  std::string value;

  wb::WBContextUI::get()->get_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, value);

  checkbox->set_active(base::atoi<int>(value, 0) != 0);
}

void PreferencesForm::update_checkbox_option(const std::string &option_name, mforms::CheckBox *checkbox) {
  std::string value = checkbox->get_active() ? "1" : "0";
  wb::WBContextUI::get()->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, value,
                                               grt::IntegerType);

#ifdef _MSC_VER
  // On Windows we have to write the following value also to the registry as our options are not
  // available yet when we need that value.
  if (option_name == "DisableSingleInstance")
    set_value_to_registry(HKEY_CURRENT_USER, "Software\\Oracle\\MySQL Workbench", "DisableSingleInstance",
                          value.c_str());
#endif
}

void PreferencesForm::show_selector_option(const std::string &option_name, mforms::Selector *selector,
                                           const std::vector<std::string> &choices) {
  std::string value;
  wb::WBContextUI::get()->get_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, value);
  selector->set_selected((int)(std::find(choices.begin(), choices.end(), value) - choices.begin()));
}

void PreferencesForm::update_selector_option(const std::string &option_name, mforms::Selector *selector,
                                             const std::vector<std::string> &choices, const std::string &default_value,
                                             bool as_number) {
  if (as_number) {
    if (selector->get_selected_index() < 0)
      wb::WBContextUI::get()->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, default_value,
                                                   grt::IntegerType);
    else
      wb::WBContextUI::get()->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name,
                                                   choices[selector->get_selected_index()], grt::IntegerType);
  } else {
    if (selector->get_selected_index() < 0)
      wb::WBContextUI::get()->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name, default_value);
    else
      wb::WBContextUI::get()->set_wb_options_value(_model.is_valid() ? _model.id() : "", option_name,
                                                   choices[selector->get_selected_index()]);
  }

  if (option_name == "ColorScheme") {
    base::Color::set_active_scheme((base::ColorScheme)selector->get_selected_index());
    NotificationCenter::get()->send("GNColorsChanged", NULL);
  }
}

mforms::TextEntry *PreferencesForm::new_entry_option(const std::string &option_name, bool numeric) {
  Option *option = new Option();
  mforms::TextEntry *entry = new mforms::TextEntry();

  option->view = mforms::manage(entry);
  option->show_value = std::bind(&PreferencesForm::show_entry_option, this, option_name, entry, numeric);
  option->update_value = std::bind(&PreferencesForm::update_entry_option, this, option_name, entry, numeric);
  _options.push_back(option);

  return entry;
}

mforms::FsObjectSelector *PreferencesForm::new_path_option(const std::string &option_name, bool file) {
  Option *option = new Option();
  mforms::FsObjectSelector *entry = new mforms::FsObjectSelector();

  entry->initialize("", file ? mforms::OpenFile : mforms::OpenDirectory, "");

  option->view = mforms::manage(entry);
  option->show_value = std::bind(&PreferencesForm::show_path_option, this, option_name, entry);
  option->update_value = std::bind(&PreferencesForm::update_path_option, this, option_name, entry);
  _options.push_back(option);

  return entry;
}

mforms::TextEntry *PreferencesForm::new_numeric_entry_option(const std::string &option_name, int minrange,
                                                             int maxrange) {
  Option *option = new Option();
  mforms::TextEntry *entry = new mforms::TextEntry();

  option->view = mforms::manage(entry);
  option->show_value = std::bind(&PreferencesForm::show_entry_option, this, option_name, entry, true);
  option->update_value =
    std::bind(&PreferencesForm::update_entry_option_numeric, this, option_name, entry, minrange, maxrange);
  _options.push_back(option);

  return entry;
}

mforms::CheckBox *PreferencesForm::new_checkbox_option(const std::string &option_name) {
  Option *option = new Option();
  mforms::CheckBox *checkbox = new mforms::CheckBox();

  option->view = mforms::manage(checkbox);
  option->show_value = std::bind(&PreferencesForm::show_checkbox_option, this, option_name, checkbox);
  option->update_value = std::bind(&PreferencesForm::update_checkbox_option, this, option_name, checkbox);
  _options.push_back(option);

  return checkbox;
}

mforms::Selector *PreferencesForm::new_selector_option(const std::string &option_name, std::string choices_string,
                                                       bool as_number) {
  Option *option = new Option();
  mforms::Selector *selector = new mforms::Selector();

  if (choices_string.empty())
    wb::WBContextUI::get()->get_wb_options_value(_model.is_valid() ? _model.id() : "", "@" + option_name + "/Items",
                                                 choices_string);

  std::vector<std::string> choices, parts = base::split(choices_string, ",");

  for (std::vector<std::string>::const_iterator iter = parts.begin(); iter != parts.end(); ++iter) {
    std::vector<std::string> tmp = base::split(*iter, ":", 1);
    if (tmp.size() == 1) {
      selector->add_item(*iter);
      choices.push_back(*iter);
    } else {
      selector->add_item(tmp[0]);
      choices.push_back(tmp[1]);
    }
  }

  option->view = mforms::manage(selector);
  option->show_value = std::bind(&PreferencesForm::show_selector_option, this, option_name, selector, choices);
  option->update_value = std::bind(&PreferencesForm::update_selector_option, this, option_name, selector, choices,
                                   choices.empty() ? "" : choices[0], as_number);
  _options.push_back(option);

  return selector;
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::ok_clicked() {
  update_values();

  mforms::Form::show(false);
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::cancel_clicked() {
  mforms::Form::show(false);
}

//--------------------------------------------------------------------------------------------------

mforms::View *PreferencesForm::create_admin_page() {
  mforms::Box *box = mforms::manage(new mforms::Box(false));
  box->set_spacing(8);
  box->set_name("Administration");

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Data Export and Import"));

    mforms::Table *table = mforms::manage(new mforms::Table());

    table->set_padding(8);
    table->set_row_spacing(12);
    table->set_column_spacing(8);

    table->set_row_count(3);
    table->set_column_count(3);

    frame->add(table);

    mforms::FsObjectSelector *pathsel;
    table->add(new_label(_("Path to mysqldump Tool:"), "Path to MySQL Tool", true), 0, 1, 0, 1, mforms::HFillFlag);
    pathsel = new_path_option("mysqldump", true);
    pathsel->get_entry()->set_tooltip(
      _("Specifiy the full path to the mysqldump tool, which is needed for the Workbench Administrator.\nIt usually "
        "comes bundled with the MySQL server and/or client packages."));
    table->add(pathsel, 1, 2, 0, 1, mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
#ifdef HAVE_BUNDLED_MYSQLDUMP
    table->add(new_label(_("Leave blank to use bundled version."), "Leave Blank for Default", false, true), 2, 3, 0, 1,
               mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
#else
    table->add(new_label(_("Full path to the mysqldump tool\nif it's not in your PATH."), "Leave Blank for Default", false, true), 2, 3, 0, 1,
               mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
#endif
    table->add(new_label(_("Path to mysql Tool:"), "Path to MySQL Tool", true), 0, 1, 1, 2, mforms::HFillFlag);
    pathsel = new_path_option("mysqlclient", true);
    pathsel->get_entry()->set_tooltip(
      _("Specifiy the full path to the mysql command line client tool, which is needed for the Workbench "
        "Administrator.\nIt usually comes bundled with the MySQL server and/or client packages."));
    table->add(pathsel, 1, 2, 1, 2, mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
#ifdef HAVE_BUNDLED_MYSQLDUMP
    table->add(new_label(_("Leave blank to use bundled version."), "Leave Blank for Default", false, true), 2, 3, 1, 2,
               mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
#else
    table->add(new_label(_("Full path to the mysql tool\nif it's not in your PATH."), "Leave Blank for Default", false, true), 2, 3, 1, 2,
               mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
#endif

    table->add(new_label(_("Export Directory Path:"), "Export Directory", true), 0, 1, 2, 3, mforms::HFillFlag);
    pathsel = new_path_option("dumpdirectory", false);
    pathsel->get_entry()->set_tooltip(
      _("Specifiy the full path to the directory where dump files should be placed by default."));
    table->add(pathsel, 1, 2, 2, 3, mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
    table->add(new_label(_("Location where dump files should be placed by default."), "Dump Location", false, true), 2, 3, 2, 3,
               mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);

    box->add(frame, false);
  }

  return box;
}

mforms::View *PreferencesForm::create_sqlide_page() {
  // General options for the SQL Editor

  mforms::Box *box = mforms::manage(new mforms::Box(false));
  box->set_spacing(8);
  box->set_name("SQL IDE");

  {
    OptionTable *table = mforms::manage(new OptionTable(this, _("SQL Editor"), true));
    {
      mforms::CheckBox *save_workspace, *discard_unsaved;

      save_workspace =
        table->add_checkbox_option("workbench:SaveSQLWorkspaceOnClose",
                                   _("Save snapshot of open editors on close"), "Save Snapshot on Close",
                                   _("A snapshot of all open scripts is saved when the SQL Editor is closed. Next time "
                                     "it is opened to the same connection that state is restored. Unsaved files will "
                                     "remain unsaved, but their contents will be preserved."));

      {
        static const char *auto_save_intervals =
          "disable:0,5 seconds:5,10 seconds:10,15 seconds:15,30 seconds:30,1 minute:60,5 minutes:300,10 minutes:600,20 "
          "minutes:1200";
        mforms::Selector *sel = new_selector_option("workbench:AutoSaveSQLEditorInterval", auto_save_intervals, true);

        table->add_option(sel, _("Auto-save scripts interval:"), "Auto Save Interval",
                          _("Interval to perform auto-saving of all open script tabs. The scripts will be restored "
                            "from the last auto-saved version if Workbench unexpectedly quits."));
      }

      discard_unsaved = table->add_checkbox_option("DbSqlEditor:DiscardUnsavedQueryTabs",
        _("Create new tabs as Query tabs instead of File"), "Create New Tabs as Query Tabs",
        _("Unsaved Query tabs do not get a close confirmation, unlike File tabs.\nHowever, once saved, such tabs will "
          "also get unsaved change confirmations.\n"
          "If Snapshot saving is enabled, query tabs are always autosaved to temporary files when the connection is "
          "closed."));
      save_workspace->signal_clicked()->connect(
        std::bind(force_checkbox_on_toggle, save_workspace, discard_unsaved, true, true));
      (*save_workspace->signal_clicked())();

      table->add_checkbox_option("DbSqlEditor:SchemaTreeRestoreState",
                                 _("Restore expanded state of the active schema objects"), "Restore Expanded States",
                                 _("Re-expand (and reload) group nodes that were previously expanded in the active "
                                   "schema when the editor was last closed."));
    }
    box->add(table, false, true);
  }

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Sidebar"));
    frame->set_name(_("Side Bar"));
    box->add(frame, false);

    mforms::Box *vbox = mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(8);
    frame->add(vbox);

    {
      mforms::CheckBox *check = new_checkbox_option("DbSqlEditor:ShowSchemaTreeSchemaContents");
      check->set_text(_("Show Schema Contents in Schema Tree"));
      check->set_tooltip(
        _("Whether to show schema contents (tables, views and routine names) in "
          "schema tree."));
      vbox->add(check, false);
    }

    {
      mforms::CheckBox *check = new_checkbox_option("DbSqlEditor:ShowMetadataSchemata");
      check->set_text(_("Show Metadata and Internal Schemas"));
      check->set_tooltip(
        _("Whether to show internal schemas in the schema tree "
          "(eg INFORMATION_SCHEMA, mysql and schemas starting with '.')."));
      vbox->add(check, false);
    }
  }

  {
    OptionTable *otable = new OptionTable(this, _("MySQL Session"), true);
    mforms::TextEntry *entry;

    entry =
      otable->add_entry_option("DbSqlEditor:KeepAliveInterval",
                               _("DBMS connection keep-alive interval (in seconds):"), "Keep Alive Interval",
                               _("Time interval between sending keep-alive messages to DBMS. "
                                 "Set to 0 to not send keep-alive messages."));
    entry->set_size(100, -1);

    entry = otable->add_entry_option("DbSqlEditor:ReadTimeOut",
                                     _("DBMS connection read timeout interval (in seconds):"), "Connection Read Timeout",
                                     _("The maximum amount of time the query can take to return data from the DBMS."
                                       "Set 0 to skip the read timeout."));

    entry = otable->add_entry_option("DbSqlEditor:ConnectionTimeOut",
                                     _("DBMS connection timeout interval (in seconds):"), "Timout Interval",
                                     _("Maximum time to wait before a connection attempt is aborted."));
    box->add(otable, false, true);
  }

  {
    OptionTable *otable = new OptionTable(this, _("Other"), true);

    {
      mforms::TextEntry *entry = new_entry_option("workbench:InternalSchema", false);
      entry->set_max_length(100);
      entry->set_size(100, -1);

      otable->add_option(
        entry, _("Internal Workbench Schema:"), "Internal Schema",
        _("This schema will be used by MySQL Workbench to store information required for certain operations."));
    }

    {
      otable->add_checkbox_option("DbSqlEditor:SafeUpdates",
                                  _("Safe Updates (rejects UPDATEs and DELETEs with no restrictions)"), "Safe Updates",
                                  _("Enables the SQL_SAFE_UPDATES option for the session. "
                                    "If enabled, MySQL aborts UPDATE or DELETE statements "
                                    "that do not use a key in the WHERE clause or a LIMIT clause. "
                                    "This makes it possible to catch UPDATE or DELETE statements "
                                    "where keys are not used properly and that would probably change "
                                    "or delete a large number of rows. "
                                    "Changing this option requires a reconnection (Query -> Reconnect to Server)"));
    }

    box->add(otable, false, true);
  }
  return box;
}

mforms::View *PreferencesForm::create_general_editor_page() {
  mforms::Box *box = mforms::manage(new mforms::Box(false));
  box->set_name("General Editor");
  box->set_spacing(8);

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("SQL Parsing in Code Editors"));
    box->add(frame, false);

    mforms::Box *vbox = mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(8);
    frame->add(vbox);

    {
      mforms::Box *tbox = mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);

      tbox->add(new_label(_("Default SQL_MODE for syntax checker:"), "Default Syntax Checker", true), false, false);
      mforms::TextEntry *entry = new_entry_option("SqlMode", false);
      entry->set_name("SQL Mode Syntax");
      entry->setInternalName("SQL mode syntax");
      entry->set_tooltip(
        _("Value of SQL_MODE DBMS session variable customizes the rules and restrictions for SQL syntax and semantics. "
          "See MySQL Server reference for details.\n"
          "This globally defined parameter determines initial value for same named parameter in each newly created "
          "model. "
          "Model scoped same named parameter in its turn affects SQL parsing within the model, and defines the value "
          "of SQL_MODE session variable when connecting to DBMS.\n"
          "Note: Empty value for this parameter will cause Workbench to treat SQL_MODE as empty string when parsing "
          "SQL within the model, but will leave DBMS session variable at its default value.\n"
          "To force Workbench to reset SQL_MODE session variable as well, this parameter needs to be set to a "
          "whitespace symbol."));
      tbox->add(entry, true, true);
    }

    {
      mforms::CheckBox *check = new_checkbox_option("SqlIdentifiersCS");
      check->set_text(_("SQL Identifiers are Case Sensitive"));
      check->set_name("Case Sensitive Identifiers");
      check->set_tooltip(_("Whether to treat identifiers separately if their names differ only in letter case."));
      vbox->add(check, false);
    }

    {
      mforms::Box *tbox = mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);

      tbox->add(new_label(_("Non-Standard SQL Delimiter:"), "Non Standard Delimiter", true), false, false);
      mforms::TextEntry *entry = new_entry_option("SqlDelimiter", false);
      entry->set_name("SQL Delimiter");
      entry->set_size(50, -1);
      entry->set_tooltip(
        _("Delimiter used for statements that use the semicolon as part of their syntax (e.g. stored routines)"));
      tbox->add(entry, false, false);
    }
  }

  {
    OptionTable *table;

    table = mforms::manage(new OptionTable(this, _("Indentation"), true));
    box->add(table, false, true);
    table->add_checkbox_option("Editor:TabIndentSpaces", _("Tab key inserts spaces instead of tabs"), "Spaces Instead of Tabs",
                               "Check if you want the tab key to indent using the configured amount of spaces.");

    table->add_entry_option("Editor:IndentWidth", "Indent width:", "Indent Width",
                            "How many spaces to insert when indenting with the tab key.");
    table->add_entry_option("Editor:TabWidth", "Tab width:", "Tab Width", "How many spaces wide are tab characters.");
  }
  return box;
}

//--------------------------------------------------------------------------------------------------

mforms::View *PreferencesForm::create_editor_page() {
  mforms::Box *box = mforms::manage(new mforms::Box(false));
  box->set_spacing(8);
  box->set_name("Editor");

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Productivity"));
    box->add(frame, false);

    mforms::Box *vbox = mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(8);
    frame->add(vbox);

    // Code completion settings.
    {
      mforms::Box *subsettings_box = mforms::manage(new mforms::Box(false));
      subsettings_box->set_padding(40, 0, 0, 0);
      subsettings_box->set_spacing(8);
      {
        mforms::CheckBox *check = new_checkbox_option("DbSqlEditor:CodeCompletionEnabled");
        scoped_connect(check->signal_clicked(),
                       std::bind(&PreferencesForm::code_completion_changed, this, check, subsettings_box));

        check->set_text(_("Enable Code Completion in Editors"));
        check->set_tooltip(
          _("If enabled SQL editors display a code completion list when pressing "
            "the defined hotkey"));
        vbox->add(check, false);
      }

      {
        mforms::CheckBox *auto_start_check = new_checkbox_option("DbSqlEditor:AutoStartCodeCompletion");
        auto_start_check->set_text(_("Automatically Start Code Completion"));
        auto_start_check->set_name("Automatically Start Code Completion");
        auto_start_check->set_tooltip(
          _("Available only if code completion is enabled. By activating "
            "this option code completion will be started automatically when you type something and wait "
            "a moment"));
        subsettings_box->add(auto_start_check, false);

        mforms::CheckBox *upper_case_check = new_checkbox_option("DbSqlEditor:CodeCompletionUpperCaseKeywords");
        upper_case_check->set_text(_("Use UPPERCASE keywords on completion"));
        upper_case_check->set_name("Use Upper Case Keywords");
        upper_case_check->set_tooltip(
          _("Normally keywords are shown and inserted as they come from the "
            "code editor configuration file. With this swich they are always upper-cased instead."));
        subsettings_box->add(upper_case_check, false);

        // Set initial enabled state of sub settings depending on whether code completion is enabled.
        std::string value;
        wb::WBContextUI::get()->get_wb_options_value(_model.is_valid() ? _model.id() : "",
                                                     "DbSqlEditor:CodeCompletionEnabled", value);
        subsettings_box->set_enabled(base::atoi<int>(value, 0) != 0);

        vbox->add(subsettings_box, false);
      }
    }

    {
      mforms::Box *tbox = mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);

      tbox->add(new_label(_("Comment type to use for comment shortcut:"), "Comment Type", true), false, false);

      std::string comment_types = "--:--,#:#";
      mforms::Selector *selector = new_selector_option("DbSqlEditor:SQLCommentTypeForHotkey", comment_types, false);
      selector->set_name("Comment Types");
      selector->set_size(150, -1);
      selector->set_tooltip(
        _("Default comment type for SQL Query editor, to be used when the comment shortcut is used."));
      tbox->add(selector, false, false);
    }

    {
      mforms::Table *table = mforms::manage(new mforms::Table());
      table->set_row_count(2);
      table->set_column_count(2);
      table->set_column_spacing(4);
      table->set_row_spacing(4);
      vbox->add(table, false);

      table->add(new_label(_("Max syntax error count:"), "Maximum Error Count", true), 0, 1, 0, 1, mforms::HFillFlag);
      mforms::TextEntry *entry = new_entry_option("SqlEditor::SyntaxCheck::MaxErrCount", false);
      entry->set_size(50, -1);
      entry->set_tooltip(
        _("Maximum number of errors for syntax checking.\n"
          "Syntax errors aren't highlighted beyond this threshold.\n"
          "Set to 0 to show all errors."));
      table->add(entry, 1, 2, 0, 1, mforms::HFillFlag);

      table->add(new_label(_("Max number of result sets:"), "Maximum Result Sets", true), 0, 1, 1, 2, mforms::HFillFlag);
      entry = new_entry_option("DbSqlEditor::MaxResultsets", false);
      entry->set_size(50, -1);
      entry->set_tooltip(_("Maximum number of result sets that can be opened for a single editor."));
      table->add(entry, 1, 2, 1, 2, mforms::HFillFlag);
    }
  }

  {
    OptionTable *table;

    table = mforms::manage(new OptionTable(this, _("SQL Beautifier"), true));
    box->add(table, false, true);
    { table->add_checkbox_option("DbSqlEditor:Reformatter:UpcaseKeywords", _("Change keywords to UPPER CASE"), "Upper Case Keywords", ""); }
  }

  return box;
}

//--------------------------------------------------------------------------------------------------

mforms::View *PreferencesForm::create_object_editor_page() {
  mforms::Box *box = mforms::manage(new mforms::Box(false));
  box->set_spacing(8);
  box->set_name("Object Editor");

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Online DDL"));
    box->add(frame, false);

    mforms::Box *vbox = mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(8);
    frame->add(vbox);

    {
      mforms::Box *line_box = mforms::manage(new mforms::Box(true));
      line_box->set_spacing(4);
      vbox->add(line_box, false);

      mforms::Label *label = new_label(_("Default algorithm for ALTER table:"), "Default Alghorithm for Alter Table", true);
      label->set_size(180, -1);
      line_box->add(label, false, true);

      std::string algorithms = "Default:DEFAULT,In place:INPLACE,Copy:COPY";
      mforms::Selector *selector = new_selector_option("DbSqlEditor:OnlineDDLAlgorithm", algorithms, false);
      selector->set_size(150, -1);
      selector->set_tooltip(
        _("If the currently connected server supports online DDL then use the selected "
          "algorithm as default. This setting can also be adjusted for each alter operation."));
      line_box->add(selector, false, false);
    }
    {
      mforms::Box *line_box = mforms::manage(new mforms::Box(true));
      line_box->set_spacing(4);
      vbox->add(line_box, false);

      mforms::Label *label = new_label(_("Default lock for ALTER table:"), "Default Lock for Alter Table", true);
      label->set_size(180, -1);
      line_box->add(label, false, true);

      std::string locks = "Default:DEFAULT,None:NONE,Shared:SHARED,Exclusive:EXCLUSIVE";
      mforms::Selector *selector = new_selector_option("DbSqlEditor:OnlineDDLLock", locks, false);
      selector->set_size(150, -1);
      selector->set_tooltip(
        _("If the currently connected server supports online DDL then use the selected "
          "lock as default. This setting can also be adjusted for each alter operation."));
      line_box->add(selector, false, false);
    }
  }

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Views"));
    box->add(frame, false);

    mforms::Box *vbox = mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(8);
    frame->add(vbox);

    {
      mforms::CheckBox *check = new_checkbox_option("DbSqlEditor:ReformatViewDDL");
      check->set_text(_("Reformat DDL for Views"));
      check->set_tooltip(
        _("Whether to automatically reformat View DDL returned by the server. The MySQL server does not store the "
          "formatting information for View definitions."));
      vbox->add(check, false);
    }
  }

  return box;
}

//--------------------------------------------------------------------------------------------------

mforms::View *PreferencesForm::create_query_page() {
  // Options specific for the query/script execution aspect of the SQL Editor

  mforms::Box *box = mforms::manage(new mforms::Box(false));
  box->set_spacing(8);
  box->set_name("Query");

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("General"));
    box->add(frame, false);

    mforms::Box *vbox = mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(8);
    frame->add(vbox);

    {
      mforms::Box *tbox = mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);

      tbox->add(new_label(_("Max. query length to store in history (in bytes):"), "Maximum History Length", true), false, false);
      mforms::TextEntry *entry = new_entry_option("DbSqlEditor:MaxQuerySizeToHistory", false);
      entry->set_size(50, -1);
      entry->set_tooltip(
        _("Queries beyond specified size will not be saved in the history when executed.\n"
          "Set to 0 to save any executed query or script"));
      tbox->add(entry, false, false);
    }

    {
      mforms::CheckBox *check = new_checkbox_option("DbSqlEditor:ContinueOnError");
      check->set_text(_("Continue SQL script execution on errors (by default)"));
      check->set_name("Comtinue on Errors");
      check->set_tooltip(_("Whether to continue skipping failed SQL statements when running a script."));
      vbox->add(check, false);
    }

    {
      mforms::CheckBox *check= new_checkbox_option("DbSqlEditor:AutocommitMode");
      check->set_text(_("New connections use auto commit mode"));
      check->set_name("Auto Commit on New Connections");
      check->set_tooltip(_("Sets the default autocommit mode for connections.\nWhen enabled, each statement will be committed immediately."
                           "\nNOTE: all query tabs in the same connection share the same transaction. "
                           "To have independent transactions, you must open a new connection."));
      vbox->add(check, false);
    }

    {
      mforms::Box *tbox = mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);

      tbox->add(new_label(_("Progress status update interval (in milliseconds):"), "Progress Interval", true), false, false);
      mforms::TextEntry *entry = new_entry_option("DbSqlEditor:ProgressStatusUpdateInterval", false);
      entry->set_size(50, -1);
      entry->set_tooltip(_("Time interval between UI updates when running SQL script."));
      tbox->add(entry, false, false);
    }
  }

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("SELECT Query Results"));
    frame->set_name("Select Query Results");
    box->add(frame, false);

    mforms::Box *vbox = mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(8);
    frame->add(vbox);

    {
      mforms::CheckBox *check = new_checkbox_option("SqlEditor:LimitRows");
      check->set_text(_("Limit Rows"));
      check->set_tooltip(
        _("Whether every select query to be implicitly adjusted to limit result set to specified number of rows by "
          "appending the LIMIT keyword to the query.\n"
          "If enabled it's still possible to load entire result set by pressing \"Fetch All\" button."));
      vbox->add(check, false);
    }

    {
      mforms::Box *tbox = mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);

      tbox->add(new_label(_("Limit Rows Count:"), "Limit Rows", true), false, false);
      mforms::TextEntry *entry = new_entry_option("SqlEditor:LimitRowsCount", false);
      entry->set_size(50, -1);
      entry->set_tooltip(
        _("Every select query to be implicitly adjusted to limit result set to specified number of rows."));
      tbox->add(entry, false, false);
    }

    {
      mforms::Box *tbox = mforms::manage(new mforms::Box(true));
      tbox->set_spacing(4);
      vbox->add(tbox, false);

      tbox->add(new_label(_("Max. Field Value Length to Display (in bytes):"), "Maximum Field Length", true), false, false);
      mforms::TextEntry *entry = new_entry_option("Recordset:FieldValueTruncationThreshold", false);
      entry->set_size(50, -1);
      entry->set_tooltip(
        _("Symbols beyond specified threashold will be truncated when showing in the grid. Doesn't affect editing "
          "field values.\n"
          "Set to -1 to disable truncation."));
      tbox->add(entry, false, false);
    }

    {
      mforms::CheckBox *check = new_checkbox_option("DbSqlEditor:MySQL:TreatBinaryAsText");
      check->set_text(_("Treat BINARY/VARBINARY as nonbinary character string"));
      check->set_name("Treat as Non Binary");
      check->set_tooltip(
        _("Whether to treat binary byte strings as nonbinary character strings.\n"
          "Binary byte string values do not appear in results grid and are marked as a BLOB values that are supposed "
          "to be viewed/edited by means of BLOB editor.\n"
          "Nonbinary character string values are shown right in results grid and can be edited with either cell's "
          "in-place editor or BLOB editor.\n"
          "Warning: Since binary byte strings tend to contain zero-bytes in their values, turning this option on may "
          "lead to data truncation when viewing/editing.\n"
          "Note: Application restart is needed to get new option value in affect."));
      vbox->add(check, false);
    }

    {
      mforms::CheckBox *check = new_checkbox_option("DbSqlEditor:IsDataChangesCommitWizardEnabled");
      check->set_text(_("Confirm Data Changes"));
      check->set_tooltip(_("Whether to show a dialog confirming changes to be made to a table recordset."));
      vbox->add(check, false);
    }

    {
      mforms::CheckBox *check = new_checkbox_option("SqlEditor:PreserveRowFilter");
      check->set_text(_("Preserve Row Filter"));
      check->set_tooltip(_("If set, keep row filter active on result set changes. Otherwise the filter is reset."));
      vbox->add(check, false);
    }

    /*{
     mforms::CheckBox *check= new_checkbox_option("DbSqlEditor:IsLiveObjectAlterationWizardEnabled");
     check->set_text(_("Enable Live Object Alteration Wizard"));
     check->set_tooltip(_(
     "Whether to use wizard providing more control over applying changes to live database object."));
     vbox->add(check, false);
     }*/
  }
  return box;
}

//--------------------------------------------------------------------------------------------------

/**
 * Triggered when the user switches the code completion enabled state. We use this to adjust the enabled
 * state for dependent sub options.
 */
void PreferencesForm::code_completion_changed(mforms::CheckBox *cc_box, mforms::Box *subsettings_box) {
  subsettings_box->set_enabled(cc_box->get_active());
}

//--------------------------------------------------------------------------------------------------

mforms::View *PreferencesForm::create_model_page() {
  mforms::Box *top_box = mforms::manage(new mforms::Box(false));
  top_box->set_spacing(8);
  top_box->set_name("Model");

  OptionTable *table;

  table = mforms::manage(new OptionTable(this, _("EER Modeler"), true));
  top_box->add(table, false, true);
  {
    table->add_checkbox_option("workbench.AutoReopenLastModel", _("Automatically reopen previous model at start"), "Reopen Previous Model", "");

#ifndef __APPLE__
    table->add_checkbox_option("workbench:ForceSWRendering",
                               _("Force use of software based rendering for EER diagrams"), "Force Software Redering Diagrams",
                               _("Enable this option if you have drawing problems in Workbench modeling. You must "
                                 "restart Workbench for the option to take effect."));
#endif

    {
      mforms::TextEntry *entry = new_numeric_entry_option("workbench:UndoEntries", 1, 500);
      entry->set_max_length(5);
      entry->set_size(100, -1);

      table->add_option(entry, _("Model undo history size:"), "Undo History Size",
                        _("Allowed values are from 1 up. Note: using high values (> 100) will increase memory usage "
                          "and slow down operation."));
    }

    {
      static const char *auto_save_intervals =
        "disable:0,10 seconds:10,15 seconds:15,30 seconds:30,1 minute:60,5 minutes:300,10 minutes:600,20 minutes:1200";
      mforms::Selector *sel = new_selector_option("workbench:AutoSaveModelInterval", auto_save_intervals, true);

      table->add_option(sel, _("Auto-save model interval:"), "Auto Save Model Interval",
                        _("Interval to perform auto-saving of the open model. The model will be restored from the last "
                          "auto-saved version if Workbench unexpectedly quits."));
    }
  }
  return top_box;
}

mforms::View* PreferencesForm::createSSHPage()
{
    Box* content = manage(new Box(false));
    content->set_spacing(8);
    content->set_name("SSH");

    OptionTable *timeouts_table;

    timeouts_table = mforms::manage(new OptionTable(this, _("Timeouts"), true));
    content->add(timeouts_table, false, true);
    {
      // SSH timeout
      {
        mforms::TextEntry *entry = new_numeric_entry_option("SSH:connectTimeout", 0, 500);
        entry->set_max_length(5);
        entry->set_size(50, -1);
        entry->set_tooltip(_(
          "Determines how long the process waits for connection until timeout"));

        timeouts_table->add_option(entry, _("SSH Connect Timeout:"), "SSH Connect Timeout",
          _("SSH connect timeout in seconds."));
      }

      // SSH readWriteTimeout
      {
        mforms::TextEntry *entry = new_numeric_entry_option("SSH:readWriteTimeout", 0, 500);
        entry->set_max_length(5);
        entry->set_size(50, -1);
        entry->set_tooltip(_(
          "Determines how long the process waits for i/o"));

        timeouts_table->add_option(entry, _("SSH Read Write Timeout:"), "SSH Read and Write Timeout",
          _("SSH Read/Write Timeout in seconds."));
      }

      // SSH commandtimeout
      {
        mforms::TextEntry *entry = new_numeric_entry_option("SSH:commandTimeout", 0, 500);
        entry->set_max_length(5);
        entry->set_size(50, -1);
        entry->set_tooltip(_(
          "Determines how long the process waits for a command output.\nThis is also affected by SSH Command Retry Count."));

        timeouts_table->add_option(entry, _("SSH Command timeout:"), "SSH Command Timeout",
          _("SSH Command Timeout in second."));
      }

      // SSH commandtimeout
      {
        mforms::TextEntry *entry = new_numeric_entry_option("SSH:commandRetryCount", 0, 500);
        entry->set_max_length(5);
        entry->set_size(50, -1);
        entry->set_tooltip(_(
          "Determines how many times we should retry reading command output after specified SSH Command Timeout option."));

        timeouts_table->add_option(entry, _("SSH Command Retry Count:"), "SSH Command Retry Timeout",
          _("SSH Command Retry count."));
      }

      // SSH buffer
      {
        mforms::TextEntry *entry = new_numeric_entry_option("SSH:BufferSize", 0, 10240);
        entry->set_max_length(5);
        entry->set_size(50, -1);
        entry->set_tooltip(_("Buffer size used for tunnel data transfer"));

        timeouts_table->add_option(entry, _("SSH BufferSize:"), "SSH Buffer Size",
          _("SSH buffer size in bytes."));
      }

      // SSH maxFileSize
      {
        mforms::TextEntry *entry = new_numeric_entry_option("SSH:maxFileSize", 0, 1024*ONE_MB);
        entry->set_max_length(10);
        entry->set_size(50, -1);
        entry->set_tooltip(_("Size used to limit transfering of big files"));

        timeouts_table->add_option(entry, _("SSH Maximum File Size:"), "SSH Maximum File Size",
          _("The maximum file that is allowed to be transfered by SSH."));
      }

      // SSH logsize
      {
        mforms::TextEntry *entry = new_numeric_entry_option("SSH:logSize", 0, 1024*ONE_MB);
        entry->set_max_length(10);
        entry->set_size(50, -1);
        entry->set_tooltip(_("Size used to limit transfering of big command output log."));

        timeouts_table->add_option(entry, _("SSH Command Execution log:"), "SSH Command Execution Log",
          _("The maximum log size that is allowed to be transfered by SSH."));
      }

    }

    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));

    mforms::Table *ssh_table= mforms::manage(new mforms::Table());

    ssh_table->set_padding(8);
    ssh_table->set_row_spacing(12);
    ssh_table->set_column_spacing(8);

    ssh_table->set_row_count(2);
    ssh_table->set_column_count(3);
    frame->add(ssh_table);
    {
      mforms::FsObjectSelector *pathsel;
      ssh_table->add(new_label(_("Path to SSH config file:"), "Path to SSH Config File", true), 0, 1, 0, 1, mforms::HFillFlag);
      pathsel= new_path_option("SSH:pathtosshconfig", true);
      pathsel->get_entry()->set_tooltip(_("Specifiy the full path to the SSH config file."));
      ssh_table->add(pathsel, 1, 2, 0, 1, mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
    }

    {
      mforms::FsObjectSelector *pathsel;
      ssh_table->add(new_label(_("Path to SSH known hosts file:"), "Path to SSH Known Hosts File", true), 0, 1, 1, 2, mforms::HFillFlag);
      pathsel= new_path_option("SSH:knownhostsfile", true);
      pathsel->get_entry()->set_tooltip(_("Specifiy the full path to the SSH known hosts file."));
      ssh_table->add(pathsel, 1, 2, 1, 2, mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
    }

    content->add(frame, false);

    return content;
}

mforms::View *PreferencesForm::create_others_page()
{
  Box* content = manage(new Box(false));
  content->set_spacing(8);
  content->set_name("Others");

  {
    mforms::Panel *frame= mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Home Screen"));
    content->add(frame, false, true);

    mforms::Box *vbox= mforms::manage(new mforms::Box(false));
    vbox->set_padding(8);
    vbox->set_spacing(8);
    frame->add(vbox);

    mforms::CheckBox *check= new_checkbox_option("HomeScreen:HeadingMessage");
    check->set_text(_("Show Welcome Message on Connections Screen"));
    check->set_tooltip("");
    vbox->add(check, true);
  }

  OptionTable *timeouts_table;

  timeouts_table = mforms::manage(new OptionTable(this, _("Timeouts"), true));
  content->add(timeouts_table, false, true);
  // migration connection timeout
  {
    mforms::TextEntry *entry = new_numeric_entry_option("Migration:ConnectionTimeOut", 0, 3600);
    entry->set_max_length(5);
    entry->set_size(50, -1);
    entry->set_tooltip(_("The interval in seconds before connection is aborted."));

    timeouts_table->add_option(entry, _("Migration Connection Timeout:"), "Migration Connection Timeout",
      _("Maximum time to wait before a connection is aborted."));
  }

  mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
  mforms::Table *optable = mforms::manage(new mforms::Table());

  optable->set_padding(8);
  optable->set_row_spacing(12);
  optable->set_column_spacing(8);

  optable->set_row_count(2);
  optable->set_column_count(3);
  frame->add(optable);

  {
    optable->add(new_label(_("URL location to display geometry point:"), "Geometry Location", true), 0, 1, 1, 2, mforms::HFillFlag);
    auto opt = new_entry_option("SqlEditor:geographicLocationURL", false);
    opt->set_tooltip("The URL to a geographic services to be used for showing a point on an earth map.\nUse %LAT% and %LON% as a placeholder for Latitude and Longitude.");
    optable->add(opt, 1, 2, 1, 2, mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
  }

  content->add(frame, false);


#ifdef _MSC_VER
  OptionTable *table = mforms::manage(new OptionTable(this, _("Others"), true));
  content->add(table, false, true);
  {
    table->add_checkbox_option("DisableSingleInstance",
      _("Allow more than one instance of MySQL Workbench to run"), "Allow More Then One Instance",
      _("By default only one instance of MySQL Workbench can run at the same time. This is more resource friendly "
        "and necessary as multiple instances share the same files (settings etc.). Change at your own risk."));
  }
#endif

  createLogLevelSelectionPulldown(content);

  return content;
}


void PreferencesForm::createLogLevelSelectionPulldown(mforms::Box *content) {
  OptionTable *logTable = mforms::manage(new OptionTable(this, _("Logs"), true));
  content->add(logTable, false, true);

  // put together comma-separated list of all loglevels (i.e: "none,error,warning,info,debug1,...")
  std::string logLevels;
  {
    logLevels.reserve(10);

    for (std::size_t i = 0; i < base::Logger::logLevelCount; i++)
      logLevels += base::Logger::logLevelName(i) + ',';

    if (logLevels.size() > 0)
      logLevels.resize(logLevels.size() - 1);
  }

  // add dropdown (combo) box
  mforms::Selector *selector = new_selector_option("workbench.logger:LogLevel", logLevels, false);
  selector->set_name("Log Level");
  selector->set_tooltip(
    _("Log level determines how serious a message has to be before it gets logged.  For example, an error is more "
      "serious than a warning, a warning is more serious than an info, etc.  So if log level is set to error, "
      "anything less serious (warning, info, etc) will not be logged.  If log level is set to warning, both warning "
      "and error will still be logged, but info and anything lower will not.  None disables all logging."));
  logTable->add_option(selector, _("Log Level"), "Log Level Information",
                       _("Sets the \"chattyness\" of logs. Choices further down the list "
                       "produce more output than the ones that preceed them."));

  // callback: on user selection, set log level
  selector->signal_changed()->connect([selector]() {
    bool ok = base::Logger::active_level(selector->get_string_value());

    if (ok)
      logInfo("Logger set to level '%s' in preferences menu\n", base::Logger::active_level().c_str());

    assert(ok);
  });
}

mforms::View *PreferencesForm::create_model_defaults_page() {
  mforms::Box *box = mforms::manage(new mforms::Box(false));
  box->set_spacing(8);
  box->set_name("Model Defaults");

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Column Defaults"));

    mforms::Table *table = mforms::manage(new mforms::Table());

    table->set_padding(12);
    table->set_column_spacing(4);
    table->set_row_spacing(8);

    table->set_column_count(4);
    table->set_row_count(2);

    frame->add(table);

    mforms::TextEntry *entry;

    table->add(new_label(_("PK Column Name:"), "Primary Key Column", true), 0, 1, 0, 1, mforms::HFillFlag);
    entry = new_entry_option("PkColumnNameTemplate", false);
    entry->set_tooltip(
      _("Substitutions:\n"
        "%table% - name of the table\n"
        "May be used as %table|upper%, %table|lower%, %table|capitalize%, %table|uncapitalize%"));
    table->add(entry, 1, 2, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);

    table->add(new_label(_("PK Column Type:"), "Primary Key Type", true), 2, 3, 0, 1, mforms::HFillFlag);
    entry = new_entry_option("DefaultPkColumnType", false);
    entry->set_tooltip(
      _("Default type for use in newly added primary key columns.\nSpecify a column type name or a user defined "
        "type.\nFlags such as UNSIGNED are not accepted."));
    table->add(entry, 3, 4, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);

    table->add(new_label(_("Column Name:"), "Column Name", true), 0, 1, 1, 2, mforms::HFillFlag);
    entry = new_entry_option("ColumnNameTemplate", false);
    entry->set_tooltip(
      _("Substitutions:\n"
        "%table% - name of the table"));
    table->add(entry, 1, 2, 1, 2, mforms::HFillFlag | mforms::HExpandFlag);

    table->add(new_label(_("Column Type:"), "Column Type", true), 2, 3, 1, 2, mforms::HFillFlag);
    entry = new_entry_option("DefaultColumnType", false);
    entry->set_tooltip(
      _("Default type for use in newly added columns.\nSpecify a column type name or a user defined type.\nFlags such "
        "as UNSIGNED are not accepted."));
    table->add(entry, 3, 4, 1, 2, mforms::HFillFlag | mforms::HExpandFlag);

    box->add(frame, false);
  }

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Foreign Key/Relationship Defaults"));
    frame->set_name("Foreign Key and Relationship Defaults");

    mforms::Table *table = mforms::manage(new mforms::Table());

    table->set_padding(8);

    frame->add(table);

    table->set_row_spacing(8);
    table->set_column_spacing(8);
    table->set_row_count(3);
    table->set_column_count(4);

    mforms::TextEntry *entry;

    table->add(new_label(_("FK Name:"), "Foreign Key", true), 0, 1, 0, 1, mforms::HFillFlag);
    entry = new_entry_option("FKNameTemplate", false);

#define SUBS_HELP                                                    \
  _("Substitutions:\n"                                               \
    "%table%, %stable% - name of the source table\n"                 \
    "%dtable% - name of the destination table (where FK is added)\n" \
    "%column%, %scolumn% - name of the source column\n"              \
    "%dcolumn% - name of the destination column\n"                   \
    "May be used as %table|upper%, %table|lower%, %table|capitalize% or %table|uncapitalize%")

    entry->set_tooltip(SUBS_HELP);
    table->add(entry, 1, 2, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);

    table->add(new_label(_("Column Name:"), "Column Name", true), 2, 3, 0, 1, mforms::HFillFlag);
    entry = new_entry_option("FKColumnNameTemplate", false);
    entry->set_tooltip(SUBS_HELP);
    table->add(entry, 3, 4, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);

    table->add(new_label(_("ON UPDATE:"), "On Update", true), 0, 1, 1, 2, mforms::HFillFlag);
    table->add(new_selector_option("db.ForeignKey:updateRule"), 1, 2, 1, 2, mforms::HFillFlag | mforms::HExpandFlag);

    table->add(new_label(_("ON DELETE:"), "On Delete", true), 2, 3, 1, 2, mforms::HFillFlag);
    table->add(new_selector_option("db.ForeignKey:deleteRule"), 3, 4, 1, 2, mforms::HFillFlag | mforms::HExpandFlag);

    table->add(new_label(_("Associative Table Name:"), "Associative Table Name", true), 0, 1, 2, 3, mforms::HFillFlag);
    entry = new_entry_option("AuxTableTemplate", false);
    entry->set_tooltip(
      _("Substitutions:\n"
        "%stable% - name of the source table\n"
        "%dtable% - name of the destination table"));
    table->add(entry, 1, 2, 2, 3, mforms::HFillFlag | mforms::HExpandFlag);

    table->add(new_label(_("for n:m relationships"), "For N to M Relationships"), 2, 4, 2, 3, mforms::HFillFlag);

    box->add(frame, false);
  }

  return box;
}

static void show_target_version(const workbench_physical_ModelRef &model, mforms::TextEntry *entry) {
  if (*model->catalog()->version()->releaseNumber() < 0)
    entry->set_value(base::strfmt("%li.%li", (long)*model->catalog()->version()->majorNumber(),
                                  (long)*model->catalog()->version()->minorNumber()));
  else
    entry->set_value(base::strfmt("%li.%li.%li", (long)*model->catalog()->version()->majorNumber(),
                                  (long)*model->catalog()->version()->minorNumber(),
                                  (long)*model->catalog()->version()->releaseNumber()));
}

static void update_target_version(workbench_physical_ModelRef model, mforms::TextEntry *entry) {
  GrtVersionRef version = bec::parse_version(entry->get_string_value());
  model->catalog()->version(version);
  version->owner(model);
}

mforms::View *PreferencesForm::create_mysql_page() {
  mforms::Box *box = mforms::manage(new mforms::Box(false));
  box->set_spacing(8);
  box->set_name("MySQL");

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Model"));

    mforms::Table *table = mforms::manage(new mforms::Table());

    table->set_padding(8);

    frame->add(table);
    table->set_row_count(2);
    table->set_column_count(2);

    if (!_model.is_valid()) {
      table->add(new_label(_("Default Target MySQL Version:"), "Default Target version", true), 0, 1, 0, 1, 0);
      version_entry = new_entry_option("DefaultTargetMySQLVersion", false);
      version_entry->set_tooltip(VALID_VERSION_TOOLTIP);
      version_entry->signal_changed()->connect(std::bind(&PreferencesForm::version_changed, this, version_entry));
      table->add(version_entry, 1, 2, 0, 1, mforms::HExpandFlag | mforms::HFillFlag);
    } else {
      // if editing model options, display the catalog version
      Option *option = new Option();
      mforms::TextEntry *entry = mforms::manage(new mforms::TextEntry());
      entry->signal_changed()->connect(std::bind(&PreferencesForm::version_changed, this, entry));
      entry->set_tooltip(VALID_VERSION_TOOLTIP);

      option->view = mforms::manage(entry);
      option->show_value = std::bind(show_target_version, _model, entry);
      option->update_value = std::bind(update_target_version, _model, entry);
      _options.push_back(option);

      table->add(new_label(_("Target MySQL Version:"), "Target Version", true), 0, 1, 0, 1, 0);
      table->add(entry, 1, 2, 0, 1, mforms::HExpandFlag | mforms::HFillFlag);
    }
    box->add(frame, false);
  }

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Model Table Defaults"));

    mforms::Box *tbox = mforms::manage(new mforms::Box(true));

    tbox->set_padding(8);

    frame->add(tbox);

    tbox->add(new_label(_("Default Storage Engine:"), "Default Storage Engine", true), false, false);
    tbox->add(new_selector_option("db.mysql.Table:tableEngine"), true, true);

    box->add(frame, false);
  }

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Forward Engineering and Synchronization"));

    mforms::Box *tbox = mforms::manage(new mforms::Box(true));
    mforms::TextEntry *entry;
    tbox->set_padding(8);

    frame->add(tbox);
    tbox->add(new_label(_("SQL_MODE to be used in generated scripts:"), "Mode for Generated Scripts", true), false, false);
    tbox->add(entry = new_entry_option("SqlGenerator.Mysql:SQL_MODE", false), true, true);
    entry->set_tooltip(_("The default value of ONLY_FULL_GROUP_BY, STRICT_TRANS_TABLES, NO_ZERO_IN_DATE, NO_ZERO_DATE, ERROR_FOR_DIVISION_BY_ZERO, NO_ENGINE_SUBSTITUTION is recommended."));
    entry->set_name("SQL Mode Scripts");

    box->add(frame, false);
  }

  return box;
}

mforms::View *PreferencesForm::create_diagram_page() {
  mforms::Box *box = mforms::manage(new mforms::Box(false));
  box->set_spacing(8);
  box->set_name("Diagram");

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("All Objects"));

    mforms::Box *vbox = mforms::manage(new mforms::Box(false));

    vbox->set_padding(8);
    vbox->set_spacing(8);

    frame->add(vbox);

    mforms::CheckBox *check;

    check = new_checkbox_option("workbench.physical.ObjectFigure:Expanded");
    check->set_text(_("Expand New Objects"));
    check->set_tooltip(_("Set the initial state of newly created objects to expanded (or collapsed)"));
    vbox->add(check, false);

    check = new_checkbox_option("SynchronizeObjectColors");
    check->set_text(_("Propagate Object Color Changes to All Diagrams"));
    check->set_tooltip(
      _("If an object figure's color is changed, all figures in all diagrams that represent the same object are also "
        "updated"));
    vbox->add(check, false);

    box->add(frame, false);
  }

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Tables"));

    mforms::Box *vbox = mforms::manage(new mforms::Box(false));

    vbox->set_padding(8);
    vbox->set_spacing(8);

    frame->add(vbox);

    mforms::CheckBox *check;

    check = new_checkbox_option("workbench.physical.TableFigure:ShowColumnTypes");
    check->set_text(_("Show Column Types"));
    check->set_tooltip(_("Show the column types along their names in table figures"));
    vbox->add(check, false);

    check = new_checkbox_option("workbench.physical.TableFigure:ShowSchemaName");
    check->set_text(_("Show Schema Name"));
    check->set_tooltip(_("Show owning schema name in the table titlebar figures"));
    vbox->add(check, false);

    {
      mforms::Box *hbox = mforms::manage(new mforms::Box(true));
      mforms::TextEntry *entry = new_entry_option("workbench.physical.TableFigure:MaxColumnTypeLength", true);

      hbox->set_spacing(4);

      // label->set_size(200, -1);
      entry->set_max_length(5);
      entry->set_size(50, -1);

      hbox->add(new_label(_("Max. Length of ENUMs and SETs to Display:"), "Maximum Length of Enumerates and Sets", true), false, false);
      hbox->add(entry, false);

      vbox->add(hbox, false);
    }

    check = new_checkbox_option("workbench.physical.TableFigure:ShowColumnFlags");
    check->set_text(_("Show Column Flags"));
    check->set_tooltip(_("Show column flags such as NOT NULL or UNSIGNED along their names in table figures"));
    vbox->add(check, false);

    {
      mforms::Box *hbox = mforms::manage(new mforms::Box(true));
      mforms::TextEntry *entry = new_entry_option("workbench.physical.TableFigure:MaxColumnsDisplayed", true);
      mforms::Label *descr = mforms::manage(new mforms::Label());

      hbox->set_spacing(4);

      // label->set_size(200, -1);
      entry->set_max_length(5);
      entry->set_size(50, -1);
      descr->set_text(_("Larger tables will be truncated."));
      descr->set_name("Truncate Large tables");
      descr->set_style(mforms::SmallHelpTextStyle);

      hbox->add(new_label(_("Max. Number of Columns to Display:"), "Maximum Columns", true), false, false);
      hbox->add(entry, false);
      hbox->add(descr, true, true);

      vbox->add(hbox, false);
    }

    box->add(frame, false);
  }

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Routines"));

    mforms::Box *hbox = mforms::manage(new mforms::Box(true));

    hbox->set_padding(8);
    hbox->set_spacing(4);

    frame->add(hbox);

    mforms::TextEntry *entry;

    hbox->add(new_label(_("Trim Routine Names Longer Than"), "Trim Routine Names"), false);

    entry = new_entry_option("workbench.physical.RoutineGroupFigure:MaxRoutineNameLength", true);
    entry->set_size(60, -1);
    entry->set_max_length(3);
    hbox->add(entry, false);

    hbox->add(new_label(_("characters"), "Characters"), false);

    box->add(frame, false);
  }

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Relationships/Connections"));
    frame->set_name("Relationships and Connections");

    mforms::Box *vbox = mforms::manage(new mforms::Box(false));

    vbox->set_padding(8);
    vbox->set_spacing(8);

    frame->add(vbox);

    mforms::CheckBox *check;

    check = new_checkbox_option("workbench.physical.Diagram:DrawLineCrossings");
    check->set_text(_("Draw Line Crossings (slow in large diagrams)"));
    check->set_name("Draw Linw Crossings");
    vbox->add(check, false);

    check = new_checkbox_option("workbench.physical.Connection:ShowCaptions");
    check->set_text(_("Show Captions"));
    check->set_name("Show Captions");
    vbox->add(check, false);

    check = new_checkbox_option("workbench.physical.Connection:CenterCaptions");
    check->set_text(_("Center Captions Over Line"));
    check->set_name("Center Captions Over Line");
    vbox->add(check, false);

    box->add(frame, false);
  }

  return box;
}

static void show_text_option(grt::DictRef options, const std::string &option_name, mforms::TextBox *text) {
  text->set_value(options.get_string(option_name));
}

static void update_text_option(grt::DictRef options, const std::string &option_name, mforms::TextBox *text) {
  options.gset(option_name, text->get_string_value());
}

void PreferencesForm::change_font_option(const std::string &option, const std::string &value) {
  std::vector<std::string>::const_iterator it;
  if ((it = std::find(_font_options.begin(), _font_options.end(), option)) != _font_options.end()) {
    int i = (int)(it - _font_options.begin());
    _font_list.node_at_row(i)->set_string(1, value);
  }
}

void PreferencesForm::font_preset_changed() {
  int i = _font_preset.get_selected_index();

  if (i >= 0) {
    wb::WBContextUI::get()->set_wb_options_value(_model.is_valid() ? _model.id() : "",
                                                 "workbench.physical.FontSet:Name", font_sets[i].name);

    change_font_option("workbench.physical.TableFigure:TitleFont", font_sets[i].object_title_font);
    change_font_option("workbench.physical.TableFigure:SectionFont", font_sets[i].object_section_font);
    change_font_option("workbench.physical.TableFigure:ItemsFont", font_sets[i].object_item_font);
    change_font_option("workbench.physical.ViewFigure:TitleFont", font_sets[i].object_title_font);
    change_font_option("workbench.physical.RoutineGroupFigure:TitleFont", font_sets[i].object_title_font);
    change_font_option("workbench.physical.RoutineGroupFigure:ItemsFont", font_sets[i].object_item_font);
    change_font_option("workbench.physical.Connection:CaptionFont", font_sets[i].object_item_font);
    change_font_option("workbench.physical.Layer:TitleFont", font_sets[i].object_item_font);
    change_font_option("workbench.model.NoteFigure:TextFont", font_sets[i].object_item_font);
  }
}

mforms::View *PreferencesForm::create_appearance_page() {
  mforms::Box *box = mforms::manage(new mforms::Box(false));
  box->set_spacing(8);
  box->set_name("Appearance");

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Color Presets"));

    mforms::Table *table = mforms::manage(new mforms::Table());

    table->set_padding(8);
    table->set_row_spacing(4);
    table->set_column_spacing(4);
    table->set_row_count(2);
    table->set_column_count(2);

    frame->add(table);

    mforms::TextBox *text;

    table->add(new_label(_("Colors available when creating tables, views etc"), "Available Colors"), 0, 1, 0, 1, mforms::HFillFlag);
    text = mforms::manage(new mforms::TextBox(mforms::VerticalScrollBar));
    text->set_size(200, 100);
    table->add(text, 0, 1, 1, 2, mforms::FillAndExpand);

    Option *option = new Option();
    _options.push_back(option);
    option->view = text;
    option->show_value = std::bind(show_text_option, get_options(), "workbench.model.ObjectFigure:ColorList", text);
    option->update_value = std::bind(update_text_option, get_options(), "workbench.model.ObjectFigure:ColorList", text);

    table->add(new_label(_("Colors available when creating layers, notes etc"), "Available Colors"), 1, 2, 0, 1, mforms::HFillFlag);
    text = mforms::manage(new mforms::TextBox(mforms::VerticalScrollBar));
    text->set_size(200, 100);
    table->add(text, 1, 2, 1, 2, mforms::FillAndExpand);

    option = new Option();
    _options.push_back(option);
    option->view = text;
    option->show_value = std::bind(&show_text_option, get_options(), "workbench.model.Figure:ColorList", text);
    option->update_value = std::bind(&update_text_option, get_options(), "workbench.model.Figure:ColorList", text);

    box->add(frame, false);
  }

  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Fonts"));

    mforms::Box *content = mforms::manage(new mforms::Box(false));
    content->set_padding(5);
    frame->add(content);

    mforms::Box *hbox = mforms::manage(new mforms::Box(true));
    content->add(hbox, false, true);
    hbox->set_spacing(12);
    hbox->set_padding(12);

    _font_preset.set_name("Font");
    _font_preset.signal_changed()->connect(std::bind(&PreferencesForm::font_preset_changed, this));

    std::string font_name;
    wb::WBContextUI::get()->get_wb_options_value(_model.is_valid() ? _model.id() : "",
                                                 "workbench.physical.FontSet:Name", font_name);

    for (size_t i = 0; font_sets[i].name; i++) {
      // skip font options that are not modeling specific
      if (base::hasPrefix(font_sets[i].name, "workbench.general") ||
          base::hasPrefix(font_sets[i].name, "workbench.scripting"))
        continue;
      _font_preset.add_item(font_sets[i].name);
      if (font_sets[i].name == font_name)
        _font_preset.set_selected((int)i);
    }
    mforms::Label *label = mforms::manage(new mforms::Label("Configure Fonts For:"));
    label->set_name("Configure Fonts");
    hbox->add(label, false, true);
    hbox->add(&_font_preset, true, true);

    _font_list.add_column(mforms::StringColumnType, _("Location"), 150, false);
    _font_list.add_column(mforms::StringColumnType, _("Font"), 150, true);
    _font_list.end_columns();

    content->add(&_font_list, true, true);
    box->add(frame, true, true);
  }

  return box;
}

/**
 * Theming and colors page.
 */
mforms::View *PreferencesForm::create_fonts_and_colors_page() {
  Box *content = manage(new Box(false));
  content->set_spacing(8);
  content->set_name("Fonts and Colors");

  {
    OptionTable *table = new OptionTable(this, _("Fonts"), true);

    table->add_option(new_entry_option("workbench.general.Editor:Font", false), _("SQL Editor:"), "SQL Editor",
                      _("Global font for SQL text editors"));

    table->add_option(new_entry_option("workbench.general.Resultset:Font", false), _("Resultset Grid:"), "Resultset Grid",
                      _("Resultset grid in SQL Editor"));

    table->add_option(new_entry_option("workbench.scripting.ScriptingShell:Font", false), _("Scripting Shell:"), "Scripting Shell",
                      _("Scripting Shell output area"));

    table->add_option(new_entry_option("workbench.scripting.ScriptingEditor:Font", false), _("Script Editor:"), "Script Editor",
                      _("Code editors in scripting shell"));

    content->add(table, true, true);
  }

#ifdef _MSC_VER
  {
    mforms::Panel *frame = mforms::manage(new mforms::Panel(mforms::TitledBoxPanel));
    frame->set_title(_("Color Scheme"));
    mforms::Box *hbox = mforms::manage(new mforms::Box(true));
    hbox->add(new_label(_("Select your scheme:"), "Select Scheme"), false, false);
    mforms::Selector *selector = new_selector_option("ColorScheme", "", true);
    selector->set_size(250, -1);
    hbox->add(selector, true, false);

    mforms::Label *help = new_label(_("The scheme that determines the core colors."), "", false, true);
    hbox->add(help, true, false);
    help->set_size(200, -1);

    frame->add(hbox);
    content->add(frame, false, true);
  }
#endif

  return content;
}

static std::string separate_camel_word(const std::string &word) {
  std::string result;

  for (std::string::const_iterator c = word.begin(); c != word.end(); ++c) {
    if (!result.empty() && *c >= 'A' && *c <= 'Z')
      result.append(" ");
    result.append(1, *c);
  }

  return result;
}

void PreferencesForm::show_colors_and_fonts() {
  std::vector<std::string> options = wb::WBContextUI::get()->get_wb_options_keys("");

  _font_options.clear();
  _font_list.clear();

  for (std::vector<std::string>::const_iterator iter = options.begin(); iter != options.end(); ++iter) {
    if (base::hasPrefix(*iter, "workbench.general") || base::hasPrefix(*iter, "workbench.scripting"))
      continue;

    if (base::hasSuffix(*iter, "Font") && base::hasPrefix(*iter, "workbench.")) {
      std::string::size_type pos = iter->find(':');

      if (pos != std::string::npos) {
        try {
          std::string part = iter->substr(pos + 1);
          std::string figure = base::split(iter->substr(0, pos), ".")[2];
          std::string caption;

          part = part.substr(0, part.length() - 4);

          // substitute some figure names
          figure = base::replaceString(figure, "NoteFigure", "TextFigure");

          caption = separate_camel_word(figure) + " " + part;

          mforms::TreeNodeRef node = _font_list.add_node();
          std::string value;
          wb::WBContextUI::get()->get_wb_options_value("", *iter, value);
          node->set_string(0, caption);
          node->set_string(1, value);

          _font_options.push_back(*iter);
        } catch (...) {
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::updateColorsAndFonts() {
  for (int c = _font_list.count(), i = 0; i < c; i++) {
    std::string value = _font_list.root_node()->get_child(i)->get_string(1);

    wb::WBContextUI::get()->set_wb_options_value("", _font_options[i], value);
  }
}

//--------------------------------------------------------------------------------------------------

void PreferencesForm::toggle_use_global() {
  _tabview.set_enabled(!_use_global.get_active());
}

//--------------------------------------------------------------------------------------------------

static struct RegisterNotifDocs_preferences_form {
  RegisterNotifDocs_preferences_form() {
    base::NotificationCenter::get()->register_notification(
      "GRNPreferencesDidCreate", "preferences", "Sent when the Preferences window is created.", "",
      "options - the options dictionary being edited\n"
      "or\n"
      "model-options - the model specific options dictionary being changed\n"
      "model-id - the object id of the model for which the options are being changed");

    base::NotificationCenter::get()->register_notification(
      "GRNPreferencesWillOpen", "preferences", "Sent when Preferences window is about to be shown on screen.", "",
      "options - the options dictionary being edited\n"
      "or\n"
      "model-options - the model specific options dictionary being changed\n"
      "model-id - the object id of the model for which the options are being changed");

    base::NotificationCenter::get()->register_notification(
      "GRNPreferencesDidClose", "preferences", "Sent after Preferences window was closed.", "",
      "saved - 1 if the user chose to save the options changed or 0 if changes were cancelled\n"
      "options - the options dictionary being edited\n"
      "or\n"
      "model-options - the model specific options dictionary being changed\n"
      "model-id - the object id of the model for which the options are being changed\n");
  }
} initdocs_preferences_form;
