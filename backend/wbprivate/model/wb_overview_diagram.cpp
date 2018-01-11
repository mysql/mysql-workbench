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

#include "wb_overview_diagram.h"
#include "workbench/wb_context.h"
#include "model/wb_context_model.h"
#include "grtpp_undo_manager.h"
#include "grt/icon_manager.h"
#include "base/string_utilities.h"

using namespace bec;
using namespace wb;
using namespace base;

class AddDiagramNode : public OverviewBE::Node {
public:
  model_ModelRef model;

  virtual bool activate(WBContext *wb) {
    wb->get_model_context()->add_new_diagram(model);
    return true;
  }
};

class DiagramNode : public OverviewBE::ObjectNode {
public:
  DiagramNode(const model_DiagramRef &view) {
    object = view;
  }

  virtual bool activate(WBContext *wb) {
    wb->get_model_context()->switch_diagram(model_DiagramRef::cast_from(object));
    return true;
  }

  virtual bool is_deletable() {
    return true;
  }

  virtual void delete_object(WBContext *wb) {
    wb->get_model_context()->delete_diagram(model_DiagramRef::cast_from(object));
  }

  virtual bool rename(WBContext *wb, const std::string &name) {
    grt::AutoUndo undo;

    object->name(name);

    undo.end(strfmt(_("Rename Diagram to '%s'"), name.c_str()));

    return true;
  }

  virtual bool is_renameable() {
    return true;
  }

  virtual int get_popup_menu_items(WBContext *wb, bec::MenuItemList &items) {
    return OverviewBE::ObjectNode::get_popup_menu_items(wb, items);
  }
};

//----------------------------------------------------------------------

DiagramListNode::DiagramListNode(model_ModelRef model) : ContainerNode(OverviewBE::OItem), _model(model) {
  id = model.id() + "/modellist";

  type = OverviewBE::ODivision;
  label = _("EER Diagrams");
  small_icon = 0;
  large_icon = 0;
  expanded = true;
  display_mode = OverviewBE::MLargeIcon;

  refresh_children();
}

void DiagramListNode::refresh_children() {
  clear_children();

  if (_model->diagrams().is_valid()) {
    for (size_t c = _model->diagrams().count(), i = 0; i < c; i++) {
      DiagramNode *node = new DiagramNode(_model->diagrams()[i]);

      node->type = OverviewBE::OItem;
      node->label = *_model->diagrams()[i]->name();

      std::string icon_name = _model->diagrams().content_class_name() + ".$.png";
      node->small_icon = IconManager::get_instance()->get_icon_id(icon_name, Icon16);
      node->large_icon = IconManager::get_instance()->get_icon_id(icon_name, Icon48);

      children.push_back(node);
    }

    AddDiagramNode *node = new AddDiagramNode();

    node->type = OverviewBE::OItem;
    node->label = _("Add Diagram");

    std::string icon_name = _model->diagrams().content_class_name() + ".$.png";
    node->small_icon = IconManager::get_instance()->get_icon_id(icon_name, Icon16, "add");
    node->large_icon = IconManager::get_instance()->get_icon_id(icon_name, Icon48, "add");
    node->model = _model;

    children.insert(children.begin(), node);
  }
}
