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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_label.h"

#include "base/log.h"
#include "base/drawing.h"

using namespace System::Drawing;
using namespace System::Windows::Forms;

using namespace MySQL::Forms;

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

//--------------------------------------------------------------------------------------------------

/**
 * Converts an mform alignment value into a Windows::Forms alignment value.
 */
ContentAlignment get_alignment(mforms::Alignment align) {
  switch (align) {
    case mforms::BottomLeft:
      return ContentAlignment::BottomLeft;
    case mforms::BottomCenter:
      return ContentAlignment::BottomCenter;
    case mforms::BottomRight:
      return ContentAlignment::BottomRight;
    case mforms::MiddleLeft:
      return ContentAlignment::MiddleLeft;
    case mforms::MiddleCenter:
      return ContentAlignment::MiddleCenter;
    case mforms::MiddleRight:
      return ContentAlignment::MiddleRight;
    case mforms::TopLeft:
      return ContentAlignment::TopLeft;
    case mforms::TopCenter:
      return ContentAlignment::TopCenter;
    case mforms::TopRight:
      return ContentAlignment::TopRight;
  }
  return ::System::Drawing::ContentAlignment::TopLeft;
}

//--------------------------------------------------------------------------------------------------

WrapControlLabel::WrapControlLabel() {
  autoWrapping = false;
  lastSize = Drawing::Size(-1, -1);
  ForeColor = Color::FromKnownColor(KnownColor::WindowText);
}

//--------------------------------------------------------------------------------------------------

void WrapControlLabel::Text::set(String ^ value) {
  __super ::set(value);
  lastSize = Drawing::Size(-1, -1);
}

//--------------------------------------------------------------------------------------------------

void WrapControlLabel::Font::set(Drawing::Font ^ value) {
  __super ::set(value);
  lastSize = Drawing::Size(-1, -1);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the preferred size of the label depending on its auto-wrapping mode.
 */
Drawing::Size WrapControlLabel::GetPreferredSize(Drawing::Size proposedSize) {
  System::Drawing::Size result;
  if (!autoWrapping) {
    // Default behavior, just let the control itself determine what it needs.
    // Don't use the proposed size, however, or the returned size will always be of the given
    // width (which may lead to wrapping we don't want).
    result = Label::GetPreferredSize(Drawing::Size::Empty);
  } else {
    // We use the minimal size if the proposed size is too small to allow computation also
    // for containers which have a priori no proper width yet (e.g. tables).
    // This will help us to solve the chicken-egg problem we have (table needs width, but wrapped label too)
    // and at the same time properly involves our minimum size.
    if (proposedSize.Width < MinimumSize.Width)
      proposedSize.Width = MinimumSize.Width;
    if (proposedSize.Height < MinimumSize.Height)
      proposedSize.Height = MinimumSize.Height;

    if (lastSize.Width == proposedSize.Width)
      return lastSize;

    if (proposedSize.Width == 0)
      result = System::Drawing::Size(0, 0);
    else {
      TextFormatFlags flags =
        TextFormatFlags::NoPadding | TextFormatFlags::WordBreak | TextFormatFlags::Left | TextFormatFlags::Top;
      result = TextRenderer::MeasureText(Text, Font, System::Drawing::Size(proposedSize.Width, 0), flags);
    }
  };

  // Make sure we don't go smaller than our minimal size anyway.
  if (result.Width < MinimumSize.Width)
    result.Width = MinimumSize.Width;
  if (result.Height < MinimumSize.Height)
    result.Height = MinimumSize.Height;

  lastSize = result;

  return result;
}

//----------------- LabelWrapper -------------------------------------------------------------------

LabelWrapper::LabelWrapper(mforms::Label *backend) : ViewWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool LabelWrapper::create(mforms::Label *backend) {
  LabelWrapper *wrapper = new LabelWrapper(backend);
  WrapControlLabel ^ label = Create<WrapControlLabel>(backend, wrapper);
  label->TextAlign = ContentAlignment::MiddleLeft;
  label->BorderStyle = BorderStyle::None;

  return true;
}

//--------------------------------------------------------------------------------------------------

void LabelWrapper::set_style(mforms::Label *backend, mforms::LabelStyle style) {
  WrapControlLabel ^ label = LabelWrapper::GetManagedObject<WrapControlLabel>(backend);
  try {
    switch (style) {
      case mforms::NormalStyle:
        label->Font = gcnew Font(DEFAULT_FONT_FAMILY, DEFAULT_FONT_SIZE, FontStyle::Regular, GraphicsUnit::Pixel);
        break;
      case mforms::BoldStyle:
        label->Font = gcnew Font(DEFAULT_FONT_FAMILY, DEFAULT_FONT_SIZE, FontStyle::Bold, GraphicsUnit::Pixel);
        break;
      case mforms::BigStyle:
        label->Font = gcnew Font(DEFAULT_FONT_FAMILY, DEFAULT_FONT_SIZE + 4, FontStyle::Regular, GraphicsUnit::Pixel);
        break;
      case mforms::BigBoldStyle:
        label->Font = gcnew Font(DEFAULT_FONT_FAMILY, DEFAULT_FONT_SIZE + 4, FontStyle::Bold, GraphicsUnit::Pixel);
        break;
      case mforms::SmallBoldStyle:
        label->Font = gcnew Font(DEFAULT_FONT_FAMILY, 8, FontStyle::Bold, GraphicsUnit::Pixel);
        break;
      case mforms::SmallStyle:
        label->Font = gcnew Font(DEFAULT_FONT_FAMILY, 8, FontStyle::Regular, GraphicsUnit::Point);
        break;
      case mforms::VerySmallStyle:
        label->Font = gcnew Font(DEFAULT_SMALL_FONT, 7.5, FontStyle::Regular, GraphicsUnit::Point);
        break;
      case mforms::InfoCaptionStyle:
        label->Font = gcnew Font(DEFAULT_FONT_FAMILY, label->Font->Size, FontStyle::Regular, GraphicsUnit::Pixel);
        break;
      case mforms::BoldInfoCaptionStyle:
        label->Font = gcnew Font(DEFAULT_FONT_FAMILY, label->Font->Size, FontStyle::Bold, GraphicsUnit::Pixel);
        break;
      case mforms::WizardHeadingStyle:
        label->Font = gcnew Font(DEFAULT_FONT_FAMILY, 12.0, FontStyle::Bold, GraphicsUnit::Pixel);
        label->ForeColor = ColorTranslator::FromHtml("#003392");
        break;
      case mforms::SmallHelpTextStyle:
        label->Font = gcnew Font(DEFAULT_FONT_FAMILY, 11, FontStyle::Regular, GraphicsUnit::Pixel);
        break;
      case mforms::VeryBigStyle:
        label->Font = gcnew Font(DEFAULT_FONT_FAMILY, 13, FontStyle::Regular, GraphicsUnit::Point);
        break;
    }

    Size newSize = label->PreferredSize;
    if (label->AutoWrapping)
      newSize.Width = label->Size.Width;
    label->Size = newSize;
  } catch (System::ArgumentException ^ e) {
    // Argument exception pops up when the system cannot find the Regular font style (corrupt font).
    logError("LabelImpl::set_style failed. %s\n", e->Message);
  }
}

//--------------------------------------------------------------------------------------------------

void LabelWrapper::set_text(mforms::Label *backend, const std::string &text) {
  WrapControlLabel ^ label = LabelWrapper::GetManagedObject<WrapControlLabel>(backend);

  String ^ new_text;
  if (!text.empty() &&
      text[text.size() - 1] == '\n') // Add a space if there's a trailing linebreak. Otherwise it would be ignored.
    new_text = CppStringToNative(text + " ");
  else
    new_text = CppStringToNative(text);

  if (label->Text != new_text) {
    label->Text = new_text;

    Size newSize = label->PreferredSize;
    if (label->AutoWrapping)
      newSize.Width = label->Size.Width;
    label->Size = newSize;
  };

  backend->set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

void LabelWrapper::set_text_align(mforms::Label *backend, mforms::Alignment align) {
  WrapControlLabel ^ label = LabelWrapper::GetManagedObject<WrapControlLabel>(backend);
  label->TextAlign = get_alignment(align);
  backend->set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

void LabelWrapper::set_color(mforms::Label *backend, const std::string &color) {
  WrapControlLabel ^ label = LabelWrapper::GetManagedObject<WrapControlLabel>(backend);
  label->ForeColor = ColorTranslator::FromHtml(CppStringToNative(color));
}

//--------------------------------------------------------------------------------------------------

void LabelWrapper::set_wrap_text(mforms::Label *backend, bool flag) {
  WrapControlLabel ^ label = LabelWrapper::GetManagedObject<WrapControlLabel>(backend);
  label->AutoWrapping = flag;
  backend->set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

void LabelWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_label_impl.create = &LabelWrapper::create;
  f->_label_impl.set_text = &LabelWrapper::set_text;
  f->_label_impl.set_style = &LabelWrapper::set_style;
  f->_label_impl.set_text_align = &LabelWrapper::set_text_align;
  f->_label_impl.set_color = &LabelWrapper::set_color;
  f->_label_impl.set_wrap_text = &LabelWrapper::set_wrap_text;
}

//--------------------------------------------------------------------------------------------------
