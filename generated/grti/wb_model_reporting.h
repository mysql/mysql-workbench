#ifndef __wb_model_reporting_h__
#define __wb_model_reporting_h__
// Automatically generated GRT module wrapper. Do not edit.

using namespace grt;

class WbModelReportingInterfaceWrapper : public grt::ModuleWrapper {
protected:
  friend class grt::GRT;
  WbModelReportingInterfaceWrapper(grt::Module* module) : grt::ModuleWrapper(module) {
  }

public:
  static const char* static_get_name() {
    return "WbModelReportingInterface";
  }
  ssize_t getAvailableReportingTemplates(const grt::StringListRef& param0) {
    grt::BaseListRef args(AnyType);
    args.ginsert(param0);
    grt::ValueRef ret = _module->call_function("getAvailableReportingTemplates", args);
    return *grt::IntegerRef::cast_from(ret);
  }
  std::string getTemplateDirFromName(const std::string& param0) {
    grt::BaseListRef args(AnyType);
    args.ginsert(grt::StringRef(param0));
    grt::ValueRef ret = _module->call_function("getTemplateDirFromName", args);
    return (std::string)StringRef::cast_from(ret);
  }
  workbench_model_reporting_TemplateInfoRef getReportingTemplateInfo(const std::string& param0) {
    grt::BaseListRef args(AnyType);
    args.ginsert(grt::StringRef(param0));
    grt::ValueRef ret = _module->call_function("getReportingTemplateInfo", args);
    return workbench_model_reporting_TemplateInfoRef::cast_from(ret);
  }
  ssize_t generateReport(const workbench_physical_ModelRef& param0, const grt::DictRef& param1) {
    grt::BaseListRef args(AnyType);
    args.ginsert(param0);
    args.ginsert(param1);
    grt::ValueRef ret = _module->call_function("generateReport", args);
    return *grt::IntegerRef::cast_from(ret);
  }
};
#endif
