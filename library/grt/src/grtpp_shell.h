/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "grt.h"

namespace grt {

  typedef enum {
    ShellCommandUnknown = -1,
    ShellCommandExit = 0,
    ShellCommandAll,
    ShellCommandError,
    ShellCommandStatement,
    ShellCommandHelp,
    ShellCommandLs,
    ShellCommandCd,
    ShellCommandRun
  } ShellCommand;

#define MYX_SHELL_CURNODE "current"

  class MYSQLGRT_PUBLIC Shell {
  public:
    Shell();
    virtual ~Shell();

    bool set_disable_quit(bool flag);

    ShellCommand execute(const std::string &linebuf);

    virtual std::string shell_type() = 0;

    virtual void init() = 0;
    virtual void print_welcome() = 0;
    virtual std::string get_prompt() = 0;
    virtual int execute_line(const std::string &linebuf) = 0;
    virtual int run_file(const std::string &file_name, bool interactive) = 0;
    virtual void show_help(const std::string &topic) = 0;

    virtual std::vector<std::string> complete_line(const std::string &line, std::string &completed) = 0;

    virtual ValueRef get_global_var(const std::string &var_name) = 0;
    virtual int set_global_var(const std::string &var_name, const ValueRef &value) = 0;

    static std::string get_abspath(const std::string &cwd, const std::string &npath);

    virtual void print(const std::string &str);

  protected:
    bool _disable_quit;
  };
};
