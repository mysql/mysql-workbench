/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

/**
 * Implementation of the mforms view, which is the base for most of the visual controls in mforms.
 */

#include "mforms/mforms.h"

#include "base/log.h"
#include "base/notifications.h"

using namespace mforms;

//--------------------------------------------------------------------------------------------------

View::View() {
  _parent = NULL;
  _view_impl = &ControlFactory::get_instance()->_view_impl;
  _layout_dirty = true;
}

//--------------------------------------------------------------------------------------------------

View::~View() {
  set_destroying();
  if (_parent && !_parent->is_destroying())
    _parent->remove_from_cache(this);

  clear_subviews();

#ifdef __APPLE__
  // Let the frontend delete all resources it allocated.
  // This is only needed for OSX as on Win + Linux we use the data free function (set in set_data()) to free
  // platform resources.
  if (_view_impl->destroy)
    _view_impl->destroy(this);
#endif
}

//--------------------------------------------------------------------------------------------------

void View::clear_subviews() {
  while (_subviews.size() > 0)
    remove_from_cache(
      _subviews[0].first); // Let descendants adjust their child lists. This will also release the object if necessary.
}

//--------------------------------------------------------------------------------------------------

void View::set_managed() {
  Object::set_managed();
  if (_parent) {
    for (std::vector<std::pair<View *, bool> >::iterator iter = _parent->_subviews.begin();
         iter != _parent->_subviews.end(); ++iter) {
      if (iter->first == this) {
        iter->second = true;
        break;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

void View::cache_view(View *sv) {
  if (!sv)
    throw std::logic_error("mforms: attempt to add NULL subview");

  if (sv->get_parent() != NULL)
    throw std::logic_error("mforms: attempt to add a subview already contained somewhere");

  if (sv == this)
    throw std::logic_error("mforms: Can't add a view inside itself");

  sv->set_parent(this);
  if (!sv->_release_on_add) // Means: don't increase the ref count, the caller retained already, but won't release.
    sv->retain();
  else
    sv->_release_on_add = false;

  _subviews.push_back(std::make_pair(sv, sv->_managed));
}

//--------------------------------------------------------------------------------------------------

void View::reorder_cache(View *sv, int position) {
  int old = get_subview_index(sv);
  if (old < 0)
    throw std::invalid_argument("mforms: invalid subview");

  std::pair<View *, bool> value = _subviews[old];
  _subviews.erase(_subviews.begin() + old);
  _subviews.insert(_subviews.begin() + position, value);
}

//--------------------------------------------------------------------------------------------------

void View::remove_from_cache(View *sv) {
  sv->_parent = NULL;
  for (std::vector<std::pair<View *, bool> >::iterator iter = _subviews.begin(); iter != _subviews.end(); ++iter) {
    if (iter->first == sv) {
      _subviews.erase(iter);
      sv->release();
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Searches for a subview with the given name in this view or any of its subviews using a depth-first search.
 */
View *View::find_subview(const std::string &name) {
  for (std::vector<std::pair<View *, bool> >::const_iterator iter = _subviews.begin(); iter != _subviews.end();
       ++iter) {
    if (iter->first->getInternalName() == name)
      return iter->first;

    View *sv = iter->first->find_subview(name);
    if (sv)
      return sv;
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

int View::get_subview_index(View *sv) {
  int i = 0;
  for (std::vector<std::pair<View *, bool> >::const_iterator iter = _subviews.begin(); iter != _subviews.end();
       ++iter, ++i) {
    if (iter->first == sv)
      return i;
  }
  return -1;
}

//--------------------------------------------------------------------------------------------------

View *View::get_subview_at_index(int index) {
  if (index < 0 || index >= (int)_subviews.size())
    return NULL;

  return _subviews[index].first;
}

//--------------------------------------------------------------------------------------------------

int View::get_subview_count() {
  return (int)_subviews.size();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the given subview is a direct child of this view.
 */
bool View::contains_subview(View *subview) {
  return subview->get_parent() == this;
}

//--------------------------------------------------------------------------------------------------

void View::set_name(const std::string &name) {
  // Optional implementation.
  if (_view_impl->set_name)
    _view_impl->set_name(this, name);
}

//--------------------------------------------------------------------------------------------------

void View::set_tooltip(const std::string &text) {
  _view_impl->set_tooltip(this, text);
}

//--------------------------------------------------------------------------------------------------

void View::set_font(const std::string &fontDescription) {
  _view_impl->set_font(this, fontDescription);
}

//--------------------------------------------------------------------------------------------------

void View::setInternalName(const std::string &name) {
  _internalName = name;
}

//--------------------------------------------------------------------------------------------------

std::string View::getInternalName() const {
    return _internalName;
}


//--------------------------------------------------------------------------------------------------

void View::set_parent(View *parent) {
  _parent = parent;
  if (_managed)
    set_managed();
}

//--------------------------------------------------------------------------------------------------

View *View::get_parent() const {
  return _parent;
}

//--------------------------------------------------------------------------------------------------

Form *View::get_parent_form() const {
  View *parent = get_parent();
  Form *form = 0;
  while (parent && (form = dynamic_cast<Form *>(parent)) == 0)
    parent = parent->get_parent();

  return form;
}

//--------------------------------------------------------------------------------------------------

int View::get_width() const {
  return (*_view_impl->get_width)(this);
}

//--------------------------------------------------------------------------------------------------

int View::get_height() const {
  return (*_view_impl->get_height)(this);
}

//--------------------------------------------------------------------------------------------------

int View::get_preferred_width() {
  return (*_view_impl->get_preferred_width)(this);
}

//--------------------------------------------------------------------------------------------------

int View::get_preferred_height() {
  return (*_view_impl->get_preferred_height)(this);
}

//--------------------------------------------------------------------------------------------------

int View::get_x() const {
  return (*_view_impl->get_x)(this);
}

//--------------------------------------------------------------------------------------------------

int View::get_y() const {
  return (*_view_impl->get_y)(this);
}

//--------------------------------------------------------------------------------------------------

void View::set_position(int x, int y) {
  (*_view_impl->set_position)(this, x, y);
}

//--------------------------------------------------------------------------------------------------

void View::set_size(int width, int height) {
  set_layout_dirty(true);
  (*_view_impl->set_size)(this, width, height);
}

//--------------------------------------------------------------------------------------------------

void View::set_min_size(int width, int height) {
  set_layout_dirty(true);
  (*_view_impl->set_min_size)(this, width, height);
}

//--------------------------------------------------------------------------------------------------

std::pair<int, int> View::client_to_screen(int x, int y) {
  return (*_view_impl->client_to_screen)(this, x, y);
}

//--------------------------------------------------------------------------------------------------

std::pair<int, int> View::screen_to_client(int x, int y) {
  return (*_view_impl->screen_to_client)(this, x, y);
}

//--------------------------------------------------------------------------------------------------

void View::show(bool flag) {
  (*_view_impl->show)(this, flag);
}

//--------------------------------------------------------------------------------------------------

bool View::is_shown() {
  return (*_view_impl->is_shown)(this);
}

//--------------------------------------------------------------------------------------------------

bool View::is_fully_visible() {
  return (*_view_impl->is_fully_visible)(this);
}

//--------------------------------------------------------------------------------------------------

void View::set_enabled(bool flag) {
  (*_view_impl->set_enabled)(this, flag);
}

//--------------------------------------------------------------------------------------------------

bool View::is_enabled() {
  return (*_view_impl->is_enabled)(this);
}

//--------------------------------------------------------------------------------------------------

void View::set_needs_repaint() {
  _view_impl->set_needs_repaint(this);
}

//--------------------------------------------------------------------------------------------------

void View::set_layout_dirty(bool value) {
  _layout_dirty = value;
  if (_parent != NULL && value)
    _parent->set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

bool View::is_layout_dirty() {
  return _layout_dirty;
}

//--------------------------------------------------------------------------------------------------

void View::relayout() {
  _view_impl->relayout(this);
  if (_parent != nullptr) // Propagate relayout up the parent chain.
    _parent->relayout();
}

//--------------------------------------------------------------------------------------------------

void View::suspend_layout() {
  if (_view_impl->suspend_layout)
    _view_impl->suspend_layout(this, true);
}

//--------------------------------------------------------------------------------------------------

void View::resume_layout() {
  if (_view_impl->suspend_layout)
    _view_impl->suspend_layout(this, false);
}

//--------------------------------------------------------------------------------------------------

void View::set_front_color(const std::string &color) {
  _view_impl->set_front_color(this, color);
}

//--------------------------------------------------------------------------------------------------

std::string View::get_front_color() {
  return _view_impl->get_front_color(this);
}

//--------------------------------------------------------------------------------------------------

void View::set_back_color(const std::string &color) {
  _view_impl->set_back_color(this, color);
}

//--------------------------------------------------------------------------------------------------

std::string View::get_back_color() {
  return _view_impl->get_back_color(this);
}

//--------------------------------------------------------------------------------------------------

void View::set_back_image(const std::string &path, Alignment align) {
  _view_impl->set_back_image(this, path, align);
}

//--------------------------------------------------------------------------------------------------
// Below code is used only for debug purpose.
// It's using the object::retain_count.
#ifdef _0
void View::show_retain_counts(int depth) {
  printf("%*s '%s' (%i)\n", depth, "--", get_name().c_str(), retain_count());

  for (std::vector<std::pair<View *, bool> >::const_iterator iter = _subviews.begin(); iter != _subviews.end();
       ++iter) {
    iter->first->show_retain_counts(depth + 1);
  }
}
#endif
//--------------------------------------------------------------------------------------------------

void View::flush_events() {
  if (_view_impl->flush_events)
    _view_impl->flush_events(this);
}

//--------------------------------------------------------------------------------------------------

void View::focus() {
  _view_impl->focus(this);
}

//--------------------------------------------------------------------------------------------------

bool View::has_focus() {
  return _view_impl->has_focus(this);
}

//--------------------------------------------------------------------------------------------------

void View::register_drop_formats(DropDelegate *target, const std::vector<std::string> &drop_formats) {
  _view_impl->register_drop_formats(this, target, drop_formats);
}

//--------------------------------------------------------------------------------------------------

DragOperation View::do_drag_drop(DragDetails details, const std::string &text) {
  return _view_impl->drag_text(this, details, text);
}

//--------------------------------------------------------------------------------------------------

DragOperation View::do_drag_drop(DragDetails details, void *data, const std::string &format) {
  return _view_impl->drag_data(this, details, data, format);
}

//--------------------------------------------------------------------------------------------------

DropPosition View::get_drop_position() {
  return _view_impl->get_drop_position(this);
}

//--------------------------------------------------------------------------------------------------

bool View::mouse_leave() {
  if (_signal_mouse_leave.num_slots() > 0)
    return *_signal_mouse_leave();
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * To be called by platform code when the active control changes (either by code or user interaction).
 */
void View::focus_changed() {
  _signal_got_focus();
  base::NotificationCenter::get()->send("GNFocusChanged", this);
}

//--------------------------------------------------------------------------------------------------

void View::resize() {
  _signal_resized();
}

//--------------------------------------------------------------------------------------------------
