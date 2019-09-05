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

#ifndef _STUB_PROGRESSBAR_H_
#define _STUB_PROGRESSBAR_H_

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class ProgressBarWrapper : public ViewWrapper {
    protected:
      ProgressBarWrapper(::mforms::ProgressBar *self) : ViewWrapper(self) {
      }

      static bool create(::mforms::ProgressBar *self) {
        return true;
      }

      static void set_value(::mforms::ProgressBar *self, float pct) {
      }

      static void set_started(::mforms::ProgressBar *self, bool flag) {
      }

      static void set_indeterminate(::mforms::ProgressBar *self, bool flag) {
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_progressbar_impl.create = &ProgressBarWrapper::create;
        f->_progressbar_impl.set_value = &ProgressBarWrapper::set_value;
        f->_progressbar_impl.set_started = &ProgressBarWrapper::set_started;
        f->_progressbar_impl.set_indeterminate = &ProgressBarWrapper::set_indeterminate;
      }
    };
  };
};

#endif /* _STUB_PROGRESSBAR_H_ */
