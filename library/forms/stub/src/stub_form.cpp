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

#include "../stub_form.h"

namespace mforms {
  namespace stub {

    bool FormWrapper::create(::mforms::Form *self, ::mforms::Form *owner, mforms::FormFlag flag) {
      return true;
    }

    void FormWrapper::set_title(::mforms::Form *self, const std::string &title) {
    }

    void FormWrapper::accept_clicked(bool *status, const bool is_run) {
    }

    void FormWrapper::cancel_clicked(bool *status, const bool is_run) {
    }

    void FormWrapper::show_modal(::mforms::Form *self, ::mforms::Button *accept, ::mforms::Button *cancel) {
    }

    bool FormWrapper::run_modal(::mforms::Form *self, ::mforms::Button *accept, ::mforms::Button *cancel) {
      return false;
    }

    void FormWrapper::close(::mforms::Form *self) {
    }

    void FormWrapper::set_content(::mforms::Form *self, ::mforms::View *child) {
    }

    void FormWrapper::flush_events(::mforms::Form *self) {
    }

    void FormWrapper::center(Form *self) {
    }

    void FormWrapper::end_modal(Form *self, bool result) {
    }

    FormWrapper::FormWrapper(::mforms::Form *form, ::mforms::Form *owner, mforms::FormFlag form_flag)
      : ViewWrapper(form) {
    }

    void FormWrapper::set_size(int width, int height) {
    }

    void FormWrapper::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_form_impl.create = &FormWrapper::create;
      f->_form_impl.close = &FormWrapper::close;
      f->_form_impl.set_title = &FormWrapper::set_title;
      f->_form_impl.show_modal = &FormWrapper::show_modal;
      f->_form_impl.run_modal = &FormWrapper::run_modal;
      f->_form_impl.set_content = &FormWrapper::set_content;
      f->_form_impl.flush_events = &FormWrapper::flush_events;
      f->_form_impl.center = &FormWrapper::center;
      f->_form_impl.end_modal = &FormWrapper::end_modal;
    }
  };
};
