/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "plugin_editor_base.h"
#include "../backend/mysql_relationship_editor.h"
#include "grtdb/db_object_helpers.h"
#include "treemodel_wrapper.h"
#include <gtkmm/notebook.h>
#include <gtkmm/textview.h>
#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/radiobutton.h>
#include "widgets_auto_cleaner.h"

//==============================================================================
//
//==============================================================================
class DbMySQLRelationshipEditor : public PluginEditorBase //, public WidgetsAutoCleaner
{
  RelationshipEditorBE *_be;

  virtual bec::BaseEditor *get_be();
  void visibility_toggled(const RelationshipEditorBE::VisibilityType visibility);
  void mandatory_toggled(const bool is_left);
  void set_to_many(const bool is_one_to_many);
  void open_editor(const bool is_for_left);

  bool _refreshing;

  virtual ~DbMySQLRelationshipEditor() {
    delete _be;
    _be = 0;
  }

  void set_caption(const std::string &cap) {
    _be->set_caption(cap);
    _signal_title_changed.emit(_be->get_title());
  }

  void set_extra_caption(const std::string &extra_cap) {
    _be->set_extra_caption(extra_cap);
  }

  void set_comment(const std::string &comm) {
    _be->set_comment(comm);
  }

  void identifying_toggled() {
    Gtk::CheckButton *cbtn = 0;
    xml()->get_widget("identifying_cbox", cbtn);

    _be->set_is_identifying(cbtn->get_active());
  }

public:
  DbMySQLRelationshipEditor(grt::Module *m, const grt::BaseListRef &args);

  virtual void do_refresh_form_data();

  virtual bool switch_edited_object(const grt::BaseListRef &args);
};

DbMySQLRelationshipEditor::DbMySQLRelationshipEditor(grt::Module *m, const grt::BaseListRef &args)
  : PluginEditorBase(m, args, "modules/data/editor_relationship.glade"),
    _be(new RelationshipEditorBE(workbench_physical_ConnectionRef::cast_from(args[0]))),
    _refreshing(false) {
  Gtk::Notebook *editor_window(0);
  xml()->get_widget("mysql_relationship_editor", editor_window);

  _be->set_refresh_ui_slot(std::bind(&DbMySQLRelationshipEditor::refresh_form_data, this));

  editor_window->reparent(*this);
  editor_window->show();

  bind_entry_and_be_setter("conn_name", this, &DbMySQLRelationshipEditor::set_caption);
  bind_entry_and_be_setter("conn_extra_name", this, &DbMySQLRelationshipEditor::set_extra_caption);
  bind_text_and_be_setter("conn_comments", this, &DbMySQLRelationshipEditor::set_comment);

  show_all();

  Gtk::RadioButton *rbtn(0);
  xml()->get_widget("fully_visible_rbtn", rbtn);
  rbtn->signal_toggled().connect(
    sigc::bind(sigc::mem_fun(this, &DbMySQLRelationshipEditor::visibility_toggled), RelationshipEditorBE::Visible));

  xml()->get_widget("draw_split_rbtn", rbtn);
  rbtn->signal_toggled().connect(
    sigc::bind(sigc::mem_fun(this, &DbMySQLRelationshipEditor::visibility_toggled), RelationshipEditorBE::Splitted));
  xml()->get_widget("hide_rbtn", rbtn);
  rbtn->signal_toggled().connect(
    sigc::bind(sigc::mem_fun(this, &DbMySQLRelationshipEditor::visibility_toggled), RelationshipEditorBE::Hidden));

  xml()->get_widget("one_to_many_rbtn", rbtn);
  rbtn->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &DbMySQLRelationshipEditor::set_to_many), true));

  xml()->get_widget("one_to_one_rbtn", rbtn);
  rbtn->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &DbMySQLRelationshipEditor::set_to_many), false));

  Gtk::CheckButton *cbtn;
  xml()->get_widget("table1_mandatory_cbox", cbtn);
  cbtn->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &DbMySQLRelationshipEditor::mandatory_toggled), true));

  xml()->get_widget("table2_mandatory_cbox", cbtn);
  cbtn->signal_toggled().connect(sigc::bind(sigc::mem_fun(this, &DbMySQLRelationshipEditor::mandatory_toggled), false));

  xml()->get_widget("identifying_cbox", cbtn);
  cbtn->signal_toggled().connect(sigc::mem_fun(this, &DbMySQLRelationshipEditor::identifying_toggled));

  Gtk::Button *btn(0);
  xml()->get_widget("edit_table1_btn", btn);
  btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &DbMySQLRelationshipEditor::open_editor), true));
  xml()->get_widget("edit_table2_btn", btn);
  btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &DbMySQLRelationshipEditor::open_editor), false));

  try {
    do_refresh_form_data();
  } catch (const Glib::Exception &e) {
    g_message("ERROR %s", e.what().c_str());
  }
}

//------------------------------------------------------------------------------
bool DbMySQLRelationshipEditor::switch_edited_object(const grt::BaseListRef &args) {
  RelationshipEditorBE *old_be = _be;

  _be = new RelationshipEditorBE(workbench_physical_ConnectionRef::cast_from(args[0]));

  _be->set_refresh_ui_slot(std::bind(&DbMySQLRelationshipEditor::refresh_form_data, this));

  _signal_title_changed.emit(_be->get_title());
  do_refresh_form_data();

  delete old_be;

  return true;
}

//------------------------------------------------------------------------------
bec::BaseEditor *DbMySQLRelationshipEditor::get_be() {
  return _be;
}

//------------------------------------------------------------------------------
void DbMySQLRelationshipEditor::visibility_toggled(const RelationshipEditorBE::VisibilityType visibility) {
  if (!_refreshing)
    _be->set_visibility(visibility);
}

//------------------------------------------------------------------------------
void DbMySQLRelationshipEditor::mandatory_toggled(const bool is_left) {
  if (!_refreshing) {
    Gtk::CheckButton *cbtn(0);
    xml()->get_widget(is_left ? "table1_mandatory_cbox" : "table2_mandatory_cbox", cbtn);
    const bool is_active = cbtn->get_active();

    if (is_left)
      _be->set_left_mandatory(is_active);
    else
      _be->set_right_mandatory(is_active);
  }
}

//------------------------------------------------------------------------------
void DbMySQLRelationshipEditor::set_to_many(const bool is_one_to_many) {
  if (!_refreshing)
    _be->set_to_many(is_one_to_many);
}

//------------------------------------------------------------------------------
void DbMySQLRelationshipEditor::open_editor(const bool is_for_left) {
  if (is_for_left)
    _be->open_editor_for_left_table();
  else
    _be->open_editor_for_right_table();
}

//------------------------------------------------------------------------------
void DbMySQLRelationshipEditor::do_refresh_form_data() {
  _refreshing = true;
  Gtk::Entry *entry;
  xml()->get_widget("conn_name", entry);
  entry->set_text(_be->get_caption());

  xml()->get_widget("conn_extra_name", entry);
  entry->set_text(_be->get_extra_caption());

  Gtk::TextView *text;
  xml()->get_widget("conn_comments", text);
  text->get_buffer()->set_text(_be->get_comment());

  Gtk::Label *label;
  xml()->get_widget("long_caption", label);
  label->set_text(_be->get_caption_long());

  xml()->get_widget("long_caption2", label);
  label->set_text(_be->get_extra_caption_long());

  const RelationshipEditorBE::VisibilityType visibility = _be->get_visibility();
  Gtk::RadioButton *rbtn(0);
  switch (visibility) {
    case RelationshipEditorBE::Visible:
      xml()->get_widget("fully_visible_rbtn", rbtn);
      break;
    case RelationshipEditorBE::Splitted:
      xml()->get_widget("draw_split_rbtn", rbtn);
      break;
    case RelationshipEditorBE::Hidden:
      xml()->get_widget("hide_rbtn", rbtn);
      break;
  }
  if (rbtn)
    rbtn->set_active(true);

  // refresh fk tab
  xml()->get_widget("table1_name", label);
  label->set_markup("<b>" + _be->get_left_table_name() + "</b>");
  xml()->get_widget("table1_fktext", label);
  label->set_text(_be->get_left_table_fk());
  xml()->get_widget("table1_columntext", label);
  label->set_text(_be->get_left_table_info());

  xml()->get_widget("table2_name", label);
  label->set_markup("<b>" + _be->get_right_table_name() + "</b>");
  xml()->get_widget("table2_columntext", label);
  label->set_text(_be->get_right_table_info());

  Gtk::CheckButton *cbtn;
  xml()->get_widget("table1_mandatory_cbox", cbtn);
  cbtn->set_active(_be->get_left_mandatory());

  xml()->get_widget("table2_mandatory_cbox", cbtn);
  cbtn->set_active(_be->get_right_mandatory());

  xml()->get_widget("identifying_cbox", cbtn);
  cbtn->set_active(_be->get_is_identifying());

  if (_be->get_to_many())
    xml()->get_widget("one_to_many_rbtn", rbtn);
  else
    xml()->get_widget("one_to_one_rbtn", rbtn);
  rbtn->set_active(true);

  _refreshing = false;
}

//------------------------------------------------------------------------------
extern "C" {
GUIPluginBase *createDbMysqlRelationshipEditor(grt::Module *m, const grt::BaseListRef &args) {
  return Gtk::manage(new DbMySQLRelationshipEditor(m, args));
}
};
