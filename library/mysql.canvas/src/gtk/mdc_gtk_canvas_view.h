#ifndef _MDC_GTK_CANVAS_VIEW_H_
#define _MDC_GTK_CANVAS_VIEW_H_

#include <gtkmm/adjustment.h>
#include <gtkmm/layout.h>

#include "mdc_canvas_view_x11.h"
#include "mdc_canvas_view_glx.h"
#include "base/trackable.h"

namespace mdc {

  class GtkCanvas : public Gtk::Layout, public base::trackable {
    typedef Gtk::Layout super;

  public:
    enum CanvasType { OpenGLCanvasType, XlibCanvasType, BufferedXlibCanvasType };

  private:
    CanvasView *_canvas;
    CanvasType _canvas_type;
    bool _reentrance;
    bool _initialized;

  public:
    GtkCanvas(CanvasType type);
    virtual ~GtkCanvas();

    CanvasView *get_canvas();

    mdc::EventState get_event_state(int event_state);

    void create_canvas();
    void set_vadjustment(const Glib::RefPtr<Gtk::Adjustment> &vadjustment);
    void set_hadjustment(const Glib::RefPtr<Gtk::Adjustment> &hadjustment);

  protected:
    bool redraw(::Cairo::RefPtr< ::Cairo::Context> context);
    virtual void on_realize();
    virtual void on_unrealize();
    virtual void on_map();
    virtual void on_size_allocate(Gtk::Allocation &alloc);

    virtual bool on_scroll_event(GdkEventScroll *event);

    virtual void on_zoom_in_event();
    virtual void on_zoom_out_event();
    virtual bool on_button_press_event(GdkEventButton *event);
    virtual bool on_button_release_event(GdkEventButton *event);
    virtual bool on_motion_notify_event(GdkEventMotion *event);
    virtual bool on_event(GdkEvent *event);
    virtual bool on_key_press_event(GdkEventKey *event);
    virtual bool on_key_release_event(GdkEventKey *event);

    void update_scrollers();
    void scroll_canvas();

    void canvas_view_needs_repaint(int, int, int, int);
    void canvas_view_viewport_changed();
  };
};
#endif /* _MDC_GTK_CANVAS_VIEW_H_ */
