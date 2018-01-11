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

#include "LogWrapper.h"

#include "ConvUtils.h"

using namespace System;

using namespace base;

namespace MySQL {
  namespace Workbench {

    //--------------------------------------------------------------------------------------------------

    void Logger::InitLogger(String ^ path) {
      // To initialize the logger simply create an instance. That's a bit weird if you consider that
      // it is a singleton, but that's how it is implemented.
      base::Logger logger = base::Logger(NativeToCppString(path));
    }

    //--------------------------------------------------------------------------------------------------

    void Logger::EnableLogLevel(LogLevel level) {
      base::Logger::enable_level((base::Logger::LogLevel)level);
    }

    //--------------------------------------------------------------------------------------------------

    void Logger::LogError(String ^ domain, String ^ message) {
      base::Logger::log(base::Logger::LogLevel::Error, NativeToCppStringRaw(domain).c_str(), "%s",
                        NativeToCppString(message).c_str());
    }

    //--------------------------------------------------------------------------------------------------

    void Logger::LogWarning(String ^ domain, String ^ message) {
      base::Logger::log(base::Logger::LogLevel::Warning, NativeToCppStringRaw(domain).c_str(), "%s",
                        NativeToCppString(message).c_str());
    }

    //--------------------------------------------------------------------------------------------------

    void Logger::LogInfo(String ^ domain, String ^ message) {
      base::Logger::log(base::Logger::LogLevel::Info, NativeToCppStringRaw(domain).c_str(), "%s",
                        NativeToCppString(message).c_str());
    }

    //--------------------------------------------------------------------------------------------------

    void Logger::LogDebug(String ^ domain, int verbosity, String ^ message) {
      switch (verbosity) {
        case 1:
          base::Logger::log(base::Logger::LogLevel::Debug, NativeToCppStringRaw(domain).c_str(), "%s",
                            NativeToCppString(message).c_str());
          break;
        case 2:
          base::Logger::log(base::Logger::LogLevel::Debug2, NativeToCppStringRaw(domain).c_str(), "%s",
                            NativeToCppString(message).c_str());
          break;
        case 3:
          base::Logger::log(base::Logger::LogLevel::Debug3, NativeToCppStringRaw(domain).c_str(), "%s",
                            NativeToCppString(message).c_str());
          break;
      }
    }

    //--------------------------------------------------------------------------------------------------

    String ^ Logger::ActiveLevel::get() {
      return CppStringToNative(base::Logger::active_level());
    }

    //--------------------------------------------------------------------------------------------------

    void Logger::ActiveLevel::set(String ^ value) {
      base::Logger::active_level(NativeToCppString(value));
    }

    //--------------------------------------------------------------------------------------------------

  } // namespace Workbench
} // namespace MySQL
