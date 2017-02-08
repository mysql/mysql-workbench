/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "base/utf8string.h"

#ifndef HAVE_PRECOMPILED_HEADERS
#include <map>
#include <vector>
#include <glib.h>
#endif

namespace mtemplate {

  struct NodeSection;
  class Template;

  class MTEMPLATELIBRARY_PUBLIC_FUNC DictionaryInterface {
  protected:
    base::utf8string _name;
    bool _is_last;

    DictionaryInterface(const base::utf8string &name) : _name(name), _is_last(false) {
    }

    typedef std::map<base::utf8string, base::utf8string> dictionary_storage;
    typedef dictionary_storage::iterator dictionary_storage_iterator;

    typedef std::vector<DictionaryInterface *> section_dictionary_storage;
    typedef section_dictionary_storage::iterator section_dictionary_storage_iterator;

    typedef std::map<base::utf8string, section_dictionary_storage> section_storage;

    virtual DictionaryInterface *getParent() = 0;

    friend NodeSection;
    friend Template;

  public:
    virtual ~DictionaryInterface() {
    }

    virtual void setValue(const base::utf8string &key, const base::utf8string &value) = 0;
    virtual base::utf8string getValue(const base::utf8string &key) = 0;

    void setIntValue(const base::utf8string &key, long value);
    void setValueAndShowSection(const base::utf8string &key, const base::utf8string &value,
                                const base::utf8string &section);
    void setFormatedValue(const base::utf8string &key, const char *format, ...); // G_GNUC_PRINTF(2, 3);

    virtual DictionaryInterface *addSectionDictionary(const base::utf8string &name) = 0;
    virtual section_dictionary_storage &getSectionDictionaries(const base::utf8string &sections) = 0;

    void setIsLast(bool value) {
      _is_last = value;
    }
    bool isLast() {
      return _is_last;
    }

    virtual void dump(int indent = 0) = 0;
  };

  class MTEMPLATELIBRARY_PUBLIC_FUNC Dictionary : public DictionaryInterface {
  protected:
    DictionaryInterface *_parent;

    dictionary_storage _dictionary;
    section_storage _section_dictionaries;
    section_dictionary_storage _no_section;

    DictionaryInterface *getParent() {
      return _parent;
    }

  public:
    Dictionary(const base::utf8string &name, DictionaryInterface *parent = NULL)
      : DictionaryInterface(name), _parent(parent) {
    }
    virtual ~Dictionary() {
    }

    //  DictionaryInterface
    virtual void setValue(const base::utf8string &key, const base::utf8string &value);
    virtual base::utf8string getValue(const base::utf8string &key);

    virtual DictionaryInterface *addSectionDictionary(const base::utf8string &name);
    virtual section_dictionary_storage &getSectionDictionaries(const base::utf8string &section);

    virtual void dump(int indent = 0);
  };

  MTEMPLATELIBRARY_PUBLIC_FUNC Dictionary *CreateMainDictionary();
  MTEMPLATELIBRARY_PUBLIC_FUNC void SetGlobalValue(const base::utf8string &key, const base::utf8string &value);

} //  namespace mtemplate
