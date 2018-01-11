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

#ifndef _MDC_BOX_H_
#define _MDC_BOX_H_

#include "mdc_layouter.h"

namespace mdc {

  class MYSQLCANVAS_PUBLIC_FUNC Box : public Layouter {
  public:
    enum Orientation { Horizontal, Vertical };

    Box(Layer *layer, Orientation orient = Horizontal, bool homogeneous = false);
    virtual ~Box();

    virtual void add(CanvasItem *item, bool expand, bool fill, bool hiddenspace = false);
    void insert_after(CanvasItem *after, CanvasItem *item, bool expand, bool fill, bool hiddenspace = false);
    void insert_before(CanvasItem *before, CanvasItem *item, bool expand, bool fill, bool hiddenspace = false);
    virtual void remove(CanvasItem *item);

    virtual void render(CairoCtx *cr);
    virtual base::Size calc_min_size();

    void set_spacing(float sp);

    virtual void foreach (const std::function<void(CanvasItem *)> &slot);

    virtual CanvasItem *get_item_at(const base::Point &pos);

    virtual void resize_to(const base::Size &size);

  protected:
    struct BoxItem {
      CanvasItem *item;
      bool expand;
      bool fill;
      bool hiddenspace; // use for spacing calculation even when hidden
    };

    typedef std::list<BoxItem> ItemList;

    Orientation _orientation;
    ItemList _children;

    float _spacing;
    bool _homogeneous;
  };

} // end of mdc namespace

#endif /* _MDC_BOX_H_ */
