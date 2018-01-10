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

      void notify(SCNotification *event);
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
