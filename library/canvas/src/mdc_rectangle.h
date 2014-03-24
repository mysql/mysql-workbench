#ifndef _MDC_RECTANGLE_H_
#define _MDC_RECTANGLE_H_

#include "mdc_figure.h"
#include "mdc_draw_util.h"

BEGIN_MDC_DECLS

class MYSQLCANVAS_PUBLIC_FUNC RectangleFigure : public Figure {
public:  
  RectangleFigure(Layer *layer);
  
  virtual void draw_contents(CairoCtx *cr);
  virtual void stroke_outline(CairoCtx *cr, float offset= 0);
  virtual void stroke_outline_gl(float offset= 0);

  virtual void draw_contents_gl();
  
  virtual bool can_render_gl() { return true; }

  void set_rounded_corners(float radius, CornerMask corners);
  void set_filled(bool flag);
    
protected:
  float _corner_radius;
  
  CornerMask _corners;
  bool _filled;
};


END_MDC_DECLS


#endif /* _MDC_RECTANGLE_H_ */
