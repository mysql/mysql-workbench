/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/mforms.h"

using namespace mforms;

DrawBox::DrawBox() : _focusedItem(-1), _lastFocusedItem(-1) {
  _drawbox_impl = &ControlFactory::get_instance()->_drawbox_impl;
  _drawbox_impl->create(this);
}

//--------------------------------------------------------------------------------------------------

void DrawBox::add(View *view, Alignment alignment) {
  cache_view(view);
  _drawbox_impl->add(this, view, alignment);
}

//--------------------------------------------------------------------------------------------------

void DrawBox::remove(View *view) {
  _drawbox_impl->remove(this, view);
  remove_from_cache(view);
}

//--------------------------------------------------------------------------------------------------

void DrawBox::move(View *view, int x, int y) {
  _drawbox_impl->move(this, view, x, y);
}

//--------------------------------------------------------------------------------------------------

void DrawBox::set_layout_dirty(bool value) {
  View::set_layout_dirty(value);
  if (value)
    _drawbox_impl->set_needs_repaint(this);
}

//--------------------------------------------------------------------------------------------------

void DrawBox::set_padding(int left, int top, int right, int bottom) {
  _view_impl->set_padding(this, left, top, right, bottom);
}

//--------------------------------------------------------------------------------------------------

void DrawBox::set_needs_repaint() {
  _drawbox_impl->set_needs_repaint(this);
}

//--------------------------------------------------------------------------------------------------

void DrawBox::set_needs_repaint_area(int x, int y, int w, int h) {
  _drawbox_impl->set_needs_repaint_area(this, x, y, w, h);
}

//--------------------------------------------------------------------------------------------------

/**
 * The content of a draw box is, by nature, drawn by the box itself, so we need to know what
 * space the box needs. Overwritten by descendants. Subviews do not automatically add to the content
 * size. If that's needed then additional computations are needed by the host.
 */
base::Size DrawBox::getLayoutSize(base::Size proposedSize) {
  return proposedSize;
}

//--------------------------------------------------------------------------------------------------

void DrawBox::repaint(cairo_t *cr, int x, int y, int w, int h) {
  if (_focusedItem != -1 && _focusedItem < static_cast<int>(_focusableList.size())) {
    drawFocus(cr, _focusableList[_focusedItem].getBounds());
  }
}

//--------------------------------------------------------------------------------------------------

void DrawBox::drawFocus(cairo_t *cr, const base::Rect r) {
  if (_drawbox_impl->drawFocus) {
    _drawbox_impl->drawFocus(this, cr, r);
  }
}

//--------------------------------------------------------------------------------------------------

void DrawBox::addFocusableArea(FocusableArea fArea) {
  if (fArea.getBounds)
  _focusableList.push_back(fArea);
}

//--------------------------------------------------------------------------------------------------

void DrawBox::clearFocusableAreas() {
  _focusedItem = -1;
  _lastFocusedItem = -1;
  _focusableList.clear();
}

//--------------------------------------------------------------------------------------------------

bool DrawBox::setFocusOnArea(const base::Point p) {
  auto it = std::find_if(_focusableList.begin(), _focusableList.end(), [&](mforms::FocusableArea const& item) {
    return item.getBounds().contains(p.x, p.y);
  });
  if (it != _focusableList.end()) {
    _focusedItem = static_cast<int>(it - _focusableList.begin());
    set_needs_repaint();
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool DrawBox::keyPress(KeyCode code, ModifierKey modifiers) {
  int handled = -1;
  if (_focusedItem > -1) {
    if (code == mforms::KeyTab && (modifiers & ModifierShift) == 0) {
      _lastFocusedItem = _focusedItem;
      _focusedItem++;
      if (_focusedItem >= static_cast<int>(_focusableList.size())) {
        _focusedItem = -1;
        handled = 0;
        set_needs_repaint();
      } else {
        handled = 1;
      }
    } else if (code == mforms::KeyTab && (modifiers & ModifierShift) != 0) {
      _lastFocusedItem = _focusedItem;
      _focusedItem--;
      if (_focusedItem < 0) {
        _focusedItem = -1;
        handled = 0;
        set_needs_repaint();
      } else {
        handled = 1;
      }
    } else if (code == mforms::KeyMenu || ((modifiers & ModifierControl) != 0 && code == mforms::KeyF10)) {
      if (_focusableList[_focusedItem].showContextMenu) {
        _focusableList[_focusedItem].showContextMenu();
      }
    } else if (code == mforms::KeyReturn) {
      if (_focusableList[_focusedItem].activate)
        _focusableList[_focusedItem].activate();
    }

    if (handled > -1 && _focusedItem > -1) {
      auto parent = dynamic_cast<mforms::ScrollPanel*>(get_parent());
      if (parent != nullptr) {
        parent->scroll_to(static_cast<int>(_focusableList[_focusedItem].getBounds().pos.x), static_cast<int>(_focusableList[_focusedItem].getBounds().pos.y));
      }
      set_needs_repaint();
    }
  }

  return handled == 1;
}

//--------------------------------------------------------------------------------------------------

bool DrawBox::focusIn() {
  if (!_focusableList.empty() && _focusedItem == -1) {
    _focusedItem = _lastFocusedItem > -1 ? _lastFocusedItem :  0;
    set_needs_repaint();
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool DrawBox::focusOut() {
  if (_focusedItem > -1) {
    _lastFocusedItem = _focusedItem;
    _focusedItem = -1;
    set_needs_repaint();
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool DrawBox::mouse_down(mforms::MouseButton button, int x, int y) {
  if (button == mforms::MouseButtonLeft) {

    auto it = std::find_if(_focusableList.begin(), _focusableList.end(), [&](mforms::FocusableArea const& item) {
      return item.getBounds().contains(x, y);
    });

    if (it != _focusableList.end()) {
      _lastFocusedItem = _focusedItem;
      _focusedItem = static_cast<int>(it - _focusableList.begin());
    } else {
      _focusedItem = -1;
    }
  }
  set_needs_repaint();
  return false;
}
