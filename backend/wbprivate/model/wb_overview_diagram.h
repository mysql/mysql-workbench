/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_OVERVIEW_DIAGRAM_H_
#define _WB_OVERVIEW_DIAGRAM_H_

#include "workbench/wb_overview.h"
/**
 * @file  wb_overview_diagram.h
 * @brief
 */

namespace wb {
  class DiagramListNode : public OverviewBE::ContainerNode {
    std::string id;
    model_ModelRef _model;

  public:
    DiagramListNode(model_ModelRef model);

    virtual void refresh_children();

    virtual std::string get_unique_id() {
      return id;
    }
  };
};

#endif /* _WB_OVERVIEW_DIAGRAM_H_ */
