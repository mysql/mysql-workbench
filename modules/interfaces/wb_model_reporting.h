/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_MODEL_REPORTING_IF_H_
#define _WB_MODEL_REPORTING_IF_H_

#include "grtpp_module_cpp.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.workbench.model.reporting.h"

// schema report interface definition header

class WbModelReportingInterfaceImpl : public grt::InterfaceImplBase {
public:
  DECLARE_REGISTER_INTERFACE(WbModelReportingInterfaceImpl,
                             DECLARE_INTERFACE_FUNCTION(WbModelReportingInterfaceImpl::getAvailableReportingTemplates),
                             DECLARE_INTERFACE_FUNCTION(WbModelReportingInterfaceImpl::getTemplateDirFromName),
                             DECLARE_INTERFACE_FUNCTION(WbModelReportingInterfaceImpl::getReportingTemplateInfo),
                             DECLARE_INTERFACE_FUNCTION(WbModelReportingInterfaceImpl::generateReport));

  virtual ssize_t getAvailableReportingTemplates(grt::StringListRef templates) = 0;

  virtual std::string getTemplateDirFromName(const std::string& template_name) = 0;

  virtual grt::Ref<workbench_model_reporting_TemplateInfo> getReportingTemplateInfo(
    const std::string& template_name) = 0;

  virtual ssize_t generateReport(grt::Ref<workbench_physical_Model> model, const grt::DictRef& options) = 0;
};

#endif /* _WB_MODEL_REPORTING_IF_H_ */
