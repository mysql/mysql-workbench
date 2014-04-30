/* 
 * Copyright (c) 2007, 2010, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _GRT_STRING_LIST_MODEL_
#define _GRT_STRING_LIST_MODEL_

#include "wbpublic_public_interface.h"
#include "tree_model.h"

namespace bec {

  class WBPUBLICBACKEND_PUBLIC_FUNC GrtStringListModel : public ListModel
  {
  public:
    enum Columns {
      Name
    };
    typedef std::vector<int> Items_ids;
    
    GrtStringListModel();
    void icon_id(IconId icon_id);
    void reset();
    void reset(const std::list<std::string> &items);

    virtual int count();
    int active_items_count() const;
    int total_items_count() const;
    virtual void refresh();
    virtual IconId get_field_icon(const NodeId &node, int column, IconSize size);

    void add_item(const grt::StringRef &item, int ident);
    void remove_item(int index);
    void remove_items(std::vector<int> &item_indexes);
    void copy_items_to_val_masks_list(std::vector<int> &item_indexes);

    void invalidate();

    int get_item_id(size_t item_index);
    std::vector<std::string> items() const;
    Items_ids items_ids() const;

    void items_val_mask(const std::string items_val_mask);
    const std::string & items_val_mask() const;
    void items_val_masks(GrtStringListModel *items_val_masks);
    GrtStringListModel * items_val_masks() const;

    virtual bool get_field(const NodeId &node, int column, std::string &value);
  protected:
    struct Item_handler
    {
      Item_handler() {}
      Item_handler(const std::string &val_, int id_) : val(val_), iid(id_) {}
      std::string val;
      int iid; // initial sequence number of the item
      bool operator<(const Item_handler &item2) const { return (val < item2.val); }
    };

    GrtStringListModel *_items_val_masks;
    std::string _items_val_mask;

    typedef std::vector<Item_handler> Items;
    Items _items;
    std::vector<size_t> _visible_items;
    bec::IconId _icon_id;
    size_t _active_items_count;

    bool _invalidated;

    void process_mask(const std::string &mask, std::vector<bool> &items, bool match_means_visible) const;
    std::string terminate_wildcard_symbols(const std::string &str);
  };
};

#endif /* _GRT_STRING_LIST_MODEL_ */
