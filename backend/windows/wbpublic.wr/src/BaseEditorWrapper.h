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

#pragma once

#include "grt/editor_base.h"

#include "DelegateWrapper.h"

using namespace MySQL::Base;

namespace MySQL {
  namespace Grt {

    ref class GrtManager;

  public
    ref class BaseEditorWrapper : public UIForm {
    protected:
      BaseEditorWrapper(bec::BaseEditor *inn);

      DelegateSlot0<void, void> ^ refresh_ui_handler;
      DelegateSlot1<void, void, int, int> ^ refresh_partial_ui_handler;

    public:
      ~BaseEditorWrapper();

      enum class PartialRefreshType {
        RefreshTextChanged = bec::BaseEditor::RefreshTextChanged,
      };

      void disable_auto_refresh();
      void enable_auto_refresh();

      bec::BaseEditor *get_unmanaged_object();
      GrtValue ^ get_object();
      String ^ get_title();
      bool is_editing_live_object();
      void apply_changes_to_live_object();
      void revert_changes_to_live_object();
      void set_refresh_ui_handler(DelegateSlot0<void, void>::ManagedDelegate ^ slot);
      void set_refresh_partial_ui_handler(DelegateSlot1<void, void, int, int>::ManagedDelegate ^ slot);

      GRT ^ get_grt();

      void show_exception(String ^ title, String ^ detail);
      void show_validation_error(String ^ title, String ^ reason);
      bool should_close_on_delete_of(String ^ oid);

      bool is_editor_dirty();
      void reset_editor_undo_stack();
    };

  } // namespace Grt
} // namespace MySQL
