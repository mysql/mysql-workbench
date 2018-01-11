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

#include "workbench_physical_tablefigure_impl.h"
#include "workbench_physical_diagram_impl.h"
#include "workbench_physical_model_impl.h"
#include "model_layer_impl.h"

#include "table_figure.h"
#include "table_figure_wb.h"

#include "grt/grt_manager.h"
#include "grtdb/db_object_helpers.h"

#include "base/wb_iterators.h"
#include "base/string_utilities.h"

using namespace base;

workbench_physical_TableFigure::ImplData::ImplData(workbench_physical_TableFigure *self) : super(self), _figure(0) {
  _pending_columns_sync = false;
  _pending_index_sync = false;
  _pending_trigger_sync = false;

  scoped_connect(self->signal_changed(),
                 std::bind(&ImplData::member_changed, this, std::placeholders::_1, std::placeholders::_2));
}

void workbench_physical_TableFigure::ImplData::update_options(const std::string &key) {
  if (key == "workbench.physical.TableFigure:MaxColumnsDisplayed") {
    workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(self()->owner()->owner()));
    int max_columns = model->get_data()->get_int_option(key, 30);

    if (_figure)
      _figure->set_max_columns_shown(max_columns);
  }

  if (base::hasPrefix(key, "workbench.physical.ObjectFigure:") ||
      base::hasPrefix(key, "workbench.physical.TableFigure:")) {
    if (_figure)
      sync_columns();

    if (key == "workbench.physical.TableFigure:ShowSchemaName") {
      if (self()->owner()->owner()->get_data()->get_int_option("workbench.physical.TableFigure:ShowSchemaName", 0) !=
          0) {
        std::string title = *self()->_table->owner()->name();
        title += ".";
        title += *self()->_table->name();
        _figure->get_title()->set_title(title);
      } else
        _figure->get_title()->set_title(*self()->_table->name());
    }
  }
}

void workbench_physical_TableFigure::ImplData::set_in_view(bool flag) {
  if (!self()->owner().is_valid())
    throw std::logic_error("adding figure to view before setting owner");

  if (flag) {
    if (self()->_table.is_valid()) {
      workbench_physical_DiagramRef diagram(workbench_physical_DiagramRef::cast_from(self()->_owner));
      diagram->get_data()->add_mapping(self()->_table, self());
    }
  } else {
    if (self()->_table.is_valid()) {
      workbench_physical_DiagramRef diagram(workbench_physical_DiagramRef::cast_from(self()->_owner));
      diagram->get_data()->remove_mapping(self()->_table);
    }
  }

  model_Figure::ImplData::set_in_view(flag);
}

void workbench_physical_TableFigure::ImplData::set_table(const db_TableRef &table) {
  // Check if we had a valid table before and revert the previous setup if so.
  if (self()->_table.is_valid()) {
    if (self()->_owner.is_valid())
      workbench_physical_DiagramRef::cast_from(self()->_owner)->get_data()->remove_mapping(self()->_table);

    _table_fk_conn.disconnect();
    _refresh_conn.disconnect();
    _changed_conn.disconnect();
  }

  // Now replace the table and run its setup.
  self()->_table = table;

  if (self()->_table.is_valid()) {
    if (self()->_owner.is_valid())
      workbench_physical_DiagramRef::cast_from(self()->_owner)->get_data()->add_mapping(table, self());

    _table_fk_conn =
      table->signal_foreignKeyChanged()->connect(std::bind(&ImplData::fk_changed, this, std::placeholders::_1));
    _refresh_conn =
      table->signal_refreshDisplay()->connect(std::bind(&ImplData::content_changed, this, std::placeholders::_1));
    _changed_conn = table->signal_changed()->connect(
      std::bind(&ImplData::table_member_changed, this, std::placeholders::_1, std::placeholders::_2));

    self()->_name = self()->_table->name();

    if (_figure) {
      // Refresh everything because table has been replaced.
      _figure->get_title()->set_title(*self()->_table->name());
      run_later(std::bind(&workbench_physical_TableFigure::ImplData::sync_columns, this));
      run_later(std::bind(&workbench_physical_TableFigure::ImplData::sync_indexes, this));
      run_later(std::bind(&workbench_physical_TableFigure::ImplData::sync_triggers, this));
    } else {
      _table_fk_conn_block = std::shared_ptr<boost::signals2::shared_connection_block>(
        new boost::signals2::shared_connection_block(_table_fk_conn));
      _changed_conn_block = std::shared_ptr<boost::signals2::shared_connection_block>(
        new boost::signals2::shared_connection_block(_changed_conn));
      _refresh_conn_block = std::shared_ptr<boost::signals2::shared_connection_block>(
        new boost::signals2::shared_connection_block(_refresh_conn));
      try_realize();
    }
  } else
    unrealize();
}

void workbench_physical_TableFigure::ImplData::table_member_changed(const std::string &name,
                                                                    const grt::ValueRef &ovalue) {
  if (name == "name") {
    self()->_name = self()->_table->name();

    if (_figure)
      _figure->get_title()->set_title(*self()->_table->name());
  } else if (name == "primaryKey") {
    if (_figure) {
      if (!_pending_columns_sync) {
        _pending_columns_sync = true;
        run_later(std::bind(&workbench_physical_TableFigure::ImplData::sync_columns, this));
      }
    }
  }
}

void workbench_physical_TableFigure::ImplData::fk_changed(const db_ForeignKeyRef &fk) {
  // resync columns to update FK indicators
  if (_figure) {
    if (!_pending_columns_sync) {
      _pending_columns_sync = true;
      run_later(std::bind(&workbench_physical_TableFigure::ImplData::sync_columns, this));
    }
  }
}

void workbench_physical_TableFigure::ImplData::content_changed(const std::string &where) {
  if ((where == "column" || where == "foreignKey") && _figure && !_pending_columns_sync) {
    _pending_columns_sync = true;
    run_later(std::bind(&workbench_physical_TableFigure::ImplData::sync_columns, this));
  }

  if (where == "index" && _figure && !_pending_index_sync) {
    _pending_index_sync = true;
    run_later(std::bind(&workbench_physical_TableFigure::ImplData::sync_indexes, this));
  }

  if (where == "trigger" && _figure && !_pending_trigger_sync) {
    _pending_trigger_sync = true;
    run_later(std::bind(&workbench_physical_TableFigure::ImplData::sync_triggers, this));
  }
}

void workbench_physical_TableFigure::ImplData::member_changed(const std::string &name, const grt::ValueRef &ovalue) {
  /* not good  if (name == "name")
    {
      if (self()->_table.is_valid())
        self()->_table->name(self()->_name);
    }
    */
  /*
  else if (name == "columnsExpanded")
  {
    return; // ignore
  }*/
  /*else*/ if (name == "indicesExpanded") {
    if (_figure)
      _figure->toggle_indexes(*self()->_indicesExpanded != 0);
  } else if (name == "triggersExpanded") {
    if (_figure)
      _figure->toggle_triggers(*self()->_triggersExpanded != 0);
  } else if (name == "color" && self()->owner().is_valid() && self()->owner()->owner().is_valid() &&
             self()->owner()->owner()->get_data()->get_int_option("SynchronizeObjectColors", 0)) {
    // this actually shouldn't be done here as there's no way to tell whether the color is being
    // changed by the user or because the object is being placed
    // once direct object property editing (from the Properties sidebar box) is removed, this can
    // be removed
    if (*grt::StringRef::cast_from(ovalue) != "")
      self()->owner()->owner()->get_data()->update_object_color_in_all_diagrams(self()->_color, "table",
                                                                                self()->_table.id());
    super::member_changed(name, ovalue);
  } else if (!get_canvas_item()) {
    // do some sanity checks
    if (name == "width") {
      if (*self()->_width <= 20)
        self()->_manualSizing = 0;
    } else if (name == "height") {
      if (*self()->_height <= 20)
        self()->_manualSizing = 0;
    }
  }
}

void workbench_physical_TableFigure::ImplData::sync_columns() {
  if (_figure) {
    wbfig::Table::ItemList::iterator iter = _figure->begin_columns_sync();

    grt::ListRef<db_Column> columns(self()->_table->columns());
    bool show_type =
      self()->owner()->owner()->get_data()->get_int_option("workbench.physical.TableFigure:ShowColumnTypes", 1) != 0;
    bool show_flags =
      self()->owner()->owner()->get_data()->get_int_option("workbench.physical.TableFigure:ShowColumnFlags", 0) != 0;
    int max_type_length =
      self()->owner()->owner()->get_data()->get_int_option("workbench.physical.TableFigure:MaxColumnTypeLength", 20);
    for (size_t c = columns.count(), i = 0; i < c; i++) {
      db_ColumnRef column = columns.get(i);
      std::string text;
      wbfig::ColumnFlags flags = (wbfig::ColumnFlags)0;

      if (self()->_table->isPrimaryKeyColumn(column))
        flags = (wbfig::ColumnFlags)(flags | wbfig::ColumnPK);
      if (self()->_table->isForeignKeyColumn(column))
        flags = (wbfig::ColumnFlags)(flags | wbfig::ColumnFK);

      if (column->isNotNull())
        flags = (wbfig::ColumnFlags)(flags | wbfig::ColumnNotNull);

      if (column->flags().get_index("UNSIGNED") != grt::BaseListRef::npos)
        flags = (wbfig::ColumnFlags)(flags | wbfig::ColumnUnsigned);
      if (column.has_member("autoIncrement") && column.get_integer_member("autoIncrement") != 0)
        flags = (wbfig::ColumnFlags)(flags | wbfig::ColumnAutoIncrement);

      text = *column->name();
      if (show_type) {
        std::string type = column->formattedRawType();

        // truncate enum and set types if its too long
        if (max_type_length > 0 && (int)type.length() > max_type_length) {
          if (g_ascii_strncasecmp(type.c_str(), "set(", 4) == 0) {
            type = type.substr(0, 3).append("(...)");
          } else if (g_ascii_strncasecmp(type.c_str(), "enum(", 5) == 0) {
            type = type.substr(0, 4).append("(...)");
          }
        }
        text.append(" ").append(type);
      }

      iter = _figure->sync_next_column(iter, column.id(), flags, text);
    }

    _figure->set_show_flags(show_flags);
    _figure->end_columns_sync(iter);

    _figure->set_dependant(self()->_table->isDependantTable() != 0);
  }
  _pending_columns_sync = false;
}

void workbench_physical_TableFigure::ImplData::sync_indexes() {
  if (_figure) {
    wbfig::Table::ItemList::iterator iter = _figure->begin_indexes_sync();

    grt::ListRef<db_Index> indexes(self()->_table->indices());

    for (size_t c = indexes.count(), i = 0; i < c; i++) {
      db_IndexRef index(indexes.get(i));
      std::string text;

      text = *index->name();

      iter = _figure->sync_next_index(iter, index.id(), text);
    }
    _figure->end_indexes_sync(iter);

    if (_figure->get_index_title() && !_figure->indexes_hidden())
      _figure->get_index_title()->set_visible(indexes.count() > 0);
  }
  _pending_index_sync = false;
}

static bool compare_trigger(const std::pair<std::string, std::string> &a,
                            const std::pair<std::string, std::string> &b) {
  return a.second.substr(5) > b.second.substr(5);
}

void workbench_physical_TableFigure::ImplData::sync_triggers() {
  if (_figure) {
    grt::ListRef<db_Trigger> triggers(self()->_table->triggers());
    std::vector<std::pair<std::string, std::string> > items;

    for (size_t c = triggers.count(), i = 0; i < c; i++) {
      db_TriggerRef trigger(triggers.get(i));
      std::string text;

      if (g_ascii_strcasecmp(trigger->timing().c_str(), "AFTER") == 0)
        text = "AFT";
      else
        text = "BEF";

      std::string event = trigger->event();
      if (g_ascii_strcasecmp(event.c_str(), "INSERT") == 0)
        text.append(" INSERT ");
      else if (g_ascii_strcasecmp(event.c_str(), "UPDATE") == 0)
        text.append(" UPDATE ");
      else if (g_ascii_strcasecmp(event.c_str(), "DELETE") == 0)
        text.append(" DELETE ");

      text.append(trigger->name());

      items.push_back(std::make_pair(trigger.id(), text));
    }

    std::sort(items.begin(), items.end(), compare_trigger);

    wbfig::Table::ItemList::iterator iter = _figure->begin_triggers_sync();
    for (base::const_range<std::vector<std::pair<std::string, std::string> > > trig(items); trig; ++trig)
      iter = _figure->sync_next_trigger(iter, trig->first, trig->second);
    _figure->end_triggers_sync(iter);

    if (_figure->get_trigger_title() && !_figure->triggers_hidden())
      _figure->get_trigger_title()->set_visible(triggers.count() > 0);
  }
  _pending_trigger_sync = false;
}

bool workbench_physical_TableFigure::ImplData::is_realizable() {
  if (!super::is_realizable())
    return false;

  if (self()->_table.is_valid())
    return true;

  return false;
}

void workbench_physical_TableFigure::ImplData::unrealize() {
  workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(self()->owner()->owner()));

  notify_will_unrealize();

  // remove tag badges
  std::list<meta_TagRef> tags(model->get_data()->get_tags_for_dbobject(self()->_table));
  for (std::list<meta_TagRef>::const_iterator end = tags.end(), tag = tags.begin(); tag != end; ++tag) {
    self()->owner()->get_data()->remove_tag_badge_from_figure(self(), *tag);
  }

  super::unrealize();

  delete _figure;
  _figure = 0;
}

bool workbench_physical_TableFigure::ImplData::realize() {
  if (_figure)
    return true;
  if (!is_realizable())
    return false;

  if (!is_main_thread()) {
    run_later(std::bind(&ImplData::realize, this));
    return true;
  }

  if (!_figure) {
    mdc::CanvasView *view = get_canvas_view();
    mdc::AreaGroup *agroup;
    workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(self()->owner()->owner()));

    view->lock();

    _figure = model->get_data()->create_table_figure(view->get_current_layer(), self()->owner(), self());

    update_options("workbench.physical.TableFigure:MaxColumnsDisplayed");

    agroup = self()->_layer->get_data()->get_area_group();

    view->get_current_layer()->add_item(_figure, agroup);

    _figure->set_color(Color::parse(*self()->_color));

    if (self()->owner()->owner()->get_data()->get_int_option("workbench.physical.TableFigure:ShowSchemaName", 0) != 0) {
      std::string title = *self()->_table->owner()->name();
      title += ".";
      title += *self()->_table->name();
      _figure->get_title()->set_title(title);
    } else
      _figure->get_title()->set_title(*self()->_table->name());

    scoped_connect(_figure->get_title()->signal_expand_toggle(),
                   std::bind(&workbench_physical_TableFigure::ImplData::toggle_title, this, std::placeholders::_1,
                             _figure->get_title()));
    if (_figure->get_index_title()) {
      scoped_connect(_figure->get_index_title()->signal_expand_toggle(),
                     std::bind(&workbench_physical_TableFigure::ImplData::toggle_title, this, std::placeholders::_1,
                               _figure->get_index_title()));
    }
    if (_figure->get_trigger_title()) {
      scoped_connect(_figure->get_trigger_title()->signal_expand_toggle(),
                     std::bind(&workbench_physical_TableFigure::ImplData::toggle_title, this, std::placeholders::_1,
                               _figure->get_trigger_title()));
    }

    _figure->set_dependant(self()->_table->isDependantTable() != 0);

    std::string sfont = self()->owner()->owner()->get_data()->get_string_option(
      strfmt("%s:SectionFont", self()->class_name().c_str()), "");
    if (!sfont.empty())
      _figure->set_section_font(mdc::FontSpec::from_string(sfont));

    _figure->toggle(*self()->_expanded != 0);
    _figure->toggle_indexes(*self()->_indicesExpanded != 0);
    _figure->toggle_triggers(*self()->_triggersExpanded != 0);

    if (self()->_table->columns().count() > 0)
      sync_columns();
    if (self()->_table->indices().count() > 0)
      sync_indexes();
    if (self()->_table->triggers().count() > 0)
      sync_triggers();

    finish_realize();

    view->unlock();

    notify_realized();

    // add badges for each tag
    std::list<meta_TagRef> tags(model->get_data()->get_tags_for_dbobject(self()->_table));
    for (std::list<meta_TagRef>::const_iterator end = tags.end(), tag = tags.begin(); tag != end; ++tag) {
      self()->owner()->get_data()->add_tag_badge_to_figure(self(), *tag);
    }

    // unblock refresh signals
    if (_table_fk_conn_block.get())
      _table_fk_conn_block->unblock();
    if (_changed_conn_block.get())
      _changed_conn_block->unblock();
    if (_refresh_conn_block.get())
      _refresh_conn_block->unblock();
  }
  return true;
}

void workbench_physical_TableFigure::ImplData::toggle_title(bool expanded, wbfig::Titlebar *sender) {
  if (sender == _figure->get_title()) {
    grt::AutoUndo undo;
    self()->expanded(expanded);
    undo.end(expanded ? _("Expand Table") : _("Collapse Table"));
  } else if (sender == _figure->get_index_title()) {
    grt::AutoUndo undo;
    self()->indicesExpanded(expanded);
    undo.end(expanded ? _("Expand Table Indices") : _("Collapse Table Indices"));
  } else if (sender == _figure->get_trigger_title()) {
    grt::AutoUndo undo;
    self()->triggersExpanded(expanded);
    undo.end(expanded ? _("Expand Table Triggers") : _("Collapse Table Triggers"));
  }
}

db_ColumnRef workbench_physical_TableFigure::ImplData::get_column_at(mdc::CanvasItem *item) {
  if (_figure && !_figure->get_columns()->empty()) {
    for (wbfig::Table::ItemList::const_iterator iter = _figure->get_columns()->begin();
         iter != _figure->get_columns()->end(); ++iter) {
      if ((*iter) == item)
        return grt::find_object_in_list(self()->_table->columns(), (*iter)->get_id());
    }
  }
  return db_ColumnRef();
}

db_IndexRef workbench_physical_TableFigure::ImplData::get_index_at(mdc::CanvasItem *item) {
  if (_figure && _figure->get_indexes() && !_figure->get_indexes()->empty()) {
    for (wbfig::Table::ItemList::const_iterator iter = _figure->get_indexes()->begin();
         iter != _figure->get_indexes()->end(); ++iter) {
      if ((*iter) == item)
        return grt::find_object_in_list(self()->_table->indices(), (*iter)->get_id());
    }
  }
  return db_IndexRef();
}

void workbench_physical_TableFigure::ImplData::set_column_highlighted(const db_ColumnRef &column, const Color *color) {
  if (_figure) {
    for (wbfig::Table::ItemList::const_iterator iter = _figure->get_columns()->begin();
         iter != _figure->get_columns()->end(); ++iter) {
      if (!column.is_valid() || (*iter)->get_id() == column.id()) {
        (*iter)->set_highlight_color(color);
        (*iter)->set_highlighted(true);
        if (column.is_valid())
          break;
      }
    }
  }
}

void workbench_physical_TableFigure::ImplData::set_column_unhighlighted(const db_ColumnRef &column) {
  if (_figure) {
    for (wbfig::Table::ItemList::const_iterator iter = _figure->get_columns()->begin();
         iter != _figure->get_columns()->end(); ++iter) {
      if (!column.is_valid() || (*iter)->get_id() == column.id()) {
        (*iter)->set_highlighted(false);
        if (column.is_valid())
          break;
      }
    }
    _figure->set_needs_render();
  }
}
