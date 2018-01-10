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

#pragma once

#include "mdc.h"
#include "grt.h"

#include "grts/structs.model.h"
#include "grts/structs.meta.h"

#include "wbpublic_public_interface.h"

#include "base_bridge.h"
#include "figure_common.h"

class WBPUBLICBACKEND_PUBLIC_FUNC model_Diagram::ImplData : public BridgeBase, public wbfig::FigureEventHub {
  typedef BridgeBase super;

protected:
  model_Diagram *_self;

  mdc::CanvasView *_canvas_view;
  boost::signals2::scoped_connection _selection_signal_conn;

  boost::signals2::signal<void(model_DiagramRef)> _selection_changed_signal;

  boost::signals2::signal<void(model_ObjectRef)> _realize_object_signal;
  boost::signals2::signal<void(model_ObjectRef)> _will_unrealize_object_signal;

  boost::signals2::signal<void(model_ObjectRef, mdc::CanvasItem *, bool, base::Point)> _item_crossed_signal;
  boost::signals2::signal<void(model_ObjectRef, mdc::CanvasItem *, base::Point, mdc::MouseButton, mdc::EventState)>
    _item_click_signal;
  boost::signals2::signal<void(model_ObjectRef, mdc::CanvasItem *, base::Point, mdc::MouseButton, mdc::EventState)>
    _item_double_click_signal;
  boost::signals2::signal<void(model_ObjectRef, mdc::CanvasItem *, bool, base::Point, mdc::MouseButton,
                               mdc::EventState)>
    _item_mouse_button_signal;

  int _updating_selection;
  bool _connected_update;

  virtual ~ImplData();

  void member_changed(const std::string &name, const grt::ValueRef &ovalue);
  virtual void member_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value);

  bool begin_selection_update();
  void end_selection_update();

  void canvas_selection_changed(bool added, mdc::CanvasItem *item);

  void realize_contents();
  void realize_selection();

  void update_options(const std::string &key);

  virtual bool is_realizable();

  virtual GrtObject *get_object() {
    return _self;
  }

  model_LayerRef get_layer_under_figure(const model_FigureRef &figure);

  virtual bool figure_click(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point,
                            mdc::MouseButton button, mdc::EventState state);
  virtual bool figure_double_click(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point,
                                   mdc::MouseButton button, mdc::EventState state);
  virtual bool figure_button_press(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point,
                                   mdc::MouseButton button, mdc::EventState state);
  virtual bool figure_button_release(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point,
                                     mdc::MouseButton button, mdc::EventState state);
  virtual bool figure_enter(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point);
  virtual bool figure_leave(const model_ObjectRef &owner, mdc::CanvasItem *target, const base::Point &point);

public:
  ImplData(model_Diagram *self);

  void set_page_counts(int x, int y);

  void block_updates(bool flag);

  void add_figure(const model_FigureRef &figure);
  virtual void add_connection(const model_ConnectionRef &conn);

  void remove_figure(const model_FigureRef &figure);
  virtual void remove_connection(const model_ConnectionRef &conn);

  void delete_layer(const model_LayerRef &layer);

  bool update_layer_of_figure(const model_FigureRef &figure);

  void select_object(const model_ObjectRef &object);
  void unselect_object(const model_ObjectRef &object);
  void unselect_all();

  virtual bool realize();
  virtual void unrealize();

public:
  mdc::CanvasView *get_canvas_view();
  bool is_canvas_view_valid() {
    return _canvas_view != NULL;
  };

  static base::Size get_size_for_page(const app_PageSettingsRef &page);

  void stack_layer(const model_LayerRef &layer, mdc::CanvasItem *item);
  void stack_connection(const model_ConnectionRef &conn, mdc::CanvasItem *item);
  void stack_figure(const model_FigureRef &figure, mdc::CanvasItem *item);

  boost::signals2::signal<void(model_DiagramRef)> *signal_selection_changed() {
    return &_selection_changed_signal;
  }

  boost::signals2::signal<void(model_ObjectRef, mdc::CanvasItem *, bool, base::Point)> *signal_item_crossed() {
    return &_item_crossed_signal;
  }
  boost::signals2::signal<void(model_ObjectRef, mdc::CanvasItem *, base::Point, mdc::MouseButton, mdc::EventState)>
    *signal_item_click() {
    return &_item_click_signal;
  }
  boost::signals2::signal<void(model_ObjectRef, mdc::CanvasItem *, base::Point, mdc::MouseButton, mdc::EventState)>
    *signal_item_double_click() {
    return &_item_double_click_signal;
  }
  boost::signals2::signal<void(model_ObjectRef, mdc::CanvasItem *, bool, base::Point, mdc::MouseButton,
                               mdc::EventState)>
    *signal_item_mouse_button() {
    return &_item_mouse_button_signal;
  }

  boost::signals2::signal<void(model_ObjectRef)> *signal_object_realized() {
    return &_realize_object_signal;
  }
  void notify_object_realize(const model_ObjectRef &object);

  boost::signals2::signal<void(model_ObjectRef)> *signal_object_will_unrealize() {
    return &_will_unrealize_object_signal;
  }
  void notify_object_will_unrealize(const model_ObjectRef &object);

  void update_size();
  void update_from_page_size();

  void add_tag_badge_to_figure(const model_FigureRef &figure, const meta_TagRef &tag);
  void remove_tag_badge_from_figure(const model_FigureRef &figure, const meta_TagRef &tag);

private:
  model_Diagram *self() const {
    return _self;
  }
};
