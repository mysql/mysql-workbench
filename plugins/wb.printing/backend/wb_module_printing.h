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
  virtual grt::ListRef<app_Plugin> getPluginInfo();

  int printDiagramsToFile(grt::ListRef<model_Diagram> view, const std::string &path, const std::string &format,
                          grt::DictRef options);
  int printToPDFFile(model_DiagramRef view, const std::string &path);
  int printToPSFile(model_DiagramRef view, const std::string &path);

  int printToPrinter(model_DiagramRef view, const std::string &printer);
};

#endif
