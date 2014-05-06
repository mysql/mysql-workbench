#ifndef _MDC_GTK_CANVAS_SCROLLER_H_
#define _MDC_GTK_CANVAS_SCROLLER_H_

#include <gtkmm/table.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/adjustment.h>

namespace mdc {
  
class GtkCanvas;

  class GtkCanvasScroller : public Gtk::Table {
    Gtk::HScrollbar _hscroll;
    Gtk::VScrollbar _vscroll;

  public:
    GtkCanvasScroller();

    Gtk::Adjustment *get_hadjustment();
    Gtk::Adjustment *get_vadjustment();

    void add(GtkCanvas &canvas);
};

};

#endif /* _MDC_GTK_CANVAS_SCROLLER_H_ */
