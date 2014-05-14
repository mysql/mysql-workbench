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
