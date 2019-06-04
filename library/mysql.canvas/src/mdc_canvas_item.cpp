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

#ifndef _MSC_VER
#include <stdexcept>
#endif

#include "mdc_canvas_item.h"
#include "mdc_algorithms.h"
#include "mdc_canvas_view.h"
#include "mdc_layer.h"
#include "mdc_draw_util.h"
#include "mdc_box_handle.h"
#include "mdc_interaction_layer.h"
#include "mdc_selection.h"
#include "mdc_magnet.h"
#include "mdc_bounds_magnet.h"
#include "base/log.h"

#define MAGNET_STICK_DISTANCE 5
DEFAULT_LOG_DOMAIN("canvas")
using namespace mdc;
using namespace base;

int mdc_live_item_count = 0;

CanvasItem::CanvasItem(Layer *layer) : _layer(layer), _parent(0) {
  mdc_live_item_count++;
  _pos.x = 0.0f;
  _pos.y = 0.0f;
  _focused = 0;
  _selected = 0;
  _accepts_focus = 0;
  _accepts_selection = 0;
  _draws_hover = 0;
  _hovering = 0;
  _disabled = 0;

  _auto_sizing = 1;
  _needs_render = 1;
  _visible = 1;
  _min_size_invalid = 1;
  _cache_toplevel_content = 0;
  _has_shadow = 0;
  _draggable = 0;
  _dragging = 0;
  _highlighted = 0;
  _highlight_color = 0;

  _disable_state_drawing = 0;

  _xpadding = 0;
  _ypadding = 0;

  _hresizeable = 1;
  _vresizeable = 1;

  _content_cache = 0;
  _content_texture = 0;
  _display_list = 0;

  _fixed_min_size = Size(-1, -1);
  _fixed_size = Size(-1, -1);

  _bounds_changed_signal.connect(std::bind(&CanvasItem::update_handles, this));

  scoped_connect(layer->get_view()->signal_zoom_changed(), std::bind(&CanvasItem::invalidate_cache, this));
}

CanvasItem::~CanvasItem() {
  --mdc_live_item_count;
  delete _highlight_color;

  if (_parent) {
    // if (!_parent->_destroyed)
    {
      Layouter *l = dynamic_cast<Layouter *>(_parent);
      if (l)
        l->remove(this);
    }
    _parent = 0;
  }
  get_layer()->remove_item(this);

  destroy_handles();

  for (std::vector<Magnet *>::iterator iter = _magnets.begin(); iter != _magnets.end(); ++iter)
    delete *iter;

  if (_content_cache)
    cairo_surface_destroy(_content_cache);

  if (_display_list != 0)
    glDeleteLists(_display_list, 1);

  if (_content_texture != 0)
    glDeleteTextures(1, &_content_texture);
}

mdc::CanvasItem *CanvasItem::find_item_with_tag(const std::string &tag) {
  if (tag == _tag)
    return this;
  return 0;
}

void CanvasItem::set_bounds(const Rect &rect) {
  Rect obounds = get_bounds();

  if (obounds != rect) {
    _pos = rect.pos;
    _size = rect.size;

    //  _bounds_changed_signal.emit(obounds);

    update_handles();
  }
}

void CanvasItem::set_position(const Point &pos) {
  if (_pos != pos) {
    Rect obounds = get_bounds();

    _pos = pos.round();

    _bounds_changed_signal(obounds);

    update_handles();
  }
}

void CanvasItem::set_size(const Size &size) {
  if (_size != size) {
    Rect obounds = get_bounds();

    _size = size;

    _bounds_changed_signal(obounds);

    update_handles();
  }
}

Rect CanvasItem::get_root_bounds() const {
  return Rect(get_root_position(), get_size());
}

Rect CanvasItem::get_padded_root_bounds() const {
  Rect bounds(get_root_bounds());

  bounds.pos.x -= LEFT_OUTER_PAD;
  bounds.pos.y -= TOP_OUTER_PAD;
  bounds.size.width += RIGHT_OUTER_PAD + LEFT_OUTER_PAD;
  bounds.size.height += BOTTOM_OUTER_PAD + TOP_OUTER_PAD;

  return bounds;
}

Rect CanvasItem::get_bounds() const {
  return Rect(get_position(), get_size());
}

Point CanvasItem::get_root_position() const {
  return convert_point_to(Point(0, 0), 0);
}

bool CanvasItem::intersects(const Rect &bounds) const {
  return bounds_intersect(bounds, get_bounds());
}

bool CanvasItem::contains_point(const Point &point) const {
  return bounds_contain_point(get_bounds(), point.x, point.y);
}

void CanvasItem::set_fixed_min_size(const Size &size) {
  _min_size_invalid = true;
  _fixed_min_size = size;
}

Size CanvasItem::calc_min_size() {
  return Size(_xpadding * 2, _ypadding * 2);
}

Size CanvasItem::get_min_size() {
  if (_min_size_invalid) {
    Size size = Size(-1, -1); //_fixed_size;
    Size msize;

    if (size.width < 0)
      size.width = _fixed_min_size.width;
    if (size.height < 0)
      size.height = _fixed_min_size.height;

    if (size.width >= 0 && size.height >= 0)
      _min_size = size;
    else {
      msize = calc_min_size();

      if (size.width < 0)
        size.width = msize.width;

      if (size.height < 0)
        size.height = msize.height;

      _min_size = size;
    }
    _min_size_invalid = false;
  }

  return _min_size;
}

void CanvasItem::set_padding(double xpad, double ypad) {
  _xpadding = xpad;
  _ypadding = ypad;

  set_needs_relayout();
}

void CanvasItem::resize_to(const Size &size) {
  if (_size != size) {
    set_size(size);
    set_needs_render();
  }
}

void CanvasItem::move_to(const Point &pos) {
  set_position(pos);

  if (is_toplevel())
    set_needs_repaint();
  else
    set_needs_render();
  //  _layer->set_needs_repaint();
}

void CanvasItem::set_fixed_size(const Size &size) {
  Rect obounds(get_bounds());

  _min_size_invalid = true;
  _fixed_size = size;
  _size = size;
  _bounds_changed_signal(obounds);
  set_needs_relayout();
}

void CanvasItem::parent_bounds_changed(const Rect &obounds, CanvasItem *item) {
  _parent_bounds_changed_signal(item, obounds);

  update_handles();
}

void CanvasItem::grand_parent_bounds_changed(CanvasItem *item, const Rect &obounds) {
  _parent_bounds_changed_signal(item, obounds);

  update_handles();
}

void CanvasItem::set_parent(CanvasItem *parent) {
  if (parent != 0 && _parent != 0 && parent != _parent)
    throw std::logic_error("setting parent to already parented item");

  _parent = parent;

  if (parent) {
    _reparent_signal();

    _parent_bounds_con = parent->signal_bounds_changed()->connect(
      std::bind(&CanvasItem::parent_bounds_changed, this, std::placeholders::_1, parent));

    _grand_parent_bounds_con = parent->signal_parent_bounds_changed()->connect(
      std::bind(&CanvasItem::grand_parent_bounds_changed, this, std::placeholders::_1, std::placeholders::_2));
  }
}

void CanvasItem::remove_from_parent() {
  if (_parent)
    dynamic_cast<Layouter *>(_parent)->remove(this);
}

CanvasView *CanvasItem::get_view() const {
  if (_layer)
    return _layer->get_view();
  return 0;
}

void CanvasItem::set_accepts_focus(bool flag) {
  _accepts_focus = flag;
}

void CanvasItem::set_accepts_selection(bool flag) {
  _accepts_selection = flag;
}

void CanvasItem::set_draws_hover(bool flag) {
  if (_draws_hover != flag) {
    _draws_hover = flag;
    set_needs_render();
  }
}

void CanvasItem::set_highlighted(bool flag) {
  if (_highlighted != flag) {
    _highlighted = flag;
    set_needs_render();
  }
}

void CanvasItem::set_highlight_color(const Color *color) {
  if (_highlight_color)
    delete _highlight_color;

  if (color)
    _highlight_color = new Color(*color);
  else
    _highlight_color = 0;

  if (_highlighted)
    set_needs_render();
}

void CanvasItem::set_selected(bool flag) {
  if (_selected != flag) {
    _selected = flag;
    if (!_selected)
      get_layer()->get_view()->focus_item(0);

    set_needs_render();
  }
}

void CanvasItem::set_focused(bool flag) {
  if (_focused != flag) {
    _focused = flag;
    set_needs_render();

    _focus_changed_signal(flag);
  }
}

void CanvasItem::set_allowed_resizing(bool horizontal, bool vertical) {
  _hresizeable = horizontal;
  _vresizeable = vertical;
}

void CanvasItem::set_draggable(bool flag) {
  _draggable = flag;
}

void CanvasItem::set_auto_sizing(bool flag) {
  _auto_sizing = flag;
  _min_size_invalid = true;
  set_needs_relayout();
}

//------------------------------------------------------------------------------

void CanvasItem::set_cache_toplevel_contents(bool flag) {
  _cache_toplevel_content = flag;
}

void CanvasItem::invalidate_cache() {
  if (_content_cache) {
    _layer->get_view()->bookkeep_cache_mem(-cairo_image_surface_get_stride(_content_cache) *
                                           cairo_image_surface_get_height(_content_cache));
    cairo_surface_destroy(_content_cache);
  }
  _content_cache = 0;
  set_needs_render();
}

void CanvasItem::set_has_shadow(bool flag) {
  if (_has_shadow != flag) {
    _has_shadow = flag;
    set_needs_render();
  }
}

void CanvasItem::set_visible(bool flag) {
  if (_visible != flag) {
    _visible = flag;
    set_needs_relayout();
  }
}

bool CanvasItem::get_parents_visible() const {
  CanvasItem *item = get_parent();

  while (item && !item->is_toplevel()) {
    if (!item->get_visible())
      return false;
    item = item->get_parent();
  }
  return true;
}

void CanvasItem::set_needs_repaint() {
  Rect bounds(get_root_bounds());

  bounds.pos.x -= LEFT_OUTER_PAD;
  bounds.pos.y -= TOP_OUTER_PAD;
  bounds.size.width += LEFT_OUTER_PAD + RIGHT_OUTER_PAD;
  bounds.size.height += TOP_OUTER_PAD + BOTTOM_OUTER_PAD;

  if (bounds.pos.x < 0)
    bounds.pos.x = 0;
  if (bounds.pos.y < 0)
    bounds.pos.y = 0;

  if (_old_bounds != bounds) {
    if (_old_bounds.width() > 0 && _old_bounds.height() > 0)
      _layer->queue_repaint(_old_bounds);
    _old_bounds = bounds;
  }
  _layer->queue_repaint(_old_bounds);
}

void CanvasItem::set_needs_render() {
  /*
  if (!_needs_render)
  {
    if (_parent && !is_toplevel())
      _parent->set_needs_render();
    else
    {
      _needs_render= true;
      _layer->set_needs_repaint();//get_bounds();
    }
  }*/

  if (_parent && !is_toplevel())
    _parent->set_needs_render();
  else if (!_needs_render) {
    _needs_render = true;

    set_needs_repaint();
  }
}

void CanvasItem::set_needs_relayout() {
  _min_size_invalid = 1;
  // propagate the relayout request up until the last parent, which will
  // be a toplevel item. the toplevel will in its turn, add itself to the layers
  // relayout queue, which will be flushed on the next redraw oportunity
  if (_parent && !is_toplevel())
    _parent->set_needs_relayout();
  else {
    CanvasItem *toplevel = get_toplevel();
    if (toplevel)
      _layer->queue_relayout(toplevel);
  }
  set_needs_render();
}

void CanvasItem::auto_size() {
  Size size = _fixed_size;
  Size minsize = get_min_size();

  minsize.width += _xpadding * 2;
  minsize.height += _ypadding * 2;

  if (size.width < 0)
    size.width = minsize.width;
  if (size.height < 0)
    size.height = minsize.height;

  resize_to(size);
}

void CanvasItem::relayout() {
  // called by layer only
  if (_auto_sizing)
    auto_size();
  else {
    Size size = get_fixed_size();

    if (size.width < 0)
      size.width = _size.width;
    if (size.height < 0)
      size.height = _size.height;

    resize_to(size);
  }
}

bool CanvasItem::is_toplevel() const {
  if (dynamic_cast<Group *>(_parent))
    return true;
  return false;
}

CanvasItem *CanvasItem::get_toplevel() const {
  if (_parent) {
    if (is_toplevel())
      return (CanvasItem *)this;
    return _parent->get_toplevel();
  }
  return 0;
}

CanvasItem::State CanvasItem::get_state() {
  if (_disabled)
    return Disabled;
  else if (_hovering && _draws_hover)
    return Hovering;
  else if (_highlighted)
    return Highlighted;
  else if (_selected)
    return Selected;
  return Normal;
}

void CanvasItem::set_state_drawing(bool flag) {
  _disable_state_drawing = !flag;
}

//--------------------------------------------------------------------------------------------------

void CanvasItem::draw_state(CairoCtx *cr) {
  if (!get_view()->is_printout() && !_disable_state_drawing) {
    switch (get_state()) {
      case Disabled:
        break;

      case Hovering:
        draw_outline_ring(cr, get_view()->get_hover_color());
        break;

      case Highlighted:
        draw_outline_ring(cr, _highlight_color ? *_highlight_color : get_view()->get_highlight_color());
        break;

      case Selected:
        draw_outline_ring(cr, get_view()->get_selection_color());
        break;

      case Normal:
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void CanvasItem::draw_state_gl() {
  if (!get_view()->is_printout() && !_disable_state_drawing) {
    switch (get_state()) {
      case Disabled:
        break;

      case Hovering:
        draw_outline_ring_gl(get_view()->get_hover_color());
        break;

      case Highlighted:
        draw_outline_ring_gl(_highlight_color ? *_highlight_color : get_view()->get_highlight_color());
        break;

      case Selected:
        draw_outline_ring_gl(get_view()->get_selection_color());
        break;

      case Normal:
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void CanvasItem::draw_outline_ring(CairoCtx *cr, const Color &color) {
  cr->save();

  cr->set_color(color, color.alpha);
  cr->set_line_width(2);
  stroke_outline(cr, 1);
  cr->stroke();

  cr->set_color(color, color.alpha * 0.3);
  cr->set_line_width(4);
  stroke_outline(cr, 2);
  cr->stroke();

  cr->restore();
}

//--------------------------------------------------------------------------------------------------

void CanvasItem::draw_outline_ring_gl(const Color &color) {
#ifndef __APPLE__
  gl_setcolor(color);
  glLineWidth(2);
  stroke_outline_gl(1);

  Color blended = Color(color.red, color.green, color.blue, 0.3 * color.alpha);
  gl_setcolor(blended);
  glLineWidth(4);
  stroke_outline_gl(1);
  glLineWidth(1);
#endif
}

//--------------------------------------------------------------------------------------------------

void CanvasItem::render(CairoCtx *cr) {
}

void CanvasItem::render_gl(CairoCtx *cr) {
}

void CanvasItem::render_to_surface(cairo_surface_t *surf, bool use_padding) {
  CairoCtx cr(surf);

  cr.scale(_layer->get_view()->get_zoom(), _layer->get_view()->get_zoom());
  if (use_padding)
    cr.translate(floor(LEFT_OUTER_PAD - _pos.x), floor(TOP_OUTER_PAD - _pos.y));
  else
    cr.translate(floor(-_pos.x), floor(-_pos.y));

  render(&cr);
}

void CanvasItem::repaint_gl(const Rect &clipArea) {
#ifndef __APPLE__
  CairoCtx *ccr = _layer->get_view()->cairoctx();

  // 1st check if we can directly render as gl
  if (can_render_gl()) {
    render_gl(ccr);
    return;
  }

  // if direct gl render wasn't available, then use the cache as a texture and render
  // that instead
  bool generate_display_list = _display_list == 0;

  // Check if we need to regenerate the cache. if so, do it and load it as a texture.
  Size texture_size = get_texture_size(Size(0, 0));
  if (_needs_render || _content_texture == 0) {
    generate_display_list = true;

    // _content_cache is the bitmap with image data, we load that as a texture and release it.
    regenerate_cache(texture_size);

    if (!_content_cache) {
      return;
    }

    if (_content_texture == 0)
      glGenTextures(1, &_content_texture);

    // setup the texture
    glBindTexture(GL_TEXTURE_2D, _content_texture);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // don't tile the image
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    // load the texture into opengl
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)texture_size.width, (int)texture_size.height, 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, // GL_UNSIGNED_INT_8_8_8_8_REV, // <-- optimal format
                 cairo_image_surface_get_data(_content_cache));

    // Once we transferred the pixel data we don't need the cache anymore.
    _layer->get_view()->bookkeep_cache_mem(-cairo_image_surface_get_stride(_content_cache) *
                                           cairo_image_surface_get_height(_content_cache));
    cairo_surface_destroy(_content_cache);
    _content_cache = 0;
  }

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  Rect bounds(get_bounds());

  bounds.pos.x -= LEFT_OUTER_PAD;
  bounds.pos.y -= TOP_OUTER_PAD;
  bounds.size.width += RIGHT_OUTER_PAD + LEFT_OUTER_PAD;
  bounds.size.height += BOTTOM_OUTER_PAD + TOP_OUTER_PAD;

  glTranslated(bounds.left(), bounds.top(), 0);

  if (generate_display_list) {
    if (_display_list == 0)
      _display_list = glGenLists(1);

    glNewList(_display_list, GL_COMPILE);

    glEnable(GL_TEXTURE_2D);

    // Render the texture.
    glBindTexture(GL_TEXTURE_2D, _content_texture);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);

    // Due to round-up to power of two the actual texture coordinates are usually somewhere within the actual texture.
    // Box coordinates must be scaled too, as the texture size depends on the zoom factor, however the box size does
    // not.
    double width = bounds.width();
    double height = bounds.height();
    cairo_user_to_device_distance(_layer->get_view()->cairoctx()->get_cr(), &width, &height);
    double max_x_coordinate = width / texture_size.width;
    double max_y_coordinate = height / texture_size.height;

    glTexCoord2d(0, 0);
    glVertex2d(0, 0);

    glTexCoord2d(max_x_coordinate, 0);
    glVertex2d(bounds.width(), 0);

    glTexCoord2d(max_x_coordinate, max_y_coordinate);
    glVertex2d(bounds.width(), bounds.height());

    glTexCoord2d(0, max_y_coordinate);
    glVertex2d(0, bounds.height());

    glEnd();
    glDisable(GL_TEXTURE_2D);

    glEndList();
  }

  glCallList(_display_list);

  glPopMatrix();
#endif
}

void CanvasItem::repaint(const Rect &clipArea, bool direct) {
  // Don't render OpenGL commands if "direct" is true, which means we are rendering to off-screen bitmap
  // (for printing, png/pdf export or similar).
  if (_layer->get_view()->has_gl() && !direct)
    repaint_gl(clipArea);
  else {
    if (direct)
      repaint_direct();
    else
      repaint_cached();
  }
}

void CanvasItem::repaint_direct() {
  CairoCtx *ccr = _layer->get_view()->cairoctx();

  ccr->save();

  render(ccr);

  ccr->restore();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the size we need to use for allocating the cache and the final texture. Depends on padding, zoom
 * and other factors.
 *
 * @param size The base size to use to compute the right texture size (must be a power of 2). Can be 0 in which
 *             case the item's entire size is used.
 */
Size CanvasItem::get_texture_size(Size size) {
  if (size.width == 0 || size.height == 0) {
    size = get_size();

    size.width += LEFT_OUTER_PAD + RIGHT_OUTER_PAD;
    size.height += TOP_OUTER_PAD + BOTTOM_OUTER_PAD;
  }

  cairo_user_to_device_distance(_layer->get_view()->cairoctx()->get_cr(), &size.width, &size.height);

  // Make the size a power of two, as required by OpenGL (at least for versions < 2.0) for textures.
  size.width = 1 << int(ceil(log(size.width) / M_LN2));
  size.height = 1 << int(ceil(log(size.height) / M_LN2));

  return size;
}

//----------------------------------------------------------------------------------------------------------------------

void CanvasItem::regenerate_cache(Size size) {
  if (!_content_cache || ((int)size.width != cairo_image_surface_get_width(_content_cache) ||
                          (int)size.height != cairo_image_surface_get_height(_content_cache))) {
    if (_content_cache) {
      _layer->get_view()->bookkeep_cache_mem(-cairo_image_surface_get_stride(_content_cache) *
                                             cairo_image_surface_get_height(_content_cache));
      cairo_surface_destroy(_content_cache);
    }

    _content_cache =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (unsigned int)size.width, (unsigned int)size.height);

    _layer->get_view()->bookkeep_cache_mem(cairo_image_surface_get_stride(_content_cache) *
                                           cairo_image_surface_get_height(_content_cache));

#ifndef WIN32
    if (_layer->get_view()->debug_enabled())
      g_message("creating cached image for %p (%i)", this,
                cairo_image_surface_get_stride(_content_cache) * cairo_image_surface_get_height(_content_cache));
#endif
  }
  memset(cairo_image_surface_get_data(_content_cache), 0,
         cairo_image_surface_get_stride(_content_cache) * cairo_image_surface_get_height(_content_cache));

  render_to_surface(_content_cache);
  _needs_render = false;
}

void CanvasItem::repaint_cached() {
  // during zooming, the cache must be rendered with scaling enabled,
  // but the blitting of the rendered image must be done with no zooming

  if ((_needs_render || !_content_cache) && _cache_toplevel_content) {
    Size size = get_texture_size(Size(0, 0));
    regenerate_cache(size);
  }

  _needs_render = false;

  if (_content_cache) {
#ifndef WIN32
    if (_layer->get_view()->debug_enabled())
      logDebug3("paint cache data for %p", this);
#endif
    // paint the image to the canvas
    _layer->get_view()->paint_item_cache(_layer->get_view()->cairoctx(), _pos.x - LEFT_OUTER_PAD,
                                         _pos.y - TOP_OUTER_PAD, _content_cache);
  } else {
    // render directly into canvas
    CairoCtx *ccr = _layer->get_view()->cairoctx();

    ccr->save();
    render(ccr);
    ccr->restore();
  }
}

CanvasItem *CanvasItem::get_common_ancestor(CanvasItem *item) const {
  const CanvasItem *my_ancestor, *other_ancestor;

  for (my_ancestor = this; my_ancestor != NULL; my_ancestor = my_ancestor->get_parent()) {
    for (other_ancestor = item; other_ancestor != NULL; other_ancestor = other_ancestor->get_parent()) {
      if (my_ancestor == other_ancestor)
        return (CanvasItem *)my_ancestor;
    }
  }
  return 0;
}

Point CanvasItem::convert_point_from(const Point &pt, CanvasItem *item) const {
  CanvasItem *ancestor = 0;
  const CanvasItem *it;
  Point point = pt;

  if (item) {
    ancestor = get_common_ancestor(item);

    // add offset from item to the common ancestor
    for (it = item; it != ancestor; it = it->get_parent())
      point = point + it->get_position();
  }

  // sub offset from us to the common ancestor
  for (it = this; it != ancestor; it = it->get_parent())
    point = point - it->get_position();

  return point;
}

Point CanvasItem::convert_point_to(const Point &pt, CanvasItem *item) const {
  CanvasItem *ancestor = item ? get_common_ancestor(item) : 0;
  const CanvasItem *it;
  Point point = pt;

  // add offset from us to the common ancestor
  for (it = this; it != ancestor; it = it->get_parent())
    point = point + it->get_position();

  if (item) {
    // sub offset from item to the common ancestor
    for (it = item; it != ancestor; it = it->get_parent())
      point = point - it->get_position();
  }
  return point;
}

Point CanvasItem::get_intersection_with_line_to(const Point &p) {
  Rect bounds(get_root_bounds());
  Point p1;
  Point p2;

  if (intersect_rect_to_line(bounds, bounds.center(), p, p1, p2)) {
    return p1;
  }

  return p;
  // throw std::logic_error("bounds intersection function has failed");
}

//--------------------------------------------------------------------------------

void CanvasItem::destroy_handles() {
  for (std::vector<ItemHandle *>::iterator i = _handles.begin(); i != _handles.end(); ++i) {
    delete *i;
  }
  _handles.clear();
}

void CanvasItem::create_handles(InteractionLayer *ilayer) {
  ItemHandle *hdl;
  struct {
    int tag;
    float x, y;
  } pos[] = {{HDL_TL, 0, 0},  {HDL_T, 0.5, 0}, {HDL_TR, 1, 0},  {HDL_L, 0, 0.5},
             {HDL_R, 1, 0.5}, {HDL_BL, 0, 1},  {HDL_B, 0.5, 1}, {HDL_BR, 1, 1}};
  Size size = get_size();

  for (int i = 0; i < 8; i++) {
    Point pt = convert_point_to(Point(ceil(size.width * pos[i].x), ceil(size.height * pos[i].y)), 0);

    hdl = new BoxHandle(ilayer, this, pt);
    hdl->set_draggable(_vresizeable || _hresizeable);
    hdl->set_tag(pos[i].tag);
    ilayer->add_handle(hdl);
    _handles.push_back(hdl);
  }
}

void CanvasItem::update_handles() {
  if (!_handles.empty()) {
    Size size = get_size();
    struct {
      int tag;
      float x, y;
    } pos[] = {{HDL_TL, 0, 0},  {HDL_T, 0.5, 0}, {HDL_TR, 1, 0},  {HDL_L, 0, 0.5},
               {HDL_R, 1, 0.5}, {HDL_BL, 0, 1},  {HDL_B, 0.5, 1}, {HDL_BR, 1, 1}};

    for (int i = 0; i < 8; i++) {
      Point pt = convert_point_to(Point(ceil(size.width * pos[i].x), ceil(size.height * pos[i].y)), 0);
      _handles[i]->move(pt);
    }
  }
}

void CanvasItem::magnetize_bounds() {
  add_magnet(new BoundsMagnet(this));
}

void CanvasItem::add_magnet(Magnet *magnet) {
  _magnets.push_back(magnet);
}

BoundsMagnet *CanvasItem::get_bounds_magnet() {
  for (std::vector<Magnet *>::const_iterator iter = _magnets.begin(); iter != _magnets.end(); ++iter) {
    if (dynamic_cast<BoundsMagnet *>(*iter))
      return dynamic_cast<BoundsMagnet *>(*iter);
  }
  return 0;
}

mdc::Magnet *CanvasItem::get_closest_magnet(const Point &point) {
  Point lpos = convert_point_from(point, 0);
  double d, bestd = MAGNET_STICK_DISTANCE;
  Magnet *best = 0;
  Magnet *bmagnet = 0;

  for (std::vector<Magnet *>::const_iterator iter = _magnets.begin(); iter != _magnets.end(); ++iter) {
    if (dynamic_cast<BoundsMagnet *>(*iter))
      bmagnet = *iter;

    d = points_distance(lpos, (*iter)->get_position_for_connector(0, Point()));
    if (d < bestd) {
      bestd = d;
      best = *iter;
    }
  }

  if (!best)
    return bmagnet;

  return best;
}

void CanvasItem::set_drag_handle_constrainer(const std::function<void(ItemHandle *, Size &)> &slot) {
  _drag_handle_constrainer = slot;
}

//------------------------------------------------------------------------------

bool CanvasItem::on_drag_handle(ItemHandle *handle, const Point &pos, bool dragging) {
  Rect oframe = get_root_bounds();
  Point npos = get_position();
  Size nsize = get_size();
  Point local_pos = pos - get_parent()->get_root_position();
  Size max_size = get_parent()->get_size();
  Size min_size = get_min_size();

  if (_hresizeable) {
    if ((handle->get_tag() & HDL_LR_MASK) == HDL_RIGHT) {
      nsize.width = pos.x - oframe.left();
      if (min_size.width > 0 && nsize.width < min_size.width)
        nsize.width = min_size.width;
      else if (nsize.width > max_size.width - npos.x)
        nsize.width = max_size.width - npos.x;
      else if (nsize.width <= 0)
        nsize.width = 1;
    } else if ((handle->get_tag() & HDL_LR_MASK) == HDL_LEFT) {
      npos.x = local_pos.x;
      nsize.width = oframe.width() + (oframe.left() - pos.x);

      if (min_size.width > 0 && nsize.width < min_size.width) {
        double dx = min_size.width - nsize.width;
        npos.x -= dx;
        nsize.width = min_size.width;
      } else if (npos.x < 0) {
        nsize.width += npos.x;
        npos.x = 0;
      }
    }
  }
  if (_vresizeable) {
    if ((handle->get_tag() & HDL_TB_MASK) == HDL_BOTTOM) {
      nsize.height = pos.y - oframe.top();
      if (min_size.height > 0 && nsize.height < min_size.height)
        nsize.height = min_size.height;
      else if (nsize.height > max_size.height - npos.y)
        nsize.height = max_size.height - npos.y;
      else if (nsize.height <= 0)
        nsize.height = 1;
    } else if ((handle->get_tag() & HDL_TB_MASK) == HDL_TOP) {
      npos.y = local_pos.y;
      nsize.height = oframe.height() + (oframe.top() - pos.y);
      if (min_size.height > 0 && nsize.height < min_size.height) {
        double dy = min_size.height - nsize.height;
        npos.y -= dy;
        nsize.height = min_size.height;
      } else if (npos.y < 0) {
        nsize.height += npos.y;
        npos.y = 0;
      }
    }
  }

  if (_drag_handle_constrainer)
    _drag_handle_constrainer(handle, nsize);

  Point npos2(npos);
  npos = get_view()->snap_to_grid(npos).round();

  // adjustment
  nsize.width += npos2.x - npos.x;
  nsize.height += npos2.y - npos.y;

  nsize = get_view()->snap_to_grid(nsize).round();

  if (nsize.width <= 0)
    throw;

  if (npos != get_position())
    move_to(npos);
  if (nsize != get_size())
    resize_to(nsize);

  update_handles();

  return true;
}

//--------------------------------------------------------------------------------------------------

bool CanvasItem::on_click(CanvasItem *target, const Point &point, MouseButton button, EventState state) {
  if (button == ButtonLeft && !_dragged) {
    CanvasView *view = get_layer()->get_view();

    // if we're a toplevel, focus and select ourselves
    if (is_toplevel()) {
      if (accepts_focus()) {
        if (state & SControlMask) {
          // focus is handled by elsewhere now
          // if (get_focused() || !get_selected())
          //  view->focus_item(0);
          // else
          //  view->focus_item(this);
        } else {
          if ((state & SModifierMask) == 0)
            view->get_selection()->set(this);
          // view->focus_item(this);
        }
      }
      return true;
    } else // a child item
    {
      // if the parent is focused and we're focusable, then focus ourselves
      if (accepts_focus()) {
        CanvasItem *parent = get_parent();
        // get the 1st parent that accepts focus
        while (parent && !parent->accepts_focus())
          parent = parent->get_parent();

        // conditions met, set focus
        if (parent && parent->accepts_focus())
          view->focus_item(this);

        return true;
      }
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool CanvasItem::on_double_click(CanvasItem *target, const Point &point, MouseButton button, EventState state) {
  return false;
}

//--------------------------------------------------------------------------------------------------

bool CanvasItem::on_button_press(CanvasItem *target, const Point &point, MouseButton button, EventState state) {
  _button_press_pos = point;

  // if we're a toplevel, prepare for dragging
  if (button == ButtonLeft) {
    _dragged = 0;
    if (is_toplevel()) // && target->_draggable)
    {
      CanvasView *view = get_layer()->get_view();

      if (accepts_selection()) {
        if (state & (SControlMask | SCommandMask))
          view->get_selection()->toggle(this);
        else if (state & SShiftMask)
          view->get_selection()->add(this);
        else {
          // select on click or drag
          //          if (!get_selected())
          // view->get_selection()->set(this);
        }
      }

      /*
      if (!get_selected())
      {
        if (accepts_focus())
          view->focus_item(this);
      }*/

      // moved to on_drag
      //      _dragging= true;
      //      view->get_selection()->begin_moving(convert_point_to(point, 0));

      return true;
    }
  }
  return false;
}

bool CanvasItem::on_button_release(CanvasItem *target, const Point &point, MouseButton button, EventState state) {
  if (button == ButtonLeft) {
    if (is_toplevel()) // && target->_draggable)
    {
      CanvasView *view = get_layer()->get_view();

      if (_dragging)
        view->get_selection()->end_moving();
      _dragging = false;

      return true;
    }
  }

  return false;
}

bool CanvasItem::on_drag(CanvasItem *target, const Point &point, EventState state) {
  _dragged = 1;

  if (is_toplevel() && (state & SLeftButtonMask)) {
    CanvasView *view = get_layer()->get_view();

    if (!get_selected()) {
      // if not selected, then select the object and start the drag

      view->get_selection()->set(this);
    }

    if (!_dragging) {
      _dragging = true;
      view->get_selection()->begin_moving(convert_point_to(_button_press_pos, 0));
    }

    if (get_selected()) {
      if (target->_draggable || target->get_toplevel()->_draggable)
        view->get_selection()->update_move(convert_point_to(point, 0));
    }
    return true;
  }

  return false;
}

bool CanvasItem::on_enter(CanvasItem *target, const Point &point) {
  // on_enter and on_leave return true (block propagation)
  // by default, unlike other events.
  // the parent items will receive their own crossing events
  if (!_hovering) {
    _hovering = 1;
    if (_draws_hover)
      set_needs_render();
  }
  return true;
}

bool CanvasItem::on_leave(CanvasItem *target, const Point &point) {
  // on_enter and on_leave return true (block propagation)
  // by default, unlike other events.
  if (_hovering) {
    _hovering = 0;
    if (_draws_hover)
      set_needs_render();
  }
  return true;
}
