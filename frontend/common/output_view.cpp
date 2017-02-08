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

#include "workbench/wb_context.h"
#include "output_view.h"
#include "base/string_utilities.h"

using namespace wb;

OutputView::OutputView(WBContext* context)
  : mforms::AppView(true, "output", true),
    _splitter(true),
    _message_list(mforms::TreeFlatList),
    _output_text(mforms::BothScrollBars) {
  _wb = context;
  _can_track_changes = false; // True when the UI setup is done and we can track further UI changes.

  add(&_splitter, true, true);

  _splitter.add(&_message_list);
  _splitter.add(&_output_text);
  Box::scoped_connect(_splitter.signal_position_changed(), std::bind(&OutputView::splitter_moved, this));

  _message_list.add_column(mforms::IconStringColumnType, "", 100, false);
  _message_list.add_column(mforms::StringColumnType, "Message", 500, false);
  _message_list.add_column(mforms::StringColumnType, "Detail", 200, false);
  _message_list.end_columns();

  _storage = bec::GRTManager::get()->get_messages_list();
  _storage->set_output_handler(std::bind(&mforms::TextBox::append_text, &_output_text, std::placeholders::_1, true));

  _messages = _storage->create_list();
  _message_list.set_selection_mode(mforms::TreeSelectMultiple);
  refresh();

  UIForm::scoped_connect(_messages->signal_row_added(), std::bind(&OutputView::row_added, this));

  _output_text.set_read_only(true);

  set_on_close(std::bind(&OutputView::will_close, this));

  _context_menu.add_item_with_title(_("Copy selected entries to clipboard"),
                                    std::bind(&OutputView::handle_command, this, "copy"));
  _context_menu.add_item_with_title(_("Clear output"), std::bind(&OutputView::handle_command, this, "clear"));
  _message_list.set_context_menu(&_context_menu);
}

//--------------------------------------------------------------------------------------------------

OutputView::~OutputView() {
  _storage->set_output_handler(std::function<void(std::string)>());
  delete _messages;
  _messages = 0;
}

//--------------------------------------------------------------------------------------------------

void OutputView::handle_command(const std::string& command) {
  if (command == "copy") {
    std::list<mforms::TreeNodeRef> selection;
    selection = _message_list.get_selection();
    std::string result;
    for (std::list<mforms::TreeNodeRef>::const_iterator iterator = selection.begin(); iterator != selection.end();
         iterator++) {
      std::string type_string;
      int row = _message_list.row_for_node(*iterator);
      grt::MessageType type = _messages->get_message_type(row);
      switch (type) {
        case grt::WarningMsg:
          type_string = _("Warning");
          break;
        case grt::ErrorMsg:
          type_string = _("Error");
          break;
        default:
          type_string = _("Info");
      }
      std::string time;
      std::string message;
      std::string details;
      _messages->get_field(row, bec::MessageListBE::Time, time);
      _messages->get_field(row, bec::MessageListBE::Message, message);
      if (message.empty())
        message = _("none");
      _messages->get_field(row, bec::MessageListBE::Detail, details);
      if (details.empty())
        details = _("n/a");
      result.append(base::strfmt(_("%s:\tTime: %s\tMessage: %s\tDetails: %s\n"), type_string.c_str(), time.c_str(),
                                 message.c_str(), details.c_str()));
    }
    mforms::Utilities::set_clipboard_text(result);
  } else if (command == "clear") {
    _messages->clear();
    refresh();
  }
}

//--------------------------------------------------------------------------------------------------

void OutputView::row_added() {
  mforms::TreeNodeRef node;
  if (_message_list.root_node()) {
    for (size_t c = _messages->count(), i = _message_list.root_node()->count(); i < c; i++) {
      std::string s;
      bec::IconId icon = _messages->get_field_icon(i, 0, bec::Icon16);
      node = _message_list.add_node();
      _messages->get_field(i, bec::MessageListBE::Time, s);
      node->set_string(0, s);
      if (icon >= 0)
        node->set_icon_path(0, bec::IconManager::get_instance()->get_icon_path(icon));
      _messages->get_field(i, bec::MessageListBE::Message, s);
      node->set_string(1, s);
      _messages->get_field(i, bec::MessageListBE::Detail, s);
      node->set_string(2, s);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void OutputView::refresh() {
  _message_list.clear();

  row_added();
}

//--------------------------------------------------------------------------------------------------

bool OutputView::will_close() {
  _can_track_changes = false;

  mforms::App::get()->undock_view(this);

  return true;
}

//--------------------------------------------------------------------------------------------------

void OutputView::setup_ui() {
  int splitter_position = _wb->read_state("message_width", "output_view", 500);
  _splitter.set_divider_position(splitter_position);
  _can_track_changes = true;
}

//--------------------------------------------------------------------------------------------------

void OutputView::splitter_moved() {
  if (!_can_track_changes)
    return;

  int splitter_position = _splitter.get_divider_position();
  _wb->save_state("message_width", "output_view", splitter_position);
}

//--------------------------------------------------------------------------------------------------
