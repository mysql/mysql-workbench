
#include "stdafx.h"

#include "grtpp.h"
#include "interfaces/plugin.h"

#include "grts/structs.workbench.model.h"
#include "grts/structs.meta.h"

#define MODULE_VERSION "1.0.0"

#ifdef _WIN32
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
