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

#pragma once
#include "mforms/mforms.h"
#include "base/data_types.h"

namespace Workbench {
  namespace X {

    class MFORMS_EXPORT ProjectForm : public mforms::Form {
      void setupUi();
      mforms::Box _box;
      mforms::Box _bbox;
      mforms::Button _cancel;
      mforms::Button _save;
      mforms::TextEntry _projectName;
      mforms::TextEntry _userName;
      mforms::TextEntry _host;
      mforms::TextEntry _port;
      void onSave();

    public:
      ProjectForm();
      virtual ~ProjectForm();
      void show();
      dataTypes::XProject _project;
    };

  } /* namespace X */
} /* namespace Workbench */
