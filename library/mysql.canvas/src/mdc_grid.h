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
