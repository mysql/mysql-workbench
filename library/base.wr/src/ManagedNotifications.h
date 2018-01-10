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

#include "base/notifications.h"

namespace MySQL {
  namespace Workbench {

    /**
     * Classes which want to get notified implement this interface.
     */
  public
    interface class IWorkbenchObserver {
      void HandleNotification(const System::String ^ name, System::IntPtr sender,
                              const System::Collections::Generic::Dictionary<System::String ^, System::String ^> ^
                                info);
    };

    /**
     * Internal class to connect managed and unmanaged code.
     */
  private
    class InterfacedObserver : public base::Observer {
    private:
      gcroot<IWorkbenchObserver ^> _managed_observer;

      virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);

    public:
      InterfacedObserver(IWorkbenchObserver ^ native_observer);

      bool WrapsObserver(IWorkbenchObserver ^ observer);
    };

  public
    ref class ManagedNotificationCenter {
      static std::vector<InterfacedObserver *> *observer_list = NULL;

    public:
      static void AddObserver(IWorkbenchObserver ^ observer, System::String ^ notification);
      static void RemoveObserver(IWorkbenchObserver ^ observer, System::String ^ notification);

      static void Send(System::String ^ notification, System::IntPtr sender);
    };

  } // namespace Workbench
} // namespace MySQL
