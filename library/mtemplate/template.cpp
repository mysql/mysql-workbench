/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "template.h"
#include <base/file_functions.h>
#include <sstream>
#include <iostream>
#include "dictionary.h"
#include "modifier.h"

#include <base/string_utilities.h>
#include <base/file_utilities.h>

namespace mtemplate {

  Template::Template(TemplateDocument document) : _document(document) {
  }

  Template::~Template() {
  }

  void Template::dump(int indent) {
    base::utf8string indent_str(indent * 2, ' ');
    base::utf8string indent_plus_str((indent + 1) * 2, ' ');

    std::cout << indent_str << "[Temaplate] = " << std::endl << indent_str << "{" << std::endl;

    for (NodeStorageType node : _document)
      node->dump(indent + 1);

    std::cout << indent_str << "}" << std::endl;
  }

  void Template::expand(DictionaryInterface *dict, TemplateOutput *output) {
    for (NodeStorageType node : _document) {
      if (node->type() == TemplateObject_Section) {
        DictionaryInterface::section_dictionary_storage &section_dicts = dict->getSectionDictionaries(node->_text);

        for (DictionaryInterface::section_dictionary_storage_iterator section_iter = section_dicts.begin();
             section_iter != section_dicts.end(); ++section_iter)
          node->expand(output, *section_iter);
      } else
        node->expand(output, dict);
    }
  }

  Template *GetTemplate(const base::utf8string &path, PARSE_TYPE type) {
    if (type == STRIP_WHITESPACE)
      throw std::invalid_argument("STRIP_WHITESPACE");

    if (base::file_exists(path) == false)
      return NULL;

    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();

    Template *temp = new Template(parseTemplate(buffer.str(), type));

    return temp;
  }

} //  namespace mtemplate
