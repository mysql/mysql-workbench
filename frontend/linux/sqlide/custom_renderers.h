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

#ifndef _CUSTOM_RENDERERS_H_
#define _CUSTOM_RENDERERS_H_

#include <gtkmm/cellrendererspin.h>
#include <gtkmm/treeview.h>
#include <glibmm/property.h>
#include <glibmm/main.h>


#include <sstream>
#include <limits>

//==============================================================================
template <class Renderer>
class CellRendererProxy : public Renderer {
public:
  CellRendererProxy() : Glib::ObjectBase(typeid(CellRendererProxy)), Renderer() {
  }
  virtual ~CellRendererProxy() {
  }

  //  virtual void get_size_vfunc (Gtk::Widget& widget, const Gdk::Rectangle* cell_area, int* x_offset, int* y_offset,
  //  int* width, int* height) const
  //  { Renderer::get_size_vfunc (widget, cell_area, x_offset, y_offset, width, height); }

  virtual Gtk::SizeRequestMode get_request_mode_vfunc() const {
    return Renderer::get_request_mode_vfunc();
  }

  virtual void get_preferred_width_vfunc(Gtk::Widget& widget, int& minimum_width, int& natural_width) const {
    Renderer::get_preferred_width_vfunc(widget, minimum_width, natural_width);
  }

  virtual void get_preferred_height_for_width_vfunc(Gtk::Widget& widget, int width, int& minimum_height,
                                                    int& natural_height) const {
    Renderer::get_preferred_height_for_width_vfunc(widget, width, minimum_height, natural_height);
  }

  virtual void get_preferred_height_vfunc(Gtk::Widget& widget, int& minimum_height, int& natural_height) const {
    Renderer::get_preferred_height_vfunc(widget, minimum_height, natural_height);
  }

  virtual void get_preferred_width_for_height_vfunc(Gtk::Widget& widget, int height, int& minimum_width,
                                                    int& natural_width) const {
    Renderer::get_preferred_width_for_height_vfunc(widget, height, minimum_width, natural_width);
  }

  virtual void render_vfunc(const ::Cairo::RefPtr< ::Cairo::Context>& cr, Gtk::Widget& widget,
                            const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area,
                            Gtk::CellRendererState flags) {
    Renderer::render_vfunc(cr, widget, background_area, cell_area, flags);
  }

  virtual bool activate_vfunc(GdkEvent* event, Gtk::Widget& widget, const Glib::ustring& path,
                              const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area,
                              Gtk::CellRendererState flags) {
    return Renderer::activate_vfunc(event, widget, path, background_area, cell_area, flags);
  }

  virtual Gtk::CellEditable* start_editing_vfunc(GdkEvent* event, Gtk::Widget& widget, const Glib::ustring& path,
                                                 const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area,
                                                 Gtk::CellRendererState flags) {
    return Renderer::start_editing_vfunc(event, widget, path, background_area, cell_area, flags);
  }

  virtual void on_editing_canceled() {
    Renderer::on_editing_canceled();
  }
};

class CustomRendererOps {
public:
  virtual ~CustomRendererOps(){};
  virtual Gtk::CellRenderer* data_renderer() {
    return 0;
  }
};

//==============================================================================
template <typename Renderer, typename RendererValueType, typename ModelValueType>
class CustomRenderer : public Gtk::CellRenderer, public CustomRendererOps {
public:
  CustomRenderer();
  virtual ~CustomRenderer() {}

  virtual Glib::PropertyProxy_Base _property_renderable() {
    return property_data_;
  }

  enum RendererType { rt_data, rt_pixbuf };
  typedef CellRendererProxy<Renderer> RealRenderer;
  RendererType _active_renderer_type;
  RealRenderer _data_renderer;
  CellRendererProxy<Gtk::CellRendererPixbuf> _pixbuf_renderer;

  Glib::PropertyProxy<bool> property_editable() {
    return property_editable_;
  }

  Glib::SignalProxy2<void, const RendererValueType&, const RendererValueType&> signal_edited() {
    return _data_renderer.signal_edited();
  }

  Gtk::TreeViewColumn* bind_columns(GridView* treeview, const std::string& name, int index,
                                    Gtk::TreeModelColumn<ModelValueType>* model_data_column,
                                    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> >* model_pixbuf_column);

  sigc::slot<void, int> set_edit_state;
  void floating_point_visible_scale(int val);
  virtual Gtk::CellRenderer* data_renderer() {
    return &_data_renderer;
  }

protected:
  Glib::Property<Glib::RefPtr<Gdk::Pixbuf> > _property_pixbuf;
  Glib::Property<RendererValueType> _property_data;
  Glib::Property<bool> _property_editable;
  Glib::Property<bool> _property_cell_background_set;
  Glib::Property<Glib::ustring> _property_cell_background;
  Glib::Property<Gdk::Color> _property_cell_background_gdk;
  Glib::PropertyProxy<Glib::RefPtr<Gdk::Pixbuf> > property_pixbuf_;
  Glib::PropertyProxy<RendererValueType> property_data_;
  Glib::PropertyProxy<bool> property_editable_;
  Glib::PropertyProxy<RendererValueType> _data_renderer_data;
  Glib::PropertyProxy<bool> property_cell_background_set_;
  Glib::PropertyProxy<Glib::ustring> property_cell_background_;
  Glib::PropertyProxy<Gdk::Color> property_cell_background_gdk_;
  sigc::connection pulseConnection;

  //  virtual void get_size_vfunc (Gtk::Widget& widget, const Gdk::Rectangle* cell_area, int* x_offset, int* y_offset,
  //  int* width, int* height) const;

  virtual Gtk::SizeRequestMode get_request_mode_vfunc() const;
  virtual void get_preferred_width_vfunc(Gtk::Widget& widget, int& minimum_width, int& natural_width) const;
  virtual void get_preferred_height_for_width_vfunc(Gtk::Widget& widget, int width, int& minimum_height,
                                                    int& natural_height) const;
  virtual void get_preferred_height_vfunc(Gtk::Widget& widget, int& minimum_height, int& natural_height) const;
  virtual void get_preferred_width_for_height_vfunc(Gtk::Widget& widget, int height, int& minimum_width,
                                                    int& natural_width) const;
  virtual bool activate_vfunc(GdkEvent* event, Gtk::Widget& widget, const Glib::ustring& path,
                              const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area,
                              Gtk::CellRendererState flags);
  virtual void render_vfunc(const ::Cairo::RefPtr< ::Cairo::Context>& cr, Gtk::Widget& widget,
                            const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area,
                            Gtk::CellRendererState flags);
  virtual Gtk::CellEditable* start_editing_vfunc(GdkEvent* event, Gtk::Widget& widget, const Glib::ustring& path,
                                                 const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area,
                                                 Gtk::CellRendererState flags);
  virtual void on_editing_canceled();
  void on_editing_done(Gtk::CellEditable* editable);

  virtual void on_data_changed();
  virtual void on_pixbuf_changed();
  virtual void on_editable_changed();
  virtual void on_cell_background_set_changed();
  virtual void on_cell_background_changed();
  virtual void on_cell_background_gdk_changed();

private:

  void on_cell_data(Gtk::CellRenderer*, const Gtk::TreeModel::iterator&, Gtk::TreeView* tree);

  Gtk::TreeModelColumn<ModelValueType>* _model_data_column;
  Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> >* _model_pixbuf_column;
  bool _editing;
  GridView* _treeview;
  sigc::slot<void> _on_editing_done;
  Gtk::TreeModel::Path _editing_path;
  std::string _floating_point_visible_scale;
  int _column_index;
};

//==============================================================================
template <typename Renderer, typename ModelValueType>
void init_data_renderer(Renderer* renderer) {
}

//------------------------------------------------------------------------------
template <typename Renderer, typename ModelValueType>
void init_data_renderer(Gtk::CellRendererSpin* renderer) {
  renderer->property_adjustment() =
    Gtk::Adjustment::create(0, std::numeric_limits<ModelValueType>::min(), std::numeric_limits<ModelValueType>::max());
}

//==============================================================================
template <typename Renderer, typename RendererValueType, typename ModelValueType>
CustomRenderer<Renderer, RendererValueType, ModelValueType>::CustomRenderer()
  : Glib::ObjectBase(typeid(CustomRenderer)),
    Gtk::CellRenderer(),
    _active_renderer_type(rt_pixbuf),
    _data_renderer(),
    _pixbuf_renderer(),
    _property_pixbuf(*this, _pixbuf_renderer._property_renderable().get_name()),
    _property_data(*this, _data_renderer._property_renderable().get_name()),
    _property_editable(*this, _data_renderer.property_editable().get_name()),
    _property_cell_background_set(*this, _data_renderer.property_cell_background_set().get_name()),
    _property_cell_background(*this, _data_renderer.property_cell_background().get_name()),
    _property_cell_background_gdk(*this, _data_renderer.property_cell_background_gdk().get_name()),
    property_pixbuf_(this, _pixbuf_renderer._property_renderable().get_name()),
    property_data_(this, _data_renderer._property_renderable().get_name()),
    property_editable_(this, _data_renderer.property_editable().get_name()),
    _data_renderer_data(&_data_renderer, _data_renderer._property_renderable().get_name()),
    property_cell_background_set_(this, _data_renderer.property_cell_background_set().get_name()),
    property_cell_background_(this, _data_renderer.property_cell_background().get_name()),
    property_cell_background_gdk_(this, _data_renderer.property_cell_background_gdk().get_name()),
    _model_data_column(NULL),
    _model_pixbuf_column(NULL),
    _editing(false),
    _treeview(NULL),
    _floating_point_visible_scale("%.3f"),
    _column_index(-1) {
  _pixbuf_renderer.property_xalign() = _data_renderer.property_xalign().get_value();

  property_data_.signal_changed().connect(sigc::mem_fun(*this, &CustomRenderer::on_data_changed));
  property_pixbuf_.signal_changed().connect(sigc::mem_fun(*this, &CustomRenderer::on_pixbuf_changed));
  property_editable_.signal_changed().connect(sigc::mem_fun(*this, &CustomRenderer::on_editable_changed));
  property_cell_background_set_.signal_changed().connect(
    sigc::mem_fun(*this, &CustomRenderer::on_cell_background_set_changed));
  property_cell_background_.signal_changed().connect(sigc::mem_fun(*this, &CustomRenderer::on_cell_background_changed));
  property_cell_background_gdk_.signal_changed().connect(
    sigc::mem_fun(*this, &CustomRenderer::on_cell_background_gdk_changed));

  init_data_renderer<Renderer, ModelValueType>(&_data_renderer);
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::floating_point_visible_scale(int val) {
  if ((val >= 0) && (val < 15)) {
    std::ostringstream oss;
    oss << "%." << val << "f";
    _floating_point_visible_scale = oss.str();
  }
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::on_data_changed() {
  _data_renderer_data = _property_data.get_value();
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::on_pixbuf_changed() {
  _active_renderer_type= (_pixbuf_renderer.property_pixbuf().get_value() ? rt_pixbuf : rt_data);
  _pixbuf_renderer.property_pixbuf()= _property_pixbuf;
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::on_editable_changed() {
  _data_renderer.property_editable() = _property_editable;
  property_mode() = _data_renderer.property_mode().get_value();
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::on_cell_background_set_changed() {
  _data_renderer.property_cell_background_set()= _property_cell_background_set;
  _pixbuf_renderer.property_cell_background_set()= _property_cell_background_set;
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::on_cell_background_changed() {
  _data_renderer.property_cell_background()= _property_cell_background;
  _pixbuf_renderer.property_cell_background()= _property_cell_background;
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::on_cell_background_gdk_changed() {
  _data_renderer.property_cell_background_gdk()= _property_cell_background_gdk;
  _pixbuf_renderer.property_cell_background_gdk()= _property_cell_background_gdk;
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
Gtk::TreeViewColumn* CustomRenderer<Renderer, RendererValueType, ModelValueType>::bind_columns(
  GridView* treeview, const std::string& name, int index, Gtk::TreeModelColumn<ModelValueType>* model_data_column,
  Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> >* model_pixbuf_column) {
  _treeview = treeview;
  _column_index = index;

  int column_count = treeview->insert_column_with_data_func(
    -1, name, *this, sigc::bind(sigc::mem_fun(this, &CustomRenderer::on_cell_data), treeview));
  Gtk::TreeViewColumn* treeview_column = treeview->get_column(column_count - 1);
  treeview_column->set_resizable(true);

  _model_data_column = model_data_column;
  treeview_column->set_renderer(*this, *model_data_column);
  _model_pixbuf_column= model_pixbuf_column;

  return treeview_column;
}

//------------------------------------------------------------------------------
template <typename RendererValueType, typename ModelValueType>
void load_cell_data(Glib::Property<RendererValueType>& prop, const ModelValueType& val, bool for_edit,
                    const std::string& floating_point_visible_scale) {
  std::ostringstream oss;
  oss << val;
  prop = oss.str();
}

//------------------------------------------------------------------------------
template <typename RendererValueType>
void load_cell_data(Glib::Property<RendererValueType>& prop, const double& val, bool for_edit,
                    const std::string& floating_point_visible_scale) {
  std::string s;

  if (for_edit) {
    std::ostringstream oss;
    oss.precision(15);
    oss << val;
    s = oss.str();

    // remove trailing zeros
    std::string::size_type point_pos = s.find_first_of(".,");
    if (std::string::npos != point_pos) {
      std::string::reverse_iterator i = s.rbegin();
      for (std::string::reverse_iterator end(s.rend()); i != end; ++i)
        if (*i != '0')
          break;
      s.erase(i.base(), s.end());
    }
  } else {
    char cstr[32];
    sprintf(cstr, floating_point_visible_scale.c_str(), val);
    s = cstr;
  }

  prop = s;
}

//------------------------------------------------------------------------------
template <typename RendererValueType>
void load_cell_data(Glib::Property<RendererValueType>& prop, const RendererValueType& val, bool for_edit,
                    const std::string& floating_point_visible_scale) {
  prop = val;
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::on_cell_data(Gtk::CellRenderer* cr,
                                                                               const Gtk::TreeModel::iterator& iter,
                                                                               Gtk::TreeView* tree) {
  Gtk::TreeIter editing_iter;
  if (!_editing_path.empty())
    editing_iter = tree->get_model()->get_iter(_editing_path);

  load_cell_data(_property_data, iter->get_value(*_model_data_column), (_editing && editing_iter == iter),
                 _floating_point_visible_scale);

  if (_model_pixbuf_column)
    _property_pixbuf = iter->get_value(*_model_pixbuf_column);
}

//------------------------------------------------------------------------------
// template <typename Renderer, typename RendererValueType, typename ModelValueType>
// void CustomRenderer<Renderer, RendererValueType, ModelValueType>::
// get_size_vfunc(Gtk::Widget& widget, const Gdk::Rectangle* cell_area, int* x_offset, int* y_offset, int* width, int*
// height) const
//{
//  _data_renderer.get_size_vfunc(widget, cell_area, x_offset, y_offset, width, height);
//}

template <typename Renderer, typename RendererValueType, typename ModelValueType>
Gtk::SizeRequestMode CustomRenderer<Renderer, RendererValueType, ModelValueType>::get_request_mode_vfunc() const {
  return _data_renderer.get_request_mode_vfunc();
}

template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::get_preferred_width_vfunc(
  Gtk::Widget& widget, int& minimum_width,
  int& natural_width) const // get_preferred_width_vfunc(Gtk::Widget& widget, int& minimum_width, int& natural_width)
{
  _data_renderer.get_preferred_width_vfunc(widget, minimum_width, natural_width);
}

template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::get_preferred_height_for_width_vfunc(
  Gtk::Widget& widget, int width, int& minimum_height, int& natural_height) const {
  _data_renderer.get_preferred_height_for_width_vfunc(widget, width, minimum_height, natural_height);
}

template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::get_preferred_height_vfunc(
  Gtk::Widget& widget, int& minimum_height, int& natural_height) const {
  _data_renderer.get_preferred_height_vfunc(widget, minimum_height, natural_height);
}

template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::get_preferred_width_for_height_vfunc(
  Gtk::Widget& widget, int height, int& minimum_width, int& natural_width) const {
  _data_renderer.get_preferred_width_for_height_vfunc(widget, height, minimum_width, natural_width);
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::render_vfunc(
  const ::Cairo::RefPtr< ::Cairo::Context>& cr, Gtk::Widget& widget, const Gdk::Rectangle& background_area,
  const Gdk::Rectangle& cell_area, Gtk::CellRendererState flags) {
  int row = -1;
  Gtk::TreePath path;
  if (_treeview->get_path_at_pos(cell_area.get_x() + 1, cell_area.get_y() + 1, path)) {
    row = path[0];
    if (row >= 0 && _column_index >= 0) {
      int srow, scol;
      _treeview->current_cell(srow, scol);
      if (_treeview->selection_is_cell() && srow >= 0 && scol >= 0 && srow == row && scol == _column_index) {
        _treeview->get_style_context()->add_class("entry");
        _treeview->get_style_context()->render_frame(cr, background_area.get_x(), background_area.get_y(),
                                                     background_area.get_width(), background_area.get_height());
        _treeview->get_style_context()->render_background(cr, background_area.get_x(), background_area.get_y(),
                                                          background_area.get_width(), background_area.get_height());

        flags |= Gtk::CELL_RENDERER_SELECTED;
      }
    }
  }
  pulseConnection.disconnect();
  switch (_active_renderer_type) {
    case rt_data:
      _data_renderer.render_vfunc(cr, widget, background_area, cell_area, flags);
      break;
    default:
      _pixbuf_renderer.render_vfunc(cr, widget, background_area, cell_area, flags);
      break;
  }
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
bool CustomRenderer<Renderer, RendererValueType, ModelValueType>::activate_vfunc(GdkEvent* event, Gtk::Widget& widget,
                                                                                 const Glib::ustring& path,
                                                                                 const Gdk::Rectangle& background_area,
                                                                                 const Gdk::Rectangle& cell_area,
                                                                                 Gtk::CellRendererState flags) {
  return _data_renderer.activate_vfunc(event, widget, path, background_area, cell_area, flags);
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
Gtk::CellEditable* CustomRenderer<Renderer, RendererValueType, ModelValueType>::start_editing_vfunc(
  GdkEvent* event, Gtk::Widget& widget, const Glib::ustring& path, const Gdk::Rectangle& background_area,
  const Gdk::Rectangle& cell_area, Gtk::CellRendererState flags) {
  Gtk::TreeIter editing_iter = _treeview->get_model()->get_iter(path);
  _editing_path = editing_iter;
  _editing = true;
  Gtk::TreePath tree_path(path);
  int row_index = tree_path[0];
  set_edit_state(row_index);
  load_cell_data(_property_data, editing_iter->get_value(*_model_data_column), true, _floating_point_visible_scale);

  Gtk::CellEditable* editable =
    _data_renderer.start_editing_vfunc(event, widget, path, background_area, cell_area, flags);
  if (editable) {
    _on_editing_done = sigc::bind(sigc::mem_fun(this, &CustomRenderer::on_editing_done), editable);
    editable->signal_editing_done().connect(_on_editing_done);
  }
  return editable;
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::on_editing_canceled() {
  _editing = false;
  Gtk::TreePath path;
  Gtk::TreeViewColumn* col;
  _treeview->get_cursor(path, col);
  if (!path.empty())
    set_edit_state(path[0]);
  _data_renderer.on_editing_canceled();
}

//------------------------------------------------------------------------------
template <typename Renderer, typename RendererValueType, typename ModelValueType>
void CustomRenderer<Renderer, RendererValueType, ModelValueType>::on_editing_done(Gtk::CellEditable* editable) {
  _editing = false;
  Gtk::TreePath path;
  Gtk::TreeViewColumn* col;
  _treeview->get_cursor(path, col);
  if (!path.empty())
    set_edit_state(path[0]);
  _on_editing_done.disconnect();
}

#endif // _CUSTOM_RENDERERS_H_
