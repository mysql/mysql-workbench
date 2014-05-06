#ifndef _INFO_BOX_H_
#define _INFO_BOX_H_

#include <gtkmm/table.h>
#include <gtkmm/label.h>
#include <gtkmm/button.h>

namespace wb
{
  class WBContextUI;
};


class InfoBox : public Gtk::Table
{
  wb::WBContextUI *_wbui;
  
  Gtk::Label _size_label;
  Gtk::Label _size_value;
  Gtk::Button _size_open;

  void realized();

  void open_options();

public:
  InfoBox(wb::WBContextUI *wbui);

  Glib::SignalProxy0<void> signal_open_diagram_options() { return _size_open.signal_clicked(); }
  
  void refresh();
};


#endif /* _INFO_BOX_H_ */
