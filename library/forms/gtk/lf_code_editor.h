/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __LF_CODE_EDITOR_H__
#define __LF_CODE_EDITOR_H__

#include "mforms/code_editor.h"
#include "lf_view.h"

#include "Scintilla.h"
#define PLAT_GTK 2
#define GTK
#include "ScintillaWidget.h"

namespace mforms {
  namespace gtk {

    typedef std::map<std::string, int> StyleNameMap;

    class CodeEditorImpl : public ViewImpl {
    public:
      CodeEditorImpl(CodeEditor *self);
      virtual ~CodeEditorImpl();
      static void init();

      virtual Gtk::Widget *get_outer() const;

      void notify(Scintilla::SCNotification *event);
      void command(unsigned long wParam, long);

    protected:
      void keyboard_event(GdkEventKey *event, CodeEditor *editor);
      void mouse_button_event(GdkEventButton *event, CodeEditor *editor);

    private:
      // private data
      GtkWidget *_sci_gtk_widget;
      Gtk::Widget *_sci_gtkmm_widget;
      ScintillaObject *_sci;
      CodeEditor *_owner;
      static bool create(CodeEditor *self, bool showInfo);
      static sptr_t send_editor(CodeEditor *self, unsigned int msg, uptr_t uparam, sptr_t sparam);
      static void show_find_panel(CodeEditor *self, bool show);
      static void set_status_text(CodeEditor *self, const std::string &text);
    };

  } // end of namespace gtk
} // end of namespace mforms

#endif
