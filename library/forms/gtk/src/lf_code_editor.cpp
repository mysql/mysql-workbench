/*
 * Copyright (c) 2010, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "../lf_mforms.h"

#include "base/string_utilities.h"
#include "../lf_code_editor.h"
#include "tinyxml.h"
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

using namespace Scintilla;

static void notify_signal(GtkWidget *w, gint wParam, gpointer lParam, mforms::gtk::CodeEditorImpl *editor)
{
    SCNotification *event = reinterpret_cast<SCNotification *>(lParam);
    editor->notify(event);
}

static void command_signal(GtkWidget *w, gint wParam, gpointer lParam, mforms::gtk::CodeEditorImpl *editor)
{
  editor->command(wParam, reinterpret_cast<long>(lParam));
}

mforms::gtk::CodeEditorImpl::CodeEditorImpl(CodeEditor* self)
       : ViewImpl(self)
       ,_sci_gtk_widget(0)
       ,_sci_gtkmm_widget(0)
       ,_sci(0)
{
  _sci_gtk_widget = scintilla_new();
  _sci_gtkmm_widget = Glib::wrap(_sci_gtk_widget);
  _sci_gtkmm_widget->reference();
  _sci = SCINTILLA(_sci_gtk_widget);
  _owner = self;
  gtk_signal_connect(GTK_OBJECT(_sci_gtk_widget), "command", GTK_SIGNAL_FUNC(command_signal), this);
  gtk_signal_connect(GTK_OBJECT(_sci_gtk_widget), SCINTILLA_NOTIFY, GTK_SIGNAL_FUNC(notify_signal), this);

  _sci_gtkmm_widget->signal_button_press_event().connect_notify(sigc::bind(sigc::mem_fun(this, &CodeEditorImpl::mouse_button_event), self));
  _sci_gtkmm_widget->show();

  _sci_gtkmm_widget->set_data("mforms", dynamic_cast<mforms::View*>(self));

  // change the default font as the default is some gigantic ugly font
  self->set_font("Bitstream Vera Sans Mono 10");
}

//------------------------------------------------------------------------------
mforms::gtk::CodeEditorImpl::~CodeEditorImpl()
{
  _sci_gtkmm_widget->unreference();
}

//------------------------------------------------------------------------------
Gtk::Widget *mforms::gtk::CodeEditorImpl::get_outer() const
{
  return _sci_gtkmm_widget;
}

//------------------------------------------------------------------------------
bool mforms::gtk::CodeEditorImpl::create(CodeEditor* self)
{
  return new mforms::gtk::CodeEditorImpl(self);
}
//------------------------------------------------------------------------------
void mforms::gtk::CodeEditorImpl::mouse_button_event(GdkEventButton *event, CodeEditor *editor)
{
  if (event->type == GDK_BUTTON_PRESS && event->button == 3) //right mouse click
  {
    if (editor->get_context_menu() != NULL)
    {
      mforms::Menu *menu = editor->get_context_menu();
      if (menu && !menu->empty())
        menu->popup_at(editor, event->x, event->y);
    }
  }
}
//------------------------------------------------------------------------------
void mforms::gtk::CodeEditorImpl::notify(SCNotification *scn)
{
  //Pass this to mforms::on_notify.
  _owner->on_notify(scn);
}

void mforms::gtk::CodeEditorImpl::command(unsigned long wParam, long)
{
  _owner->on_command(wParam >> 16);
}

sptr_t mforms::gtk::CodeEditorImpl::send_editor(CodeEditor* self, unsigned int msg, uptr_t uparam, sptr_t sparam)
{
  CodeEditorImpl* ce = self->get_data<CodeEditorImpl>();
  if (ce) {
    return scintilla_send_message(ce->_sci, msg, uparam, sparam);
  }
  return 0;
}


void mforms::gtk::CodeEditorImpl::set_status_text(CodeEditor *self, const std::string &text)
{
  CodeEditorImpl* ce = self->get_data<CodeEditorImpl>();
  if (ce)
  {
   // no-op for now
   // g_message("set_status_text: %s", text.c_str());
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::CodeEditorImpl::init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_code_editor_impl.create = mforms::gtk::CodeEditorImpl::create;
  f->_code_editor_impl.send_editor = mforms::gtk::CodeEditorImpl::send_editor;
  f->_code_editor_impl.set_status_text = mforms::gtk::CodeEditorImpl::set_status_text;
}

