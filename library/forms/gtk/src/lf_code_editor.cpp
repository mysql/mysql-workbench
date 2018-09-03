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

#include "../lf_mforms.h"

#include "base/string_utilities.h"
#include "../lf_code_editor.h"
#include <memory>
#include <ctype.h>
#include <stdlib.h>
#include "base/wb_memory.h"
#include "base/wb_iterators.h"

#include "Scintilla.h"
#include "SciLexer.h"
#define PLAT_GTK 2
#define GTK
#include "ScintillaWidget.h"

#ifndef g_signal_handlers_disconnect_by_data
#define g_signal_handlers_disconnect_by_data(instance, data) \
  g_signal_handlers_disconnect_matched((instance), G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, (data))
#endif

static void notify_signal(GtkWidget *w, gint wParam, gpointer lParam, mforms::gtk::CodeEditorImpl *editor) {
  SCNotification *event = reinterpret_cast<SCNotification *>(lParam);
  editor->notify(event);
}

static void command_signal(GtkWidget *w, gint wParam, gpointer lParam, mforms::gtk::CodeEditorImpl *editor) {
  editor->command(wParam, reinterpret_cast<long>(lParam));
}

mforms::gtk::CodeEditorImpl::CodeEditorImpl(CodeEditor *self)
  : ViewImpl(self), _sci_gtk_widget(0), _sci_gtkmm_widget(0), _sci(0) {
  _sci_gtk_widget = scintilla_new();
  _sci_gtkmm_widget = Glib::wrap(_sci_gtk_widget);
  _sci_gtkmm_widget->reference();
  _sci_gtkmm_widget->set_hexpand(true);

  _sci = SCINTILLA(_sci_gtk_widget);
  _owner = self;
  g_signal_connect(_sci_gtk_widget, "command", G_CALLBACK(command_signal), this);
  g_signal_connect(_sci_gtk_widget, SCINTILLA_NOTIFY, G_CALLBACK(notify_signal), this);

  _sci_gtkmm_widget->signal_button_press_event().connect_notify(
    sigc::bind(sigc::mem_fun(this, &CodeEditorImpl::mouse_button_event), self));
  _sci_gtkmm_widget->signal_key_release_event().connect_notify(
    sigc::bind(sigc::mem_fun(this, &CodeEditorImpl::keyboard_event), self));
  _sci_gtkmm_widget->signal_key_press_event().connect_notify(
    sigc::bind(sigc::mem_fun(this, &CodeEditorImpl::keyboard_event), self));
  _sci_gtkmm_widget->show();

  _sci_gtkmm_widget->set_data("mforms", dynamic_cast<mforms::View *>(self));

  // change the default font as the default is some gigantic ugly font
  self->set_font("Bitstream Vera Sans Mono 10");
}

//------------------------------------------------------------------------------
mforms::gtk::CodeEditorImpl::~CodeEditorImpl() {
  g_signal_handlers_disconnect_by_data(_sci_gtk_widget, this);
  _sci_gtkmm_widget->unreference();
}

//------------------------------------------------------------------------------
Gtk::Widget *mforms::gtk::CodeEditorImpl::get_outer() const {
  return _sci_gtkmm_widget;
}

//------------------------------------------------------------------------------
bool mforms::gtk::CodeEditorImpl::create(CodeEditor *self, bool showInfo) {
  return new mforms::gtk::CodeEditorImpl(self);
}
//------------------------------------------------------------------------------
void mforms::gtk::CodeEditorImpl::keyboard_event(GdkEventKey *event, CodeEditor *editor) {
  if (event->type == GDK_KEY_RELEASE && event->keyval == GDK_KEY_Menu) {
    if (editor->get_context_menu() != NULL) {
      mforms::Menu *menu = editor->get_context_menu();
      if (menu && !menu->empty())
        menu->popup_at(editor, 0, 0); // gtk will handle position automagically
    }
  } else if (event->type == GDK_KEY_PRESS) {
    _owner->key_event(GetKeys(event->keyval), GetModifiers(event->state, event->keyval), "");
  }
}

void mforms::gtk::CodeEditorImpl::mouse_button_event(GdkEventButton *event, CodeEditor *editor) {
  if (event->type == GDK_BUTTON_PRESS && event->button == 3) // right mouse click
  {
    if (editor->get_context_menu() != NULL) {
      mforms::Menu *menu = editor->get_context_menu();
      if (menu && !menu->empty())
        menu->popup_at(editor, event->x, event->y);
    }
  }
}
//------------------------------------------------------------------------------
void mforms::gtk::CodeEditorImpl::notify(SCNotification *scn) {
  // Pass this to mforms::on_notify.
  _owner->on_notify(scn);
}

void mforms::gtk::CodeEditorImpl::command(unsigned long wParam, long) {
  _owner->on_command(wParam >> 16);
}

sptr_t mforms::gtk::CodeEditorImpl::send_editor(CodeEditor *self, unsigned int msg, uptr_t uparam, sptr_t sparam) {
  CodeEditorImpl *ce = self->get_data<CodeEditorImpl>();
  if (ce) {
    return scintilla_send_message(ce->_sci, msg, uparam, sparam);
  }
  return 0;
}

void mforms::gtk::CodeEditorImpl::set_status_text(CodeEditor *self, const std::string &text) {
  CodeEditorImpl *ce = self->get_data<CodeEditorImpl>();
  if (ce) {
    // no-op for now
    // g_message("set_status_text: %s", text.c_str());
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::CodeEditorImpl::init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_code_editor_impl.create = mforms::gtk::CodeEditorImpl::create;
  f->_code_editor_impl.send_editor = mforms::gtk::CodeEditorImpl::send_editor;
  f->_code_editor_impl.set_status_text = mforms::gtk::CodeEditorImpl::set_status_text;
}
