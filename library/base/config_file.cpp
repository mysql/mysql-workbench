/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

/**
 * Configuration file reader/writer.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <float.h>

#include "base/config_file.h"
#include "base/util_functions.h"
#include "base/string_utilities.h"

#include "base/log.h"

DEFAULT_LOG_DOMAIN(DOMAIN_BASE);

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

using namespace std;
using namespace base;

// What character separates key and value.
const std::string EqualIndicators = "=:";

typedef struct {
  std::string key;
  std::string value;
  std::string pre_comment;  // A comment before the entry (separate line(s)).
  std::string post_comment; // A comment after the entry (same line).
} ConfigEntry;

typedef std::vector<ConfigEntry> EntryList; // Must be a vector instead of a map to preserve order and
                                            // allow multiple values with the same key (includes).
typedef EntryList::iterator EntryListIterator;

typedef struct {
  std::string name;
  std::string comment; // A comment placed before a section start.
  EntryList keys;
} ConfigSection;

typedef std::vector<ConfigSection> SectionList;
typedef SectionList::iterator SectionListIterator;

/**
 * Extracts the next key from the next key/value pair in the line and returns it.
 */
std::string extract_next_word(std::string &line) {
  size_t position = line.find_first_of(EqualIndicators);
  std::string word = std::string("");

  if (position != std::string::npos) {
    word = line.substr(0, position);
    line.erase(0, position + 1);
  } else {
    word = line;
    line = "";
  }

  return base::trim(word);
}

//----------------- ConfigurationFile::Private -----------------------------------------------------

class ConfigurationFile::Private {
public:
  int _flags;
  SectionList _sections;
  bool _dirty;
  std::string _trailing_comments; // Any comments after the last key-value or section entry.

  Private(std::string file_name, ConfigFileFlags flags);

  ConfigEntry *get_entry_in_section(std::string key, std::string section, bool auto_create);
  ConfigSection *get_section(std::string section, bool auto_create);

  void clear_includes(const std::string &section_name);
  void add_include(const std::string &section_name, const std::string &include);
  void add_include_dir(const std::string &section_name, const std::string &include);
  std::vector<std::string> get_includes(const std::string &section_name);

  bool set_value(std::string key_name, std::string value, std::string section_name);
  void set_dirty();
  void clear();

  bool create_key(std::string key, std::string value, std::string pre_comment, std::string post_comment,
                  std::string section);
  bool delete_key(std::string key, std::string section_name);
  bool create_section(std::string section_name, std::string comment);
  bool delete_section(std::string name);

  int section_count();
  int key_count();
  int key_count_for_section(const std::string &section_name);
  bool is_dirty();

  std::string make_comment(const std::string &text);

  bool load(const std::string &file_name);
  bool save(const std::string &file_name);
};

//--------------------------------------------------------------------------------------------------

ConfigurationFile::Private::Private(std::string file_name, ConfigFileFlags flags) {
  _dirty = false;
  _flags = flags;

  // Always create a default section.
  _sections.push_back(ConfigSection());

  if (!file_name.empty())
    load(file_name);
}

//--------------------------------------------------------------------------------------------------

/**
 * Looks up the given key in the given section and returns its entry if found, otherwise NULL.
 */
ConfigEntry *ConfigurationFile::Private::get_entry_in_section(std::string key, std::string section_name,
                                                              bool auto_create) {
  ConfigSection *section = get_section(section_name, auto_create && (_flags & AutoCreateSections) != 0);

  if (section == NULL)
    return NULL;

  for (EntryListIterator iterator = section->keys.begin(); iterator != section->keys.end(); iterator++) {
    if (strcasecmp(iterator->key.c_str(), key.c_str()) == 0)
      return &(*iterator);
  }

  if (auto_create) {
    _dirty = true;
    ConfigEntry new_entry;
    new_entry.key = base::trim(key);
    section->keys.push_back(new_entry);
    return &section->keys.back();
  }

  return NULL;
}

//--------------------------------------------------------------------------------------------------

/**
 * Looks up the given section in our list and returns a pointer to it if found.
 * If the section could not be found and auto_create is true then it is created and returned.
 * Otherwise NULL is returned.
 */
ConfigSection *ConfigurationFile::Private::get_section(std::string section_name, bool auto_create) {
  section_name = base::trim(section_name);
  for (SectionListIterator iterator = _sections.begin(); iterator != _sections.end(); iterator++) {
    if (strcasecmp(iterator->name.c_str(), section_name.c_str()) == 0)
      return &(*iterator);
  }

  if (auto_create) {
    create_section(section_name, "");
    return &_sections.back();
  }

  return NULL;
}

//--------------------------------------------------------------------------------------------------

bool is_include(ConfigEntry &entry) {
  std::string key = base::tolower(entry.key);
  return key == "!include" || key == "!includedir";
}

void ConfigurationFile::Private::clear_includes(const std::string &section_name) {
  ConfigSection *section = get_section(section_name, (_flags & AutoCreateSections) != 0);

  if (section == NULL)
    return;

  section->keys.erase(std::remove_if(section->keys.begin(), section->keys.end(), is_include), section->keys.end());
  _dirty = true;
}

//--------------------------------------------------------------------------------------------------

void ConfigurationFile::Private::add_include(const std::string &section_name, const std::string &include) {
  ConfigSection *section = get_section(section_name, (_flags & AutoCreateSections) != 0);

  if (section == NULL)
    return;

  ConfigEntry *entry = get_entry_in_section("!include", section->name, true);
  entry->value = include;

  _dirty = true;
}

//--------------------------------------------------------------------------------------------------

void ConfigurationFile::Private::add_include_dir(const std::string &section_name, const std::string &include) {
  ConfigSection *section = get_section(section_name, (_flags & AutoCreateSections) != 0);

  if (section == NULL)
    return;

  ConfigEntry *entry = get_entry_in_section("!includedir", section->name, true);
  entry->value = include;

  _dirty = true;
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> ConfigurationFile::Private::get_includes(const std::string &section_name) {
  std::vector<std::string> result;
  ConfigSection *section = get_section(section_name, (_flags & AutoCreateSections) != 0);

  if (section != NULL) {
    for (EntryListIterator iterator = section->keys.begin(); iterator != section->keys.end(); iterator++) {
      if (is_include(*iterator))
        result.push_back(iterator->value);
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets the value for the given key in the given section. If the section does not exist yet and
 * the AutoCreateSections flag is set then it is created first. Similarly for the key (and the
 * AutoCreateKeys flag).
 * Returns true if the value could be set, false otherwise.
 */
bool ConfigurationFile::Private::set_value(std::string key_name, std::string value, std::string section_name) {
  ConfigEntry *entry = get_entry_in_section(key_name, section_name, (_flags & AutoCreateKeys) != 0);

  if (entry == NULL)
    return false;

  entry->value = base::trim(value);
  _dirty = true;

  return true;
}

//--------------------------------------------------------------------------------------------------

void ConfigurationFile::Private::set_dirty() {
  _dirty = true;
}

//--------------------------------------------------------------------------------------------------

void ConfigurationFile::Private::clear() {
  _dirty = false;
  _trailing_comments = "";
  _sections.clear();
  _sections.push_back(ConfigSection()); // Make sure we always have a default section.
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::Private::delete_section(std::string name) {
  name = base::trim(name);
  if (name.empty()) // Don't remove the default section.
    return false;

  for (SectionListIterator iterator = _sections.begin(); iterator != _sections.end(); iterator++) {
    if (strcasecmp((*iterator).name.c_str(), name.c_str()) == 0) {
      _sections.erase(iterator);
      return true;
    }
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes the given key from the given selection. Returns true if successful, otherwise false.
 */
bool ConfigurationFile::Private::delete_key(std::string key, std::string section_name) {
  ConfigSection *section = get_section(section_name, false);

  if (section == NULL)
    return false;

  key = base::trim(key);
  for (EntryListIterator iterator = section->keys.begin(); iterator != section->keys.end(); iterator++) {
    if (strcasecmp(iterator->key.c_str(), key.c_str()) == 0) {
      section->keys.erase(iterator);
      return true;
    }
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Given a key, a value and a section, this function will attempt to locate the
 * Key within the given section, and if it finds it, change the keys value to
 * the new value. If it does not locate the key, it will create a new key with
 * the proper value and place it in the section requested.
 */
bool ConfigurationFile::Private::create_key(std::string key_name, std::string value, std::string pre_comment,
                                            std::string post_comment, std::string section_name) {
  ConfigEntry *entry = get_entry_in_section(key_name, section_name, true);

  if (entry == NULL)
    return false;

  entry->value = base::trim(value);
  entry->pre_comment = pre_comment;
  entry->post_comment = post_comment;
  _dirty = true;

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates a new section if no section with that name exists already. Returns true if a new section
 * has been created, otherwise false.
 */
bool ConfigurationFile::Private::create_section(std::string section_name, std::string comment) {
  // TODO: this should be necessary.
  if (get_section(section_name, false) != NULL)
    return false;

  ConfigSection section;
  section.name = base::trim(section_name);
  section.comment = comment;
  _sections.push_back(section);
  _dirty = true;

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the number of sections.
 */
int ConfigurationFile::Private::section_count() {
  return (int)_sections.size();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the total number of keys in all sections.
 */
int ConfigurationFile::Private::key_count() {
  int result = 0;

  for (SectionListIterator iterator = _sections.begin(); iterator != _sections.end(); iterator++)
    result += (int)iterator->keys.size();

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the number of keys in the given section. Returns 0 if the section could not be found.
 */
int ConfigurationFile::Private::key_count_for_section(const std::string &section_name) {
  ConfigSection *section = get_section(section_name, false);
  if (section != NULL)
    return (int)section->keys.size();

  return 0;
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::Private::is_dirty() {
  return _dirty;
}

//--------------------------------------------------------------------------------------------------

/**
 * Converts the given text to a comment string (prefixing it with the comment character) if it is
 * not empty and does not yet have a comment character at the first position.
 */
std::string ConfigurationFile::Private::make_comment(const std::string &text) {
  if (text.size() == 0)
    return text;

  if (text[0] != '#' && text[0] != ';')
    return "# " + text;

  return text;
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::Private::load(const std::string &file_name) {
  ifstream file(file_name.c_str());

  if (file.is_open()) {
    bool done = false;

    std::string line;
    std::string comment;
    ConfigSection *section = get_section("", true); // Start with the default section which must always be there.

    while (!done) {
      std::getline(file, line);

      line = base::trim(line);
      done = file.eof() || file.bad() || file.fail();

      // Skip empty lines.
      if (line.size() == 0)
        continue;

      switch (line[0]) {
        case '#':
        case ';':
          if (!comment.empty())
            comment += "\n";
          comment += line;
          break;

        case '[': {
          // Start a new section.
          size_t closing_bracket_position = line.rfind(']');
          if (closing_bracket_position != std::string::npos)
            line.erase(closing_bracket_position, 1);
          else
            logWarning("Unterminated section specifier found in file %s: %s\n", file_name.c_str(), line.c_str());

          line.erase(0, 1);
          section = get_section(line, true);
          section->comment = comment;
          comment = "";
        } break;

        case '!': {
          std::string post_comment;
          std::string::size_type comment_position = line.find('#'); // For post comments only # is allowed.
          if (comment_position != std::string::npos) {
            post_comment = line.substr(comment_position, line.size());
            line = line.substr(0, comment_position - 1);
          }

          std::vector<std::string> parts = base::split(line, " ");
          if (parts.size() == 2) {
            std::string key = base::tolower(parts[0]);
            if (key == "!include" || key == "!includedir") {
              ConfigEntry *entry = get_entry_in_section(key, section->name, true);
              entry->value = parts[1];
              entry->pre_comment = comment;
              entry->post_comment = post_comment;
              comment = "";
            } else
              logWarning("Invalid include sequence found in file %s: %s\n", file_name.c_str(), line.c_str());
          } else
            logWarning("Invalid include sequence found in file %s: %s\n", file_name.c_str(), line.c_str());
        } break;

        default:
          if (line.size() > 0) {
            // Normal key/value pair or a single key.
            std::string post_comment;
            std::string::size_type comment_position = line.find('#'); // For post comments only # is allowed.
            if (comment_position != std::string::npos) {
              post_comment = line.substr(comment_position, line.size());
              line = line.substr(0, comment_position - 1);
            }
            std::string key = extract_next_word(line);
            std::string value = line;

            if (key.size() > 0) {
              ConfigEntry *entry = get_entry_in_section(key, section->name, true);
              entry->value = base::trim(value);
              entry->pre_comment = comment;
              entry->post_comment = post_comment;
              comment = "";
            }
          }
          break;
      }
    }

    _trailing_comments = comment;
  } else
    return false;

  file.close();

  return true;
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::Private::save(const std::string &file_name) {
  if (file_name.size() == 0)
    return false;

  fstream output(file_name.c_str(), ios::out | ios::trunc);

  if (output.is_open()) {
    SectionListIterator section_iterator;
    EntryListIterator entry_iterator;
    ConfigEntry entry;

    bool empty = true; // No output written yet.

    // We removed all empty lines on load, so add one for each pre-comment, section comment or section entry
    // (except for the very first entry).
    for (section_iterator = _sections.begin(); section_iterator != _sections.end(); section_iterator++) {
      if (!empty)
        output << '\n';

      if (!section_iterator->comment.empty()) {
        output << make_comment(section_iterator->comment) << '\n';
        empty = false;
      }

      if (!section_iterator->name.empty()) {
        output << "[" << section_iterator->name.c_str() << "]\n";
        empty = false;
      }

      for (entry_iterator = section_iterator->keys.begin(); entry_iterator != section_iterator->keys.end();
           entry_iterator++) {
        if (!entry_iterator->pre_comment.empty()) {
          if (!empty)
            output << '\n';

          output << make_comment(entry_iterator->pre_comment) << '\n';
        }

        empty = false;
        if (!entry_iterator->key.empty()) {
          if (entry_iterator->key.find("!include") == 0) // Include entry?
            output << entry_iterator->key << ' ' << entry_iterator->value;
          else {
            if (!entry_iterator->value.empty())
              output << entry_iterator->key << ' ' << EqualIndicators[0] << ' ' << entry_iterator->value;
            else
              output << entry_iterator->key;
          }
        }
        if (!entry_iterator->post_comment.empty())
          output << ' ' << make_comment(entry_iterator->post_comment);
        output << '\n';
      }
    }

    if (!_trailing_comments.empty()) {
      if (!empty)
        output << '\n';
      output << make_comment(_trailing_comments) << '\n';
    }

  } else
    return false;

  _dirty = false;

  output.flush();
  output.close();

  return true;
}

//----------------- ConfigurationFile --------------------------------------------------------------

ConfigurationFile::ConfigurationFile(std::string file_name, ConfigFileFlags flags) {
  data = new Private(file_name, flags);
}

//--------------------------------------------------------------------------------------------------

ConfigurationFile::ConfigurationFile(ConfigFileFlags flags) {
  data = new Private("", flags);
}

//--------------------------------------------------------------------------------------------------

ConfigurationFile::~ConfigurationFile() {
  delete data;
}

//--------------------------------------------------------------------------------------------------

void ConfigurationFile::clear_includes(const std::string &section_name) {
  data->clear_includes(section_name);
}

//--------------------------------------------------------------------------------------------------

void ConfigurationFile::add_include(const std::string &section_name, const std::string &include) {
  data->add_include(section_name, include);
}

//--------------------------------------------------------------------------------------------------

void ConfigurationFile::add_include_dir(const std::string &section_name, const std::string &include) {
  data->add_include_dir(section_name, include);
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> ConfigurationFile::get_includes(const std::string &section_name) {
  return data->get_includes(section_name);
}

//--------------------------------------------------------------------------------------------------

void ConfigurationFile::clear() {
  data->clear();
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::is_dirty() {
  return data->is_dirty();
}

//--------------------------------------------------------------------------------------------------

/**
 * Checks if the a key in a given section exists.
 */
bool ConfigurationFile::has_key(const std::string &key, const std::string &section) {
  return data->get_entry_in_section(key, section, false) != NULL;
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::has_section(const std::string &section_name) {
  return data->get_section(section_name, false) != NULL;
}

//--------------------------------------------------------------------------------------------------

/**
 * Loads the given file and adds its content to the current content. Comments will be retained.
 */
bool ConfigurationFile::load(const std::string &file_name) {
  return data->load(file_name);
}

//--------------------------------------------------------------------------------------------------

/**
 * Saves the content to the given file.
 */
bool ConfigurationFile::save(const std::string &file_name) {
  return data->save(file_name);
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets the comment for a given entry. Returns true if the entry was found, otherwise false.
 */
bool ConfigurationFile::set_key_pre_comment(std::string key, std::string comment, std::string section_name) {
  ConfigEntry *entry = data->get_entry_in_section(key, section_name, (data->_flags & AutoCreateKeys) != 0);
  if (entry == NULL)
    return false;

  data->_dirty = true;
  entry->pre_comment = comment;

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets the comment for a given section. Returns true if the section was found, otherwise false.
 */
bool ConfigurationFile::set_section_comment(std::string section_name, std::string comment) {
  ConfigSection *section = data->get_section(section_name, (data->_flags & AutoCreateSections) != 0);
  if (section == NULL)
    return false;

  data->set_dirty();
  section->comment = comment;
  return true;
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::set_value(std::string key_name, std::string value, std::string section_name) {
  return data->set_value(key_name, value, section_name);
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::set_float(std::string key, float value, std::string section) {
  char buffer[64];
  snprintf(buffer, 64, "%f", value);

  return data->set_value(key, buffer, section);
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::set_int(std::string key, int value, std::string section) {
  char buffer[64];
  snprintf(buffer, 64, "%d", value);

  return data->set_value(key, buffer, section);
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::set_bool(std::string key, bool value, std::string section) {
  return data->set_value(key, value ? "True" : "False", section);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the value of the given key as string. An empty string is returned if the key could not be
 * found or is empty.
 */
std::string ConfigurationFile::get_value(std::string key, std::string section) {
  ConfigEntry *entry = data->get_entry_in_section(key, section, false);

  return (entry == NULL) ? "" : entry->value;
}

//--------------------------------------------------------------------------------------------------

double ConfigurationFile::get_float(std::string key, std::string section) {
  std::string value = base::unquote_identifier(get_value(key, section));

  if (value.size() == 0)
    return FLT_MIN;

  // This conversion handles invalid entries with more than one letter at the end
  // (those are simply ignored). The right size computation is done in both cases.
  // There's no error handling in case of an overflow.
  double factor = 1;
  switch (::tolower(value[value.size() - 1])) {
    // All cases fall through.
    case 'g':
      factor *= 1024;
      /* fall-thru */
    case 'm':
      factor *= 1024;
      /* fall-thru */
    case 'k':
      factor *= 1024;
      value[value.size() - 1] = 0;
      /* fall-thru */
  }
  return factor * base::atof<float>(value, 0.0);
}

//--------------------------------------------------------------------------------------------------

int ConfigurationFile::get_int(std::string key, std::string section) {
  std::string value = base::unquote_identifier(get_value(key, section));

  if (value.size() == 0)
    return INT_MIN;

  // Same as for get_float().
  // Note: for int overflow can happen much quicker with G suffix or large values.
  int factor = 1;
  switch (::tolower(value[value.size() - 1])) {
    // All cases fall through.
    case 'g':
      factor *= 1024;
      /* fall-thru */
    case 'm':
      factor *= 1024;
      /* fall-thru */
    case 'k':
      factor *= 1024;
      value[value.size() - 1] = 0;
      /* fall-thru */
  }

  return factor * base::atoi<int>(value, 0);
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::get_bool(std::string key, std::string section) {
  std::string value = base::tolower(base::unquote_identifier(get_value(key, section)));

  if (value == "true" || value == "yes")
    return true;

  if (base::atoi<int>(value, 0) != 0)
    return true;

  return false;
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::delete_section(std::string name) {
  return data->delete_section(name);
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::delete_key(std::string key, std::string section_name) {
  return data->delete_key(key, section_name);
}

//--------------------------------------------------------------------------------------------------

bool ConfigurationFile::create_key(std::string key, std::string value, std::string pre_comment,
                                   std::string post_comment, std::string section) {
  return data->create_key(key, value, pre_comment, post_comment, section);
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates a new section if no section with that name exists already. Returns true if a new section
 * has been created, otherwise false.
 */
bool ConfigurationFile::create_section(std::string section_name, std::string comment) {
  return data->create_section(section_name, comment);
}

//--------------------------------------------------------------------------------------------------

int ConfigurationFile::section_count() {
  return data->section_count();
}

//--------------------------------------------------------------------------------------------------

int ConfigurationFile::key_count() {
  return data->key_count();
}

//--------------------------------------------------------------------------------------------------

int ConfigurationFile::key_count_for_secton(const std::string &section_name) {
  return data->key_count_for_section(section_name);
}

//--------------------------------------------------------------------------------------------------
