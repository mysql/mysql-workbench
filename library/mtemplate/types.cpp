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

#include "types.h"
#include "output.h"
#include "dictionary.h"

#include <iostream>

#include <base/string_utilities.h>

namespace mtemplate {
  static const base::utf8string TEMPLATE_TAG_BEGINNING("{{");
  static const base::utf8string TEMPLATE_TAG_END("}}");
  static const base::utf8string TEMPLATE_SECTION_BEGINNING("#");
  static const base::utf8string TEMPLATE_SECTION_END("/");

  static const base::utf8string TEMPLATE_TAG_CHARACTERS("#/ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

  std::size_t GetTextLength(const base::utf8string &temp_template, bool check_new_lines = true);
  bool IsBlankString(const base::utf8string &text);
  base::utf8string FormatErrorLog(const base::utf8string &template_string, std::size_t pos);

  //-----------------------------------------------------------------------------------
  //  NodeText stuff
  //-----------------------------------------------------------------------------------
  NodeText::NodeText(const base::utf8string &text, std::size_t length)
    : NodeTextInterface(TemplateObject_Text, text, length) {
  }

  bool NodeText::expand(TemplateOutput *output, DictionaryInterface *dict) {
    if (isHidden())
      return true;

    output->out(_text);
    return true;
  }
  //-----------------------------------------------------------------------------------
  void NodeText::dump(int indent) {
    base::utf8string hidden = isHidden() ? "[hidden]" : "";
    base::utf8string indent_str(indent * 2, ' ');
    std::cout << indent_str << "[Text]" << hidden << " = " << _text << std::endl;
  }
  //-----------------------------------------------------------------------------------
  NodeText *NodeText::parse(const base::utf8string &template_string, PARSE_TYPE type) {
    std::size_t end = GetTextLength(template_string);

    if (end == base::utf8string::npos)
      end = template_string.length();

    base::utf8string text = template_string.substr(0, end);

    return new NodeText(text, end);
  }

  //-----------------------------------------------------------------------------------
  //  NodeNewLine stuff
  //-----------------------------------------------------------------------------------
  void NodeNewLine::dump(int indent) {
    base::utf8string hidden = isHidden() ? "[hidden]" : "";
    base::utf8string indent_str(indent * 2, ' ');
    std::cout << indent_str << "[NewLine]" << hidden << std::endl;
  }

  bool NodeNewLine::expand(TemplateOutput *output, DictionaryInterface *dict) {
    if (isHidden())
      return true;

    output->out("\n");
    return true;
  }

  NodeNewLine *NodeNewLine::parse(const base::utf8string &template_string, PARSE_TYPE type) {
    return new NodeNewLine();
  }
  //-----------------------------------------------------------------------------------
  //  NodeVariable stuff
  //-----------------------------------------------------------------------------------
  bool NodeVariable::expand(TemplateOutput *output, DictionaryInterface *dict) {
    if (isHidden())
      return true;

    base::utf8string result = dict->getValue(_text);

    //   if (result == "")
    //     std::cout << "WARNING: value for " << _text << " is an empty string" << std::endl;

    for (std::vector<ModifierAndArgument>::iterator iter = _modifiers.begin(); iter != _modifiers.end(); ++iter) {
      Modifier *mod = mtemplate::GetModifier(iter->_name);
      if (mod)
        result = mod->modify(result, iter->_arg);
    }

    output->out(result);
    return true;
  }
  //-----------------------------------------------------------------------------------
  void NodeVariable::dump(int indent) {
    base::utf8string hidden = isHidden() ? "[hidden]" : "";
    base::utf8string indent_str(indent * 2, ' ');
    std::cout << indent_str << "[Variable]" << hidden << " = " << _text << std::endl;
  }
  //-----------------------------------------------------------------------------------
  NodeVariable *NodeVariable::parse(const base::utf8string &template_string, PARSE_TYPE type) {
    base::utf8string::size_type end = template_string.find(TEMPLATE_TAG_END);

    if (end == base::utf8string::npos)
      throw std::logic_error(base::utf8string("mtemplate: Could not find the end of the tag '}}'.\n") +
                             template_string);

    base::utf8string::size_type begin = TEMPLATE_TAG_END.length();
    base::utf8string variableName = template_string.substr(begin, end - begin);

    std::vector<base::utf8string> parts = variableName.split(":");

    variableName = parts[0];

    std::vector<ModifierAndArgument> modifiers;

    //    Parse modifiers from "parts"
    for (std::size_t index = 1; index < parts.size(); ++index) {
      base::utf8string part = parts[index];
      std::size_t equal = part.find('=');
      base::utf8string arg = "";

      if (equal != base::utf8string::npos) {
        arg = part.substr(equal);
        part = part.substr(0, equal);
      }

      modifiers.push_back({part, arg});
    }

    return new NodeVariable(variableName, end + TEMPLATE_TAG_END.length(), modifiers);
  }

  //-----------------------------------------------------------------------------------
  //  NodeSection stuff
  //-----------------------------------------------------------------------------------
  NodeSection::NodeSection(const base::utf8string &text, std::size_t length, TemplateDocument &contents)
    : NodeInterface(TemplateObject_Section, text, length), _contents(contents), _is_separator(false) {
  }
  //-----------------------------------------------------------------------------------
  bool NodeSection::expand(TemplateOutput *output, DictionaryInterface *dict) {
    if (isHidden())
      return true;

    for (NodeStorageType node : _contents) {
      if (node->type() == TemplateObject_Section) {
        //    Check for separator sections special marker
        NodeSection *sec = dynamic_cast<NodeSection *>(node.get());
        if (sec->is_separator() && dict->isLast() == false) {
          node->expand(output, dict);
          continue;
        }

        DictionaryInterface::section_dictionary_storage &section_dicts = dict->getSectionDictionaries(node->_text);

        for (DictionaryInterface *item : section_dicts)
          node->expand(output, item);
      } else
        node->expand(output, dict);
    }
    return true;
  }
  //-----------------------------------------------------------------------------------
  void NodeSection::dump(int indent) {
    base::utf8string hidden = isHidden() ? "[hidden]" : "";
    base::utf8string indent_str(indent * 2, ' ');
    std::cout << indent_str << "[Section]" << hidden << " = " << _text << std::endl << indent_str << "{" << std::endl;

    for (NodeStorageType node : _contents)
      node->dump(indent + 1);

    std::cout << indent_str << "}" << std::endl;
  }
  //-----------------------------------------------------------------------------------
  NodeSection *NodeSection::parse(const base::utf8string &template_string, PARSE_TYPE type) {
    base::utf8string::size_type end = template_string.find(TEMPLATE_TAG_END);

    if (end == base::utf8string::npos)
      throw std::logic_error(base::utf8string("mtemplate: Could not find the end of the tag '}}'.\n") +
                             template_string);

    base::utf8string::size_type begin = TEMPLATE_TAG_END.length() + TEMPLATE_SECTION_BEGINNING.length();
    base::utf8string sectionName = template_string.substr(begin, end - begin);

    begin = end + TEMPLATE_TAG_END.length();

    end = template_string.find(TEMPLATE_TAG_BEGINNING + TEMPLATE_SECTION_END + sectionName + TEMPLATE_TAG_END, begin);

    if (end == base::utf8string::npos)
      throw std::logic_error(base::utf8string("mtemplate: Could not find the end of the tag '}}'.\n") +
                             template_string);

    base::utf8string inner_string = template_string.substr(begin, end - begin);

    TemplateDocument contents = parseTemplate(inner_string, type);

    //  Check for separators...only the last one will be taken into account
    base::utf8string separator_text = sectionName + "_separator";

    for (NodeStorageType node : contents) {
      NodeSection *node_section = dynamic_cast<NodeSection *>(node.get());
      if (node_section == NULL)
        continue;

      if (node_section->text() == separator_text) {
        node_section->set_is_separator();
        break;
      }
    }

    end += (TEMPLATE_TAG_BEGINNING + TEMPLATE_SECTION_END + sectionName + TEMPLATE_TAG_END).length();

    return new NodeSection(sectionName, end, contents);
  }

  //-----------------------------------------------------------------------------------
  //  Template stuff
  //-----------------------------------------------------------------------------------
  std::size_t GetTextLength(const base::utf8string &temp_template, bool check_new_lines) {
    std::size_t begin = 0;

    while (begin < temp_template.size()) {
      std::size_t pos = temp_template.find(TEMPLATE_TAG_BEGINNING, begin);

      if (check_new_lines) {
        std::size_t new_line_pos = temp_template.find("\n", begin);
        if (new_line_pos < pos)
          pos = new_line_pos;
      }

      //  Check if the whole string is valid text
      if (pos == base::utf8string::npos)
        return temp_template.length();

      //  Check if a line feed was found
      if (temp_template.at(pos) == '\n')
        return pos;

      //  Check if it's a valid node. It's only a valid node when it starts with {{ and it's
      //  followed by one of the characters in TEMPLATE_TAG_CHARACTERS
      if (TEMPLATE_TAG_CHARACTERS.find(temp_template[pos + 2]) != base::utf8string::npos)
        return pos;

      begin = pos;
    }

    return base::utf8string::npos;
  }

  TemplateDocument parseTemplate(const base::utf8string &template_string, PARSE_TYPE type) {
    TemplateDocument doc;
    base::utf8string temp_template = template_string;

    while (temp_template.length() > 0) {
      if (temp_template[0] == '\n') {
        NodeNewLine *item = NodeNewLine::parse(temp_template, type);
        temp_template = temp_template.substr(item->_length);
        doc.push_back(NodeStorageType(item));

        if (type == DO_NOT_STRIP)
          continue;

        if (doc.size() == 1) {
          item->hide();
          continue;
        }

        mtemplate::TemplateDocument::iterator last = --(--doc.end());
        NodeStorageType last_node = *last;

        if (last_node->type() == TemplateObject_Text && IsBlankString(last_node->text())) {
          last_node->hide();
          item->hide();
        } else if (last_node->type() == TemplateObject_NewLine) {
          item->hide();
        }

        if (doc.size() == 2)
          continue;

        mtemplate::TemplateDocument::iterator second_last = --last;
        NodeStorageType second_last_node = *second_last;

        if (last_node->type() == TemplateObject_Section) {
          if (second_last_node->type() == TemplateObject_NewLine)
            item->hide();
        } else if (last_node->type() == TemplateObject_Text && IsBlankString(last_node->text())) {
          if (second_last_node->type() == TemplateObject_NewLine) {
            item->hide();
            last_node->hide();
          }
        }
      } else if (temp_template.starts_with("{{{{")) {
        throw std::logic_error("mtemplate: File contains invalid character sequence '{{{{'");
      } else if (temp_template.starts_with("{{{")) { // Special case of {{{
        NodeText *item = NodeText::parse("{", type);
        temp_template = temp_template.substr(item->_length);
        doc.push_back(NodeStorageType(item));
      } else if (temp_template.starts_with("{{")) { //  A node was found {{SOME_NOME}}
        base::utf8string::utf8char first_char = temp_template[TEMPLATE_TAG_BEGINNING.length()];

        switch (first_char) {
          case '#': {
            NodeSection *item = NodeSection::parse(temp_template, type);
            doc.push_back(NodeStorageType(item));
            temp_template = temp_template.substr(item->_length);
            break;
          }
          case '>': //  includes
            throw std::logic_error("mtemplate: Includes not implemented");
          case '%': //  pragma
            throw std::logic_error("mtemplate: Pragma not implemented");
          case '!': //  comment
            throw std::logic_error("mtemplate: Comment not implemented");
          default: {
            NodeVariable *item = NodeVariable::parse(temp_template, type);
            doc.push_back(NodeStorageType(item));
            temp_template = temp_template.substr(item->_length);
          }
        }

      } else {
        NodeText *item = NodeText::parse(temp_template, type);
        temp_template = temp_template.substr(item->_length);
        doc.push_back(NodeStorageType(item));
      }
    }

    return doc;
  }

  bool IsBlankString(const base::utf8string &text) {
    return text.find_first_not_of(" \t\n\v\f\r") == base::utf8string::npos;
  }

  base::utf8string FormatErrorLog(const base::utf8string &template_string, std::size_t pos,
                                  const base::utf8string &error) {
    std::size_t eol = template_string.find('\n');
    if (eol == base::utf8string::npos)
      eol = template_string.length();
    base::utf8string result = template_string.substr(0, eol);

    result += '\n';
    result += base::utf8string(pos, ' ') + "^\n";
    result += error;
    return result;
  }
}
