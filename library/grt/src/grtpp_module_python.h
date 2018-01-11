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

#pragma once

#include "python_context.h"
#include <string>
#include "grt.h"

namespace grt {

  class PythonModuleLoader;

  class MYSQLGRT_PUBLIC PythonModule : public Module {
    friend class PythonModuleLoader;

  public:
    PythonModule(PythonModuleLoader *loader, PyObject *module);
    virtual ~PythonModule();

    void add_parse_function(const std::string &name, PyObject *return_type, PyObject *arguments, PyObject *callable);

  protected:
    PyObject *_module;

    virtual ValueRef call_python_function(const BaseListRef &args, PyObject *function, const Function &funcdef);
  };

  class MYSQLGRT_PUBLIC PythonModuleLoader : public ModuleLoader {
  public:
    PythonModuleLoader(const std::string &module_path);
    virtual ~PythonModuleLoader();

    virtual std::string get_loader_name() {
      return LanguagePython;
    }

    virtual Module *init_module(const std::string &path);

    virtual void refresh();

    void add_module_dir(const std::string &path);
    virtual bool load_library(const std::string &file);

    virtual bool run_script_file(const std::string &path);
    virtual bool run_script(const std::string &script);

    virtual bool check_file_extension(const std::string &path);

    PythonContext *get_python_context() {
      return &_pycontext;
    }

  protected:
    friend class PythonModule;

    PythonContext _pycontext;
  };
};
