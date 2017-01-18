#ifndef __plugin_h__
#define __plugin_h__
// Automatically generated GRT module wrapper. Do not edit.

using namespace grt;

class PluginInterfaceWrapper : public grt::ModuleWrapper {
protected:
  friend class grt::GRT;
  PluginInterfaceWrapper(grt::Module *module) : grt::ModuleWrapper(module) {
  }

public:
  static const char *static_get_name() {
    return "PluginInterface";
  }
  grt::ListRef<app_Plugin> getPluginInfo() {
    grt::BaseListRef args(AnyType);

    grt::ValueRef ret = _module->call_function("getPluginInfo", args);
    return grt::ListRef<app_Plugin>::cast_from(ret);
  }
};
#endif
