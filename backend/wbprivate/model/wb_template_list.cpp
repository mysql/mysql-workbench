/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/menu.h"
#include "mforms/toolbar.h"
#include "mforms/scrollpanel.h"

#include "grts/structs.db.h"

#include "workbench/wb_context.h"
#include "wb_template_list.h"
#include "wb_context_model.h"
#include "wb_model_diagram_form.h"
#include "wb_component_physical.h"

using namespace mforms;
using namespace base;

size_t TableTemplateList::count() {
  return (int)grt::BaseListRef::cast_from(grt::GRT::get()->get("/wb/options/options/TableTemplates")).count();
}

bool TableTemplateList::get_field(const bec::NodeId &node, ColumnId column, std::string &value) {
  grt::BaseListRef templates(grt::BaseListRef::cast_from(grt::GRT::get()->get("/wb/options/options/TableTemplates")));
  if (node[0] < templates.count()) {
    db_TableRef table = db_TableRef::cast_from(templates[node[0]]);

    switch (column) {
      case 0:
        value = table->name();
        return true;
      case 1: {
        for (size_t c = table->columns().count(), i = 0; i < c; i++) {
          if (!value.empty())
            value.append(", ");
          value.append(table->columns()[i]->name());
        }
        return true;
      }
    }
  }
  return false;
}

void TableTemplateList::refresh() {
}

std::string TableTemplateList::get_selected_template() {
  std::string name;
  get_field(selected_index(), 0, name);
  return name;
}

//------------------------------------------------------------------------------------------------

void TableTemplateList::prepare_context_menu() {
  _context_menu = manage(new Menu());
  _context_menu->set_handler(std::bind(&TableTemplatePanel::on_action, _owner, std::placeholders::_1));
  _context_menu->signal_will_show()->connect(std::bind(&TableTemplateList::menu_will_show, this));

  _context_menu->add_item("New Table from Template", "use_template");
  _context_menu->add_separator();
  _context_menu->add_item("Edit Template...", "edit_templates");
}

//------------------------------------------------------------------------------------------------

void TableTemplateList::menu_will_show() {
}

//------------------------------------------------------------------------------------------------

TableTemplateList::TableTemplateList(TableTemplatePanel *owner)
  : BaseSnippetList("snippet_mwb.png", this), _owner(owner) {
  prepare_context_menu();
  refresh_snippets();

  _defaultSnippetActionCb = [&](int x, int y) {
     Snippet *snippet = snippet_from_point(x, y);
     if (snippet != nullptr) {
       set_selected(snippet);
       _owner->on_action("use_template");
     }
  };
}

//------------------------------------------------------------------------------------------------

TableTemplateList::~TableTemplateList() {
  _context_menu->release();
}

//------------------------------------------------------------------------------------------------

bool TableTemplateList::mouse_double_click(mforms::MouseButton button, int x, int y) {
  BaseSnippetList::mouse_double_click(button, x, y);

  if (button == MouseButtonLeft) {
    Snippet *snippet = snippet_from_point(x, y);
    if (snippet != NULL && snippet == _selected_snippet) {
      _owner->on_action("use_template");
      return true;
    }
  }

  return false;
}

//------------------------------------------------------------------------------------------------

TableTemplatePanel::TableTemplatePanel(wb::WBContextModel *cmodel)
  : mforms::Box(false), _templates(this), _context(cmodel) {
#ifdef _MSC_VER
  set_padding(3, 3, 3, 3);
  _templates.set_back_color(base::Color::getApplicationColorAsString(AppColorPanelContentArea, false));
#elif __linux__
  _templates.set_back_color("#f2f2f2");
#endif
    
  _scroll_panel = manage(new mforms::ScrollPanel());
  _scroll_panel->add(&_templates);

  _toolbar = mforms::manage(new mforms::ToolBar(mforms::PaletteToolBar));

  mforms::ToolBarItem *item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Edit Templates");
  item->setInternalName("edit_templates");
  item->set_icon(mforms::App::get()->get_resource_path("edit_table_templates.png"));
  item->set_tooltip("Open the table template editor");
  scoped_connect(item->signal_activated(),
                 std::bind(&TableTemplatePanel::toolbar_item_activated, this, std::placeholders::_1));
  _toolbar->add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::SeparatorItem));
  _toolbar->add_item(item);

  /*
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_icon(mforms::App::get()->get_resource_path("snippet_add.png"));
  item->set_name("add_template");
  item->set_tooltip("Create a table template from the selected table object.");
  scoped_connect(item->signal_activated(), std::bind(&TableTemplatePanel::toolbar_item_activated, this,
  std::placeholders::_1));
  _toolbar->add_item(item);
*/
  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Use Template");
  item->setInternalName("use_template");
  item->set_icon(mforms::App::get()->get_resource_path("tiny_new_table.png"));
  item->set_tooltip("Create a new table based on the selected table template");
  scoped_connect(item->signal_activated(),
                 std::bind(&TableTemplatePanel::toolbar_item_activated, this, std::placeholders::_1));
  _toolbar->add_item(item);

  add(_toolbar, false, true);
  add(_scroll_panel, true, true);
}

void TableTemplatePanel::on_action(const std::string &action) {
  if (action == "edit_templates") {
    grt::BaseListRef args(true);
    args.ginsert(grt::StringRef(_templates.get_selected_template()));
    grt::GRT::get()->call_module_function("WbTableUtils", "openTableTemplateEditorFor", args);
    _templates.refresh_snippets();
  } else if (action == "use_template") {
    if (!_templates.get_selected_template().empty()) {
      grt::BaseListRef args(true);
      args.ginsert(workbench_physical_ModelRef::cast_from(_context->get_active_model(true))->catalog()->schemata()[0]);
      args.ginsert(grt::StringRef(_templates.get_selected_template()));
      db_TableRef table(
        db_TableRef::cast_from(grt::GRT::get()->call_module_function("WbTableUtils", "createTableFromTemplate", args)));
      if (table.is_valid()) {
        model_DiagramRef d = _context->get_active_model_diagram(true);
        if (d.is_valid()) {
          wb::ModelDiagramForm *diagram = _context->get_diagram_form_for_diagram_id(d.id());
          if (diagram) {
            std::list<GrtObjectRef> objects;
            objects.push_back(table);
            diagram->perform_drop(10, 10, WB_DBOBJECT_DRAG_TYPE, objects);
          }
        }
      }
    } else
      mforms::Utilities::show_message("Empty Selection", "Please select template to be used.", "Ok");
  }
}

void TableTemplatePanel::toolbar_item_activated(mforms::ToolBarItem *item) {
  on_action(item->getInternalName());
}
