/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "wb_model_public_interface.h"

#include "grtpp_module_cpp.h"
#include "interfaces/plugin.h"
#include "interfaces/wb_model_reporting.h"

#include "grts/structs.model.h"
#include "grts/structs.db.h"
#include "wb_model_public_interface.h"

#define WbModel_VERSION "1.0.0"

class WB_MODEL_WBM_PUBLIC_FUNC WbModelImpl : public grt::ModuleImplBase,
                                             public WbModelReportingInterfaceImpl,
                                             public PluginInterfaceImpl {
public:
  WbModelImpl(grt::CPPModuleLoader *ldr);

  DEFINE_INIT_MODULE(WbModel_VERSION, "Oracle and/or its affiliates", grt::ModuleImplBase,
                     DECLARE_MODULE_FUNCTION(WbModelImpl::getPluginInfo),
                     DECLARE_MODULE_FUNCTION(WbModelImpl::autolayout),
                     DECLARE_MODULE_FUNCTION(WbModelImpl::createDiagramWithCatalog),
                     DECLARE_MODULE_FUNCTION(WbModelImpl::createDiagramWithObjects),
                     DECLARE_MODULE_FUNCTION(WbModelImpl::fitObjectsToContents),
                     DECLARE_MODULE_FUNCTION(WbModelImpl::center),
                     DECLARE_MODULE_FUNCTION(WbModelImpl::getAvailableReportingTemplates),
                     DECLARE_MODULE_FUNCTION(WbModelImpl::getTemplateDirFromName),
                     DECLARE_MODULE_FUNCTION(WbModelImpl::getReportingTemplateInfo),
                     DECLARE_MODULE_FUNCTION(WbModelImpl::generateReport),
                     DECLARE_MODULE_FUNCTION(WbModelImpl::expandAllObjects),
                     DECLARE_MODULE_FUNCTION(WbModelImpl::collapseAllObjects));

  virtual grt::ListRef<app_Plugin> getPluginInfo();

  int center(model_DiagramRef view);
  int autolayout(model_DiagramRef view);

  int createDiagramWithCatalog(workbench_physical_ModelRef model, db_CatalogRef catalog);
  int createDiagramWithObjects(workbench_physical_ModelRef model, grt::ListRef<GrtObject> objects);

  int fitObjectsToContents(const grt::ListRef<model_Object> &figures);

  int expandAllObjects(model_DiagramRef view);
  int collapseAllObjects(model_DiagramRef view);

  // Model Reporting
  virtual ssize_t getAvailableReportingTemplates(grt::StringListRef templates);

  virtual std::string getTemplateDirFromName(const std::string &template_name);

  virtual workbench_model_reporting_TemplateInfoRef getReportingTemplateInfo(const std::string &template_name);

  virtual ssize_t generateReport(workbench_physical_ModelRef model, const grt::DictRef &options);

private:
  void initializeReporting();
  void begin_undo_group();
  void end_undo_group(const std::string &action_desc);
  workbench_physical_DiagramRef add_model_view(const db_CatalogRef &catalog, int xpages, int ypages);

  grt::ListRef<GrtObject> _selected_objects;
  bool _use_objects_from_catalog;

  int do_autolayout(const model_LayerRef &layer, grt::ListRef<model_Object> &selection);
  int do_autoplace_any_list(const model_DiagramRef &view, grt::ListRef<GrtObject> &obj_list);
  int autoplace_relations(const model_DiagramRef &view, const grt::ListRef<db_Table> &tables);
  void handle_fklist_change(const model_DiagramRef &view, const db_TableRef &table, const db_ForeignKeyRef &fk,
                            bool added);

  workbench_model_reporting_TemplateStyleInfoRef get_template_style_from_name(std::string template_name,
                                                                              std::string template_style_name);

  grt::UndoManager *_undo_man;
};