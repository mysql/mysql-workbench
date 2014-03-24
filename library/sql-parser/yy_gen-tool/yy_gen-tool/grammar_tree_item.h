#pragma once

#include <string>
#include <list>

class Grammar_tree_item
{
public:
//  enum Item_kind {ik_terminal, ik_nonterminal};
  typedef std::list<const Grammar_tree_item *> Item_list;

  Grammar_tree_item();
  Grammar_tree_item(const char *text/*, Item_kind item_kind*/);
  virtual ~Grammar_tree_item(void);
  std::string text() const;
  std::string text_unquoted() const;
  void text(const std::string &text);
  bool empty() const;
  bool is_terminal() const;
  bool is_prec_directive() const;
  bool is_last_non_prec_child(const Grammar_tree_item *item) const;
  bool non_prec_child_exists() const;
  bool has_more_then_one_meaningful_child() const;
  const Item_list * items() const;
  void add_item_as_last(const Grammar_tree_item *item);
  void add_item_as_first(const Grammar_tree_item *item);
  void flush(std::ostream& os) const;
//  inline Item_kind item_kind() const;

private:
  std::string _text;
  Item_list _items;
//  Item_kind _item_kind;
};
