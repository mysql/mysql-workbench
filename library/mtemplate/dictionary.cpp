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
void DictionaryInterface::setIntValue(const base::utf8string& key, long int value) 
{
  setValue(key, base::strfmt("%ld", value));
}
  
void DictionaryInterface::setValueAndShowSection(const base::utf8string& key, const base::utf8string &value, const base::utf8string& section) 
{
  if (value.size() == 0)
    return;
  DictionaryInterface *dict = addSectionDictionary(section);
  dict->setValue(key, value);
}

void DictionaryInterface::setFormatedValue(const base::utf8string& key, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  base::utf8string result = base::strfmt(format, args);
  va_end(args);
  
  setValue(key, result);
}

  
//-----------------------------------------------------------------------------------
//  DictionaryGlobal stuff
//-----------------------------------------------------------------------------------
class DictionaryGlobal : public DictionaryInterface
{
protected:
  dictionary_storage _dictionary;
  section_dictionary_storage _no_section;

  DictionaryInterface *getParent()                                     { return NULL; }
  
public:
  
  DictionaryGlobal() : DictionaryInterface("global")                    {  }
  ~DictionaryGlobal()                                                   {  }
  
  //  DictionaryInterface
  virtual void setValue(const base::utf8string &key, const base::utf8string &value)                  { _dictionary[key] = value; }
  virtual base::utf8string getValue(const base::utf8string &key)                                      { return _dictionary.find(key) == _dictionary.end() ? "" : _dictionary[key]; }
  
  virtual DictionaryInterface *addSectionDictionary(const base::utf8string &name)              { return NULL; }
  virtual section_dictionary_storage &getSectionDictionaries(const base::utf8string &sections) { return _no_section; }

  virtual void dump(int indent)
  {
    base::utf8string indent_str(indent * 2, ' ');
    base::utf8string indent_plus_str((indent + 1) * 2, ' ');

    std::cout << indent_str << "[" << _name << "] = " << std::endl
              << indent_str << "{" << std::endl;
    
    for (std::map<base::utf8string, base::utf8string>::iterator iter = _dictionary.begin(); iter != _dictionary.end(); ++iter)
      std::cout << indent_plus_str << "[" << iter->first << "] = \"" << iter->second << "\"" << std::endl;
    
    std::cout << indent_str << "}" << std::endl;
  }
} GlobalDictionary;

//-----------------------------------------------------------------------------------
//  Dictionary stuff
//-----------------------------------------------------------------------------------
void Dictionary::setValue(const base::utf8string& key, const base::utf8string& value)
{ 
  _dictionary[ key ] = value; 
}

base::utf8string Dictionary::getValue(const base::utf8string& key)                      
{ 
  if (_dictionary.find(key) != _dictionary.end())
    return _dictionary[key];
  
  if (_parent)
    return _parent->getValue(key);
  
  return GlobalDictionary.getValue(key);
}

DictionaryInterface* Dictionary::addSectionDictionary(const base::utf8string& name) 
{ 
  base::utf8string newName = _name + name + base::utf8string("/");
  DictionaryInterface *_sectionDict = new Dictionary(newName, this);
  
  if (_section_dictionaries[name].size() > 0)
    _section_dictionaries[name].back()->setIsLast(false);
  
  _sectionDict->setIsLast(true);
  _section_dictionaries[name].push_back(_sectionDict);
  return _sectionDict;
}

Dictionary::section_dictionary_storage &Dictionary::getSectionDictionaries(const base::utf8string& section)
{
  if (_section_dictionaries.find(section) == _section_dictionaries.end())
    return _no_section;
  return _section_dictionaries[section];
}

void Dictionary::dump(int indent) 
{
  base::utf8string indent_str(indent * 2, ' ');
  base::utf8string indent_plus_str((indent + 1) * 2, ' ');
  
  if (_dictionary.size() == 0 && _section_dictionaries.size() == 0)
  {
    std::cout << indent_str << "[" << _name << "] = " << "{  }" << std::endl;
    return;
  }
  
  
  std::cout << indent_str << "[" << _name << "] = " << std::endl
            << indent_str << "{" << std::endl;
  
  for (std::map<base::utf8string, base::utf8string>::iterator iter = _dictionary.begin(); iter != _dictionary.end(); ++iter)
    std::cout << indent_plus_str << "[" << iter->first << "] = \"" << iter->second << "\"" << std::endl;
  
  for (std::map<base::utf8string, std::vector<DictionaryInterface *> >::iterator iter = _section_dictionaries.begin(); iter != _section_dictionaries.end(); ++iter)
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

void SetGlobalValue(const base::utf8string& key, const base::utf8string& value)
{ 
  GlobalDictionary.setValue(key, value);
}


}   //  namespace mtemplate


