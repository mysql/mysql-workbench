/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DIAGRAM_SIZE_FORM_H_
#define _DIAGRAM_SIZE_FORM_H_

#include <gtkmm/dialog.h>
#include <gtkmm/builder.h>
#include "base/trackable.h"

namespace wb {
  class WBContextUI;
  class DiagramOptionsBE;
};

namespace mdc {
  class GtkCanvas;
};

class DiagramSizeForm : public Gtk::Dialog, public base::trackable {
  Glib::RefPtr<Gtk::Builder> _xml;
  mdc::GtkCanvas *_canvas;
  wb::DiagramOptionsBE *_be;

  void realize_be();
  void init();
  void spin_changed();
  void changed();
  void ok_clicked();

public:
  DiagramSizeForm(GtkDialog *gobj, Glib::RefPtr<Gtk::Builder> xml);
  virtual ~DiagramSizeForm();

  static DiagramSizeForm *create();
};

#endif /* _DIAGRAM_SIZE_FORM_H_ */
