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

#include "treemodel_wrapper.h"
#include "wbdbg.h"
#include <gtkmm/icontheme.h>
#include <string.h>

//------------------------------------------------------------------------------
void TreeModelWrapper::update_root_node(const bec::NodeId& root_node) {
  _root_node_path = root_node.toString();
  _root_node_path_dot = root_node.toString() + ".";

  _stamp++;
}

//------------------------------------------------------------------------------
// bool TreeModelWrapper::init_gtktreeiter(GtkTreeIter *it, const bec::NodeId &node) const
//{
//  if ( _tm && it && node.is_valid() )
//  {
//    Index id(it, node);
//    id.stamp(_stamp);
//  }
//  return it && node.is_valid();
//}

//------------------------------------------------------------------------------
// bec::NodeId TreeModelWrapper::node_for_iter(const iterator &iter) const
//{
//  const GtkTreeIter* it = iter.gobj();
//  const Index id(it);
//  bec::NodeId  node;
//
//  if ( it )
//  {
//    if (id.cmp_stamp(_stamp))
//      node = id.to_node();
//    else
//      g_warning("reference to invalid iter in grt list/tree model");
//  }
//
//  return node;
//}

//------------------------------------------------------------------------------
bec::NodeId TreeModelWrapper::get_node_for_path(const Gtk::TreeModel::Path& path) const {
  if (path.empty())
    return _root_node_path_dot;
  return bec::NodeId(_root_node_path_dot + path.to_string());
}

//------------------------------------------------------------------------------
bool TreeModelWrapper::get_iter_vfunc(const Path& path, iterator& iter) const {
  if (!tm() || _invalid)
    return false;

  bool ret = false;
  GtkTreeIter* it = iter.gobj();

  // g_message("get iter for %s + %s\n", _root_node_path_dot.c_str(), path.to_string().c_str());
  bec::NodeId node(_root_node_path_dot + path.to_string());

  try {
    if (node.is_valid() && tm()->count_children(tm()->get_parent(node)) > node.back())
      ret = init_gtktreeiter(it, node);
  } catch (...) {
    ret = false;
  }

  return ret;
}

Gtk::TreeModel::Path TreeModelWrapper::get_path_vfunc(const iterator& iter) const {
  bec::NodeId node = node_for_iter(iter);
  Gtk::TreeModel::Path path;

  if (node.is_valid()) {
    const int node_depth = node.depth();

    // get path from an iterator. The iterator points to the node, so
    // we have to trim the root node prefix so we have a path to the tree
    for (int i = bec::NodeId(_root_node_path).depth(); i < node_depth; i++)
      path.push_back(node[i]);
  }
  return path;
}

//------------------------------------------------------------------------------

void TreeModelWrapper::tree_row_collapsed(const iterator& iter, const Path& path) {
  if (tm()) {
    if (_expanded_rows)
      _expanded_rows->erase(path.to_string());

    tm()->collapse_node(node_for_iter(iter));
  }
}

//------------------------------------------------------------------------------

void TreeModelWrapper::tree_row_expanded(const iterator& iter, const Path& path) {
  if (tm()) {
    if (_expanded_rows)
      _expanded_rows->insert(path.to_string());

    tm()->expand_node(node_for_iter(iter));
  }
}

//------------------------------------------------------------------------------
void TreeModelWrapper::get_icon_value(const iterator& iter, int column, const bec::NodeId& node,
                                      Glib::ValueBase& value) const {
  if (!tm())
    return;

  static ImageCache* pixbufs = ImageCache::get_instance();
  static Glib::RefPtr<Gtk::IconTheme> icon_theme = Gtk::IconTheme::get_default();
  GValue* gval = value.gobj();

  g_value_init(gval, GDK_TYPE_PIXBUF);

  bec::IconId icon_id = tm()->get_field_icon(node, column, get_icon_size());
  if (icon_id != 0) {
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = pixbufs->image(icon_id);

    if (pixbuf)
      g_value_set_object(gval, pixbuf->gobj());
    else {
      if (tm()->is_expandable(node)) {
        Glib::RefPtr<Gdk::Pixbuf> pixbuf = icon_theme->load_icon("folder", 16, (Gtk::IconLookupFlags)0);

        if (pixbuf)
          g_value_set_object(gval, pixbuf->gobj());
      }
    }
  } else {
    if (tm()->is_expandable(node)) {
      Glib::RefPtr<Gdk::Pixbuf> pixbuf = icon_theme->load_icon("folder", 16, (Gtk::IconLookupFlags)0);

      if (pixbuf)
        g_value_set_object(gval, pixbuf->gobj());
    }
  }
}

//------------------------------------------------------------------------------

Gtk::TreeModelFlags TreeModelWrapper::get_flags_vfunc() const {
  if (_show_as_list)
    return Gtk::TREE_MODEL_ITERS_PERSIST | Gtk::TREE_MODEL_LIST_ONLY;
  else
    return Gtk::TREE_MODEL_ITERS_PERSIST;
}

//------------------------------------------------------------------------------
bool TreeModelWrapper::iter_children_vfunc(const iterator& parent, iterator& iter) const {
  // dprint("%s\n", __FUNCTION__);
  return _show_as_list ? false : iter_nth_child_vfunc(parent, 0, iter);
}

//------------------------------------------------------------------------------
bool TreeModelWrapper::iter_parent_vfunc(const iterator& child, iterator& iter) const {
  // dprint("%s\n", __FUNCTION__);
  bool ret = false;
  if (tm()) {
    bec::NodeId node(node_for_iter(child));

    if (node.is_valid()) {
      // Make iter (parent would be iterator) invalid from the start, just in case the code below
      // will fail
      reset_iter(iter);

      bec::NodeId would_be_parent(tm()->get_parent(node));
      if (would_be_parent.is_valid()) {
        init_gtktreeiter(iter.gobj(), would_be_parent);
        ret = true;
        // dprint("%s(child->'%s', parent->'%s')\n", __FUNCTION__, bec::NodeId(tm->nodeuid_to_path(uid)).repr().c_str(),
        // would_be_parent.repr().c_str());
      }
    }
  }
  return ret;
}

//------------------------------------------------------------------------------
bool TreeModelWrapper::iter_nth_child_vfunc(const iterator& parent, int n, iterator& iter) const {
  // dprint("%s\n", __FUNCTION__);
  // Sets @a iter to be the child of @a parent using the given index.  The first
  // index is 0.  If @a n is too big, or @a parent has no children, @a iter is set
  // to an invalid iterator and false is returned.
  // See also iter_nth_root_child_vfunc()
  bool ret = false;
  bec::NodeId parent_node(node_for_iter(parent));

  // Invalidate iter_next
  reset_iter(iter);
  if (tm()) {
    if (parent_node.is_valid()) {
      const int children_count = tm()->count_children(parent_node);

      if (n >= 0 && children_count > 0 && n < children_count) {
        bec::NodeId child(tm()->get_child(parent_node, n));
        if (child.is_valid()) {
          init_gtktreeiter(iter.gobj(), child);
          ret = true;
          // dprint("%s(parent->'%s', int n = %i, iter->'%s')\n", __FUNCTION__, parent_node.repr().c_str(), n,
          // child.repr().c_str());
        }
      }
    }
  }
  return ret;
}

//------------------------------------------------------------------------------
bool TreeModelWrapper::iter_nth_root_child_vfunc(int n, iterator& iter) const {
  // dprint("%s(n = %i), _tm->count() = %i   (as list=%i)\n", __FUNCTION__, n, _tm->count(), _show_as_list);
  bool ret = false;
  bec::NodeId root_node(_root_node_path);
  // Sets @a iter to be the child of at the root level using the given index.  The first
  // index is 0.  If @a n is too big, or if there are no children, @a iter is set
  // to an invalid iterator and false is returned.
  // See also iter_nth_child_vfunc().
  if (tm() && n >= 0 && n < (int)tm()->count_children(root_node)) {
    bec::NodeId node = tm()->get_child(root_node, n);
    init_gtktreeiter(iter.gobj(), node);
    ret = true;
    // dprint("%i = %s(n = %i, iter& -> '%s')\n", ret, __FUNCTION__, n, node.repr().c_str());
  }

  return ret;
}

//------------------------------------------------------------------------------
bool TreeModelWrapper::iter_has_child_vfunc(const iterator& iter) const {
  if (_invalid)
    return false;

  // dprint("has child? %i (cont= %i)", _show_as_list ? false : iter_n_children_vfunc(iter) != 0,
  // iter_n_children_vfunc(iter));
  return _show_as_list ? false : iter_n_children_vfunc(iter) != 0;
}

//------------------------------------------------------------------------------
int TreeModelWrapper::iter_n_children_vfunc(const iterator& iter) const {
  if (!_children_count_enabled)
    return 1;

  int ret = 0;

  bec::NodeId node(node_for_iter(iter));

  if (tm() && node.is_valid()) {
    // can't expand here otherwise the live tree will pre-fetch everything when the tree is shown
    if (!_delay_expanding_nodes)
      // need to expand here because otherwise the backend won't know how many children the node has
      tm()->expand_node(node);

    ret = tm()->count_children(node);
  }

  return ret;
}

//------------------------------------------------------------------------------
int TreeModelWrapper::iter_n_root_children_vfunc() const {
  const bec::NodeId parent(_root_node_path);
  const int ret = tm() ? tm()->count_children(parent) : 0;

  // dprint("%i = %s() at path '%s'\n", ret, __FUNCTION__, _root_node_path.c_str());

  return ret;
}

//------------------------------------------------------------------------------
void TreeModelWrapper::block_expand_collapse_signals() {
  _expand_signal.block();
  _collapse_signal.block();
}

//------------------------------------------------------------------------------
void TreeModelWrapper::unblock_expand_collapse_signals() {
  _expand_signal.unblock();
  _collapse_signal.unblock();
}
