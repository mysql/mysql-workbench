
#pragma once

#include "grtpp_module_cpp.h"
#include "grts/structs.app.h"

// plugin interface definition header
//
// Plugins must implement this interface to be recognized by the
// plugin manager.

class PluginInterfaceImpl : public grt::InterfaceImplBase {
public:
  DECLARE_REGISTER_INTERFACE(PluginInterfaceImpl, DECLARE_INTERFACE_FUNCTION(PluginInterfaceImpl::getPluginInfo));

  virtual grt::ListRef<app_Plugin> getPluginInfo() = 0;
};
