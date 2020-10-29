/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <unordered_set>

#include "base/log.h"
#include "base/drawing.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/notifications.h"
#include "base/drawing.h"
#include "base/xml_functions.h"

#include "SciLexer.h"

#include "mforms/mforms.h"
#include "mforms/utilities.h"

#include "mysql/MySQLRecognizerCommon.h"
#include "SymbolTable.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_BE)

using namespace mforms;
using namespace base;

// Marker ID assignments. Markers with higher number overlay lower ones.
// Note: the order here matches the LineMarkup enum, so we can directly use the enum as marker flags.
//       The LineMarkup is a bitmask (so we can create what Scintilla calls a marker set).
#define CE_STATEMENT_MARKER 0
#define CE_ERROR_MARKER 1
#define CE_BREAKPOINT_MARKER 2
#define CE_BREAKPOINT_HIT_MARKER 3
#define CE_CURRENT_LINE_MARKER 4
#define CE_ERROR_CONTINUE_MARKER 5

#define AC_LIST_SEPARATOR '\x19' // Unused codes as separators.
#define AC_TYPE_SEPARATOR '\x18'

#define ERROR_INDICATOR INDIC_CONTAINER
#define ERROR_INDICATOR_VALUE 42 // Arbitrary value.

typedef struct {
  bool hiresImage;
  int width;
  int height;
  void *data;
} ImageRecord;
static std::map<std::string, ImageRecord> registeredImages; // Registered RGBA images used in all editors.

//----------------- CodeEditorConfig -----------------------------------------------------------------------------------

CodeEditorConfig::CodeEditorConfig(SyntaxHighlighterLanguage language) {
  _used_language = language;
  _xmlDocument = nullptr;
  _xmlLanguageElement = nullptr;

  std::string lexer;
  std::string override_lexer;
  switch (language) {
    case mforms::LanguageMySQL56:
      override_lexer = "SCLEX_MYSQL_56";
      lexer = "SCLEX_MYSQL";
      break;

    case mforms::LanguageMySQL57:
      override_lexer = "SCLEX_MYSQL_57";
      lexer = "SCLEX_MYSQL";
      break;

    case mforms::LanguageMySQL80:
      override_lexer = "SCLEX_MYSQL_80";
      lexer = "SCLEX_MYSQL";
      break;

    case mforms::LanguageHtml:
      lexer = "SCLEX_HTML";
      break;

    case mforms::LanguagePython:
      lexer = "SCLEX_PYTHON";
      break;

    case mforms::LanguageCpp:
      lexer = "SCLEX_CPP";
      break;

    case mforms::LanguageJS:
      lexer = "SCLEX_CPP";
      override_lexer = "SCLEX_CPP_JS";
      break;

    case mforms::LanguageJson:
      lexer = "SCLEX_CPP";
      override_lexer = "SCLEX_CPP_JSON";
      break;

    default:
      return;
  }

  // Load the user's config file if it exists, otherwise use the default one.
  std::string config_file =
    mforms::Utilities::get_special_folder(mforms::ApplicationData) + "/MySQL/Workbench/code_editor.xml";

  if (!base::file_exists(config_file))
    config_file = App::get()->get_resource_path("") + "/data/code_editor.xml";

  try {
    _xmlDocument = base::xml::loadXMLDoc(config_file);
    if (_xmlDocument == nullptr) {
      logError("Code Editor Config: cannot load configuration file \"%s\"\n", config_file.c_str());
      return;
    }
  } catch (std::runtime_error& re) {
    logError("Code Editor Config: cannot load configuration file \"%s\":\n\t%s \n", config_file.c_str(), re.what());
    return;
  }

  auto rootElement = base::xml::getXmlRoot(_xmlDocument);
  if (!base::xml::nameIs(rootElement, "languages")) {
    logError("Code Editor: invalid configuration file \"%s\"\n", config_file.c_str());
    return;
  }

  {
    auto current = rootElement->children;

    // Load the available language identifiers. All remaining values are loaded on demand.
    while (current != nullptr) {
      if (base::xml::nameIs(current, "language")) {
        std::string languageName = base::xml::getProp(current, "name");
        if (languageName == lexer)
          _xmlLanguageElement = current;
        _languages.push_back(languageName);
      }
      current = current->next;
    }
  }

  if (_xmlLanguageElement == nullptr) {
    logWarning(
      "Code Editor: could not find settings for language %s in configuration file "
      "\"%s\"\n",
      lexer.c_str(), config_file.c_str());
    return;
  }

  parse_properties();
  parse_settings();
  parse_keywords();
  parse_styles();

  // check if there's another config section containing values that should override the base one
  if (!override_lexer.empty() && override_lexer != lexer) {
    bool found = false;
    // Load the available language identifiers. All remaining values are loaded on demand.

    auto current = rootElement->children;
    while (current != nullptr) {
      if (base::xml::nameIs(current, "language")) {
        std::string languageName = base::xml::getProp(current, "name");
        if (languageName == override_lexer) {
          _xmlLanguageElement = current;
          found = true;
          break;
        }
      }
      current = current->next;
    }

    if (found) {
      parse_properties();
      parse_settings();
      parse_keywords();
      parse_styles();
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

CodeEditorConfig::~CodeEditorConfig() {
  if (_xmlDocument != nullptr)
    xmlFree(_xmlDocument);
  _xmlDocument = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditorConfig::parse_properties() {
  auto current = _xmlLanguageElement->children;
  while (current != nullptr) {
    if (base::xml::nameIs(current, "property")) {
      std::string pName = base::xml::getProp(current, "name");
      std::string pValue = base::xml::getProp(current, "value");
      if (!pName.empty() && !pValue.empty())
        _properties[pName] = pValue;
    }
    current = current->next;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditorConfig::parse_settings() {
  auto current = _xmlLanguageElement->children;
  while (current != nullptr) {
    if (base::xml::nameIs(current, "setting")) {
      std::string pName = base::xml::getProp(current, "name");
      std::string pValue = base::xml::getProp(current, "value");
      if (!pName.empty() && !pValue.empty())
        _settings[pName] = pValue;
    }
    current = current->next;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditorConfig::parse_keywords() {
  auto current = _xmlLanguageElement->children;
  while (current != nullptr) {
    if (base::xml::nameIs(current, "keywords")) {
      std::string pName = base::xml::getProp(current, "name");
      std::string text = base::xml::getContentRecursive(current);
      if (!pName.empty() && !text.empty())
        _keywords[pName] = text;
    }
    current = current->next;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditorConfig::parse_styles() {
  auto current = _xmlLanguageElement->children;
  while (current != nullptr) {
    if (base::xml::nameIs(current, "style")) {
      int id = (int)std::strtol(base::xml::getProp(current, "id").c_str(), nullptr, 10);
      if (id < 0) {
        current = current->next;
        continue;
      }

      std::map<std::string, std::string> entries;
      auto attribute = current->properties;
      while (attribute != nullptr) {
        if (base::xml::nameIs(attribute, "id")) {
          attribute = attribute->next;
          continue;
        }

        entries[std::string((const char*)attribute->name)] = base::xml::getContent(attribute->children);
        attribute = attribute->next;
      }
      _styles[id] = entries;
    }
    current = current->next;
  }
}

//----------------- CodeEditor -----------------------------------------------------------------------------------------

CodeEditor::CodeEditor(void* host, bool showInfo) : _host(host) {
  _code_editor_impl = &ControlFactory::get_instance()->_code_editor_impl;

  _code_editor_impl->create(this, showInfo);
  _code_editor_impl->send_editor(this, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
  _context_menu = NULL;
  _find_panel = NULL;
  _scroll_on_resize = true;
  _auto_indent = false;

  scoped_connect(Form::main_form()->signal_deactivated(), std::bind(&CodeEditor::auto_completion_cancel, this));
  base::NotificationCenter::get()->add_observer(this, "GNColorsChanged");

  // Load the marker images for folding, debugging, error marker etc.
  setupMarker(CE_STATEMENT_MARKER, "editor_statement.png");
  setupMarker(CE_ERROR_MARKER, "editor_error.png");
  setupMarker(CE_BREAKPOINT_MARKER, "editor_breakpoint.png");
  setupMarker(CE_BREAKPOINT_HIT_MARKER, "editor_breakpoint_hit.png");
  setupMarker(CE_CURRENT_LINE_MARKER, "editor_current_pos.png");
  setupMarker(CE_ERROR_CONTINUE_MARKER, "editor_continue_on_error.png");

  // Determine fixed settings that don't change with the OS appearance.
  // Margin 0: line numbers.
  _code_editor_impl->send_editor(this, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
#if defined(__APPLE__)
  _code_editor_impl->send_editor(this, SCI_STYLESETSIZE, STYLE_LINENUMBER, static_cast<sptr_t>(10));
#else
  _code_editor_impl->send_editor(this, SCI_STYLESETSIZE, STYLE_LINENUMBER, static_cast<sptr_t>(8));
#endif
  sptr_t lineNumberStyleWidth =
  _code_editor_impl->send_editor(this, SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t) "_9999");
  _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 0, lineNumberStyleWidth);
  _code_editor_impl->send_editor(this, SCI_SETMARGINSENSITIVEN, 0, 0);

  // Margin 1: all kind of markers (debugging, current line, code line indicator etc.).
  _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 1, 16);
  _code_editor_impl->send_editor(this, SCI_SETMARGINSENSITIVEN, 1, 1);

  // Margin 2: folding.
  _code_editor_impl->send_editor(this, SCI_SETPROPERTY, (uptr_t) "fold", (sptr_t) "1");
  _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 2, 13);
  _code_editor_impl->send_editor(this, SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CHANGE, 0);
  _code_editor_impl->send_editor(this, SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
  _code_editor_impl->send_editor(this, SCI_SETMARGINSENSITIVEN, 2, 1); // Margin is clickable.

  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_CIRCLEMINUS);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_CIRCLEPLUS);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNERCURVE);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_CIRCLEPLUSCONNECTED);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_CIRCLEMINUSCONNECTED);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNERCURVE);

  // Margin 3: empty, to provide some spacing between the folder margin and the text.
  _code_editor_impl->send_editor(this, SCI_SETMARGINTYPEN, 3, SC_MARGIN_BACK);
  _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 3, 5);
  _code_editor_impl->send_editor(this, SCI_SETMARGINSENSITIVEN, 3, 0);

  // Init markers & indicators for highlighting of syntax errors.
  _code_editor_impl->send_editor(this, SCI_INDICSETUNDER, ERROR_INDICATOR, 1);
  _code_editor_impl->send_editor(this, SCI_INDICSETSTYLE, ERROR_INDICATOR, INDIC_SQUIGGLE);

  // Other settings.
  _code_editor_impl->send_editor(this, SCI_SETEXTRAASCENT, 3, 0);
  _code_editor_impl->send_editor(this, SCI_SETEXTRADESCENT, 3, 0);

  _code_editor_impl->send_editor(this, SCI_SETCARETLINEVISIBLE, 1, 0);
  _code_editor_impl->send_editor(this, SCI_SETCARETWIDTH, 2, 0);

  // - Tabulators + indentation
  _code_editor_impl->send_editor(this, SCI_SETTABINDENTS, 1, 0);
  _code_editor_impl->send_editor(this, SCI_SETBACKSPACEUNINDENTS, 1, 0);

  // - Call tips
  _code_editor_impl->send_editor(this, SCI_SETMOUSEDWELLTIME, 200, 0);

  // - Line ending + scrolling.
  _code_editor_impl->send_editor(this, SCI_SETSCROLLWIDTHTRACKING, 1, 0);
  _code_editor_impl->send_editor(this, SCI_SETEOLMODE, SC_EOL_LF, 0);

  // - Auto completion
  _code_editor_impl->send_editor(this, SCI_AUTOCSETSEPARATOR, AC_LIST_SEPARATOR, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETTYPESEPARATOR, AC_TYPE_SEPARATOR, 0);

  updateColors();
}

//----------------------------------------------------------------------------------------------------------------------

CodeEditor::~CodeEditor() {
  base::NotificationCenter::get()->remove_observer(this);

  delete _find_panel;
  auto_completion_cancel();
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::updateColors() {
  bool darkMode = App::get()->isDarkModeActive();

  base::Color color = base::Color::getSystemColor(base::TextBackgroundColor);
  long mainBackground = color.toBGR();
  _code_editor_impl->send_editor(this, SCI_STYLESETBACK, STYLE_DEFAULT, mainBackground);

  // Propagate the new background color to all editor text styles.
  for (int i = 0; i < STYLE_DEFAULT; ++i)
    _code_editor_impl->send_editor(this, SCI_STYLESETBACK, i, mainBackground);
  for (int i = STYLE_LASTPREDEFINED + 1; i < STYLE_MAX; ++i)
    _code_editor_impl->send_editor(this, SCI_STYLESETBACK, i, mainBackground);

  for (int n = SC_MARKNUM_FOLDEREND; n <= SC_MARKNUM_FOLDEROPEN; ++n) {
    _code_editor_impl->send_editor(this, SCI_MARKERSETFORE, n, mainBackground); // Front/back meaning is reversed here.
    _code_editor_impl->send_editor(this, SCI_MARKERSETBACK, n, 0xA0B0B0);
  }

  _code_editor_impl->send_editor(this, SCI_STYLESETFORE, STYLE_LINENUMBER, darkMode ? 0x808080 : 0x907522);
  _code_editor_impl->send_editor(this, SCI_STYLESETBACK, STYLE_LINENUMBER, mainBackground);

  _code_editor_impl->send_editor(this, SCI_INDICSETFORE, ERROR_INDICATOR, darkMode ? 0x281FFF : 0x2119D0);

  // Other settings.
  color = Color::getSystemColor(base::SelectedTextBackgroundColor);
  _code_editor_impl->send_editor(this, SCI_SETSELBACK, 1, color.toBGR());
#ifndef __APPLE__
  _code_editor_impl->send_editor(this, SCI_SETSELALPHA, 64, 0);
#endif

  if (darkMode) {
    _code_editor_impl->send_editor(this, SCI_SETCARETLINEBACK, 0xFFFFFF, 0);
    _code_editor_impl->send_editor(this, SCI_SETCARETLINEBACKALPHA, 0x38, 0);
    _code_editor_impl->send_editor(this, SCI_SETCARETFORE, 0xAEAEAE, 0);
  } else {
    _code_editor_impl->send_editor(this, SCI_SETCARETLINEBACK, 0x000000, 0);
    _code_editor_impl->send_editor(this, SCI_SETCARETLINEBACKALPHA, 0x10, 0);
    _code_editor_impl->send_editor(this, SCI_SETCARETFORE, 0x404040, 0);
  }

  // - Call tips
  color = base::Color::getSystemColor(base::TextColor);
  _code_editor_impl->send_editor(this, SCI_CALLTIPSETFORE, color.toBGR(), 0);
  _code_editor_impl->send_editor(this, SCI_CALLTIPSETBACK, darkMode ? 0x404040 : 0xF8F8F8, 0);

  auto applyStyle = [&](int id, auto &values) {
    std::string value = darkMode ? values["fore-color-dark"] : values["fore-color-light"];
    if (value.empty())
      value = values["fore-color"]; // Backwards compatibility.

    if (!value.empty()) {
      Color color = Color::parse(value);
      _code_editor_impl->send_editor(this, SCI_STYLESETFORE, id, color.toBGR());
    }

    value = darkMode ? values["back-color-dark"] : values["back-color-light"];
    if (!value.empty()) {
      Color color = Color::parse(value);
      _code_editor_impl->send_editor(this, SCI_STYLESETBACK, id, color.toBGR());
    }

    value = base::tolower(values["bold"]);
    if (!value.empty()) {
      bool flag = value == "1" || value == "yes" || value == "true";
      _code_editor_impl->send_editor(this, SCI_STYLESETBOLD, id, flag);
    }

    value = base::tolower(values["italic"]);
    if (!value.empty()) {
      bool flag = value == "1" || value == "yes" || value == "true";
      _code_editor_impl->send_editor(this, SCI_STYLESETITALIC, id, flag);
    }
  };

  // First load the default style and apply that to all text style entries. Then loop again for individual settings.
  for (auto &style : _currentStyles) {
    if (style.first == 0) {
      for (int i = 0; i < STYLE_DEFAULT; ++i)
        applyStyle(i, style.second);
      for (int i = STYLE_LASTPREDEFINED + 1; i < STYLE_MAX; ++i)
        applyStyle(i, style.second);
      break;
    }
  }

  for (auto &style : _currentStyles) {
    if (style.first == 0)
      continue;
    applyStyle(style.first, style.second);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::setWidth(EditorMargin margin, int size, const std::string& adjustText) {
  if (!adjustText.empty())
    size = (int)_code_editor_impl->send_editor(this, SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t)adjustText.c_str());
  switch (margin) {
    case LineNumberMargin:
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 0, size);
      break;
    case MarkersMargin:
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 1, size);
      break;
    case FolderMargin:
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 2, size);
      break;
    case TextMargin:
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 3, size);
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::setColor(EditorMargin margin, base::Color color, bool foreground) {
  switch (margin) {
    case FolderMargin:
    case LineNumberMargin:
    case TextMargin:
      if (foreground)
        _code_editor_impl->send_editor(this, SCI_STYLESETFORE, STYLE_LINENUMBER, color.toRGB());
      else
        _code_editor_impl->send_editor(this, SCI_STYLESETBACK, STYLE_LINENUMBER, color.toRGB());
      break;
    case MarkersMargin:
      for (int n = 25; n < 32; ++n) { // Markers 25..31 are reserved for folding.
        if (foreground)
          _code_editor_impl->send_editor(this, SCI_MARKERSETFORE, n, color.toRGB());
        else
          _code_editor_impl->send_editor(this, SCI_MARKERSETBACK, n, color.toRGB());
      }
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::showMargin(EditorMargin margin, bool show) {
  sptr_t size = 0;
  const sptr_t defaultSize = 16;
  switch (margin) {
    case LineNumberMargin:
      if (!show)
        _marginSize.margin1 = _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 0, 0);
      else {
        size = _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 0, 0);
        if (size > 0)
          break;
        size = _marginSize.margin1 > 0 ? _marginSize.margin1 : defaultSize;
      }
      break;
    case MarkersMargin:
      if (!show)
        _marginSize.margin2 = _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 1, 0);
      else {
        size = _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 1, 0);
        if (size > 0)
          break;
        size = _marginSize.margin2 > 0 ? _marginSize.margin2 : defaultSize;
      }
      break;
    case FolderMargin:
      if (!show)
        _marginSize.margin3 = _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 2, 0);
      else {
        size = _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 2, 0);
        if (size > 0)
          break;
        size = _marginSize.margin3 > 0 ? _marginSize.margin3 : defaultSize;
      }
      break;
    case TextMargin:
      if (!show)
        _marginSize.margin4 = _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 3, 0);
      else {
        size = _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 3, 0);
        if (size > 0)
          break;
        size = _marginSize.margin4 > 0 ? _marginSize.margin4 : defaultSize;
      }
      break;
    default:
      break;
  }
  if (!show)
    setWidth(margin, 0);
  else
    setWidth(margin, (int)size);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::setMarginText(const std::string& str) {
  setMarginText(str, line_count() - 1);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::setScrollWidth(size_t width) {
  _code_editor_impl->send_editor(this, SCI_SETSCROLLWIDTH, width, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::setMarginText(const std::string& str, size_t line) {
  sptr_t size = _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 3, 0);
  sptr_t lineNumberStyleWidth =
    _code_editor_impl->send_editor(this, SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t)str.c_str());
  if (lineNumberStyleWidth > size)
    _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 3, lineNumberStyleWidth);

  _code_editor_impl->send_editor(this, SCI_MARGINSETTEXT, line, (sptr_t)str.c_str());
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_text(const char* text) {
  _code_editor_impl->send_editor(this, SCI_SETTEXT, 0, (sptr_t)text);
}

//----------------------------------------------------------------------------------------------------------------------

int CodeEditor::getLineHeight(int line) {
  return (int)_code_editor_impl->send_editor(this, SCI_TEXTHEIGHT, line, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_value(const std::string& value) {
  // When passing text as std::string we have a length and can hence use a different
  // way to set the text in the control, which preserves embedded nulls.
  _code_editor_impl->send_editor(this, SCI_CLEARALL, 0, 0);
  _code_editor_impl->send_editor(this, SCI_APPENDTEXT, value.size(), (sptr_t)value.c_str());
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_text_keeping_state(const char* text) {
  sptr_t caret_position = _code_editor_impl->send_editor(this, SCI_GETCURRENTPOS, 0, 0);
  sptr_t selection_start = _code_editor_impl->send_editor(this, SCI_GETSELECTIONSTART, 0, 0);
  sptr_t selection_end = _code_editor_impl->send_editor(this, SCI_GETSELECTIONEND, 0, 0);
  sptr_t first_line = _code_editor_impl->send_editor(this, SCI_GETFIRSTVISIBLELINE, 0, 0);

  _code_editor_impl->send_editor(this, SCI_SETTEXT, 0, (sptr_t)text);

  _code_editor_impl->send_editor(this, SCI_SETCURRENTPOS, caret_position, 0);
  _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, selection_start, 0);
  _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND, selection_end, 0);
  _code_editor_impl->send_editor(this, SCI_SETFIRSTVISIBLELINE, first_line, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::append_text(const char* text, size_t length) {
  _code_editor_impl->send_editor(this, SCI_APPENDTEXT, length, (sptr_t)text);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::replace_selected_text(const std::string& text) {
  std::size_t start, length;
  get_selection(start, length);
  _code_editor_impl->send_editor(this, SCI_REPLACESEL, 0, (sptr_t)text.c_str());

  _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, start + text.length(), 0);
  _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND, start + text.length(), 0);
}

//----------------------------------------------------------------------------------------------------------------------

const std::string CodeEditor::get_text(bool selection_only) {
  char* text = nullptr;
  sptr_t length;
  if (selection_only) {
    length = _code_editor_impl->send_editor(this, SCI_GETSELTEXT, 0, 0);
    if (length > 0) {
      text = (char*)malloc(length);
    }
    if (text != nullptr) // Can be null if not memory is available.
      _code_editor_impl->send_editor(this, SCI_GETSELTEXT, length, (sptr_t)text);
  } else {
    length = _code_editor_impl->send_editor(this, SCI_GETTEXTLENGTH, 0, 0) + 1;
    if (length > 0) {
      text = (char*)malloc(length);
    }
    if (text != nullptr)
      _code_editor_impl->send_editor(this, SCI_GETTEXT, length, (sptr_t)text);
  }
  if (text != nullptr) {
    std::string result(text, length - 1);
    free(text);

    return result;
  }
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

const std::string CodeEditor::get_text_in_range(size_t start, size_t end) {
  Sci_TextRange range;

  range.chrg.cpMin = (long)start;

  sptr_t length = _code_editor_impl->send_editor(this, SCI_GETTEXTLENGTH, 0, 0);
  range.chrg.cpMax = (long)(end > start + length ? length - start : end);

  range.lpstrText = (char*)malloc(end - start + 1); // Don't forget the 0 terminator.
  _code_editor_impl->send_editor(this, SCI_GETTEXTRANGE, 0, (sptr_t)&range);

  if (range.lpstrText == NULL)
    return std::string();

  std::string result(range.lpstrText, end - start);
  free(range.lpstrText);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

std::pair<const char*, std::size_t> CodeEditor::get_text_ptr() {
  std::pair<const char*, std::size_t> result;
  result.first = reinterpret_cast<const char *>(_code_editor_impl->send_editor(this, SCI_GETCHARACTERPOINTER, 0, 0));
  result.second = _code_editor_impl->send_editor(this, SCI_GETTEXTLENGTH, 0, 0);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_selection(std::size_t start, std::size_t length) {
  _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, start, 0);
  _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND, start + length, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::clear_selection() {
  sptr_t current_pos = _code_editor_impl->send_editor(this, SCI_GETCURRENTPOS, 0, 0);
  _code_editor_impl->send_editor(this, SCI_SETEMPTYSELECTION, current_pos, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::get_selection(std::size_t& start, std::size_t& length) {
  start = _code_editor_impl->send_editor(this, SCI_GETSELECTIONSTART, 0, 0);
  length = _code_editor_impl->send_editor(this, SCI_GETSELECTIONEND, 0, 0) - start;
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::get_range_of_line(ssize_t line, ssize_t& start, ssize_t& end) {
  start = _code_editor_impl->send_editor(this, SCI_POSITIONFROMLINE, line, 0);
  end = _code_editor_impl->send_editor(this, SCI_GETLINEENDPOSITION, line, 0);

  return start < 0 || end < 0;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::setupMarker(int marker, const std::string& name) {
  if (base::hasSuffix(name, ".xpm")) {
    std::string path = App::get()->get_resource_path(name);

    gchar* content = NULL;
    if (g_file_get_contents(path.c_str(), &content, NULL, NULL)) {
      _code_editor_impl->send_editor(this, SCI_MARKERDEFINEPIXMAP, marker, (sptr_t)content);
      g_free(content);
    }
  } else { // png
    if (ensureImage(name)) {
      auto &data = registeredImages[name];
      _code_editor_impl->send_editor(this, SCI_RGBAIMAGESETWIDTH, data.width, 0);
      _code_editor_impl->send_editor(this, SCI_RGBAIMAGESETHEIGHT, data.height, 0);
      _code_editor_impl->send_editor(this, SCI_RGBAIMAGESETSCALE, data.hiresImage ? 200 : 100, 0);
      _code_editor_impl->send_editor(this, SCI_MARKERDEFINERGBAIMAGE, marker, reinterpret_cast<sptr_t>(data.data));
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Called before lines are removed. Need to record disappearing markers.
 */
void CodeEditor::handleMarkerDeletion(size_t position, size_t length) {
  if (length == 0)
    return;

  LineMarkupChangeset changeset;
  if (length == static_cast<size_t>(_code_editor_impl->send_editor(this, SCI_GETLENGTH, 0, 0))) {
    // Clean editor? Send empty changeset to signal that all markers can go.
    _marker_changed_event(changeset, true);
    return;
  }

  sptr_t currentLine = _code_editor_impl->send_editor(this, SCI_LINEFROMPOSITION, position, 0);
  sptr_t endLine = _code_editor_impl->send_editor(this, SCI_LINEFROMPOSITION, position + length - 1, 0);

  // Don't include the first line as Scintilla merges current and last line markers, so we have to update them.
  ++currentLine;

  currentLine = _code_editor_impl->send_editor(this, SCI_MARKERNEXT, currentLine, LineMarkupAll);

  while (currentLine > -1 && currentLine <= endLine) {
    LineMarkup markup = (LineMarkup)_code_editor_impl->send_editor(this, SCI_MARKERGET, currentLine, LineMarkupAll);
    LineMarkupChangeEntry entry = {(int)currentLine, 0, markup};
    changeset.push_back(entry);

    // MARKERNEXT effectively searches lines for the given markers, the docs say.
    currentLine = _code_editor_impl->send_editor(this, SCI_MARKERNEXT, currentLine + 1, LineMarkupAll);
  }

  if (!changeset.empty())
    _marker_changed_event(changeset, true);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Called after an edit action took place. We have to record markers that got moved by that action.
 */
void CodeEditor::handleMarkerMove(Sci_Position position, Sci_Position linesAdded) {
  if (linesAdded == 0)
    return;

  LineMarkupChangeset changeset;
  sptr_t currentLine = _code_editor_impl->send_editor(this, SCI_LINEFROMPOSITION, position, 0);

  if (linesAdded < 0) {
    // Lines have been deleted and markers merged. Since we don't know which have been combined remove them all
    // and send a separate deletion event for this case.
    _code_editor_impl->send_editor(this, SCI_MARKERDELETE, currentLine, -1);
    changeset.push_back({(int)currentLine, 0, LineMarkupAll});
    _marker_changed_event(changeset, true);
    changeset.clear();
  }

  // Ignore the first line if it has been edited after the line start.
  if (position > static_cast<Sci_Position>(_code_editor_impl->send_editor(this, SCI_POSITIONFROMLINE, currentLine, 0)))
    ++currentLine;

  currentLine = _code_editor_impl->send_editor(this, SCI_MARKERNEXT, currentLine, LineMarkupAll);
  while (currentLine > -1) {
    LineMarkup markup = (LineMarkup)_code_editor_impl->send_editor(this, SCI_MARKERGET, currentLine, LineMarkupAll);
    LineMarkupChangeEntry entry = {(int)(currentLine - linesAdded), (int)currentLine, markup};
    changeset.push_back(entry);

    currentLine = _code_editor_impl->send_editor(this, SCI_MARKERNEXT, currentLine + 1, LineMarkupAll);
  }

  if (!changeset.empty())
    _marker_changed_event(changeset, false);
}

//----------------------------------------------------------------------------------------------------------------------

char32_t CodeEditor::getCharAt(size_t position) {
  return static_cast<char32_t>(_code_editor_impl->send_editor(this, SCI_GETCHARAT, position, 0));
}

//----------------------------------------------------------------------------------------------------------------------

static const std::unordered_set<char32_t> braces = { '(', '{', '[', '<', ')', '}', ']', '>' };

void CodeEditor::updateBraceHighlighting() {
  size_t caretPos = get_caret_pos();
  ssize_t brace1Pos = INVALID_POSITION;
  ssize_t brace2Pos = INVALID_POSITION;

  // Highlight an opening brace regardless of the side the caret is, unless another opening brace follows directly
  // (and similary for closing braces).
  char32_t c = getCharAt(caretPos);

  if (braces.count(c) > 0)
    brace1Pos = caretPos;
  else if (caretPos > 0) {
    c = getCharAt(caretPos - 1);
    if (braces.count(c) > 0)
      brace1Pos = caretPos - 1;
  }

  if (brace1Pos > INVALID_POSITION) {
    brace2Pos = _code_editor_impl->send_editor(this, SCI_BRACEMATCH, brace1Pos, 0);
    if (brace2Pos == INVALID_POSITION)
      _code_editor_impl->send_editor(this, SCI_BRACEBADLIGHT, brace1Pos, 0);
    else
      _code_editor_impl->send_editor(this, SCI_BRACEHIGHLIGHT, brace1Pos, brace2Pos);
  } else
    _code_editor_impl->send_editor(this, SCI_BRACEHIGHLIGHT, brace1Pos, brace2Pos);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Ensures the given image is loaded from disk and stored in our image list.
 * Returns true if the image exists.
 */
bool CodeEditor::ensureImage(std::string const& name) {
  if (registeredImages.find(name) != registeredImages.end())
    return true;

  cairo_surface_t* image = Utilities::load_icon(name);
  if (image == nullptr)
    return false;

  if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(image);
    return false;
  }

  int width = cairo_image_surface_get_width(image);
  int height = cairo_image_surface_get_height(image);

  unsigned char* data = cairo_image_surface_get_data(image);

  // We not only need to keep the data around for the lifetime of the editor we also have
  // to swap blue and red.
  unsigned char *target = reinterpret_cast<unsigned char *>(malloc(4 * width * height));
  if (target != nullptr) {
    ImageRecord entry = {
      Utilities::is_hidpi_icon(image),
      cairo_image_surface_get_width(image),
      cairo_image_surface_get_height(image),
      target
    };
    registeredImages[name] = entry;
    int j = 0;
    while (j < 4 * width * height) {
      target[j] = data[j + 2];
      target[j + 1] = data[j + 1];
      target[j + 2] = data[j];
      target[j + 3] = data[j + 3];
      j += 4;
    }
  }
  cairo_surface_destroy(image);

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::loadConfiguration(SyntaxHighlighterLanguage language) {
  CodeEditorConfig config(language);

  // Keywords.
  if (language == LanguageMySQL56 || language == LanguageMySQL57 || language == LanguageMySQL80) {
    MySQLVersion version;
    switch (language) {
      case LanguageMySQL56:
        version = MySQLVersion::MySQL56;
        break;
      case LanguageMySQL57:
        version = MySQLVersion::MySQL57;
        break;
      default:
        version = MySQLVersion::MySQL80;
        break;
    }

    auto &functions = MySQLSymbolInfo::systemFunctionsForVersion(version);
    std::string list;
    for (auto const& name : functions)
      list += base::tolower(name) + " "; // Keyword matching in Scintilla uses lower case compare.

    _code_editor_impl->send_editor(this, SCI_SETKEYWORDS, 3, (sptr_t)list.c_str());

    auto &keywords = MySQLSymbolInfo::keywordsForVersion(version);
    list = "";
    for (auto const& name : keywords)
      list += base::tolower(name) + " ";

    _code_editor_impl->send_editor(this, SCI_SETKEYWORDS, 1, (sptr_t)list.c_str());

  } else {
    std::map<std::string, std::string> keywords = config.get_keywords();

    // Key word list sets are from currently active lexer, so that must be set before calling here.
    sptr_t length = _code_editor_impl->send_editor(this, SCI_DESCRIBEKEYWORDSETS, 0, 0);
    if (length > 0) {
      char* keyword_sets = (char*)malloc(length + 1);
      _code_editor_impl->send_editor(this, SCI_DESCRIBEKEYWORDSETS, 0, (sptr_t)keyword_sets);
      std::vector<std::string> keyword_list_names = base::split(keyword_sets, "\n");
      free(keyword_sets);

      for (auto iterator : keywords) {
        std::string list_name = iterator.first;
        int list_index = base::index_of(keyword_list_names, list_name);
        if (list_index > -1)
          _code_editor_impl->send_editor(this, SCI_SETKEYWORDS, list_index, reinterpret_cast<sptr_t>(iterator.second.c_str()));
      }
    }
  }

  // Properties.
  for (auto const& property : config.get_properties()) {
    _code_editor_impl->send_editor(this, SCI_SETPROPERTY,
      reinterpret_cast<uptr_t>(property.first.c_str()), reinterpret_cast<sptr_t>(property.second.c_str()));
  }

  // Settings.
  for (auto const& setting : config.get_settings()) {
    if (setting.first == "usetabs") {
      int value = base::atoi<int>(setting.second, 0);
      _code_editor_impl->send_editor(this, SCI_SETUSETABS, value, 0);
    } else if (setting.first == "tabwidth") {
      int value = base::atoi<int>(setting.second, 0);
      _code_editor_impl->send_editor(this, SCI_SETTABWIDTH, value, 0);
    } else if (setting.first == "indentation") {
      int value = base::atoi<int>(setting.second, 0);
      _code_editor_impl->send_editor(this, SCI_SETINDENT, value, 0);
    }
  }

  // Styles.
  _currentStyles = config.get_styles();
  updateColors();
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::handle_notification(const std::string &name, void *sender, NotificationInfo &info) {
  if (name == "GNColorsChanged")
    updateColors();
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_language(SyntaxHighlighterLanguage language) {
  switch (language) {
    case mforms::LanguageMySQL56:
    case mforms::LanguageMySQL57:
    case mforms::LanguageMySQL80:
      _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_MYSQL, 0);
      break;

    case mforms::LanguageHtml:
      _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_HTML, 0);
      break;

    case mforms::LanguagePython:
      _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_PYTHON, 0);
      break;

    case mforms::LanguageCpp:
    case mforms::LanguageJS:
    case mforms::LanguageJson:
      _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_CPP, 0);
      break;

    default:
      // No (known) language. Syntax highlighting will be switched off.
      _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_NULL, 0);
      return;
  }

  loadConfiguration(language);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::show_markup(LineMarkup markup, size_t line) {
  // The marker mask contains one bit for each set marker (0..31).
  sptr_t marker_mask = _code_editor_impl->send_editor(this, SCI_MARKERGET, line, 0);
  sptr_t new_marker_mask = 0;
  if ((markup & mforms::LineMarkupStatement) != 0) {
    if ((marker_mask & LineMarkupStatement) == 0)
      new_marker_mask |= LineMarkupStatement;
  }
  if ((markup & mforms::LineMarkupErrorContinue) != 0) {
    if ((marker_mask & LineMarkupErrorContinue) == 0)
      new_marker_mask |= LineMarkupErrorContinue;
  }
  if ((markup & mforms::LineMarkupError) != 0) {
    if ((marker_mask & LineMarkupError) == 0)
      new_marker_mask |= LineMarkupError;
  }
  if ((markup & mforms::LineMarkupBreakpoint) != 0) {
    if ((marker_mask & LineMarkupBreakpoint) == 0)
      new_marker_mask |= LineMarkupBreakpoint;
  }
  if ((markup & mforms::LineMarkupBreakpointHit) != 0) {
    if ((marker_mask & LineMarkupBreakpointHit) == 0)
      new_marker_mask |= LineMarkupBreakpointHit;
  }
  if ((markup & mforms::LineMarkupCurrent) != 0) {
    if ((marker_mask & LineMarkupCurrent) == 0)
      new_marker_mask |= LineMarkupCurrent;
  }

  if (new_marker_mask != 0)
    _code_editor_impl->send_editor(this, SCI_MARKERADDSET, line, new_marker_mask);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::remove_markup(LineMarkup markup, ssize_t line) {
  if (markup == mforms::LineMarkupAll || line < 0) {
    if (line < 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETEALL, -1, 0);
    else
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, -1);
  } else {
    if ((markup & mforms::LineMarkupStatement) != 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, CE_STATEMENT_MARKER);
    if ((markup & mforms::LineMarkupError) != 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, CE_ERROR_MARKER);
    if ((markup & mforms::LineMarkupErrorContinue) != 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, CE_ERROR_CONTINUE_MARKER);
    if ((markup & mforms::LineMarkupBreakpoint) != 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, CE_BREAKPOINT_MARKER);
    if ((markup & mforms::LineMarkupBreakpointHit) != 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, CE_BREAKPOINT_HIT_MARKER);
    if ((markup & mforms::LineMarkupCurrent) != 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, CE_CURRENT_LINE_MARKER);
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::has_markup(LineMarkup markup, size_t line) {
  sptr_t markers = _code_editor_impl->send_editor(this, SCI_MARKERGET, line, 0);

  if ((markup & markers) != 0)
    return true;

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::show_indicator(RangeIndicator indicator, size_t start, size_t length) {
  // Scintilla supports a model that not only sets an indicator in a given range but additionally
  // assigns a value to this indicator. This is to allow drawing an indicator with different styles.
  // However, currently all values are drawn in the same style and it is neither clear if that will ever
  // change nor what the actual use is, given that you always can set different indicators for different styles.
  // Unfortunately, you cannot query an indicator as such, only its value, so we have to set a value too
  // even if that is quite useless.
  switch (indicator) {
    case mforms::RangeIndicatorError:
      _code_editor_impl->send_editor(this, SCI_SETINDICATORCURRENT, ERROR_INDICATOR, 0);
      _code_editor_impl->send_editor(this, SCI_SETINDICATORVALUE, ERROR_INDICATOR_VALUE, 0);
      _code_editor_impl->send_editor(this, SCI_INDICATORFILLRANGE, start, length);

      break;
    case mforms::RangeIndicatorNone: // No effect here. Only used to have "none" value for indicator tests.
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

RangeIndicator CodeEditor::indicator_at(size_t position) {
  sptr_t result = _code_editor_impl->send_editor(this, SCI_INDICATORVALUEAT, ERROR_INDICATOR, position);
  if (result == ERROR_INDICATOR_VALUE)
    return mforms::RangeIndicatorError;

  return mforms::RangeIndicatorNone;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::remove_indicator(RangeIndicator indicator, size_t start, size_t length) {
  switch (indicator) {
    case mforms::RangeIndicatorError:
      _code_editor_impl->send_editor(this, SCI_SETINDICATORCURRENT, ERROR_INDICATOR, 0);
      _code_editor_impl->send_editor(this, SCI_INDICATORCLEARRANGE, start, length);

      break;
    case mforms::RangeIndicatorNone: // No effect here. Only needed to have a "none" value for indicator tests.
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

size_t mforms::CodeEditor::line_count() {
  return _code_editor_impl->send_editor(this, SCI_GETLINECOUNT, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

size_t CodeEditor::text_length() {
  return _code_editor_impl->send_editor(this, SCI_GETLENGTH, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

size_t CodeEditor::position_from_line(size_t line_number) {
  return _code_editor_impl->send_editor(this, SCI_POSITIONFROMLINE, line_number, 0);
}

//----------------------------------------------------------------------------------------------------------------------

size_t CodeEditor::line_from_position(size_t position) {
  return _code_editor_impl->send_editor(this, SCI_LINEFROMPOSITION, position, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_font(const std::string& fontDescription) {
  // Set this font for all styles.
  std::string font;
  float size;
  bool bold;
  bool italic;

  // Bold and italic are ignored as they can be set by a highlighter style.
  if (base::parse_font_description(fontDescription, font, size, bold, italic)) {
#if !defined(_MSC_VER) && !defined(__APPLE__)
    // Scintilla requires the ! in front of the font name to interpret it as a pango/fontconfig font.
    if (!font.empty() && font[0] != '!')
      font = "!" + font;
#endif

    for (int i = 0; i < 128; i++) {
      _code_editor_impl->send_editor(this, SCI_STYLESETFONT, i, (sptr_t)font.c_str());
      _code_editor_impl->send_editor(this, SCI_STYLESETSIZE, i, (sptr_t)size);
    }
  }

  // Recompute the line number margin width if it is visible.
  sptr_t lineNumberStyleWidth = _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 0, 0);
  if (lineNumberStyleWidth > 0) {
    lineNumberStyleWidth = _code_editor_impl->send_editor(this, SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t) "_9999");
    _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 0, lineNumberStyleWidth);
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Converts scintilla key modifier codes to mforms codes.
 */
mforms::ModifierKey getModifiers(int scintilla_modifiers) {
  mforms::ModifierKey modifiers = mforms::ModifierNoModifier;
  if ((scintilla_modifiers & SCMOD_CTRL) == SCMOD_CTRL)
    modifiers = modifiers | mforms::ModifierControl;
  if ((scintilla_modifiers & SCMOD_ALT) == SCMOD_ALT)
    modifiers = modifiers | mforms::ModifierAlt;
  if ((scintilla_modifiers & SCMOD_SHIFT) == SCMOD_SHIFT)
    modifiers = modifiers | mforms::ModifierShift;

  return modifiers;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::on_notify(SCNotification* notification) {
  switch (notification->nmhdr.code) {
    case SCN_MARGINCLICK: {
      sptr_t line = _code_editor_impl->send_editor(this, SCI_LINEFROMPOSITION, notification->position, 0);

#ifndef __APPLE__
      // This handling is already implemented in ScintillaView.mm.
      if (notification->margin == 2) {
        // A click on the folder margin. Toggle the current line if possible.
        _code_editor_impl->send_editor(this, SCI_TOGGLEFOLD, line, 0);
      }
#endif

      mforms::ModifierKey modifiers = getModifiers(notification->modifiers);
      _gutter_clicked_event(notification->margin, (int)line, modifiers);
      break;
    }

    case SCN_MODIFIED: {
      // Check for markers that would get removed.
      // Usually doesn't come with any of the other notification types.
      if ((notification->modificationType & SC_MOD_BEFOREDELETE) != 0)
        handleMarkerDeletion(notification->position, notification->length);

      // Text insertion or removal.
      if ((notification->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) != 0) {
        handleMarkerMove(notification->position, notification->linesAdded);

        _change_event(notification->position, notification->length, notification->linesAdded,
                      (notification->modificationType & SC_MOD_INSERTTEXT) != 0);
      }

      break;
    }

    case SCN_AUTOCSELECTION:
      _auto_completion_event(mforms::AutoCompletionSelection, notification->position, notification->text);
      break;

    case SCN_AUTOCCANCELLED:
      _auto_completion_event(mforms::AutoCompletionCancelled, 0, "");
      break;

    case SCN_AUTOCCHARDELETED:
      _auto_completion_event(mforms::AutoCompletionCharDeleted, 0, "");
      break;

    case SCN_DWELLSTART:
      _dwell_event(true, notification->position, notification->x, notification->y);
      break;

    case SCN_DWELLEND:
      _dwell_event(false, 0, 0, 0);
      break;

    case SCN_UPDATEUI:
      switch (notification->updated) {
        case SC_UPDATE_CONTENT: // Contents, styling or markers have been changed.
          break;
        case SC_UPDATE_SELECTION: // Selection has been changed or the caret moved.
          updateBraceHighlighting();
          NotificationCenter::get()->send("GNTextSelectionChanged", this);
          break;
        case SC_UPDATE_V_SCROLL: // Scrolled vertically.
          break;
        case SC_UPDATE_H_SCROLL: // Scrolled horizontally.
          break;
      }
      break;

    case SCN_CHARADDED:
      _char_added_event(notification->ch);
      if (_auto_indent && notification->ch == '\n') {
        sptr_t pos = _code_editor_impl->send_editor(this, SCI_GETCURRENTPOS, 0, 0);
        sptr_t line = _code_editor_impl->send_editor(this, SCI_LINEFROMPOSITION, pos, 0);
        if (line > 0) {
          sptr_t indentation = _code_editor_impl->send_editor(this, SCI_GETLINEINDENTATION, line - 1, 0);
          if (indentation > 0) {
            // Switch off tabs for a moment. We don't want a mix of tabs and spaces auto inserted
            // and tabs mess up the new indentation.
            sptr_t use_tabs = _code_editor_impl->send_editor(this, SCI_GETUSETABS, 0, 0);
            _code_editor_impl->send_editor(this, SCI_SETUSETABS, 0, 0);

            _code_editor_impl->send_editor(this, SCI_SETLINEINDENTATION, line, indentation);
            _code_editor_impl->send_editor(this, SCI_GOTOPOS, pos + indentation, 0);

            _code_editor_impl->send_editor(this, SCI_SETUSETABS, use_tabs, 0);
          }
        }
      }
      break;

    case SCN_FOCUSIN:
      focus_changed();
      break;

    case SCN_FOCUSOUT:
      _signal_lost_focus(); // For validation, parsing etc.
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::on_command(int command) {
  // TODO: removal candidate.
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::key_event(KeyCode code, ModifierKey modifier, const std::string& text) {
  // Return true if the key event can be further processed by the sender.
  // Return false if it is handled in backend code.
  if (_key_event_signal.empty())
    return true;

  return *_key_event_signal(code, modifier, text);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_features(CodeEditorFeature features, bool flag) {
  if ((features & mforms::FeatureWrapText) != 0) {
    if (flag)
      _code_editor_impl->send_editor(this, SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
    else
      _code_editor_impl->send_editor(this, SCI_SETWRAPMODE, SC_WRAP_NONE, 0);
  }

  if ((features & mforms::FeatureGutter) != 0) {
    if (flag) {
      sptr_t width = _code_editor_impl->send_editor(this, SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t) "_9999");

      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 0, width); // line numbers
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 1, 16);    // markers
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 2, 16);    // fold markers
    } else {
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 0, 0);
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 1, 0);
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 2, 0);
    }
  }

  if ((features & mforms::FeatureReadOnly) != 0)
    _code_editor_impl->send_editor(this, SCI_SETREADONLY, flag, 0);

  if ((features & mforms::FeatureShowSpecial) != 0) {
    _code_editor_impl->send_editor(this, SCI_SETVIEWEOL, flag, 0);
    if (flag)
      _code_editor_impl->send_editor(this, SCI_SETVIEWWS, SCWS_VISIBLEALWAYS, 0);
    else
      _code_editor_impl->send_editor(this, SCI_SETVIEWWS, SCWS_INVISIBLE, 0);
  }

  if ((features & mforms::FeatureUsePopup) != 0)
    _code_editor_impl->send_editor(this, SCI_USEPOPUP, flag, 0);

  if ((features & mforms::FeatureConvertEolOnPaste) != 0)
    _code_editor_impl->send_editor(this, SCI_SETPASTECONVERTENDINGS, flag, 0);

  if ((features & mforms::FeatureScrollOnResize) != 0)
    _scroll_on_resize = true;

  if ((features & mforms::FeatureFolding) != 0)
    _code_editor_impl->send_editor(this, SCI_SETPROPERTY, (uptr_t) "fold", flag ? (sptr_t) "1" : (sptr_t) "0");

  if ((features & mforms::FeatureAutoIndent) != 0)
    _auto_indent = true;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::toggle_features(CodeEditorFeature features) {
  // Toggling a feature involves querying its current state which is sometimes not possible with a
  // single value, so instead of returning the current state and let the application call
  // set_features we do it internally with this toggle_features function.

  if ((features & mforms::FeatureWrapText) != 0) {
    if (_code_editor_impl->send_editor(this, SCI_GETWRAPMODE, 0, 0) == SC_WRAP_NONE)
      _code_editor_impl->send_editor(this, SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
    else
      _code_editor_impl->send_editor(this, SCI_SETWRAPMODE, SC_WRAP_NONE, 0);
  }

  if ((features & mforms::FeatureGutter) != 0)
    set_features(mforms::FeatureGutter, _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 0, 0) == 0);

  if ((features & mforms::FeatureReadOnly) != 0)
    set_features(mforms::FeatureReadOnly, _code_editor_impl->send_editor(this, SCI_GETREADONLY, 0, 0) == 0);

  if ((features & mforms::FeatureShowSpecial) != 0)
    set_features(mforms::FeatureShowSpecial, _code_editor_impl->send_editor(this, SCI_GETVIEWEOL, 0, 0) == 0);

  // if ((features & mforms::FeatureUsePopup) != 0)
  // Not searchable so we cannot toggle it.

  if ((features & mforms::FeatureConvertEolOnPaste) != 0)
    set_features(mforms::FeatureConvertEolOnPaste,
                 _code_editor_impl->send_editor(this, SCI_GETPASTECONVERTENDINGS, 0, 0) != 0);

  if ((features & mforms::FeatureScrollOnResize) != 0)
    _scroll_on_resize = !_scroll_on_resize;

  if ((features & mforms::FeatureFolding) != 0) {
    bool folding_enabled = _code_editor_impl->send_editor(this, SCI_GETPROPERTYINT, (uptr_t) "fold", 0) != 0;
    _code_editor_impl->send_editor(this, SCI_SETPROPERTY, (uptr_t) "fold",
                                   folding_enabled ? (sptr_t) "0" : (sptr_t) "1");
  }

  if ((features & mforms::FeatureAutoIndent) != 0)
    _auto_indent = !_auto_indent;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_read_only(bool flag) {
  _code_editor_impl->send_editor(this, SCI_SETREADONLY, flag, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::reset_undo_stack() {
  _code_editor_impl->send_editor(this, SCI_EMPTYUNDOBUFFER, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::reset_dirty() {
  _code_editor_impl->send_editor(this, SCI_SETSAVEPOINT, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::is_dirty() {
  return _code_editor_impl->send_editor(this, SCI_GETMODIFY, 0, 0) != 0;
}

//----------------------------------------------------------------------------------------------------------------------

size_t CodeEditor::get_caret_pos() {
  return _code_editor_impl->send_editor(this, SCI_GETCURRENTPOS, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_caret_pos(size_t position) {
  _code_editor_impl->send_editor(this, SCI_GOTOPOS, position, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::get_line_column_pos(size_t position, size_t& line, size_t& column) {
  line = _code_editor_impl->send_editor(this, SCI_LINEFROMPOSITION, position, 0);
  column = _code_editor_impl->send_editor(this, SCI_GETCOLUMN, position, 0);
  ;
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::can_undo() {
  return _code_editor_impl->send_editor(this, SCI_CANUNDO, 0, 0) != 0;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::undo() {
  _code_editor_impl->send_editor(this, SCI_UNDO, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::can_redo() {
  return _code_editor_impl->send_editor(this, SCI_CANREDO, 0, 0) != 0;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::redo() {
  _code_editor_impl->send_editor(this, SCI_REDO, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::can_cut() {
  return can_copy() && can_delete();
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::cut() {
  _code_editor_impl->send_editor(this, SCI_CUT, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::can_copy() {
  sptr_t length = _code_editor_impl->send_editor(this, SCI_GETSELECTIONEND, 0, 0) -
                  _code_editor_impl->send_editor(this, SCI_GETSELECTIONSTART, 0, 0);
  return length > 0;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::copy() {
  _code_editor_impl->send_editor(this, SCI_COPY, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::can_paste() {
  return _code_editor_impl->send_editor(this, SCI_CANPASTE, 0, 0) != 0;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::paste() {
  _code_editor_impl->send_editor(this, SCI_PASTE, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::can_delete() {
  return can_copy() && _code_editor_impl->send_editor(this, SCI_GETREADONLY, 0, 0) == 0;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::do_delete() {
  replace_selected_text("");
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::select_all() {
  _code_editor_impl->send_editor(this, SCI_SELECTALL, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_status_text(const std::string& text) {
  // Optional implementation.
  if (_code_editor_impl->set_status_text != NULL)
    _code_editor_impl->set_status_text(this, text);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::show_find_panel(bool replace) {
  if (_find_panel == NULL)
    _find_panel = new FindPanel(this);
  _find_panel->enable_replace(replace);

  if (_show_find_panel)
    _show_find_panel(this, true);
  _find_panel->focus();
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::hide_find_panel() {
  if (_find_panel == NULL)
    return;

  if (_show_find_panel && _find_panel->is_shown())
    _show_find_panel(this, false);
  focus();
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_show_find_panel_callback(std::function<void(CodeEditor*, bool)> callback) {
  _show_find_panel = callback;
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::find_and_highlight_text(const std::string& search_text, FindFlags flags, bool scroll_to,
                                         bool backwards) {
  if (search_text.size() == 0)
    return false;

  bool wrap = (flags & FindWrapAround) != 0;
  int search_flags = 0;
  if (flags & FindMatchCase)
    search_flags |= SCFIND_MATCHCASE;
  if (flags & FindWholeWords)
    search_flags |= SCFIND_WHOLEWORD;
  if (flags & FindRegex)
    search_flags |= SCFIND_REGEXP;

  sptr_t selection_start = _code_editor_impl->send_editor(this, SCI_GETSELECTIONSTART, 0, 0);
  sptr_t selection_end = _code_editor_impl->send_editor(this, SCI_GETSELECTIONEND, 0, 0);

  // Sets the start point for the upcoming search to the begin of the current selection.
  // For forward searches we have therefore to set the selection start to the current selection end
  // for proper incremental search. This does not harm as we either get a new selection if something
  // is found or the previous selection is restored.
  if (!backwards)
    _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, selection_end, 0);
  _code_editor_impl->send_editor(this, SCI_SEARCHANCHOR, 0, 0);

  sptr_t result = 0;

  // The following call will also set the selection if something was found.
  if (backwards) {
    result = _code_editor_impl->send_editor(this, SCI_SEARCHPREV, search_flags, (sptr_t)search_text.c_str());
    if (result < 0 && wrap) {
      // Try again from the end of the document if nothing could be found so far and
      // wrapped search is set.
      _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART,
                                     _code_editor_impl->send_editor(this, SCI_GETTEXTLENGTH, 0, 0), 0);
      _code_editor_impl->send_editor(this, SCI_SEARCHANCHOR, 0, 0);
      result = _code_editor_impl->send_editor(this, SCI_SEARCHNEXT, search_flags, (sptr_t)search_text.c_str());
    }
  } else {
    result = _code_editor_impl->send_editor(this, SCI_SEARCHNEXT, search_flags, (sptr_t)search_text.c_str());
    if (result < 0 && wrap) {
      // Try again from the start of the document if nothing could be found so far and
      // wrapped search is set.
      _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, 0, 0);
      _code_editor_impl->send_editor(this, SCI_SEARCHANCHOR, 0, 0);
      result = _code_editor_impl->send_editor(this, SCI_SEARCHNEXT, search_flags, (sptr_t)search_text.c_str());
    }
  }

  if (result >= 0) {
    if (scroll_to)
      _code_editor_impl->send_editor(this, SCI_SCROLLCARET, 0, 0);
  } else {
    // Restore the former selection if we did not find anything.
    _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, selection_start, 0);
    _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND, selection_end, 0);
  }
  return (result >= 0) ? true : false;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Searches the given text and replaces it by new_text. Returns the number of replacements performed.
 */
size_t CodeEditor::find_and_replace_text(const std::string& search_text, const std::string& new_text, FindFlags flags,
                                         bool do_all) {
  if (search_text.size() == 0)
    return 0;

  // The current position is where we start searching for single occurrences. Otherwise we start at
  // the beginning of the document.
  sptr_t start_position;
  if (do_all)
    start_position = 0;
  else
    start_position = _code_editor_impl->send_editor(this, SCI_GETCURRENTPOS, 0, 0);
  sptr_t endPosition = _code_editor_impl->send_editor(this, SCI_GETTEXTLENGTH, 0, 0);

  int search_flags = 0;
  if (flags & FindMatchCase)
    search_flags |= SCFIND_MATCHCASE;
  if (flags & FindWholeWords)
    search_flags |= SCFIND_WHOLEWORD;
  if (flags & FindRegex)
    search_flags |= SCFIND_REGEXP;
  _code_editor_impl->send_editor(this, SCI_SETSEARCHFLAGS, search_flags, 0);
  _code_editor_impl->send_editor(this, SCI_SETTARGETSTART, start_position, 0);
  _code_editor_impl->send_editor(this, SCI_SETTARGETEND, endPosition, 0);

  sptr_t result;

  size_t replace_count = 0;
  if (do_all) {
    while (true) {
      result =
        _code_editor_impl->send_editor(this, SCI_SEARCHINTARGET, search_text.size(), (sptr_t)search_text.c_str());
      if (result < 0)
        break;

      replace_count++;
      /*result =*/_code_editor_impl->send_editor(this, SCI_REPLACETARGET, new_text.size(), (sptr_t)new_text.c_str());

      // The replacement changes the target range to the replaced text. Continue after that until the end.
      // The text length might be changed by the replacement so make sure the target end is the actual
      // text end.
      _code_editor_impl->send_editor(this, SCI_SETTARGETSTART,
                                     _code_editor_impl->send_editor(this, SCI_GETTARGETEND, 0, 0), 0);
      _code_editor_impl->send_editor(this, SCI_SETTARGETEND,
                                     _code_editor_impl->send_editor(this, SCI_GETTEXTLENGTH, 0, 0), 0);
    }
  } else {
    result = _code_editor_impl->send_editor(this, SCI_SEARCHINTARGET, search_text.size(), (sptr_t)search_text.c_str());
    replace_count = (result < 0) ? 0 : 1;

    if (replace_count > 0) {
      /*result =*/_code_editor_impl->send_editor(this, SCI_REPLACETARGET, new_text.size(), (sptr_t)new_text.c_str());

      // For a single replace we set the new selection to the replaced text.
      _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART,
                                     _code_editor_impl->send_editor(this, SCI_GETTARGETSTART, 0, 0), 0);
      _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND,
                                     _code_editor_impl->send_editor(this, SCI_GETTARGETEND, 0, 0), 0);
    }
  }

  return replace_count;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::jump_to_next_placeholder() {
  sptr_t current_pos = _code_editor_impl->send_editor(this, SCI_GETCURRENTPOS, 0, 0);
  sptr_t text_size = _code_editor_impl->send_editor(this, SCI_GETLENGTH, 0, 0);
  Sci_TextToFind what;

  what.lpstrText = (char*)"<{";
  what.chrg.cpMin = (long)current_pos;
  what.chrg.cpMax = (long)text_size;
  sptr_t result = _code_editor_impl->send_editor(this, SCI_FINDTEXT, 0, (sptr_t)&what);
  bool found = false;
  if (result >= 0) {
    static const int max_placeholder_length = 256;
    what.lpstrText = (char*)"}>";
    what.chrg.cpMin = (long)result;
    what.chrg.cpMax = what.chrg.cpMin + max_placeholder_length; // arbitrary max size for placeholder text
    result = _code_editor_impl->send_editor(this, SCI_FINDTEXT, 0, (sptr_t)&what);
    if (result >= 0) {
      char buffer[max_placeholder_length];
      Sci_TextRange tr;
      tr.chrg.cpMin = what.chrg.cpMin;
      tr.chrg.cpMax = (long)result + 2;
      tr.lpstrText = buffer;
      _code_editor_impl->send_editor(this, SCI_GETTEXTRANGE, 0, (sptr_t)&tr);

      if (!memchr(buffer, '\n', tr.chrg.cpMax - tr.chrg.cpMin)) {
        // jump to placeholder and select it
        _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, tr.chrg.cpMin, 0);
        _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND, tr.chrg.cpMax, 0);
        _code_editor_impl->send_editor(this, SCI_SCROLLCARET, 0, 0);
        found = true;
      }
    }
  }
  if (!found) {
    // restore the cursor to where it was
    _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, current_pos, 0);
    _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND, current_pos, 0);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_show(size_t chars_entered, const std::vector<std::pair<int, std::string> >& entries) {
  if (entries.size() == 0)
    return;

  std::stringstream list;
  for (size_t i = 0; i < entries.size(); ++i) {
    if (i > 0)
      list << AC_LIST_SEPARATOR;
    list << entries[i].second;
    if (entries[i].first > -1)
      list << AC_TYPE_SEPARATOR << entries[i].first;
  }
  _code_editor_impl->send_editor(this, SCI_AUTOCSHOW, chars_entered, (sptr_t)list.str().c_str());
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_show(size_t chars_entered, const std::vector<std::string>& entries) {
  std::stringstream list;
  for (size_t i = 0; i < entries.size(); ++i) {
    if (i > 0)
      list << AC_LIST_SEPARATOR;
    list << entries[i];
  }
  _code_editor_impl->send_editor(this, SCI_AUTOCSHOW, chars_entered, (sptr_t)list.str().c_str());
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_cancel() {
  _code_editor_impl->send_editor(this, SCI_AUTOCCANCEL, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_options(bool ignore_case, bool choose_single, bool auto_hide, bool drop_rest_of_word,
                                         bool cancel_at_start) {
  _code_editor_impl->send_editor(this, SCI_AUTOCSETIGNORECASE, ignore_case, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETCHOOSESINGLE, choose_single, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETAUTOHIDE, auto_hide, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETDROPRESTOFWORD, drop_rest_of_word, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETCANCELATSTART, cancel_at_start, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_max_size(int width, int height) {
  _code_editor_impl->send_editor(this, SCI_AUTOCSETMAXHEIGHT, height, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETMAXWIDTH, width, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_register_images(const std::vector<std::pair<int, std::string> > &images) {
  for (auto &image : images) {
    if (ensureImage(image.second)) {
      auto &data = registeredImages[image.second];
      _code_editor_impl->send_editor(this, SCI_RGBAIMAGESETWIDTH, data.width, 0);
      _code_editor_impl->send_editor(this, SCI_RGBAIMAGESETHEIGHT, data.height, 0);

      // Scale factors are not supported for code completion images.
      //_code_editor_impl->send_editor(this, SCI_RGBAIMAGESETSCALE, data.hiresImage ? 200 : 100, 0);

      _code_editor_impl->send_editor(this, SCI_REGISTERRGBAIMAGE, image.first, reinterpret_cast<sptr_t>(data.data));
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool CodeEditor::auto_completion_active() {
  return _code_editor_impl->send_editor(this, SCI_AUTOCACTIVE, 0, 0) != 0;
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_stops(const std::string& stops) {
  _code_editor_impl->send_editor(this, SCI_AUTOCSTOPS, 0, (sptr_t)stops.c_str());
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_fillups(const std::string& fillups) {
  _code_editor_impl->send_editor(this, SCI_AUTOCSETFILLUPS, 0, (sptr_t)fillups.c_str());
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::show_calltip(bool show, size_t position, const std::string& value) {
  if (show)
    _code_editor_impl->send_editor(this, SCI_CALLTIPSHOW, position, (sptr_t)value.c_str());
  else
    _code_editor_impl->send_editor(this, SCI_CALLTIPCANCEL, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::set_eol_mode(mforms::EndOfLineMode mode, bool convert) {
  _code_editor_impl->send_editor(this, SCI_SETEOLMODE, mode, 0);
  if (convert)
    _code_editor_impl->send_editor(this, SCI_CONVERTEOLS, mode, 0);
}

//----------------------------------------------------------------------------------------------------------------------

sptr_t CodeEditor::send_editor(unsigned int message, uptr_t wParam, sptr_t lParam) {
  return _code_editor_impl->send_editor(this, message, wParam, lParam);
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::lost_focus() {
  _signal_lost_focus();
}

//----------------------------------------------------------------------------------------------------------------------

void CodeEditor::resize() {
  if (_scroll_on_resize)
    _code_editor_impl->send_editor(this, SCI_SCROLLCARET, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------
