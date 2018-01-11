/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <mforms/view.h>

#include <vector>

namespace mforms {
  class ListBox;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct ListBoxImplPtrs {
    bool (*create)(ListBox *self, bool multi_select);
    void (*clear)(ListBox *self);
    void (*set_heading)(ListBox *self, const std::string &text);
    void (*add_items)(ListBox *self, const std::list<std::string> &items);
    size_t (*add_item)(ListBox *self, const std::string &item);
    void (*remove_indexes)(ListBox *self, const std::vector<size_t> &indexes);
    void (*remove_index)(ListBox *self, size_t index);
    std::string (*get_text)(ListBox *self);
    void (*set_index)(ListBox *self, ssize_t index);
    ssize_t (*get_index)(ListBox *self);
    std::vector<size_t> (*get_selected_indices)(ListBox *self);
    size_t (*get_count)(ListBox *self);
    std::string (*get_string_value_from_index)(ListBox *self, size_t index);
  };
#endif
#endif

  /** A list control with a single column and multiple rows. */
  class MFORMS_EXPORT ListBox : public View {
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
    size_t add_item(const std::string &item);

    /** Quickly add multiple items to the list. */
    void add_items(const std::list<std::string> &items);

    /** Remove the item at the given index. */
    void remove_index(size_t index);

    /** Quickly remove multiple items from the list.
     *  Note: it is assumed that the indices are sorted in increasing order! */
    void remove_indexes(const std::vector<size_t> &indexes);

    /** Sets the selected row in the list. */
    void set_selected(ssize_t index);

    /** Gets the text of the selected row in the list. */
    virtual std::string get_string_value();

    /** Gets the index of the selected row in the list */
    ssize_t get_selected_index();

    /** Gets the list of selected indexes in the list */
    std::vector<size_t> get_selected_indices(); // For multi selection lists.

    /** Gets the number of rows in the list */
    size_t get_count();

    /** Gets the string value from the "index" position */
    std::string get_string_value_from_index(size_t index);

#ifndef SWIG
    /** Signal emitted when the selection changes.

     In Python use add_changed_callback()
     */
    boost::signals2::signal<void()> *signal_changed() {
      return &_signal_changed;
    }

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  public:
    void selection_changed();
#endif
#endif
  protected:
    ListBoxImplPtrs *_listbox_impl;

    boost::signals2::signal<void()> _signal_changed;

    bool _updating;
  };
};
