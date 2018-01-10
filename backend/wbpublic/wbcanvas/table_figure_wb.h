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

#ifndef __TABLE_FIGURE_WB_H__
#define __TABLE_FIGURE_WB_H__

#include "table_figure.h"

namespace wbbridge {
  namespace physical {
    class TableFigure;
  };
};

namespace wbfig {

  class WBPUBLICBACKEND_PUBLIC_FUNC WBTable : public Table {
    typedef Table super;

  public:
    WBTable(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self);
    virtual ~WBTable();

    virtual void set_color(const base::Color &color);
    virtual void set_dependant(bool flag);

    virtual void set_allow_manual_resizing(bool flag);

    virtual ItemList::iterator begin_columns_sync();
    virtual ItemList::iterator sync_next_column(ItemList::iterator iter, const std::string &id, ColumnFlags type,
                                                const std::string &text);
    virtual void end_columns_sync(ItemList::iterator iter);

    virtual ItemList::iterator begin_indexes_sync();
    virtual ItemList::iterator sync_next_index(ItemList::iterator iter, const std::string &id, const std::string &text);
    virtual void end_indexes_sync(ItemList::iterator iter);

    virtual ItemList::iterator begin_triggers_sync();
    virtual ItemList::iterator sync_next_trigger(ItemList::iterator iter, const std::string &id,
                                                 const std::string &text);
    virtual void end_triggers_sync(ItemList::iterator iter);

    virtual Titlebar *get_index_title() {
      return &_index_title;
    }
    virtual Titlebar *get_trigger_title() {
      return &_trigger_title;
    }

    void hide_indices();
    void hide_triggers();

    void hide_columns();
    virtual ItemList *get_columns() {
      return &_columns;
    }
    virtual ItemList *get_indexes() {
      return &_indexes;
    }

    virtual void set_max_columns_shown(int count);

    virtual void set_content_font(const mdc::FontSpec &font);

  protected:
    mdc::Box _content_box;

    ShrinkableBox _column_box;
    ItemList _columns;

    Titlebar _index_title;
    mdc::Box _index_box;
    ItemList _indexes;

    Titlebar _trigger_title;
    mdc::Box _trigger_box;
    ItemList _triggers;

    Titlebar _footer;

    virtual bool get_expanded();
    virtual void toggle(bool flag);

    virtual bool get_indexes_expanded();
    virtual bool get_triggers_expanded();
    virtual void toggle_indexes(bool flag);
    virtual void toggle_triggers(bool flag);

    wbfig::FigureItem *create_truncated_item(mdc::Layer *layer, wbfig::FigureEventHub *hub);
  };
};

#endif
