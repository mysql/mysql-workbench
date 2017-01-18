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
