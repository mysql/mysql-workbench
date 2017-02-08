
#ifndef _WB_MODEL_REPORTING_IF_H_
#define _WB_MODEL_REPORTING_IF_H_

#include "grtpp_module_cpp.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.workbench.model.reporting.h"

// schema report interface definition header

class WbModelReportingInterfaceImpl : public grt::InterfaceImplBase {
public:
  DECLARE_REGISTER_INTERFACE(WbModelReportingInterfaceImpl,
                             DECLARE_INTERFACE_FUNCTION(WbModelReportingInterfaceImpl::getAvailableReportingTemplates),
                             DECLARE_INTERFACE_FUNCTION(WbModelReportingInterfaceImpl::getTemplateDirFromName),
                             DECLARE_INTERFACE_FUNCTION(WbModelReportingInterfaceImpl::getReportingTemplateInfo),
                             DECLARE_INTERFACE_FUNCTION(WbModelReportingInterfaceImpl::generateReport));

  virtual ssize_t getAvailableReportingTemplates(grt::StringListRef templates) = 0;

  virtual std::string getTemplateDirFromName(const std::string& template_name) = 0;

  virtual grt::Ref<workbench_model_reporting_TemplateInfo> getReportingTemplateInfo(
    const std::string& template_name) = 0;

  virtual ssize_t generateReport(grt::Ref<workbench_physical_Model> model, const grt::DictRef& options) = 0;
};

#endif /* _WB_MODEL_REPORTING_IF_H_ */
