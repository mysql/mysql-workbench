#ifndef __wbvalidation_h__
#define __wbvalidation_h__
// Automatically generated GRT module wrapper. Do not edit.

using namespace grt;

class WbValidationInterfaceWrapper : public grt::ModuleWrapper {
protected:
  friend class grt::GRT;
  WbValidationInterfaceWrapper(grt::Module* module) : grt::ModuleWrapper(module) {
  }

public:
  static const char* static_get_name() {
    return "WbValidationInterface";
  }
  ssize_t validate(const std::string& param0, const ObjectRef& param1) {
    grt::BaseListRef args(AnyType);
    args.ginsert(grt::StringRef(param0));
    args.ginsert(param1);
    grt::ValueRef ret = _module->call_function("validate", args);
    return *grt::IntegerRef::cast_from(ret);
  }
  std::string getValidationDescription(const ObjectRef& param0) {
    grt::BaseListRef args(AnyType);
    args.ginsert(param0);
    grt::ValueRef ret = _module->call_function("getValidationDescription", args);
    return (std::string)StringRef::cast_from(ret);
  }
};
#endif
