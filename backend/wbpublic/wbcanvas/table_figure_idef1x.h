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

#ifndef _TABLE_FIGURE_IDEF1X_H_
#define _TABLE_FIGURE_IDEF1X_H_

#include "table_figure.h"

namespace wbfig {

  class Separator : public mdc::Figure {
  public:
    Separator(mdc::Layer *layer) : mdc::Figure(layer) {
      _top_empty = false;
      _bottom_empty = false;
    }

    virtual void draw_contents(mdc::CairoCtx *cr);

    virtual base::Size calc_min_size();
    void set_top_empty(bool flag);
    void set_bottom_empty(bool flag);

  protected:
    bool _top_empty;
    bool _bottom_empty;
  };

  class Idef1xTable : public Table {
  public:
    Idef1xTable(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self);

    virtual void set_color(const base::Color &color);
    virtual void set_dependant(bool flag);

    virtual ItemList::iterator begin_columns_sync();

    virtual ItemList::iterator sync_next_column(ItemList::iterator iter, const std::string &id, ColumnFlags flags,
                                                const std::string &text);

    virtual void end_columns_sync(ItemList::iterator iter);

    virtual ItemList::iterator begin_indexes_sync() {
      return ItemList::iterator();
    }
    virtual ItemList::iterator sync_next_index(ItemList::iterator iter, const std::string &id,
                                               const std::string &text) {
      return ItemList::iterator();
    }
    virtual void end_indexes_sync(ItemList::iterator iter) {
    }

    virtual ItemList::iterator begin_triggers_sync() {
      return ItemList::iterator();
    }
    virtual ItemList::iterator sync_next_trigger(ItemList::iterator iter, const std::string &id,
                                                 const std::string &text) {
      return ItemList::iterator();
    }
    virtual void end_triggers_sync(ItemList::iterator iter) {
    }

    virtual ItemList *get_columns() {
      return &_columns;
    }

  protected:
    mdc::Box _column_box;
    std::set<std::string> _unique_oids;
    Separator _separator;

    ItemList _columns;

    virtual void end_sync(mdc::Box &box, ItemList &list, ItemList::iterator iter);
  };
};

#endif /* _TABLE_FIGURE_IDEF1X_H_ */
