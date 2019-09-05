/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "workbench_model_imagefigure_impl.h"

#include "model_layer_impl.h"
#include "model_model_impl.h"
#include "model_diagram_impl.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN(DOMAIN_CANVAS_BE)

using namespace base;

workbench_model_ImageFigure::ImplData::ImplData(workbench_model_ImageFigure *self)
  : super(self), _figure(0), _thumbnail(0) {
  _resizable = true;
  _last_click = 0;
}

std::string workbench_model_ImageFigure::ImplData::set_filename(const std::string &fn) {
  if (fn != *self()->_filename) {
    std::string internal_name;
    if (fn != "") {
      if (fn[0] == '@')
        internal_name = fn;
      else {
        internal_name = self()->owner()->owner()->get_data()->get_delegate()->attach_image(fn);
        if (internal_name.empty()) {
          g_warning("Image '%s' could not be attached to document.", fn.c_str());
          return "";
        }

        self()->owner()->owner()->get_data()->get_delegate()->release_image(self()->_filename);
      }
    }

    if (_figure) {
      cairo_surface_t *img = self()->owner()->owner()->get_data()->get_delegate()->fetch_image(internal_name);
      if (img) {
        _figure->set_image(img);
        cairo_surface_destroy(img);

        shrink_if_needed();

        self()->_width = _figure->get_size().width;
        self()->_height = _figure->get_size().height;
      } else {
        logWarning("Could not load image '%s' for '%s'", fn.c_str(), self()->name().c_str());
      }
    }

    self()->_filename = internal_name;

    return internal_name;
  } else
    return self()->_filename;
}

void workbench_model_ImageFigure::ImplData::set_keep_aspect_ratio(bool flag) {
  self()->_keepAspectRatio = flag ? 1 : 0;

  if (_figure)
    dynamic_cast<wbfig::Image *>(_figure)->keep_aspect_ratio(*self()->_keepAspectRatio != 0);
}

/*  else if (_figure && name == "width" && *__keepAspectRatio!=0)
  {
    double ratio= dynamic_cast<wbfig::Image*>(_figure)->get_aspect_ratio();

    __width= grt::DoubleRef::cast_from(value);
    __height= *__width / ratio;
    if (__height < 1)
      __height= 1;

    _figure->set_fixed_size(Size(__width, __height));
  }
  else if (_figure && name == "height" && *__keepAspectRatio!=0)
  {
    double ratio= dynamic_cast<wbfig::Image*>(_figure)->get_aspect_ratio();

    __height= grt::DoubleRef::cast_from(value);
    __width= *__height * ratio;
    if (__width < 1)
      __width= 1;

    _figure->set_fixed_size(Size(__width, __height));
  }*/

void workbench_model_ImageFigure::ImplData::unrealize() {
  notify_will_unrealize();

  super::unrealize();

  delete _figure;
  _figure = 0;

  if (_thumbnail)
    cairo_surface_destroy(_thumbnail);
  _thumbnail = 0;
}

bool workbench_model_ImageFigure::ImplData::shrink_if_needed() {
  Size size(_figure->calc_min_size());
  Size max_size(get_canvas_view()->get_total_view_size());
  bool resized = false;

  max_size.width -= 20;
  max_size.height -= 20;

  // shrink image if too big
  if (size.width > max_size.width) {
    size.width = max_size.width;
    resized = true;
  }
  if (size.height > max_size.height) {
    size.height = max_size.height;
    resized = true;
  }
  if (resized) {
    self()->_manualSizing = 1;
    _figure->set_fixed_size(size);
  } else
    _figure->resize_to(size);

  return resized;
}

bool workbench_model_ImageFigure::ImplData::realize() {
  if (_figure)
    return true;
  if (!is_realizable())
    return false;

  if (!is_main_thread()) {
    run_later(std::bind(&ImplData::realize, this));
    return true;
  }

  if (!_figure) {
    mdc::CanvasView *view = self()->owner()->get_data()->get_canvas_view();
    mdc::AreaGroup *agroup;

    view->lock();

    wbfig::Image *image = _figure = new wbfig::Image(view->get_current_layer(), self()->owner()->get_data(), self());
    image->keep_aspect_ratio(*self()->_keepAspectRatio != 0);

    agroup = self()->layer()->get_data()->get_area_group();

    view->get_current_layer()->add_item(_figure, agroup);

    std::string path;
    if (self()->_filename.is_valid())
      path = *self()->_filename;
    if (!path.empty()) {
      cairo_surface_t *img = self()->owner()->owner()->get_data()->get_delegate()->fetch_image(path);
        logWarning("Could not load image '%s' for '%s'\n", path.c_str(), self()->name().c_str());
      if (!img) {
        _figure->set_image(img);
        cairo_surface_destroy(img);
      }
    }

    if (shrink_if_needed()) {
      self()->_width = _figure->get_size().width;
      self()->_height = _figure->get_size().height;
    } else if (*self()->_width == 0.0 || *self()->_height == 0.0) {
      self()->_width = _figure->get_size().width;
      self()->_height = _figure->get_size().height;
    }

    finish_realize();

    view->unlock();

    self()->owner()->get_data()->notify_object_realize(self());
  }
  return true;
}

void workbench_model_ImageFigure::ImplData::render_mini(mdc::CairoCtx *cr) {
  if (!_thumbnail && _figure && _figure->get_image()) {
    Size image_size = _figure->get_size();

    // Create a thumbnail and remember the scale used.
    if (image_size.width > 256) {
      int w, h;

      w = 256;
      h = (int)((w * image_size.height) / image_size.width);
      if (h < 1)
        h = 1;

      _thumbnail = cairo_surface_create_similar(_figure->get_image(), CAIRO_CONTENT_COLOR_ALPHA, w, h);
      mdc::CairoCtx cr(_thumbnail);

      cr.set_color(Color(1.0, 1.0, 1.0), 0.0);
      cr.paint();
      cr.scale(image_size.width / w, image_size.height / h);
      cr.set_source_surface(_figure->get_image(), 0.0, 0.0);
      cairo_pattern_set_filter(cairo_get_source(cr.get_cr()), CAIRO_FILTER_BEST);
      cr.paint();
    } else {
      _thumbnail = cairo_surface_reference(_figure->get_image());
    }
  }

  if (_thumbnail) {
    cr->save();
    cr->rectangle(*self()->_left, *self()->_top, *self()->_width, *self()->_height);
    cr->clip();
    cr->translate(*self()->_left, *self()->_top);
    cr->scale(self()->_width / cairo_image_surface_get_width(_thumbnail),
              self()->_height / cairo_image_surface_get_height(_thumbnail));
    cr->set_source_surface(_thumbnail, 0.0, 0.0);
    cairo_pattern_set_filter(cairo_get_source(cr->get_cr()), CAIRO_FILTER_BEST);
    cr->paint();
    cr->restore();
  } else {
    model_Figure::ImplData::render_mini(cr);
  }
}
