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

#include "figure_common.h"
#include "base/string_utilities.h"

using namespace wbfig;
using namespace base;

Titlebar::Titlebar(mdc::Layer *layer, FigureEventHub *hub, BaseFigure *owner, bool expander)
  : mdc::Box(layer, mdc::Box::Horizontal), _hub(hub), _owner(owner), _icon_text(layer) {
  set_padding(6, 4);
  set_spacing(6);

  _corners = mdc::CNone;
  _back_color = Color(1, 1, 1);
  _border_color = Color(0.7, 0.7, 0.7);

  _icon_text.set_allow_shrinking(true);
  //_icon_text.set_auto_sizing(false);

  add(&_icon_text, true, true);

  if (expander) {
    _expander = new mdc::Button(layer, mdc::ExpanderButton);
    scoped_connect(_expander->signal_activate(), std::bind(&Titlebar::expand_toggled, this));
    _expander->set_pen_color(Color(0.4, 0.4, 0.4));

    add(_expander, false, false);
  } else
    _expander = 0;
}

Titlebar::~Titlebar() {
  delete _expander;
}

bool Titlebar::on_button_press(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                               mdc::EventState state) {
  if (!_hub || !_hub->figure_button_press(_owner->represented_object(), target, point, button, state))
    return super::on_button_press(target, point, button, state);
  return false;
}

bool Titlebar::on_button_release(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                 mdc::EventState state) {
  if (!_hub || !_hub->figure_button_release(_owner->represented_object(), target, point, button, state))
    return super::on_button_release(target, point, button, state);
  return false;
}

bool Titlebar::on_enter(mdc::CanvasItem *target, const Point &point) {
  if (!_hub || !_hub->figure_enter(_owner->represented_object(), target, point))
    return super::on_enter(target, point);
  return false;
}

bool Titlebar::on_leave(mdc::CanvasItem *target, const Point &point) {
  if (!_hub || !_hub->figure_leave(_owner->represented_object(), target, point))
    return super::on_leave(target, point);
  return false;
}

//--------------------------------------------------------------------------------------------------

bool Titlebar::on_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button, mdc::EventState state) {
  if (!_hub || !_hub->figure_click(_owner->represented_object(), target, point, button, state))
    return super::on_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------

bool Titlebar::on_double_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                               mdc::EventState state) {
  if (!_hub || !_hub->figure_double_click(_owner->represented_object(), target, point, button, state))
    return super::on_double_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------

void Titlebar::set_icon(cairo_surface_t *icon) {
  _icon_text.set_icon(icon);
}

void Titlebar::set_title(const std::string &text) {
  _icon_text.set_text(text);
}

void Titlebar::set_expanded(bool flag) {
  if (_expander && _expander->get_active() != flag)
    _expander->set_active(flag);
}

bool Titlebar::get_expanded() {
  return _expander->get_active();
}

void Titlebar::expand_toggled() {
  _signal_expand_toggle(_expander->get_active());
}

void Titlebar::set_color(const Color &color) {
  _back_color = color;
  set_needs_render();
}

void Titlebar::set_text_color(const Color &color) {
  _icon_text.set_pen_color(color);
}

void Titlebar::set_border_color(const Color &color) {
  _border_color = color;
}

void Titlebar::set_font(const mdc::FontSpec &font) {
  _icon_text.set_font(font);
}

void Titlebar::set_rounded(mdc::CornerMask corners) {
  _corners = corners;
}

void Titlebar::render(mdc::CairoCtx *cr) {
  cr->save();

  mdc::stroke_rounded_rectangle(cr, get_bounds(), _corners, 8);

  cr->set_color(_back_color);
  cr->set_line_width(1);

  cr->fill_preserve();
  cr->set_color(_border_color);
  cr->stroke();
  cr->restore();

  mdc::Box::render(cr);
}

void Titlebar::set_auto_sizing(bool flag) {
  mdc::Box::set_auto_sizing(flag);
  _icon_text.set_auto_sizing(flag);
}

//-------------------------------------------------------------------------------------------------------------

FigureItem::FigureItem(mdc::Layer *layer, FigureEventHub *hub, BaseFigure *owner)
  : mdc::IconTextFigure(layer), _hub(hub), _owner(owner), _dirty(true) {
  set_font(get_view()->get_default_font());
}

bool FigureItem::on_button_press(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                 mdc::EventState state) {
  if (target != this || !_hub->figure_button_press(_owner->represented_object(), target, point, button, state))
    return super::on_button_press(target, point, button, state);
  return false;
}

bool FigureItem::on_button_release(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                   mdc::EventState state) {
  if (target != this || !_hub->figure_button_release(_owner->represented_object(), target, point, button, state))
    return super::on_button_release(target, point, button, state);
  return false;
}

bool FigureItem::on_enter(mdc::CanvasItem *target, const Point &point) {
  if (target != this || !_hub->figure_enter(_owner->represented_object(), target, point))
    return super::on_enter(target, point);
  return false;
}

bool FigureItem::on_leave(mdc::CanvasItem *target, const Point &point) {
  if (target != this || !_hub->figure_leave(_owner->represented_object(), target, point))
    return super::on_leave(target, point);
  return false;
}

//--------------------------------------------------------------------------------------------------

bool FigureItem::on_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button, mdc::EventState state) {
  if (target != this || !_hub->figure_click(_owner->represented_object(), target, point, button, state))
    return super::on_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------

bool FigureItem::on_double_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                 mdc::EventState state) {
  if (target != this || !_hub->figure_double_click(_owner->represented_object(), target, point, button, state))
    return super::on_double_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------

Point FigureItem::get_intersection_with_line_to(const Point &p) {
  Point point(mdc::CanvasItem::get_intersection_with_line_to(p));
  Rect bounds(get_root_bounds());

  if (point.x - bounds.left() < bounds.width() / 2)
    point.x = bounds.left();
  else
    point.x = bounds.right();

  point.y = (bounds.bottom() + bounds.top()) / 2;

  return point;
}

Rect FigureItem::get_root_bounds() const {
  if (!get_visible() || !get_parents_visible())
    return get_toplevel()->get_root_bounds();
  return super::get_root_bounds();
}

void FigureItem::draw_state(mdc::CairoCtx *cr) {
  switch (get_state()) {
    case Hovering:
      cr->save();
      cr->set_color(get_view()->get_hover_color(), 0.5);
      stroke_outline(cr, 0);
      cr->fill();
      cr->restore();
      break;
    case Highlighted:
      cr->save();
      cr->set_color(_highlight_color ? *_highlight_color : get_view()->get_highlight_color(), 0.3);
      stroke_outline(cr, 0);
      cr->fill();
      cr->restore();
      break;
    default:
      mdc::IconTextFigure::draw_state(cr);
  }
}

//-------------------------------------------------------------------------------------------------------------

BaseFigure::BaseFigure(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &object)
  : mdc::Box(layer, mdc::Box::Vertical), _hub(hub) {
  _represented_object = &object.content();

#ifndef __APPLE__ // disabling caching allows for hidpi correctness
  set_cache_toplevel_contents(true);
#endif
  set_draggable(true);
  _manual_resizing = false;
  _resizing = false;
}

bool BaseFigure::on_button_press(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                 mdc::EventState state) {
  if (target != this || !_hub->figure_button_press(represented_object(), target, point, button, state))
    return super::on_button_press(target, point, button, state);
  return false;
}

bool BaseFigure::on_button_release(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                   mdc::EventState state) {
  if (target != this || !_hub->figure_button_release(represented_object(), target, point, button, state))
    return super::on_button_release(target, point, button, state);
  return false;
}

bool BaseFigure::on_enter(mdc::CanvasItem *target, const Point &point) {
  if (!_hub->figure_enter(represented_object(), target, point))
    return super::on_enter(target, point);
  return false;
}

bool BaseFigure::on_leave(mdc::CanvasItem *target, const Point &point) {
  if (!_hub->figure_leave(represented_object(), target, point))
    return super::on_leave(target, point);
  return false;
}

bool BaseFigure::on_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button, mdc::EventState state) {
  if (target != this || !_hub->figure_click(represented_object(), target, point, button, state))
    return super::on_click(target, point, button, state);
  return false;
}

bool BaseFigure::on_double_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                 mdc::EventState state) {
  if (target != this || !_hub->figure_double_click(represented_object(), target, point, button, state))
    return super::on_double_click(target, point, button, state);
  return false;
}

void BaseFigure::unset_color() {
  set_draw_background(false);
}

void BaseFigure::set_color(const Color &color) {
  set_background_color(color);
  set_draw_background(true);
}

void BaseFigure::highlight(const Color *color) {
  set_highlight_color(color);
  set_highlighted(true);
}

void BaseFigure::unhighlight() {
  set_highlight_color(0);
  set_highlighted(false);
}

void BaseFigure::set_allow_manual_resizing(bool flag) {
  _manual_resizing = flag;
  invalidate_min_sizes();
  if (!flag)
    set_fixed_size(Size(-1, -1));
}

void BaseFigure::invalidate_min_sizes(mdc::CanvasItem *item) {
  item->set_needs_relayout();
  if (dynamic_cast<mdc::Layouter *>(item)) {
    dynamic_cast<mdc::Layouter *>(item)->foreach ([](mdc::CanvasItem * item) {
      invalidate_min_sizes(item); 
    });
  }
}

void BaseFigure::invalidate_min_sizes() {
  invalidate_min_sizes(this);
}

bool BaseFigure::on_drag_handle(mdc::ItemHandle *handle, const Point &pos, bool dragging) {
  if (!_manual_resizing)
    set_allow_manual_resizing(true); // TODO: what about undo record?

  if (dragging) {
    if (!_resizing)
      _initial_bounds = get_bounds();
    _resizing = true;
  }

  bool flag = super::on_drag_handle(handle, pos, dragging);

  if (!dragging) {
    _resizing = false;
    _signal_interactive_resize(_initial_bounds);
  }
  if (flag)
    set_fixed_size(get_size());

  return flag;
}

void BaseFigure::set_content_font(const mdc::FontSpec &font) {
  _content_font = font;
  if (font.family == "Arial")
    _content_font = font;
}

void BaseFigure::set_state_drawing(bool flag) {
  _disable_state_drawing = !flag;
}

static BaseFigure::ItemList::iterator find_item(BaseFigure::ItemList &list, const std::string &id) {
  for (BaseFigure::ItemList::iterator i = list.begin(); i != list.end(); ++i) {
    if ((*i)->get_id() == id)
      return i;
  }
  return list.end();
}

BaseFigure::ItemList::iterator BaseFigure::begin_sync(mdc::Box &box, ItemList &list) {
  get_layer()->get_view()->lock();
  get_layer()->get_view()->lock_redraw();
  return list.begin();
}

BaseFigure::ItemList::iterator BaseFigure::sync_next(mdc::Box &box, ItemList &list, ItemList::iterator iter,
                                                     const std::string &id, cairo_surface_t *icon,
                                                     const std::string &text, const CreateItemSlot &create_item,
                                                     const UpdateItemSlot &update_item)

{
  ItemList::iterator i = find_item(list, id);

  if (i == list.end()) // item is new, insert it
  {
    FigureItem *item;

    if (create_item)
      item = create_item(get_layer(), _hub);
    else
      item = new FigureItem(get_layer(), _hub, this);

    if (update_item)
      update_item(item);

    if (_manual_resizing)
      item->set_auto_sizing(false);
    item->set_allow_shrinking(true);
    item->set_spacing(2);
    item->set_padding(4, 4);

    item->set_font(_content_font);

    item->set_icon(icon);
    item->set_text(text);
    item->set_id(id);

    list.insert(iter, item); // insert before

    _signal_item_added(item);

    return iter;        // return same
  } else if (i == iter) // item is in the same position, just update
  {
    if ((*i)->get_icon() != icon || (*i)->get_text() != text) {
      (*i)->set_icon(icon);
      (*i)->set_text(text);
      (*i)->set_dirty();
    }

    if (update_item)
      update_item(*i);

    ++iter; // go to next
    return iter;
  } else // item has moved, update and move up
  {
    FigureItem *item = *i;

    (*i)->set_icon(icon);
    (*i)->set_text(text);
    (*i)->set_dirty();
    if (update_item)
      update_item(*i);

    list.erase(i);
    list.insert(iter, item);

    return iter;
  }
}

void BaseFigure::end_sync(mdc::Box &box, ItemList &list, ItemList::iterator iter) {
  // everything after iter is outdated, so just delete everything
  while (iter != list.end()) {
    ItemList::iterator next = iter;

    ++next;

    delete *iter;
    list.erase(iter);

    iter = next;
  }

  bool dirty = false;

  // first check if anything changed
  for (ItemList::const_iterator i = list.begin(); i != list.end(); ++i) {
    if ((*i)->get_dirty())
      dirty = true;
    (*i)->set_dirty(false);
  }

  // resync box
  if (dirty) {
    box.remove_all();
    for (ItemList::const_iterator i = list.begin(); i != list.end(); ++i) {
      box.add(*i, false, true, true);
    }
    box.set_needs_relayout();

    if (_manual_resizing) {
      Size size = get_min_size();
      Size fsize = get_fixed_size();

      if (size.height > fsize.height)
        set_fixed_size(Size(fsize.width, size.height));
    }
  }
  get_layer()->get_view()->unlock_redraw();
  get_layer()->get_view()->unlock();
}

//---------------------------------------------------------------------------------------------------

CaptionFigure::CaptionFigure(mdc::Layer *layer, FigureEventHub *hub, model_Object *owner)
  : mdc::TextFigure(layer), _hub(hub), _owner_object(owner) {
  set_draw_outline(true);
}

bool CaptionFigure::on_button_press(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                    mdc::EventState state) {
  if (!_hub->figure_button_press(_owner_object, target, point, button, state))
    return super::on_button_press(target, point, button, state);
  return false;
}

bool CaptionFigure::on_button_release(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                      mdc::EventState state) {
  if (!_hub->figure_button_release(_owner_object, target, point, button, state))
    return super::on_button_release(target, point, button, state);
  return false;
}

bool CaptionFigure::on_enter(mdc::CanvasItem *target, const Point &point) {
  if (!_hub->figure_enter(_owner_object, target, point))
    return super::on_enter(target, point);
  return false;
}

bool CaptionFigure::on_leave(mdc::CanvasItem *target, const Point &point) {
  if (!_hub->figure_leave(_owner_object, target, point))
    return super::on_leave(target, point);
  return false;
}

//--------------------------------------------------------------------------------------------------

bool CaptionFigure::on_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                             mdc::EventState state) {
  if (!_hub->figure_click(_owner_object, target, point, button, state))
    return super::on_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------

bool CaptionFigure::on_double_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                    mdc::EventState state) {
  if (!_hub->figure_double_click(_owner_object, target, point, button, state))
    return super::on_double_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------

ShrinkableBox::ShrinkableBox(mdc::Layer *layer, mdc::Box::Orientation orientation)
  : mdc::Box(layer, orientation), _visible_part_size(0.0), _manual_resizing(false) {
  _hidden_item_count = 0;
  _limit_item_count = 0;
}

Size ShrinkableBox::calc_min_size() {
  if (_manual_resizing) {
    if (_children.empty())
      return Size(0, 0); // Box::calc_min_size();
    else {
      Size min_size = Box::calc_min_size();
      min_size.height = _children.front().item->get_min_size().height;

      return min_size;
    }
  } else if (_limit_item_count > 0 && _limit_item_count + 1 < (int)_children.size()) {
    Size min_item_size = _children.front().item->get_min_size();
    Size min_size = Box::calc_min_size();

    min_size.height = min_item_size.height * (_limit_item_count + 1) + _spacing * (_limit_item_count);

    return min_size;
  } else {
    return Box::calc_min_size();
  }
}

#define EXTRA_LABEL_SIZE 10
#define EXTRA_LABEL_PADDING 2

void ShrinkableBox::resize_to(const Size &size) {
  if (!_children.empty() &&
      (_manual_resizing || (_limit_item_count > 0 && _limit_item_count < (int)_children.size()))) {
    Size used_size;
    int number_of_items_that_fit = 0;
    int i;
    // hide items that won't fit
    Size min_item_size = _children.front().item->get_min_size();

    if (_orientation == mdc::Box::Horizontal) {
      number_of_items_that_fit = (int)floor((size.width - 2 * _xpadding + _spacing) / (min_item_size.width + _spacing));
    } else {
      number_of_items_that_fit =
        (int)floor((size.height - 2 * _ypadding + _spacing) / (min_item_size.height + _spacing));
    }

    _hidden_item_count = (int)_children.size() - number_of_items_that_fit;
    if (_hidden_item_count > 0) {
      // add space needed by "X items more..." label
      number_of_items_that_fit =
        (int)floor((size.height - EXTRA_LABEL_SIZE + _spacing) / (min_item_size.height + _spacing));
      _hidden_item_count = (int)_children.size() - number_of_items_that_fit;
    }

    if (number_of_items_that_fit > 0) {
      if (_orientation == mdc::Box::Horizontal)
        _visible_part_size = (float)(number_of_items_that_fit * (min_item_size.width + _spacing));
      else
        _visible_part_size = (float)(number_of_items_that_fit * (min_item_size.height + _spacing));
    } else
      _visible_part_size = 0;

    i = 0;
    for (ItemList::iterator iter = _children.begin(); iter != _children.end(); ++iter, ++i) {
      if (i < number_of_items_that_fit)
        iter->item->set_visible(true);
      else
        iter->item->set_visible(false);
    }
  }
  super::resize_to(size);
}

void ShrinkableBox::set_item_count_limit(int limit) {
  _limit_item_count = limit;

  resize_to(calc_min_size());
}

void ShrinkableBox::set_allow_manual_resizing(bool flag) {
  if (_manual_resizing != flag) {
    _manual_resizing = flag;
    if (!flag) {
      for (ItemList::iterator iter = _children.begin(); iter != _children.end(); ++iter)
        iter->item->set_visible(true);
      _hidden_item_count = 0;
    }
    set_needs_relayout();
  }
}

void ShrinkableBox::render(mdc::CairoCtx *cr) {
  super::render(cr);
  if (_hidden_item_count > 0) {
    char text[100];
    sprintf(text, "%i more...", _hidden_item_count);
    Point pos = get_position();

    cr->save();

    cairo_text_extents_t extents;
    mdc::FontSpec font("Helvetica", mdc::SNormal, mdc::WNormal, EXTRA_LABEL_SIZE);

    cr->get_text_extents(font, text, extents);
    cr->set_font(font);
    pos.y += _visible_part_size + (get_size().height - 2 * _ypadding - _visible_part_size - extents.height) / 2 -
             extents.y_bearing;
    pos.x += (get_size().width - extents.width) / 2;

    cr->move_to(pos);
    cr->show_text(text);
    cr->stroke();

    cr->restore();
  }
}
