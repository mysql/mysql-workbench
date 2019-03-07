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
#include "../backend/wb_editor_storednote.h"
#include "gtk_helpers.h"
#include <gtkmm/textview.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/box.h>
#include <gtkmm/messagedialog.h>

class StoredNoteEditor : public PluginEditorBase {
  StoredNoteEditorBE *_be;
  Glib::RefPtr<Gtk::Builder> _xml;

  virtual bec::BaseEditor *get_be() {
    return _be;
  }

  void apply();
  void discard();

  virtual bool can_close();

public:
  StoredNoteEditor(grt::Module *m, const grt::BaseListRef &args) : PluginEditorBase(m, args), _be(0) {
    set_border_width(8);

    _xml = Gtk::Builder::create_from_file(
      bec::GRTManager::get()->get_data_file_path("modules/data/editor_storednote.glade"));

    Gtk::Box *box(0);
    _xml->get_widget("vbox1", box);
    box->reparent(*this);

    show_all();

    switch_edited_object(args);

    Gtk::Button *btn(0);
    _xml->get_widget("apply", btn);
    btn->signal_clicked().connect(sigc::mem_fun(this, &StoredNoteEditor::apply));

    _xml->get_widget("discard", btn);
    btn->signal_clicked().connect(sigc::mem_fun(this, &StoredNoteEditor::discard));
  }

  virtual ~StoredNoteEditor() {
    delete _be;
  }

  virtual bool switch_edited_object(const grt::BaseListRef &args) {
    Gtk::Box *vbox;
    _xml->get_widget("editor_placeholder", vbox);

    delete _be;
    _be = new StoredNoteEditorBE(GrtStoredNoteRef::cast_from(args[0]));

    embed_code_editor(_be->get_sql_editor()->get_container(), vbox, false);
    _be->load_text();
    return true;
  }
};

//------------------------------------------------------------------------------
void StoredNoteEditor::apply() {
  _be->commit_changes();
}

//------------------------------------------------------------------------------
void StoredNoteEditor::discard() {
  _be->load_text();
}

//------------------------------------------------------------------------------
bool StoredNoteEditor::can_close() {
  if (!_be->can_close()) {
    Gtk::MessageDialog dlg(
      "<b>There are unsaved changes in the editor</b>\nPlease Apply or Revert these changes before closing.", true,
      Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);

    dlg.run();
    return false;
  }

  return true;
}

extern "C" {
GUIPluginBase *createStoredNoteEditor(grt::Module *m, const grt::BaseListRef &args) {
  return Gtk::manage(new StoredNoteEditor(m, args));
}
};
