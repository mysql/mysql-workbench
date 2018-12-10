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

#include "workbench/wb_context.h"
#include "wb_diagram_options.h"

#include "wbcanvas/model_diagram_impl.h"
#include "wbcanvas/model_figure_impl.h"
#include "base/string_utilities.h"

// if you change these values, make sure to update the Diagram Options UIs
// to allow different max values in the edit boxes (or query the max values)
#define MAX_X_PAGES 100
#define MAX_Y_PAGES 100

using namespace std;

using namespace wb;
using namespace base;

class wb::SizerFigure : public mdc::Figure {
  friend class DiagramOptionsBE;

  DiagramOptionsBE *_owner;

  Size _paper_size;
  double _width;
  double _height;

  double _mini_pw, _mini_ph;

public:
  SizerFigure(mdc::Layer *layer, DiagramOptionsBE *owner, const Size &paper_size, double width, double height)
    : mdc::Figure(layer), _owner(owner), _paper_size(paper_size), _width(width), _height(height) {
    set_cache_toplevel_contents(false);
    set_accepts_focus(false);
    set_accepts_selection(false);
    set_allowed_resizing(false, false);
  }

  ~SizerFigure() {
  }

  virtual void draw_contents(mdc::CairoCtx *cr) {
    double pw = _paper_size.width;
    double ph = _paper_size.height;
    Rect bounds(get_bounds());

    int xpages = (int)(_width / pw);
    int ypages = (int)(_height / ph);

    if (ypages < 1)
      ypages = 1;
    if (xpages < 1)
      xpages = 1;

    _mini_pw = bounds.width() / (xpages + 1);
    _mini_ph = bounds.height() / (ypages + 1);

    if (pw > ph && _mini_pw < _mini_ph)
      _mini_ph = _mini_pw * (ph / pw);
    else
      _mini_pw = _mini_ph * (pw / ph);

    if (_mini_pw > 50) {
      _mini_ph = 50 * (_mini_ph / _mini_pw);
      _mini_pw = 50;
    }

    _mini_pw = floor(_mini_pw);
    _mini_ph = floor(_mini_ph);

    cr->save();
    cr->set_line_width(1);

    cr->set_color(Color(0.5, 0.5, 0.5));
    cr->paint();

    cr->set_color(Color::white());
    cr->rectangle(0, 0, _mini_pw * xpages, _mini_ph * ypages);
    cr->fill();

    cr->set_color(Color::black());
    for (double x = 0; x < bounds.width(); x += _mini_pw) {
      cr->move_to(x + .5, 0.5);
      cr->line_to(x + .5, bounds.height() - 0.5);
      cr->stroke();
    }
    for (double y = 0; y < bounds.height(); y += _mini_ph) {
      cr->move_to(0.5, y + .5);
      cr->line_to(bounds.width() - 0.5, y + .5);
      cr->stroke();
    }

    // cr->fill();
    cr->restore();
  }

  virtual bool on_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button, mdc::EventState state) {
    if (button == mdc::ButtonLeft) {
      double xpages = ceil(point.x / _mini_pw);
      double ypages = ceil(point.y / _mini_ph);

      int xc, yc;
      _owner->get_min_size_in_pages(xc, yc);

      if (xpages < xc)
        xpages = xc;
      if (ypages < yc)
        ypages = yc;

      if (xpages > MAX_X_PAGES)
        xpages = MAX_X_PAGES;
      if (ypages > MAX_Y_PAGES)
        ypages = MAX_Y_PAGES;

      _width = _paper_size.width * xpages;
      _height = _paper_size.height * ypages;

      (*_owner->signal_changed())();

      set_needs_render();
    }
    return true;
  }
};

DiagramOptionsBE::DiagramOptionsBE(mdc::CanvasView *view, model_DiagramRef target_view, WBContext *wb)
  : _view(view), _target_view(target_view) {
  view->get_background_layer()->set_visible(false);
  view->get_background_layer()->set_grid_visible(false);
  view->set_page_layout(1, 1);
  view->set_page_size(view->get_viewable_size());

  _sizer = 0;
  if (target_view.is_valid()) {
    Size size(model_Diagram::ImplData::get_size_for_page(wb->get_document()->pageSettings()));

    _sizer = new SizerFigure(view->get_current_layer(), this, size, target_view->width(), target_view->height());

    view->get_current_layer()->add_item(_sizer);

    _name = target_view->name();
  }
  scoped_connect(view->signal_viewport_changed(), std::bind(&DiagramOptionsBE::update_size, this));
}

DiagramOptionsBE::~DiagramOptionsBE() {
  delete _sizer;
}

void DiagramOptionsBE::update_size() {
  _view->set_page_size(_view->get_viewable_size());
  if (_sizer)
    _sizer->set_fixed_size(_view->get_viewable_size());
}

int DiagramOptionsBE::get_xpages() {
  if (_sizer)
    return std::max((int)(_sizer->_width / _sizer->_paper_size.width), 1);
  return 1;
}

int DiagramOptionsBE::get_ypages() {
  if (_sizer)
    return std::max((int)(_sizer->_height / _sizer->_paper_size.height), 1);
  return 1;
}

void DiagramOptionsBE::get_max_page_counts(int &max_xpages, int &max_ypages) {
  max_xpages = MAX_X_PAGES;
  max_ypages = MAX_Y_PAGES;
}

void DiagramOptionsBE::get_min_size_in_pages(int &xc, int &yc) {
  double xmax = 0;
  double ymax = 0;

  GRTLIST_FOREACH(model_Figure, _target_view->figures(), figure) {
    Rect bounds = (*figure)->get_data()->get_canvas_item()->get_root_bounds();

    xmax = max(xmax, bounds.right());
    ymax = max(ymax, bounds.bottom());
  }

  xc = (int)ceil(xmax / _sizer->_paper_size.width);
  yc = (int)ceil(ymax / _sizer->_paper_size.height);
}

void DiagramOptionsBE::set_xpages(int c) {
  c = min(c, MAX_X_PAGES);

  int xc, yc;
  get_min_size_in_pages(xc, yc);

  if (c > 0 && c != get_xpages() && c >= xc && _sizer) {
    _sizer->_width = _sizer->_paper_size.width * c;
    _sizer->set_needs_render();
  }
}

void DiagramOptionsBE::set_ypages(int c) {
  c = min(c, MAX_Y_PAGES);

  int xc, yc;
  get_min_size_in_pages(xc, yc);

  if (c > 0 && c != get_ypages() && c >= yc && _sizer) {
    _sizer->_height = _sizer->_paper_size.height * c;
    _sizer->set_needs_render();
  }
}

std::string DiagramOptionsBE::get_name() {
  return _target_view->name();
}

void DiagramOptionsBE::set_name(const std::string &name) {
  _name = name;
}

void DiagramOptionsBE::commit() {
  grt::AutoUndo undo;

  _target_view->name(_name);
  _target_view->width(_sizer->_width);
  _target_view->height(_sizer->_height);

  undo.end(_("Set Diagram Properties"));
}
