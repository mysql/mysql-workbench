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

#pragma once

#include <mforms/base.h>
#include <mforms/view.h>

namespace mforms {
  class Label;

  enum LabelStyle {
    // normal text in normal system font
    NormalStyle,
    // same as above, but bold
    BoldStyle,
    // bold text, slightly smaller
    SmallBoldStyle,
    // large text
    BigStyle,
    // same as above, but bold
    BigBoldStyle,
    // smaller than normal text
    SmallStyle,
    // As small as possible but still readable, for titles under widgets etc.
    VerySmallStyle,

    // style for showing some information, can be same as normal or a bit smaller
    InfoCaptionStyle,
    BoldInfoCaptionStyle,
    // wizard heading, in windows its blue, black elsewhere.. also bold and a bigger than normal
    WizardHeadingStyle,
    // description/help text to show for options, smaller than normal for fitting longer descriptions
    SmallHelpTextStyle,

    VeryBigStyle // 18pt
  };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct LabelImplPtrs {
    bool (*create)(Label *self);
    void (*set_style)(Label *self, LabelStyle style);
    void (*set_text)(Label *self, const std::string &text);
    void (*set_text_align)(Label *self, Alignment align);
    void (*set_color)(Label *self, const std::string &color);
    void (*set_wrap_text)(Label *self, bool flag);
  };
#endif
#endif

  /** A control with some static text. */
  class MFORMS_EXPORT Label : public View {
  public:
    Label(const std::string &text, bool right_align = false);
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
    Label();
#endif
#endif

    /** Sets whether the text should wrap when it doesn't fit horizontally.

     Note that this does not work well in GTK. */
    void set_wrap_text(bool flag);

    /** Sets the alignment of the text in the available space, horizontally and vertically. */
    void set_text_align(Alignment align);

    /** Sets the text to be displayed. */
    void set_text(const std::string &text);

    /** Sets the style of the text. */
    void set_style(LabelStyle style);

    /** Sets the text color. */
    void set_color(const std::string &color);

  protected:
    LabelImplPtrs *_label_impl;
  };
};
