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
#ifndef _STUB_TREENODEVIEW_H_
#define _STUB_TREENODEVIEW_H_

#include "stub_view.h"
#include "stub_treenode.h"

namespace mforms {
namespace stub {


class TreeNodeViewWrapper : public ViewWrapper
{
private:
  TreeNodeWrapper *_root;

  TreeNodeViewWrapper(TreeNodeView *self, mforms::TreeOptions opts)
      : ViewWrapper(self)
  {
    _root = new TreeNodeWrapper();

    self->set_data(this);
  }

  int add_column(TreeColumnType type, const std::string &name, int initial_width, bool editable)
  {
    return 0;
  }

  void end_columns()
  {
  }

  static bool create(TreeNodeView *self, mforms::TreeOptions opt)
  {
    TreeNodeViewWrapper *ptree_node_view = new TreeNodeViewWrapper(self, opt);
    return true;
  }

  static int add_column(TreeNodeView *self, TreeColumnType type, const std::string &name, int width, bool editable)
  {
    return 0;
  }
  static int add_column(TreeNodeView *self, TreeColumnType type, const std::string &name, int width, bool editable, bool a)
  {
    return 0;
  }
  static void end_columns(TreeNodeView *self)
  {
  }
  static void clear(TreeNodeView *self)
  {
  }
  static TreeSelectionMode get_selection_mode(TreeNodeView *self)
  {
    return TreeSelectSingle;
  }
  static void set_selection_mode(TreeNodeView *self, TreeSelectionMode mode)
  {
  }
  static TreeNodeRef get_selected_node(TreeNodeView *self)
  {
    return TreeNodeRef();
  }
  static TreeNodeRef root_node(TreeNodeView *self)
  {
    TreeNodeViewWrapper *ptree_node_view = self->get_data<TreeNodeViewWrapper>();
    return ptree_node_view->root_node();
  }

  TreeNodeRef root_node()
  {
    return mforms::TreeNodeRef(_root);
  }

  static std::list<TreeNodeRef> get_selection(TreeNodeView *self)
  {
    return std::list<TreeNodeRef>();
  }

  static void set_selected(TreeNodeView* self, TreeNodeRef node, bool flag)
  {
  }

  static void set_allow_sorting(TreeNodeView *self, bool)
  {
  }

  static void freeze_refresh(TreeNodeView *self, bool)
  {
  }

  static int row_for_node(TreeNodeView *self, TreeNodeRef node)
  {
    return 0;
  }

  static TreeNodeRef node_at_row(TreeNodeView *self, int row)
  {
    return self->root_node();
  }

  static void set_row_height(TreeNodeView *self, int height)
  {
  }

  static void clear_selection(TreeNodeView *self)
  {
  }

  static TreeNodeRef  node_with_tag(TreeNodeView *self, const std::string &tag)
  {
      return self->root_node();
  }

public:
  static void init()
  {
    ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

    f->_treenodeview_impl.create= &TreeNodeViewWrapper::create;
    f->_treenodeview_impl.add_column= &TreeNodeViewWrapper::add_column;
    f->_treenodeview_impl.end_columns= &TreeNodeViewWrapper::end_columns;
    f->_treenodeview_impl.clear= &TreeNodeViewWrapper::clear;
    f->_treenodeview_impl.get_selected_node= &TreeNodeViewWrapper::get_selected_node;
    f->_treenodeview_impl.set_selected= &TreeNodeViewWrapper::set_selected;
    f->_treenodeview_impl.get_selection= &TreeNodeViewWrapper::get_selection;
    f->_treenodeview_impl.set_allow_sorting = &TreeNodeViewWrapper::set_allow_sorting;
    f->_treenodeview_impl.freeze_refresh = &TreeNodeViewWrapper::freeze_refresh;
    f->_treenodeview_impl.set_selection_mode = &TreeNodeViewWrapper::set_selection_mode;
    f->_treenodeview_impl.get_selection_mode = &TreeNodeViewWrapper::get_selection_mode;
    f->_treenodeview_impl.root_node = &TreeNodeViewWrapper::root_node;
    f->_treenodeview_impl.node_at_row = &TreeNodeViewWrapper::node_at_row;
    f->_treenodeview_impl.row_for_node = &TreeNodeViewWrapper::row_for_node;
    f->_treenodeview_impl.set_row_height = &TreeNodeViewWrapper::set_row_height;
    f->_treenodeview_impl.clear_selection = &TreeNodeViewWrapper::clear_selection;
    f->_treenodeview_impl.node_with_tag = &TreeNodeViewWrapper::node_with_tag;
  }
};

  
}
}

#endif
