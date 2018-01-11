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

#ifndef _WORKBENCH_PHYSICAL_MODEL_IMPL_H_
#define _WORKBENCH_PHYSICAL_MODEL_IMPL_H_

#include "model_model_impl.h"
#include "grts/structs.workbench.physical.h"

#include "grt/grt_manager.h"
#include "grtpp_notifications.h"

#include "table_figure.h"
#include "connection_figure.h"

#include "model_connection_impl.h"

enum PhysicalRelationshipNotation {
  PRClassicNotation,
  PRIdef1xNotation,
  PRCrowFoofnotation, // aka Information Engineering
  PRUMLNotation,
  PRFromColumnNotation,
  PRBarkerNotation
};

enum PhysicalFigureNotation {
  PFWorkbenchNotation,
  PFWorkbenchSimpleNotation,
  PFWorkbenchPKOnlyNotation,
  PFIdef1xNotation,
  PFClassicNotation,
  PFBarkerNotation
};

class WBPUBLICBACKEND_PUBLIC_FUNC workbench_physical_Model::ImplData : public model_Model::ImplData,
                                                                       public grt::GRTObserver {
  typedef model_Model::ImplData super;

private:
  PhysicalRelationshipNotation _relationship_notation;
  PhysicalFigureNotation _figure_notation;

  std::map<std::string, boost::signals2::connection> _tag_connections;

  void tag_list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value, const meta_TagRef &tag);
  void list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value);
  void dict_changed(grt::internal::OwnedDict *dict, bool added, const std::string &key);

  // Observer
  virtual void handle_grt_notification(const std::string &name, grt::ObjectRef sender, grt::DictRef info);

public:
  ImplData(workbench_physical_Model *self);
  virtual ~ImplData();

  void update_relationship_figure(model_Connection::ImplData *cfig, bool imandatory, bool imany, bool fmandatory,
                                  bool fmany);

  void member_changed_comm(const std::string &name, const grt::ValueRef &value);
  mdc::LineEndType get_line_end_type(bool mand, bool many, bool start);
  std::string get_line_end_caption(bool mand, bool many, bool start);
  wbfig::Table *create_table_figure(mdc::Layer *layer, const model_DiagramRef &diagram, const model_ObjectRef &table);

  PhysicalRelationshipNotation get_relationship_notation() const {
    return _relationship_notation;
  }
  PhysicalFigureNotation get_figure_notation() const {
    return _figure_notation;
  }

  std::list<meta_TagRef> get_tags_for_dbobject(const db_DatabaseObjectRef &dbobject);

private:
  workbench_physical_Model *self() const {
    return (workbench_physical_Model *)_owner;
  }
};

#endif
