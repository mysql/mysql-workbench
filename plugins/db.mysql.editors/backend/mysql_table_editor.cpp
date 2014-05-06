/*
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql_table_editor.h"
#include "grt/grt_dispatcher.h"
#include "grtdb/db_object_helpers.h"
#include "db.mysql/src/module_db_mysql.h"
#include "grt/validation_manager.h"

#include "base/string_utilities.h"

#include "mforms/code_editor.h"

using namespace bec;
using namespace base;

MySQLTableColumnsListBE::MySQLTableColumnsListBE(MySQLTableEditorBE *owner)
  : bec::TableColumnsListBE(owner)
{
}

bool MySQLTableColumnsListBE::set_field(const NodeId &node, ColumnId column, const std::string &value)
{
  db_mysql_ColumnRef col;

  if (node.is_valid() && node[0] < real_count())
  {
    col= static_cast<MySQLTableEditorBE*>(_owner)->table()->columns().get(node[0]);
    if (!col.is_valid())
      return false;

    switch ((ColumnListColumns)column)
    {
    case Type:
      // Remove auto increment for non integer types
      if (is_int_datatype(value) == false)
        col->autoIncrement(false);
      break;
    case Default:
      // If a default value is set then auto increment for a column doesn't make sense.
      if (!base::trim(value).empty())
      {
        AutoUndoEdit undo(_owner);

        bool result = TableColumnsListBE::set_field(node, column, value);
        col->autoIncrement(false);
        undo.end(
          strfmt(_("Set Default Value and Unset Auto Increment '%s.%s'"), _owner->get_name().c_str(),
            col->name().c_str()
          )
        );

        return result;
      }
      break;
    default:
      break;
    }
  }
  return TableColumnsListBE::set_field(node, column, value);
}

bool MySQLTableColumnsListBE::set_field(const ::bec::NodeId &node, ColumnId column, ssize_t value)
{
  db_mysql_ColumnRef col;
  
  if (node.is_valid() && node[0] < real_count())
  {
    col= static_cast<MySQLTableEditorBE*>(_owner)->table()->columns().get(node[0]);
    if (!col.is_valid())
      return false;

    db_SimpleDatatypeRef columnType;
    switch ((MySQLColumnListColumns)column)
    {
    case IsAutoIncrement:
      // Determine actually used column type first.
      if (col->userType().is_valid() && col->userType()->actualType().is_valid())
        columnType= col->userType()->actualType();
      else
        if (col->simpleType().is_valid() && col->simpleType()->group().is_valid())
          columnType= col->simpleType();
        
      if (columnType.is_valid() && columnType->group().is_valid())
      {
        // Allow removing the auto inc setting even for non-numeric columns so we can
        // switch that off *after* we changed the column type or for invalid/old models
        // which have an auto inc set for non-numeric columns.
        if (is_int_datatype(columnType->name()))
        {
          AutoUndoEdit undo(_owner);

          if (value)
          {
            // check if there's already a column with auto-increment set and unset them
            grt::ListRef<db_mysql_Column> columns(static_cast<MySQLTableEditorBE*>(_owner)->table()->columns());
            
            for (size_t c = columns.count(), i= 0; i < c; i++)
            {
              if (*columns[i]->autoIncrement() != 0 && col != columns[i])
              {
                columns[i]->autoIncrement(0);
              }
            }
          }
                    
          col->autoIncrement(value != 0);

          // If auto increment is enabled then reset any default value.
          if (col->autoIncrement() && !(*col->defaultValue()).empty())
            bec::ColumnHelper::set_default_value(col, "");

          // if this is a primary key and auto-inc was set, then we should move this to the
          // beginning of the pk index 
          if (value && *_owner->get_table()->isPrimaryKeyColumn(col))
          {
            db_IndexRef index(_owner->get_table()->primaryKey());
            size_t oindex= 0;
            bool found= false;

            for (size_t c= index->columns().count(), i= 0; i < c; i++)
            {
              if (index->columns()[i]->referencedColumn() == col)
              {
                found= true;
                oindex= i;
                break;
              }
            }
            if (found)
            {
              index->columns().reorder(oindex, 0);
            }
          }
          _owner->update_change_date();
          (*_owner->get_table()->signal_refreshDisplay())("column");
          undo.end(value ? 
            strfmt(_("Set Auto Increment '%s.%s'"), _owner->get_name().c_str(), col->name().c_str()) : 
            strfmt(_("Unset Auto Increment '%s.%s'"), _owner->get_name().c_str(), col->name().c_str()));
        }
        else
          col->autoIncrement(false);
      }
      return true;
    case IsAutoIncrementable:
      return false;
    }
  }
  return TableColumnsListBE::set_field(node, column, value);
}

bool MySQLTableColumnsListBE::get_field_grt(const ::bec::NodeId &node, ColumnId column, grt::ValueRef &value)
{
  db_mysql_ColumnRef col;
  
  if (node.is_valid())
  {
    if (node[0] < real_count())
      col= static_cast<MySQLTableEditorBE*>(_owner)->table()->columns().get(node[0]);
    if (col.is_valid())
    {
      switch (column)
      {
      case IsAutoIncrement:
        value= col->autoIncrement();
        return true;
      case IsAutoIncrementable:
        value = grt::IntegerRef(0);
        if (col->simpleType().is_valid() && col->simpleType()->group().is_valid())
        {
          if (col->simpleType()->group()->name() == "numeric")
            value= grt::IntegerRef(1);
        }
        return true;
      case HasCharset:
        value= grt::IntegerRef(0);
        if (col->simpleType().is_valid())
        {
          if (col->simpleType()->group()->name() == "string" || col->simpleType()->group()->name() == "text"
            || col->simpleType()->name() == "ENUM")
            value= grt::IntegerRef(1);
        }
        return true;          
      }
    }
  }
  return TableColumnsListBE::get_field_grt(node, column, value);
}

static bool can_be_timestamp(const char *value)
{
  if (*value == '\'')
    return true;
  
  // accept anything that looks like a date
  for (; *value; ++value)
    if (!(isdigit(*value) || *value == ':' || *value == '-' || *value == '.' || *value == ' '))
      return false;
  return true;
}


bec::MenuItemList MySQLTableColumnsListBE::get_popup_items_for_nodes(const std::vector<bec::NodeId> &nodes)
{
  bec::MenuItemList items = bec::TableColumnsListBE::get_popup_items_for_nodes(nodes);
  bec::MenuItem item;
  
  if (nodes.size() == 1)
  {
    grt::ListRef<db_Column> columns(static_cast<MySQLTableEditorBE*>(_owner)->table()->columns());
    const size_t idx = nodes.front()[0];

    db_ColumnRef col;
    if (idx < columns.count())
      col = columns.get(idx);

    if (col.is_valid() && col->simpleType().is_valid())
    {
      std::string type = col->simpleType()->name();
      bool improved_timestamp_support = false;
      GrtVersionRef target_version = _owner->get_rdbms_target_version();
      // in MySQL 5.6, CURRENT_TIMESTAMP works for TIMESTAMP and DATETIME
      // and for any number of colums
      if (target_version.is_valid() &&
         (*target_version->majorNumber() > 5 ||
            (*target_version->majorNumber() == 5 &&
             *target_version->minorNumber() >= 6)))
          improved_timestamp_support = true;
      
      if (type == "TIMESTAMP" || (improved_timestamp_support && type == "DATETIME"))
      {
        bool seen_current_ts = false;
        bool current_timestamp_allowed = false; // only the 1st TIMESTAMP column can have CURRENT_TIMESTAMP, unless all
        // previous ones are set to 0 or a constant
        bool flag = false;
        
        if (improved_timestamp_support)
          current_timestamp_allowed = true;
        else
        {
          GRTLIST_FOREACH(db_Column, columns, c)
          {
            if ((*c)->simpleType().is_valid() && (*c)->simpleType()->name() == "TIMESTAMP")
            {
              if (*c == col)
              {
                current_timestamp_allowed= !seen_current_ts;
                flag = true;
              }
              
              if (!((*c)->defaultValue() == "0" || can_be_timestamp((*c)->defaultValue().c_str())))
                seen_current_ts = true;

              // a column before some other TS column that is already marked as CURRENT_TIMESTAMP cannot become CURRENT_TS
              if (*c != col && flag && strstr((*c)->defaultValue().c_str(), "TIMESTAMP"))
                current_timestamp_allowed = false;
            }
          }
        }

        item.caption = "Default 0";
        item.name    = "TSToolStripMenuItem";
        item.enabled = true;
        items.push_back(item);        

        item.caption = "Default CURRENT_TIMESTAMP";
        item.name    = "currentTSToolStripMenuItem";
        item.enabled = current_timestamp_allowed;
        items.push_back(item);

        item.caption = "Default NULL ON UPDATE CURRENT_TIMESTAMP";
        item.name    = "currentTSNullOnUpdateToolStripMenuItem";
        item.enabled = current_timestamp_allowed;
        items.push_back(item);

        item.caption = "Default CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP";
        item.name    = "currentTSOnUpdateToolStripMenuItem";
        item.enabled = current_timestamp_allowed;
        items.push_back(item);
      }
      else if (col->simpleType()->group()->name() == "numeric" ||
               col->simpleType()->group()->name() == "datetime")
      {
        item.caption = "Default 0";
        item.name    = "0ToolStripMenuItem";
        item.enabled = true;
        items.push_back(item);
      }
      else if (col->simpleType()->group()->name() == "string" ||
               col->simpleType()->group()->name() == "text")
      {
        item.caption = "Default ''";
        item.name    = "EmptyToolStripMenuItem";
        item.enabled = true;
        items.push_back(item);
      }
    }
  }
  return items;
}


bool MySQLTableColumnsListBE::activate_popup_item_for_nodes(const std::string &name, const std::vector<bec::NodeId> &orig_nodes)
{
  AutoUndoEdit undo(_owner);
  std::string value;
  bool changed= false;

  if (name == "TSToolStripMenuItem" || name == "0ToolStripMenuItem")
    value = "0";
  else if (name == "EmptyToolStripMenuItem")
    value = "''";
  else if (name == "currentTSToolStripMenuItem")
    value = "CURRENT_TIMESTAMP";
  else if (name == "currentTSNullOnUpdateToolStripMenuItem")
    value = "NULL ON UPDATE CURRENT_TIMESTAMP";
  else if (name == "currentTSOnUpdateToolStripMenuItem")
    value = "CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP";

  if (!value.empty())
  {
    for (std::vector<bec::NodeId>::const_iterator iter= orig_nodes.begin();
         iter != orig_nodes.end(); ++iter)
    {
      if ((*iter)[0] < real_count())
      {
        db_ColumnRef col(_owner->get_table()->columns().get((*iter)[0]));
        
        if (col.is_valid())
        {
          col->defaultValue(value);
          changed= true;
        }
      }
    }
  }
  if (changed)
  {
    undo.end(_("Set Column Default"));
    _owner->do_partial_ui_refresh(TableEditorBE::RefreshColumnList);
    return true;
  }
  else
    undo.cancel();
  return TableColumnsListBE::activate_popup_item_for_nodes(name, orig_nodes);
}  


//--------------------------------------------------------------------------------------------------

#include <mforms/table.h>
#include <mforms/treenodeview.h>
#include <mforms/textentry.h>
#include <mforms/button.h>
#include <mforms/box.h>
#include <mforms/label.h>
#include <mforms/toolbar.h>

class MySQLTriggerPanel : public mforms::Box
{
public:
  MySQLTriggerPanel(MySQLTableEditorBE *editor)
  : mforms::Box(true), _editor(editor), _list(mforms::TreeFlatList|mforms::TreeSizeSmall), _button(mforms::SmallButton), _refreshing(false)
  {
    _edited_trigger_index = 0;

    set_spacing(8);
    set_padding(4);
    
    _list.set_size(230, -1);
    _list.set_name("triggers list");
    add(&_list, false, true);
    _rtable.set_name("trigger right pane");
    add(&_rtable, true, true);
    
    _rtable.set_row_count(2);
    _rtable.set_column_count(5);
    
    _button.set_text("Delete Trigger");
    _rtable.set_row_spacing(8);
    _rtable.set_column_spacing(6);
    
    _name.set_name("trigger name");
    _name.set_read_only(true);
    //TODO: Check if we can make this code usable again
//    _name.signal_changed()->connect(boost::bind(&MySQLTriggerPanel::name_changed, this));
    _definer.set_name("trigger definer");
    _definer.set_read_only(true);
    //TODO: Check if we can make this code usable again
//    _definer.signal_changed()->connect(boost::bind(&MySQLTriggerPanel::definer_changed, this));

    _namel.set_style(mforms::SmallStyle);
    _definerl.set_style(mforms::SmallStyle);
    _namel.set_text("Trigger Name:");
    _definerl.set_text("Definer:");
    _rtable.add(&_namel, 0, 1, 0, 1, mforms::HFillFlag);
    _rtable.add(&_name, 1, 2, 0, 1, mforms::HExpandFlag|mforms::HFillFlag);
    _rtable.add(&_definerl, 2, 3, 0, 1, mforms::HFillFlag);
    _rtable.add(&_definer, 3, 4, 0, 1, mforms::HExpandFlag|mforms::HFillFlag);
    _rtable.add(&_button, 4, 5, 0, 1, mforms::HFillFlag);

    _list.add_column(mforms::StringColumnType, _("Name"), 110, false);
    _list.add_column(mforms::StringColumnType, _("Timing/Event"), 100, false);
    _list.end_columns();
    _list.signal_changed()->connect(boost::bind(&MySQLTriggerPanel::selection_changed, this));

    _rtable.add(editor->get_sql_editor()->get_container(), 0, 5, 1, 2,
                mforms::HExpandFlag|mforms::VExpandFlag|mforms::HFillFlag|mforms::VFillFlag);
    editor->get_sql_editor()->set_sql_check_enabled(true);
    editor->get_sql_editor()->get_editor_control()->set_language(mforms::LanguageMySQL);
    
    _button.signal_clicked()->connect(boost::bind(&MySQLTriggerPanel::clicked, this));
    _code_editor = _editor->get_sql_editor()->get_editor_control();
    _code_editor->signal_lost_focus()->connect(boost::bind(&MySQLTriggerPanel::code_edited, this));
    refresh();
    selection_changed();
  }

  ~MySQLTriggerPanel()
  {
    _rtable.remove(_editor->get_sql_editor()->get_container());
  }

  static bool compare_order(db_TriggerRef a, db_TriggerRef b)
  {
    return a->order() > b->order();
  }
  
  void refresh()
  {
    _refreshing = true;

    std::set<std::string> leftover;
    leftover.insert("BEFORE INSERT");
    leftover.insert("AFTER INSERT");
    leftover.insert("BEFORE UPDATE");
    leftover.insert("AFTER UPDATE");
    leftover.insert("BEFORE DELETE");
    leftover.insert("AFTER DELETE");

    mforms::TreeNodeRef selected = _list.get_selected_node();
    int old_selected = 0;
    if (selected)
      old_selected = _list.row_for_node(selected);
    
    _list.clear();

    grt::ListRef<db_Trigger> triggers(_editor->get_table()->triggers());
    std::map<std::string, db_TriggerRef> trigmap;
    GRTLIST_FOREACH(db_Trigger, triggers, trig)
    {
      std::string t = (*trig)->timing();
      t.append(" ").append((*trig)->event());
      trigmap.insert(std::make_pair(t, *trig));
    }
//    std::sort(trigvec.begin(), trigvec.end(), &MySQLTriggerPanel::compare_order);

    mforms::TreeNodeRef node;
    std::map<std::string, db_TriggerRef>::iterator it;
    for (std::set<std::string>::const_iterator t = leftover.begin(); t != leftover.end(); ++t)
    {
      node = _list.add_node();
      if ((it = trigmap.find(*t)) != trigmap.end())
        node->set_string(0, it->second->name());
      else
        node->set_string(0, "-");

      node->set_string(1, *t);
    }

    _list.select_node(_list.node_at_row(old_selected));

    _refreshing = false;
  }
  
  void code_edited()
  {
    if (_selected_trigger.is_valid())
    {
      if (_code_editor->is_dirty() && _selected_trigger->sqlDefinition() != _code_editor->get_string_value())
      {
        AutoUndoEdit undo(_editor, _selected_trigger, "sql");
        _editor->freeze_refresh_on_object_change();
        grt::IntegerRef res = _editor->_sql_parser->parse_trigger(_selected_trigger, _code_editor->get_string_value().c_str());
        db_TriggerRef temp_trigger;
        _editor->_parsing_services->parseTrigger(temp_trigger, _code_editor->get_string_value());

        _editor->thaw_refresh_on_object_change(true);
        _name.set_value(_selected_trigger->name());
        _definer.set_value(_selected_trigger->definer());
        mforms::TreeNodeRef node = _list.node_at_row(_edited_trigger_index);
        if (node)
        {
          node->set_string(0, _selected_trigger->name());
        }
        undo.end(strfmt(_("Edit trigger `%s` of `%s`.`%s`"), _selected_trigger->name().c_str(), _editor->get_schema_name().c_str(), _editor->get_name().c_str()));
      }
    }
    else
      refresh();
  }

  void clicked()
  {
    std::string timing, event;
    mforms::TreeNodeRef node = _list.get_selected_node();
    
    if (base::partition(node->get_string(1), " ", timing, event))
    {
      grt::ListRef<db_Trigger> triggers(_editor->get_table()->triggers());
      db_TriggerRef trigger;
      GRTLIST_FOREACH(db_Trigger, triggers, trig)
      {
        if ((*trig)->timing() == timing && (*trig)->event() == event)
        {
          trigger = *trig;
          break;
        }
      }
      
      if (trigger.is_valid())
      {
        _editor->freeze_refresh_on_object_change();
        AutoUndoEdit undo(_editor);
        triggers.remove_value(trigger);
        undo.end(base::strfmt("Delete trigger %s", trigger->name().c_str()));
        
        node->set_string(0, "-");
        _editor->thaw_refresh_on_object_change(true);
      }
      else
      {
        _editor->freeze_refresh_on_object_change();
        AutoUndoEdit undo(_editor);
        trigger = db_mysql_TriggerRef(_editor->get_grt());
        trigger->owner(_editor->get_table());
        trigger->name(base::strfmt("%s_%c%s", _editor->get_name().c_str(),
                                   timing[0], event.substr(0, 3).c_str()));
        trigger->event(event);
        trigger->timing(timing);
        triggers.insert(trigger);
        undo.end(base::strfmt("Added trigger to %s.%s", _editor->get_schema_name().c_str(), _editor->get_name().c_str()));
        node->set_string(0, trigger->name());
        _editor->thaw_refresh_on_object_change(true);
      }
      update_editor();
    }
  }
  
  void selection_changed()
  {
    if (_refreshing)
      return;
    if (_code_editor->is_dirty())
      code_edited();

    update_editor();
  }


  void update_editor()
  {
    mforms::TreeNodeRef node = _list.get_selected_node();
    if (!node)
    {
      _list.select_node(_list.node_at_row(0));
      return;
    }

    std::string timing, event;
    std::string sql;
    bool editor_enabled = true;
    base::partition(node->get_string(1), " ", timing, event);
    grt::ListRef<db_Trigger> triggers(_editor->get_table()->triggers());
    db_TriggerRef trigger;
    GRTLIST_FOREACH(db_Trigger, triggers, trig)
    {
      if ((*trig)->timing() == timing && (*trig)->event() == event)
      {
        trigger = *trig;
        break;
      }
    }

    bool empty_trigger = false;
    if (_selected_trigger != trigger)
    {
      _selected_trigger = trigger;

      if (trigger.is_valid())
      {
        _button.set_text("Delete Trigger");
        _name.set_value(trigger->name());
        _definer.set_value(trigger->definer());
        _name.set_enabled(true);
        _definer.set_enabled(true);
      
        if (trigger->sqlDefinition().empty())
        {
          sql.append(base::strfmt("CREATE TRIGGER `%s`.`%s` %s %s ON `%s` FOR EACH ROW\n    ", _editor->get_schema_name().c_str(),
                                  trigger->name().c_str(), timing.c_str(), event.c_str(), _editor->get_name().c_str()));
        }
        else
          sql.append(trigger->sqlDefinition());

        _edited_trigger_index = _list.row_for_node(_list.get_selected_node());
        _code_editor->set_text_keeping_state(sql.c_str());
      }
      else
      {
        empty_trigger = true;
      }
    }

    if (empty_trigger || !_selected_trigger.is_valid())
    {
      _name.set_value("");
      _definer.set_value("");
      _name.set_enabled(false);
      _name.set_read_only(true);
      _definer.set_enabled(false);
      _definer.set_read_only(true);
      _button.set_text("Add Trigger");
      editor_enabled = false;

      _edited_trigger_index = _list.row_for_node(_list.get_selected_node());
      sql = "-- Trigger not defined. Click Add Trigger to create it.\n";
      _code_editor->set_text_keeping_state(sql.c_str());
    }
    _button.set_enabled(true);
    _code_editor->reset_dirty();
    _code_editor->set_enabled(editor_enabled);
  }
  
private:
  MySQLTableEditorBE *_editor;
  mforms::Table _rtable;
  mforms::TreeNodeView _list;
  mforms::TextEntry _name;
  mforms::TextEntry _definer;
  mforms::Label _namel;
  mforms::Label _definerl;
  mforms::Button _button;
  mforms::CodeEditor *_code_editor;
  db_TriggerRef _selected_trigger;
  int _edited_trigger_index;
  bool _refreshing;
};

//--------------------------------------------------------------------------------------------------

MySQLTableEditorBE::MySQLTableEditorBE(::bec::GRTManager *grtm, db_mysql_TableRef table, const db_mgmt_RdbmsRef &rdbms)
  : TableEditorBE(grtm, table, rdbms), _table(table), _columns(this), _partitions(this), _indexes(this), _trigger_panel(0)
{
  _updating_triggers = false;
  if (_table->isStub() == 1)
  {
    int rc;
    rc = mforms::Utilities::show_warning(
      _("Edit Stub Table"),
      _("The table you are trying to edit is a model-only stub, created to represent missing external tables referenced by foreign keys.\n"
        "Such tables are ignored by forward engineering and synchronization.\n\n"
        "You may convert this table to a real one that appears also in the generated SQL or keep it as stub."),
                                         _("Convert to real table"), _("Edit as is"));
    if (rc == mforms::ResultOk)
      db_mysql_TableRef(table)->isStub(0);
  }
}

MySQLTableEditorBE::~MySQLTableEditorBE()
{
  delete _trigger_panel;
}

mforms::View *MySQLTableEditorBE::get_trigger_panel()
{
  if (!_trigger_panel)
    _trigger_panel = new MySQLTriggerPanel(this);
  return _trigger_panel;
}


std::vector<std::string> MySQLTableEditorBE::get_index_types()
{
  std::vector<std::string> index_types;

  index_types.push_back("INDEX");
  index_types.push_back("UNIQUE");
  // FULLTEXT exists only in MyISAM prior to 5.6. in 5.6+ InnoDB also supports it
  if (_table->tableEngine() == "MyISAM" || (_table->tableEngine() == "InnoDB" && is_server_version_at_least(5, 6)))
    index_types.push_back("FULLTEXT");
  // SPATIAL is not supported by InnoDB
  if (_table->tableEngine() == "MyISAM")
    index_types.push_back("SPATIAL");

  // these are special types for PK and FK
  index_types.push_back("PRIMARY");
//  index_types.push_back("FOREIGN");
  return index_types;
}

std::vector<std::string> MySQLTableEditorBE::get_index_storage_types()
{
  std::vector<std::string> index_types;

  index_types.push_back("BTREE"); // BTREE is supported by all engines
  if (_table->tableEngine() == "MyISAM")
    index_types.push_back("RTREE"); // as of 5.7, RTREE is recognized by parser but not supported
  if (_table->tableEngine() == "MEMORY" || _table->tableEngine() == "HEAP" || _table->tableEngine() == "ndbcluster")
    index_types.push_back("HASH");
  
  return index_types;
}

std::vector<std::string> MySQLTableEditorBE::get_fk_action_options()
{
  std::vector<std::string> action_options;

  action_options.push_back("RESTRICT");
  action_options.push_back("CASCADE");
  action_options.push_back("SET NULL");
  action_options.push_back("NO ACTION");
  
  return action_options;
}

std::vector<std::string> MySQLTableEditorBE::get_engines_list()
{
  std::vector<std::string> engines;

  DbMySQLImpl *module= get_grt()->find_native_module<DbMySQLImpl>("DbMySQL");
  if (!module)
    throw std::runtime_error("Module DbMySQL could not be located");

  grt::ListRef<db_mysql_StorageEngine> engines_ret(module->getKnownEngines());

  for (size_t c= engines_ret.count(), i= 0; i < c; i++)
    engines.push_back(engines_ret[i]->name());

  return engines;
}

/**
 * Determines if the currently set engine supports foreign keys and reports the outcome to the caller.
 */
bool MySQLTableEditorBE::engine_supports_foreign_keys()
{
  grt::StringRef name = _table->tableEngine();
  if (name == "") // No engine set. Assume db default allows FKs.
    return true;
  
  db_mysql_StorageEngineRef engine = bec::TableHelper::get_engine_by_name(get_grt(), name);
  if (engine.is_valid())
    return engine->supportsForeignKeys() == 1;
  
  return false; // Don't know anything about this engine, so assume it doesn't support FKs.
}

static struct TableOption
{
  const char *option_name;
  const char *object_field;
  bool text;
} table_options[]= {
  {"PACK_KEYS",       "packKeys", false},
  {"PASSWORD",        "password", true},
  {"AUTO_INCREMENT",  "nextAutoInc", true},
  {"DELAY_KEY_WRITE", "delayKeyWrite", false},
  {"ROW_FORMAT",      "rowFormat", true},
  {"KEY_BLOCK_SIZE",  "keyBlockSize", true},
  {"AVG_ROW_LENGTH",  "avgRowLength", true},
  {"MAX_ROWS",        "maxRows", true},
  {"MIN_ROWS",        "minRows", true},
  {"DATA DIRECTORY",  "tableDataDir", true},
  {"INDEX DIRECTORY", "tableIndexDir", true},
  {"UNION",           "mergeUnion", true},
  {"INSERT_METHOD",   "mergeInsert", true},
  {"ENGINE",          "tableEngine", false},
  {"CHARACTER SET",   "defaultCharacterSetName", false},
  {"COLLATE",         "defaultCollationName", false},
  {"CHECKSUM",        "checksum", false},
  {NULL, NULL, false}
};

void MySQLTableEditorBE::set_table_option_by_name(const std::string& name, const std::string& value)
{
  //g_message("%s('%s','%s')", __FUNCTION__, name.c_str(), value.c_str());
  bool found= false;

  for (size_t i= 0; table_options[i].option_name; i++)
  {
    if (name.compare(table_options[i].option_name) == 0)
    {
      if (_table.get_metaclass()->get_member_type(table_options[i].object_field).base.type == grt::IntegerType)
      {
        int ivalue= atoi(value.c_str());

        if (ivalue != *grt::IntegerRef::cast_from(_table.get_member(table_options[i].object_field)))
        {
          AutoUndoEdit undo(this);
          _table.set_member(table_options[i].object_field, grt::IntegerRef(ivalue));
          update_change_date();
          undo.end(strfmt(_("Change '%s' for '%s'"), name.c_str(), _table->name().c_str()));
        }
      }
      else
      {
        if (value != *grt::StringRef::cast_from(_table.get_member(table_options[i].object_field)))
        {
          if (table_options[i].text)
          {
            AutoUndoEdit undo(this, _table, table_options[i].object_field);

            update_change_date();
            _table.set_member(table_options[i].object_field, grt::StringRef(value));
            
            undo.end(strfmt(_("Change '%s' for '%s'"), name.c_str(), _table->name().c_str()));
          }
          else
          {
            AutoUndoEdit undo(this);
            _table.set_member(table_options[i].object_field, grt::StringRef(value));
            update_change_date();
            undo.end(strfmt(_("Change '%s' for '%s'"), name.c_str(), _table->name().c_str()));
          }

          if ("ENGINE" == name)
            bec::ValidationManager::validate_instance(_table, "chk_fk_lgc");
        }
      }
      found= true;
      break;
    }
  }

  if (found)
    return;

  if(name.compare("CHARACTER SET - COLLATE") == 0)
  { // shortcut that sets both CHARACTER SET and COLLATE separated by a - 
    if (value != get_table_option_by_name(name))
    {
      std::string charset, collation;
      parse_charset_collation(value, charset, collation);
      if (charset != *_table->defaultCharacterSetName() || collation != *_table->defaultCollationName())
      {
        RefreshUI::Blocker blocker(*this);
        AutoUndoEdit undo(this);
        set_table_option_by_name("CHARACTER SET", charset);
        set_table_option_by_name("COLLATE", collation);
        update_change_date();
        undo.end(strfmt(_("Change Charset/Collation for '%s'"), _table->name().c_str()));
      }
    }
  }
  else
    throw std::invalid_argument("Invalid option "+name);
}

std::string MySQLTableEditorBE::get_table_option_by_name(const std::string& name)
{
  if(name.compare("PACK_KEYS") == 0)
    return _table->packKeys();
  else if(name.compare("PASSWORD") == 0)
    return _table->password();
  else if(name.compare("AUTO_INCREMENT") == 0)
    return _table->nextAutoInc();
  else if(name.compare("DELAY_KEY_WRITE") == 0)
    return _table->delayKeyWrite().repr();
  else if(name.compare("ROW_FORMAT") == 0)
    return _table->rowFormat();
  else if(name.compare("KEY_BLOCK_SIZE") == 0)
    return _table->keyBlockSize();
  else if(name.compare("AVG_ROW_LENGTH") == 0)
    return _table->avgRowLength();
  else if(name.compare("MAX_ROWS") == 0)
    return _table->maxRows();
  else if(name.compare("MIN_ROWS") == 0)
    return _table->minRows();
  else if(name.compare("CHECKSUM") == 0)
    return _table->checksum().repr();
  else if(name.compare("DATA DIRECTORY") == 0)
    return _table->tableDataDir();
  else if(name.compare("INDEX DIRECTORY") == 0)
    return _table->tableIndexDir();
  else if(name.compare("UNION") == 0)
    return _table->mergeUnion();
  else if(name.compare("INSERT_METHOD") == 0)
    return _table->mergeInsert();
  else if(name.compare("ENGINE") == 0)
    return _table->tableEngine();
  else if(name.compare("CHARACTER SET - COLLATE") == 0)
    return format_charset_collation(_table->defaultCharacterSetName().c_str(), _table->defaultCollationName().c_str());
  else if(name.compare("CHARACTER SET") == 0)
    return _table->defaultCharacterSetName();
  else if(name.compare("COLLATE") == 0)
    return _table->defaultCollationName();
  else
    throw std::invalid_argument("Invalid option "+name);
  return std::string("");
}

//--------------------------------------------------------------------------------------------------

/**
 * Loads the current trigger sql text into the editor control and marks that as not dirty.
 */
void MySQLTableEditorBE::load_trigger_sql()
{
  if (_trigger_panel && !_updating_triggers)
  {
    _updating_triggers = true;
    _trigger_panel->refresh();
    _trigger_panel->update_editor();
    _updating_triggers = false;
  }
}


//--------------------------------------------------------------------------------------------------

bool MySQLTableEditorBE::can_close()
{
  _trigger_panel->code_edited(); // Same handling as for focus-lost. Might not be necessary but better safe than sorry.
  return TableEditorBE::can_close();
}

//--------------------------------------------------------------------------------------------------

bool MySQLTableEditorBE::set_partition_type(const std::string &type)
{
  if (type.compare(*table()->partitionType())!=0)
  {
    if (type == "RANGE" || type == "LIST")
    {
      AutoUndoEdit undo(this);
      table()->partitionType(type);
      if (table()->partitionCount() == 0)
        table()->partitionCount(1);
      if (get_explicit_partitions())
        reset_partition_definitions((int)table()->partitionCount(), 
        get_explicit_subpartitions() ? (int)*table()->subpartitionCount() : 0);
      update_change_date();
      undo.end(strfmt(_("Change Partition Type for '%s'"), get_name().c_str()));
      return true;
    }
    else if (type == "LINEAR HASH" || type == "HASH" || 
             type == "LINEAR KEY" || type == "KEY" || type == "")
    {
      AutoUndoEdit undo(this);
      table()->partitionType(type);
      if (table()->partitionCount() == 0)
        table()->partitionCount(1);
      table()->subpartitionCount(0);
      table()->subpartitionExpression("");
      table()->subpartitionType("");
      if (get_explicit_partitions())
        reset_partition_definitions((int)table()->partitionCount(), 0);
      update_change_date();
      undo.end(strfmt(_("Change Partition Type for '%s'"), get_name().c_str()));
      return true;
    }
  }
  return false;
}


std::string MySQLTableEditorBE::get_partition_type()
{
  return *table()->partitionType();
}


void MySQLTableEditorBE::set_partition_expression(const std::string &expr)
{
  AutoUndoEdit undo(this, table(), "partitionExpression");

  table()->partitionExpression(expr);
  
  update_change_date();
  undo.end(strfmt(_("Set Partition Expression for '%s'"), get_name().c_str()));
}


std::string MySQLTableEditorBE::get_partition_expression()
{
  return *table()->partitionExpression();
}


void MySQLTableEditorBE::set_partition_count(int count)
{
  AutoUndoEdit undo(this);
  if (count > 0)
    table()->partitionCount(count);
  else
    table()->partitionCount(1);
  if (get_explicit_partitions())
    reset_partition_definitions((int)table()->partitionCount(),
    get_explicit_partitions() ? (int)*table()->subpartitionCount() : 0);
  update_change_date();
  undo.end(strfmt(_("Set Partition Count for '%s'"), get_name().c_str()));
}


int MySQLTableEditorBE::get_partition_count()
{
  return (int)*table()->partitionCount();
}


bool MySQLTableEditorBE::subpartition_count_allowed()
{
  return (*table()->partitionType() == "RANGE" || *table()->partitionType() == "LIST");
}

bool MySQLTableEditorBE::set_subpartition_type(const std::string &type)
{
  if (*table()->partitionType() == "RANGE" || *table()->partitionType() == "LIST")
  {
    AutoUndoEdit undo(this, table(), "subpartitionType");

    table()->subpartitionType(type);
    
    update_change_date();
    undo.end(strfmt(_("Set Subpartition Type for '%s'"), get_name().c_str()));
    return true;
  }
  return false;
}


std::string MySQLTableEditorBE::get_subpartition_type()
{
  return *table()->subpartitionType();
}


bool MySQLTableEditorBE::set_subpartition_expression(const std::string &expr)
{
  if (*table()->partitionType() == "RANGE" || *table()->partitionType() == "LIST")
  {
    AutoUndoEdit undo(this, table(), "subpartitionExpression");

    table()->subpartitionExpression(expr);

    update_change_date();
    undo.end(strfmt(_("Set Subpartition Expression for '%s'"), get_name().c_str()));
    return true;
  }
  return false;
}


std::string MySQLTableEditorBE::get_subpartition_expression()
{
  return *table()->subpartitionExpression();
}


void MySQLTableEditorBE::set_subpartition_count(int count)
{
  if (*table()->partitionType() == "RANGE" || *table()->partitionType() == "LIST")
  {
    AutoUndoEdit undo(this);
    table()->subpartitionCount(count);
    if (get_explicit_subpartitions())
      reset_partition_definitions((int)table()->partitionCount(), (int)table()->subpartitionCount());
    update_change_date();
    undo.end(strfmt(_("Set Subpartition Count for '%s'"), get_name().c_str()));
  }
}


int MySQLTableEditorBE::get_subpartition_count()
{
  return (int)*table()->subpartitionCount();
}


void MySQLTableEditorBE::set_explicit_partitions(bool flag)
{
  if (flag != get_explicit_partitions())
  {
    AutoUndoEdit undo(this);
    if (flag)
    {
      if (table()->partitionCount() == 0)
      {
        table()->partitionCount(2);
      }
      reset_partition_definitions((int)table()->partitionCount(), (int)table()->subpartitionCount());
    }
    else
      reset_partition_definitions(0, 0);
    update_change_date();
    undo.end(flag ? 
      strfmt(_("Manually Define Partitions for '%s'"), get_name().c_str()) :
      strfmt(_("Implicitly Define Partitions for '%s'"), get_name().c_str()));
  }
}


void MySQLTableEditorBE::set_explicit_subpartitions(bool flag)
{
  if (flag != get_explicit_subpartitions())
  {
    if (get_explicit_partitions())
    {
      AutoUndoEdit undo(this);
      if (flag)
      {
        if (table()->subpartitionCount() == 0)
        {
          table()->subpartitionCount(2);
        }
        reset_partition_definitions((int)table()->partitionCount(), (int)table()->subpartitionCount());
      }
      else
        reset_partition_definitions((int)table()->partitionCount(), 0);
      update_change_date();
      undo.end(flag ? 
        strfmt(_("Manually Define SubPartitions for '%s'"), get_name().c_str()) :
        strfmt(_("Implicitly Define SubPartitions for '%s'"), get_name().c_str()));
    }
  }
}


bool MySQLTableEditorBE::get_explicit_partitions()
{
  return table()->partitionDefinitions().count() > 0;
}


bool MySQLTableEditorBE::get_explicit_subpartitions()
{
  return table()->partitionDefinitions().count() > 0 
    && table()->partitionDefinitions().get(0)->subpartitionDefinitions().count() > 0;
}

void MySQLTableEditorBE::reset_partition_definitions(int parts, int subparts)
{
  grt::ListRef<db_mysql_PartitionDefinition> pdefs(table()->partitionDefinitions());

  AutoUndoEdit undo(this);

  while ((int)pdefs.count() < parts)
  {
    db_mysql_PartitionDefinitionRef part(get_grt());

    part->owner(table());
    part->name(grt::StringRef::format("part%i", pdefs.count()));
    pdefs.insert(part);
  }

  while ((int)pdefs.count() > parts)
  {
    pdefs.remove(pdefs.count()-1);
  }

  for (size_t c= pdefs.count(), i= 0; i < c; i++)
  {
    grt::ListRef<db_mysql_PartitionDefinition> spdefs(pdefs[i]->subpartitionDefinitions());

    while ((int)spdefs.count() < subparts)
    {
      db_mysql_PartitionDefinitionRef part(get_grt());

      part->owner(pdefs[i]);
      part->name(grt::StringRef::format("subpart%i", i*subparts + spdefs.count()));
      spdefs.insert(part);
    }

    while ((int)spdefs.count() > subparts)
    {
      spdefs.remove(spdefs.count()-1);
    }
  }

  update_change_date();
  undo.end("Reset Partitioning");
}


db_TableRef MySQLTableEditorBE::create_stub_table(const std::string &schema, const std::string &table)
{
  db_SchemaRef dbschema = grt::find_named_object_in_list(get_catalog()->schemata(), schema, false);
  db_TableRef dbtable;

  if (dbschema.is_valid())
    dbtable = grt::find_named_object_in_list(dbschema->tables(), table);
  else
  {
    dbschema = db_mysql_SchemaRef(get_grt());
    dbschema->owner(get_catalog());
    dbschema->name(schema);
    dbschema->comment("This schema was created for a stub table");
    get_catalog()->schemata().insert(dbschema);
  }

  if (!dbtable.is_valid())
  {
    dbtable = db_mysql_TableRef(get_grt());
    dbtable->owner(dbschema);
    dbtable->name(table);
    dbtable->isStub(1);
    dbschema->tables().insert(dbtable);
  }

  return dbtable;
}


static db_SimpleDatatypeRef get_simple_datatype(const db_ColumnRef &column)
{
  if (column->simpleType().is_valid())
    return column->simpleType();
  if (column->userType().is_valid())
    return column->userType()->actualType();
  return db_SimpleDatatypeRef();
}

bool MySQLTableEditorBE::check_column_referenceable_by_fk(const db_ColumnRef &column1, const db_ColumnRef &column2)
{  
  // from 5.1 manual: 
  // - Corresponding columns in the foreign key and the referenced key must have similar internal data types
  // inside InnoDB so that they can be compared without a type conversion. 
  // - The size and sign of integer types must be the same. 
  // - The length of string types need not be the same. 
  // - For nonbinary (character) string columns, the character set and collation must be the same.
  
  db_SimpleDatatypeRef stype1 = get_simple_datatype(column1);
  db_SimpleDatatypeRef stype2 = get_simple_datatype(column2);

  if (!stype1.is_valid() || !stype2.is_valid())
    return false;
  
  if (stype1 != stype2)
    return false;
  
  if (stype1->group()->name() == "numeric")
  {
    // check sign (size is already checked by previous if)
    bool unsigned1= column1->flags().get_index("UNSIGNED") != grt::BaseListRef::npos;
    bool unsigned2= column2->flags().get_index("UNSIGNED") != grt::BaseListRef::npos;

    if (unsigned1 != unsigned2)
      return false;
  }

  if (stype1->group()->name() == "string")
  {
    // check collation and charset
    if (column1->characterSetName() != column2->characterSetName() || column1->collationName() != column2->collationName())
      return false;
  }

  return true;
}

//--------------------------------------------------------------------------------

MySQLTablePartitionTreeBE::MySQLTablePartitionTreeBE(MySQLTableEditorBE *owner)
: _owner(owner)
{
}


bool MySQLTablePartitionTreeBE::set_field(const NodeId &node, ColumnId column, const std::string &value)
{
  db_mysql_PartitionDefinitionRef pdef(get_definition(node));

  if (!pdef.is_valid())
    return false;

  switch ((Columns)column)
  {
  case Name:
    if (pdef->name() != value)
    {
      AutoUndoEdit undo(_owner, pdef, "name");

      pdef->name(value);
      
      _owner->update_change_date();
      undo.end(strfmt(_("Change Partition Name for '%s'"), _owner->get_name().c_str()));
    }
    return true;

  case Value:
    if (pdef->value() != value)
    {
      AutoUndoEdit undo(_owner, pdef, "value");

      pdef->value(value);

      _owner->update_change_date();
      undo.end(strfmt(_("Change Partition Parameter for '%s'"), _owner->get_name().c_str()));
    }
    return true;

  case MinRows:
    if (pdef->minRows() != value)
    {
      AutoUndoEdit undo(_owner, pdef, "minRows");

      pdef->minRows(value);
      
      _owner->update_change_date();
      undo.end(strfmt(_("Change Partition Min Rows for '%s'"), _owner->get_name().c_str()));
    }
    return true;

  case MaxRows:
    if (pdef->maxRows() != value)
    {
      AutoUndoEdit undo(_owner, pdef, "maxRows");
      
      pdef->maxRows(value);

      _owner->update_change_date();
      undo.end(strfmt(_("Change Partition Max Rows for '%s'"), _owner->get_name().c_str()));
    }
    return true;

  case DataDirectory:
    if (pdef->dataDirectory() != value)
    {
      AutoUndoEdit undo(_owner, pdef, "dataDirectory");

      pdef->dataDirectory(value);

      _owner->update_change_date();
      undo.end(strfmt(_("Change Partition Data Directory for '%s'"), _owner->get_name().c_str()));
    }
    return true;

  case IndexDirectory:
    if (pdef->indexDirectory() != value)
    {
      AutoUndoEdit undo(_owner, pdef, "indexDirectory");

      pdef->indexDirectory(value);
      
      _owner->update_change_date();
      undo.end(strfmt(_("Change Partition Index Directory for '%s'"), _owner->get_name().c_str()));
    }
    return true;

  case Comment:
    if (pdef->comment() != value)
    {
      AutoUndoEdit undo(_owner, pdef, "comment");

      pdef->comment(value);

      _owner->update_change_date();
      undo.end(strfmt(_("Change Partition Comment for '%s'"), _owner->get_name().c_str()));
    }
    return true;
  }

  return false;
}


bool MySQLTablePartitionTreeBE::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value)
{
  db_mysql_PartitionDefinitionRef pdef(get_definition(node));

  if (!pdef.is_valid())
    return false;

  switch ((Columns)column)
  {
  case Name:
    value= pdef->name();
    return true;

  case Value:
    value= pdef->value();
    return true;

  case MinRows:
    value= pdef->minRows();
    return true;

  case MaxRows:
    value= pdef->maxRows();
    return true;

  case DataDirectory:
    value= pdef->dataDirectory();
    return true;

  case IndexDirectory:
    value= pdef->indexDirectory();
    return true;

  case Comment:
    value= pdef->comment();
    return true;
  }

  return false;
}


grt::Type MySQLTablePartitionTreeBE::get_field_type(const NodeId &node, ColumnId column)
{
  return grt::StringType;
}


db_mysql_PartitionDefinitionRef MySQLTablePartitionTreeBE::get_definition(const NodeId &node)
{
  if (node.depth() == 1)
  {
    if (node[0] < _owner->table()->partitionDefinitions().count())
      return _owner->table()->partitionDefinitions()[node[0]];
  }
  else if (node.depth() == 2)
  {
    if (node[0] < _owner->table()->partitionDefinitions().count())
    {
      db_mysql_PartitionDefinitionRef def(_owner->table()->partitionDefinitions()[node[0]]);

      if (node[1] < def->subpartitionDefinitions().count())
        return def->subpartitionDefinitions()[node[1]];
    }
  }
  return db_mysql_PartitionDefinitionRef();
}



size_t MySQLTablePartitionTreeBE::count_children(const NodeId &parent)
{
  if (parent.depth() == 1)
  {
    db_mysql_PartitionDefinitionRef def(get_definition(parent));

    if (def.is_valid())
      return (int)def->subpartitionDefinitions().count();
  }
  else if (parent.depth() == 0)
    return (int)_owner->table()->partitionDefinitions().count();

  return 0;
}


NodeId MySQLTablePartitionTreeBE::get_child(const NodeId &parent, size_t index)
{
  if (count_children(parent) > index)
    return NodeId(parent).append(index);

  throw std::logic_error("Invalid index");
}


//-------------------------------------------------------------------------------------------------

MySQLTableIndexListBE::MySQLTableIndexListBE(MySQLTableEditorBE *owner)
: IndexListBE(owner)
{
}


bool MySQLTableIndexListBE::set_field(const NodeId &node, ColumnId column, const std::string &value)
{
  if (!index_editable(get_selected_index()))
    return IndexListBE::set_field(node, column, value);

  db_mysql_IndexRef index(db_mysql_IndexRef::cast_from(get_selected_index()));

  if (!index.is_valid())
    return IndexListBE::set_field(node, column, value);

  switch (column)
  {
  case StorageType:
    if (value != *index->indexKind())
    {
      AutoUndoEdit undo(_owner, index, "indexKind");
      index->indexKind(value);
      undo.end(strfmt(_("Change Storage Type of Index '%s.%s'"), _owner->get_name().c_str(), index->name().c_str()));
    }
    return true;
  case RowBlockSize:
    if (atoi(value.c_str()) != *index->keyBlockSize())
    {
      AutoUndoEdit undo(_owner, index, "keyBlockSize");
      index->keyBlockSize(atoi(value.c_str()));
      undo.end(strfmt(_("Change Key Block Size of Index '%s.%s'"), _owner->get_name().c_str(), index->name().c_str()));
    }
    return true;
  case Parser:
    if (value != *index->withParser())
    {
      AutoUndoEdit undo(_owner, index, "withParser");
      index->withParser(value);
      undo.end(strfmt(_("Change Parser of Index '%s.%s'"), _owner->get_name().c_str(), index->name().c_str()));
    }
    return true;
  default:
    return IndexListBE::set_field(node, column, value);
  }
}


bool MySQLTableIndexListBE::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value)
{
  if ( node.is_valid() )
  {
    const bool existing_node = node.end() < real_count();
    
    switch (column)
    {
    case StorageType:
      value= existing_node ? db_mysql_IndexRef::cast_from(get_selected_index())->indexKind() : grt::StringRef("");
      return true;
    case RowBlockSize:
      value= existing_node ? grt::StringRef(db_mysql_IndexRef::cast_from(get_selected_index())->keyBlockSize().repr()) : grt::StringRef("");
      return true;
    case Parser:
      value= existing_node ? db_mysql_IndexRef::cast_from(get_selected_index())->withParser() : grt::StringRef("");
      return true;
    default:
      return IndexListBE::get_field_grt(node, column, value);
    }
  }
  return false;
}
