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

#include "grt.h"
#include "interfaces/plugin.h"

#include "grts/structs.workbench.model.h"

#define MODULE_VERSION "1.0.0"

#ifdef _MSC_VER
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(windows_dll)
#elif defined(__APPLE__)
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(osx_dylib)
#else
#define FRONTEND_LIBNAME(obj, windows_dll, linux_so, osx_dylib) obj->moduleName(linux_so)
#endif

static grt::ListRef<app_Plugin> get_mysql_plugins_info();

class WbEditorsModuleImpl : public grt::ModuleImplBase, public PluginInterfaceImpl {
public:
  WbEditorsModuleImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE(MODULE_VERSION, "Oracle and/or its affiliates", grt::ModuleImplBase,
                     DECLARE_MODULE_FUNCTION(WbEditorsModuleImpl::getPluginInfo), NULL);

  virtual grt::ListRef<app_Plugin> getPluginInfo() override {
    return get_mysql_plugins_info();
  }
};

static void set_object_argument(app_PluginRef &plugin, const std::string &struct_name) {
  app_PluginObjectInputRef pdef(grt::Initialized);

  pdef->objectStructName(struct_name);
  pdef->owner(plugin);

  plugin->inputValues().insert(pdef);
}

static grt::ListRef<app_Plugin> get_mysql_plugins_info() {
  grt::ListRef<app_Plugin> editors(true);

  app_PluginRef note_editor(grt::Initialized);
  app_PluginRef image_editor(grt::Initialized);
  app_PluginRef stored_note_editor(grt::Initialized);
  app_PluginRef stored_sql_editor(grt::Initialized);
  app_PluginRef layer_editor(grt::Initialized);

  FRONTEND_LIBNAME(note_editor, ".\\wb.model.editors.wbp.fe.dll", "wb.model.editors.wbp.so",
                   "wb.model.editors.mwbplugin");
  note_editor->pluginType("gui");
  note_editor->moduleFunctionName("NoteEditor");
  set_object_argument(note_editor, "workbench.model.NoteFigure");
  note_editor->caption("Edit Note");
  note_editor->rating(10);
  note_editor->name("wb.plugin.edit.note");
  note_editor->groups().insert("model/Editors");
  editors.insert(note_editor);

  FRONTEND_LIBNAME(image_editor, ".\\wb.model.editors.wbp.fe.dll", "wb.model.editors.wbp.so",
                   "wb.model.editors.mwbplugin");
  image_editor->pluginType("gui");
  image_editor->moduleFunctionName("ImageEditor");
  set_object_argument(image_editor, "workbench.model.ImageFigure");
  image_editor->caption("Edit Image");
  image_editor->rating(10);
  image_editor->name("wb.plugin.edit.image");
  image_editor->groups().insert("model/Editors");
  editors.insert(image_editor);

  FRONTEND_LIBNAME(layer_editor, ".\\wb.model.editors.wbp.fe.dll", "wb.model.editors.wbp.so",
                   "wb.model.editors.mwbplugin");
  layer_editor->pluginType("gui");
  layer_editor->moduleFunctionName("PhysicalLayerEditor");
  set_object_argument(layer_editor, "workbench.physical.Layer");
  layer_editor->caption("Edit Layer");
  layer_editor->rating(10);
  layer_editor->name("wb.plugin.edit.physical.layer");
  layer_editor->groups().insert("model/Editors");
  editors.insert(layer_editor);

  FRONTEND_LIBNAME(stored_note_editor, ".\\wb.model.editors.wbp.fe.dll", "wb.model.editors.wbp.so",
                   "wb.model.editors.mwbplugin");
  stored_note_editor->pluginType("gui");
  stored_note_editor->moduleFunctionName("StoredNoteEditor");
  set_object_argument(stored_note_editor, "GrtStoredNote");
  stored_note_editor->caption("Edit Note");
  stored_note_editor->rating(10);
  stored_note_editor->name("wb.plugin.edit.stored_note");
  stored_note_editor->groups().insert("model/Editors");
  editors.insert(stored_note_editor);

  FRONTEND_LIBNAME(stored_sql_editor, ".\\wb.model.editors.wbp.fe.dll", "wb.model.editors.wbp.so",
                   "wb.model.editors.mwbplugin");
  stored_sql_editor->pluginType("gui");
  stored_sql_editor->moduleFunctionName("StoredNoteEditor");
  set_object_argument(stored_sql_editor, "db.Script");
  stored_sql_editor->caption("Edit SQL Script");
  stored_sql_editor->rating(10);
  stored_sql_editor->name("wb.plugin.edit.sqlscript");
  stored_sql_editor->groups().insert("model/Editors");
  editors.insert(stored_sql_editor);

  return editors;
}

GRT_MODULE_ENTRY_POINT(WbEditorsModuleImpl);
