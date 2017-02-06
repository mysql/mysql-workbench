#ifndef _MDC_CANVAS_VIEW_OPENGL_H_
#define _MDC_CANVAS_VIEW_OPENGL_H_

#include "mdc_canvas_public.h"
#include "mdc_canvas_view.h"

namespace mdc {

  MYSQLCANVAS_PUBLIC_FUNC std::string detect_opengl_version();

  class MYSQLCANVAS_PUBLIC_FUNC OpenGLCanvasView : public CanvasView {
  public:
    OpenGLCanvasView(int width, int height);
    virtual ~OpenGLCanvasView();

    virtual bool has_gl() const {
      return true;
    }

    virtual bool initialize();

    static void check_error();

    virtual void make_current() = 0;
    virtual void remove_current() = 0;
    virtual void swap_buffers() = 0;

    virtual void begin_repaint(int, int, int, int);
    virtual void end_repaint();
  };

} // end of mdc namespace

#endif /* _MDC_CANVAS_VIEW_OPENGL_H_ */
