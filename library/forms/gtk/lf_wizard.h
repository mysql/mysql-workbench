/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _LF_WIZARD_H_
#define _LF_WIZARD_H_

#include "mforms/mforms.h"

#include "lf_base.h"
#include "lf_form.h"
#include "lf_mforms.h"

#include <gtkmm.h>

namespace mforms {
  namespace gtk {

    class WizardImpl : public FormImpl {
      Gtk::Table _top_table;
      Gtk::Label _heading;
      Gtk::ScrolledWindow _content;
      Gtk::Box _button_box;
      Gtk::Button _cancel_btn;
      Gtk::Button _fwd_btn;
      Gtk::Button _back_btn;
      Gtk::Button _extra_btn;
      Gtk::Table _step_table;
      Gtk::Label _fwd_label;
      Gtk::Label _extra_label;
      Gtk::EventBox _step_background;
      runtime::loop _loop;

      typedef std::pair<Gtk::Image *, Gtk::Label *> ImageLabel;
      std::vector<ImageLabel> _steps;

      void refresh_step_list(const std::vector<std::string> &steps);

      static void cancel(::mforms::Wizard *wiz);
      static bool delete_event(GdkEventAny *ev, ::mforms::Wizard *wiz);

    protected:
      WizardImpl(::mforms::Wizard *wiz, ::mforms::Form *owner);

      static bool create(::mforms::Wizard *self, ::mforms::Form *owner);
      static void set_title(::mforms::Wizard *self, const std::string &title);
      static void run_modal(::mforms::Wizard *self);
      static void close(::mforms::Wizard *self);
      static void flush_events(::mforms::Wizard *self);
      static void set_content(::mforms::Wizard *self, View *view);
      static void set_heading(::mforms::Wizard *self, const std::string &);
      static void set_step_list(::mforms::Wizard *self, const std::vector<std::string> &);
      static void set_allow_cancel(::mforms::Wizard *self, bool flag);
      static void set_allow_back(::mforms::Wizard *self, bool flag);
      static void set_allow_next(::mforms::Wizard *self, bool flag);
      static void set_show_extra(::mforms::Wizard *self, bool flag);
      static void set_extra_caption(::mforms::Wizard *self, const std::string &);
      static void set_next_caption(::mforms::Wizard *self, const std::string &);

    public:
      static void init();

      static void set_icon_path(const std::string &path);
    };

  } // end of gtk namespace
} // end of mforms namespace

#endif
