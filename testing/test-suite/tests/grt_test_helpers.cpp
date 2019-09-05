/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "grt_test_helpers.h"

#include "grt/tree_model.h"
#include "base/file_functions.h"
#include "base/string_utilities.h"

#include "casmine.h"
#include "common.h"

namespace casmine {

$ModuleEnvironment() {};

//----------------------------------------------------------------------------------------------------------------------

std::string grt_type_to_str(grt::Type grt_type) {
  switch (grt_type) {
    case grt::AnyType:
      return "AnyType";
    case grt::IntegerType:
      return "IntegerType";
    case grt::DoubleType:
      return "DoubleType";
    case grt::StringType:
      return "StringType";
    case grt::ObjectType:
      return "ObjectType";
    case grt::ListType:
      return "ListType";
    case grt::DictType:
      return "DictType";
    default:
      return "UNKNOWN";
  }
};

//----------------------------------------------------------------------------------------------------------------------

static void dump_tree_node(std::ofstream &out, bec::TreeModel *tree, const bec::NodeId &node,
                           const std::vector<ssize_t> &columns, bool dump_type) {
  out << node.toString() << ":" << tree->count_children(node) << ":";

  for (std::vector<ssize_t>::const_iterator column = columns.begin(); column != columns.end(); ++column) {
    try {
      std::string value;
      bool ret = tree->get_field(node, *column, value);
      if (!ret) {
        ssize_t i;
        ret = tree->get_field(node, *column, i);
        if (!ret)
          out << "::0:0";
        else
          out << "::1:" << i;
      } else
        out << "::1:" << value;
    } catch (std::exception &exc) {
      out << "::(exception:" << exc.what() << ")";
    }

    if (dump_type) {
      try {
        grt::Type type = tree->get_field_type(node, *column);
        out << "(" << grt_type_to_str(type) << ")";
      } catch (std::exception &exc) {
        out << "(exception:" << exc.what() << ")";
      }
    }
  }

  out << std::endl;

  for (size_t c = tree->count_children(node), i = 0; i < c; i++) {
    bec::NodeId child(tree->get_child(node, i));
    g_assert(node.toString() != child.toString());

    dump_tree_node(out, tree, child, columns, dump_type);
  }
}

//----------------------------------------------------------------------------------------------------------------------

class FollowedObjectsStackHelper {
public:
  FollowedObjectsStackHelper(bool clear_stack_on_destroy) : _clear_stack_on_destroy(clear_stack_on_destroy) {
  }

  ~FollowedObjectsStackHelper() {
    if (_clear_stack_on_destroy)
      _followed_obj_stack.clear();
  }
  static std::list<std::string> *followed_obj_stack() {
    return &_followed_obj_stack;
  }

private:
  static std::list<std::string> _followed_obj_stack;
  bool _clear_stack_on_destroy;
};

std::list<std::string> FollowedObjectsStackHelper::_followed_obj_stack;

//--------------------------------------------------------------------------------------------------

static std::string pathToString(const std::string &msg, const std::list<std::string> &l) {
  std::ostringstream oss;

  oss << msg << " ";

  for (std::list<std::string>::const_iterator iter = l.begin(); iter != l.end(); ++iter)
    oss << *iter;

  return oss.str();
}

//--------------------------------------------------------------------------------------------------

void compareValues(std::string const& major_msg, grt::ValueRef actualValue, grt::ValueRef expectedValue,
  bool compare_obj_id, std::list<std::string> *followed_obj_stack, std::list<std::string> &followed_path) {
  std::string msg = pathToString(major_msg, followed_path);
  if (actualValue.is_valid() && !expectedValue.is_valid())
    $fail(msg + ", have an actual value where none was expected");

  if (!actualValue.is_valid() && expectedValue.is_valid())
    $fail(msg + ", expected a value but none was found");

  FollowedObjectsStackHelper FollowedObjectsStackHelper(!followed_obj_stack);
  if (!followed_obj_stack)
    followed_obj_stack = FollowedObjectsStackHelper::followed_obj_stack();

  $expect(grt_type_to_str(actualValue.type())).toBe(grt_type_to_str(expectedValue.type()), msg + ", different types");

  switch (actualValue.type()) {
    case grt::IntegerType: {
      $expect(*grt::IntegerRef::cast_from(actualValue)).toBe(*grt::IntegerRef::cast_from(expectedValue), msg + ", different values");
      break;
    }

    case grt::DoubleType: {
      $expect(*grt::DoubleRef::cast_from(actualValue)).toBe(*grt::DoubleRef::cast_from(expectedValue), msg + ", different values");
      break;
    }

    case grt::StringType: {
      $expect(*grt::StringRef::cast_from(actualValue)).toBe(*grt::StringRef::cast_from(expectedValue), msg + ", different values");
      break;
    }

    case grt::ObjectType: {
      grt::ObjectRef actualObject(grt::ObjectRef::cast_from(actualValue));
      grt::ObjectRef expectedObject(grt::ObjectRef::cast_from(expectedValue));

      // Check if we had this object already (can happen due to recursive grt tree structure).
      if (followed_obj_stack->end() !=
          std::find(followed_obj_stack->begin(), followed_obj_stack->end(), expectedObject.id()))
        break;
      followed_obj_stack->push_back(expectedObject.id());

      $expect(actualObject.class_name()).toBe(expectedObject.class_name(), msg + ", class names differ");

      if (compare_obj_id)
        $expect(actualObject->id()).toBe(expectedObject->id(), msg + ", object ids differ");

      grt::MetaClass *actualMetaData = actualObject.get_metaclass();
      grt::MetaClass *expectedMetaData = expectedObject.get_metaclass();

      $expect(actualMetaData->name()).toBe(expectedMetaData->name(), msg + ", meta class names differ");

      std::string mem_attr;
      grt::Type mem_type;
      bool skip_member_cmp;

      do {
        for (grt::MetaClass::MemberList::const_iterator mem = actualMetaData->get_members_partial().begin();
             mem != actualMetaData->get_members_partial().end(); ++mem) {
          const grt::MetaClass::Member &res_mem = mem->second;

          if (res_mem.overrides)
            continue;

          skip_member_cmp = false;

          mem_type = res_mem.type.base.type; // ValueRef.TypeSpec.SimpleTypeSpec.Type
          if (mem_type == grt::ObjectType) {
            if (strncasecmp("owner", res_mem.name.c_str(), 6) == 0)
              skip_member_cmp = true;
            else {
              if (!res_mem.owned_object)
                skip_member_cmp = true;
            }
          }

          if (!skip_member_cmp) {
            mem_attr = actualMetaData->get_member_attribute(res_mem.name, "dontdiff");
            if (strncasecmp("1", mem_attr.c_str(), 2) == 0)
              skip_member_cmp = true;
          }

          if (!skip_member_cmp) {
            if (followed_path.size() == 0)
              followed_path.push_back(res_mem.name);
            else
              followed_path.push_back("." + res_mem.name);

            compareValues(major_msg, actualObject.get_member(res_mem.name), expectedObject.get_member(res_mem.name),
                              compare_obj_id, followed_obj_stack, followed_path);

            followed_path.pop_back();
          }
        }

        actualMetaData = actualMetaData->parent();
      } while (actualMetaData != nullptr);

      followed_obj_stack->pop_back();
      break;
    }

    case grt::ListType: {
      grt::BaseListRef actualList = grt::BaseListRef::cast_from(actualValue);
      grt::BaseListRef expectedList = grt::BaseListRef::cast_from(expectedValue);

      for (size_t i = expectedList.count(); i < actualList.count(); i++)
        std::cout << actualList[i].toString().c_str() << "\n\n";

      $expect(actualList.count()).toBe(expectedList.count(), msg + ", list sizes differ");

      for (unsigned n = 0; n < actualList.count(); n++) {
        followed_path.push_back(base::strfmt("[%i]", n));

        compareValues(major_msg, actualList.get(n), expectedList.get(n), compare_obj_id, followed_obj_stack,
                          followed_path);

        followed_path.pop_back();
      }

      break;
    }

    case grt::DictType: {
      grt::DictRef actualDict = grt::DictRef::cast_from(actualValue);
      grt::DictRef expectedDict = grt::DictRef::cast_from(expectedValue);

      $expect(actualDict.count()).toBe(expectedDict.count(), msg + ", dict sizes differ");

      for (auto actualIterator = actualDict.begin(), expectedIterator = expectedDict.begin();
           actualIterator != actualDict.end(); ++actualIterator, ++expectedIterator) {
        $expect(actualIterator->first).toBe(expectedIterator->first, msg + ", dict keys differ");

        followed_path.push_back("[" + actualIterator->first + "]");

        compareValues(major_msg, actualIterator->second, expectedIterator->second, compare_obj_id,
                      followed_obj_stack, followed_path);

        followed_path.pop_back();
      }

      break;
    }

    case grt::AnyType:
    default: {
      // ANY is considered as empty value.
      break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void deepCompareGrtValues(std::string const& major_msg, const grt::ValueRef &actual, const grt::ValueRef &expected,
  bool compare_obj_id, std::list<std::string> *followed_obj_stack) {
  std::list<std::string> path;
  compareValues(major_msg, actual, expected, compare_obj_id, followed_obj_stack, path);
}

//----------------------------------------------------------------------------------------------------------------------

void dumpTreeModel(const std::string &path, bec::TreeModel *tree, const std::vector<ssize_t> &columns, bool dump_type) {
  std::ofstream out(path.c_str(), std::ios::out | std::ios::trunc);

  if (out.is_open()) {
    bec::NodeId node = tree->get_root();

    dump_tree_node(out, tree, node, columns, dump_type);
  }
}

//----------------------------------------------------------------------------------------------------------------------

}
