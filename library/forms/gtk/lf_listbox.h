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

#ifndef _LF_LISTBOX_H_
#define _LF_LISTBOX_H_

#include "mforms/listbox.h"
#include "base/log.h"
#include "lf_view.h"

namespace mforms {
  namespace gtk {

    class ListBoxImpl : public ViewImpl {
      class ComboColumn : public Gtk::TreeModel::ColumnRecord {
      public:
        ComboColumn() {
          add(_item);
        }
        Gtk::TreeModelColumn<Glib::ustring> _item;
      };

      ComboColumn _ccol;
      Glib::RefPtr<Gtk::ListStore> _store;
      Gtk::TreeView _lbox;
      Gtk::ScrolledWindow _swin;

      virtual Gtk::Widget *get_outer() const {
        return &(const_cast<Gtk::ScrolledWindow &>(_swin));
      }
      virtual Gtk::Widget *get_inner() const {
        return &(const_cast<Gtk::TreeView &>(_lbox));
      }

    protected:
      ListBoxImpl(::mforms::ListBox *self, bool multi_select);
      static void selection_changed(::mforms::ListBox *self);
      static bool create(::mforms::ListBox *self, bool multi_select);
      static void clear(::mforms::ListBox *self);
      static size_t add_item(::mforms::ListBox *self, const std::string &item);
      static void add_items(::mforms::ListBox *self, const std::list<std::string> &items);
      static void remove_indices(mforms::ListBox *backend, const std::vector<size_t> &indices);
      static void remove_index(mforms::ListBox *backend, size_t index);
      static std::string get_text(::mforms::ListBox *self);
      static void set_index(::mforms::ListBox *self, ssize_t index);
      static ssize_t get_index(::mforms::ListBox *self);
      static void set_heading(ListBox *self, const std::string &text);
      static std::vector<size_t> get_selected_indices(ListBox *self);
      static size_t get_count(ListBox *self);
      static std::string get_string_value_from_index(ListBox *self, size_t index);

    public:
      static void init();
    };
  }
}

#endif /* _LF_LISTBOX_H_ */
