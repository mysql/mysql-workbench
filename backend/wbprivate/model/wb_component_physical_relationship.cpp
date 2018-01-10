/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_component_physical.h"
#include "model/wb_model_diagram_form.h"
#include "relationship_canvas_floater.h"

#include "grtdb/db_object_helpers.h"
#include "grts/structs.db.mysql.h"
#include "base/string_utilities.h"

#include "workbench/wb_context.h"
#include "grtpp_undo_manager.h"

#include "wbcanvas/workbench_physical_diagram_impl.h"
#include "wbcanvas/workbench_physical_tablefigure_impl.h"

using namespace wb;
using namespace base;

WBComponentPhysical::RelationshipToolContext::RelationshipToolContext(WBComponentPhysical *owner,
                                                                      ModelDiagramForm *form, RelationshipType rtype)
  : owner(owner), view(form), state(RPickingStart), type(rtype), floater(0) {
  workbench_physical_Diagram::ImplData *view_bridge =
    workbench_physical_DiagramRef::cast_from(form->get_model_diagram())->get_data();
  if (view_bridge)
    scoped_connect(view_bridge->signal_item_crossed(),
                   std::bind(&RelationshipToolContext::on_figure_crossed, this, std::placeholders::_1,
                             std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

  if (type == RelationshipPick) {
    floater = new RelationshipFloater(view);
    view->add_floater(floater);

    scoped_connect(floater->signal_done_clicked(), std::bind(&RelationshipToolContext::source_picking_done, this));

    last_message = _("Select the Table to receive the Foreign Key or the Column(s) to turn into a Foreign Key.");
  } else if (type == RelationshipnmId)
    last_message = _("Select the first Table to be joined.");
  else {
    floater = 0;
    last_message = _("Select the Table to receive the Foreign Key.");
  }
  owner->get_wb()->_frontendCallbacks->show_status_text(last_message);
}

void WBComponentPhysical::RelationshipToolContext::cancel() {
  if (itable.is_valid()) {
    itable->get_data()->unhighlight();
    itable->get_data()->set_column_unhighlighted(db_ColumnRef());
  }
  if (ftable.is_valid()) {
    ftable->get_data()->unhighlight();
    ftable->get_data()->set_column_unhighlighted(db_ColumnRef());
  }

  if (floater) {
    floater->remove_from_parent();
    delete floater;
    floater = 0;
  }

  if (hovering.is_valid())
    leave_table(hovering);

  if (state != RFinished)
    owner->get_wb()->_frontendCallbacks->show_status_text(_("Cancelled."));
}

bool WBComponentPhysical::RelationshipToolContext::pick_table(const workbench_physical_TableFigureRef &table) {
  workbench_physical_TableFigure::ImplData *tfig = table->get_data();

  if (type == RelationshipnmId && !table->table()->primaryKey().is_valid()) {
    last_message =
      strfmt(_("'%s' has no Primary Key. Please add a PK or select another Table."), table->table()->name().c_str());
    return false;
  }

  itable = table;

  tfig->highlight();

  if (type == RelationshipnmId)
    last_message = strfmt(_("'%s' selected. Please select the second Table."), table->table()->name().c_str());
  else
    last_message = strfmt(_("'%s' selected. Please select the Referenced Table."), table->table()->name().c_str());

  return true;
}

bool WBComponentPhysical::RelationshipToolContext::pick_reftable(const workbench_physical_TableFigureRef &table) {
  if (!table->table()->primaryKey().is_valid()) {
    last_message =
      strfmt(_("'%s' has no Primary Key. Please add a PK or select another Table."), table->table()->name().c_str());
    return false;
  }

  ftable = table;

  return true;
}

/**
 ****************************************************************************
 * @brief Add a column from the source table to a relationship creation context
 *
 * Called by the relationship handling code when the user is picking columns
 * from initial table for a relationship.
 *
 * @param column the column from the source table
 * @return true if the column was added, false if the column was already
 *       added
 ****************************************************************************
 */
bool WBComponentPhysical::RelationshipToolContext::add_column(const db_ColumnRef &column) {
  for (std::vector<db_ColumnRef>::iterator iter = columns.begin(); iter != columns.end(); ++iter) {
    if (*iter == column)
      return false;
  }
  columns.push_back(column);

  std::string name = column->formattedType();
  if (g_utf8_strlen(name.data(), (gssize)name.size()) > 20) {
    gchar *buf = (gchar *)g_malloc((gssize)name.size() + 1);
    g_utf8_strncpy(buf, name.data(), 20);
    name = buf;
    g_free(buf);
  }

  floater->add_column(*column->name() + " " + name);

  return true;
}

bool WBComponentPhysical::RelationshipToolContext::pick_column(const workbench_physical_TableFigureRef &table,
                                                               const db_ColumnRef &column) {
  if (column.is_valid()) {
    // if so, then make sure that all columns belong to the same table and
    // add it to the list of source columns
    if (itable.is_valid()) {
      // column from another table
      if (table != itable) {
        last_message = strfmt(_("Foreign Key columns must belong to the same table."));
        return false;
      }
    }
    if (!add_column(column)) {
      last_message = strfmt(_("Column has already been picked, please pick another or pick the referenced table."));
      return false;
    }

    table->get_data()->set_column_highlighted(column);

    itable = table;
    table->get_data()->highlight();

    last_message = strfmt(_("Column '%s' selected."), column->name().c_str());
    return false;
  } else // !column.is_valid()
  {
    if (!columns.empty()) {
      // already picking columns, require that to be continued
      last_message =
        strfmt(_("Pick other columns that belong to the Foreign Key or proceed to pick the referenced table or "
                 "matching columns in it."));
      return false;
    } else
      return pick_table(table);
  }
}

/**
 ****************************************************************************
 * @brief Add a column from the ref table to a relationship creation context
 *
 * Called by the relationship handling code when the user is picking columns
 * from the final table for a relationship. The column data type must match
 * the type from the initial table that was picked in the same order.
 *
 * @param column the column from the final table
 * @return true if the column was added, false if the columns datatype doesn't
 *    match the expected type
 ****************************************************************************
 */
bool WBComponentPhysical::RelationshipToolContext::add_refcolumn(const db_ColumnRef &column) {
  if (columns.size() <= refcolumns.size())
    return false; // no more columns to add

  // this should be done only on the model validation stage
  // if (!bec::ColumnHelper::compare_column_types(columns[refcolumns.size()], column))
  //  return false;

  refcolumns.push_back(column);

  return true;
}

bool WBComponentPhysical::RelationshipToolContext::pick_refcolumn(const workbench_physical_TableFigureRef &table,
                                                                  const db_ColumnRef &column) {
  if (column.is_valid()) {
    // if so, then make sure that all columns belong to the same table and
    // add it to the list of source columns
    if (ftable.is_valid()) {
      // column from bad table
      if (table != ftable) {
        last_message = strfmt(_("Referenced columns must belong to the same table."));
        return false;
      }
    }
    if (!add_refcolumn(column)) {
      last_message = strfmt(_("Invalid column, please pick an appropriate column with matching type."));
      return false;
    }

    table->get_data()->set_column_highlighted(column);

    ftable = table;
    table->get_data()->highlight();

    if (done_picking_columns())
      return true;
    else {
      floater->pick_next_target();
      last_message = strfmt(_("Column '%s' selected."), column->name().c_str());
    }
    return false;
  } else // !column.is_valid()
  {
    // otherwise, continue and see if the table itself should be picked
    if (!columns.empty())
      // already picking columns, require that to be continued
      last_message = strfmt(_("Please pick the referenced column."));
    else
      return pick_reftable(table);
    return false;
  }
}

bool WBComponentPhysical::RelationshipToolContext::finish() {
  bool flag;
  if (columns.empty())
    flag = finish_for_tables();
  else
    flag = finish_for_columns();
  return flag;
}

bool WBComponentPhysical::RelationshipToolContext::finish_for_columns() {
  bool imany = false, fmany = false;
  bool imand = true, fmand = true;
  // bool identifying= false;

  switch (type) {
    case Relationship11Id:
      imany = false;
      fmany = false; /*identifying= true;*/
      break;
    case Relationship1nId:
      imany = true;
      fmany = false; /*identifying= true;*/
      break;
    case Relationship11NonId:
      imany = false;
      fmany = false; /*identifying= false;*/
      break;
    case Relationship1nNonId:
      imany = true;
      fmany = false; /*identifying= false;*/
      break;
    case RelationshipnmId:
      imany = true;
      fmany = true;
      break;
    case RelationshipPick:
      imany = true;
      fmany = false; /*identifying= true;*/
      break;
    default:
      break;
  }
  imand = view->get_tool_argument("workbench.physical.Connection:optional") != "1";
  fmand = view->get_tool_argument("workbench.physical.Connection:refOptional") != "1";

  itable->get_data()->unhighlight();

  itable->get_data()->set_column_unhighlighted(db_ColumnRef());
  ftable->get_data()->set_column_unhighlighted(db_ColumnRef());

  if (imany && fmany) {
    grt::AutoUndo undo;

    bool ok = owner->create_nm_relationship(view, itable, ftable, imand, fmand);

    if (ok) {
      undo.end(_("Create Relationship"));

      last_message = strfmt(_("Relationship between '%s' and '%s' created through an associative table."),
                            itable->table()->name().c_str(), ftable->table()->name().c_str());
    } else
      last_message = strfmt(_("Cannot create relationship between '%s' and '%s'."), itable->table()->name().c_str(),
                            ftable->table()->name().c_str());
  } else {
    // create the FK, the relationship will be auto-created as a side-effect
    db_ForeignKeyRef fk;

    grt::AutoUndo undo;

    fk = bec::TableHelper::create_foreign_key_to_table(
      itable->table(), columns, ftable->table(), refcolumns, imand, imany,
      workbench_physical_ModelRef::cast_from(view->get_model_diagram()->owner())->rdbms(),
      owner->get_wb()->get_wb_options(), view->get_model_diagram()->owner()->options());
    if (fk.is_valid()) {
      undo.end(_("Create Relationship"));

      last_message = strfmt(_("Relationship between '%s' and '%s' created."), itable->table()->name().c_str(),
                            ftable->table()->name().c_str());
    } else {
      last_message = strfmt(_("Cannot create relationship between '%s' and '%s'."), itable->table()->name().c_str(),
                            ftable->table()->name().c_str());
    }
  }
  return true;
}

bool WBComponentPhysical::RelationshipToolContext::finish_for_tables() {
  bool imany = false, fmany = false;
  bool imand = true, fmand = true;
  bool identifying = false;

  switch (type) {
    case Relationship11Id:
      imany = false;
      fmany = false;
      identifying = true;
      break;
    case Relationship1nId:
      imany = true;
      fmany = false;
      identifying = true;
      break;
    case Relationship11NonId:
      imany = false;
      fmany = false;
      identifying = false;
      break;
    case Relationship1nNonId:
      imany = true;
      fmany = false;
      identifying = false;
      break;
    case RelationshipnmId:
      imany = true;
      fmany = true;
      break;
    case RelationshipPick:
      imany = true;
      fmany = false;
      identifying = false;
      break;
    default:
      break;
  }
  imand = view->get_tool_argument("workbench.physical.Connection:optional") != "1";
  fmand = view->get_tool_argument("workbench.physical.Connection:refOptional") != "1";

  itable->get_data()->unhighlight();

  itable->get_data()->set_column_unhighlighted(db_ColumnRef());
  ftable->get_data()->set_column_unhighlighted(db_ColumnRef());

  if (imany && fmany) {
    grt::AutoUndo undo;
    bool ok = owner->create_nm_relationship(view, itable, ftable, imand, fmand);

    if (ok) {
      undo.end(_("Create Relationship"));

      last_message = strfmt(_("Relationship between '%s' and '%s' created through an associative table."),
                            itable->table()->name().c_str(), ftable->table()->name().c_str());
    } else
      last_message = strfmt(_("Cannot create relationship between '%s' and '%s'."), itable->table()->name().c_str(),
                            ftable->table()->name().c_str());
  } else {
    // create the FK
    db_ForeignKeyRef fk;
    grt::AutoUndo undo;

    {
      grt::AutoUndo fkundo;
      fk = bec::TableHelper::create_foreign_key_to_table(
        itable->table(), ftable->table(), imand, fmand, imany, identifying,
        workbench_physical_ModelRef::cast_from(view->get_model_diagram()->owner())->rdbms(),
        owner->get_wb()->get_wb_options(), view->get_model_diagram()->owner()->options());

      fkundo.end(strfmt("Add ForeignKey to %s", itable->table()->name().c_str()));
    }
    if (fk.is_valid()) {
      workbench_physical_DiagramRef::cast_from(view->get_model_diagram())->createConnectionForForeignKey(fk);

      undo.end(_("Create Relationship"));

      last_message = strfmt(_("Relationship between '%s' and '%s' created."), itable->table()->name().c_str(),
                            ftable->table()->name().c_str());
    } else {
      undo.cancel();

      last_message = strfmt(_("Cannot create relationship between '%s' and '%s'."), itable->table()->name().c_str(),
                            ftable->table()->name().c_str());
    }
  }
  return true;
}

void WBComponentPhysical::RelationshipToolContext::on_figure_crossed(const model_ObjectRef &owner,
                                                                     mdc::CanvasItem *item, bool enter,
                                                                     const Point &pos) {
  if (owner.is_instance<workbench_physical_TableFigure>()) {
    if (enter)
      enter_table(workbench_physical_TableFigureRef::cast_from(owner));
    else
      leave_table(workbench_physical_TableFigureRef::cast_from(owner));
  }
}

void WBComponentPhysical::RelationshipToolContext::enter_table(const workbench_physical_TableFigureRef &table) {
  bool hover_columns = false;

  if (state == RPickingEnd) {
    if (table->table()->columns().count() > 0)
      table->get_data()->get_canvas_item()->set_draws_hover(true);

    if (type == RelationshipPick)
      hover_columns = true;
  } else {
    table->get_data()->get_canvas_item()->set_draws_hover(true);

    if (type == RelationshipPick)
      hover_columns = true;
  }
  hovering = table;

  if (hover_columns) {
    wbfig::Table *tfig = dynamic_cast<wbfig::Table *>(table->get_data()->get_canvas_item());
    if (tfig) {
      wbfig::Table::ItemList *columns = tfig->get_columns();
      for (wbfig::Table::ItemList::iterator iter = columns->begin(); iter != columns->end(); ++iter) {
        (*iter)->set_draws_hover(true);
      }
    }
  }
}

void WBComponentPhysical::RelationshipToolContext::leave_table(const workbench_physical_TableFigureRef &table) {
  wbfig::Table *tfig = dynamic_cast<wbfig::Table *>(table->get_data()->get_canvas_item());
  if (tfig) {
    wbfig::Table::ItemList *columns = tfig->get_columns();
    for (wbfig::Table::ItemList::iterator iter = columns->begin(); iter != columns->end(); ++iter)
      (*iter)->set_draws_hover(false);
  }
  table->get_data()->get_canvas_item()->set_draws_hover(false);
  hovering = workbench_physical_TableFigureRef();
}

bool WBComponentPhysical::RelationshipToolContext::button_press(ModelDiagramForm *view, const Point &pos) {
  std::string result;

  switch (state) {
    case RPickingStart: {
      // check if there's a table at the clicked point
      model_ObjectRef obj = view->get_object_at(pos);
      if (obj.is_valid() && obj.is_instance(workbench_physical_TableFigure::static_class_name())) {
        workbench_physical_TableFigureRef table(workbench_physical_TableFigureRef::cast_from(obj));
        bool done = false;

        workbench_physical_TableFigure::ImplData *tfig = table->get_data();

        if (type == RelationshipPick && tfig) {
          // if we're picking columns, then check if what was clicked is a column
          db_ColumnRef column(tfig->get_column_at(view->get_leaf_item_at(pos)));

          // picked a column from a different table, if only 1 column from the other table was
          // selected, auto-switch to target column picking
          if (table != itable && columns.size() == 1) {
            state = RPickingEnd;
            return button_press(view, pos);
          }
          done = pick_column(table, column);
        } else
          done = pick_table(table);

        if (done)
          state = RPickingEnd;

        result = last_message;
      } else
        result = _("Please select the table to receive the Foreign Key.");

      break;
    }

    case RPickingEnd: {
      // check if there's a table at the clicked point
      model_ObjectRef obj = view->get_object_at(pos);
      if (obj.is_valid() && obj.is_instance(workbench_physical_TableFigure::static_class_name())) {
        workbench_physical_TableFigureRef table(workbench_physical_TableFigureRef::cast_from(obj));
        workbench_physical_TableFigure::ImplData *tfig = table->get_data();
        bool done = false;

        if (type == RelationshipPick && tfig) {
          // if we're picking columns, then check if what was clicked is a column
          db_ColumnRef column(tfig->get_column_at(view->get_leaf_item_at(pos)));
          done = pick_refcolumn(table, column);
        } else
          done = pick_reftable(table);

        if (done && finish())
          state = RFinished;

        result = last_message;
      } else
        result = _("Please select the referenced table.");

      break;
    }

    case RFinished:
      return true;

    case RCancelled:
      return true;

    default:
      return false;
  }

  if (!result.empty()) {
    last_message = result;
    owner->get_wb()->_frontendCallbacks->show_status_text(last_message);
  }

  if (state == RFinished)
    return true;

  return false;
}

void WBComponentPhysical::RelationshipToolContext::source_picking_done() {
  if (columns.size() > 0) {
    floater->setup_pick_target();

    state = RPickingEnd;
    last_message = _("Please pick referenced columns or table.");

    owner->get_wb()->_frontendCallbacks->show_status_text(last_message);
  }
}
