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

#include "plugin_editor_base.h"
#include "grt/editor_base.h"
#include "base/string_utilities.h"
#include <gtkmm/combobox.h>
#include "mforms/code_editor.h"
#include "gtk/mforms_gtk.h"

using base::strfmt;

#define TEXT_UPDATE_TIMER 500

//------------------------------------------------------------------------------
PluginEditorBase::PluginEditorBase(grt::Module *module, const grt::BaseListRef &args, const char *glade_file)
  : GUIPluginBase(module),
    _editor_notebook(0),
    _xml(0),
    _live_object_editor_decorator_xml(0),
    _live_object_editor_decorator_control(0),
    _live_editor_placeholder(0),
    _old_embedded_editor(0),
    _old_embedded_find(0) {
  _refreshing = false;
  set_shadow_type(Gtk::SHADOW_NONE);

  if (glade_file)
    _xml = Gtk::Builder::create_from_file(bec::GRTManager::get()->get_data_file_path(glade_file));
}

//------------------------------------------------------------------------------
PluginEditorBase::~PluginEditorBase() {
}

//------------------------------------------------------------------------------

std::string PluginEditorBase::get_title() {
  return get_be()->get_title();
}

//------------------------------------------------------------------------------
void PluginEditorBase::load_glade(const char *glade_xml_filename) {
  if (_xml)
    throw std::logic_error("XML already created"); // TODO: better message here

  if (glade_xml_filename) {
    _xml = Gtk::Builder::create_from_file(bec::GRTManager::get()->get_data_file_path(glade_xml_filename));
    if (!_xml)
      throw std::logic_error("Can't load glade xml");
  }
}

//------------------------------------------------------------------------------
void PluginEditorBase::refresh_form_data() {
  if (_refreshing)
    return;

  _refreshing = true;

  decorate_object_editor();

  do_refresh_form_data();

  _refreshing = false;
}

//------------------------------------------------------------------------------
bool PluginEditorBase::is_editing_live_object() {
  return get_be()->is_editing_live_object();
}
//------------------------------------------------------------------------------
void PluginEditorBase::decorate_object_editor() {
  if (!_editor_notebook)
    return;

  if (is_editing_live_object()) {
    if (!_live_object_editor_decorator_control) {
      _live_object_editor_decorator_xml = Gtk::Builder::create_from_file(
        bec::GRTManager::get()->get_data_file_path("modules/data/live_editor_decoration.glade"));
      _live_object_editor_decorator_xml->get_widget("box1", _live_object_editor_decorator_control);
      _live_object_editor_decorator_xml->get_widget("live_editor_placeholder", _live_editor_placeholder);

      Gtk::Button *apply_live_edtior_button = 0;
      _live_object_editor_decorator_xml->get_widget("apply_live_edtior_button", apply_live_edtior_button);
      apply_live_edtior_button->signal_pressed().connect(
        sigc::mem_fun(this, &PluginEditorBase::apply_changes_to_live_object));
      Gtk::Button *revert_live_edtior_button = 0;
      _live_object_editor_decorator_xml->get_widget("revert_live_edtior_button", revert_live_edtior_button);
      revert_live_edtior_button->signal_pressed().connect(
        sigc::mem_fun(this, &PluginEditorBase::revert_changes_to_live_object));
      //     Gtk::Button *close_live_edtior_button= 0;
      //     _live_object_editor_decorator_xml->get_widget("close_live_edtior_button", close_live_edtior_button);
      //     close_live_edtior_button->signal_pressed().connect(sigc::mem_fun(this,
      //     &PluginEditorBase::close_live_object_editor));
    }

    if (_editor_notebook->get_parent() != _live_object_editor_decorator_control) {
      _editor_notebook->reparent(*_live_editor_placeholder);
      _live_object_editor_decorator_control->reparent(*this);
      _live_object_editor_decorator_control->show();
    }
  } else {
    if (_editor_notebook->get_parent() != this) {
      if (_editor_notebook->get_parent() == _live_object_editor_decorator_control)
        _live_object_editor_decorator_control->unparent();
      _editor_notebook->reparent(*this);
    }
  }
}
//------------------------------------------------------------------------------
void PluginEditorBase::apply_changes_to_live_object() {
  // make sure changes to a treeview are committed
  Gtk::Widget *focus = dynamic_cast<Gtk::Window *>(_editor_notebook->get_toplevel())->get_focus();
  if (focus && dynamic_cast<Gtk::Entry *>(focus)) {
    // if this is an entry, then it could be a tree cell being edited.. check it's parent
    if (dynamic_cast<Gtk::TreeView *>(focus->get_parent()))
      focus->activate();
  }

  bec::BaseEditor *editor = get_be();
  editor->apply_changes_to_live_object();
}
//------------------------------------------------------------------------------
void PluginEditorBase::revert_changes_to_live_object() {
  get_be()->revert_changes_to_live_object();
}
//------------------------------------------------------------------------------
void PluginEditorBase::close_live_object_editor() {
  if (get_be()->can_close()) {
    Gtk::Notebook *notebook = dynamic_cast<Gtk::Notebook *>(get_parent());
    if (notebook) {
      hide();
      notebook->remove_page(*this);
      bool visible = false;
      for (int c = notebook->get_n_pages(), i = 0; i < c; i++) {
        if (notebook->get_nth_page(i)->is_visible()) {
          visible = true;
          break;
        }
      }
      if (!visible)
        notebook->hide();
    } else {
      delete get_toplevel();
    }
  }
}
//------------------------------------------------------------------------------

void PluginEditorBase::add_option_combo_change_handler(Gtk::ComboBox *combo, const std::string &option,
                                                       const sigc::slot<void, std::string, std::string> &setter) {
  combo->signal_changed().connect(
    sigc::bind(sigc::mem_fun(this, &PluginEditorBase::combo_changed), combo, option, setter));
}

//------------------------------------------------------------------------------
sigc::connection PluginEditorBase::add_entry_change_timer(Gtk::Entry *entry,
                                                          const sigc::slot<void, std::string> &setter) {
  TextChangeTimer timer;

  timer.commit = sigc::bind(sigc::mem_fun(this, &PluginEditorBase::entry_timeout), entry);
  timer.setter = setter;
  _timers[entry] = timer;

  return entry->signal_changed().connect(sigc::bind(sigc::mem_fun(this, &PluginEditorBase::entry_changed), entry));
}

//------------------------------------------------------------------------------
sigc::connection PluginEditorBase::add_text_change_timer(Gtk::TextView *text,
                                                         const sigc::slot<void, std::string> &setter) {
  TextChangeTimer timer;

  timer.commit = sigc::bind(sigc::mem_fun(this, &PluginEditorBase::text_timeout), text);
  timer.setter = setter;
  _timers[text] = timer;

  return text->get_buffer()->signal_changed().connect(
    sigc::bind(sigc::mem_fun(this, &PluginEditorBase::text_changed), text));
}

//------------------------------------------------------------------------------
bool PluginEditorBase::entry_timeout(Gtk::Entry *entry) {
  _timers[entry].setter(entry->get_text());
  return false;
}

//------------------------------------------------------------------------------
bool PluginEditorBase::text_timeout(Gtk::TextView *text) {
  _timers[text].setter(text->get_buffer()->get_text());
  return false;
}

//------------------------------------------------------------------------------
void PluginEditorBase::entry_changed(Gtk::Entry *entry) {
  if (!_refreshing) {
    if (_timers[entry].conn)
      _timers[entry].conn.disconnect();

    _timers[entry].conn = Glib::signal_timeout().connect(_timers[entry].commit, TEXT_UPDATE_TIMER);
  }
}

//------------------------------------------------------------------------------
void PluginEditorBase::text_changed(Gtk::TextView *text) {
  if (!_refreshing) {
    if (_timers[text].conn)
      _timers[text].conn.disconnect();

    _timers[text].conn = Glib::signal_timeout().connect(_timers[text].commit, TEXT_UPDATE_TIMER);
  }
}

//------------------------------------------------------------------------------
void PluginEditorBase::commit_text_changes() {
  for (std::map<Gtk::Widget *, TextChangeTimer>::iterator iter = _timers.begin(); iter != _timers.end(); ++iter) {
    if (iter->second.conn) {
      iter->second.commit();
      iter->second.conn.disconnect();
    }
  }
}

//------------------------------------------------------------------------------
void PluginEditorBase::combo_changed(Gtk::ComboBox *combo, const std::string &option,
                                     const sigc::slot<void, std::string, std::string> &setter) {
  if (!_refreshing) {
    Gtk::TreeModel::iterator iter = combo->get_active();
    if (iter) {
      Gtk::TreeRow row = *iter;
      Glib::ustring text;
      row.get_value(0, text);
      setter(option, text);
    }
  }
}

bool PluginEditorBase::should_close_on_delete_of(const std::string &oid) {
  return get_be()->should_close_on_delete_of(oid);
}

//------------------------------------------------------------------------------
void PluginEditorBase::embed_code_editor(mforms::View *container, Gtk::Box *vbox, bool commit_on_focus_out) {
  if (_old_embedded_editor)
    vbox->remove(*_old_embedded_editor);
  if (_old_embedded_find)
    vbox->remove(*_old_embedded_find);
  _old_embedded_find = 0;
  _old_embedded_editor = 0;

  if (container) {
    Gtk::Widget *editorw = mforms::widget_for_view(container);
    _old_embedded_editor = editorw;

    editorw->set_size_request(-1, 100);
    vbox->pack_end(*editorw, true, true);
    vbox->resize_children();

    if (commit_on_focus_out)
      editorw->signal_focus_out_event().connect(
        sigc::bind_return(sigc::hide(sigc::mem_fun(get_be(), &bec::BaseEditor::commit_changes)), false));
  }
}

void PluginEditorBase::focus_widget_when_idle(Gtk::Widget *w) {
  Glib::signal_idle().connect(sigc::bind_return(sigc::mem_fun(w, &Gtk::Widget::grab_focus), false));
}
