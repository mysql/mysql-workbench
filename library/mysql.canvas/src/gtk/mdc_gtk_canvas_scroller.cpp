
#include "mdc_gtk_canvas_scroller.h"
#include "mdc_gtk_canvas_view.h"

using namespace mdc;

GtkCanvasScroller::GtkCanvasScroller() : Gtk::Table(2, 2) {
  attach(_vscroll, 1, 2, 0, 1, Gtk::FILL, Gtk::FILL | Gtk::EXPAND);
  attach(_hscroll, 0, 1, 1, 2, Gtk::FILL | Gtk::EXPAND, Gtk::FILL);
  show_all();

  _hscroll.get_adjustment()->set_page_increment(50.0);
  _hscroll.get_adjustment()->set_step_increment(5.0);

  _vscroll.get_adjustment()->set_page_increment(50.0);
  _vscroll.get_adjustment()->set_step_increment(5.0);
}

void GtkCanvasScroller::add(GtkCanvas &canvas) {
  attach(canvas, 0, 1, 0, 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL | Gtk::EXPAND);
  canvas.show();
  canvas.set_vadjustment(_vscroll.get_adjustment());
  canvas.set_hadjustment(_hscroll.get_adjustment());
}

Glib::RefPtr<Gtk::Adjustment> GtkCanvasScroller::get_hadjustment() {
  return _hscroll.get_adjustment();
}

Glib::RefPtr<Gtk::Adjustment> GtkCanvasScroller::get_vadjustment() {
  return _vscroll.get_adjustment();
}
