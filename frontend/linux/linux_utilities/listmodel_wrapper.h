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

//!
//! \addtogroup linuxui Linux UI
//! @{
//!

#ifndef __LISTMODEL_WRAPPER_H__
#define __LISTMODEL_WRAPPER_H__

// use IconManager to retrive icon file
// reuse wb_icons to get actual Pixbufs
// ListModel get_field obtains value of the given field
// NodeId is an index to a value in some data structure inside the backend
// It seems that UI model should be build in advance, as it is not possible to know
// what types are for columns

#include <gtkmm/treemodel.h>
#include <gtkmm/treeview.h>
#include <gtkmm/iconview.h>
#include <gtkmm/liststore.h>

#include "base/trackable.h"
#include "grt/tree_model.h"
#include "image_cache.h"
#include "wbdbg.h"
#include <sstream>

//==============================================================================
//
//==============================================================================
template <int T>
struct Pow {
  enum { Value = 256 * Pow<T - 1>::Value };
};

template <>
struct Pow<0> {
  enum { Value = 1 };
};

//==============================================================================
//
//==============================================================================
struct Index {
  struct Info {
    unsigned int mode : 2;
    unsigned int stamp : 6;
  };

public:
  typedef std::set<std::string> ExternalMap;
  enum {
    K = 3,
    StampMax = 0x3f,
    ModeMask = 0xC0,
    MaxDepth = (sizeof(GtkTreeIter) - 1) / K,
    MaxValue = Pow<K>::Value - 1
  };
  enum Mode { Internal = 0x01, External = 0x02, ListNode = 0x03 };

  Index(GtkTreeIter* it, const bec::NodeId& node);
  Index(const GtkTreeIter* it);

  int stamp() const;
  void stamp(const int st);
  bool cmp_stamp(const int st) const;

  Mode mode() const {
    return (Mode)(((Info*)_raw_data)->mode);
  }
  void mode(const Mode m) {
    ((Info*)_raw_data)->mode = m;
  }

  bec::NodeId to_node() const;
  static void reset_iter(GtkTreeIter* it);

private:
  int word(const int w) const;
  void word(const int w, const int v);

  char* _raw_data;
  static ExternalMap _ext_map;
  std::string* _ext;
};

//! \brief if a column created by TreeModelWrapper should be read only or editable
enum { READONLY_COLUMN = 0, EDITABLE_COLUMN = 1 };

typedef Gtk::TreeModelColumn<int> IntColumn;
typedef Gtk::TreeModelColumn<double> DoubleColumn;
typedef Gtk::TreeModelColumn<Glib::ustring> StringColumn;

class ListModelWrapper;

Gtk::TreeModel::Path node2path(const ::bec::NodeId& node);

//! Enums to define columns view and behaviour. These are used in various ColumnsModel::append_*_column
enum Editable { RO = 0, EDITABLE = 1, EDITABLE_WO_FIRST = 2 };
//! Enums to define columns view and behaviour. These are used in various ColumnsModel::append_*_column
enum Iconic { NO_ICON = 0, WITH_ICON = 1 };
enum ToggleAction { TOGGLE_BY_WRAPPER, TOGGLE_BY_GTKMM };

//! \brief generic model for a TreeModelWrapper
//!
//! Using ColumnsModel we do not need to create models for Gtk::TreeView in separate classes.
//! With ColumnsModel it is to add columns to a model.
class ColumnsModel : public Gtk::TreeModel::ColumnRecord {
  class ComboColumns : public Gtk::TreeModel::ColumnRecord {
  public:
    ComboColumns() {
      add(name);
    }

    Gtk::TreeModelColumn<Glib::ustring> name;
  } _combo_columns;

public:
  ColumnsModel(ListModelWrapper* tmw, Gtk::TreeView* treeview)
    : Gtk::TreeModel::ColumnRecord(), _tmw(tmw), _treeview(treeview) {
  }
  ~ColumnsModel();

  void reset(const bool cleanup_only_self = false);

  void add_model_column(Gtk::TreeModelColumnBase* col, int bec_tm_idx);

  const StringColumn& set_text_column(const int bec_tm_idx, const bool editable, Gtk::IconView* iv);

  //! \brief adds a column that will contain the description for the column index
  void add_tooltip_column(int bec_tm_idx);

  //! \brief appends integer column named @name
  //!
  //! append_int_column adds column which can store integers. Parameter bec_tm_idx is used to map gtk's indices
  //! to a bec::TreeModel indices. E.g. when adding combo column two
  //!
  const IntColumn& append_int_column(const int bec_tm_idx, const std::string& name, const Editable editable = RO);
  const DoubleColumn& append_double_column(const int bec_tm_idx, const std::string& name, const Editable editable = RO);
  void disable_edit_first_row(Gtk::CellRenderer* cell, const Gtk::TreeIter& iter);
  const StringColumn& append_string_column(const int bec_tm_idx, const std::string& name, const Editable editable = RO,
                                           const Iconic have_icon = NO_ICON);
  const StringColumn& append_markup_column(const int bec_tm_idx, const std::string& name,
                                           const Iconic have_icon = NO_ICON);
  const StringColumn& append_combo_column(const int bec_tm_idx, const std::string& name,
                                          Glib::RefPtr<Gtk::ListStore> list_w, const Editable editable = RO,
                                          bool popup_only = false);
  //! TOGGLE_BY_* are temp workaround for gtkmm's _connect_auto_store_editable_signal_handler
  //! not always working with our ListModelWrapper. Sometimes we have double setting(reverting) of a value in
  //! a BE model thus leaving value the same ( !(!value)). Needs further digging
  int append_check_column(const int bec_tm_idx, const std::string& name, const Editable Editable = RO,
                          const ToggleAction action = TOGGLE_BY_WRAPPER);

  int add_generic_column(const int bec_tm_idx, Gtk::TreeModelColumnBase* column, Gtk::TreeViewColumn* vcolumn);

  int ui2bec(const int index_of_ui_column) const;

  Glib::RefPtr<Gtk::ListStore> create_model(const grt::StringListRef& source) {
    Glib::RefPtr<Gtk::ListStore> list_w(Gtk::ListStore::create(_combo_columns));

    grt::StringListRef::const_iterator last = source.end();
    for (grt::StringListRef::const_iterator iter = source.begin(); iter != last; ++iter) {
      Gtk::TreeRow row = *list_w->append();
      row[_combo_columns.name] = **iter;
    }

    return list_w;
  }

  Glib::RefPtr<Gtk::ListStore> create_model(const std::vector<std::string>& source) {
    Glib::RefPtr<Gtk::ListStore> list_w(Gtk::ListStore::create(_combo_columns));

    std::vector<std::string>::const_iterator last = source.end();
    for (std::vector<std::string>::const_iterator iter = source.begin(); iter != last; ++iter) {
      Gtk::TreeRow row = *list_w->append();
      row[_combo_columns.name] = *iter;
    }

    return list_w;
  }

private:
  std::list<Gtk::TreeModelColumnBase*> _columns;
  ListModelWrapper* _tmw;
  Gtk::TreeView* _treeview;
  std::vector<int> _ui2bec; //! map ui indices to a bec ones
  void add_bec_index_mapping(const int bec_tm_index);

  //    void toggled(const Glib::ustring& path, Gtk::CellRendererToggle* cell);
};

template <typename T>
inline T convert(const Glib::ustring& str) {
  throw std::logic_error(std::string("Add conversion for ") + typeid(T).name());
}

template <>
inline int convert(const Glib::ustring& str) {
  return atoi(str.c_str());
}

template <>
inline Glib::ustring convert(const Glib::ustring& str) {
  return str;
}

template <typename T>
inline bool can_convert(const Glib::ustring& str, T& val) {
  std::istringstream ss(str);
  ss >> val;
  return (bool)(ss);
}

template <>
inline bool can_convert(const Glib::ustring& str, Glib::ustring& val) {
  val = str;
  return true;
}

template <>
inline bool can_convert(const Glib::ustring& str, std::string& val) {
  val = str;
  return true;
}

//==============================================================================
#if GLIB_CHECK_VERSION(2, 44, 1)
class ListModelWrapper : public Gtk::TreeModel,
                         public Gtk::TreeDragDest,
                         public Gtk::TreeDragSource,
                         public Glib::Object,
                         public base::trackable
#elif GLIB_CHECK_VERSION(2, 42, 0)
class ListModelWrapper : public Gtk::TreeModel,
                         public Glib::Object,
                         public Gtk::TreeDragDest,
                         public Gtk::TreeDragSource,
                         public base::trackable
#elif GLIB_CHECK_VERSION(2, 40, 2)
class ListModelWrapper : public Gtk::TreeModel,
                         public Gtk::TreeDragDest,
                         public Gtk::TreeDragSource,
                         public Glib::Object,
                         public base::trackable
#else
class ListModelWrapper : public Glib::Object,
                         public Gtk::TreeModel,
                         public Gtk::TreeDragDest,
                         public Gtk::TreeDragSource,
                         public base::trackable

#endif

{
  friend class ColumnsModel;

protected:
  void model_changed(const bec::NodeId&, int) {
    ++_stamp;
  }

  //! root_node_path is a prefix for a subtree, also if you want to hide dummy root node '0'
  //! you may pass NodeId(0) to ListModelWrapper
  ListModelWrapper(bec::ListModel* tm, Gtk::TreeView* treeview, const std::string& name);

public:
  ~ListModelWrapper();

  static Glib::RefPtr<ListModelWrapper> create(bec::ListModel* tm, Gtk::TreeView* treeview, const std::string& name) {
    return Glib::RefPtr<ListModelWrapper>(new ListModelWrapper(tm, treeview, name));
  }

  /**
   Marks the tree as invalid.
   Methods like get_iter_vfunc will return false and the backend tree model
   will be treated as no longer valid.
   */
  void invalidate();

  void set_iconview(Gtk::IconView* iv);

  template <class T>
  void after_cell_edit(const Glib::ustring& path_string, const Glib::ustring& new_text,
                       const typename Gtk::TreeModelColumn<T>& column) {
    Gtk::TreeModel::iterator iter = get_iter(Gtk::TreePath(path_string));
    if (iter) {
      Gtk::TreeRow row = *iter;
      T val;
      if (can_convert<T>(new_text, val))
        row[column] = val;
    }
  }

  void after_cell_toggle(const Glib::ustring& path_string, const Gtk::TreeModelColumn<bool>& column) {
    Gtk::TreeModel::iterator iter = get_iter(Gtk::TreePath(path_string));
    if (iter) {
      Gtk::TreeRow row = *iter;
      const bool new_value = !row[column];
      row[column] = new_value;
    }
  }

  ColumnsModel& model() {
    return _columns;
  }

  void set_icon_size(const bec::IconSize icon_size) {
    _icon_size = icon_size;
  }

  bec::IconSize get_icon_size() const {
    return _icon_size;
  }

  bec::NodeId node_for_iter(const iterator& iter) const;

  virtual bec::NodeId get_node_for_path(const Gtk::TreeModel::Path& path) const;

  bec::ListModel* get_be_model() const {
    return *_tm;
  }
  void set_be_model(bec::ListModel* tm);

  void set_name(const std::string& nm) {
    _name = nm;
  }
  const std::string& name() const {
    return _name;
  }
  void refresh();
  void note_row_added();

  typedef sigc::slot<void, const iterator&, int, GType, Glib::ValueBase&> FakeColumnValueGetter;
  typedef sigc::slot<void, const iterator&, int, GType, const Glib::ValueBase&> FakeColumnValueSetter;

  void set_fake_column_value_getter(FakeColumnValueGetter fake_getter);
  void set_fake_column_value_setter(FakeColumnValueSetter fake_setter);

  void set_row_draggable_slot(const sigc::slot<bool, Gtk::TreeModel::Path>& slot);

  typedef std::vector<bec::NodeId> NodeIdArray;
  NodeIdArray get_selection() const;
#ifndef NO_MENU_MANAGER
  void handle_popup(const int x, const int y, const int time, GdkEventButton* evb);
  virtual bool handle_popup_event(GdkEvent* event);
  void set_fe_menu_handler(const sigc::slot<void, const std::string&, const std::vector<bec::NodeId>&>& slot) {
    _fe_menu_handler = slot;
  }

private:
  sigc::slot<void, const std::string&, const std::vector<bec::NodeId>&> _fe_menu_handler;
#endif
protected:
  virtual Gtk::TreeModelFlags get_flags_vfunc() const;
  virtual int get_n_columns_vfunc() const;
  virtual GType get_column_type_vfunc(int index) const;

  /**
  Sets @a iter_next to refer to the node following @a iter it at the current level.
  If there is no next iter, false is returned and iter_next is set to be invalid.

  @param iter An iterator.
  @param iter_next An iterator that will be set to refer to the next node, or will be set as invalid.
  @result true if the operation was possible.
  */
  virtual bool iter_next_vfunc(const iterator& iter, iterator& iter_next) const;

  /**
  Sets @a iter to a valid iterator pointing to @a path

  @param path An path to a node.
  @param iter An iterator that will be set to refer to a node to the path, or will be set as invalid.
  @result true if the operation was possible.
  */
  virtual bool get_iter_vfunc(const Path& path, iterator& iter) const;

  /**
  Sets @a iter to refer to the first child of @a parent. If @a parent has no children,
  false is returned and @a iter is set to be invalid.

  @param parent An iterator.
  @param iter An iterator that will be set to refer to the firt child node, or will be set as invalid.
  @result true if the operation was possible.
  */
  virtual bool iter_children_vfunc(const iterator& parent, iterator& iter) const;

  /**
  Sets @a iter to be the parent of @a child. If @a child is at the toplevel, and
  doesn't have a parent, then @a iter is set to an invalid iterator and false
  is returned.

  @param child An iterator.
  @param iter An iterator that will be set to refer to the parent node, or will be set as invalid.
  @result true if the operation was possible.
  */
  virtual bool iter_parent_vfunc(const iterator& child, iterator& iter) const;

  /**
  Sets @a iter to be the child of @a parent using the given index.  The first
  index is 0.  If @a n is too big, or @a parent has no children, @a iter is set
  to an invalid iterator and false is returned.
  See also iter_nth_root_child_vfunc()

  @param parent An iterator.
  @param n The index of the child node to which @a iter should be set.
  @param iter An iterator that will be set to refer to the nth node, or will be set as invalid.
  @result true if the operation was possible.
  */
  virtual bool iter_nth_child_vfunc(const iterator& parent, int n, iterator& iter) const;

  /**
  Sets @a iter to be the child of at the root level using the given index.  The first
  index is 0.  If @a n is too big, or if there are no children, @a iter is set
  to an invalid iterator and false is returned.
  See also iter_nth_child_vfunc().

  @param n The index of the child node to which @a iter should be set.
  @param iter An iterator that will be set to refer to the nth node, or will be set as invalid.
  @result true if the operation was possible.
  */
  virtual bool iter_nth_root_child_vfunc(int n, iterator& iter) const;

  /**
  Returns true if @a iter has children, false otherwise.

  @param iter The iterator to test for children.
  @result true if @a iter has children.
  */
  virtual bool iter_has_child_vfunc(const iterator& iter) const;

  /**
  Returns the number of children that @a iter has.
  See also iter_n_root_children_vfunc().

  @param iter The iterator to test for children.
  @result The number of children of @a iter.
  */
  virtual int iter_n_children_vfunc(const iterator& iter) const;

  /**
  Returns the number of toplevel nodes.
  See also iter_n_children().

  @result The number of children at the root level.
  */
  virtual int iter_n_root_children_vfunc() const;

  /**
  Lets the tree ref the node.  This is an optional method for models to
  implement.  To be more specific, models may ignore this call as it exists
  primarily for performance reasons.

  This function is primarily meant as a way for views to let caching model know
  when nodes are being displayed (and hence, whether or not to cache that
  node.)  For example, a file-system based model would not want to keep the
  entire file-hierarchy in memory, just the sections that are currently being
  displayed by every current view.

  A model should be expected to be able to get an iter independent of its
  reffed state.

  @param iter the iterator.
  */
  // virtual void ref_node_vfunc(const iterator& iter) const;

  /**
  Lets the tree unref the node.  This is an optional method for models to
  implement.  To be more specific, models may ignore this call as it exists
  primarily for performance reasons.

  For more information on what this means, see unref_node_vfunc().
  Please note that nodes that are deleted are not unreffed.

  @param iter the iterator.
 */
  // virtual void unref_node_vfunc(const iterator& iter) const;

  /** Override and implement this in a derived TreeModel class.
  Returns a Path referenced by @a iter.

  @param iter The iterator.
  @result The path.
  */
  virtual TreeModel::Path get_path_vfunc(const iterator& iter) const;

  /**
  Initializes and sets @a value to that at @a column.

  @param iter The iterator.
  @param column The column to lookup the value at.
  @param value An empty Glib:Value to set.
  */
  virtual void get_value_vfunc(const iterator& iter, int column, Glib::ValueBase& value) const;

  /**
  Initializes and sets @a icon value to that at @a column.

  @param iter The iterator.
  @param column The column to lookup the value at.
  @param node The grt tree node at the iter.
  @param value An empty Glib:Value to set.
  */
  virtual void get_icon_value(const iterator& iter, int column, const bec::NodeId& node, Glib::ValueBase& value) const;

  /**
  @note This virtual method is not recommended.  To check
  whether an iterator is valid, call TreeStore::iter_is_valid(),
  ListStore::iter_is_valid() or TreeModelSort::iter_is_valid() directly
  instead.  Because these methods are intended to be used only for debugging
  and/or testing purposes, it doesn't make sense to provide an abstract
  interface to them.

  @result true if the iterator is valid.

  @deprecated Use iter_is_valid() in the derived class.
  */
  virtual bool iter_is_valid(const iterator& iter) const;

  // Called by TreeRow, which is a friend class:
  // The comment about set_row_changed() in the documentation is based on my reading of the source of
  // gtk_list_store_set_value() and gtk_tree_store_set_value().
  /**
  Row::set_value() work.
  You can probably just implement this by calling set_value_vfunc().
  Your implementation of set_value_impl() should also call set_row_changed() after changing the value.
  */
  virtual void set_value_impl(const iterator& row, int column, const Glib::ValueBase& value);

  // This might not need to be virtual, but it's not a big deal. murrayc.
  virtual void get_value_impl(const iterator& row, int column, Glib::ValueBase& value) const;

  // From TreeDragSource
  virtual bool drag_data_get_vfunc(const Gtk::TreeModel::Path& path, Gtk::SelectionData& selection_data) const;
  virtual bool row_draggable_vfunc(const TreeModel::Path& path) const;
  virtual bool drag_data_delete_vfunc(const Gtk::TreeModel::Path& path);

  // From TreeDragDest
  virtual bool row_drop_possible_vfunc(const Gtk::TreeModel::Path& dest,
                                       const Gtk::SelectionData& selection_data) const;
  virtual bool drag_data_received_vfunc(const Gtk::TreeModel::Path& dest, const Gtk::SelectionData& selection_data);

protected:
  virtual bool init_gtktreeiter(GtkTreeIter* it, const bec::NodeId& uid = bec::NodeId()) const;
  void reset_iter(iterator& iter) const throw();

private:
  bec::ListModel** _tm;

protected:
  Gtk::TreeView* _treeview;
  Gtk::IconView* _iconview;
  Gtk::Menu* _context_menu;
  int _stamp;               //!< stamp to check if an iterator is invalid
  ColumnsModel _columns;    //!< Generic model of columns
  bec::IconSize _icon_size; //!< icon size to set by mode view changed buttons
  struct LMPtr* _self_ref;  // hack to workaround a crash..
  bool _invalid;

  std::string _name;

  FakeColumnValueGetter _fake_column_value_getter;
  FakeColumnValueSetter _fake_column_value_setter;

  sigc::slot<bool, TreeModel::Path> _row_draggable;
  static void* on_bec_model_destroyed(void*);
};

#endif

//!
//! @}
//!
