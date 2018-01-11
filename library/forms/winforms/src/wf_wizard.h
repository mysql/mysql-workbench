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

#pragma once

namespace MySQL {
  namespace Forms {

    ref class WizardForm;

  public
    class WizardWrapper : public FormWrapper {
    protected:
      WizardWrapper(mforms::Wizard *backend, mforms::Form *owner);

      static bool create(mforms::Wizard *backend, mforms::Form *parent);
      static void set_title(mforms::Wizard *backend, const std::string &title);
      static void run_modal(mforms::Wizard *backend);
      static void close(mforms::Wizard *backend);
      static void set_content(mforms::Wizard *backend, mforms::View *view);
      static void set_heading(mforms::Wizard *backend, const std::string &heading);
      static void set_step_list(mforms::Wizard *backend, const std::vector<std::string> &steps);
      static void set_allow_cancel(mforms::Wizard *backend, bool flag);
      static void set_allow_back(mforms::Wizard *backend, bool flag);
      static void set_allow_next(mforms::Wizard *backend, bool flag);
      static void set_show_extra(mforms::Wizard *backend, bool flag);
      static void set_extra_caption(mforms::Wizard *backend, const std::string &caption);
      static void set_next_caption(mforms::Wizard *backend, const std::string &caption);

    public:
      static void init();
    };
  }
}
