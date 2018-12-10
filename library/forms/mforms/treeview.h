/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <mforms/view.h>

/**
 * Implementation of a control class for a treeview control based on node objects.
 */

namespace mforms {
  class TreeView;
  class ContextMenu;

  /** Determines what type a column should have (mainly describing the column *editor*). */
  enum TreeColumnType {
    StringColumnType,      //!< simple string, with text entry editor
    StringLTColumnType,    //!< same as StringColumnType, but truncated at the left end if it doesn't fit
    IntegerColumnType,     //!< numeric field, with text entry editor
    LongIntegerColumnType, //!< 64bit numeric field, with text entry editor
    CheckColumnType,       //!< boolean field, with checkbox
    TriCheckColumnType,    //!< checkbox field (same as CheckColumnType) which also accepts -1 as mixed state
    IconColumnType,        //!< icon field, value is the icon path
    IconStringColumnType = IconColumnType,
    NumberWithUnitColumnType, //!< string type, representing numbers with a unit suffix (like KB, MB, ms etc)
    FloatColumnType           //!< a double precision floating point number
  };

  /** Options used to customize what to show in the tree. */
  enum TreeOptions {
    TreeDefault = 0,
    TreeTranslucent = 1 << 1,      // Where supported make the tree stand out by adding blurred background translucency.
    TreeNoColumns = 1 << 3,        // On non-Windows platforms columns are always on, so switch them on
                                   // Windows too by default and use this flag to switch them off, if really needed.
    TreeAllowReorderRows = 1 << 4, // Allows row reordering, sets TreeCanBeDragSource implicitly.
    TreeShowColumnLines = 1 << 5,  // Show column separator lines.
    TreeShowRowLines = 1 << 6,     // Show row separator lines.
    TreeNoBorder = 1 << 7,         // Switch off the border around the control. Default is to show the border.
    TreeSidebar = 1 << 8,          // Sidebar style treeview.
    TreeNoHeader = 1 << 9,         // Don't show a column header. Only meaningful if columns are used.
    TreeShowHeader = 0,            // For backwards compatibility.
    TreeFlatList = 1 << 10,        // No child items expected.
    TreeAltRowColors = 1 << 11,    // Enable alternating row colors.
    TreeSizeSmall = 1 << 12,       // Small text.
    TreeIndexOnTag = 1 << 13,      // Keep a node index on the tags (use with node_with_tag).

    TreeCanBeDragSource = 1 << 14, //!< allow the tree to be a drag source, data used depends on actual tree
    TreeColumnsAutoResize = 1 << 15, //!< used only on OSX enable column autoresize
  };

#ifndef SWIG
  inline TreeOptions operator|(TreeOptions a, TreeOptions b) {
    return (TreeOptions)((int)a | (int)b);
  }
#endif

  enum TreeSelectionMode {
    TreeSelectSingle,  // 0 or 1 item selected
    TreeSelectMultiple // 0+ items selection
  };

  typedef struct TextAttributes TreeNodeTextAttributes;

  // This struct represents the data comprising a node
  // including a collection of child nodes.
  struct MFORMS_EXPORT TreeNodeSkeleton {
  public:
    TreeNodeSkeleton(){};
    TreeNodeSkeleton(const std::string &caption, const std::string &icon, const std::string &tag);
    std::string caption;
    std::string icon;
    std::string tag;
    std::vector<TreeNodeSkeleton> children;
  };

  // This struct represents a collection of nodes sharing tag, icon and structure
  struct MFORMS_EXPORT TreeNodeCollectionSkeleton {
  public:
    TreeNodeCollectionSkeleton(){};
    TreeNodeCollectionSkeleton(const std::string &icon);
    std::string icon;
    std::vector<TreeNodeSkeleton> children;
    std::vector<std::string> captions;
  };

  class MFORMS_EXPORT TreeNodeData {
  protected:
    int _refcount;

  public:
    TreeNodeData() : _refcount(0) {
    }
    virtual ~TreeNodeData() {
    }
    void retain() {
      _refcount++;
    }
    void release() {
      _refcount--;
      if (_refcount == 0)
        delete this;
    }
  };

  class TreeNode;
  struct MFORMS_EXPORT TreeNodeRef {
  private:
    TreeNode *node;

  public:
// following methods are private, but need to be made public for the platform implementation
#ifndef SWIG
    TreeNodeRef(TreeNode *anode);
    TreeNode *ptr() {
      return node;
    }
#endif
  public:
    TreeNodeRef() : node(0) {
    }
    TreeNodeRef(const TreeNodeRef &other);
    ~TreeNodeRef();
#ifndef SWIG
    TreeNodeRef &operator=(const TreeNodeRef &other);
#endif
    TreeNode *operator->() const;
    TreeNode *operator->();

    operator bool() const {
      return node != 0;
    }
    bool operator<(const TreeNodeRef &other) const {
      return node < other.node;
    }
    bool operator==(const TreeNodeRef &other) const;
    bool operator!=(const TreeNodeRef &other) const;

    bool is_valid();
  };

  class MFORMS_EXPORT TreeNode {
    friend struct TreeNodeRef;

  protected:
    virtual void release() = 0;
    virtual void retain() = 0;

  public:
    virtual ~TreeNode(){};

    virtual bool equals(const TreeNode &other) = 0;
#ifndef SWIG
    virtual bool is_valid() const = 0;
#endif
    virtual int level() const = 0; // 0 for root, 1 for top level etc.

    virtual void set_icon_path(int column, const std::string &icon) = 0;

    virtual void set_string(int column, const std::string &value) = 0;
    virtual void set_int(int column, int value) = 0;
    virtual void set_long(int column, std::int64_t value) = 0;
    virtual void set_bool(int column, bool value) = 0;
    virtual void set_float(int column, double value) = 0;

    virtual void set_attributes(int column, const TreeNodeTextAttributes &attrs) = 0;

    virtual std::string get_string(int column) const = 0;
    virtual int get_int(int column) const = 0;
    virtual std::int64_t get_long(int column) const = 0;
    virtual bool get_bool(int column) const = 0;
    virtual double get_float(int column) const = 0;

    virtual int count() const = 0;
    virtual TreeNodeRef add_child() {
      return insert_child(-1);
    }
    virtual TreeNodeRef insert_child(int index) = 0;
    virtual void remove_from_parent() = 0;
    virtual void remove_children(); // default impl provided, subclasses may override to provide faster impl
    virtual TreeNodeRef get_child(int index) const = 0;
    virtual int get_child_index(TreeNodeRef child) const = 0;
    virtual TreeNodeRef get_parent() const = 0;
    virtual TreeNodeRef find_child_with_tag(const std::string &tag); // This will search the sub nodes sequentially.
    virtual TreeNodeRef previous_sibling() const = 0;
    virtual TreeNodeRef next_sibling() const = 0;

    // Moves this node to a different place relative to the given node (which must be in the same tree).
    virtual void move_node(TreeNodeRef node, bool before) = 0;

    // This is a very special function and I'm not sure it should be here.
    // It creates nodes out of the collection's captions and adds the same child nodes to each of those nodes
    // as given in the children member. This works iteratively.
    virtual std::vector<mforms::TreeNodeRef> add_node_collection(const TreeNodeCollectionSkeleton &nodes,
                                                                 int position = -1) = 0;

    virtual void expand() = 0;
    virtual void collapse() = 0;
    virtual bool is_expanded() = 0;

    // Primitive check. Use TreeView::can_expand() instead for potentially more sophisticated checks.
    virtual bool can_expand() {
      return count() > 0;
    };
    virtual void toggle();

    virtual void set_tag(const std::string &tag) = 0;
    virtual std::string get_tag() const = 0;

    virtual void set_data(TreeNodeData *data) = 0;
    virtual TreeNodeData *get_data() const = 0;
  };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct TreeViewImplPtrs {
    bool (*create)(TreeView *self, TreeOptions options);

#if defined(_MSC_VER)
    int (*add_column)(TreeView *self, TreeColumnType type, const std::string &name, int initial_width, bool editable);
#else
    int (*add_column)(TreeView *self, TreeColumnType type, const std::string &name, int initial_width, bool editable,
                      bool attributed);
#endif
    void (*end_columns)(TreeView *self);

    void (*clear)(TreeView *self);

    TreeNodeRef (*root_node)(TreeView *self);

    void (*set_row_height)(TreeView *self, int height);

    void (*set_allow_sorting)(TreeView *self, bool);

    void (*freeze_refresh)(TreeView *self, bool);

    void (*set_selection_mode)(TreeView *self, TreeSelectionMode mode);
    TreeSelectionMode (*get_selection_mode)(TreeView *self);

    std::list<TreeNodeRef> (*get_selection)(TreeView *self);
    TreeNodeRef (*get_selected_node)(TreeView *self);

    void (*clear_selection)(TreeView *self);
    void (*set_selected)(TreeView *self, TreeNodeRef node, bool state);
    void (*scrollToNode)(TreeView *self, TreeNodeRef node);

    int (*row_for_node)(TreeView *self, TreeNodeRef node);
    TreeNodeRef (*node_at_row)(TreeView *self, int row);
    TreeNodeRef (*node_at_position)(TreeView *self, base::Point position);
    TreeNodeRef (*node_with_tag)(TreeView *self, const std::string &tag);

    void (*set_column_title)(TreeView *self, int column, const std::string &title);

    void (*set_column_visible)(TreeView *self, int column, bool flag);
    bool (*get_column_visible)(TreeView *self, int column);

    void (*set_column_width)(TreeView *self, int column, int width);
    int (*get_column_width)(TreeView *self, int column);

    void (*BeginUpdate)(TreeView *self);
    void (*EndUpdate)(TreeView *self);
  };
#endif
#endif

  class ContextMenu;

  /** Control to show nodes in multiple columns in the form of a tree.

   Before adding items, you must first define the columns and the content types with
   add_column() and end_columns()
   */
  class MFORMS_EXPORT TreeView : public View {
  public:
    TreeView(TreeOptions options);
    ~TreeView();

    /** Adds a column to be displayed in the tree.

     @param type - type of value to be displayed in column
     @param name - name/caption to show in header
     @param initia_width - width in pixels of the column
     @param editable - whether editing is allowed for rows in this column
     */
    int add_column(TreeColumnType type, const std::string &name, int initial_width, bool editable = false,
                   bool attributed = false);
    /** Must be called after needed add_column() calls are finished. */
    void end_columns();

    /** Sets the title of the given column */
    void set_column_title(int column, const std::string &title);

    /** Sets a callback that's called when the mouse hovers on a row.

     Callback must return path to an icon if it wants it to be displayed. When the user clicks it,
     the node_activated() handler is called, with a negated, one-based index of the icon index (-1, -2 etc.).
     */
    void set_row_overlay_handler(const std::function<std::vector<std::string>(TreeNodeRef)> &overlay_icon_for_node);

    int get_column_count() const {
      return (int)_column_types.size();
    }
    TreeColumnType get_column_type(int column);

    /** Removes all nodes */
    void clear();
    TreeNodeRef root_node();

    TreeNodeRef add_node();

    /** Selects selection type */
    void set_selection_mode(TreeSelectionMode mode);
    /** Returns the current selection mode */
    TreeSelectionMode get_selection_mode();

    /** Returns a list of the selected nodes */
    std::list<TreeNodeRef> get_selection();
    /** Returns the selected node. In case multiple selected nodes exist, only the 1st is returned */
    TreeNodeRef get_selected_node();
    int get_selected_row();
    void clear_selection();

    /** Sets the selection state of the node */
    void select_node(TreeNodeRef node);
    void set_node_selected(TreeNodeRef node, bool flag);
    void scrollToNode(TreeNodeRef node);

    int row_for_node(TreeNodeRef node);
    TreeNodeRef node_at_row(int row);
    TreeNodeRef node_at_position(base::Point position); // TODO: Linux

    /** Requires TreeIndexOnTag */
    TreeNodeRef node_with_tag(const std::string &tag);

    /** Sets the height of a row in pixels */
    void set_row_height(int height);

    /** Toggles sorting of rows when the user clicks on a column header

     Be careful with the get_selected() and set_selected() methods when sorting is enabled,
     as the index of a row will change depending on the sorting. Use get_row_tag() for
     an immutable identifier for rows.

     \warning When sorting is enabled, you must use freeze_refresh()/thaw_refresh() when making changes to tree,
     otherwise the contents will become corrupted.
     */
    void set_allow_sorting(bool flag);

    /** Freezes refresh of tree display. Use when updating a large number of rows or when sorting is enabled. */
    void freeze_refresh();

    /** Unfreezes a previously done freeze_refresh() */
    void thaw_refresh();

    void set_column_visible(int column, bool flag);

    bool get_column_visible(int column);

#ifndef SWIG
    /** Signal emitted when the selected row is changed

     In Python use add_changed_callback()
     */
    boost::signals2::signal<void()> *signal_changed() {
      return &_signal_changed;
    }
    /** Signal emitted when the user double-clicks a cell.

     Arguments passed are row node and column index.

     In Python use add_activated_callback()
     */
    boost::signals2::signal<void(TreeNodeRef, int)> *signal_node_activated() {
      return &_signal_activated;
    }

    boost::signals2::signal<void(TreeNodeRef, bool)> *signal_expand_toggle() {
      return &_signal_expand_toggle;
    }

    boost::signals2::signal<void(int)> *signal_column_resized() {
      return &_signal_column_resized;
    }

    /** Sets a callback to handle changed made by user to tree cells.

     if this handler is set, it must call set() itself whenever a cell is edited
     otherwise changes will not be committed.

     */
    void set_cell_edit_handler(const std::function<void(TreeNodeRef, int, std::string)> &handler);
#endif
    /** Sets a context menu to be attached to the treeview, to be shown on right click

     Note: Ownership of the context menu remains with the caller and it will not be freed
     when this object is deleted. */
    void set_context_menu(ContextMenu *menu);

    /** Sets a context menu to be attached to the treeviews header, to be shown on right click

     You can use the get_clicked_header_column() from the menu's will_show handler to get the column
     that was right clicked.

     Note: Ownership of the context menu remains with the caller and it will not be freed
     when this object is deleted. */
    void set_header_menu(ContextMenu *menu);

    /** Returns the context menu object attached to the treeview */
    ContextMenu *get_context_menu() {
      return _context_menu;
    }

    /** Returns the context menu object attached to the treeview header */
    ContextMenu *get_header_menu() {
      return _header_menu;
    }

    int get_clicked_header_column() {
      return _clicked_header_column;
    }

    /** Returns the current width of a column */
    int get_column_width(int column);

    /** Sets the widths of a column */
    void set_column_width(int column, int width);

  public:
    // backwards compatibility with TreeView

    int count() {
      return root_node()->count();
    }

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  public:
    // Following methods are for internal use.
    virtual void changed();
    virtual void node_activated(TreeNodeRef row, int column);
    virtual bool can_expand(TreeNodeRef row);

    // to be called BEFORE a node is expanded or AFTER its collapsed
    virtual void expand_toggle(TreeNodeRef row, bool expanded);

    // for use when sorting NumericWithUnitColumns in platform code
    static double parse_string_with_unit(const char *s);

    // To be called when user edits a cell. If this returns true, the cell value should be
    // updated by the caller, otherwise it should be left unchanged.
    virtual bool cell_edited(TreeNodeRef row, int column, const std::string &value);

    // Drag/drop support. The treeview has a different handling as dragging is initiated by the platform
    // and we call back to get data and formats (pull request). The general handling, in opposition,
    // is using a push request, initiating the drag in the backend.

    // Return format, data and other details for a drag operation that is about to start.
    // Called only if this tree can be a drag source at all (see create flags).
    // If the result of this call is false then no native drag operation is started.
    virtual bool get_drag_data(DragDetails &details, void **data, std::string &format);

    // Called only if get_drag_data returned true (and the tree can be a drag source).
    // The given operation tells what happened.
    virtual void drag_finished(DragOperation operation);

    virtual void column_resized(int column);

    // Called when the mouse hovers on a row
    std::vector<std::string> overlay_icons_for_node(TreeNodeRef row);

    // Called when mouse clicks on a overlay icon
    void overlay_icon_for_node_clicked(TreeNodeRef row, int index);

    // Called when right clicking on a header/title of a column, so that the context menu handler can know
    // what column is it being shown for
    void header_clicked(int column);

    void BeginUpdate();
    void EndUpdate();
#endif
#endif

  private:
    TreeViewImplPtrs *_treeview_impl;
    boost::signals2::signal<void()> _signal_changed;
    boost::signals2::signal<void(TreeNodeRef, int)> _signal_activated;
    boost::signals2::signal<void(TreeNodeRef, bool)> _signal_expand_toggle;
    std::function<void(TreeNodeRef, int, std::string)> _cell_edited;
    boost::signals2::signal<void(int)> _signal_column_resized;
    std::function<std::vector<std::string>(TreeNodeRef)> _overlay_icons_for_node;
    ContextMenu *_context_menu;
    ContextMenu *_header_menu;
    std::vector<TreeColumnType> _column_types;
    int _update_count;
    int _clicked_header_column;
    bool _index_on_tag;
    bool _end_column_called;
  };
}
