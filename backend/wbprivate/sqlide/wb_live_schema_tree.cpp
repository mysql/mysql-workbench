/*
 * Copyright (c) 2009, 2022, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_live_schema_tree.h"
#include "grtdb/charset_utils.h"
#include "grt/icon_manager.h"

#include "grts/structs.db.query.h"

#include "base/string_utilities.h"
#include "base/log.h"

#include "mforms/app.h"
#include <boost/make_shared.hpp>

using namespace wb;
using namespace bec;
using namespace base;
using namespace grt;

DEFAULT_LOG_DOMAIN("SqlEditorSchemaTree");

const short LiveSchemaTree::COLUMN_DATA = 0x01;
const short LiveSchemaTree::TRIGGER_DATA = 0x02;
const short LiveSchemaTree::INDEX_DATA = 0x04;
const short LiveSchemaTree::FK_DATA = 0x08;

const std::string LiveSchemaTree::SCHEMA_TAG = "_SCHEMA_";
const std::string LiveSchemaTree::TABLES_TAG = "_TABLES_";
const std::string LiveSchemaTree::VIEWS_TAG = "_VIEWS_";
const std::string LiveSchemaTree::PROCEDURES_TAG = "_PROCEDURES_";
const std::string LiveSchemaTree::FUNCTIONS_TAG = "_FUNCTIONS_";
const std::string LiveSchemaTree::TABLE_TAG = "_TABLE_";
const std::string LiveSchemaTree::VIEW_TAG = "_VIEW_";
const std::string LiveSchemaTree::ROUTINE_TAG = "_ROUTINE_";
const std::string LiveSchemaTree::COLUMNS_TAG = "_COLUMNS_";
const std::string LiveSchemaTree::INDEXES_TAG = "_INDEXES_";
const std::string LiveSchemaTree::TRIGGERS_TAG = "_TRIGGERS_";
const std::string LiveSchemaTree::FOREIGN_KEYS_TAG = "_FOREIGN_KEYS_";
const std::string LiveSchemaTree::COLUMN_TAG = "_COLUMN_";
const std::string LiveSchemaTree::INDEX_TAG = "_INDEX_";
const std::string LiveSchemaTree::TRIGGER_TAG = "_TRIGGER_";
const std::string LiveSchemaTree::FOREIGN_KEY_TAG = "_FOREIGN_KEY_";

const std::string LiveSchemaTree::FETCHING_CAPTION = _("fetching...");
const std::string LiveSchemaTree::ERROR_FETCHING_CAPTION = _("could not be fetched");
const std::string LiveSchemaTree::TABLES_CAPTION = _("Tables");
const std::string LiveSchemaTree::VIEWS_CAPTION = _("Views");
const std::string LiveSchemaTree::PROCEDURES_CAPTION = _("Stored Procedures");
const std::string LiveSchemaTree::FUNCTIONS_CAPTION = _("Functions");

const std::string LiveSchemaTree::COLUMNS_CAPTION = _("Columns");
const std::string LiveSchemaTree::INDEXES_CAPTION = _("Indexes");
const std::string LiveSchemaTree::TRIGGERS_CAPTION = _("Triggers");
const std::string LiveSchemaTree::FOREIGN_KEYS_CAPTION = _("Foreign Keys");

const std::string LiveSchemaTree::LST_INFO_BOX_DETAIL_ROW =
  "<tr>"
  "<td style=\"border:none; padding-left: 15px;\">%s</td>"
  "<td style=\"border:none; padding-left: 15px;\"><font color='#717171'>%s</font></td>"
  "</tr>";

const int LiveSchemaTree::TABLES_NODE_INDEX = 0;
const int LiveSchemaTree::VIEWS_NODE_INDEX = 1;
const int LiveSchemaTree::PROCEDURES_NODE_INDEX = 2;
const int LiveSchemaTree::FUNCTIONS_NODE_INDEX = 3;
const int LiveSchemaTree::TABLE_COLUMNS_NODE_INDEX = 0;
const int LiveSchemaTree::TABLE_INDEXES_NODE_INDEX = 1;
const int LiveSchemaTree::TABLE_FOREIGN_KEYS_NODE_INDEX = 2;
const int LiveSchemaTree::TABLE_TRIGGERS_NODE_INDEX = 3;

const char* LiveSchemaTree::_schema_tokens[16] = {
  "",                                                            // Empty item to use 0 as not found
  "CASCADE", "SET NULL", "SET DEFAULT", "RESTRICT", "NO ACTION", // The update/delete rules on foreign keys
  "BTREE",   "FULLTEXT", "HASH",        "RTREE",    "SPATIAL",   // The index types
  "INSERT",  "UPDATE",   "DELETE",                               // Trigger events
  "BEFORE",  "AFTER"};                                           // Trigger timing

LiveSchemaTree::LSTData::LSTData() : details(""){};

void LiveSchemaTree::LSTData::copy(LSTData* other) {
  this->details = other->details;
}

std::string LiveSchemaTree::LSTData::get_details(bool full, const mforms::TreeNodeRef& node) {
  std::string ret_val("");

  if (full) {
    std::string owner_name = node->get_string(0);

    ret_val = strfmt(_("<b>%s:</b> <font color='#148814'><b>%s</b></font><br><br>"), get_object_name().c_str(),
                     owner_name.c_str());
  } else
    ret_val = details;

  return ret_val;
}

void LiveSchemaTree::ColumnData::copy(LSTData* other) {
  LSTData::copy(other);

  ColumnData* pother = dynamic_cast<ColumnData*>(other);

  if (pother) {
    this->name = pother->name;
    this->type = pother->type;
    this->default_value = pother->default_value;
    this->charset_collation = pother->charset_collation;
    this->is_id = pother->is_id;
    this->is_pk = pother->is_pk;
    this->is_fk = pother->is_fk;
    this->is_idx = pother->is_idx;
  }
}

std::string LiveSchemaTree::ColumnData::get_details(bool full, const mforms::TreeNodeRef& node) {
  std::string ret_val("");

  if (details.empty()) {
    std::string html_name = name;
    if (is_pk || is_idx) {
      if (is_pk)
        html_name = "<u>" + html_name + "</u>";

      html_name = "<b>" + html_name + "</b>";
    }

    std::string html_type = type;
    if (is_pk)
      html_type += " PK";

    details += strfmt(LST_INFO_BOX_DETAIL_ROW.c_str(), html_name.c_str(), html_type.c_str());
  }

  if (full) {
    ret_val = LSTData::get_details(full, node);
    if (!charset_collation.empty()) {
      ret_val += _("<b>Collation:</b>  ");
      ret_val += charset_collation;
      ret_val += "<br><br>";
    }
    ret_val += _("<b>Definition:</b><table style=\"border: none; border-collapse: collapse;\">");
    ret_val += details;
    ret_val += "</table><br><br>";
  } else
    ret_val = details;

  return ret_val;
}

void LiveSchemaTree::FKData::copy(LSTData* other) {
  LSTData::copy(other);

  FKData* pother = dynamic_cast<FKData*>(other);

  if (pother) {
    this->referenced_table = pother->referenced_table;
    this->from_cols = pother->from_cols;
    this->to_cols = pother->to_cols;
    this->update_rule = pother->update_rule;
    this->delete_rule = pother->delete_rule;
  }
}

std::string LiveSchemaTree::FKData::get_details(bool full, const mforms::TreeNodeRef& node) {
  std::string ret_val("");

  if (details.empty()) {
    std::string target =
      strfmt("%s (%s \xE2\x86\x92 %s)", referenced_table.c_str(), from_cols.c_str(), to_cols.c_str());
    details = "<table style=\"border: none; border-collapse: collapse;\">";
    details += strfmt(LST_INFO_BOX_DETAIL_ROW.c_str(), "Target", target.c_str());
    details += strfmt(LST_INFO_BOX_DETAIL_ROW.c_str(), "On Update", externalize_token(update_rule).c_str());
    details += strfmt(LST_INFO_BOX_DETAIL_ROW.c_str(), "On Delete", externalize_token(delete_rule).c_str());
    details += "</table>";
  }

  if (full) {
    ret_val = LSTData::get_details(full, node);
    ret_val += _("<b>Definition:</b><br>");
    ret_val += details;
  } else
    ret_val = details;

  return ret_val;
}

void LiveSchemaTree::IndexData::copy(LSTData* other) {
  LSTData::copy(other);

  IndexData* pother = dynamic_cast<IndexData*>(other);

  if (pother) {
    this->columns.assign(pother->columns.begin(), pother->columns.end());
    this->type = pother->type;
    this->unique = pother->unique;
    this->visible = pother->visible;
  }
}

std::string LiveSchemaTree::IndexData::get_details(bool full, const mforms::TreeNodeRef& node) {
  std::string ret_val("");

  if (details.empty()) {
    details = "<table style=\"border: none; border-collapse: collapse;\">";
    details += strfmt(LST_INFO_BOX_DETAIL_ROW.c_str(), "Type", externalize_token(type).c_str());
    details += strfmt(LST_INFO_BOX_DETAIL_ROW.c_str(), "Unique", unique ? "Yes" : "No");
    details += strfmt(LST_INFO_BOX_DETAIL_ROW.c_str(), "Visible", visible ? "Yes" : "No");
    details += strfmt(LST_INFO_BOX_DETAIL_ROW.c_str(), "Columns", columns[0].c_str());

    for (std::size_t index = 1; index < columns.size(); index++)
      details += strfmt(LST_INFO_BOX_DETAIL_ROW.c_str(), "", columns[index].c_str());

    details += "</table>";
  }

  if (full) {
    ret_val = LSTData::get_details(full, node);
    ret_val += _("<b>Definition:</b><br>");
    ret_val += details;
  } else
    ret_val = details;

  return ret_val;
}

void LiveSchemaTree::TriggerData::copy(LSTData* other) {
  LSTData::copy(other);

  TriggerData* pother = dynamic_cast<TriggerData*>(other);

  if (pother) {
    this->event_manipulation = pother->event_manipulation;
    this->timing = pother->timing;
  }
}

std::string LiveSchemaTree::TriggerData::get_details(bool full, const mforms::TreeNodeRef& node) {
  std::string ret_val("");

  if (details.empty()) {
    details = "<table style=\"border: none; border-collapse: collapse;\">";
    details += strfmt(LST_INFO_BOX_DETAIL_ROW.c_str(), "Event", externalize_token(event_manipulation).c_str());
    details += strfmt(LST_INFO_BOX_DETAIL_ROW.c_str(), "Timing", externalize_token(timing).c_str());
    details += "</table>";
  }

  if (full) {
    ret_val = LSTData::get_details(full, node);
    ret_val += _("<b>Definition:</b><br>");
    ret_val += details;
  } else
    ret_val = details;

  return ret_val;
}

void LiveSchemaTree::ObjectData::copy(LSTData* other) {
  LSTData::copy(other);

  ObjectData* pother = dynamic_cast<ObjectData*>(other);

  if (pother) {
    this->fetched = pother->fetched;
    this->fetching = pother->fetching;
  }
}

void LiveSchemaTree::ViewData::copy(LSTData* other) {
  ObjectData::copy(other);

  ViewData* pother = dynamic_cast<ViewData*>(other);

  if (pother) {
    this->_loaded_mask = pother->_loaded_mask;
    this->_loading_mask = pother->_loading_mask;
    this->columns_load_error = pother->columns_load_error;
  }
}

std::string LiveSchemaTree::ViewData::get_details(bool full, const mforms::TreeNodeRef& node) {
  std::string ret_val;

  if (full)
    ret_val = LSTData::get_details(full, node);

  if (_loaded_mask & COLUMN_DATA) {
    int count = (get_type() == Table) ? node->get_child(TABLE_COLUMNS_NODE_INDEX)->count() : node->count();
    if (count) {
      ret_val += _("<b>Columns:</b><table style=\"border: none; border-collapse: collapse;\">");

      for (int index = 0; index < count; index++) {
        ColumnData* pdata;
        if (get_type() == Table)
          pdata = dynamic_cast<ColumnData*>(node->get_child(TABLE_COLUMNS_NODE_INDEX)->get_child(index)->get_data());
        else
          pdata = dynamic_cast<ColumnData*>(node->get_child(index)->get_data());

        ret_val += pdata->get_details(false, node);
      }

      ret_val += "</table><br><br>";
    }
  }

  // Appends the error message if needed
  if (columns_load_error)
    ret_val += details;

  return ret_val;
}

std::string LiveSchemaTree::TableData::get_details(bool full, const mforms::TreeNodeRef& node) {
  std::string ret_val = ViewData::get_details(full, node);

  if (_loaded_mask & FK_DATA) {
    int count = node->get_child(TABLE_FOREIGN_KEYS_NODE_INDEX)->count();
    if (count) {
      ret_val += _("<div><b>Related Tables:</b></div>");

      for (int index = 0; index < count; index++) {
        FKData* pdata =
          dynamic_cast<FKData*>(node->get_child(TABLE_FOREIGN_KEYS_NODE_INDEX)->get_child(index)->get_data());

        ret_val += pdata->get_details(false, node);
      }
    }
  }

  return ret_val;
}

void LiveSchemaTree::SchemaData::copy(LSTData* other) {
  LSTData::copy(other);

  SchemaData* pother = dynamic_cast<SchemaData*>(other);

  if (pother) {
    this->fetched = pother->fetched;
    this->fetching = pother->fetching;
  }
}

//--------------------------------------------------------------------------------------------------

short LiveSchemaTree::ViewData::get_loaded_mask() {
  return _loaded_mask;
}

//--------------------------------------------------------------------------------------------------

void LiveSchemaTree::ViewData::set_unloaded_data(short mask) {
  // Gets the data that is loaded and needs to be marked as unloaded
  short filter = _loaded_mask & mask;

  _loaded_mask -= filter;
}

void LiveSchemaTree::ViewData::set_loaded_data(short mask) {
  // Gets the data that is being marked as loaded and was loading
  short filter = mask & _loading_mask;
  _loading_mask -= filter;

  // Now marks the data as loaded
  _loaded_mask |= mask;
}

short LiveSchemaTree::ViewData::get_loading_mask() {
  return _loading_mask;
}

void LiveSchemaTree::ViewData::set_loading_mask(short mask) {
  _loading_mask = mask;
}

bool LiveSchemaTree::ViewData::is_data_loaded(short mask) {
  return (_loaded_mask & mask) == mask;
}

bool LiveSchemaTree::ViewData::is_update_complete() {
  bool ret_val = false;

  if (_reload_mask) {
    short loaded_mask = get_loaded_mask();

    if ((loaded_mask & _reload_mask) == _reload_mask) {
      _reload_mask = 0;
      ret_val = true;
    }
  }

  return ret_val;
}

std::string LiveSchemaTree::ProcedureData::get_details(bool full, const mforms::TreeNodeRef& node) {
  std::string ret_val = ObjectData::get_details(true, node);
  ret_val += ObjectData::get_details(false, node);

  return ret_val;
}

std::string LiveSchemaTree::FunctionData::get_details(bool full, const mforms::TreeNodeRef& node) {
  std::string ret_val = ObjectData::get_details(true, node);
  ret_val += ObjectData::get_details(false, node);

  return ret_val;
}
//--------------------------------------------------------------------------------------------------

LiveSchemaTree::LiveSchemaTree(MySQLVersion version)
  : _model_view(nullptr),
    _schema_pattern(0),
    _object_pattern(0),
    _case_sensitive_identifiers(false),
    _is_schema_contents_enabled(true),
    _enabled_events(false),
    _version(version),
    _base(0),
    _filter(),
    _filter_type(Any) {
  fill_node_icons();

  // Setup the schema node collection skeleton
  mforms::TreeNodeCollectionSkeleton schema_nodes(_icon_paths[Schema]);

  mforms::TreeNodeSkeleton tables(TABLES_CAPTION, _icon_paths[TableCollection], TABLES_TAG);
  schema_nodes.children.push_back(tables);

  mforms::TreeNodeSkeleton views(VIEWS_CAPTION, _icon_paths[ViewCollection], VIEWS_TAG);
  schema_nodes.children.push_back(views);

  mforms::TreeNodeSkeleton procedures(PROCEDURES_CAPTION, _icon_paths[ProcedureCollection], PROCEDURES_TAG);
  schema_nodes.children.push_back(procedures);

  mforms::TreeNodeSkeleton functions(FUNCTIONS_CAPTION, _icon_paths[FunctionCollection], FUNCTIONS_TAG);
  schema_nodes.children.push_back(functions);
  _node_collections[Schema] = schema_nodes;

  // Setup the table node collection skeleton
  mforms::TreeNodeCollectionSkeleton table_nodes(_icon_paths[Table]);

  mforms::TreeNodeSkeleton columns(COLUMNS_CAPTION, _icon_paths[ColumnCollection], COLUMNS_TAG);
  table_nodes.children.push_back(columns);

  mforms::TreeNodeSkeleton indexes(INDEXES_CAPTION, _icon_paths[IndexCollection], INDEXES_TAG);
  table_nodes.children.push_back(indexes);

  mforms::TreeNodeSkeleton foreign_keys(FOREIGN_KEYS_CAPTION, _icon_paths[ForeignKeyCollection], FOREIGN_KEYS_TAG);
  mforms::TreeNodeSkeleton fetching_fk(FETCHING_CAPTION, _icon_paths[ForeignKey], "");
  foreign_keys.children.push_back(fetching_fk);
  table_nodes.children.push_back(foreign_keys);

  mforms::TreeNodeSkeleton triggers(TRIGGERS_CAPTION, _icon_paths[TriggerCollection], TRIGGERS_TAG);
  mforms::TreeNodeSkeleton fetching_trigger(FETCHING_CAPTION, _icon_paths[Trigger], "");
  triggers.children.push_back(fetching_trigger);
  table_nodes.children.push_back(triggers);

  _node_collections[Table] = table_nodes;

  // Setup the view node collection skeleton
  mforms::TreeNodeCollectionSkeleton view_nodes(_icon_paths[View]);
  mforms::TreeNodeSkeleton fetching_view(FETCHING_CAPTION, _icon_paths[View], "");
  view_nodes.children.push_back(fetching_view);
  _node_collections[View] = view_nodes;

  mforms::TreeNodeCollectionSkeleton procedure_nodes(_icon_paths[Procedure]);
  _node_collections[Procedure] = procedure_nodes;

  mforms::TreeNodeCollectionSkeleton function_nodes(_icon_paths[Function]);
  _node_collections[Function] = function_nodes;

  mforms::TreeNodeCollectionSkeleton table_column_nodes(_icon_paths[TableColumn]);
  _node_collections[TableColumn] = table_column_nodes;

  mforms::TreeNodeCollectionSkeleton view_column_nodes(_icon_paths[ViewColumn]);
  _node_collections[ViewColumn] = view_column_nodes;

  mforms::TreeNodeCollectionSkeleton index_nodes(_icon_paths[Index]);
  _node_collections[Index] = index_nodes;

  mforms::TreeNodeCollectionSkeleton trigger_nodes(_icon_paths[Trigger]);
  _node_collections[Trigger] = trigger_nodes;

  mforms::TreeNodeCollectionSkeleton fk_nodes(_icon_paths[ForeignKey]);
  _node_collections[ForeignKey] = fk_nodes;
}

LiveSchemaTree::~LiveSchemaTree() {
  clean_filter();
}

void LiveSchemaTree::set_fetch_delegate(std::shared_ptr<FetchDelegate> delegate) {
  _fetch_delegate = delegate;
}

void LiveSchemaTree::set_delegate(std::shared_ptr<Delegate> delegate) {
  _delegate = delegate;
}

unsigned char LiveSchemaTree::internalize_token(const std::string& token) {
  unsigned char found_index = 0;
  for (unsigned char index = 1; !found_index && (index < (sizeof(_schema_tokens) / sizeof(char*))); index++) {
    if (token == _schema_tokens[index])
      found_index = index;
  }

  return found_index;
}

std::string LiveSchemaTree::externalize_token(unsigned char c) {
  return (c > 0 && c < (sizeof(_schema_tokens) / sizeof(char*))) ? _schema_tokens[c] : "";
}

void LiveSchemaTree::set_case_sensitive_identifiers(bool flag) {
  _case_sensitive_identifiers = flag;
}

bool LiveSchemaTree::identifiers_equal(const std::string& a, const std::string& b) {
  return base::string_compare(a, b, _case_sensitive_identifiers) == 0;
}

void LiveSchemaTree::setup_node(mforms::TreeNodeRef node, ObjectType type, mforms::TreeNodeData* pdata,
                                bool ignore_null_data) {
  switch (type) {
    case Schema:
      node->set_data(pdata ? pdata : new SchemaData());
      break;
    case Table:
      node->set_data(pdata ? pdata : new TableData());
      break;
    case View:
      node->set_data(pdata ? pdata : new ViewData());
      break;
    case Procedure:
      node->set_data(pdata ? pdata : new ProcedureData());
      break;
    case Function:
      node->set_data(pdata ? pdata : new FunctionData());
      break;
    case ViewColumn:
      if (pdata || !ignore_null_data)
        node->set_data(pdata ? pdata : new ColumnData(type));
      break;
    case TableColumn:
      node->set_data(pdata ? pdata : new ColumnData(type));
      break;
    case Index:
      node->set_data(pdata ? pdata : new IndexData());
      break;
    case Trigger:
      if (pdata || !ignore_null_data)
        node->set_data(pdata ? pdata : new TriggerData());
      break;
    case ForeignKey:
      if (pdata || !ignore_null_data)
        node->set_data(pdata ? pdata : new FKData());
      break;
    default:
      break;
  }
}

void LiveSchemaTree::fill_node_icons() {
  _icon_paths[Schema] = get_node_icon_path(Schema);
  _icon_paths[TableCollection] = get_node_icon_path(TableCollection);
  _icon_paths[ViewCollection] = get_node_icon_path(ViewCollection);
  _icon_paths[ProcedureCollection] = get_node_icon_path(ProcedureCollection);
  _icon_paths[FunctionCollection] = get_node_icon_path(FunctionCollection);
  _icon_paths[Table] = get_node_icon_path(Table);
  _icon_paths[View] = get_node_icon_path(View);
  _icon_paths[Procedure] = get_node_icon_path(Procedure);
  _icon_paths[Function] = get_node_icon_path(Function);
  _icon_paths[ColumnCollection] = get_node_icon_path(ColumnCollection);
  _icon_paths[IndexCollection] = get_node_icon_path(IndexCollection);
  _icon_paths[ForeignKeyCollection] = get_node_icon_path(ForeignKeyCollection);
  _icon_paths[TriggerCollection] = get_node_icon_path(TriggerCollection);
  _icon_paths[ViewColumn] = get_node_icon_path(ViewColumn);
  _icon_paths[TableColumn] = get_node_icon_path(TableColumn);
  _icon_paths[Index] = get_node_icon_path(Index);
  _icon_paths[ForeignKey] = get_node_icon_path(ForeignKey);
  _icon_paths[Trigger] = get_node_icon_path(Trigger);
}

std::string LiveSchemaTree::get_node_icon_path(ObjectType type) {
  bec::IconId icon = get_node_icon(type);
  return bec::IconManager::get_instance()->get_icon_file(icon);
}

bec::IconId LiveSchemaTree::get_node_icon(ObjectType type) {
  bec::IconId icon;

  switch (type) {
    case Schema:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Schema.unloaded.side.$.png", bec::Icon16);
      break;
    case TableCollection:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Table.many.side.$.png", bec::Icon16);
      break;
    case ViewCollection:
      icon = bec::IconManager::get_instance()->get_icon_id("db.View.many.side.$.png", bec::Icon16);
      break;
    // TODO: Update the Procedure and Function collection icons to the correct value
    case ProcedureCollection:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Routine.many.side.$.png", bec::Icon16);
      break;
    case FunctionCollection:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Routine.many.side.$.png", bec::Icon16);
      break;
    case Table:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Table.side.$.png", bec::Icon16);
      break;
    case View:
      icon = bec::IconManager::get_instance()->get_icon_id("db.View.side.$.png", bec::Icon16);
      break;
    case Procedure:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Routine.side.$.png", bec::Icon16);
      break;
    case Function:
      icon = bec::IconManager::get_instance()->get_icon_id("grt_function.png", bec::Icon16);
      break;
    case ColumnCollection:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Column.many.side.$.png", bec::Icon16);
      break;
    case IndexCollection:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Index.many.side.$.png", bec::Icon16);
      break;
    case ForeignKeyCollection:
      icon = bec::IconManager::get_instance()->get_icon_id("db.ForeignKey.many.side.$.png", bec::Icon16);
      break;
    case TriggerCollection:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Trigger.many.side.$.png", bec::Icon16);
      break;
    case ViewColumn:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Column.side.$.png", bec::Icon16);
      break;
    case TableColumn:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Column.side.$.png", bec::Icon16);
      break;
    case Index:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Index.side.$.png", bec::Icon16);
      break;
    case ForeignKey:
      icon = bec::IconManager::get_instance()->get_icon_id("db.ForeignKey.side.$.png", bec::Icon16);
      break;
    case Trigger:
      icon = bec::IconManager::get_instance()->get_icon_id("db.Trigger.side.$.png", bec::Icon16);
      break;
    default:
      icon = -1;
      break;
  }

  return icon;
}

void LiveSchemaTree::update_node_icon(mforms::TreeNodeRef node) {
  bec::IconId icon = 0;

  LSTData* pnode_data = dynamic_cast<LSTData*>(node->get_data());
  if (pnode_data) {
    switch (pnode_data->get_type()) {
      case Schema: {
        SchemaData* pdata = dynamic_cast<SchemaData*>(node->get_data());
        if (pdata->fetched)
          icon = bec::IconManager::get_instance()->get_icon_id("db.Schema.side.$.png", bec::Icon16);
        else if (pdata->fetching)
          icon = bec::IconManager::get_instance()->get_icon_id("db.Schema.loading.side.$.png", bec::Icon16);
        else
          icon = bec::IconManager::get_instance()->get_icon_id("db.Schema.unloaded.side.$.png", bec::Icon16);
      } break;

      case View: {
        ViewData* pdata = dynamic_cast<ViewData*>(node->get_data());

        if (pdata->columns_load_error)
          icon = bec::IconManager::get_instance()->get_icon_id("db.View.broken.side.$.png", bec::Icon16);
        else
          icon = bec::IconManager::get_instance()->get_icon_id("db.View.side.$.png", bec::Icon16);
      } break;
      case TableColumn: {
        ColumnData* pdata = dynamic_cast<ColumnData*>(node->get_data());

        if (pdata->is_pk)
          icon = bec::IconManager::get_instance()->get_icon_id("db.Column.pk.side.$.png", bec::Icon16);
        else if (pdata->is_fk)
          icon = bec::IconManager::get_instance()->get_icon_id("db.Column.fk.side.$.png", bec::Icon16);
      } break;
      // No other item changes icon
      default:
        break;
    }

    if (icon)
      node->set_icon_path(0, bec::IconManager::get_instance()->get_icon_file(icon));
  }
}

/*
 * Function: update_change_data
 * Description: Uses the received information to
 *              - Generate the list of nodes to be removed
 *              - Purge children to let there only the items to be added
 * Parameters:
 *   parent: the tree node for which the children list will be updated
 *   children: the list of names that will be used on the process
 *   type: the type of children that will be affected (some nodes may have children of different types)
 *   to_remove: a vector containing the nodes to be removed from the parent node
 */
void LiveSchemaTree::update_change_data(mforms::TreeNodeRef parent, base::StringListPtr children, ObjectType type,
                                        std::vector<mforms::TreeNodeRef>& to_remove) {
  mforms::TreeNodeRef node;

  int total_nodes = parent->count();
  if (total_nodes == 1 && parent->get_child(0)->get_string(0) == FETCHING_CAPTION)
    to_remove.push_back(parent->get_child(0));
  else {
    for (int index = 0; index < total_nodes; index++) {
      node = parent->get_child(index);
      LSTData* pchild_data = dynamic_cast<LSTData*>(node->get_data());

      // Ensure only items of the same type are analyzed
      if (pchild_data && pchild_data->get_type() == type) {
        std::list<std::string>::iterator found_child =
          std::find(children->begin(), children->end(), node->get_string(0));

        // If not found, the item will be removed
        if (found_child == children->end())
          to_remove.push_back(node);
        // If found, the item is removed from the incoming list
        // to let only nodes to be added
        else
          children->erase(found_child);
      }
    }
  }
}

/*
 * Function: update_node_children
 * Description: Updates the children of a node based on a received children name list:
 *              Removes children nodes not contained on the received name list (if not just appending)
 *              Adds children nodes for names on the list for which a node is missing
 * Parameters:
 *   parent: the tree node for which the children list will be updated
 *   children: the list of names that will be used on the process
 *   type: the type of children that will be affected (some nodes may have children of different types)
 *   sorted: used to determine whether the nodes should be inserted in the proper position to keep the objects
 *           sorted or if they just need to be appended the way they come.
 *   just_append: suppresses the logic of removing nodes (i.e. when searching nodes with different filters
 *                they are only appended on the base tree)
 * Return Value: a boolean value indicating whether the children list changed (nodes were added or removed)
 *
 * NOTE : That children may change, so if the original list is needed, the caller needs to have a copy
 */
bool LiveSchemaTree::update_node_children(mforms::TreeNodeRef parent, base::StringListPtr children, ObjectType type,
                                          bool sorted, bool just_append) {
  bool ret_val = false;

  if (_base) {
    std::vector<std::string> path = get_node_path(parent);
    mforms::TreeNodeRef base_parent = _base->get_node_from_path(path);
    ret_val = _base->update_node_children(base_parent, children, type, sorted, just_append);

    // Filters the arrived data
    GPatternSpec* pattern = NULL;
    if (type == Schema)
      pattern = _schema_pattern;
    else if (is_object_type(SchemaObject, type))
      pattern = _object_pattern;

    filter_children(type, base_parent, parent, pattern);
  } else {
    mforms::TreeNodeRef node;
    bool added = false;
    bool removed = false;
    std::vector<mforms::TreeNodeRef> childs_to_remove;

    //_model_view->freeze_refresh(); cannot be called from a background thread.

    // Calculates the nodes to be removed and the new nodes to be created
    update_change_data(parent, children, type, childs_to_remove);

    // Removes deleted children if needed...
    if (!just_append && !childs_to_remove.empty()) {
      for (std::size_t index = 0; index < childs_to_remove.size(); index++)
        childs_to_remove[index]->remove_from_parent();

      removed = true;
    }

    // If at this point there are nodes on the parent, then the new nodes
    // need to be inserted into the right position.
    // If not, as they are sorted, they just get added.
    std::vector<mforms::TreeNodeRef> added_nodes;
    std::vector<mforms::TreeNodeRef> group_added_nodes;
    std::string icon_path = get_node_icon_path(type);

    if (sorted)
      children->sort(
        std::bind(base::stl_string_compare, std::placeholders::_1, std::placeholders::_2, _case_sensitive_identifiers));

    if (!children->empty()) {
      // it and it_end are used on the iteration process
      // start and end are used to determine a group to be inserted
      // which by default is the whole list
      std::list<std::string>::const_iterator it = children->begin(), start = it, it_end = children->end(), end = it_end;

      // final_group will be used to prevent searching for the position
      // of the children once one has been found to be at the end.
      bool final_group = false;
      int last_position = -1;
      _node_collections[type].captions.clear();

      if (parent->count() > 0 && sorted) {
        // Initializes last_position with the position of the first
        // Item to be inserted
        find_child_position(parent, *it, type, last_position);

        end = start;

        do {
          it++;
          if (it != it_end) {
            int position = 0;
            if (!find_child_position(parent, *it, type, position)) {
              if (position != last_position) {
                // Assigns the captions for the group to be inserted
                _node_collections[type].captions.assign(start, it);

                group_added_nodes = parent->add_node_collection(_node_collections[type], last_position);
                added_nodes.insert(added_nodes.end(), group_added_nodes.begin(), group_added_nodes.end());

                // Start is reset to the iterator position
                start = it;

                last_position = position == -1 ? position : position + (int)_node_collections[type].captions.size();

                // Shortcut to quit once an item has been found to be located
                // At the end of the list
                final_group = (last_position == -1);
              }
            }
          }

          // All the time end will advance with the iterator
          end = it;
        } while (it != it_end && !final_group);
      }

      // Inserts the last group of nodes...
      end = children->end();
      _node_collections[type].captions.assign(start, end);
      if (!_node_collections[type].captions.empty()) {
        // positions the index on the previous position (last in group)
        group_added_nodes = parent->add_node_collection(_node_collections[type], last_position);
        added_nodes.insert(added_nodes.end(), group_added_nodes.begin(), group_added_nodes.end());
      }

      for (std::size_t index = 0; index < added_nodes.size(); index++) {
        setup_node(added_nodes[index], type);
        added = true;
      }
    }

    ret_val = (added || removed);

    std::string icon = get_node_icon_path(type);

    //_model_view->thaw_refresh(); Cannot be called from a background thread.
  }

  return ret_val;
}

std::string LiveSchemaTree::get_field_description(const mforms::TreeNodeRef& node) {
  std::string text;
  mforms::TreeNodeRef temp_node = node;

  try {
    while (temp_node && !text.length()) {
      LSTData* pdata = dynamic_cast<LSTData*>(temp_node->get_data());

      if (pdata) {
        ObjectType type = pdata->get_type();

        if (is_object_type(TableOrView, type)) {
          short fetch_mask = (type == Table) ? COLUMN_DATA | INDEX_DATA : COLUMN_DATA;

          load_table_details(type, get_schema_name(node), temp_node->get_string(0), fetch_mask);
        } else if (is_object_type(RoutineObject, type)) {
          load_routine_details(temp_node);
        }

        text = pdata->get_details(true, temp_node);
      } else
        temp_node = temp_node->get_parent();
    }
  } catch (std::exception& e) {
    text = _("Unable to retrieve node description.");
    logError("Exception retrieving node description : %s\n", strfmt("%s", e.what()).c_str());
  }

  return text;
}

void LiveSchemaTree::set_notify_on_reload(const mforms::TreeNodeRef& node) {
  mforms::TreeNodeRef temp_node = node;

  LSTData* pdata = NULL;
  while (temp_node && !pdata) {
    pdata = dynamic_cast<LSTData*>(temp_node->get_data());

    if (pdata)
      notify_on_reload_data = pdata;
    else
      temp_node = temp_node->get_parent();
  }
}

void LiveSchemaTree::notify_on_reload(const mforms::TreeNodeRef& node) {
  mforms::TreeNodeRef temp_node = node;

  LSTData* pdata = NULL;
  while (temp_node && !pdata) {
    pdata = dynamic_cast<LSTData*>(temp_node->get_data());

    if (pdata && pdata == notify_on_reload_data && pdata->is_update_complete())
      _model_view->changed();
    else
      temp_node = temp_node->get_parent();
  }
}

void LiveSchemaTree::set_active_schema(const std::string& schema) {
  mforms::TreeNodeTextAttributes attrs;

  if (_model_view) {
    mforms::TreeNodeRef current_active = get_child_node(_model_view->root_node(), _active_schema);
    mforms::TreeNodeRef new_active = get_child_node(_model_view->root_node(), schema);

    if (current_active) {
      std::string name = current_active->get_string(0);
      current_active->set_string(0, name);
      current_active->set_attributes(0, attrs);
    }

    if (new_active) {
      std::string name = new_active->get_string(0);
      attrs.bold = true;
      new_active->set_string(0, name);
      new_active->set_attributes(0, attrs);

      new_active->expand();
    }
  }

  _active_schema = schema;

  if (_base)
    _base->set_active_schema(schema);
}

void LiveSchemaTree::update_live_object_state(ObjectType type, const std::string& schema_name,
                                              const std::string& old_obj_name, const std::string& new_obj_name) {
  if (_model_view) {
    mforms::TreeNodeRef schema_node;
    bool created = old_obj_name.empty() && !new_obj_name.empty();
    bool deleted = !old_obj_name.empty() && new_obj_name.empty();
    bool changed = !old_obj_name.empty() && !new_obj_name.empty();

    if (type == Schema) {
      if (created)
        insert_node(_model_view->root_node(), new_obj_name, type);
      else if (deleted) {
        schema_node = get_child_node(_model_view->root_node(), old_obj_name);

        if (schema_node)
          schema_node->remove_from_parent();
      }
    } else {
      schema_node = get_child_node(_model_view->root_node(), schema_name);

      if (schema_node) {
        mforms::TreeNodeRef object_node;
        if (!created) {
          switch (type) {
            case Table:
              object_node = this->get_child_node(schema_node->get_child(TABLES_NODE_INDEX), old_obj_name);
              break;
            case View:
              object_node = this->get_child_node(schema_node->get_child(VIEWS_NODE_INDEX), old_obj_name);
              break;
            case Procedure:
              object_node = this->get_child_node(schema_node->get_child(PROCEDURES_NODE_INDEX), old_obj_name, type);
              break;
            case Function:
              object_node = this->get_child_node(schema_node->get_child(FUNCTIONS_NODE_INDEX), old_obj_name, type);
              break;
            default:
              break;
          }
        }

        if (changed && object_node) {
          if (old_obj_name != new_obj_name)
            object_node->set_string(0, new_obj_name);

          // As the object has changed we trigger a reload on
          // Its content
          reload_object_data(object_node);
        } else {
          if (created) {
            mforms::TreeNodeRef parent_node;
            int target_position = 0;
            switch (type) {
              case Table:
                if (!find_child_position(schema_node->get_child(TABLES_NODE_INDEX), new_obj_name, type,
                                         target_position))
                  parent_node = schema_node->get_child(TABLES_NODE_INDEX);
                break;
              case View:
                if (!find_child_position(schema_node->get_child(VIEWS_NODE_INDEX), new_obj_name, type, target_position))
                  parent_node = schema_node->get_child(VIEWS_NODE_INDEX);
                break;
              case Procedure:
                if (!find_child_position(schema_node->get_child(PROCEDURES_NODE_INDEX), new_obj_name, type,
                                         target_position))
                  parent_node = schema_node->get_child(PROCEDURES_NODE_INDEX);
                break;
              case Function:
                if (!find_child_position(schema_node->get_child(FUNCTIONS_NODE_INDEX), new_obj_name, type,
                                         target_position))
                  parent_node = schema_node->get_child(FUNCTIONS_NODE_INDEX);
                break;
              default:
                break;
            }

            if (parent_node)
              insert_node(parent_node, new_obj_name, type);
          } else {
            if (object_node)
              object_node->remove_from_parent();
          }
        }
      }
    }
  }
}

void LiveSchemaTree::schema_contents_arrived(const std::string& schema_name, base::StringListPtr tables,
                                             base::StringListPtr views, base::StringListPtr procedures,
                                             base::StringListPtr functions, bool just_append) {
  if (_base) {
    _base->schema_contents_arrived(schema_name, tables, views, procedures, functions, just_append);
    filter_data();
  } else {
    if (_model_view) {
      mforms::TreeNodeRef schema_node = get_child_node(_model_view->root_node(), schema_name);

      if (schema_node) {
        mforms::TreeNodeRef tables_node = schema_node->get_child(TABLES_NODE_INDEX);
        mforms::TreeNodeRef views_node = schema_node->get_child(VIEWS_NODE_INDEX);
        mforms::TreeNodeRef procedures_node = schema_node->get_child(PROCEDURES_NODE_INDEX);
        mforms::TreeNodeRef functions_node = schema_node->get_child(FUNCTIONS_NODE_INDEX);

        SchemaData* pdata = dynamic_cast<SchemaData*>(schema_node->get_data());

        // When an error occurred all the incoming lists are NULL
        if (tables && views && procedures && functions) {
          int old_table_count = tables_node->count();
          int old_view_count = tables_node->count();

          // We need to duplicate the data, because it's being changed inside update_node_children
          // and we can't do this because it's shared between threads
          update_node_children(tables_node, std::make_shared<StringList>(*tables), Table, true, just_append);
          update_node_children(views_node, std::make_shared<StringList>(*views), View, true, just_append);
          update_node_children(procedures_node, std::make_shared<StringList>(*procedures), Procedure, true,
                               just_append);
          update_node_children(functions_node, std::make_shared<StringList>(*functions), Function, true, just_append);

          // If there were nodes that means this is a refresh, in such case loaded tables
          // must be reloaded so the changes are displayed
          if (old_table_count) {
            for (std::size_t index = 0; index < (std::size_t)tables_node->count(); index++) {
              mforms::TreeNodeRef pnode = tables_node->get_child((int)index);
              reload_object_data(pnode);
            }
          }

          // If there were nodes that means this is a refresh, in such case loaded tables
          // must be reloaded so the changes are displayed
          if (old_view_count) {
            for (std::size_t index = 0; index < (std::size_t)views_node->count(); index++) {
              mforms::TreeNodeRef pnode = views_node->get_child((int)index);
              reload_object_data(pnode);
            }
          }

          if (!just_append)
            pdata->fetched = true;

          tables_node->set_string(0, TABLES_CAPTION);
          views_node->set_string(0, VIEWS_CAPTION);
          procedures_node->set_string(0, PROCEDURES_CAPTION);
          functions_node->set_string(0, FUNCTIONS_CAPTION);
        }
        // This section will be reached whenever there´s an exception loading the schema data
        else {
          tables_node->set_string(0, TABLES_CAPTION + " " + ERROR_FETCHING_CAPTION);
          views_node->set_string(0, VIEWS_CAPTION + " " + ERROR_FETCHING_CAPTION);
          procedures_node->set_string(0, PROCEDURES_CAPTION + " " + ERROR_FETCHING_CAPTION);
          functions_node->set_string(0, FUNCTIONS_CAPTION + " " + ERROR_FETCHING_CAPTION);
        }

        pdata->fetching = false;
        update_node_icon(schema_node);
      }
    }
  }
}

void LiveSchemaTree::load_table_details(ObjectType object_type, const std::string schema_name,
                                        const std::string object_name, int fetch_mask) {
  if (_model_view) {
    mforms::TreeNodeRef node;

    // If object type is unknown there's no need to pull the
    // object from the tree
    if (object_type != Any)
      node = get_node_for_object(schema_name, object_type, object_name);

    if (node)
      load_table_details(node, fetch_mask);
    else
      fetch_table_details(object_type, schema_name, object_name, fetch_mask);
  }
}

void LiveSchemaTree::load_table_details(mforms::TreeNodeRef& node, int fetch_mask) {
  ViewData* pdata = dynamic_cast<ViewData*>(node->get_data());

  if (pdata) {
    short loaded_mask = pdata->get_loaded_mask();
    short loading_mask = pdata->get_loading_mask();

    // Calculates what needs to be loaded based on what was requested and what is already loaded
    short data_load_flags = (fetch_mask ^ loaded_mask) & fetch_mask;

    // Calculates what needs to be loaded based on what was left on the previous check and what is already being loaded
    data_load_flags = (data_load_flags ^ loading_mask) & data_load_flags;

    if (data_load_flags) {
      pdata->set_loading_mask(data_load_flags);
      std::string schema_name = get_schema_name(node);
      fetch_table_details(pdata->get_type(), schema_name, node->get_string(0), data_load_flags);
    }
  }
}

void LiveSchemaTree::fetch_table_details(ObjectType object_type, const std::string schema_name,
                                         const std::string object_name, int fetch_mask) {
  std::shared_ptr<FetchDelegate> delegate = _fetch_delegate.lock();

  if (delegate) {
    delegate->fetch_object_details(
      schema_name, object_name, object_type, fetch_mask,
      std::bind(&LiveSchemaTree::update_node_children, this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
  }
}

void LiveSchemaTree::load_routine_details(mforms::TreeNodeRef& node)

{
  ObjectData* pdata = dynamic_cast<ObjectData*>(node->get_data());

  if (pdata && !pdata->fetched && !pdata->fetching) {
    pdata->fetching = true;

    std::string schema_name = get_schema_name(node);

    std::shared_ptr<FetchDelegate> delegate = _fetch_delegate.lock();

    if (delegate)
      delegate->fetch_routine_details(schema_name, node->get_string(0), pdata->get_type());
  }
}

void LiveSchemaTree::load_data_for_filter(const std::string& schema_filter, const std::string& object_filter) {
  if (std::shared_ptr<FetchDelegate> delegate = _fetch_delegate.lock()) {
    std::string remote_schema_filter = get_filter_wildcard(schema_filter, RemoteLike);
    std::string remote_object_filter = get_filter_wildcard(object_filter, RemoteLike);
    delegate->fetch_data_for_filter(
      remote_schema_filter, remote_object_filter,
      std::bind(&LiveSchemaTree::schema_contents_arrived, this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
  }
}

void LiveSchemaTree::load_schema_content(mforms::TreeNodeRef& schema_node) {
  SchemaData* data = dynamic_cast<SchemaData*>(schema_node->get_data());

  if (!data->fetched && !data->fetching) {
    data->fetching = true;
    std::string name = schema_node->get_string(0);

    if (_base) {
      mforms::TreeNodeRef base_schema_node = _base->get_node_from_path(get_node_path(schema_node));

      base_schema_node->get_child(TABLES_NODE_INDEX)->set_string(0, TABLES_CAPTION + " " + FETCHING_CAPTION);
      base_schema_node->get_child(VIEWS_NODE_INDEX)->set_string(0, VIEWS_CAPTION + " " + FETCHING_CAPTION);
      base_schema_node->get_child(PROCEDURES_NODE_INDEX)->set_string(0, PROCEDURES_CAPTION + " " + FETCHING_CAPTION);
      base_schema_node->get_child(FUNCTIONS_NODE_INDEX)->set_string(0, FUNCTIONS_CAPTION + " " + FETCHING_CAPTION);
    }

    schema_node->get_child(TABLES_NODE_INDEX)->set_string(0, TABLES_CAPTION + " " + FETCHING_CAPTION);
    schema_node->get_child(VIEWS_NODE_INDEX)->set_string(0, VIEWS_CAPTION + " " + FETCHING_CAPTION);
    schema_node->get_child(PROCEDURES_NODE_INDEX)->set_string(0, PROCEDURES_CAPTION + " " + FETCHING_CAPTION);
    schema_node->get_child(FUNCTIONS_NODE_INDEX)->set_string(0, FUNCTIONS_CAPTION + " " + FETCHING_CAPTION);

    update_node_icon(schema_node);

    if (std::shared_ptr<FetchDelegate> delegate = _fetch_delegate.lock()) {
      delegate->fetch_schema_contents(
        name, std::bind(&LiveSchemaTree::schema_contents_arrived, this, std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
    }
  }
}

mforms::TreeNodeRef LiveSchemaTree::get_node_for_object(const std::string& schema_name, ObjectType type,
                                                        const std::string& name) {
  mforms::TreeNodeRef object_node = mforms::TreeNodeRef();

  if (_model_view) {
    mforms::TreeNodeRef schema_node = get_child_node(_model_view->root_node(), schema_name);

    if (schema_node) {
      if (type == Schema)
        object_node = schema_node;
      else {
        switch (type) {
          case Table:
            object_node = get_child_node(schema_node->get_child(TABLES_NODE_INDEX), name);
            break;
          case View:
            object_node = get_child_node(schema_node->get_child(VIEWS_NODE_INDEX), name);
            break;
          case Procedure:
            object_node = get_child_node(schema_node->get_child(PROCEDURES_NODE_INDEX), name, type);
            break;
          case Function:
            object_node = get_child_node(schema_node->get_child(FUNCTIONS_NODE_INDEX), name, type);
            break;
          default:
            break;
        }
      }
    }
  }

  return object_node;
}

mforms::TreeNodeRef LiveSchemaTree::create_node_for_object(const std::string& schema_name, ObjectType type,
                                                           const std::string& name) {
  bool created_schema = false;
  mforms::TreeNodeRef object_node = mforms::TreeNodeRef();
  mforms::TreeNodeRef parent_node = mforms::TreeNodeRef();

  if (_model_view) {
    mforms::TreeNodeRef schema_node = get_child_node(_model_view->root_node(), schema_name);

    // Creates the schema if doesnt exist...
    if (!schema_node) {
      schema_node = insert_node(_model_view->root_node(), schema_name, Schema);
      created_schema = true;
    }

    switch (type) {
      case Table:
        parent_node = schema_node->get_child(TABLES_NODE_INDEX);
        break;
      case View:
        parent_node = schema_node->get_child(VIEWS_NODE_INDEX);
        break;
      case Procedure:
        parent_node = schema_node->get_child(PROCEDURES_NODE_INDEX);
        break;
      case Function:
        parent_node = schema_node->get_child(FUNCTIONS_NODE_INDEX);
        break;
      default:
        break;
    }

    if (parent_node)
      object_node = insert_node(parent_node, name, type);
    else if (created_schema)
      schema_node->remove_from_parent();
  }

  return object_node;
}

grt::BaseListRef LiveSchemaTree::get_selected_objects() {
  grt::ListRef<db_query_LiveDBObject> selection(true);

  if (_model_view) {
    std::list<mforms::TreeNodeRef> selnodes(_model_view->get_selection());
    db_query_LiveDBObjectRef table_object;

    std::list<mforms::TreeNodeRef>::const_iterator index, end = selnodes.end();
    for (index = selnodes.begin(); index != end; index++) {
      db_query_LiveDBObjectRef obj(grt::Initialized);

      mforms::TreeNodeRef node = *index;
      LSTData* pdata = dynamic_cast<LSTData*>(node->get_data());
      ObjectType current_type = Any;

      if (pdata)
        current_type = pdata->get_type();
      else {
        if (node->get_tag() == TABLES_TAG)
          current_type = TableCollection;
        else if (node->get_tag() == VIEWS_TAG)
          current_type = ViewCollection;
        else if (node->get_tag() == PROCEDURES_TAG)
          current_type = ProcedureCollection;
        else if (node->get_tag() == FUNCTIONS_TAG)
          current_type = FunctionCollection;
        else if (node->get_tag() == COLUMNS_TAG)
          current_type = ColumnCollection;
        else if (node->get_tag() == INDEXES_TAG)
          current_type = IndexCollection;
        else if (node->get_tag() == TRIGGERS_TAG)
          current_type = TriggerCollection;
        else if (node->get_tag() == FOREIGN_KEYS_TAG)
          current_type = ForeignKeyCollection;
      }

      // the selection list is always sorted top elements first, so we now for sure that
      // if a node and one of its subnodes are selected, the parent node will always come
      // first in the list
      switch (current_type) {
        case Any:
        case NoneType:
          break;
        case Schema:
          obj->type("db.Schema");
          obj->schemaName(node->get_string(0));
          obj->name(node->get_string(0));
          break;
        case Table:
          obj->type("db.Table");
          obj->schemaName(node->get_parent()->get_parent()->get_string(0));
          obj->name(node->get_string(0));
          table_object = obj;
          break;
        case TableColumn:
        case Trigger:
        case Index:
        case ForeignKey:
          obj->schemaName(node->get_parent()->get_parent()->get_parent()->get_parent()->get_string(0));
          if (!table_object.is_valid() || table_object->schemaName() != obj->schemaName() ||
              table_object->name() != node->get_parent()->get_parent()->get_string(0)) {
            table_object = db_query_LiveDBObjectRef(grt::Initialized);
            table_object->type("db.Table");
            table_object->schemaName(obj->schemaName());
            table_object->name(node->get_parent()->get_parent()->get_string(0));
          }
          if (current_type == TableColumn)
            obj->type("db.Column");
          else if (current_type == Trigger)
            obj->type("db.Trigger");
          else if (current_type == Index)
            obj->type("db.Index");
          else if (current_type == ForeignKey)
            obj->type("db.ForeignKey");
          obj->owner(table_object);
          obj->name(node->get_string(0));
          break;
        case View:
          obj->type("db.View");
          obj->schemaName(node->get_parent()->get_parent()->get_string(0));
          obj->name(node->get_string(0));
          table_object = obj;
          break;
        case ViewColumn:
          obj->schemaName(node->get_parent()->get_parent()->get_parent()->get_string(0));
          if (!table_object.is_valid() || table_object->schemaName() != obj->schemaName() ||
              table_object->name() != node->get_parent()->get_string(0)) {
            table_object = db_query_LiveDBObjectRef(grt::Initialized);
            table_object->type("db.View");
            table_object->schemaName(obj->schemaName());
            table_object->name(node->get_parent()->get_string(0));
          }
          obj->type("db.Column");
          obj->owner(table_object);
          obj->name(node->get_string(0));
          break;
        case Procedure:
          obj->type("db.StoredProcedure");
          obj->schemaName(node->get_parent()->get_parent()->get_string(0));
          obj->name(node->get_string(0));
          break;
        case Function:
          obj->type("db.Function");
          obj->schemaName(node->get_parent()->get_parent()->get_string(0));
          obj->name(node->get_string(0));
          break;
        case TableCollection:
          obj->schemaName(node->get_parent()->get_string(0));
          obj->type("tables");
          obj->name("");
          break;
        case ViewCollection:
          obj->schemaName(node->get_parent()->get_string(0));
          obj->type("views");
          obj->name("");
          break;
        case ProcedureCollection:
          obj->schemaName(node->get_parent()->get_string(0));
          obj->type("storedProcedures");
          obj->name("");
          break;
        case FunctionCollection:
          obj->schemaName(node->get_parent()->get_string(0));
          obj->type("functions");
          obj->name("");
          break;
        case ColumnCollection:
        case IndexCollection:
        case TriggerCollection:
        case ForeignKeyCollection:
          obj->schemaName(node->get_parent()->get_parent()->get_parent()->get_string(0));
          if (!table_object.is_valid() || table_object->schemaName() != obj->schemaName() ||
              table_object->name() != node->get_parent()->get_parent()->get_string(0)) {
            table_object = db_query_LiveDBObjectRef(grt::Initialized);
            table_object->type("db.Table");
            table_object->schemaName(obj->schemaName());
            table_object->name(node->get_parent()->get_string(0));
          }
          obj->owner(table_object);
          if (current_type == ForeignKeyCollection)
            obj->type("foreignKeys");
          else if (current_type == TriggerCollection)
            obj->type("triggers");
          else if (current_type == IndexCollection)
            obj->type("indexes");
          else if (current_type == ColumnCollection)
            obj->type("columns");
          break;
        case ForeignKeyColumn:
        case IndexColumn:
          break;
      }
      if (obj->type() != "")
        selection.insert(obj);
    }
  }

  return selection;
}

bec::MenuItemList LiveSchemaTree::get_popup_items_for_nodes(const std::list<mforms::TreeNodeRef>& nodes) {
  bec::MenuItemList items;

  {
    mforms::TreeNodeRef node;
    if (nodes.size())
      node = nodes.front();

    int type = -1;
    if (node) {
      LSTData* pdata = dynamic_cast<LSTData*>(node->get_data());

      if (pdata)
        type = pdata->get_type();
    }

    if (type == Schema) {
      bec::MenuItem active_schema_item;
      active_schema_item.caption = _("Set as Default Schema");
      active_schema_item.internalName = "set_active_schema";
      active_schema_item.accessibilityName = "Set Active Schema";
      active_schema_item.enabled = nodes.size() == 1;

      bec::MenuItem filter_schema_item;
      filter_schema_item.caption = _("Filter to This Schema");
      filter_schema_item.internalName = "filter_schema";
      filter_schema_item.accessibilityName = "Filter Schema";
      filter_schema_item.enabled = nodes.size() == 1;

      items.push_back(active_schema_item);
      items.push_back(filter_schema_item);
      bec::MenuItem item;
      item.type = MenuSeparator;
      item.internalName = "builtins_separator"; // this indicates where plugins should start adding their menu items
      item.accessibilityName = "Separator";
      items.push_back(item);
    } else if (type == Table || type == View || type == TableColumn || type == ViewColumn || type == ViewCollection ||
               type == ColumnCollection) {
      bec::MenuItem view_item;
      {
        std::string caption = _("Select Rows");
        {
          DictRef options = DictRef::cast_from(grt::GRT::get()->get("/wb/options/options"));
          bool limit_rows = (0 != options.get_int("SqlEditor:LimitRows"));
          ssize_t limit_rows_count = options.get_int("SqlEditor:LimitRowsCount");
          if (limit_rows && (limit_rows_count <= 0))
            limit_rows = false;
          if (limit_rows)
            caption += _(" - Limit ") + std::to_string(limit_rows_count);
        }
        view_item.caption = caption;
      }
      view_item.internalName = "select_data";
      view_item.accessibilityName = "Select Data";
      view_item.enabled = !nodes.empty() && (nodes.size() == 1 || (type == TableColumn || type == ViewColumn));
      items.push_back(view_item);
      bec::MenuItem item;
      item.type = MenuSeparator;
      item.internalName = "builtins_separator"; // this indicates where plugins should start adding their menu items
      item.accessibilityName = "Separator";
      items.push_back(item);
    }
  }
  {
    bec::MenuItem item;
    item.type = MenuSeparator;
    item.internalName = "bottom_plugins_separator";
    item.accessibilityName = "Separator"; // this indicates where plugins should start adding their menu items
    items.push_back(item);

    item.type = MenuAction;
    item.caption = _("Refresh All");
    item.internalName = "refresh";
    item.accessibilityName = "Refresh";
    items.push_back(item);
  }

  return items;
}

bool LiveSchemaTree::activate_popup_item_for_nodes(const std::string& name,
                                                   const std::list<mforms::TreeNodeRef>& unsorted_nodes) {
  std::vector<ChangeRecord> changes;

  mforms::TreeNodeRef pnode;

  std::string schema_name = "";
  std::string object_name = "";
  std::string object_detail = "";

  if (std::shared_ptr<Delegate> delegate = _delegate.lock()) {
    if (name == "refresh") {
      delegate->tree_refresh();
      return true;
    } else if (name == "set_active_schema") {
      pnode = unsorted_nodes.front();
      ChangeRecord record = {Schema, "", pnode->get_string(0), ""};
      changes.push_back(record);
      delegate->tree_activate_objects("activate", changes);
      return true;
    } else if (name == "filter_schema") {
      pnode = unsorted_nodes.front();
      ChangeRecord record = {Schema, "", pnode->get_string(0), ""};
      changes.push_back(record);
      delegate->tree_activate_objects("filter", changes);
      return true;
    } else if (name == "select_data") {
      std::list<mforms::TreeNodeRef>::const_iterator index, end = unsorted_nodes.end();
      mforms::TreeNodeRef pnode;
      LSTData* pdata;
      std::string schema_name = "";
      std::string object_name = "";
      std::string object_detail = "";
      ObjectType type = Any;
      bool use_columns = false;

      for (index = unsorted_nodes.begin(); index != end; index++) {
        pnode = (*index);
        pdata = dynamic_cast<LSTData*>(pnode->get_data());

        if (pdata) {
          schema_name = "";
          object_name = "";
          object_detail = "";
          type = pdata->get_type();
          schema_name = get_schema_name(pnode);

          if (is_object_type(SchemaObject, type))
            object_name = pnode->get_string(0);
          else {
            if (pdata->get_type() == TableColumn) {
              object_name = pnode->get_parent()->get_parent()->get_string(0);
              object_detail = pnode->get_string(0);
              type = Table;
              use_columns = true;
            } else if (pdata->get_type() == ViewColumn) {
              object_name = pnode->get_parent()->get_string(0);
              object_detail = pnode->get_string(0);
              type = View;
              use_columns = true;
            }
          }

          if (object_name.length() > 0) {
            ChangeRecord record = {type, schema_name, object_name, object_detail};
            changes.push_back(record);
          }
        } else {
          if (pnode->get_tag() == COLUMNS_TAG) {
            int size = pnode->count();
            mforms::TreeNodeRef child;

            schema_name = get_schema_name(pnode);
            object_name = pnode->get_parent()->get_string(0);

            for (int index = 0; index < size; index++) {
              child = pnode->get_child(index);
              ChangeRecord record = {Table, schema_name, object_name, child->get_string(0)};
              changes.push_back(record);
            }
          }
        }
      }

      if (use_columns)
        delegate->tree_activate_objects(name + "_columns", changes);
      else
        delegate->tree_activate_objects(name, changes);
      return true;
    } else
      return delegate->sidebar_action(name);
  }

  return false;
}

bool LiveSchemaTree::is_schema_contents_enabled() const {
  return _is_schema_contents_enabled;
}

void LiveSchemaTree::is_schema_contents_enabled(bool value) {
  _is_schema_contents_enabled = value;
}

//--------------------------------------------------------------------------------------------------

void LiveSchemaTree::set_no_connection() {
  _model_view->clear();
  mforms::TreeNodeRef node = _model_view->add_node();
  node->set_string(0, "Not connected");
}

//--------------------------------------------------------------------------------------------------

void LiveSchemaTree::set_enabled(bool enabled) {
  _model_view->set_enabled(enabled);
}

//--------------------------------------------------------------------------------------------------

std::string LiveSchemaTree::get_filter_wildcard(const std::string& filter, FilterType type) {
  std::string wildcard = filter;
  if (filter.length() == 0)
    wildcard = "*";

  switch (type) {
    case LocalRegexp:
    case LocalLike:
    case RemoteRegexp:
      if ('*' != wildcard.at(wildcard.length() - 1))
        wildcard += "*";
      break;
    case RemoteLike:
      base::replaceStringInplace(wildcard, "%", "\\%");
      base::replaceStringInplace(wildcard, "_", "\\_");
      base::replaceStringInplace(wildcard, "?", "_");
      base::replaceStringInplace(wildcard, "*", "%");

      if ('%' != wildcard.at(wildcard.length() - 1))
        wildcard += "%";

      break;
  }

  return wildcard;
}

//--------------------------------------------------------------------------------------------------
void LiveSchemaTree::filter_data() {
  _enabled_events = false;

  // Removes all the objects on the target tree
  _model_view->clear();

  mforms::TreeNodeRef base_root = _base->_model_view->root_node();
  mforms::TreeNodeRef this_root = _model_view->root_node();
  filter_children(Schema, base_root, this_root, _schema_pattern);

  // To keep the active schema on the filtered tree
  set_active_schema(_base->_active_schema);

  _enabled_events = true;
}

//--------------------------------------------------------------------------------------------------

/*
*  filter_children_collection: will trigger a children copy for the collection nodes of the given source
*                              right now the pattern is only used for nodes on schema collections
*/
void LiveSchemaTree::filter_children_collection(mforms::TreeNodeRef& source, mforms::TreeNodeRef& target) {
  LSTData* pdata = dynamic_cast<LSTData*>(source->get_data());

  if (pdata) {
    mforms::TreeNodeRef source_collection;
    mforms::TreeNodeRef target_collection;

    switch (pdata->get_type()) {
      case Schema: {
        source_collection = source->get_child(TABLES_NODE_INDEX);
        target_collection = target->get_child(TABLES_NODE_INDEX);
        bool found_tables = filter_children(Table, source_collection, target_collection, _object_pattern);

        source_collection = source->get_child(VIEWS_NODE_INDEX);
        target_collection = target->get_child(VIEWS_NODE_INDEX);
        bool found_views = filter_children(View, source_collection, target_collection, _object_pattern);

        source_collection = source->get_child(PROCEDURES_NODE_INDEX);
        target_collection = target->get_child(PROCEDURES_NODE_INDEX);
        bool found_procedures = filter_children(Procedure, source_collection, target_collection, _object_pattern);

        source_collection = source->get_child(FUNCTIONS_NODE_INDEX);
        target_collection = target->get_child(FUNCTIONS_NODE_INDEX);
        bool found_functions = filter_children(Function, source_collection, target_collection, _object_pattern);

        if (_object_pattern && !(found_tables || found_views || found_procedures || found_functions))
          target->remove_from_parent();
      } break;
      case Table:
        source_collection = source->get_child(TABLE_COLUMNS_NODE_INDEX);
        target_collection = target->get_child(TABLE_COLUMNS_NODE_INDEX);
        filter_children(TableColumn, source_collection, target_collection);

        source_collection = source->get_child(TABLE_INDEXES_NODE_INDEX);
        target_collection = target->get_child(TABLE_INDEXES_NODE_INDEX);
        filter_children(Index, source_collection, target_collection);

        source_collection = source->get_child(TABLE_FOREIGN_KEYS_NODE_INDEX);
        target_collection = target->get_child(TABLE_FOREIGN_KEYS_NODE_INDEX);
        filter_children(ForeignKey, source_collection, target_collection);

        source_collection = source->get_child(TABLE_TRIGGERS_NODE_INDEX);
        target_collection = target->get_child(TABLE_TRIGGERS_NODE_INDEX);
        filter_children(Trigger, source_collection, target_collection);
        break;
      case View:
        filter_children(ViewColumn, source, target);
        break;
      default:
        break;
    }
  }
}
/*
*  filter_children: will create duplicate objects in target for the children in source matching the given pattern
*                   if no pattern is specified, all the children will be cuplicated
*/
bool LiveSchemaTree::filter_children(ObjectType type, mforms::TreeNodeRef& source, mforms::TreeNodeRef& target,
                                     GPatternSpec* pattern) {
  // Validation to occur only on schema child objects if a pattern is set
  bool validate = is_object_type(DatabaseObject, type) && pattern;

  // Clears the collection...
  target->remove_children();

  int count = source->count();
  for (int index = 0; index < count; index++) {
    mforms::TreeNodeRef source_node = source->get_child(index);

// #ifdef GLIB_VERSION_2_70 won't work in RHEL9/OL9 because the glib-2.68 package already contains this definition
#ifdef g_pattern_spec_match_string
      bool match_string  = g_pattern_spec_match_string(pattern, base::toupper(source_node->get_string(0)).c_str());
#else
      bool match_string = g_pattern_match_string(pattern, base::toupper(source_node->get_string(0)).c_str());
#endif
    if (!validate || match_string) {
      std::vector<mforms::TreeNodeRef> group_added_nodes;
      _node_collections[type].captions.clear();
      _node_collections[type].captions.push_back(source_node->get_string(0));
      group_added_nodes = target->add_node_collection(_node_collections[type]);
      setup_node(group_added_nodes[0], type, source_node->get_data(), true);

      // For each found node, continues with their children...
      if (type == Schema || type == Table || type == View)
        filter_children_collection(source_node, group_added_nodes[0]);

      if (source_node->is_expanded())
        group_added_nodes[0]->expand();
      else
        group_added_nodes[0]->collapse();
    }
  }

  if (source->is_expanded() != target->is_expanded()) {
    if (source->is_expanded())
      target->expand();
    else
      target->collapse();
  }

  return target->count() > 0;
}

//--------------------------------------------------------------------------------------------------

void LiveSchemaTree::clean_filter() {
  if (_filter.length() > 0) {
    _filter_type = Any;
    _filter = "";

    g_pattern_spec_free(_schema_pattern);
    _schema_pattern = NULL;

    if (_object_pattern) {
      g_pattern_spec_free(_object_pattern);
      _object_pattern = NULL;
    }
  }
}

void LiveSchemaTree::set_filter(std::string filter) {
  // Cleans the previous filter if any...
  clean_filter();

  if (filter.length() > 0) {
    _filter = filter;

    std::vector<std::string> filters = base::split(_filter, ".", 2);

    // Gets the filter wildcard strings
    std::string schema_filter = base::toupper(get_filter_wildcard(filters[0], LocalLike));
    std::string object_filter = base::toupper(get_filter_wildcard(filters.size() > 1 ? filters[1] : "", LocalLike));

    _schema_pattern = g_pattern_spec_new(schema_filter.c_str());

    if (filters.size() > 1 && object_filter != "*")
      _object_pattern = g_pattern_spec_new(object_filter.c_str());
  }
}

void LiveSchemaTree::set_model_view(mforms::TreeView* target) {
  _model_view = target;

  if (_model_view) {
    scoped_connect(_model_view->signal_expand_toggle(),
                   std::bind(&LiveSchemaTree::expand_toggled, this, std::placeholders::_1, std::placeholders::_2));
    scoped_connect(_model_view->signal_node_activated(),
                   std::bind(&LiveSchemaTree::node_activated, this, std::placeholders::_1, std::placeholders::_2));

    _model_view->set_row_overlay_handler(
      std::bind(&LiveSchemaTree::overlay_icons_for_tree_node, this, std::placeholders::_1));
  }
}

/* Function : binary_search_node
 * Description : Search a child on a given node based on it's name, assumes the children
 *               are sorted
 * Parameters : parent, the node where the search is being performed
 *              min, max : usual parameters on the binary search
 *              name : the name of the child node being searched
 *              type : the type of the child node to be searched (not being used)
 *              position : output parameter to store:
 *                        - If the node is found, the position in parent's list
 *                        - If not found, the position where it should be located if it is going to be added
 * Return Value : if found, the child node
 */
mforms::TreeNodeRef LiveSchemaTree::binary_search_node(const mforms::TreeNodeRef& parent, int min, int max,
                                                       const std::string& name, ObjectType type, int& position) {
  if (max < min)
    return mforms::TreeNodeRef();
  else {
    int middle = (max + min) / 2;
    position = middle;

    mforms::TreeNodeRef node = parent->get_child(middle);

    int comparison = base::string_compare(node->get_string(0), name, _case_sensitive_identifiers);

    if (comparison < 0)
      return binary_search_node(parent, middle + 1, max, name, type, ++position);
    else if (comparison > 0)
      return binary_search_node(parent, min, middle - 1, name, type, position);
    else
      return node;
  }
}

/* Function : get_child_node
 * Description : Searches a specific child on a given node, supports both sequential search
 *               and binary search. Binary search should be used when searching for schemas,
 *               tables, views and routines which are sorted. Sequential search is there for
 *               the non sorted nodes.
 */
mforms::TreeNodeRef LiveSchemaTree::get_child_node(const mforms::TreeNodeRef& parent, const std::string& name,
                                                   ObjectType type, bool binary_search) {
  int last_position = 0;
  bool found = false;
  mforms::TreeNodeRef child;

  if (binary_search) {
    if (parent && parent->count())
      child = binary_search_node(parent, 0, parent->count() - 1, name, type, last_position);

    if (child)
      found = true;
  } else {
    if (parent && parent->count()) {
      for (int index = 0; !found && index < parent->count(); index++) {
        child = parent->get_child(index);

        found = (base::string_compare(child->get_string(0), name, _case_sensitive_identifiers) == 0);

        if (found && type != Any) {
          LSTData* pdata = dynamic_cast<LSTData*>(child->get_data());
          found = (pdata && type == pdata->get_type());
        }
      }
    }
  }

  return found ? child : mforms::TreeNodeRef();
}

/* Function : find_child_position
 * Description : given a child name, searches it on the received parent. This function is
 *               intended for usage on sorted collections of nodes
 * Parameters : parent, the node where the search will occur
 *              type, the type of the object being searched
 *              position, output parameter containing:
 *                - If found, the index of the found node
 *                - If not found, the position where the node should be (i.e. to add it there)
 * Return value : boolean value indicating whether the node was found or not
 */
bool LiveSchemaTree::find_child_position(const mforms::TreeNodeRef& parent, const std::string& name, ObjectType type,
                                         int& position) {
  mforms::TreeNodeRef child;

  position = 0;

  if (parent && parent->count())
    child = binary_search_node(parent, 0, parent->count() - 1, name, type, position);

  if (parent->count() == position)
    position = -1;

  return child ? true : false;
}

void LiveSchemaTree::update_schemata(base::StringListPtr schema_list) {
  mforms::TreeNodeRef schema_node;

  if (_model_view) {
    mforms::TreeNodeRef root = _model_view->root_node();
    if (root && root->count() > 0 && !root->get_child(0)->get_data()) {
      // the tree was in no-connection mode
      _model_view->clear();
      root = _model_view->root_node();
    }

    schema_list->sort(
      std::bind(base::stl_string_compare, std::placeholders::_1, std::placeholders::_2, _case_sensitive_identifiers));

    update_node_children(root, schema_list, Schema, true);

    // Re-sets the active schema at view level
    if (_active_schema.length())
      set_active_schema(_active_schema);

    int total_schemas = root->count();
    for (int index = 0; index < total_schemas; index++) {
      schema_node = root->get_child(index);
      SchemaData* data = dynamic_cast<SchemaData*>(schema_node->get_data());

      if (data->fetched) {
        data->fetched = false;

        if (schema_node->is_expanded())
          load_schema_content(schema_node);
      }
    }
  }
}

void LiveSchemaTree::expand_toggled(mforms::TreeNodeRef node, bool value) {
  if (_enabled_events) {
    LSTData* node_data = dynamic_cast<LSTData*>(node->get_data());

    if (value) {
      if (node_data) {
        switch (node_data->get_type()) {
          case Schema:
            load_schema_content(node);
            break;
          case Table:
            load_table_details(node, COLUMN_DATA | INDEX_DATA);
            break;
          case View: {
            load_table_details(node, COLUMN_DATA);

            ViewData* pdata = dynamic_cast<ViewData*>(node->get_data());
            if (pdata->columns_load_error) {
              node->remove_children();
              update_node_icon(node);
            }
          } break;
          default:
            break;
        }
      } else {
        std::string node_tag = node->get_tag();
        mforms::TreeNodeRef parent = node->get_parent();

        if (node_tag == TRIGGERS_TAG)
          load_table_details(parent, TRIGGER_DATA);
        else if (node_tag == FOREIGN_KEYS_TAG)
          load_table_details(parent, FK_DATA);
      }
    }

    // If there's a base tree the expansion state needs to be propagated to that tree
    // Events should be disabled there so only the expand state will be propagated
    if (_base) {
      std::vector<std::string> path = get_node_path(node);
      mforms::TreeNodeRef base_node = _base->get_node_from_path(path);
      if (value)
        base_node->expand();
      else
        base_node->collapse();
    }
  }
}

//--------------------------------------------------------------------------------------------------

void LiveSchemaTree::node_activated(mforms::TreeNodeRef node, int column) {
  LSTData* node_data = dynamic_cast<LSTData*>(node->get_data());

  if (node_data) {
    std::string node_name = node->get_string(0);

    switch (node_data->get_type()) {
      case Schema: {
        std::vector<ChangeRecord> changes;

        ChangeRecord record = {Schema, "", node_name, ""};
        changes.push_back(record);

        if (std::shared_ptr<Delegate> delegate = _delegate.lock()) {
          switch (column) {
            case -1:
              delegate->tree_activate_objects("inspect", changes);
              break;
            case -2:
              delegate->tree_activate_objects("alter", changes);
              break;
            default:
              delegate->tree_activate_objects("activate", changes);
#ifndef _MSC_VER
              node->toggle();
#endif
              break;
          }
        }
      } break;

      case Table:
      /* fall-thru */
      case View: {
        if (column < 0) {
          std::vector<ChangeRecord> changes;
          ChangeRecord record = {node_data->get_type(), get_schema_name(node), node_name, ""};
          changes.push_back(record);

          if (std::shared_ptr<Delegate> delegate = _delegate.lock()) {
            switch (column) {
              case -1:
                delegate->tree_activate_objects("inspect", changes);
                break;
              case -2:
                delegate->tree_activate_objects("alter", changes);
                break;
              case -3:
                delegate->tree_activate_objects("select_data", changes);
                break;
#ifndef _MSC_VER
              default:
                node->toggle();
                break;
#endif
            }
          }
          break;
        }
      }
      /* fall-thru */
      case Procedure:
      /* fall-thru */
      case Function: {
        if (column < 0) {
          std::vector<ChangeRecord> changes;
          ChangeRecord record = {node_data->get_type(), get_schema_name(node), node_name, ""};
          changes.push_back(record);

          if (std::shared_ptr<Delegate> delegate = _delegate.lock()) {
            switch (column) {
              case -1:
                delegate->tree_activate_objects("alter", changes);
                break;
              case -2:
                delegate->tree_activate_objects("execute", changes);
                break;
            }
          }
          break;
        }
      }
      /* fall-thru */
      default:
        node_name = base::quoteIdentifierIfNeeded(node_name, '`', _version);
        sql_editor_text_insert_signal(node_name);
        break;
    }
  }
#ifndef _MSC_VER
  else
    node->toggle();
#endif
}

//--------------------------------------------------------------------------------------------------

/**
 * Finds the parent schema for a specific node in the tree.
 */
std::string LiveSchemaTree::get_schema_name(const mforms::TreeNodeRef& node) {
  std::string ret_val;
  mforms::TreeNodeRef temp_node = node;
  mforms::TreeNodeRef parent = temp_node->get_parent();

  // Safety validation, all nodes in the LST should have a parent
  // Even schema nodes which it's parent is root
  if (parent) {
    while (parent->get_parent()) {
      temp_node = parent;
      parent = parent->get_parent();
    }

    ret_val = temp_node->get_string(0);
  }

  return ret_val;
}

/*
* get_node_path: Gets the name path to a node from root
*/
std::vector<std::string> LiveSchemaTree::get_node_path(const mforms::TreeNodeRef& node) {
  std::vector<std::string> path;

  mforms::TreeNodeRef temp_node = node;
  mforms::TreeNodeRef parent = temp_node->get_parent();

  // Safety validation, all nodes in the LST should have a parent
  // Even schema nodes which it's parent is root
  if (parent) {
    path.insert(path.begin(), temp_node->get_string(0));

    while (parent->get_parent()) {
      temp_node = parent;
      path.insert(path.begin(), temp_node->get_string(0));

      parent = parent->get_parent();
    }
  }

  return path;
}

/*
* get_node_from_path: Finds a node in a tree following a name path
* TODO: This will not work in the case of the routines, as there's no way to know if the path is
*       for a procedure or for a function, on this case a sequential search will be done so
*       procedures will be searched first all the time
*/
mforms::TreeNodeRef LiveSchemaTree::get_node_from_path(std::vector<std::string> path) {
  mforms::TreeNodeRef temp_node = _model_view->root_node();
  std::size_t index = 0;
  bool error = false;
  bool use_binary_search = true;

  while (!error && index < path.size()) {
    temp_node = get_child_node(temp_node, path[index], Any, use_binary_search);

    if (temp_node && temp_node->is_valid()) {
      index++;

      // Uses binary search only on the db object collection nodes
      std::string tag = temp_node->get_tag();

      use_binary_search = (tag == TABLES_TAG || tag == VIEWS_TAG);
    } else
      error = true;
  }

  return error ? mforms::TreeNodeRef() : temp_node;
}

/*
* is_object_type: Validates that the type of a given object is in a specific group
*/
bool LiveSchemaTree::is_object_type(ObjectTypeValidation validation, ObjectType type) {
  switch (validation) {
    case DatabaseObject:
      return (type == Schema || type == Table || type == View || type == Procedure || type == Function);
      break;
    case SchemaObject:
      return (type == Table || type == View || type == Procedure || type == Function);
      break;
    case TableOrView:
      return (type == Table || type == View);
      break;
    case ColumnObject:
      return (type == TableColumn || type == ViewColumn);
      break;
    case RoutineObject:
      return (type == Procedure || type == Function);
      break;
  }

  return false;
}

mforms::TreeNodeRef LiveSchemaTree::insert_node(mforms::TreeNodeRef parent, const std::string& name, ObjectType type) {
  mforms::TreeNodeRef node;

  int target_position = 0;
  if (!find_child_position(parent, name, type, target_position)) {
    std::vector<mforms::TreeNodeRef> group_added_nodes;

    _node_collections[type].captions.clear();
    _node_collections[type].captions.push_back(name);
    group_added_nodes = parent->add_node_collection(_node_collections[type], target_position);
    node = group_added_nodes[0];

    setup_node(node, type);
  }

  return node;
}

void LiveSchemaTree::reload_object_data(mforms::TreeNodeRef& node) {
  ViewData* pdata = dynamic_cast<ViewData*>(node->get_data());
  if (pdata) {
    short loaded_mask = pdata->get_loaded_mask();
    if (loaded_mask) {
      // This was a successful update so in case the error icon was loaded
      // It needs to be reset
      if (pdata->columns_load_error) {
        pdata->columns_load_error = false;
        update_node_icon(node);
      }

      // Marks the data that is expected to be loaded on the node
      pdata->set_reload_mask(loaded_mask);

      // Identifies the node expansion state
      bool is_expanded = node->is_expanded();
      int expanded_mask = 0;

      if (is_expanded && pdata->get_type() == Table) {
        expanded_mask |= node->get_child(TABLE_COLUMNS_NODE_INDEX)->is_expanded() ? COLUMN_DATA : 0;
        expanded_mask |= node->get_child(TABLE_INDEXES_NODE_INDEX)->is_expanded() ? INDEX_DATA : 0;
        expanded_mask |= node->get_child(TABLE_TRIGGERS_NODE_INDEX)->is_expanded() ? TRIGGER_DATA : 0;
        expanded_mask |= node->get_child(TABLE_FOREIGN_KEYS_NODE_INDEX)->is_expanded() ? FK_DATA : 0;
      }

      // Invalidates any loaded data on the object
      pdata->set_unloaded_data(loaded_mask);

      // Removes any loaded information to allow reload
      discard_object_data(node, loaded_mask);

      // Reloads the previously loaded information if the node is
      // expanded, if not, it will be loaded on expansion
      if (loaded_mask) {
        load_table_details(node, loaded_mask);

        // Triggers the expansion of the subnodes
        if (is_expanded) {
          node->expand();
          if (expanded_mask) {
            if (expanded_mask & COLUMN_DATA)
              node->get_child(TABLE_COLUMNS_NODE_INDEX)->expand();
            if (expanded_mask & INDEX_DATA)
              node->get_child(TABLE_INDEXES_NODE_INDEX)->expand();
            if (expanded_mask & TRIGGER_DATA)
              node->get_child(TABLE_TRIGGERS_NODE_INDEX)->expand();
            if (expanded_mask & FK_DATA)
              node->get_child(TABLE_FOREIGN_KEYS_NODE_INDEX)->expand();
          }
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

void LiveSchemaTree::discard_object_data(mforms::TreeNodeRef& node, int data_mask) {
  mforms::TreeNodeRef parent_node;

  if (data_mask & COLUMN_DATA) {
    LSTData* pdata = dynamic_cast<LSTData*>(node->get_data());
    if (pdata->get_type() == Table)
      parent_node = node->get_child(TABLE_COLUMNS_NODE_INDEX);
    else
      parent_node = node;

    parent_node->remove_children();
  }

  if (data_mask & INDEX_DATA) {
    parent_node = node->get_child(TABLE_INDEXES_NODE_INDEX);
    parent_node->remove_children();
  }

  if (data_mask & TRIGGER_DATA) {
    parent_node = node->get_child(TABLE_TRIGGERS_NODE_INDEX);
    parent_node->remove_children();
  }

  if (data_mask & FK_DATA) {
    parent_node = node->get_child(TABLE_FOREIGN_KEYS_NODE_INDEX);
    parent_node->remove_children();
  }
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> LiveSchemaTree::overlay_icons_for_tree_node(mforms::TreeNodeRef node) {
  LSTData* data = dynamic_cast<LSTData*>(node->get_data());

  std::vector<std::string> icons;
  if (data) {
    switch (data->get_type()) {
      case Schema:
        icons.push_back(mforms::App::get()->get_resource_path("wb_item_overlay_inspector.png"));
        icons.push_back(mforms::App::get()->get_resource_path("wb_item_overlay_editor.png"));
        break;
      case Table:
      case View:
        icons.push_back(mforms::App::get()->get_resource_path("wb_item_overlay_inspector.png"));
        icons.push_back(mforms::App::get()->get_resource_path("wb_item_overlay_editor.png"));
        icons.push_back(mforms::App::get()->get_resource_path("wb_item_overlay_result.png"));
        break;
      case Procedure:
      case Function:
        icons.push_back(mforms::App::get()->get_resource_path("wb_item_overlay_editor.png"));
        icons.push_back(mforms::App::get()->get_resource_path("wb_item_overlay_execute.png"));
        break;
      default:
        break;
    }
  }
  return icons;
}

//--------------------------------------------------------------------------------------------------
