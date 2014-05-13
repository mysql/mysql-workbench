/* 
 * Copyright (c) 2010, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "base/log.h"
#include "base/drawing.h"
#include "base/string_utilities.h"
#include "base/notifications.h"
#include "tinyxml.h"
#include "SciLexer.h"
#include "mforms/mforms.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_BE)

using namespace Scintilla;

using namespace mforms;
using namespace base;

// Marker ID assignments. Markers with higher number overlay lower ones.
#define CE_STATEMENT_MARKER 0
#define CE_ERROR_MARKER 1
#define CE_BREAKPOINT_MARKER 2
#define CE_BREAKPOINT_HIT_MARKER 3
#define CE_CURRENT_LINE_MARKER 4

#define AC_LIST_SEPARATOR '\x19' // Unused codes as separators.
#define AC_TYPE_SEPARATOR '\x18'

#define ERROR_INDICATOR INDIC_CONTAINER
#define ERROR_INDICATOR_VALUE 42 // Arbitrary value.

//----------------- CodeEditorConfig ---------------------------------------------------------------

CodeEditorConfig::CodeEditorConfig(SyntaxHighlighterLanguage language)
{
  _used_language = language;
  _language_element = NULL;
  _document = NULL;

  std::string lexer;
  std::string override_lexer;
  switch (language)
  {
  case mforms::LanguageMySQL50:
    override_lexer = "SCLEX_MYSQL_50";
    lexer = "SCLEX_MYSQL";
    break;
      
  case mforms::LanguageMySQL51:
    override_lexer = "SCLEX_MYSQL_51";
    lexer = "SCLEX_MYSQL";
    break;
      
  case mforms::LanguageMySQL55:
    override_lexer = "SCLEX_MYSQL_55";
    lexer = "SCLEX_MYSQL";
    break;
      
  case mforms::LanguageMySQL56:
    override_lexer = "SCLEX_MYSQL_56";
    lexer = "SCLEX_MYSQL";
    break;
      
  case mforms::LanguageMySQL57:
    override_lexer = "SCLEX_MYSQL_57";
    lexer = "SCLEX_MYSQL";
    break;
      
  case mforms::LanguageHtml:
    lexer = "SCLEX_HTML";
    break;

  case mforms::LanguageLua:
    lexer = "SCLEX_LUA";
    break;

  case mforms::LanguagePython:
    lexer = "SCLEX_PYTHON";
    break;

  case mforms::LanguageCpp:
    lexer = "SCLEX_CPP";
    break;

  default:
    return;
  }

  // Load the user's config file if it exists, otherwise use the default one.
  std::string config_file = mforms::Utilities::get_special_folder(mforms::ApplicationData) + "/MySQL/Workbench/code_editor.xml";
  if (!g_file_test(config_file.c_str(), G_FILE_TEST_EXISTS))
    config_file = App::get()->get_resource_path("") + "/data/code_editor.xml";

  _document = new TiXmlDocument(config_file);
  if (!_document->LoadFile())
  {
    log_error("Code Editor Config: cannot load configuration file \"%s\":\n\t%s (row: %d, column: %d)\n",
      config_file.c_str(), _document->ErrorDesc(), _document->ErrorRow(), _document->ErrorCol());
    return;
  }

  TiXmlElement *element = _document->FirstChildElement("languages");
  if (element == NULL)
  {
    log_error("Code Editor: invalid configuration file \"%s\"\n", config_file.c_str());
    return;
  }

  // Load the available language identifiers. All remaining values are loaded on demand.
  for (TiXmlElement *language_element = element->FirstChildElement(); language_element != NULL;
    language_element = language_element->NextSiblingElement())
  {
    std::string language_name = *language_element->Attribute(std::string("name"));
    if (language_name == lexer)
      _language_element = language_element;
    _languages.push_back(language_name);
  }

  if (_language_element == NULL)
  {
    log_warning("Code Editor: could not find settings for language %s in configuration file "
      "\"%s\"\n", lexer.c_str(), config_file.c_str());
    return;
  }

  parse_properties();
  parse_settings();
  parse_keywords();
  parse_styles();
  
  // check if there's another config section containing values that should override the base one
  if (!override_lexer.empty() && override_lexer != lexer)
  {
    bool found = false;
    // Load the available language identifiers. All remaining values are loaded on demand.
    for (TiXmlElement *language_element = element->FirstChildElement(); language_element != NULL;
         language_element = language_element->NextSiblingElement())
    {
      std::string language_name = *language_element->Attribute(std::string("name"));
      if (language_name == override_lexer)
      {
        _language_element = language_element;
        found = true;
        break;
      }
    }

    if (found)
    {
      parse_properties();
      parse_settings();
      parse_keywords();
      parse_styles();
    }
  }
}

//--------------------------------------------------------------------------------------------------

CodeEditorConfig::~CodeEditorConfig()
{
  delete _document;
}

//--------------------------------------------------------------------------------------------------

void CodeEditorConfig::parse_properties()
{
  for (TiXmlElement *entry = _language_element->FirstChildElement("property"); entry != NULL;
    entry = entry->NextSiblingElement("property"))
  {
    const char* property_name = entry->Attribute("name");
    const char* property_value = entry->Attribute("value");
    if (property_name != NULL && property_value != NULL)
      _properties[property_name] = property_value;
  }
}

//--------------------------------------------------------------------------------------------------

void CodeEditorConfig::parse_settings()
{
  for (TiXmlElement *entry = _language_element->FirstChildElement("setting"); entry != NULL;
    entry = entry->NextSiblingElement("setting"))
  {
    const char* property_name = entry->Attribute("name");
    const char* property_value = entry->Attribute("value");
    if (property_name != NULL && property_value != NULL)
      _settings[property_name] = property_value;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Recursively collects text of this and all its child entries.
 */
std::string collect_text(TiXmlNode *entry)
{
  std::string result;
  for (TiXmlNode* child = entry->FirstChild(); child!= NULL; child = child->NextSibling())
  {
    const TiXmlText* childText = child->ToText();
    if (childText)
      result += childText->ValueStr() + collect_text(child);
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

void CodeEditorConfig::parse_keywords()
{
  for (TiXmlElement* entry = _language_element->FirstChildElement("keywords"); entry != NULL;
    entry = entry->NextSiblingElement("keywords"))
  {
    std::string property_name = *entry->Attribute(std::string("name"));
    std::string text = collect_text(entry);
    _keywords[property_name] = text;
  }
}

//--------------------------------------------------------------------------------------------------

void CodeEditorConfig::parse_styles()
{
  for (TiXmlElement* entry = _language_element->FirstChildElement("style"); entry != NULL;
    entry = entry->NextSiblingElement("style"))
  {
    int id = -1;
    entry->Attribute("id", &id);
    if (id < 0)
      continue;

    std::map<std::string, std::string> entries;
    for (TiXmlAttribute *attribute = entry->FirstAttribute(); attribute != NULL; attribute = attribute->Next())
    {
      if (strcmp(attribute->Name(), "id") == 0)
        continue;
      entries[attribute->Name()] = attribute->Value();
    }
    _styles[id] = entries;
  }
}

//----------------- CodeEditor ---------------------------------------------------------------------

CodeEditor::CodeEditor(void *host)
  : _host(host)
{
  _code_editor_impl= &ControlFactory::get_instance()->_code_editor_impl;

  _code_editor_impl->create(this);
  _code_editor_impl->send_editor(this, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
  _context_menu = NULL;
  _find_panel = NULL;
  _scroll_on_resize = true;

  setup();
}

//--------------------------------------------------------------------------------------------------

CodeEditor::~CodeEditor()
{
  auto_completion_cancel();
  for (std::map<int, void*>::iterator iterator = _images.begin(); iterator != _images.end(); ++iterator)
    free(iterator->second);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::setup()
{
  _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_NULL, 0);
  _code_editor_impl->send_editor(this, SCI_STYLERESETDEFAULT, 0, 0); // Reset default style to what it was initially.
  _code_editor_impl->send_editor(this, SCI_STYLECLEARALL, 0, 0); // Set all other styles to the default style.

  _code_editor_impl->send_editor(this, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
#if defined(__APPLE__)
  _code_editor_impl->send_editor(this, SCI_STYLESETSIZE, STYLE_LINENUMBER, (sptr_t)10);
#else
  _code_editor_impl->send_editor(this, SCI_STYLESETSIZE, STYLE_LINENUMBER, (sptr_t)8);
#endif
  sptr_t lineNumberStyleWidth = _code_editor_impl->send_editor(this, SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t)"_999999");
  _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 0, lineNumberStyleWidth);
  _code_editor_impl->send_editor(this, SCI_SETMARGINSENSITIVEN, 0, 0);

  // Margin: Markers.
  _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 1, 16);
  _code_editor_impl->send_editor(this, SCI_SETMARGINSENSITIVEN, 1, 1);

  // Folder setup.
  _code_editor_impl->send_editor(this, SCI_SETPROPERTY, (uptr_t)"fold", (sptr_t)"1");
  _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 2, 16);
  _code_editor_impl->send_editor(this, SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
  _code_editor_impl->send_editor(this, SCI_SETMARGINSENSITIVEN, 2, 1); // Margin is clickable.
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
  _code_editor_impl->send_editor(this, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
  for (int n= 25; n < 32; ++n) // Markers 25..31 are reserved for folding.
  {
    _code_editor_impl->send_editor(this, SCI_MARKERSETFORE, n, 0xffffff);
    _code_editor_impl->send_editor(this, SCI_MARKERSETBACK, n, 0x404040);
  }

  // Margin: Line number style.
  _code_editor_impl->send_editor(this, SCI_STYLESETFORE, STYLE_LINENUMBER, 0x404040);
  _code_editor_impl->send_editor(this, SCI_STYLESETBACK, STYLE_LINENUMBER, 0xE0E0E0);

  // Init markers & indicators for highlighting of syntax errors.
  _code_editor_impl->send_editor(this, SCI_INDICSETFORE, ERROR_INDICATOR, 0x2119D0);
  _code_editor_impl->send_editor(this, SCI_INDICSETUNDER, ERROR_INDICATOR, 1);
  _code_editor_impl->send_editor(this, SCI_INDICSETSTYLE, ERROR_INDICATOR, INDIC_SQUIGGLE);

  // Gutter markers for errors and statements, breakpoints and current code line.
  setup_marker(CE_STATEMENT_MARKER, "editor_statement");
  setup_marker(CE_ERROR_MARKER, "editor_error");
  setup_marker(CE_BREAKPOINT_MARKER, "editor_breakpoint");
  setup_marker(CE_BREAKPOINT_HIT_MARKER, "editor_breakpoint_hit");
  setup_marker(CE_CURRENT_LINE_MARKER, "editor_current_pos");

  // Other settings.
  Color color = App::get()->get_system_color(mforms::SystemColorHighlight);
  int rawColor = (int)(255 * color.red) + ((int)(255 * color.green) << 8) +
    ((int)(255 * color.blue) << 16);
  _code_editor_impl->send_editor(this, SCI_SETSELBACK, 1, rawColor);
  _code_editor_impl->send_editor(this, SCI_SETSELFORE, 1, 0xFFFFFF);

  _code_editor_impl->send_editor(this, SCI_SETCARETLINEVISIBLE, 1, 0);
  _code_editor_impl->send_editor(this, SCI_SETCARETLINEBACK, 0xF8C800, 0);
  _code_editor_impl->send_editor(this, SCI_SETCARETLINEBACKALPHA, 20, 0);

  // - Tabulators + indentation
  _code_editor_impl->send_editor(this, SCI_SETTABINDENTS, 1, 0);
  _code_editor_impl->send_editor(this, SCI_SETBACKSPACEUNINDENTS, 1, 0);

  // - Call tips
  _code_editor_impl->send_editor(this, SCI_CALLTIPSETFORE, 0x202020, 0);
  _code_editor_impl->send_editor(this, SCI_CALLTIPSETBACK, 0xF0F0F0, 0);

  _code_editor_impl->send_editor(this, SCI_SETMOUSEDWELLTIME, 200, 0);

  // - Line ending + scrolling.
  _code_editor_impl->send_editor(this, SCI_SETSCROLLWIDTHTRACKING, 1, 0);
  _code_editor_impl->send_editor(this, SCI_SETEOLMODE, SC_EOL_LF, 0);

  // - Auto completion
  _code_editor_impl->send_editor(this, SCI_AUTOCSETSEPARATOR, AC_LIST_SEPARATOR, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETTYPESEPARATOR, AC_TYPE_SEPARATOR, 0);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::set_text(const char* text)
{
  _code_editor_impl->send_editor(this, SCI_SETTEXT, 0, (sptr_t)text);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::set_value(const std::string &value)
{
  // When passing text as std::string we have a length and can hence use a different
  // way to set the text in the control, which preserves embedded nulls.
  _code_editor_impl->send_editor(this, SCI_CLEARALL, 0, 0);
  _code_editor_impl->send_editor(this, SCI_APPENDTEXT, value.size(), (sptr_t)value.c_str());
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::set_text_keeping_state(const char* text)
{
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

//--------------------------------------------------------------------------------------------------

void CodeEditor::append_text(const char* text, size_t length)
{
  _code_editor_impl->send_editor(this, SCI_APPENDTEXT, length, (sptr_t)text);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::replace_selected_text(const std::string& text)
{
  size_t start, length;
  get_selection(start, length);
  _code_editor_impl->send_editor(this, SCI_REPLACESEL, 0, (sptr_t)text.c_str());

  _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, start + text.length(), 0);
  _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND, start + text.length(), 0);
}

//--------------------------------------------------------------------------------------------------

const std::string CodeEditor::get_text(bool selection_only)
{
  char* text;
  sptr_t length;
  if (selection_only)
  {
    length =_code_editor_impl->send_editor(this, SCI_GETSELTEXT, 0, 0);
    text = (char*)malloc(length);
    if (text != NULL) // Can be null if not memory is available.
      _code_editor_impl->send_editor(this, SCI_GETSELTEXT, length, (sptr_t)text);
  }
  else
  {
    length = _code_editor_impl->send_editor(this, SCI_GETTEXTLENGTH, 0, 0) + 1;
    text = (char*)malloc(length);
    if (text != NULL)
      _code_editor_impl->send_editor(this, SCI_GETTEXT, length, (sptr_t)text);
  }
  if (text != NULL)
  {
    std::string result(text, length - 1);
    free(text);

    return result;
  }
  return "";
}

//--------------------------------------------------------------------------------------------------

const std::string CodeEditor::get_text_in_range(size_t start, size_t end)
{
  Scintilla::Sci_TextRange range;

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

//--------------------------------------------------------------------------------------------------

std::pair<const char*, size_t> CodeEditor::get_text_ptr()
{
  std::pair<const char*, size_t> result;
  result.first = (const char*)_code_editor_impl->send_editor(this, SCI_GETCHARACTERPOINTER, 0, 0);
  result.second = _code_editor_impl->send_editor(this, SCI_GETTEXTLENGTH, 0, 0);

  return result;
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::set_selection(size_t start, size_t length)
{
  _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, start, 0);
  _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND, start + length, 0);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::clear_selection()
{
  sptr_t current_pos = _code_editor_impl->send_editor(this, SCI_GETCURRENTPOS, 0, 0);
  _code_editor_impl->send_editor(this, SCI_SETEMPTYSELECTION, current_pos, 0);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::get_selection(size_t &start, size_t &length)
{
  start = _code_editor_impl->send_editor(this, SCI_GETSELECTIONSTART, 0, 0);
  length = _code_editor_impl->send_editor(this, SCI_GETSELECTIONEND, 0, 0) - start;
}

//--------------------------------------------------------------------------------------------------

bool CodeEditor::get_range_of_line(ssize_t line, ssize_t &start, ssize_t &end)
{
  start = _code_editor_impl->send_editor(this, SCI_POSITIONFROMLINE, line, 0);
  end = _code_editor_impl->send_editor(this, SCI_GETLINEENDPOSITION, line, 0);

  return start < 0 || end < 0;
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::setup_marker(int marker, const std::string& name)
{
  std::string path = App::get()->get_resource_path(name + ".xpm");

  gchar *content = NULL;
  if (g_file_get_contents(path.c_str(), &content, NULL, NULL))
  {
    _code_editor_impl->send_editor(this, SCI_MARKERDEFINEPIXMAP, marker, (sptr_t)content);
    g_free(content);
  }
  _code_editor_impl->send_editor(this, SCI_MARKERSETBACK, marker, 0xD01921);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::load_configuration(SyntaxHighlighterLanguage language)
{
  CodeEditorConfig config(language);

  // Keywords.
  std::map<std::string, std::string> keywords = config.get_keywords();

  // Key word list sets are from currently active lexer, so that must be set before calling here.
  sptr_t length = _code_editor_impl->send_editor(this, SCI_DESCRIBEKEYWORDSETS, 0, 0);

  char* keyword_sets = (char*)malloc(length + 1);
  _code_editor_impl->send_editor(this, SCI_DESCRIBEKEYWORDSETS, 0, (sptr_t)keyword_sets);
  std::vector<std::string> keyword_list_names = base::split(keyword_sets, "\n");
  free(keyword_sets);

  for (std::map<std::string, std::string>::const_iterator iterator = keywords.begin();
    iterator != keywords.end(); ++iterator)
  {
    std::string list_name = iterator->first;
    int list_index = base::index_of(keyword_list_names, list_name);
    if (list_index > -1)
      _code_editor_impl->send_editor(this, SCI_SETKEYWORDS, list_index, (sptr_t)iterator->second.c_str());
  }

  // Properties.
  std::map<std::string, std::string> properties = config.get_properties();
  for (std::map<std::string, std::string>::const_iterator iterator = properties.begin();
    iterator != properties.end(); ++iterator)
  {
    _code_editor_impl->send_editor(this, SCI_SETPROPERTY, (uptr_t)iterator->first.c_str(), (sptr_t)iterator->second.c_str());
  }

  // Settings.
  std::map<std::string, std::string> settings = config.get_settings();
  for (std::map<std::string, std::string>::const_iterator iterator = settings.begin();
    iterator != settings.end(); ++iterator)
  {
    if (iterator->first == "usetabs")
    {
      int int_value = atoi(iterator->second.c_str());
      _code_editor_impl->send_editor(this, SCI_SETUSETABS, int_value, 0);
    }
    else
      if (iterator->first == "tabwidth")
      {
        int int_value = atoi(iterator->second.c_str());
        _code_editor_impl->send_editor(this, SCI_SETTABWIDTH, int_value, 0);
      }
      else
        if (iterator->first == "indentation")
        {
          int int_value = atoi(iterator->second.c_str());
          _code_editor_impl->send_editor(this, SCI_SETINDENT, int_value, 0);
        }
  }

  // Styles.
  std::map<int, std::map<std::string, std::string> > styles = config.get_styles();
  for (std::map<int, std::map<std::string, std::string> >::const_iterator iterator = styles.begin();
    iterator != styles.end(); ++iterator)
  {
    int id = iterator->first;
    std::map<std::string, std::string> values = iterator->second;
    std::string value = values["fore-color"];
    if (!value.empty())
    {
      // Colors for Scintilla must be converted from HTML to BGR style.
      Color color = Color::parse(value);
      int raw_color = (int)(255 * color.red) + ((int)(255 * color.green) << 8) +
        ((int)(255 * color.blue) << 16);
      _code_editor_impl-> send_editor(this, SCI_STYLESETFORE, id, raw_color);
    }

    value = values["back-color"];
    if (!value.empty())
    {
      Color color = Color::parse(value);
      int raw_color = (int)(255 * color.red) + ((int)(255 * color.green) << 8) +
        ((int)(255 * color.blue) << 16);
      _code_editor_impl->send_editor(this, SCI_STYLESETBACK, id, raw_color);
    }

    value = base::tolower(values["bold"]);
    if (!value.empty())
    {
      bool flag = value == "1" || value == "yes" || value == "true";
      _code_editor_impl->send_editor(this, SCI_STYLESETBOLD, id, flag);
    }

    value = base::tolower(values["italic"]);
    if (!value.empty())
    {
      bool flag = value == "1" || value == "yes" || value == "true";
      _code_editor_impl->send_editor(this, SCI_STYLESETITALIC, id, flag);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::set_language(SyntaxHighlighterLanguage language)
{
  switch (language)
  {
  case mforms::LanguageMySQL50:
  case mforms::LanguageMySQL51:
  case mforms::LanguageMySQL55:
  case mforms::LanguageMySQL56:
  case mforms::LanguageMySQL57:
    _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_MYSQL, 0);
    break;

  case mforms::LanguageHtml:
    _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_HTML, 0);
    break;

  case mforms::LanguageLua:
    _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_LUA, 0);
    break;

  case mforms::LanguagePython:
    _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_PYTHON, 0);
    break;

  case mforms::LanguageCpp:
    _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_CPP, 0);
    break;

  default:
      // No language. Reset all styling so the editor appears without syntax highlighting.
    _code_editor_impl->send_editor(this, SCI_SETLEXER, SCLEX_NULL, 0);
    _code_editor_impl->send_editor(this, SCI_STYLERESETDEFAULT, 0, 0); // Reset default style to what it was initially.
    _code_editor_impl->send_editor(this, SCI_STYLECLEARALL, 0, 0); // Set all other styles to the default style.
    return;
  }

  load_configuration(language);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::show_markup(LineMarkup markup, size_t line)
{
  // The marker mask contains one bit for each set marker (0..31).
  sptr_t marker_mask = _code_editor_impl->send_editor(this, SCI_MARKERGET, line, 0);
  sptr_t new_marker_mask = 0;
  if ((markup & mforms::LineMarkupStatement) != 0)
  {
    sptr_t mask = 1 << CE_STATEMENT_MARKER;
    if ((marker_mask & mask) != mask)
      new_marker_mask |= mask;
  }
  if ((markup & mforms::LineMarkupError) != 0)
  {
    sptr_t mask = 1 << CE_ERROR_MARKER;
    if ((marker_mask & mask) != mask)
      new_marker_mask |= mask;
  }
  if ((markup & mforms::LineMarkupBreakpoint) != 0)
  {
    sptr_t mask = 1 << CE_BREAKPOINT_MARKER;
    if ((marker_mask & mask) != mask)
      new_marker_mask |= mask;
  }
  if ((markup & mforms::LineMarkupBreakpointHit) != 0)
  {
    sptr_t mask = 1 << CE_BREAKPOINT_HIT_MARKER;
    if ((marker_mask & mask) != mask)
      new_marker_mask |= mask;
  }
  if ((markup & mforms::LineMarkupCurrent) != 0)
  {
    sptr_t mask = 1 << CE_CURRENT_LINE_MARKER;
    if ((marker_mask & mask) != mask)
      new_marker_mask |= mask;
  }

  _code_editor_impl->send_editor(this, SCI_MARKERADDSET, line, new_marker_mask);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::remove_markup(LineMarkup markup, ssize_t line)
{
  if (markup == mforms::LineMarkupAll || line < 0)
  {
    if (line < 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETEALL, -1, 0);
    else
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, -1);
  }
  else
  {
    if ((markup & mforms::LineMarkupStatement) != 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, CE_STATEMENT_MARKER);
    if ((markup & mforms::LineMarkupError) != 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, CE_ERROR_MARKER);
    if ((markup & mforms::LineMarkupBreakpoint) != 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, CE_BREAKPOINT_MARKER);
    if ((markup & mforms::LineMarkupBreakpointHit) != 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, CE_BREAKPOINT_HIT_MARKER);
    if ((markup & mforms::LineMarkupCurrent) != 0)
      _code_editor_impl->send_editor(this, SCI_MARKERDELETE, line, CE_CURRENT_LINE_MARKER);
  }
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::show_indicator(RangeIndicator indicator, size_t start, size_t length)
{
  // Scintilla supports a model that not only sets an indicator in a given range but additionally
  // assigns a value to this indicator. This is to allow drawing an indicator with different styles.
  // However, currently all values are drawn in the same style and it is neither clear if that will ever
  // change nor what the actual use is, given that you always can set different indicators for different styles.
  // Unfortunately, you cannot query an indicator as such, only its value, so we have to set a value too
  // even if that is quite useless.
  switch (indicator)
  {
  case mforms::RangeIndicatorError:
    _code_editor_impl->send_editor(this, SCI_SETINDICATORCURRENT, ERROR_INDICATOR, 0);
    _code_editor_impl->send_editor(this, SCI_SETINDICATORVALUE, ERROR_INDICATOR_VALUE, 0);
    _code_editor_impl->send_editor(this, SCI_INDICATORFILLRANGE, start, length);

    break;
  case mforms::RangeIndicatorNone: // No effect here. Only used to have "none" value for indicator tests.
    break;
  }
}

//--------------------------------------------------------------------------------------------------

RangeIndicator CodeEditor::indicator_at(size_t position)
{
  sptr_t result = _code_editor_impl->send_editor(this, SCI_INDICATORVALUEAT, ERROR_INDICATOR, position);
  if (result == ERROR_INDICATOR_VALUE)
    return mforms::RangeIndicatorError;

  return mforms::RangeIndicatorNone;
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::remove_indicator(RangeIndicator indicator, size_t start, size_t length)
{
  switch (indicator)
  {
  case mforms::RangeIndicatorError:
    _code_editor_impl->send_editor(this, SCI_SETINDICATORCURRENT, ERROR_INDICATOR, 0);
    _code_editor_impl->send_editor(this, SCI_INDICATORCLEARRANGE, start, length);

    break;
  case mforms::RangeIndicatorNone: // No effect here. Only needed to have a "none" value for indicator tests.
    break;
  }
}

//--------------------------------------------------------------------------------------------------

size_t mforms::CodeEditor::line_count()
{
   return _code_editor_impl->send_editor(this, SCI_GETLINECOUNT, 0, 0);
}

//--------------------------------------------------------------------------------------------------

size_t CodeEditor::text_length()
{
  return _code_editor_impl->send_editor(this, SCI_GETLENGTH, 0, 0);
}

//--------------------------------------------------------------------------------------------------

size_t CodeEditor::position_from_line(size_t line_number)
{
  return _code_editor_impl->send_editor(this, SCI_POSITIONFROMLINE, line_number, 0);
}

//--------------------------------------------------------------------------------------------------

size_t CodeEditor::line_from_position(size_t position)
{
  return _code_editor_impl->send_editor(this, SCI_LINEFROMPOSITION, position, 0);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::set_font(const std::string& fontDescription)
{
  // Set this font for all styles.
  std::string font;
  float size;
  bool bold;
  bool italic;
  if (base::parse_font_description(fontDescription, font, size, bold, italic))
  {
    // NOTE: The original MONOSPACE font in windows was Bitstream Vera Sans Mono
    //       but in Windows 8 the code editors using this font were getting hung so
    //       we decided to use Lucida Console as the new MONOSPACE font in windows.
#ifdef _WIN32
    if (base::toupper(font) == "BITSTREAM VERA SANS MONO")
      font = DEFAULT_MONOSPACE_FONT_FAMILY;
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
    // scintilla requires the ! in front of the font name to interpret it as a pango/fontconfig font
    // the non-pango version is totally unusable
    if (!font.empty() && font[0] != '!')
       font = "!"+font;
#endif
    for (int i = 0; i < 128; i++)
    {
      _code_editor_impl->send_editor(this, SCI_STYLESETFONT, i, (sptr_t)font.c_str());
      _code_editor_impl->send_editor(this, SCI_STYLESETSIZE, i, (sptr_t)size);
      _code_editor_impl->send_editor(this, SCI_STYLESETBOLD, i, (sptr_t)bold);
      _code_editor_impl->send_editor(this, SCI_STYLESETITALIC, i, (sptr_t)italic);
    }
  }

  // Recompute the line number margin width if it is visible.
  sptr_t lineNumberStyleWidth = _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 0, 0);
  if (lineNumberStyleWidth > 0)
  {
    lineNumberStyleWidth = _code_editor_impl->send_editor(this, SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t)"_999999");
    _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 0, lineNumberStyleWidth);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Converts scintilla key modifier codes to mforms codes.
 */
mforms::ModifierKey getModifiers(int scintilla_modifiers)
{
  mforms::ModifierKey modifiers = mforms::ModifierNoModifier;
  if ((scintilla_modifiers & SCMOD_CTRL) == SCMOD_CTRL)
    modifiers = modifiers | mforms::ModifierControl;
  if ((scintilla_modifiers & SCMOD_ALT) == SCMOD_ALT)
    modifiers = modifiers | mforms::ModifierAlt;
  if ((scintilla_modifiers & SCMOD_SHIFT) == SCMOD_SHIFT)
    modifiers = modifiers | mforms::ModifierShift;

  return modifiers;
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::on_notify(SCNotification* notification)
{
  switch (notification->nmhdr.code)
  {
  case SCN_MARGINCLICK:
  {
    sptr_t line = _code_editor_impl->send_editor(this, SCI_LINEFROMPOSITION, notification->position, 0);
    if (notification->margin == 2)
    {
      // A click on the folder margin. Toggle the current line if possible.
      _code_editor_impl->send_editor(this, SCI_TOGGLEFOLD, line, 0);
    }
    
    mforms::ModifierKey modifiers = getModifiers(notification->modifiers);
    _gutter_clicked_event(notification->margin, (int)line, modifiers);
    break;
  };
  
  case SCN_MODIFIED:
  {
    // Decide depending on the modification type what to do.
    // There can be more than one modification carried by one notification.
    if (notification->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))
      _change_event(notification->position, notification->length, notification->linesAdded,
        (notification->modificationType & SC_MOD_INSERTTEXT) != 0);
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
    switch (notification->updated)
    {
    case SC_UPDATE_CONTENT:   // Contents, styling or markers have been changed.
      break;
    case SC_UPDATE_SELECTION: // Selection has been changed or the caret moved.
      NotificationCenter::get()->send("GNTextSelectionChanged", this);
      break;
    case SC_UPDATE_V_SCROLL:  // Scrolled vertically.
      break;
    case SC_UPDATE_H_SCROLL:  // Scrolled horizontally.
      break;
    }
    break;

  case SCN_CHARADDED:
    _char_added_event(notification->ch);
    break;
  case SCN_FOCUSOUT:
    _signal_lost_focus();
    break;
  }
}
//--------------------------------------------------------------------------------------------------

void CodeEditor::on_command(int command)
{
  // TODO: removal candidate.
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::set_features(CodeEditorFeature features, bool flag)
{
  if ((features & mforms::FeatureWrapText) != 0)
  {
    if (flag)
      _code_editor_impl->send_editor(this, SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
    else
      _code_editor_impl->send_editor(this, SCI_SETWRAPMODE, SC_WRAP_NONE, 0);
  }

  if ((features & mforms::FeatureGutter) != 0)
  {
    if (flag)
    {
      sptr_t width = _code_editor_impl->send_editor(this, SCI_TEXTWIDTH, STYLE_LINENUMBER, (sptr_t)"_999999");

      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 0, width); // line numbers
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 1, 16); // markers
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 2, 16); // fold markers
    }
    else
    {
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 0, 0);
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 1, 0);
      _code_editor_impl->send_editor(this, SCI_SETMARGINWIDTHN, 2, 0);
    }
  }

  if ((features & mforms::FeatureReadOnly) != 0)
    _code_editor_impl->send_editor(this, SCI_SETREADONLY, flag, 0);

  if ((features & mforms::FeatureShowSpecial) != 0)
  {
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
    _code_editor_impl->send_editor(this, SCI_SETPROPERTY, (uptr_t)"fold", flag ? (sptr_t)"1" : (sptr_t)"0");
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::toggle_features(CodeEditorFeature features)
{
  // Toggling a feature involves querying its current state which is sometimes not possible with a
  // single value, so instead of returning the current state and let the application call
  // set_features we do it internally with this toggle_features function.
  
  if ((features & mforms::FeatureWrapText) != 0)
  {
    if (_code_editor_impl->send_editor(this, SCI_GETWRAPMODE, 0, 0) == SC_WRAP_NONE)
      _code_editor_impl->send_editor(this, SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
    else
      _code_editor_impl->send_editor(this, SCI_SETWRAPMODE, SC_WRAP_NONE, 0);
  }

  if ((features & mforms::FeatureGutter) != 0)
      set_features(mforms::FeatureGutter,
        _code_editor_impl->send_editor(this, SCI_GETMARGINWIDTHN, 0, 0) == 0);

  if ((features & mforms::FeatureReadOnly) != 0)
    set_features(mforms::FeatureReadOnly, 
      _code_editor_impl->send_editor(this, SCI_GETREADONLY, 0, 0) == 0);

  if ((features & mforms::FeatureShowSpecial) != 0)
    set_features(mforms::FeatureShowSpecial, 
      _code_editor_impl->send_editor(this, SCI_GETVIEWEOL, 0, 0) == 0);

  // if ((features & mforms::FeatureUsePopup) != 0)
  // Not searchable so we cannot toggle it.
  
  if ((features & mforms::FeatureConvertEolOnPaste) != 0)
    set_features(mforms::FeatureConvertEolOnPaste,
      _code_editor_impl->send_editor(this, SCI_GETPASTECONVERTENDINGS, 0, 0) != 0);

  if ((features & mforms::FeatureScrollOnResize) != 0)
    _scroll_on_resize = !_scroll_on_resize;

  if ((features & mforms::FeatureFolding) != 0)
  {
    bool folding_enabled = _code_editor_impl->send_editor(this, SCI_GETPROPERTYINT, (uptr_t)"fold", 0) != 0;
    _code_editor_impl->send_editor(this, SCI_SETPROPERTY, (uptr_t)"fold", folding_enabled ? (sptr_t)"0" : (sptr_t)"1");
  }
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::reset_dirty()
{
  _code_editor_impl->send_editor(this, SCI_EMPTYUNDOBUFFER, 0, 0);
}

//--------------------------------------------------------------------------------------------------

bool CodeEditor::is_dirty()
{
  return _code_editor_impl->send_editor(this, SCI_GETMODIFY, 0, 0) != 0;
}

//--------------------------------------------------------------------------------------------------

size_t CodeEditor::get_caret_pos()
{
  return _code_editor_impl->send_editor(this, SCI_GETCURRENTPOS, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::set_caret_pos(size_t position)
{
  _code_editor_impl->send_editor(this, SCI_GOTOPOS, position, 0);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::get_line_column_pos(size_t position, size_t &line, size_t &column)
{
  line = _code_editor_impl->send_editor(this, SCI_LINEFROMPOSITION, position, 0);
  column = _code_editor_impl->send_editor(this, SCI_GETCOLUMN, position, 0);;
}

//--------------------------------------------------------------------------------------------------

bool CodeEditor::can_undo()
{
  return _code_editor_impl->send_editor(this, SCI_CANUNDO, 0, 0) != 0;
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::undo()
{
  _code_editor_impl->send_editor(this, SCI_UNDO, 0, 0);
}

//--------------------------------------------------------------------------------------------------

bool CodeEditor::can_redo()
{
  return _code_editor_impl->send_editor(this, SCI_CANREDO, 0, 0) != 0;
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::redo()
{
  _code_editor_impl->send_editor(this, SCI_REDO, 0, 0);
}

//--------------------------------------------------------------------------------------------------

bool CodeEditor::can_cut()
{
  return can_copy() && can_delete();
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::cut()
{
  _code_editor_impl->send_editor(this, SCI_CUT, 0, 0);
}

//--------------------------------------------------------------------------------------------------

bool CodeEditor::can_copy()
{
  sptr_t length = _code_editor_impl->send_editor(this, SCI_GETSELECTIONEND, 0, 0) -
    _code_editor_impl->send_editor(this, SCI_GETSELECTIONSTART, 0, 0);
  return length > 0;
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::copy()
{
  _code_editor_impl->send_editor(this, SCI_COPY, 0, 0);
}

//--------------------------------------------------------------------------------------------------

bool CodeEditor::can_paste()
{
  return _code_editor_impl->send_editor(this, SCI_CANPASTE, 0, 0) != 0;
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::paste()
{
  _code_editor_impl->send_editor(this, SCI_PASTE, 0, 0);
}

//--------------------------------------------------------------------------------------------------

bool CodeEditor::can_delete()
{
  return can_copy() && _code_editor_impl->send_editor(this, SCI_GETREADONLY, 0, 0) == 0;
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::do_delete()
{
  replace_selected_text("");
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::select_all()
{
  _code_editor_impl->send_editor(this, SCI_SELECTALL, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::set_status_text(const std::string& text)
{
  // Optional implementation.
  if (_code_editor_impl->set_status_text != NULL)
    _code_editor_impl->set_status_text(this, text);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::show_find_panel(bool replace)
{
  if (_find_panel == NULL)
    _find_panel = new FindPanel(this);
  _find_panel->enable_replace(replace);

  if (_show_find_panel)
    _show_find_panel(this, true);
  _find_panel->focus();
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::hide_find_panel()
{
  if (_find_panel == NULL)
    return;

  if (_show_find_panel && _find_panel->is_shown())
    _show_find_panel(this, false);
  focus();
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::set_show_find_panel_callback(boost::function<void (CodeEditor*, bool)> callback)
{
  _show_find_panel = callback;
}

//--------------------------------------------------------------------------------------------------

bool CodeEditor::find_and_highlight_text(const std::string& search_text, FindFlags flags,
  bool scroll_to, bool backwards)
{
  if (search_text.size() == 0)
    return false;

  bool wrap = (flags & FindWrapAround) != 0;
  int search_flags= 0;
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
  if (backwards)
  {
    result = _code_editor_impl->send_editor(this, SCI_SEARCHPREV, search_flags, (sptr_t) search_text.c_str());
    if (result < 0 && wrap)
    {
      // Try again from the end of the document if nothing could be found so far and
      // wrapped search is set.
      _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART,
        _code_editor_impl->send_editor(this, SCI_GETTEXTLENGTH, 0, 0), 0);
      _code_editor_impl->send_editor(this, SCI_SEARCHANCHOR, 0, 0);
      result = _code_editor_impl->send_editor(this, SCI_SEARCHNEXT, search_flags, (sptr_t) search_text.c_str());
    }
  }
  else
  {
    result = _code_editor_impl->send_editor(this, SCI_SEARCHNEXT, search_flags, (sptr_t) search_text.c_str());
    if (result < 0 && wrap)
    {
      // Try again from the start of the document if nothing could be found so far and
      // wrapped search is set.
      _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, 0, 0);
      _code_editor_impl->send_editor(this, SCI_SEARCHANCHOR, 0, 0);
      result = _code_editor_impl->send_editor(this, SCI_SEARCHNEXT, search_flags, (sptr_t) search_text.c_str());
    }
  }

  if (result >= 0)
  {
    if (scroll_to)
      _code_editor_impl->send_editor(this, SCI_SCROLLCARET, 0, 0);
  }
  else
  {
    // Restore the former selection if we did not find anything.
    _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, selection_start, 0);
    _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND, selection_end, 0);
  }
  return (result >= 0) ? true : false;

}

//--------------------------------------------------------------------------------------------------

/**
 * Searches the given text and replaces it by new_text. Returns the number of replacements performed.
 */
size_t CodeEditor::find_and_replace_text(const std::string& search_text, const std::string& new_text,
  FindFlags flags, bool do_all)
{
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

  int search_flags= 0;
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
  if (do_all)
  {
    while (true)
    {
      result = _code_editor_impl->send_editor(this, SCI_SEARCHINTARGET, search_text.size(), (sptr_t)search_text.c_str());
      if (result < 0)
        break;

      replace_count++;
      /*result =*/ _code_editor_impl->send_editor(this, SCI_REPLACETARGET, new_text.size(), (sptr_t)new_text.c_str());

      // The replacement changes the target range to the replaced text. Continue after that until the end.
      // The text length might be changed by the replacement so make sure the target end is the actual
      // text end.
      _code_editor_impl->send_editor(this, SCI_SETTARGETSTART,
        _code_editor_impl->send_editor(this, SCI_GETTARGETEND, 0, 0), 0);
      _code_editor_impl->send_editor(this, SCI_SETTARGETEND,
        _code_editor_impl->send_editor(this, SCI_GETTEXTLENGTH, 0, 0), 0);
    }
  }
  else
  {
    result = _code_editor_impl->send_editor(this, SCI_SEARCHINTARGET, search_text.size(), (sptr_t)search_text.c_str());
    replace_count = (result < 0) ? 0 : 1;

    if (replace_count > 0)
    {
      /*result =*/ _code_editor_impl->send_editor(this, SCI_REPLACETARGET, new_text.size(), (sptr_t)new_text.c_str());

      // For a single replace we set the new selection to the replaced text.
      _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART,
        _code_editor_impl->send_editor(this, SCI_GETTARGETSTART, 0, 0), 0);
      _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND,
        _code_editor_impl->send_editor(this, SCI_GETTARGETEND, 0, 0), 0);
    }
  }

  return replace_count;
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::jump_to_next_placeholder()
{
  sptr_t current_pos = _code_editor_impl->send_editor(this, SCI_GETCURRENTPOS, 0, 0);
  sptr_t text_size = _code_editor_impl->send_editor(this, SCI_GETLENGTH, 0, 0);
  Sci_TextToFind what;
  
  what.lpstrText = (char*)"<{";
  what.chrg.cpMin = (long)current_pos;
  what.chrg.cpMax = (long)text_size;
  sptr_t result = _code_editor_impl->send_editor(this, SCI_FINDTEXT, 0, (sptr_t)&what);
  bool found = false;
  if (result >= 0)
  {
    static const int max_placeholder_length = 256;
    what.lpstrText = (char*)"}>";
    what.chrg.cpMin = (long)result;
    what.chrg.cpMax = what.chrg.cpMin + max_placeholder_length; // arbitrary max size for placeholder text
    result = _code_editor_impl->send_editor(this, SCI_FINDTEXT, 0, (sptr_t) &what);
    if (result >= 0)
    {
      char buffer[max_placeholder_length];
      TextRange tr;
      tr.chrg.cpMin = what.chrg.cpMin;
      tr.chrg.cpMax = (long)result + 2;
      tr.lpstrText= buffer;
      _code_editor_impl->send_editor(this, SCI_GETTEXTRANGE, 0, (sptr_t)&tr);
      
      if (!memchr(buffer, '\n', tr.chrg.cpMax - tr.chrg.cpMin))
      {
        // jump to placeholder and select it      
        _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, tr.chrg.cpMin, 0);
        _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND, tr.chrg.cpMax, 0);
        _code_editor_impl->send_editor(this, SCI_SCROLLCARET, 0, 0);
        found = true;
      }
    }
  }
  if (!found)
  {
    // restore the cursor to where it was
    _code_editor_impl->send_editor(this, SCI_SETSELECTIONSTART, current_pos, 0);
    _code_editor_impl->send_editor(this, SCI_SETSELECTIONEND, current_pos, 0);
  }
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_show(size_t chars_entered, const std::vector<std::pair<int, std::string> >& entries)
{
  if (entries.size() == 0)
    return;
  
  std::stringstream list;
  for (size_t i = 0; i < entries.size(); ++i)
  {
    if (i > 0)
      list << AC_LIST_SEPARATOR;
    list << entries[i].second;
    if (entries[i].first > -1)
      list << AC_TYPE_SEPARATOR << entries[i].first;
  }
  _code_editor_impl->send_editor(this, SCI_AUTOCSHOW, chars_entered, (sptr_t)list.str().c_str());
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_show(size_t chars_entered, const std::vector<std::string>& entries)
{
  std::stringstream list;
  for (size_t i = 0; i < entries.size(); ++i)
  {
    if (i > 0)
      list << AC_LIST_SEPARATOR;
    list << entries[i];
  }
  _code_editor_impl->send_editor(this, SCI_AUTOCSHOW, chars_entered, (sptr_t)list.str().c_str());
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_cancel()
{
  _code_editor_impl->send_editor(this, SCI_AUTOCCANCEL, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_options(bool ignore_case, bool choose_single, bool auto_hide,
  bool drop_rest_of_word, bool cancel_at_start)
{
  _code_editor_impl->send_editor(this, SCI_AUTOCSETIGNORECASE, ignore_case, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETCHOOSESINGLE, choose_single, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETAUTOHIDE, auto_hide, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETDROPRESTOFWORD, drop_rest_of_word, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETCANCELATSTART, cancel_at_start, 0);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_max_size(int width, int height)
{
  _code_editor_impl->send_editor(this, SCI_AUTOCSETMAXHEIGHT, height, 0);
  _code_editor_impl->send_editor(this, SCI_AUTOCSETMAXWIDTH, width, 0);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_register_images(const std::vector<std::pair<int, std::string> >& images)
{
  for (size_t i = 0; i < images.size(); ++i)
  {
    std::string path = App::get()->get_resource_path(images[i].second);
    if (g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
    {
      if (g_str_has_suffix(path.c_str(), ".png"))
      {
        cairo_surface_t *image = cairo_image_surface_create_from_png(path.c_str());
        if (image == NULL)
          continue;
        if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS)
        {
          cairo_surface_destroy(image);
          continue;
        }
        
        int width = cairo_image_surface_get_width(image);
        int height = cairo_image_surface_get_height(image);
        _code_editor_impl->send_editor(this, SCI_RGBAIMAGESETWIDTH, width, 0);
        _code_editor_impl->send_editor(this, SCI_RGBAIMAGESETHEIGHT, height, 0);

        unsigned char *data = cairo_image_surface_get_data(image);
        
        // We not only need to keep the data around for the lifetime of the editor we also have
        // to swap blue and red. Allocate memory for this in our internal images list. Release what
        // is there already for this id.
        std::map<int, void*>::iterator entry = _images.find(images[i].first);
        if (entry != _images.end())
          free(entry->second);
        
        unsigned char *target = (unsigned char*)malloc(4 * width * height);
        if (target != NULL)
        {
          _images[images[i].first] = target;
          int j = 0;
          while (j < 4 * width * height)
          {
            target[j] = data[j + 2];
            target[j + 1] = data[j + 1];
            target[j + 2] = data[j];
            target[j + 3] = data[j + 3];
            j += 4;
          }
        }
        _code_editor_impl->send_editor(this, SCI_REGISTERRGBAIMAGE, images[i].first, (sptr_t)target);
        cairo_surface_destroy(image);
      }
      else
        if (g_str_has_suffix(path.c_str(), ".xpm"))
        {
          gchar* contents;
          gsize length;
          if (g_file_get_contents(path.c_str(), &contents, &length, NULL))
          {
            _code_editor_impl->send_editor(this, SCI_REGISTERIMAGE, images[i].first, (sptr_t)contents);
            g_free(contents);
          }
        }
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool CodeEditor::auto_completion_active()
{
  return _code_editor_impl->send_editor(this, SCI_AUTOCACTIVE, 0, 0) != 0;
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_stops(const std::string& stops)
{
  _code_editor_impl->send_editor(this, SCI_AUTOCSTOPS, 0, (sptr_t)stops.c_str());
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::auto_completion_fillups(const std::string& fillups)
{
  _code_editor_impl->send_editor(this, SCI_AUTOCSETFILLUPS, 0, (sptr_t)fillups.c_str());
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::show_calltip(bool show, size_t position, const std::string& value)
{
  if (show)
    _code_editor_impl->send_editor(this, SCI_CALLTIPSHOW, position, (sptr_t)value.c_str());
  else
    _code_editor_impl->send_editor(this, SCI_CALLTIPCANCEL, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::set_eol_mode(mforms::EndOfLineMode mode, bool convert)
{
  _code_editor_impl->send_editor(this, SCI_SETEOLMODE, mode, 0);
  if (convert)
    _code_editor_impl->send_editor(this, SCI_CONVERTEOLS, mode, 0);
}

//--------------------------------------------------------------------------------------------------

sptr_t CodeEditor::send_editor(unsigned int message, uptr_t wParam, sptr_t lParam)
{
  return _code_editor_impl->send_editor(this, message, wParam, lParam);
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::lost_focus()
{
  _signal_lost_focus();
}

//--------------------------------------------------------------------------------------------------

void CodeEditor::resize()
{
  if (_scroll_on_resize)
    _code_editor_impl->send_editor(this, SCI_SCROLLCARET, 0, 0);
}

//--------------------------------------------------------------------------------------------------
