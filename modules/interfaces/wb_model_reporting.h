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
