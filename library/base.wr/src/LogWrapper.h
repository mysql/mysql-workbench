/* 
 * Copyright (c) 2011, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __LOGWRAPPER_H__
#define __LOGWRAPPER_H__

#include "base/log.h"

namespace MySQL {
namespace Workbench {

  /**
   * Wrapper for the base logger class, so we can use it from C# too.
   * Note: due to the way the base class is implemented we only have one log domain (Managed) for
   * all modules.
   */
  public enum class LogLevel {
    LogNone     = base::Logger::LogNone, 
    LogError    = base::Logger::LogError,
    LogWarning  = base::Logger::LogWarning,
    LogInfo     = base::Logger::LogInfo,
    LogDebug    = base::Logger::LogDebug,
    LogDebug2   = base::Logger::LogDebug2,
    LogDebug3   = base::Logger::LogDebug3
  };

  public ref class Logger
  {
  public:
    static void InitLogger(System::String^ path);
    static void EnableLogLevel(LogLevel level);

    static void LogError(System::String^ domain, System::String^ message);
    static void LogWarning(System::String^ domain, System::String^ message);
    static void LogInfo(System::String^ domain, System::String^ message);
    static void LogDebug(System::String^ domain, int verbosity, System::String^ message);

    static property System::String^ ActiveLevel
    {
      System::String^ get();
      void set(System::String^ value);
    }
  };

} // namespace Workbench
} // namespace MySQL

#endif // __LOGWRAPPER_H__
