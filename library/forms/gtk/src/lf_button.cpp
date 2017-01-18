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
#include "../lf_mforms.h"

#include "mforms/button.h"
#include "mforms/app.h"
//#include "../lf_view.h"
#include "../lf_button.h"

mforms::gtk::ButtonImpl::ButtonImpl(::mforms::Button *self, ::mforms::ButtonType btype, bool concrete)
  : ViewImpl(self), _holder(0), _label(0), _button(0), _icon(0) {
  if (concrete) {
    _button = Gtk::manage(new Gtk::Button());
    _holder = Gtk::manage(new Gtk::Box());
    if (btype == ::mforms::PushButton) {
      _label = Gtk::manage(new Gtk::Label());
      _holder->pack_start(*_label, true, true);
    } else {
      _icon = Gtk::manage(new Gtk::Image());
      _holder->pack_start(*_icon, true, true);
      _button->set_relief(Gtk::RELIEF_NONE);
    }
    _button->add(*_holder);
    _button->signal_clicked().connect(sigc::bind(sigc::ptr_fun(&ButtonImpl::callback), self));

    _button->show_all();
    setup();
  }
}

void mforms::gtk::ButtonImpl::callback(::mforms::Button *self) {
  self->callback();
}

bool mforms::gtk::ButtonImpl::create(::mforms::Button *self, ::mforms::ButtonType btype) {
  return new ButtonImpl(self, btype, true) != 0;
}

void mforms::gtk::ButtonImpl::set_text(::mforms::Button *self, const std::string &text) {
  if (self) {
    ButtonImpl *button = self->get_data<ButtonImpl>();

    if (button) {
      button->set_text(text);
    }
  }
}

void mforms::gtk::ButtonImpl::set_text(const std::string &text) {
  if (_label) {
    _label->set_label(text);
    _button->set_use_underline(true);
    _label->set_use_underline(true);
  } else {
    _button->set_label(text);
    _button->set_use_underline(true);
  }
}

void mforms::gtk::ButtonImpl::set_icon(::mforms::Button *self, const std::string &path) {
  if (self) {
    ButtonImpl *button = self->get_data<ButtonImpl>();

    if (button) {
      if (button->_icon == nullptr) {
        button->_icon = Gtk::manage(new Gtk::Image());
        if (button->_label != nullptr)
          button->_holder->remove(*button->_label);

        button->_holder->pack_start(*button->_icon, true, true);
        button->_icon->show();
        button->_button->show_all();
      }

      if (button->_icon) {
        std::string p = mforms::App::get()->get_resource_path(path);
        button->_icon->set(p);
      }
    }
  }
}

void mforms::gtk::ButtonImpl::enable_internal_padding(Button *self, bool enabled) {
  ButtonImpl *button = self->get_data<ButtonImpl>();
  if (button) {
    if (!button->_holder->get_children().empty()) {
      Gtk::Widget *widget = button->_holder->get_children()[0]; // If there's something inside, it's only one item.
      if (widget != nullptr) {
        if (enabled) {
          widget->set_margin_top(0);
          widget->set_margin_right(8);
          widget->set_margin_bottom(0);
          widget->set_margin_left(8);
        } else {
          widget->set_margin_top(0);
          widget->set_margin_right(0);
          widget->set_margin_bottom(0);
          widget->set_margin_left(0);
        }
      }
    }
  }
}

void mforms::gtk::ButtonImpl::init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_button_impl.create = &ButtonImpl::create;
  f->_button_impl.set_text = &ButtonImpl::set_text;
  f->_button_impl.set_icon = &ButtonImpl::set_icon;
  f->_button_impl.enable_internal_padding = &ButtonImpl::enable_internal_padding;
}
