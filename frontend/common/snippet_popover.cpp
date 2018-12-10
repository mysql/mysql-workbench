/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "snippet_popover.h"

#include "base/log.h"
#include "base/drawing.h"

#include "mforms/imagebox.h"
#include "mforms/textentry.h"
#include "mforms/label.h"
#include "mforms/panel.h"
#include "mforms/app.h"
#include "mforms/code_editor.h"
#include "mforms/button.h"
#include "mforms/box.h"

using namespace wb;
using namespace mforms;

//----------------- SnippetPopover -------------------------------------------------------------------------------------

SnippetPopover::SnippetPopover(mforms::View *owner) : Popover(owner, mforms::PopoverStyleNormal) {
  setName("Snippet Editor");
  _content = manage(new Box(false));
  _content->set_name("Snippet Content");

  _header = manage(new Box(true));
  _header->set_spacing(10);
  ImageBox* image = manage(new ImageBox());
  image->set_image(mforms::App::get()->get_resource_path("snippet_sql.png"));
  _heading_label = manage(new Label("Heading"), false);
  _heading_label->set_style(mforms::BoldStyle);
  _heading_entry = manage(new TextEntry(), false);
  _header->add(image, false, true);
  _header->add(_heading_label, true, true);

  _borderPanel = manage(new Panel(mforms::FilledPanel));

  base::Color backgroundColor = base::Color::getSystemColor(base::WindowBackgroundColor);
  _borderPanel->set_back_color(backgroundColor.to_html());

  _borderPanel->set_padding(1);
  _editor = manage(new CodeEditor());
  _editor->set_language(mforms::LanguageMySQL);
  _editor->set_text("USE SQL CODE;");
  _editor->set_features(mforms::FeatureGutter, false);
  _editor->signal_changed()->connect(
    std::bind(&SnippetPopover::text_changed, this, std::placeholders::_1, std::placeholders::_2));
  _borderPanel->add(_editor);

  Box* button_box = manage(new Box(true));
  button_box->set_spacing(8);
  button_box->set_name("Button bar");

  _revert_button = manage(new Button(mforms::ToolButton));
  _revert_button->set_tooltip("Discard all changes and revert to the current version");
  _revert_button->set_icon(App::get()->get_resource_path(
#ifdef __APPLE__
    "tiny_undo_mac.png"
#else
    "tiny_undo.png"
#endif
    ));
  _revert_button->set_name("Revert");
  _revert_button->signal_clicked()->connect(std::bind(&SnippetPopover::revert_clicked, this));

  _edit_button = manage(new Button());
  _edit_button->set_text("Edit");
  _edit_button->set_size(65, -1);
  _edit_button->signal_clicked()->connect(std::bind(&SnippetPopover::edit_clicked, this));

  _close_button = manage(new Button());
  _close_button->set_text("Done");
  _close_button->set_size(65, -1);
  _close_button->signal_clicked()->connect(std::bind(&SnippetPopover::close_clicked, this));

  button_box->add(_revert_button, false, true);
  button_box->add_end(_close_button, false, true);
  button_box->add_end(_edit_button, false, true);

  _content->add(_header, false, true);
  _content->add(_borderPanel, true, true);
  _content->add_end(button_box, false, true);
  _content->set_spacing(4);

  set_content(_content);

  base::NotificationCenter::get()->add_observer(this, "GNColorsChanged");
}

//----------------------------------------------------------------------------------------------------------------------

SnippetPopover::~SnippetPopover() {
  base::NotificationCenter::get()->remove_observer(this);

  _heading_label->release();
  _heading_entry->release();
  _content->release();
}

//----------------------------------------------------------------------------------------------------------------------

void SnippetPopover::revert_clicked() {
  _heading_label->set_text(_original_heading);
  _heading_entry->set_value(_original_heading);
  _editor->set_value(_original_text);
  _revert_button->set_enabled(false);
  set_read_only(true);
}

//----------------------------------------------------------------------------------------------------------------------

void SnippetPopover::edit_clicked() {
  set_read_only(false);
}

//----------------------------------------------------------------------------------------------------------------------

void SnippetPopover::close_clicked() {
  close();
  _closed();
}

//----------------------------------------------------------------------------------------------------------------------

void SnippetPopover::text_changed(int start_line, int lines_changed) {
  _revert_button->set_enabled(true);
}

//----------------------------------------------------------------------------------------------------------------------

void SnippetPopover::set_heading(const std::string& text) {
  _original_heading = text;
  _heading_entry->set_value(text);
  _heading_label->set_text(text);
}

//----------------------------------------------------------------------------------------------------------------------

void SnippetPopover::set_text(const std::string& text) {
  _original_text = text;
  _editor->set_value(text);
  _revert_button->set_enabled(false);
}

//----------------------------------------------------------------------------------------------------------------------

void SnippetPopover::set_read_only(bool flag) {
  // We have to exchange a label and a text entry, depending on the read-only state
  // because we cannot give the text entry a display format and cannot edit a label.
  if (flag) {
    _heading_label->set_text(_heading_entry->get_string_value());
    if (_header->contains_subview(_heading_entry))
      _header->remove(_heading_entry);
    if (!_header->contains_subview(_heading_label))
      _header->add(_heading_label, true, true);
  } else {
    if (_header->contains_subview(_heading_label))
      _header->remove(_heading_label);
    if (!_header->contains_subview(_heading_entry))
      _header->add(_heading_entry, true, true);
    _heading_entry->focus();
  }
  _editor->set_features(mforms::FeatureReadOnly, flag);
  _edit_button->set_enabled(flag);
}

//----------------------------------------------------------------------------------------------------------------------

std::string SnippetPopover::get_text() {
  return _editor->get_text(false);
}

//----------------------------------------------------------------------------------------------------------------------

std::string SnippetPopover::get_heading() {
  return _heading_entry->get_string_value();
}

//----------------------------------------------------------------------------------------------------------------------

bool SnippetPopover::has_changed() {
  // We don't have a change event from the text entry so we compare the content instead.
  return _revert_button->is_enabled() || (_heading_entry->get_string_value() != _original_heading);
}

//----------------------------------------------------------------------------------------------------------------------

void SnippetPopover::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  if (name == "GNColorsChanged") {
    base::Color backgroundColor = base::Color::getSystemColor(base::WindowBackgroundColor);
    _borderPanel->set_back_color(backgroundColor.to_html());
  }
}

//----------------------------------------------------------------------------------------------------------------------
