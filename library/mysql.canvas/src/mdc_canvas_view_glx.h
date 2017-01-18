#ifndef _MDC_CANVAS_VIEW_GLX_H_
#define _MDC_CANVAS_VIEW_GLX_H_

#include "mdc_canvas_view_opengl.h"
#include "mdc_canvas_view.h"

#include <X11/Xlib.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

namespace mdc {
  std::string detect_opengl_version();

  class GLXCanvasView : public OpenGLCanvasView {
  public:
    GLXCanvasView(Display *dpy, Window win, Visual *visual, int width, int height);
    virtual ~GLXCanvasView();

    virtual bool initialize();

    virtual void make_current();
    virtual void remove_current();
    virtual void swap_buffers();

    virtual void update_view_size(int, int);

  protected:
    GLXContext _glxcontext;
    Display *_display;
    Window _window;

    Visual *_visual;
  };
};

#endif /* _MDC_CANVAS_VIEW_GLX_H_ */
