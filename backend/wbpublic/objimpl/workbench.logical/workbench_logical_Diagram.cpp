/*
* Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#include <grts/structs.workbench.logical.h>

#include <grtpp_util.h>

//================================================================================
// workbench_logical_Diagram

class workbench_logical_Diagram::ImplData {};

void workbench_logical_Diagram::init() {
  // if (!_data) _data= new workbench_logical_Diagram::ImplData();
}

void workbench_logical_Diagram::set_data(ImplData *data) {
}

workbench_logical_Diagram::~workbench_logical_Diagram() {
  delete _data;
}

model_LayerRef workbench_logical_Diagram::placeNewLayer(double x, double y, double width, double height,
                                                        const std::string &name) {
  // add code here
  return model_LayerRef();
}
