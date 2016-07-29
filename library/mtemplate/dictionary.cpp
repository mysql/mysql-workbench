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
#include "stdafx.h"
#include "dictionary.h"
#include "string_utilities.h"

#include <iostream>

namespace mtemplate {

//-----------------------------------------------------------------------------------
//  DictionaryInterface stuff
//-----------------------------------------------------------------------------------
void DictionaryInterface::set_int_value(const std::string& key, long int value) 
{
  set_value(key, base::strfmt("%ld", value));
}
  
void DictionaryInterface::set_value_and_show_section(const std::string& key, const std::string &value, const std::string& section) 
{
  if (value.size() == 0)
    return;
  DictionaryInterface *dict = add_section_dictionary(section);
  dict->set_value(key, value);
}

void DictionaryInterface::set_formated_value(const std::string& key, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  std::string result = base::strfmt(format, args);
  va_end(args);
  
  set_value(key, result);
}

  
//-----------------------------------------------------------------------------------
//  DictionaryGlobal stuff
//-----------------------------------------------------------------------------------
class DictionaryGlobal : public DictionaryInterface
{
protected:
  dictionary_storage _dictionary;
  section_dictionary_storage _no_section;

  DictionaryInterface *get_parent()                                     { return NULL; }
  
public:
  
  DictionaryGlobal() : DictionaryInterface("global")                    {  }
  ~DictionaryGlobal()                                                   {  }
  
  //  DictionaryInterface
  virtual void set_value(const std::string &key, const std::string &value)                  { _dictionary[key] = value; }
  virtual std::string get_value(const std::string &key)                                      { return _dictionary.find(key) == _dictionary.end() ? "" : _dictionary[key]; }
  
  virtual DictionaryInterface *add_section_dictionary(const std::string &name)              { return NULL; }
  virtual section_dictionary_storage &get_section_dictionaries(const std::string &sections) { return _no_section; }

  virtual void dump(int indent)
  {
    std::string indent_str(indent * 2, ' ');
    std::string indent_plus_str((indent + 1) * 2, ' ');

    std::cout << indent_str << "[" << _name << "] = " << std::endl
              << indent_str << "{" << std::endl;
    
    for (std::map<std::string, std::string>::iterator iter = _dictionary.begin(); iter != _dictionary.end(); ++iter)
      std::cout << indent_plus_str << "[" << iter->first << "] = \"" << iter->second << "\"" << std::endl;
    
    std::cout << indent_str << "}" << std::endl;
  }
} GlobalDictionary;

//-----------------------------------------------------------------------------------
//  Dictionary stuff
//-----------------------------------------------------------------------------------
void Dictionary::set_value(const std::string& key, const std::string& value)
{ 
  _dictionary[ key ] = value; 
}

std::string Dictionary::get_value(const std::string& key)                      
{ 
  if (_dictionary.find(key) != _dictionary.end())
    return _dictionary[key];
  
  if (_parent)
    return _parent->get_value(key);
  
  return GlobalDictionary.get_value(key);
}

DictionaryInterface* Dictionary::add_section_dictionary(const std::string& name) 
{ 
  std::string newName = _name + name + std::string("/");
  DictionaryInterface *_sectionDict = new Dictionary(newName, this);
  
  if (_section_dictionaries[name].size() > 0)
    _section_dictionaries[name].back()->set_is_last(false);
  
  _sectionDict->set_is_last(true);
  _section_dictionaries[name].push_back(_sectionDict);
  return _sectionDict;
}

Dictionary::section_dictionary_storage &Dictionary::get_section_dictionaries(const std::string& section)
{
  if (_section_dictionaries.find(section) == _section_dictionaries.end())
    return _no_section;
  return _section_dictionaries[section];
}

void Dictionary::dump(int indent) 
{
  std::string indent_str(indent * 2, ' ');
  std::string indent_plus_str((indent + 1) * 2, ' ');
  
  if (_dictionary.size() == 0 && _section_dictionaries.size() == 0)
  {
    std::cout << indent_str << "[" << _name << "] = " << "{  }" << std::endl;
    return;
  }
  
  
  std::cout << indent_str << "[" << _name << "] = " << std::endl
            << indent_str << "{" << std::endl;
  
  for (std::map<std::string, std::string>::iterator iter = _dictionary.begin(); iter != _dictionary.end(); ++iter)
    std::cout << indent_plus_str << "[" << iter->first << "] = \"" << iter->second << "\"" << std::endl;
  
  for (std::map<std::string, std::vector<DictionaryInterface *> >::iterator iter = _section_dictionaries.begin(); iter != _section_dictionaries.end(); ++iter)
    for (std::vector<DictionaryInterface *>::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2)
      (*iter2)->dump(indent + 1);

  std::cout << indent_str << "}" << std::endl;
}

//-----------------------------------------------------------------------------------
//  General stuff
//-----------------------------------------------------------------------------------
Dictionary *CreateMainDictionary()
{
  return new Dictionary("/", NULL);
}

void SetGlobalValue(const std::string& key, const std::string& value)
{ 
  GlobalDictionary.set_value(key, value);
}


}   //  namespace mtemplate


