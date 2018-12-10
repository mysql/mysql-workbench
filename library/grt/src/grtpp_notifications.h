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

#pragma once

// A variant of the global notification center which can take GRT objects in notifications.

#include "grt.h"
#include "base/notifications.h"

namespace grt {
  class MYSQLGRT_PUBLIC GRTObserver : public base::Observer {
  protected:
    friend class GRTNotificationCenter;
    virtual void handle_grt_notification(const std::string &name, ObjectRef sender, DictRef info) = 0;

    virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) override;

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
