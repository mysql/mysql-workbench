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
