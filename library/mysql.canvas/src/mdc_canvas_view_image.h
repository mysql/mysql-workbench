#ifndef _MDC_CANVAS_VIEW_IMAGE_H_
#define _MDC_CANVAS_VIEW_IMAGE_H_

#include "mdc_canvas_view.h"

namespace mdc {

  class MYSQLCANVAS_PUBLIC_FUNC ImageCanvasView : public CanvasView {
  public:
    ImageCanvasView(int width, int height, cairo_format_t format = CAIRO_FORMAT_RGB24);
    virtual ~ImageCanvasView();

    virtual void begin_repaint(int x, int y, int w, int h);
    virtual void end_repaint();
    virtual void update_view_size(int width, int height);

    void save_to(const std::string &path);

    const unsigned char *get_image_data(size_t &size);

    virtual bool has_gl() const {
      return false;
    }

  protected:
    cairo_surface_t *_buffer;
    cairo_format_t _format;
  };
};

#endif /* _MDC_CANVAS_VIEW_IMAGE_H_ */
