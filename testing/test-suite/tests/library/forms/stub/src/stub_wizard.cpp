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

#include "../stub_wizard.h"
#include "../stub_view.h"

#include "base/string_utilities.h"

namespace mforms {
  namespace stub {

    //------------------------------------------------------------------------------
    WizardWrapper::WizardWrapper(::mforms::Wizard *wiz) : ObjectWrapper(wiz) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::cancel(::mforms::Wizard *wiz) {
    }

    //------------------------------------------------------------------------------
    bool WizardWrapper::create(::mforms::Wizard *self, Form *owner) {
      return true;
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::set_title(::mforms::Wizard *self, const std::string &title) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::run_modal(::mforms::Wizard *self) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::close(::mforms::Wizard *self) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::set_content(::mforms::Wizard *self, View *view) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::set_heading(::mforms::Wizard *self, const std::string &heading) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::set_step_list(::mforms::Wizard *self, const std::vector<std::string> &steps) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::refresh_step_list(const std::vector<std::string> &steps) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::set_icon_path(const std::string &path) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::set_allow_cancel(::mforms::Wizard *self, bool flag) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::set_allow_back(::mforms::Wizard *self, bool flag) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::set_allow_next(::mforms::Wizard *self, bool flag) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::set_show_extra(::mforms::Wizard *self, bool flag) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::set_extra_caption(::mforms::Wizard *self, const std::string &caption) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::set_next_caption(::mforms::Wizard *self, const std::string &caption) {
    }

    //------------------------------------------------------------------------------
    void WizardWrapper::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_wizard_impl.create = &WizardWrapper::create;
      f->_wizard_impl.set_title = &WizardWrapper::set_title;
      f->_wizard_impl.run_modal = &WizardWrapper::run_modal;
      f->_wizard_impl.close = &WizardWrapper::close;

      f->_wizard_impl.set_content = &WizardWrapper::set_content;
      f->_wizard_impl.set_heading = &WizardWrapper::set_heading;
      f->_wizard_impl.set_step_list = &WizardWrapper::set_step_list;
      f->_wizard_impl.set_allow_cancel = &WizardWrapper::set_allow_cancel;
      f->_wizard_impl.set_allow_back = &WizardWrapper::set_allow_back;
      f->_wizard_impl.set_allow_next = &WizardWrapper::set_allow_next;
      f->_wizard_impl.set_show_extra = &WizardWrapper::set_show_extra;

      f->_wizard_impl.set_extra_caption = &WizardWrapper::set_extra_caption;
      f->_wizard_impl.set_next_caption = &WizardWrapper::set_next_caption;
    }

  } // end of stub namespace
} // end of mforms namespace
