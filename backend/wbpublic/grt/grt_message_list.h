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

#ifndef _GRT_MESSAGE_LIST_H_
#define _GRT_MESSAGE_LIST_H_

#include "base/trackable.h"
#include "tree_model.h"
#include "refresh_ui.h"
#include <set>

namespace bec {

  class GRTManager;
  class MessageListBE;

  class WBPUBLICBACKEND_PUBLIC_FUNC MessageListStorage : public base::trackable {
  public:
    struct MessageEntry {
      grt::MessageType type;
      IconId icon;
      time_t timestamp;
      std::string source;
      std::string message;
      std::string detail;
    };

    typedef std::shared_ptr<MessageEntry> MessageEntryRef;

    MessageListStorage(GRTManager *grtm);

    boost::signals2::signal<void(MessageEntryRef)> *signal_new_message() {
      return &_new_message;
    }

    MessageListBE *create_list(const std::string &filter_to_source = "");

    void clear_all();

    void handle_message(const grt::Message &msg);
    void set_output_handler(const std::function<void(std::string)> &handler);

  private:
    friend class MessageListBE;

    GRTManager *_grtm;
    boost::signals2::signal<void(MessageEntryRef)> _new_message;
    std::function<void(std::string)> _output_handler;
    std::vector<MessageEntryRef> _entries;
    IconId _error_icon;
    IconId _warning_icon;
    IconId _info_icon;

    void validation_notify(const grt::Validator::Tag &tag, const grt::ObjectRef &o, const std::string &msg,
                           const int level);
  };

  //! Backend for message's tab in Output window. To clear run popup and feed
  //! menu item's name to activate_node_popup_item.
  class WBPUBLICBACKEND_PUBLIC_FUNC MessageListBE : public TreeModel, public RefreshUI {
  public:
    enum Column { Time, Message, Detail };

    void clear();
    virtual size_t count_children(const NodeId &parent);
    virtual bool get_field(const NodeId &node, ColumnId column, std::string &value);
    virtual void refresh() {
    }
    virtual IconId get_field_icon(const NodeId &node, ColumnId column, IconSize size);
    virtual grt::MessageType get_message_type(const NodeId &node);
    virtual size_t count();
    virtual MenuItemList get_popup_items_for_nodes(const std::vector<NodeId> &nodes);
    virtual bool activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &nodes);

    void add_source(const std::string &source);
    void remove_source(const std::string &source);

    void add_message(MessageListStorage::MessageEntryRef message);

    boost::signals2::signal<void()> *signal_show() {
      return &_list_show;
    }
    boost::signals2::signal<void()> *signal_row_added() {
      return &_list_changed;
    }

  private:
    friend class MessageListStorage;

    MessageListBE(MessageListStorage *owner);

  private:
    MessageListStorage *_owner;

    std::vector<MessageListStorage::MessageEntryRef> _entries;
    boost::signals2::signal<void()> _list_changed;
    boost::signals2::signal<void()> _list_show;
    std::set<std::string> _wanted_sources;
    boost::signals2::scoped_connection _conn;

    bool _notified;
  };
};

#endif
