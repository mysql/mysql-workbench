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

// This default log domain is a convenience feature for C/C++ code to allow omitting a domain in
// log_* calls. You can always use base::Logger::log to specify an arbitrary domain.
#define DEFAULT_LOG_DOMAIN(domain) static const char* const default_log_domain = domain;

#if defined(DEBUG) || defined(_DEBUG)
// Same as logError, but throws exception in debug builds.
#define logFatal(...) base::Logger::log_throw(base::Logger::LogLevel::Error, default_log_domain, __VA_ARGS__)
#else
#define logFatal(...) base::Logger::log(base::Logger::LogLevel::Error, default_log_domain, __VA_ARGS__)
#endif

#define logError(...) base::Logger::log(base::Logger::LogLevel::Error, default_log_domain, __VA_ARGS__)
#define logException(msg, exc) base::Logger::log_exc(base::Logger::LogLevel::Error, default_log_domain, msg, exc)
#define logWarning(...) base::Logger::log(base::Logger::LogLevel::Warning, default_log_domain, __VA_ARGS__)
#define logInfo(...) base::Logger::log(base::Logger::LogLevel::Info, default_log_domain, __VA_ARGS__)
#define logDebug(...) base::Logger::log(base::Logger::LogLevel::Debug, default_log_domain, __VA_ARGS__)
#define logDebug2(...) base::Logger::log(base::Logger::LogLevel::Debug2, default_log_domain, __VA_ARGS__)
#define logDebug3(...) base::Logger::log(base::Logger::LogLevel::Debug3, default_log_domain, __VA_ARGS__)

// Predefined domains
#define DOMAIN_BASE "base library"
#define DOMAIN_GRT "grt"
#define DOMAIN_GRT_DIFF "grt_diff"

#define DOMAIN_WQE_BE "WQE backend"
#define DOMAIN_WQE_WRAPPER "WQE managed"
#define DOMAIN_WQE_NATIVE "WQE native"

#define DOMAIN_MFORMS_BE "mforms backend"
#define DOMAIN_MFORMS_WRAPPER "mforms managed"
#define DOMAIN_MFORMS_NET "mforms .net"
#define DOMAIN_MFORMS_COCOA "mforms cocoa"
#define DOMAIN_MFORMS_GTK "mforms gtk"

#define DOMAIN_WB_CONTEXT "WBContext"
#define DOMAIN_WB_CONTEXT_WRAPPER "WBContext managed"
#define DOMAIN_WB_CONTEXT_UI "WBContext UI"
#define DOMAIN_WB_MODULE "WBModule"

#define DOMAIN_SQL_PARSER "SQL parser"

#define DOMAIN_COMMAND_HANDLING "Command"

#define DOMAIN_CANVAS_BE "Canvas backend"
#define DOMAIN_CANVAS_WRAPPER "Canvas managed"
#define DOMAIN_CANVAS_NATIVE "Canvas native"

#include <string>
#include <stdarg.h>
#include "base/common.h"

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#include <glib.h>
#endif

namespace base {
#if defined(DEBUG) || defined(_DEBUG) || defined(ENABLE_DEBUG)
#define DEFAULT_LOG_TO_STDERR 1
#else
#define DEFAULT_LOG_TO_STDERR 0
#endif

  class BASELIBRARY_PUBLIC_FUNC Logger {
  public:
    enum class LogLevel { Disabled, Error, Warning, Info, Debug, Debug2, Debug3, Count };

    static const size_t logLevelCount = static_cast<std::size_t>(LogLevel::Count);

    Logger(const bool stderr_log, const std::string& target_file);
    Logger(const std::string& dir, const bool stderr_log = DEFAULT_LOG_TO_STDERR, const std::string& file_name = "wb",
           int limit = 10); // Later logdir or set of log files can be passed

    static void enable_level(const LogLevel level);
    static void disable_level(const LogLevel level);
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
    static void log(const LogLevel level, const char* const domain, const char* format, ...) G_GNUC_PRINTF(3, 4);
#else
    static void log(const LogLevel level, const char* const domain, const char* format, ...);
#endif
    static void log_throw(const LogLevel level, const char* const domain, const char* format, ...);
    static void log_exc(const LogLevel level, const char* const domain, const char* msg, const std::exception& exc);
    static std::string get_state();
    static void set_state(const std::string& state);
    static std::string log_filename();
    static std::string log_dir();

    static std::string active_level();
    static bool active_level(const std::string& value);

    static void log_to_stderr(bool value);

    static const std::string& logLevelName(std::size_t index) {
      return _logLevelNames[index];
    }

    static bool wasLogLevelSpecifiedByUser() {
      return _logLevelSpecifiedByUser;
    }
    static void setLogLevelSpecifiedByUser() {
      _logLevelSpecifiedByUser = true;
    }

  protected:
    static void logv(const LogLevel level, const char* const domain, const char* format, va_list args);

  private:
    struct LoggerImpl;
    static LoggerImpl* _impl;

    static const std::string _logLevelNames[logLevelCount];
    static bool _logLevelSpecifiedByUser; // false until set to true
  };

} // End of namespace
