#include "info_box.h"
#include "workbench/wb_context_ui.h"
#include "base/string_utilities.h"


InfoBox::InfoBox(wb::WBContextUI *wbui)
  : _wbui(wbui)
{
  set_border_width(8);

  _size_label.set_text(_("Diagram Size:"));
  _size_value.set_text("");
  
  attach(_size_label, 0, 1, 0, 1, Gtk::FILL, Gtk::FILL);
  attach(_size_value, 1, 2, 0, 1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL);

  _size_open.set_label("Change Size...");
  attach(_size_open, 1, 2, 1, 2, Gtk::FILL, Gtk::FILL);

  show_all();
}


void InfoBox::refresh()
{
  _size_open.set_sensitive(!_wbui->get_active_diagram_info().empty());
  _size_value.set_text(_wbui->get_active_diagram_info());
}
