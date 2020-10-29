/*
 * Copyright (c) 2011, 2020, Oracle and/or its affiliates.
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

#include "base/notifications.h"
#include "base/log.h"
#include <stdexcept>
#include <algorithm>

DEFAULT_LOG_DOMAIN(DOMAIN_BASE);

using namespace base;

static NotificationCenter *nc = 0;

//----------------------------------------------------------------------------------------------------------------------

Observer::~Observer() {
  NotificationCenter *nc = NotificationCenter::get();
  if (nc->is_registered(this))
    logError("Notifications: Observer %p was deleted while still listening for notifications.\n", this);
}

//----------------------------------------------------------------------------------------------------------------------

void NotificationCenter::set_instance(NotificationCenter *center) {
  std::map<std::string, NotificationHelp> help;

  if (nc) {
    help = nc->_notification_help;
    delete nc;
  }
  nc = center;
  nc->_notification_help = help;
}

//----------------------------------------------------------------------------------------------------------------------

NotificationCenter *NotificationCenter::get() {
  if (!nc)
    nc = new NotificationCenter();
  return nc;
}

//----------------------------------------------------------------------------------------------------------------------

NotificationCenter::~NotificationCenter() {
  if (_observers.size() > 0) {
    logError("Notifications: The following observers are not unregistered:\n");

    for (std::list<ObserverEntry>::iterator next, iter = _observers.begin(); iter != _observers.end(); ++iter)
      logError("\tObserver %p, for message: %s\n", iter->observer, iter->observed_notification.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void NotificationCenter::register_notification(const std::string &name, const std::string &context,
                                               const std::string &general_info, const std::string &sender_info,
                                               const std::string &info_info) {
  NotificationHelp help;
  help.context = context;
  help.summary = general_info;
  help.sender = sender_info;
  help.info = info_info;
  _notification_help[name] = help;
}

//----------------------------------------------------------------------------------------------------------------------

void NotificationCenter::add_observer(Observer *observer, const std::string &name) {
  ObserverEntry entry;
  entry.observer = observer;
  entry.observed_notification = name;
  _observers.push_back(entry);
}

//----------------------------------------------------------------------------------------------------------------------

bool NotificationCenter::remove_observer(Observer *observer, const std::string &name) {
  auto iter = std::remove_if(_observers.begin(), _observers.end(), [&](auto &value) {
    return value.observer == observer && (name.empty() || name == value.observed_notification);
  });
  
  bool found = iter != _observers.end();
  
  _observers.erase(iter, _observers.end());

  return found;
}

//----------------------------------------------------------------------------------------------------------------------

bool NotificationCenter::is_registered(Observer *observer) {
  for (std::list<ObserverEntry>::iterator next, iter = _observers.begin(); iter != _observers.end(); ++iter) {
    if (iter->observer == observer)
      return true;
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void NotificationCenter::send(const std::string &name, void *sender, NotificationInfo &info) {
  if (name.substr(0, 2) != "GN")
    throw std::invalid_argument("Attempt to send notification with a name that doesn't start with GN\n");

  if (_notification_help.find(name) == _notification_help.end())
    logInfo("Notification %s is not registered\n", name.c_str());

  // act on a copy of the observer list, because one of them could remove stuff from the list
  std::list<ObserverEntry> copy(_observers);
  for (std::list<ObserverEntry>::iterator iter = copy.begin(); iter != copy.end(); ++iter) {
    if (iter->observed_notification.empty() || iter->observed_notification == name) {
      // if (iter->callback)
      //  iter->callback(name, sender, info);
      // else
      iter->observer->handle_notification(name, sender, info);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void NotificationCenter::send(const std::string &name, void *sender) {
  NotificationInfo info;
  send(name, sender, info);
}

//----------------------------------------------------------------------------------------------------------------------
