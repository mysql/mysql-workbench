/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "common.h"

// Terminal text styling support.

namespace casmine {

  enum class AnsiStyle {
    Reset,
    Bold,
    Dim,
    Italic,
    Underline,
    Inverse,
    Hidden,
    StrikeThrough,

    BlackForeground,
    RedForeground,
    GreenForeground,
    YellowForeground,
    BlueForeground,
    MagentaForeground,
    CyanForeground,
    WhiteForeground,
    GrayForeground,

    BlackBackground,
    RedBackground,
    GreenBackground,
    YellowBackground,
    BlueBackground,
    MagentaBackground,
    CyanBackground,
    WhiteBackground,
    GrayBackground,
  };

  // Wrap the given input with the codes for the given style (on/off). and return the code sequence.
  std::string style(AnsiStyle style, std::string const& input);

  void initAnsiStyles();

  // Stream modifiers.
  std::ostream& clearScreen(std::ostream& stream);
  std::ostream& stylesReset(std::ostream& stream);

  std::ostream& styleBoldOn(std::ostream& stream);
  std::ostream& styleBoldOff(std::ostream& stream);
  std::ostream& styleDimOn(std::ostream& stream);
  std::ostream& styleDimOff(std::ostream& stream);
  std::ostream& styleItalicOn(std::ostream& stream);
  std::ostream& styleItalicOff(std::ostream& stream);
  std::ostream& styleUnderlineOn(std::ostream& stream);
  std::ostream& styleUnderlineOff(std::ostream& stream);
  std::ostream& styleInverseOn(std::ostream& stream);
  std::ostream& styleInverseOff(std::ostream& stream);
  std::ostream& styleHiddenOn(std::ostream& stream);
  std::ostream& styleHiddenOff(std::ostream& stream);
  std::ostream& styleStrikeThroughOn(std::ostream& stream);
  std::ostream& styleStrikeThroughOff(std::ostream& stream);
  std::ostream& styleBlackOn(std::ostream& stream);
  std::ostream& styleBlackOff(std::ostream& stream);
  std::ostream& styleRedOn(std::ostream& stream);
  std::ostream& styleRedOff(std::ostream& stream);
  std::ostream& styleGreenOn(std::ostream& stream);
  std::ostream& styleGreenOff(std::ostream& stream);
  std::ostream& styleYellowOn(std::ostream& stream);
  std::ostream& styleYellowOff(std::ostream& stream);
  std::ostream& styleBlueOn(std::ostream& stream);
  std::ostream& styleBlueOff(std::ostream& stream);
  std::ostream& styleMagentaOn(std::ostream& stream);
  std::ostream& styleMagentaOff(std::ostream& stream);
  std::ostream& styleCyanOn(std::ostream& stream);
  std::ostream& styleCyanOff(std::ostream& stream);
  std::ostream& styleWhiteOn(std::ostream& stream);
  std::ostream& styleWhiteOff(std::ostream& stream);
  std::ostream& styleGrayOn(std::ostream& stream);
  std::ostream& styleGrayOff(std::ostream& stream);
  std::ostream& styleBlackBackgroundOn(std::ostream& stream);
  std::ostream& styleBlackBackgroundOff(std::ostream& stream);
  std::ostream& styleRedBackgroundOn(std::ostream& stream);
  std::ostream& styleRedBackgroundOff(std::ostream& stream);
  std::ostream& styleGreenBackgroundOn(std::ostream& stream);
  std::ostream& styleGreenBackgroundOff(std::ostream& stream);
  std::ostream& styleYellowBackgroundOn(std::ostream& stream);
  std::ostream& styleYellowBackgroundOff(std::ostream& stream);
  std::ostream& styleBlueBackgroundOn(std::ostream& stream);
  std::ostream& styleBlueBackgroundOff(std::ostream& stream);
  std::ostream& styleMagentaBackgroundOn(std::ostream& stream);
  std::ostream& styleMagentaBackgroundOff(std::ostream& stream);
  std::ostream& styleCyanBackgroundOn(std::ostream& stream);
  std::ostream& styleCyanBackgroundOff(std::ostream& stream);
  std::ostream& styleWhiteBackgroundOn(std::ostream& stream);
  std::ostream& styleWhiteBackgroundOff(std::ostream& stream);
  std::ostream& styleGrayBackgroundOn(std::ostream& stream);
  std::ostream& styleGrayBackgroundOff(std::ostream& stream);

}
