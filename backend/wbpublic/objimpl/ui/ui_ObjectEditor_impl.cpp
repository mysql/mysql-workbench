/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "ui_ObjectEditor_impl.h"
#include "grtpp_notifications.h"

#include "grt/editor_base.h"

void ui_ObjectEditor::ImplData::notify_will_open() {
  grt::GRTNotificationCenter::get()->send_grt(GRNObjectEditorWillOpen, self(), grt::DictRef());
}

bool ui_ObjectEditor::ImplData::notify_will_close() {
  grt::DictRef info(true);
  info.gset("cancel", 0);
  grt::GRTNotificationCenter::get()->send_grt(GRNObjectEditorWillClose, self(), info);
  if (info.get_int("cancel") != 0)
    return false;
  return true;
}

void ui_ObjectEditor::ImplData::notify_did_close() {
  grt::GRTNotificationCenter::get()->send_grt(GRNObjectEditorDidClose, self(), grt::DictRef());
}

void ui_ObjectEditor::ImplData::notify_did_switch_object(bec::BaseEditor *editor) {
  _editor = editor;
  self()->object(editor->get_object());
  grt::GRTNotificationCenter::get()->send_grt(GRNEditorFormDidSwitchObject, self(), grt::DictRef());
}

void ui_ObjectEditor::ImplData::notify_will_save() {
  grt::GRTNotificationCenter::get()->send_grt(GRNEditorFormWillSave, self(), grt::DictRef());
}

void ui_ObjectEditor::ImplData::notify_did_revert() {
  grt::GRTNotificationCenter::get()->send_grt(GRNEditorFormDidRevert, self(), grt::DictRef());
}

GrtObjectRef ui_ObjectEditor::ImplData::edited_object() {
  return _editor ? _editor->get_object() : GrtObjectRef();
}

//--------------------------------------------------------------------------------------------------

static struct RegisterNotifDocs_ui_ObjectEditor_impl {
  RegisterNotifDocs_ui_ObjectEditor_impl() {
    // GRNObjectEditorDidCreate?
    base::NotificationCenter::get()->register_notification(GRNObjectEditorWillOpen, "objecteditor",
                                                           "Sent before an object editor is shown on screen.",
                                                           ui_ObjectEditor::static_class_name(), "");
    base::NotificationCenter::get()->register_notification(
      GRNObjectEditorWillClose, "objecteditor",
      "Sent when an object editor is about to be closed.\n"
      "If the editor supports it, setting the cancel in the info dictionary to 1 will prevent it from being closed.",
      ui_ObjectEditor::static_class_name(), "cancel - 0 or 1, whether closing of the editor should be cancelled");
    base::NotificationCenter::get()->register_notification(
      GRNObjectEditorDidClose, "objecteditor",
      "Sent when an object editor was closed.\n"
      "",
      ui_ObjectEditor::static_class_name(), "cancel - 0 or 1, whether closing of the editor should be cancelled");

    base::NotificationCenter::get()->register_notification(
      GRNEditorFormDidSwitchObject, "objecteditor", "Sent when the object editor receives a new object to be edited.",
      ui_ObjectEditor::static_class_name(), "old-object - reference to the object that was being previously edited");
    base::NotificationCenter::get()->register_notification(GRNEditorFormWillSave, "objecteditor",
                                                           "In live object editors, this is sent when the user clicks "
                                                           "the Save button and the editor contents are about to be "
                                                           "saved.",
                                                           ui_ObjectEditor::static_class_name(), "");
    base::NotificationCenter::get()->register_notification(GRNEditorFormDidRevert, "objecteditor",
                                                           "In live object editors, this is sent when the user clicks "
                                                           "the Revert button and the editor contents are about to be "
                                                           "restored.",
                                                           ui_ObjectEditor::static_class_name(), "");
  }
} initdocs_ui_ObjectEditor_impl;
