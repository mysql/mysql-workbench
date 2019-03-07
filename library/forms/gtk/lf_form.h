/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _LF_FORM_H_
#define _LF_FORM_H_

#include "mforms/mforms.h"
#include <boost/signals2.hpp>
#include <sigc++/sigc++.h>

#include "gtk_helpers.h"
#include "main_app.h"
#include "lf_button.h"
#include "lf_view.h"
#include "lf_mforms.h"

namespace mforms {
  namespace gtk {

    class FormImpl : public ViewImpl {
      runtime::loop _loop;
      Gtk::Window *_window;
      int _in_modal_loop;
      bool _result;
      virtual Gtk::Widget *get_outer() const {
        return _window;
      }
      boost::signals2::scoped_connection accept_c;
      boost::signals2::scoped_connection cancel_c;

      static bool create(::mforms::Form *self, ::mforms::Form *owner, mforms::FormFlag flag);
      static void set_title(::mforms::Form *self, const std::string &title);
      void accept_clicked(bool *status, const bool is_run);
      void cancel_clicked(bool *status, const bool is_run);
      bool on_widget_delete_event(GdkEventAny *event, Button *cancel);
      bool can_delete_widget(GdkEventAny *event);
      static void show_modal(::mforms::Form *self, ::mforms::Button *accept, ::mforms::Button *cancel);
      static void end_modal(::mforms::Form *self, bool result);
      bool on_key_release(GdkEventKey *event, bool *status, const bool is_run, ::mforms::Button *accept,
                          ::mforms::Button *cancel);
      static bool run_modal(::mforms::Form *self, ::mforms::Button *accept, ::mforms::Button *cancel);
      static void close(::mforms::Form *self);
      static void set_content(::mforms::Form *self, ::mforms::View *child);
      static void flush_events(::mforms::Form *self);
      static void center(Form *self);
      static void set_menubar(mforms::Form *self, mforms::MenuBar *menu);
      void realized(mforms::Form *owner, Gdk::WMDecoration flags);
      virtual void set_name(const std::string &name);
      virtual void show(bool show);
      bool on_focus_event(GdkEventFocus *ev, ::mforms::Form *form);

    public:
      FormImpl(::mforms::Form *form, ::mforms::Form *owner, mforms::FormFlag form_flag);
      virtual void set_title(const std::string &title);
      static void init();
      static void init_main_form(Gtk::Window *main);
      Gtk::Window *get_window() {
        return _window;
      }
    };
  };
};

#endif
