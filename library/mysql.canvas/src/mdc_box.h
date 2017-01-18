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
