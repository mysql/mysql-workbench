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

#ifndef __TABLE_FIGURE_H__
#define __TABLE_FIGURE_H__

#include "figure_common.h"
#include <set>
#include "wbpublic_public_interface.h"

namespace wbbridge {
  namespace physical {
    class TableFigure;
  };
};

namespace wbfig {

  class ItemMagnet : public mdc::Magnet {
    virtual double constrain_angle(double angle) const;
    virtual void owner_bounds_changed(const base::Rect &obounds);
    virtual void owner_parent_bounds_changed(mdc::CanvasItem *item, const base::Rect &obounds);

  public:
    ItemMagnet(mdc::CanvasItem *owner);
  };

  enum ColumnFlags {
    ColumnPK = (1 << 0),
    ColumnFK = (1 << 1),
    ColumnNotNull = (1 << 2),
    ColumnAutoIncrement = (1 << 3),
    ColumnUnsigned = (1 << 4),

    ColumnListTruncated = (1 << 5)
  };

  class Table;

  class TableColumnItem : public FigureItem {
    ItemMagnet *_magnet;
    ColumnFlags _flags;

    bool check_column_connection(mdc::Connector *connector);

    virtual base::Size calc_min_size();
    virtual void draw_contents(mdc::CairoCtx *cr);

  public:
    mdc::Magnet *get_item_magnet() {
      return _magnet;
    }
    TableColumnItem(mdc::Layer *layer, FigureEventHub *hub, Table *owner);

    void set_column_flags(ColumnFlags flags);
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC Table : public BaseFigure {
    typedef BaseFigure super;

  public:
    Table(mdc::Layer *layer, FigureEventHub *hub, const model_ObjectRef &self, bool collapsible);

    Titlebar *get_title() {
      return &_title;
    }
    virtual Titlebar *get_index_title() {
      return 0;
    }
    virtual Titlebar *get_trigger_title() {
      return 0;
    }

    void set_show_flags(bool flag);

    virtual void set_dependant(bool flag) = 0;

    virtual ItemList *get_columns() = 0;

    virtual ItemList::iterator begin_columns_sync() = 0;
    virtual ItemList::iterator sync_next_column(ItemList::iterator iter, const std::string &id, ColumnFlags type,
                                                const std::string &text) = 0;
    virtual void end_columns_sync(ItemList::iterator iter) = 0;

    virtual ItemList *get_indexes() {
      return 0;
    }

    virtual ItemList::iterator begin_indexes_sync() = 0;
    virtual ItemList::iterator sync_next_index(ItemList::iterator iter, const std::string &id,
                                               const std::string &text) = 0;
    virtual void end_indexes_sync(ItemList::iterator iter) = 0;

    virtual ItemList::iterator begin_triggers_sync() = 0;
    virtual ItemList::iterator sync_next_trigger(ItemList::iterator iter, const std::string &id,
                                                 const std::string &text) = 0;
    virtual void end_triggers_sync(ItemList::iterator iter) = 0;

    virtual void highlight(const base::Color *color = 0) {
      _background.set_highlight_color(color);
      _background.set_highlighted(true);
      set_highlight_color(color);
      set_highlighted(true);
      set_needs_render();
    }

    virtual void unhighlight() {
      _background.set_highlighted(false);
      set_highlighted(false);
      set_needs_render();
    }

    virtual void set_title_font(const mdc::FontSpec &font);
    virtual void set_section_font(const mdc::FontSpec &font);
    virtual void set_content_font(const mdc::FontSpec &font);

    bool columns_hidden() {
      return _hide_columns;
    }
    bool indexes_hidden() {
      return _hide_indexes;
    }
    bool triggers_hidden() {
      return _hide_triggers;
    }

    mdc::BoxSideMagnet *get_sides_magnet() {
      return _sides_magnet;
    }

    virtual void toggle(bool flag) {
    }
    virtual void toggle_indexes(bool flag) {
    }
    virtual void toggle_triggers(bool flag) {
    }

    virtual void set_max_columns_shown(int count) {
    }

  protected:
    mdc::RectangleFigure _background;
    boost::signals2::signal<void(int, bool)> _signal_index_crossed;

    mdc::BoxSideMagnet *_sides_magnet;

    Titlebar _title;
    double _original_column_box_height;

    bool _hide_columns;
    bool _hide_indexes;
    bool _hide_triggers;

    bool _show_flags;

    wbfig::FigureItem *create_column_item(mdc::Layer *layer, wbfig::FigureEventHub *hub);
    void update_column_item(wbfig::FigureItem *item, ColumnFlags flags);

    wbfig::FigureItem *create_index_item(mdc::Layer *layer, wbfig::FigureEventHub *hub);

    bool compare_connection_position(mdc::Connector *a, mdc::Connector *b, mdc::BoxSideMagnet::Side vertical);

    virtual bool get_expanded() {
      return true;
    }

    virtual bool get_indexes_expanded() {
      return true;
    }
    virtual bool get_triggers_expanded() {
      return true;
    }
  };
};

#endif
