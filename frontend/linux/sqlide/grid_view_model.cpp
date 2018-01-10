/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "sqlide/grid_view_model.h"
#include "sqlide/grid_view.h"
#include "linux_utilities/gtk_helpers.h"
#include "custom_renderers.h"
#include "base/string_utilities.h"

GridViewModel::Ref GridViewModel::create(bec::GridModel::Ref model, GridView *view, const std::string &name) {
  return Ref(new GridViewModel(model, view, name));
}

GridViewModel::GridViewModel(bec::GridModel::Ref model, GridView *view, const std::string &name)
  : Glib::ObjectBase(typeid(GridViewModel)),
    ListModelWrapper(model.get(), view, name),
    _model(model),
    _view(view),
    _row_numbers_visible(true),
    _text_cell_fixed_height(false) {
  _ignore_column_resizes = 0;
  view->set_rules_hint(true); // enable alternating row colors
  set_fake_column_value_getter(sigc::mem_fun(this, &GridViewModel::get_cell_value));
  // set_fake_column_value_setter(sigc::mem_fun(this, &GridViewModel::set_cell_value));
}

GridViewModel::~GridViewModel() {
}

template <size_t ValueTypeCode = bec::GridModel::StringType>
struct ValueTypeTraits {
  typedef Glib::ustring ValueType;
  typedef Gtk::CellRendererText Renderer;
  typedef Glib::ustring RendererValueType;
};

template <>
struct ValueTypeTraits<bec::GridModel::NumericType> {
  typedef int ValueType;
  typedef Gtk::CellRendererSpin Renderer;
  typedef Glib::ustring RendererValueType;
};

template <>
struct ValueTypeTraits<bec::GridModel::FloatType> {
  typedef double ValueType;
  typedef Gtk::CellRendererText Renderer;
  typedef Glib::ustring RendererValueType;
};

int GridViewModel::refresh(bool reset_columns) {
  freeze_notify();
  model_changed(bec::NodeId(), -1);

  if (reset_columns) {
    ColumnsModel &columns = model();
    columns.reset();
    _col_index_map.clear();

    // aux columns
    Gtk::TreeModelColumn<Gdk::Color> *color_column;
    {
      color_column = new Gtk::TreeModelColumn<Gdk::Color>;
      columns.add_model_column(color_column, -1);

      if (_row_numbers_visible) {
        Gtk::TreeViewColumn *col = add_column<ValueTypeTraits<> >(-2, "#", RO, NULL);
        (*col->get_cells().begin())->property_cell_background() = "LightGray";
        col->set_min_width(35);
        col->set_resizable(true);
      }
    }

    ignore_column_resizes(true);
    bool is_model_editable = !_model->is_readonly();
    for (int index = 0, count = _model->get_column_count(); index < count; ++index) {
      Editable is_col_editable =
        (is_model_editable && (bec::GridModel::BlobType != _model->get_column_type(index))) ? EDITABLE : RO;
      std::string label = base::sanitize_utf8(_model->get_column_caption(index));
      bec::GridModel::ColumnType type = _model->get_column_type(index);
      Gtk::TreeViewColumn *col;
      switch (type) {
        case bec::GridModel::NumericType:
          col = add_column<ValueTypeTraits<bec::GridModel::NumericType> >(index, label, is_col_editable, 0);
          col->set_min_width(10);
          col->set_fixed_width(50);
          break;
        case bec::GridModel::FloatType:
          col = add_column<ValueTypeTraits<bec::GridModel::FloatType> >(index, label, is_col_editable, 0);
          col->set_min_width(10);
          col->set_fixed_width(50);
          break;
        default:
          col = add_column<ValueTypeTraits<bec::GridModel::StringType> >(index, label, is_col_editable, 0);
          col->set_min_width(10);
          col->set_fixed_width(100);
          break;
      }
      col->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
      col->set_resizable(true);
      _current_column_size[index] = col->get_width();
    }

    // horrible hack to workaround shitty gtk column resizing, needed to make the last column resizable
    // even in Ubuntu's bizarre scrollbar that covers the resize area and also make the last column have a fixed size,
    // instead of using all available space when there's more than needed
    Gtk::TreeViewColumn *col = add_column<ValueTypeTraits<bec::GridModel::StringType> >(-3, "", RO, NULL);
    col->set_min_width(5);
    col->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
    col->set_expand(true);
    col->set_resizable(true);

    ignore_column_resizes(false);
  }
  thaw_notify();
  return 0;
}

void GridViewModel::onColumnsResized(const std::vector<Gtk::TreeViewColumn *> &cols) {
  if (_ignore_column_resizes != 0)
    return;
  std::vector<Gtk::TreeViewColumn *>::const_iterator it;
  std::vector<int> columns;
  columns.resize(cols.size());
  for (it = cols.begin(); it != cols.end(); ++it) {
    int column_idx = column_index(*it);
    if (_current_column_size[column_idx] != (*it)->get_width()) {
      _current_column_size[column_idx] = (*it)->get_width();
      columns.push_back(column_idx);
    }
  }

  if (!columns.empty() && columns_resized)
    columns_resized(columns);
}

void GridViewModel::set_column_width(int column, int width) {
  ignore_column_resizes(true);
  Gtk::TreeViewColumn *tc = _view->get_column(column + 1);
  if (tc)
    tc->set_fixed_width(width);
  ignore_column_resizes(false);
}

void GridViewModel::set_text_cell_fixed_height(bool val) {
  _text_cell_fixed_height = val;
}

void GridViewModel::on_column_header_button_press(GdkEventButton *ev, Gtk::TreeViewColumn *column) {
  if (ev->button == 3) {
    int col = column_index(column);
    column_right_clicked(col, ev->x, ev->y);
  }
}

template <typename ValueTypeTraits>
Gtk::TreeViewColumn *GridViewModel::add_column(int index, const std::string &name, Editable editable,
                                               Gtk::TreeModelColumnBase *color_column) {
  ColumnsModel &columns = model();

  typedef Gtk::TreeModelColumn<typename ValueTypeTraits::ValueType> ModelColumn;
  ModelColumn *col = new ModelColumn();
  columns.add_model_column(col, index);
  Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > *icon = NULL;
  if (index != -3) {
    icon = new Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> >();
    columns.add_model_column(icon, index);
  }
  typedef CustomRenderer<typename ValueTypeTraits::Renderer, typename ValueTypeTraits::RendererValueType,
                         typename ValueTypeTraits::ValueType>
    CustomRenderer;
  CustomRenderer *renderer = Gtk::manage(new CustomRenderer());

  Gtk::CellRenderer *rendr = &renderer->_data_renderer;
  if (GTK_IS_CELL_RENDERER_TEXT(rendr->gobj())) {
    Gtk::CellRendererText *txtrendr = (Gtk::CellRendererText *)rendr;
    if (_text_cell_fixed_height)
      txtrendr->set_fixed_height_from_font(
        1); // This will assume that there's only one row of text in the cell so the height check will be much faster.
    else
      txtrendr->set_fixed_height_from_font(-1);
  }

  renderer->floating_point_visible_scale(_model->floating_point_visible_scale());

  renderer->set_edit_state = sigc::bind(sigc::mem_fun(_model.get(), &::bec::GridModel::set_edited_field), index);
  Gtk::TreeViewColumn *treeview_column = renderer->bind_columns(_view, name, index, col, icon);
  if (index >= 0 || index == -2) {
    treeview_column->signal_clicked().connect(
      sigc::bind(sigc::mem_fun(_view, &GridView::on_column_header_clicked), treeview_column, index));
    treeview_column->set_clickable();
  }

  if (index >= 0) {
    Gtk::Label *label = Gtk::manage(new Gtk::Label(name));
    label->show();
    treeview_column->set_widget(*label);
    // another hack to allow right click catching in header
    Gtk::Button *btn = (Gtk::Button *)label->get_ancestor(GTK_TYPE_BUTTON);
    if (btn)
      btn->signal_button_press_event().connect_notify(
        sigc::bind(sigc::mem_fun(this, &GridViewModel::on_column_header_button_press), treeview_column));
    else
      g_warning("Could not find a button widget for treeview header\n");
  }
  if (color_column)
    treeview_column->add_attribute(renderer->property_cell_background_gdk(), *color_column);
  _col_index_map[treeview_column] = index;

  set_ellipsize(index, true);
  // TODO: implement editable wothout first row
  if (editable == EDITABLE || editable == EDITABLE_WO_FIRST) {
    renderer->property_editable() = true;
    renderer->signal_edited().connect(sigc::bind(
      sigc::mem_fun(*this, &GridViewModel::after_cell_edit<typename ValueTypeTraits::ValueType>), sigc::ref(*col)));
    renderer->signal_edited().connect(sigc::mem_fun(_view, &GridView::on_cell_edited));
    renderer->signal_editing_started().connect(
      sigc::bind(sigc::mem_fun(_view, &GridView::on_cell_editing_started), treeview_column));
    renderer->signal_editing_canceled().connect(sigc::mem_fun(_view, &GridView::on_cell_editing_done));
  }

  return treeview_column;
}

void GridViewModel::set_ellipsize(const int column, const bool on) {
  Gtk::TreeViewColumn *col = 0;
  for (std::map<Gtk::TreeViewColumn *, int>::const_iterator end = _col_index_map.end(), it = _col_index_map.begin();
       it != end; ++it) {
    if (it->second == column) {
      col = it->first;
      break;
    }
  }

  if (col) {
    const std::vector<Gtk::CellRenderer *> rends = col->get_cells();
    const int rends_size = rends.size();
    for (int i = 0; i < rends_size; ++i) {
      CustomRendererOps *cr = dynamic_cast<CustomRendererOps *>(rends[i]);
      if (cr) {
        Gtk::CellRendererText *crt = dynamic_cast<Gtk::CellRendererText *>(cr->data_renderer());
        crt->property_ellipsize() = on ? Pango::ELLIPSIZE_END : Pango::ELLIPSIZE_NONE;
        crt->property_ellipsize_set() = on;
      }
    }
  }
}

int GridViewModel::column_index(Gtk::TreeViewColumn *col) {
  std::map<Gtk::TreeViewColumn *, int>::const_iterator i = _col_index_map.find(col);
  return (_col_index_map.end() == i) ? -1 : i->second;
}

void GridViewModel::get_cell_value(const iterator &iter, int column, GType type, Glib::ValueBase &value) {
  bec::NodeId node = node_for_iter(iter);
  if (!node.is_valid())
    return;

  switch (column) {
    case -1: {
      //    static const Gdk::Color odd_row_color("white");
      //    static const Gdk::Color even_row_color("snow2");
      //    const Gdk::Color *color= (node[0]%2) ? &even_row_color : &odd_row_color;

      g_value_init(value.gobj(), GDK_TYPE_RGBA);
      g_value_set_boxed(value.gobj(), 0);
    } break;
    case -2: {
      if (type == GDK_TYPE_PIXBUF)
        g_value_init(value.gobj(), GDK_TYPE_PIXBUF);
      else {
        std::ostringstream oss;
        size_t row = node[0] + 1;
        if (_model->is_readonly() || (row < _model->count()))
          oss << row;
        else
          oss << "*";
        set_glib_string(value, oss.str().c_str());
      }
    } break;

    case -3:
      set_glib_string(value, "");
      break;
  }
}

void GridViewModel::set_cell_value(const iterator &itier, int column, GType type, const Glib::ValueBase &value) {
}

bool GridViewModel::handle_popup_event(GdkEvent *event) {
  return false;
}

void GridViewModel::get_value_vfunc(const iterator &iter, int column, Glib::ValueBase &value) const {
  ListModelWrapper::get_value_vfunc(iter, column, value);
  before_render(column, &value);
}
