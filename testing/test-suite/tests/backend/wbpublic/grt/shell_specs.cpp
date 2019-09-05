/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/grt_shell.h"
#include "grt/grt_dispatcher.h"
#include "grt/grt_manager.h"
#include "wb_test_helpers.h"
#include "casmine.h"

using namespace grt;
using namespace bec;
using namespace casmine;

extern void register_all_metaclasses();

namespace {

$ModuleEnvironment() {};

$TestData {
  GRTDispatcher::Ref dispatcher;
};
  
$describe("Grt Shell Backend") {
  $beforeAll([&]() {
    register_all_metaclasses();
    grt::GRT::get()->scan_metaclasses_in("../../res/grt/");
    grt::GRT::get()->end_loading_metaclasses();
    data->dispatcher = GRTDispatcher::create_dispatcher(false, true);
  });

  $afterAll([&]() {
    data->dispatcher->shutdown();
    data->dispatcher.reset();
    WorkbenchTester::reinitGRT();
  });

  $it("Test History Navigation", [&]() {
    bool flag;
    std::string line;

    ShellBE *shell = new ShellBE(data->dispatcher);
    shell->set_saves_history(10);
    shell->save_history_line("line1");
    flag = shell->previous_history_line("newline", line);
    $expect(flag).toBe(true, "previous line");
    $expect(line).toBe("line1", "previous line value");


    flag = shell->next_history_line(line);
    $expect(flag).toBe(true, "next line");
    $expect(line).toBe("newline", "next line value");

    shell->save_history_line("line2");
    shell->save_history_line("line3");

    flag = shell->next_history_line(line);
    $expect(flag).toBe(false, "next");

    flag = shell->previous_history_line("newline", line);
    $expect(flag).toBe(true, "previous line");
    $expect(line).toBe("line3", "previous line value");

    flag = shell->previous_history_line("line3", line);
    $expect(flag).toBe(true, "previous line");
    $expect(line).toBe("line2", "previous line value");


    flag = shell->previous_history_line("line2", line);
    $expect(flag).toBe(true, "previous line");
    $expect(line).toBe("line1", "previous line value");

    flag = shell->previous_history_line("line1", line);
    $expect(flag).toBe(false, "prevous line");

    flag = shell->next_history_line(line);
    $expect(flag).toBe(true, "next line");
    $expect(line).toBe("line2", "next line value");

    flag = shell->next_history_line(line);
    $expect(flag).toBe(true, "next line");
    $expect(line).toBe("line3", "next line value");

    flag = shell->next_history_line(line);
    $expect(flag).toBe(true);
    $expect(line).toBe("newline", "next line value");

    flag = shell->next_history_line(line);
    $expect(flag).toBe(false, "previous line");

    flag = shell->previous_history_line("newline", line);
    $expect(flag).toBe(true, "previous line");
    $expect(line).toBe("line3", "previous line value");

    delete shell;
  });

  $it("Additional History Lines Test", [&]() {
    bool flag;
    ShellBE *shell = new ShellBE(data->dispatcher);

    shell->set_saves_history(10);
    shell->set_save_directory(CasmineContext::get()->outputDir());

    shell->save_history_line("line1");
    shell->save_history_line("line2");
    shell->save_history_line("line3.1\nline3.2\n\nline3.3");
    shell->save_history_line("line4");
    shell->save_history_line("line5");

    shell->set_snippet_data("hello world\nsnippet line this");
    shell->store_state();

    delete shell;

    shell = new ShellBE(data->dispatcher);
    shell->set_saves_history(10);
    shell->set_save_directory(CasmineContext::get()->outputDir());
    shell->restore_state();

    std::string line;

    flag = shell->previous_history_line("newline", line);
    $expect(flag).toBe(true, "get restored line");

    $expect(line).toBe("line5", "last line");

    flag = shell->previous_history_line(line, line);
    $expect(flag).toBe(true, "prev after save");
    $expect(line).toBe("line4", "last line ");

    flag = shell->previous_history_line(line, line);
    $expect(flag).toBe(true, "prev after save");
    $expect(line).toBe("line3.1\nline3.2\n\nline3.3", "last line");

    line = shell->get_snippet_data();
    $expect(line).toBe("hello world\nsnippet line this", "snippet");
    delete shell;
  });

};

}
