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
#ifndef _LF_TREENODEVIEW_H_
#define _LF_TREENODEVIEW_H_

#include <mforms/mforms.h>

#include "lf_view.h"

namespace mforms {
namespace gtk {


struct TreeNodeDataRef
{
  TreeNodeData *_data;

  TreeNodeDataRef()
  : _data(0)
  {
  }

  TreeNodeDataRef(TreeNodeData *data)
  : _data(data) 
  {
    if (_data) _data->retain();
  }

  TreeNodeDataRef(const TreeNodeDataRef &other)
  : _data(other._data)
  {
    if (_data) _data->retain();
  }

  ~TreeNodeDataRef() 
  {
    if (_data) _data->release();
  }

  TreeNodeDataRef& operator= (const TreeNodeDataRef &other)
  {
    if (_data != other._data)
    {
      if (_data) _data->release();
      _data = other._data;
      if (_data) _data->retain();
    }
    return *this;
  }
};


class TreeNodeViewImpl : public ViewImpl
{
  friend class RootTreeNodeImpl;
  friend class TreeNodeImpl;
private:
  class ColumnRecord : public Gtk::TreeModelColumnRecord
  {
    template<class C>
    Gtk::TreeModelColumn<C> *add_model_column()
    {
      Gtk::TreeModelColumn<C> *col = new Gtk::TreeModelColumn<C>();
      columns.push_back(col);
      add(*col);
      return col;
    }

    void on_cell_editing_started(Gtk::CellEditable* e, const Glib::ustring &path);
    bool on_focus_out(GdkEventFocus *event, Gtk::Entry *e);

  public:
    std::vector<Gtk::TreeModelColumnBase*> columns;
    Gtk::TreeModelColumn<std::string>     _tag_column;
    Gtk::TreeModelColumn<TreeNodeDataRef>   _data_column;
    std::vector<int> column_value_index;
    std::vector<int> column_attr_index;

    template<class C>
      const Gtk::TreeModelColumn<C> &get(int column)
      {
        Gtk::TreeModelColumnBase* c= columns[column];
        
        return *static_cast<Gtk::TreeModelColumn<C>*>(c);
      }

    virtual ~ColumnRecord();
    void add_tag_column();
    void add_data_column();
    Gtk::TreeModelColumn<std::string>& tag_column();
    Gtk::TreeModelColumn<TreeNodeDataRef>& data_column();
    int add_string(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr, bool with_icon, bool align_right = false);
    int add_integer(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr);
    int add_long_integer(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr);
    int add_float(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr);
    int add_check(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr);
    int add_tri_check(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr);
    void format_tri_check(Gtk::CellRenderer* cell, const Gtk::TreeIter& iter, const Gtk::TreeModelColumn<int>& column);
  };

  ColumnRecord _columns;
  

  Gtk::ScrolledWindow _swin;
  Gtk::TreeView       _tree;

  sigc::connection _conn;
  int _row_height;
  bool _flat_list;
  bool _tagmap_enabled;
  bool _drag_source_enabled;

  Gtk::TreePath _overlayed_row;
  std::vector<Cairo::RefPtr<Cairo::ImageSurface> > _overlay_icons;
  int _hovering_overlay;
  int _clicking_overlay;
  bool _mouse_inside;

  GdkEventButton *_org_event;
  int _drag_button;
  int _drag_start_x;
  int _drag_start_y;
  bool _drag_in_progress;

  Glib::RefPtr<Gtk::TreeStore> _tree_store;
  Glib::RefPtr<Gtk::TreeModelSort> _sort_model;
  std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > _pixbufs;

  std::map<std::string, Gtk::TreeRowReference> _tagmap;

  mforms::TreeNodeRef _root_node;

  mforms::TreeNodeRef find_node_at_row(const Gtk::TreeModel::Children &trow, int &c, int row);

  Gtk::TreeView *tree_view() { return &_tree; }
  virtual Gtk::Widget *get_outer() const { return &(const_cast<Gtk::ScrolledWindow&>(_swin)); }
  virtual Gtk::Widget *get_inner() const { return &(const_cast<Gtk::TreeView&>(_tree)); }

  TreeNodeViewImpl(TreeNodeView *self, mforms::TreeOptions opts);
  ~TreeNodeViewImpl();
  void string_edited(const Glib::ustring &path, const Glib::ustring &new_text, int column);
  void toggle_edited(const Glib::ustring &path, int column);
  void on_activated(const Gtk::TreeModel::Path&, Gtk::TreeViewColumn*);
  void on_will_expand(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path);
  void on_collapsed(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path);
  bool on_button_event(GdkEventButton *ev);
  bool on_button_release(GdkEventButton* ev);
  bool on_motion_notify(GdkEventMotion* ev);
  bool on_expose_event(GdkEventExpose *ev);
  bool on_enter_notify(GdkEventCrossing *ev);
  bool on_leave_notify(GdkEventCrossing *ev);

  void slot_drag_end(const Glib::RefPtr<Gdk::DragContext> &context);
  bool slot_drag_failed(const Glib::RefPtr<Gdk::DragContext> &context,Gtk::DragResult result);

  void set_allow_sorting(bool flag);

  int add_column(TreeColumnType type, const std::string &name, int initial_width, bool editable, bool attributed);
  void end_columns();
  static bool create(TreeNodeView *self, mforms::TreeOptions opt);
  static int add_column(TreeNodeView *self, TreeColumnType type, const std::string &name, int width, bool editable, bool attr);
  static void end_columns(TreeNodeView *self);
  static void clear(TreeNodeView *self);
  static TreeNodeRef root_node(TreeNodeView *self);
  static TreeNodeRef get_selected_node(TreeNodeView *self);
  static std::list<TreeNodeRef> get_selection(TreeNodeView *self);
  static void set_selected(TreeNodeView* self, TreeNodeRef node, bool flag);
  static TreeSelectionMode get_selection_mode(TreeNodeView *self);
  static void set_selection_mode(TreeNodeView *self, TreeSelectionMode mode);
  static void clear_selection(TreeNodeView *self);

  static int row_for_node(TreeNodeView *self, TreeNodeRef node);
  static TreeNodeRef node_at_row(TreeNodeView *self, int row);
  static TreeNodeRef node_with_tag(TreeNodeView *self, const std::string &tag);

  static void set_row_height(TreeNodeView *self, int height);

  static void set_allow_sorting(TreeNodeView* self, bool flag);
  static void freeze_refresh(TreeNodeView* self, bool flag);
  static void set_column_visible(TreeNodeView* self, int column, bool flag);
  static bool get_column_visible(TreeNodeView* self, int column);
  static void set_column_width(TreeNodeView* self, int column, int width);
  static int get_column_width(TreeNodeView* self, int column);


  Gtk::TreeModel::iterator to_list_iter(const Gtk::TreeModel::iterator &it);
  Gtk::TreeModel::Path to_list_path(const Gtk::TreeModel::Path &path);
  Gtk::TreeModel::iterator to_sort_iter(const Gtk::TreeModel::iterator &it);
  Gtk::TreeModel::Path to_sort_path(const Gtk::TreeModel::Path &path);

  void header_clicked(Gtk::TreeModelColumnBase*, Gtk::TreeViewColumn*);

  virtual void set_back_color(const std::string &color);

public:
  static void init();

  int index_for_column_attr(int i) { return _columns.column_attr_index[i]; }
  int index_for_column(int i) { return _columns.column_value_index[i]; }
  Glib::RefPtr<Gtk::TreeStore> tree_store() { return _tree_store; }

  TreeNodeView* get_owner();
};

  
}
}

#endif
