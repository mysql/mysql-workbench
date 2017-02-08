/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/grt_shell.h"
#include "grt/grt_dispatcher.h"
#include "grt/grt_manager.h"
#include "wb_helpers.h"

using namespace grt;
using namespace bec;

BEGIN_TEST_DATA_CLASS(be_shell)
public:
GRTDispatcher::Ref dispatcher;

TEST_DATA_CONSTRUCTOR(be_shell) {
  bec::GRTManager::get();
  dispatcher = GRTDispatcher::create_dispatcher(false, true);
}

END_TEST_DATA_CLASS;

TEST_MODULE(be_shell, "grt shell backend");

TEST_FUNCTION(1) { // test history navigation
  bool flag;
  std::string line;

  ShellBE *shell = new ShellBE(dispatcher);
  shell->set_saves_history(10);
  shell->save_history_line("line1");
  flag = shell->previous_history_line("newline", line);
  ensure("previous line", flag);
  ensure_equals("previous line value", line, "line1");

  flag = shell->next_history_line(line);
  ensure("next line", flag);
  ensure_equals("next line value", line, "newline");

  shell->save_history_line("line2");
  shell->save_history_line("line3");

  flag = shell->next_history_line(line);
  ensure("next", !flag);

  flag = shell->previous_history_line("newline", line);
  ensure("previous line", flag);
  ensure_equals("previous line value", line, "line3");

  flag = shell->previous_history_line("line3", line);
  ensure("previous line", flag);
  ensure_equals("previous line value", line, "line2");

  flag = shell->previous_history_line("line2", line);
  ensure("previous line", flag);
  ensure_equals("previous line value", line, "line1");

  flag = shell->previous_history_line("line1", line);
  ensure("previous line", !flag);

  flag = shell->next_history_line(line);
  ensure("next line", flag);
  ensure_equals("next line value", line, "line2");

  flag = shell->next_history_line(line);
  ensure("next line", flag);
  ensure_equals("next line value", line, "line3");

  flag = shell->next_history_line(line);
  ensure("next line", flag);
  ensure_equals("next line value", line, "newline");

  flag = shell->next_history_line(line);
  ensure("previous line", !flag);

  flag = shell->previous_history_line("newline", line);
  ensure("previous line", flag);
  ensure_equals("previous line value", line, "line3");

  delete shell;
}

TEST_FUNCTION(2) {
  bool flag;
  ShellBE *shell = new ShellBE(dispatcher);

  shell->set_saves_history(10);
  shell->set_save_directory(".");

  shell->save_history_line("line1");
  shell->save_history_line("line2");
  shell->save_history_line("line3.1\nline3.2\n\nline3.3");
  shell->save_history_line("line4");
  shell->save_history_line("line5");

  shell->set_snippet_data("hello world\nsnippet line this");
  shell->store_state();

  delete shell;

  shell = new ShellBE(dispatcher);
  shell->set_saves_history(10);
  shell->set_save_directory(".");
  shell->restore_state();

  std::string line;

  flag = shell->previous_history_line("newline", line);
  ensure("get restored line", flag);

  ensure_equals("last line ", line, "line5");

  flag = shell->previous_history_line(line, line);
  ensure("prev after save", flag);
  ensure_equals("last line ", line, "line4");

  flag = shell->previous_history_line(line, line);
  ensure("prev after save", flag);
  ensure_equals("last line ", line, "line3.1\nline3.2\n\nline3.3");

  line = shell->get_snippet_data();
  ensure_equals("snippet", line, "hello world\nsnippet line this");

  delete shell;
}

TEST_FUNCTION(99) {
  dispatcher.reset();
}

END_TESTS
