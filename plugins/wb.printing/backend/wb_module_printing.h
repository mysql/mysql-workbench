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

#ifndef _WB_MODULE_PRINTING_H_
#define _WB_MODULE_PRINTING_H_

#include "wb_module_printing.h"

#include "interfaces/plugin.h"

#define WBModule_VERSION "1.0.0"

class WbPrintingImpl : public grt::ModuleImplBase, PluginInterfaceImpl {
  typedef grt::ModuleImplBase super;

public:
  WbPrintingImpl(grt::CPPModuleLoader *ldr);

  DEFINE_INIT_MODULE(WBModule_VERSION, "Oracle and/or its affiliates", grt::ModuleImplBase,
                     DECLARE_MODULE_FUNCTION(WbPrintingImpl::getPluginInfo),

                     DECLARE_MODULE_FUNCTION(WbPrintingImpl::printDiagramsToFile),
                     DECLARE_MODULE_FUNCTION(WbPrintingImpl::printToPDFFile),
                     DECLARE_MODULE_FUNCTION(WbPrintingImpl::printToPSFile),
                     DECLARE_MODULE_FUNCTION(WbPrintingImpl::printToPrinter));

private:
  virtual grt::ListRef<app_Plugin> getPluginInfo() override;

  int printDiagramsToFile(grt::ListRef<model_Diagram> view, const std::string &path, const std::string &format,
                          grt::DictRef options);
  int printToPDFFile(model_DiagramRef view, const std::string &path);
  int printToPSFile(model_DiagramRef view, const std::string &path);

  int printToPrinter(model_DiagramRef view, const std::string &printer);
};

#endif
