/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _GRT_MANAGER_H_
#define _GRT_MANAGER_H_

#include <grtpp.h>

#include "base/trackable.h"
#include "base/threading.h"
#include "common.h"
#include "grt_dispatcher.h"
#include "grt_shell.h"
#include "grt_value_inspector.h"
#include "grt_message_list.h"

#include "wbpublic_public_interface.h"

#include "plugin_manager.h"
#include <boost/signals2/connection.hpp>

namespace bec {

  class Clipboard;

  /** Manages a GRT context and other associated objects useful for a GRT shell and other apps.
   *
   * @ingroup begrt
   */ 
  class WBPUBLICBACKEND_PUBLIC_FUNC GRTManager : public base::trackable
  {
  public:
    struct Timer
    {
      boost::function<bool ()> slot;
      GTimeVal next_trigger;
      double interval;

      Timer(const boost::function<bool ()> &slot, double interval);

      bool trigger();

      double delay_for_next_trigger(const GTimeVal &now);
    };

  public:
    /** Constructor.
     * 
     * @param threaded enable threading
     * @param verbose enable verbose output
     */ 
    GRTManager(bool threaded = true, bool verbose = false);
    virtual ~GRTManager();

    static GRTManager *get_instance_for(grt::GRT *grt);

    void set_basedir(const std::string &path);
    std::string get_basedir() { return _basedir; }

    void set_datadir(const std::string &path);
    std::string get_data_file_path(const std::string &file);

    void set_user_datadir(const std::string &path);
    std::string get_user_datadir() { return _user_datadir; }

    std::string get_tmp_dir();
    std::string get_unique_tmp_subdir();
    void cleanup_tmp_dir();

    void set_module_extensions(const std::list<std::string> &extensions);

    void rescan_modules();
    int do_scan_modules(const std::string &path, const std::list<std::string> &exts, bool refresh);
    void scan_modules_grt(grt::GRT *grt, const std::list<std::string> &extensions, bool refresh);

    void set_clipboard(Clipboard *clipb);

    Clipboard *get_clipboard() { return _clipboard; }

    void set_search_paths(const std::string &module_sp,
                          const std::string &struct_sp,
                          const std::string &libraries_sp);
    
    void set_user_extension_paths(const std::string &user_module_path,
                                  const std::string &user_library_path,
                                  const std::string &user_script_path);
    
    std::string get_user_module_path() const { return _user_module_path; }
    std::string get_user_library_path() const { return _user_library_path; }
    std::string get_user_script_path() const { return _user_script_path; }

    // main window statusbar text
    void push_status_text(const std::string &message);
    void replace_status_text(const std::string &message);
    void pop_status_text();
    void set_status_slot(const boost::function<void (std::string)> &slot);

    void push_cancel_query_callback(const boost::function<bool ()> &slot);
    void pop_cancel_query_callback();
  public:
    GRTDispatcher::Ref get_dispatcher() const { return _dispatcher; };

    grt::GRT *get_grt() const { return _grt; };

    void initialize(bool init_python, const std::string &loader_module_path= "");
    bool initialize_shell(const std::string &shell_type);

    bool cancel_idle_tasks();
    void perform_idle_tasks();

    PluginManager *get_plugin_manager() const { return _plugin_manager; }

    bool is_threaded() { return _threaded; }
    bool in_main_thread();

    // shell
    ShellBE *get_shell();

    void execute_grt_task(const std::string &title,
                          const boost::function<grt::ValueRef (grt::GRT*)> &function,
                          const boost::function<void (grt::ValueRef)> &finished_cb);


    // message displaying (as dialogs)
    void show_error(const std::string &message, const std::string &detail, bool important= true);
    void show_warning(const std::string &title, const std::string &message, bool important= false);
    void show_message(const std::string &title, const std::string &message, bool important= false);


    MessageListStorage *get_messages_list();

    //
    void set_app_option_slots(const boost::function<grt::ValueRef (std::string)> &slot,
                              const boost::function<void (std::string, grt::ValueRef)> &set_slot);
    grt::ValueRef get_app_option(const std::string &name);
    std::string get_app_option_string(const std::string &name);
    long get_app_option_int(const std::string &name, long default_= 0);
    void set_app_option(const std::string &name, const grt::ValueRef &value);
    
    boost::signals2::connection run_once_when_idle(const boost::function<void ()> &func);
    boost::signals2::connection run_once_when_idle(base::trackable *owner, const boost::function<void ()> &func);

    void block_idle_tasks();
    void unblock_idle_tasks();

    Timer *run_every(const boost::function<bool ()> &slot, double seconds);
    void cancel_timer(Timer *timer);
    double delay_for_next_timeout();

    void set_timeout_request_slot(const boost::function<void ()> &slot);

    void flush_timers();

    void terminate() { _terminated= true; };
    bool terminated() { return _terminated; };
    void reset_termination() {_terminated= false; };

    void set_db_file_path(const std::string &db_file_path) { _db_file_path= db_file_path; }
    std::string get_db_file_path() { return _db_file_path; }

    bool has_unsaved_changes() { return _has_unsaved_changes; }
    void has_unsaved_changes(bool has_unsaved_changes) { _has_unsaved_changes= has_unsaved_changes; }

    // use for advisory locks on grt globals tree
    // ex: UI should not refresh layer and catalog trees while a plugin is running
    bool try_soft_lock_globals_tree();
    void soft_lock_globals_tree();
    void soft_unlock_globals_tree();
    bool is_globals_tree_locked();
    
    
  public:
    boost::function<void (bec::ArgumentPool&)> update_plugin_arguments_pool; // set by WBContext
    
    bec::MenuItemList get_plugin_context_menu_items(const std::list<std::string> &groups,
                                                    const bec::ArgumentPool &argument_pool);
    bool check_plugin_runnable(const app_PluginRef &plugin, const bec::ArgumentPool &argpool,
                               bool debug_output= false);
    
    void open_object_editor(const GrtObjectRef &object, bec::GUIPluginFlags flags = bec::NoFlags);
    
  protected:
    grt::GRT *_grt;
    bool _has_unsaved_changes;
    GRTDispatcher::Ref _dispatcher;
    base::Mutex _idle_mutex;
    base::Mutex _timer_mutex;


  public:
    void add_dispatcher(const bec::GRTDispatcher::Ref disp);
    void remove_dispatcher(const bec::GRTDispatcher::Ref disp);
  protected:
    typedef std::map<const GRTDispatcher::Ref, void*> DispatcherMap;
    DispatcherMap _disp_map;
    base::Mutex _disp_map_mutex;

    PluginManager *_plugin_manager;

    Clipboard *_clipboard;


    ShellBE *_shell;

    MessageListStorage *_messages_list;

    boost::function<void (std::string)> _status_text_slot;
    
    std::list<Timer*> _timers;
    std::set<Timer*> _cancelled_timers;
    boost::function<void ()> _timeout_request;

    // Using two signals to manage the idle tasks
    boost::signals2::signal<void ()> _idle_signals[2];
    int _current_idle_signal;
    
    int _idle_blocked;

    std::list<std::string> _module_extensions;

    std::string _basedir;
    std::string _datadir;
    std::string _user_datadir;
    std::string _module_pathlist;
    std::string _struct_pathlist;
    std::string _libraries_pathlist;
    std::string _db_file_path;

    std::string _user_module_path;
    std::string _user_library_path;
    std::string _user_script_path;

    boost::function<grt::ValueRef (std::string)> _get_app_option_slot;
    boost::function<void (std::string, grt::ValueRef)> _set_app_option_slot;
    
    bool _threaded;
    bool _verbose;

    int _globals_tree_soft_lock_count;
    
    virtual bool load_structs();
    virtual bool load_modules();
    virtual bool load_libraries();
    virtual bool init_module_loaders(const std::string &loader_module_path, bool init_python);

    bool init_loaders(const std::string &loader_module_path, bool init_python);
    
    void flush_shell_output();

  private:
    bool _terminated; // true if application termination was requested by the BE or a plugin.
    static std::map<grt::GRT*,GRTManager*> _instances;
    
    grt::ValueRef setup_grt(grt::GRT *grt);

    void shell_write(const std::string &text);

    void task_error_cb(const std::exception &error, const std::string &title);
  };

};

#endif /* _GRT_MANAGER_H_ */

