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

#include "grtdb/db_object_helpers.h"
#include "model_figure_impl.h"
#include "workbench_physical_connection_impl.h"
#include "workbench_physical_model_impl.h"
#include "workbench_physical_diagram_impl.h"
#include "workbench_physical_tablefigure_impl.h"

#include "model_diagram_impl.h"

using namespace base;

/*

workbench_physical_Connection

Connection objects for physical models have a 1:1 relation to foreign keys (per view),
the same way a table figure is 1:1 with a db table object.

They can be created in one of the following situations:

- a table with a foreign key is added to the view
- a table referenced by a foreign key in another table is added to the view
- a foreign key is added to a table already in the view

In the first case, createConnectionForForeignKey() or createConnectionsForTable()
functions must be called manually. If the ref table is not in the view, the connection
will not be created.

In the second case, createConnectionsForTable() must be called and connections
will be created for all foreign keys from other tables that are in the view too.

In the 3rd case, a listener (WBComponentPhysical) will call createConnectionForForeignKey()
which will create a connection if both tables for the FK are in the view. If the FK
definition is incomplete, nothing will be added.
The same listener will create the connection if the definition becomes valid or remove
it if the FK is deleted.

*/

workbench_physical_Connection::ImplData::ImplData(workbench_physical_Connection *owner) : super(owner) {
  _highlighting = false;

  scoped_connect(owner->signal_changed(),
                 std::bind(&ImplData::member_changed, this, std::placeholders::_1, std::placeholders::_2));
}

workbench_physical_Connection::ImplData::~ImplData() {
}

void workbench_physical_Connection::ImplData::set_in_view(bool flag) {
  model_DiagramRef diagram(self()->owner());

  if (flag) {
    if (!_line && diagram.is_valid() && !_realize_conn.connected())
      _realize_conn = diagram->get_data()->signal_object_realized()->connect(
        std::bind(&ImplData::object_realized, this, std::placeholders::_1));
  } else
    _realize_conn.disconnect();
  model_Connection::ImplData::set_in_view(flag);

  if (flag && !_line && diagram->owner().is_valid() &&
      workbench_physical_ModelRef::cast_from(diagram->owner())->get_data()->get_relationship_notation() ==
        PRFromColumnNotation &&
      !is_realizable()) {
    // force another realize attempt once in the idle loop if the notation is connect to columns
    // because at this time it could be that the target columns are not yet created
    run_later(std::bind(&ImplData::try_realize, this));
  }
}

void workbench_physical_Connection::ImplData::set_foreign_key(const db_ForeignKeyRef &fk) {
  bool owner_valid = self()->owner().is_valid();

  // remove mapping of the old FK
  if (owner_valid && self()->_foreignKey.is_valid())
    workbench_physical_DiagramRef::cast_from(self()->owner())
      ->get_data()
      ->remove_fk_mapping(self()->_foreignKey, self());

  self()->_foreignKey = fk;

  if (owner_valid && fk.is_valid())
    workbench_physical_DiagramRef::cast_from(self()->owner())->get_data()->add_fk_mapping(fk, self());

  update_connected_tables();

  if (!_line && owner_valid && !_realize_conn.connected())
    _realize_conn = model_DiagramRef::cast_from(self()->owner())
                      ->get_data()
                      ->signal_object_realized()
                      ->connect(std::bind(&ImplData::object_realized, this, std::placeholders::_1));

  _fk_member_changed_conn.disconnect();
  _fk_changed_conn.disconnect();

  if (fk.is_valid()) {
    _fk_member_changed_conn = fk->signal_changed()->connect(
      std::bind(&ImplData::fk_member_changed, this, std::placeholders::_1, std::placeholders::_2));

    if (fk->owner().is_valid())
      _fk_changed_conn =
        fk->owner()->signal_foreignKeyChanged()->connect(std::bind(&ImplData::fk_changed, this, std::placeholders::_1));
  }
}

/** Sets startFigure and endFigure according to the tables in the foreignKey
 */
void workbench_physical_Connection::ImplData::update_connected_tables() {
  db_TableRef table;
  db_TableRef reftable;

  if (self()->owner().is_valid()) {
    if (self()->foreignKey().is_valid()) {
      table = db_TableRef::cast_from(self()->foreignKey()->owner());
      reftable = self()->foreignKey()->referencedTable();
    }

    if (table.is_valid() && reftable.is_valid()) {
      workbench_physical_DiagramRef view(workbench_physical_DiagramRef::cast_from(self()->owner()));
      model_FigureRef table_figure;
      model_FigureRef reftable_figure;

      table_figure = view->getFigureForDBObject(table);
      reftable_figure = view->getFigureForDBObject(reftable);

      bool changed = false;
      if (table_figure != self()->startFigure()) {
        self()->startFigure(table_figure);
        changed = true;
      }
      if (reftable_figure != self()->endFigure()) {
        self()->endFigure(reftable_figure);
        changed = true;
      }

      if (changed) {
        unrealize();
        try_realize();
      }
    } else
      unrealize();
  }
}

void workbench_physical_Connection::ImplData::object_realized(const model_ObjectRef &object) {
  if (object.is_instance(workbench_physical_TableFigure::static_class_name())) {
    workbench_physical_TableFigureRef figure(workbench_physical_TableFigureRef::cast_from(object));
    db_TableRef table(figure->table());

    if (self()->foreignKey().is_valid() &&
        (table == self()->foreignKey()->owner() || table == self()->foreignKey()->referencedTable()))
      try_realize();
  }
}

void workbench_physical_Connection::ImplData::member_changed(const std::string &name, const grt::ValueRef &ovalue) {
  if (_line && name == "caption") {
    set_above_caption(self()->_caption);
  } else if (_line && name == "extraCaption") {
    set_below_caption(self()->_extraCaption);
  } else if (name == "captionXOffs") {
    _above_offset.x = self()->_captionXOffs;
    if (_line)
      update_above_caption_pos();
  } else if (name == "captionYOffs") {
    _above_offset.y = self()->_captionYOffs;
    if (_line)
      update_above_caption_pos();
  } else if (name == "extraCaptionXOffs") {
    _below_offset.x = self()->_extraCaptionXOffs;
    if (_line)
      update_below_caption_pos();
  } else if (name == "extraCaptionYOffs") {
    _below_offset.y = self()->_extraCaptionYOffs;
    if (_line)
      update_below_caption_pos();
  } else if (name == "startCaptionXOffs") {
    _start_offset.x = self()->_startCaptionXOffs;
    if (_line)
      update_start_caption_pos();
  } else if (name == "startCaptionYOffs") {
    _start_offset.y = self()->_startCaptionYOffs;
    if (_line)
      update_start_caption_pos();
  } else if (name == "endCaptionXOffs") {
    _end_offset.x = self()->_endCaptionXOffs;
    if (_line)
      update_end_caption_pos();
  } else if (name == "endCaptionYOffs") {
    _end_offset.y = self()->_endCaptionYOffs;
    if (_line)
      update_end_caption_pos();
  } else if (_line && name == "middleSegmentOffset") {
    _line->set_segment_offset(0, self()->_middleSegmentOffset);
  } else if (name == "endFigure" || name == "startFigure") {
    unrealize();
    try_realize();
  }
}

void workbench_physical_Connection::ImplData::fk_changed(const db_ForeignKeyRef &fk) {
  if (self()->foreignKey() == fk && _line)
    update_connected_tables();
}

void workbench_physical_Connection::ImplData::fk_member_changed(const std::string &name, const grt::ValueRef &ovalue) {
  update_line_ends();

  if (name == "owner") {
    _fk_changed_conn.disconnect();
    if (self()->foreignKey()->owner().is_valid())
      _fk_changed_conn = self()->foreignKey()->owner()->signal_foreignKeyChanged()->connect(
        std::bind(&ImplData::fk_changed, this, std::placeholders::_1));
  }
}

void workbench_physical_Connection::ImplData::caption_bounds_changed(const Rect &obounds, mdc::TextFigure *figure) {
  if (!figure->is_dragging())
    return;

  model_Connection::ImplData::caption_bounds_changed(obounds, figure);

  if (figure == _above_caption) {
    self()->_captionXOffs = _above_offset.x;
    self()->_captionYOffs = _above_offset.y;
  } else if (figure == _below_caption) {
    self()->_extraCaptionXOffs = _below_offset.x;
    self()->_extraCaptionYOffs = _below_offset.y;
  } else if (figure == _start_caption) {
    self()->_startCaptionXOffs = _start_offset.x;
    self()->_startCaptionYOffs = _start_offset.y;
  } else if (figure == _end_caption) {
    self()->_endCaptionXOffs = _end_offset.x;
    self()->_endCaptionYOffs = _end_offset.y;
  }
}

void workbench_physical_Connection::ImplData::update_line_ends() {
  workbench_physical_Model::ImplData *model =
    dynamic_cast<workbench_physical_Model::ImplData *>(self()->owner()->owner()->get_data());
  if (model && _line) {
    model->update_relationship_figure(this, *self()->_foreignKey->mandatory() != 0, *self()->_foreignKey->many() != 0,
                                      *self()->_foreignKey->referencedMandatory() != 0, false);
  }
}

void workbench_physical_Connection::ImplData::layout_changed() {
  double offset = _line->get_segment_offset(0);

  if (offset != *self()->_middleSegmentOffset)
    self()->_middleSegmentOffset = offset;
}

void workbench_physical_Connection::ImplData::highlight(const base::Color *color) {
  model_Connection::ImplData::highlight(color);

  if (_above_caption) {
    _above_caption->set_highlighted(true);
    if (color)
      _above_caption->set_highlight_color(color);
  }
  if (_below_caption) {
    _below_caption->set_highlighted(true);
    if (color)
      _below_caption->set_highlight_color(color);
  }
  if (_start_caption) {
    _start_caption->set_highlighted(true);
    if (color)
      _start_caption->set_highlight_color(color);
  }
  if (_end_caption) {
    _end_caption->set_highlighted(true);
    if (color)
      _end_caption->set_highlight_color(color);
  }
}

void workbench_physical_Connection::ImplData::unhighlight() {
  if (_above_caption)
    _above_caption->set_highlighted(false);
  if (_below_caption)
    _below_caption->set_highlighted(false);
  if (_start_caption)
    _start_caption->set_highlighted(false);
  if (_end_caption)
    _end_caption->set_highlighted(false);
  model_Connection::ImplData::unhighlight();
}

static wbfig::FigureItem *get_table_column_with_id(wbfig::Table *table, const std::string &id) {
  wbfig::BaseFigure::ItemList *items = table->get_columns();
  for (wbfig::BaseFigure::ItemList::iterator iter = items->begin(); iter != items->end(); ++iter) {
    if ((*iter)->get_id() == id)
      return *iter;
  }
  return 0;
}

mdc::CanvasItem *workbench_physical_Connection::ImplData::get_start_canvas_item() {
  if (self()->_foreignKey.is_valid()) {
    // get the item corresponding to the FK column
    wbfig::Table *table = dynamic_cast<wbfig::Table *>(super::get_start_canvas_item());
    if (!table) {
      if (super::get_start_canvas_item())
        throw std::logic_error("invalid connection endpoint");
      return 0;
    }
    if (workbench_physical_ModelRef::cast_from(self()->owner()->owner())->get_data()->get_relationship_notation() ==
          PRFromColumnNotation &&
        self()->_foreignKey->columns().count() > 0 && self()->_foreignKey->columns()[0].is_valid())
      return get_table_column_with_id(table, self()->_foreignKey->columns()[0].id());
    return table;
  }
  return 0;
}

mdc::CanvasItem *workbench_physical_Connection::ImplData::get_end_canvas_item() {
  if (self()->_foreignKey.is_valid()) {
    // get the item corresponding to the FK referenced column
    wbfig::Table *table = dynamic_cast<wbfig::Table *>(super::get_end_canvas_item());
    if (!table) {
      if (super::get_end_canvas_item())
        throw std::logic_error("invalid connection endpoint");
      return 0;
    }
    if (workbench_physical_ModelRef::cast_from(self()->owner()->owner())->get_data()->get_relationship_notation() ==
          PRFromColumnNotation &&
        self()->_foreignKey->referencedColumns().count() > 0 && self()->_foreignKey->referencedColumns()[0].is_valid())
      return get_table_column_with_id(table, self()->_foreignKey->referencedColumns()[0].id());
    return table;
  }
  return 0;
}

void workbench_physical_Connection::ImplData::table_changed(const std::string &detail) {
  if (!bec::TableHelper::is_identifying_foreign_key(db_TableRef::cast_from(self()->_foreignKey->owner()),
                                                    self()->_foreignKey))
    _line->set_line_pattern(mdc::Dashed2Pattern);
  else
    _line->set_line_pattern(mdc::SolidPattern);
  _line->set_needs_render();
}

bool workbench_physical_Connection::ImplData::realize() {
  if (_line)
    return true;

  if (!is_realizable())
    return false;

  if (!is_main_thread()) {
    run_later(std::bind(&ImplData::realize, this));
    return true;
  }

  {
    get_canvas_view()->lock();

    mdc::CanvasItem *start_item = get_start_canvas_item();
    mdc::CanvasItem *end_item = get_end_canvas_item();

    _line = new wbfig::Connection(start_item->get_layer(), self()->owner()->get_data(), self());

    if (!bec::TableHelper::is_identifying_foreign_key(db_TableRef::cast_from(self()->_foreignKey->owner()),
                                                      self()->_foreignKey))
      _line->set_line_pattern(mdc::Dashed2Pattern);
    else
      _line->set_line_pattern(mdc::SolidPattern);

    if (workbench_physical_TableFigureRef::cast_from(self()->_startFigure)->table() == self()->_foreignKey->owner()) {
      _table_changed_conn =
        db_TableRef::cast_from(self()->_foreignKey->owner())
          ->signal_refreshDisplay()
          ->connect(std::bind(&workbench_physical_Connection::ImplData::table_changed, this, std::placeholders::_1));
    } else {
      _table_changed_conn = self()->_foreignKey->referencedTable()->signal_refreshDisplay()->connect(
        std::bind(&workbench_physical_Connection::ImplData::table_changed, this, std::placeholders::_1));
    }

    _line->set_start_figure(start_item);
    _line->set_end_figure(end_item);

    _line->set_segment_offset(0, self()->_middleSegmentOffset);
    _line->get_layouter()->update();

    scoped_connect(_line->signal_layout_changed(),
                   std::bind(&workbench_physical_Connection::ImplData::layout_changed, this));

    scoped_connect(_line->get_layouter()->signal_changed(),
                   std::bind(&workbench_physical_Connection::ImplData::layout_changed, this));
    if (workbench_physical_ModelRef::cast_from(self()->owner()->owner())->get_data()->get_relationship_notation() ==
        PRFromColumnNotation) {
      dynamic_cast<wbfig::ConnectionLineLayouter *>(_line->get_layouter())
        ->set_type(wbfig::ConnectionLineLayouter::ZLine);
    }

    start_item->get_layer()->add_item(_line);

    set_above_caption(self()->_caption);
    set_below_caption(self()->_extraCaption);
    update_line_ends();

    get_canvas_view()->unlock();

    _realize_conn.disconnect();

    finish_realize();

    notify_realized();
  }
  return true;
}

void workbench_physical_Connection::ImplData::unrealize() {
  if (_line) {
    notify_will_unrealize();

    if (_highlighting)
      unhighlight();

    _table_changed_conn.disconnect();

    super::unrealize();
  }
}
