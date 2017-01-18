
#include "stdafx.h"

#define Sample_VERSION "1.0.0"

class SampleImpl : public grt::ModuleImplBase {
public:
  SampleImpl(grt::GRT *grt) : grt::ModuleImplBase(grt) {
  }

  DEFINE_INIT_MODULE(Sample_VERSION, "Your Company Name", ModuleImplBase, DECLARE_MODULE_FUNCTION(SampleImpl::add),
                     DECLARE_MODULE_FUNCTION(SampleImpl::sub));

  int add(int op1, int op2) {
    return op1 + op2;
  }

  int sub(int op1, int op2) {
    return op1 - op2;
  }
};

GRT_MODULE_ENTRY_POINT(SampleImpl);
