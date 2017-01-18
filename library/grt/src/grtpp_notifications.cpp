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

#include "grtpp_notifications.h"

using namespace grt;

void GRTNotificationCenter::setup() {
  base::NotificationCenter::set_instance(new GRTNotificationCenter());
}

GRTNotificationCenter *GRTNotificationCenter::get() {
  return dynamic_cast<GRTNotificationCenter *>(base::NotificationCenter::get());
}

void GRTNotificationCenter::add_grt_observer(GRTObserver *observer, const std::string &name, ObjectRef object) {
  GRTObserverEntry entry;
  entry.observer = observer;
  entry.observed_notification = name;
  entry.observed_object_id = object.is_valid() ? object.id() : "";
  _grt_observers.push_back(entry);
}

bool GRTNotificationCenter::remove_grt_observer(GRTObserver *observer, const std::string &name, ObjectRef object) {
  bool found = false;
  for (std::list<GRTObserverEntry>::iterator next, iter = _grt_observers.begin(); iter != _grt_observers.end();) {
    next = iter;
    ++next;
    if (iter->observer == observer && (name.empty() || name == iter->observed_notification) &&
        (!object.is_valid() || object.id() == iter->observed_object_id)) {
      found = true;
      _grt_observers.erase(iter);
    }
    iter = next;
  }
  return found;
}

void GRTNotificationCenter::send_grt(const std::string &name, ObjectRef sender, DictRef info) {
  if (name.substr(0, 3) != "GRN")
    throw std::invalid_argument("Attempt to send GRT notification with a name that doesn't start with GRN");

  // act on a copy of the observer list, because one of them could remove stuff from the list
  std::list<GRTObserverEntry> copy(_grt_observers);
  for (std::list<GRTObserverEntry>::iterator iter = copy.begin(); iter != copy.end(); ++iter) {
    if ((iter->observed_notification.empty() || iter->observed_notification == name) &&
        (iter->observed_object_id.empty() || !sender.is_valid() || iter->observed_object_id == sender.id())) {
      iter->observer->handle_grt_notification(name, sender, info);
    }
  }
}
