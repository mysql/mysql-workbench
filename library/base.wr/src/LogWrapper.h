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

#include "base/log.h"

namespace MySQL {
  namespace Workbench {

    /**
     * Wrapper for the base logger class, so we can use it from C# too.
     * Note: due to the way the base class is implemented we only have one log domain (Managed) for
     * all modules.
     */
  public
    enum class LogLevel {
      Disabled = base::Logger::LogLevel::Disabled,
      Error = base::Logger::LogLevel::Error,
      Warning = base::Logger::LogLevel::Warning,
      Info = base::Logger::LogLevel::Info,
      Debug = base::Logger::LogLevel::Debug,
      Debug2 = base::Logger::LogLevel::Debug2,
      Debug3 = base::Logger::LogLevel::Debug3
    };

  public
    ref class Logger {
    public:
      static void InitLogger(System::String ^ path);
      static void EnableLogLevel(LogLevel level);

      static void LogError(System::String ^ domain, System::String ^ message);
      static void LogWarning(System::String ^ domain, System::String ^ message);
      static void LogInfo(System::String ^ domain, System::String ^ message);
      static void LogDebug(System::String ^ domain, int verbosity, System::String ^ message);

      static property System::String ^ ActiveLevel {
        System::String ^ get();
        void set(System::String ^ value);
      }
    };

  } // namespace Workbench
} // namespace MySQL
