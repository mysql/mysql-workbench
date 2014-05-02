#ifndef __wb_model_reporting_h_
#define __wb_model_reporting_h_
// Automatically generated GRT module wrapper. Do not edit.

using namespace grt;

class WbModelReportingInterfaceWrapper : public grt::ModuleWrapper {
protected:
  friend class grt::GRT;
  WbModelReportingInterfaceWrapper(grt::Module *module)
  : grt::ModuleWrapper(module) {}

public:
  static const char *static_get_name() { return "WbModelReportingInterface"; }
  int getAvailableReportingTemplates(const grt::StringListRef& param0)
  {
    grt::BaseListRef args(get_grt(), AnyType);
    args.ginsert(param0);
    grt::ValueRef ret= _module->call_function("getAvailableReportingTemplates", args);
    return *grt::IntegerRef::cast_from(ret);
  }
  std::string getTemplateDirFromName(const std::string & param0)
  {
    grt::BaseListRef args(get_grt(), AnyType);
    args.ginsert(grt::StringRef(param0));
    grt::ValueRef ret= _module->call_function("getTemplateDirFromName", args);
    return (std::string)StringRef::cast_from(ret);
  }
  grt::Ref<workbench_model_reporting_TemplateInfo> getReportingTemplateInfo(const std::string & param0)
  {
    grt::BaseListRef args(get_grt(), AnyType);
    args.ginsert(grt::StringRef(param0));
    grt::ValueRef ret= _module->call_function("getReportingTemplateInfo", args);
    return grt::Ref<workbench_model_reporting_TemplateInfo>::cast_from(ret);
  }
  int generateReport(const grt::Ref<workbench_physical_Model>& param0, const grt::DictRef& param1)
  {
    grt::BaseListRef args(get_grt(), AnyType);
    args.ginsert(param0);
    args.ginsert(param1);
    grt::ValueRef ret= _module->call_function("generateReport", args);
    return *grt::IntegerRef::cast_from(ret);
  }
};
#endif
