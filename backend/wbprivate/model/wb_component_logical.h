
#ifndef _WB_COMPONENT_LOGICAL_H_
#define _WB_COMPONENT_LOGICAL_H_

#include "wb_component.h"
#include "grts/structs.workbench.h"
#include "grts/structs.workbench.logical.h"

namespace wb {

  class MYSQLWBBACKEND_PUBLIC_FUNC WBComponentLogical : virtual public WBComponent {
  public:
    WBComponentLogical(WBContext *wb);
    virtual ~WBComponentLogical(){};

    static std::string name() {
      return "logical";
    }
    virtual std::string get_name() {
      return WBComponentLogical::name();
    }
    virtual std::string get_diagram_class_name() {
      return workbench_logical_Diagram::static_class_name();
    }

    void setup_logical_model(workbench_DocumentRef &doc);

    virtual void activate_canvas_object(const model_ObjectRef &object, bool newwindow) {
    }
    virtual bool handles_figure(const model_ObjectRef &) {
      return false;
    }

    virtual void setup_canvas_tool(wb::ModelDiagramForm *, const std::string &) {
    }
    virtual bool delete_model_object(const model_ObjectRef &, bool figure_only) {
      return false;
    }
  };
};

#endif /* _WB_COMPONENT_LOGICAL_H_ */
