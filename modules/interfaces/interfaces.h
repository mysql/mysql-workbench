
#include "grt.h"

#include "plugin.h"
#include "sqlgenerator.h"
#include "wbvalidation.h"
#include "wb_model_reporting.h"

inline void register_interfaces() {
  if (!grt::GRT::get()->get_interface("PluginInterface"))
    PluginInterfaceImpl::register_interface(); // this is already registered in PluginManager
  SQLGeneratorInterfaceImpl::register_interface();
  WbValidationInterfaceImpl::register_interface();
  WbModelReportingInterfaceImpl::register_interface();
}
