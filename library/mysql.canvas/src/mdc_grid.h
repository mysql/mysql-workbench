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

#ifndef _MDC_GRID_H_
#define _MDC_GRID_H_

#if 0

#include "mdc_common.h"
#include "mdc_layouter.h"

namespace mdc {

typedef unsigned short GridIndex;
  
class Grid : public Layouter {
public:
  Grid(CanvasView *canvas, GridIndex rows, GridIndex cols);

  virtual void add(CanvasItem *item, GridIndex row, GridIndex col, GridIndex rspan=1, GridIndex cspan=1);
  
  virtual void relayout();

protected:
  struct GridCell {
    CanvasItem *item;
    GridIndex row;
    GridIndex col;
    GridIndex rspan;
    GridIndex cspan;
  };
  
  int _row_num;
  int _column_num;

  std::vector<GridRow> _rows;
};

};

#endif

#endif /* _MDC_GRID_H_ */
