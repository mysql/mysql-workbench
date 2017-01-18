/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef __IMAGE_FIGURE_H__
#define __IMAGE_FIGURE_H__

#include "figure_common.h"
#include "wbpublic_public_interface.h"

namespace wbfig {

  class WBPUBLICBACKEND_PUBLIC_FUNC Image : public BaseFigure {
    typedef BaseFigure super;

    mdc::ImageFigure _image;

    bool _keep_aspect_ratio;

    virtual bool on_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                          mdc::EventState state);
    virtual bool on_double_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);

  public:
    Image(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self);

    void keep_aspect_ratio(bool flag);
    double get_aspect_ratio();

    void set_image(cairo_surface_t *image);

    bool set_image(const std::string &filename);

    cairo_surface_t *get_image();

    virtual void set_allow_manual_resizing(bool flag);
  };
};

#endif
