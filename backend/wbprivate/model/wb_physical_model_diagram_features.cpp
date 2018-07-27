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

#include "wb_physical_model_diagram_features.h"
#include "wbcanvas/model_diagram_impl.h"
#include "wbcanvas/workbench_physical_connection_impl.h"
#include "wbcanvas/workbench_physical_tablefigure_impl.h"

#include "mforms/form.h"
#include "mforms/popover.h"
#include "mforms/label.h"

#include "wbcanvas/badge_figure.h"

#include "workbench/wb_context.h"

#include "wb_component.h"
#include "model/wb_model_diagram_form.h"

using namespace wb;
using namespace base;

#define TOOLTIP_DELAY 1.0

#define DOUBLE_CLICK_DISTANCE_THRESHOLD 4.0

class wb::Tooltip : public mforms::Popover {
  mforms::Label _label;
  bool _visible;

public:
  Tooltip() : mforms::Popover(nullptr, mforms::PopoverStyleTooltip), _visible(false) {
    set_content(&_label);
  }

  void show(int x, int y) {
    _visible = true;
    mforms::Popover::show(x, y, mforms::StartRight);
  }

  virtual void close() {
    _visible = false;
    mforms::Popover::close();
  }

  bool get_visible() {
    return _visible;
  }

  void set_text(const std::string &text) {
    _label.set_text(text);
  }
};

//--------------------------------------------------------------------------------------------------

void PhysicalModelDiagramFeatures::on_figure_double_click(const model_ObjectRef &owner, mdc::CanvasItem *item,
                                                          const Point &pos, mdc::MouseButton button,
                                                          mdc::EventState state) {
  if (button == mdc::ButtonLeft)
    activate_item(owner, item, state);
}

//--------------------------------------------------------------------------------------------------

void PhysicalModelDiagramFeatures::on_figure_mouse_button(const model_ObjectRef &owner, mdc::CanvasItem *item,
                                                          bool press, const Point &pos, mdc::MouseButton button,
                                                          mdc::EventState state) {
}

void PhysicalModelDiagramFeatures::on_figure_crossed(const model_ObjectRef &owner, mdc::CanvasItem *over, bool enter,
                                                     const Point &pos) {
  if (enter) {
    if (over != _last_over_item) {
      _last_over_item = over;
      if (mforms::Form::main_form()->is_active())
        tooltip_setup(owner);
    }
  } else {
    tooltip_cancel();
    _last_over_item = NULL;
  }

  if (owner.is_instance<workbench_physical_Connection>() && !_highlight_all) {
    workbench_physical_ConnectionRef conn(workbench_physical_ConnectionRef::cast_from(owner));

    highlight_connection(conn, enter);
  }

  if (owner.is_instance<workbench_physical_TableFigure>() && !_highlight_all) {
    workbench_physical_TableFigureRef table(workbench_physical_TableFigureRef::cast_from(owner));
    wbfig::Table *figure = dynamic_cast<wbfig::Table *>(table->get_data()->get_canvas_item());

    if (figure && over == figure->get_title())
      highlight_table(table, enter);
    else {
      // if over index, highlight it and the columns for it
      db_IndexRef index(table->get_data()->get_index_at(over));

      if (index.is_valid())
        highlight_table_index(table, index, enter);
    }
  }
}

void PhysicalModelDiagramFeatures::on_selection_changed() {
}

void PhysicalModelDiagramFeatures::on_figure_will_unrealize(const model_ObjectRef &object) {
  if (object.id() == _highlighted_connection_id)
    highlight_connection(workbench_physical_ConnectionRef::cast_from(object), false);
}

void PhysicalModelDiagramFeatures::activate_item(const model_ObjectRef &owner, mdc::CanvasItem *item,
                                                 mdc::EventState state) {
  (*owner->owner()->signal_objectActivated())(owner, (state & mdc::SControlMask) != 0);
}

PhysicalModelDiagramFeatures::PhysicalModelDiagramFeatures(ModelDiagramForm *diagram) : _diagram(diagram) {
  _last_over_item = 0;
  _tooltip = nullptr;
  _tooltip_timer = 0;
  _highlight_all = false;

  model_Diagram::ImplData *impl = diagram->get_model_diagram()->get_data();

  scoped_connect(impl->signal_selection_changed(),
                 std::bind(&PhysicalModelDiagramFeatures::on_selection_changed, this));

  scoped_connect(impl->signal_item_crossed(),
                 std::bind(&PhysicalModelDiagramFeatures::on_figure_crossed, this, std::placeholders::_1,
                           std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
  scoped_connect(impl->signal_item_double_click(),
                 std::bind(&PhysicalModelDiagramFeatures::on_figure_double_click, this, std::placeholders::_1,
                           std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
  scoped_connect(
    impl->signal_item_mouse_button(),
    std::bind(&PhysicalModelDiagramFeatures::on_figure_mouse_button, this, std::placeholders::_1, std::placeholders::_2,
              std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
  scoped_connect(impl->signal_object_will_unrealize(),
                 std::bind(&PhysicalModelDiagramFeatures::on_figure_will_unrealize, this, std::placeholders::_1));

  scoped_connect(mforms::Form::main_form()->signal_deactivated(),
                 std::bind(&PhysicalModelDiagramFeatures::tooltip_cancel, this));
}

PhysicalModelDiagramFeatures::~PhysicalModelDiagramFeatures() {
  tooltip_cancel();
}

// Table highlighting

void PhysicalModelDiagramFeatures::highlight_table(const workbench_physical_TableFigureRef &table, bool flag) {
  Color tocolor(0.0, 0.8, 0.0, 0.4);
  Color fromcolor(0.0, 0.6, 1.0, 0.4);

  if (flag && !_highlight_all)
    table->get_data()->highlight();
  else
    table->get_data()->unhighlight();

  // find all connections that start or end at this table
  grt::ListRef<model_Connection> connections(_diagram->get_model_diagram()->connections());

  for (grt::ListRef<model_Connection>::const_iterator conn = connections.begin(); conn != connections.end(); ++conn) {
    db_ForeignKeyRef fk(workbench_physical_ConnectionRef::cast_from(*conn)->foreignKey());

    if (!fk.is_valid())
      continue;

    if ((*conn)->startFigure() == table) {
      workbench_physical_TableFigure::ImplData *dtable =
        !(*conn)->endFigure().is_valid()
          ? 0
          : workbench_physical_TableFigureRef::cast_from((*conn)->endFigure())->get_data();

      if (dtable) {
        size_t count = fk->referencedColumns().count();
        for (size_t i = 0; i < count; i++) {
          if (flag)
            dtable->set_column_highlighted(fk->referencedColumns()[i], &tocolor);
          else
            dtable->set_column_unhighlighted(fk->referencedColumns()[i]);
        }
        if (flag)
          dtable->highlight(&tocolor);
        else
          dtable->unhighlight();
      }

      size_t count = fk->columns().count();
      for (size_t i = 0; i < count; i++) {
        if (flag)
          table->get_data()->set_column_highlighted(fk->columns()[i], &tocolor);
        else
          table->get_data()->set_column_unhighlighted(fk->columns()[i]);
      }
      if (flag)
        (*conn)->get_data()->highlight(&tocolor);
      else
        (*conn)->get_data()->unhighlight();
    } else if ((*conn)->endFigure() == table) {
      workbench_physical_TableFigure::ImplData *stable =
        !(*conn)->startFigure().is_valid()
          ? 0
          : workbench_physical_TableFigureRef::cast_from((*conn)->startFigure())->get_data();

      if (stable) {
        size_t count = fk->columns().count();
        for (size_t i = 0; i < count; i++) {
          if (flag)
            stable->set_column_highlighted(fk->columns()[i], &fromcolor);
          else
            stable->set_column_unhighlighted(fk->columns()[i]);
        }

        count = fk->referencedColumns().count();
        for (size_t i = 0; i < count; i++) {
          if (flag)
            table->get_data()->set_column_highlighted(fk->referencedColumns()[i], &fromcolor);
          else
            table->get_data()->set_column_unhighlighted(fk->referencedColumns()[i]);
        }

        if (flag && !_highlight_all)
          stable->highlight(&fromcolor);
        else
          stable->unhighlight();
      }
      if (flag)
        (*conn)->get_data()->highlight(&fromcolor);
      else
        (*conn)->get_data()->unhighlight();
    }
  }
}

// Table Index Highlighting

void PhysicalModelDiagramFeatures::highlight_table_index(const workbench_physical_TableFigureRef &table,
                                                         const db_IndexRef &index, bool entered) {
  wbfig::Table *figure = dynamic_cast<wbfig::Table *>(table->get_data()->get_canvas_item());

  if (!figure)
    return;

  size_t index_i = table->table()->indices().get_index(index);

  // highlight columns that are in the index + the index itself
  if (index_i != grt::BaseListRef::npos) {
    wbfig::Table::ItemList *indexes = figure->get_indexes();
    wbfig::Table::ItemList *columns = figure->get_columns();
    if (indexes && columns) {
      wbfig::Table::ItemList::iterator iter;
      iter = indexes->begin();
      ssize_t i = index_i;
      while (iter != indexes->end() && --i >= 0)
        ++iter;
      if (iter != indexes->end()) {
        // highlight the index
        (*iter)->set_highlight_color(0);
        (*iter)->set_highlighted(entered);
      }

      // highlight columns in the index
      for (wbfig::Table::ItemList::const_iterator iter = columns->begin(); iter != columns->end(); ++iter) {
        std::string column_id = (*iter)->get_id();
        bool ok = false;

        // lookup the index for matching columns
        for (grt::ListRef<db_IndexColumn>::const_iterator ic = index->columns().begin(); ic != index->columns().end();
             ++ic) {
          if ((*ic)->referencedColumn().is_valid() && (*ic)->referencedColumn().id() == column_id) {
            ok = true;
            break;
          }
        }
        if (ok)
          (*iter)->set_highlighted(entered);
      }
    }
  }
}

// Connection Highlighting

void PhysicalModelDiagramFeatures::highlight_connection(const workbench_physical_ConnectionRef &conn, bool flag) {
  workbench_physical_TableFigure::ImplData *stable =
    !conn->startFigure().is_valid() ? 0 : workbench_physical_TableFigureRef::cast_from(conn->startFigure())->get_data();
  workbench_physical_TableFigure::ImplData *dtable =
    !conn->endFigure().is_valid() ? 0 : workbench_physical_TableFigureRef::cast_from(conn->endFigure())->get_data();

  if (flag) {
    base::Color color(_diagram->get_view()->get_highlight_color());
    conn->get_data()->highlight(&color);
    _highlighted_connection_id = conn.id();
  } else {
    conn->get_data()->unhighlight();
    _highlighted_connection_id.clear();
  }

  if (stable) {
    size_t count = conn->foreignKey().is_valid() ? conn->foreignKey()->columns().count() : 0;
    for (size_t i = 0; i < count; i++) {
      if (flag)
        stable->set_column_highlighted(conn->foreignKey()->columns()[i]);
      else
        stable->set_column_unhighlighted(conn->foreignKey()->columns()[i]);
    }
  }
  if (dtable) {
    size_t count = (conn->foreignKey().is_valid()) ? conn->foreignKey()->referencedColumns().count() : 0;
    for (size_t i = 0; i < count; i++) {
      if (flag)
        dtable->set_column_highlighted(conn->foreignKey()->referencedColumns()[i]);
      else
        dtable->set_column_unhighlighted(conn->foreignKey()->referencedColumns()[i]);
    }
  }
}

void PhysicalModelDiagramFeatures::highlight_all_connections(bool flag) {
  model_DiagramRef diagram(_diagram->get_model_diagram());

  _highlight_all = flag;
  for (size_t c = diagram->figures().count(), i = 0; i < c; i++) {
    if (workbench_physical_TableFigureRef::can_wrap(diagram->figures()[i]))
      highlight_table(workbench_physical_TableFigureRef::cast_from(diagram->figures()[i]), flag);
  }
}

// Tooltips

void PhysicalModelDiagramFeatures::tooltip_setup(const model_ObjectRef &object) {
  if (_tooltip_timer) {
    cancel_timer(_tooltip_timer);
    _tooltip_timer = 0;
  }

  if (_tooltip && _tooltip->get_visible()) {
    _tooltip->close();
  }

  Point position;
  if (_diagram->current_mouse_position(position)) {
    if (_tooltip && _tooltip->get_visible())
      show_tooltip(object, _last_over_item);
    else {
      if (object.is_valid()) {
        std::function<void()> f = std::bind(&PhysicalModelDiagramFeatures::show_tooltip, this, object, _last_over_item);
        _tooltip_timer = run_every(std::bind(&base::run_and_return_value<bool>, f), TOOLTIP_DELAY);
      }
    }
  }
}

void PhysicalModelDiagramFeatures::tooltip_cancel() {
  if (_tooltip_timer) {
    cancel_timer(_tooltip_timer);
    _tooltip_timer = 0;
  }

  if (_tooltip && _tooltip->get_visible()) {
    _tooltip->close();
  }
}

void PhysicalModelDiagramFeatures::show_tooltip(const model_ObjectRef &object, mdc::CanvasItem *item) {
  if (object.is_valid()) {
    if (_tooltip || _tooltip_timer)
      tooltip_cancel();

    std::string text;
    WBComponent *compo = _diagram->get_owner()->get_wb()->get_component_handling(object);
    if (compo)
      text = compo->get_object_tooltip(object, item);

    if (!text.empty()) {
      if (text[text.size() - 1] == '\n')
        text = text.substr(0, text.size() - 1);

      if (_tooltip == nullptr) {
        _tooltip = new Tooltip();
      }

      _tooltip->set_text(text);

      _tooltip->show(-1, -1);
    }
  }
}

//

mdc::CanvasView *PhysicalModelDiagramFeatures::get_canvas_view() {
  return _diagram->get_view();
}

bec::GRTManager::Timer *PhysicalModelDiagramFeatures::run_every(const std::function<bool()> &slot, double seconds) {
  return bec::GRTManager::get()->run_every(slot, seconds);
}

void PhysicalModelDiagramFeatures::cancel_timer(bec::GRTManager::Timer *timer) {
  bec::GRTManager::get()->cancel_timer(timer);
}
