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

#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <vector>

#include <glib/gstdio.h>

#include "base/c++helpers.h"

#include "base/log.h"
#include "base/wb_memory.h"
#include "base/file_utilities.h"
#include "base/file_functions.h" // TODO: these two file libs should really be only one.
#include "base/string_utilities.h"

using namespace base;

static const char* LevelText[] = {"", "ERR", "WRN", "INF", "DB1", "DB2", "DB3"};
/*static*/ const std::string Logger::_logLevelNames[] = {"none",   "error",  "warning", "info",
                                                         "debug1", "debug2", "debug3"};
/*static*/ bool Logger::_logLevelSpecifiedByUser = false;

//--------------------------------------------------------------------------------------------------

struct Logger::LoggerImpl {
  LoggerImpl() {
    // Default values for all available log levels.
    _levels[enumIndex(Logger::LogLevel::Disabled)] = false; // Disable None level.
    _levels[enumIndex(Logger::LogLevel::Error)] = true;
    _levels[enumIndex(Logger::LogLevel::Warning)] = true;
    _levels[enumIndex(Logger::LogLevel::Info)] = true; //  Includes all g_message calls.
#if !defined(DEBUG) && !defined(_DEBUG)
    _levels[enumIndex(Logger::LogLevel::Debug)] = false;  // General debug messages.
    _levels[enumIndex(Logger::LogLevel::Debug2)] = false; // Verbose debug messages.
#else
    _levels[enumIndex(Logger::LogLevel::Debug)] = true;
    _levels[enumIndex(Logger::LogLevel::Debug2)] = true;
#endif
    _levels[enumIndex(Logger::LogLevel::Debug3)] = false; // Really chatty, should be switched on only on demand.
  }

  bool level_is_enabled(const Logger::LogLevel level) const {
    return _levels[enumIndex(level)];
  }

  std::string _dir;
  std::string _filename;

  bool _levels[Logger::logLevelCount];
  bool _new_line_pending; // Set to true when the last logged entry ended with a new line.
  bool _std_err_log;
};

Logger::LoggerImpl* Logger::_impl = nullptr;

//--------------------------------------------------------------------------------------------------

std::string Logger::log_filename() {
  return _impl ? _impl->_filename : "";
}

//--------------------------------------------------------------------------------------------------

std::string Logger::log_dir() {
  return _impl ? _impl->_dir : "";
}

//--------------------------------------------------------------------------------------------------

Logger::Logger(const bool stderr_log, const std::string& target_file) {
  if (!_impl)
    _impl = new Logger::LoggerImpl();

  _impl->_std_err_log = stderr_log;

  if (!target_file.empty()) {
    _impl->_filename = target_file;

    FILE_scope_ptr fp = base_fopen(_impl->_filename.c_str(), "w");
  }
}

//--------------------------------------------------------------------------------------------------

Logger::Logger(const std::string& dir, const bool stderr_log, const std::string& file_name, int limit) {
  std::vector<std::string> filenames;

  // Creates the file names array
  filenames.push_back(strfmt("%s.log", file_name.data()));
  for (int index = 1; index < limit; index++)
    filenames.push_back(strfmt("%s.%d.log", file_name.data(), index));

  if (!_impl)
    _impl = new Logger::LoggerImpl();

  _impl->_std_err_log = stderr_log;

  _impl->_new_line_pending = true;
  if (!dir.empty() && !file_name.empty()) {
    _impl->_dir = base::joinPath(dir.c_str(), "log", "");
    _impl->_filename = base::joinPath(_impl->_dir.c_str(), filenames[0].c_str(), "");
    try {
      create_directory(_impl->_dir, 0700, true);
    } catch (const file_error& e) {
      // We need to catch the exception here, otherwise WB will be aborted
      fprintf(stderr, "Exception in logger: %s\n", e.what());
    }

    // Rotate log files: wb.log -> wb.1.log, wb.1.log -> wb.2.log, ...
    for (int i = limit - 1; i > 0; --i) {
      try {
        std::string filename = base::joinPath(_impl->_dir.c_str(), filenames[i].c_str(), "");
        if (file_exists(filename))
          remove(filename);

        std::string filename2 = base::joinPath(_impl->_dir.c_str(), filenames[i - 1].c_str(), "");
        if (file_exists(filename2))
          rename(filename2, filename);
      } catch (...) {
        // we do not care for rename exceptions here!
      }
    }
    // truncate log file we do not need gigabytes of logs
    FILE_scope_ptr fp = base_fopen(_impl->_filename.c_str(), "w");
  }
}

//--------------------------------------------------------------------------------------------------

void Logger::enable_level(const LogLevel level) {
  if (enumIndex(level) < logLevelCount)
    _impl->_levels[enumIndex(level)] = true;
}

//--------------------------------------------------------------------------------------------------

void Logger::disable_level(const LogLevel level) {
  if (enumIndex(level) < logLevelCount)
    _impl->_levels[enumIndex(level)] = false;
}

//--------------------------------------------------------------------------------------------------

void local_free(char* d) {
  g_free(d);
}

//--------------------------------------------------------------------------------------------------

/**
 * Logs the given text with the given domain to the current log file.
 * Note: it should be pretty safe to use utf-8 encoded text too here, though avoid log messages
 * which are several thousands of chars long.
 */
void Logger::logv(LogLevel level, const char* const domain, const char* format, va_list args) {
  scope_ptr<char, local_free> buffer(g_strdup_vprintf(format, args));

  // Print to stderr if no logger is created (yet).
  if (!_impl) {
    fprintf(stderr, "%s", buffer.get());
    fflush(stderr);
    return;
  }

  const time_t t = time(NULL);
  struct tm tm;
#ifdef _MSC_VER
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif

  FILE_scope_ptr fp = _impl->_filename.empty() ? NULL : base_fopen(_impl->_filename.c_str(), "a");

  if (fp) {
    if (_impl->_new_line_pending)
      fprintf(fp, "%02u:%02u:%02u [%3s][%15s]: ", tm.tm_hour, tm.tm_min, tm.tm_sec, LevelText[enumIndex(level)],
              domain);
    fwrite(buffer, 1, strlen(buffer.get()), fp);
  }

  // No explicit newline here. If messages are composed (e.g. python errors)
  // they get split over several lines and lose all their formatting.
  // Additionally for messages with implicit newline (which are most) we get many empty lines.
  if (_impl->_std_err_log) {
#if defined(_MSC_VER)
    HANDLE hConsole = 0;
    WORD wOldColorAttrs;
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    if ((level == LogLevel::Error) || (level == LogLevel::Warning)) {
      hConsole = GetStdHandle(STD_ERROR_HANDLE);
      GetConsoleScreenBufferInfo(hConsole, &csbiInfo);
      wOldColorAttrs = csbiInfo.wAttributes;
      SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
      if (level == LogLevel::Error)
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
      else if (level == LogLevel::Warning)
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);
    }
#elif !defined(__APPLE__)
    if (level == LogLevel::Error)
      fprintf(stderr, "\e[1;31m");
    else if (level == LogLevel::Warning)
      fprintf(stderr, "\e[1m");
#endif

#ifdef _MSC_VER
    if (_impl->_new_line_pending) {
      char* tmp = g_strdup_printf("%02u:%02u:%02u [%3s][%15s]: ", tm.tm_hour, tm.tm_min, tm.tm_sec,
                                  LevelText[enumIndex(level)], domain);
      OutputDebugStringA(tmp);
      g_free(tmp);
    }
    // if you want the program to stop when a specific log msg is printed, put a bp in the next line and set condition
    // to log_msg_serial==#
    OutputDebugStringA(buffer.get());
#endif
    // We need the data in stderr even in Windows, so that the output can be read from other tools.
    if (_impl->_new_line_pending)
      fprintf(stderr, "%02u:%02u:%02u [%3s][%15s]: ", tm.tm_hour, tm.tm_min, tm.tm_sec, LevelText[enumIndex(level)],
              domain);

    // If you want the program to stop when a specific log msg is printed, put a bp in the next line
    // and set condition to log_msg_serial==#
    fprintf(stderr, "%s", buffer.get());

#if defined(_MSC_VER)
    if ((level == LogLevel::Error) || (level == LogLevel::Warning))
      SetConsoleTextAttribute(hConsole, wOldColorAttrs);
#elif !defined(__APPLE__)
    if (level == LogLevel::Error || level == LogLevel::Warning)
      fprintf(stderr, "\e[0m");
#endif
  }

  const char ending_char = buffer[strlen(buffer) - 1];
  _impl->_new_line_pending = (ending_char == '\n') || (ending_char == '\r');
}

//--------------------------------------------------------------------------------------------------

void Logger::log(const Logger::LogLevel level, const char* const domain, const char* format, ...) {
  if (_impl->level_is_enabled(level)) {
    va_list args;
    va_start(args, format);
    logv(level, domain, format, args);
    va_end(args);
  }
}

//--------------------------------------------------------------------------------------------------

void Logger::log_exc(const LogLevel level, const char* const domain, const char* msg, const std::exception& exc) {
  log(level, domain, "%s: Exception: %s\n", msg, exc.what());
}

//--------------------------------------------------------------------------------------------------

void Logger::log_throw(const LogLevel level, const char* const domain, const char* format, ...) {
  if (_impl->level_is_enabled(level)) {
    va_list args;
    va_start(args, format);
    logv(level, domain, format, args);
    va_end(args);

    throw std::logic_error("");
  }
}

//--------------------------------------------------------------------------------------------------

std::string Logger::get_state() {
  std::string state = "";
  if (_impl) {
    for (std::size_t i = 0; i < logLevelCount; ++i) {
      state += _impl->level_is_enabled((Logger::LogLevel)i) ? "1" : "0";
    }
  }
  return state;
}

//--------------------------------------------------------------------------------------------------

void Logger::set_state(const std::string& state) {
  if (_impl && state.length() >= logLevelCount) {
    for (std::size_t i = 0; i < logLevelCount; ++i) {
      const char level = state[i];
      if (level == '1') {
        Logger::enable_level((Logger::LogLevel)i);
      } else if (level == '0') {
        Logger::disable_level((Logger::LogLevel)i);
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the most chatty log level which is currently active.
 */
std::string Logger::active_level() {
  if (_impl == NULL)
    return "none";

  int i;
  for (i = logLevelCount - 1; i >= 0; i--)
    if (_impl->level_is_enabled((LogLevel)i))
      break;

  switch (i) {
    case 0:
      return "none";
    case 1:
      return "error";
    case 2:
      return "warning";
    case 3:
      return "info";
    case 4:
      return "debug1";
    case 5:
      return "debug2";
    case 6:
      return "debug3";
  }

  return "none";
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to set a log level, which implicitly enables all less-chatty levels too.
 * E.g. setting "info" not only enables info, but also warning and error etc.
 */
bool Logger::active_level(const std::string& value) {
  if (_impl == NULL)
    return false;

  int levelIndex = logLevelCount - 1;
  while (levelIndex >= 0 && !same_string(value, _logLevelNames[levelIndex]))
    levelIndex--;

  if (levelIndex < 0)
    return false; // Invalid value given. Ignore it.

  for (int i = 0; i < int(logLevelCount); ++i) {
    if (levelIndex >= i)
      enable_level((LogLevel)i);
    else
      disable_level((LogLevel)i);
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
void Logger::log_to_stderr(bool value) {
  _impl->_std_err_log = value;
}
