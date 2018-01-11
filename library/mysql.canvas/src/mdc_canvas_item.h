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

#ifndef _MDC_CANVAS_ITEM_H_
#define _MDC_CANVAS_ITEM_H_

#include "mdc_common.h"
#include "mdc_canvas_public.h"

#include "mdc_events.h"
#include "base/trackable.h"

#include <boost/signals2.hpp>

namespace mdc {

  class CanvasView;
  class Layer;
  class InteractionLayer;
  class ItemHandle;
  class Magnet;
  class BoundsMagnet;

#define HDL_LEFT 1
#define HDL_RIGHT 2
#define HDL_TOP (1 << 2)
#define HDL_BOTTOM (2 << 2)

#define HDL_LR_MASK 3
#define HDL_TB_MASK (3 << 2)

#define HDL_TL (HDL_TOP | HDL_LEFT)
#define HDL_T (HDL_TOP)
#define HDL_TR (HDL_TOP | HDL_RIGHT)
#define HDL_L (HDL_LEFT)
#define HDL_R (HDL_RIGHT)
#define HDL_BL (HDL_BOTTOM | HDL_LEFT)
#define HDL_B (HDL_BOTTOM)
#define HDL_BR (HDL_BOTTOM | HDL_RIGHT)

#define LEFT_OUTER_PAD 4
#define RIGHT_OUTER_PAD 6
#define TOP_OUTER_PAD 4
#define BOTTOM_OUTER_PAD 6

  class MYSQLCANVAS_PUBLIC_FUNC CanvasItem : public base::trackable {
    friend class CanvasView;
    friend class InteractionLayer;

  public:
    enum State {
      Normal,   // normal state
      Hovering, // mouse over
      Highlighted,
      Selected, // selected
      Disabled  // disabled object
    };

    CanvasItem(Layer *layer);
    virtual ~CanvasItem();

    // geometry

    virtual base::Rect get_bounds() const;
    virtual base::Rect get_root_bounds() const;
    virtual base::Rect get_padded_root_bounds() const;
    virtual bool intersects(const base::Rect &bounds) const;
    virtual bool contains_point(const base::Point &point) const;

    virtual void resize_to(const base::Size &size);
    virtual void move_to(const base::Point &pos);
    void set_position(const base::Point &pos);
    void set_size(const base::Size &size);
    void set_bounds(const base::Rect &rect);

    base::Point get_root_position() const;
    inline base::Point get_position() const {
      return _pos;
    };

    virtual void set_fixed_min_size(const base::Size &size);
    base::Size get_min_size();
    virtual base::Size calc_min_size();
    inline base::Size get_size() const {
      return _size;
    }
    inline base::Size get_fixed_size() const {
      return _fixed_size;
    }
    virtual void set_fixed_size(const base::Size &size);
    virtual void set_auto_sizing(bool flag);
    bool auto_sizing() const {
      return _auto_sizing;
    }

    virtual void set_padding(double xpad, double ypad);

    base::Point convert_point_from(const base::Point &pt, CanvasItem *item) const;
    base::Point convert_point_to(const base::Point &pt, CanvasItem *item) const;

    virtual base::Point get_intersection_with_line_to(const base::Point &p);

    // structure

    Layer *get_layer() const {
      return _layer;
    };
    virtual void set_parent(CanvasItem *parent);
    void remove_from_parent();

    CanvasView *get_view() const;

    CanvasItem *get_parent() const {
      return _parent;
    };
    CanvasItem *get_common_ancestor(CanvasItem *item) const;

    bool is_toplevel() const;
    CanvasItem *get_toplevel() const;

    // rendering
    bool get_visible() const {
      return _visible;
    };
    void set_visible(bool flag);
    bool get_parents_visible() const;

    void set_cache_toplevel_contents(bool flag);
    void invalidate_cache();

    void set_has_shadow(bool flag);

    virtual void auto_size();
    void relayout();
    virtual void repaint(const base::Rect &clipRect, bool direct);
    virtual void render(CairoCtx *cr);
    void repaint_gl(const base::Rect &clipRect);
    virtual void render_gl(mdc::CairoCtx *cr);
    void render_to_surface(cairo_surface_t *surf, bool use_padding = true);

    virtual bool can_render_gl() {
      return false;
    }

    virtual void draw_state(CairoCtx *cr);
    virtual void draw_state_gl();
    virtual void draw_outline_ring(CairoCtx *cr, const base::Color &color);
    virtual void draw_outline_ring_gl(const base::Color &color);

    State get_state();
    virtual void stroke_outline(CairoCtx *, float offset = 0) const {
    }
    virtual void stroke_outline_gl(float offset = 0) const {
    }

    virtual void set_needs_relayout();
    void set_needs_render();
    void set_needs_repaint();

    // behaviour

    virtual void set_selected(bool flag = true);
    virtual void set_focused(bool flag = true);
    void set_accepts_selection(bool flag = true);
    bool accepts_selection() const {
      return _accepts_selection;
    };
    void set_accepts_focus(bool flag = true);
    bool accepts_focus() const {
      return _accepts_focus;
    };
    void set_draws_hover(bool flag = true);
    void set_highlighted(bool flag = true);
    void set_highlight_color(const base::Color *color); // NULL unsets the color

    void set_draggable(bool flag);
    bool is_draggable() {
      return _draggable;
    }
    void set_allowed_resizing(bool horizontal, bool vertical);

    bool is_dragging() {
      return _dragging;
    }

    bool get_focused() const {
      return _focused;
    };
    bool get_selected() const {
      return _selected;
    };

    virtual void set_state_drawing(bool flag);
    bool get_state_drawing() {
      return !_disable_state_drawing;
    }

    // private stuff
    virtual void create_handles(InteractionLayer *ilayer);
    virtual void update_handles();
    void destroy_handles();

    void magnetize_bounds();
    void add_magnet(Magnet *magnet);

    BoundsMagnet *get_bounds_magnet();
    Magnet *get_closest_magnet(const base::Point &point);

    void set_drag_handle_constrainer(const std::function<void(ItemHandle *, base::Size &)> &slot);

    // signals

    boost::signals2::signal<void(const base::Rect &)> *signal_bounds_changed() {
      return &_bounds_changed_signal;
    }
    boost::signals2::signal<void(CanvasItem *, const base::Rect &)> *signal_parent_bounds_changed() {
      return &_parent_bounds_changed_signal;
    }

    boost::signals2::signal<void()> *signal_reparent() {
      return &_reparent_signal;
    }

    boost::signals2::signal<void(bool)> *signal_focus_change() {
      return &_focus_changed_signal;
    }

    // other stuff
    void set_tag(const std::string &tag) {
      _tag = tag;
    }
    std::string get_tag() const {
      return _tag;
    }

    virtual CanvasItem *find_item_with_tag(const std::string &tag);

  private:
    base::Point _pos;
    base::Size _size;
    base::Rect _old_bounds;

    boost::signals2::scoped_connection _parent_bounds_con;
    boost::signals2::scoped_connection _grand_parent_bounds_con;

    void parent_bounds_changed(const base::Rect &obounds, CanvasItem *item);
    void grand_parent_bounds_changed(CanvasItem *item, const base::Rect &obounds);

    static void *parent_destroyed(void *data);

  protected:
    Layer *_layer;
    CanvasItem *_parent;

    cairo_surface_t *_content_cache;
    GLuint _content_texture;
    GLuint _display_list; // OpenGL's rendering list for this item.

    std::string _tag;

    base::Size _fixed_size;
    base::Size _min_size;
    base::Size _fixed_min_size;
    double _xpadding;
    double _ypadding;
    base::Point _button_press_pos;

    base::Color *_highlight_color;

    std::vector<ItemHandle *> _handles;
    std::vector<Magnet *> _magnets;

    std::function<void(ItemHandle *, base::Size &)> _drag_handle_constrainer;

    boost::signals2::signal<void(const base::Rect &)> _bounds_changed_signal;
    boost::signals2::signal<void(CanvasItem *, const base::Rect &)> _parent_bounds_changed_signal;
    boost::signals2::signal<void(bool)> _focus_changed_signal;
    boost::signals2::signal<void()> _reparent_signal;

    unsigned int _auto_sizing : 1;
    unsigned int _needs_render : 1;
    unsigned int _min_size_invalid : 1;

    unsigned int _visible : 1;

    unsigned int _disabled : 1;
    unsigned int _focused : 1;
    unsigned int _accepts_focus : 1;
    unsigned int _selected : 1;
    unsigned int _accepts_selection : 1;
    unsigned int _hovering : 1;
    unsigned int _draws_hover : 1;
    unsigned int _highlighted : 1;

    unsigned int _draggable : 1;

    unsigned int _hresizeable : 1;
    unsigned int _vresizeable : 1;

    unsigned int _cache_toplevel_content : 1;
    unsigned int _has_shadow : 1;

    unsigned int _dragging : 1;
    unsigned int _dragged : 1;
    unsigned int _disable_state_drawing : 1;

    base::Size get_texture_size(base::Size size);
    void repaint_direct();
    void repaint_cached();
    void regenerate_cache(base::Size size);

    // virtual bool can_drag_handle_to(const base::Point &pos);
    // virtual void end_drag_handle_to(const base::Point &pos);

    virtual bool on_button_press(CanvasItem *target, const base::Point &point, MouseButton button, EventState state);
    virtual bool on_button_release(CanvasItem *target, const base::Point &point, MouseButton button, EventState state);
    // drag event is received when the item is dragged with ButtonLeft pressed
    virtual bool on_drag(CanvasItem *target, const base::Point &point, EventState state);

    virtual bool on_enter(CanvasItem *target, const base::Point &point);
    virtual bool on_leave(CanvasItem *target, const base::Point &point);

    virtual bool on_click(CanvasItem *target, const base::Point &point, MouseButton button, EventState state);
    virtual bool on_double_click(CanvasItem *target, const base::Point &point, MouseButton button, EventState state);

    virtual bool on_drag_handle(ItemHandle *handle, const base::Point &pos, bool dragging);
  };

} // end of mdc namespace

#endif /* _MDC_CANVAS_ITEM_H_ */
