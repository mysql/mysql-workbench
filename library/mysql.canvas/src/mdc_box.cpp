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

#include "mdc_box.h"

using namespace mdc;
using namespace base;

Box::Box(Layer *layer, Orientation orient, bool homogeneous) : Layouter(layer) {
  _spacing = 0;
  _orientation = orient;
  _homogeneous = homogeneous;
}

Box::~Box() {
}

void Box::set_spacing(float sp) {
  _spacing = sp;
  set_needs_relayout();
}

void Box::foreach (const std::function<void(CanvasItem *)> &slot) {
  for (ItemList::const_iterator iter = _children.begin(); iter != _children.end();) {
    ItemList::const_iterator next = iter;
    ++next;
    slot(iter->item);
    iter = next;
  }
}

void Box::render(CairoCtx *cr) {
  Layouter::render(cr);

  cr->translate(get_position());
  for (ItemList::const_iterator iter = _children.begin(); iter != _children.end(); ++iter) {
    if (iter->item->get_visible()) {
      cr->save();

      iter->item->render(cr);

      cr->restore();
    }
  }
}

CanvasItem *Box::get_item_at(const Point &pos) {
  Point npos = pos - get_position();

  for (ItemList::reverse_iterator iter = _children.rbegin(); iter != _children.rend(); ++iter) {
    if (iter->item->get_visible() && iter->item->contains_point(npos)) {
      Layouter *litem = dynamic_cast<Layouter *>(iter->item);
      if (litem) {
        CanvasItem *item = litem->get_item_at(npos);
        if (item)
          return item;
      }
      return iter->item;
    }
  }
  return 0;
}

void Box::add(CanvasItem *item, bool expand, bool fill, bool hiddenspace) {
  BoxItem bitem;

  bitem.item = item;
  bitem.expand = expand;
  bitem.fill = fill;
  bitem.hiddenspace = hiddenspace;

  item->set_parent(this);
  _children.push_back(bitem);

  set_needs_relayout();
}

void Box::insert_after(CanvasItem *after, CanvasItem *item, bool expand, bool fill, bool hiddenspace) {
  BoxItem bitem;

  bitem.item = item;
  bitem.expand = expand;
  bitem.fill = fill;
  bitem.hiddenspace = hiddenspace;

  item->set_parent(this);

  bool added = false;

  for (ItemList::iterator iter = _children.begin(); iter != _children.end(); ++iter) {
    if ((*iter).item == after) {
      _children.insert(iter, bitem);
      added = true;
      break;
    }
  }
  if (!added)
    _children.push_back(bitem);

  set_needs_relayout();
}

void Box::insert_before(CanvasItem *before, CanvasItem *item, bool expand, bool fill, bool hiddenspace) {
  BoxItem bitem;

  bitem.item = item;
  bitem.expand = expand;
  bitem.fill = fill;
  bitem.hiddenspace = hiddenspace;

  item->set_parent(this);

  bool added = false;

  for (ItemList::iterator prev = _children.end(), iter = _children.begin(); iter != _children.end();
       prev = iter, ++iter) {
    if ((*iter).item == before) {
      if (prev != _children.end())
        _children.insert(prev, bitem);
      else
        _children.push_front(bitem);
      added = true;
      break;
    }
  }
  if (!added)
    _children.push_back(bitem);

  set_needs_relayout();
}

void Box::remove(CanvasItem *item) {
  for (ItemList::iterator iter = _children.begin(); iter != _children.end(); ++iter) {
    if (iter->item == item) {
      item->set_parent(0);
      _children.erase(iter);
      break;
    }
  }

  set_needs_relayout();
}

Size Box::calc_min_size() {
  Size size;
  int count = 0;

  if (_orientation == Horizontal) {
    double maxwidth = 0;

    for (ItemList::const_iterator iter = _children.begin(); iter != _children.end(); ++iter) {
      Size csize = iter->item->get_fixed_size();
      Size min_size = iter->item->get_min_size();
      if (csize.width < 0)
        csize.width = min_size.width;
      if (csize.height < 0)
        csize.height = min_size.height;

      if (iter->item->get_visible()) {
        if (_homogeneous)
          maxwidth = std::max(maxwidth, csize.width);
        else
          size.width += csize.width;

        size.height = std::max(size.height, csize.height);
        count++;
      } else if (iter->hiddenspace)
        size.height = std::max(size.height, csize.height);
    }

    if (count > 0) {
      if (_homogeneous)
        size.width = maxwidth * count;

      size.width += (count - 1) * _spacing;
    }
  } else {
    double maxheight = 0;

    for (ItemList::const_iterator iter = _children.begin(); iter != _children.end(); ++iter) {
      Size csize = iter->item->get_fixed_size();
      Size min_size = iter->item->get_min_size();
      if (csize.width < 0)
        csize.width = min_size.width;
      if (csize.height < 0)
        csize.height = min_size.height;

      if (iter->item->get_visible()) {
        if (_homogeneous)
          maxheight = std::max(maxheight, csize.height);
        else
          size.height += csize.height;

        size.width = std::max(size.width, csize.width);
        count++;
      } else if (iter->hiddenspace)
        size.width = std::max(size.width, csize.width);
    }

    if (count > 0) {
      if (_homogeneous)
        size.height = maxheight * count;

      size.height += (count - 1) * _spacing;
    }
  }

  size.width += _xpadding * 2;
  size.height += _ypadding * 2;

  return size;
}

void Box::resize_to(const Size &size) {
  double x, y;
  double width, height;
  int num_visible = 0;
  int num_expand = 0;
  Point cpos;
  Size csize;

  Layouter::resize_to(size);

  x = _xpadding;
  y = _ypadding;

  // count how many items we have to layout and how many are expandable
  for (ItemList::const_iterator iter = _children.begin(); iter != _children.end(); ++iter) {
    if (iter->item->get_visible()) {
      num_visible++;
      if (iter->expand)
        num_expand++;
    }
  }

  if (num_visible == 0)
    return;

  cpos.x = x;
  cpos.y = y;
  if (_orientation == Horizontal) {
    width = size.width;
    height = size.height - _ypadding * 2;

    csize.height = std::max(1.0, height);

    if (_homogeneous) {
      double iwidth;

      width = width - _spacing * (num_visible - 1);
      iwidth = width / num_visible;

      for (ItemList::const_iterator iter = _children.begin(); iter != _children.end(); ++iter) {
        if (iter->item->get_visible()) {
          if (num_visible == 1)
            csize.width = width;
          else
            csize.width = iwidth;

          num_visible--;
          width -= iwidth;

          iter->item->set_position(cpos);
          iter->item->resize_to(csize);

          cpos.x += csize.width + _spacing;
        }
      }
    } else {
      double iwidth;

      if (num_expand > 0) {
        // calculate the leftover space for distributing between expandable items
        width = width - get_min_size().width;
        iwidth = width / num_expand;
      } else {
        width = 0;
        iwidth = 0;
      }

      for (ItemList::const_iterator iter = _children.begin(); iter != _children.end(); ++iter) {
        if (iter->item->get_visible()) {
          csize.width = std::max(iter->item->get_fixed_size().width, iter->item->get_min_size().width);

          if (iter->expand) {
            if (iter->fill) {
              if (num_expand == 1)
                csize.width += width;
              else
                csize.width += iwidth;
            }
            num_expand--;
            width -= iwidth;
          }

          iter->item->set_position(cpos);
          iter->item->resize_to(csize);

          cpos.x += csize.width + _spacing;
        }
      }
    }
  } else // Vertical
  {
    width = size.width - _xpadding * 2;
    height = size.height - _ypadding * 2;

    csize.width = std::max(1.0, width);

    if (_homogeneous) {
      double iheight;

      height = height - _spacing * (num_visible - 1);
      iheight = height / num_visible;

      for (ItemList::const_iterator iter = _children.begin(); iter != _children.end(); ++iter) {
        if (iter->item->get_visible()) {
          if (num_visible == 1)
            csize.height = height;
          else
            csize.height = iheight;

          num_visible--;
          height -= iheight;

          iter->item->set_position(cpos);
          iter->item->resize_to(csize);

          cpos.y += csize.height + _spacing;
        }
      }
    } else {
      double iheight;

      if (num_expand > 0) {
        // calculate the leftover space for distributing between expandable items
        height = height - get_min_size().height;
        iheight = height / num_expand;
      } else {
        height = 0;
        iheight = 0;
      }

      for (ItemList::const_iterator iter = _children.begin(); iter != _children.end(); ++iter) {
        if (iter->item->get_visible()) {
          csize.height = std::max(iter->item->get_fixed_size().height, iter->item->get_min_size().height);

          if (iter->expand) {
            if (iter->fill) {
              if (num_expand == 1)
                csize.height += height;
              else
                csize.height += iheight;
            }
            num_expand--;
            height -= iheight;
          }

          iter->item->set_position(cpos);
          iter->item->resize_to(csize);

          cpos.y += csize.height + _spacing;
        }
      }
    }
  }
}
