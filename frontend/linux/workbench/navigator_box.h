/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

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
