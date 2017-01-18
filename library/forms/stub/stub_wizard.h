/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _STUB_WIZARD_H_
#define _STUB_WIZARD_H_

#include "stub_base.h"

namespace mforms {
  namespace stub {

    class WizardWrapper : public ObjectWrapper {
      void refresh_step_list(const std::vector<std::string> &steps);

      static void cancel(::mforms::Wizard *wiz);

    protected:
      WizardWrapper(::mforms::Wizard *wiz);

      static bool create(::mforms::Wizard *self, Form *owner);
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

  } // end of stub namespace
} // end of mforms namespace

#endif
