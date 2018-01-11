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

#include "../lf_mforms.h"
#include "../lf_listbox.h"
DEFAULT_LOG_DOMAIN("lf_listbox");

namespace mforms {
  namespace gtk {

    //------------------------------------------------------------------------------
    mforms::gtk::ListBoxImpl::ListBoxImpl(::mforms::ListBox *self, bool multi_select)
      : ViewImpl(self), _store(Gtk::ListStore::create(_ccol)), _lbox(_store) {
      _swin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
      _swin.set_shadow_type(Gtk::SHADOW_IN);

      _lbox.append_column("Item", _ccol._item);
      _lbox.set_headers_visible(false);
      _lbox.get_selection()->signal_changed().connect(sigc::bind(sigc::ptr_fun(&ListBoxImpl::selection_changed), self));

      _lbox.get_selection()->set_mode(multi_select ? Gtk::SELECTION_MULTIPLE : Gtk::SELECTION_SINGLE);

      _swin.add(_lbox);
      _lbox.show();
      _swin.show();
    }

    //------------------------------------------------------------------------------
    void ListBoxImpl::selection_changed(::mforms::ListBox *self) {
      self->selection_changed();
    }

    //------------------------------------------------------------------------------
    bool ListBoxImpl::create(::mforms::ListBox *self, bool multi_select) {
      return new ListBoxImpl(self, multi_select) != 0;
    }

    //------------------------------------------------------------------------------
    void ListBoxImpl::clear(::mforms::ListBox *self) {
      ListBoxImpl *sel = self->get_data<ListBoxImpl>();

      sel->_store->clear();
    }

    //------------------------------------------------------------------------------
    size_t ListBoxImpl::add_item(::mforms::ListBox *self, const std::string &item) {
      ListBoxImpl *sel = self->get_data<ListBoxImpl>();

      Gtk::TreeModel::iterator it = sel->_store->append();
      if (it) {
        Gtk::TreeModel::Row row = *it;
        if (row)
          row[sel->_ccol._item] = item;
      }
      return 0;
    }

    //------------------------------------------------------------------------------
    void ListBoxImpl::add_items(::mforms::ListBox *self, const std::list<std::string> &items) {
      ListBoxImpl *sel = self->get_data<ListBoxImpl>();

      if (sel) {
        for (std::list<std::string>::const_iterator iter = items.begin(); iter != items.end(); ++iter)
          add_item(self, *iter);
      }
    }
    //------------------------------------------------------------------------------
    void ListBoxImpl::remove_indices(mforms::ListBox *self, const std::vector<size_t> &indices) {
      ListBoxImpl *sel = self->get_data<ListBoxImpl>();

      if (sel) {
        std::list<Gtk::TreeModel::RowReference> rows;
        int row_num = 0;
        for (Gtk::TreeModel::iterator it = sel->_store->children().begin(); it != sel->_store->children().end(); ++it) {
          if (std::find(indices.begin(), indices.end(), row_num) != indices.end()) {
            rows.push_back(Gtk::TreeModel::RowReference(sel->_store, sel->_store->get_path(it)));
          }
          row_num++;
        }

        for (std::list<Gtk::TreeModel::RowReference>::iterator it = rows.begin(); it != rows.end(); ++it) {
          Gtk::TreeModel::iterator iter = sel->_store->get_iter(it->get_path());
          sel->_store->erase(iter);
        }
      }
    }
    //------------------------------------------------------------------------------
    void ListBoxImpl::remove_index(mforms::ListBox *self, size_t index) {
      ListBoxImpl *sel = self->get_data<ListBoxImpl>();

      if (sel) {
        typedef Gtk::TreeModel::Children type_children;
        type_children children = sel->_store->children();
        size_t row_num = 0;
        for (Gtk::TreeModel::iterator it = children.begin(); it <= children.end(); ++it) {
          if (row_num == index) {
            sel->_store->erase(it);
            break;
          }
          row_num++;
        }
      }
    }
    //------------------------------------------------------------------------------
    std::string ListBoxImpl::get_text(::mforms::ListBox *self) {
      ListBoxImpl *sel = self->get_data<ListBoxImpl>();
      std::string text;

      if (sel) {
        Gtk::TreeModel::const_iterator iter = sel->_lbox.get_selection()->get_selected();
        if (iter) {
          Gtk::TreeModel::Row row = *iter;
          if (row)
            text = ((Glib::ustring)(row[sel->_ccol._item])).raw();
        }
      }
      return text;
    }

    //------------------------------------------------------------------------------
    void ListBoxImpl::set_index(::mforms::ListBox *self, ssize_t index) {
      ListBoxImpl *sel = self->get_data<ListBoxImpl>();

      if (sel) {
        Glib::RefPtr<Gtk::TreeView::Selection> selection = sel->_lbox.get_selection();
        Gtk::TreeModel::Children children = sel->_store->children();
        if (children.size() > (unsigned int)index && index >= 0) {
          Gtk::TreeModel::Row row = children[index];
          if (row)
            selection->select(row);
        }
      }
    }

    //------------------------------------------------------------------------------
    ssize_t ListBoxImpl::get_index(::mforms::ListBox *self) {
      ListBoxImpl *sel = self->get_data<ListBoxImpl>();
      ssize_t ret = -1;

      if (sel) {
        Gtk::TreeModel::iterator selected = sel->_lbox.get_selection()->get_selected();
        if (selected) {
          Gtk::TreePath path(selected);
          ret = path.back();
        }
      }

      return ret;
    }

    //------------------------------------------------------------------------------
    void ListBoxImpl::set_heading(ListBox *self, const std::string &text) {
      logWarning("mforms::ListBoxImpl::set_heading('%s') not implemented", text.c_str());
    }

    static void get_selected_indices_walk_selected(const Gtk::TreeModel::Path &path, std::vector<size_t> *res) {
      res->push_back(path.back());
    }

    //------------------------------------------------------------------------------
    std::vector<std::size_t> ListBoxImpl::get_selected_indices(ListBox *self) {
      ListBoxImpl *lbi = self->get_data<ListBoxImpl>();
      std::vector<size_t> res;
      // Walk selected items and add each to the res list
      lbi->_lbox.get_selection()->selected_foreach_path(
        sigc::bind(sigc::ptr_fun(get_selected_indices_walk_selected), &res));
      return res;
    }

    //------------------------------------------------------------------------------

    size_t ListBoxImpl::get_count(ListBox *self) {
      ListBoxImpl *sel = self->get_data<ListBoxImpl>();
      return sel->_store->children().size();
    }

    //------------------------------------------------------------------------------

    std::string ListBoxImpl::get_string_value_from_index(ListBox *self, size_t index) {
      ListBoxImpl *sel = self->get_data<ListBoxImpl>();
      Gtk::TreeModel::Children children = sel->_store->children();
      std::string result;
      if (children.size() > index)
        children[index]->get_value<std::string>(0, result);
      return result;
    }

    //------------------------------------------------------------------------------

    void ListBoxImpl::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_listbox_impl.create = &ListBoxImpl::create;
      f->_listbox_impl.clear = &ListBoxImpl::clear;
      f->_listbox_impl.add_item = &ListBoxImpl::add_item;
      f->_listbox_impl.add_items = &ListBoxImpl::add_items;
      f->_listbox_impl.set_index = &ListBoxImpl::set_index;
      f->_listbox_impl.get_index = &ListBoxImpl::get_index;
      f->_listbox_impl.get_selected_indices = &ListBoxImpl::get_selected_indices;
      f->_listbox_impl.get_text = &ListBoxImpl::get_text;
      f->_listbox_impl.set_heading = &ListBoxImpl::set_heading;
      f->_listbox_impl.remove_index = &ListBoxImpl::remove_index;
      f->_listbox_impl.remove_indexes = &ListBoxImpl::remove_indices;
      f->_listbox_impl.get_count = &ListBoxImpl::get_count;
      f->_listbox_impl.get_string_value_from_index = &ListBoxImpl::get_string_value_from_index;
    }
  }
}
