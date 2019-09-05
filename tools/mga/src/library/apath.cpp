/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "accessible.h"
#include "role.h"
#include "utilities.h"

#include "apath.h"

using namespace mga;
using namespace aal;

static std::map<std::string, Role> roleMap = {
  { "unknown", Role::Unknown },
  { "any", Role::Any },
  { "*", Role::Any },
  { "application", Role::Application },
  { "window", Role::Window },
  { "button", Role::Button },
  { "radiobutton", Role::RadioButton },
  { "radiogroup", Role::RadioGroup },
  { "checkbox", Role::CheckBox },
  { "combobox", Role::ComboBox },
  { "expander", Role::Expander },
  { "grid", Role::Grid },
  { "textbox", Role::TextBox },
  { "treeview", Role::TreeView},
  { "label", Role::Label },
  { "pane", Role::Pane },
  { "menu", Role::Menu },
  { "menubar", Role::MenuBar },
  { "menuitem", Role::MenuItem },
  { "separator", Role::Separator },
  { "splitcontainer", Role::SplitContainer },
  { "splitter", Role::Splitter },
  { "groupbox", Role::GroupBox },
  { "image", Role::Image },
  { "tabview", Role::TabView },
  { "tabpage", Role::TabPage },
  { "datepicker", Role::DatePicker },
  { "row", Role::Row },
  { "column", Role::Column },
  { "cell", Role::Cell },
  { "scrollbox", Role::ScrollBox },
  { "slider", Role::Slider },
  { "stepper", Role::Stepper },
  { "list", Role::List },
  { "iconview", Role::IconView },
  { "progressindicator", Role::ProgressIndicator },
  { "busyindicator", Role::BusyIndicator },
  { "scrollbar", Role::ScrollBar },
  { "scrollthumb", Role::ScrollThumb },
  { "hyperlink", Role::HyperLink },
};

static std::map<std::string, size_t> axisMap = {
  { "child", 0 },
  { "parent", 1 },
  { "..", 1 },
  { "self", 2 },
  { ".", 2 },
  { "ancestor", 3 },
  { "ancestor-or-self", 4 },
  { "descendant", 5 },
  { "descendant-or-self", 6 },
  { "following", 7 },
  { "following-sibling", 8 },
  { "preceding", 9 },
  { "preceding-sibling", 10 }
};

//----------------------------------------------------------------------------------------------------------------------

APath::APath(Accessible *root) : _root(root) {
}

//----------------------------------------------------------------------------------------------------------------------

static std::vector<std::string> tokenizePath(std::string const& input);

/**
 * Retrieves UI elements addressed by the given path. The general path syntax is:
 *   localization-step/localization-step/...
 *  where a localization step is defined as:
 *   axis::node-test[predicate 1][predicate 2]...
 *
 * Supported axes are:
 *   child - all direct children of a node
 *   parent - the direct parent of a node
 *   self - the node itself
 *   ancestor - all parents of a node (excluding the root node)
 *   ancestor-or-self - like ancestor, but including the root node.
 *   descendant - all child, grandchild etc. nodes
 *   descendant-or-self - like descendant, but including the node itself
 *   following - all following siblings, including their children
 *   following-sibling - like following, but without children
 *   preceding - all preceding siblings, including their children
 *   preceding-sibling - like preceding, but without children
 *
 * There are shortcuts to simplify path axes:
 *   nothing - if no axis is given child is assumed
 *   . - like self
 *   .. - like parent
 *   / - the root node
 *   // - all elements, including the root node (corresponds to /descendant-or-self/).
 *   .// - like descendant (corresponds to ./descendant/)
 *
 * Supported node tests are:
 *   ::role - elements with the given role
 *   ::* - elements with any role (same as if no node test was given).
 *
 * At the moment we support child indices and property comparisons in predicates.
 * You can create a union of multiple path expressions by joining them with a pipe symbol (|).
 *
 * @param anchor The element to start search with. Can be nullptr if path is absolute.
 * @param path The search path to executed.
 * @param includeEmptyNames True if elements with empty names should be included in the result.
 * @param includeInternal True if internal elements should be included in the result.
 */
AccessibleList APath::execute(Accessible *anchor, std::string const& path, bool includeEmptyNames, bool includeEmptyIDs,
  bool includeInternal) {
  AccessibleList result;

  if (path.empty())
    return result;

  auto tokens = tokenizePath(path);
  auto begin = tokens.begin();
  while (true) {
    auto subResult = executePath(anchor, begin, tokens.end(), includeEmptyNames, includeEmptyIDs, includeInternal);
    for (auto &entry : subResult)
      result.push_back(std::move(entry));
    
    if (begin == tokens.end())
      break;
    ++begin;
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

static AccessibleList getChildren(Accessible *element, bool recursive) {
  if (recursive) {
    AccessibleList accessibles;
    element->children(accessibles, true);
    return accessibles;
  }
  
  return element->children();
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList APath::getParents(Accessible *element) const {
  AccessibleList result;
  while (!element->isRoot()) {
    result.push_back(element->getParent());
    element = result.back().get();
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList APath::getSiblings(Accessible *element, bool leading, bool trailing) const {
  AccessibleList result;
  if (element->isRoot())
    return result;

  bool foundElement = false;
  for (auto &child : getChildren(element->getParent().get(), false))
    if (child->equals(element))
      foundElement = true;
    else {
      if (!foundElement && leading)
        result.push_back(std::move(child));
      if (foundElement && trailing)
        result.push_back(std::move(child));
    }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

enum class PredicateValueType {
  Invalid,
  Boolean,
  String,
  Number,
  Operator,
  Self,
  ID,
  Name,
  Description,
  Text,
  Valid,
  Enabled,
  Focused,
  ChildCount,
  Title,
};

struct PredicateValue {
  PredicateValueType type;

  double numberValue;
  bool boolValue;
  std::string stringValue;

  PredicateValue(PredicateValueType aType, double value) : type(aType), numberValue(value), boolValue(false) {};
  PredicateValue(PredicateValueType aType, bool value) : type(aType), numberValue(0.0), boolValue(value) {};
  PredicateValue(PredicateValueType aType, std::string const& value)
      : type(aType), numberValue(0.0), boolValue(false), stringValue(value) {
  }
  ;
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * A tokenizer for expressions used in a predicate.
 */
static std::vector<PredicateValue> tokenizeExpression(std::string const& input) {
  std::vector<PredicateValue> result;

  const char *run = input.c_str();
  while (*run != '\0') {
    const char *head = run;
    switch (*run) {
      case ' ':
      case '\t':
        ++run;
        break;

      case '-': case '+':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        char *end;
        double value = std::strtod(run, &end);
        result.emplace_back(PredicateValueType::Number, value);
        run = end;
        break;
      }

      case '.':
        result.emplace_back(PredicateValueType::Self, std::string("."));
        ++run;
        break;

      case '"': case '\'': {
        char quote = *run;
        while (*++run != quote && *run != '\0')
          ;
        if (*run == quote) {
          ++head;
          result.emplace_back(PredicateValueType::String, std::string(head, static_cast<size_t>(run - head)));
        } else
          throw std::runtime_error("Unclosed quote: " + input);

        if (*run != '\0')
          ++run;
        break;
      }

      case '<': case '>': {
        if (*++run == '=')
          ++run;
        result.emplace_back(PredicateValueType::Operator, std::string(head, static_cast<size_t>(run - head)));
        break;
      }

      case '=':
        result.emplace_back(PredicateValueType::Operator, std::string(head, static_cast<size_t>(++run - head)));
        break;

      case '!': {
        ++run;
        if (*run == '=')
          result.emplace_back(PredicateValueType::Operator, std::string(head, static_cast<size_t>(++run - head)));
        else
          throw std::runtime_error("Invalid operator: " + std::string(head, static_cast<size_t>(run - head)));

        break;
      }

      case 'f':
        if (strncmp(run, "false", 5) == 0) {
          run += 5;
          result.emplace_back(PredicateValueType::Boolean, false);
        }
        break;
      case 't':
        if (strncmp(run, "true", 4) == 0) {
          run += 4;
          result.emplace_back(PredicateValueType::Boolean, true);
        }
        break;
      case 's': {
        if (strncmp(run, "self", 4) == 0) {
          run += 4;
          result.emplace_back(PredicateValueType::Self, std::string(""));
        } else
          result.emplace_back(PredicateValueType::Invalid, std::string(head, static_cast<size_t>(++run - head)));
        break;
      }

      case '@': {
        static std::map<std::string, PredicateValueType> propertyMap = {
          { "id", PredicateValueType::ID },
          { "name", PredicateValueType::Name },
          { "description", PredicateValueType::Description },
          { "text", PredicateValueType::Text },
          { "valid", PredicateValueType::Valid },
          { "enabled", PredicateValueType::Enabled },
          { "childcount", PredicateValueType::ChildCount },
          { "focused", PredicateValueType::Focused },
          { "title", PredicateValueType::Title },
        };

        head = run;
        while (std::isalpha(*++run))
          ;

        PredicateValueType type = PredicateValueType::Invalid;
        std::string property(head + 1, static_cast<size_t>(run - head - 1));
        auto iterator = propertyMap.find(property);
        if (iterator != propertyMap.end())
          type = iterator->second;

        if (type != PredicateValueType::Invalid) {
          result.emplace_back(type, std::string(""));
        } else {
          throw std::runtime_error(std::string("Invalid property reference: ") + head);
        }

        break;
      }

      default:
        throw std::runtime_error(std::string("Invalid character sequence in expression: ") + head);
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * A tokenizer for the main elements of a path.
 */
static std::vector<std::string> tokenizePath(std::string const& input) {
  std::vector<std::string> result;

  const char *run = input.c_str();
  while (*run != '\0') {
    const char *head = run;
    switch (*run) {
      case ' ':
      case '\t':
        ++run;
        break;

      case '.':
      case '/': {
        size_t count = 1;
        if (*(run + 1) == '/') {
          ++count;
          if (*(run + 2) == '/')
            ++count;
        }
        result.emplace_back(run, count);
        run += count;
        break;
      }

      case ':': { // Keep role separator as a whole.
        size_t count = 1;
        if (*(run + 1) == ':')
          ++count;
        result.emplace_back(run, count);
        run += count;
        break;
      }

      case '"': case '\'': {
        char quote = *run;
        while (*++run != quote && *run != '\0')
          ;
        if (*run == quote) {
          result.emplace_back(head, static_cast<size_t>(run - head) + 1);
        } else
          throw std::runtime_error("Unclosed quote: " + input);

        if (*run != '\0')
          ++run;
        break;
      }

      case '[': { // Take over the predicate as a whole. There's another tokenizer for that.
        while (*++run != ']' && *run != '\0')
          ;
        if (*run == ']') {
          result.emplace_back(head, static_cast<size_t>(run - head) + 1);
        } else
          throw std::runtime_error("Unbalanced brackets: " + input);

        if (*run != '\0')
          ++run;
        break;
      }

      default:
        if (isalpha(*run)) {
          // For axes and roles, which all use ASCII.
          while (isalpha(*run) || *run == '-')
            ++run;
          result.emplace_back(head, static_cast<size_t>(run - head));
          break;
        }
        result.emplace_back(run++, 1);
        break;
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

static bool compareText(std::string const& text, std::string const& op, std::string const& value) {
  if (op.empty())
    return !text.empty();

  switch (op[0]) {
    case '<':
      if (op.size() == 2) // must be <=
        return text <= value;
      return text < value;

    case '>':
      if (op.size() == 2) // must be <=
        return text >= value;
      return text > value;

    case '!':
      return text != value;

    default:
      return text == value;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static bool compareNumber(long number, std::string const& op, long value) {
  if (op.empty())
    return number != 0;

  switch (op[0]) {
    case '<':
      if (op.size() == 2)
        return number <= value;
      return number < value;

    case '>':
      if (op.size() == 2)
        return number >= value;
      return number > value;

    case '!':
      return number != value;

    default:
      return number == value;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static bool compareBool(bool lhs, std::string const& op, bool rhs) {
  if (op.empty())
    return lhs;

  switch (op[0]) {
    case '<':
      if (op.size() == 2)
        return lhs <= rhs;
      return lhs < rhs;

    case '>':
      if (op.size() == 2)
        return lhs >= rhs;
      return lhs > rhs;

    case '!':
      return lhs != rhs;

    default:
      return lhs == rhs;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static bool getBoolValue(PredicateValue const& value) {
  switch (value.type) {
    case PredicateValueType::Boolean:
      return value.boolValue;
    case PredicateValueType::Number:
      return value.numberValue != 0;
    default:
      throw std::runtime_error("Boolean value expected");
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::string getStringValue(PredicateValue const& value) {
  if (value.type == PredicateValueType::String
      || value.type == PredicateValueType::ID
      || value.type == PredicateValueType::Name)
    return value.stringValue;

  throw std::runtime_error("String value expected");
}

//----------------------------------------------------------------------------------------------------------------------

static double getNumberValue(PredicateValue const& value) {
  switch (value.type) {
    case PredicateValueType::Boolean:
      return value.boolValue;
    case PredicateValueType::Number:
      return value.numberValue;
    default:
      throw std::runtime_error("Number value expected");
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList APath::executePath(Accessible *anchor, StringListIterator &begin, StringListIterator const& end,
  bool includeEmptyNames, bool includeEmptyIDs, bool includeInternal) {
  AccessibleList result;

  if (*begin == "/" && (end - begin == 1)) {
    ++begin;
    result.push_back(_root->clone());
    return result;
  }

  Accessible *start = anchor;
  if (start == nullptr || (*begin)[0] == '/') // Absolute path or no anchor given?
    start = _root;

  // Loop over subranges, delimited by a slash.
  // Bail out when we reach the end of the path.
  while (begin != end && *begin != "|") {

    if (*begin == "/" || *begin == "./")
      ++begin;

    std::string axis = Utilities::toLower(*begin);
    if (axis == "::") // No axis, use default.
      axis = "child";
    else {
      if (axis == ".//") {
        axis = "descendant";
      } else if (axis == "//") {
        axis = "descendant-or-self";
      }
      ++begin;
    }

    Role role = Role::Any;
    if (begin != end && *begin == "::") {
      ++begin;
      std::string roleName = Utilities::toLower(*begin++);
      if (roleMap.find(roleName) == roleMap.end())
        throw std::runtime_error("Invalid role specified: " + roleName);
      role = roleMap[roleName];
    }

    if (axisMap.find(axis) == axisMap.end())
      throw std::runtime_error("Invalid path axis specified: " + axis);

    size_t axisIndex = axisMap[axis];
    switch (axisIndex) {
      case 0: { // child
        AccessibleList children;
        if (result.empty())
          children = getChildren(start, false);
        else {
          for (auto &entry : result) {
            auto temp = getChildren(entry.get(), false);
            Utilities::appendVector(children, temp);
          }
        }
        result = std::move(children);

        break;
      }

      case 1: // parent
        if (result.empty()) {
          if (start->isRoot())
            throw std::runtime_error("There is no parent for the root node");
          else
            result.push_back(start->getParent());
        } else {
          for (auto &entry : result) {
            if (entry->isRoot())
              throw std::runtime_error("There is no parent for the root node");
            else
              entry = entry->getParent();
          }
        }
        break;

      case 2: // self
        if (result.empty())
          result.push_back(start->clone());
        break;

      case 3: // ancestor
      case 4: { // ancestor-or-self
        if (result.empty()) {
          if (start->isRoot())
            throw std::runtime_error("There is no parent for the root node");
          result = getParents(start);

          if (axisIndex == 4)
            result.insert(result.begin(), start->clone());
          break;
        }

        AccessibleList parents;
        for (auto &element : result) {
          Accessible *raw = element.get();
          if (axisIndex == 4)
            parents.push_back(std::move(element));
          auto temp = getParents(raw);
          Utilities::appendVector(parents, temp);
        }
        result = std::move(parents);
        break;
      }

      case 5: // descendant
      case 6: { // descendant-or-self
        if (result.empty()) {
          result = getChildren(start, true);
          if (axisIndex == 6)
            result.insert(result.begin(), start->clone());
          break;
        }

        AccessibleList children;
        for (auto &element : result) {
          Accessible *raw = element.get();
          if (axisIndex == 6)
            children.push_back(std::move(element));
          auto temp = getChildren(raw, true);
          Utilities::appendVector(children, temp);
        }
        result = std::move(children);

        break;
      }

      case 7: // following
      case 8: // following-sibling
      case 9: // preceding
      case 10: { // preceding-sibling
        bool preceding = axisIndex == 9 || axisIndex == 10;
        bool siblingsOnly = axisIndex == 8 || axisIndex == 10;

        AccessibleList collected;
        std::vector<Accessible *> elements;
        if (result.empty())
          elements.push_back(start);
        else {
          for (auto &element : result)
            elements.push_back(element.get());
        }

        for (Accessible *element : elements) {
          auto siblings = getSiblings(element, preceding, !preceding);
          if (siblingsOnly) {
            Utilities::appendVector(collected, siblings);
          } else {
            for (auto &sibling : siblings) {
              result.push_back(std::move(sibling));
              auto children = getChildren(sibling.get(), true);
              Utilities::appendVector(collected, children);
            }
          }
        }

        result = std::move(collected);
        break;
      }
    }

    // Remove all elements which don't match the specified role.
    if (role != Role::Any)
      result.erase(std::remove_if(result.begin(), result.end(), [role] (AccessibleRef const& element) {
        return element->getRole() != role;
      }), result.end());

    // Remove elements that have no name and/or id assigned (if specified). Same for elements that are
    // considered to be internal (like those added by the OS).
    if (!includeEmptyNames) {
      result.erase(std::remove_if(result.begin(), result.end(), [] (AccessibleRef const& element) {
        return element->getName().empty();
      }), result.end());
    }

    if (!includeEmptyIDs) {
      result.erase(std::remove_if(result.begin(), result.end(), [] (AccessibleRef const& element) {
        return element->getID().empty();
      }), result.end());
    }

    // TODO: decide if we even can support that internal flag or remove it.
    std::ignore = includeInternal;
    /*if (!includeInternal) {
      result.erase(std::remove_if(result.begin(), result.end(), [role] (AccessibleRef const& element) {
        return element->isInternal();
      }), result.end());
    }*/

    // Last step: filter by predicate. In XPath predicates can also contain XPath and some math expressions.
    // We don't go that far atm. and support only these expressions:
    //   - self: (or .) to refer to an element's text (same as @text).
    //   - properties: @id, @name, @text, @valid, @enabled, @focused, @childCount, @title (not case sensitive).
    //   - A number as index into the child list.
    // Except for the index the expression must be formulated as a relation (e.g. [@enabled=true]) to be valid.
    while (begin != end && (*begin)[0] == '[') {
      auto predicate = begin->substr(1, begin->size() - 2);
      auto expression = tokenizeExpression(predicate);
      ++begin;

      if (expression.empty())
        return result;

      // Is it an index?
      if (expression.size() == 1 && expression[0].type == PredicateValueType::Number) {
        // Remove everything but the value indexed by the given number.
        // Keep in mind the index is one-based.
        long index = static_cast<long>(expression[0].numberValue);
        if (index < 1 || index > static_cast<long>(result.size()))
          throw std::runtime_error("Index out of range: " + std::to_string(index));

        --index;
        AccessibleRef temp = std::move(result[static_cast<size_t>(index)]);
        result.clear();
        result.push_back(std::move(temp));

        continue;
      }

      // Something more complex. Validate and act according to the type.
      if (expression.size() !=  3)
        throw std::runtime_error("Invalid predicate specified: " + predicate);

      if (expression[1].type != PredicateValueType::Operator)
        throw std::runtime_error("Invalid relation specified in predicate: " + predicate);

      std::string op = expression[1].stringValue;

      switch (expression[0].type) {
        case PredicateValueType::Self:
        case PredicateValueType::Text: {
          std::string value = getStringValue(expression[2]);
          result.erase(std::remove_if(result.begin(), result.end(), [&](AccessibleRef const& element) {
            try {
              //std::cout << element->dump() << std::endl;
              return !compareText(element->getText(), op, value);
            } catch (...) {
              return true;
            }
          }), result.end());

          break;
        }

        case PredicateValueType::ID: {
          std::string value = getStringValue(expression[2]);
          result.erase(std::remove_if(result.begin(), result.end(), [&](AccessibleRef const& element) {
            return !compareText(element->getID(), op, value);
          }), result.end());

          break;
        }

        case PredicateValueType::Name: {
          std::string value = getStringValue(expression[2]);
          result.erase(std::remove_if(result.begin(), result.end(), [&](AccessibleRef const& element) {
            return !compareText(element->getName(), op, value);
          }), result.end());

          break;
        }

        case PredicateValueType::Description: {
          std::string value = getStringValue(expression[2]);
          result.erase(std::remove_if(result.begin(), result.end(), [&](AccessibleRef const& element) {
            return !compareText(element->getDescription(), op, value);
          }), result.end());

          break;
        }

        case PredicateValueType::Valid: {
          bool value = getBoolValue(expression[2]);
          result.erase(std::remove_if(result.begin(), result.end(), [op, value](AccessibleRef const& element) {
            return !compareBool(element->isValid(), op, value);
          }), result.end());

          break;
        }

        case PredicateValueType::Enabled: {
          bool value = getBoolValue(expression[2]);
          result.erase(std::remove_if(result.begin(), result.end(), [&](AccessibleRef const& element) {
            try {
              return !compareBool(element->isEnabled(), op, value);
            } catch (...) {
              return true;
            }
          }), result.end());

          break;
        }

        case PredicateValueType::Focused: {
          bool value = getBoolValue(expression[2]);
          result.erase(std::remove_if(result.begin(), result.end(), [&](AccessibleRef const& element) {
            bool canFocus = element->canFocus();
            try {
              return !compareBool(canFocus && element->isFocused(), op, value);
            } catch (...) {
              return true;
            }
          }), result.end());

          break;
        }

        case PredicateValueType::ChildCount: {
          long count = static_cast<long>(getNumberValue(expression[2]));
          if (count < 0)
            throw std::runtime_error("The value must be greater than zero (" + std::to_string(count) + ")");

          result.erase(std::remove_if(result.begin(), result.end(), [&](AccessibleRef const& element) {
            return !compareNumber(static_cast<long>(element->children().size()), op, count);
          }), result.end());

          break;
        }

        case PredicateValueType::Title: {
          std::string value = getStringValue(expression[2]);
          result.erase(std::remove_if(result.begin(), result.end(), [&](AccessibleRef const& element) {
            try {
              return !compareText(element->getTitle(), op, value);
            } catch (...) {
              return true;
            }
          }), result.end());

          break;
        }

        default: // Something invalid here.
          return result;
      }
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------
