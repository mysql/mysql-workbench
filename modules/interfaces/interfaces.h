
#include <grtpp.h>

#include "plugin.h"
#include "sqlgenerator.h"
#include "wbvalidation.h"
#include "wb_model_reporting.h"

inline void register_interfaces(grt::GRT *grt)
{
  if (!grt->get_interface("PluginInterface"))
    PluginInterfaceImpl::register_interface(grt); // this is already registered in PluginManager
  SQLGeneratorInterfaceImpl::register_interface(grt);
  WbValidationInterfaceImpl::register_interface(grt);
  WbModelReportingInterfaceImpl::register_interface(grt);
}
