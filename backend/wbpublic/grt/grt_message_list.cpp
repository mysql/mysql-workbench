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

#include "grt_message_list.h"
#include "grt_manager.h"
#include "validation_manager.h"
#include "base/string_utilities.h"

using namespace bec;

//--------------------------------------------------------------------------------------------------

MessageListStorage::MessageListStorage(GRTManager *grtm) : _grtm(grtm) {
  _error_icon = IconManager::get_instance()->get_icon_id("mini_error.png");
  _warning_icon = IconManager::get_instance()->get_icon_id("mini_warning.png");
  _info_icon = IconManager::get_instance()->get_icon_id("mini_notice.png");

  scoped_connect(ValidationManager::signal_notify(),
                 std::bind(&MessageListStorage::validation_notify, this, std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3, std::placeholders::_4));
}

//--------------------------------------------------------------------------------------------------

MessageListBE *MessageListStorage::create_list(const std::string &filter_to_source) {
  MessageListBE *list = new MessageListBE(this);

  return list;
}

//--------------------------------------------------------------------------------------------------

void MessageListStorage::clear_all() {
  _entries.clear();
}

//--------------------------------------------------------------------------------------------------

void MessageListStorage::handle_message(const grt::Message &msg) {
  if (msg.type == grt::OutputMsg) {
    if (_output_handler)
      _grtm->run_once_when_idle(std::bind(_output_handler, msg.text));
    return;
  }

  MessageEntryRef entry(new MessageEntry());

  switch (msg.type) {
    case grt::ErrorMsg:
      entry->icon = _error_icon;
      break;
    case grt::WarningMsg:
      entry->icon = _warning_icon;
      break;
    case grt::InfoMsg:
      entry->icon = _info_icon;
      break;
    case grt::ControlMsg:
      entry->icon = -1; // hack
      break;
    case grt::ProgressMsg:
      // ignore
      return;

    default:
      entry->icon = 0;
      break;
  }
  entry->type = msg.type;
  entry->timestamp = msg.timestamp;
  entry->message = msg.text;
  std::string::size_type end = entry->message.size();
  while (end > 0 && entry->message[end - 1] == '\n')
    --end;
  entry->message = entry->message.substr(0, end);
  entry->detail = msg.detail;
  if (entry->icon >= 0)
    _entries.push_back(entry);
  _new_message(entry);
}

//--------------------------------------------------------------------------------------------------

void MessageListStorage::set_output_handler(const std::function<void(std::string)> &handler) {
  _output_handler = handler;
}

//--------------------------------------------------------------------------------------------------

void MessageListStorage::validation_notify(const grt::Validator::Tag &, const grt::ObjectRef &o, const std::string &m,
                                           const int level) {
  if (level != grt::NoErrorMsg) {
    grt::Message msg;

    msg.type = (grt::MessageType)level;
    msg.timestamp = time(NULL);
    msg.text = m;
    msg.progress = 0.0;

    handle_message(msg);
  }
}

//--------------------------------------------------------------------------------------------------

MessageListBE::MessageListBE(MessageListStorage *owner) : _owner(owner) {
  _notified = false;

  _conn = _owner->signal_new_message()->connect(std::bind(&MessageListBE::add_message, this, std::placeholders::_1));
}

//--------------------------------------------------------------------------------------------------

void MessageListBE::add_message(MessageListStorage::MessageEntryRef message) {
  if (message->icon == -1)
    return; // Ignore control messages.

  if (!_owner->_grtm->in_main_thread()) {
    _owner->_grtm->run_once_when_idle(std::bind(&MessageListBE::add_message, this, message));
    return;
  }
  if (_wanted_sources.empty() || _wanted_sources.find(message->source) != _wanted_sources.end()) {
    _entries.push_back(message);
    (*signal_row_added())();
  }
}

//--------------------------------------------------------------------------------------------------

void MessageListBE::add_source(const std::string &source) {
  _wanted_sources.insert(source);
}

//--------------------------------------------------------------------------------------------------

void MessageListBE::remove_source(const std::string &source) {
  _wanted_sources.erase(source);
}

//--------------------------------------------------------------------------------------------------

void MessageListBE::clear() {
  _entries.clear();
}

//--------------------------------------------------------------------------------------------------

size_t MessageListBE::count_children(const NodeId &node) {
  if (node.depth() == 0)
    return (int)_entries.size();
  return 0;
}

//--------------------------------------------------------------------------------------------------

bool MessageListBE::get_field(const NodeId &node, ColumnId column, std::string &value) {
  switch ((Column)column) {
    case Time:
      if (node[0] < _entries.size()) {
        char buffer[100];
        const struct tm *tm = localtime(&_entries[node[0]]->timestamp);

        strftime(buffer, sizeof(buffer), "%H:%M:%S", tm);
        value = buffer;
        return true;
      }
      break;

    case Message:
      if (node[0] < _entries.size()) {
        value = _entries[node[0]]->message;
        return true;
      }
      break;
    case Detail:
      if (node[0] < _entries.size()) {
        value = _entries[node[0]]->detail;
        return true;
      }
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

IconId MessageListBE::get_field_icon(const NodeId &node, ColumnId column, IconSize size) {
  if (node[0] < _entries.size())
    return _entries[node[0]]->icon;

  return 0;
}

//--------------------------------------------------------------------------------------------------

/**
 *  Returns the type of the message in the list. Since only error, info and warning messages
 *  are handled here only these 3 types are returned.
 */
grt::MessageType bec::MessageListBE::get_message_type(const NodeId &node) {
  if (node[0] < _entries.size())
    return _entries[node[0]]->type;

  return grt::InfoMsg;
}

//--------------------------------------------------------------------------------------------------

size_t MessageListBE::count() {
  _notified = false;
  return _entries.size();
}

//--------------------------------------------------------------------------------------------------

MenuItemList MessageListBE::get_popup_items_for_nodes(const std::vector<NodeId> &nodes) {
  MenuItemList menu;
  MenuItem item;

  item.type = bec::MenuAction;
  item.internalName = "clear_messages";
  item.accessibilityName = "Clear";
  item.caption = _("Clear");
  item.enabled = true;
  menu.push_back(item);

  return menu;
}

//--------------------------------------------------------------------------------------------------

bool MessageListBE::activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &nodes) {
  if (name == "clear_messages") {
    clear();
    do_ui_refresh();
  } else
    return false;

  return true;
}

//--------------------------------------------------------------------------------------------------
