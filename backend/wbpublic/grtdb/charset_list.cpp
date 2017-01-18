/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WIN32
#include <algorithm>
#endif

#include "charset_list.h"
#include "grts/structs.db.mgmt.h"

using namespace bec;

CharsetList::CharsetList(const std::string &path) {
  _charset_list_path = path;
}

void CharsetList::picked_charset(const NodeId &node) {
  if (std::find(_recently_used.begin(), _recently_used.end(), node[0]) != _recently_used.end())
    _recently_used.erase(std::find(_recently_used.begin(), _recently_used.end(), node[0]));
  _recently_used.push_front(node[0]);
  if (_recently_used.size() > 5)
    _recently_used.pop_back();
}

size_t CharsetList::count_children(const NodeId &parent) {
  grt::ListRef<db_CharacterSet> charsets =
    grt::ListRef<db_CharacterSet>::cast_from(grt::GRT::get()->get(_charset_list_path));

  if (parent.depth() == 0)
    return (charsets.count() + _recently_used.size() + 1);
  else
    return charsets[parent[0]]->collations().count();
}

bool CharsetList::get_field(const NodeId &node, ColumnId column, std::string &value) {
  grt::ListRef<db_CharacterSet> charsets =
    grt::ListRef<db_CharacterSet>::cast_from(grt::GRT::get()->get(_charset_list_path));

  switch ((CharsetListColumns)column) {
    case Name:
      if (node.depth() == 1) {
        if (node[0] < _recently_used.size()) {
          std::list<size_t>::const_iterator it = _recently_used.begin();
          size_t i = node[0];
          while (i > 0) {
            --it;
            --i;
          }
          value = charsets[*it]->name();
        } else if (node[0] == _recently_used.size())
          value = "";
        else
          value = charsets[node[0] - _recently_used.size() - 1]->name();
      } else {
        if (node[0] < _recently_used.size()) {
          std::list<size_t>::const_iterator it = _recently_used.begin();
          size_t i = node[0];
          while (i > 0) {
            --it;
            --i;
          }
          value = charsets[*it]->collations()[node[1]];
        } else
          value = charsets[node[0] - static_cast<unsigned int>(_recently_used.size()) - 1]->collations()[node[1]];
      }
      return true;
    default:
      return false;
  }
}

std::string CharsetList::get_field_description(const NodeId &node, ColumnId column) {
  grt::ListRef<db_CharacterSet> charsets =
    grt::ListRef<db_CharacterSet>::cast_from(grt::GRT::get()->get(_charset_list_path));

  switch ((CharsetListColumns)column) {
    case Name:
      if (node.depth() == 1) {
        if (node[0] < _recently_used.size()) {
          std::list<size_t>::const_iterator it = _recently_used.begin();
          size_t i = node[0];
          while (i > 0) {
            --it;
            --i;
          }
          return charsets[*it]->description();
        } else
          return charsets[node[0] - _recently_used.size() - 1]->description();
      }
  }
  return "";
}
