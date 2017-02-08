#ifndef __WIDGET_SAVER_H__
#define __WIDGET_SAVER_H__

#include <sigc++/sigc++.h>

namespace bec {
  class GRTManager;
}

namespace Gtk {
  class Paned;
}

namespace mforms {
  class ToolBar;
}

namespace utils {
  namespace gtk {

    void save_settings(Gtk::Paned* paned, const bool right_side = false);
    sigc::connection load_settings(Gtk::Paned* paned, const sigc::slot<void> defaults_slot = sigc::slot<void>(),
                                   const bool right_side = false, const int min_size = 0);

  } // ns gtk
} // ns utils

#endif
