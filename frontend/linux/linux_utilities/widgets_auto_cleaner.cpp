#include "widgets_auto_cleaner.h"
#include <gtkmm/widget.h>

//------------------------------------------------------------------------------
WidgetsAutoCleaner::~WidgetsAutoCleaner() {
  delete_widgets();
}

//------------------------------------------------------------------------------
void WidgetsAutoCleaner::add(Gtk::Widget* w) {
  if (_widgets.end() != std::find(_widgets.begin(), _widgets.end(), w))
    _widgets.push_back(w);
}

//------------------------------------------------------------------------------
void WidgetsAutoCleaner::delete_widgets() {
  const int n = _widgets.size();
  for (int i = n - 1; i >= 0; --i) {
    delete _widgets[i];
    _widgets[i] = 0;
  }
  _widgets.clear();
}
