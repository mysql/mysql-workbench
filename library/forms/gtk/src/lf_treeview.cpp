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

#include <inttypes.h>

#include "../lf_mforms.h"
#include "../lf_treeview.h"
#include "../lf_utilities.h"

#include <glib.h>
#include "base/log.h"

DEFAULT_LOG_DOMAIN("mforms.linux");

namespace mforms {
  namespace gtk {

    static int count_rows_in_node(Gtk::TreeView *tree, const Gtk::TreeIter &iter) {
      if (tree->row_expanded(Gtk::TreePath(iter))) {
        Gtk::TreeRow row = *iter;
        int count = 0;
        for (Gtk::TreeIter last = row.children().end(), i = row.children().begin(); i != last; i++) {
          count++;
          count += count_rows_in_node(tree, i);
        }
        return count;
      }
      return 0;
    }

    static int calc_row_for_node(Gtk::TreeView *tree, const Gtk::TreeIter &iter) {
      Gtk::TreeIter parent = iter->parent();
      int node_index = Gtk::TreePath(iter).back();
      int row = node_index;

      if (parent) {
        for (Gtk::TreeIter i = parent->children().begin(); i != iter; i++)
          row += count_rows_in_node(tree, i);
        row += calc_row_for_node(tree, parent) + 1;
      } else {
        Gtk::TreePath path(iter);
        while (path.prev())
          row += count_rows_in_node(tree, tree->get_model()->get_iter(path));
      }
      return row;
    }

    CustomTreeStore::CustomTreeStore(const Gtk::TreeModelColumnRecord &columns)
      : TreeStore(columns){
          // noop
        };

    Glib::RefPtr<CustomTreeStore> CustomTreeStore::create(const Gtk::TreeModelColumnRecord &columns) {
      return Glib::RefPtr<CustomTreeStore>(new CustomTreeStore(columns));
    }

    void CustomTreeStore::copy_iter(Gtk::TreeModel::iterator &from, Gtk::TreeModel::iterator &to) {
      for (int i = 0; i < get_n_columns(); ++i) {
        Glib::ValueBase val;
        get_value_impl(from, i, val);
        set_value_impl(to, i, val);
      }
    }

    bool RootTreeNodeImpl::is_root() const {
      return true;
    }

    bool RootTreeNodeImpl::is_valid() const {
      return _treeview != 0;
    }

    bool RootTreeNodeImpl::equals(const TreeNode &other) {
      const RootTreeNodeImpl *impl = dynamic_cast<const RootTreeNodeImpl *>(&other);
      if (impl)
        return impl == this;
      return false;
    }

    int RootTreeNodeImpl::level() const {
      return 0;
    }

    RootTreeNodeImpl::RootTreeNodeImpl(TreeViewImpl *tree) // rename
      : _treeview(tree),
        _refcount(0) // refcount must start at 0
    {
    }

    void RootTreeNodeImpl::invalidate() {
      _treeview = 0;
    }

    void RootTreeNodeImpl::release() {
      _refcount--;
      if (_refcount == 0)
        delete this;
    }

    void RootTreeNodeImpl::retain() {
      _refcount++;
    }

    int RootTreeNodeImpl::count() const {
      if (is_valid()) {
        Glib::RefPtr<Gtk::TreeStore> store(_treeview->tree_store());
        return store->children().size();
      }
      return 0;
    }

    bool RootTreeNodeImpl::can_expand() {
      return count() > 0;
    }

    Gtk::TreeIter RootTreeNodeImpl::create_child(int index) {
      Glib::RefPtr<Gtk::TreeStore> store(_treeview->tree_store());
      Gtk::TreeIter new_iter;

      if (index < 0 || index >= (int)store->children().size())
        new_iter = store->append();
      else {
        Gtk::TreePath path;
        path.push_back(index);
        new_iter = store->insert(store->get_iter(path));
      }

      return new_iter;
    }

    Gtk::TreeIter RootTreeNodeImpl::create_child(int index, Gtk::TreeIter *other_parent) {
      Glib::RefPtr<Gtk::TreeStore> store(_treeview->tree_store());
      Gtk::TreeIter new_iter;

      if (index < 0)
        new_iter = other_parent ? store->append((*other_parent)->children()) : store->append();
      else {
        Gtk::TreePath path;

        if (other_parent)
          path = store->get_path(*other_parent);

        path.push_back(index);

        new_iter = store->insert(store->get_iter(path));
      }

      return new_iter;
    }

    TreeNodeRef RootTreeNodeImpl::insert_child(int index) {
      if (is_valid()) {
        Gtk::TreeIter new_iter = create_child(index);
        return ref_from_iter(new_iter);
      }
      return TreeNodeRef();
    }

    std::vector<mforms::TreeNodeRef> RootTreeNodeImpl::add_node_collection(const TreeNodeCollectionSkeleton &nodes,
                                                                           int position) {
      std::vector<Gtk::TreeIter> added_iters;
      std::vector<mforms::TreeNodeRef> added_nodes;

      // Allocates enough room for the returned TreeNodeRefs
      added_nodes.reserve(nodes.captions.size());

      // If the nodes have children, also allocates enough room
      // For the created iters
      bool sub_items = !nodes.children.empty();
      if (sub_items)
        added_iters.reserve(nodes.captions.size());

      Glib::RefPtr<Gtk::TreeStore> store(_treeview->tree_store());
      Gtk::TreeIter new_iter;

      std::vector<std::string>::const_iterator it, end = nodes.captions.end();

      // Gets the icon to be used on all the nodes...
      Glib::RefPtr<Gdk::Pixbuf> pixbuf;
      if (!nodes.icon.empty())
        pixbuf = UtilitiesImpl::get_cached_icon(nodes.icon);

      int index_for_string = _treeview->index_for_column(0);
      int index_for_icon = index_for_string - 1;

      store->freeze_notify();

      for (it = nodes.captions.begin(); it != end; ++it) {
        if (!new_iter)
          new_iter = create_child(position);
        else
          new_iter = store->insert_after(new_iter);

        // Masks the iterator as a row
        Gtk::TreeRow row = *new_iter;

        // Sets the string
        std::string nvalue(*it);

        //    base::replace(nvalue, "&", "&amp;");
        //   base::replace(nvalue, "<", "&lt;");
        //  base::replace(nvalue, ">", "&gt;");

        row.set_value(index_for_string, nvalue);

        // Sets the icon...
        row.set_value(index_for_icon, pixbuf);

        added_nodes.push_back(ref_from_iter(new_iter));

        // If there are sub items the iter needs to be stored so
        // it gets the childs added
        if (sub_items)
          added_iters.push_back(new_iter);
      }

      // If there are sub items adds them into each of the
      // added iters at this level
      if (sub_items)
        add_children_from_skeletons(added_iters, nodes.children);

      store->thaw_notify();

      return added_nodes;
    }

    void RootTreeNodeImpl::add_children_from_skeletons(const std::vector<Gtk::TreeIter> &parents,
                                                       const std::vector<TreeNodeSkeleton> &children) {
      std::vector<Gtk::TreeIter> last_item;
      Glib::RefPtr<Gtk::TreeStore> store(_treeview->tree_store());
      Gtk::TreeIter new_iter;
      Gtk::TreeRow row;

      // Takes each received child and inserts it to all the received parents
      std::vector<TreeNodeSkeleton>::const_iterator it, end = children.end();
      for (it = children.begin(); it != end; it++) {
        // added iters is the child nodes being added, if the structure indicates they
        // will also have childs, then it is needed to store them to call the function
        // recursively, we reserve the needed space for performance reasons
        std::vector<Gtk::TreeIter> added_iters;
        bool sub_items = !(*it).children.empty();
        if (sub_items)
          added_iters.reserve(parents.size());

        // Gets the icon to be used on this child...
        Glib::RefPtr<Gdk::Pixbuf> pixbuf(UtilitiesImpl::get_cached_icon((*it).icon));

        // Formats the string to be used in this child...
        std::string nvalue((*it).caption);
        //  base::replace(nvalue, "&", "&amp;");
        //  base::replace(nvalue, "<", "&lt;");
        //  base::replace(nvalue, ">", "&gt;");

        // Gets the indexes where the information will be stored
        int index_for_string = _treeview->index_for_column(0);
        int index_for_icon = index_for_string - 1;
        Gtk::TreeModelColumn<std::string> &tag_column = _treeview->_columns.tag_column();

        // Now inserts the child on all the received parents
        for (size_t index = 0; index < parents.size(); index++) {
          if (last_item.size() > index) {
            new_iter = store->insert_after(last_item[index]);
            last_item[index] = new_iter;
          } else {
            Gtk::TreeIter parent = parents[index];
            new_iter = create_child(-1, &parent);
            last_item.push_back(new_iter);
          }

          // Sets the new item data
          row = *new_iter;
          row.set_value(index_for_string, nvalue);
          row.set_value(index_for_icon, pixbuf);
          row[tag_column] = (*it).tag;

          // If this child also has childs, stores the new child to be
          // a futur parent
          if (sub_items)
            added_iters.push_back(new_iter);
        }

        // If childs will be assigned then it is done by calling the
        // function again
        if (sub_items)
          add_children_from_skeletons(added_iters, (*it).children);
      }
    }

    void RootTreeNodeImpl::remove_from_parent() {
      throw std::logic_error("Cannot delete root node");
    }

    TreeNodeRef RootTreeNodeImpl::get_child(int index) const {
      if (is_valid()) {
        Glib::RefPtr<Gtk::TreeStore> store(_treeview->tree_store());
        return ref_from_iter(store->children()[index]);
      }
      return TreeNodeRef();
    }

    TreeNodeRef RootTreeNodeImpl::get_parent() const {
      return TreeNodeRef();
    }

    void RootTreeNodeImpl::expand() {
    }

    void RootTreeNodeImpl::collapse() {
      g_warning("Can't collapse root node");
    }

    bool RootTreeNodeImpl::is_expanded() {
      return true;
    }

    void RootTreeNodeImpl::set_attributes(int column, const mforms::TreeNodeTextAttributes &attrs) {
      // noop
    }

    void RootTreeNodeImpl::set_icon_path(int column, const std::string &icon) { // noop
    }

    void RootTreeNodeImpl::set_string(int column, const std::string &value) { // noop
    }

    void RootTreeNodeImpl::set_int(int column, int value) { // noop
    }

    void RootTreeNodeImpl::set_long(int column, std::int64_t value) { // noop
    }

    void RootTreeNodeImpl::set_bool(int column, bool value) { // noop
    }

    void RootTreeNodeImpl::set_float(int column, double value) { // noop
    }

    std::string RootTreeNodeImpl::get_string(int column) const {
      return "";
    }

    int RootTreeNodeImpl::get_int(int column) const {
      return 0;
    }

    std::int64_t RootTreeNodeImpl::get_long(int column) const {
      return 0;
    }

    bool RootTreeNodeImpl::get_bool(int column) const {
      return false;
    }

    double RootTreeNodeImpl::get_float(int column) const {
      return 0.0;
    }

    void RootTreeNodeImpl::set_tag(const std::string &tag) { // noop
    }

    std::string RootTreeNodeImpl::get_tag() const {
      return "";
    }

    void RootTreeNodeImpl::set_data(TreeNodeData *data) { // noop
    }

    TreeNodeData *RootTreeNodeImpl::get_data() const {
      return NULL;
    }

    TreeNodeRef RootTreeNodeImpl::previous_sibling() const {
      return TreeNodeRef();
    }

    TreeNodeRef RootTreeNodeImpl::next_sibling() const {
      return TreeNodeRef();
    }

    int RootTreeNodeImpl::get_child_index(TreeNodeRef child) const {
      TreeNodeImpl *node = dynamic_cast<TreeNodeImpl *>(child.ptr());
      if (node)
        return (int)node->path().front();
      return -1;
    }

    void RootTreeNodeImpl::move_node(TreeNodeRef node, bool before) {
      // noop
    }

    Glib::RefPtr<Gtk::TreeStore> TreeNodeImpl::model() {
      // _rowref.get_model() causes crashes in OEL6 because of a refcounting bug in
      // TreeRowReference that was only fixed in gtkmm 2.20
      return _treeview->tree_store(); // Glib::RefPtr<Gtk::TreeStore>::cast_dynamic(_rowref.get_model());
    }

    Gtk::TreeIter TreeNodeImpl::iter() {
      return model()->get_iter(_rowref.get_path());
    }

    inline Gtk::TreeIter TreeNodeImpl::iter() const {
      TreeNodeImpl *non_const_this = const_cast<TreeNodeImpl *>(this);

      return non_const_this->iter();
    }

    inline Gtk::TreePath TreeNodeImpl::path() {
      return _rowref.get_path();
    }

    bool TreeNodeImpl::is_root() const {
      return false;
    }

    TreeNodeImpl::TreeNodeImpl(TreeViewImpl *tree, Glib::RefPtr<Gtk::TreeStore> model, const Gtk::TreePath &path)
      : RootTreeNodeImpl(tree), _rowref(model, path) {
    }

    TreeNodeImpl::TreeNodeImpl(TreeViewImpl *tree, const Gtk::TreeRowReference &ref)
      : RootTreeNodeImpl(tree), _rowref(ref) {
    }

    bool TreeNodeImpl::equals(const TreeNode &other) {
      const TreeNodeImpl *impl = dynamic_cast<const TreeNodeImpl *>(&other);
      if (impl)
        return impl->_rowref == _rowref;
      return false;
    }

    bool TreeNodeImpl::is_valid() const {
      return _treeview && _rowref.is_valid();
    }

    void TreeNodeImpl::invalidate() {
      if (_treeview) {
        std::map<std::string, Gtk::TreeRowReference>::iterator it = _treeview->_tagmap.find(get_tag());
        if (it != _treeview->_tagmap.end())
          _treeview->_tagmap.erase(it);
      }

      _treeview = 0;
      _rowref = Gtk::TreeRowReference();
    }

    int TreeNodeImpl::count() const {
      if (is_valid()) {
        // Glib::RefPtr<Gtk::TreeStore> store(model());
        Gtk::TreeRow row = *iter();
        return row.children().size();
      }
      return 0;
    }

    Gtk::TreeIter TreeNodeImpl::create_child(int index) {
      Glib::RefPtr<Gtk::TreeStore> store(model());
      Gtk::TreeIter new_iter;

      if (index < 0)
        new_iter = store->append(iter()->children());
      else {
        Gtk::TreePath path;
        Gtk::TreeIter child_iter;
        path = _rowref.get_path();
        path.push_back(index);
        child_iter = store->get_iter(path);
        if (!child_iter)
          new_iter = store->append(iter()->children());
        else
          new_iter = store->insert(child_iter);
      }
      return new_iter;
    }

    void TreeNodeImpl::remove_from_parent() {
      if (is_valid()) {
        if (_treeview->_tagmap_enabled) {
          std::map<std::string, Gtk::TreeRowReference>::iterator it;
          if ((it = _treeview->_tagmap.find(get_tag())) != _treeview->_tagmap.end())
            _treeview->_tagmap.erase(it);
        }

        Glib::RefPtr<Gtk::TreeStore> store(model());
        store->erase(iter());
        invalidate();
      }
    }

    TreeNodeRef TreeNodeImpl::get_child(int index) const {
      if (is_valid()) {
        Gtk::TreeRow row = *iter();
        return ref_from_iter(row->children()[index]);
      }
      return TreeNodeRef();
    }

    TreeNodeRef TreeNodeImpl::get_parent() const {
      if (is_valid()) {
        Gtk::TreePath path = _rowref.get_path();
        if (path.empty() || !path.up() || path.empty())
          return _treeview->_root_node;
        return ref_from_path(path);
      }
      return TreeNodeRef();
    }

    void TreeNodeImpl::expand() {
      if (is_valid() && !is_expanded()) {
        if (!_treeview->tree_view()->expand_row(
              _rowref.get_path(), false)) // if somehow we got null, then we need to call expand_toggle ourselves
        {                                 // cause it will not emmit the test-will-expand signal that should trigger
          TreeView *view = _treeview->get_owner(); // expand_toggle call
          if (view)
            view->expand_toggle(mforms::TreeNodeRef(this), true);
        }
      }
    }

    bool TreeNodeImpl::can_expand() {
      if (is_valid()) {
        Gtk::TreeRow row = *iter();
        return row->children().size() > 0;
      }
      return false;
    }

    void TreeNodeImpl::collapse() {
      if (is_valid())
        _treeview->tree_view()->collapse_row(_rowref.get_path());
    }

    bool TreeNodeImpl::is_expanded() {
      if (is_valid())
        return _treeview->tree_view()->row_expanded(_rowref.get_path());
      return false;
    }

    void TreeNodeImpl::set_attributes(int column, const TreeNodeTextAttributes &attrs) {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        Pango::AttrList attrlist;

        if (attrs.bold) {
          Pango::Attribute a = Pango::Attribute::create_attr_weight(Pango::WEIGHT_BOLD);
          attrlist.insert(a);
        }
        if (attrs.italic) {
          Pango::Attribute a = Pango::Attribute::create_attr_style(Pango::STYLE_ITALIC);
          attrlist.insert(a);
        }
        if (attrs.color.is_valid()) {
          Pango::Attribute a = Pango::Attribute::create_attr_foreground((guint16)(attrs.color.red * 0xffff),
                                                                        (guint16)(attrs.color.green * 0xffff),
                                                                        (guint16)(attrs.color.blue * 0xffff));
          attrlist.insert(a);
        }
        int i = _treeview->index_for_column_attr(column);
        if (i < 0)
          g_warning("TreeNode::set_attributes() called on a column with no attributes supported");
        else
          row.set_value(i, attrlist);
      }
    }

    void TreeNodeImpl::set_icon_path(int column, const std::string &icon) {
      Gtk::TreeRow row = *iter();
      if (!icon.empty()) {
        Glib::RefPtr<Gdk::Pixbuf> pixbuf = UtilitiesImpl::get_cached_icon(icon);
        if (pixbuf)
          row.set_value(_treeview->index_for_column(column) - 1, pixbuf);
      } else
        row.set_value(_treeview->index_for_column(column) - 1, Glib::RefPtr<Gdk::Pixbuf>());
    }

    void TreeNodeImpl::set_string(int column, const std::string &value) {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        int i = _treeview->index_for_column(column);
        switch (_treeview->_tree_store->get_column_type(i)) {
          case G_TYPE_BOOLEAN:
            row.set_value(i, value != "0" ? true : false);
            break;
          case G_TYPE_INT:
            row.set_value(i, base::atoi<int>(value, 0));
            break;
          case G_TYPE_INT64:
            row.set_value(i, base::atoi<long long>(value, 0LL));
            break;
          default: {
            // std::string nvalue(value);
            // base::replace(nvalue, "&", "&amp;");
            // base::replace(nvalue, "<", "&lt;");
            // base::replace(nvalue, ">", "&gt;");
            row.set_value(i, value);
            break;
          }
        }
      }
    }

    void TreeNodeImpl::set_int(int column, int value) {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        int i = _treeview->index_for_column(column);
        switch (_treeview->_tree_store->get_column_type(i)) {
          case G_TYPE_BOOLEAN:
            row.set_value(i, value != 0);
            break;
          default:
            row.set_value(i, value);
            break;
        }
      }
    }

    void TreeNodeImpl::set_long(int column, std::int64_t value) {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        row.set_value(_treeview->index_for_column(column), value);
      }
    }

    void TreeNodeImpl::set_bool(int column, bool value) {
      if (is_valid() && !is_root()) {
        set_int(column, value);
      }
    }

    void TreeNodeImpl::set_float(int column, double value) {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        row.set_value(_treeview->index_for_column(column), value);
      }
    }

    std::string TreeNodeImpl::get_string(int column) const {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        std::string value;
        row.get_value(_treeview->index_for_column(column), value);
        return value;
      }
      return "";
    }

    int TreeNodeImpl::get_int(int column) const {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        int i = _treeview->index_for_column(column);
        int value;
        bool bvalue;
        switch (_treeview->_tree_store->get_column_type(i)) {
          case G_TYPE_BOOLEAN:
            row.get_value(i, bvalue);
            value = bvalue ? 1 : 0;
            break;
          default:
            row.get_value(i, value);
            break;
        }
        return value;
      }
      return 0;
    }

    std::int64_t TreeNodeImpl::get_long(int column) const {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        std::int64_t value;
        row.get_value(_treeview->index_for_column(column), value);
        return value;
      }
      return 0;
    }

    bool TreeNodeImpl::get_bool(int column) const {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        bool value;
        row.get_value(_treeview->index_for_column(column), value);
        return value;
      }
      return false;
    }

    double TreeNodeImpl::get_float(int column) const {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        double value;
        row.get_value(_treeview->index_for_column(column), value);
        return value;
      }
      return 0.0;
    }

    void TreeNodeImpl::set_tag(const std::string &tag) {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        std::string old_tag = row[_treeview->_columns.tag_column()];

        if (!old_tag.empty()) {
          std::map<std::string, Gtk::TreeRowReference>::iterator it = _treeview->_tagmap.find(old_tag);
          if (it != _treeview->_tagmap.end())
            _treeview->_tagmap.erase(it);
        }

        row[_treeview->_columns.tag_column()] = tag;

        if (tag.empty()) {
          std::map<std::string, Gtk::TreeRowReference>::iterator it = _treeview->_tagmap.find(tag);
          if (it != _treeview->_tagmap.end())
            _treeview->_tagmap.erase(it);
        } else
          _treeview->_tagmap[tag] = _rowref;
      }
    }

    std::string TreeNodeImpl::get_tag() const {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        std::string tag = row[_treeview->_columns.tag_column()];
        return tag;
      }
      return "";
    }

    void TreeNodeImpl::set_data(TreeNodeData *data) {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        row[_treeview->_columns.data_column()] = TreeNodeDataRef(data);
      }
    }

    TreeNodeData *TreeNodeImpl::get_data() const {
      if (is_valid() && !is_root()) {
        Gtk::TreeRow row = *iter();
        TreeNodeDataRef data = row[_treeview->_columns.data_column()];
        return data._data;
      }
      return NULL;
    }

    int TreeNodeImpl::level() const {
      if (is_root())
        return 0;
      return _treeview->tree_store()->iter_depth(*iter()) + 1;
    }

    TreeNodeRef TreeNodeImpl::next_sibling() const {
      if (is_root())
        return TreeNodeRef();

      Gtk::TreePath path = _rowref.get_path();
      path.next();
      Gtk::TreeIter iter = _treeview->tree_store()->get_iter(path);
      if (iter)
        return ref_from_path(path);
      return TreeNodeRef();
    }

    TreeNodeRef TreeNodeImpl::previous_sibling() const {
      if (is_root())
        return TreeNodeRef();

      Gtk::TreePath path = _rowref.get_path();
      if (!path.prev())
        return TreeNodeRef();

      return ref_from_path(path);
    }

    int TreeNodeImpl::get_child_index(TreeNodeRef child) const {
      TreeNodeImpl *node = dynamic_cast<TreeNodeImpl *>(child.ptr());
      if (node) {
        if (_rowref.get_path().is_ancestor(node->path())) {
          int lvl = level();
          if (lvl <= (int)node->path().size())
            return node->path()[lvl];
        }
      }
      return -1;
    }

    void TreeNodeImpl::move_node(TreeNodeRef node, bool before) {
      TreeNodeImpl *location = dynamic_cast<TreeNodeImpl *>(node.ptr());
      if (location) {
        Glib::RefPtr<CustomTreeStore> store = Glib::RefPtr<CustomTreeStore>::cast_dynamic(_treeview->tree_store());

        Gtk::TreeIter to = store->get_iter(location->path());
        if (before)
          to = store->insert(to);
        else
          to = store->insert_after(to);

        TreeNodeRef ref = ref_from_iter(to);
        TreeNodeImpl *new_loc = dynamic_cast<TreeNodeImpl *>(ref.ptr());
        if (new_loc) {
          new_loc->duplicate_node(this);
          this->remove_from_parent();
          _rowref = Gtk::TreeRowReference(new_loc->model(), new_loc->model()->get_path(new_loc->iter()));
        }
      }
    }

    static void copy_row(Gtk::TreeIter iter, Gtk::TreeIter newiter, Glib::RefPtr<CustomTreeStore> store,
                         const bool &tagmap_enabled, const Gtk::TreeModelColumn<std::string> &tag_col,
                         std::map<std::string, Gtk::TreeRowReference> &tag_map) {
      Gtk::TreeIter previter;
      while (iter) {
        if (previter && newiter.equal(previter))
          newiter = store->insert_after(newiter);

        store->copy_iter(iter, newiter);
        // we need to move the data if we're using tagmap
        if (tagmap_enabled) {
          Gtk::TreeRow from_row = *iter;
          std::string old_tag = from_row[tag_col];
          if (!old_tag.empty()) {
            tag_map[old_tag] = Gtk::TreeRowReference(store, store->get_path(newiter));
            from_row[tag_col] = "";
          }
        }

        previter = newiter;

        if (!iter->children().empty())
          copy_row(iter->children().begin(), store->append(newiter->children()), store, tagmap_enabled, tag_col,
                   tag_map);

        iter++;
      }
    }

    Gtk::TreeIter TreeNodeImpl::duplicate_node(TreeNodeRef oldnode) {
      TreeNodeImpl *oldnodeimpl = dynamic_cast<TreeNodeImpl *>(oldnode.ptr());
      if (oldnodeimpl) {
        Glib::RefPtr<CustomTreeStore> store = Glib::RefPtr<CustomTreeStore>::cast_dynamic(_treeview->tree_store());
        Gtk::TreeIter from = store->get_iter(oldnodeimpl->path());
        Gtk::TreeIter to = store->get_iter(_rowref.get_path());
        store->copy_iter(from, to);

        // we need to move the data if we're using tagmap
        if (_treeview->_tagmap_enabled) {
          Gtk::TreeRow from_row = *from;
          std::string old_tag = from_row[_treeview->_columns.tag_column()];
          if (!old_tag.empty()) {
            from_row[_treeview->_columns.tag_column()] = "";
            _treeview->_tagmap[old_tag] = Gtk::TreeRowReference(store, store->get_path(to));
            from_row[_treeview->_columns.tag_column()] = "";
          }
        }

        if (!from->children().empty())
          copy_row(from->children().begin(), store->append(to->children()), store, _treeview->_tagmap_enabled,
                   _treeview->_columns.tag_column(), _treeview->_tagmap);

        return to;
      }

      return Gtk::TreeIter();
    }

    inline TreeNodeRef RootTreeNodeImpl::ref_from_iter(const Gtk::TreeIter &iter) const {
      Gtk::TreePath path(iter);
      return TreeNodeRef(new TreeNodeImpl(_treeview, _treeview->tree_store(), path));
    }

    inline TreeNodeRef RootTreeNodeImpl::ref_from_path(const Gtk::TreePath &path) const {
      return TreeNodeRef(new TreeNodeImpl(_treeview, _treeview->tree_store(), path));
    }

    //---------------------------------------------------------------------------------------

    TreeViewImpl::ColumnRecord::~ColumnRecord() {
      for (std::vector<Gtk::TreeModelColumnBase *>::iterator iter = columns.begin(); iter != columns.end(); ++iter)
        delete *iter;
    }

    void TreeViewImpl::ColumnRecord::add_tag_column() {
      add(_tag_column);
    }

    void TreeViewImpl::ColumnRecord::add_data_column() {
      add(_data_column);
    }

    Gtk::TreeModelColumn<std::string> &TreeViewImpl::ColumnRecord::tag_column() {
      return _tag_column;
    }

    Gtk::TreeModelColumn<TreeNodeDataRef> &TreeViewImpl::ColumnRecord::data_column() {
      return _data_column;
    }

    template <typename T>
    std::pair<Gtk::TreeViewColumn *, int> TreeViewImpl::ColumnRecord::create_column(Gtk::TreeView *tree,
                                                                                    const std::string &title,
                                                                                    bool editable, bool attr,
                                                                                    bool with_icon, bool align_right) {
      Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > *icon = 0;
      std::string tmp = title;
      base::replaceStringInplace(tmp, "_", "__");
      Gtk::TreeViewColumn *column = Gtk::manage(new Gtk::TreeViewColumn(tmp));
      int idx;

      if (with_icon) {
        Gtk::CellRendererPixbuf *cell = Gtk::manage(new Gtk::CellRendererPixbuf());
        icon = add_model_column<Glib::RefPtr<Gdk::Pixbuf> >();
        column->pack_start(*cell, false);
        column->add_attribute(cell->property_pixbuf(), *icon);
      }

      Gtk::TreeModelColumn<T> *col = add_model_column<T>();
      column_value_index.push_back(size() - 1);

      Gtk::CellRendererText *cell = Gtk::manage(new Gtk::CellRendererText());
      if (align_right)
        cell->set_alignment(1.0, 0.5); // always align right, it's number!

      column->pack_start(*cell);
      column->add_attribute(cell->property_text(), *col);
      if (attr) {
        Gtk::TreeModelColumn<Pango::AttrList> *acol = add_model_column<Pango::AttrList>();
        column_attr_index.push_back(size() - 1);
        column->add_attribute(cell->property_attributes(), *acol);
      } else
        column_attr_index.push_back(-1);

      cell->property_editable() = editable;

      if (editable)
        cell->signal_editing_started().connect(sigc::mem_fun(this, &ColumnRecord::on_cell_editing_started));

      idx = tree->append_column(*column);
      tree->get_column(idx - 1)->set_resizable(true);
      return std::make_pair(column, idx - 1);
    }

    template std::pair<Gtk::TreeViewColumn *, int> TreeViewImpl::ColumnRecord::create_column<Glib::ustring>(
      Gtk::TreeView *tree, const std::string &title, bool editable, bool attr, bool with_icon,
      bool align_right = false);
    template std::pair<Gtk::TreeViewColumn *, int> TreeViewImpl::ColumnRecord::create_column<int>(
      Gtk::TreeView *tree, const std::string &title, bool editable, bool attr, bool with_icon, bool align_right = true);
    template std::pair<Gtk::TreeViewColumn *, int> TreeViewImpl::ColumnRecord::create_column<std::int64_t>(
      Gtk::TreeView *tree, const std::string &title, bool editable, bool attr, bool with_icon, bool align_right = true);
    template std::pair<Gtk::TreeViewColumn *, int> TreeViewImpl::ColumnRecord::create_column<double>(
      Gtk::TreeView *tree, const std::string &title, bool editable, bool attr, bool with_icon, bool align_right = true);

    int TreeViewImpl::ColumnRecord::add_integer(Gtk::TreeView *tree, const std::string &title, bool editable,
                                                bool attr) {
      std::pair<Gtk::TreeViewColumn *, int> ret = create_column<int>(tree, title, editable, attr, false, true);
      return ret.second;
    }

    int TreeViewImpl::ColumnRecord::add_string(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr,
                                               bool with_icon, bool align_right) {
      std::pair<Gtk::TreeViewColumn *, int> ret =
        create_column<Glib::ustring>(tree, title, editable, attr, with_icon, align_right);
      return ret.second;
    }

    int TreeViewImpl::ColumnRecord::add_long_integer(Gtk::TreeView *tree, const std::string &title, bool editable,
                                                     bool attr) {
      std::pair<Gtk::TreeViewColumn *, int> ret = create_column<std::int64_t>(tree, title, editable, attr, false, true);
      return ret.second;
    }

    int TreeViewImpl::ColumnRecord::add_float(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr) {
      std::pair<Gtk::TreeViewColumn *, int> ret = create_column<double>(tree, title, editable, attr, false, true);
      return ret.second;
    }

    int TreeViewImpl::ColumnRecord::add_check(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr) {
      Gtk::TreeModelColumn<bool> *column = add_model_column<bool>();
      int idx;
      column_value_index.push_back(size() - 1);

      if (editable)
        idx = tree->append_column_editable(title, *column);
      else
        idx = tree->append_column(title, *column);
      if (attr) {
      } else
        column_attr_index.push_back(-1);

      return idx - 1;
    }

    void TreeViewImpl::ColumnRecord::format_tri_check(Gtk::CellRenderer *cell, const Gtk::TreeIter &iter,
                                                      const Gtk::TreeModelColumn<int> &column) {
      Gtk::CellRendererToggle *toggle = (Gtk::CellRendererToggle *)cell;
      if (toggle) {
        int val = iter->get_value(column);
        if (val == -1) {
          toggle->set_property("inconsistent", true);
          toggle->set_active(true);
        } else {
          toggle->set_property("inconsistent", false);
          toggle->set_active(val != 0);
        }
      }
    }

    int TreeViewImpl::ColumnRecord::add_tri_check(Gtk::TreeView *tree, const std::string &title, bool editable,
                                                  bool attr) {
      std::string tmp = title;
      base::replaceStringInplace(tmp, "_", "__");
      Gtk::TreeViewColumn *column = Gtk::manage(new Gtk::TreeViewColumn(tmp));

      Gtk::TreeModelColumn<int> *col = add_model_column<int>();
      int idx;
      column_value_index.push_back(size() - 1);

      Gtk::CellRendererToggle *cell = Gtk::manage(new Gtk::CellRendererToggle());

      column->pack_start(*cell);

      if (!attr) {
        column_attr_index.push_back(-1);
      }

      column->set_cell_data_func(*cell,
                                 sigc::bind(sigc::mem_fun(this, &TreeViewImpl::ColumnRecord::format_tri_check), *col));

      idx = tree->append_column(*column);

      return idx - 1;
    }

    void TreeViewImpl::ColumnRecord::on_cell_editing_started(Gtk::CellEditable *e, const Glib::ustring &path) {
      Gtk::Widget *w = dynamic_cast<Gtk::Widget *>(e);
      if (w)
        w->signal_focus_out_event().connect(
          sigc::bind(sigc::mem_fun(this, &ColumnRecord::on_focus_out), dynamic_cast<Gtk::Entry *>(e)), false);
    }

    bool TreeViewImpl::ColumnRecord::on_focus_out(GdkEventFocus *event, Gtk::Entry *e) {
      // Emulate pressing Enter on the text entry so that a focus out will save ongoing changes
      // instead of discarding them
      if (!event->in)
        e->activate();
      return false;
    }

    //---------------------------------------------------------------------------------------

    TreeViewImpl::TreeViewImpl(TreeView *self, mforms::TreeOptions opts) : ViewImpl(self), _row_height(-1) {
      _mouse_inside = false;
      _hovering_overlay = -1;
      _clicking_overlay = -1;

      _drag_source_enabled = (opts & mforms::TreeCanBeDragSource) != 0;
      _drag_in_progress = false;
      _drag_button = 0;
      _flat_list = (opts & mforms::TreeFlatList) != 0;
      _tagmap_enabled = (opts & mforms::TreeIndexOnTag) != 0;

      _swin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
      if (opts & mforms::TreeNoBorder)
        _swin.set_shadow_type(Gtk::SHADOW_NONE);
      else
        _swin.set_shadow_type(Gtk::SHADOW_IN);

      _conn = _tree.get_selection()->signal_changed().connect(sigc::mem_fun(self, &TreeView::changed));
      _tree.signal_row_activated().connect(sigc::mem_fun(this, &TreeViewImpl::on_activated));
      _tree.signal_row_collapsed().connect(sigc::mem_fun(this, &TreeViewImpl::on_collapsed));
      //  _tree.signal_row_expanded().connect(sigc::mem_fun(this, &TreeViewImpl::on_expanded));
      _tree.signal_test_expand_row().connect(
        sigc::bind_return(sigc::mem_fun(this, &TreeViewImpl::on_will_expand), false));
      _tree.signal_key_release_event().connect(sigc::mem_fun(this, &TreeViewImpl::on_key_release), false);
      _tree.signal_button_press_event().connect(sigc::mem_fun(this, &TreeViewImpl::on_button_event), false);
      //  _tree.set_reorderable((opts & mforms::TreeAllowReorderRows) || (opts & mforms::TreeCanBeDragSource)); // we
      //  need this to have D&D working
      _tree.signal_button_release_event().connect(sigc::mem_fun(this, &TreeViewImpl::on_button_release), false);
      _tree.signal_draw().connect(sigc::mem_fun(this, &TreeViewImpl::on_draw_event), true);
      //  _tree.signal_expose_event().connect(sigc::mem_fun(this, &TreeViewImpl::on_expose_event), true);
      _tree.signal_realize().connect(sigc::mem_fun(this, &TreeViewImpl::on_realize));
      _tree.signal_motion_notify_event().connect(sigc::mem_fun(this, &TreeViewImpl::on_motion_notify), false);
      _tree.signal_enter_notify_event().connect(sigc::mem_fun(this, &TreeViewImpl::on_enter_notify), false);
      _tree.signal_leave_notify_event().connect(sigc::mem_fun(this, &TreeViewImpl::on_leave_notify), false);

      _is_drag_source = false;
      if (opts & mforms::TreeCanBeDragSource) {
        _is_drag_source = true;
        Gtk::Widget *w = this->get_outer();
        if (w) {
          // connect signals to apropriate methods that are defined in ViewImpl
          w->signal_drag_data_get().connect(sigc::mem_fun(this, &TreeViewImpl::slot_drag_data_get));
          w->signal_drag_begin().connect(sigc::mem_fun(this, &TreeViewImpl::slot_drag_begin));
          w->signal_drag_end().connect(sigc::mem_fun(this, &TreeViewImpl::slot_drag_end));
          w->signal_drag_failed().connect(sigc::mem_fun(this, &TreeViewImpl::slot_drag_failed));
        }
        _tree.add_events(Gdk::POINTER_MOTION_MASK);
      }

      _swin.add(_tree);
      _swin.show_all();
      _tree.set_headers_visible((opts & mforms::TreeNoHeader) == 0);
    }

    TreeViewImpl::~TreeViewImpl() {
    }

    void TreeViewImpl::slot_drag_end(const Glib::RefPtr<Gdk::DragContext> &context) {
      ViewImpl::slot_drag_end(context);
      _drag_in_progress = false;
      _drag_button = 0;
    }

    bool TreeViewImpl::slot_drag_failed(const Glib::RefPtr<Gdk::DragContext> &context, Gtk::DragResult result) {
      bool ret_val = ViewImpl::slot_drag_failed(context, result);
      _drag_in_progress = false;
      _drag_button = 0;
      return ret_val;
    }

    bool TreeViewImpl::on_draw_event(const ::Cairo::RefPtr< ::Cairo::Context> &context) {
      if (!_overlay_icons.empty() && !_overlayed_row.empty() && _mouse_inside) {
        Gdk::Rectangle rect;
        Gdk::Rectangle vrect;
        int i = 1;
        _tree.get_visible_rect(vrect);
        _tree.get_background_area(_overlayed_row, *_tree.get_column(_tree.get_columns().size() - 1), rect);

        int x = vrect.get_x() + vrect.get_width() - 4;
        for (std::vector<Cairo::RefPtr<Cairo::ImageSurface> >::const_iterator icon = _overlay_icons.begin();
             icon != _overlay_icons.end(); ++icon)
          x -= (*icon)->get_width() + 4;

        for (std::vector<Cairo::RefPtr<Cairo::ImageSurface> >::const_iterator icon = _overlay_icons.begin();
             icon != _overlay_icons.end(); ++icon, ++i) {
          if (*icon) {
            context->set_source(*icon, x, rect.get_y() + (rect.get_height() - (*icon)->get_height()) / 2);
            x += (*icon)->get_width() + 4;
            if (i - 1 == _hovering_overlay)
              context->paint();
            else
              context->paint_with_alpha(0.4);
          }
        }
      }

      return false;
    }

    bool TreeViewImpl::on_enter_notify(GdkEventCrossing *ev) {
      _mouse_inside = true;
      return false;
    }

    bool TreeViewImpl::on_leave_notify(GdkEventCrossing *ev) {
      if (_mouse_inside) {
        _mouse_inside = false;
        _overlay_icons.clear();
        _hovering_overlay = -1;
        _clicking_overlay = -1;
        _tree.queue_draw();
      }
      return false;
    }

    bool TreeViewImpl::on_motion_notify(GdkEventMotion *ev) {
      int dummy;
      Gtk::TreeViewColumn *column;
      Gtk::TreePath path;

      // handle overlay icons
      if (_clicking_overlay < 0) {
        _overlay_icons.clear();
        _hovering_overlay = -1;
        _overlayed_row.clear();
        _tree.queue_draw();
      }
      if (!_drag_in_progress && _tree.get_path_at_pos(ev->x, ev->y, path, column, dummy, dummy)) {
        mforms::TreeView *tv = dynamic_cast<mforms::TreeView *>(owner);
        mforms::TreeNodeRef node(new TreeNodeImpl(this, tree_store(), path));

        if (node) {
          std::vector<std::string> icons = tv->overlay_icons_for_node(node);
          if (!icons.empty()) {
            int icon_rect_x;
            Gdk::Rectangle rect;
            Gdk::Rectangle vrect;

            _overlayed_row = path;

            _tree.get_background_area(path, *column, rect);

            _tree.get_visible_rect(vrect);
            icon_rect_x = vrect.get_width() - 4;

            for (std::vector<std::string>::const_iterator icon = icons.begin(); icon != icons.end(); ++icon) {
              Cairo::RefPtr<Cairo::ImageSurface> surf;
              if (!icon->empty()) {
                surf = Cairo::ImageSurface::create_from_png(*icon);
                if (!surf)
                  g_warning("Could not load %s", icon->c_str());
              }
              _overlay_icons.push_back(surf);
              icon_rect_x -= surf->get_width() + 4;
            }

            for (std::vector<Cairo::RefPtr<Cairo::ImageSurface> >::const_iterator icon = _overlay_icons.begin();
                 icon != _overlay_icons.end(); ++icon) {
              if (ev->x - vrect.get_x() > icon_rect_x && ev->x - vrect.get_x() < icon_rect_x + (*icon)->get_width()) {
                _hovering_overlay = icon - _overlay_icons.begin();
                break;
              }
              icon_rect_x += (*icon)->get_width() + 4;
            }
            _tree.queue_draw();
          }
        }
      }

      // drag and drop
      if (_drag_in_progress || _drag_button == 0 || !ev || !_drag_source_enabled) {
        return false;
      }

      Gtk::Widget *w = this->get_outer();
      if (w) {
        if (w->drag_check_threshold(_drag_start_x, _drag_start_y, ev->x, ev->y)) {
          {
            // Because of problems when Treeview has been set to multiselect,
            // there are some DnD problems, below code is fixing those.
            // We check if users already selected something,
            // if he is trying to drag not selected row,
            // we change selection.
            std::vector<Gtk::TreePath> path_selection;
            path_selection = _tree.get_selection()->get_selected_rows();

            Gtk::TreeModel::Path selpath;
            if (_tree.get_path_at_pos(_drag_start_x, _drag_start_y, selpath)) {
              bool found = false;
              for (std::vector<Gtk::TreePath>::iterator it = path_selection.begin(); it != path_selection.end(); ++it) {
                if (selpath == *it) {
                  found = true;
                  break;
                }
              }
              if (!found) {
                // Old selection is invalid, we need to create new one
                _tree.get_selection()->unselect_all();
                _tree.get_selection()->select(selpath);
              }
            }
          }

          TreeView *view = dynamic_cast<TreeView *>(owner);
          if (view) {
            mforms::DragDetails details;

            Gtk::TreeModel::Path path;
            if (_tree.get_path_at_pos(_drag_start_x, _drag_start_y, path)) {
              Cairo::RefPtr<Cairo::Surface> row_icon = _tree.create_row_drag_icon(path);
              details.image = row_icon->cobj();

              void *data = NULL;
              std::string text;
              std::string format;
              details.location = base::Point(ev->x, ev->y);
              details.allowedOperations = mforms::DragOperationCopy;
              if (!view->get_drag_data(details, &data, format)) {
                format = "STRING";
                std::list<mforms::TreeNodeRef> selection = view->get_selection();

                for (std::list<mforms::TreeNodeRef>::const_iterator iterator = selection.begin();
                     iterator != selection.end(); ++iterator) {
                  if (!(*iterator)->get_string(0).empty()) {
                    if (text.size() > 0)
                      text.append(", ");
                    text.append((*iterator)->get_string(0));
                  }
                }

                if (text.empty()) {
                  return false;
                }

                data = &text;
              }

              mforms::DragOperation operation = view->do_drag_drop(details, data, format);
              view->drag_finished(operation);
            }
          }
        }
      }
      return false;
    }

    bool TreeViewImpl::on_button_release(GdkEventButton *ev) {
      if (!_drag_in_progress && _hovering_overlay >= 0 && _hovering_overlay == _clicking_overlay) {
        mforms::TreeView *tv = dynamic_cast<mforms::TreeView *>(owner);
        mforms::TreeNodeRef node(new TreeNodeImpl(this, tree_store(), _overlayed_row));
        if (node)
          tv->overlay_icon_for_node_clicked(node, _clicking_overlay);
      }
      _clicking_overlay = -1;

      if (_drag_in_progress) {
        return false;
      }

      //  _tree.
      _drag_button = 0;
      return false;
    }

    TreeView *TreeViewImpl::get_owner() {
      TreeView *view = dynamic_cast<TreeView *>(owner);
      if (view)
        return view;
      return NULL;
    }

    void TreeViewImpl::set_back_color(const std::string &color) {
      if (!force_sys_colors) {
        if (!color.empty()) {
          Gdk::RGBA gtk_color(color);
          Gdk::RGBA gtk_selected_color;
          
          gtk_selected_color.set_rgba(gtk_color.get_red() / 2, gtk_color.get_green() / 2, gtk_color.get_blue() / 2);
          
          _tree.override_background_color(gtk_color, Gtk::STATE_FLAG_NORMAL);
          _tree.override_background_color(gtk_selected_color, Gtk::STATE_FLAG_SELECTED);
        }
      }
    }

    void TreeViewImpl::string_edited(const Glib::ustring &path, const Glib::ustring &new_text, int column) {
      if (_tree_store) {
        Gtk::TreePath tree_path = to_list_path(Gtk::TreePath(path));
        Gtk::TreeRow row = *_tree_store->get_iter(tree_path);
        if (dynamic_cast<TreeView *>(owner)->cell_edited(TreeNodeRef(new TreeNodeImpl(this, _tree_store, tree_path)),
                                                         column, new_text))
          row[_columns.get<Glib::ustring>(column)] = new_text;
      }
    }

    void TreeViewImpl::toggle_edited(const Glib::ustring &path, int column) {
      if (_tree_store) {
        Gtk::TreePath tree_path = to_list_path(Gtk::TreePath(path));

        TreeNodeRef node(new TreeNodeImpl(this, _tree_store, tree_path));
        // Because of TriCheckColumnType we need to use int instead of bool
        int value = node->get_int(column) == 0 ? 1 : 0;
        std::stringstream ss;
        ss << value;

        if (dynamic_cast<TreeView *>(owner)->cell_edited(node, column, ss.str()))
          node->set_int(column, value);
      }
    }

    void TreeViewImpl::on_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column) {
      mforms::TreeView *tv = dynamic_cast<mforms::TreeView *>(
        owner); // owner is from deeply hidden class TreeViewImpl->ViewImpl->ObjectImpl.owner
      if (tv) {
        Gtk::TreePath tree_path = to_list_path(path);
        tv->node_activated(mforms::TreeNodeRef(new TreeNodeImpl(this, _tree_store, tree_path)),
                           (intptr_t)column->get_data("index"));
      }
    }

    void TreeViewImpl::on_will_expand(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path) {
      mforms::TreeView *tv = dynamic_cast<mforms::TreeView *>(owner);
      if (tv) {
        Gtk::TreePath tree_path = to_list_path(path);
        tv->expand_toggle(mforms::TreeNodeRef(new TreeNodeImpl(this, _tree_store, tree_path)), true);
      }
    }

    void TreeViewImpl::on_collapsed(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path) {
      mforms::TreeView *tv = dynamic_cast<mforms::TreeView *>(owner);
      if (tv) {
        Gtk::TreePath tree_path = to_list_path(path);
        tv->expand_toggle(mforms::TreeNodeRef(new TreeNodeImpl(this, _tree_store, tree_path)), false);
      }
    }

    bool TreeViewImpl::on_key_release(GdkEventKey *ev) {
      mforms::TreeView *tv = dynamic_cast<mforms::TreeView *>(owner);
      TreeNodeRef node = this->get_selected_node(tv);
      if (ev->keyval == GDK_KEY_Menu) {
        mforms::TreeView *tv = dynamic_cast<mforms::TreeView *>(owner);
        if (tv) {
          tv->get_context_menu()->popup_at(mforms::gtk::ViewImpl::get_view_for_widget(this->get_outer()),
                                           base::Point(0, 0)); // gtk will handle position automagically
          return false;
        }
      }

      if (!node.is_valid())
        return false;

      if (ev->keyval == GDK_KEY_Left)
        node->collapse();
      else if (ev->keyval == GDK_KEY_Right)
        node->expand();

      return false;
    }

    bool TreeViewImpl::on_button_event(GdkEventButton *event) {
      bool ret_val = false;

      if (event->button == 1 && _drag_button == 0 && _hovering_overlay >= 0) {
        _clicking_overlay = _hovering_overlay;
      }

      if (event->button == 3) {
        mforms::TreeView *tv = dynamic_cast<mforms::TreeView *>(owner);
        if (tv->get_context_menu())
          tv->get_context_menu()->popup_at(mforms::gtk::ViewImpl::get_view_for_widget(this->get_outer()),
                                           base::Point(event->x, event->y));

        std::list<TreeNodeRef> selected_nodes = this->get_selection(tv);

        // For multiple selection will work with this handle
        // In other case will let the default handle

        // TODO: Add validation for when the right click is done on a node
        //      that is not part of the selected group, in such case the
        //      return value should be false to clear the selection and
        //      select that node
        if (selected_nodes.size() > 1)
          ret_val = true;
      } else if (event->button == 1 && _drag_button == 0) {
        Gtk::TreeModel::Path path;
        Gtk::TreeViewDropPosition pos;
        if (_tree.get_dest_row_at_pos(event->x, event->y, path, pos) && _is_drag_source) {
          _drag_button = event->button;
          _drag_start_x = event->x;
          _drag_start_y = event->y;
        }
      }

      return ret_val;
    }

    bool TreeViewImpl::on_header_button_event(GdkEventButton *event, int column) {
      if (event->button == 3) {
        mforms::TreeView *tv = dynamic_cast<mforms::TreeView *>(owner);

        tv->header_clicked(column);

        if (tv->get_header_menu())
          tv->get_header_menu()->popup_at(mforms::gtk::ViewImpl::get_view_for_widget(this->get_inner()),
                                          base::Point(event->x, event->y));
      }
      return false;
    }

    int TreeViewImpl::add_column(TreeColumnType type, const std::string &name, int initial_width, bool editable,
                                 bool attributed) {
      int column = -1;
      switch (type) {
        case IconColumnType:
          column = _columns.add_string(&_tree, name, editable, attributed, true);
          if (editable) {
            std::vector<Gtk::CellRenderer *> rends = _tree.get_column(column)->get_cells();
            ((Gtk::CellRendererText *)rends[1])
              ->signal_edited()
              .connect(sigc::bind(sigc::mem_fun(this, &TreeViewImpl::string_edited), column));
          }
          break;
        case StringColumnType:
        case StringLTColumnType:
          column = _columns.add_string(&_tree, name, editable, attributed, false);
          if (editable) {
            Gtk::CellRendererText *rend = ((Gtk::CellRendererText *)*_tree.get_column(column)->get_cells().begin());
            rend->signal_edited().connect(sigc::bind(sigc::mem_fun(this, &TreeViewImpl::string_edited), column));
            if (type == StringLTColumnType)
              rend->property_ellipsize() = Pango::ELLIPSIZE_START;
            else
              rend->property_ellipsize() = Pango::ELLIPSIZE_END;
          }
          break;
        case IntegerColumnType:
          column = _columns.add_integer(&_tree, name, editable, attributed);
          if (editable) {
            ((Gtk::CellRendererText *)*_tree.get_column(column)->get_cells().begin())
              ->signal_edited()
              .connect(sigc::bind(sigc::mem_fun(this, &TreeViewImpl::string_edited), column));
          }
          break;
        case LongIntegerColumnType:
          column = _columns.add_long_integer(&_tree, name, editable, attributed);
          if (editable) {
            ((Gtk::CellRendererText *)*_tree.get_column(column)->get_cells().begin())
              ->signal_edited()
              .connect(sigc::bind(sigc::mem_fun(this, &TreeViewImpl::string_edited), column));
          }
          break;
        case FloatColumnType:
          column = _columns.add_float(&_tree, name, editable, attributed);
          if (editable) {
            ((Gtk::CellRendererText *)*_tree.get_column(column)->get_cells().begin())
              ->signal_edited()
              .connect(sigc::bind(sigc::mem_fun(this, &TreeViewImpl::string_edited), column));
          }
          break;
        case CheckColumnType:
          column = _columns.add_check(&_tree, name, editable, attributed);
          if (editable) {
            ((Gtk::CellRendererToggle *)*_tree.get_column(column)->get_cells().begin())
              ->signal_toggled()
              .connect(sigc::bind(sigc::mem_fun(this, &TreeViewImpl::toggle_edited), column));
          }
          break;
        case NumberWithUnitColumnType:
          column = _columns.add_string(&_tree, name, editable, attributed, false, true);
          if (editable) {
            ((Gtk::CellRendererText *)*_tree.get_column(column)->get_cells().begin())
              ->signal_edited()
              .connect(sigc::bind(sigc::mem_fun(this, &TreeViewImpl::string_edited), column));
          }
          break;
        case TriCheckColumnType:
          column = _columns.add_tri_check(&_tree, name, editable, attributed);
          if (editable) {
            ((Gtk::CellRendererToggle *)*_tree.get_column(column)->get_cells().begin())
              ->signal_toggled()
              .connect(sigc::bind(sigc::mem_fun(this, &TreeViewImpl::toggle_edited), column));
          }
          break;
      }

      Gtk::TreeViewColumn *tvc = _tree.get_column(column);
      {
        Gtk::Label *label = Gtk::manage(new Gtk::Label(name));
        label->show();
        tvc->set_widget(*label);
      }
      if (initial_width > 0)
        tvc->set_fixed_width(initial_width);
      tvc->set_resizable(true);
      tvc->set_data("index", (void *)(intptr_t)column);

      return column;
    }

    void TreeViewImpl::end_columns() {
      _columns.add_tag_column();
      _columns.add_data_column();

      _tree_store = CustomTreeStore::create(_columns);
      _tree.set_model(_tree_store);

      _root_node = TreeNodeRef(new RootTreeNodeImpl(this));

      // enable sorting if previously enabled
      if (_tree.get_headers_clickable())
        set_allow_sorting(true);
    }

    bool TreeViewImpl::create(TreeView *self, mforms::TreeOptions opt) {
      return new TreeViewImpl(self, opt) != 0;
    }

    int TreeViewImpl::add_column(TreeView *self, TreeColumnType type, const std::string &name, int width, bool editable,
                                 bool attr) {
      TreeViewImpl *tree = self->get_data<TreeViewImpl>();

      return tree->add_column(type, name, width, editable, attr);
    }

    void TreeViewImpl::end_columns(TreeView *self) {
      TreeViewImpl *tree = self->get_data<TreeViewImpl>();

      tree->end_columns();
    }

    void TreeViewImpl::clear(TreeView *self) {
      TreeViewImpl *tree = self->get_data<TreeViewImpl>();

      if (tree->_tree_store)
        tree->_tree_store->clear();
    }

    TreeNodeRef TreeViewImpl::get_selected_node(TreeView *self) {
      TreeViewImpl *tree = self->get_data<TreeViewImpl>();

      if (tree->_tree.get_selection()->get_mode() == Gtk::SELECTION_MULTIPLE) {
        std::vector<Gtk::TreePath> path_selection = tree->_tree.get_selection()->get_selected_rows();
        if (path_selection.size() == 1)
          return TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path_selection[0]));
        else if (!path_selection.empty()) {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *column;
          tree->_tree.get_cursor(path, column);
          if (!path.empty())
            return TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path));
          return TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path_selection[0]));
        }
      } else if (tree->_tree.get_selection()->get_selected()) {
        const Gtk::TreePath path(tree->to_list_iter(tree->_tree.get_selection()->get_selected()));
        if (!path.empty())
          return TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path));
      }
      return TreeNodeRef();
    }

    std::list<TreeNodeRef> TreeViewImpl::get_selection(TreeView *self) {
      TreeViewImpl *tree = self->get_data<TreeViewImpl>();
      std::list<TreeNodeRef> selection;

      if (tree->_tree.get_selection()->get_mode() == Gtk::SELECTION_MULTIPLE) {
        std::vector<Gtk::TreePath> path_selection;
        path_selection = tree->_tree.get_selection()->get_selected_rows();

        size_t size = path_selection.size();

        if (size > 0) {
          for (size_t index = 0; index < size; index++) {
            auto path = path_selection[index];
            if (tree->_sort_model)
              path = tree->_sort_model->convert_path_to_child_path(path);

            selection.push_back(TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path)));
          }
        }
      } else {
        Gtk::TreePath path(tree->to_list_iter(tree->_tree.get_selection()->get_selected()));
        if (!path.empty()) {
          if (tree->_sort_model)
            path = tree->_sort_model->convert_path_to_child_path(path);
          selection.push_back(TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path)));
        }
      }
      return selection;
    }

    void TreeViewImpl::set_selected(TreeView *self, TreeNodeRef node, bool flag) {
      TreeViewImpl *tree = self->get_data<TreeViewImpl>();
      TreeNodeImpl *nodei = dynamic_cast<TreeNodeImpl *>(node.ptr());

      if (nodei) {
        tree->_conn.block();
        Gtk::TreePath path = nodei->path();
        path = tree->to_sort_path(path);
        if (flag)
          tree->_tree.get_selection()->select(path);
        else
          tree->_tree.get_selection()->unselect(path);
      }
      tree->_conn.unblock();
    }

    template <typename T>
    int column_value_compare(const T &val1, const T &val2) {
      int result = 0;
      if (val2 < val1)
        result = -1;
      else if (val2 > val1)
        result = 1;
      return result;
    }

    template <typename T>
    int column_numeric_compare(const Gtk::TreeModel::iterator &it1, const Gtk::TreeModel::iterator &it2,
                               Gtk::TreeModelColumn<T> *col) {
      return column_value_compare((*it1).get_value(*col), (*it2).get_value(*col));
    }

    int column_string_compare(const Gtk::TreeModel::iterator &it1, const Gtk::TreeModel::iterator &it2,
                              Gtk::TreeModelColumn<Glib::ustring> *col, int type) {
      int result = 0;

      switch (type) {
        case StringColumnType:
        case StringLTColumnType:
        case IconColumnType: {
          std::string val1 = (*it1).get_value(*col).c_str();
          std::string val2 = (*it2).get_value(*col).c_str();
          result = base::string_compare(val2, val1, false);
          break;
        }
        case IntegerColumnType:
        case LongIntegerColumnType: {
          std::istringstream ss_val1((*it1).get_value(*col).c_str());
          std::istringstream ss_val2((*it2).get_value(*col).c_str());
          int64_t val1 = 0;
          int64_t val2 = 0;
          ss_val1 >> val1;
          ss_val2 >> val2;
          result = column_value_compare(val1, val2);
          break;
        }
        case FloatColumnType:
        case NumberWithUnitColumnType: {
          double val1 = mforms::TreeView::parse_string_with_unit((*it1).get_value(*col).c_str());
          double val2 = mforms::TreeView::parse_string_with_unit((*it2).get_value(*col).c_str());
          result = column_value_compare(val1, val2);
          break;
        }
      }

      return result;
    }

    void TreeViewImpl::set_allow_sorting(TreeView *self, bool flag) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      impl->set_allow_sorting(flag);
    }

    void TreeViewImpl::set_allow_sorting(bool flag) {
      if (_tree.get_headers_visible())
        _tree.set_headers_clickable(flag);

      if (flag == false || !_tree_store)
        return;

      if (!_sort_model)
        _sort_model = Gtk::TreeModelSort::create(_tree_store);

      for (std::size_t i = 0; i < _tree.get_columns().size(); ++i) {
        Gtk::TreeViewColumn *col = _tree.get_column(i);
        Gtk::TreeModelColumnBase *mcol = _columns.columns[index_for_column(i)];

        if (col == NULL || mcol == NULL) {
          logWarning("Invalid column pointer[col:%s][mcol:%s]\n", col == NULL ? "NULL" : "NOT NULL",
                     mcol == NULL ? "NULL" : "NOT NULL");
          continue;
        }

        col->signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &TreeViewImpl::header_clicked), mcol, col));

        //  Align columns text depending on the visualization type
        float align = 0.0f;

        switch (get_owner()->get_column_type(i)) {
          case StringColumnType:
          case StringLTColumnType:
          case IconColumnType:
            align = 0.0f;
            break;
          case IntegerColumnType:
          case LongIntegerColumnType:
          case FloatColumnType:
          case NumberWithUnitColumnType:
            align = 1.0f;
            break;
          case CheckColumnType:
          case TriCheckColumnType:
            align = 0.5f;
            break;
        }

        Glib::ListHandle<Gtk::CellRenderer *> renderers = col->get_cells();

        for (Glib::ListHandle<Gtk::CellRenderer *>::const_iterator iter = renderers.begin(); iter != renderers.end();
             ++iter)
          (*iter)->set_alignment(align, 0);

        //  Set the proper compare method to the sorting functions depending on the storage type
        switch (mcol->type()) {
          case G_TYPE_INT:
            _sort_model->set_sort_func(
              *mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<int>), (Gtk::TreeModelColumn<int> *)mcol));
            break;
          case G_TYPE_LONG:
            _sort_model->set_sort_func(
              *mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<long>), (Gtk::TreeModelColumn<long> *)mcol));
            break;
          case G_TYPE_INT64:
            _sort_model->set_sort_func(
              *mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<int64_t>), (Gtk::TreeModelColumn<int64_t> *)mcol));
            break;
          case G_TYPE_UINT:
            _sort_model->set_sort_func(*mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<unsigned int>),
                                                         (Gtk::TreeModelColumn<unsigned int> *)mcol));
            break;
          case G_TYPE_ULONG:
            _sort_model->set_sort_func(*mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<unsigned long>),
                                                         (Gtk::TreeModelColumn<unsigned long> *)mcol));
            break;
          case G_TYPE_UINT64:
            _sort_model->set_sort_func(*mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<uint64_t>),
                                                         (Gtk::TreeModelColumn<uint64_t> *)mcol));
            break;
          case G_TYPE_FLOAT:
          case G_TYPE_DOUBLE:
            _sort_model->set_sort_func(
              *mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<double>), (Gtk::TreeModelColumn<double> *)mcol));
            break;
          case G_TYPE_STRING:
            _sort_model->set_sort_func(
              *mcol, sigc::bind(sigc::ptr_fun(column_string_compare), (Gtk::TreeModelColumn<Glib::ustring> *)mcol,
                                get_owner()->get_column_type(i)));
            break;
          default:
            logWarning("Unknown column storage type[%d]\n", static_cast<int>(mcol->type()));
            break;
        }
      }

      // temporarily disable selection change signal, gtk emits it when setting model
      _conn.disconnect();
      _tree.set_model(_sort_model);
      _conn = _tree.get_selection()->signal_changed().connect(
        sigc::mem_fun(dynamic_cast<TreeView *>(owner), &TreeView::changed));
    }

    void TreeViewImpl::freeze_refresh(TreeView *self, bool flag) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      Gtk::TreeView *tv = &(impl->_tree);

      if (tv->get_headers_visible())
        tv->set_headers_clickable(!flag);

      if (!flag) {
        tv->thaw_child_notify();
        // tv->set_model(impl->_tree_store);
      } else {
        tv->freeze_child_notify();
        // tv->unset_model();
      }
    }

    Gtk::TreeModel::iterator TreeViewImpl::to_sort_iter(const Gtk::TreeModel::iterator &it) {
      return (_tree.get_headers_clickable() && _sort_model) ? _sort_model->convert_child_iter_to_iter(it) : it;
    }

    Gtk::TreeModel::Path TreeViewImpl::to_sort_path(const Gtk::TreeModel::Path &path) {
      return (_tree.get_headers_clickable() && _sort_model) ? _sort_model->convert_child_path_to_path(path) : path;
    }

    Gtk::TreeModel::iterator TreeViewImpl::to_list_iter(const Gtk::TreeModel::iterator &it) {
      return (_tree.get_headers_clickable() && _sort_model) ? _sort_model->convert_iter_to_child_iter(it) : it;
    }

    Gtk::TreeModel::Path TreeViewImpl::to_list_path(const Gtk::TreeModel::Path &path) {
      return (_tree.get_headers_clickable() && _sort_model) ? _sort_model->convert_path_to_child_path(path) : path;
    }

    void TreeViewImpl::header_clicked(Gtk::TreeModelColumnBase *cbase, Gtk::TreeViewColumn *col) {
      if (!(col && cbase))
        return;

      // Get sort order if anything, if absent set to ASC
      void *data = col->get_data("sord");
      Gtk::SortType sort_order = (Gtk::SortType)((long)data);
      if (sort_order == Gtk::SORT_ASCENDING)
        sort_order = Gtk::SORT_DESCENDING;
      else
        sort_order = Gtk::SORT_ASCENDING;

      const std::vector<Gtk::TreeViewColumn *> cols = _tree.get_columns();
      for (int i = cols.size() - 1; i >= 0; --i) {
        if (cols[i] != col)
          cols[i]->set_sort_indicator(false);
      }

      // Call set_sort_column
      _sort_model->set_sort_column(*cbase, sort_order);
      col->set_sort_indicator(true);
      col->set_sort_order(sort_order);
      col->set_data("sord", (void *)sort_order);
    }

    void TreeViewImpl::set_row_height(TreeView *self, int height) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      impl->_row_height = height;
    }

    TreeNodeRef TreeViewImpl::root_node(TreeView *self) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      return impl->_root_node;
    }

    TreeSelectionMode TreeViewImpl::get_selection_mode(TreeView *self) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      switch (impl->_tree.get_selection()->get_mode()) {
        case Gtk::SELECTION_BROWSE:
        case Gtk::SELECTION_MULTIPLE:
          return TreeSelectMultiple;
        case Gtk::SELECTION_SINGLE:
        default:
          return TreeSelectSingle;
      }
      return TreeSelectSingle;
    }

    void TreeViewImpl::set_selection_mode(TreeView *self, TreeSelectionMode mode) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      switch (mode) {
        case TreeSelectSingle:
          impl->_tree.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
          break;
        case TreeSelectMultiple:
          impl->_tree.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
          break;
      }
    }

    void TreeViewImpl::clear_selection(TreeView *self) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      impl->_tree.get_selection()->unselect_all();
    }

    int TreeViewImpl::row_for_node(TreeView *self, TreeNodeRef node) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      TreeNodeImpl *nodei = dynamic_cast<TreeNodeImpl *>(node.ptr());
      if (impl && nodei) {
        if (impl->_flat_list) {
          if (!nodei->path().empty())
            return nodei->path().back();
        } else
          return calc_row_for_node(&impl->_tree, impl->tree_store()->get_iter(nodei->path()));
      }
      return -1;
    }

    mforms::TreeNodeRef TreeViewImpl::find_node_at_row(const Gtk::TreeModel::Children &children, int &c, int row) {
      for (Gtk::TreeIter last = children.end(), i = children.begin(); i != last; i++) {
        Gtk::TreePath path(*i);
        if (c == row)
          return TreeNodeRef(new TreeNodeImpl(this, _tree_store, path));
        c++;
        if (_tree.row_expanded(path)) {
          Gtk::TreeRow trow = **i;
          TreeNodeRef ref = find_node_at_row(trow.children(), c, row);
          if (ref)
            return ref;
        }
      }
      return TreeNodeRef();
    }

    TreeNodeRef TreeViewImpl::node_at_row(TreeView *self, int row) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      if (impl && row >= 0) {
        Gtk::TreePath path;
        if (impl->_flat_list) {
          path.push_back(row);
          return TreeNodeRef(new TreeNodeImpl(impl, impl->tree_store(), path));
        }
        int i = 0;
        return impl->find_node_at_row(impl->tree_store()->children(), i, row);
      }
      return TreeNodeRef();
    }

    TreeNodeRef TreeViewImpl::node_with_tag(TreeView *self, const std::string &tag) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      if (impl->_tagmap_enabled) {
        std::map<std::string, Gtk::TreeRowReference>::iterator it;
        if ((it = impl->_tagmap.find(tag)) == impl->_tagmap.end())
          return TreeNodeRef();
        return TreeNodeRef(new TreeNodeImpl(impl, it->second));
      }
      throw std::logic_error("node_with_tag() requires tree to be created with TreeIndexOnTag");
    }

    void TreeViewImpl::set_column_visible(TreeView *self, int column, bool flag) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      Gtk::TreeViewColumn *col = impl->_tree.get_column(column);
      if (col)
        col->set_visible(flag);
    }

    bool TreeViewImpl::get_column_visible(TreeView *self, int column) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      Gtk::TreeViewColumn *col = impl->_tree.get_column(column);
      if (col)
        return col->get_visible();
      return false;
    }

    void TreeViewImpl::set_column_title(TreeView *self, int column, const std::string &title) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      Gtk::TreeViewColumn *col = impl->_tree.get_column(column);
      if (col) {
        dynamic_cast<Gtk::Label *>(col->get_widget())->set_text(title);
      }
    }

    mforms::DropPosition TreeViewImpl::get_drop_position() {
      Gtk::TreePath path;
      Gtk::TreeViewDropPosition pos;
      _tree.get_drag_dest_row(path, pos);

      switch (pos) {
        case Gtk::TREE_VIEW_DROP_BEFORE:
          return mforms::DropPositionTop;
        case Gtk::TREE_VIEW_DROP_AFTER:
          return mforms::DropPositionBottom;
        case Gtk::TREE_VIEW_DROP_INTO_OR_AFTER:
        case Gtk::TREE_VIEW_DROP_INTO_OR_BEFORE:
          return mforms::DropPositionOn;
        default:
          return mforms::DropPositionUnknown;
      }
    }

    void TreeViewImpl::on_realize() {
      // nasty workaround to allow context menu for tree headers
      for (int i = 0; i < (int)_tree.get_columns().size(); i++) {
        Gtk::Widget *w = _tree.get_column(i)->get_widget();
        while (w && !dynamic_cast<Gtk::Button *>(w))
          w = w->get_parent();
        if (dynamic_cast<Gtk::Button *>(w))
          w->signal_button_press_event().connect(
            sigc::bind(sigc::mem_fun(this, &TreeViewImpl::on_header_button_event), i), false);
      }
    }

    void TreeViewImpl::set_column_width(TreeView *self, int column, int width) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      Gtk::TreeViewColumn *col = impl->_tree.get_column(column);
      if (col) {
        col->set_resizable(true);
        col->set_fixed_width(width);
      }
    }

    int TreeViewImpl::get_column_width(TreeView *self, int column) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      Gtk::TreeViewColumn *col = impl->_tree.get_column(column);
      if (col)
        return col->get_width();
      return 0;
    }

    mforms::TreeNodeRef TreeViewImpl::node_at_position(TreeView *self, base::Point position) {
      TreeViewImpl *impl = self->get_data<TreeViewImpl>();
      Gtk::TreePath path;
      if (!impl->_tree.get_path_at_pos(position.x, position.y, path))
        return mforms::TreeNodeRef();

      return TreeNodeRef(new TreeNodeImpl(impl, impl->tree_store(), path));
    }

    void TreeViewImpl::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_treeview_impl.create = &TreeViewImpl::create;
      f->_treeview_impl.root_node = &TreeViewImpl::root_node;
      f->_treeview_impl.add_column = &TreeViewImpl::add_column;
      f->_treeview_impl.end_columns = &TreeViewImpl::end_columns;
      f->_treeview_impl.clear = &TreeViewImpl::clear;
      f->_treeview_impl.get_selection_mode = &TreeViewImpl::get_selection_mode;
      f->_treeview_impl.set_selection_mode = &TreeViewImpl::set_selection_mode;
      f->_treeview_impl.clear_selection = &TreeViewImpl::clear_selection;
      f->_treeview_impl.get_selected_node = &TreeViewImpl::get_selected_node;
      f->_treeview_impl.get_selection = &TreeViewImpl::get_selection;
      f->_treeview_impl.set_selected = &TreeViewImpl::set_selected;
      f->_treeview_impl.set_allow_sorting = &TreeViewImpl::set_allow_sorting;
      f->_treeview_impl.freeze_refresh = &TreeViewImpl::freeze_refresh;
      f->_treeview_impl.set_row_height = &TreeViewImpl::set_row_height;
      f->_treeview_impl.node_at_row = &TreeViewImpl::node_at_row;
      f->_treeview_impl.node_with_tag = &TreeViewImpl::node_with_tag;
      f->_treeview_impl.row_for_node = &TreeViewImpl::row_for_node;
      f->_treeview_impl.set_column_visible = &TreeViewImpl::set_column_visible;
      f->_treeview_impl.get_column_visible = &TreeViewImpl::get_column_visible;
      f->_treeview_impl.set_column_title = &TreeViewImpl::set_column_title;
      f->_treeview_impl.set_column_width = &TreeViewImpl::set_column_width;
      f->_treeview_impl.get_column_width = &TreeViewImpl::get_column_width;
      f->_treeview_impl.node_at_position = &TreeViewImpl::node_at_position;
    }
  }
}
