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

#include "base/threading.h"
#include "base/log.h"
#include "base/file_utilities.h"

#include "grtpp_module_python.h"
#include "grtpp_module_cpp.h"

#include "python_context.h"

#include "glib/gstdio.h"
#include "objimpl/wrapper/grt_PyObject_impl.h"

#include "base/notifications.h"
#include "base/file_functions.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "mforms/utilities.h"

#include "grt/grt_manager.h"

using namespace grt;
using namespace bec;
using namespace base;

DEFAULT_LOG_DOMAIN("GRTManager");

static GThread *main_thread = nullptr;

static void init_all() {
  if (main_thread == nullptr) {
    main_thread = g_thread_self();
  }
}

GRTManager::GRTManager(bool threaded) : _has_unsaved_changes(false), _threaded(threaded), _verbose(false) {
  _grt = grt::GRT::get();
  _globals_tree_soft_lock_count = 0;

  _current_idle_signal = 0;

  init_all();

  _grt->set_verbose(_verbose);

  _terminated = false;
  _idle_blocked = false;
  _clipboard = 0;

  _dispatcher = GRTDispatcher::create_dispatcher(_threaded, true);
  _shell = new ShellBE(_dispatcher);
  _plugin_manager = _grt->get_native_module<PluginManagerImpl>();
  _messages_list = new MessageListStorage(this);
}

GRTManager::Ref GRTManager::get() {
  static GRTManager::Ref instance(new GRTManager(true));
  return instance;
}

void GRTManager::setVerbose(bool verbose) {
  _verbose = verbose;
  _grt->set_verbose(_verbose);
}

bool GRTManager::try_soft_lock_globals_tree() {
// returns true if lock count was 0 and then lock it
#if GLIB_CHECK_VERSION(2, 32, 0)
  if (g_atomic_int_add(&_globals_tree_soft_lock_count, 1) == 0)
    return true;
#else
  if (g_atomic_int_exchange_and_add(&_globals_tree_soft_lock_count, 1) == 0)
    return true;
#endif
  // lock failed, decrement it back
  g_atomic_int_add(&_globals_tree_soft_lock_count, -1);
  return false;
}

void GRTManager::soft_lock_globals_tree() {
  g_atomic_int_add(&_globals_tree_soft_lock_count, 1);
}

void GRTManager::soft_unlock_globals_tree() {
  g_atomic_int_add(&_globals_tree_soft_lock_count, -1);
}

bool GRTManager::is_globals_tree_locked() {
  return g_atomic_int_get(&_globals_tree_soft_lock_count) != 0;
}

void GRTManager::set_basedir(const std::string &path) {
  if (!g_path_is_absolute(path.c_str())) {
    gchar *dir = g_get_current_dir();
    _basedir = base::makePath(dir, path);
    g_free(dir);
  } else
    _basedir = path;
}

void GRTManager::set_datadir(const std::string &path) {
  if (!g_path_is_absolute(path.c_str())) {
    gchar *dir = g_get_current_dir();
    _datadir = base::makePath(dir, path);
    g_free(dir);
  } else
    _datadir = path;
}

std::string GRTManager::get_data_file_path(const std::string &file) {
  return base::makePath(_datadir, file);
}

void GRTManager::set_user_datadir(const std::string &path) {
  if (!g_path_is_absolute(path.c_str())) {
    gchar *dir = g_get_current_dir();
    _user_datadir = base::makePath(dir, path);
    g_free(dir);
  } else
    _user_datadir = path;
}

void GRTManager::set_module_extensions(const std::list<std::string> &extensions) {
  _module_extensions = extensions;
}

void GRTManager::set_clipboard(Clipboard *clipb) {
  _clipboard = clipb;
}

bool GRTManager::in_main_thread() {
  if (main_thread == g_thread_self())
    return true;
  return false;
}

GRTManager::~GRTManager() {
  _dispatcher->shutdown();
  _dispatcher.reset();

  delete _shell;
  _shell = 0;
  delete _messages_list;
  _messages_list = 0;

  for (std::list<Timer *>::iterator iter = _timers.begin(); iter != _timers.end(); ++iter)
    delete *iter;
}

void GRTManager::set_search_paths(const std::string &module_sp, const std::string &struct_sp,
                                  const std::string &libraries_sp) {
  _module_pathlist = module_sp;
  _struct_pathlist = struct_sp;
  _libraries_pathlist = libraries_sp;
}

void GRTManager::set_user_extension_paths(const std::string &user_module_path, const std::string &user_library_path,
                                          const std::string &user_script_path) {
  _user_module_path = user_module_path;
  _user_library_path = user_library_path;
  _user_script_path = user_script_path;

  _module_pathlist = base::pathlistPrepend(_module_pathlist, user_module_path);
  _libraries_pathlist = base::pathlistPrepend(_libraries_pathlist, user_library_path);
}

ShellBE *GRTManager::get_shell() {
  return _shell;
}

MessageListStorage *GRTManager::get_messages_list() {
  return _messages_list;
}

void GRTManager::task_error_cb(const std::exception &error, const std::string &title) {
  mforms::Utilities::show_error(title, error.what(), _("Close"));
}

void GRTManager::execute_grt_task(const std::string &title, const std::function<grt::ValueRef()> &function,
                                  const std::function<void(grt::ValueRef)> &finished_cb) {
  GRTTask::Ref task = GRTTask::create_task(title, _dispatcher, function);

  // connect finished_cb provided by caller (after ours)
  task->signal_finished()->connect(finished_cb);

  scoped_connect(task->signal_failed(), std::bind(&GRTManager::task_error_cb, this, std::placeholders::_1, title));

  _dispatcher->add_task(task);
}

void GRTManager::add_dispatcher(const GRTDispatcher::Ref dispatcher) {
  if (_dispatcher != dispatcher) {
    MutexLock disp_map_mutex(_disp_map_mutex);
    _disp_map[dispatcher];
  }
}

void GRTManager::remove_dispatcher(const GRTDispatcher::Ref dispatcher) {
  MutexLock disp_map_mutex(_disp_map_mutex);
  if (_disp_map.find(dispatcher) != _disp_map.end())
    _disp_map.erase(dispatcher);
}

void GRTManager::show_error(const std::string &message, const std::string &detail, bool important) {
  // If we're being called from the GRT thread, then raise a runtime error.
  if (main_thread == _dispatcher->get_thread())
    throw grt_runtime_error(message, detail);

  _shell->write_line("ERROR:" + message);
  if (!detail.empty())
    _shell->write_line("  " + detail);

  if (important)
    mforms::Utilities::show_error(message, detail, _("Close"));
}

void GRTManager::show_warning(const std::string &title, const std::string &message, bool important) {
  _shell->write_line("WARNING: " + title);
  _shell->write_line("    " + message);
  // XXX redo
  //  if (important)
  //    _warning_cb(title, message);
}

void GRTManager::show_message(const std::string &title, const std::string &message, bool important) {
  _shell->write_line(title + ": " + message);
  // XXX redo
  // if (important)
  //  _message_cb(2, title, message);
}

void GRTManager::cleanUpAndReinitialize() {
  _dispatcher->shutdown();
  _dispatcher.reset();

  delete _shell;
  _shell = 0;
  delete _messages_list;
  _messages_list = 0;

  base::MutexLock lock(_timer_mutex);
  for (auto it: _timers) {
    delete it;
  }
  _timers.clear();

  for (auto it: _cancelled_timers) {
    delete it;
  }
  _cancelled_timers.clear();

  _dispatcher = GRTDispatcher::create_dispatcher(_threaded, true);
  _shell = new ShellBE(_dispatcher);
  _plugin_manager = _grt->get_native_module<PluginManagerImpl>();
  _messages_list = new MessageListStorage(this);
}

void GRTManager::initialize(bool init_python, const std::string &loader_module_path) {
  _dispatcher->start();

  load_structs();

  init_module_loaders(loader_module_path, init_python);

#ifdef _MSC_VER
  add_python_module_dir(_basedir + "\\python");
  add_python_module_dir(_basedir + "\\modules");
#elif __APPLE__
  add_python_module_dir(_basedir + "/plugins");
#else
  std::vector<std::string> path(base::split(_module_pathlist, G_SEARCHPATH_SEPARATOR_S));
  for (std::vector<std::string>::const_iterator i = path.begin(); i != path.end(); ++i)
    add_python_module_dir(*i);

#endif

  pyobject_initialize();

  load_libraries();

  load_modules();
}

bool GRTManager::initialize_shell(const std::string &shell_type) {
  if (!_shell->setup(shell_type.empty() ? grt::LanguagePython : shell_type)) {
    logWarning("Could not initialize GRT shell of type '%s'\n", shell_type.c_str());
    return false;
  }
  return true;
}

/**
 * Cancels all pending idle tasks. Useful if their execution is no longer necessary and can even cause
 * a crash because the used objects are going soon.
 * Returns true if the task could be completed, false if the manager is currently in in idle execution.
 * Warning: canceling idle tasks unconditionally might lead to other problems, so use with extreme care.
 */
bool GRTManager::cancel_idle_tasks() {
  //   { TODO
  //     MutexLock disp_map_mutex(_disp_map_mutex);
  //     for (DispatcherMap::iterator i = _disp_map.begin(), i_end = _disp_map.end(); i != i_end; ++i)
  //       i->first->cancel_all_tasks();
  //   }

  if (_idle_blocked)
    return false;

  block_idle_tasks(); // TODO: use idle mutex.
  MutexLock lock(_idle_mutex);
  _current_idle_signal = 0;
  _idle_signals[0].disconnect_all_slots();
  _idle_signals[1].disconnect_all_slots();

  unblock_idle_tasks();

  return true;
}

static void nothing() {
}

void GRTManager::perform_idle_tasks() {
  // flush the dispatcher callback queue
  {
    DispatcherMap copy;

    {
      MutexLock disp_map_mutex(_disp_map_mutex);
      copy = _disp_map;
    }

    // We need to call main general dispatcher as it's not on the dispatcher list.
    if (_dispatcher)
      _dispatcher->flush_pending_callbacks();

    for (DispatcherMap::iterator i = copy.begin(), i_end = copy.end(); i != i_end; ++i)
      i->first->flush_pending_callbacks();
  }

  if (_shell) {
    // flush the shell output buffer
    _shell->flush_shell_output();
  }
  bool locked = _idle_task_blocker_mutex.tryLock();
  if (locked) {
    try {
      if (!_idle_blocked) {
        if (!_idle_signals[_current_idle_signal].empty()) {
          block_idle_tasks();

          int signal_to_emit = 0;
          {
            MutexLock lock(_idle_mutex);
            signal_to_emit = _current_idle_signal;
            _current_idle_signal = _current_idle_signal ? 0 : 1;
          }

          _idle_signals[signal_to_emit]();

          _idle_signals[signal_to_emit].disconnect_all_slots();

          // XXX disconnect_all_slots() will somehow leave bound functions hanging around until the signal is
          // connected to something.. if they hold shared_refs to objects, those will be kept around until
          // the signal is connected again, which sounds like a bug.. so we just do a dummy connection to force
          // shared refs to be released immediately.. should investigate why is this happening at all
          // how to test: put a bp in ~DbSqlEditorForm() and close the SQL Editor... if it is deleted immediately,
          // it works as expected, if it only gets deleted after opening another editor, then its broken
          _idle_signals[signal_to_emit].connect(std::bind(nothing));
          unblock_idle_tasks();
        }
      }
    } catch (...) {
      unblock_idle_tasks();
      _idle_task_blocker_mutex.unlock();
      throw;
    }
    _idle_task_blocker_mutex.unlock();
  }
}

boost::signals2::connection GRTManager::run_once_when_idle(const std::function<void()> &slot) {
  if (!slot)
    throw std::invalid_argument("Adding null slot for idle");

  MutexLock lock(_idle_mutex);
  return _idle_signals[_current_idle_signal].connect(slot);
}

boost::signals2::connection GRTManager::run_once_when_idle(base::trackable *owner, const std::function<void()> &slot) {
  if (!slot)
    throw std::invalid_argument("Adding null slot for idle");
  MutexLock lock(_idle_mutex);
  boost::signals2::connection tmp(_idle_signals[_current_idle_signal].connect(slot));
  owner->track_connection(tmp);
  return tmp;
}

void GRTManager::block_idle_tasks() {
  _idle_blocked++;
}

void GRTManager::unblock_idle_tasks() {
  _idle_blocked--;
}

GRTManager::Timer::Timer(const std::function<bool()> &slot, double interval) {
  this->slot = slot;
  this->interval = interval;

  g_get_current_time(&next_trigger);
  g_time_val_add(&next_trigger, (glong)(interval * G_USEC_PER_SEC));
}

bool GRTManager::Timer::trigger() {
  bool flag = slot ? slot() : false;

  g_get_current_time(&next_trigger);
  g_time_val_add(&next_trigger, (glong)(interval * G_USEC_PER_SEC));

  return flag;
}

double GRTManager::Timer::delay_for_next_trigger(const GTimeVal &now) {
  double delay;

  delay = next_trigger.tv_sec - now.tv_sec;
  delay += (double)(next_trigger.tv_usec - now.tv_usec) / G_USEC_PER_SEC;

  return delay;
}

GRTManager::Timer *GRTManager::run_every(const std::function<bool()> &slot, double seconds) {
  Timer *timer = new Timer(slot, seconds);
  GTimeVal now;

  g_get_current_time(&now);

  double delay = timer->delay_for_next_trigger(now);

  {
    base::MutexLock lock(_timer_mutex);

    // insert it in order of delay for next trigger
    bool inserted = false;
    for (std::list<Timer *>::iterator iter = _timers.begin(); iter != _timers.end(); ++iter) {
      if ((*iter)->delay_for_next_trigger(now) > delay) {
        _timers.insert(iter, timer);
        inserted = true;
        break;
      }
    }
    if (!inserted)
      _timers.push_back(timer);
  }
  _timeout_request();

  return timer;
}

void GRTManager::cancel_timer(GRTManager::Timer *timer) {
  base::MutexLock lock(_timer_mutex);
  std::list<Timer *>::iterator it = std::find(_timers.begin(), _timers.end(), timer);
  if (it != _timers.end()) {
    delete *it;
    _timers.erase(it);
  } else
    _cancelled_timers.insert(timer);
  // if the timer is not in the timers list, then it may be getting executed,
  // so add it to a list of timers so it doesn't get readded to the timers list
}

void GRTManager::flush_timers() {
  GTimeVal now;
  g_get_current_time(&now);

  std::list<Timer *> triggered;

  // first get a list of timers that trigger now

  std::list<Timer *>::iterator next, iter = _timers.begin();
  {
    base::MutexLock lock(_timer_mutex);
    while (iter != _timers.end()) {
      next = iter;
      ++next;

      if ((*iter)->delay_for_next_trigger(now) > 0.00001)
        break;

      triggered.push_back(*iter);
      _timers.erase(iter);

      iter = next;
    }
  }

  // after this point it's impossible for the timer to be cancelled
  // because it is not in the timers list anymore

  // and then trigger and reinsert them to the timer list
  for (iter = triggered.begin(); iter != triggered.end(); ++iter) {
    // the timer can get cancelled at this point or later, if it happens after
    // its executed, then it will be deleted in the next iteration

    if ((*iter)->trigger()) // if callback returns false, don't readd it
    {
      double delay = (*iter)->delay_for_next_trigger(now);

      base::MutexLock lock(_timer_mutex);

      if (_cancelled_timers.find(*iter) == _cancelled_timers.end()) {
        // insert it in order of delay for next trigger
        bool inserted = false;
        for (std::list<Timer *>::iterator jter = _timers.begin(); jter != _timers.end(); ++jter) {
          if ((*jter)->delay_for_next_trigger(now) > delay) {
            _timers.insert(jter, *iter);
            inserted = true;
            break;
          }
        }
        if (!inserted)
          _timers.push_back(*iter);
      } else
        delete *iter;

    } else {
      base::MutexLock lock(_timer_mutex);
      delete *iter;
    }
  }
  base::MutexLock lock(_timer_mutex);
  _cancelled_timers.clear();
}

double GRTManager::delay_for_next_timeout() {
  double delay = -1;

  base::MutexLock lock(_timer_mutex);
  if (!_timers.empty()) {
    GTimeVal now;
    g_get_current_time(&now);
    delay = _timers.front()->delay_for_next_trigger(now);
    if (delay < 0)
      delay = 0.0;
  }

  return delay;
}

void GRTManager::set_timeout_request_slot(const std::function<void()> &slot) {
  _timeout_request = slot;
}

bool GRTManager::load_structs() {
  if (_verbose)
    _shell->write_line(_("Loading struct definitions..."));

  int c, count = 0;
  gchar **paths = g_strsplit(_struct_pathlist.c_str(), G_SEARCHPATH_SEPARATOR_S, 0);

  for (int i = 0; paths[i]; i++) {
    if (g_file_test(paths[i], G_FILE_TEST_IS_DIR)) {
      if (_verbose)
        _shell->writef(_("Looking for struct files in '%s'.\n"), paths[i]);

      try {
        c = _grt->scan_metaclasses_in(paths[i]);

        count += c;
      } catch (std::exception &exc) {
        _shell->writef(_("Could not load structs from '%s': %s\n"), paths[i], exc.what());
      }
    }
  }

  _grt->end_loading_metaclasses();

  _shell->writef(_("Registered %i GRT classes.\n"), count);

  g_strfreev(paths);

  return false;
}

bool GRTManager::init_module_loaders(const std::string &loader_module_path, bool init_python) {
  if (_verbose)
    _shell->write_line(_("Initializing Loaders..."));
  if (!init_loaders(loader_module_path, init_python))
    _shell->write_line(_("Failed initializing Loaders."));

  return true;
}

bool GRTManager::load_libraries() {
  gchar **paths = g_strsplit(_libraries_pathlist.c_str(), G_SEARCHPATH_SEPARATOR_S, 0);
  for (size_t i = 0; paths[i]; i++) {
    GDir *dir = g_dir_open(paths[i], 0, NULL);
    if (dir) {
      const gchar *fname;
      while ((fname = g_dir_read_name(dir))) {
        gchar *path;

        path = g_strdup_printf("%s%c%s", paths[i], G_DIR_SEPARATOR, fname);
        if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
          ModuleLoader *loader = _grt->get_module_loader_for_file(fname);

          if (loader) {
            if (_verbose)
              _shell->write_line(strfmt(_("Loading GRT library %s"), path));
            loader->load_library(path);
          }
        }
        g_free(path);
      }
      g_dir_close(dir);
    }
  }

  g_strfreev(paths);

  return true;
}

bool GRTManager::load_modules() {
  if (_verbose)
    _shell->write_line(_("Loading modules..."));
  scan_modules_grt(_module_extensions, false);

  return true;
}

void GRTManager::rescan_modules() {
  load_modules();
}

bool GRTManager::init_loaders(const std::string &loader_module_path, bool init_python) {
  if (init_python) {
    try {
      if (grt::init_python_support(loader_module_path)) {
        if (_verbose)
          _shell->write_line(_("Python loader initialized."));
      }
    } catch (std::exception &exc) {
      _shell->write_line(strfmt("Error initializing Python loader: %s", exc.what()));
    }
  }

  return true;
}

int GRTManager::do_scan_modules(const std::string &path, const std::list<std::string> &extensions, bool refresh) {
  int c;

  if (!g_file_test(path.c_str(), G_FILE_TEST_IS_DIR))
    return 0;

  if (_verbose)
    _grt->send_output(strfmt(_("Looking for modules in '%s'.\n"), path.c_str()));

  try {
    c = _grt->scan_modules_in(path, _basedir, extensions.empty() ? _module_extensions : extensions, refresh);
  } catch (std::exception &exc) {
    _grt->send_output(strfmt(_("Error scanning for modules: %s\n"), exc.what()));

    return 0;
  }

  if (_verbose)
    _grt->send_output(strfmt(_("%i modules found\n"), c));

  return c;
}

void GRTManager::scan_modules_grt(const std::list<std::string> &extensions, bool refresh) {
  int c, count = 0;
  gchar **paths = g_strsplit(_module_pathlist.c_str(), G_SEARCHPATH_SEPARATOR_S, 0);

  for (int i = 0; paths[i]; i++) {
    c = do_scan_modules(paths[i], extensions, refresh);
    if (c >= 0)
      count += c;
  }

  _grt->end_loading_modules();

  _shell->writef(_("Registered %i modules (from %i files).\n"), _grt->get_modules().size(), count);

  g_strfreev(paths);
}

void GRTManager::set_app_option_slots(const std::function<grt::ValueRef(std::string)> &slot,
                                      const std::function<void(std::string, grt::ValueRef)> &set_slot) {
  _get_app_option_slot = slot;
  _set_app_option_slot = set_slot;
}

void GRTManager::set_app_option(const std::string &name, const grt::ValueRef &value) {
  if (_set_app_option_slot)
    _set_app_option_slot(name, value);
}

grt::ValueRef GRTManager::get_app_option(const std::string &name) {
  if (_get_app_option_slot)
    return _get_app_option_slot(name);
  return grt::ValueRef();
}

std::string GRTManager::get_app_option_string(const std::string &name, std::string default_) {
  grt::ValueRef value(get_app_option(name));
  if (value.is_valid() && grt::StringRef::can_wrap(value))
    return *grt::StringRef::cast_from(value);
  return default_;
}

long GRTManager::get_app_option_int(const std::string &name, long default_) {
  grt::ValueRef value(get_app_option(name));
  if (value.is_valid() && grt::IntegerRef::can_wrap(value))
    return (long)*grt::IntegerRef::cast_from(value);
  return default_;
}

std::string GRTManager::get_tmp_dir() {
  // Add the current process ID to the path to make this unique.
  std::string res = g_get_tmp_dir();
  if (base::hasSuffix(res, "/") || base::hasSuffix(res, "\\"))
    res.resize(res.size() - 1);
  res += "/" + std::string("mysql-workbench-");
#ifdef _MSC_VER
  res += std::to_string(GetCurrentProcessId()) + "/";
#else
  res += std::to_string(::getpid()) + "/";
#endif
  base::create_directory(res, 0700, true);
  return res;
}

std::string GRTManager::get_unique_tmp_subdir() {
  for (;;) {
    std::string unique_name = get_guid();
    // get_guid returns upper-lower case combined string (base64), which could potentially lead
    // to duplicate dirnames in case-insensitive filesystems

    std::string path = get_tmp_dir().append(unique_name).append("/");
    if (!g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
      return path;
  }
  return "";
}

void GRTManager::cleanup_tmp_dir() {
  (void)base_rmdir_recursively(get_tmp_dir().c_str());
}

void GRTManager::push_status_text(const std::string &message) {
  _status_text_slot(message);
}

void GRTManager::replace_status_text(const std::string &message) {
  // pop_status_text();
  push_status_text(message);
}

void GRTManager::pop_status_text() {
  _status_text_slot("");
}

void GRTManager::set_status_slot(const std::function<void(std::string)> &slot) {
  _status_text_slot = slot;
}

//--------------------------------------------------------------------------------------------------

struct sortpluginbyrating {
  bool operator()(const app_PluginRef &a, const app_PluginRef &b) const {
    return a->rating() < b->rating();
  }
};

bec::MenuItemList GRTManager::get_plugin_context_menu_items(const std::list<std::string> &groups,
                                                            const bec::ArgumentPool &argument_pool) {
  // get all plugins in wanted groups
  std::vector<app_PluginRef> plugins;

  for (std::list<std::string>::const_iterator group = groups.begin(); group != groups.end(); ++group) {
    std::vector<app_PluginRef> tmp(get_plugin_manager()->get_plugins_for_group(*group));

    for (std::vector<app_PluginRef>::const_iterator pl = tmp.begin(); pl != tmp.end(); ++pl) {
      if (std::find(plugins.begin(), plugins.end(), *pl) == plugins.end()) {
        plugins.push_back(*pl);
      }
    }
  }
  // sort by rating
  std::sort(plugins.begin(), plugins.end(), sortpluginbyrating());

  bec::MenuItemList items;
  // filter by available arguments
  for (std::vector<app_PluginRef>::const_iterator pl = plugins.begin(); pl != plugins.end(); ++pl) {
    // if (check_plugin_runnable(*pl, argument_pool))
    {
      bec::MenuItem item;
      item.caption = *(*pl)->caption() + ((*pl)->pluginType() == "gui" ? "..." : "");
      item.internalName = "plugin:" + *(*pl)->name();
      item.accessibilityName = *(*pl)->accessibilityName();
      item.enabled = check_plugin_runnable(*pl, argument_pool);
      item.accessibilityName = (*pl)->accessibilityName();
      if (item.caption.empty())
        item.caption = item.accessibilityName;
      item.type = MenuAction;
      items.push_back(item);
    }
  }
  return items;
}

//--------------------------------------------------------------------------------------------------
bool GRTManager::check_plugin_runnable(const app_PluginRef &plugin, const bec::ArgumentPool &argpool,
                                       bool debug_output) {
  bool debug_args = strstr(plugin->name().c_str(), "-debugargs-") != 0 || debug_output;

  for (size_t c = plugin->inputValues().count(), i = 0; i < c; i++) {
    app_PluginInputDefinitionRef pdef(plugin->inputValues()[i]);
    std::string searched_key;
    if (!argpool.find_match(pdef, searched_key, false).is_valid()) {
      if (debug_args) {
        _grt->send_output(base::strfmt("Debug: Plugin %s cannot execute because argument %s is not available\n",
                                       plugin->name().c_str(), searched_key.c_str()));
        _grt->send_output("Debug: Available arguments:\n");

        argpool.dump_keys(
          std::bind<void>([this](const std::string &str) { _grt->send_output(str); }, std::placeholders::_1));
      }
      return false;
    }
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void GRTManager::open_object_editor(const GrtObjectRef &object, bec::GUIPluginFlags flags) {
  try {
    grt::BaseListRef args(AnyType);
    args.ginsert(object);

    app_PluginRef plugin(_plugin_manager->select_plugin_for_input("catalog/Editors", args));
    if (!plugin.is_valid())
      plugin = _plugin_manager->select_plugin_for_input("model/Editors", args);

    if (plugin.is_valid())
      _plugin_manager->open_gui_plugin(plugin, args, flags);
    else {
      logError("No suitable editor found for object of type '%s'.",
               object.get_metaclass()->get_attribute("caption").c_str());

      mforms::Utilities::show_error(_("Edit Object"), strfmt(_("No suitable editor found for object of type '%s'."),
                                                             object.get_metaclass()->get_attribute("caption").c_str()),
                                    "OK");
    }
  } catch (grt::grt_runtime_error &exc) {
    logError("Exception in Open object editor: %s\n%s", exc.what(), exc.detail.c_str());

    mforms::Utilities::show_error(_("Edit Object"), strfmt("%s\n%s", exc.what(), exc.detail.c_str()), "OK");
  } catch (std::exception &exc) {
    logException("Open object editor", exc);
    mforms::Utilities::show_error(_("Edit Object"), strfmt("%s", exc.what()), "OK");
  }
}
