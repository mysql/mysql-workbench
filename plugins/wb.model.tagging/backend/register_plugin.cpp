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

#include "stdafx.h"

#include "grtpp.h"
#include "interfaces/plugin.h"

#include "grts/structs.workbench.model.h"
#include "grts/structs.meta.h"

#define MODULE_VERSION "1.0.0"

#ifdef _MSC_VER
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(windows_dll)
#elif defined(__APPLE__)
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(osx_dylib)
#else
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(linux_so)
#endif

static grt::ListRef<app_Plugin> get_plugins_info(grt::GRT *grt);

class WbTaggingModuleImpl : public grt::ModuleImplBase, public PluginInterfaceImpl {
public:
  WbTaggingModuleImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE(MODULE_VERSION, "Sun Microsystems Inc", grt::ModuleImplBase,
                     DECLARE_MODULE_FUNCTION(WbTaggingModuleImpl::getPluginInfo), NULL);

  virtual grt::ListRef<app_Plugin> getPluginInfo() {
    return get_plugins_info(get_grt());
  }
};

static void set_object_argument(app_PluginRef &plugin, const std::string &struct_name) {
  app_PluginObjectInputRef pdef(plugin.get_grt());

  pdef->objectStructName(struct_name);
  pdef->owner(plugin);

  plugin->inputValues().insert(pdef);
}

static grt::ListRef<app_Plugin> get_plugins_info(grt::GRT *grt) {
  grt::ListRef<app_Plugin> editors(grt);

  app_PluginRef tag_editor(grt);

  FRONTEND_LIBNAME(tag_editor, ".\\wb.model.tagging.wbp.fe.dll", "wb.model.tagging.wbp.so",
                   "wb.model.tagging.mwbplugin");
  tag_editor->pluginType("gui");
  tag_editor->moduleFunctionName("WbTagEditor");
  set_object_argument(tag_editor, "workbench.physical.Model");
  tag_editor->caption("Edit Object Tags");
  tag_editor->rating(10);
  tag_editor->name("wb.plugin.edit.tags");
  tag_editor->groups().insert("model/Editors");
  editors.insert(tag_editor);

  return editors;
}

GRT_MODULE_ENTRY_POINT(WbTaggingModuleImpl);
