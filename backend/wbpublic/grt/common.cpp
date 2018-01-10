/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "base/threading.h"
#include "base/log.h"

#include "common.h"

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"

#include "mforms/utilities.h"

DEFAULT_LOG_DOMAIN("bec");

#include <cstdlib>

using namespace base;

namespace bec {

  static bool validate_member(const grt::ClassMember *member, const GrtObjectRef &object, bool &failed) {
    GrtObjectRef child;
    switch (member->type.base.type) {
      case grt::DictType:
        break;
      case grt::ListType:
        if (member->owned_object) {
          grt::BaseListRef list(grt::BaseListRef::cast_from(object->get_member(member->name)));
          for (size_t i = 0; i < list.count(); i++) {
            if (GrtObjectRef::can_wrap(list.get(i))) {
              GrtObjectRef child(GrtObjectRef::cast_from(list.get(i)));
              if (!child.is_valid()) {
                failed = true;
                logError("Child item %li of list %s::%s is NULL\n", (long int)i,
                         GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str());
              } else {
                if (!child->owner().is_valid()) {
                  logError("owner of %s::%s[%li] %s is NULL (expected %s <%s>)\n",
                           GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), (long int)i,
                           child->id().c_str(), object->id().c_str(), object->class_name().c_str());
                  failed = true;
                } else if (child->owner() != object) {
                  logError("owner of %s::%s[%li] %s is wrong (expected %s <%s>, is %s <%s>)\n",
                           GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), (long int)i,
                           child->id().c_str(), object->id().c_str(), object->class_name().c_str(),
                           child->owner()->id().c_str(), child->owner()->class_name().c_str());
                  failed = true;
                } else
                  failed = validate_tree_structure(child) || failed;
              }
            } else
              logInfo("Unknown object in list %s\n", member->name.c_str());
          }
        } else {
          grt::BaseListRef list(grt::BaseListRef::cast_from(object->get_member(member->name)));
          for (size_t i = 0; i < list.count(); i++) {
            if (GrtObjectRef::can_wrap(list.get(i))) {
              GrtObjectRef child(GrtObjectRef::cast_from(list.get(i)));
              if (!child.is_valid()) {
                logError("Child item %li of list %s::%s is NULL\n", (long int)i,
                         GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str());
                failed = true;
              } else {
                if (!child->owner().is_valid()) {
                  logError("owner of %s::%s[%li] %s is NULL (expected %s <%s>)\n",
                           GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), (long int)i,
                           child->id().c_str(), object->id().c_str(), object->class_name().c_str());
                  failed = true;
                } else if (child->owner() == object) {
                  logError("owner of %s::%s[%li] is wrong (not supposed to be %s)\n",
                           GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), (long int)i,
                           child->owner()->id().c_str());
                  failed = true;
                }
              }
            }
          }
        }
        break;
      case grt::ObjectType:
        child = GrtObjectRef::cast_from(object->get_member(member->name));
        if (child.is_valid()) {
          if (child->owner() != object && member->owned_object) {
            logError("owner of %s::%s is wrong (expected %s, is %s)\n",
                     GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), object->id().c_str(),
                     child->owner()->id().c_str());
            failed = true;
          }
          if (member->owned_object)
            failed = validate_tree_structure(child) || failed;
        }
        break;
      default:
        break;
    }
    return true;
  }

  /** Validates a GRT object tree for correct owner values
   */
  bool validate_tree_structure(const grt::ObjectRef &object) {
    bool failed = false;
    grt::MetaClass *mc = object->get_metaclass();
    mc->foreach_member(std::bind(validate_member, std::placeholders::_1, GrtObjectRef::cast_from(object), failed));
    return !failed;
  }

  /**
   * Moves the given object within the list to the given position.
   * This position must be local to the enclosing group.
   */
  template <class T>
  void move_list_ref_item(grt::ListRef<T> items, const grt::ValueRef &object, ssize_t to) {
    grt::Type object_type = object.type();

    std::string group_name;
    std::string item_name;
    std::string search_name;
    size_t group_indicator_position = std::string::npos;

    grt::Ref<T> item, target_item;
    size_t ui_item_index = grt::BaseListRef::npos;
    size_t item_index = grt::BaseListRef::npos;
    size_t target_index = grt::BaseListRef::npos;

    // Obtains data from the elements lists like:
    // - Top level elements
    // - First and Last positions of the grouped items
    // - Grouped elements
    std::vector<std::string> names_list;
    std::map<std::string, size_t> name_positions;
    std::map<std::string, int> initial_positions;
    std::map<std::string, int> final_positions;
    std::map<std::string, grt::ListRef<T> > groups;

    // Collect names of all ungrouped items and groups in an own list for lookup.
    int item_count = 0;
    for (grt::TypedListConstIterator<T> iterator = items.begin(); iterator != items.end(); ++iterator) {
      grt::ListRef<T> target_group;

      std::string item_name = (*iterator)->name();
      size_t position = item_name.find("/");

      if (position != std::string::npos) {
        std::string group_name = item_name.substr(0, position + 1);
        if (std::find(names_list.begin(), names_list.end(), group_name) == names_list.end()) {
          item_name = group_name;
          target_group = grt::ListRef<T>(true);
          groups[item_name] = target_group;
        } else {
          final_positions[group_name] = item_count++;
          target_group = groups[group_name];
          item_name = "";
        }
      }

      // Updates data with item's element
      if (item_name.length()) {
        name_positions[item_name] = names_list.size();
        names_list.push_back(item_name);
        initial_positions[item_name] = item_count;
        final_positions[item_name] = item_count;

        item_count++;
      }

      if (target_group.is_valid())
        target_group->insert_unchecked(*iterator);
    }

    // Now gets the moved element start position
    bool grouped = false;
    if (object_type == grt::ObjectType) {
      item = grt::Ref<T>::cast_from(object);
      item_name = item->name();
      group_indicator_position = item_name.find("/");

      // If this is a grouped item it is moved within its group.
      if (group_indicator_position != std::string::npos) {
        group_name = item_name.substr(0, group_indicator_position + 1);
        grouped = true;
      }
    } else
      item_name = object.toString();

    if (grouped) {
      grt::ListRef<T> target_group = groups[group_name];
      target_group->get_index(object);

      ui_item_index = target_group->get_index(object);
    } else
      ui_item_index = name_positions[item_name];

    // Pre-processes the to value, to see if it is one of the move to commands
    if (to < 0) {
      switch (to) {
        case MoveUp:
          to = (int)ui_item_index - 1;
          break;
        case MoveDown:
          to = (int)ui_item_index + 2;
          break;
        case MoveTop:
          to = grouped ? 1 : 0;
          break;
      }
    }

    if (grouped) {
      grt::ListRef<T> target_group = groups[group_name];
      item_index = items->get_index(item);

      // In case o should point to the last element on the group
      if (to == MoveBottom)
        to = (int)target_group->count() - 1;
      else {
        // This adjustment is needed because of the way reorder works
        if ((int)ui_item_index < to)
          to--;
      }

      // The element is already first one after group tiles
      if (ui_item_index == 0 && to < 0)
        to = 0;

      target_item = grt::Ref<T>::cast_from(target_group->get(to));
      target_index = items->get_index(target_item);

      items.reorder(item_index, target_index);
    } else {
      // In case o should point to the last element on the entire list
      if (to == MoveBottom)
        to = (int)names_list.size() - 1;
      // This adjustment is needed because of the way reorder works
      else if ((int)ui_item_index < to)
        to--;

      target_index = initial_positions[names_list[to]];
      item_index = initial_positions[item_name];

      if (groups.count(item_name)) {
        grt::ListRef<T> target_group = groups[item_name];
        // When reordering items we need to consider the position of the reordered
        // items in relation with the target index.

        bool before = true;
        if (item_index > target_index)
          before = false;
        ;

        for (grt::TypedListConstIterator<T> iterator = target_group.begin(); iterator != target_group.end();
             ++iterator) {
          item_index = items.get_index(*iterator);

          // Items before the target are positioned on the target index
          if (before) {
            // This identifies the first item AFTER the target index
            if (item_index > target_index) {
              target_index++;
              before = false;
            } else
              items.reorder(item_index, target_index);
          }

          // On Items after the original target index will be inserted
          // after each other, this is to avoid reversing the connections
          // position
          if (!before)
            items.reorder(item_index, target_index++);
        }
      } else
        items.reorder(item_index, target_index);
    }
  }

  // Template instantiation to avoid having all this code in the header file.
  template WBPUBLICBACKEND_PUBLIC_FUNC void move_list_ref_item<db_mgmt_Connection>(
    grt::ListRef<db_mgmt_Connection> items, const grt::ValueRef &object, ssize_t to);
};
