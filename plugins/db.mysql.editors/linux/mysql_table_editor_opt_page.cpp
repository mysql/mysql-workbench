/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql_table_editor_fe.h"
#include "grtdb/db_object_helpers.h"

#include "mysql_table_editor_opt_page.h"
#include <gtkmm/entry.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/togglebutton.h>
//------------------------------------------------------------------------------
DbMySQLTableEditorOptPage::DbMySQLTableEditorOptPage(DbMySQLTableEditor* owner, MySQLTableEditorBE* be,
                                                     Glib::RefPtr<Gtk::Builder> xml)
  : _owner(owner), _be(be), _xml(xml), _refreshing(0) {
  Gtk::Entry* entry(0);
  _xml->get_widget("table_password_entry", entry);
  _owner->add_entry_change_timer(
    entry, sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_table_option), "PASSWORD"));

  _xml->get_widget("auto_increment_entry", entry);
  _owner->add_entry_change_timer(
    entry, sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_table_option), "AUTO_INCREMENT"));

  _xml->get_widget("avg_row_length_entry", entry);
  _owner->add_entry_change_timer(
    entry, sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_table_option), "AVG_ROW_LENGTH"));

  _xml->get_widget("min_rows_entry", entry);
  _owner->add_entry_change_timer(
    entry, sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_table_option), "MIN_ROWS"));

  _xml->get_widget("max_rows_entry", entry);
  _owner->add_entry_change_timer(
    entry, sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_table_option), "MAX_ROWS"));

  _xml->get_widget("data_directory_entry", entry);
  _owner->add_entry_change_timer(
    entry, sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_table_option), "DATA DIRECTORY"));

  _xml->get_widget("index_directory_entry", entry);
  _owner->add_entry_change_timer(
    entry, sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_table_option), "INDEX DIRECTORY"));

  _xml->get_widget("union_tables_entry", entry);
  _owner->add_entry_change_timer(
    entry, sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_table_option), "UNION"));

  Gtk::ToggleButton* toggle(0);
  _xml->get_widget("delay_key_updates_toggle", toggle);
  toggle->signal_toggled().connect(
    sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_toggled_table_option), "DELAY_KEY_WRITE"));

  _xml->get_widget("use_checksum_toggle", toggle);
  toggle->signal_toggled().connect(
    sigc::bind(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_toggled_table_option), "CHECKSUM"));

  std::vector<std::string> list;

  Gtk::ComboBox* combo(0);
  _xml->get_widget("pack_keys_combo", combo);
  list.push_back("Default");
  list.push_back("Pack None");
  list.push_back("Pack All");
  setup_combo_for_string_list(combo);
  fill_combo_from_string_list(combo, list);
  combo->signal_changed().connect(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_pack_keys));

  _xml->get_widget("row_format_combo", combo);
  list.clear();
  list.push_back("Default");
  list.push_back("Dynamic");
  list.push_back("Fixed");
  list.push_back("Compressed");
  list.push_back("Redundant");
  list.push_back("Compact");
  setup_combo_for_string_list(combo);
  fill_combo_from_string_list(combo, list);
  combo->signal_changed().connect(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_row_format));

  _xml->get_widget("key_block_size_combo", combo);
  list.clear();
  list.push_back("1KB");
  list.push_back("2KB");
  list.push_back("4KB");
  list.push_back("8KB");
  list.push_back("16KB");
  setup_combo_for_string_list(combo);
  fill_combo_from_string_list(combo, list);
  combo->signal_changed().connect(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_key_block_size));

  _xml->get_widget("merge_method_combo", combo);
  list.clear();
  list.push_back("Prevent Inserts");
  list.push_back("First Table");
  list.push_back("Last Table");
  setup_combo_for_string_list(combo);
  fill_combo_from_string_list(combo, list);
  combo->signal_changed().connect(sigc::mem_fun(this, &DbMySQLTableEditorOptPage::set_merge_method));
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorOptPage::switch_be(MySQLTableEditorBE* be) {
  _be = be;
  refresh();
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorOptPage::refresh() {
  _refreshing = 1;

  Gtk::Entry* entry(0);
  _xml->get_widget("table_password_entry", entry);
  entry->set_text(_be->get_table_option_by_name("PASSWORD"));

  _xml->get_widget("auto_increment_entry", entry);
  entry->set_text(_be->get_table_option_by_name("AUTO_INCREMENT"));

  _xml->get_widget("avg_row_length_entry", entry);
  entry->set_text(_be->get_table_option_by_name("AVG_ROW_LENGTH"));

  _xml->get_widget("min_rows_entry", entry);
  entry->set_text(_be->get_table_option_by_name("MIN_ROWS"));

  _xml->get_widget("max_rows_entry", entry);
  entry->set_text(_be->get_table_option_by_name("MAX_ROWS"));

  _xml->get_widget("data_directory_entry", entry);
  entry->set_text(_be->get_table_option_by_name("DATA DIRECTORY"));

  _xml->get_widget("index_directory_entry", entry);
  entry->set_text(_be->get_table_option_by_name("INDEX DIRECTORY"));

  _xml->get_widget("union_tables_entry", entry);
  entry->set_text(_be->get_table_option_by_name("UNION"));

  Gtk::ToggleButton* toggle(0);
  _xml->get_widget("use_checksum_toggle", toggle);
  toggle->set_active(_be->get_table_option_by_name("CHECKSUM") != "0");

  _xml->get_widget("delay_key_updates_toggle", toggle);
  toggle->set_active(_be->get_table_option_by_name("DELAY_KEY_WRITE") != "0");

  std::string selected = _be->get_table_option_by_name("PACK_KEYS");
  std::string value;

  if ("DEFAULT" == selected)
    value = "Default";
  else if ("0" == selected)
    value = "Pack None";
  else if ("1" == selected)
    value = "Pack All";

  Gtk::ComboBox* combo(0);
  _xml->get_widget("pack_keys_combo", combo);
  set_selected_combo_item(combo, value);

  value.clear();
  selected = _be->get_table_option_by_name("ROW_FORMAT");

  if (selected == "DEFAULT")
    value = "Default";
  else if (selected == "DYNAMIC")
    value = "Dynamic";
  else if (selected == "FIXED")
    value = "Fixed";
  else if (selected == "COMPRESSED")
    value = "Compressed";
  else if (selected == "REDUNDANT")
    value = "Redundant";
  else if (selected == "COMPACT")
    value = "Compact";

  _xml->get_widget("row_format_combo", combo);
  set_selected_combo_item(combo, value);

  value.clear();
  value = _be->get_table_option_by_name("KEY_BLOCK_SIZE");

  value += "KB";
  _xml->get_widget("key_block_size_combo", combo);
  set_selected_combo_item(combo, value);

  value.clear();
  selected = _be->get_table_option_by_name("INSERT_METHOD");

  if ("NO" == selected)
    value = "Prevent Inserts";
  else if ("FIRST" == selected)
    value = "First Table";
  else if ("LAST" == selected)
    value = "Last Table";

  _xml->get_widget("merge_method_combo", combo);
  set_selected_combo_item(combo, value);

  _refreshing = 0;
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorOptPage::set_table_option(const std::string& value, const char* option) {
  _be->set_table_option_by_name(option, value);
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorOptPage::set_toggled_table_option(const char* option) {
  Gtk::ToggleButton* toggle(0);

  if (*option == 'C') // option -> "CHECKSUM"
    _xml->get_widget("use_checksum_toggle", toggle);
  else if (*option == 'D') // option -> "DELAY_KEY_WRITE"
    _xml->get_widget("delay_key_updates_toggle", toggle);

  _be->set_table_option_by_name(option, toggle->get_active() == 0 ? "0" : "1");
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorOptPage::set_pack_keys() {
  if (_refreshing)
    return;

  Gtk::ComboBox* combo(0);
  _xml->get_widget("pack_keys_combo", combo);
  const std::string selected = get_selected_combo_item(combo);
  std::string value = "DEFAULT";

  if ("Pack None" == selected)
    value = "0";
  else if ("Pack All" == selected)
    value = "1";

  _be->set_table_option_by_name("PACK_KEYS", value);
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorOptPage::set_row_format() {
  if (_refreshing)
    return;

  Gtk::ComboBox* combo(0);
  _xml->get_widget("row_format_combo", combo);
  const std::string selected = get_selected_combo_item(combo);
  std::string value = "DEFAULT";

  if ("Dynamic" == selected)
    value = "DYNAMIC";
  else if ("Fixed" == selected)
    value = "FIXED";
  else if ("Compressed" == selected)
    value = "COMPRESSED";
  else if ("Redundant" == selected)
    value = "REDUNDANT";
  else if ("Compact" == selected)
    value = "COMPACT";

  _be->set_table_option_by_name("ROW_FORMAT", value);
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorOptPage::set_key_block_size() {
  if (_refreshing) {
    return;
  }
  Gtk::ComboBox* combo(0);
  _xml->get_widget("key_block_size_combo", combo);

  std::stringstream ss;
  ss << base::atoi<int>(get_selected_combo_item(combo), 0);

  _be->set_table_option_by_name("KEY_BLOCK_SIZE", ss.str());
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorOptPage::set_merge_method() {
  if (_refreshing)
    return;

  Gtk::ComboBox* combo(0);
  _xml->get_widget("merge_method_combo", combo);
  const std::string selected = get_selected_combo_item(combo);
  std::string value = "NO";

  if ("First Table" == selected)
    value = "FIRST";
  else if ("Last Table" == selected)
    value = "LAST";

  _be->set_table_option_by_name("INSERT_METHOD", value);
}
