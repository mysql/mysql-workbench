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

#ifndef _MDC_SELECTION_H_
#define _MDC_SELECTION_H_

#include "mdc_canvas_public.h"

#include "mdc_common.h"
#include "base/threading.h"

#include <boost/signals2.hpp>

#include <set>

namespace mdc {

  class CanvasView;
  class CanvasItem;

  class MYSQLCANVAS_PUBLIC_FUNC Selection {
  public:
    typedef std::set<CanvasItem *> ContentType;

    Selection(CanvasView *view);
    ~Selection();

    void set(CanvasItem *item);
    void add(CanvasItem *item);
    void remove(CanvasItem *item);
    void toggle(CanvasItem *item);

    void add(const std::list<CanvasItem *> &items);
    void toggle(const std::list<CanvasItem *> &items);

    void remove_items_outside(const base::Rect &rect);

    void clear(bool keep_move_info = false);

    void begin_multi_selection();
    void end_multi_selection();

    void begin_moving(const base::Point &mouse_pos);
    void update_move(const base::Point &mouse_pos);
    void end_moving();
    bool is_moving();

    ContentType get_contents() {
      return _items;
    };
    bool empty() const {
      return _items.empty();
    };

    boost::signals2::signal<void(bool, mdc::CanvasItem *)> *signal_changed() {
      return &_signal_changed;
    }
    boost::signals2::signal<void()> *signal_begin_dragging() {
      return &_signal_begin_drag;
    }
    boost::signals2::signal<void()> *signal_end_dragging() {
      return &_signal_end_drag;
    }

  private:
    struct DragData {
      base::Point offset;
      base::Point position;
      // Surface *image;

      // DragData() : image(0) {}
      // DragData(const DragData &other) : offset(other.offset), image(other.image ? new
      // Surface(other.image->get_surface()) : 0) {}
      //~DragData() { delete image; }
      DragData() {}
      DragData(const DragData &other) = default;
      DragData &operator=(const DragData &other) {
        offset = other.offset;
        position = other.position;
        // image= 0;
        // if (other.image)
        // image= new Surface(other.image->get_surface());
        return *this;
      }
    };

    ContentType _items;

    ContentType _old_state;
    ContentType _current_selection;

    boost::signals2::signal<void()> _signal_begin_drag;
    boost::signals2::signal<void()> _signal_end_drag;

    std::map<CanvasItem *, DragData> _drag_data;
    base::RecMutex _mutex;
    CanvasView *_view;

    void lock();
    void unlock();

    // void render_drag_images(CairoCtx *cr);

    boost::signals2::signal<void(bool, mdc::CanvasItem *)> _signal_changed;
    int _block_signals;
  };

} // end of mdc namespace

#endif
