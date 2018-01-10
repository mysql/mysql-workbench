/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */


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
