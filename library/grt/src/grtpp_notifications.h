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

#pragma once

#include "grt.h"
#include "base/notifications.h"

namespace grt {
  class MYSQLGRT_PUBLIC GRTObserver : public base::Observer {
  protected:
    friend class GRTNotificationCenter;
    virtual void handle_grt_notification(const std::string &name, ObjectRef sender, DictRef info) = 0;

    virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
    }

  public:
    virtual ~GRTObserver() {
    }
  };

  class MYSQLGRT_PUBLIC GRTNotificationCenter : public base::NotificationCenter {
    struct GRTObserverEntry {
      std::string observed_notification;
      GRTObserver *observer;
      std::string observed_object_id;
    };

    std::list<GRTObserverEntry> _grt_observers;

  public:
    static GRTNotificationCenter *get();

    void add_grt_observer(GRTObserver *observer, const std::string &name = "", ObjectRef object = ObjectRef());
    bool remove_grt_observer(GRTObserver *observer, const std::string &name = "", ObjectRef object = ObjectRef());

    // must be called from main thread only
    void send_grt(const std::string &name, ObjectRef sender, DictRef info);

  public:
    static void setup();
  };
};
