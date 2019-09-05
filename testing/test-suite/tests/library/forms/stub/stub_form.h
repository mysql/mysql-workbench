/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _STUB_FORM_H_
#define _STUB_FORM_H_

#include "stub_button.h"
#include "stub_view.h"
#include "stub_mforms.h"

namespace mforms {
  namespace stub {

    class FormWrapper : public ViewWrapper {
      static bool create(::mforms::Form *self, ::mforms::Form *owner, mforms::FormFlag flag);
      static void set_title(::mforms::Form *self, const std::string &title);
      void accept_clicked(bool *status, const bool is_run);
      void cancel_clicked(bool *status, const bool is_run);
      static void show_modal(::mforms::Form *self, ::mforms::Button *accept, ::mforms::Button *cancel);
      static bool run_modal(::mforms::Form *self, ::mforms::Button *accept, ::mforms::Button *cancel);
      static void close(::mforms::Form *self);
      static void set_content(::mforms::Form *self, ::mforms::View *child);
      static void flush_events(::mforms::Form *self);
      static void center(Form *self);
      FormWrapper(::mforms::Form *form, ::mforms::Form *owner, mforms::FormFlag form_flag);
      virtual void set_size(int width, int height);
      static void end_modal(Form *self, bool result);

    public:
      static void init();
    };
  };
};

#endif
