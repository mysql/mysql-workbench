/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "base/ui_form.h"
#include "base/log.h"
#include "base/notifications.h"

using namespace bec;

static std::map<std::string, UIForm *> ui_form_instances;

UIForm::UIForm() : _owner_data(0), _frontend_data(0) {
  ui_form_instances[form_id()] = this;

  base::NotificationInfo info;
  info["form"] = form_id();
  base::NotificationCenter::get()->send(GNUIFormCreated, 0, info);
}

UIForm::~UIForm() {
  base::NotificationInfo info;
  info["form"] = form_id();
  base::NotificationCenter::get()->send(GNUIFormDestroyed, 0, info);

  ui_form_instances.erase(ui_form_instances.find(form_id()));
}

bec::UIForm *UIForm::form_with_id(const std::string &id) {
  if (ui_form_instances.find(id) != ui_form_instances.end())
    return ui_form_instances[id];
  return 0;
}

std::string UIForm::form_id() {
  return base::strfmt("<UIForm %p>", this);
}

void UIForm::set_frontend_data(void *data) {
  _frontend_data = data;
}

void *UIForm::get_frontend_data() {
  return _frontend_data;
}

void UIForm::set_owner_data(void *data) {
  _owner_data = data;
}
void *UIForm::get_owner_data() {
  return _owner_data;
}

bool UIForm::is_main_form() {
  return false;
}

// Target description for cut/copy/delete menu items and for paste, after a copy is made.
std::string UIForm::get_edit_target_name() {
  return "";
}

bool UIForm::can_undo() {
  return false;
}
bool UIForm::can_redo() {
  return false;
}
bool UIForm::can_cut() {
  return can_copy() && can_delete();
}
bool UIForm::can_copy() {
  return false;
}
bool UIForm::can_paste() {
  return false;
}
bool UIForm::can_delete() {
  return false;
}
bool UIForm::can_select_all() {
  return false;
}

void UIForm::undo() {
}
void UIForm::redo() {
}
void UIForm::cut() {
}
void UIForm::copy() {
}
void UIForm::paste() {
}
void UIForm::delete_selection() {
}
void UIForm::select_all() {
}

//--------------------------------------------------------------------------------------------------

static struct RegisterNotifDocs_ui_form {
  RegisterNotifDocs_ui_form() {
    base::NotificationCenter::get()->register_notification(
      GNUIFormCreated, "application",
      "Sent when a new form object is created.\n"
      "Note: at the time this notification is sent, the form object is not yet fully initialized.",
      "", "form - identifier of the form");

    base::NotificationCenter::get()->register_notification(GNUIFormDestroyed, "application",
                                                           "Sent when a new form object is destroyed.", "",
                                                           "form - identifier of the form");

    base::NotificationCenter::get()->register_notification("GNFormTitleDidChange", "application",
                                                           "Sent when a form's title change.", "",
                                                           "form - identifier of the form\n"
                                                           "title - the new title of the form");
    base::NotificationCenter::get()->register_notification("GNTextSelectionChanged", "application",
                                                           "Sent when the selection or the caret position\n"
                                                           " in an editor changed.",
                                                           "", "");
    base::NotificationCenter::get()->register_notification("GNFocusChanged", "application",
                                                           "Sent when a control gets the input focus.", "", "");
    base::NotificationCenter::get()->register_notification(
      "GNColorsChanged", "application",
      "Sent when either the color scheme or one of its colors was changed in the preferences.", "", "");
  }
} initdocs_ui_form;
