#ifndef _WB_OVERVIEW_DIAGRAM_H_
#define _WB_OVERVIEW_DIAGRAM_H_

#include "workbench/wb_overview.h"
/**
 * @file  wb_overview_diagram.h
 * @brief 
 */


namespace wb
{
  class DiagramListNode : public OverviewBE::ContainerNode
  {
    std::string id;
    model_ModelRef _model;

  public:
    DiagramListNode(model_ModelRef model);

    virtual void refresh_children();

    virtual std::string get_unique_id() { return id; }
  };
};

#endif /* _WB_OVERVIEW_DIAGRAM_H_ */
