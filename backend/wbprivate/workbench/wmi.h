/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WMI_H_
#define _WMI_H_

#include "grt.h"

#define _WIN32_DCOM
#include <atlbase.h>
#include <wbemidl.h>
#include <comutil.h>

#pragma comment(lib, "wbemuuid.lib")

/**
 * Interface functions for Workbench modules to work with Windows Management Instrumentation.
 * This code can only be used from the Windows version of WB.
 */
namespace wmi {
  class WmiServices;
  /**
   * Helper class to for fast monitoring of certain properties (like CPU load).
   */
  class WmiMonitor {
  private:
    WmiServices* _owner;
    IWbemServices* _services;
    CComPtr<IWbemRefresher> _refresher;
    CComPtr<IWbemHiPerfEnum> _enumerator;
    long _enumeratorId;
    CComBSTR _propertyName;
    long _propertyHandle;
    long _namePropertyHandle;
    bool _findTotal; // For multiple CPUs there is a total counter which we will return.
  public:
    WmiMonitor(IWbemServices* services, const std::string& parameter);
    ~WmiMonitor();

    std::string readValue();
    inline WmiServices* owner() {
      return _owner;
    }
  };

  /**
   * Main class providing WMI based services (stats, service control, queries).
   */
  class WmiServices {
  private:
    CComPtr<IWbemServices> _services;

  protected:
    static void allocate_locator();
    static void deallocate_locator();

  public:
    WmiServices(const std::string& server, const std::string& user, const std::string& password);
    ~WmiServices();

    grt::DictListRef query(const std::string& query);
    std::string serviceControl(const std::string& service, const std::string& action);
    std::string systemStat(const std::string& what);

    WmiMonitor* startMonitoring(const std::string& parameter);
    void stopMonitoring(WmiMonitor* monitor);
  };
}

#endif // #ifndef _WMI_H_
