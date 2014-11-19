/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <inttypes.h>

#include "../lf_mforms.h"
#include "../lf_treenodeview.h"
#include "../lf_utilities.h"

#include <glib.h>
#include "base/log.h"
#include "base/string_utilities.h"

#define NO_INT64_COLUMNS

DEFAULT_LOG_DOMAIN("mforms.linux");

namespace mforms {
namespace gtk {




static int count_rows_in_node(Gtk::TreeView *tree, const Gtk::TreeIter &iter)
{
  if (tree->row_expanded(Gtk::TreePath(iter)))
  {
    Gtk::TreeRow row = *iter;
    int count = 0;
    for (Gtk::TreeIter last = row.children().end(), i = row.children().begin(); i != last; i++)
    {
      count++;
      count += count_rows_in_node(tree, i);
    }
    return count;
  }
  return 0;
}


static int calc_row_for_node(Gtk::TreeView *tree, const Gtk::TreeIter &iter)
{
  Gtk::TreeIter parent = iter->parent();
  int node_index = Gtk::TreePath(iter).back();
  int row = node_index;

  if (parent)
  {
    for (Gtk::TreeIter i = parent->children().begin(); i != iter; i++)
      row += count_rows_in_node(tree, i);
    row += calc_row_for_node(tree, parent) + 1;
  }
  else
  {
    Gtk::TreePath path(iter);
    while (path.prev())
      row += count_rows_in_node(tree, tree->get_model()->get_iter(path));
  }
  return row;
}

class TreeNodeViewImpl;


class RootTreeNodeImpl : public ::mforms::TreeNode
{
protected:
  TreeNodeViewImpl *_treeview;
  int _refcount;
  
  inline TreeNodeRef ref_from_iter(const Gtk::TreeIter &iter) const;
  inline TreeNodeRef ref_from_path(const Gtk::TreePath &path) const;

  virtual bool is_root() const
  {
    return true;
  }

  virtual bool is_valid() const
  {
    return _treeview != 0;
  }

  virtual bool equals(const TreeNode &other)
  {
    const RootTreeNodeImpl *impl = dynamic_cast<const RootTreeNodeImpl*>(&other);
    if (impl)
      return impl == this;
    return false;
  }

  virtual int level() const
  {
    return 0;
  }

public:
  RootTreeNodeImpl(TreeNodeViewImpl *tree)
  : _treeview(tree), _refcount(0) // refcount must start at 0
  {
  }

  virtual void invalidate()
  {
    _treeview = 0;
  }

  virtual void release()
  {
    _refcount--;
    if (_refcount == 0)
      delete this;
  }

  virtual void retain()
  {
    _refcount++;
  }

  virtual int count() const
  {
    if (is_valid())
    {
      Glib::RefPtr<Gtk::TreeStore> store(_treeview->tree_store());
      return store->children().size();
    }
    return 0;
  }
  
  virtual bool can_expand() {
    return count() > 0;
  }

  virtual Gtk::TreeIter create_child(int index)
  {
    Glib::RefPtr<Gtk::TreeStore> store(_treeview->tree_store());
    Gtk::TreeIter new_iter;
    
    if (index < 0 || index >= (int)store->children().size())
      new_iter = store->append();
    else
    {
      Gtk::TreePath path;
      path.push_back(index);
      new_iter = store->insert(store->get_iter(path));
    }
      
    return new_iter;
  }

  virtual Gtk::TreeIter create_child(int index, Gtk::TreeIter *other_parent)
  {
    Glib::RefPtr<Gtk::TreeStore> store(_treeview->tree_store());
    Gtk::TreeIter new_iter;
    
    if (index < 0)
      new_iter = other_parent ? store->append((*other_parent)->children()) : store->append();
    else
    {
      Gtk::TreePath path;

      if (other_parent)
        path = store->get_path(*other_parent);
      
      path.push_back(index);
        
      new_iter = store->insert(store->get_iter(path));
    }
      
    return new_iter;
  }

  virtual TreeNodeRef insert_child(int index)
  {
    if (is_valid())
    {
      Gtk::TreeIter new_iter = create_child(index);
      return ref_from_iter(new_iter);
    }
    return TreeNodeRef();
  }

  virtual std::vector<mforms::TreeNodeRef> add_node_collection(const TreeNodeCollectionSkeleton &nodes, int position = -1)
  {
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
    int index_for_icon   = index_for_string - 1;
    
    store->freeze_notify();
    
    for (it = nodes.captions.begin(); it != end; ++it)
    {
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
  
  virtual void add_children_from_skeletons(const std::vector<Gtk::TreeIter>& parents, const std::vector<TreeNodeSkeleton>& children)
  {
    std::vector<Gtk::TreeIter> last_item;
    Glib::RefPtr<Gtk::TreeStore> store(_treeview->tree_store());
    Gtk::TreeIter new_iter;
    Gtk::TreeRow row;
    
    // Takes each received child and inserts it to all the received parents
    std::vector<TreeNodeSkeleton>::const_iterator it, end = children.end();
    for(it = children.begin(); it != end; it++)
    {
      // added iters is the child nodes being added, if the structure indicates they
      // will also have childs, then it is needed to store them to call the function
      // recursively, we reserve the needed space for performance reasons
      std::vector<Gtk::TreeIter> added_iters;
      bool sub_items = !(*it).children.empty();
      if(sub_items)
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
      int index_for_icon   = index_for_string - 1;
      Gtk::TreeModelColumn<std::string>& tag_column = _treeview->_columns.tag_column();
        
      // Now inserts the child on all the received parents
      for(size_t index=0; index < parents.size(); index++)
      {
        if (last_item.size() > index)
        {
          new_iter = store->insert_after(last_item[index]);
          last_item[index] = new_iter;
        }
        else
        {
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
        if(sub_items)
          added_iters.push_back(new_iter);
      }
      
      // If childs will be assigned then it is done by calling the 
      // function again
      if(sub_items)
        add_children_from_skeletons(added_iters, (*it).children);
    }
  }  
  
  virtual void remove_from_parent()
  {
    throw std::logic_error("Cannot delete root node");
  }

  virtual TreeNodeRef get_child(int index) const
  {
    if (is_valid())
    {
      Glib::RefPtr<Gtk::TreeStore> store(_treeview->tree_store());
      return ref_from_iter(store->children()[index]);
    }
    return TreeNodeRef();
  }

  virtual TreeNodeRef get_parent() const
  {
    return TreeNodeRef();
  }


  virtual void expand()
  {
  }

  virtual void collapse()
  {
    g_warning("Can't collapse root node");
  }

  virtual bool is_expanded()
  {
    return true;
  }

  virtual void set_attributes(int column, const mforms::TreeNodeTextAttributes &attrs)
  {
    // noop
  }

  virtual void set_icon_path(int column, const std::string &icon)
  { // noop
  }

  virtual void set_string(int column, const std::string &value)
  { // noop
  }

  virtual void set_int(int column, int value)
  { // noop
  }

  virtual void set_long(int column, boost::int64_t value)
  { // noop
  }

  virtual void set_bool(int column, bool value)
  { // noop
  }

  virtual void set_float(int column, double value)
  { // noop
  }

  virtual std::string get_string(int column) const
  {
    return "";
  }

  virtual int get_int(int column) const
  {
    return 0;
  }

  virtual boost::int64_t get_long(int column) const
  {
    return 0;
  }

  virtual bool get_bool(int column) const
  {
    return false;
  }

  virtual double get_float(int column) const
  {
    return 0.0;
  }
    
  virtual void set_tag(const std::string &tag)
  { // noop
  }

  virtual std::string get_tag() const
  {
    return "";
  }

  virtual void set_data(TreeNodeData *data)
  { // noop
  }

  virtual TreeNodeData *get_data() const
  {
    return NULL;
  }

  virtual TreeNodeRef previous_sibling() const
  {
    return TreeNodeRef();
  }

  virtual TreeNodeRef next_sibling() const
  {
    return TreeNodeRef();
  }

  virtual int get_child_index(TreeNodeRef child) const
  {
    return -1;
  }
};


class TreeNodeImpl : public RootTreeNodeImpl
{
  // If _rowref becomes invalidated (eg because Model was deleted),
  // we just ignore all operations on the node
  Gtk::TreeRowReference _rowref;
  bool _is_expanding;

public:
  inline Glib::RefPtr<Gtk::TreeStore> model()
  {
    // _rowref.get_model() causes crashes in OEL6 because of a refcounting bug in 
    // TreeRowReference that was only fixed in gtkmm 2.20
    return _treeview->tree_store(); // Glib::RefPtr<Gtk::TreeStore>::cast_dynamic(_rowref.get_model());
  }

  inline Gtk::TreeIter iter()
  {
    return model()->get_iter(_rowref.get_path());
  }

  inline Gtk::TreeIter iter() const
  {
    TreeNodeImpl* non_const_this = const_cast<TreeNodeImpl*>(this);
    
    return non_const_this->iter();
  }

  inline Gtk::TreePath path()
  {
    return _rowref.get_path();
  }

  virtual bool is_root() const
  {
    return false;
  }

public:
  TreeNodeImpl(TreeNodeViewImpl *tree, Glib::RefPtr<Gtk::TreeStore> model, const Gtk::TreePath &path)
  : RootTreeNodeImpl(tree), _rowref(model, path), _is_expanding(false)
  {
  }

  TreeNodeImpl(TreeNodeViewImpl *tree, const Gtk::TreeRowReference &ref)
  : RootTreeNodeImpl(tree), _rowref(ref), _is_expanding(false)
  {
  }

  virtual bool equals(const TreeNode &other)
  {
    const TreeNodeImpl *impl = dynamic_cast<const TreeNodeImpl*>(&other);
    if (impl)
      return impl->_rowref == _rowref;
    return false;
  }

  virtual bool is_valid() const
  {
    return _treeview && _rowref.is_valid();
  }

  virtual void invalidate()
  {
    if (_treeview)
    {
      std::map<std::string, Gtk::TreeRowReference>::iterator it = _treeview->_tagmap.find(get_tag());
      if (it != _treeview->_tagmap.end())
        _treeview->_tagmap.erase(it);
    }

    _treeview = 0;
    _rowref = Gtk::TreeRowReference();
  }

  virtual int count() const
  {
    if (is_valid())
    {
      //Glib::RefPtr<Gtk::TreeStore> store(model());
      Gtk::TreeRow row = *iter();
      return row.children().size();
    }
    return 0;
  }

  virtual Gtk::TreeIter create_child(int index)
  {
    Glib::RefPtr<Gtk::TreeStore> store(model());
    Gtk::TreeIter new_iter;

    if (index < 0)
      new_iter = store->append(iter()->children());
    else
    {
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
  
  virtual void remove_from_parent()
  {
    if (is_valid())
    {
      if (_treeview->_tagmap_enabled)
      {
        std::map<std::string, Gtk::TreeRowReference>::iterator it;
        if ((it = _treeview->_tagmap.find(get_tag())) != _treeview->_tagmap.end())
          _treeview->_tagmap.erase(it);
      }

      Glib::RefPtr<Gtk::TreeStore> store(model());
      store->erase(iter());

      invalidate();
    }
  }

  virtual TreeNodeRef get_child(int index) const
  {
    if (is_valid())
    {
      Gtk::TreeRow row = *iter();
      return ref_from_iter(row->children()[index]);
    }
    return TreeNodeRef();
  }

  virtual TreeNodeRef get_parent() const
  {
    if (is_valid())
    {
      Gtk::TreePath path = _rowref.get_path();
      if (path.empty() || !path.up() || path.empty())
        return _treeview->_root_node;
      return ref_from_path(path);
    }
    return TreeNodeRef();
  }

  virtual void expand()
  {
    if (is_valid() && !is_expanded())
    {

      if (!_treeview->tree_view()->expand_row(_rowref.get_path(), false)) //if somehow we got null, then we need to call expand_toggle ourselves
      {                                                                   //cause it will not emmit the test-will-expand signal that should trigger
        TreeNodeView *view = _treeview->get_owner();                      //expand_toggle call
        if (view)
          view->expand_toggle(mforms::TreeNodeRef(this), true);
      }
    }
  }

  virtual bool can_expand()
  {
    if(is_valid())
    {
      Gtk::TreeRow row = *iter();
      return row->children().size() > 0;
    }
    return false;
  }

  virtual void collapse()
  {
    if (is_valid())
      _treeview->tree_view()->collapse_row(_rowref.get_path());
  }

  virtual bool is_expanded()
  {
    if (is_valid())
      return _treeview->tree_view()->row_expanded(_rowref.get_path());
    return false;
  }

  virtual void set_attributes(int column, const TreeNodeTextAttributes &attrs)
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      Pango::AttrList attrlist;

      if (attrs.bold)
      {
        Pango::Attribute a = Pango::Attribute::create_attr_weight(Pango::WEIGHT_BOLD);
        attrlist.insert(a);
      }
      if (attrs.italic)
      {
        Pango::Attribute a = Pango::Attribute::create_attr_style(Pango::STYLE_ITALIC);
        attrlist.insert(a);
      }
      if (attrs.color.is_valid())
      {
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

  virtual void set_icon_path(int column, const std::string &icon)
  {
    Gtk::TreeRow row = *iter();
    if (!icon.empty())
    {
      Glib::RefPtr<Gdk::Pixbuf> pixbuf = UtilitiesImpl::get_cached_icon(icon);
      if (pixbuf)
        row.set_value(_treeview->index_for_column(column)-1, pixbuf);
    }
    else
      row.set_value(_treeview->index_for_column(column)-1, Glib::RefPtr<Gdk::Pixbuf>());
  }

  virtual void set_string(int column, const std::string &value)
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      int i = _treeview->index_for_column(column);
      switch (_treeview->_tree_store->get_column_type(i))
      {
      case G_TYPE_BOOLEAN:
        row.set_value(i, value != "0" ? true : false);
        break;
      case G_TYPE_INT:
        row.set_value(i, base::atoi<int>(value, 0));
        break;
      case G_TYPE_INT64:
        row.set_value(i, base::atoi<long long>(value, 0LL));
        break;
      default:
        {
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

  virtual void set_int(int column, int value)
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      int i = _treeview->index_for_column(column);
      switch (_treeview->_tree_store->get_column_type(i))
      {
      case G_TYPE_BOOLEAN:
        row.set_value(i, value != 0);
        break;
      default:
        row.set_value(i, value);
        break;
      }
    }
  }

  virtual void set_long(int column, boost::int64_t value)
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      row.set_value(_treeview->index_for_column(column), base::strfmt("%" PRId64, value));
    }
  }

  virtual void set_bool(int column, bool value)
  {
    if (is_valid() && !is_root())
    {
			set_int(column, value);
    }
  }

  virtual void set_float(int column, double value)
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      row.set_value(_treeview->index_for_column(column), value);
    }
  }

  virtual std::string get_string(int column) const
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      std::string value;
      row.get_value(_treeview->index_for_column(column), value);
      return value;
    }
    return "";
  }

  virtual int get_int(int column) const
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      int i = _treeview->index_for_column(column);
      int value;
      bool bvalue;
      switch (_treeview->_tree_store->get_column_type(i))
      {
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

  virtual boost::int64_t get_long(int column) const
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      std::string value;
      row.get_value(_treeview->index_for_column(column), value);
      return strtoll(value.c_str(), NULL, 0);
    }
    return 0;
  } 

  virtual bool get_bool(int column) const
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      bool value;
      row.get_value(_treeview->index_for_column(column), value);
      return value;
    }
    return false;
 }

  virtual double get_float(int column) const
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      double value;
      row.get_value(_treeview->index_for_column(column), value);
      return value;
    }
    return 0.0;
  }
    
  virtual void set_tag(const std::string &tag)
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      std::string old_tag = row[_treeview->_columns.tag_column()];

      if (!old_tag.empty())
      {
        std::map<std::string, Gtk::TreeRowReference>::iterator it = _treeview->_tagmap.find(old_tag);
        if (it != _treeview->_tagmap.end())
          _treeview->_tagmap.erase(it);
      }

      row[_treeview->_columns.tag_column()] = tag;

      if (tag.empty())
      {
        std::map<std::string, Gtk::TreeRowReference>::iterator it = _treeview->_tagmap.find(tag);
        if (it != _treeview->_tagmap.end())
          _treeview->_tagmap.erase(it);
      }
      else
        _treeview->_tagmap[tag] = _rowref;
    }
  }

  virtual std::string get_tag() const
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      std::string tag = row[_treeview->_columns.tag_column()];
      return tag;
    }
    return "";
  }

  virtual void set_data(TreeNodeData *data)
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      row[_treeview->_columns.data_column()] = TreeNodeDataRef(data);
    }
  }

  virtual TreeNodeData *get_data() const
  {
    if (is_valid() && !is_root())
    {
      Gtk::TreeRow row = *iter();
      TreeNodeDataRef data = row[_treeview->_columns.data_column()];
      return data._data;
    }
    return NULL;
  }

  virtual int level() const
  {
    if (is_root())
      return 0;
    return _treeview->tree_store()->iter_depth(*iter()) + 1;
  }

  virtual TreeNodeRef next_sibling() const
  {
    if (is_root())
      return TreeNodeRef();

    Gtk::TreePath path = _rowref.get_path();
    path.next();
    Gtk::TreeIter iter = _treeview->tree_store()->get_iter(path);
    if (iter)
      return ref_from_path(path);
    return TreeNodeRef();
  }

  virtual TreeNodeRef previous_sibling() const
  {
    if (is_root())
      return TreeNodeRef();

    Gtk::TreePath path = _rowref.get_path();
    if (!path.prev())
      return TreeNodeRef();

    return ref_from_path(path);
  }

  virtual int get_child_index(TreeNodeRef child) const
  {
    dynamic_cast<TreeNodeImpl*>(child.ptr());
    TreeNodeImpl *node = dynamic_cast<TreeNodeImpl*>(child.ptr());
    if (node)
      return ((std::vector<int>)node->path().get_indices())[0];
    return -1;
  }
};


inline TreeNodeRef RootTreeNodeImpl::ref_from_iter(const Gtk::TreeIter &iter) const
{
  Gtk::TreePath path(iter);
  return TreeNodeRef(new TreeNodeImpl(_treeview, _treeview->tree_store(), path));
}

inline TreeNodeRef RootTreeNodeImpl::ref_from_path(const Gtk::TreePath &path) const
{
  return TreeNodeRef(new TreeNodeImpl(_treeview, _treeview->tree_store(), path));
}



//---------------------------------------------------------------------------------------

TreeNodeViewImpl::ColumnRecord::~ColumnRecord()
{
  for (std::vector<Gtk::TreeModelColumnBase*>::iterator iter= columns.begin();
       iter != columns.end(); ++iter)
    delete *iter;
}

void TreeNodeViewImpl::ColumnRecord::add_tag_column()
{
  add(_tag_column);
}

void TreeNodeViewImpl::ColumnRecord::add_data_column()
{
  add(_data_column);
}

Gtk::TreeModelColumn<std::string>& TreeNodeViewImpl::ColumnRecord::tag_column()
{
  return _tag_column;
}

Gtk::TreeModelColumn<TreeNodeDataRef>& TreeNodeViewImpl::ColumnRecord::data_column()
{
  return _data_column;
}

int TreeNodeViewImpl::ColumnRecord::add_string(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr, bool with_icon, bool align_right)
{
  Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > *icon= 0;
  std::string tmp = title;
  base::replace(tmp, "_", "__");
  Gtk::TreeViewColumn *column= Gtk::manage(new Gtk::TreeViewColumn(tmp));
  int idx;

  if (with_icon)
  {
    Gtk::CellRendererPixbuf *cell = Gtk::manage(new Gtk::CellRendererPixbuf());
    icon = add_model_column<Glib::RefPtr<Gdk::Pixbuf> >();
    column->pack_start(*cell, false);
    column->add_attribute(cell->property_pixbuf(), *icon);
    //columns.push_back(icon);
  }


  Gtk::TreeModelColumn<Glib::ustring> *col = add_model_column<Glib::ustring>();
  column_value_index.push_back(size()-1);

  Gtk::CellRendererText *cell = Gtk::manage(new Gtk::CellRendererText());

  if (align_right)
    cell->set_alignment(1.0, 0.5);

  column->pack_start(*cell);
  column->add_attribute(cell->property_text(), *col);
  if (attr)
  {
    Gtk::TreeModelColumn<Pango::AttrList> *acol = add_model_column<Pango::AttrList>();
    column_attr_index.push_back(size()-1);
    column->add_attribute(cell->property_attributes(), *acol);
  }
  else
    column_attr_index.push_back(-1);

  cell->property_editable() = editable;

  if (editable)
    cell->signal_editing_started().connect(sigc::mem_fun(this, &ColumnRecord::on_cell_editing_started));

  idx= tree->append_column(*column);
  tree->get_column(idx-1)->set_resizable(true);

  return idx-1;
}


int TreeNodeViewImpl::ColumnRecord::add_integer(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr)
{
  Gtk::TreeModelColumn<int> *column= add_model_column<int>();
  int idx;
  column_value_index.push_back(size()-1);

  if (editable)
    idx = tree->append_column_editable(title, *column);
  else
    idx = tree->append_column(title, *column);

  if (attr)
  {
  }
  else
    column_attr_index.push_back(-1);

  if (editable)
    tree->get_column(idx)->get_first_cell_renderer()->signal_editing_started().connect(sigc::mem_fun(this, &ColumnRecord::on_cell_editing_started));

  return idx-1;
}

int TreeNodeViewImpl::ColumnRecord::add_long_integer(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr)
{
#ifdef NO_INT64_COLUMNS
  Gtk::TreeModelColumn<Glib::ustring> *column= add_model_column<Glib::ustring>();
#else
  Gtk::TreeModelColumn<boost::int64_t> *column= add_model_column<boost::int64_t>();
#endif
  int idx;
  column_value_index.push_back(size()-1);

  if (editable)
    idx = tree->append_column_editable(title, *column);
  else
    idx = tree->append_column(title, *column);
  if (attr)
  {
  }
  else
    column_attr_index.push_back(-1);

  if (editable)
    tree->get_column(idx)->get_first_cell_renderer()->signal_editing_started().connect(sigc::mem_fun(this, &ColumnRecord::on_cell_editing_started));

  return idx-1;
}

int TreeNodeViewImpl::ColumnRecord::add_float(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr)
{

  Gtk::TreeModelColumn<double> *column= add_model_column<double>();
    int idx;
    column_value_index.push_back(size()-1);

    if (editable)
      idx = tree->append_column_editable(title, *column);
    else
      idx = tree->append_column(title, *column);

    if (attr)
    {
    }
    else
      column_attr_index.push_back(-1);

    if (editable)
      tree->get_column(idx)->get_first_cell_renderer()->signal_editing_started().connect(sigc::mem_fun(this, &ColumnRecord::on_cell_editing_started));

    return idx-1;
}

int TreeNodeViewImpl::ColumnRecord::add_check(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr)
{
  Gtk::TreeModelColumn<bool> *column= add_model_column<bool>();
  int idx;
  column_value_index.push_back(size()-1);

  if (editable)
    idx = tree->append_column_editable(title, *column);
  else
    idx = tree->append_column(title, *column);
  if (attr)
  {
  }
  else
    column_attr_index.push_back(-1);

  return idx-1;
}

void TreeNodeViewImpl::ColumnRecord::format_tri_check(Gtk::CellRenderer* cell,
    const Gtk::TreeIter& iter,
    const Gtk::TreeModelColumn<int>& column)
{
  Gtk::CellRendererToggle *toggle = (Gtk::CellRendererToggle *)cell;
  if (toggle)
  {
    int val = iter->get_value(column);
    if (val == -1)
    {
      toggle->set_property("inconsistent", true);
      toggle->set_active(true);
    }
    else
    {
      toggle->set_property("inconsistent", false);
      toggle->set_active(val != 0);
    }
  }
}

int TreeNodeViewImpl::ColumnRecord::add_tri_check(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr)
{
  std::string tmp = title;
  base::replace(tmp, "_", "__");
  Gtk::TreeViewColumn *column= Gtk::manage(new Gtk::TreeViewColumn(tmp));

  Gtk::TreeModelColumn<int> *col= add_model_column<int>();
  int idx;
  column_value_index.push_back(size()-1);

  Gtk::CellRendererToggle *cell = Gtk::manage(new Gtk::CellRendererToggle());

  column->pack_start(*cell);

  if (!attr)
  {
    column_attr_index.push_back(-1);
  }

  column->set_cell_data_func(*cell, sigc::bind(sigc::mem_fun(this, &TreeNodeViewImpl::ColumnRecord::format_tri_check), *col));

  idx = tree->append_column(*column);

  return idx-1;
}

void TreeNodeViewImpl::ColumnRecord::on_cell_editing_started(Gtk::CellEditable* e, const Glib::ustring &path)
{
  Gtk::Widget* w = dynamic_cast<Gtk::Widget*>(e);
  if (w)
    w->signal_focus_out_event().connect(sigc::bind(sigc::mem_fun(this, &ColumnRecord::on_focus_out), dynamic_cast<Gtk::Entry*>(e)), false);
}


bool TreeNodeViewImpl::ColumnRecord::on_focus_out(GdkEventFocus *event, Gtk::Entry *e)
{
  // Emulate pressing Enter on the text entry so that a focus out will save ongoing changes
  // instead of discarding them
  if (!event->in)
    e->activate();
  return false;
}


//---------------------------------------------------------------------------------------

TreeNodeViewImpl::TreeNodeViewImpl(TreeNodeView *self, mforms::TreeOptions opts)
  : ViewImpl(self), _row_height(-1), _org_event(0)
{
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

  _conn = _tree.get_selection()->signal_changed().connect(sigc::mem_fun(self, &TreeNodeView::changed));
  _tree.signal_row_activated().connect(sigc::mem_fun(this, &TreeNodeViewImpl::on_activated));
  _tree.signal_row_collapsed().connect(sigc::mem_fun(this, &TreeNodeViewImpl::on_collapsed));
//  _tree.signal_row_expanded().connect(sigc::mem_fun(this, &TreeNodeViewImpl::on_expanded));
  _tree.signal_test_expand_row().connect(sigc::bind_return(sigc::mem_fun(this, &TreeNodeViewImpl::on_will_expand), false));
  _tree.signal_key_release_event().connect(sigc::mem_fun(this, &TreeNodeViewImpl::on_key_release), false);
  _tree.signal_button_press_event().connect(sigc::mem_fun(this, &TreeNodeViewImpl::on_button_event), false);
//  _tree.set_reorderable((opts & mforms::TreeAllowReorderRows) || (opts & mforms::TreeCanBeDragSource)); // we need this to have D&D working
  _tree.signal_button_release_event().connect(sigc::mem_fun(this, &TreeNodeViewImpl::on_button_release), false);
  _tree.signal_expose_event().connect(sigc::mem_fun(this, &TreeNodeViewImpl::on_expose_event), true);
  _tree.signal_realize().connect(sigc::mem_fun(this, &TreeNodeViewImpl::on_realize));
  _tree.signal_motion_notify_event().connect(sigc::mem_fun(this, &TreeNodeViewImpl::on_motion_notify), false);
  _tree.signal_enter_notify_event().connect(sigc::mem_fun(this, &TreeNodeViewImpl::on_enter_notify), false);
  _tree.signal_leave_notify_event().connect(sigc::mem_fun(this, &TreeNodeViewImpl::on_leave_notify), false);

  _is_drag_source = false;
  if (opts & mforms::TreeCanBeDragSource)
  {
    _is_drag_source = true;
    Gtk::Widget *w = this->get_outer();
    if (w)
    {
      //connect signals to apropriate methods that are defined in ViewImpl
      w->signal_drag_data_get().connect(sigc::mem_fun(this, &TreeNodeViewImpl::slot_drag_data_get));
      w->signal_drag_begin().connect(sigc::mem_fun(this, &TreeNodeViewImpl::slot_drag_begin));
      w->signal_drag_end().connect(sigc::mem_fun(this, &TreeNodeViewImpl::slot_drag_end));
      w->signal_drag_failed().connect(sigc::mem_fun(this, &TreeNodeViewImpl::slot_drag_failed));
    }
    _tree.add_events(Gdk::POINTER_MOTION_MASK);
  }


  _swin.add(_tree);
  _swin.show_all();
  _tree.set_headers_visible((opts & mforms::TreeNoHeader) == 0);
}

TreeNodeViewImpl::~TreeNodeViewImpl()
{
  if (_org_event)
    delete _org_event;
}

void TreeNodeViewImpl::slot_drag_end(const Glib::RefPtr<Gdk::DragContext> &context)
{
  ViewImpl::slot_drag_end(context);
  _drag_in_progress  = false;
  _drag_button = 0;
  if (_org_event)
  {
    delete _org_event;
    _org_event = 0;
  }
}

bool TreeNodeViewImpl::slot_drag_failed(const Glib::RefPtr<Gdk::DragContext> &context,Gtk::DragResult result)
{
  bool ret_val = ViewImpl::slot_drag_failed(context, result);
  _drag_in_progress  = false;
  _drag_button = 0;
  if (_org_event)
  {
    delete _org_event;
    _org_event = 0;
  }
  return ret_val;

}

bool TreeNodeViewImpl::on_expose_event(GdkEventExpose *ev)
{
  if (!_overlay_icons.empty() && !_overlayed_row.empty() && _mouse_inside)
  {
    Cairo::RefPtr<Cairo::Context> context(_tree.get_bin_window()->create_cairo_context());
    Gdk::Rectangle rect;
    Gdk::Rectangle vrect;
    int i = 1;
    _tree.get_visible_rect(vrect);
    _tree.get_background_area(_overlayed_row, *_tree.get_column(_tree.get_columns().size()-1), rect);

    int x = vrect.get_x() + vrect.get_width() - 4;
    for (std::vector<Cairo::RefPtr<Cairo::ImageSurface> >::const_iterator icon = _overlay_icons.begin();
        icon != _overlay_icons.end(); ++icon)
      x -= (*icon)->get_width() + 4;

    for (std::vector<Cairo::RefPtr<Cairo::ImageSurface> >::const_iterator icon = _overlay_icons.begin();
        icon != _overlay_icons.end(); ++icon, ++i)
    {
      if (*icon)
      {
        context->set_source(*icon, x, rect.get_y() + (rect.get_height() - (*icon)->get_height()) / 2);
        x += (*icon)->get_width() + 4;
        if (i-1 == _hovering_overlay)
          context->paint();
        else
          context->paint_with_alpha(0.4);
      }
    }
  }

  return false;
}



bool TreeNodeViewImpl::on_enter_notify(GdkEventCrossing *ev)
{
  _mouse_inside = true;
  return false;
}


bool TreeNodeViewImpl::on_leave_notify(GdkEventCrossing *ev)
{
  if (_mouse_inside)
  {
    _mouse_inside = false;
    _overlay_icons.clear();
    _hovering_overlay = -1;
    _clicking_overlay = -1;
    _tree.queue_draw();
  }
  return false;  
}


bool TreeNodeViewImpl::on_motion_notify(GdkEventMotion *ev)
{
  int dummy;
  Gtk::TreeViewColumn *column;
  Gtk::TreePath path;

  // handle overlay icons
  if (_clicking_overlay < 0)
  {
    _overlay_icons.clear();
    _hovering_overlay=-1;
    _overlayed_row.clear();
    _tree.queue_draw();
  }
  if (!_drag_in_progress && _tree.get_path_at_pos(ev->x, ev->y, path, column, dummy, dummy))
  {
    mforms::TreeNodeView* tv = dynamic_cast<mforms::TreeNodeView*>(owner);
    mforms::TreeNodeRef node(new TreeNodeImpl(this, tree_store(), path));

    if (node)
    {
      std::vector<std::string> icons = tv->overlay_icons_for_node(node);
      if (!icons.empty())
      {
        int icon_rect_x;
        Gdk::Rectangle rect;
        Gdk::Rectangle vrect;

        _overlayed_row = path;

        _tree.get_background_area(path, *column, rect);

        _tree.get_visible_rect(vrect);
        icon_rect_x = vrect.get_width() - 4;
        
        for (std::vector<std::string>::const_iterator icon = icons.begin();
          icon != icons.end(); ++icon)
        {
          Cairo::RefPtr<Cairo::ImageSurface> surf;
          if (!icon->empty())
          {
            surf = Cairo::ImageSurface::create_from_png(*icon);
            if (!surf)
              g_warning("Could not load %s", icon->c_str());
          }
          _overlay_icons.push_back(surf);
          icon_rect_x -= surf->get_width() + 4;
        }

        for (std::vector<Cairo::RefPtr<Cairo::ImageSurface> >::const_iterator icon = _overlay_icons.begin();
            icon != _overlay_icons.end(); ++icon)
        {
          if (ev->x - vrect.get_x() > icon_rect_x && ev->x - vrect.get_x() < icon_rect_x + (*icon)->get_width())
          {
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
  if (_drag_in_progress || _drag_button == 0 || !ev || !_drag_source_enabled)
  {
    return false;
  }

  Gtk::Widget *w = this->get_outer();
  if (w)
  {
    if (w->drag_check_threshold(_drag_start_x, _drag_start_y, ev->x, ev->y))
    {
      if (_org_event != 0)
      {
        delete _org_event;
        _org_event = 0;
      }

      {
        //Because of problems when Treeview has been set to multiselect,
        //there are some DnD problems, below code is fixing those.
        //We check if users already selected something,
        //if he is trying to drag not selected row,
        //we change selection.
        std::vector<Gtk::TreePath> path_selection;
        path_selection = _tree.get_selection()->get_selected_rows();

        Gtk::TreeModel::Path selpath;
        if(_tree.get_path_at_pos(_drag_start_x, _drag_start_y, selpath))
        {
          bool found = false;
          for(std::vector<Gtk::TreePath>::iterator it = path_selection.begin(); it != path_selection.end(); ++it)
          {
            if (selpath == *it)
            {
              found = true;
              break;
            }
          }
          if (!found)
          {
            //Old selection is invalid, we need to create new one
            _tree.get_selection()->unselect_all();
            _tree.get_selection()->select(selpath);
          }
        }
      }


      TreeNodeView *view = dynamic_cast<TreeNodeView*>(owner);
      if (view)
      {
        mforms::DragDetails details;

        Gtk::TreeModel::Path path;
        if(_tree.get_path_at_pos(_drag_start_x, _drag_start_y, path))
        {
          Glib::RefPtr<Gdk::Pixmap> row_icon = _tree.create_row_drag_icon(path);
          int ico_w, ico_h;
          row_icon->get_size(ico_w, ico_h);

          Glib::RefPtr<Gdk::Pixbuf> pix = Gdk::Pixbuf::create(row_icon->get_image(0, 0, ico_w, ico_h), 0, 0, ico_w, ico_h);
          //sadly we need a cairo_surface
          details.image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ico_w, ico_h);
          cairo_t *cr = cairo_create(details.image);
          gdk_cairo_set_source_pixbuf(cr, pix->gobj(), 0, 0);
          cairo_paint(cr);

          void *data = NULL;
          std::string text;
          std::string format;
          details.location = base::Point(ev->x, ev->y);
          details.allowedOperations = mforms::DragOperationCopy;
          if(!view->get_drag_data(details, &data, format))
          {

            format = "STRING";
            std::list<mforms::TreeNodeRef> selection = view->get_selection();

            for (std::list<mforms::TreeNodeRef>::const_iterator iterator = selection.begin(); iterator != selection.end(); ++iterator)
            {
                  if (!(*iterator)->get_string(0).empty())
                  {
                    if (text.size() > 0)
                      text.append(", ");
                    text.append((*iterator)->get_string(0));
                  }
            }

            if (text.empty())
            {
              cairo_surface_destroy(details.image);
              cairo_destroy(cr);
              return false;
            }

            data = &text;
          }

          mforms::DragOperation operation = view->do_drag_drop(details, data, format);
          cairo_surface_destroy(details.image);
          cairo_destroy(cr);
          view->drag_finished(operation);
        }
      }
    }
  }
  return false;
}

bool TreeNodeViewImpl::on_button_release(GdkEventButton* ev)
{
  if (!_drag_in_progress && _hovering_overlay >= 0 && _hovering_overlay == _clicking_overlay)
  { 
    mforms::TreeNodeView* tv = dynamic_cast<mforms::TreeNodeView*>(owner);
    mforms::TreeNodeRef node(new TreeNodeImpl(this, tree_store(), _overlayed_row));
    if (node)
      tv->overlay_icon_for_node_clicked(node, _clicking_overlay);
  }
  _clicking_overlay = -1;

  if (_drag_in_progress)
  {
    return false;
  }

  //Because of problems when Treeview has been set to multiselect,
  //there are some DnD problems, below code is fixing those.
  //we need this to emit press event again cause we're changing default behavior of it
  if (_org_event != 0)
  {
    gtk_propagate_event((GtkWidget*)_tree.gobj(), (GdkEvent*)_org_event);
    delete _org_event;
    _org_event = 0;
  }
//  _tree.
  _drag_button = 0;
  return false;
}

TreeNodeView* TreeNodeViewImpl::get_owner()
{
  TreeNodeView* view = dynamic_cast<TreeNodeView*>(owner);
  if(view)
    return view;
  return NULL;
}

void TreeNodeViewImpl::set_back_color(const std::string &color)
{
  if (!force_sys_colors)
  {
    if (!color.empty())
    {
      Gdk::Color col(color);
      _tree.get_colormap()->alloc_color(col);
      _tree.modify_base(Gtk::STATE_NORMAL, col);
    }
  }
}


void TreeNodeViewImpl::string_edited(const Glib::ustring &path, const Glib::ustring &new_text, int column)
{
  if (_tree_store)
  {
    Gtk::TreePath tree_path = to_list_path(Gtk::TreePath(path));
    Gtk::TreeRow row= *_tree_store->get_iter(tree_path);
    if (dynamic_cast<TreeNodeView*>(owner)->cell_edited(TreeNodeRef(new TreeNodeImpl(this, _tree_store, tree_path)), column, new_text))
      row[_columns.get<Glib::ustring>(column)]= new_text;
  }
}
  

void TreeNodeViewImpl::toggle_edited(const Glib::ustring &path, int column)
{
  if (_tree_store)
  {
    Gtk::TreePath tree_path = to_list_path(Gtk::TreePath(path));

    TreeNodeRef node(new TreeNodeImpl(this, _tree_store, tree_path));
    //Because of TriCheckColumnType we need to use int instead of bool
    int value = node->get_int(column) == 0 ? 1 : 0;
    std::stringstream ss;
    ss << value;
   
    if (dynamic_cast<TreeNodeView*>(owner)->cell_edited(node, column, ss.str()))
      node->set_int(column, value);
  }
}

void TreeNodeViewImpl::on_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
{
  mforms::TreeNodeView* tv = dynamic_cast<mforms::TreeNodeView*>(owner); // owner is from deeply hidden class TreeNodeViewImpl->ViewImpl->ObjectImpl.owner
  if (tv)
  {
    Gtk::TreePath tree_path = to_list_path(path);
    tv->node_activated(mforms::TreeNodeRef(new TreeNodeImpl(this, _tree_store, tree_path)), (intptr_t)column->get_data("index"));
  }
}

void TreeNodeViewImpl::on_will_expand(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path)
{
  mforms::TreeNodeView* tv = dynamic_cast<mforms::TreeNodeView*>(owner);
  if (tv)
  {
    Gtk::TreePath tree_path = to_list_path(path);
    tv->expand_toggle(mforms::TreeNodeRef(new TreeNodeImpl(this, _tree_store, tree_path)), true);
  }
}

void TreeNodeViewImpl::on_collapsed(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path)
{
  mforms::TreeNodeView* tv = dynamic_cast<mforms::TreeNodeView*>(owner);
  if (tv)
  {
    Gtk::TreePath tree_path = to_list_path(path);
    tv->expand_toggle(mforms::TreeNodeRef(new TreeNodeImpl(this, _tree_store, tree_path)), false);
  }
}

bool TreeNodeViewImpl::on_key_release(GdkEventKey *ev)
{
  mforms::TreeNodeView* tv = dynamic_cast<mforms::TreeNodeView*>(owner);
  TreeNodeRef node = this->get_selected_node(tv);
  if (!node.is_valid())
    return false;

  if (ev->keyval == GDK_Left)
    node->collapse();
  else if (ev->keyval == GDK_Right)
    node->expand();

  return false;
}

bool TreeNodeViewImpl::on_button_event(GdkEventButton *event)
{
  bool ret_val = false;

  if (event->button == 1 && _drag_button == 0 && _hovering_overlay >= 0)
  {
    _clicking_overlay = _hovering_overlay;
  }
  
  if (event->button == 3)
  {
    mforms::TreeNodeView* tv = dynamic_cast<mforms::TreeNodeView*>(owner); 
    if (tv->get_context_menu())
      tv->get_context_menu()->popup_at(mforms::gtk::ViewImpl::get_view_for_widget(this->get_outer()), base::Point(event->x, event->y));
    
    std::list<TreeNodeRef> selected_nodes = this->get_selection(tv);
    
    // For multiple selection will work with this handle
    // In other case will let the default handle
    
    //TODO: Add validation for when the right click is done on a node
    //      that is not part of the selected group, in such case the
    //      return value should be false to clear the selection and
    //      select that node
    if (selected_nodes.size() > 1)
      ret_val = true;
  }
  else if (event->button == 1 && _drag_button == 0)
  {
    Gtk::TreeModel::Path  path;
    Gtk::TreeViewDropPosition   pos;
    if (_tree.get_dest_row_at_pos(event->x, event->y, path, pos) && _is_drag_source)
    {
      if (_org_event == 0)
      {
        _org_event = new GdkEventButton(*event);
        _drag_button = event->button;
        _drag_start_x = event->x;
        _drag_start_y = event->y;
        ret_val = true;
      }
    }
  }

  return ret_val;
}


bool TreeNodeViewImpl::on_header_button_event(GdkEventButton *event, int column)
{
  if (event->button == 3)
  {
    mforms::TreeNodeView* tv = dynamic_cast<mforms::TreeNodeView*>(owner);

    tv->header_clicked(column);

    if (tv->get_header_menu())
      tv->get_header_menu()->popup_at(mforms::gtk::ViewImpl::get_view_for_widget(this->get_inner()), base::Point(event->x, event->y));
  }
  return false;
}



int TreeNodeViewImpl::add_column(TreeColumnType type, const std::string &name, int initial_width, bool editable, bool attributed)
{
  int column = -1;
  switch (type)
  {
  case IconColumnType:
    column= _columns.add_string(&_tree, name, editable, attributed, true);
    if (editable)
    {
      std::vector<Gtk::CellRenderer*> rends(_tree.get_column(column)->get_cell_renderers());
      ((Gtk::CellRendererText*)rends[1])->signal_edited().
        connect(sigc::bind(sigc::mem_fun(this, &TreeNodeViewImpl::string_edited), column));
    }
    break;
  case StringColumnType:
  case StringLTColumnType:
    column= _columns.add_string(&_tree, name, editable, attributed, false);
    if (editable)
    {
      Gtk::CellRendererText *rend = ((Gtk::CellRendererText*)_tree.get_column(column)->get_first_cell_renderer());
      rend->signal_edited().connect(sigc::bind(sigc::mem_fun(this, &TreeNodeViewImpl::string_edited), column));
      if (type == StringLTColumnType)
        rend->property_ellipsize() = Pango::ELLIPSIZE_START;
      else
        rend->property_ellipsize() = Pango::ELLIPSIZE_END;
    }
    break;
  case IntegerColumnType: 
    column= _columns.add_integer(&_tree, name, editable, attributed);
    if (editable)
    {
      ((Gtk::CellRendererText*)_tree.get_column(column)->get_first_cell_renderer())->signal_edited().
        connect(sigc::bind(sigc::mem_fun(this, &TreeNodeViewImpl::string_edited), column));
    }
    break;
  case LongIntegerColumnType: 
    column= _columns.add_long_integer(&_tree, name, editable, attributed);
    if (editable)
    {
      ((Gtk::CellRendererText*)_tree.get_column(column)->get_first_cell_renderer())->signal_edited().
        connect(sigc::bind(sigc::mem_fun(this, &TreeNodeViewImpl::string_edited), column));
    }
    break;
  case FloatColumnType:
    column= _columns.add_float(&_tree, name, editable, attributed);
    if (editable)
    {
      ((Gtk::CellRendererText*)_tree.get_column(column)->get_first_cell_renderer())->signal_edited().
        connect(sigc::bind(sigc::mem_fun(this, &TreeNodeViewImpl::string_edited), column));
    }
    break;
  case CheckColumnType: 
    column= _columns.add_check(&_tree, name, editable, attributed);
    if (editable)
    {
      ((Gtk::CellRendererToggle*)_tree.get_column(column)->get_first_cell_renderer())->signal_toggled().
        connect(sigc::bind(sigc::mem_fun(this, &TreeNodeViewImpl::toggle_edited), column));
    }
    break;
  case NumberWithUnitColumnType:
    column= _columns.add_string(&_tree, name, editable, attributed, false, true);
    if (editable)
    {
      ((Gtk::CellRendererText*)_tree.get_column(column)->get_first_cell_renderer())->signal_edited().
        connect(sigc::bind(sigc::mem_fun(this, &TreeNodeViewImpl::string_edited), column));
    }
    break;
  case TriCheckColumnType:
    column= _columns.add_tri_check(&_tree, name, editable, attributed);
    if (editable)
    {
      ((Gtk::CellRendererToggle*)_tree.get_column(column)->get_first_cell_renderer())->signal_toggled().
        connect(sigc::bind(sigc::mem_fun(this, &TreeNodeViewImpl::toggle_edited), column));
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
  tvc->set_data("index", (void*)(intptr_t)column);
  
  return column;
}


void TreeNodeViewImpl::end_columns()
{
  _columns.add_tag_column();
  _columns.add_data_column();

  _tree_store= Gtk::TreeStore::create(_columns);
  _tree.set_model(_tree_store);

  _root_node = TreeNodeRef(new RootTreeNodeImpl(this));

  // enable sorting if previously enabled
  if (_tree.get_headers_clickable())
    set_allow_sorting(true);
}



bool TreeNodeViewImpl::create(TreeNodeView *self, mforms::TreeOptions opt)
{
  return new TreeNodeViewImpl(self, opt) != 0;
}

int TreeNodeViewImpl::add_column(TreeNodeView *self, TreeColumnType type, const std::string &name, int width, bool editable, bool attr)
{
  TreeNodeViewImpl* tree = self->get_data<TreeNodeViewImpl>();

  return tree->add_column(type, name, width, editable, attr);
}

void TreeNodeViewImpl::end_columns(TreeNodeView *self)
{
  TreeNodeViewImpl* tree = self->get_data<TreeNodeViewImpl>();

  tree->end_columns();
}

void TreeNodeViewImpl::clear(TreeNodeView *self)
{
  TreeNodeViewImpl* tree = self->get_data<TreeNodeViewImpl>();

  if (tree->_tree_store)
    tree->_tree_store->clear();
}

TreeNodeRef TreeNodeViewImpl::get_selected_node(TreeNodeView *self)
{
  TreeNodeViewImpl* tree = self->get_data<TreeNodeViewImpl>();
  
  if (tree->_tree.get_selection()->get_mode() == Gtk::SELECTION_MULTIPLE)
  {
     std::vector<Gtk::TreePath> path_selection = tree->_tree.get_selection()->get_selected_rows();
     if (path_selection.size() == 1)
       return TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path_selection[0]));
     else if (!path_selection.empty())
     {
       Gtk::TreePath path;
			 Gtk::TreeViewColumn *column;
       tree->_tree.get_cursor(path, column);
       if (!path.empty())
         return TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path));
       return TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path_selection[0]));
     }
  }
  else
    if (tree->_tree.get_selection()->get_selected())
    {
      const Gtk::TreePath path(tree->to_list_iter(tree->_tree.get_selection()->get_selected()));
      if (!path.empty())
        return TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path));
    }
  return TreeNodeRef(); 
}

std::list<TreeNodeRef> TreeNodeViewImpl::get_selection(TreeNodeView *self)
{
  TreeNodeViewImpl* tree = self->get_data<TreeNodeViewImpl>();
  std::list<TreeNodeRef> selection;
  
  if (tree->_tree.get_selection()->get_mode() == Gtk::SELECTION_MULTIPLE)
  {
    std::vector<Gtk::TreePath> path_selection;
    path_selection = tree->_tree.get_selection()->get_selected_rows();
    
    size_t size = path_selection.size();
    
    if (size > 0)
    {
      for(size_t index = 0; index < size; index++)
        selection.push_back(TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path_selection[index])));
    }
  }
  else
  {
    const Gtk::TreePath path(tree->to_list_iter(tree->_tree.get_selection()->get_selected()));
    if (!path.empty())
      selection.push_back(TreeNodeRef(new TreeNodeImpl(tree, tree->_tree_store, path)));
  }
  return selection;
}


void TreeNodeViewImpl::set_selected(TreeNodeView* self, TreeNodeRef node, bool flag)
{
  TreeNodeViewImpl* tree = self->get_data<TreeNodeViewImpl>();
  TreeNodeImpl *nodei = dynamic_cast<TreeNodeImpl*>(node.ptr());

  if (nodei)
  {
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

template<typename T>
int column_value_compare(const T &val1, const T &val2)
{
  int result = 0;
  if (val2 < val1) result = -1;
  else if (val2 > val1) result = 1;
  return result;
}

template<typename T>
int column_numeric_compare(const Gtk::TreeModel::iterator& it1, const Gtk::TreeModel::iterator& it2, Gtk::TreeModelColumn<T> *col)
{
  return column_value_compare((*it1).get_value(*col), (*it2).get_value(*col));
}

int column_string_compare(const Gtk::TreeModel::iterator& it1, const Gtk::TreeModel::iterator& it2, Gtk::TreeModelColumn<Glib::ustring> *col, int type)
{
  int result = 0;
  
  switch (type)
  {
    case StringColumnType:
    case StringLTColumnType:
    case IconColumnType:
    {
      std::string val1 = (*it1).get_value(*col).c_str();
      std::string val2 = (*it2).get_value(*col).c_str();
      result = base::string_compare(val2, val1, false);
      break;
    }
    case IntegerColumnType:
    case LongIntegerColumnType:
    {
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
    case NumberWithUnitColumnType:
    {
      double val1 = mforms::TreeNodeView::parse_string_with_unit((*it1).get_value(*col).c_str());
      double val2 = mforms::TreeNodeView::parse_string_with_unit((*it2).get_value(*col).c_str());
      result = column_value_compare(val1, val2);
      break;
    }
  }
  
  return result;
}

void TreeNodeViewImpl::set_allow_sorting(TreeNodeView* self, bool flag)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  impl->set_allow_sorting(flag);
}

void TreeNodeViewImpl::set_allow_sorting(bool flag)
{
  if (_tree.get_headers_visible())
    _tree.set_headers_clickable(flag);
  
  if (flag == false || !_tree_store)
    return;
    
  if (!_sort_model)
    _sort_model = Gtk::TreeModelSort::create(_tree_store);

  for (std::size_t i = 0; i < _tree.get_columns().size(); ++i)
  {
    Gtk::TreeViewColumn* col = _tree.get_column(i);
    Gtk::TreeModelColumnBase *mcol = _columns.columns[index_for_column(i)];
    
    if (col == NULL || mcol == NULL)
    {
      log_warning("Invalid column pointer[col:%s][mcol:%s]\n", col == NULL ? "NULL" : "NOT NULL", mcol == NULL ? "NULL" : "NOT NULL");
      continue;
    }

    col->signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &TreeNodeViewImpl::header_clicked),mcol,col));
    
    //  Align columns text depending on the visualization type
    float align = 0.0f;
    
    switch(get_owner()->get_column_type(i))
    {
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
    
    Glib::ListHandle<Gtk::CellRenderer*> renderers = col->get_cell_renderers();
    
    for (Glib::ListHandle<Gtk::CellRenderer*>::const_iterator iter = renderers.begin(); iter != renderers.end(); ++iter)
      (*iter)->set_alignment(align, 0);
    
    //  Set the proper compare method to the sorting functions depending on the storage type
    switch (mcol->type())
    {
      case G_TYPE_INT:
        _sort_model->set_sort_func(*mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<int>), (Gtk::TreeModelColumn<int>*)mcol));
        break;
      case G_TYPE_LONG:
        _sort_model->set_sort_func(*mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<long>), (Gtk::TreeModelColumn<long>*)mcol));
        break;
      case G_TYPE_INT64:
        _sort_model->set_sort_func(*mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<int64_t>), (Gtk::TreeModelColumn<int64_t>*)mcol));
        break;
      case G_TYPE_UINT:
        _sort_model->set_sort_func(*mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<unsigned int>), (Gtk::TreeModelColumn<unsigned int>*)mcol));
        break;
      case G_TYPE_ULONG:
        _sort_model->set_sort_func(*mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<unsigned long>), (Gtk::TreeModelColumn<unsigned long>*)mcol));
        break;
      case G_TYPE_UINT64:
        _sort_model->set_sort_func(*mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<uint64_t>), (Gtk::TreeModelColumn<uint64_t>*)mcol));
        break;
      case G_TYPE_FLOAT:
      case G_TYPE_DOUBLE:
        _sort_model->set_sort_func(*mcol, sigc::bind(sigc::ptr_fun(column_numeric_compare<double>), (Gtk::TreeModelColumn<double>*)mcol));
        break;
      case G_TYPE_STRING:
        _sort_model->set_sort_func(*mcol, sigc::bind(sigc::ptr_fun(column_string_compare), (Gtk::TreeModelColumn<Glib::ustring>*)mcol, get_owner()->get_column_type(i)));
        break;
      default:
        log_warning("Unknown column storage type");
        break;
    }
  }

  // temporarily disable selection change signal, gtk emits it when setting model
  _conn.disconnect();
  _tree.set_model(_sort_model);
  _conn = _tree.get_selection()->signal_changed().connect(sigc::mem_fun(dynamic_cast<TreeNodeView*>(owner), &TreeNodeView::changed));
}

void TreeNodeViewImpl::freeze_refresh(TreeNodeView* self, bool flag)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  Gtk::TreeView *tv = &(impl->_tree);

  if (tv->get_headers_visible())
    tv->set_headers_clickable(!flag);
    
  if (!flag)
  {
    tv->thaw_child_notify();
    //tv->set_model(impl->_tree_store);
  }
  else
  {
    tv->freeze_child_notify();
    //tv->unset_model();
  }
}

Gtk::TreeModel::iterator TreeNodeViewImpl::to_sort_iter(const Gtk::TreeModel::iterator &it)
{
  return (_tree.get_headers_clickable() && _sort_model) ? _sort_model->convert_child_iter_to_iter(it) : it;
}

Gtk::TreeModel::Path TreeNodeViewImpl::to_sort_path(const Gtk::TreeModel::Path &path)
{
  return (_tree.get_headers_clickable() && _sort_model) ? _sort_model->convert_child_path_to_path(path) : path;
}

Gtk::TreeModel::iterator TreeNodeViewImpl::to_list_iter(const Gtk::TreeModel::iterator &it)
{
  return (_tree.get_headers_clickable() && _sort_model) ? _sort_model->convert_iter_to_child_iter(it) : it;
}

Gtk::TreeModel::Path TreeNodeViewImpl::to_list_path(const Gtk::TreeModel::Path &path)
{
  return (_tree.get_headers_clickable() && _sort_model) ? _sort_model->convert_path_to_child_path(path) : path;
}

void TreeNodeViewImpl::header_clicked(Gtk::TreeModelColumnBase* cbase, Gtk::TreeViewColumn* col)
{
  if (!(col && cbase))
    return;

  // Get sort order if anything, if absent set to ASC
  void* data = col->get_data("sord");
  Gtk::SortType sort_order = (Gtk::SortType)((long)data);
  if (sort_order == Gtk::SORT_ASCENDING)
    sort_order = Gtk::SORT_DESCENDING;
  else
    sort_order = Gtk::SORT_ASCENDING;

  const std::vector<Gtk::TreeViewColumn*> cols = _tree.get_columns();
  for (int i = cols.size() - 1; i >= 0; --i)
  {
    if (cols[i] != col)
      cols[i]->set_sort_indicator(false);
  }

  // Call set_sort_column
  _sort_model->set_sort_column(*cbase, sort_order);
  col->set_sort_indicator(true);
  col->set_sort_order(sort_order);
  col->set_data("sord", (void*)sort_order);
}


void TreeNodeViewImpl::set_row_height(TreeNodeView *self, int height)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  impl->_row_height = height;
}


TreeNodeRef TreeNodeViewImpl::root_node(TreeNodeView *self)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  return impl->_root_node;
}

TreeSelectionMode TreeNodeViewImpl::get_selection_mode(TreeNodeView *self)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  switch (impl->_tree.get_selection()->get_mode())
  {
  case Gtk::SELECTION_BROWSE:
  case Gtk::SELECTION_EXTENDED:
    return TreeSelectMultiple;
  case Gtk::SELECTION_SINGLE:
  default:
    return TreeSelectSingle;
  }
  return TreeSelectSingle;
}

void TreeNodeViewImpl::set_selection_mode(TreeNodeView *self, TreeSelectionMode mode)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  switch (mode)
  {
  case TreeSelectSingle:
    impl->_tree.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
    break;
  case TreeSelectMultiple:
    impl->_tree.get_selection()->set_mode(Gtk::SELECTION_EXTENDED);
    break;
  }
}

void TreeNodeViewImpl::clear_selection(TreeNodeView *self)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  impl->_tree.get_selection()->unselect_all();
}


int TreeNodeViewImpl::row_for_node(TreeNodeView *self, TreeNodeRef node)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  TreeNodeImpl *nodei = dynamic_cast<TreeNodeImpl*>(node.ptr());
  if (impl && nodei)
  {
    if (impl->_flat_list)
    {
      if (!nodei->path().empty())
        return nodei->path().back();
    }
    else
      return calc_row_for_node(&impl->_tree, impl->tree_store()->get_iter(nodei->path()));
  }
  return -1;
}

mforms::TreeNodeRef TreeNodeViewImpl::find_node_at_row(const Gtk::TreeModel::Children &children, int &c, int row)
{
  for (Gtk::TreeIter last = children.end(), i = children.begin(); i != last; i++)
  {
    Gtk::TreePath path(*i);
    if (c == row)
      return TreeNodeRef(new TreeNodeImpl(this, _tree_store, path));
    c++;
    if (_tree.row_expanded(path))
    {
      Gtk::TreeRow trow = **i;
      TreeNodeRef ref = find_node_at_row(trow.children(), c, row);
      if (ref)
        return ref;
    }
  }
  return TreeNodeRef();
}


TreeNodeRef TreeNodeViewImpl::node_at_row(TreeNodeView *self, int row)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  if (impl && row >= 0)
  {
    Gtk::TreePath path;
    if (impl->_flat_list)
    {
      path.push_back(row);
      return TreeNodeRef(new TreeNodeImpl(impl, impl->tree_store(), path));
    }
    int i = 0;
    return impl->find_node_at_row(impl->tree_store()->children(), i, row);
  }
  return TreeNodeRef();
}


TreeNodeRef TreeNodeViewImpl::node_with_tag(TreeNodeView *self, const std::string &tag)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  if (impl->_tagmap_enabled)
  {
    std::map<std::string, Gtk::TreeRowReference>::iterator it;
    if ((it = impl->_tagmap.find(tag)) == impl->_tagmap.end())
      return TreeNodeRef();
    return TreeNodeRef(new TreeNodeImpl(impl, it->second));
  }
  throw std::logic_error("node_with_tag() requires tree to be created with TreeIndexOnTag");
}


void TreeNodeViewImpl::set_column_visible(TreeNodeView *self, int column, bool flag)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  Gtk::TreeViewColumn *col = impl->_tree.get_column(column);
  if (col)
    col->set_visible(flag);
}


bool TreeNodeViewImpl::get_column_visible(TreeNodeView *self, int column)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  Gtk::TreeViewColumn *col = impl->_tree.get_column(column);
  if (col)
    return col->get_visible();
  return false;
}


void TreeNodeViewImpl::set_column_title(TreeNodeView *self, int column, const std::string &title)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  Gtk::TreeViewColumn *col = impl->_tree.get_column(column);
  if (col)
  {
    dynamic_cast<Gtk::Label*>(col->get_widget())->set_text(title);
  }
}

mforms::DropPosition TreeNodeViewImpl::get_drop_position()
{
  Gtk::TreePath path;
  Gtk::TreeViewDropPosition pos;
  _tree.get_drag_dest_row(path, pos);

  switch(pos)
  {
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


void TreeNodeViewImpl::on_realize()
{
  // nasty workaround to allow context menu for tree headers
  for (int i = 0; i < (int)_tree.get_columns().size(); i++)
  {
    Gtk::Widget *w = _tree.get_column(i)->get_widget();
    while (w && !dynamic_cast<Gtk::Button*>(w))
      w = w->get_parent();
    if (dynamic_cast<Gtk::Button*>(w))
      w->signal_button_press_event().connect(sigc::bind(sigc::mem_fun(this, &TreeNodeViewImpl::on_header_button_event), i), false);
  }
}


void TreeNodeViewImpl::set_column_width(TreeNodeView *self, int column, int width)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  Gtk::TreeViewColumn *col = impl->_tree.get_column(column);
  if (col)
  {
    col->set_resizable(true);
    col->set_fixed_width(width);
  }
}


int TreeNodeViewImpl::get_column_width(TreeNodeView *self, int column)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  Gtk::TreeViewColumn *col = impl->_tree.get_column(column);
  if (col)
    return col->get_width();
  return 0;
}

mforms::TreeNodeRef TreeNodeViewImpl::node_at_position(TreeNodeView *self, base::Point position)
{
  TreeNodeViewImpl* impl = self->get_data<TreeNodeViewImpl>();
  Gtk::TreePath path;
  if (!impl->_tree.get_path_at_pos(position.x, position.y, path))
    return mforms::TreeNodeRef();

  return TreeNodeRef(new TreeNodeImpl(impl, impl->tree_store(), path));
}


void TreeNodeViewImpl::init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_treenodeview_impl.create= &TreeNodeViewImpl::create;
  f->_treenodeview_impl.root_node= &TreeNodeViewImpl::root_node;
  f->_treenodeview_impl.add_column= &TreeNodeViewImpl::add_column;
  f->_treenodeview_impl.end_columns= &TreeNodeViewImpl::end_columns;
  f->_treenodeview_impl.clear= &TreeNodeViewImpl::clear;
  f->_treenodeview_impl.get_selection_mode= &TreeNodeViewImpl::get_selection_mode;
  f->_treenodeview_impl.set_selection_mode= &TreeNodeViewImpl::set_selection_mode;
  f->_treenodeview_impl.clear_selection= &TreeNodeViewImpl::clear_selection;
  f->_treenodeview_impl.get_selected_node= &TreeNodeViewImpl::get_selected_node;
  f->_treenodeview_impl.get_selection= &TreeNodeViewImpl::get_selection;
  f->_treenodeview_impl.set_selected= &TreeNodeViewImpl::set_selected;
  f->_treenodeview_impl.set_allow_sorting= &TreeNodeViewImpl::set_allow_sorting;
  f->_treenodeview_impl.freeze_refresh= &TreeNodeViewImpl::freeze_refresh;
  f->_treenodeview_impl.set_row_height= &TreeNodeViewImpl::set_row_height;
  f->_treenodeview_impl.node_at_row= &TreeNodeViewImpl::node_at_row;
  f->_treenodeview_impl.node_with_tag= &TreeNodeViewImpl::node_with_tag;
  f->_treenodeview_impl.row_for_node= &TreeNodeViewImpl::row_for_node;
  f->_treenodeview_impl.set_column_visible= &TreeNodeViewImpl::set_column_visible;
  f->_treenodeview_impl.get_column_visible= &TreeNodeViewImpl::get_column_visible;
  f->_treenodeview_impl.set_column_title= &TreeNodeViewImpl::set_column_title;
  f->_treenodeview_impl.set_column_width= &TreeNodeViewImpl::set_column_width;
  f->_treenodeview_impl.get_column_width= &TreeNodeViewImpl::get_column_width;
  f->_treenodeview_impl.node_at_position = &TreeNodeViewImpl::node_at_position;
}

}
}
