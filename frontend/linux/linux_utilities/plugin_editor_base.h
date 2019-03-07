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

//!
//! \addtogroup linuxui Linux UI
//! @{
//!

#ifndef _PLUGIN_EDITOR_BASE_H_
#define _PLUGIN_EDITOR_BASE_H_

#include "gtk_helpers.h"
#include "grt.h"
#include "grtui/gui_plugin_base.h"
#include <gtkmm/frame.h>
#include <gtkmm/notebook.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/textview.h>
#include <gtkmm/entry.h>
#include <gtkmm/paned.h>
#include <gtkmm/builder.h>
#include "base/string_utilities.h"
#include "mforms/view.h"

using base::strfmt;

namespace Gtk {
  class TextView;
}

namespace mforms {
  class CodeEditor;
};

class SqlEditorFE;

class PluginEditorBase : public Gtk::Frame, public GUIPluginBase {
public:
  PluginEditorBase(grt::Module *module, const grt::BaseListRef &args, const char *glade_xml = 0);
  virtual ~PluginEditorBase();

  std::string get_title();

  Gtk::Notebook *editor_notebook() {
    return _editor_notebook;
  }

  void focus_widget_when_idle(Gtk::Widget *w);

  virtual bool switch_edited_object(const grt::BaseListRef &args) {
    return false;
  }

  void load_glade(const char *glade_xml_filename);

  bool is_editing_live_object();
  bool should_close_on_delete_of(const std::string &oid);
  void refresh_form_data();
  void commit_text_changes();
  void close_live_object_editor();

  virtual void show() {
    Gtk::Frame::show();
  }
  virtual void hide() {
    Gtk::Frame::hide();
  }

  virtual bool can_close() {
    return true;
  }

  sigc::signal<void, std::string> signal_title_changed() {
    return _signal_title_changed;
  }

  virtual sigc::connection add_entry_change_timer(Gtk::Entry *entry, const sigc::slot<void, std::string> &setter);
  virtual sigc::connection add_text_change_timer(Gtk::TextView *text, const sigc::slot<void, std::string> &setter);

  // Warning! before using these functions make sure that _xml field was created in ctor by passign xml file name
  // service functions
  template <typename Be, typename Setter>
  inline sigc::connection bind_entry_and_be_setter(const char *glade_entry_name, Be *be, const Setter &setter) {
    Gtk::Entry *entry(0);
    _xml->get_widget(glade_entry_name, entry);
    return entry ? add_entry_change_timer(entry, sigc::mem_fun(be, setter)) : sigc::connection();
  }

  template <typename Be, typename Setter>
  inline sigc::connection bind_text_and_be_setter(const char *glade_text_name, Be *be, const Setter &setter) {
    Gtk::TextView *entry(0);
    _xml->get_widget(glade_text_name, entry);
    return entry ? add_text_change_timer(entry, sigc::mem_fun(be, setter)) : sigc::connection();
  }

  void embed_code_editor(mforms::View *container, Gtk::Box *vbox, bool commit_on_focus_out = true);

protected:
  struct TextChangeTimer {
    sigc::connection conn;
    sigc::slot<bool> commit;
    sigc::slot<void, std::string> setter;
  };

  std::map<Gtk::Widget *, TextChangeTimer> _timers;
  sigc::signal<void, std::string> _signal_title_changed;

  bool _refreshing;

  virtual void add_option_combo_change_handler(Gtk::ComboBox *combo, const std::string &option,
                                               const sigc::slot<void, std::string, std::string> &setter);

  virtual void do_refresh_form_data() {
  }

  virtual bec::BaseEditor *get_be() = 0;

  Glib::RefPtr<Gtk::Builder> xml() const {
    return _xml;
  }
  Gtk::Box *decorator_control() {
    return _live_object_editor_decorator_control;
  }

  Gtk::Notebook *_editor_notebook;
  virtual void decorate_object_editor();

private:
  Glib::RefPtr<Gtk::Builder> _xml;

  Glib::RefPtr<Gtk::Builder> _live_object_editor_decorator_xml;
  Gtk::Box *_live_object_editor_decorator_control;
  Gtk::Container *_live_editor_placeholder;
  Gtk::Widget *_old_embedded_editor, *_old_embedded_find;

  void apply_changes_to_live_object();
  void revert_changes_to_live_object();

  virtual void execute() {
  } // doesn't do anything, just need to implement this from GUIPluginBase

  bool entry_timeout(Gtk::Entry *entry);
  bool text_timeout(Gtk::TextView *text);

  void entry_changed(Gtk::Entry *entry);
  void text_changed(Gtk::TextView *text);

  // TODO: Remove this code
  void combo_changed(Gtk::ComboBox *combo, const std::string &option,
                     const sigc::slot<void, std::string, std::string> &setter);
};

#endif /* _PLUGIN_EDITOR_BASE_H_ */

//!
//! @}
//!
