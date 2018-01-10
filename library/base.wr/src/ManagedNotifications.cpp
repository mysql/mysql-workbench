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

#include "ManagedNotifications.h"
#include "LogWrapper.h"
#include "ConvUtils.h"

using namespace System;
using namespace System::Collections::Generic;

using namespace base;

using namespace MySQL::Workbench;

//----------------- InterfacedObserver -------------------------------------------------------------

InterfacedObserver::InterfacedObserver(IWorkbenchObserver ^ native_observer) {
  _managed_observer = native_observer;
}

//--------------------------------------------------------------------------------------------------

void InterfacedObserver::handle_notification(const std::string &name, void *sender, NotificationInfo &info) {
  _managed_observer->HandleNotification(CppStringToNativeRaw(name), IntPtr(sender), CppStringMapToDictionary(info));
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines if this object is the interface for the given observer.
 */
bool InterfacedObserver::WrapsObserver(IWorkbenchObserver ^ observer) {
  return static_cast<IWorkbenchObserver ^>(_managed_observer) == observer;
}

//--------------------------------------------------------------------------------------------------

void ManagedNotificationCenter::AddObserver(IWorkbenchObserver ^ observer, String ^ notification) {
  msclr::lock lock(observer);
  if (!observer_list)
    observer_list = new std::vector<InterfacedObserver *>;

  InterfacedObserver *iObserver = new InterfacedObserver(observer);
  observer_list->push_back(iObserver);
  base::NotificationCenter::get()->add_observer(iObserver, NativeToCppStringRaw(notification));

  Logger::LogDebug("Managed Notify", 1, "Registered managed observer for " + notification + "\n");
}

//--------------------------------------------------------------------------------------------------

void ManagedNotificationCenter::RemoveObserver(IWorkbenchObserver ^ observer, String ^ notification) {
  msclr::lock lock(observer);
  if (!observer_list)
    return;

  std::string message =
    (notification == nullptr || notification->Length == 0) ? "" : NativeToCppStringRaw(notification);

  // Find the interfaced observer we created for the workbench observer.
  for (std::vector<InterfacedObserver *>::const_iterator iterator = observer_list->begin();
       iterator != observer_list->end(); ++iterator) {
    if ((*iterator)->WrapsObserver(observer)) {
      base::NotificationCenter::get()->remove_observer(*iterator, message);
      delete *iterator;
      observer_list->erase(iterator);
      if (observer_list->empty()) {
        delete observer_list;
        observer_list = NULL;
      }

      if (notification->Length == 0)
        notification = "any notification";
      Logger::LogDebug("Managed Notify", 1, "Removed managed observer for " + notification + "\n");

      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void ManagedNotificationCenter::Send(String ^ notification, IntPtr sender) {
  base::NotificationCenter::get()->send(NativeToCppStringRaw(notification),
                                        sender != IntPtr::Zero ? sender.ToPointer() : NULL);
}

//--------------------------------------------------------------------------------------------------
