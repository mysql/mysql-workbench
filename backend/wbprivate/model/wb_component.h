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

#include "workbench/wb_backend_public_interface.h"
#include "base/trackable.h"
#include "grt.h"
#include "grtpp_util.h"
#include "grts/structs.model.h"
#include "base/geometry.h"

namespace mdc {
  class CanvasItem;
};

namespace wb {
  class WBContext;
  struct WBOptions;

  class ModelDiagramForm;

  class WBComponent : public base::trackable {
  public:
    WBComponent(WBContext *context);
    virtual ~WBComponent(){};

    inline WBContext *get_wb() {
      return _wb;
    }

    virtual std::string get_name() = 0;
    virtual std::string get_diagram_class_name() {
      return "";
    }

    virtual void setup_context_grt(WBOptions *options) {
    }
    virtual void load_app_options(bool update) {
    }
    virtual void save_app_options() {
    }

    virtual void close_document() {
    }
    virtual void reset_document() {
    }
    virtual void document_loaded() {
    }
    virtual void block_model_notifications() {
    }
    virtual void unblock_model_notifications() {
    }

    virtual bool handles_figure(const model_ObjectRef &figure) = 0;
    virtual GrtObjectRef get_object_for_figure(const model_ObjectRef &figure) {
      return GrtObjectRef();
    }

    virtual bool can_paste_object(const grt::ObjectRef &object) {
      return false;
    }
    virtual model_ObjectRef paste_object(ModelDiagramForm *view, const grt::ObjectRef &object,
                                         grt::CopyContext &copy_context) {
      throw std::logic_error("not implemented");
      return model_ObjectRef();
    }
    virtual void copy_object_to_clipboard(const grt::ObjectRef &object, grt::CopyContext &copy_context) {
      throw std::logic_error("not implemented");
    }

    // toolbar/menubar handling
    virtual grt::ListRef<app_ShortcutItem> get_shortcut_items() {
      return grt::ListRef<app_ShortcutItem>();
    }
    virtual app_ToolbarRef get_tools_toolbar() {
      return app_ToolbarRef();
    }
    virtual app_ToolbarRef get_tool_options(const std::string &tool) {
      return app_ToolbarRef();
    }

    virtual std::vector<std::string> get_command_dropdown_items(const std::string &option) {
      return std::vector<std::string>();
    }

    virtual std::string get_command_option_value(const std::string &option);
    virtual void set_command_option_value(const std::string &option, const std::string &item);

    virtual std::string get_object_tooltip(const model_ObjectRef &object, mdc::CanvasItem *item) {
      return "";
    }

    // tool handling
    virtual void setup_canvas_tool(ModelDiagramForm *view, const std::string &tool) = 0;

    virtual bool delete_model_object(const model_ObjectRef &object, bool figure_only) = 0;

    // drag&
    virtual bool accepts_drop(ModelDiagramForm *view, int x, int y, const std::string &type,
                              const std::list<GrtObjectRef> &objects) {
      return false;
    }
    virtual bool accepts_drop(ModelDiagramForm *view, int x, int y, const std::string &type, const std::string &text) {
      return false;
    }

    virtual bool perform_drop(ModelDiagramForm *view, int x, int y, const std::string &type,
                              const std::list<GrtObjectRef> &objects) {
      return false;
    }
    virtual bool perform_drop(ModelDiagramForm *view, int x, int y, const std::string &type, const std::string &text) {
      return false;
    }

  protected:
    grt::ValueRef place_object(ModelDiagramForm *view, const base::Point &pos, const std::string &object_struct,
                               const grt::DictRef &args = grt::DictRef());

  public:
    virtual void activate_canvas_object(const model_ObjectRef &figure, bool newwindow) = 0;

  protected:
    WBContext *_wb;
  };
};
