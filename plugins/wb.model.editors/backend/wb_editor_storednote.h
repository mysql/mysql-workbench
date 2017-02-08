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

#pragma once

#include "wb_editor_backend_public_interface.h"
#include "grt/editor_base.h"
#include "sqlide/sql_editor_be.h"
#include "grts/structs.workbench.model.h"
#include <memory>

namespace mforms {
  class ToolBarItem;
};

class WBEDITOR_BACKEND_PUBLIC_FUNC StoredNoteEditorBE : public bec::BaseEditor {
  GrtStoredNoteRef _note;

public:
  StoredNoteEditorBE(const GrtStoredNoteRef &note);

  bool is_script();

  virtual MySQLEditor::Ref get_sql_editor();

  void set_name(const std::string &name);
  std::string get_name();

  virtual std::string get_title();

  void load_text();
  virtual void commit_changes();
  virtual bool has_editor() {
    return true;
  }

protected:
  MySQLEditor::Ref _sql_editor;

  void set_text(grt::StringRef ext);
  grt::StringRef get_text(bool &isutf8);

  void changed_selector(mforms::ToolBarItem *item);
};
