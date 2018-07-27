/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

/*
 * TreeModel is the base class for all list or tree based
 * backend classes.
 */
#include <algorithm>

#include "grt.h"
#include "grt/icon_manager.h"
#include "grt/common.h"
#include "base/ui_form.h" // for menu stuff
#include "base/trackable.h"
#include "base/threading.h"

#include "wbpublic_public_interface.h"
#include <ctype.h>

#include <set>
#include <algorithm>

namespace mforms {
  class MenuBase;
}

namespace bec {
  /** A tree node index.
   * Used to index nodes in a tree or list backend that inherits from ListModel or TreeModel.
   * A nodeId is like an index, in the simplest case of a flat list, it will contain
   * a single integer index. For trees, it will contain one index for each parent node
   * until the leaf node it refers to.
   *
   * Since nodeIds are just indices, a nodeId may not point to the same item after
   * the list/tree contents change.
   *
   * @ingroup begrt
   */

  /**
    \class NodeId
    \brief descibes path to a node starting from root for a Tree or it will contain an index (single entry) for List

    Imagine we have a tree with a two root nodes and two children of each root. So to address first child of the
    first root node we need to have a path like that: "0.0". To address the second child of the first root node:
    "0.1"
  */
  struct WBPUBLICBACKEND_PUBLIC_FUNC NodeId {
    typedef std::string *uid; //!< To map short-living NodeId path to a persistent value
                              //!< This is needed for Gtk::TreeModel iterators
    typedef std::vector<size_t> Index;
    Index index; //!< Path itself

    NodeId();
    NodeId(const NodeId &copy);
    NodeId(size_t i);
    NodeId(const std::string &str);
    ~NodeId();

    inline NodeId &operator=(const NodeId &node) {
      index = node.index;

      return *this;
    }

    bool operator<(const NodeId &r) const;

    inline bool operator==(const NodeId &node) const {
      return equals(node);
    }

    bool equals(const NodeId &node) const;

    inline size_t depth() const {
      return index.size();
    }

    size_t &operator[](size_t i);
    const size_t &operator[](size_t i) const;

    size_t end() const;
    inline size_t back() const {
      return end();
    }

    bool previous();
    bool next();

    inline bool is_valid() const {
      return !index.empty();
    }

    NodeId parent() const;
    std::string description() const;
    std::string toString(const char separator = '.') const;

    NodeId &append(size_t i);
    NodeId &prepend(size_t i);
  };

  //----------------------------------------------------------------------------

  /**
    \class NodeIds
    \brief Mapper of short-living NodeId to a persistent item

    This class was added cause GtkTreeIter which is used in TreeModel for Gtk::TreeView
    has only three int-size fields describing iterator state. And it is not possible to fit entire NodeId
    there, moreover GtkTreeIter along with Gtk::TreeIter have no indication when iterator is
    destroyed. So the was a need to have something small which can be fit into those fields.
    Lifetime of the items named 'uid' is defined by the initial mapping when the uid is created
    and the issue of NodeIds::flush call. The mapped entity 'uid' is a pointer to a std::string stored
    in the std::set. For example if we need to have an item which lifetime is longer that NodeId("1.2.3")
    we should obtain mapped uid of the NodeId("1.2.3"). That mapped item - uid will be a pointer
    to a string in the std::set and the string itself will store "1.2.3". Having that uid allows us to get back
    path which can be stored between NodeId creations. That is used for example when we obtain Gtk::TreeIter
    and need to move to the next node in the Tree thus advance iterator.
    It is not advisable to dereference 'uid' to obtain std::string from std::string*, as future implementation
    may change that.
  */
  class NodeIds {
  public:
    NodeIds() {
    }

    //! Resets map of NodeId paths to uid
    void flush();

    //! Maps path with type of std::string from NodeId. This function is used for
    //! convenience. See map_node_id(const NodeId&)
    NodeId::uid map_node_id(const std::string &path_from_nodeid);
    NodeId::uid map_node_id(const NodeId &nid) {
      return map_node_id(nid.toString());
    }

    //! Reverse mapping from 'uid' to a path
    const std::string &map_node_id(const NodeId::uid nodeid);

  private:
    typedef std::set<std::string> Map;

    Map _map;
  };

  //------------------------------------------------------------------------------
  inline void NodeIds::flush() {
    _map.clear();
  }

  //------------------------------------------------------------------------------
  inline NodeId::uid NodeIds::map_node_id(const std::string &path_from_nodeid) {
    Map::const_iterator it = _map.find(path_from_nodeid);
    if (_map.end() != it)
      return (NodeId::uid) & (*it);
    else {
      // TODO: make a faster way. Probably we can use item from insert
      _map.insert(path_from_nodeid);
      return map_node_id(path_from_nodeid);
    }
  }

  //------------------------------------------------------------------------------
  inline const std::string &NodeIds::map_node_id(const NodeId::uid nodeid) {
    // Note that dereference of nodeid is dangerous after flush was called
    // That should be protected by stamp approach in TreeModel wrapper.
    static std::string empty;
    return nodeid ? *nodeid : empty;
  }

  /** Base list model class.
   */
  class WBPUBLICBACKEND_PUBLIC_FUNC ListModel : public base::trackable {
  private:
    NodeIds _nodeid_map;
    boost::signals2::signal<void(bec::NodeId, int)> _tree_changed_signal;

  public:
    typedef size_t ColumnId;
    typedef size_t RowId;

    virtual ~ListModel(){};

    virtual size_t count() = 0;
    virtual NodeId get_node(size_t index);
    virtual bool has_next(const NodeId &node);
    virtual NodeId get_next(const NodeId &node);

    boost::signals2::signal<void(bec::NodeId, int)> *tree_changed_signal() {
      return &_tree_changed_signal;
    }

    void tree_changed(int old_child_count = -1, const bec::NodeId &parent = bec::NodeId()) {
      _tree_changed_signal(parent, old_child_count);
      _nodeid_map.flush();
    }

    NodeId::uid nodeid_to_uid(const NodeId &nodeid) {
      return _nodeid_map.map_node_id(nodeid);
    }

    NodeId::uid nodeid_path_to_uid(const std::string &path) {
      return _nodeid_map.map_node_id(path);
    }

    const std::string &nodeuid_to_path(const NodeId::uid nodeuid) {
      return _nodeid_map.map_node_id(nodeuid);
    }
    virtual bool get_field(const NodeId &node, ColumnId column, std::string &value);
    virtual bool get_field(const NodeId &node, ColumnId column, ssize_t &value);
    virtual bool get_field(const NodeId &node, ColumnId column, bool &value);
    virtual bool get_field(const NodeId &node, ColumnId column, double &value);

    virtual bool get_field_repr(const NodeId &node, ColumnId column, std::string &value) {
      return get_field(node, column, value);
    }

    // representation of the field as a GRT value
    virtual grt::ValueRef get_grt_value(const NodeId &node, ColumnId column);

    virtual std::string get_field_description(const NodeId &node, ColumnId column);
    virtual IconId get_field_icon(const NodeId &node, ColumnId column, IconSize size);

    virtual void refresh() = 0;
    virtual void refresh_node(const NodeId &node) {
    }

    virtual void reset() {
    } //!

    virtual void reorder(const NodeId &node, size_t index) {
      throw std::logic_error("not implemented");
    }
    void reorder_up(const NodeId &node);
    void reorder_down(const NodeId &node);

    virtual bool activate_node(const NodeId &node) {
      throw std::logic_error("not implemented");
      return false;
    }

    // Parent can be NULL if the root node is meant.
    virtual void update_menu_items_for_nodes(mforms::MenuBase *parent, const std::vector<NodeId> &nodes){};

    // Deprecated. Use update_menu_items_for_nodes for new code. MenuItemList and related code will go.
    virtual MenuItemList get_popup_items_for_nodes(const std::vector<NodeId> &nodes) {
      return MenuItemList();
    }
    //! Returns true if item was processed by BE, false - BE is unable to process command and FE should do it
    virtual bool activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &nodes) {
      throw std::logic_error("not implemented");
    }

    virtual bool can_delete_node(const NodeId &node) {
      return false;
    }
    virtual bool delete_node(const NodeId &node) {
      throw std::logic_error("not implemented");
    }

    // for editable lists only
    virtual grt::Type get_field_type(const NodeId &node, ColumnId column);

    virtual bool set_field(const NodeId &node, ColumnId column, const std::string &value);
    virtual bool set_field(const NodeId &node, ColumnId column, ssize_t value);
    virtual bool set_field(const NodeId &node, ColumnId column, double value);

    virtual bool set_convert_field(const NodeId &node, ColumnId column, const std::string &value);

    //! By default we do not allow to edit items.
    //! This is a recently added method. It will replace occasionally used is_renameable
    virtual bool is_editable(const NodeId &node) const {
      return false;
    }
    virtual bool is_deletable(const NodeId &node) const {
      return false;
    }
    virtual bool is_copyable(const NodeId &node) const {
      return false;
    }

    // Indicates if a given node is to be visually exposed (e.g. an active schema in a schema tree).
    virtual bool is_highlighted(const NodeId &node) {
      return false;
    }

    virtual void dump(int show_field);

  protected:
    // for internal use only
    virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);

    grt::ValueRef parse_value(grt::Type type, const std::string &value);
  };

  /** Base tree model class.
   */
  class WBPUBLICBACKEND_PUBLIC_FUNC TreeModel : public ListModel {
  public:
    virtual size_t count();
    virtual NodeId get_node(size_t index);

    virtual NodeId get_root() const;
    virtual size_t get_node_depth(const NodeId &node);
    inline NodeId get_parent(const NodeId &node) const {
      return node.parent();
    }

    virtual size_t count_children(const NodeId &parent) = 0;
    virtual NodeId get_child(const NodeId &parent, size_t index) {
      return NodeId(parent).append(index);
    }
    virtual bool has_next(const NodeId &node);
    virtual NodeId get_next(const NodeId &node);

    virtual bool is_expandable(const NodeId &node_id);
    virtual bool expand_node(const NodeId &node);
    virtual void collapse_node(const NodeId &node);
    virtual bool is_expanded(const NodeId &node);

    void save_expand_info(const std::string &path);

    virtual void dump(int show_field);
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC GridModel : public ListModel {
  public:
    typedef std::shared_ptr<GridModel> Ref;

    enum ColumnType { UnknownType, StringType, NumericType, FloatType, DatetimeType, BlobType };

    virtual size_t get_column_count() const = 0;
    virtual std::string get_column_caption(ColumnId column) = 0;
    virtual ColumnType get_column_type(ColumnId column) = 0;
    virtual bool is_readonly() const {
      return false;
    } //!
    virtual std::string readonly_reason() const {
      return std::string();
    } //!
    virtual bool is_field_null(const bec::NodeId &node, ColumnId column) {
      return false;
    } //!
    virtual bool set_field_null(const bec::NodeId &node, ColumnId column) {
      return set_convert_field(node, column, "");
    } //!
    virtual void set_edited_field(RowId row_index, ColumnId col_index) {
    }

  public:
    typedef std::list<std::pair<ColumnId, int> > SortColumns;
    virtual void sort_by(ColumnId column, int direction, bool retaining) {
    }
    virtual SortColumns sort_columns() const {
      return SortColumns();
    }

  public:
    virtual int floating_point_visible_scale() {
      return 3;
    }
  };
};
