/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MSC_VER
#include <stdarg.h>
#endif

#include "grt_manager.h"
#include "grt_shell.h"
#include "base/file_functions.h"
#include "base/file_utilities.h"

using namespace grt;
using namespace bec;

#define SNIPPETS_FILENAME "shell_snippets.txt"
#define HISTORY_FILENAME "shell_history.txt"
#define BOOKMARKS_FILENAME "shell_bookmarks.txt"

//----------------------------------------------------------------------------------------------------------------------

ShellBE::ShellBE(const GRTDispatcher::Ref dispatcher) : _dispatcher(dispatcher) {
  _shell = 0;

  _save_history_size = 0;
  _skip_history = 0;

  _history_ptr = _history.begin();
}

//----------------------------------------------------------------------------------------------------------------------

ShellBE::~ShellBE() {
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::set_save_directory(const std::string &path) {
  _savedata_dir = path;
}

//----------------------------------------------------------------------------------------------------------------------

grt::ValueRef ShellBE::get_shell_variable(const std::string &varname) {
  return _shell->get_global_var(varname);
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::set_saves_history(int size) {
  _save_history_size = size;
  if (size <= 0) {
    _history.clear();
    _history_ptr = _history.end();
  } else {
    while ((int)_history.size() > size)
      _history.pop_back();
    _history_ptr = _history.end();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::run_script_file(const std::string &path) {
  grt::ModuleLoader *loader = grt::GRT::get()->get_module_loader_for_file(path);
  if (!loader)
    throw std::runtime_error("Unsupported script file " + path);

  if (!loader->run_script_file(path))
    throw std::runtime_error("An error occurred while executing the script " + path);
}

//----------------------------------------------------------------------------------------------------------------------

bool ShellBE::run_script(const std::string &script, const std::string &language) {
  grt::ModuleLoader *loader = grt::GRT::get()->get_module_loader(language);
  if (loader)
    return loader->run_script(script);
  throw std::runtime_error("Language " + language + " is not supported or enabled");
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::process_line_async(const std::string &line) {
  GRTShellTask::Ref task = GRTShellTask::create_task("User shell command", _dispatcher, line);

  task->signal_message().connect(std::bind(&ShellBE::handle_msg, this, std::placeholders::_1));
  task->set_handle_messages_from_thread();

  task->signal_finished().connect(
    std::bind(&ShellBE::shell_finished_cb, this, std::placeholders::_1, std::placeholders::_2, line));

  _dispatcher->execute_now(task);
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::shell_finished_cb(ShellCommand result, const std::string &prompt, const std::string &line) {
  if (result == ShellCommandExit) {
    bec::GRTManager::get()->terminate();
    _current_statement.clear();
  } else if (result == ShellCommandUnknown) {
    if (_current_statement.empty())
      _current_statement = line;
    else
      _current_statement += "\n" + line;
  } else if (result == ShellCommandStatement) {
    if (_current_statement.empty())
      _current_statement = line;
    else
      _current_statement += "\n" + line;

    if (_save_history_size > 0 && _current_statement != "\n" && _current_statement != "")
      save_history_line(_current_statement);

    _current_statement.clear();
  } else {
    if (_current_statement.empty())
      _current_statement = line;
    else
      _current_statement += "\n" + line;

    if (_save_history_size > 0 && _current_statement != "\n" && _current_statement != "")
      save_history_line(_current_statement);

    _current_statement.clear();
  }

  if (_ready_slot)
    _ready_slot(prompt);
}

//----------------------------------------------------------------------------------------------------------------------

bool ShellBE::setup(const std::string &lang) {
  if (!grt::GRT::get()->init_shell(lang))
    return false;

  _shell = grt::GRT::get()->get_shell();
  grt::GRT::get()->get_shell()->set_disable_quit(true);

  _shell->print_welcome();

  start();

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::start() {
  _skip_history = 0;
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::save_history_line(const std::string &line) {
  if (line.empty())
    return;
  if (_skip_history > 0) {
    --_skip_history;
    return;
  }

  // remove empty cmd if it is on top of the stack
  if (!_history.empty() && (*_history.begin()).empty())
    _history.pop_front();

  if (line[line.size() - 1] == '\n')
    _history.push_front(line.substr(0, line.size() - 1));
  else
    _history.push_front(line);
  if ((int)_history.size() > _save_history_size)
    _history.pop_back();
  _history_ptr = _history.begin();
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::clear_history() {
  _history.clear();
  _history_ptr = _history.end();
}

//----------------------------------------------------------------------------------------------------------------------

bool ShellBE::previous_history_line(const std::string &current_line, std::string &line) {
  if (_history_ptr == _history.end())
    return false;

  bool saved_current = !(_history_ptr == _history.begin());

  // save the current line to the history
  if (!current_line.empty() && !saved_current) {
    saved_current = true;
    save_history_line(current_line);
  }
  std::list<std::string>::const_iterator tmp = _history_ptr;
  ++tmp;
  if (tmp == _history.end())
    return false;

  if (saved_current)
    line = *++_history_ptr;
  else
    line = *_history_ptr++;

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool ShellBE::next_history_line(std::string &line) {
  if (_history_ptr != _history.begin()) {
    --_history_ptr;
    line = *_history_ptr;
    if (_history_ptr == _history.begin()) {
      _history.pop_front();
      _history_ptr = _history.begin();
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::reset_history_position() {
  _history_ptr = _history.begin();
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::set_ready_handler(const std::function<void(const std::string &)> &slot) {
  _ready_slot = slot;
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::set_output_handler(const std::function<void(const std::string &)> &slot) {
  _output_slot = slot;
  if (_output_slot)
    flush_shell_output(); // Write out any pending text we might have.
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::handle_msg(const Message &msg) {
  // the default msg handler outputs everything to shell
  switch (msg.type) {
    case grt::ErrorMsg:
      write_line("ERROR: " + msg.text + "\n");
      break;
    case grt::WarningMsg:
      write_line("WARNING: " + msg.text + "\n");
      break;
    case grt::InfoMsg:
      write_line("INFO: " + msg.text + "\n");
      break;
    case grt::ProgressMsg:
      write_line("Progress: " + msg.text + "\n");
      break;
    case grt::OutputMsg:
      write(msg.text);
      break;
    default:
      write_line("Message: " + msg.text + "\n");
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::writef(const char *fmt, ...) {
  va_list ap;
  char *tmp;
  std::string line;

  va_start(ap, fmt);
  tmp = g_strdup_vprintf(fmt, ap);
  line = tmp;
  g_free(tmp);
  va_end(ap);

  // Cache the text if there is no output slot set yet (usually at app start).
  // Flush this queue when we have an output slot and are running in the main thread currently.
  if (bec::GRTManager::get()->is_threaded()) {
    {
      base::MutexLock lock(_text_queue_mutex);
      _text_queue.push_back(line);
    }

    // if we're in the main thread, flush the message queue
    if (bec::GRTManager::get()->in_main_thread() && _output_slot)
      flush_shell_output();
  } else {
    // If not threaded print directly (given we have an output slot).
    // The text queue is flushed when an output slot is set.
    if (_output_slot)
      _output_slot(line);
    else {
      base::MutexLock lock(_text_queue_mutex);
      _text_queue.push_back(line);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::write_line(const std::string &line) {
  writef("%s\n", line.c_str());
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::write(const std::string &text) {
  writef("%s", text.c_str());
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::flush_shell_output() {
  if (!_output_slot)
    return;

  std::string line;

  while (true) {
    {
      base::MutexLock lock(_text_queue_mutex);
      if (_text_queue.empty())
        break;
      line = _text_queue.front();
      _text_queue.pop_front();
    }
    _output_slot(line);
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> ShellBE::complete_line(const std::string &line, std::string &nprefix) {
  return _shell->complete_line(line, nprefix);
}

//----------------------------------------------------------------------------------------------------------------------

std::string ShellBE::get_snippet_data() {
  std::string path = base::makePath(_savedata_dir, SNIPPETS_FILENAME);
  gchar *contents;
  gsize length;

  if (g_file_get_contents(path.c_str(), &contents, &length, NULL)) {
    std::string text = std::string(contents, contents + length);
    g_free(contents);
    return text;
  }
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::set_snippet_data(const std::string &data) {
  std::string path = base::makePath(_savedata_dir, SNIPPETS_FILENAME);

  // Make sure path exists, if not create it with privileges 755
  g_mkdir_with_parents(_savedata_dir.c_str(), 0755);

  if (!g_file_set_contents(path.c_str(), data.c_str(), (gssize)data.size(), NULL)) {
    throw std::runtime_error("Could not save file " + path);
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> ShellBE::get_grt_tree_bookmarks() {
  return _grt_tree_bookmarks;
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::add_grt_tree_bookmark(const std::string &path) {
  if (std::find(_grt_tree_bookmarks.begin(), _grt_tree_bookmarks.end(), path) == _grt_tree_bookmarks.end())
    _grt_tree_bookmarks.push_back(path);
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::delete_grt_tree_bookmark(const std::string &path) {
  std::vector<std::string>::iterator iter = std::find(_grt_tree_bookmarks.begin(), _grt_tree_bookmarks.end(), path);
  if (iter != _grt_tree_bookmarks.end())
    _grt_tree_bookmarks.erase(iter);
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::store_state() {
  // Make sure path exists, if not create it with privileges 744
  g_mkdir_with_parents(_savedata_dir.c_str(), 0700);

  {
    std::string path = base::makePath(_savedata_dir, HISTORY_FILENAME);

    FILE *f = base_fopen(path.c_str(), "w+");
    if (!f)
      throw std::runtime_error("Could not save file " + path);

    for (std::list<std::string>::const_iterator i = _history.begin(); i != _history.end(); ++i) {
      char **lines = g_strsplit(i->c_str(), "\n", 0);
      for (int j = 0; lines[j]; j++)
        fprintf(f, " %s\n", lines[j]);
      g_strfreev(lines);
      fprintf(f, "\n");
    }
    fclose(f);
  }

  {
    std::string path = base::makePath(_savedata_dir, BOOKMARKS_FILENAME);

    FILE *f = base_fopen(path.c_str(), "w+");
    if (!f)
      throw std::runtime_error("Could not save file " + path);

    for (std::vector<std::string>::const_iterator i = _grt_tree_bookmarks.begin(); i != _grt_tree_bookmarks.end();
         ++i) {
      fprintf(f, "%s\n", i->c_str());
    }
    fclose(f);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ShellBE::restore_state() {
  {
    char line[1024];
    std::string path = base::makePath(_savedata_dir, HISTORY_FILENAME);
    std::string command;
    FILE *f = base_fopen(path.c_str(), "r");
    if (f) {
      _history.clear();

      while (!feof(f) && fgets(line, sizeof(line), f)) {
        if (*line == ' ')
          command += line + 1;
        else {
          while (!command.empty() && (command[command.size() - 1] == ' ' || command[command.size() - 1] == '\n'))
            command = command.substr(0, command.size() - 1);

          if (!command.empty())
            _history.push_back(command);
          command = "";
        }
      }
      fclose(f);
    }
    _history_ptr = _history.begin();
  }

  {
    char line[1024];
    std::string path = base::makePath(_savedata_dir, BOOKMARKS_FILENAME);
    FILE *f = base_fopen(path.c_str(), "r");
    if (f) {
      bool cleared = false;

      while (!feof(f) && fgets(line, sizeof(line), f)) {
        char *p = strchr(line, '\n');
        if (p)
          *p = 0;
        if (strlen(line) > 0 && line[0] == '/') {
          if (!cleared)
            _grt_tree_bookmarks.clear();
          cleared = true;
          _grt_tree_bookmarks.push_back(g_strstrip(line));
        }
      }
      fclose(f);
    } else {
      _grt_tree_bookmarks.push_back("/");
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
