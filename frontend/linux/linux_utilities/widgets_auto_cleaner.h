#ifndef __WIDGETS_AUTO_CLEANER_H__
#define __WIDGETS_AUTO_CLEANER_H__

#include <vector>

namespace Gtk {
  class Widget;
}

class WidgetsAutoCleaner {
public:
  virtual ~WidgetsAutoCleaner();
  template <typename T>
  T* manage(T* w) {
    add(w);
    return w;
  }
  void delete_widgets();

private:
  void add(Gtk::Widget* w);
  std::vector<Gtk::Widget*> _widgets;
};

#endif
