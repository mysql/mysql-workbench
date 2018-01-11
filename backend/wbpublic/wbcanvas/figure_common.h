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

#ifndef _FIGURE_COMMON_H_
#define _FIGURE_COMMON_H_

#include "mdc.h"

#include <algorithm>

#include "wbpublic_public_interface.h"
#include <grts/structs.model.h>

namespace wbfig {

  /** Abstract class to be overridden by diagram class which will receive events from various objects in canvas.
   */
  class FigureEventHub {
  public:
    virtual ~FigureEventHub(){};
    virtual bool figure_click(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point,
                              mdc::MouseButton button, mdc::EventState state) = 0;
    virtual bool figure_double_click(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point,
                                     mdc::MouseButton button, mdc::EventState state) = 0;
    virtual bool figure_button_press(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point,
                                     mdc::MouseButton button, mdc::EventState state) = 0;
    virtual bool figure_button_release(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point,
                                       mdc::MouseButton button, mdc::EventState state) = 0;
    virtual bool figure_enter(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point) = 0;
    virtual bool figure_leave(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point) = 0;
  };

  class BaseFigure;

  class WBPUBLICBACKEND_PUBLIC_FUNC Titlebar : public mdc::Box {
    typedef mdc::Box super;

  public:
    Titlebar(mdc::Layer *layer, FigureEventHub *hub, BaseFigure *owner, bool expander);
    virtual ~Titlebar();

    void set_icon(cairo_surface_t *icon);
    void set_title(const std::string &text);

    inline const std::string &get_title() const {
      return _icon_text.get_text();
    }

    void set_color(const base::Color &color);
    void set_text_color(const base::Color &color);
    void set_font(const mdc::FontSpec &font);
    const mdc::FontSpec &get_font() {
      return _icon_text.get_font();
    }
    void set_rounded(mdc::CornerMask corners);
    void set_border_color(const base::Color &color);

    void set_expanded(bool flag);
    bool get_expanded();

    virtual void set_auto_sizing(bool flag);

    void auto_size() {
      _icon_text.auto_size();
    }

    boost::signals2::signal<void(bool)> *signal_expand_toggle() {
      return &_signal_expand_toggle;
    }

  protected:
    FigureEventHub *_hub;
    BaseFigure *_owner;

    mdc::IconTextFigure _icon_text;
    mdc::Button *_expander;

    base::Color _back_color;
    mdc::CornerMask _corners;
    base::Color _border_color;

    boost::signals2::signal<void(bool)> _signal_expand_toggle;

    void expand_toggled();

    virtual void render(mdc::CairoCtx *cr);

    virtual bool on_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                          mdc::EventState state);
    virtual bool on_double_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_button_press(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_button_release(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                   mdc::EventState state);
    virtual bool on_enter(mdc::CanvasItem *target, const base::Point &point);
    virtual bool on_leave(mdc::CanvasItem *target, const base::Point &point);
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC CaptionFigure : public mdc::TextFigure {
    typedef mdc::TextFigure super;

    FigureEventHub *_hub;
    model_Object *_owner_object;

    virtual bool on_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                          mdc::EventState state);
    virtual bool on_double_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_button_press(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_button_release(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                   mdc::EventState state);
    virtual bool on_enter(mdc::CanvasItem *target, const base::Point &point);
    virtual bool on_leave(mdc::CanvasItem *target, const base::Point &point);

  public:
    CaptionFigure(mdc::Layer *layer, FigureEventHub *hub, model_Object *owner);
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC FigureItem : public mdc::IconTextFigure {
    typedef mdc::IconTextFigure super;

    FigureEventHub *_hub;
    BaseFigure *_owner;

    std::string _object_id;
    bool _dirty;

    virtual bool on_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                          mdc::EventState state);
    virtual bool on_double_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_button_press(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_button_release(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                   mdc::EventState state);
    virtual bool on_enter(mdc::CanvasItem *target, const base::Point &point);
    virtual bool on_leave(mdc::CanvasItem *target, const base::Point &point);

    virtual void draw_state(mdc::CairoCtx *cr);
    virtual base::Point get_intersection_with_line_to(const base::Point &p);
    virtual base::Rect get_root_bounds() const;

  public:
    FigureItem(mdc::Layer *layer, FigureEventHub *hub, BaseFigure *owner);

    std::string get_id() {
      return _object_id;
    }
    void set_id(const std::string &id) {
      _object_id = id;
    }
    void set_dirty(bool flag = true) {
      _dirty = flag;
    }
    bool get_dirty() {
      return _dirty;
    }
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC BaseFigure : public mdc::Box {
    typedef mdc::Box super;

  public:
    typedef std::list<FigureItem *> ItemList;

    // default implementation just sets background color
    virtual void unset_color();
    virtual void set_color(const base::Color &color);

    virtual void set_title_font(const mdc::FontSpec &font) {
    }
    virtual void set_content_font(const mdc::FontSpec &font);

    virtual void highlight(const base::Color *color = 0);
    virtual void unhighlight();

    virtual void set_allow_manual_resizing(bool flag);

    boost::signals2::signal<void(base::Rect)> *signal_interactive_resize() {
      return &_signal_interactive_resize;
    }

    boost::signals2::signal<void(FigureItem *)> *signal_item_added() {
      return &_signal_item_added;
    }

    virtual void toggle(bool flag) {
    }
    virtual void set_state_drawing(bool flag);

    model_ObjectRef represented_object() {
      return model_ObjectRef(_represented_object);
    }

    bool in_user_resize() const {
      return _resizing;
    }

  protected:
    BaseFigure(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &object);

    FigureEventHub *_hub;
    model_Object *_represented_object;

    boost::signals2::signal<void(base::Rect)> _signal_interactive_resize;
    boost::signals2::signal<void(FigureItem *)> _signal_item_added;

    base::Rect _initial_bounds;
    mdc::FontSpec _content_font;
    bool _manual_resizing;
    bool _resizing;

    void invalidate_min_sizes();
    static void invalidate_min_sizes(mdc::CanvasItem *item);

    typedef std::function<FigureItem *(mdc::Layer *, FigureEventHub *)> CreateItemSlot;
    typedef std::function<void(FigureItem *)> UpdateItemSlot;

    virtual ItemList::iterator begin_sync(mdc::Box &box, ItemList &list);
    virtual ItemList::iterator sync_next(mdc::Box &box, ItemList &list, ItemList::iterator iter, const std::string &id,
                                         cairo_surface_t *icon, const std::string &text,
                                         const CreateItemSlot &create_item = CreateItemSlot(),
                                         const UpdateItemSlot &update_item = UpdateItemSlot());

    virtual void end_sync(mdc::Box &box, ItemList &list, ItemList::iterator iter);

    virtual bool on_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                          mdc::EventState state);
    virtual bool on_double_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_button_press(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_button_release(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                   mdc::EventState state);
    virtual bool on_enter(mdc::CanvasItem *target, const base::Point &point);
    virtual bool on_leave(mdc::CanvasItem *target, const base::Point &point);

    virtual bool on_drag_handle(mdc::ItemHandle *handle, const base::Point &pos, bool dragging);
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC ShrinkableBox : public mdc::Box {
    typedef mdc::Box super;

    int _limit_item_count;
    int _hidden_item_count;
    float _visible_part_size;
    bool _manual_resizing;

    virtual void render(mdc::CairoCtx *cr);
    virtual void resize_to(const base::Size &size);

  public:
    ShrinkableBox(mdc::Layer *layer, mdc::Box::Orientation orientation);
    virtual base::Size calc_min_size();

    void set_item_count_limit(int limit);

    void set_allow_manual_resizing(bool flag);
  };
};

#endif /* _FIGURE_COMMON_H_ */
