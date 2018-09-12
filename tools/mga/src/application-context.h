/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "types.h"

#include <string>

#pragma once

#ifdef _MSC_VER
#  ifdef MGA_EXPORTS
#    define MGA_PUBLIC __declspec(dllexport)
#  else
#    define MGA_PUBLIC __declspec(dllimport)
#  endif
#else
#  define MGA_PUBLIC
#endif

namespace mga {

  class MGA_PUBLIC ApplicationContext {
  private:
    std::string _currentDir;
    bool _debugMode;
    std::string _configFile;

  protected:
    ApplicationContext();
    ApplicationContext(const ApplicationContext&) = delete;
    ApplicationContext& operator= (ApplicationContext&) = delete;
    void showHelp();

  public:
    static ApplicationContext& get();
    virtual ~ApplicationContext();

    ExitCode initialize();
    ExitCode parseParams(int argc, const char **argv, char **envp);
    void run();
    ExitCode shutDown();
  };
}

