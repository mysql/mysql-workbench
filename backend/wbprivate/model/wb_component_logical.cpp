#include "stdafx.h"

#include "wb_component_logical.h"

#include "grts/structs.workbench.logical.h"


using namespace wb;
using namespace bec;
using namespace grt;


WBComponentLogical::WBComponentLogical(WBContext *wb)
: WBComponent(wb)
{
}


void WBComponentLogical::setup_logical_model(grt::GRT *grt, workbench_DocumentRef &doc)
{
  // init logical model
  workbench_logical_ModelRef lmodel(grt);
  lmodel->owner(doc);

  doc->logicalModel(lmodel);
}

