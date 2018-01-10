/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _GRTPP_SHELL_PYTHON_H_
#define _GRTPP_SHELL_PYTHON_H_

#include "grtpp_module_python.h"
#include "grtpp_shell.h"

namespace grt {
  class MYSQLGRT_PUBLIC PythonShell : public Shell {
  public:
    PythonShell();

    virtual std::string shell_type() {
      return "python";
    }

    virtual void init();
    virtual void print_welcome();
    virtual std::string get_prompt();
    virtual int execute_line(const std::string &linebuf);
    virtual int run_file(const std::string &file_name, bool interactive);
    virtual void show_help(const std::string &topic);

    virtual std::vector<std::string> complete_line(const std::string &line, std::string &completed);

    virtual ValueRef get_global_var(const std::string &var_name);
    virtual int set_global_var(const std::string &var_name, const ValueRef &value);

  protected:
    std::vector<std::string> get_tokens_for_prefix(const std::string &prefix);

    std::string _current_line;

    PythonModuleLoader *_loader;
  };
};

#endif /* _GRTPP_SHELL_PYTHON_H_ */
