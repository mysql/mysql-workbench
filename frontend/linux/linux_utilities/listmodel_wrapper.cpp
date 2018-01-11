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

#include "listmodel_wrapper.h"
#include "gtk_helpers.h"
#include <gtkmm/cellrenderercombo.h>
#include <gtkmm/icontheme.h>
#include "base/string_utilities.h"

//#define DEBUG
#undef DEBUG

#ifdef DEBUG
#define lmwdprint(...) fprintf(stderr, __VA_ARGS__)
#else
#define lmwdprint(...)
#endif

static void process_menu_actions(
  const std::string& command, bec::ListModel* model, const std::vector<bec::NodeId>& nodes,
  const sigc::slot<void, const std::string&, const std::vector<bec::NodeId>&> fe_menu_handler) {
  if (!model->activate_popup_item_for_nodes(command, nodes) && !fe_menu_handler.empty())
    fe_menu_handler(command, nodes);
}

//------------------------------------------------------------------------------
static void run_menu_and_forward_action(
  const bec::MenuItemList& items, const int x, const int y, const int time, bec::ListModel* model,
  const std::vector<bec::NodeId>& nodes,
  const sigc::slot<void, const std::string&, const std::vector<bec::NodeId>&> fe_menu_handler, Gtk::Menu* menu) {
  if (!items.empty())
    run_popup_menu(items, time, sigc::bind(sigc::ptr_fun(process_menu_actions), model, nodes, fe_menu_handler), menu);
}

Index::ExternalMap Index::_ext_map;

//------------------------------------------------------------------------------
Index::Index(GtkTreeIter* it, const bec::NodeId& node) : _raw_data((char*)it), _ext(0) {
  // memset(_raw_data, 0xff, sizeof(*it));
  //*_raw_data = 0;
  reset_iter(it);

  const int depth = node.depth();

  const Mode m = (depth < MaxDepth) ? (depth == 1 ? ListNode : Internal) : External;
  mode(m);

  if (m == External) {
    const std::string nrepr = node.toString();
    std::pair<ExternalMap::iterator, bool> res = _ext_map.insert(nrepr);
    _ext = const_cast<std::string*>(&(*(res.first)));
    it->user_data = (void*)_ext;
  } else if (m == Internal) {
    for (int i = 0; i < depth; ++i)
      word(i, node[i]);
  } else if (m == ListNode) {
    it->user_data = (void*)(intptr_t)(node[0]);
  }
}

//------------------------------------------------------------------------------
Index::Index(const GtkTreeIter* it) : _raw_data((char*)it), _ext(0) {
  if (mode() == External)
    _ext = reinterpret_cast<std::string*>(((GtkTreeIter*)_raw_data)->user_data);
}

//------------------------------------------------------------------------------
void Index::stamp(const int st) {
  ((Info*)_raw_data)->stamp = st % StampMax;
}

//------------------------------------------------------------------------------
int Index::stamp() const {
  return ((Info*)_raw_data)->stamp;
}

//------------------------------------------------------------------------------
bool Index::cmp_stamp(const int st) const {
  return ((Info*)_raw_data)->stamp == (st % StampMax);
}

//------------------------------------------------------------------------------
int Index::word(const int w) const {
  int ret = 0;

  if (mode() == Internal) {
    char* dest = (char*)&ret;
    const int start_byte = (w * K) + 1;

    memcpy(dest, _raw_data + start_byte, K);
  }

  return ret;
}

//------------------------------------------------------------------------------
void Index::word(const int w, const int v) {
  if (mode() == Internal) {
    int start_byte = (w * K) + 1;
    char* dest = _raw_data + start_byte;
    char* src = (char*)&v;

    memcpy(dest, src, K);
  } else {
    throw std::logic_error("Can't change external Node ref\n");
  }
}

//------------------------------------------------------------------------------
bec::NodeId Index::to_node() const {
  bec::NodeId node;
  if (mode() == Internal) {
    int v;
    char* dst = (char*)&v;
    char* src = _raw_data + 1;

    for (int i = 0; i < MaxDepth; ++i) {
      v = MaxValue;
      memcpy(dst, src, K);
      src += K;
      if (v != MaxValue)
        node.append(v);
      else
        break;
    }
  } else if (mode() == External) {
    if (_ext)
      node = bec::NodeId(*_ext);
  } else if (mode() == ListNode)
    node.append((long)(((GtkTreeIter*)_raw_data)->user_data));

  return node;
}

//------------------------------------------------------------------------------
void Index::reset_iter(GtkTreeIter* it) {
  memset(it, 0xff, sizeof(*it));
  *((char*)it) = 0;
}

//------------------------------------------------------------------------------
Gtk::TreeModel::Path node2path(const ::bec::NodeId& node) {
  const int depth = node.depth();
  Gtk::TreeModel::Path path;

  for (int i = 0; i < depth; i++)
    path.push_back(node[i]);

  return path;
}

//------------------------------------------------------------------------------
ColumnsModel::~ColumnsModel() {
  reset(true); // true means clean up only self, and do not touch tree view
}

//------------------------------------------------------------------------------
void ColumnsModel::reset(const bool cleanup_only_self) {
  if (!cleanup_only_self)
    _treeview->remove_all_columns();

  std::list<Gtk::TreeModelColumnBase*>::iterator it = _columns.begin();
  std::list<Gtk::TreeModelColumnBase*>::const_iterator last = _columns.end();

  for (; last != it; ++it)
    delete *it;

  _columns.clear();
}

//------------------------------------------------------------------------------
void ColumnsModel::add_bec_index_mapping(const int bec_tm_index) {
  _ui2bec.push_back(bec_tm_index);
}

//------------------------------------------------------------------------------
int ColumnsModel::ui2bec(const int index_of_ui_column) const {
  g_assert((size_t)index_of_ui_column < _ui2bec.size());

  return _ui2bec[index_of_ui_column];
}

//------------------------------------------------------------------------------
const StringColumn& ColumnsModel::set_text_column(const int bec_tm_idx, const bool editable, Gtk::IconView* iv) {
  // Create columns
  // add mapping from ui to bec
  // add them with add() to the model
  // add columns to the IconView
  // check if editable and bind signals

  Gtk::TreeModelColumn<Glib::ustring>* text = new Gtk::TreeModelColumn<Glib::ustring>;
  Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>>* icon = new Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>>;

  _columns.push_back(text);
  _columns.push_back(icon);

  add_bec_index_mapping(bec_tm_idx);
  add_bec_index_mapping(bec_tm_idx);

  add(*text);
  add(*icon);

  iv->set_text_column(*text);
  iv->set_pixbuf_column(*icon);

  return *text;
}

//------------------------------------------------------------------------------
void ColumnsModel::add_tooltip_column(int bec_tm_idx) {
  Gtk::TreeModelColumn<Glib::ustring>* col = new Gtk::TreeModelColumn<Glib::ustring>;
  add(*col);
  add_bec_index_mapping(-bec_tm_idx); // negative means pick description

  _columns.push_back(col);
}

//------------------------------------------------------------------------------
const IntColumn& ColumnsModel::append_int_column(const int bec_tm_idx, const std::string& name,
                                                 const Editable editable) {
  Gtk::TreeModelColumn<int>* col = new Gtk::TreeModelColumn<int>;

  add(*col);
  add_bec_index_mapping(bec_tm_idx);

  int nr_of_cols;
  if (editable == EDITABLE) {
    nr_of_cols = _treeview->append_column_editable(base::replaceString(name, "_", "__"), *col);

    Gtk::CellRendererText* cell = (Gtk::CellRendererText*)(_treeview->get_column_cell_renderer(nr_of_cols - 1));
    cell->signal_edited().connect(
      sigc::bind(sigc::mem_fun(*_tmw, &ListModelWrapper::after_cell_edit<int>), sigc::ref(*col)));
  } else
    nr_of_cols = _treeview->append_column(base::replaceString(name, "_", "__"), *col);

  _treeview->get_column(nr_of_cols - 1)->set_resizable(true);

  _columns.push_back(col);

  return *col;
}

//------------------------------------------------------------------------------
const DoubleColumn& ColumnsModel::append_double_column(const int bec_tm_idx, const std::string& name,
                                                       const Editable editable) {
  Gtk::TreeModelColumn<double>* col = new Gtk::TreeModelColumn<double>;
  add_bec_index_mapping(bec_tm_idx);
  add(*col);

  _columns.push_back(col);

  return *col;
}

//------------------------------------------------------------------------------
void ColumnsModel::disable_edit_first_row(Gtk::CellRenderer* cell, const Gtk::TreeIter& iter) {
  Gtk::CellRendererText* txt = (Gtk::CellRendererText*)cell;
  if (txt) {
    Gtk::TreeModel::Path path = this->_treeview->get_model()->get_path(iter);
    if (path[0] == 0)
      txt->property_editable() = false;
    else
      txt->property_editable() = true;
  }
}

const StringColumn& ColumnsModel::append_string_column(const int bec_tm_idx, const std::string& name,
                                                       const Editable editable, const Iconic have_icon) {
  Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>>* icon = 0;

  Gtk::TreeViewColumn* column = Gtk::manage(new Gtk::TreeViewColumn(base::replaceString(name, "_", "__")));

  if (have_icon == WITH_ICON) {
    icon = new Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>>;
    add(*icon);
    add_bec_index_mapping(bec_tm_idx);
    column->pack_start(*icon, false);

    _columns.push_back(icon);
  }

  Gtk::TreeModelColumn<Glib::ustring>* col = new Gtk::TreeModelColumn<Glib::ustring>;
  add(*col);
  add_bec_index_mapping(bec_tm_idx);
  column->pack_start(*col);

  _columns.push_back(col);

  int nr_of_cols = _treeview->append_column(*column);
  _treeview->get_column(nr_of_cols - 1)->set_resizable(true);

  if (editable == EDITABLE || editable == EDITABLE_WO_FIRST) {
    std::vector<Gtk::CellRenderer*> rends = column->get_cells();

    Gtk::CellRendererText* cell = (Gtk::CellRendererText*)rends[icon ? 1 : 0];
    cell->property_editable() = true;
    cell->signal_edited().connect(
      sigc::bind(sigc::mem_fun(*_tmw, &ListModelWrapper::after_cell_edit<Glib::ustring>), sigc::ref(*col)));

    if (editable == EDITABLE_WO_FIRST)
      column->set_cell_data_func(*cell, sigc::mem_fun(this, &ColumnsModel::disable_edit_first_row));
  }

  return *col;
}

//------------------------------------------------------------------------------
const StringColumn& ColumnsModel::append_markup_column(const int bec_tm_idx, const std::string& name,
                                                       const Iconic have_icon) {
  Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>>* icon = 0;

  Gtk::TreeViewColumn* column = Gtk::manage(new Gtk::TreeViewColumn(base::replaceString(name, "_", "__")));

  if (have_icon == WITH_ICON) {
    icon = new Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>>;
    add(*icon);
    add_bec_index_mapping(bec_tm_idx);
    column->pack_start(*icon, false);

    _columns.push_back(icon);
  }

  Gtk::TreeModelColumn<Glib::ustring>* col = new Gtk::TreeModelColumn<Glib::ustring>;
  Gtk::CellRendererText* cell = Gtk::manage(new Gtk::CellRendererText());
  add(*col);
  add_bec_index_mapping(bec_tm_idx);
  column->pack_start(*cell);
  column->add_attribute(cell->property_markup(), *col);

  _columns.push_back(col);

  int nr_of_cols = _treeview->append_column(*column);
  _treeview->get_column(nr_of_cols - 1)->set_resizable(true);

  return *col;
}

//------------------------------------------------------------------------------
const StringColumn& ColumnsModel::append_combo_column(const int bec_tm_idx, const std::string& name,
                                                      Glib::RefPtr<Gtk::ListStore> list_w, const Editable editable,
                                                      bool popup_only) {
  Gtk::TreeModelColumn<Glib::ustring>* choosen = new Gtk::TreeModelColumn<Glib::ustring>;
  _columns.push_back(choosen);
  add(*choosen);
  add_bec_index_mapping(bec_tm_idx);

  Gtk::TreeView::Column* col = Gtk::manage(new Gtk::TreeViewColumn(base::replaceString(name, "_", "__")));
  Gtk::CellRendererCombo* cell = Gtk::manage(new Gtk::CellRendererCombo);
  col->pack_start(*cell);

  col->add_attribute(cell->property_text(), *choosen);
  cell->property_model() = list_w;
  cell->property_text_column() = 0;
  cell->property_editable() = editable;
  cell->property_has_entry() = !popup_only;

  Gtk::TreeModelColumn<Glib::RefPtr<Gtk::TreeModel>>* model_col =
    new Gtk::TreeModelColumn<Glib::RefPtr<Gtk::TreeModel>>();
  add_bec_index_mapping(bec_tm_idx);
  add(*model_col);
  const int nr_of_cols = _treeview->append_column(*col);

  _columns.push_back(model_col);

  _treeview->get_column(nr_of_cols - 1)->set_resizable(true);

  if (editable == EDITABLE) {
    Gtk::CellRendererText* cell = (Gtk::CellRendererText*)(_treeview->get_column_cell_renderer(nr_of_cols - 1));
    cell->signal_edited().connect(
      sigc::bind(sigc::mem_fun(*_tmw, &ListModelWrapper::after_cell_edit<Glib::ustring>), sigc::ref(*choosen)));
  }

  return *choosen;
}

//------------------------------------------------------------------------------
int ColumnsModel::append_check_column(const int bec_tm_idx, const std::string& name, const Editable editable,
                                      const ToggleAction action) {
  Gtk::TreeModelColumn<bool>* col = new Gtk::TreeModelColumn<bool>;
  _columns.push_back(col);
  add(*col);
  add_bec_index_mapping(bec_tm_idx);

  int nr_of_cols;
  // If we have bec_tm_idx set to negative value it means that column added is not
  // directly mapped to a model. Handling of values set/get are done through
  // ListModelWrapper::_fake_column_value_getter/setter slot
  if (editable == EDITABLE) {
    nr_of_cols = _treeview->append_column_editable(base::replaceString(name, "_", "__"), *col);

    Gtk::CellRendererToggle* cell = (Gtk::CellRendererToggle*)(_treeview->get_column_cell_renderer(nr_of_cols - 1));
    cell->property_activatable() = true;
    if (action == TOGGLE_BY_WRAPPER /* && bec_tm_idx >= 0 */) {
      cell->signal_toggled().connect(sigc::bind(sigc::mem_fun(*_tmw, &ListModelWrapper::after_cell_toggle /*<bool>*/
                                                              ),
                                                sigc::ref(*col)));
    }
  } else
    nr_of_cols = _treeview->append_column(base::replaceString(name, "_", "__"), *col);

  _treeview->get_column(nr_of_cols - 1)->set_resizable(true);

  return nr_of_cols;
}

//------------------------------------------------------------------------------

int ColumnsModel::add_generic_column(const int bec_tm_idx, Gtk::TreeModelColumnBase* column,
                                     Gtk::TreeViewColumn* vcolumn) {
  add(*column);
  add_bec_index_mapping(bec_tm_idx);

  return _treeview->append_column(*vcolumn);
}

//------------------------------------------------------------------------------
void ColumnsModel::add_model_column(Gtk::TreeModelColumnBase* col, int bec_tm_idx) {
  add(*col);
  add_bec_index_mapping(bec_tm_idx);
  _columns.push_back(col);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

ListModelWrapper::ListModelWrapper(bec::ListModel* tm, Gtk::TreeView* treeview, const std::string& name)
  : Glib::ObjectBase(typeid(ListModelWrapper)),
    Glib::Object(),
    _treeview(treeview),
    _iconview(0),
    _context_menu(0),
    _stamp(1),
    _columns(this, treeview),
    _icon_size(bec::Icon16),
    _name(name) {
  _invalid = false;

  scoped_connect(tm->tree_changed_signal(),
                 std::bind(&::ListModelWrapper::model_changed, this, std::placeholders::_1, std::placeholders::_2));

  // keep an indirect pointer because sometimes bec::ListModel is destroyed before the wrapper
  // and in that case we don't want to keep using the dead pointer
  _tm = new bec::ListModel*();
  *_tm = tm;

  tm->add_destroy_notify_callback(_tm, &ListModelWrapper::on_bec_model_destroyed);
#ifndef NO_MENU_MANAGER
  if (_treeview)
    _treeview->signal_event().connect(sigc::mem_fun(this, &ListModelWrapper::handle_popup_event));
#endif
}

//------------------------------------------------------------------------------
void* ListModelWrapper::on_bec_model_destroyed(void* data) {
  bec::ListModel** ptr = (bec::ListModel**)data;
  *ptr = 0;
  return 0;
}

//------------------------------------------------------------------------------
ListModelWrapper::~ListModelWrapper() {
  delete _context_menu;
  if (*_tm)
    (*_tm)->remove_destroy_notify_callback(_tm);
  *_tm = 0;
}

//------------------------------------------------------------------------------
void ListModelWrapper::set_iconview(Gtk::IconView* iv) {
  _iconview = iv;
#ifndef NO_MENU_MANAGER
  if (_iconview)
    _iconview->signal_event().connect(sigc::mem_fun(this, &ListModelWrapper::handle_popup_event));
#endif
}

//------------------------------------------------------------------------------
ListModelWrapper::NodeIdArray ListModelWrapper::get_selection() const {
  NodeIdArray nodes;
  std::vector<Gtk::TreePath> paths;

  if (_treeview)
    paths = _treeview->get_selection()->get_selected_rows();
  else if (_iconview)
    paths = _iconview->get_selected_items();

  const int size = paths.size();
  nodes.reserve(size);
  for (int i = 0; i < size; ++i)
    nodes.push_back(get_node_for_path(paths[i]));

  return nodes;
}

//------------------------------------------------------------------------------
void ListModelWrapper::set_be_model(bec::ListModel* tm) {
  if (*_tm)
    (*_tm)->remove_destroy_notify_callback(_tm);
  *_tm = tm;
  if (*_tm) {
    (*_tm)->add_destroy_notify_callback(_tm, &ListModelWrapper::on_bec_model_destroyed);
  }
}

#ifndef NO_MENU_MANAGER
//------------------------------------------------------------------------------
void ListModelWrapper::handle_popup(const int x, const int y, const int time, GdkEventButton* evb) {
  Gtk::TreeModel::Path path;
  Gtk::TreeView::Column* column(0);
  int cell_x(-1);
  int cell_y(-1);

  ListModelWrapper::NodeIdArray list = get_selection();

  bool there_is_path_at_pos = false;
  if (_treeview)
    there_is_path_at_pos = _treeview->get_path_at_pos(x, y, path, column, cell_x, cell_y);
  else if (_iconview) {
    path = _iconview->get_path_at_pos(x, y);
    there_is_path_at_pos = path.gobj() && !path.empty();
  }

  if (there_is_path_at_pos) {
    // Check that @path is on selection, otherwise add @path to selection
    bec::NodeId node = get_node_for_path(path);
    // list stores current selection
    bool path_at_pos_is_in_selection = false;
    for (int i = list.size() - 1; i >= 0; --i) {
      if (node == list[i]) {
        path_at_pos_is_in_selection = true;
        break;
      }
    }

    if (!path_at_pos_is_in_selection) {
      // Add it, if user holds Ctrl while clicking right mouse btn
      // Otherwise clear selection, and select only @path
      const bool clear_selection = evb ? (!(evb->state & GDK_CONTROL_MASK)) : false;
      if (clear_selection) {
        if (_treeview)
          _treeview->get_selection()->unselect_all();
        if (_iconview)
          _iconview->unselect_all();
      }

      if (_treeview)
        _treeview->get_selection()->select(path);
      if (_iconview)
        _iconview->select_path(path);

      list = get_selection();
    }
  }
  if (!_context_menu)
    _context_menu = new Gtk::Menu();

  run_menu_and_forward_action((*_tm)->get_popup_items_for_nodes(list), x, y, time, *_tm, list, _fe_menu_handler,
                              _context_menu);
}

//------------------------------------------------------------------------------
bool ListModelWrapper::handle_popup_event(GdkEvent* event) {
  bool ret = false;
  if ((event->type == GDK_BUTTON_PRESS && event->button.button == 3) ||
      (event->type == GDK_KEY_RELEASE && event->key.keyval == GDK_KEY_Menu)) {
    GdkEventButton* evb = (GdkEventButton*)event;
    handle_popup((int)event->button.x, (int)event->button.y, event->button.time, evb);
    // Return true to preserve selection. In handle_popup check that popup is requested
    // on selection.
    ret = true;
  }
  return ret;
}
#endif

//------------------------------------------------------------------------------
Gtk::TreeModelFlags ListModelWrapper::get_flags_vfunc() const {
  return Gtk::TREE_MODEL_ITERS_PERSIST | Gtk::TREE_MODEL_LIST_ONLY;
}

//------------------------------------------------------------------------------
bec::NodeId ListModelWrapper::get_node_for_path(const Gtk::TreeModel::Path& path) const {
  if (path.empty())
    return bec::NodeId();
  return bec::NodeId(path.to_string());
}

//------------------------------------------------------------------------------
bool ListModelWrapper::init_gtktreeiter(GtkTreeIter* it, const bec::NodeId& node) const {
  if (*_tm && it && node.is_valid()) {
    Index id(it, node);
    id.stamp(_stamp);
  }
  return it && node.is_valid();
}

//------------------------------------------------------------------------------
bec::NodeId ListModelWrapper::node_for_iter(const iterator& iter) const {
  const GtkTreeIter* it = iter.gobj();
  bec::NodeId node;

  if (it) {
    const Index id(it);
    if (id.cmp_stamp(_stamp))
      node = id.to_node();
  }

  return node;
}

//------------------------------------------------------------------------------
void ListModelWrapper::refresh() {
  if (*_tm)
    (*_tm)->refresh();
  model_changed(bec::NodeId(), -1);
}

//------------------------------------------------------------------------------
void ListModelWrapper::note_row_added() {
  if (*_tm) {
    (*_tm)->refresh();
    Gtk::TreePath path((*_tm)->count() - 1);

    row_inserted(path, get_iter(path));
  }
}

//------------------------------------------------------------------------------
void ListModelWrapper::set_fake_column_value_getter(FakeColumnValueGetter fake_getter) {
  _fake_column_value_getter = fake_getter;
}

//------------------------------------------------------------------------------
void ListModelWrapper::set_fake_column_value_setter(FakeColumnValueSetter fake_setter) {
  _fake_column_value_setter = fake_setter;
}

//------------------------------------------------------------------------------
void ListModelWrapper::reset_iter(iterator& iter) const throw() {
  GtkTreeIter* it = iter.gobj();
  Index::reset_iter(it);
}

//------------------------------------------------------------------------------
int ListModelWrapper::get_n_columns_vfunc() const {
  return _columns.size();
}

//------------------------------------------------------------------------------
GType ListModelWrapper::get_column_type_vfunc(int index) const {
  return *(_columns.types() + index);
}

//------------------------------------------------------------------------------
bool ListModelWrapper::iter_next_vfunc(const iterator& iter, iterator& iter_next) const {
  bool ret = false;
  bec::NodeId current_node = node_for_iter(iter);

  // g_message("LMW::iter_next_vfunc: %s", _name.c_str());

  // Invalidate iter_next
  reset_iter(iter_next);

  if (*_tm) {
    if (current_node.is_valid() && (*_tm)->has_next(current_node)) {
      try {
        // Obtain parent of the current node to get number of children of the parent node
        current_node = (*_tm)->get_next(current_node);
      } catch (...) {
        current_node = bec::NodeId();
      }
      // Check if the resulted nodeid is valid and setup iter_next with values pointing to the nodeid
      if (current_node.is_valid())
        ret = init_gtktreeiter(iter_next.gobj(), current_node);
    }
  }
  return ret;
}

//------------------------------------------------------------------------------
bool ListModelWrapper::get_iter_vfunc(const Path& path, iterator& iter) const {
  bool ret = false;
  GtkTreeIter* it = iter.gobj();

  // Invalidate iter_next
  reset_iter(iter);

  if (*_tm) {
    bec::NodeId node(path.to_string());

    if (node.is_valid() && node.back() < (*_tm)->count())
      ret = init_gtktreeiter(it, node);
  }
  return ret;
}

//------------------------------------------------------------------------------
bool ListModelWrapper::iter_children_vfunc(const iterator& parent, iterator& iter) const {
  return false;
}

//------------------------------------------------------------------------------
bool ListModelWrapper::iter_parent_vfunc(const iterator& child, iterator& iter) const {
  return false;
}

//------------------------------------------------------------------------------
bool ListModelWrapper::iter_nth_child_vfunc(const iterator& parent, int n, iterator& iter) const {
  return false;
}

//------------------------------------------------------------------------------
bool ListModelWrapper::iter_nth_root_child_vfunc(int n, iterator& iter) const {
  bool ret = false;
  // Sets @a iter to be the child of at the root level using the given index.  The first
  // index is 0.  If @a n is too big, or if there are no children, @a iter is set
  // to an invalid iterator and false is returned.
  // See also iter_nth_child_vfunc().
  if (*_tm && n >= 0 && n < iter_n_root_children_vfunc()) {
    bec::NodeId node = (*_tm)->get_node(n);
    init_gtktreeiter(iter.gobj(), node);
    ret = true;
  }

  return ret;
}

//------------------------------------------------------------------------------
bool ListModelWrapper::iter_has_child_vfunc(const iterator& iter) const {
  return iter_n_children_vfunc(iter) != 0;
}

//------------------------------------------------------------------------------
int ListModelWrapper::iter_n_children_vfunc(const iterator& iter) const {
  return 0;
}

//------------------------------------------------------------------------------
int ListModelWrapper::iter_n_root_children_vfunc() const {
  const int ret = *_tm ? (*_tm)->count() : 0;
  return ret;
}

//------------------------------------------------------------------------------
Gtk::TreeModel::Path ListModelWrapper::get_path_vfunc(const iterator& iter) const {
  const bec::NodeId node = node_for_iter(iter);
  Gtk::TreeModel::Path path;

  if (node.is_valid()) {
    const int node_depth = node.depth();

    for (int i = 0; i < node_depth; i++)
      path.push_back(node[i]);
  }

  return path;
}

//------------------------------------------------------------------------------
void ListModelWrapper::get_icon_value(const iterator& iter, int column, const bec::NodeId& node,
                                      Glib::ValueBase& value) const {
  if (!*_tm)
    return;

  static ImageCache* pixbufs = ImageCache::get_instance();
  static Glib::RefPtr<Gtk::IconTheme> icon_theme = Gtk::IconTheme::get_default();
  bec::IconId icon_id = (*_tm)->get_field_icon(node, column, get_icon_size());
  value.init(Glib::Value<Glib::RefPtr<Gdk::Pixbuf>>::value_type());
  if (icon_id != 0 && icon_id != -1) {
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = pixbufs->image(icon_id);
    if (pixbuf) {
      Glib::Value<Glib::RefPtr<Gdk::Pixbuf>> pixbufValue;
      pixbufValue.init(Glib::Value<Glib::RefPtr<Gdk::Pixbuf>>::value_type());
      pixbufValue.set(pixbuf);
      value = pixbufValue;
    }
  }
}

//------------------------------------------------------------------------------
void ListModelWrapper::get_value_vfunc(const iterator& iter, int column, Glib::ValueBase& value) const {
  if (!*_tm)
    return;

  bec::NodeId node = node_for_iter(iter);

  if (node.is_valid()) {
    const GType type = *(_columns.types() + column);

    column = _columns.ui2bec(column);

    if (column < 0) {
      if (!_fake_column_value_getter.empty())
        _fake_column_value_getter(iter, column, type, value);
    } else if (type == GDK_TYPE_PIXBUF) {
      get_icon_value(iter, column, node, value);
    } else {
      switch (type) {
        case G_TYPE_BOOLEAN: {
          bool bv = false;
          (*_tm)->get_field(node, column, bv);
          set_glib_bool(value, bv);
          lmwdprint("LMW::get_value_vfunc: %s: node %s, col %i: value: %i\n", _name.c_str(), node.repr().c_str(),
                    column, bv);
          break;
        }
        case G_TYPE_INT:
        case G_TYPE_UINT: {
          ssize_t iv = 0;
          (*_tm)->get_field(node, column, iv);
          set_glib_int(value, iv);
          lmwdprint("LMW::get_value_vfunc: %s: node %s, col %i: value: %i\n", _name.c_str(), node.repr().c_str(),
                    column, iv);
          break;
        }
        case G_TYPE_LONG:
        case G_TYPE_ULONG:
        case G_TYPE_INT64:
        case G_TYPE_UINT64: {
          throw std::logic_error("Imlement long ints in get_value_func");
          break;
        }
        case G_TYPE_FLOAT:
        case G_TYPE_DOUBLE: {
          double dv = 0.0;
          (*_tm)->get_field(node, column, dv);
          set_glib_double(value, dv);
          lmwdprint("LMW::get_value_vfunc: %s: node %s, col %i: value: %f\n", _name.c_str(), node.repr().c_str(),
                    column, dv);
          break;
        }
        case G_TYPE_STRING: {
          std::string sv;
          if (column < 0)
            sv = (*_tm)->get_field_description(node, -column);
          else
            (*_tm)->get_field_repr(node, column, sv);
          set_glib_string(value, sv, true);
          lmwdprint("LMW::get_value_vfunc: %s: node %s, col %i: value: '%s'\n", _name.c_str(), node.repr().c_str(),
                    column, sv.c_str());
          break;
        }
        default:
          set_glib_string(value, "<unkn>");
          break;
      }
    }
  }
}

//------------------------------------------------------------------------------
bool ListModelWrapper::iter_is_valid(const iterator& iter) const {
  bec::NodeId node(node_for_iter(iter));

  return node.is_valid();
}

//------------------------------------------------------------------------------
void ListModelWrapper::set_value_impl(const iterator& row, int column, const Glib::ValueBase& value) {
  if (!*_tm)
    return;

  bec::NodeId node(node_for_iter(row));

  if (node.is_valid()) {
    const GType type = *(_columns.types() + column);
    column = _columns.ui2bec(column);

    if (column < 0) {
      if (!_fake_column_value_setter.empty())
        _fake_column_value_setter(row, column, type, value);
    } else {
      switch (type) {
        case G_TYPE_BOOLEAN: {
          Glib::Value<bool> v;
          v.init(value.gobj());
          lmwdprint("LMW::set_value_impl:%s node %s, column %i, value %i\n", _name.c_str(), node.repr().c_str(), column,
                    v.get());
          (*_tm)->set_field(node, column, (ssize_t)v.get());
          break;
        }
        case G_TYPE_INT:
        case G_TYPE_UINT: {
          Glib::Value<ssize_t> v;
          v.init(value.gobj());
          lmwdprint("LMW::set_value_impl: node %s, column %i, value %i\n", node.repr().c_str(), column, v.get());
          (*_tm)->set_field(node, column, v.get());
          break;
        }
        case G_TYPE_FLOAT:
        case G_TYPE_DOUBLE: {
          Glib::Value<double> v;
          v.init(value.gobj());
          (*_tm)->set_field(node, column, v.get());
          break;
        }
        case G_TYPE_STRING: {
          Glib::Value<std::string> v;
          v.init(value.gobj());
          (*_tm)->set_field(node, column, v.get());
          lmwdprint("LMW::set_value: %s '%s'\n", _name.c_str(), v.get().c_str());
          break;
        }
        default:
          break;
      }
    }
  }
}

//------------------------------------------------------------------------------
void ListModelWrapper::get_value_impl(const iterator& row, int column, Glib::ValueBase& value) const {
  get_value_vfunc(row, column, value);
}

//------------------------------------------------------------------------------
void ListModelWrapper::set_row_draggable_slot(const sigc::slot<bool, Gtk::TreeModel::Path>& slot) {
  _row_draggable = slot;
}

//------------------------------------------------------------------------------
bool ListModelWrapper::drag_data_get_vfunc(const Gtk::TreeModel::Path& path, Gtk::SelectionData& selection_data) const {
  selection_data.set("text/path", path.to_string());
  return true;
}

//------------------------------------------------------------------------------
bool ListModelWrapper::drag_data_delete_vfunc(const Gtk::TreeModel::Path& path) {
  return true;
}

//------------------------------------------------------------------------------
bool ListModelWrapper::row_draggable_vfunc(const TreeModel::Path& path) const {
  if (_row_draggable)
    return _row_draggable(path);

  return true;
}

//------------------------------------------------------------------------------
bool ListModelWrapper::row_drop_possible_vfunc(const Gtk::TreeModel::Path& dest,
                                               const Gtk::SelectionData& selection_data) const {
  return true;
}

//------------------------------------------------------------------------------
bool ListModelWrapper::drag_data_received_vfunc(const Gtk::TreeModel::Path& dest,
                                                const Gtk::SelectionData& selection_data) {
  bool ret = false;
  // Currently this works for linear lists
  try {
    (*_tm)->reorder(bec::NodeId((const char*)selection_data.get_data()), dest.front());
    ret = true;
  } catch (const std::logic_error& e) {
  }
  return ret;
}

//------------------------------------------------------------------------------
void ListModelWrapper::invalidate() {
  _invalid = true;
  *_tm = 0;
  _stamp++;
}
