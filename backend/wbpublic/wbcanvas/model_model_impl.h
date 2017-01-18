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
#include "grt.h"

#include "grts/structs.model.h"

#include "wbpublic_public_interface.h"

#include "base_bridge.h"

class WBPUBLICBACKEND_PUBLIC_FUNC ModelBridgeDelegate {
public:
  virtual ~ModelBridgeDelegate(){};
  virtual mdc::CanvasView *create_diagram(const model_DiagramRef &mview) = 0;
  virtual void free_canvas_view(mdc::CanvasView *view) = 0;

  virtual cairo_surface_t *fetch_image(const std::string &file) = 0;
  virtual std::string attach_image(const std::string &name) = 0;
  virtual void release_image(const std::string &name) = 0;
};

class WBPUBLICBACKEND_PUBLIC_FUNC model_Model::ImplData : public BridgeBase {
  typedef BridgeBase super;

protected:
  model_Model *_owner;

  ModelBridgeDelegate *_delegate;
  boost::signals2::signal<void(std::string)> _options_changed_signal;
  bool _reset_pending;
  bool _options_signal_installed;

  void member_changed(const std::string &name);

  grt::DictRef get_app_options_dict();

  void option_changed(grt::internal::OwnedDict *dict, bool added, const std::string &option);

  void list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value);

  virtual GrtObject *get_object() {
    return _owner;
  }

public:
  ImplData(model_Model *owner);

  //  void add_diagram(const model_DiagramRef &view);
  void remove_diagram(const model_DiagramRef &view);

  virtual bool realize();
  virtual void unrealize();

  void reset_connections();
  void reset_figures();
  void reset_layers();

  std::string common_color_for_db_object(const grt::ObjectRef &object, const std::string &member);

  void update_object_color_in_all_diagrams(const std::string &color, const std::string &object_member,
                                           const std::string &object_id);

public:
  void set_delegate(ModelBridgeDelegate *delegate) {
    _delegate = delegate;
  }
  ModelBridgeDelegate *get_delegate() {
    return _delegate;
  }

  app_PageSettingsRef get_page_settings();

  std::string get_string_option(const std::string &name, const std::string &defvalue);
  int get_int_option(const std::string &name, int defvalue);

  boost::signals2::signal<void(std::string)> *signal_options_changed() {
    return &_options_changed_signal;
  }
};
