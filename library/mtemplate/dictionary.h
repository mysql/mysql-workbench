/*
 * Copyright (c) 2007, 2016, Oracle and/or its affiliates. All rights reserved.
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
 * 
 */

#pragma once

#include "common.h"

#include <string>
#include <map>
#include <vector>

#if !defined(_WIN32) && !defined(__APPLE)
  #include <glib.h>
#endif

namespace mtemplate {

struct NodeSection;
class Template;
  
class DictionaryInterface
{
protected:
  
  std::string _name;
  bool _is_last;
  
  DictionaryInterface(const std::string &name) : _name(name), _is_last(false)    {  }
  
  typedef std::map<std::string, std::string> dictionary_storage;
  typedef dictionary_storage::iterator dictionary_storage_iterator;
  
  typedef std::vector<DictionaryInterface *> section_dictionary_storage;
  typedef section_dictionary_storage::iterator section_dictionary_storage_iterator;
  
  typedef std::map<std::string, section_dictionary_storage> section_storage;

  virtual DictionaryInterface *get_parent() = 0;

  friend NodeSection;
  friend Template;
  
public:
  
  virtual ~DictionaryInterface()                                {  }
  
  virtual void set_value(const std::string &key, const std::string &value) = 0;
  virtual std::string get_value(const std::string &key) = 0;
  
  void set_int_value(const std::string &key, long value);
  void set_value_and_show_section(const std::string &key, const std::string &value, const std::string &section);
  void set_formated_value(const std::string &key, const char *format, ...); // G_GNUC_PRINTF(2, 3);

  virtual DictionaryInterface *add_section_dictionary(const std::string &name) = 0;
  virtual section_dictionary_storage &get_section_dictionaries(const std::string &sections) = 0;
  
  void set_is_last(bool value)  { _is_last = value; }
  bool is_last()                { return _is_last; }
  
  virtual void dump(int indent = 0) = 0;
  
};
  
class Dictionary : public DictionaryInterface
{
protected:
  
  DictionaryInterface *_parent;
  
  dictionary_storage _dictionary;
  section_storage _section_dictionaries;
  section_dictionary_storage _no_section;

  DictionaryInterface *get_parent(){ return _parent; }
  
public:
  
  Dictionary(const std::string &name, DictionaryInterface *parent = NULL) 
    : DictionaryInterface(name), _parent(parent)                                  {  }
  virtual ~Dictionary()                                             {  }
  
  //  DictionaryInterface
  virtual void set_value(const std::string &key, const std::string &value);
  virtual std::string get_value(const std::string &key);
  
  virtual DictionaryInterface *add_section_dictionary(const std::string &name);
  virtual section_dictionary_storage &get_section_dictionaries(const std::string &section);

  virtual void dump(int indent = 0);
};


Dictionary *CreateMainDictionary();
void SetGlobalValue(const std::string &key, const std::string &value);

}   //  namespace mtemplate

