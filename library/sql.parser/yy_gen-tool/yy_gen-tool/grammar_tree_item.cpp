/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grammar_tree_item.h"
#include <algorithm>
#include <sstream>

Grammar_tree_item::Grammar_tree_item(const char *text/*, Item_kind item_kind*/)
:
  _text(text)/*,
  _item_kind(item_kind)*/
{
}

Grammar_tree_item::Grammar_tree_item()
:
  _text("")/*,
  _item_kind(item_kind)*/
{
}

Grammar_tree_item::~Grammar_tree_item(void)
{
}

std::string Grammar_tree_item::text() const
{
  return _text;
}

std::string Grammar_tree_item::text_unquoted() const
{
  if (!_text.empty() && _text[0] == '\'')
  {
    std::stringstream oss;
    oss << (int)_text[1];
    return oss.str();
  }
  else
    return _text;
}

void Grammar_tree_item::text(const std::string &text)
{
  _text= text;
}

bool Grammar_tree_item::empty() const
{
  return _text.empty();
}

bool Grammar_tree_item::non_prec_child_exists() const
{
  bool result= false;
  
  bool prev_item_was_prec_directive= false;
  for (Item_list::const_iterator i= _items.begin(); i != _items.end(); ++i)
  {
    if ((*i)->is_prec_directive())
      prev_item_was_prec_directive= true;
    else if (prev_item_was_prec_directive || (*i)->empty())
      prev_item_was_prec_directive= false;
    else
    {
      result= true;
      break;
    }
  }

  return result;
}

bool Grammar_tree_item::has_more_then_one_meaningful_child() const
{
  bool result= false;
  
  bool prev_item_was_prec_directive= false;
  bool meaningful_child_was_found= false;
  for (Item_list::const_iterator i= _items.begin(); i != _items.end(); ++i)
  {
    if ((*i)->is_prec_directive())
      prev_item_was_prec_directive= true;
    else if (prev_item_was_prec_directive)
      prev_item_was_prec_directive= false;
    else if (!meaningful_child_was_found)
      meaningful_child_was_found= true;
    else
    {
      result= true;
      break;
    }
  }

  return result;
}

bool Grammar_tree_item::is_terminal() const
{
  bool result= true;

  for (std::string::const_iterator i= _text.begin(); i != _text.end(); ++i)
  {
    if (isalpha(*i))
      if (islower(*i))
      {
        result= false;
        break;
      }
  }

  return result;
}

bool Grammar_tree_item::is_last_non_prec_child(const Grammar_tree_item *item) const
{
  char neq_count= 0;

  if (_items.back() != item)
  {
    // coping with msvs'2005 bug, its rbegin() implementation returns garbage instead of last element
    Item_list::const_iterator i= _items.end();
    do
    {
      --i;

      if (item == (*i))
        return (0 == neq_count);
      else if ((*i)->is_prec_directive())
        neq_count= 0;
      else if (1 < ++neq_count)
        return false;
    }
    while (i != _items.begin());
  }

  return (0 == neq_count);
}

bool Grammar_tree_item::is_prec_directive() const
{
  if (_text.empty() || ('%' != _text[0]))
    return false;
  return (0 == _stricmp("%prec", _text.c_str()));
}

/*
Item_kind Grammar_tree_item::item_kind() const
{
  return _item_kind;
}
*/
const Grammar_tree_item::Item_list * Grammar_tree_item::items() const
{
  return &_items;
}

void Grammar_tree_item::add_item_as_last(const Grammar_tree_item *item)
{
  _items.push_back(item);
}

void Grammar_tree_item::add_item_as_first(const Grammar_tree_item *item)
{
  _items.push_front(item);
}

void Grammar_tree_item::flush(std::ostream& os) const
{
  os << "<elem name= '" << _text << "'>";
  for (Item_list::const_iterator i= _items.begin(); i != _items.end(); ++i)
    (*i)->flush(os);
  os << "</elem>";
}
