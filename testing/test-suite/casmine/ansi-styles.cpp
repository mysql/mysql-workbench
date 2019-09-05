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

#include "ansi-styles.h"
#include "helpers.h"

namespace casmine {

static bool supportsColor = false;

typedef struct {
  std::string open;
  std::string close;
} CodePair;

static std::map<AnsiStyle, CodePair> ansiCodes = {
  { AnsiStyle::Reset, { "\u001b[0m", "\u001b[0m" }},
  { AnsiStyle::Bold, { "\u001b[1m", "\u001b[22m" }},
  { AnsiStyle::Dim, { "\u001b[2m", "\u001b[22m" }},
  { AnsiStyle::Italic, { "\u001b[3m", "\u001b[23m" }},
  { AnsiStyle::Underline, { "\u001b[4m", "\u001b[24m" }},
  { AnsiStyle::Inverse, { "\u001b[7m", "\u001b[27m" }},
  { AnsiStyle::Hidden, { "\u001b[8m", "\u001b[28m" }},
  { AnsiStyle::StrikeThrough, { "\u001b[9m", "\u001b[ 29m" }},

  { AnsiStyle::BlackForeground, { "\u001b[38;5;0m", "\u001b[39m" }},
  { AnsiStyle::RedForeground, { "\u001b[38;5;160m", "\u001b[39m" }},
  { AnsiStyle::GreenForeground, { "\u001b[38;5;76m", "\u001b[39m" }},
  { AnsiStyle::YellowForeground, { "\u001b[38;5;220m", "\u001b[39m" }},
  { AnsiStyle::BlueForeground, { "\u001b[38;5;38m", "\u001b[39m" }},
  { AnsiStyle::MagentaForeground, { "\u001b[38;5;200m", "\u001b[39m" }},
  { AnsiStyle::CyanForeground, { "\u001b[38;5;37m", "\u001b[39m" }},
  { AnsiStyle::WhiteForeground, { "\u001b[38;5;255m", "\u001b[39m" }},
  { AnsiStyle::GrayForeground, { "\u001b[38;5;251m", "\u001b[39m" }},

  { AnsiStyle::BlackBackground, { "\u001b[48;5;0m", "\u001b[49m" }},
  { AnsiStyle::RedBackground, { "\u001b[48;5;196m", "\u001b[49m" }},
  { AnsiStyle::GreenBackground, { "\u001b[48;5;112m", "\u001b[49m" }},
  { AnsiStyle::YellowBackground, { "\u001b[48;5;226m", "\u001b[49m" }},
  { AnsiStyle::BlueBackground, { "\u001b[48;5;39m", "\u001b[49m" }},
  { AnsiStyle::MagentaBackground, { "\u001b[48;5;200m", "\u001b[49m" }},
  { AnsiStyle::CyanBackground, { "\u001b[48;5;37m", "\u001b[49m" }},
  { AnsiStyle::WhiteBackground, { "\u001b[48;5;255m", "\u001b[49m" }},
  { AnsiStyle::GrayBackground, { "\u001b[48;5;251m", "\u001b[49m" }},
};

//----------------------------------------------------------------------------------------------------------------------

std::string style(AnsiStyle style, std::string const& input) {
  if (supportsColor)
    return ansiCodes[style].open + input + ansiCodes[style].close;
  return input;
}

//----------------------------------------------------------------------------------------------------------------------

void initAnsiStyles() {
  if (getEnvVar("CASMINE_NO_COLORS", "0") != "0")
    return;

  if (!isatty(fileno(stdout)))
    return;

#ifdef _MSC_VER

  #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
  #endif

  // Set output mode to handle virtual terminal sequences.
  HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  if (outputHandle == INVALID_HANDLE_VALUE)
    return;

  DWORD currentMode = 0;
  if (!GetConsoleMode(outputHandle, &currentMode))
    return;

  if (!SetConsoleMode(outputHandle, currentMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
    return;

  supportsColor = true;
  return;
#endif

  if (!getEnvVar("TERM_PROGRAM").empty()) {
    supportsColor = true;
    return;
  }

  std::string text = getEnvVar("TERM");
  std::string transformed;
  std::transform(text.begin(), text.end(), std::back_inserter(transformed), ::tolower);
  if (text.rfind("-256color") == text.size() - 9
    || text.rfind("-256") == text.size() - 4
    || text.rfind("screen") == text.size() - 6
    || text.find("xterm") == 0
    || text.find("vt100") == 0
    || text.find("color") != std::string::npos
    || text.find("ansi") != std::string::npos
    || text.find("cygwin") != std::string::npos
    || text.find("linux") != std::string::npos) {
    supportsColor = true;
    return;
  }

  if (!getEnvVar("COLORTERM").empty()) {
    supportsColor = true;
    return;
  }
}

//----------------------------------------------------------------------------------------------------------------------

#define STYLE_ON(style) if (supportsColor) {\
  std::string code = ansiCodes[AnsiStyle::style].open;\
  stream.write(code.c_str(), code.size());\
}

#define STYLE_OFF(style) if (supportsColor) {\
  std::string code = ansiCodes[AnsiStyle::style].close;\
  stream.write(code.c_str(), code.size());\
}

std::ostream& clearScreen(std::ostream& stream) { if (supportsColor) stream.write("\u001b[2J", 5); return stream; }
std::ostream& stylesReset(std::ostream& stream) { STYLE_ON(Reset); return stream; }

std::ostream& styleBoldOn(std::ostream& stream) { STYLE_ON(Bold); return stream; }
std::ostream& styleBoldOff(std::ostream& stream) { STYLE_OFF(Bold); return stream; }
std::ostream& styleDimOn(std::ostream& stream) { STYLE_ON(Dim); return stream; }
std::ostream& styleDimOff(std::ostream& stream) { STYLE_OFF(Dim); return stream; }
std::ostream& styleItalicOn(std::ostream& stream) { STYLE_ON(Italic); return stream; }
std::ostream& styleItalicOff(std::ostream& stream) { STYLE_OFF(Italic); return stream; }
std::ostream& styleUnderlineOn(std::ostream& stream) { STYLE_ON(Underline); return stream; }
std::ostream& styleUnderlineOff(std::ostream& stream) { STYLE_OFF(Underline); return stream; }
std::ostream& styleInverseOn(std::ostream& stream) { STYLE_ON(Inverse); return stream; }
std::ostream& styleInverseOff(std::ostream& stream) { STYLE_OFF(Inverse); return stream; }
std::ostream& styleHiddenOn(std::ostream& stream) { STYLE_ON(Hidden); return stream; }
std::ostream& styleHiddenOff(std::ostream& stream) { STYLE_OFF(Hidden); return stream; }
std::ostream& styleStrikeThroughOn(std::ostream& stream) { STYLE_ON(StrikeThrough); return stream; }
std::ostream& styleStrikeThroughOff(std::ostream& stream) { STYLE_OFF(StrikeThrough); return stream; }
std::ostream& styleBlackOn(std::ostream& stream) { STYLE_ON(BlackForeground); return stream; }
std::ostream& styleBlackOff(std::ostream& stream) { STYLE_OFF(BlackForeground); return stream; }
std::ostream& styleRedOn(std::ostream& stream) { STYLE_ON(RedForeground); return stream; }
std::ostream& styleRedOff(std::ostream& stream) { STYLE_OFF(RedForeground); return stream; }
std::ostream& styleGreenOn(std::ostream& stream) { STYLE_ON(GreenForeground); return stream; }
std::ostream& styleGreenOff(std::ostream& stream) { STYLE_OFF(GreenForeground); return stream; }
std::ostream& styleYellowOn(std::ostream& stream) { STYLE_ON(YellowForeground); return stream; }
std::ostream& styleYellowOff(std::ostream& stream) { STYLE_OFF(YellowForeground); return stream; }
std::ostream& styleBlueOn(std::ostream& stream) { STYLE_ON(BlueForeground); return stream; }
std::ostream& styleBlueOff(std::ostream& stream) { STYLE_OFF(BlueForeground); return stream; }
std::ostream& styleMagentaOn(std::ostream& stream) { STYLE_ON(MagentaForeground); return stream; }
std::ostream& styleMagentaOff(std::ostream& stream) { STYLE_OFF(MagentaForeground); return stream; }
std::ostream& styleCyanOn(std::ostream& stream) { STYLE_ON(CyanForeground); return stream; }
std::ostream& styleCyanOff(std::ostream& stream) { STYLE_OFF(CyanForeground); return stream; }
std::ostream& styleWhiteOn(std::ostream& stream) { STYLE_ON(WhiteForeground); return stream; }
std::ostream& styleWhiteOff(std::ostream& stream) { STYLE_OFF(WhiteForeground); return stream; }
std::ostream& styleGrayOn(std::ostream& stream) { STYLE_ON(GrayForeground); return stream; }
std::ostream& styleGrayOff(std::ostream& stream) { STYLE_OFF(GrayForeground); return stream; }
std::ostream& styleBlackBackgroundOn(std::ostream& stream) { STYLE_ON(BlackBackground); return stream; }
std::ostream& styleBlackBackgroundOff(std::ostream& stream) { STYLE_OFF(BlackBackground); return stream; }
std::ostream& styleRedBackgroundOn(std::ostream& stream) { STYLE_ON(RedBackground); return stream; }
std::ostream& styleRedBackgroundOff(std::ostream& stream) { STYLE_OFF(RedBackground); return stream; }
std::ostream& styleGreenBackgroundOn(std::ostream& stream) { STYLE_ON(GreenBackground); return stream; }
std::ostream& styleGreenBackgroundOff(std::ostream& stream) { STYLE_OFF(GreenBackground); return stream; }
std::ostream& styleYellowBackgroundOn(std::ostream& stream) { STYLE_ON(YellowBackground); return stream; }
std::ostream& styleYellowBackgroundOff(std::ostream& stream) { STYLE_OFF(YellowBackground); return stream; }
std::ostream& styleBlueBackgroundOn(std::ostream& stream) { STYLE_ON(BlueBackground); return stream; }
std::ostream& styleBlueBackgroundOff(std::ostream& stream) { STYLE_OFF(BlueBackground); return stream; }
std::ostream& styleMagentaBackgroundOn(std::ostream& stream) { STYLE_ON(MagentaBackground); return stream; }
std::ostream& styleMagentaBackgroundOff(std::ostream& stream) { STYLE_OFF(MagentaBackground); return stream; }
std::ostream& styleCyanBackgroundOn(std::ostream& stream) { STYLE_ON(CyanBackground); return stream; }
std::ostream& styleCyanBackgroundOff(std::ostream& stream) { STYLE_OFF(CyanBackground); return stream; }
std::ostream& styleWhiteBackgroundOn(std::ostream& stream) { STYLE_ON(WhiteBackground); return stream; }
std::ostream& styleWhiteBackgroundOff(std::ostream& stream) { STYLE_OFF(WhiteBackground); return stream; }
std::ostream& styleGrayBackgroundOn(std::ostream& stream) { STYLE_ON(GrayBackground); return stream; }
std::ostream& styleGrayBackgroundOff(std::ostream& stream) { STYLE_OFF(GrayBackground); return stream; }

}
