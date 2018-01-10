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

#ifndef __VIEW_FIGURE_H__
#define __VIEW_FIGURE_H__

#include "figure_common.h"
#include "wbpublic_public_interface.h"

namespace wbbridge {
  namespace physical {
    class ViewFigure;
  };
};

namespace wbfig {

  class WBPUBLICBACKEND_PUBLIC_FUNC View : public BaseFigure {
    friend class wbbridge::physical::ViewFigure;

    Titlebar _title;

  public:
    View(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self);
    virtual ~View();

    virtual void set_color(const base::Color &color);
    void set_title(const std::string &title);

    virtual void set_title_font(const mdc::FontSpec &font);

    Titlebar *get_title() {
      return &_title;
    }
  };
};

#endif
