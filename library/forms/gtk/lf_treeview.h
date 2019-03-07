/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _LF_TREEVIEW_H_
#define _LF_TREEVIEW_H_

#include "mforms/mforms.h"

#include "lf_view.h"
#include "base/string_utilities.h"

namespace mforms {
  namespace gtk {

    struct TreeNodeDataRef {
      TreeNodeData *_data;

      TreeNodeDataRef() : _data(0) {
      }

      TreeNodeDataRef(TreeNodeData *data) : _data(data) {
        if (_data)
          _data->retain();
      }

      TreeNodeDataRef(const TreeNodeDataRef &other) : _data(other._data) {
        if (_data)
          _data->retain();
      }

      ~TreeNodeDataRef() {
        if (_data)
          _data->release();
      }

      TreeNodeDataRef &operator=(const TreeNodeDataRef &other) {
        if (_data != other._data) {
          if (_data)
            _data->release();
          _data = other._data;
          if (_data)
            _data->retain();
        }
        return *this;
      }
    };

    class CustomTreeStore : public Gtk::TreeStore {
    public:
      CustomTreeStore(const Gtk::TreeModelColumnRecord &columns);

      static Glib::RefPtr<CustomTreeStore> create(const Gtk::TreeModelColumnRecord &columns);

      void copy_iter(Gtk::TreeModel::iterator &from, Gtk::TreeModel::iterator &to);
    };

    class TreeViewImpl; // rename

    class RootTreeNodeImpl : public ::mforms::TreeNode {
    protected:
      TreeViewImpl *_treeview;
      int _refcount;

      inline TreeNodeRef ref_from_iter(const Gtk::TreeIter &iter) const;
      inline TreeNodeRef ref_from_path(const Gtk::TreePath &path) const;

      virtual bool is_root() const;

      virtual bool is_valid() const;

      virtual bool equals(const TreeNode &other);

      virtual int level() const;

    public:
      RootTreeNodeImpl(TreeViewImpl *tree);

      virtual void invalidate();

      virtual void release();

      virtual void retain();

      virtual int count() const;

      virtual bool can_expand();

      virtual Gtk::TreeIter create_child(int index);

      virtual Gtk::TreeIter create_child(int index, Gtk::TreeIter *other_parent);

      virtual TreeNodeRef insert_child(int index);

      virtual std::vector<mforms::TreeNodeRef> add_node_collection(const TreeNodeCollectionSkeleton &nodes,
                                                                   int position = -1);

      virtual void add_children_from_skeletons(const std::vector<Gtk::TreeIter> &parents,
                                               const std::vector<TreeNodeSkeleton> &children);

      virtual void remove_from_parent();

      virtual TreeNodeRef get_child(int index) const;

      virtual TreeNodeRef get_parent() const;

      virtual void expand();

      virtual void collapse();

      virtual bool is_expanded();

      virtual void set_attributes(int column, const mforms::TreeNodeTextAttributes &attrs);

      virtual void set_icon_path(int column, const std::string &icon);

      virtual void set_string(int column, const std::string &value);

      virtual void set_int(int column, int value);

      virtual void set_long(int column, std::int64_t value);

      virtual void set_bool(int column, bool value);

      virtual void set_float(int column, double value);

      virtual std::string get_string(int column) const;

      virtual int get_int(int column) const;

      virtual std::int64_t get_long(int column) const;

      virtual bool get_bool(int column) const;

      virtual double get_float(int column) const;

      virtual void set_tag(const std::string &tag);

      virtual std::string get_tag() const;

      virtual void set_data(TreeNodeData *data);

      virtual TreeNodeData *get_data() const;

      virtual TreeNodeRef previous_sibling() const;

      virtual TreeNodeRef next_sibling() const;

      virtual int get_child_index(TreeNodeRef child) const;

      virtual void move_node(TreeNodeRef node, bool before);
    };

    class TreeNodeImpl : public RootTreeNodeImpl {
      // If _rowref becomes invalidated (eg because Model was deleted),
      // we just ignore all operations on the node
      Gtk::TreeRowReference _rowref;

    public:
      inline Glib::RefPtr<Gtk::TreeStore> model();
      inline Gtk::TreeIter iter();

      inline Gtk::TreeIter iter() const;

      inline Gtk::TreePath path();

      virtual bool is_root() const;

      virtual Gtk::TreeIter duplicate_node(TreeNodeRef oldnode);

    public:
      TreeNodeImpl(TreeViewImpl *tree, Glib::RefPtr<Gtk::TreeStore> model, const Gtk::TreePath &path);

      TreeNodeImpl(TreeViewImpl *tree, const Gtk::TreeRowReference &ref);

      virtual bool equals(const TreeNode &other);

      virtual bool is_valid() const;

      virtual void invalidate();

      virtual int count() const;

      virtual Gtk::TreeIter create_child(int index);

      virtual void remove_from_parent();

      virtual TreeNodeRef get_child(int index) const;

      virtual TreeNodeRef get_parent() const;

      virtual void expand();

      virtual bool can_expand();

      virtual void collapse();

      virtual bool is_expanded();

      virtual void set_attributes(int column, const TreeNodeTextAttributes &attrs);

      virtual void set_icon_path(int column, const std::string &icon);

      virtual void set_string(int column, const std::string &value);

      virtual void set_int(int column, int value);

      virtual void set_long(int column, std::int64_t value);

      virtual void set_bool(int column, bool value);

      virtual void set_float(int column, double value);

      virtual std::string get_string(int column) const;

      virtual int get_int(int column) const;

      virtual std::int64_t get_long(int column) const;

      virtual bool get_bool(int column) const;

      virtual double get_float(int column) const;

      virtual void set_tag(const std::string &tag);

      virtual std::string get_tag() const;

      virtual void set_data(TreeNodeData *data);

      virtual TreeNodeData *get_data() const;

      virtual int level() const;

      virtual TreeNodeRef next_sibling() const;

      virtual TreeNodeRef previous_sibling() const;

      virtual int get_child_index(TreeNodeRef child) const;

      virtual void move_node(TreeNodeRef node, bool before);
    };

    class TreeViewImpl : public ViewImpl // rename
    {
      friend class RootTreeNodeImpl;
      friend class TreeNodeImpl;

    private:
      class ColumnRecord : public Gtk::TreeModelColumnRecord {
        template <class C>
        Gtk::TreeModelColumn<C> *add_model_column() {
          Gtk::TreeModelColumn<C> *col = new Gtk::TreeModelColumn<C>();
          columns.push_back(col);
          add(*col);
          return col;
        }

        void on_cell_editing_started(Gtk::CellEditable *e, const Glib::ustring &path);
        bool on_focus_out(GdkEventFocus *event, Gtk::Entry *e);

      public:
        std::vector<Gtk::TreeModelColumnBase *> columns;
        Gtk::TreeModelColumn<std::string> _tag_column;
        Gtk::TreeModelColumn<TreeNodeDataRef> _data_column;
        std::vector<int> column_value_index;
        std::vector<int> column_attr_index;

        template <class C>
        const Gtk::TreeModelColumn<C> &get(int column) {
          Gtk::TreeModelColumnBase *c = columns[column];

          return *static_cast<Gtk::TreeModelColumn<C> *>(c);
        }

        virtual ~ColumnRecord();
        void add_tag_column();
        void add_data_column();
        Gtk::TreeModelColumn<std::string> &tag_column();
        Gtk::TreeModelColumn<TreeNodeDataRef> &data_column();
        int add_string(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr, bool with_icon,
                       bool align_right = false);
        int add_integer(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr);
        int add_long_integer(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr);
        int add_float(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr);
        int add_check(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr);
        int add_tri_check(Gtk::TreeView *tree, const std::string &title, bool editable, bool attr);
        template <typename T>
        std::pair<Gtk::TreeViewColumn *, int> create_column(Gtk::TreeView *tree, const std::string &title,
                                                            bool editable, bool attr, bool with_icon,
                                                            bool align_right = false);
        void format_tri_check(Gtk::CellRenderer *cell, const Gtk::TreeIter &iter,
                              const Gtk::TreeModelColumn<int> &column);
      };

      bool _is_drag_source;
      ColumnRecord _columns;

      Gtk::ScrolledWindow _swin;
      Gtk::TreeView _tree;

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

      Gtk::TreeView *tree_view() {
        return &_tree;
      }
      virtual Gtk::Widget *get_outer() const {
        return &(const_cast<Gtk::ScrolledWindow &>(_swin));
      }
      virtual Gtk::Widget *get_inner() const {
        return &(const_cast<Gtk::TreeView &>(_tree));
      }

      TreeViewImpl(TreeView *self, mforms::TreeOptions opts);
      ~TreeViewImpl();
      void string_edited(const Glib::ustring &path, const Glib::ustring &new_text, int column);
      void toggle_edited(const Glib::ustring &path, int column);
      void on_activated(const Gtk::TreeModel::Path &, Gtk::TreeViewColumn *);
      void on_will_expand(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path);
      void on_collapsed(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path);
      void on_realize();
      bool on_header_button_event(GdkEventButton *ev, int);
      bool on_key_release(GdkEventKey *ev);
      bool on_button_event(GdkEventButton *ev);
      bool on_button_release(GdkEventButton *ev);
      bool on_motion_notify(GdkEventMotion *ev);
      bool on_draw_event(const ::Cairo::RefPtr< ::Cairo::Context> &context);
      bool on_enter_notify(GdkEventCrossing *ev);
      bool on_leave_notify(GdkEventCrossing *ev);

      void slot_drag_end(const Glib::RefPtr<Gdk::DragContext> &context);
      bool slot_drag_failed(const Glib::RefPtr<Gdk::DragContext> &context, Gtk::DragResult result);

      void set_allow_sorting(bool flag);

      int add_column(TreeColumnType type, const std::string &name, int initial_width, bool editable, bool attributed);
      void end_columns();
      static bool create(TreeView *self, mforms::TreeOptions opt);
      static int add_column(TreeView *self, TreeColumnType type, const std::string &name, int width, bool editable,
                            bool attr);
      static void end_columns(TreeView *self);
      static void clear(TreeView *self);
      static TreeNodeRef root_node(TreeView *self);
      static TreeNodeRef get_selected_node(TreeView *self);
      static std::list<TreeNodeRef> get_selection(TreeView *self);
      static void set_selected(TreeView *self, TreeNodeRef node, bool flag);
      static void scrollToNode(TreeView *self, TreeNodeRef node);
      static TreeSelectionMode get_selection_mode(TreeView *self);
      static void set_selection_mode(TreeView *self, TreeSelectionMode mode);
      static void clear_selection(TreeView *self);

      static int row_for_node(TreeView *self, TreeNodeRef node);
      static TreeNodeRef node_at_row(TreeView *self, int row);
      static TreeNodeRef node_with_tag(TreeView *self, const std::string &tag);

      static void set_row_height(TreeView *self, int height);

      static void set_allow_sorting(TreeView *self, bool flag);
      static void freeze_refresh(TreeView *self, bool flag);
      static void set_column_visible(TreeView *self, int column, bool flag);
      static bool get_column_visible(TreeView *self, int column);
      static void set_column_title(TreeView *self, int column, const std::string &title);

      static void set_column_width(TreeView *self, int column, int width);
      static int get_column_width(TreeView *self, int column);
      static TreeNodeRef node_at_position(TreeView *self, base::Point position);

      Gtk::TreeModel::iterator to_list_iter(const Gtk::TreeModel::iterator &it);
      Gtk::TreeModel::Path to_list_path(const Gtk::TreeModel::Path &path);
      Gtk::TreeModel::iterator to_sort_iter(const Gtk::TreeModel::iterator &it);
      Gtk::TreeModel::Path to_sort_path(const Gtk::TreeModel::Path &path);

      void header_clicked(Gtk::TreeModelColumnBase *, Gtk::TreeViewColumn *);

      virtual void set_back_color(const std::string &color);

    protected:
      mforms::DropPosition get_drop_position();

    public:
      static void init();

      int index_for_column_attr(int i) {
        return _columns.column_attr_index[i];
      }
      int index_for_column(int i) {
        return _columns.column_value_index[i];
      }
      Glib::RefPtr<Gtk::TreeStore> tree_store() {
        return _tree_store;
      }

      TreeView *get_owner();
    };
  }
}

#endif
