
#ifndef _WBVALIDATION_IF_H_
#define _WBVALIDATION_IF_H_

#include "grtpp_module_cpp.h"
#include "grts/structs.db.h"

// database object validation interface definition header

class WbValidationInterfaceImpl : public grt::InterfaceImplBase //, public grt::Validator
{
public:
  DECLARE_REGISTER_INTERFACE(WbValidationInterfaceImpl, DECLARE_INTERFACE_FUNCTION(grt::Validator::validate),
                             DECLARE_INTERFACE_FUNCTION(WbValidationInterfaceImpl::getValidationDescription));

  //// Call all validations
  virtual std::string getValidationDescription(const grt::ObjectRef& root) = 0;
};

#endif /* _WBVALIDATION_IF_H_ */
