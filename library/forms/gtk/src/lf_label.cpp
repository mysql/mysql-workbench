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

#include "../lf_mforms.h"
#include "../lf_label.h"
#include "gtk_helpers.h"

mforms::gtk::LabelImpl::LabelImpl(::mforms::Label *self) : ViewImpl(self), _font_set(false) {
  _style = mforms::NormalStyle;
  _label = Gtk::manage(new Gtk::Label(""));
  _label->set_alignment(0, 0.5);
  _label->set_use_underline(false);

  setup();
  _label->signal_draw().connect(sigc::bind(sigc::ptr_fun(mforms::gtk::draw_event_slot), _label), false);
  _label->signal_realize().connect(sigc::mem_fun(this, &LabelImpl::realized));
  _label->show();
}

bool mforms::gtk::LabelImpl::create(::mforms::Label *self) {
  return new LabelImpl(self) != 0;
}

void mforms::gtk::LabelImpl::set_style(::mforms::Label *self, ::mforms::LabelStyle style) {
  LabelImpl *label = self->get_data<LabelImpl>();
  label->_style = style;
  if (label->_label->get_realized())
    label->realized();
}

void mforms::gtk::LabelImpl::realized() {
  if (!_font_set) {
    _font = _label->get_pango_context()->get_font_description();
    _font_set = true;
  }
  Pango::FontDescription font(_font);
  // Pango::FontDescription font(Gtk::RC::get_style(*_label)->get_font());
  {
    switch (_style) {
      case ::mforms::NormalStyle:
        // l->Font= gcnew System::Drawing::Font(l->Font, System::Drawing::FontStyle::Regular);
        break;
      case ::mforms::BigStyle: {
        font.set_size(font.get_size() * 4 / 3);
        _label->override_font(font);
      } break;
      case ::mforms::VeryBigStyle: {
        font.set_size(font.get_size() * 5 / 3);
        _label->override_font(font);
      } break;
      case ::mforms::BigBoldStyle: {
        font.set_size(font.get_size() * 4 / 3);
        font.set_weight(Pango::WEIGHT_BOLD);
        _label->override_font(font);
      } break;
      case ::mforms::BoldStyle: {
        font.set_weight(Pango::WEIGHT_BOLD);
        _label->override_font(font);
      } break;
      case ::mforms::SmallBoldStyle: {
        font.set_weight(Pango::WEIGHT_BOLD);
        font.set_size(font.get_size() * 5 / 7);
        _label->override_font(font);
      } break;
      case ::mforms::SmallStyle: {
        font.set_size(font.get_size() * 5 / 6);
        _label->override_font(font);
      } break;
      case ::mforms::VerySmallStyle: {
        font.set_size(font.get_size() * 3 / 5);
        _label->override_font(font);
      } break;
      case ::mforms::InfoCaptionStyle:
        break;
      case ::mforms::BoldInfoCaptionStyle: {
        font.set_weight(Pango::WEIGHT_BOLD);
        _label->override_font(font);
      } break;
      case ::mforms::WizardHeadingStyle: {
        font.set_size(font.get_size() * 1.2);
        font.set_weight(Pango::WEIGHT_BOLD);
        _label->override_font(font);
      } break;
      case ::mforms::SmallHelpTextStyle: {
        font.set_size(font.get_size() * 4 / 5);
        _label->override_font(font);
      } break;
    }
  }
}

void mforms::gtk::LabelImpl::set_text(::mforms::Label *self, const std::string &text) {
  LabelImpl *label = self->get_data<LabelImpl>();

  if (label)
    ((Gtk::Label *)label->_label)->set_text(text);
}

void mforms::gtk::LabelImpl::set_color(::mforms::Label *self, const std::string &text) {
  LabelImpl *label = self->get_data<LabelImpl>();
  if (label)
    ((Gtk::Label *)label->_label)->override_color(color_to_rgba(Gdk::Color(text)), Gtk::STATE_FLAG_NORMAL);
}

void mforms::gtk::LabelImpl::set_wrap_text(::mforms::Label *self, bool flag) {
  LabelImpl *label = self->get_data<LabelImpl>();

  if (label) {
    ((Gtk::Label *)label->_label)->set_line_wrap(flag);
  }
}

void mforms::gtk::LabelImpl::set_text_align(::mforms::Label *self, ::mforms::Alignment align) {
  LabelImpl *label = self->get_data<LabelImpl>();

  if (label) {
    /*
    Gtk::Justification gtk_align = Gtk::JUSTIFY_LEFT;
    if ( align == mforms::BottomCenter || align == mforms::MiddleCenter || mforms::TopCenter )
      gtk_align = Gtk::JUSTIFY_CENTER;
    if ( align == mforms::BottomRight || align == mforms::MiddleRight || mforms::TopRight )
      gtk_align = Gtk::JUSTIFY_RIGHT;

    ((Gtk::Label*)label->_label)->set_justify(gtk_align);
     */

    float x = 0, y = 0;
    switch (align) {
      case ::mforms::BottomLeft:
        x = 0;
        y = 1;
        break;
      case ::mforms::BottomCenter:
        x = 0.5;
        y = 1;
        break;
      case ::mforms::BottomRight:
        x = 1;
        y = 1;
        break;
      case ::mforms::MiddleLeft:
        x = 0;
        y = 0.5;
        break;
      case ::mforms::MiddleCenter:
        x = 0.5;
        y = 0.5;
        break;
      case ::mforms::MiddleRight:
        x = 1;
        y = 0.5;
        break;
      case ::mforms::TopLeft:
        x = 0;
        y = 0;
        break;
      case ::mforms::TopCenter:
        x = 0.5;
        y = 0;
        break;
      case ::mforms::TopRight:
        x = 1;
        y = 0;
        break;
      case ::mforms::NoAlign:
        break;
    }
    ((Gtk::Label *)label->_label)->set_alignment(x, y);
  }
}

void mforms::gtk::LabelImpl::init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_label_impl.create = &LabelImpl::create;
  f->_label_impl.set_style = &LabelImpl::set_style;
  f->_label_impl.set_text = &LabelImpl::set_text;
  f->_label_impl.set_text_align = &LabelImpl::set_text_align;
  f->_label_impl.set_color = &LabelImpl::set_color;
  f->_label_impl.set_wrap_text = &LabelImpl::set_wrap_text;
}
