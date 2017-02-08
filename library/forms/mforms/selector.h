/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include <mforms/view.h>

namespace mforms {
  class Selector;

  enum SelectorStyle {
    SelectorCombobox, //!< The value list is shown when clicking the arrow. The value is freely editable.
    SelectorPopup     //!< The value list is shown when clicking the arrow. The value can only be selected out of the
                      //! values in the list and not freely edited.
  };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct SelectorImplPtrs {
    bool (*create)(Selector *self, SelectorStyle style);
    void (*clear)(Selector *self);
    void (*add_items)(Selector *self, const std::list<std::string> &items);
    int (*add_item)(Selector *self, const std::string &item);
    std::string (*get_item)(Selector *self, int index);
    std::string (*get_text)(Selector *self);
    void (*set_index)(Selector *self, int index);
    int (*get_index)(Selector *self);
    int (*get_item_count)(Selector *self);
    void (*set_value)(Selector *self, const std::string &value);
  };
#endif
#endif

  /** A button (or text entry) with a popup menu that allows selection of a single item. */
  class MFORMS_EXPORT Selector : public View {
  public:
    /** Constructor.

     @param style - SelectorCombobox or SelectorPopup
     @param SelectorCombobox will allow input of any text in addition to picking an item from a list. */
    Selector(SelectorStyle style = SelectorPopup);

    /** Removes all items from the selectable list */
    void clear();
    /** Adds an item to the selectable list */
    int add_item(const std::string &item);
    /** Adds many items at once */
    void add_items(const std::list<std::string> &items);

    /** Gets the title of the item at the given index */
    std::string get_item_title(int i);

    /** Sets the selected item to the given index */
    void set_selected(int index);
    /** Gets the index of the given text in the list of availilable items, or -1 if not found */
    int index_of_item_with_title(const std::string &title);

    /** Gets the selected item text */
    virtual std::string get_string_value();
    /** Gets the selected item index */
    int get_selected_index();

    // set_value methods sets control text to the given value if selector is of type SelectorCombo,
    // otherwise shortcuts to set_selected(index_of_item_with_title)
    void set_value(const std::string &value);

    /** Gets number of items in the list */
    int get_item_count();

#ifndef SWIG
    /** Signal emitted when selected item changes */
    boost::signals2::signal<void()> *signal_changed() {
      return &_signal_changed;
    }

  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    void callback();
#endif
#endif
  protected:
    SelectorImplPtrs *_selector_impl;
    boost::signals2::signal<void()> _signal_changed;
    bool _updating;
    bool _editable;
  };
};
