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
#include "wb_overview_physical.h"
#include "wb_overview_physical_schema.h"

#include "workbench/wb_context.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_command_ui.h"
#include "wb_component_physical.h"

#include "grt/incremental_list_updater.h"

#include "wb_overview_privileges.h"

#include "grt/icon_manager.h"
#include "grt/clipboard.h"
#include "base/ui_form.h"
#include "grt/exceptions.h"

#include "grts/structs.db.h"
#include "base/string_utilities.h"

#include "mforms/toolbar.h"
#include "mforms/menubar.h"

#include "base/trackable.h"

/**
 * @file  wb_overview_physical.cpp
 * @brief Physical Model specific panels for the overview window
 */

using namespace bec;
using namespace wb;
using namespace wb::internal;
using namespace base;

#define SCRIPT_NODE NodeId(3)
#define NOTE_NODE NodeId(4)

PhysicalSchemataNode::PhysicalSchemataNode(workbench_physical_ModelRef amodel) : ContainerNode(OverviewBE::OGroup) {
  type = OverviewBE::ODivision;
  object = amodel->catalog();
  model = amodel;
  label = _("Physical Schemas");
  small_icon = 0;
  large_icon = 0;
  expanded = true;
  display_mode = OverviewBE::MSmallIcon;
}

void PhysicalSchemataNode::init() {
  grt::ListRef<db_Schema> schemata = model->catalog()->schemata();
  for (size_t c = schemata.count(), i = 0; i < c; i++)
    children.push_back(create_child_node(schemata.get(i)));
}

OverviewBE::Node *PhysicalSchemataNode::create_child_node(db_SchemaRef schema) {
  PhysicalSchemaNode *node = new PhysicalSchemaNode(schema);
  node->init();
  return node;
}

bool PhysicalSchemataNode::add_object(WBContext *wb) {
  bec::GRTManager::get()->open_object_editor(wb->get_component<WBComponentPhysical>()->add_new_db_schema(model));
  return true;
}

void PhysicalSchemataNode::delete_object(WBContext *wb) {
  // dynamic_cast<WBComponentPhysical*>(wb->get_component("physical"))->delete_
}

class SchemaListUpdater
  : public IncrementalListUpdater<std::vector<OverviewBE::Node *>::iterator, OverviewBE::Node *, size_t> {
  virtual dest_iterator get_dest_iterator() {
    return _nodes.begin();
  }

  virtual source_iterator get_source_iterator() {
    return 0;
  }

  virtual dest_iterator increment_dest(dest_iterator &iter) {
    return ++iter;
  }

  virtual source_iterator increment_source(source_iterator &iter) {
    return ++iter;
  }

  virtual bool has_more_dest(dest_iterator iter) {
    return iter != _nodes.end();
  }

  virtual bool has_more_source(source_iterator iter) {
    return _schemata.is_valid() && iter < _schemata.count();
  }

  virtual bool items_match(dest_iterator diter, source_iterator siter) {
    return (*diter)->object == _schemata.get(siter);
  }

  virtual dest_ref get_dest(dest_iterator iter) {
    _reused_items.insert(*iter);
    return *iter;
  }

  virtual void update(dest_ref dest_item, source_iterator source_item) {
    dest_item->refresh();
  }

  // begin adding items to the begginning of the dest list
  virtual dest_iterator begin_adding() {
    for (dest_iterator i = _nodes.begin(); i != _nodes.end(); ++i) {
      if (_reused_items.find(*i) == _reused_items.end())
        delete *i;
    }
    _nodes.clear();

    return _nodes.end();
  }

  virtual dest_iterator add(dest_iterator &iter, source_iterator source_item) {
    return ++_nodes.insert(iter, _schema_node_instantiation_slot(_schemata.get(source_item)));
  }

  virtual dest_iterator add(dest_iterator &iter, dest_ref item) {
    // store the items that are reused so they're not deleted later
    _reused_items.insert(item);

    return ++_nodes.insert(iter, item);
  }

  // end adding items to the dest item, stuff after the last item added must be removed
  virtual void end_adding(dest_iterator iter) {
  }

  std::vector<OverviewBE::Node *> &_nodes;
  std::set<OverviewBE::Node *> _reused_items;
  grt::ListRef<db_Schema> _schemata;
  typedef std::function<OverviewBE::Node *(db_SchemaRef)> SchemaNodeInstantiationSlot;
  SchemaNodeInstantiationSlot _schema_node_instantiation_slot;

public:
  SchemaListUpdater(std::vector<OverviewBE::Node *> &nodes, const grt::ListRef<db_Schema> &schemata,
                    SchemaNodeInstantiationSlot schema_node_instantiation_slot)
    : _nodes(nodes), _schemata(schemata), _schema_node_instantiation_slot(schema_node_instantiation_slot) {
  }
};

void PhysicalSchemataNode::refresh_children() {
  focused = 0;

  SchemaListUpdater updater(children, db_CatalogRef::cast_from(object)->schemata(),
                            std::bind(&PhysicalSchemataNode::create_child_node, this, std::placeholders::_1));

  updater.execute();
}

//------------------------------------------------------------------------

class ModelObjectNode : public OverviewBE::ObjectNode, public base::trackable {
public:
  std::string member;

  virtual void delete_object(WBContext *wb) {
    grt::AutoUndo undo;
    // removal from the list will trigger deletion of the file automatically

    grt::ListRef<GrtObject>::cast_from(object->owner().get_member(member)).remove_value(object);

    undo.end(strfmt(_("Delete '%s'"), object->name().c_str()));
  }

  virtual bool is_deletable() {
    return true;
  }

  virtual bool is_renameable() {
    return true;
  }

  virtual bool rename(WBContext *wb, const std::string &name) {
    //= bec::GRTManager::get();
    // QQQgrt->lock_tree_write();

    workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(object->owner()));
    grt::ListRef<GrtStoredNote> notes;

    if (object.is_instance(db_Script::static_class_name()))
      notes = grt::ListRef<GrtStoredNote>::cast_from(model->scripts());
    else
      notes = model->notes();

    for (size_t c = notes.count(), i = 0; i < c; i++) {
      GrtStoredNoteRef note(notes[i]);

      if (note != object && *note->name() == name) {
        // QQQgrt->unlock_tree_write();
        throw bec::validation_error(_("Duplicate object name."));
      }
    }

    grt::AutoUndo undo;
    object->name(name);
    undo.end(strfmt(_("Rename '%s' to '%s'"), object->name().c_str(), name.c_str()));
    // QQQgrt->unlock_tree_write();
    return true;
  }

  // XXX hack to remove Edit Notes... from script menu.. remove once bug is fixed
  virtual int get_popup_menu_items(WBContext *wb, bec::MenuItemList &items) {
    int c = OverviewBE::ObjectNode::get_popup_menu_items(wb, items);

    if (object.is_instance(db_Script::static_class_name())) {
      for (bec::MenuItemList::iterator iter = items.begin(); iter != items.end(); ++iter) {
        if (iter->internalName == "plugin:wb.plugin.edit.stored_note") {
          items.erase(iter);
          c--;
          break;
        }
      }
    }

    return c;
  }
};

SQLScriptsNode::SQLScriptsNode(workbench_physical_ModelRef model, PhysicalOverviewBE *owner)
  : ContainerNode(OverviewBE::OItem), _owner(owner), _model(model) {
  object = model;
  id = model->id() + "/scripts";

  type = OverviewBE::ODivision;
  label = _("SQL Scripts");
  expanded = false;
  display_mode = OverviewBE::MLargeIcon;

  refresh_children();
}

int SQLScriptsNode::get_popup_menu_items(WBContext *wb, bec::MenuItemList &items) {
  bec::MenuItem item;

  item.type = bec::MenuSeparator;
  items.push_back(item);

  item.type = bec::MenuAction;
  item.accessibilityName = "Add Script File";
  item.internalName = "builtin:add_script_file";
  item.caption = _("Add Script File...");
  items.push_back(item);

  return 2;
}

bool SQLScriptsNode::add_new(WBContext *wb) {
  WBComponentPhysical *compo = wb->get_component<WBComponentPhysical>();

  bec::GRTManager::get()->open_object_editor(compo->add_new_stored_script(_model, ""));
  return true;
}

static void script_object_changed(const std::string &member, const grt::ValueRef &value, PhysicalOverviewBE *owner) {
  if (member == "name")
    owner->send_refresh_scripts();
}

void SQLScriptsNode::refresh_children() {
  clear_children();
  if (_model->scripts().is_valid()) {
    for (size_t c = _model->scripts().count(), i = 0; i < c; i++) {
      db_ScriptRef script(_model->scripts()[i]);
      ModelObjectNode *node = new ModelObjectNode();

      node->member = "scripts";
      node->label = script->name();
      node->object = script;
      node->small_icon = IconManager::get_instance()->get_icon_id(script.get_metaclass(), Icon16);
      node->large_icon = IconManager::get_instance()->get_icon_id(script.get_metaclass(), Icon48);

      script->signal_changed()->connect(
        std::bind(script_object_changed, std::placeholders::_1, std::placeholders::_2, _owner));
      children.push_back(node);
    }

    OverviewBE::AddObjectNode *add_node =
      new OverviewBE::AddObjectNode(std::bind(&SQLScriptsNode::add_new, this, std::placeholders::_1));

    add_node->label = _("Add Script");
    add_node->small_icon = IconManager::get_instance()->get_icon_id("db.Script.$.png", Icon16, "add");
    add_node->large_icon = IconManager::get_instance()->get_icon_id("db.Script.$.png", Icon48, "add");
    children.insert(children.begin(), add_node);
  }
}

NotesNode::NotesNode(workbench_physical_ModelRef model, PhysicalOverviewBE *owner)
  : ContainerNode(OverviewBE::OItem), _owner(owner), _model(model) {
  object = model;
  id = model->id() + "/notes";

  type = OverviewBE::ODivision;
  label = _("Model Notes");
  expanded = false;
  display_mode = OverviewBE::MLargeIcon;

  refresh_children();
}

int NotesNode::get_popup_menu_items(WBContext *wb, bec::MenuItemList &items) {
  bec::MenuItem item;

  item.type = bec::MenuSeparator;
  items.push_back(item);

  item.type = bec::MenuAction;
  item.accessibilityName = "Add Note File";
  item.internalName = "builtin:add_note_file";
  item.caption = _("Add Note File...");
  items.push_back(item);

  return 2;
}

bool NotesNode::add_new(WBContext *wb) {
  WBComponentPhysical *compo = wb->get_component<WBComponentPhysical>();

  bec::GRTManager::get()->open_object_editor(compo->add_new_stored_note(_model, ""));
  return true;
}

static void note_object_changed(const std::string &member, const grt::ValueRef &value, PhysicalOverviewBE *owner) {
  if (member == "name")
    owner->send_refresh_notes();
}

void NotesNode::refresh_children() {
  clear_children();
  if (_model->notes().is_valid()) {
    for (size_t c = _model->notes().count(), i = 0; i < c; i++) {
      GrtStoredNoteRef note(_model->notes()[i]);
      ModelObjectNode *node = new ModelObjectNode();

      node->member = "notes";
      node->label = note->name();
      node->object = note;
      node->small_icon = IconManager::get_instance()->get_icon_id(note.get_metaclass(), Icon16);
      node->large_icon = IconManager::get_instance()->get_icon_id(note.get_metaclass(), Icon48);

      note->signal_changed()->connect(
        std::bind(note_object_changed, std::placeholders::_1, std::placeholders::_2, _owner));
      children.push_back(node);
    }

    OverviewBE::AddObjectNode *add_node =
      new OverviewBE::AddObjectNode(std::bind(&NotesNode::add_new, this, std::placeholders::_1));
    add_node->label = _("Add Note");
    add_node->small_icon = IconManager::get_instance()->get_icon_id("GrtStoredNote.$.png", Icon16, "add");
    add_node->large_icon = IconManager::get_instance()->get_icon_id("GrtStoredNote.$.png", Icon48, "add");
    children.insert(children.begin(), add_node);
  }
}

class PhysicalRootNode : public OverviewBE::ContainerNode {
public:
  PhysicalRootNode(workbench_physical_ModelRef model, PhysicalOverviewBE *owner)
    : ContainerNode(OverviewBE::ODivision) {
    type = OverviewBE::ORoot;
    if (model->rdbms().is_valid())
      label = strfmt("%s Model", model->rdbms()->caption().c_str());
    expanded = true;
    object = model;
    display_mode = OverviewBE::MSmallIcon;

    children.push_back(new DiagramListNode(model));

    {
      internal::PhysicalSchemataNode *node = new internal::PhysicalSchemataNode(model);
      node->init();
      children.push_back(node);
    }

    children.push_back(new internal::PrivilegeInfoNode(model->catalog(), owner));

    children.push_back(new internal::SQLScriptsNode(model, owner));
    children.push_back(new internal::NotesNode(model, owner));
  }
};

//----------------- PhysicalOverviewBE -------------------------------------------------------------

PhysicalOverviewBE::PhysicalOverviewBE(WBContext *wb) : OverviewBE(wb), _menu(0), _toolbar(0) {
  _schemata_node_index = 1;
  _menu = 0;
  NotificationCenter::get()->add_observer(this, "GNColorsChanged");
}

//--------------------------------------------------------------------------------------------------

PhysicalOverviewBE::~PhysicalOverviewBE() {
  NotificationCenter::get()->remove_observer(this);
  delete _toolbar;
  delete _menu;
}

//--------------------------------------------------------------------------------------------------

void PhysicalOverviewBE::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  if (name == "GNColorsChanged") {
    // Single colors or the entire color scheme changed.
    update_toolbar_icons();
  }
}

//--------------------------------------------------------------------------------------------------

mforms::ToolBar *PhysicalOverviewBE::get_toolbar() {
  if (!_toolbar) {
    _toolbar = wb::WBContextUI::get()->get_command_ui()->create_toolbar("data/model_toolbar.xml");
    update_toolbar_icons();
  }
  return _toolbar;
}

//--------------------------------------------------------------------------------------------------

// Implemented in wb_sql_editor_form_ui.cpp.
extern std::string find_icon_name(std::string icon_name, bool use_win8);

void PhysicalOverviewBE::update_toolbar_icons() {
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

//--------------------------------------------------------------------------------------------------

OverviewBE::ContainerNode *PhysicalOverviewBE::create_root_node(workbench_physical_ModelRef model,
                                                                PhysicalOverviewBE *owner) {
  return new PhysicalRootNode(model, owner);
}

bool PhysicalOverviewBE::can_close() {
  return _wb->can_close_document();
}

void PhysicalOverviewBE::close() {
  _wb->close_document();
}

std::string PhysicalOverviewBE::identifier() const {
  return "overview.physical";
}

std::string PhysicalOverviewBE::get_form_context_name() const {
  return WB_CONTEXT_PHYSICAL_OVERVIEW;
}

std::string PhysicalOverviewBE::get_title() {
  const char *dirty_mark = "";
  if (_wb->has_unsaved_changes())
    dirty_mark = "*";
  if (_model.is_valid() && _model->rdbms().is_valid())
    return strfmt(_("%s Model%s"), _model->rdbms()->caption().c_str(), dirty_mark);
  return std::string(_("MySQL Model")) + dirty_mark;
}

void PhysicalOverviewBE::set_model(workbench_physical_ModelRef model) {
  if (_root_node)
    delete _root_node;

  _model = model;
  _root_node = create_root_node(model, this);

  tree_changed();
}

model_ModelRef PhysicalOverviewBE::get_model() {
  return _model;
}

int PhysicalOverviewBE::get_default_tab_page_index() {
  return (int)_model->catalog()->schemata().get_index(_model->catalog()->defaultSchema());
}

static bool has_selection(PhysicalOverviewBE *overview) {
  grt::ListRef<GrtObject> selection(overview->get_selection());
  return selection.is_valid() && selection.count() > 0;
}

mforms::MenuBar *PhysicalOverviewBE::get_menubar() {
  if (!_menu) {
    _menu = wb::WBContextUI::get()->get_command_ui()->create_menubar_for_context(WB_CONTEXT_PHYSICAL_OVERVIEW);

    static const char *diagram_only_items[] = {
        "diagram_size",
        "fnotation",
        "rnotation",
        "wb.edit.goToNextSelected",
        "wb.edit.goToPreviousSelected",
        "wb.edit.selectSimilar",
        "wb.edit.selectConnected",
        "wb.edit.toggleGridAlign",
        "wb.edit.toggleGrid",
        "wb.edit.togglePageGrid",
        "wb.view.zoomDefault",
        "wb.view.zoomIn",
        "wb.view.zoomOut",
        "wb.view.setFigureNotation",
        "wb.view.setRelationshipNotation",
        "wb.view.setMarker:1",
        "wb.view.setMarker:2",
        "wb.view.setMarker:3",
        "wb.view.setMarker:4",
        "wb.view.setMarker:5",
        "wb.view.setMarker:6",
        "wb.view.setMarker:7",
        "wb.view.setMarker:8",
        "wb.view.setMarker:9",
        "wb.view.goToMarker:1",
        "wb.view.goToMarker:2",
        "wb.view.goToMarker:3",
        "wb.view.goToMarker:4",
        "wb.view.goToMarker:5",
        "wb.view.goToMarker:6",
        "wb.view.goToMarker:7",
        "wb.view.goToMarker:8",
        "wb.view.goToMarker:9",
        nullptr
    };

    for (int i = 0; diagram_only_items[i]; i++)
      _menu->set_item_enabled(diagram_only_items[i], false);

    std::vector<mforms::MenuItem *> items(_menu->find_item("arrange")->get_subitems());
    for (std::vector<mforms::MenuItem *>::const_iterator i = items.begin(); i != items.end(); ++i) {
      (*i)->set_enabled(false);
    }

    mforms::MenuItem *item = _menu->find_item("wb.edit.editObject");
    if (item)
      item->add_validator(std::bind(has_selection, this));
    item = _menu->find_item("wb.edit.editObjectInNewWindow");
    if (item)
      item->add_validator(std::bind(has_selection, this));
  }
  return _menu;
}

internal::PhysicalSchemaNode *PhysicalOverviewBE::get_active_schema_node() {
  NodeId node(get_focused_child(NodeId(_schemata_node_index)));

  if (node.is_valid())
    return dynamic_cast<PhysicalSchemaNode *>(get_node_by_id(node));
  return 0;
}

void PhysicalOverviewBE::send_refresh_diagram(const model_DiagramRef &view) {
  if (view.is_valid()) {
    NodeId node = get_node_child_for_object(NodeId(0), view);

    send_refresh_node(node);
  } else {
    send_refresh_children(NodeId(0));
  }
}

void PhysicalOverviewBE::send_refresh_roles() {
  send_refresh_children(NodeId(2).append(1));
}

void PhysicalOverviewBE::send_refresh_users() {
  send_refresh_children(NodeId(2).append(0));
}

void PhysicalOverviewBE::send_refresh_scripts() {
  send_refresh_children(SCRIPT_NODE);
}

void PhysicalOverviewBE::send_refresh_notes() {
  send_refresh_children(NOTE_NODE);
}

void PhysicalOverviewBE::send_refresh_schema_list() {
  send_refresh_children(NodeId(_schemata_node_index));
}

void PhysicalOverviewBE::send_refresh_for_schema(const db_SchemaRef &schema, bool refresh_object_itself) {
  NodeId schema_node = get_node_child_for_object(NodeId(_schemata_node_index), schema);

  if (schema_node.is_valid() && refresh_object_itself)
    send_refresh_node(schema_node);
  else
    send_refresh_children(NodeId(_schemata_node_index));
}

void PhysicalOverviewBE::send_refresh_for_schema_object(const GrtObjectRef &object, bool refresh_object_itself) {
  NodeId schema_node;

  NodeId schemata_node = NodeId(_schemata_node_index);
  schema_node = get_node_child_for_object(schemata_node, object->owner());

  if (object.is_instance(db_Table::static_class_name()))
    schema_node.append(0);
  else if (object.is_instance(db_View::static_class_name()))
    schema_node.append(1);
  else if (object.is_instance(db_Routine::static_class_name()))
    schema_node.append(2);
  else if (object.is_instance(db_RoutineGroup::static_class_name()))
    schema_node.append(3);

  if (refresh_object_itself) {
    // find the object node in the list
    NodeId object_node = get_node_child_for_object(schema_node, object);

    if (object_node.is_valid())
      send_refresh_node(object_node);

    return;
  }

  if (schema_node.is_valid())
    send_refresh_children(schema_node);
}

void PhysicalOverviewBE::refresh_node(const bec::NodeId &node_id, bool children) {
  Node *node = get_node_by_id(node_id);
  if (node) {
    node->refresh();

    if (children) {
      OverviewBE::ContainerNode *n = dynamic_cast<OverviewBE::ContainerNode *>(node);
      if (n)
        n->refresh_children();
    }
  }
}

std::string PhysicalOverviewBE::get_node_drag_type(const bec::NodeId &node) {
  // for schema objects
  if (node.depth() > 1 && node[0] == 1)
    return WB_DBOBJECT_DRAG_TYPE;
  else if (node == SCRIPT_NODE)
    return "file";
  else if (node == NOTE_NODE)
    return "file";
  return OverviewBE::get_node_drag_type(node);
}

bool PhysicalOverviewBE::should_accept_file_drop_to_node(const bec::NodeId &node, const std::string &path) {
  return true;
}

void PhysicalOverviewBE::add_file_to_node(const bec::NodeId &node, const std::string &path) {
  if (node == SCRIPT_NODE)
    _wb->get_component<WBComponentPhysical>()->add_new_stored_script(_model, path);
  else if (node == NOTE_NODE)
    _wb->get_component<WBComponentPhysical>()->add_new_stored_note(_model, path);
  else
    throw std::logic_error("Cannot add file to node");
}

bool PhysicalOverviewBE::get_file_data_for_node(const bec::NodeId &node, char *&data, size_t &length) {
  GrtStoredNoteRef note(GrtStoredNoteRef::cast_from(get_node_by_id(node)->object));

  data = 0;

  if (note.is_valid()) {
    std::string text = _wb->get_attached_file_contents(note->filename());

    data = (char *)g_memdup(text.data(), (int)text.size());
    length = text.size();

    return true;
  }

  return false;
}

std::string PhysicalOverviewBE::get_file_for_node(const bec::NodeId &node) {
  GrtStoredNoteRef note(GrtStoredNoteRef::cast_from(get_node_by_id(node)->object));

  if (note.is_valid())
    return _wb->get_attached_file_tmp_path(note->filename());
  return "";
}

bool PhysicalOverviewBE::can_undo() {
  return grt::GRT::get()->get_undo_manager()->can_undo();
}

bool PhysicalOverviewBE::can_redo() {
  return grt::GRT::get()->get_undo_manager()->can_redo();
}

void PhysicalOverviewBE::undo() {
  grt::GRT::get()->get_undo_manager()->undo();
}

void PhysicalOverviewBE::redo() {
  grt::GRT::get()->get_undo_manager()->redo();
}
