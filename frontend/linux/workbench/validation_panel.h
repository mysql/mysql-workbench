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

//#include "wb_config.h"
#ifdef COMMERCIAL_CODE

#ifndef __VALIDATION_PANEL_H__
#define __VALIDATION_PANEL_H__

#include <gtkmm/box.h>
#include <gtkmm/treeview.h>
#include <gtkmm/label.h>
#include <gtkmm/image.h>
#include <glibmm/refptr.h>
#include "validation_manager.h"
#include "listmodel_wrapper.h"
#include "base/trackable.h"

namespace Gtk {
  class TreeView;
  class Box;
}

class ValidationPanel : public Gtk::Box, public base::trackable {
public:
  ValidationPanel();

  void refresh(const bec::NodeId& node, int ocount);
  Gtk::Box* notebook_label(Gtk::Box* lbl = 0);

private:
  void size_request_slot(Gtk::Allocation& req);

  bec::ValidationMessagesBE _be;
  Gtk::TreeView _tv;
  Glib::RefPtr<ListModelWrapper> _model;
  Gtk::Box* _label_box;
  Gtk::Image _icon;
  Gtk::Label _label;
};

#endif
#endif
