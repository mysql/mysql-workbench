/*
 * Copyright (c) 2007, 2020, Oracle and/or its affiliates.
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

#include "grtpp_shell_python.h"
#include "base/string_utilities.h"
#include "base/threading.h"
#include "base/log.h"
#include "base/file_utilities.h"
#include "base/xml_functions.h"

#include "grt.h"
#include "grtpp_util.h"
#include "grtpp_shell.h"
#include "grtpp_module_cpp.h"
#include "grtpp_undo_manager.h"
#include "grtpp_notifications.h"

#include <cppconn/exception.h>
#include <algorithm>
#include <glib.h>

#include "serializer.h"
#include "unserializer.h"
#include <iostream>

DEFAULT_LOG_DOMAIN(DOMAIN_GRT)

using namespace grt;
using namespace base;

//--------------------------------------------------------------------------------------------------

std::shared_ptr<GRT> GRT::get() {
  static std::shared_ptr<GRT> instance(new GRT);
  return instance;
}

//--------------------------------------------------------------------------------------------------

std::string grt::type_to_str(Type type) {
  switch (type) {
    case UnknownType:
      return "";
    case IntegerType:
      return "int";
    case DoubleType:
      return "real";
    case StringType:
      return "string";
    case ListType:
      return "list";
    case DictType:
      return "dict";
    case ObjectType:
      return "object";
  }
  return "";
}

//--------------------------------------------------------------------------------------------------
std::map<std::string, base::any> grt::convert(const grt::DictRef dict) {
  std::map<std::string, base::any> result;
  for (auto it = dict.begin(); it != dict.end(); ++it) {
    auto val = dict.get(it->first);
    std::pair<std::string, base::any> item;
    if (val.is_valid()) {
      switch (val.type()) {
        case grt::IntegerType:
          item = {it->first, grt::IntegerRef::extract_from(val)};
          break;
        case grt::DoubleType:
          item = {it->first, grt::DoubleRef::extract_from(val)};
          break;
        case grt::StringType:
          item = {it->first, grt::StringRef::extract_from(val)};
          break;
        case grt::ListType: {
          auto list = grt::BaseListRef::cast_from(val);
          std::vector<base::any> vec(list.count());
          for (std::size_t i = 0; i < list.count(); ++i)
            vec[i] = list.get(i);
          break;
        }
        case grt::DictType:
          item = {it->first, convert(grt::DictRef::cast_from(val))};
          break;
        case grt::ObjectType:
          item = {it->first, grt::ObjectRef::cast_from(val)};
          break;
        default:
          item = {it->first, val};
      }
    } else
      item = {it->first, nullptr};

    result.insert(item);
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

Type grt::str_to_type(const std::string &type) {
  char ini = type[0];
  if (ini == 'i' && type == "int")
    return IntegerType;
  else if ((ini == 'd' && type == "double") || (ini == 'r' && type == "real"))
    return DoubleType;
  else if (ini == 's' && type == "string")
    return StringType;
  else if (ini == 'l' && type == "list")
    return ListType;
  else if (ini == 'd' && type == "dict")
    return DictType;
  else if (ini == 'o' && type == "object")
    return ObjectType;
  return UnknownType;
}

std::string Message::format(bool withtype) const {
  std::string text;

  if (withtype) {
    switch (type) {
      case InfoMsg:
        text = "Info: ";
        break;
      case WarningMsg:
        text = "Warning: ";
        break;
      case ErrorMsg:
        text = "Error: ";
        break;
      default:
        text = "";
        break;
    }
  }

  text += this->text;

  if (!detail.empty())
    text += " (" + detail + ")";

  return text;
}

//--------------------------------------------------------------------------------------------------

type_error::type_error(Type expected, Type actual)
  : std::logic_error(std::string("Type mismatch: expected type ")
                       .append(type_to_str(expected))
                       .append(", but got ")
                       .append(type_to_str(actual))) {
}

type_error::type_error(TypeSpec expected, TypeSpec actual)
  : std::logic_error(std::string("Type mismatch: expected ")
                       .append(fmt_type_spec(expected))
                       .append(", but got ")
                       .append(fmt_type_spec(actual))) {
}

type_error::type_error(Type expected, Type actual, Type container)
  : std::logic_error(std::string("Type mismatch: expected content-type ")
                       .append(type_to_str(expected))
                       .append(", but got ")
                       .append(type_to_str(actual))) {
}

type_error::type_error(const std::string &expected, Type actual)
  : std::logic_error(
      std::string("Type mismatch: expected ").append(expected).append(", but got ").append(type_to_str(actual))) {
}

type_error::type_error(const std::string &expected, const std::string &actual)
  : std::logic_error(
      std::string("Type mismatch: expected object of type ").append(expected).append(", but got ").append(actual)) {
}

type_error::type_error(const std::string &expected, const std::string &actual, Type container)
  : std::logic_error(std::string("Type mismatch: expected content object of type ")
                       .append(expected)
                       .append(", but got ")
                       .append(actual)) {
}

db_error::db_error(const sql::SQLException &exc) : std::runtime_error(exc.what()), _error(exc.getErrorCode()) {
}

//----------------- Value ----------------------------------------------------------------------------------------------

internal::Value* internal::Value::retain() {
  g_atomic_int_inc(&_refcount);
  return this;
}

//----------------------------------------------------------------------------------------------------------------------

void internal::Value::release() {
#ifdef WB_DEBUG
  if (_refcount == 0)
    logWarning("GRT: releasing invalid object\n");
#endif
  if (g_atomic_int_dec_and_test(&_refcount))
    delete this;
}

//----------------------------------------------------------------------------------------------------------------------

base::refcount_t internal::Value::refcount() const {
  return g_atomic_int_get(&_refcount);
}

//----------------------------------------------------------------------------------------------------------------------

StringRef StringRef::format(const char *format, ...) {
  va_list args;
  char *tmp;
  StringRef ret;

  va_start(args, format);
  tmp = g_strdup_vprintf(format, args);
  va_end(args);

  ret = StringRef(tmp);
  g_free(tmp);

  return ret;
}

//--------------------------------------------------------------------------------------------------

GRT::GRT() : _check_serialized_crc(false), _verbose(false), _testing(false) {
  _scanning_modules = false;

  _tracking_changes = 0;
  _shell = 0;

  if (getenv("GRT_VERBOSE"))
    _verbose = true;

  GRTNotificationCenter::setup();

  _default_undo_manager = new UndoManager;

  add_module_loader(new CPPModuleLoader());

  // register metaclass for base class
  add_metaclass(MetaClass::create_base_class());

  _root = grt::DictRef(true);
}

GRT::~GRT() {

  for (auto &it: _messageSlotStack) {
    delete it;
  }
   _messageSlotStack.clear();

  delete _shell;
  _shell = nullptr;

  delete _default_undo_manager;
  _default_undo_manager = nullptr;


  for (const auto it: _modules) {
    auto module = it->getModule();
    delete it;
    if (module) {
      g_module_close(module);
    }
  }

  _modules.clear();

  for (std::map<std::string, Interface *>::iterator iter = _interfaces.begin(); iter != _interfaces.end(); ++iter)
    delete iter->second;
  _interfaces.clear();

  for (std::list<ModuleLoader *>::iterator iter = _loaders.begin(); iter != _loaders.end(); ++iter)
    delete *iter;
  _loaders.clear();

  for (std::map<std::string, MetaClass *>::iterator iter = _metaclasses.begin(); iter != _metaclasses.end(); ++iter)
    delete iter->second;
  _metaclasses.clear();
  
  // We need to first release PythonLoader so we don't end up Python calling some WB modules
  auto iter = std::remove_if(_loaders.begin(), _loaders.end(), 
                  [&](auto module) { return module->get_loader_name() == grt::LanguagePython; });

  for (; iter != _loaders.end(); ++iter) {
    delete *iter;
  }
  
  _loaders.erase(iter, _loaders.end());

}

void GRT::push_undo_manager(UndoManager *um) {
  _undo_managers.push_back(um);
}

UndoManager *GRT::pop_undo_manager() {
  if (_undo_managers.empty())
    return 0;
  UndoManager *tmp = _undo_managers.back();
  _undo_managers.pop_back();
  return tmp;
}

UndoManager *GRT::get_undo_manager() const {
  return _undo_managers.empty() ? _default_undo_manager : _undo_managers.back();
}

void GRT::start_tracking_changes() {
  _tracking_changes++;
}

void GRT::stop_tracking_changes() {
  _tracking_changes--;
}

void GRT::set_verbose(bool flag) {
  _verbose = flag;
}

UndoGroup *GRT::begin_undoable_action(UndoGroup *group) {
  start_tracking_changes();
  return get_undo_manager()->begin_undo_group(group);
}

void GRT::end_undoable_action(const std::string &group_description) {
  if (!get_undo_manager()->end_undo_group(group_description, true)) {
    if (getenv("DEBUG_UNDO"))
      logWarning("'%s' was empty\n", group_description.c_str());
  }
  stop_tracking_changes();
}

void GRT::cancel_undoable_action() {
  get_undo_manager()->cancel_undo_group();
  stop_tracking_changes();
}

void GRT::lock() const {
}

void GRT::unlock() const {
}

//--------------------------------------------------------------------------------------------------

bool GRT::metaclassesNeedRegister() {
  return !internal::ClassRegistry::get_instance()->isEmpty();
}

void GRT::load_metaclasses(const std::string &file, std::list<std::string> *requiresList) {
  xmlNodePtr root;
  xmlDocPtr doc;

  doc = base::xml::loadXMLDoc(file);

  root = xmlDocGetRootElement(doc);

  if (root && xmlStrcmp(root->name, (xmlChar *)"gstructs") == 0) {
    root = root->children;
    while (root) {
      if (xmlStrcmp(root->name, (xmlChar *)"gstruct") == 0) {
        MetaClass *gstruct = MetaClass::from_xml(file, root);
        if (gstruct) {
          MetaClass *tmp;
          if ((tmp = get_metaclass(gstruct->name())) == 0) {
            add_metaclass(gstruct);
          } else if (tmp != gstruct) {
            delete gstruct;
            throw std::runtime_error("Duplicate struct " + gstruct->name());
          }

          // add it to the ordered list of metaclasses, we dont want placeholders here
          _metaclasses_list.push_back(gstruct);
        }
      } else if (xmlStrcmp(root->name, (xmlChar *)"requires") == 0) {
        xmlChar *path = xmlGetProp(root, (xmlChar *)"file");
        if (path) {
          if (requiresList)
            requiresList->push_back((char *)path);
          xmlFree(path);
        }
      }
      root = root->next;
    }
  }
  xmlFreeDoc(doc);
}

//--------------------------------------------------------------------------------------------------

void GRT::reinitialiseForTests() {
  delete _shell;
  _shell = nullptr;
  delete _default_undo_manager;
  _default_undo_manager = nullptr;

  // We need to first release PythonLoader so we don't end up Python calling some WB modules
  for (std::list<ModuleLoader *>::iterator iter = _loaders.begin(); iter != _loaders.end(); ++iter) {
    if ((*iter)->get_loader_name() == grt::LanguagePython) {
      delete *iter;
      _loaders.erase(iter);
      break;
    }
  }

  for (const auto &it: _modules) {
    auto module = it->getModule();
    delete it;
    if (module) {
      g_module_close(module);
    }
  }

  _modules.clear();
  _objects_cache.clear();
  _cached_module_wrapper.clear();

  for (std::map<std::string, Interface *>::iterator iter = _interfaces.begin(); iter != _interfaces.end(); ++iter)
    delete iter->second;
  _interfaces.clear();

  for (std::list<ModuleLoader *>::iterator iter = _loaders.begin(); iter != _loaders.end(); ++iter)
    delete *iter;
  _loaders.clear();

  for (std::map<std::string, MetaClass *>::iterator iter = _metaclasses.begin(); iter != _metaclasses.end(); ++iter) {
    logDebug3("Deleting metaclass: %s\n", iter->first.c_str());
    delete iter->second;
  }
  _metaclasses.clear();
  _metaclasses_list.clear();

  internal::ClassRegistry::get_instance()->cleanUp();
  _root.clear();

  _scanning_modules = false;

  _tracking_changes = 0;

  if (getenv("GRT_VERBOSE"))
  _verbose = true;

  GRTNotificationCenter::setup();

  _default_undo_manager = new UndoManager;

  add_module_loader(new CPPModuleLoader());

  // register metaclass for base class
  add_metaclass(MetaClass::create_base_class());

  _root = grt::DictRef(true);
}

//--------------------------------------------------------------------------------------------------

int GRT::scan_metaclasses_in(const std::string &directory, std::multimap<std::string, std::string> *requiresMap) {
  GDir *dir;
  const char *entry;
  size_t old_count = _metaclasses.size();

  dir = g_dir_open(directory.c_str(), 0, NULL);
  if (!dir)
    throw grt::os_error("Invalid path " + directory);

  while ((entry = g_dir_read_name(dir)) != NULL) {
    if ((g_str_has_prefix(entry, "structs.")) && (g_str_has_suffix(entry, ".xml"))) {
      char *path = g_build_filename(directory.c_str(), entry, NULL);
      std::list<std::string> reqs;

      reqs.clear();
      try {
        load_metaclasses(path, &reqs);
      } catch (...) {
        g_free(path);
        throw;
      }

      if (requiresMap) {
        for (std::list<std::string>::const_iterator i = reqs.begin(); i != reqs.end(); ++i)
          requiresMap->insert(std::pair<std::string, std::string>(path, *i));
      }

      g_free(path);
    }
  }

  g_dir_close(dir);
  return (int)(_metaclasses.size() - old_count);
}

static void dfs_visit(MetaClass *u, const std::multimap<MetaClass *, MetaClass *> &adjacents,
                      std::set<MetaClass *> &visited, std::list<MetaClass *> &sorted) {
  visited.insert(u);

  std::multimap<MetaClass *, MetaClass *>::const_iterator iter = adjacents.find(u);

  while (iter != adjacents.end() && iter->first == u) {
    MetaClass *v = iter->second;

    if (visited.find(v) == visited.end())
      dfs_visit(v, adjacents, visited, sorted);
    ++iter;
  }

  sorted.push_front(u);
}

static std::list<MetaClass *> sort_metaclasses(const std::list<MetaClass *> &list) {
  std::list<MetaClass *> sorted;
  std::set<MetaClass *> visited;
  std::multimap<MetaClass *, MetaClass *> adjacents;
  typedef std::pair<MetaClass *, MetaClass *> pair;

  for (std::list<MetaClass *>::const_iterator iter = list.begin(); iter != list.end(); ++iter) {
    if ((*iter)->parent())
      adjacents.insert(pair((*iter)->parent(), *iter));
  }

  for (std::list<MetaClass *>::const_iterator u = list.begin(); u != list.end(); ++u) {
    if (visited.find(*u) == visited.end())
      dfs_visit(*u, adjacents, visited, sorted);
  }
  return sorted;
}

void GRT::end_loading_metaclasses(bool check_class_binding) {
  bool undefined = false;
  bool validate_error = false;

  for (std::map<std::string, MetaClass *>::iterator iter = _metaclasses.begin(); iter != _metaclasses.end(); ++iter) {
    if (iter->second->placeholder()) {
      undefined = true;
      logWarning("MetaClass '%s' is undefined but was referred in '%s'\n", iter->second->name().c_str(),
                 iter->second->source().c_str());
    }
    if (!iter->second->validate())
      validate_error = true;
  }
  if (undefined)
    throw std::runtime_error("One or more undefined meta classes were referred by other structs");
  if (validate_error)
    throw std::runtime_error("Validation error in loaded metaclasses");

  // register GRT object classes
  internal::ClassRegistry::get_instance()->register_all();

  if (check_class_binding) {
    // check if there are any metaclasses with unbound members
    for (std::map<std::string, MetaClass *>::iterator iter = _metaclasses.begin(); iter != _metaclasses.end(); ++iter) {
      if (!iter->second->is_bound())
        logWarning("Allocation function of '%s' is unbound, which probably means the implementing C++ class was not"
          "registered\n",
          iter->second->name().c_str());
    }
  }

  // do a topological sort of the list of metaclasses, so that they're hierarchical order
  _metaclasses_list = sort_metaclasses(_metaclasses_list);
}

MetaClass *GRT::get_metaclass(const std::string &name) const {
  std::map<std::string, MetaClass *>::const_iterator iter;

  if ((iter = _metaclasses.find(name)) == _metaclasses.end())
    return 0;
  return iter->second;
}

void GRT::add_metaclass(MetaClass *stru) {
  _metaclasses[stru->name()] = stru;
}

//--------------------------------------------------------------------------------------------------

void GRT::set_root(const ValueRef &root) {
  AutoLock lock;
  _root = root;
  // only nodes starting from /wb/doc (ie, in a model) should be marked global for undo tracking
  //  if (_root.is_valid())
  //    _root.mark_global();
}

ValueRef GRT::get(const std::string &path) const {
  AutoLock lock;

  return get_value_by_path(_root, path);
}

void GRT::set(const std::string &path, const ValueRef &value) {
  AutoLock lock;

  if (!set_value_by_path(_root, path, value))
    throw grt::bad_item("Invalid path " + path);
}

ObjectRef GRT::find_object_by_id(const std::string &id, const std::string &subpath) {
  /*
  std::map<std::string,internal::Object*>::const_iterator iter= _objects.find(id);
  if (iter == _objects.end())
    return ObjectRef(iter->second);
  return ObjectRef();
   */

  std::map<std::string, ObjectRef>::const_iterator iter;
  if ((iter = _objects_cache.find(id)) != _objects_cache.end())
    return iter->second;

  ValueRef start = get(subpath);
  ObjectRef result = ObjectRef();

  if (start.is_valid()) {
    switch (start.type()) {
      case ListType:
        result = find_child_object(BaseListRef::cast_from(start), id);
        break;
      case DictType:
        result = find_child_object(DictRef::cast_from(start), id);
        break;
      case ObjectType:
        result = find_child_object(ObjectRef::cast_from(start), id);
        break;
      default:
        throw std::invalid_argument("Value at " + subpath + " is not a container");
    }
  }
  if (result.is_valid())
    _objects_cache[id] = result;

  return result;
}

//--------------------------------------------------------------------------------------------------

void GRT::serialize(const ValueRef &value, const std::string &path, const std::string &doctype,
                    const std::string &version, bool list_objects_as_links) {
  internal::Serializer ser;

  ser.save_to_xml(value, path, doctype, version, list_objects_as_links);
}

std::shared_ptr<grt::internal::Unserializer> GRT::get_unserializer() {
  return std::shared_ptr<grt::internal::Unserializer>(new internal::Unserializer(_check_serialized_crc));
};

ValueRef GRT::unserialize(const std::string &path, std::shared_ptr<grt::internal::Unserializer> unserializer) {
  if (!unserializer)
    unserializer = std::shared_ptr<grt::internal::Unserializer>(new internal::Unserializer(_check_serialized_crc));

  if (!g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
    throw os_error(path);

  try {
    return unserializer->load_from_xml(path);
  } catch (std::exception &exc) {
    throw std::runtime_error(
      std::string("Error unserializing GRT data from ").append(path).append(": ").append(exc.what()));
  }
}

ValueRef GRT::unserialize(const std::string &path, std::string &doctype_ret, std::string &version_ret) {
  internal::Unserializer unser(_check_serialized_crc);

  if (!g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
    throw os_error(path);
  try {
    return unser.load_from_xml(path, &doctype_ret, &version_ret);
  } catch (std::exception &exc) {
    throw grt_runtime_error("Error unserializing GRT data from " + path, exc.what());
  }
}

xmlDocPtr GRT::load_xml(const std::string &path) {
  return base::xml::loadXMLDoc(path);
}

void GRT::get_xml_metainfo(xmlDocPtr doc, std::string &doctype_ret, std::string &version_ret) {
  base::xml::getXMLDocMetainfo(doc, doctype_ret, version_ret);
}

ValueRef GRT::unserialize_xml(xmlDocPtr doc, const std::string &source_path) {
  internal::Unserializer unser(_check_serialized_crc);

  try {
    return unser.unserialize_xmldoc(doc, source_path);
  } catch (std::exception &exc) {
    throw grt_runtime_error("Error unserializing GRT data", exc.what());
  }
}

std::string GRT::serialize_xml_data(const ValueRef &value, const std::string &doctype, const std::string &version,
                                    bool list_objects_as_links) {
  return internal::Serializer().serialize_to_xmldata(value, doctype, version, list_objects_as_links);
}

ValueRef GRT::unserialize_xml_data(const std::string &data) {
  return internal::Unserializer(_check_serialized_crc).unserialize_xmldata(data.data(), data.size());
}

//--------------------------------------------------------------------------------

void GRT::add_module_loader(ModuleLoader *loader) {
  _loaders.push_back(loader);

  loader->refresh();
}

bool GRT::load_module(const std::string &path, const std::string &basePath, bool refresh) {
  std::string shortendPath = base::relativePath(basePath, path);
  if (shortendPath != path)
    shortendPath = "<base dir>/" + shortendPath;

  for (std::list<ModuleLoader *>::iterator loader = _loaders.begin(); loader != _loaders.end(); ++loader) {
    if ((*loader)->check_file_extension(path)) {
      logDebug2("Trying to load module '%s' (%s)\n", shortendPath.c_str(), (*loader)->get_loader_name().c_str());

      // Problems, if any, are logged in init_module.
      Module *module = (*loader)->init_module(path);
      if (module) {
        try {
          if (refresh)
            refresh_module(module);
          else
            register_new_module(module);
        } catch (std::exception &exc) {
          logDebug("Deleting module %s because of %s\n", module->name().c_str(), exc.what());
          delete module;
          throw;
        }
        return true;
      }
    }
  }
  return false;
}

ModuleLoader *GRT::get_module_loader(const std::string &name) {
  for (std::list<ModuleLoader *>::const_iterator iter = _loaders.begin(); iter != _loaders.end(); ++iter) {
    if ((*iter)->get_loader_name() == name)
      return *iter;
  }
  return 0;
}

ModuleLoader *GRT::get_module_loader_for_file(const std::string &path) {
  for (std::list<ModuleLoader *>::const_iterator iter = _loaders.begin(); iter != _loaders.end(); ++iter) {
    if ((*iter)->check_file_extension(path))
      return *iter;
  }
  return 0;
}

int GRT::scan_modules_in(const std::string &path, const std::string &basePath, const std::list<std::string> &exts,
                         bool reload) {
  GDir *dir;
  const char *entry;
  int count = 0;
  GError *error = NULL;

  dir = g_dir_open(path.c_str(), 0, &error);
  if (!dir) {
    send_warning(strfmt("Cannot open module directory %s: %s", path.c_str(), error->message));
    g_error_free(error);
    return -1;
  }

  _scanning_modules = true;

  if (verbose())
    send_info(strfmt("Scanning module directory %s.", path.c_str()));

  while ((entry = g_dir_read_name(dir)) != NULL) {
    std::string entry_path = path;
    std::string module_path;

    entry_path.append(G_DIR_SEPARATOR_S).append(entry);

    // check if it's a bundle directory
    module_path = module_path_in_bundle(entry_path);
    if (module_path.empty())
      module_path = entry_path;

    // if we were given a list of extensions, check if it matches
    if (!exts.empty()) {
      std::string::size_type end = module_path.rfind('.');
      if (end == std::string::npos)
        continue;
      std::string name = module_path.substr(0, end);
      bool ok = false;

      for (std::list<std::string>::const_iterator ext = exts.begin(); ext != exts.end(); ++ext) {
        std::string e = *ext;
        std::string _e = "_" + e.substr(1); // some module types (like python) need _ext instead of .ext

        if (g_str_has_suffix(name.c_str(), e.c_str())) {
          ok = true;
          break;
        } else if (g_str_has_suffix(name.c_str(), _e.c_str())) {
          ok = true;
          break;
        }
      }

      if (!ok)
        continue;
    }

    try {
      if (load_module(module_path, basePath, reload)) {
        count++;
      }
    } catch (std::exception &exc) {
      send_warning(strfmt("Could not load %s: %s", entry, exc.what()));
    }
  }

  g_dir_close(dir);

  _scanning_modules = false;

  refresh_loaders();

  return count;
}

static bool compare_modules(Module *a, Module *b) {
  return g_ascii_strcasecmp(a->name().c_str(), b->name().c_str()) < 0;
}

void GRT::end_loading_modules() {
  std::sort(_modules.begin(), _modules.end(), compare_modules);
}

Module *GRT::get_module(const std::string &name) {
  for (std::vector<Module *>::iterator iter = _modules.begin(); iter != _modules.end(); ++iter) {
    if ((*iter)->name() == name)
      return *iter;
  }
  return 0;
}

grt::ValueRef GRT::call_module_function(const std::string &module, const std::string &function,
                                        const grt::BaseListRef &args) {
  Module *m = get_module(module);
  if (!m)
    throw grt::module_error("Module " + module + " not found");
  return m->call_function(function, args);
}

std::vector<Module *> GRT::find_modules_matching(const std::string &interface_name, const std::string &name_pattern) {
  std::vector<Module *> result;

  for (std::vector<Module *>::const_iterator module = _modules.begin(); module != _modules.end(); ++module) {
    bool ok = true;
    if (!interface_name.empty()) {
      ok = false;

      if (std::find((*module)->get_interfaces().begin(), (*module)->get_interfaces().end(), interface_name) !=
          (*module)->get_interfaces().end())
        ok = true;
    }

    if (ok && (name_pattern.empty() || g_pattern_match_simple(name_pattern.c_str(), (*module)->name().c_str()))) {
      result.push_back(*module);
    }
  }

  return result;
}

void GRT::refresh_loaders() {
  for (std::list<ModuleLoader *>::iterator iter = _loaders.begin(); iter != _loaders.end(); ++iter) {
    (*iter)->refresh();
  }
}

void GRT::register_new_module(Module *module) {
  module->validate();

  if (get_module(module->name()))
    throw std::runtime_error("Duplicate module " + module->name());

  _modules.push_back(module);

  if (!_scanning_modules)
    refresh_loaders();
}

void GRT::unregister_module(Module *module) {
  std::vector<Module *>::iterator iter = std::find(_modules.begin(), _modules.end(), module);
  if (iter != _modules.end())
    _modules.erase(iter);

  refresh_loaders();

  // XXX don't delete for now until we're sure there's no side-effects
  // delete module;
}

void GRT::refresh_module(Module *module) {
  bool found = false;

  module->validate();

  for (std::vector<Module *>::iterator iter = _modules.begin(); iter != _modules.end(); ++iter) {
    if ((*iter)->name() == module->name()) {
      delete *iter;

      *iter = module;
      found = true;
      break;
    }
  }
  if (!found)
    register_new_module(module);
}

void GRT::register_new_interface(Interface *iface) {
  if (get_interface(iface->name()))
    throw std::logic_error("Duplicate interface " + iface->name());

  _interfaces[iface->name()] = iface;
}

const Interface *GRT::get_interface(const std::string &name) {
  std::map<std::string, Interface *>::const_iterator iter;
  if ((iter = _interfaces.find(name)) == _interfaces.end())
    return 0;
  return iter->second;
}

//--------------------------------------------------------------------------------

void GRT::set_context_data(const std::string &key, void *value, void (*free_value)(void *)) {
  unset_context_data(key);

  _context_data[key].first = value;
  _context_data[key].second = free_value;
}

void GRT::unset_context_data(const std::string &key) {
  if (_context_data.find(key) != _context_data.end()) {
    if (_context_data[key].second)
      (*_context_data[key].second)(_context_data[key].first);
    _context_data.erase(key);
  }
}

void *GRT::get_context_data(const std::string &key) {
  return _context_data[key].first;
}

//--------------------------------------------------------------------------------

bool GRT::init_shell(const std::string &shell_type) {
  if (shell_type == LanguagePython)
    _shell = new PythonShell;
  else
    throw std::runtime_error("Invalid shell type " + shell_type);

  _shell->init();

  return true;
}

std::string GRT::shell_type() {
  if (dynamic_cast<PythonShell *>(_shell))
    return LanguagePython;

  return "";
}

Shell *GRT::get_shell() {
  return _shell;
}

//--------------------------------------------------------------------------------

void GRT::pushMessageHandler(SlotHolder *slot) {
  base::RecMutexLock lock(_message_mutex);
  _messageSlotStack.push_back(slot);
}

void GRT::popMessageHandler() {
  base::RecMutexLock lock(_message_mutex);
  if (_messageSlotStack.empty()) {
    logError("popMessageHandler() called on empty handler stack");
  } else {
    delete _messageSlotStack.back();
    _messageSlotStack.pop_back();
  }
}

void GRT::removeMessageHandler(SlotHolder *slot) {
  base::RecMutexLock lock(_message_mutex);
  auto iter = std::find(_messageSlotStack.begin(), _messageSlotStack.end(), slot);
  if (iter != _messageSlotStack.end()) {
    delete *iter;
    _messageSlotStack.erase(iter);
  }
}

bool GRT::handle_message(const Message &msg, void *sender) {
  // Don't log any message if there's no message slot is occupied. It just means
  // we don't want anything logged.
  if (!_messageSlotStack.empty()) {
    int i = 0;
    SlotHolder *slot = nullptr;
    for (;;) {
      {
        base::RecMutexLock lock(_message_mutex);
        if ((int)_messageSlotStack.size() - i - 1 >= 0) {
          slot = _messageSlotStack[_messageSlotStack.size() - i - 1];
          ++i;
        } else
          break;
      }
      if (slot->slot(msg, sender))
        return true;
    }
  }
  logError("Unhandled message (%lu): %s\n", (unsigned long)_messageSlotStack.size(), msg.format().c_str());
  return false;
}

void GRT::push_status_query_handler(const StatusQuerySlot &slot) {
  _status_query_slot_stack.push_back(slot);
}

void GRT::pop_status_query_handler() {
  _status_query_slot_stack.pop_back();
}

bool GRT::query_status() {
  if (_status_query_slot_stack.empty())
    return false;
  return _status_query_slot_stack.back()();
}

// XXX: these handlers should go and be replaced by pure log_* calls.
void GRT::send_error(const std::string &message, const std::string &details, void *sender) {
  base::RecMutexLock lock(_message_mutex);
  Message msg;
  msg.type = ErrorMsg;
  msg.text = message;
  msg.detail = details;
  msg.timestamp = time(NULL);
  msg.progress = 0.0;
  handle_message(msg, sender);

  logError("%s\t%s\n", message.c_str(), details.c_str());
}

void GRT::send_warning(const std::string &message, const std::string &details, void *sender) {
  base::RecMutexLock lock(_message_mutex);
  Message msg;
  msg.type = WarningMsg;
  msg.text = message;
  msg.detail = details;
  msg.timestamp = time(NULL);
  msg.progress = 0.0;
  handle_message(msg, sender);

  logWarning("%s\t%s\n", message.c_str(), details.c_str());
}

void GRT::send_info(const std::string &message, const std::string &details, void *sender) {
  base::RecMutexLock lock(_message_mutex);
  Message msg;
  msg.type = InfoMsg;
  msg.text = message;
  msg.detail = details;
  msg.timestamp = time(NULL);
  msg.progress = 0.0;
  handle_message(msg, sender);

  logInfo("%s\t%s\n", message.c_str(), details.c_str());
}

void GRT::reset_progress_steps() {
  _progress_step_stack.clear();
}

/**
 Used to begin tracking progress in a sub-task from a larger task.
 A large task can be broken down in smaller sub-tasks.
 The progress of the task as a whole will be tracked from a 0.0 to 1.0, and the sub-tasks
 can also be individually reported to be ranging from 0.0 to 1.0. As long as begin_progress_step()
 is called with correct values for the range of each sub-task in the larger task, the progress
 of the task as a whole will be computed and reported as the user would expect. This has the advantage
 that the sub-tasks don't need to know their relative progress in the big task, only their own progress.

 begin - from 0.0 to 1.0, initial value of the progress for the sub-task
 end - from 0.0 to 1.0, the final value of the progress for the sub-task once its completed
 */
void GRT::begin_progress_step(float from, float to) {
  _progress_step_stack.push_back(std::make_pair(from, to));
}

void GRT::end_progress_step() {
  _progress_step_stack.pop_back();
}

void GRT::send_progress(float percentage, const std::string &message, const std::string &details, void *sender) {
  base::RecMutexLock lock(_message_mutex);
  Message msg;
  msg.type = ProgressMsg;
  msg.text = message;
  msg.detail = details;
  msg.timestamp = time(NULL);

  // calculate the actual progress percentage
  if (!_progress_step_stack.empty()) {
    std::vector<std::pair<float, float> >::reverse_iterator rit;
    for (rit = _progress_step_stack.rbegin(); rit != _progress_step_stack.rend(); ++rit)
      percentage = (*rit).first + ((*rit).second - (*rit).first) * percentage;
  }
  msg.progress = percentage;
  handle_message(msg, sender);

  // Progress messages are often sent without a real message, just to trigger some actions.
  // Don't log empty lines.
  //  if (message.size() > 0)
  //    log_debug3("%s\t%s", message.c_str(), details.c_str());
}

void GRT::send_verbose(const std::string &message, void *sender) {
  base::RecMutexLock lock(_message_mutex);
  Message msg;
  msg.type = VerboseMsg;
  msg.text = message;
  msg.detail = "";
  msg.timestamp = time(NULL);
  msg.progress = 0.0;
  handle_message(msg, sender);

  logDebug2("%s", message.c_str());
}

void GRT::send_output(const std::string &message, void *sender) {
  base::RecMutexLock lock(_message_mutex);
  Message msg;
  msg.type = OutputMsg;
  msg.text = message;
  msg.detail = "";
  msg.timestamp = time(NULL);
  msg.progress = 0.0;
  handle_message(msg, sender);

  // Log send_output only when verbose is on to avoid duplicate prints to stdout, when logged text also goes to
  // stderr/out
  // TODO: fix the actual cause for sending duplicates and remove _verbose (doesn't fit to our logging).
  if (_verbose)
    logDebug("%s", message.c_str());
}
