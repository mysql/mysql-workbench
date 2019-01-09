/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_model_diagram_form.h"
#include "workbench/wb_context.h"
#include "wb_component.h"
#include "wb_component_basic.h"
#include "canvas_floater.h"

#include "mdc_back_layer.h"
#include "mforms/form.h"

#include "model/wb_context_model.h"
#include "model/wb_layer_tree.h"

#include "grt/clipboard.h"

#include "wbcanvas/model_figure_impl.h"
#include "wbcanvas/model_layer_impl.h"
#include "wbcanvas/model_diagram_impl.h"
#include "wbcanvas/model_connection_impl.h"

#include "workbench/wb_context_ui.h"

#include "wb_physical_model_diagram_features.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/log.h"

#include "mforms/menubar.h"

#define DEFAULT_TOOL "basic/select"

DEFAULT_LOG_DOMAIN("ModelDiagram");

using namespace wb;
using namespace bec;
using namespace base;

static const double zoom_steps[] = { 2.0, 1.5, 1.2, 1.0, 0.95, 0.9, 0.85, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1 };

//----------------------------------------------------------------------------------------------------------------------

ModelDiagramForm::ModelDiagramForm(WBComponent *owner, const model_DiagramRef &view)
  : _catalog_tree(NULL),
    _view(NULL),
    _owner(owner),
    _model_diagram(view),
    _mini_view(NULL),
    _menu(NULL),
    _toolbar(NULL),
    _tools_toolbar(NULL),
    _options_toolbar(NULL) {
  _drag_panning = false;
  _space_panning = false;
  _highlight_fks = false;
  _inline_edit_context = 0;
  _update_count = 0;
  _layer_tree = 0;

  scoped_connect(_model_diagram->signal_list_changed(),
                 std::bind(&ModelDiagramForm::diagram_changed, this, std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3));

  _current_mouse_x = -1;
  _current_mouse_y = -1;

  _main_layer = 0;
  _floater_layer = 0;
  _badge_layer = 0;

  _tool = DEFAULT_TOOL;

  _paste_offset = 0;
  _shortcuts = WBContextUI::get()->get_command_ui()->get_shortcuts_for_context(WB_CONTEXT_MODEL);

  scoped_connect(owner->get_wb()->get_clipboard()->signal_changed(),
                 std::bind(&ModelDiagramForm::clipboard_changed, this));

  _features = new PhysicalModelDiagramFeatures(this);

  _options_toolbar = new mforms::ToolBar(mforms::OptionsToolBar);
  NotificationCenter::get()->add_observer(this, "GNColorsChanged");
  NotificationCenter::get()->add_observer(this, "GNMainFormChanged");
}

//----------------------------------------------------------------------------------------------------------------------

ModelDiagramForm::~ModelDiagramForm() {
  NotificationCenter::get()->remove_observer(this);
  _idle_node_mark.disconnect();

  delete _layer_tree;
  delete _options_toolbar;
  delete _toolbar;
  delete _tools_toolbar;
  delete _menu;
  delete _mini_view;
  delete _features;
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  if (name == "GNColorsChanged") {
    // Single colors or the entire color scheme changed.
    update_toolbar_icons();
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string ModelDiagramForm::get_form_context_name() const {
  return WB_CONTEXT_MODEL;
}

//----------------------------------------------------------------------------------------------------------------------

mforms::ToolBar* ModelDiagramForm::get_toolbar() {
  if (!_toolbar) {
    _toolbar = WBContextUI::get()->get_command_ui()->create_toolbar("data/model_diagram_toolbar.xml");
    update_toolbar_icons();
  }
  return _toolbar;
}

//----------------------------------------------------------------------------------------------------------------------

// Implemented in wb_sql_editor_form_ui.cpp.
extern std::string find_icon_name(std::string icon_name, bool use_win8);

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::update_toolbar_icons() {
  if (_toolbar == NULL)
    return; // Can happen if the diagram hasn't shown yet.

  bool use_win8;

  switch (base::Color::get_active_scheme()) {
    case base::ColorSchemeStandardWin8:
    case base::ColorSchemeStandardWin8Alternate:
      use_win8 = true;
      break;

    default:
      use_win8 = false;
  }

  mforms::ToolBarItem *item = _toolbar->find_item("wb.toggleSidebar");
  if (item != NULL) {
    item->set_icon(find_icon_name(item->get_icon(), use_win8));
    item->set_alt_icon(find_icon_name(item->get_alt_icon(), use_win8));
  }

  item = _toolbar->find_item("wb.toggleSecondarySidebar");
  if (item != NULL) {
    item->set_icon(find_icon_name(item->get_icon(), use_win8));
    item->set_alt_icon(find_icon_name(item->get_alt_icon(), use_win8));
  }
}

//----------------------------------------------------------------------------------------------------------------------

mforms::TreeView *ModelDiagramForm::get_layer_tree() {
  if (!_layer_tree) {
    _layer_tree = new LayerTree(this, _model_diagram);
    _layer_tree->refresh();
  }
  return _layer_tree;
}

//----------------------------------------------------------------------------------------------------------------------

mforms::ToolBar *ModelDiagramForm::get_tools_toolbar() {
  if (!_tools_toolbar) {
    _tools_toolbar = new mforms::ToolBar(mforms::ToolPickerToolBar);
    app_ToolbarRef toolbar[3];

    toolbar[0] = app_ToolbarRef::cast_from(
      grt::GRT::get()->unserialize(base::makePath(get_wb()->get_datadir(), "data/tools_toolbar.xml")));
    toolbar[1] = get_wb()->get_component_named("basic")->get_tools_toolbar();
    toolbar[2] = get_wb()->get_component_named("physical")->get_tools_toolbar();

    for (int t = 0; t < 3; t++) {
      for (size_t c = toolbar[t]->items().count(), i = 0; i < c; i++) {
        app_ToolbarItemRef titem(toolbar[t]->items()[i]);
        mforms::ToolBarItem *item;

        if (titem->itemType() == "separator")
          item = mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem));
        else {
          item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));

          std::string iconName = *titem->icon();
          std::string darkIcon = *titem->darkIcon();
          if (mforms::App::get()->isDarkModeActive() && !darkIcon.empty()) {
            item->set_icon(IconManager::get_instance()->get_icon_path(darkIcon));
          } else {
            item->set_icon(IconManager::get_instance()->get_icon_path(iconName));
          }

          item->set_name(*titem->accessibilityName());
          item->setInternalName(base::split(*titem->command(), ":").back());
          scoped_connect(item->signal_activated(), std::bind(&ModelDiagramForm::set_tool, this, item->getInternalName()));
        }
        std::string shortcut;
        for (std::vector<WBShortcut>::const_iterator iter = _shortcuts.begin(); iter != _shortcuts.end(); ++iter) {
          if (iter->command == *titem->command()) {
            shortcut = iter->shortcut;
            break;
          }
        }
        if (shortcut.empty())
          item->set_tooltip(titem->tooltip());
        else
          item->set_tooltip(strfmt("%s (Quick key - press %s)", titem->tooltip().c_str(), shortcut.c_str()));

        _tools_toolbar->add_item(item);

        if (item->getInternalName() == DEFAULT_TOOL)
          item->set_checked(true);
      }
      toolbar[t]->reset_references();
    }
  }
  return _tools_toolbar;
}

//----------------------------------------------------------------------------------------------------------------------

mforms::ToolBar* ModelDiagramForm::get_options_toolbar() {
  update_options_toolbar();
  return _options_toolbar;
}

//----------------------------------------------------------------------------------------------------------------------

mforms::MenuBar *ModelDiagramForm::get_menubar() {
  if (!_menu) {
    _menu = WBContextUI::get()->get_command_ui()->create_menubar_for_context(WB_CONTEXT_MODEL);
    scoped_connect(_menu->signal_will_show(), std::bind(&ModelDiagramForm::revalidate_menu, this));

    mforms::MenuItem *item = _menu->find_item("wb.edit.editSelectedFigure");
    if (item)
      item->add_validator(std::bind(&ModelDiagramForm::has_selection, this));
    item = _menu->find_item("wb.edit.editSelectedFigureInNewWindow");
    if (item)
      item->add_validator(std::bind(&ModelDiagramForm::has_selection, this));
  }
  revalidate_menu();
  return _menu;
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::revalidate_menu() {
  static const char *figure_notations[] = {"workbench/default", "workbench/simple", "workbench/pkonly",
                                           "classic",           "idef1x",           NULL};
  static const char *relationship_notations[] = {"crowsfoot", "classic", "fromcolumn", "uml", "idef1x", NULL};
  if (_menu) {
    bool has_selection_ = has_selection();

    _menu->set_item_enabled("wb.edit.goToNextSelected", has_selection_);
    _menu->set_item_enabled("wb.edit.goToPreviousSelected", has_selection_);
    _menu->set_item_enabled("wb.edit.selectSimilar", has_selection_);
    _menu->set_item_enabled("wb.edit.selectConnected", get_selection().count() == 1);

    _menu->set_item_checked("wb.edit.toggleGridAlign",
                            bec::GRTManager::get()->get_app_option_int("AlignToGrid", 0) != 0);
    _menu->set_item_checked("wb.edit.toggleGrid", get_diagram_options().get_int("ShowGrid", 1) != 0);
    _menu->set_item_checked("wb.edit.togglePageGrid", get_diagram_options().get_int("ShowPageGrid", 1) != 0);
    _menu->set_item_checked("wb.edit.toggleFKHighlight", get_diagram_options().get_int("ShowFKHighlight", 0) != 0);
    std::string notation = workbench_physical_ModelRef::cast_from(get_model_diagram()->owner())->figureNotation();
    for (int i = 0; figure_notations[i]; i++)
      _menu->set_item_checked(strfmt("wb.view.setFigureNotation:%s", figure_notations[i]),
                              notation == figure_notations[i]);

    notation = workbench_physical_ModelRef::cast_from(get_model_diagram()->owner())->connectionNotation();
    for (int i = 0; relationship_notations[i]; i++)
      _menu->set_item_checked(strfmt("wb.view.setRelationshipNotation:%s", relationship_notations[i]),
                              notation == relationship_notations[i]);

    model_ModelRef model(get_model_diagram()->owner());
    for (int i = 1; i < 10; i++) {
      _menu->set_item_checked(strfmt("wb.view.setMarker:%i", i), false);
      _menu->set_item_enabled(strfmt("wb.view.goToMarker:%i", i), false);
    }

    for (size_t c = model->markers().count(), i = 0; i < c; i++) {
      _menu->set_item_checked(strfmt("wb.view.setMarker:%s", model->markers().get(i)->name().c_str()), true);
      _menu->set_item_enabled(strfmt("wb.view.goToMarker:%s", model->markers().get(i)->name().c_str()), true);
    }

    _menu->find_item("plugins")->validate();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::update_options_toolbar() {
  app_ToolbarRef toolbar = get_wb()->get_component_named("basic")->get_tool_options(get_tool());

  _options_toolbar->remove_all();

  if (!toolbar.is_valid())
    toolbar = get_wb()->get_component_named("physical")->get_tool_options(get_tool());
  if (!toolbar.is_valid())
    return;

  for (size_t c = toolbar->items().count(), i = 0; i < c; i++) {
    app_ToolbarItemRef titem(toolbar->items()[i]);
    mforms::ToolBarItem *item;

    std::string type = titem->itemType();
    if (type == "label") {
      item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
      item->set_text(*titem->command());
    } else if (type == "dropdown") {
      std::string selected;
      std::vector<std::string> items(get_dropdown_items(titem->name(), titem->command(), selected));
      if (!items.empty() && !items.front().empty() && items.front()[0] == '#')
        item = mforms::manage(new mforms::ToolBarItem(mforms::ColorSelectorItem));
      else
        item = mforms::manage(new mforms::ToolBarItem(mforms::SelectorItem));
      item->set_selector_items(items);
      item->set_text(selected);
      scoped_connect(item->signal_activated(),
                     std::bind(&ModelDiagramForm::select_dropdown_item, this, titem->command(), item));
    } else if (type == "separator")
      item = mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem));
    else if (type == "action") {
      item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    } else if (type == "check") {
      item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
      // icon text is used as label for check items
      item->set_text(IconManager::get_instance()->get_icon_path(*titem->icon()));
    } else if (type == "toggle") {
      item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
    } else
      continue;

    if (!titem->icon().empty())
      item->set_icon(IconManager::get_instance()->get_icon_path(*titem->icon()));
    if (!titem->altIcon().empty())
      item->set_alt_icon(IconManager::get_instance()->get_icon_path(*titem->altIcon()));

    item->set_name(titem->accessibilityName());
    item->setInternalName(titem->name());
    item->set_tooltip(titem->tooltip());

    _options_toolbar->add_item(item);
  }

  // don't reset refs here, only do it when closing, since the toolbar object will be used later
  // toolbar->reset_references();
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> ModelDiagramForm::get_dropdown_items(const std::string &name, const std::string &option,
                                                              std::string &selected) {
  std::vector<std::string> items;
  WBComponent *compo;

  compo = get_wb()->get_component_named(base::split(name, "/")[0]);
  if (compo) {
    std::string::size_type p = option.find(':');
    if (p != std::string::npos) {
      std::string option_name = option.substr(p + 1);
      items = compo->get_command_dropdown_items(option_name);
      selected = compo->get_command_option_value(option_name);
    }
  }
  return items;
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::select_dropdown_item(const std::string &option, mforms::ToolBarItem *item) {
  WBComponent *compo;

  compo = get_wb()->get_component_named(base::split(item->getInternalName(), "/")[0]);
  if (compo) {
    std::string::size_type p = option.find(':');
    if (p != std::string::npos) {
      std::string option_name = option.substr(p + 1);
      compo->set_command_option_value(option_name, item->get_text());
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::toggle_checkbox_item(const std::string &name, const std::string &option, bool state) {
  WBComponent *compo;

  compo = get_wb()->get_component_named(base::split(name, "/")[0]);
  if (compo) {
    std::string::size_type p = option.find(':');
    if (p != std::string::npos) {
      std::string option_name = option.substr(p + 1);
      compo->set_command_option_value(option, state ? "1" : "0");
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::activate_catalog_tree_item(const grt::ValueRef &value) {
  if (value.is_valid() && db_DatabaseObjectRef::can_wrap(value)) {
    db_DatabaseObjectRef object(db_DatabaseObjectRef::cast_from(value));

    bec::GRTManager::get()->open_object_editor(object);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::selection_changed() {
  get_wb()->request_refresh(RefreshSelection, "", 0);

  if (bec::GRTManager::get()->in_main_thread())
    revalidate_menu();
  else
    bec::GRTManager::get()->run_once_when_idle(std::bind(&ModelDiagramForm::revalidate_menu, this));
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::diagram_changed(grt::internal::OwnedList *olist, bool added, const grt::ValueRef &val) {
  _idle_node_mark.disconnect();
  if (added)
    _idle_node_mark =
      bec::GRTManager::get()->run_once_when_idle(std::bind(&ModelDiagramForm::mark_catalog_node, this, val, true));
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::mark_catalog_node(grt::ValueRef val, bool mark) {
  if (model_ObjectRef::can_wrap(val)) {
    model_ObjectRef f(model_ObjectRef::cast_from(val));
    if (f.is_valid())
      _catalog_tree->mark_node(_owner->get_object_for_figure(f), mark);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::attach_canvas_view(mdc::CanvasView *cview) {
  _view = cview;

  cview->set_tag(_model_diagram.id());
  cview->set_grid_snapping(bec::GRTManager::get()->get_app_option_int("AlignToGrid", 0) != 0);
  cview->get_background_layer()->set_grid_visible(_model_diagram->options().get_int("ShowGrid", 1) != 0);
  cview->get_background_layer()->set_paper_visible(_model_diagram->options().get_int("ShowPageGrid", 1) != 0);

  scoped_connect(cview->get_selection()->signal_begin_dragging(),
                 std::bind(&ModelDiagramForm::begin_selection_drag, this));
  scoped_connect(cview->get_selection()->signal_end_dragging(), std::bind(&ModelDiagramForm::end_selection_drag, this));
  scoped_connect(_model_diagram->get_data()->signal_selection_changed(),
                 std::bind(&ModelDiagramForm::selection_changed, this));

  _main_layer = _view->get_current_layer();
  _badge_layer = _view->new_layer("badges");
  _floater_layer = _view->new_layer("floater");

  // force updates
  selection_changed();
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::close() {
  set_closed(true);
  _mini_view->set_active_view(NULL, model_DiagramRef());

  if (_mini_view != 0) {
    delete _mini_view;
    _mini_view = 0;
  }

  _model_diagram->get_data()->unrealize();
}

//----------------------------------------------------------------------------------------------------------------------

CatalogTreeView *ModelDiagramForm::get_catalog_tree() {
  if (_catalog_tree == NULL) {
    _catalog_tree = new CatalogTreeView(this);
    _catalog_tree->set_activate_callback(
      std::bind(&ModelDiagramForm::activate_catalog_tree_item, this, std::placeholders::_1));
  }
  return _catalog_tree;
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::notify_catalog_tree(const CatalogNodeNotificationType &notify_type, grt::ValueRef value) {
  _idle_node_mark.disconnect(); // if there is pending mark_node, disable it
  if (_catalog_tree) {
    switch (notify_type) {
      case wb::NodeUnmark:
        _catalog_tree->mark_node(value, false);
        break;
      case wb::NodeAddUpdate:
        _catalog_tree->add_update_node_caption(value);
        break;
      case wb::NodeDelete:
        _catalog_tree->remove_node(value);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::refill_catalog_tree() {
  if (!_catalog_tree) {
    _catalog_tree = new CatalogTreeView(this);
    _catalog_tree->set_activate_callback(
      std::bind(&ModelDiagramForm::activate_catalog_tree_item, this, std::placeholders::_1));
  }

  _catalog_tree->refill(true);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::set_closed(bool flag) {
  if (_model_diagram.is_valid())
    _model_diagram->closed(flag != 0);
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::is_closed() {
  return *_model_diagram->closed() != 0;
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::set_button_callback(
  const std::function<bool(ModelDiagramForm *, mdc::MouseButton, bool, Point, mdc::EventState)> &cb) {
  _handle_button = cb;
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::set_motion_callback(const std::function<bool(ModelDiagramForm *, Point, mdc::EventState)> &cb) {
  _handle_motion = cb;
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::set_reset_tool_callback(const std::function<void(ModelDiagramForm *)> &cb) {
  _reset_tool = cb;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Enables or disables zooming via shortcut and mouse click (similar to Adobe Photoshop).
 *
 * @param enable If true the tool is enabled otherwise the previous tool is restored.
 * @param zoomin If true mouse clicks will zoom into the diagram otherwise zoom out.
 */
void ModelDiagramForm::enable_zoom_click(bool enable, bool zoomin) {
  if (!enable) {
    _reset_tool(this);
    _tool = _old_tool;
    _cursor = _old_cursor;
    _reset_tool = _old_reset_tool;
    _handle_button = _old_handle_button;
    _handle_motion = _old_handle_motion;

    set_tool(_tool);

    return;
  }

  _old_tool = _tool;

  if (zoomin)
    _tool = WB_TOOL_ZOOM_IN;
  else
    _tool = WB_TOOL_ZOOM_OUT;

  _old_reset_tool = _reset_tool;
  _old_handle_button = _handle_button;
  _old_handle_motion = _handle_motion;
  _old_cursor = _cursor;
  WBComponent *compo = _owner->get_wb()->get_component_named("basic");
  compo->setup_canvas_tool(this, _tool);

  set_tool(_tool);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::enable_panning(bool flag) {
  if (flag) {
    _old_tool = _tool;
    _old_reset_tool = _reset_tool;
    _old_handle_button = _handle_button;
    _old_handle_motion = _handle_motion;
    _old_cursor = _cursor;
    _tool = WB_TOOL_HAND;
    WBComponent *compo = _owner->get_wb()->get_component_named("basic");
    compo->setup_canvas_tool(this, _tool);

    set_tool(_tool);
  } else {
    _reset_tool(this);
    _tool = _old_tool;
    _cursor = _old_cursor;
    _reset_tool = _old_reset_tool;
    _handle_button = _old_handle_button;
    _handle_motion = _old_handle_motion;
    set_tool(_tool);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::begin_selection_drag() {
  mdc::Selection::ContentType selection(_view->get_selection()->get_contents());
  mdc::AreaGroup *root = _view->get_current_layer()->get_root_area_group();

  // save current position of everything before dragging around objects
  // so that we can store that for undo
  {
    _old_positions.clear();

    grt::ListRef<model_Object> sel(_model_diagram->selection());
    for (size_t c = sel.count(), i = 0; i < c; i++) {
      if (sel[i]->is_instance(model_Figure::static_class_name())) {
        model_FigureRef figure(model_FigureRef::cast_from(sel[i]));

        _old_positions[figure.valueptr()].pos = Point(figure->left(), figure->top());
        _old_positions[figure.valueptr()].layer_id = figure->layer().id();
      } else if (sel[i]->is_instance(model_Layer::static_class_name())) {
        model_LayerRef layer(model_LayerRef::cast_from(sel[i]));

        _old_positions[layer.valueptr()].pos = Point(layer->left(), layer->top());
      }
    }
  }

  // remove objects from respective layers so they can be freely dragged
  for (mdc::Selection::ContentType::const_iterator iter = selection.begin(); iter != selection.end(); ++iter) {
    // if the layer of the figure is also selected, then leave the figure alone
    Point rpos = (*iter)->get_root_position();

    if (root != (*iter)->get_parent() && !(*iter)->get_parent()->get_selected()) {
      root->add(*iter);
      (*iter)->move_to(rpos);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::end_selection_drag() {
  std::string name;
  bool moved = false;
  int count = 0;

  grt::UndoManager *um = grt::GRT::get()->get_undo_manager();

  um->begin_undo_group();

  // save original position of objects into undo record
  grt::ListRef<model_Object> sel(_model_diagram->selection());
  for (size_t c = sel.count(), i = 0; i < c; i++) {
    if (sel[i]->is_instance(model_Figure::static_class_name())) {
      model_FigureRef figure(model_FigureRef::cast_from(sel[i]));
      {
        Point p = _old_positions[figure.valueptr()].pos;
        std::string layer_id = _old_positions[figure.valueptr()].layer_id;

        if (*figure->left() != p.x || *figure->top() != p.y || layer_id != figure->layer().id()) {
          moved = true;
          um->add_undo(new grt::UndoObjectChangeAction(figure, "left", grt::DoubleRef(p.x)));
          um->add_undo(new grt::UndoObjectChangeAction(figure, "top", grt::DoubleRef(p.y)));
        }
      }
      name = figure->name();
      count++;
    } else if (sel[i]->is_instance(model_Layer::static_class_name())) {
      model_LayerRef layer(model_LayerRef::cast_from(sel[i]));
      {
        Point p = _old_positions[layer.valueptr()].pos;

        if (*layer->left() != p.x || *layer->top() != p.y) {
          moved = true;
          um->add_undo(new grt::UndoObjectChangeAction(layer, "left", grt::DoubleRef(p.x)));
          um->add_undo(new grt::UndoObjectChangeAction(layer, "top", grt::DoubleRef(p.y)));
        }
      }
      name = layer->name();
      count++;
    }
  }

  // change the layer of moved stuff
  {
    grt::AutoUndo undo;
    if (relocate_figures()) {
      moved = true;
      undo.end("Update Object Layers");
    } else
      undo.cancel();
  }

  if (!moved) {
    um->cancel_undo_group();
    return;
  }

  if (!name.empty() && count == 1)
    um->end_undo_group(strfmt(_("Move %s"), name.c_str()));
  else
    um->end_undo_group(_("Move Objects"));
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::current_mouse_position(int &x, int &y) {
  int w, h;

  _view->get_view_size(w, h);
  x = _current_mouse_x;
  y = _current_mouse_y;

  if (x < 0 || y < 0 || x >= w || y >= h)
    return false;
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::current_mouse_position(Point &pos) {
  int x, y;
  bool inside = current_mouse_position(x, y);

  pos = _view->window_to_canvas(x, y);

  return inside;
}

//----------------------------------------------------------------------------------------------------------------------

mdc::CanvasItem *ModelDiagramForm::get_leaf_item_at(const Point &pos) {
  return _view->get_leaf_item_at(pos);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::handle_mouse_button(mdc::MouseButton button, bool press, int x, int y, mdc::EventState state) {
  if (_features)
    _features->tooltip_cancel();

  stop_editing();

  Point pos(_view->window_to_canvas(x, y));

  if (button == mdc::ButtonRight && !press) {
    model_ObjectRef object = get_object_at(pos);

    bec::MenuItemList items;

    // If the user clicks in a selected object or in the view just show popup.
    // If user clicks in an unselected object, select it and show popup.

    if (object.is_valid() && _model_diagram->selection().get_index(object) == grt::BaseListRef::npos)
      _view->get_selection()->set(_view->get_item_at(pos));

    {
      std::list<std::string> groups;

      groups.push_back("Catalog/*");
      groups.push_back("Model/*");

      get_wb()->get_model_context()->get_object_list_popup_items(
        this, std::vector<bec::NodeId>(), grt::ListRef<GrtObject>::cast_from(_model_diagram->selection()),
        get_edit_target_name(), groups, items);
    }
    // else
    //_owner->get_model_popup_items(items);

    if (!items.empty()) {
      int x, y;
      _view->canvas_to_window(pos, x, y);

      _context_menu.clear();
      _context_menu.add_items_from_list(items);
      _context_menu.set_handler(
        [](const std::string &str) { wb::WBContextUI::get()->get_command_ui()->activate_command(str); });

      _context_menu.popup_at(NULL, x, y);
    }
    return;
  }

  // temporarily select Hand tool when middle-button-dragging
  if (button == mdc::ButtonMiddle) {
    if (press && !_drag_panning && !_space_panning) {
      _drag_panning = true;
      enable_panning(true);
    } else if (!press && _drag_panning) {
      _drag_panning = false;
      enable_panning(false);
    }
  }

  if (press && button == mdc::ButtonLeft) {
    // Handle zoom clicks.
    if (_tool == WB_TOOL_ZOOM_IN) {
      zoom_in();
      return;
    } else if (_tool == WB_TOOL_ZOOM_OUT) {
      zoom_out();
      return;
    }
  }

  if (button != mdc::ButtonLeft && button != mdc::ButtonMiddle)
    return;

  if (_handle_button && _handle_button(this, button, press, pos, state))
    return;

  _view->handle_mouse_button(button, press, x, y, state);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::handle_mouse_double_click(mdc::MouseButton button, int x, int y, mdc::EventState state) {
  stop_editing();

  if (button != mdc::ButtonLeft && button != mdc::ButtonMiddle)
    return;

  _view->handle_mouse_double_click(button, x, y, state);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::handle_mouse_move(int x, int y, mdc::EventState state) {
  Point pos(_view->window_to_canvas(x, y));

  _current_mouse_x = x;
  _current_mouse_y = y;

  if (_handle_motion && _handle_motion(this, pos, state))
    return;

  _view->handle_mouse_move(x, y, state);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::handle_mouse_leave(int x, int y, mdc::EventState state) {
  _view->handle_mouse_leave(x, y, state);
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::handle_key(const mdc::KeyInfo &key, bool press, mdc::EventState state) {
  if (press) {
    // cancel tooltip on keypress
    if (_features)
      _features->tooltip_cancel();

    for (std::vector<WBShortcut>::const_iterator iter = _shortcuts.begin(); iter != _shortcuts.end(); ++iter) {
      if (iter->modifiers == state && iter->key == key) {
        if (iter->command.find("tool:") == 0)
          set_tool(iter->command.substr(strlen("tool:")));
        else if (iter->command == "zoomin")
          zoom_in();
        else if (iter->command == "zoomout")
          zoom_out();
        else if (iter->command == "zoomdefault")
          set_zoom(1);
        else
          wb::WBContextUI::get()->get_command_ui()->activate_command(iter->command);
        return true;
      }
    }

    if (key.keycode == mdc::KSpace) {
      if (state == 0) {
        if (!_drag_panning && !_space_panning) {
          _space_panning = true;
          enable_panning(true);
        }
      } else if (_tool != WB_TOOL_ZOOM_IN && _tool != WB_TOOL_ZOOM_OUT) {
        if (state == mdc::SControlMask) {
          enable_zoom_click(true, true);
          return true;
        } else if (state == mdc::SAltMask) {
          enable_zoom_click(true, false);
          return true;
        }
      } else
        return true; // Still return true to tell the caller we consumed the input
                     // (even though only the first appearance, but if we don't it is as if
                     // we hadn't consumed it at all).
    }
    
    if (key.keycode == mdc::KDelete) {
        delete_selection();
        return true;
    }
  } else {
    // Key release.
    if (_space_panning) {
      _space_panning = false;
      enable_panning(false);
    }
    if (_tool == WB_TOOL_ZOOM_IN || _tool == WB_TOOL_ZOOM_OUT)
      enable_zoom_click(false, false);
  }

  return _view->handle_key(key, press, state);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::set_tool(std::string tool) {
  if (_tool != DEFAULT_TOOL)
    reset_tool(false);

  _tool = tool;

  WBComponent *compo = _owner->get_wb()->get_component_named(base::split(tool, "/")[0]);
  if (compo)
    compo->setup_canvas_tool(this, tool);
  else
    throw std::runtime_error("Invalid tool " + tool);

  std::vector<mforms::ToolBarItem *> items = _tools_toolbar->get_items();
  for (std::vector<mforms::ToolBarItem *>::iterator item = items.begin(); item != items.end(); ++item) {
    if ((*item)->get_type() == mforms::ToggleItem) {
      if ((*item)->getInternalName() == _tool)
        (*item)->set_checked(true);
      else
        (*item)->set_checked(false);
    }
  }

  if (_owner->get_wb()->_frontendCallbacks->tool_changed)
    _owner->get_wb()->_frontendCallbacks->tool_changed(_view);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::reset_tool(bool notify) {
  if (_tools_toolbar) {
    mforms::ToolBarItem *item = _tools_toolbar->find_item(_tool);
    if (!_tool.empty() && item)
      item->set_checked(false);
    item = _tools_toolbar->find_item(DEFAULT_TOOL);
    if (item)
      item->set_checked(true);
  }

  _tool = DEFAULT_TOOL;

  if (_reset_tool)
    _reset_tool(this);

  _cursor = "";

  std::function<bool()> f = []() { return false; };
  _handle_button = (std::bind(f));
  _handle_motion = (std::bind(f));
  _reset_tool = (std::bind(f));

  if (notify && _owner->get_wb()->_frontendCallbacks->tool_changed)
    _owner->get_wb()->_frontendCallbacks->tool_changed(_view);
}

std::string ModelDiagramForm::get_tool_argument(const std::string &option) {
  return _tool_args[option];
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Sets options for the selected tool
 *
 * Change the value of an option for the currently selected tool.
 *
 * @param option name of the option
 * @param value value of the option
 */
void ModelDiagramForm::set_tool_argument(const std::string &option, const std::string &value) {
  _tool_args[option] = value;

  _tool_argument_changed(option);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::set_zoom(double zoom) {
  _model_diagram->zoom(zoom);
}

//----------------------------------------------------------------------------------------------------------------------

double ModelDiagramForm::get_zoom() {
  return _model_diagram->zoom();
}

//----------------------------------------------------------------------------------------------------------------------

bec::Clipboard *ModelDiagramForm::get_clipboard() {
  return bec::GRTManager::get()->get_clipboard();
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::can_undo() {
  return grt::GRT::get()->get_undo_manager()->can_undo();
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::can_redo() {
  return grt::GRT::get()->get_undo_manager()->can_redo();
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::can_copy() {
  return get_copiable_selection().count() > 0;
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::can_select_all() {
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

static void check_if_can_paste(WBComponent *compo, const grt::ObjectRef &object, bool *result) {
  if (compo->can_paste_object(object))
    *result = true;
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::can_paste() {
  std::list<grt::ObjectRef> data(get_clipboard()->get_data());
  WBContext *wb = _owner->get_wb();

  for (std::list<grt::ObjectRef>::iterator iter = data.begin(); iter != data.end(); ++iter) {
    if (!(*iter).is_valid()) {
      logWarning("copy buffer has null value\n");
      return false;
    }
    bool result = false;
    wb->foreach_component(std::bind(check_if_can_paste, std::placeholders::_1, *iter, &result));
    if (!result)
      return false;
  }
  return !get_clipboard()->empty();
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::can_delete() {
  return has_selection();
}

std::string ModelDiagramForm::get_edit_target_name() {
  grt::ListRef<model_Object> sel(get_copiable_selection());

  if (sel.count() == 0)
    return "";

  if (sel.count() == 1) {
    std::string name;

    name = sel[0]->name().c_str();
    if (name.empty() && sel[0]->has_member("caption"))
      name = sel[0]->get_string_member("caption");

    return strfmt("'%s'", name.c_str());
  } else
    return strfmt(_("%i Selected Figures"), (int)sel.count());
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::undo() {
  grt::GRT::get()->get_undo_manager()->undo();
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::redo() {
  grt::GRT::get()->get_undo_manager()->redo();
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::cut() {
  grt::UndoManager *um = grt::GRT::get()->get_undo_manager();

  std::string edit_target_name = get_edit_target_name();

  um->begin_undo_group();
  copy();

  int count = (int)get_copiable_selection().count();
  remove_selection();
  um->end_undo_group();
  um->set_action_description(strfmt(_("Cut %s"), edit_target_name.c_str()));

  _owner->get_wb()->_frontendCallbacks->show_status_text(strfmt(_("%i figure(s) cut."), count));
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::copy() {
  // object must be duplicated on copy because if the object is edited after copy
  // whatever is pasted should still be in the same state as it was on copy
  grt::ListRef<model_Object> selection(get_copiable_selection());
  bec::Clipboard *clip = get_clipboard();
  int count = 0;
  grt::CopyContext copy_context;

  clip->clear();
  for (size_t c = selection.count(), i = 0; i < c; i++) {
    WBComponent *compo = _owner->get_wb()->get_component_handling(selection.get(i));
    if (compo) {
      compo->copy_object_to_clipboard(selection.get(i), copy_context);
      count++;
    }
  }
  clip->set_content_description(get_edit_target_name());

  copy_context.finish();
  clip->changed();

  _owner->get_wb()->_frontendCallbacks->show_status_text(strfmt(_("%i object(s) copied."), count));
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::clipboard_changed() {
  _paste_offset = 0;
}

//----------------------------------------------------------------------------------------------------------------------

static void get_component_that_can_paste(WBComponent *compo, const grt::ObjectRef &object, WBComponent **result) {
  if (compo->can_paste_object(object))
    *result = compo;
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::paste() {
  // Prevent too many updates.
  UpdateLock lock(this);

  WBContext *wb = _owner->get_wb();
  int count = 0;
  int duplicated = 0;

  _paste_offset += 20;

  _owner->get_wb()->_frontendCallbacks->show_status_text(_("Pasting figures..."));

  grt::CopyContext context;

  grt::AutoUndo undo;

  _model_diagram->unselectAll();

  std::list<grt::ObjectRef> data(get_clipboard()->get_data());

  // find what objects are in layers so that if they are also selected individually, they don't
  // get copied again
  std::set<std::string> skip_objects;
  for (std::list<grt::ObjectRef>::iterator iter = data.begin(); iter != data.end(); ++iter) {
    if (model_LayerRef::can_wrap(*iter)) {
      grt::ListRef<model_Figure> figures(model_LayerRef::cast_from(*iter)->figures());
      GRTLIST_FOREACH(model_Figure, figures, fig) {
        skip_objects.insert((*fig)->id());
      }
    }
  }

  for (std::list<grt::ObjectRef>::iterator iter = data.begin(); iter != data.end(); ++iter) {
    // paste the object
    WBComponent *compo = 0;
    wb->foreach_component(std::bind(get_component_that_can_paste, std::placeholders::_1, *iter, &compo));
    if (compo) {
      if (skip_objects.find((*iter)->id()) == skip_objects.end()) {
        compo->paste_object(this, *iter, context);
        count++;
      }
    } else
      logWarning("Don't know how to paste %s\n", iter->class_name().c_str());
  }
  context.finish();
  undo.end(strfmt(_("Paste %s"), get_clipboard()->get_content_description().c_str()));

  if (duplicated == 0)
    _owner->get_wb()->_frontendCallbacks->show_status_text(strfmt(_("%i figure(s) pasted."), count));
  else
    _owner->get_wb()->_frontendCallbacks->show_status_text(
      strfmt(_("%i figure(s) pasted, %i duplicated."), count, duplicated));
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::select_all() {
  for (size_t c = get_model_diagram()->figures().count(), i = 0; i < c; i++)
    get_model_diagram()->selectObject(get_model_diagram()->figures().get(i));

  for (size_t c = get_model_diagram()->layers().count(), i = 0; i < c; i++)
    get_model_diagram()->selectObject(get_model_diagram()->layers().get(i));
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::remove_selection(bool deleteSelection) {
  grt::UndoManager *um = grt::GRT::get()->get_undo_manager();
  grt::ListRef<model_Object> selection = get_selection();

  std::vector<model_ObjectRef> objects;
  std::string edit_target_name = get_edit_target_name();

  um->begin_undo_group();

  for (size_t c = selection.count(), i = 0; i < c; i++) {
    if (selection.get(i).is_instance(model_Object::static_class_name()))
      objects.push_back(model_ObjectRef::cast_from(selection.get(i)));
  }

  std::string actionDescription;
  std::string statusText;
  if (deleteSelection) {
    for (size_t c = objects.size(), i = 0; i < c; i++)
      _owner->get_wb()->get_model_context()->delete_object(objects[i]);

    actionDescription = strfmt(_("Delete %s"), edit_target_name.c_str());
    statusText = strfmt(_("%i object(s) deleted."), (int)objects.size());
  } else {
    for (size_t c = objects.size(), i = 0; i < c; i++)
      _owner->get_wb()->get_model_context()->remove_figure(objects[i]);

    actionDescription = strfmt(_("Remove %s"), edit_target_name.c_str());
    statusText = strfmt(_("%i figure(s) removed. The corresponding DB objects were kept."), (int)objects.size());
  }

  um->end_undo_group();
  um->set_action_description(actionDescription);

  _owner->get_wb()->_frontendCallbacks->show_status_text(statusText);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::delete_selection() {
  remove_selection(true);
}

//----------------------------------------------------------------------------------------------------------------------

std::string ModelDiagramForm::get_diagram_info_text() {
  if (_model_diagram.is_valid())
    return strfmt("%i x %i mm", (int)*_model_diagram->width(), (int)*_model_diagram->height());
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> ModelDiagramForm::get_accepted_drop_types() {
  std::vector<std::string> vec;
  vec.push_back(WB_DBOBJECT_DRAG_TYPE);
  return vec;
}

//----------------------------------------------------------------------------------------------------------------------

grt::ListRef<model_Object> ModelDiagramForm::get_selection() {
  return _model_diagram->selection();
}

//----------------------------------------------------------------------------------------------------------------------

grt::ListRef<model_Object> ModelDiagramForm::get_copiable_selection() {
  grt::ListRef<model_Object> sel(_model_diagram->selection());
  grt::ListRef<model_Object> copiable(true);

  for (size_t c = sel.count(), i = 0; i < c; i++) {
    if (!sel.get(i).is_instance(model_Connection::static_class_name()))
      copiable.insert(sel[i]);
  }
  return copiable;
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::has_selection() {
  return _model_diagram->selection().count() > 0;
}

//----------------------------------------------------------------------------------------------------------------------

static mdc::CanvasItem *extractItem(const model_ObjectRef &object) {
  mdc::CanvasItem *item = nullptr;

  if (object.is_instance(model_Figure::static_class_name())) {
    item = model_FigureRef::cast_from(object)->get_data()->get_canvas_item();
  } else if (object.is_instance(model_Connection::static_class_name())) {
    item = model_ConnectionRef::cast_from(object)->get_data()->get_canvas_item();
  } else if (object.is_instance(model_Layer::static_class_name())) {
    item = model_LayerRef::cast_from(object)->get_data()->get_area_group();
  } else {
    logWarning("Unhandled CanvasItem: %s\n", object.class_name().c_str());
  }
  return item;
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::is_visible(const model_ObjectRef &object, bool partially) {
  mdc::CanvasItem *item = extractItem(object);

  if (item == nullptr)
    return false;

  Rect bounds = item->get_root_bounds();
  Rect viewport = _view->get_viewport();

  if (partially)
    return mdc::bounds_intersect(viewport, bounds);
  else
    return mdc::bounds_contain_bounds(viewport, bounds);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::focus_and_make_visible(const model_ObjectRef &object, bool select) {
  mdc::CanvasItem *item = extractItem(object);
  if (item) {
    mdc::CanvasView *view = item->get_view();
    Rect bounds = item->get_root_bounds();
    Rect viewport = view->get_viewport();

    if (!mdc::bounds_contain_bounds(viewport, bounds)) {
      Point offset = viewport.pos;

      // get the offset that will move the viewport the least to bring the item inside it

      if (bounds.left() < viewport.left())
        offset.x = bounds.left() - 20;
      else if (bounds.right() > viewport.right())
        offset.x = bounds.right() - viewport.width();

      if (bounds.top() < viewport.top())
        offset.y = bounds.top() - 20;
      else if (bounds.bottom() > viewport.bottom())
        offset.y = bounds.bottom() - viewport.height();

      view->set_offset(offset);
    }
    view->focus_item(item);
    if (select)
      view->get_selection()->set(item);
  }
}

//----------------------------------------------------------------------------------------------------------------------

static model_ObjectRef search_object_list(const grt::ListRef<model_Object> &objects, size_t starting_index,
                                          const std::string &text) {
  for (size_t count = objects.count(), i = starting_index; i < count; i++) {
    model_ObjectRef object(objects[i]);

    if (strstr(object->name().c_str(), text.c_str()))
      return object;
  }

  return model_ObjectRef();
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::search_and_focus_object(const std::string &text) {
  if (text.empty())
    return false;
  grt::ListRef<model_Object> selection(get_selection());
  model_ObjectRef selected, found_object;

  if (selection.count() > 0)
    selected = selection[0];

  if (!selected.is_valid() || selected.is_instance(model_Figure::static_class_name())) {
    size_t index = 0;

    if (selected.is_valid()) {
      index = _model_diagram->figures().get_index(selected);
      if (index == grt::BaseListRef::npos)
        index = 0;
      else
        index++;
    }
    found_object = search_object_list(_model_diagram->figures(), index, text);
  }

  if (!found_object.is_valid() && (!selected.is_valid() || selected.is_instance(model_Connection::static_class_name()))) {
    size_t index = 0;
    if (selected.is_valid()) {
      index = _model_diagram->connections().get_index(selected);
      if (index == grt::BaseListRef::npos)
        index = 0;
      else
        index++;
    }
    found_object = search_object_list(_model_diagram->connections(), index, text);
  }

  if (!found_object.is_valid() && (!selected.is_valid() || selected.is_instance(model_Layer::static_class_name()))) {
    size_t index = 0;
    if (selected.is_valid()) {
      index = _model_diagram->layers().get_index(selected);
      if (index == grt::BaseListRef::npos)
        index = 0;
      else
        index++;
    }
    found_object = search_object_list(_model_diagram->layers(), index, text);
  }

  if (found_object.is_valid()) {
    bec::GRTManager::get()->replace_status_text(strfmt(_("Found %s '%s'"),
                                                       found_object.get_metaclass()->get_attribute("caption").c_str(),
                                                       found_object->name().c_str()));

    focus_and_make_visible(found_object, true);

    return true;
  } else {
    if (_model_diagram->selection().count() > 0)
      bec::GRTManager::get()->replace_status_text(_("No more matches"));
    else
      bec::GRTManager::get()->replace_status_text(_("No match found"));
  }

  _model_diagram->selection().remove_all();

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::set_cursor(const std::string &cursor) {
  _cursor = cursor;
}

//----------------------------------------------------------------------------------------------------------------------

model_LayerRef ModelDiagramForm::get_layer_at(const Point &pos, Point &offset) {
  model_LayerRef layer;
  mdc::AreaGroup *ag = 0;

  mdc::CanvasItem *item = _view->get_item_at(pos);

  if (!item) {
    offset = pos;
    return _model_diagram->rootLayer();
  }
  item = item->get_toplevel();

  while (item && (ag = dynamic_cast<mdc::AreaGroup *>(item)) == 0)
    item = item->get_parent();

  if (ag) {
    for (size_t c = _model_diagram->layers().count(), i = 0; i < c; i++) {
      layer = _model_diagram->layers().get(i);

      if (ag == layer->get_data()->get_area_group()) {
        offset = ag->convert_point_from(pos, 0);

        return layer;
      }
    }
  }

  offset = pos;

  return _model_diagram->rootLayer();
}

//----------------------------------------------------------------------------------------------------------------------

model_LayerRef ModelDiagramForm::get_layer_bounding(const Rect &rect, Point &offset) {
  grt::ListRef<model_Layer> layers(_model_diagram->layers());

  for (grt::ListRef<model_Layer>::const_reverse_iterator layer = layers.rbegin(); layer != layers.rend(); ++layer) {
    if ((*layer)->get_data()->get_area_group() &&
        mdc::bounds_contain_bounds((*layer)->get_data()->get_area_group()->get_root_bounds(), rect)) {
      return *layer;
    }
  }
  return model_LayerRef();
}

//----------------------------------------------------------------------------------------------------------------------

model_ObjectRef ModelDiagramForm::get_object_at(const Point &pos) {
  mdc::CanvasItem *item = _view->get_item_at(pos);

  if (!item)
    return model_ObjectRef();

  std::string id = item->get_tag();
  if (id.empty())
    return model_ObjectRef();

  model_ObjectRef object;

  object = grt::find_object_in_list(_model_diagram->figures(), id);
  if (object.is_valid())
    return object;

  object = grt::find_object_in_list(_model_diagram->layers(), id);
  if (object.is_valid())
    return object;

  object = grt::find_object_in_list(_model_diagram->connections(), id);
  if (object.is_valid())
    return object;

  return model_ObjectRef();
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::relocate_figures() {
  bool relocated = false;
  grt::ListRef<model_Figure> figures(_model_diagram->figures());

  // relocate all figures in the diagram to the layers they're inside
  for (size_t c = figures.count(), i = 0; i < c; i++) {
    model_FigureRef figure(figures[i]);

    relocated |= _model_diagram->get_data()->update_layer_of_figure(figure);
  }

  return relocated;
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::accepts_drop(int x, int y, const std::string &type, const std::list<GrtObjectRef> &objects) {
  return _owner->accepts_drop(this, x, y, type, objects);
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::accepts_drop(int x, int y, const std::string &type, const std::string &text) {
  return _owner->accepts_drop(this, x, y, type, text);
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::perform_drop(int x, int y, const std::string &type, const std::list<GrtObjectRef> &objects) {
  bool retval = _owner->perform_drop(this, x, y, type, objects);
  if (_catalog_tree && retval) // if it was accepted then we can mark all objects
  { // we will do the long way so we will not need to reload the whole tree and remember expanded rows
    std::list<GrtObjectRef>::const_iterator it;
    for (it = objects.begin(); it != objects.end(); ++it)
      _catalog_tree->mark_node(*it);
  }
  return retval;
}

//----------------------------------------------------------------------------------------------------------------------

bool ModelDiagramForm::perform_drop(int x, int y, const std::string &type, const std::string &text) {
  return _owner->perform_drop(this, x, y, type, text);
}

//----------------------------------------------------------------------------------------------------------------------

mdc::Layer *ModelDiagramForm::get_floater_layer() {
  return _floater_layer;
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::add_floater(Floater *floater) {
  Point pos;

  pos.x = _view->get_viewport().right() - 200;
  pos.y = _view->get_viewport().top() + 20;

  floater->move_to(pos);
  _floater_layer->add_item(floater);
}

//----------------------------------------------------------------------------------------------------------------------

WBContext *ModelDiagramForm::get_wb() {
  return _owner->get_wb();
}

//----------------------------------------------------------------------------------------------------------------------

// inline editing
void ModelDiagramForm::begin_editing(const Rect &rect, const std::string &text, float text_size, bool multiline) {
  if (_inline_edit_context) {
    int x, y;
    int width, height;

    _inline_edit_context->set_font_size(text_size);
    _inline_edit_context->set_multiline(multiline);

    _view->canvas_to_window(rect, x, y, width, height);

    _inline_edit_context->begin_editing(x, y, width, height, text);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::stop_editing() {
  if (_inline_edit_context)
    _inline_edit_context->end_editing();
}

//----------------------------------------------------------------------------------------------------------------------

static void forward_edit_finished(const std::string &text, EditFinishReason reason, ModelDiagramForm *form) {
  (*form->signal_editing_done())(text, reason);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::set_inline_editor_context(InlineEditContext *context) {
  _inline_edit_context = context;

  scoped_connect(_inline_edit_context->signal_edit_finished(),
                 std::bind(forward_edit_finished, std::placeholders::_1, std::placeholders::_2, this));
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::zoom_in() {
  model_DiagramRef view(get_model_diagram());
  double zoom = *view->zoom();

  for (size_t i = 0; i < sizeof(zoom_steps) / sizeof(*zoom_steps); i++) {
    if (zoom >= zoom_steps[i]) {
      if (i > 0)
        view->zoom(zoom_steps[i - 1]);
      return;
    }
  }
  view->zoom(zoom_steps[0]);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::zoom_out() {
  model_DiagramRef view(get_model_diagram());
  double zoom = *view->zoom();

  for (size_t i = 0; i < sizeof(zoom_steps) / sizeof(*zoom_steps); i++) {
    if (zoom >= zoom_steps[i]) {
      if (i + 1 < sizeof(zoom_steps) / sizeof(*zoom_steps))
        view->zoom(zoom_steps[i + 1]);
      return;
    }
  }
  view->zoom(zoom_steps[sizeof(zoom_steps) / sizeof(*zoom_steps) - 1]);
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::setup_mini_view(mdc::CanvasView *view) {
  if (!_mini_view) {
    _mini_view = new MiniView(view->get_current_layer());

    view->initialize();
    view->get_background_layer()->set_visible(false);
    view->set_page_layout(1, 1);
    view->set_page_size(view->get_viewable_size());

    view->get_current_layer()->add_item(_mini_view);

    int w, h;
    view->get_view_size(w, h);

    _mini_view->set_active_view(get_view(), get_model_diagram());

    update_mini_view_size(w, h);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::update_mini_view_size(int w, int h) {
  if (_mini_view) {
    mdc::CanvasView *view = _mini_view->get_layer()->get_view();

    view->update_view_size(w, h);
    view->set_page_size(view->get_viewable_size());

    _mini_view->update_size();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::setBackgroundColor(base::Color const& color) {
  if (_mini_view != nullptr)
    _mini_view->setBackgroundColor(color);
  if (_view != nullptr)
    _view->setBackgroundColor(color);
}

//----------------------------------------------------------------------------------------------------------------------

std::string ModelDiagramForm::get_title() {
  return std::string(_model_diagram->name());
}

//----------------------------------------------------------------------------------------------------------------------

void ModelDiagramForm::set_highlight_fks(bool flag) {
  _highlight_fks = flag;
  _features->highlight_all_connections(flag);
}

//----------------- UpdateLock -----------------------------------------------------------------------------------------

ModelDiagramForm::UpdateLock::~UpdateLock() {
  if (_form->_update_count > 0)
    _form->_update_count--;
  if (_form->_update_count == 0)
    _form->_layer_tree->refresh();
}

//----------------------------------------------------------------------------------------------------------------------
