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
#include "../backend/mysql_schema_editor.h"
#include "grtdb/db_object_helpers.h"
#include "image_cache.h"
#include <gtkmm/image.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/textview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/box.h>
//#include <gtk/gtkversion.h>

extern const char *DEFAULT_CHARSET_CAPTION;
extern const char *DEFAULT_COLLATION_CAPTION;

class SchemaEditor : public PluginEditorBase {
  MySQLSchemaEditorBE *_be;
  std::string _old_name;

  virtual bec::BaseEditor *get_be() {
    return _be;
  }

public:
  virtual ~SchemaEditor() {
    delete _be;
    _be = 0;
  }

  SchemaEditor(grt::Module *m, const grt::BaseListRef &args)
    : PluginEditorBase(m, args, "modules/data/editor_schema.glade"),
      _be(new MySQLSchemaEditorBE(db_mysql_SchemaRef::cast_from(args[0]))) {
    xml()->get_widget("mysql_schema_editor_notebook", _editor_notebook);

    Gtk::Image *image;
    xml()->get_widget("image", image);
    image->set(ImageCache::get_instance()->image_from_filename("db.Schema.editor.48x48.png", false));

    bind_entry_and_be_setter("name_entry", this, &SchemaEditor::set_name);
    if (_be->is_editing_live_object() && _be->get_schema()->oldName() != "") {
      Gtk::Entry *entry;
      xml()->get_widget("name_entry", entry);
      entry->set_sensitive(false);
    }

    Gtk::Button *btn;
    xml()->get_widget("refactor_btn", btn);
    btn->set_sensitive(_be->refactor_possible());
    btn->signal_clicked().connect(sigc::mem_fun(this, &SchemaEditor::refactor_schema));

    Gtk::ComboBox *charset_combo = nullptr;
    xml()->get_widget("charset_combo", charset_combo);
    Glib::RefPtr<Gtk::ListStore> store(
      Glib::RefPtr<Gtk::ListStore>::cast_dynamic(xml()->get_object("collation_store")));
    setup_combo_for_string_list(charset_combo);
    fill_combo_from_string_list(charset_combo, _be->get_charset_list());
    add_option_combo_change_handler(charset_combo, "CHARACTER SET",
                                    sigc::mem_fun(this, &SchemaEditor::set_schema_option_by_name));
    add_option_combo_change_handler(charset_combo, "CHARACTER SET",
                                    sigc::mem_fun(this, &SchemaEditor::charset_combo_changed));

    Gtk::ComboBox *collation_combo = nullptr;
    xml()->get_widget("collation_combo", collation_combo);
    add_option_combo_change_handler(collation_combo, "COLLATE",
                                    sigc::mem_fun(this, &SchemaEditor::set_schema_option_by_name));
    setup_combo_for_string_list(collation_combo);

    set_selected_combo_item(charset_combo, DEFAULT_CHARSET_CAPTION);
    
    
    Gtk::TextView *tview;
    xml()->get_widget("text_view", tview);
    add_text_change_timer(tview, sigc::mem_fun(this, &SchemaEditor::set_comment));

    //! widget->reparent(*this);
    add(*_editor_notebook);
    _editor_notebook->show();

    show_all();

    refresh_form_data();
  }
  
  void charset_combo_changed(const std::string &name, const std::string &value) {
    if (name != "CHARACTER SET")
      return;
    
    Gtk::ComboBox *collation_combo;
    xml()->get_widget("collation_combo", collation_combo);
    
    std::vector<std::string> vec = _be->get_charset_collation_list(value);
    fill_combo_from_string_list(collation_combo, vec);
    
    set_selected_combo_item(collation_combo, DEFAULT_COLLATION_CAPTION);
  }

  void set_name(const std::string &name) {
    if (_be) {
      _be->set_name(name);
      Gtk::Button *btn;
      xml()->get_widget("refactor_btn", btn);
      btn->set_sensitive(_be->refactor_possible());
    }
  }

  void refactor_schema() {
    if (_be) {
      _be->refactor_catalog();
      Gtk::Button *btn;
      xml()->get_widget("refactor_btn", btn);
      btn->set_sensitive(_be->refactor_possible());
    }
  }

  void set_comment(const std::string &text) {
    if (_be)
      _be->set_comment(text);
  }

  void set_schema_option_by_name(const std::string &name, const std::string &value) {
    if (!_be)
      return;
    
    if (name == "CHARACTER SET" and value == DEFAULT_CHARSET_CAPTION)
      _be->set_schema_option_by_name(name, "");
    else if (name == "COLLATE" and value == DEFAULT_COLLATION_CAPTION)
      _be->set_schema_option_by_name(name, "");
    else
      _be->set_schema_option_by_name(name, value);    
  }

  virtual void do_refresh_form_data() {
    Gtk::Entry *entry;
    xml()->get_widget("name_entry", entry);

    Gtk::TextView *tview;
    xml()->get_widget("text_view", tview);

    Gtk::ComboBox *combo;
    xml()->get_widget("charset_combo", combo);

    Gtk::Button *btn;
    xml()->get_widget("refactor_btn", btn);

    if (_be) {
      _old_name = _be->get_name();
      entry->set_text(_old_name);

      tview->get_buffer()->set_text(_be->get_comment());

      bool is_editing_live_obj = is_editing_live_object();
      tview->set_sensitive(!is_editing_live_obj);
      Gtk::Label *tlabel;
      xml()->get_widget("label5", tlabel);
      tlabel->set_sensitive(!is_editing_live_obj);
      btn->set_sensitive(_be->refactor_possible());
    }
  }

  virtual bool switch_edited_object(const grt::BaseListRef &args);
};

//------------------------------------------------------------------------------
bool SchemaEditor::switch_edited_object(const grt::BaseListRef &args) {
  MySQLSchemaEditorBE *old_be = _be;
  _be = new MySQLSchemaEditorBE(db_mysql_SchemaRef::cast_from(args[0]));

  if (_be) {
    do_refresh_form_data();

    delete old_be;
    old_be = 0;
  } else
    _be = old_be;

  return true;
}

extern "C" {
GUIPluginBase *createDbMysqlSchemaEditor(grt::Module *m, const grt::BaseListRef &args) {
  return Gtk::manage(new SchemaEditor(m, args));
}
};
