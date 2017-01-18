/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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
