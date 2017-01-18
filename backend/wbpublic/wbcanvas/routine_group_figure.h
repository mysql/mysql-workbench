/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef __ROUTINE_GROUP_FIGURE_H__
#define __ROUTINE_GROUP_FIGURE_H__

#include "figure_common.h"
#include "wbpublic_public_interface.h"

namespace wbbridge {
  namespace physical {
    class RoutinesFigure;
  };
};

namespace wbfig {

  class WBPUBLICBACKEND_PUBLIC_FUNC RoutineGroup : public BaseFigure {
    typedef BaseFigure super;

    friend class wbbridge::physical::RoutinesFigure;

    Titlebar _title;
    Titlebar _footer;
    mdc::Box _content_box;

    ItemList _routines;

  public:
    RoutineGroup(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self);
    virtual ~RoutineGroup();

    Titlebar *get_title() {
      return &_title;
    }

    virtual void set_title_font(const mdc::FontSpec &font);
    virtual void set_content_font(const mdc::FontSpec &font);

    virtual void set_color(const base::Color &color);
    void set_title(const std::string &title, const std::string &subtitle);

    virtual void toggle(bool flag);

    ItemList::iterator begin_routines_sync();
    ItemList::iterator sync_next_routine(ItemList::iterator iter, const std::string &id, const std::string &text);
    void end_routines_sync(ItemList::iterator iter);
  };
};

#endif
