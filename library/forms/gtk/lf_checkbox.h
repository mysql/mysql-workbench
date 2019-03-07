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

#ifndef _LF_CHECKBOX_H_
#define _LF_CHECKBOX_H_

#include "mforms/mforms.h"

#include "lf_button.h"

namespace mforms {
  namespace gtk {

    class CheckBoxImpl : public ButtonImpl {
      Gtk::CheckButton *_check;
      virtual Gtk::Widget *get_outer() const {
        return _check;
      }

    protected:
      static bool create(::mforms::CheckBox *self, bool square) {
        return new CheckBoxImpl(self, square) != 0;
      }

      static void set_active(::mforms::CheckBox *self, bool flag) {
        CheckBoxImpl *cb = self->get_data<CheckBoxImpl>();

        if (cb)
          cb->_check->set_active(flag);
      }

      static bool get_active(::mforms::CheckBox *self) {
        CheckBoxImpl *cb = self->get_data<CheckBoxImpl>();
        return (cb ? cb->_check->get_active() : false);
      }

      CheckBoxImpl(::mforms::CheckBox *self, bool square) : ButtonImpl(self) {
        delete _button;
        _check = Gtk::manage(new Gtk::CheckButton());
        _check->set_use_underline(false);
        _check->signal_clicked().connect(sigc::bind(sigc::ptr_fun(&CheckBoxImpl::callback), self));
        _button = _check;
        _check->show();
      }

      static void callback(::mforms::CheckBox *self) {
        self->callback();
      }

      virtual void set_text(const std::string &text) {
        if (_label)
          _label->set_label(text);
        else
          _button->set_label(text);
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_checkbox_impl.create = &CheckBoxImpl::create;
        f->_checkbox_impl.set_active = &CheckBoxImpl::set_active;
        f->_checkbox_impl.get_active = &CheckBoxImpl::get_active;
      }
    };
  };
};

#endif
