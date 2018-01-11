/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/ui_form.h"
#include "grt.h"
#include "grt/grt_manager.h"
#include "grtpp_undo_manager.h"
#include "sqlide/sql_editor_be.h"
#include "refresh_ui.h"

#include "wbpublic_public_interface.h"

class GrtObject;

namespace bec {
  struct ChangeSet;

  class WBPUBLICBACKEND_PUBLIC_FUNC UndoObjectChangeGroup : public grt::UndoGroup {
    std::string _object_id;
    std::string _member;

  public:
    UndoObjectChangeGroup(const std::string &object_id, const std::string &member);

    virtual bool matches_group(UndoGroup *group) const;
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC BaseEditor : public UIForm, public RefreshUI {
  public:
    // Partial refresh.
    enum {
      RefreshTextChanged, // Refresh the title of the active editor, button states etc.
    };

    BaseEditor(const grt::Ref<GrtObject> &object);
    virtual ~BaseEditor();

    virtual std::string get_form_context_name() const;

    virtual MySQLEditor::Ref get_sql_editor() {
      return MySQLEditor::Ref();
    }

    virtual bool should_close_on_delete_of(const std::string &oid) {
      return get_object().id() == oid;
    }

    GrtObjectRef get_object() {
      return _object;
    };
    void set_object(GrtObjectRef value);

    virtual bool has_editor() {
      return false;
    }
    virtual bool is_editing_live_object() {
      return false;
    }
    virtual void apply_changes_to_live_object();
    virtual void revert_changes_to_live_object();
    virtual void refresh_live_object() {
    }
    virtual void commit_changes() {
    } // Store changes in the backend but don't reset any dirty state
      // so we still can undo. Live editors reload content to reset the undo stack.
    virtual void reset_editor_undo_stack() {
    } // Called after changes were applied (mostly live objects).

    virtual void on_object_changed();

    void freeze_refresh_on_object_change();
    bool is_refresh_frozen();
    void thaw_refresh_on_object_change(bool discard_pending = false);

    virtual bool is_editor_dirty();
    virtual bool can_close();

  protected:
    boost::signals2::scoped_connection _ui_refresh_conn;

    std::set<std::string> _ignored_object_fields_for_ui_refresh;
    int _ignore_object_changes_for_ui_refresh;
    int _ignored_object_changes_for_ui_refresh;

    void add_listeners(const grt::Ref<GrtObject> &object);

    void run_from_grt(const std::function<void()> &slot);

  private:
    friend class AutoUndoEdit;

    grt::Ref<GrtObject> _object;
    void object_member_changed(const std::string &member, const grt::ValueRef &ovalue);

    void undo_applied();
  };

  struct FreezeRefresh {
    BaseEditor *_ed;
    bool _discard_pending;

    FreezeRefresh(BaseEditor *ed, bool discard_pending = true) : _ed(ed), _discard_pending(discard_pending) {
      ed->freeze_refresh_on_object_change();
    }
    ~FreezeRefresh() {
      _ed->thaw_refresh_on_object_change(_discard_pending);
    }
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC AutoUndoEdit : public grt::AutoUndo {
    static void undo_applied(grt::UndoAction *applied, grt::UndoGroup *group, BaseEditor *editor) {
      if (group == applied)
        editor->undo_applied();
    }

  public:
    AutoUndoEdit(BaseEditor *editor)
      // if editing a live object, this should be a no-op
      : grt::AutoUndo(editor->is_editing_live_object()) {
      if (group) {
        editor->scoped_connect(grt::GRT::get()->get_undo_manager()->signal_undo(),
                               std::bind(&AutoUndoEdit::undo_applied, std::placeholders::_1, group, editor));
        editor->scoped_connect(grt::GRT::get()->get_undo_manager()->signal_redo(),
                               std::bind(&AutoUndoEdit::undo_applied, std::placeholders::_1, group, editor));
      }
    }

    AutoUndoEdit(BaseEditor *editor, const grt::ObjectRef &object, const std::string &member)
      : grt::AutoUndo(new UndoObjectChangeGroup(object.id(), member), editor->is_editing_live_object()) {
      if (group) {
        editor->scoped_connect((grt::GRT::get()->get_undo_manager()->signal_undo()),
                               std::bind(&AutoUndoEdit::undo_applied, std::placeholders::_1, group, editor));
        editor->scoped_connect((grt::GRT::get()->get_undo_manager()->signal_redo()),
                               std::bind(&AutoUndoEdit::undo_applied, std::placeholders::_1, group, editor));
      }
    }
  };
};
