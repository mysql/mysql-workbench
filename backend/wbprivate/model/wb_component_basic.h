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

#ifndef _WB_COMPONENT_BASIC_H_
#define _WB_COMPONENT_BASIC_H_

// Basic Model Handling (layers etc)

#include "wb_component.h"
#include "mdc_events.h"

namespace wb {

// basic tool names
#define WB_TOOL_SELECT "basic/select"
#define WB_TOOL_HAND "basic/hand"
#define WB_TOOL_DELETE "basic/delete"
#define WB_TOOL_LAYER "basic/layer"
#define WB_TOOL_NOTE "basic/note"
#define WB_TOOL_IMAGE "basic/image"
#define WB_TOOL_ZOOM_IN "basic/zoomin"
#define WB_TOOL_ZOOM_OUT "basic/zoomout"

  class WBComponentBasic : public WBComponent {
  public:
    WBComponentBasic(WBContext *wb);
    virtual ~WBComponentBasic();

    static std::string name() {
      return "basic";
    }
    virtual std::string get_name() {
      return WBComponentBasic::name();
    }
    virtual std::string get_diagram_class_name() {
      return model_Diagram::static_class_name();
    }

    // void delete_selection();

  protected:
    virtual void load_app_options(bool update);

    // Canvas Handling
    virtual void setup_canvas_tool(ModelDiagramForm *view, const std::string &tool);

    // Toolbar Handling
    virtual app_ToolbarRef get_tools_toolbar();
    virtual app_ToolbarRef get_tool_options(const std::string &tool);

    virtual std::vector<std::string> get_command_dropdown_items(const std::string &option);

    virtual grt::ListRef<app_ShortcutItem> get_shortcut_items();

    virtual bool handles_figure(const model_ObjectRef &object);

    virtual void activate_canvas_object(const model_ObjectRef &figure, bool newwindow);

  private:
    std::map<std::string, app_ToolbarRef> _toolbars;
    grt::ListRef<app_ShortcutItem> _shortcuts;

    // Model Objects
    grt::ValueRef place_layer(ModelDiagramForm *view, const base::Rect &rect);

    void delete_object(ModelDiagramForm *view, const base::Point &pos);

    virtual bool delete_model_object(const model_ObjectRef &object, bool figure_only);

    virtual bool can_paste_object(const grt::ObjectRef &object);
    virtual model_ObjectRef paste_object(ModelDiagramForm *view, const grt::ObjectRef &object,
                                         grt::CopyContext &copy_context);
    virtual void copy_object_to_clipboard(const grt::ObjectRef &object, grt::CopyContext &copy_context);

    void reset_tool(ModelDiagramForm *view, void *);
    bool handle_motion_event(ModelDiagramForm *, base::Point, mdc::EventState, void *);
    bool handle_button_event(ModelDiagramForm *, mdc::MouseButton, bool, base::Point, mdc::EventState, void *);
  };
};

#endif
