/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _DATA_SOURCE_SELECTOR_H_
#define _DATA_SOURCE_SELECTOR_H_

#include "grtui/grt_wizard_plugin.h"
#include "grtui/wizard_view_text_page.h"

#include "mforms/fs_object_selector.h"
#include "mforms/panel.h"
#include "mforms/box.h"
#include "mforms/radiobutton.h"

struct DataSourceSelector : public base::trackable {
  enum SourceType { ModelSource = 0, ServerSource = 1, FileSource = 2 };

  mforms::Panel panel;
  mforms::Box box;
  mforms::RadioButton* model_radio;
  mforms::RadioButton* server_radio;
  mforms::RadioButton* file_radio;
  mforms::Box browse_box;
  mforms::FsObjectSelector file_selector;

  DataSourceSelector(bool SaveFile = false)
    : panel(::mforms::TitledBoxPanel), box(false), browse_box(true), file_selector(true) {
    box.set_spacing(4);
    box.set_padding(12);
    box.set_homogeneous(true);
    panel.add(&box);

    int group = mforms::RadioButton::new_id();
    model_radio = mforms::manage(new mforms::RadioButton(group));
    server_radio = mforms::manage(new mforms::RadioButton(group));
    file_radio = mforms::manage(new mforms::RadioButton(group));

    box.add(model_radio, false, true);
    model_radio->set_text(_("Model Schemata"));
    box.add(server_radio, false, true);
    server_radio->set_text(_("Live Database Server"));
    file_radio->set_text(_("Script File:"));
    box.add(&browse_box, false, true);

    browse_box.set_spacing(8);
    browse_box.add(file_radio, false, true);
    browse_box.add(&file_selector, true, true);

    file_selector.initialize("", SaveFile ? mforms::SaveFile : mforms::OpenFile, "SQL Files (*.sql)|*.sql");
    scoped_connect(file_radio->signal_clicked(), std::bind(&DataSourceSelector::file_source_selected, this));
  }

  void file_source_selected() {
    file_selector.set_enabled(file_radio->get_active());
  }

  void set_change_slot(const std::function<void()>& change_slot) {
    scoped_connect(model_radio->signal_clicked(), change_slot);
    scoped_connect(server_radio->signal_clicked(), change_slot);
    scoped_connect(file_radio->signal_clicked(), change_slot);
  }

  void set_source(SourceType type) {
    switch (type) {
      case ModelSource:
        model_radio->set_active(true);
        (*model_radio->signal_clicked())();
        break;
      case ServerSource:
        server_radio->set_active(true);
        (*server_radio->signal_clicked())();
        break;
      case FileSource:
        file_radio->set_active(true);
        (*file_radio->signal_clicked())();
        break;
    }
  }

  SourceType get_source() {
    if (model_radio->get_active())
      return ModelSource;
    else if (server_radio->get_active())
      return ServerSource;
    else
      return FileSource;
  }
};

#endif //#define _DB_ALTER_SCRIPT_H_
