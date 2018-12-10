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

#ifndef _WB_MODEL_DIAGRAM_FORM_H_
#define _WB_MODEL_DIAGRAM_FORM_H_

#include "base/ui_form.h"
#include "base/notifications.h"

#include "workbench/wb_command_ui.h"
#include "grt/icon_manager.h"
#include "mini_view.h"

#include "mforms/menu.h"
#include "mforms/toolbar.h"
#include "wb_catalog_tree_view.h"
#include "wb_context_model.h"

namespace mforms {
  class ToolBarItem;
  class TreeView;
};

namespace wb {
  class Floater;
  class WBContext;
  class WBComponent;

  enum EditFinishReason { EditCancelled, EditReturnPressed, EditTabPressed, EditShiftTabPressed };

  typedef boost::signals2::signal<void(const std::string &, const grt::ValueRef &)> ChangeSignal;

  class ModelDiagramForm;
  class PhysicalModelDiagramFeatures;
  class LayerTree;

  class InlineEditContext {
    boost::signals2::signal<void(std::string, EditFinishReason)> _signal_edit_finished;

  public:
    virtual ~InlineEditContext() {
    }

    virtual void begin_editing(int x, int y, int width, int height, const std::string &text) = 0;
    virtual void end_editing() = 0;

    virtual void set_font_size(float size) = 0;
    virtual void set_multiline(bool flag) = 0;

    boost::signals2::signal<void(std::string, EditFinishReason)> *signal_edit_finished() {
      return &_signal_edit_finished;
    }
  };

  class UpdateLock;

  class MYSQLWBBACKEND_PUBLIC_FUNC ModelDiagramForm : public bec::UIForm, public base::Observer {
    friend class UpdateLock;

  public:
    ModelDiagramForm(WBComponent *owner, const model_DiagramRef &view);
    virtual ~ModelDiagramForm();

    void attach_canvas_view(mdc::CanvasView *cview);

    virtual bool is_main_form() {
      return true;
    }
    virtual std::string get_form_context_name() const;

    mdc::CanvasView *get_view() {
      return _view;
    }
    model_DiagramRef &get_model_diagram() {
      return _model_diagram;
    }

    grt::DictRef get_model_options() {
      return _model_diagram->owner()->options();
    }
    grt::DictRef get_diagram_options() {
      return _model_diagram->options();
    }
    CatalogTreeView *get_catalog_tree();
    void notify_catalog_tree(const wb::CatalogNodeNotificationType &notify_type, grt::ValueRef value);
    void refill_catalog_tree();

    void set_closed(bool flag);
    bool is_closed();
    virtual void close();

    mdc::CanvasItem *get_leaf_item_at(const base::Point &pos);

    bec::Clipboard *get_clipboard();

    WBContext *get_wb();

    virtual std::string get_title();

    virtual bool can_undo();
    virtual bool can_redo();
    virtual bool can_copy();
    virtual bool can_paste();
    virtual bool can_delete();
    virtual bool can_select_all();

    virtual void undo();
    virtual void redo();
    virtual void cut();
    virtual void copy();
    virtual void paste();
    virtual void delete_selection();
    virtual void select_all();

    void remove_selection(bool deleteSelection = false);

    virtual std::string get_edit_target_name();

    std::string get_diagram_info_text();

    std::vector<std::string> get_accepted_drop_types();

    grt::ListRef<model_Object> get_selection();
    grt::ListRef<model_Object> get_copiable_selection();
    bool has_selection();

    double get_zoom();
    void set_zoom(double zoom);
    void zoom_in();
    void zoom_out();

    void set_button_callback(
      const std::function<bool(ModelDiagramForm *, mdc::MouseButton, bool, base::Point, mdc::EventState)> &cb);
    void set_motion_callback(const std::function<bool(ModelDiagramForm *, base::Point, mdc::EventState)> &cb);
    void set_reset_tool_callback(const std::function<void(ModelDiagramForm *)> &cb);

    std::string get_tool() {
      return _tool;
    }
    void set_tool(std::string tool);
    void reset_tool(bool notify);
    void set_tool_argument(const std::string &option, const std::string &value);
    std::string get_tool_argument(const std::string &option);

    bool is_visible(const model_ObjectRef &object, bool partially);
    void focus_and_make_visible(const model_ObjectRef &object, bool select);

    bool search_and_focus_object(const std::string &text);

    void set_cursor(const std::string &cursor);
    inline const std::string &get_cursor() {
      return _cursor;
    }

    // sidebar
    mforms::TreeView *get_layer_tree();
    MiniView *get_mini_view() {
      return _mini_view;
    }

    void setup_mini_view(mdc::CanvasView *view);
    void update_mini_view_size(int w, int h);
    void setBackgroundColor(base::Color const& color);

    // events
    void handle_mouse_move(int x, int y, mdc::EventState state);
    void handle_mouse_button(mdc::MouseButton button, bool press, int x, int y, mdc::EventState state);
    void handle_mouse_double_click(mdc::MouseButton button, int x, int y, mdc::EventState state);
    void handle_mouse_leave(int x, int y, mdc::EventState state);
    bool handle_key(const mdc::KeyInfo &key, bool press, mdc::EventState state);

    bool current_mouse_position(int &x, int &y);
    bool current_mouse_position(base::Point &pos);

    // drag&drop
    bool accepts_drop(int x, int y, const std::string &type, const std::list<GrtObjectRef> &objects);
    bool accepts_drop(int x, int y, const std::string &type, const std::string &text);

    bool perform_drop(int x, int y, const std::string &type, const std::list<GrtObjectRef> &objects);
    bool perform_drop(int x, int y, const std::string &type, const std::string &text);

    model_LayerRef get_layer_at(const base::Point &pos, base::Point &offset);
    model_LayerRef get_layer_bounding(const base::Rect &rect, base::Point &offset);
    model_ObjectRef get_object_at(const base::Point &pos);

    mdc::Layer *get_floater_layer();
    void add_floater(Floater *floater);

    void enable_panning(bool flag);
    void enable_zoom_click(bool enable, bool zoomin);

    boost::signals2::signal<void(std::string)> *signal_tool_argument_changed() {
      return &_tool_argument_changed;
    }

    WBComponent *get_owner() {
      return _owner;
    }

    bool get_highlight_fks() {
      return _highlight_fks;
    }
    void set_highlight_fks(bool flag);

    // inline editing
    void begin_editing(const base::Rect &rect, const std::string &text, float text_size, bool multiline);
    void stop_editing();
    boost::signals2::signal<void(std::string, EditFinishReason)> *signal_editing_done() {
      return &_signal_editing_done;
    }

    void set_inline_editor_context(InlineEditContext *context);

    virtual mforms::ToolBar *get_toolbar();
    mforms::ToolBar *get_tools_toolbar();
    mforms::ToolBar *get_options_toolbar();
    void update_options_toolbar();

    virtual mforms::MenuBar *get_menubar();
    void revalidate_menu();

  protected:
    struct OldPosition {
      base::Point pos;
      std::string layer_id;
    };
    CatalogTreeView *_catalog_tree;
    mdc::CanvasView *_view;
    mdc::Layer *_main_layer;
    mdc::Layer *_floater_layer;
    mdc::Layer *_badge_layer;
    WBComponent *_owner;
    model_DiagramRef _model_diagram;
    int _current_mouse_x;
    int _current_mouse_y;
    std::string _tool;
    std::string _cursor;
    std::map<std::string, std::string> _tool_args;
    std::vector<WBShortcut> _shortcuts;

    LayerTree *_layer_tree;
    MiniView *_mini_view;

    PhysicalModelDiagramFeatures *_features;
    boost::signals2::connection _idle_node_mark;

    std::map<grt::internal::Value *, OldPosition> _old_positions;
    InlineEditContext *_inline_edit_context;
    boost::signals2::signal<void(std::string, EditFinishReason)> _signal_editing_done;

    double _paste_offset;

    mforms::MenuBar *_menu;
    mforms::ToolBar *_toolbar;
    mforms::ToolBar *_tools_toolbar;
    mforms::ToolBar *_options_toolbar;

    boost::signals2::signal<void(std::string)> _tool_argument_changed;

    std::function<bool(ModelDiagramForm *, mdc::MouseButton, bool, base::Point, mdc::EventState)> _handle_button;
    std::function<bool(ModelDiagramForm *, base::Point, mdc::EventState)> _handle_motion;
    std::function<void(ModelDiagramForm *)> _reset_tool;

    bool _drag_panning;
    bool _space_panning;

    bool _highlight_fks;

    // saved state for tmp panning
    std::string _old_tool;
    std::string _old_cursor;
    std::function<void(ModelDiagramForm *)> _old_reset_tool;
    std::function<bool(ModelDiagramForm *, mdc::MouseButton, bool, base::Point, mdc::EventState)> _old_handle_button;
    std::function<bool(ModelDiagramForm *, base::Point, mdc::EventState)> _old_handle_motion;

    void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);
    void update_toolbar_icons();
    void clipboard_changed();

    bool relocate_figures();

    void begin_selection_drag();
    void end_selection_drag();

    void diagram_changed(grt::internal::OwnedList *, bool, const grt::ValueRef &);

    void mark_catalog_node(grt::ValueRef val, bool mark);

    void selection_changed();

    std::vector<std::string> get_dropdown_items(const std::string &name, const std::string &option,
                                                std::string &selected);
    void select_dropdown_item(const std::string &option, mforms::ToolBarItem *item);
    void toggle_checkbox_item(const std::string &name, const std::string &option, bool state);

    void activate_catalog_tree_item(const grt::ValueRef &value);

  private:
    int _update_count; // If > 0 don't refresh depending structures.
    mforms::Menu _context_menu;

    // Local class.
    class UpdateLock {
    private:
      ModelDiagramForm *_form;

    public:
      UpdateLock(ModelDiagramForm *form) {
        _form = form;
        _form->_update_count++;
      };
      ~UpdateLock();
    };
  };
};

#endif
