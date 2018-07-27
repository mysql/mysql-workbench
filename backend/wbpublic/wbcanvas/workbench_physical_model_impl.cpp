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

#include "workbench_physical_model_impl.h"
#include "workbench_physical_diagram_impl.h"
#include "workbench_physical_connection_impl.h"

#include "grt/grt_manager.h"

#include "table_figure_wb.h"
#include "table_figure_idef1x.h"
#include "table_figure_simple.h"

workbench_physical_Model::ImplData::ImplData(workbench_physical_Model *self) : super(self) {
  _relationship_notation = PRCrowFoofnotation;
  _figure_notation = PFWorkbenchNotation;

  scoped_connect(self->signal_changed(),
                 std::bind(&ImplData::member_changed_comm, this, std::placeholders::_1, std::placeholders::_2));

  scoped_connect(self->signal_list_changed(), std::bind(&ImplData::list_changed, this, std::placeholders::_1,
                                                        std::placeholders::_2, std::placeholders::_3));
  scoped_connect(self->signal_dict_changed(), std::bind(&ImplData::dict_changed, this, std::placeholders::_1,
                                                        std::placeholders::_2, std::placeholders::_3));
  grt::GRTNotificationCenter::get()->add_grt_observer(this, "GRNPreferencesDidClose");
}

void workbench_physical_Model::ImplData::handle_grt_notification(const std::string &name, grt::ObjectRef sender,
                                                                 grt::DictRef info) {
  //  if (name == "GRNPreferencesDidClose" && info.get_int("saved") == 1)
  {
    //    run_later(std::bind(&workbench_physical_Model::ImplData::reset_figures, this));
    //    run_later(std::bind(&workbench_physical_Model::ImplData::reset_connections, this));
  }
}

workbench_physical_Model::ImplData::~ImplData() {
  grt::GRTNotificationCenter::get()->remove_grt_observer(this);
}

void workbench_physical_Model::ImplData::member_changed_comm(const std::string &name, const grt::ValueRef &ovalue) {
  PhysicalRelationshipNotation rnot;
  PhysicalFigureNotation fnot;
  if (name == "connectionNotation") {
    std::string s = self()->_connectionNotation;
    if (s == "classic")
      rnot = PRClassicNotation;
    else if (s == "idef1x")
      rnot = PRIdef1xNotation;
    else if (s == "crowsfoot" || s == "ie")
      rnot = PRCrowFoofnotation;
    else if (s == "barker")
      rnot = PRBarkerNotation;
    else if (s == "uml")
      rnot = PRUMLNotation;
    else if (s == "fromcolumn")
      rnot = PRFromColumnNotation;
    else
      rnot = PRCrowFoofnotation;
    if (_relationship_notation != rnot) {
      _relationship_notation = rnot;
      run_later(std::bind(&workbench_physical_Model::ImplData::reset_connections, this));
    }
  } else if (name == "figureNotation") {
    std::string s = self()->_figureNotation;
    if (s == "workbench" || s == "workbench/default")
      fnot = PFWorkbenchNotation;
    else if (s == "workbench/simple")
      fnot = PFWorkbenchSimpleNotation;
    else if (s == "workbench/pkonly")
      fnot = PFWorkbenchPKOnlyNotation;
    else if (s == "idef1x")
      fnot = PFIdef1xNotation;
    else if (s == "classic")
      fnot = PFClassicNotation;
    else if (s == "barker")
      fnot = PFBarkerNotation;
    else
      fnot = PFWorkbenchNotation;
    if (_figure_notation != fnot) {
      _figure_notation = fnot;
      run_later(std::bind(&workbench_physical_Model::ImplData::reset_figures, this));
      run_later(std::bind(&workbench_physical_Model::ImplData::reset_connections,
                          this)); /* needed to restore the connection attachments */
    }
  }
}

mdc::LineEndType workbench_physical_Model::ImplData::get_line_end_type(bool mand, bool many, bool start) {
  switch (_relationship_notation) {
    case PRIdef1xNotation:
      if (start)
        return mdc::FilledCircleEnd;
      else if (!mand)
        return mdc::HollowDiamondEnd;
      else
        return mdc::NormalEnd;
    case PRCrowFoofnotation:
      if (mand && many)
        return mdc::ChickenFoot1End;
      else if (!mand && many)
        return mdc::ChickenFoot0End;
      else if (mand && !many)
        return mdc::Cross1End;
      else
        return mdc::Cross0End;
    case PRBarkerNotation:
      if (many)
        return mdc::ChickenFootEnd;
      else
        return mdc::NormalEnd;
    case PRFromColumnNotation:
      return mdc::BoldStickEnd;
    case PRClassicNotation:
    case PRUMLNotation:
      return mdc::NormalEnd;
  }
  return mdc::NormalEnd;
}

std::string workbench_physical_Model::ImplData::get_line_end_caption(bool mand, bool many, bool start) {
  switch (_relationship_notation) {
    case PRIdef1xNotation:
      if (start) {
        if (!mand && many)
          return "";
        else if (!mand && !many)
          return "Z";
        else if (mand && many)
          return "P";
        else if (mand && !many)
          return "1";
      }
      break;

    case PRFromColumnNotation:
      // TODO: if else has same code is this intentional
      if (start) {
        if (many)
          return "\xe2\x88\x9e";
        else
          return "1";
      } else {
        if (many)
          return "\xe2\x88\x9e";
        else
          return "1";
      }
      break;
    case PRCrowFoofnotation:
    case PRBarkerNotation:
      break;

    case PRClassicNotation:
    case PRUMLNotation:
      // TODO: if else has same code is this intentional
      if (start) {
        if (!mand && many)
          return "0..*";
        else if (!mand && !many)
          return "0..1";
        else if (mand && many)
          return "1..*";
        else if (mand && !many)
          return "1";
      } else {
        if (!mand && many)
          return "0..*";
        else if (!mand && !many)
          return "0..1";
        else if (mand && many)
          return "1..*";
        else if (mand && !many)
          return "1";
      }
      break;
  }
  return "";
}

void workbench_physical_Model::ImplData::update_relationship_figure(model_Connection::ImplData *cfig, bool imandatory,
                                                                    bool imany, bool fmandatory, bool fmany) {
  wbfig::Connection *conn = dynamic_cast<wbfig::Connection *>(cfig->get_canvas_item());
  if (conn) {
    conn->set_end_type(get_line_end_type(imandatory, imany, true), get_line_end_type(fmandatory, fmany, false));

    cfig->set_start_caption(get_line_end_caption(imandatory, imany, true));
    cfig->set_end_caption(get_line_end_caption(fmandatory, fmany, false));

    if (_relationship_notation == PRBarkerNotation) {
      conn->set_start_dashed(imandatory);
      conn->set_end_dashed(fmandatory);
    } else if (_relationship_notation == PRClassicNotation) {
      if (imany && fmany)
        conn->set_diamond_type(wbfig::Connection::Filled);
      else if (imany && !fmany)
        conn->set_diamond_type(wbfig::Connection::RightEmpty);
      else if (!imany && fmany)
        conn->set_diamond_type(wbfig::Connection::LeftEmpty);
      else
        conn->set_diamond_type(wbfig::Connection::Empty);
    } else if (_relationship_notation == PRFromColumnNotation) {
    } else {
      conn->set_start_dashed(false);
      conn->set_end_dashed(false);
    }
  }
}

wbfig::Table *workbench_physical_Model::ImplData::create_table_figure(mdc::Layer *layer,
                                                                      const model_DiagramRef &diagram,
                                                                      const model_ObjectRef &forTable) {
  switch (_figure_notation) {
    case PFWorkbenchNotation:
      return new wbfig::WBTable(layer, diagram->get_data(), forTable);

    case PFWorkbenchSimpleNotation: {
      wbfig::WBTable *table = new wbfig::WBTable(layer, diagram->get_data(), forTable);
      table->hide_indices();
      table->hide_triggers();
      return table;
    }

    case PFWorkbenchPKOnlyNotation: {
      wbfig::WBTable *table = new wbfig::WBTable(layer, diagram->get_data(), forTable);
      table->hide_columns();
      table->hide_indices();
      table->hide_triggers();
      return table;
    }

    case PFIdef1xNotation:
      return new wbfig::Idef1xTable(layer, diagram->get_data(), forTable);

    case PFClassicNotation:
      return new wbfig::SimpleTable(layer, diagram->get_data(), forTable);

    case PFBarkerNotation: {
      wbfig::SimpleTable *table = new wbfig::SimpleTable(layer, diagram->get_data(), forTable);
      table->set_barker_notation(true);
      return table;
    }
    default:
      return 0;
  }
}

void workbench_physical_Model::ImplData::tag_list_changed(grt::internal::OwnedList *list, bool added,
                                                          const grt::ValueRef &value, const meta_TagRef &tag) {
  if (list == tag->objects().valueptr()) {
    meta_TaggedObjectRef to(meta_TaggedObjectRef::cast_from(value));
    if (added) {
      db_DatabaseObjectRef dbobj(to->object());
      model_FigureRef figure;

      for (grt::ListRef<workbench_physical_Diagram>::const_iterator end = self()->diagrams().end(),
                                                                    diag = self()->diagrams().begin();
           diag != end; ++diag) {
        figure = (*diag)->get_data()->get_figure_for_dbobject(dbobj);
        if (figure.is_valid()) {
          (*diag)->get_data()->add_tag_badge_to_figure(figure, tag);
        }
      }
    } else {
      db_DatabaseObjectRef dbobj(to->object());
      model_FigureRef figure;

      for (grt::ListRef<workbench_physical_Diagram>::const_iterator end = self()->diagrams().end(),
                                                                    diag = self()->diagrams().begin();
           diag != end; ++diag) {
        figure = (*diag)->get_data()->get_figure_for_dbobject(dbobj);
        if (figure.is_valid()) {
          (*diag)->get_data()->remove_tag_badge_from_figure(figure, tag);
        }
      }
    }
  }
}

void workbench_physical_Model::ImplData::list_changed(grt::internal::OwnedList *list, bool added,
                                                      const grt::ValueRef &value) {
  if (list == self()->_tags.valueptr()) {
    if (added) {
      meta_TagRef tag(meta_TagRef::cast_from(value));

      // monitor changes to list of objects tagged
      _tag_connections[tag.id()] = tag->signal_list_changed()->connect(std::bind(
        &ImplData::tag_list_changed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, tag));
    } else {
      meta_TagRef tag(meta_TagRef::cast_from(value));

      if (_tag_connections.find(tag.id()) != _tag_connections.end()) {
        _tag_connections[tag.id()].disconnect();
        _tag_connections.erase(_tag_connections.find(tag.id()));
      }
    }
  }
}

void workbench_physical_Model::ImplData::dict_changed(grt::internal::OwnedDict *dict, bool added,
                                                      const std::string &key) {
  if (g_str_has_prefix(key.c_str(), "workbench.physical.TableFigure:") ||
      g_str_has_prefix(key.c_str(), "workbench.physical.ViewFigure:") ||
      g_str_has_prefix(key.c_str(), "workbench.physical.RoutineGroupFigure:")) {
    run_later(std::bind(&workbench_physical_Model::ImplData::reset_figures, this));
  }
}

std::list<meta_TagRef> workbench_physical_Model::ImplData::get_tags_for_dbobject(const db_DatabaseObjectRef &dbobject) {
  std::list<meta_TagRef> list;

  for (grt::ListRef<meta_Tag>::const_iterator end = self()->tags().end(), tag = self()->tags().begin(); tag != end;
       ++tag) {
    for (grt::ListRef<meta_TaggedObject>::const_iterator oend = (*tag)->objects().end(),
                                                         oiter = (*tag)->objects().begin();
         oiter != oend; ++oiter) {
      if ((*oiter)->object() == dbobject) {
        list.push_back(*tag);
        break;
      }
    }
  }
  return list;
}
