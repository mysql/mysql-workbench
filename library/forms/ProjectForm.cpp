/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/ProjectForm.h"
#include "base/string_utilities.h"
#include <cctype>

namespace Workbench {
  namespace X {

    ProjectForm::ProjectForm() : mforms::Form(mforms::Form::main_form()), _box(false), _bbox(true) {
      set_title("Project Manager");
      set_name("project_manager");
      set_content(&_box);

      _box.set_padding(20);
      _box.set_spacing(20);
      _cancel.set_text("Cancel");
      _save.set_text("Save");

      mforms::Utilities::add_end_ok_cancel_buttons(&_bbox, &_save, &_cancel);
      _box.add_end(&_bbox, false, true);
      setupUi();

      _save.signal_clicked()->connect([=]() { onSave(); });
    }

    ProjectForm::~ProjectForm() {
    }

    static mforms::Label *new_label(const std::string &text, bool right_align = false, bool help = false) {
      mforms::Label *label = mforms::manage(new mforms::Label());
      label->set_text(text);
      if (right_align)
        label->set_text_align(mforms::MiddleRight);
      if (help) {
        label->set_style(mforms::SmallHelpTextStyle);
        label->set_wrap_text(true);
        label->set_size(50, -1);
      }

      return label;
    }

    void ProjectForm::setupUi() {
      mforms::Table *table = mforms::manage(new mforms::Table());

      table->set_padding(8);
      table->set_row_spacing(12);
      table->set_column_spacing(8);

      table->set_row_count(3);
      table->set_column_count(4);

      table->add(new_label(_("Project name:"), false), 0, 1, 0, 1, mforms::HFillFlag);
      table->add(&_projectName, 1, 4, 0, 1, mforms::HFillFlag | mforms::HExpandFlag);
      _projectName.set_placeholder_text("Name of the project");

      table->add(new_label(_("Host:"), false), 0, 1, 1, 2, mforms::HFillFlag);
      table->add(&_host, 1, 2, 1, 2, mforms::HFillFlag | mforms::HExpandFlag);
      _host.set_placeholder_text("127.0.0.1");

      table->add(new_label(_("Port:"), false), 2, 3, 1, 2, mforms::HFillFlag);
      table->add(&_port, 3, 4, 1, 2, mforms::NoFillExpandFlag);
      _port.set_placeholder_text("33060");
      _port.set_size(60, -1);

      table->add(new_label(_("Username:"), false), 0, 1, 2, 3, mforms::HFillFlag);
      table->add(&_userName, 1, 4, 2, 3, mforms::HFillFlag | mforms::HExpandFlag);

      _box.add(table, true, true);

      set_size(500, 200);
    }

    void ProjectForm::show() {
      run_modal(nullptr, &_cancel);
    }

    void ProjectForm::onSave() {
      dataTypes::XProject tmpProject;
      tmpProject.name = _projectName.get_string_value();
      tmpProject.connection.userName = _userName.get_string_value();
      tmpProject.connection.hostName = _host.get_string_value();

      std::string errMessage = "Please correct the following errors before continue:\n";
      std::locale loc("C");
      bool wasError = false;
      if (tmpProject.name.empty()) {
        errMessage += " Project name cannot be empty\n";
        wasError = true;
      }

      if (std::find_if(tmpProject.name.begin(), tmpProject.name.end(), [=](char c) { return !std::isalnum(c, loc); }) !=
          tmpProject.name.end()) {
        errMessage += " Project name can contain only alphanumeric characters without space\n";
        wasError = true;
      }

      try {
        if (!_port.get_string_value().empty())
          tmpProject.connection.port = base::atoi<ssize_t>(_port.get_string_value());
      } catch (std::bad_cast &) {
        errMessage += " Port number must be a number\n";
        wasError = true;
      }

      if (tmpProject.connection.hostName.empty()) {
        errMessage += " HostName cannot be empty";
        wasError = true;
      }

      if (!tmpProject.isValid() || wasError) {
        mforms::Utilities::show_error(_("Validation error"), errMessage, _("Ok"));
        return;
      }

      if (tmpProject.isValid()) {
        _project = tmpProject;
        this->close();
      }
    }

  } /* namespace X */
} /* namespace Workbench */
