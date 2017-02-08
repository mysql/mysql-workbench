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
#ifndef __NOTE_FIGURE_H__
#define __NOTE_FIGURE_H__

#include "figure_common.h"
#include "wbpublic_public_interface.h"

namespace wbfig {

  class WBPUBLICBACKEND_PUBLIC_FUNC Note : public BaseFigure {
    typedef BaseFigure super;
    mdc::TextFigure _text;

    virtual bool on_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                          mdc::EventState state);
    virtual bool on_double_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);

  public:
    Note(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self);
    virtual ~Note();

    void set_text_color(const base::Color &color);
    void set_text(const std::string &text);

    void set_font(const std::string &text);

    virtual void set_content_font(const mdc::FontSpec &font);

    virtual void set_allow_manual_resizing(bool flag);
  };
};

#endif
