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

#ifndef _MDC_CANVAS_VIEW_H_
#define _MDC_CANVAS_VIEW_H_

#include "mdc_canvas_public.h"

#include "mdc_layer.h"
#include "mdc_events.h"
#include "mdc_canvas_item.h"
#include "mdc_selection.h"
#include "base/threading.h"

#ifndef _MSC_VER
#include <glib.h>
#endif

namespace mdc {

  class Line;

  class BackLayer;

  enum SelectType { SelectSet, SelectAdd, SelectToggle };

  class MYSQLCANVAS_PUBLIC_FUNC CanvasView {
    friend class BackLayer;
    friend class CanvasViewExtras;

  public:
    typedef std::list<Layer *> LayerList;

    virtual ~CanvasView();

    void lock_ui();
    void unlock_ui();

    void lock();
    void unlock();

    void lock_redraw();
    void unlock_redraw();

    void pre_destroy();

    inline void set_user_data(void *data) {
      _user_data = data;
    }
    void *get_user_data() {
      return _user_data;
    }

    void set_tag(const std::string &tag);
    std::string get_tag() const {
      return _tag;
    }

    mdc::CanvasItem *find_item_with_tag(const std::string &tag);

    void set_printout_mode(bool flag);
    bool is_printout() {
      return _printout_mode;
    }

    virtual void update_view_size(int width, int height) = 0;

    void set_offset(const base::Point &offs);
    virtual void scroll_to(const base::Point &offs);

    void set_zoom(float zoom);
    float get_zoom() const {
      return _zoom;
    };

    void set_page_size(const base::Size &size);
    base::Size get_page_size() const {
      return _page_size;
    };
    void set_page_layout(Count xpages, Count ypages);
    void get_page_layout(Count &xpages, Count &ypages) {
      xpages = _x_page_num;
      ypages = _y_page_num;
    }

    // logical view size
    base::Size get_total_view_size() const;

    // physical view size
    inline void get_view_size(int &w, int &h) const {
      w = _view_width;
      h = _view_height;
    }

    base::Size get_viewable_size() const;

    base::Rect get_viewport() const;

    base::Rect get_viewport_range() const;

    base::Rect get_content_bounds() const;

    virtual base::Point window_to_canvas(int x, int y) const;
    virtual base::Rect window_to_canvas(int x, int y, int w, int h) const;
    virtual void canvas_to_window(const base::Point &pt, int &x, int &y) const;
    virtual void canvas_to_window(const base::Rect &rect, int &x, int &y, int &w, int &h) const;

    void show_grid();
    void hide_grid();
    bool get_grid_shown();

    void set_grid_snapping(bool flag);
    bool get_grid_snapping();

    base::Point snap_to_grid(const base::Point &pos);
    base::Size snap_to_grid(const base::Size &size);

    void set_draws_line_hops(bool flag);

    Layer *new_layer(const std::string &name);
    void set_current_layer(Layer *layer);
    Layer *get_current_layer() const {
      return _current_layer;
    }
    Layer *get_layer(const std::string &name);
    BackLayer *get_background_layer() const {
      return _blayer;
    }
    InteractionLayer *get_interaction_layer() const {
      return _ilayer;
    }

    void add_layer(Layer *layer);
    void remove_layer(Layer *layer);
    virtual LayerList &get_layers();

    void remove_item(mdc::CanvasItem *item);

    virtual void raise_layer(Layer *layer, Layer *above = 0);
    virtual void lower_layer(Layer *layer);

    CanvasItem *get_item_at(int x, int y);
    CanvasItem *get_item_at(const base::Point &point);

    CanvasItem *get_leaf_item_at(int x, int y);
    CanvasItem *get_leaf_item_at(const base::Point &point);

    typedef std::function<bool(CanvasItem *)> ItemCheckFunc;
    std::list<CanvasItem *> get_items_bounded_by(const base::Rect &rect, const ItemCheckFunc &pred = ItemCheckFunc());

    void repaint();
    void repaint(int x, int y, int width, int height);

    void set_needs_repaint_all_items();

    void queue_repaint();
    void queue_repaint(const base::Rect &bounds);

    virtual void handle_mouse_move(int x, int y, EventState state);
    virtual void handle_mouse_button(MouseButton button, bool press, int x, int y, EventState state);
    virtual void handle_mouse_double_click(MouseButton button, int x, int y, EventState state);
    virtual void handle_mouse_enter(int x, int y, EventState state);
    virtual void handle_mouse_leave(int x, int y, EventState state);

    bool handle_key(const KeyInfo &key, bool press, EventState state);

    void start_dragging_rectangle(const base::Point &pos);
    base::Rect finish_dragging_rectangle();

    bool focus_item(CanvasItem *item);
    CanvasItem *get_focused_item();

    void select_items_inside(const base::Rect &rect, SelectType type, Group *group = 0);

    Selection *get_selection() const {
      return _selection;
    };
    Selection::ContentType get_selected_items();

    void update_line_crossings(Line *line);

    virtual bool initialize();

    const FontSpec &get_default_font();
    base::Color get_selection_color() const {
      return base::Color(0.6, 0.85, 0.95, 1.0);
    }
    base::Color get_highlight_color() const {
      return base::Color(1, 0.6, 0.0, 0.8);
    }
    base::Color get_hover_color() const {
      return base::Color(0.85, 0.5, 0.5, 0.8);
    }

    void setBackgroundColor(base::Color const& color);

    inline CairoCtx *cairoctx() const {
      return _cairo;
    }
    virtual bool has_gl() const = 0;

    virtual Surface *create_temp_surface(const base::Size &size) const;

    void export_png(const std::string &filename, bool crop = false);
    void export_pdf(const std::string &filename, const base::Size &size_in_pt);
    void export_ps(const std::string &filename, const base::Size &size_in_pt);
    void export_svg(const std::string &filename, const base::Size &size_in_pt);

    void set_event_callbacks(
      const std::function<bool(CanvasView *, MouseButton, bool, base::Point, EventState)> &button_handler,
      const std::function<bool(CanvasView *, base::Point, EventState)> &motion_handler,
      const std::function<bool(CanvasView *, KeyInfo, EventState, bool)> &key_handler);

    boost::signals2::signal<void()> *signal_resized() {
      return &_resized_signal;
    }
    boost::signals2::signal<void(int, int, int, int)> *signal_repaint() {
      return &_need_repaint_signal;
    }
    boost::signals2::signal<void()> *signal_viewport_changed() {
      return &_viewport_changed_signal;
    }
    boost::signals2::signal<void()> *signal_zoom_changed() {
      return &_zoom_changed_signal;
    }

    void enable_debug(bool flag) {
      _debug = flag;
    }
    inline bool debug_enabled() {
      return _debug;
    }

    double get_fps() {
      return _fps;
    }
    inline void bookkeep_cache_mem(int amount) {
      _total_item_cache_mem += amount;
    }

    void paint_item_cache(CairoCtx *cr, double x, double y, cairo_surface_t *cached_item, double alpha = 1.0);

  protected:
    void *_user_data;
    std::string _tag;

    cairo_surface_t *_crsurface;
    CairoCtx *_cairo;
    cairo_matrix_t _trmatrix;

    int _ui_lock;
    int _repaint_lock;
    int _repaints_missed;

    FontSpec _default_font;

    LayerList _layers;
    BackLayer *_blayer;
    InteractionLayer *_ilayer;
    Layer *_current_layer;

    CanvasItem *_focused_item;

    Selection *_selection;

    base::Size _page_size;
    Count _x_page_num;
    Count _y_page_num;

    float _zoom;
    base::Point _offset;
    base::Point _extra_offset;
    int _view_width;
    int _view_height;

    float _grid_size;
    bool _grid_snapping;
    bool _printout_mode;
    bool _line_hop_rendering;

    bool _destroying;
    bool _debug;

    double _fps;

    size_t _total_item_cache_mem;

    boost::signals2::signal<void()> _resized_signal;
    boost::signals2::signal<void(int, int, int, int)> _need_repaint_signal;
    boost::signals2::signal<void()> _viewport_changed_signal;
    boost::signals2::signal<void()> _zoom_changed_signal;

    std::function<bool(CanvasView *, MouseButton, bool, base::Point, EventState)> _button_event_relay;
    std::function<bool(CanvasView *, base::Point, EventState)> _motion_event_relay;
    std::function<bool(CanvasView *, KeyInfo, EventState, bool)> _key_event_relay;

    CanvasView(int width, int height);

    virtual void begin_repaint(int wx, int wy, int ww, int wh) = 0;
    virtual void end_repaint() = 0;

    void repaint_area(const base::Rect &rect, int wx, int wy, int ww, int wh);

    void update_offsets();
    void apply_transformations();
    void apply_transformations_gl();
    void reset_transformations_gl();
    void apply_transformations_for_conversion(cairo_matrix_t *matrix) const;

    bool perform_auto_scroll(const base::Point &mouse_pos);

    void render_for_export(const base::Rect &bounds, CairoCtx *cr);

  private:
    struct ClickInfo {
      base::Point pos;
    };

    EventState _event_state;
    CanvasItem *_last_click_item;
    CanvasItem *_last_over_item;
    std::vector<ClickInfo> _last_click_info;
    base::Point _last_mouse_pos;

    base::RecMutex _lock;

    static void *canvas_item_destroyed(void *data);
    void set_last_click_item(CanvasItem *item);
    void set_last_over_item(CanvasItem *item);
  };

} // end of mdc namespace

#endif /* _MDC_CANVAS_VIEW_H_ */
