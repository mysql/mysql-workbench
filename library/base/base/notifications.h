/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _BASE_NOTIFICATIONS_H_
#define _BASE_NOTIFICATIONS_H_

// Global Notification Center

// This can be easily misused so before using a new notification, discuss with others
// to ensure it's the right way.

#include <list>
#include <string>
#include <map>

#include "base/common.h"

namespace base {
  typedef std::map<std::string, std::string> NotificationInfo;

  class BASELIBRARY_PUBLIC_FUNC Observer {
  public:
    virtual void handle_notification(const std::string &name, void *sender, NotificationInfo &info) = 0;
    virtual ~Observer();
  };

  class BASELIBRARY_PUBLIC_FUNC NotificationCenter {
  private:
    struct ObserverEntry {
      std::string observed_notification;
      Observer *observer;
    };

    std::list<ObserverEntry> _observers;

  public:
    struct NotificationHelp {
      std::string context;
      std::string summary;
      std::string sender;
      std::string info;
    };

  private:
    // notification name -> help
    std::map<std::string, NotificationHelp> _notification_help;

  protected:
    static void set_instance(NotificationCenter *center);

  public:
    static NotificationCenter *get();
    virtual ~NotificationCenter();

    void register_notification(const std::string &name, const std::string &context, const std::string &general_info,
                               const std::string &sender_info, // type - description
                               const std::string &info_info);  // fields - description
    const std::map<std::string, NotificationHelp> &get_registered_notifications() {
      return _notification_help;
    }
    NotificationHelp get_registered_notification(const std::string &name) {
      return _notification_help[name];
    }

    void add_observer(Observer *observer, const std::string &name = "");
    bool remove_observer(Observer *observer, const std::string &name = "");
    bool is_registered(Observer *observer);

    // notification names MUST start with GN (global notification) for easy grepping

    // must be called from main thread only
    void send(const std::string &name, void *sender, NotificationInfo &info);
    void send(const std::string &name, void *sender);
  };
};

#endif
