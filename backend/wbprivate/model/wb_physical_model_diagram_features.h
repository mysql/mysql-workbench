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

#pragma once

#include "mdc.h"
#include "grts/structs.model.h"
#include "grts/structs.workbench.physical.h"

#include "grt/grt_manager.h"

namespace wb {

  class ModelDiagramForm;
  class Tooltip;

  /** Implementation of diagram manipulation goodies (basic stuff is in wb_model_diagram_form)
   */
  class PhysicalModelDiagramFeatures : public base::trackable {
    friend class ModelDiagramForm;

    ModelDiagramForm *_diagram;
    base::Point _last_click_pos;

    mdc::CanvasItem *_last_over_item;
    Tooltip *_tooltip;
    bec::GRTManager::Timer *_tooltip_timer;

    std::string _highlighted_connection_id;
    std::string _highlighted_table_id;

    bool _highlight_all;

    void on_figure_double_click(const model_ObjectRef &owner, mdc::CanvasItem *item, const base::Point &pos,
                                mdc::MouseButton button, mdc::EventState state);
    void on_figure_mouse_button(const model_ObjectRef &owner, mdc::CanvasItem *item, bool press, const base::Point &pos,
                                mdc::MouseButton button, mdc::EventState state);
    void on_figure_crossed(const model_ObjectRef &owner, mdc::CanvasItem *item, bool enter, const base::Point &pos);

    void on_selection_changed();

    void on_figure_will_unrealize(const model_ObjectRef &object);

    void activate_item(const model_ObjectRef &owner, mdc::CanvasItem *item, mdc::EventState state);

    void highlight_connection(const workbench_physical_ConnectionRef &conn, bool flag);
    void highlight_table(const workbench_physical_TableFigureRef &table, bool flag);
    void highlight_table_index(const workbench_physical_TableFigureRef &table, const db_IndexRef &index, bool entered);

    void tooltip_setup(const model_ObjectRef &owner);
    void tooltip_cancel();
    void show_tooltip(const model_ObjectRef &owner, mdc::CanvasItem *item);

    mdc::CanvasView *get_canvas_view();
    bec::GRTManager::Timer *run_every(const std::function<bool()> &slot, double seconds);
    void cancel_timer(bec::GRTManager::Timer *timer);

  public:
    PhysicalModelDiagramFeatures(ModelDiagramForm *diagram);
    virtual ~PhysicalModelDiagramFeatures();

    void highlight_all_connections(bool flag);

    //    virtual bool key_pressed();
  };
};
