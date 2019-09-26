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

#include "grt.h"
#include "grtpp_module_cpp.h"
#include "grtpp_util.h"
#include "base/log.h"
#include "base/string_utilities.h"
#include "base/file_functions.h"
#include "base/file_utilities.h"

DEFAULT_LOG_DOMAIN(DOMAIN_GRT)

using namespace grt;

//--------------------------------------------------------------------------------

// A bundle (as in macOS bundles) is a directory tree containing all files needed for
// a self contained plugin, including dynamic libraries and data files.
std::string GRT::module_path_in_bundle(const std::string &path) {
  if (!g_str_has_suffix(path.c_str(), ".mwbplugin") || !g_file_test(path.c_str(), G_FILE_TEST_IS_DIR))
    return "";

#ifdef __APPLE__
  // a Mac bundle plugin, this is Mac specific and is used for obj-c plugins
  if (g_file_test((path + "/Contents/Info.plist").c_str(), G_FILE_TEST_IS_REGULAR)) {
    std::string module_file;

    FILE *file = base_fopen((path + "/Contents/Info.plist").c_str(), "r");
    if (file) {
      char line[256];

      while (fgets(line, sizeof(line), file)) {
        // find out the db.mysql.foobar.grt.dylib name
        if (strstr(line, "<key>GRTModuleFileName</key>")) {
          if (fgets(line, sizeof(line), file)) {
            char *name = strstr(line, "<string>");
            if (name) {
              char *end;
              name += strlen("<string>");
              end = strrchr(name, '<');
              if (end) {
                *end = 0;
                module_file = std::string(path).append("/Contents/Frameworks/").append(name);
              }
            }
          }
          break;
        }
      }

      fclose(file);
    }

    if (!module_file.empty())
      return module_file;
  }
#endif

  if (g_file_test((path + "/main_grt.py").c_str(), G_FILE_TEST_IS_REGULAR))
    return path + "/main_grt.py";

  return "";
}

//--------------------------------------------------------------------------------
// Module Loader

//--------------------------------------------------------------------------------
// Modules

Module::Module(ModuleLoader *loader) : _is_bundle(false), _loader(loader) {
}

bool Module::has_function(const std::string &name) const {
  return get_function(name) != 0;
}

ValueRef Module::call_function(const std::string &name, const grt::BaseListRef &args) {
  const Function *f = get_function(name);

  if (!f)
    throw grt::module_error(std::string("Module ").append(_name).append(" doesn't have function ").append(name));

  return f->call(args);
}

std::string Module::bundle_path() const {
  return base::dirname(_path);
}

std::string Module::default_icon_path() const {
  return bundle_path() + "/icon.png";
}

void Module::add_function(const Function &func) {
  _functions.push_back(func);
}

static bool parse_type_spec(const char *arg, TypeSpec &fp) {
  if (*arg == 'a' && strlen(arg) == 1)
    fp.base.type = AnyType;
  else if (*arg == 'i' && strlen(arg) == 1)
    fp.base.type = IntegerType;
  else if (*arg == 'r' && strlen(arg) == 1)
    fp.base.type = DoubleType;
  else if (*arg == 's' && strlen(arg) == 1)
    fp.base.type = StringType;
  else if (*arg == 'l' || *arg == 'd') {
    if (*arg == 'l')
      fp.base.type = ListType;
    else
      fp.base.type = DictType;
    if (arg[1] == '<') {
      if (arg[2] == 'i' && arg[3] == '>')
        fp.content.type = IntegerType;
      else if (arg[2] == 'r' && arg[3] == '>')
        fp.content.type = DoubleType;
      else if (arg[2] == 's' && arg[3] == '>')
        fp.content.type = StringType;
      else if (arg[2] == 'd' && arg[3] == '>')
        fp.content.type = DictType;
      else if (arg[2] == 'o') {
        fp.content.type = ObjectType;
        if (arg[3] == '@') {
          fp.content.object_class = std::string(arg + 4);
          fp.content.object_class = fp.content.object_class.substr(0, fp.content.object_class.find('>'));
        }
      } else
        return false;
    } else if (arg[1] == 0)
      fp.content.type = AnyType;
    else
      return false;
  } else if (*arg == 'o') {
    fp.base.type = ObjectType;
    if (arg[1] == '@') {
      fp.base.object_class = arg + 2;
    }
  }
  return true;
}

static bool parse_param_spec(char *arg, ArgSpec &aspec) {
  char *ptr;

  // the optional arg name
  ptr = strchr(arg, ' ');
  if (ptr) {
    aspec.name = ptr + 1;
    *ptr = 0;
  }

  return parse_type_spec(arg, aspec.type);
}

bool Module::add_parse_function_spec(const std::string &spec,
                                     const std::function<ValueRef(BaseListRef, Module *, Module::Function)> &caller) {
  if (!spec.empty()) {
    char **parts = g_strsplit(spec.c_str(), ":", 0);
    char **args;
    int i, argc;
    Module::Function func;

    if (g_strv_length(parts) != 3) {
      g_strfreev(parts);
      return false;
    }

    func.name = parts[0];

    // parse return type
    if (!parse_type_spec(parts[1], func.ret_type)) {
      g_strfreev(parts);
      return false;
    }

    // parse arguments
    args = g_strsplit(parts[2], ",", 0);
    g_strfreev(parts);

    argc = g_strv_length(args);
    for (i = 0; i < argc; i++) {
      ArgSpec arg;

      if (!parse_param_spec(args[i], arg)) {
        g_strfreev(args);
        return false;
      }
      func.arg_types.push_back(arg);
    }

    g_strfreev(args);

    // add slot for calling the function
    func.call = std::bind(caller, std::placeholders::_1, this, func);

    _functions.push_back(func);

    return true;
  }

  return false;
}

const Module::Function *Module::get_function(const std::string &name) const {
  for (std::vector<Function>::const_iterator iter = _functions.begin(); iter != _functions.end(); ++iter) {
    if (iter->name == name)
      return &*iter;
  }

  if (!_extends.empty()) {
    Module *module = grt::GRT::get()->get_module(_extends);
    if (!module)
      throw std::runtime_error(
        base::strfmt("Parent module '%s' of module '%s' was not found", _extends.c_str(), _name.c_str()));

    return module->get_function(name);
  }
  return 0;
}

void Module::validate() const {
  if (name().empty())
    throw std::runtime_error("Invalid module, name is not set");

  // validate if registered functions conform to interfaces
  for (Interfaces::const_iterator iter = _interfaces.begin(); iter != _interfaces.end(); ++iter) {
    const Interface *iface = grt::GRT::get()->get_interface(*iter);
    if (iface) {
      if (!iface->check_conformance(this))
        throw std::logic_error(
          std::string("Module ").append(name()).append(" does not conform to interface ").append(*iter));
    } else
      logWarning("Interface '%s' implemented by module '%s' is not registered\n", iter->c_str(), name().c_str());
  }
}

void Module::set_global_data(const std::string &key, const std::string &value) {
  std::string k = name();
  k.append(":").append(key);

  grt::DictRef dict;

  dict =
    grt::DictRef::cast_from(get_value_by_path(grt::GRT::get()->root(), grt::GRT::get()->global_module_data_path()));
  dict.set(k, grt::StringRef(value));
}

void Module::set_global_data(const std::string &key, int value) {
  std::string k = name();
  k.append(":").append(key);

  grt::DictRef dict;
  dict =
    grt::DictRef::cast_from(get_value_by_path(grt::GRT::get()->root(), grt::GRT::get()->global_module_data_path()));
  dict.set(k, grt::IntegerRef(value));
}

int Module::global_int_data(const std::string &key, int default_value) {
  std::string k = name();
  k.append(":").append(key);

  grt::DictRef dict;
  dict =
    grt::DictRef::cast_from(get_value_by_path(grt::GRT::get()->root(), grt::GRT::get()->global_module_data_path()));
  return (int)*grt::IntegerRef::cast_from(dict.get(k, grt::IntegerRef(default_value)));
}

std::string Module::global_string_data(const std::string &key, const std::string &default_value) {
  std::string k = name();
  k.append(":").append(key);

  grt::DictRef dict;
  dict =
    grt::DictRef::cast_from(get_value_by_path(grt::GRT::get()->root(), grt::GRT::get()->global_module_data_path()));
  return *grt::StringRef::cast_from(dict.get(k, grt::StringRef(default_value)));
}

void Module::set_document_data(const std::string &key, const std::string &value) {
  std::string k = name();
  k.append(":").append(key);

  grt::DictRef dict;

  dict =
    grt::DictRef::cast_from(get_value_by_path(grt::GRT::get()->root(), grt::GRT::get()->document_module_data_path()));
  dict.set(k, grt::StringRef(value));
}

void Module::set_document_data(const std::string &key, int value) {
  std::string k = name();
  k.append(":").append(key);

  grt::DictRef dict;
  dict =
    grt::DictRef::cast_from(get_value_by_path(grt::GRT::get()->root(), grt::GRT::get()->document_module_data_path()));
  dict.set(k, grt::IntegerRef(value));
}

int Module::document_int_data(const std::string &key, int default_value) {
  std::string k = name();
  k.append(":").append(key);

  grt::DictRef dict;
  dict =
    grt::DictRef::cast_from(get_value_by_path(grt::GRT::get()->root(), grt::GRT::get()->document_module_data_path()));
  return (int)*grt::IntegerRef::cast_from(dict.get(k, grt::IntegerRef(default_value)));
}

std::string Module::document_string_data(const std::string &key, const std::string &default_value) {
  std::string k = name();
  k.append(":").append(key);

  grt::DictRef dict;
  dict =
    grt::DictRef::cast_from(get_value_by_path(grt::GRT::get()->root(), grt::GRT::get()->document_module_data_path()));
  return *grt::StringRef::cast_from(dict.get(k, grt::StringRef(default_value)));
}
