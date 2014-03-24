/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _TREE_MODEL_H_
#define _TREE_MODEL_H_

/*
 * TreeModel is the base class for all list or tree based
 * backend classes. 
 */
#include <algorithm>

#include <grtpp.h>
#include "grt/icon_manager.h"
#include "grt/common.h"
#include "base/ui_form.h" // for menu stuff
#include "base/trackable.h"
#include "base/threading.h"

#include "wbpublic_public_interface.h"
#include <boost/shared_ptr.hpp>
#include <ctype.h>

#include <set>
#include <algorithm>

namespace mforms {
  class MenuBase;
}

namespace bec
{
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
  */  
  // TODO: Add threshold so we do not accumulate megabytes of allocated index vectors
  template <typename T>
  class WBPUBLICBACKEND_PUBLIC_FUNC Pool
  {
    private:
      Pool(const Pool&)
      {
        throw std::runtime_error("Copy of Pool forbidden");
      }
      Pool& operator=(const Pool&)
      {
        throw std::runtime_error("Assignment of Pool forbidden");
      }
    public:
      static void delete_object(T* obj)
      {
        delete obj;
      }
  
      Pool()
          : _pool(4)
      {}
    
      ~Pool()
      {
        {
          base::MutexLock lock(_sync);
          std::for_each(_pool.begin(), _pool.end(), Pool::delete_object);
        }
      }
    
      T* get()
      {
        T* item = 0;
        try
        {
          base::MutexLock lock(_sync);
          if ( _pool.size() > 0 )
          { 
            item = _pool.back();
            _pool.pop_back();
          }
        }
        catch (...)
        {
          //TODO: check when pop may throw
        }
      
        if ( !item )
        {
          item = new T;
        }
      
        return item;
      }
    
      void put(T* item)
      {
        base::MutexLock lock(_sync);
        _pool.push_back(item);
      }
  
    private:
      std::vector<T*>  _pool;
      base::Mutex          _sync; //! Protect against concurrent access within threads
  };

  /**
    \class NodeId
    \brief descibes path to a node starting from root for a Tree or it will contain an index (single entry) for List

    Imagine we have a tree with a two root nodes and two children of each root. So to address first child of the
    first root node we need to have a path like that: "0.0". To address the second child of the first root node:
    "0.1"
  */
  struct WBPUBLICBACKEND_PUBLIC_FUNC NodeId 
  {
    typedef std::string*                uid;   //!< To map short-living NodeId path to a persistent value
                                               //!< This is needed for Gtk::TreeModel iterators
    typedef std::vector<int>          Index; 
    static Pool<Index>                 *_pool; //!< Pool of allocated std::vectors (Index)
    Index                              *index; //!< Path itself

    static Pool<Index>* pool()
    {
      return _pool ? _pool : (_pool = new Pool<Index>);
    }

    // NodeId will get index from the pool
    NodeId()
        : index(0)
    {
      index = pool()->get();
    }

    NodeId(const NodeId &copy)
        : index(0)
    {
      index = pool()->get();
      if ( copy.index )
        *index = *copy.index;
    }

    NodeId(const int i)
        : index(0)
    {
      index = pool()->get();
      index->push_back(i);
    }

    NodeId(const std::string &str)
        : index(0)
    {
      index = pool()->get();
      try
      {
        const char* chr = str.c_str();
        const int size = (int)str.length();
        std::string num;
        num.reserve(size);
        
        for ( int i = 0; i < size; i++ )
        {
          if ( isdigit(chr[i]) )
            num.push_back(chr[i]);
          else if ( '.' == chr[i] || ':' == chr[i] )
          {
            if ( !num.empty() )
            {
              index->push_back(atoi(num.c_str()));
              num.clear();
            }
          }
          else
            throw std::runtime_error("Wrong format of NodeId");
        }
        
        if ( !num.empty() )
          index->push_back(atoi(num.c_str()));
      }
      catch (...)
      {
        index->clear();
        pool()->put(index);
        index = 0;
        throw;
      }
    }

    ~NodeId() 
    {
      index->clear();
      pool()->put(index);
      index = 0;
    }

    inline NodeId &operator = (const NodeId &node)
    {
      *index = *node.index;
      
      return *this;
    }

    bool operator < (const NodeId &r) const
    {
      bool ret = true;
      
      if (index && r.index)
      {
        // Shorter nodeids must go before longer. For example in a list ["0.1", "0.1.1"]
        // longer nodeid is a subnode of the "0.1", so in case of deletion subnode deleted first
        // (That's true only when traversing list from the end)
        if (index->size() < r.index->size())
          ret = true;
        else if (index->size() > r.index->size())
          ret = false;
        else
        {
          // It is assumed that this nodeid is less than @r. Walk index vectors. If current value
          // from this->index is less than or equal to the corresponding value from r.index the pair is skipped
          // as it complies with assumption that this node is less than @r.
          // Once current value becomes greater than @r's the assumption about current node's 
          // less than @r becomes false, therefore this node is greater than @r.
          const int size = index->size();
          for (int i = 0; i < size; ++i)
          {
            if ((*index)[i] > (*r.index)[i])
            {
              ret = false;
              break;
            }
          }
        }
      }
      
      return ret;
    }

    inline bool operator == (const NodeId &node) const
    {
      return equals(node);
    }

    bool equals(const NodeId &node) const
    {
      // TODO: Check if we need to compare content of the index and node.index vectors
      return index && node.index && *node.index == *index;
    }

    int depth() const
    {
      return (int)index->size();
    }

    inline int& operator[] (unsigned int i) const
    {
      if ( i < index->size() )
        return const_cast<int&>((*index)[i]);
      throw std::range_error("invalid index");
    }

    inline int end() const
    {
      if ( index->size() > 0 )
        return (*index)[index->size() - 1];
      throw std::logic_error("invalid node id. NodeId::back applied to an empty NodeId instance.");
    }
    
    inline int back() const
    {
      return end();
    }
    
    // Set leaf to the previous index, e.g. for node with path "1.3.2" it will become "1.3.1".
    inline bool previous() const
    {
      bool ret = false;
      if ( index->size() > 0 )
      {
        --((*index)[index->size() - 1]);
        ret = true;
      }
      return ret;
    }
    
    // Set leaf to the next index, e.g. for node with path "1.3.2" it will become "1.3.3".
    inline bool next() const
    {
      bool ret = false;
      if ( index->size() > 0 )
      {
        ++((*index)[index->size() - 1]);
        ret = true;
      }
      return ret;
    }

    inline bool is_valid() const
    {
      return index->size() != 0;
    }
    
    inline NodeId parent() const
    {
      if (depth() < 2)
        return NodeId();
      NodeId copy(*this);
      copy.index->pop_back();
      return copy;
    }

    inline std::string repr(const char separator = '.') const
    {
      std::string r = "";
      const int depth = (int)index->size();
      for (int i= 0; i < depth; i++)
      {
        char buf[30];
        g_snprintf(buf, sizeof(buf), "%i", (*index)[i]);
        if (i > 0)
          r= r + separator + buf;
        else
          r= buf;
      }
      return r;
    }

    inline NodeId &append(const int i)
    {
      if (i < 0)
        throw std::invalid_argument("negative node index is invalid");

      index->push_back(i);

      return *this;
    }

    inline NodeId &prepend(const int i)
    {
      if (i < 0)
        throw std::invalid_argument("negative node index is invalid");

      index->insert(index->begin(), i);

      return *this;
    }
  };

  //----------------------------------------------------------------------------
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
  class NodeIds
  {
    public:
      NodeIds() {}

      //! Resets map of NodeId paths to uid
      void flush();

      //! Maps path with type of std::string from NodeId. This function is used for
      //! convience. See map_node_id(const NodeId&)
      NodeId::uid map_node_id(const std::string& path_from_nodeid);
      NodeId::uid map_node_id(const NodeId& nid)
      {
        return map_node_id(nid.repr());
      }

      //! Reverse mapping from 'uid' to a path
      const std::string& map_node_id(const NodeId::uid nodeid);

    private:
      typedef std::set<std::string> Map;

      Map          _map;
  };

  //------------------------------------------------------------------------------
  inline void NodeIds::flush()
  {
    _map.clear();
  }

  //------------------------------------------------------------------------------
  inline NodeId::uid NodeIds::map_node_id(const std::string& path_from_nodeid)
  {
    const Map::const_iterator it = _map.find(path_from_nodeid);
    if ( _map.end() != it )
      return (const NodeId::uid)&(*it);
    else
    {
      //TODO: make a faster way. Probably we can use item from insert
      _map.insert(path_from_nodeid);
      return map_node_id(path_from_nodeid);
    }
  }

  //------------------------------------------------------------------------------
  inline const std::string& NodeIds::map_node_id(const NodeId::uid nodeid)
  {
    // Note that dereference of nodeid is dangerous after flush was called
    // That should be protected by stamp approach in TreeModel wrapper.
    static std::string empty;
    return nodeid ? *nodeid : empty;
  }
  
  /** Base list model class.
   *
   * @ingroup begrt
   */
  class WBPUBLICBACKEND_PUBLIC_FUNC ListModel : public base::trackable
  {
   private:
    NodeIds               _nodeid_map;
    boost::signals2::signal<void (bec::NodeId, int)>    _tree_changed_signal;
    
   public:
    virtual ~ListModel() {};

    virtual int count()= 0;
    virtual NodeId get_node(int index);
    virtual bool has_next(const NodeId &node);
    virtual NodeId get_next(const NodeId &node);

    boost::signals2::signal<void (bec::NodeId, int)>* tree_changed_signal(){return &_tree_changed_signal;}

    void tree_changed(int old_child_count = -1, const bec::NodeId &parent = bec::NodeId())
    {
      _tree_changed_signal(parent, old_child_count);
      _nodeid_map.flush();
    }

    NodeId::uid nodeid_to_uid(const NodeId& nodeid)
    {
      return _nodeid_map.map_node_id(nodeid);
    }

    NodeId::uid nodeid_path_to_uid(const std::string& path)
    {
      return _nodeid_map.map_node_id(path);
    }

    const std::string& nodeuid_to_path(const NodeId::uid nodeuid)
    {
      return _nodeid_map.map_node_id(nodeuid);
    }
//    virtual void set_data(NodeId node, void *data)= 0;
//    virtual void *get_data(NodeId node)= 0;

    virtual bool get_field(const NodeId &node, int column, std::string &value);
    virtual bool get_field(const NodeId &node, int column, int &value);
    virtual bool get_field(const NodeId &node, int column, long long &value);
    virtual bool get_field(const NodeId &node, int column, bool &value);
    virtual bool get_field(const NodeId &node, int column, double &value);

    virtual bool get_field_repr(const NodeId &node, int column, std::string &value) { return get_field(node, column, value); }

    // representation of the field as a GRT value
    virtual grt::ValueRef get_grt_value(const NodeId &node, int column);

    virtual std::string get_field_description(const NodeId &node, int column);
    virtual IconId get_field_icon(const NodeId &node, int column, IconSize size);

    virtual void refresh()= 0;
    virtual void refresh_node(const NodeId &node) {}

    virtual void reset() {} //!

    virtual void reorder(const NodeId &node, int index) { throw std::logic_error("not implemented"); }
    void reorder_up(const NodeId &node);
    void reorder_down(const NodeId &node);

    virtual bool activate_node(const NodeId &node) { throw std::logic_error("not implemented"); return false; }

    // Parent can be NULL if the root node is meant.
    virtual void update_menu_items_for_nodes(mforms::MenuBase *parent, const std::vector<NodeId> &nodes) { };

    // Deprecated. Use update_menu_items_for_nodes for new code. MenuItemList and related code will go.
    virtual MenuItemList get_popup_items_for_nodes(const std::vector<NodeId> &nodes) { return MenuItemList(); }
    //! Returns true if item was processed by BE, false - BE is unable to process command and FE should do it
    virtual bool activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &nodes) { throw std::logic_error("not implemented"); }

    virtual bool can_delete_node(const NodeId &node) { return false; }
    virtual bool delete_node(const NodeId &node) { throw std::logic_error("not implemented"); }

    // for editable lists only
    virtual grt::Type get_field_type(const NodeId &node, int column);


    virtual bool set_field(const NodeId &node, int column, const std::string &value);
    virtual bool set_field(const NodeId &node, int column, int value);
    virtual bool set_field(const NodeId &node, int column, long long value);
    virtual bool set_field(const NodeId &node, int column, double value);

    virtual bool set_convert_field(const NodeId &node, int column, const std::string &value);

    //! By default we do not allow to edit items.
    //! This is a recently added method. It will replace occasionally used is_renameable
    virtual bool is_editable(const NodeId& node) const { return false; }
    virtual bool is_deletable(const NodeId& node) const { return false; }
    virtual bool is_copyable(const NodeId& node) const { return false; }

    // Indicates if a given node is to be visually exposed (e.g. an active schema in a schema tree).
    virtual bool is_highlighted(const NodeId& node) { return false; }

    virtual void dump(int show_field);
  protected:
    // for internal use only
    virtual bool get_field_grt(const NodeId &node, int column, grt::ValueRef &value);

    grt::ValueRef parse_value(grt::Type type, const std::string &value);
  };



  /** Base tree model class.
   *
   * @ingroup begrt
   */
  class WBPUBLICBACKEND_PUBLIC_FUNC TreeModel : public ListModel
  {
  public:
    virtual int count();
    virtual NodeId get_node(int index);

    virtual NodeId get_root() const;
    virtual int get_node_depth(const NodeId &node);
    inline NodeId get_parent(const NodeId &node) const { return node.parent(); }

    virtual int count_children(const NodeId &parent)= 0;
    virtual NodeId get_child(const NodeId &parent, int index) { return NodeId(parent).append(index); }
    virtual bool has_next(const NodeId &node);
    virtual NodeId get_next(const NodeId &node);

    virtual bool is_expandable(const NodeId &node_id);
    virtual bool expand_node(const NodeId &node);
    virtual void collapse_node(const NodeId &node);
    virtual bool is_expanded(const NodeId &node);

    void save_expand_info(const std::string &path);


    virtual void dump(int show_field);
  protected:

  };



  class WBPUBLICBACKEND_PUBLIC_FUNC GridModel : public ListModel
  {
  public:
    typedef boost::shared_ptr<GridModel> Ref;

    enum ColumnType
    {
      UnknownType,
      StringType,
      NumericType,
      FloatType,
      DatetimeType,
      BlobType
    };

    virtual int get_column_count() const= 0;
    virtual std::string get_column_caption(int column)= 0;
    virtual ColumnType get_column_type(int column)= 0;
    virtual bool is_readonly() const { return false; } //!
    virtual std::string readonly_reason() const { return std::string(); } //!
    virtual bool is_field_null(const bec::NodeId &node, int column) { return false; } //!
    virtual bool set_field_null(const bec::NodeId &node, int column) { return set_convert_field(node, column, ""); } //!
    virtual void set_edited_field(int row_index, int col_index) { }

  public:
    typedef size_t ColumnId;
    typedef std::list<std::pair<ColumnId, int> > SortColumns;
    virtual void sort_by(ColumnId column, int direction, bool retaining) {}
    virtual SortColumns sort_columns() const { return SortColumns(); }

  public:
    virtual int floating_point_visible_scale() { return 3; }
  };

};

#ifdef _MSC_VER
#pragma make_public(::bec::ListModel)
#pragma make_public(::bec::TreeModel)
#pragma make_public(::bec::NodeId)
#pragma make_public(::bec::GridModel)
#endif


#endif /* _TREE_MODEL_H_ */
