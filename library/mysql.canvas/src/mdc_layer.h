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

#ifndef _MDC_LAYER_H_
#define _MDC_LAYER_H_

#include "mdc_common.h"
#include "mdc_group.h"
#include "mdc_events.h"
#include "base/trackable.h"

namespace mdc {

  class CanvasView;
  class CanvasItem;
  class AreaGroup;

  class MYSQLCANVAS_PUBLIC_FUNC Layer : public base::trackable {
  public:
    Layer(CanvasView *view);
    virtual ~Layer();

    void set_root_area(AreaGroup *group);

    void set_name(const std::string &name);
    std::string get_name() const {
      return _name;
    }

    virtual void add_item(CanvasItem *item, AreaGroup *location = 0);
    virtual void remove_item(CanvasItem *item);

    virtual void set_visible(bool flag);
    bool visible() const {
      return _visible;
    };

    virtual void repaint_pending();
    virtual void repaint(const base::Rect &aBounds);
    void repaint_for_export(const base::Rect &aBounds);

    inline CanvasView *get_view() const {
      return _owner;
    };

    void queue_relayout(CanvasItem *item);
    void invalidate_caches();

    void set_needs_repaint_all_items();

    void queue_repaint();
    void queue_repaint(const base::Rect &bounds);

    CanvasItem *get_other_item_at(const base::Point &point, CanvasItem *item);

    CanvasItem *get_item_at(const base::Point &point);
    CanvasItem *get_top_item_at(const base::Point &point);

    AreaGroup *get_root_area_group() const {
      return _root_area;
    }

    typedef std::function<bool(CanvasItem *)> ItemCheckFunc;

    std::list<CanvasItem *> get_items_bounded_by(const base::Rect &rect, const ItemCheckFunc &pred = ItemCheckFunc(),
                                                 mdc::Group *inside_group = 0);

    Group *create_group_with(const std::list<CanvasItem *> &contents);
    void dissolve_group(Group *group);

    AreaGroup *create_area_group_with(const std::list<CanvasItem *> &contents);

    base::Rect get_bounds_of_item_list(const std::list<CanvasItem *> &items);

  protected:
    CanvasView *_owner;
    AreaGroup *_root_area;

    std::string _name;

    std::list<CanvasItem *> _relayout_queue;

    bool _visible;

    bool _needs_repaint;

    Layer *get_layer_under_this();

  private:
    void view_resized();
  };

} // end of mdc namespace

#endif /* _MDC_LAYER_H_ */
