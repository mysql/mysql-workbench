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

    Glib::RefPtr<Gtk::Adjustment> get_hadjustment();
    Glib::RefPtr<Gtk::Adjustment> get_vadjustment();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual" // The GtkCanvas is descendant of Gtk::Layout
    void add(GtkCanvas &canvas);                      //
#pragma GCC diagnostic pop
  };
};

#endif /* _MDC_GTK_CANVAS_SCROLLER_H_ */
