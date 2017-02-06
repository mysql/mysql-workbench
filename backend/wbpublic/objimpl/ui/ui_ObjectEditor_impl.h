/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _UI_OBJECTEDITOR_IMPL_H_
#define _UI_OBJECTEDITOR_IMPL_H_

#include "grts/structs.ui.h"

#define GRNObjectEditorWillOpen "GRNObjectEditorWillOpen"
#define GRNObjectEditorWillClose "GRNObjectEditorWillClose"
#define GRNObjectEditorDidClose "GRNObjectEditorDidClose"
#define GRNEditorFormDidSwitchObject "GRNEditorFormDidSwitchObject"
#define GRNEditorFormWillSave "GRNEditorFormWillSave"
#define GRNEditorFormDidRevert "GRNEditorFormDidRevert"

namespace bec {
  class BaseEditor;
}

class ui_ObjectEditor::ImplData {
public:
  ImplData(ui_ObjectEditor *self) : _self(self) {
  }

  void notify_will_open();
  bool notify_will_close();
  void notify_did_close();

  void notify_did_switch_object(bec::BaseEditor *editor);

  // live editors only
  void notify_will_save();
  void notify_did_revert();

  GrtObjectRef edited_object();
  bool editing_live_object();

private:
  ui_ObjectEditor *_self;
  bec::BaseEditor *_editor;

  ui_ObjectEditorRef self() {
    return ui_ObjectEditorRef(_self);
  }
};

#endif
