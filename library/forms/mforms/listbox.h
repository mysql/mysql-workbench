/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _MFORMS_LISTBOX_H_
#define _MFORMS_LISTBOX_H_

#include <mforms/view.h>

#include <vector>

namespace mforms {
  class ListBox;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct ListBoxImplPtrs
  {
    bool (*create)(ListBox *self, bool multi_select);
    void (*clear)(ListBox *self);
    void (*set_heading)(ListBox *self, const std::string &text);
    void (*add_items)(ListBox *self, const std::list<std::string> &items);
    int (*add_item)(ListBox *self, const std::string &item);
    void (*remove_indexes)(ListBox *self, const std::vector<int> &indexes); // TODO: Linux.
    void (*remove_index)(ListBox *self, int index); // TODO: Linux.
    std::string (*get_text)(ListBox *self);
    void (*set_index)(ListBox *self, int index);
    int (*get_index)(ListBox *self);
    std::vector<int> (*get_selected_indices)(ListBox* self);
  };
#endif
#endif

  /** A list control with a single column and multiple rows. */
  class MFORMS_EXPORT ListBox : public View
  {
  public:
    /** Constructor.
     
     @param multi - whether multiple selection is allowed. 
     */
    ListBox(bool multi);

    /** Clear rows from the list. */
    void clear();
    /** Sets a text for the control heading. */
    void set_heading(const std::string &text);
    
    /** Add an item in a new row to the list. */
    int add_item(const std::string &item);
    
    /** Quickly add multiple items to the list. */
    void add_items(const std::list<std::string> &items);

    /** Remove the item at the given index. */
    void remove_index(int index);

    /** Quickly remove multiple items from the list.
     *  Note: it is assumed that the indices are sorted in increasing order! */
    void remove_indexes(const std::vector<int> &indexes);

    /** Sets the selected row in the list. */
    void set_selected(int index);

    /** Gets the text of the selected row in the list. */
    virtual std::string get_string_value();
    
    /** Gets the index of the selected row in the list */
    int get_selected_index();
    
    /** Gets the list of selected indexes in the list */
    std::vector<int> get_selected_indices(); // For multi selection lists.

#ifndef SWIG
    /** Signal emitted when the selection changes. 
     
     In Python use add_changed_callback()
     */
    boost::signals2::signal<void ()>* signal_changed() { return &_signal_changed; }

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  public:
    void selection_changed();
#endif
#endif
  protected:
    ListBoxImplPtrs *_listbox_impl;

    boost::signals2::signal<void ()> _signal_changed;
    
    bool _updating;
  };
};

#endif
