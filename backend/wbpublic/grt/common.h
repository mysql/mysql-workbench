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

#pragma once

#include <cstring>
#include <vector>
#include <string>
#include <glib.h>
#include <algorithm>

#ifndef _MSC_VER
#include <stdexcept>
#include <stdarg.h>
#endif

#include <string.h>
#include "grt.h"

#include "wbpublic_public_interface.h"

namespace bec {

  enum MatchType { MatchAny, MatchBefore, MatchAfter, MatchLast };

  enum MoveType { MoveTop = -1, MoveUp = -2, MoveDown = -3, MoveBottom = -4 };

  enum FindType { FindPrefix, FindFull };

  WBPUBLICBACKEND_PUBLIC_FUNC bool validate_tree_structure(const grt::ObjectRef &object);

  template <class T>
  size_t find_list_ref_item_position(grt::ListRef<T> &item_data, std::string &name, MatchType match = MatchAny,
                                     grt::Ref<T> *reference = NULL, FindType find_mode = FindPrefix) {
    if ((match == MatchBefore || match == MatchAfter) && !reference)
      throw std::invalid_argument("A reference must be specified for MatchBefore and MatchAfter");

    bool search_enabled = match != MatchAfter;
    bool exit = false;

    size_t index = grt::BaseListRef::npos;

    for (grt::TypedListConstIterator<T> end = item_data.end(), inst = item_data.begin(); inst != end && !exit; ++inst) {
      // If skip is defined will omit the entries until the 'skip' element is found
      if (search_enabled) {
        // For MatchBefore the search ends when the reference item is found
        if (match == MatchBefore && (*reference) == (*inst))
          exit = true;
        else {
          std::string item_name = (*inst)->name();

          int compare_result =
            (find_mode == FindPrefix) ? item_name.compare(0, name.length(), name) : item_name.compare(name);

          // index will contain always the position of the last matched entry
          if (compare_result == 0) {
            index = item_data.get_index(*inst);

            // MatchBefore needs to search until the reference is found
            // MatchLast needs to search until the whole list has been searched to get the last match
            // MatchAfter and MatchAny are done as soon as a match is found
            if (match != MatchBefore && match != MatchLast)
              exit = true;
          }
        }
      }

      // For MatchAfter the search starts once the reference item has been found
      else if ((*reference) == (*inst))
        search_enabled = true;
    }

    return index;
  }

  template <class T>
  WBPUBLICBACKEND_PUBLIC_FUNC void move_list_ref_item(grt::ListRef<T> items, const grt::ValueRef &object, ssize_t to);
};
