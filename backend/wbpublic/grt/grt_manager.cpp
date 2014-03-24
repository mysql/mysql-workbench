/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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


#include "stdafx.h"

#include <grtpp_module_python.h>
#include <grtpp_module_cpp.h>
#include <grtpp_module_lua.h>

#include <python_context.h>

#include "grt/grt_manager.h"
#include "base/threading.h"
#include "glib/gstdio.h"
#include "base/log.h"
#include "objimpl/ui/grt_PyObject_impl.h"

#include "base/notifications.h"
#include "base/file_functions.h"
#include "base/string_utilities.h"
#include "mforms/utilities.h"

using namespace grt;
using namespace bec;
using namespace base;

DEFAULT_LOG_DOMAIN("GRTManager");

static GThread *main_thread= 0;

std::map<grt::GRT*,GRTManager*> GRTManager::_instances;

static base::Mutex _instance_mutex;

static void init_all()
{
  if (!main_thread)
   {
    base::threading_init();
    main_thread= g_thread_self();
    if (!g_thread_supported())
      throw std::runtime_error("Could not initialize Glib thread support");
  }
}


GRTManager *create_grt_manager(bool threaded, bool verbose = false)
{
  return new GRTManager(threaded, verbose);
}


GRTManager::GRTManager(bool threaded, bool verbose)
: _has_unsaved_changes(false), _threaded(threaded), _verbose(verbose)
{
  _globals_tree_soft_lock_count= 0;

  _current_idle_signal = 0;

  init_all();

  _grt= new GRT();
  
  _grt->set_verbose(verbose);
  
  _terminated= false;
  _idle_blocked= false;
  _clipboard= 0;

  // add self to the mgr instances table asap, because the other objects
  // may need to call get_instance_for()
  {
    base::MutexLock _lock(_instance_mutex);
    _instances[_grt]= this;
  }
  
  _dispatcher.reset(new GRTDispatcher(_grt, _threaded, true));

  _shell= new ShellBE(this, _dispatcher.get());

  _plugin_manager= _grt->get_native_module<PluginManagerImpl>();

  _messages_list= new MessageListStorage(this);

  PythonContext::set_run_once_when_idle(boost::bind(&GRTManager::run_once_when_idle_, this, _1));
}

bool GRTManager::try_soft_lock_globals_tree()
{
  // returns true if lock count was 0 and then lock it
#if GLIB_CHECK_VERSION(2,32,0)
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


void GRTManager::soft_lock_globals_tree()
{
  g_atomic_int_add(&_globals_tree_soft_lock_count, 1);
}

void GRTManager::soft_unlock_globals_tree()
{
  g_atomic_int_add(&_globals_tree_soft_lock_count, -1);
}


bool GRTManager::is_globals_tree_locked()
{
  return g_atomic_int_get(&_globals_tree_soft_lock_count) != 0;
}

GRTManager *GRTManager::get_instance_for(GRT *grt)
{
  base::MutexLock lock(_instance_mutex);
  std::map<GRT*,GRTManager*>::iterator iter= _instances.find(grt);
  if (iter != _instances.end())
    return iter->second;
  return NULL;
}


void GRTManager::set_basedir(const std::string &path)
{
  if (!g_path_is_absolute(path.c_str()))
  {
    gchar *dir= g_get_current_dir();
    _basedir= make_path(dir, path);
    g_free(dir);
  }
  else
    _basedir= path;
}


void GRTManager::set_datadir(const std::string &path)
{
  if (!g_path_is_absolute(path.c_str()))
  {
    gchar *dir= g_get_current_dir();
    _datadir= make_path(dir, path);
    g_free(dir);
  }
  else
    _datadir= path;
}


std::string GRTManager::get_data_file_path(const std::string &file)
{
  return make_path(_datadir, file);
}


void GRTManager::set_user_datadir(const std::string &path)
{
  if (!g_path_is_absolute(path.c_str()))
  {
    gchar *dir= g_get_current_dir();
    _user_datadir= make_path(dir, path);
    g_free(dir);
  }
  else
    _user_datadir= path;
}


void GRTManager::set_module_extensions(const std::list<std::string> &extensions)
{
  _module_extensions= extensions;
}


void GRTManager::set_clipboard(Clipboard *clipb)
{
  _clipboard= clipb;
}


bool GRTManager::in_main_thread()
{
  if (main_thread == g_thread_self())
    return true;
  return false;
}


GRTManager::~GRTManager()
{
  {
    base::MutexLock _lock(_instance_mutex);
    _instances.erase(_grt);
  }  

  _dispatcher->shutdown();
  _dispatcher.reset();

  delete _shell;
  _shell = 0;
  delete _messages_list;
  _messages_list = 0;

  delete _grt;
  _grt = 0;

  for (std::list<Timer*>::iterator iter= _timers.begin(); iter != _timers.end(); ++iter)
    delete *iter;

}


void GRTManager::set_search_paths(const std::string &module_sp, 
                                  const std::string &struct_sp,
                                  const std::string &libraries_sp)
{
  _module_pathlist= module_sp;
  _struct_pathlist= struct_sp;
  _libraries_pathlist= libraries_sp;
}


void GRTManager::set_user_extension_paths(const std::string &user_module_path,
                                          const std::string &user_library_path,
                                          const std::string &user_script_path)
{
  _user_module_path= user_module_path;
  _user_library_path= user_library_path;
  _user_script_path= user_script_path;
  
  _module_pathlist= pathlist_prepend(_module_pathlist, user_module_path);
  _libraries_pathlist= pathlist_prepend(_libraries_pathlist, user_library_path);
}


ShellBE *GRTManager::get_shell()
{
  return _shell;
}


MessageListStorage *GRTManager::get_messages_list()
{
  return _messages_list;
}

void GRTManager::task_error_cb(const std::exception &error, const std::string &title)
{
  mforms::Utilities::show_error(title, error.what(), _("Close"));
}


void GRTManager::execute_grt_task(const std::string &title,
                                  const boost::function<grt::ValueRef (grt::GRT*)> &function,
                                  const boost::function<void (grt::ValueRef)> &finished_cb)
{
  GRTTask *task(new GRTTask(title, _dispatcher.get(), function));

  // connect finished_cb provided by caller (after ours)
  task->signal_finished()->connect(finished_cb);

  scoped_connect(task->signal_failed(),boost::bind(&GRTManager::task_error_cb, this, _1, title));

  _dispatcher->add_task(task);
}


void GRTManager::add_dispatcher(GRTDispatcher::Ref disp)
{
  MutexLock disp_map_mutex(_disp_map_mutex);
  _disp_map[disp];
}


void GRTManager::remove_dispatcher(GRTDispatcher *disp)
{
  MutexLock disp_map_mutex(_disp_map_mutex);
  for (DispMap::iterator i= _disp_map.begin(), end= _disp_map.end(); i != end; ++i)
  {
    if (i->first.get() == disp)
    {
      _disp_map.erase(i);
      break;
    }
  }
}


void GRTManager::show_error(const std::string &message, const std::string &detail, bool important)
{
  // If we're being called from the GRT thread, then raise a runtime error.
  if (main_thread == _dispatcher->get_thread())
    throw grt_runtime_error(message, detail);

  _shell->write_line("ERROR:" + message);
  if (!detail.empty())
    _shell->write_line("  " + detail);

  if (important)
    mforms::Utilities::show_error(message, detail, _("Close"));
}


void GRTManager::show_warning(const std::string &title, const std::string &message, bool important)
{
  _shell->write_line("WARNING: "+title);
  _shell->write_line("    "+message);
//XXX redo
//  if (important)
//    _warning_cb(title, message);
}


void GRTManager::show_message(const std::string &title, const std::string &message, bool important)
{
  _shell->write_line(title+": "+message);
//XXX redo
  //if (important)
  //  _message_cb(2, title, message);
}


void GRTManager::initialize(bool init_python, const std::string &loader_module_path)
{
  _dispatcher->start(_dispatcher);

  load_structs();

  init_module_loaders(loader_module_path, init_python);

#ifdef _WIN32
  add_python_module_dir(_grt, _basedir + "\\python");
  add_python_module_dir(_grt, _basedir + "\\modules");
#elif !defined(__APPLE__)
  std::vector<std::string> path(base::split(_module_pathlist, G_SEARCHPATH_SEPARATOR_S));
  for (std::vector<std::string>::const_iterator i= path.begin(); i != path.end(); ++i)
    add_python_module_dir(_grt, *i);
#endif

  pyobject_initialize();

  load_libraries();
    
  load_modules();
}


bool GRTManager::initialize_shell(const std::string &shell_type)
{
  if (!_shell->setup(shell_type.empty() ? grt::LanguageLua : shell_type))
  {
    g_warning("Could not initialize GRT shell of type '%s'", shell_type.c_str());
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
bool GRTManager::cancel_idle_tasks()
{
//   { TODO
//     MutexLock disp_map_mutex(_disp_map_mutex);
//     for (DispMap::iterator i = _disp_map.begin(), i_end = _disp_map.end(); i != i_end; ++i)
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

static void nothing() {}

void GRTManager::perform_idle_tasks()
{
  // flush the dispatcher callback queue
  {
    DispMap copy;
    
    {
      MutexLock disp_map_mutex(_disp_map_mutex);
      copy= _disp_map;
    }
    
    for (DispMap::iterator i= copy.begin(), i_end= copy.end(); i != i_end; ++i)
      i->first->flush_pending_callbacks();
  }

  if (_shell)
  {
    // flush the shell output buffer
    _shell->flush_shell_output();
  }

  if (!_idle_blocked)
  {
    if (!_idle_signals[_current_idle_signal].empty())
    {
      block_idle_tasks(); // TODO: that's not thread safe, why isn't the idle mutex used.

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
      _idle_signals[signal_to_emit].connect(boost::bind(nothing));
      unblock_idle_tasks();
    }
  }
}


boost::signals2::connection GRTManager::run_once_when_idle(const boost::function<void ()> &slot)
{
  if (!slot)
    throw std::invalid_argument("Adding null slot for idle");

  MutexLock lock(_idle_mutex);
  return _idle_signals[_current_idle_signal].connect(slot);
}


void GRTManager::run_once_when_idle(base::trackable *owner, const boost::function<void ()> &slot)
{
  if (!slot)
    throw std::invalid_argument("Adding null slot for idle");
  MutexLock lock(_idle_mutex);
  owner->track_connection(_idle_signals[_current_idle_signal].connect(slot));
}



void GRTManager::block_idle_tasks()
{
  _idle_blocked++;
}


void GRTManager::unblock_idle_tasks()
{
  _idle_blocked--;
}


GRTManager::Timer::Timer(const boost::function<bool ()> &slot, double interval)
{
  this->slot= slot;
  this->interval= interval;

  g_get_current_time(&next_trigger);
  g_time_val_add(&next_trigger, (glong)(interval*G_USEC_PER_SEC));
}


bool GRTManager::Timer::trigger()
{
  bool flag= slot?slot():false;

  g_get_current_time(&next_trigger);
  g_time_val_add(&next_trigger, (glong)(interval*G_USEC_PER_SEC));

  return flag;
}


double GRTManager::Timer::delay_for_next_trigger(const GTimeVal &now)
{
  double delay;

  delay= next_trigger.tv_sec - now.tv_sec;
  delay+= (double)(next_trigger.tv_usec - now.tv_usec) / G_USEC_PER_SEC;

  return delay;
}


GRTManager::Timer *GRTManager::run_every(const boost::function<bool ()> &slot, double seconds)
{
  Timer *timer= new Timer(slot, seconds);
  GTimeVal now;

  g_get_current_time(&now);

  double delay= timer->delay_for_next_trigger(now);

  {
    base::MutexLock lock(_timer_mutex);

    // insert it in order of delay for next trigger
    bool inserted= false;
    for (std::list<Timer*>::iterator iter= _timers.begin(); iter != _timers.end(); ++iter)
    {
      if ((*iter)->delay_for_next_trigger(now) > delay)
      {
        _timers.insert(iter, timer);
        inserted= true;
        break;
      }
    }
    if (!inserted)
      _timers.push_back(timer);

  }
  _timeout_request();

  return timer;
}


void GRTManager::cancel_timer(GRTManager::Timer *timer)
{
  base::MutexLock lock(_timer_mutex);
  std::list<Timer*>::iterator it= std::find(_timers.begin(), _timers.end(), timer);
  if (it != _timers.end())
  {
    delete *it;
    _timers.erase(it);
  }
  else
    _cancelled_timers.insert(timer);
  // if the timer is not in the timers list, then it may be getting executed,
  // so add it to a list of timers so it doesn't get readded to the timers list
}


void GRTManager::flush_timers()
{
  GTimeVal now;
  g_get_current_time(&now);

  std::list<Timer*> triggered;

  // first get a list of timers that trigger now

  std::list<Timer*>::iterator next, iter= _timers.begin();
  {
    base::MutexLock lock(_timer_mutex);
    while (iter != _timers.end())
    {
      next= iter;
      ++next;

      if ((*iter)->delay_for_next_trigger(now) > 0.00001)
        break;

      triggered.push_back(*iter);
      _timers.erase(iter);

      iter= next;
    }
  }

  // after this point it's impossible for the timer to be cancelled
  // because it is not in the timers list anymore

  // and then trigger and reinsert them to the timer list
  for (iter= triggered.begin(); iter != triggered.end(); ++iter)
  {
    // the timer can get cancelled at this point or later, if it happens after
    // its executed, then it will be deleted in the next iteration

    if ((*iter)->trigger()) // if callback returns false, don't readd it
    {
      double delay= (*iter)->delay_for_next_trigger(now);

      base::MutexLock lock(_timer_mutex);

      if (_cancelled_timers.find(*iter) == _cancelled_timers.end())
      {
        // insert it in order of delay for next trigger
        bool inserted= false;
        for (std::list<Timer*>::iterator jter= _timers.begin(); jter != _timers.end(); ++jter)
        {
          if ((*jter)->delay_for_next_trigger(now) > delay)
          {
            _timers.insert(jter, *iter);
            inserted= true;
            break;
          }
        }
        if (!inserted)
          _timers.push_back(*iter);
      }
      else
        delete *iter;

    }
    else
    {
      base::MutexLock lock(_timer_mutex);
      delete *iter;
    }
  }
  base::MutexLock lock(_timer_mutex);
  _cancelled_timers.clear();
}


double GRTManager::delay_for_next_timeout()
{
  double delay= -1;

  base::MutexLock lock(_timer_mutex);
  if (!_timers.empty())
  {
    GTimeVal now;
    g_get_current_time(&now);
    delay= _timers.front()->delay_for_next_trigger(now);
    if (delay < 0)
      delay= 0.0;
  }

  return delay;
}


void GRTManager::set_timeout_request_slot(const boost::function<void ()> &slot)
{
  _timeout_request= slot;
}


bool GRTManager::load_structs()
{
  if (_verbose)
    _shell->write_line(_("Loading struct definitions..."));

  int c, count= 0;
  gchar **paths= g_strsplit(_struct_pathlist.c_str(), G_SEARCHPATH_SEPARATOR_S, 0);

  for (int i= 0; paths[i]; i++)
  {
    if (g_file_test(paths[i], G_FILE_TEST_IS_DIR))
    {
      if (_verbose)
        _shell->writef(_("Looking for struct files in '%s'.\n"), paths[i]);
      
      try {
        c= _grt->scan_metaclasses_in(paths[i]);

        count+= c;
      } catch (std::exception &exc) {
        _shell->writef(_("Could not load structs from '%s': %s\n"),
                       paths[i], exc.what());
      }
    }
  }

  _grt->end_loading_metaclasses();

  _shell->writef(_("Registered %i GRT classes.\n"), count);

  g_strfreev(paths);

  return false;
}



bool GRTManager::init_module_loaders(const std::string &loader_module_path, bool init_python)
{
  if (_verbose)
    _shell->write_line(_("Initializing Loaders..."));
  if (!init_loaders(loader_module_path, init_python))
    _shell->write_line(_("Failed initializing Loaders."));
  
  return true;
}


bool GRTManager::load_libraries()
{
  gchar **paths= g_strsplit(_libraries_pathlist.c_str(), G_SEARCHPATH_SEPARATOR_S, 0);
  for (size_t i= 0; paths[i]; i++)
  {
    #ifdef _WIN32
    GDir *dir= g_dir_open_utf8(paths[i], 0, NULL);
    #else
    GDir *dir= g_dir_open(paths[i], 0, NULL);
    #endif

    if (dir)
    {
      const gchar *fname;
      while ((fname= g_dir_read_name(dir)))
      {
        gchar *path;

        path= g_strdup_printf("%s%c%s", paths[i], G_DIR_SEPARATOR, fname);
        if (g_file_test(path, G_FILE_TEST_IS_REGULAR))
        {
          ModuleLoader *loader= _grt->get_module_loader_for_file(fname);
          
          if (loader)
          {
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


bool GRTManager::load_modules()
{
  if (_verbose)
    _shell->write_line(_("Loading modules..."));
  scan_modules_grt(_grt, _module_extensions, false);
  
  return true;
}


void GRTManager::rescan_modules()
{
  load_modules();
}


bool GRTManager::init_loaders(const std::string &loader_module_path, bool init_python)
{
  try
  {
    _grt->add_module_loader(new LuaModuleLoader(_grt));
    if (_verbose) _shell->write_line(_("Lua loader initialized."));
  }
  catch (std::exception &exc)
  {
    _shell->write_line(strfmt("Error initializing Lua loader: %s", exc.what()));
  }
  
  if (init_python)
  {
    try
    {
      if (grt::init_python_support(_grt, loader_module_path))
      {
        if (_verbose) _shell->write_line(_("Python loader initialized."));
      }
    }
    catch (std::exception &exc)
    {
      _shell->write_line(strfmt("Error initializing Python loader: %s", exc.what()));
    }
  }

  return true;
}


int GRTManager::do_scan_modules(const std::string &path, const std::list<std::string> &extensions, bool refresh)
{
  int c;

  if (!g_file_test(path.c_str(), G_FILE_TEST_IS_DIR))
  {
 //   if (_verbose)
//      _grt->send_output(strfmt(_("Skipping non-existent module directory '%s'.\n"), path.c_str()));
    return 0;
  }

  if (_verbose)
    _grt->send_output(strfmt(_("Looking for modules in '%s'.\n"), path.c_str()));
  
  try
  {
    c= _grt->scan_modules_in(path, extensions.empty() ? _module_extensions : extensions, refresh);
  }
  catch (std::exception &exc)
  {
    _grt->send_output(strfmt(_("Error scanning for modules: %s\n"),
                             exc.what()));
    
    return 0;
  }

  if (_verbose)
    _grt->send_output(strfmt(_("%i modules found\n"), c));

  return c;
}


void GRTManager::scan_modules_grt(grt::GRT *grt, const std::list<std::string> &extensions, bool refresh)
{
  int c, count= 0;
  gchar **paths= g_strsplit(_module_pathlist.c_str(), G_SEARCHPATH_SEPARATOR_S, 0);
  
  for (int i= 0; paths[i]; i++)
  {
    c= do_scan_modules(paths[i], extensions, refresh);
    if (c >= 0)
      count+= c;
  }

  _grt->end_loading_modules();
  
  _shell->writef(_("Registered %i modules (from %i files).\n"),
                 _grt->get_modules().size(), count);

  g_strfreev(paths);
}


void GRTManager::set_app_option_slots(const boost::function<grt::ValueRef (std::string)> &slot,
                                      const boost::function<void (std::string, grt::ValueRef)> &set_slot)
{
  _get_app_option_slot= slot;
  _set_app_option_slot= set_slot;
}


void GRTManager::set_app_option(const std::string &name, const grt::ValueRef &value)
{
  if (_set_app_option_slot)
    _set_app_option_slot(name, value);
}


grt::ValueRef GRTManager::get_app_option(const std::string &name)
{
  if (_get_app_option_slot)
    return _get_app_option_slot(name);
  return grt::ValueRef();
}


std::string GRTManager::get_app_option_string(const std::string &name)
{
  grt::ValueRef value(get_app_option(name));
  if (value.is_valid() && grt::StringRef::can_wrap(value))
    return *grt::StringRef::cast_from(value);
  return "";  
}


long GRTManager::get_app_option_int(const std::string &name, long default_)
{
  grt::ValueRef value(get_app_option(name));
  if (value.is_valid() && grt::IntegerRef::can_wrap(value))
    return *grt::IntegerRef::cast_from(value);
  return default_;
}

std::string GRTManager::get_tmp_dir()
{
  std::string res;
#ifdef _WIN32
  res.append(g_get_tmp_dir()).append("/MySQL Workbench/");
#else
  res.append(g_get_tmp_dir()).append("/mysql-workbench.").append(g_get_user_name()).append("/");
#endif
  g_mkdir(res.c_str(), 0700);
  return res;
}


std::string GRTManager::get_unique_tmp_subdir()
{
  for (;;)
  {
    std::string unique_name= get_guid(); 
    // get_guid returns upper-lower case combined string (base64), which could potentially lead
    // to duplicate dirnames in case-insensitive filesystems
  
    std::string path = get_tmp_dir().append(unique_name).append("/");
    if (!g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
      return path;
  }
  return "";
}


void GRTManager::cleanup_tmp_dir()
{
  (void) base_rmdir_recursively(get_tmp_dir().c_str());
}

void GRTManager::push_status_text(const std::string &message)
{
  _status_text_slot(message);
}


void GRTManager::replace_status_text(const std::string &message)
{
  // pop_status_text();
  push_status_text(message);
}


void GRTManager::pop_status_text()
{
  _status_text_slot("");
}


void GRTManager::set_status_slot(const boost::function<void (std::string)> &slot)
{
  _status_text_slot= slot;
}





//--------------------------------------------------------------------------------------------------

struct sortpluginbyrating
{
  bool operator ()(const app_PluginRef &a, const app_PluginRef &b) const
  {
    return a->rating() < b->rating();
  }
};

bec::MenuItemList GRTManager::get_plugin_context_menu_items(const std::list<std::string> &groups,
                                                            const bec::ArgumentPool &argument_pool)
{
  // get all plugins in wanted groups
  std::vector<app_PluginRef> plugins;
  
  for (std::list<std::string>::const_iterator group= groups.begin(); group != groups.end(); ++group)
  {
    std::vector<app_PluginRef> tmp(get_plugin_manager()->get_plugins_for_group(*group));
    
    for (std::vector<app_PluginRef>::const_iterator pl= tmp.begin(); pl != tmp.end(); ++pl)
    {
      if (std::find(plugins.begin(), plugins.end(), *pl) == plugins.end())
      {
        plugins.push_back(*pl);
      }
    }
  }
  // sort by rating
  std::sort(plugins.begin(), plugins.end(), sortpluginbyrating());
  
  bec::MenuItemList items;
  // filter by available arguments
  for (std::vector<app_PluginRef>::const_iterator pl= plugins.begin(); pl != plugins.end(); ++pl)
  {
    //if (check_plugin_runnable(*pl, argument_pool))
    {  
      bec::MenuItem item;
      item.caption= *(*pl)->caption() + ((*pl)->pluginType()=="gui"?"...":"");
      item.name= "plugin:"+*(*pl)->name();
      item.enabled= check_plugin_runnable(*pl, argument_pool);
      if (item.caption.empty())
        item.caption= item.name;
      item.type= MenuAction;
      items.push_back(item);
    }
  }
  return items;
}

//--------------------------------------------------------------------------------------------------

bool GRTManager::check_plugin_runnable(const app_PluginRef &plugin, const bec::ArgumentPool &argpool,
                                       bool debug_output)
{
  bool debug_args = strstr(plugin->name().c_str(), "-debugargs-") != 0 || debug_output;
  
  for (size_t c= plugin->inputValues().count(), i= 0; i < c; i++)
  {
    app_PluginInputDefinitionRef pdef(plugin->inputValues()[i]);
    std::string searched_key;
    if (!argpool.find_match(pdef, searched_key, false).is_valid())
    {
      if (debug_args)
      {
        _grt->send_output(base::strfmt("Debug: Plugin %s cannot execute because argument %s is not available\n",
                                       plugin->name().c_str(), searched_key.c_str()));
        _grt->send_output("Debug: Available arguments:\n");
        argpool.dump_keys(boost::bind(&grt::GRT::send_output, _grt, _1, (void*)0));
      }
      return false;
    }
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void GRTManager::open_object_editor(const GrtObjectRef &object, bec::GUIPluginFlags flags)
{
  try 
  {
    grt::BaseListRef args(_grt, AnyType);
    args.ginsert(object);
    
    app_PluginRef plugin(_plugin_manager->select_plugin_for_input("catalog/Editors", args));
    if (!plugin.is_valid())
      plugin= _plugin_manager->select_plugin_for_input("model/Editors", args);
    
    if (plugin.is_valid())
      _plugin_manager->open_gui_plugin(plugin, args, flags);
    else
    {
      log_error("No suitable editor found for object of type '%s'.", 
                object.get_metaclass()->get_attribute("caption").c_str());

      mforms::Utilities::show_error(_("Edit Object"), 
                                    strfmt(_("No suitable editor found for object of type '%s'."), 
                                           object.get_metaclass()->get_attribute("caption").c_str()), 
                                    "OK");
    }
  }
  catch (grt::grt_runtime_error &exc) 
  {
    log_error("Exception in Open object editor: %s\n%s", exc.what(), exc.detail.c_str());

    mforms::Utilities::show_error(_("Edit Object"), 
                                  strfmt("%s\n%s", exc.what(), exc.detail.c_str()),
                                  "OK");
  }
  catch (std::exception &exc) 
  {
    log_exception("Open object editor", exc);
    mforms::Utilities::show_error(_("Edit Object"), 
                                  strfmt("%s", exc.what()),
                                  "OK");    
  }
}

