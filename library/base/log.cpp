/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef HAVE_PRECOMPILED_HEADERS
  #include <string>
  #include <stdio.h>
  #include <stdarg.h>
  #include <time.h>
  #include <string.h>
  #include <vector>

  #include <glib/gstdio.h>
#endif

#include "base/log.h"
#include "base/wb_memory.h"
#include "base/file_utilities.h"
#include "base/file_functions.h" // TODO: these two file libs should really be only one.
#include "base/string_utilities.h"

using namespace base;

static const char* LevelText[] = {"", "ERR", "WRN", "INF", "DB1", "DB2", "DB3"};

//--------------------------------------------------------------------------------------------------

struct Logger::LoggerImpl
{
  LoggerImpl()
  {
    // Default values for all available log levels.
    _levels[LogNone] = false; // Disable None level.
    _levels[LogError] = true;
    _levels[LogWarning] = true;
    _levels[LogInfo] = true; //  Includes all g_message calls.
#if !defined(DEBUG) && !defined(_DEBUG)
    _levels[LogDebug]  = false; // General debug messages.
    _levels[LogDebug2] = false; // Verbose debug messages.
#else
    _levels[LogDebug]  = true;
    _levels[LogDebug2] = true;
#endif
    _levels[LogDebug3] = false; // Really chatty, should be switched on only on demand.
  }

  bool level_is_enabled(const Logger::LogLevel level) const
  {
    return _levels[level];
  }

  std::string _filename;
  bool        _levels[Logger::NumOfLevels + 1];
  std::string _dir;
  bool        _new_line_pending; // Set to true when the last logged entry ended with a new line.
  bool        _std_err_log;
};

Logger::LoggerImpl*  Logger::_impl = 0;

//--------------------------------------------------------------------------------------------------

std::string Logger::log_filename()
{
  return _impl ? _impl->_filename : "";
}

//--------------------------------------------------------------------------------------------------

std::string Logger::log_dir()
{
  return _impl ? _impl->_dir : "";
}

//--------------------------------------------------------------------------------------------------

Logger::Logger(const bool stderr_log, const std::string& target_file)
{
  if (!_impl)
    _impl = new Logger::LoggerImpl();

  _impl->_std_err_log = stderr_log;

  if (!target_file.empty())
  {
    _impl->_filename = target_file;

    FILE_scope_ptr fp = base_fopen(_impl->_filename.c_str(), "w");
  }
}

//--------------------------------------------------------------------------------------------------

Logger::Logger(const std::string& dir, const bool stderr_log, const std::string& file_name, int limit)
{
  std::vector<std::string>filenames;

  // Creates the file names array
  filenames.push_back(strfmt("%s.log", file_name.data()));
  for(int index = 1; index < limit; index++)
    filenames.push_back(strfmt("%s.%d.log", file_name.data(), index));

  if (!_impl)
    _impl = new Logger::LoggerImpl();

  _impl->_std_err_log = stderr_log;

  _impl->_new_line_pending = true;
  if (!dir.empty() && !file_name.empty())
  {
    _impl->_dir = base::join_path(dir.c_str(), "log", "");
    _impl->_filename = base::join_path(_impl->_dir.c_str(), filenames[0].c_str(), "");
    try
    {
      create_directory(_impl->_dir, 0700, true);
    }
    catch (const file_error& e)
    {
      // We need to catch the exception here, otherwise WB will be aborted
      fprintf(stderr, "Exception in logger: %s\n", e.what());
    }

    // Rotate log files: wb.log -> wb.1.log, wb.1.log -> wb.2.log, ...
    for (int i = limit-1; i > 0; --i)
    {
      try
      {
        if (file_exists((_impl->_dir + filenames[i])))
          remove(_impl->_dir + filenames[i]);
        if (file_exists((_impl->_dir + filenames[i-1])))
          rename((_impl->_dir + filenames[i-1]), (_impl->_dir + filenames[i]));
      }
      catch (...)
      {
        // we do not care for rename exceptions here!
      }
    }
    // truncate log file we do not need gigabytes of logs
    FILE_scope_ptr fp = base_fopen(_impl->_filename.c_str(), "w");
  }
}

//--------------------------------------------------------------------------------------------------

void Logger::enable_level(const LogLevel level)
{
  if (level <= NumOfLevels)
    _impl->_levels[level] = true;
}

//--------------------------------------------------------------------------------------------------

void Logger::disable_level(const LogLevel level)
{

  if (level <= NumOfLevels)
    _impl->_levels[level] = false;
}

//--------------------------------------------------------------------------------------------------

void local_free(char *d)
{
  g_free(d);
}

//--------------------------------------------------------------------------------------------------

/**
 * Logs the given text with the given domain to the current log file.
 * Note: it should be pretty safe to use utf-8 encoded text too here, though avoid log messages
 * which are several thousands of chars long.
 */
void Logger::logv(LogLevel level, const char* const domain, const char* format, va_list args)
{
  scope_ptr<char, local_free> buffer(g_strdup_vprintf(format, args));

  // Print to stderr if no logger is created (yet).
  if (!_impl)
  {
    fprintf(stderr, "%s", buffer.get());
    fflush(stderr);
    return;
  }

  const time_t t = time(NULL);
  struct tm tm;
#ifdef _WIN32
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif

  FILE_scope_ptr fp = _impl->_filename.empty() ? NULL : base_fopen(_impl->_filename.c_str(), "a");

  if (fp)
  {
    if (_impl->_new_line_pending)
      fprintf(fp, "%02u:%02u:%02u [%3s][%15s]: ", tm.tm_hour, tm.tm_min, tm.tm_sec, LevelText[level], domain);
    fwrite(buffer, 1, strlen(buffer.get()), fp);
  }

  // No explicit newline here. If messages are composed (e.g. python errors)
  // they get split over several lines and lose all their formatting.
  // Additionally for messages with implicit newline (which are most) we get many empty lines.
  if (_impl->_std_err_log)
  {
# if defined(_WIN32)
    HANDLE  hConsole = 0;
    WORD wOldColorAttrs;
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    if ((level == LogError) || (level == LogWarning))
    {
      hConsole = GetStdHandle(STD_ERROR_HANDLE);
      GetConsoleScreenBufferInfo(hConsole, &csbiInfo);
      wOldColorAttrs = csbiInfo.wAttributes;
      SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
      if (level == LogError)
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
      else if (level == LogWarning)
        SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);
    }
# elif !defined(__APPLE__)
    if (level == LogError)
      fprintf(stderr, "\e[1;31m");
    else if (level == LogWarning)
      fprintf(stderr, "\e[1m");
# endif

#ifdef _WIN32
    if (_impl->_new_line_pending)
    {
      char *tmp = g_strdup_printf("%02u:%02u:%02u [%3s][%15s]: ", tm.tm_hour, tm.tm_min, tm.tm_sec, LevelText[level], domain);
      OutputDebugStringA(tmp);
      g_free(tmp);
    }
    // if you want the program to stop when a specific log msg is printed, put a bp in the next line and set condition to log_msg_serial==#
    OutputDebugStringA(buffer.get());
#endif
    // we need the data in stderr even in Windows, so that the output can be read from copytables
    if (_impl->_new_line_pending)
      fprintf(stderr, "%02u:%02u:%02u [%3s][%15s]: ", tm.tm_hour, tm.tm_min, tm.tm_sec, LevelText[level], domain);
      
    // if you want the program to stop when a specific log msg is printed, put a bp in the next line and set condition to log_msg_serial==#
    fprintf(stderr, "%s", buffer.get());

# if defined(_WIN32)
    if ((level == LogError) || (level == LogWarning))
      SetConsoleTextAttribute(hConsole, wOldColorAttrs);
# elif !defined(__APPLE__)
    if (level == LogError || level == LogWarning)
      fprintf(stderr, "\e[0m");
# endif
  }

  const char ending_char = buffer[strlen(buffer)- 1];
  _impl->_new_line_pending = (ending_char == '\n') || (ending_char == '\r');
}

//--------------------------------------------------------------------------------------------------

void Logger::log(const Logger::LogLevel level, const char* const domain, const char* format, ...)
{
  if (_impl->level_is_enabled(level))
  {
    va_list args;
    va_start(args, format);
    logv(level, domain, format, args);
    va_end(args);
  }
}

//--------------------------------------------------------------------------------------------------

void Logger::log_exc(const LogLevel level, const char* const domain, const char* msg, const std::exception &exc)
{
  log(level, domain, "%s: Exception: %s\n", msg, exc.what());
}

//--------------------------------------------------------------------------------------------------

void Logger::log_throw(const LogLevel level, const char* const domain, const char* format, ...)
{
  if (_impl->level_is_enabled(level))
  {
    va_list args;
    va_start(args, format);
    logv(level, domain, format, args);
    va_end(args);

    throw std::logic_error("");
  }
}

//--------------------------------------------------------------------------------------------------

std::string Logger::get_state()
{
  std::string state = "";
  if (_impl)
  {
    for (int i = 0; i < Logger::NumOfLevels + 1; ++i)
    {
      state += _impl->level_is_enabled((Logger::LogLevel)i) ? "1" : "0";
    }
  }
  return state;
}

//--------------------------------------------------------------------------------------------------

void Logger::set_state(const std::string& state)
{
  if (_impl && state.length() >= Logger::NumOfLevels)
  {
    for (int i = 0; i < Logger::NumOfLevels + 1; ++i)
    {
      const char level = state[i];
      if (level == '1')
      {
        Logger::enable_level((Logger::LogLevel)i);
      }
      else if (level == '0')
      {
        Logger::disable_level((Logger::LogLevel)i);
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the most chatty log level which is currently active.
 */
std::string Logger::active_level()
{
  if (_impl == NULL)
    return "none";

  int i;
  for (i = NumOfLevels; i > 0; i--)
    if (_impl->level_is_enabled((LogLevel) i))
      break;

  switch (i)
  {
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
bool Logger::active_level(const std::string& value)
{
  if (_impl == NULL)
    return false;

  std::string levels[] = { "none", "error", "warning", "info", "debug1", "debug2", "debug3" };

  int levelIndex = NumOfLevels;
  while (levelIndex >= 0 && !same_string(value, levels[levelIndex]))
    levelIndex--;

  if (levelIndex < 0)
    return false; // Invalid value given. Ignore it.

  for (int i = 1; i < ((int)NumOfLevels + 1); ++i)
  {
    if (levelIndex >= i)
      enable_level((LogLevel)i);
    else
      disable_level((LogLevel)i);
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
void Logger::log_to_stderr(bool value)
{
    _impl->_std_err_log = value;
}
