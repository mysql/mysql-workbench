//!
//! \addtogroup linuxui Linux UI
//! @{
//!

#ifndef _NAVIGATOR_BOX_H_
#define _NAVIGATOR_BOX_H_

#include "gtk/mdc_gtk_canvas_view.h"
#include <gtkmm/box.h>
#include <gtkmm/scale.h>
#include <gtkmm/comboboxtext.h>

namespace wb {
  class ModelDiagramForm;
};

class NavigatorBox : public Gtk::Box {
  wb::ModelDiagramForm *_model;
  mdc::GtkCanvas _canvas;
  Gtk::HScale _slider;
  Gtk::ComboBoxText _combo;
  Gtk::Button _zoom_in;
  Gtk::Button _zoom_out;
  bool _changing_zoom;

  void size_change(Gtk::Allocation &alloc);
  void canvas_realize();

  void slider_changed();
  void combo_changed(bool force_update);

public:
  NavigatorBox();

  void set_model(wb::ModelDiagramForm *model);

  void refresh();
};

#endif /* _NAVIGATOR_BOX_H_ */

//!
//! @}
//!
