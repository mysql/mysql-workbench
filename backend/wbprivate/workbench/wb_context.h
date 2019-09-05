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

#pragma once

#ifndef _MSC_VER
#include <vector>
#endif

#include "base/ui_form.h"
#include "grt/grt_manager.h"
#include "base/notifications.h"

#include "wb_context_names.h"

#include "grts/structs.workbench.h"
#include "wb_backend_public_interface.h"

#include "grtpp_undo_manager.h"

#include "mforms/utilities.h"
#include "base/trackable.h"
#include "base/threading.h"
#include "base/data_types.h"

#define WBContext_VERSION 5

#define WB_DBOBJECT_DRAG_TYPE "com.mysql.workbench.DatabaseObject"
#define WB_CONTROL_DRAG_TYPE "com.mysql.workbench.control"

const int ONE_MB = 1024*1024;

namespace mdc {
  class CanvasView;
  class CanvasItem;
};

namespace bec {
  class Clipboard;
  class IconManager;
};

class SqlEditorForm;

namespace wb {

  class WBContextUI;
  class WBContextModel;
  class WBContextSQLIDE;
  class WorkbenchImpl;
  class WBComponent;

  class ModelFile;

  class TunnelManager;

  enum RefreshType {
    RefreshNeeded, // Front end should schedule a refresh flush asap (usually called in worker thread).
    RefreshNothing,
    RefreshSchemaNoReload,
    RefreshNewDiagram,
    RefreshSelection,
    RefreshCloseEditor, // argument: object-id (close all if "")

    RefreshNewModel,

    RefreshOverviewNodeInfo,     // argument: node-id, overview ptr (bec::UIForm*)
    RefreshOverviewNodeChildren, // argument: node-id, overview ptr

    RefreshDocument,
    RefreshCloseDocument,
    RefreshZoom,
    RefreshTimer,

    RefreshFinishEdits // Force all ongoing edit operations (eg in TreeView cells) to be committed
  };

  struct MYSQLWBBACKEND_PUBLIC_FUNC WBPaperSize {
    std::string name;
    std::string caption;
    double width;
    double height;
    bool margins_set;
    double margin_top;
    double margin_bottom;
    double margin_left;
    double margin_right;
    std::string description;
  };

// basic toolbar names
#define WB_TOOLBAR_OPTIONS "options"

// main view types
#define WB_MAIN_VIEW_DB_QUERY "dbquery"

  class ModelDiagramForm;
  class FindDialogBE;

  enum PageOrientation { Landscape, Portrait };

  enum RequestInputFlag { InputPassword = (1 << 0) };

  struct MYSQLWBBACKEND_PUBLIC_FUNC WBFrontendCallbacks {
    // Args: type, title, file extensions
    std::function<std::string(std::string, std::string, std::string)> show_file_dialog;

    // Show some text in the application's status bar: must be thread-safe
    std::function<void(std::string)> show_status_text;

    // Open an editor
    // Args: grtmanager, module containing plugin, editor dll, editor class, edited object
    std::function<NativeHandle(grt::Module *, std::string, std::string, grt::BaseListRef, bec::GUIPluginFlags)>
      open_editor;
    // Show/Hide an editor
    // Args: editor handle (e.g: window handle)
    std::function<void(NativeHandle)> show_editor;
    std::function<void(NativeHandle)> hide_editor;

    // Execute a built-in command
    std::function<void(std::string)> perform_command;

    // Create a new diagram.
    std::function<mdc::CanvasView *(const model_DiagramRef &)> create_diagram;
    // Destroy a previously created canvas view
    std::function<void(mdc::CanvasView *)> destroy_view;
    // Signals the current view has been changed
    std::function<void(mdc::CanvasView *)> switched_view;

    // Open the named type of main view tab with the given form object. ownership is passed to frontend
    // Args: type (eg query), bec::UIForm*
    std::function<void(std::string, std::shared_ptr<bec::UIForm>)> create_main_form_view;
    std::function<void(bec::UIForm *)> destroy_main_form_view;

    // The tool for the view has been changed
    std::function<void(mdc::CanvasView *)> tool_changed;

    // Refresh interface
    std::function<void(RefreshType, std::string, NativeHandle)> refresh_gui;
    std::function<void(bool)> lock_gui;

    // Closes the application
    std::function<bool()> quit_application;
  };

  struct MYSQLWBBACKEND_PUBLIC_FUNC WBOptions {
    std::string basedir;
    std::string plugin_search_path;
    std::string struct_search_path;
    std::string module_search_path;
    std::string library_search_path;
    std::string cdbc_driver_search_path;
    std::string user_data_dir;
    std::string open_at_startup_type; // model, query, admin, script
    std::string open_at_startup;
    std::string open_connection;
    std::string run_at_startup; // script to be executed when started
    std::string run_language;   // language of the script in run_at_startup
    std::string binaryName;
    bool force_sw_rendering;
    bool force_opengl_rendering;
    bool verbose;
    bool quit_when_done;
    bool testing;         // True if we are currently running unit tests.
    bool init_python;     // True by default. Can be switched off for testing.
    bool full_init;       // True by default. Should be switched off when the options are created for an already running
                          // instance of WB.
    bool logLevelSet;
    WBOptions(const std::string &appBinaryName);
    ~WBOptions();
    void analyzeCommandLineArguments();
    dataTypes::OptionsList *programOptions;
  };

#define FOREACH_COMPONENT(list, iter) \
  for (std::vector<WBComponent *>::iterator iter = list.begin(); iter != list.end(); ++iter)

  class MYSQLWBBACKEND_PUBLIC_FUNC WBContext : public base::trackable, base::Observer {
    friend class WorkbenchImpl;
    friend class WBComponent;
    friend class WBContextUI;

  public:
    WBContext(bool verbose = false);
    virtual ~WBContext();

    bool software_rendering_enforced();
    bool opengl_rendering_enforced();

    bool init_(WBFrontendCallbacks *callbacks, WBOptions *options);
    void init_finish_(WBOptions *options);
    void finalize();

    bool is_commercial();

    bec::UIForm *get_active_form();
    bec::UIForm *get_active_main_form();

    WBContextModel *get_model_context() {
      return _model_context;
    }
    WBContextSQLIDE *get_sqlide_context() {
      return _sqlide_context;
    }

    // Document handling.
    void new_document();
    bool can_close_document(); // returns false for cancelled
    bool close_document();

    void close_document_finish();
    void new_model_finish();

    // save document

    bool save_as(const std::string &path);

    std::string get_filename() const;

    void report_bug(const std::string &errorInfo);

    // plugins
    void execute_plugin(const std::string &plugin_name, const bec::ArgumentPool &argpool = bec::ArgumentPool());

    void update_plugin_arguments_pool(bec::ArgumentPool &args);

    // DB Querying
    std::shared_ptr<SqlEditorForm> add_new_query_window(const db_mgmt_ConnectionRef &target,
                                                        bool restore_session = true);
    std::shared_ptr<SqlEditorForm> add_new_query_window();

    // Admin
    void add_new_admin_window(const db_mgmt_ConnectionRef &target);

    // Generic plugin tabs
    void add_new_plugin_window(const std::string &plugin_id, const std::string &caption);

    // GUI Plugin
    void register_builtin_plugins(grt::ListRef<app_Plugin> plugins);

    void close_gui_plugin(NativeHandle handle);

    //
    void request_refresh(RefreshType type, const std::string &str, NativeHandle ptr = (NativeHandle)0);

    const std::string &get_user_datadir() const {
      return _user_datadir;
    }
    // TODO: Temporary solution need to make ModelFile grt class
    workbench_DocumentRef openModelFile(const std::string &file);
    std::string getTempDir();
    int closeModelFile();
    std::string getDbFilePath();

    bool open_document(const std::string &file);
    void open_script_file(const std::string &file);
    void open_recent_document(int index);
    bool has_unsaved_changes();
    bool save_changes();

    bool open_file_by_extension(const std::string &path, bool interactive);

    bec::PluginManager *get_plugin_manager() {
      return _plugin_manager;
    }
    template <class C>
    C *get_component() {
      return dynamic_cast<C *>(get_component_named(C::name()));
    }

    WBComponent *get_component_named(const std::string &name);

    WBComponent *get_component_handling(const model_ObjectRef &object);

    void foreach_component(const std::function<void(WBComponent *)> &slot);

    WorkbenchImpl *get_workbench() {
      return _workbench;
    };

    bec::Clipboard *get_clipboard() const {
      return _clipboard;
    }

    workbench_WorkbenchRef get_root();
    workbench_DocumentRef get_document();
    grt::DictRef get_wb_options();

    std::string get_datadir() const {
      return _datadir;
    }

    bool cancel_idle_tasks();
    void flush_idle_tasks(bool force);

    // utilities for error reporting
    void show_exception(const std::string &operation, const std::exception &exc);
    void show_exception(const std::string &operation, const grt::grt_runtime_error &exc);

    template <class R>
    R execute_in_main_thread(const std::string &name, const std::function<R()> &function) {
      return bec::GRTManager::get()->get_dispatcher()->call_from_main_thread /*<R>*/ (function, true, false);
    }
    void execute_in_main_thread(const std::string &name, const std::function<void()> &function, bool wait);

    grt::ValueRef execute_in_grt_thread(const std::string &name, const std::function<grt::ValueRef()> &function);

    void execute_async_in_grt_thread(const std::string &name, const std::function<grt::ValueRef()> &function);

    bool activate_live_object(const GrtObjectRef &object);

    std::string create_attached_file(const std::string &group, const std::string &tmpl);
    void save_attached_file_contents(const std::string &name, const char *data, size_t size);
    std::string get_attached_file_contents(const std::string &name);
    std::string get_attached_file_tmp_path(const std::string &name);
    void delete_attached_file(const std::string &name);
    std::string recreate_attached_file(const std::string &name, const std::string &data);
    int export_attached_file_contents(const std::string &name, const std::string &export_to);

    void block_user_interaction(bool flag);
    bool user_interaction_allowed() {
      return _user_interaction_blocked == 0;
    }

    // State handling.
    std::string read_state(const std::string &name, const std::string &domain, const std::string &default_value);
    int read_state(const std::string &name, const std::string &domain, const int &default_value);
    double read_state(const std::string &name, const std::string &domain, const double &default_value);
    bool read_state(const std::string &name, const std::string &domain, const bool &default_value);
    grt::ValueRef read_state(const std::string &name, const std::string &domain);

    void save_state(const std::string &name, const std::string &domain, const std::string &value);
    void save_state(const std::string &name, const std::string &domain, const int &value);
    void save_state(const std::string &name, const std::string &domain, const double &value);
    void save_state(const std::string &name, const std::string &domain, const bool &value);
    void save_state(const std::string &name, const std::string &domain, grt::ValueRef value);

  protected:
    friend class WBContextModel; // to access _components

    bec::PluginManager *_plugin_manager;

    int _user_interaction_blocked;
    bool _send_messages_to_shell;
    bool _asked_for_saving;
    bool _initialization_finished;
    bool _attachments_changed;

    std::string _datadir;
    std::string _user_datadir;

    struct RefreshRequest {
      RefreshType type;
      std::string str;
      NativeHandle ptr;
      double timestamp;
    };

    // Predicate for pending refresh removal on close.
    struct CancelRefreshCandidate {
      bool operator()(RefreshRequest request) {
        return (request.type == RefreshNewModel || request.type == RefreshNewDiagram ||
                request.type == RefreshOverviewNodeChildren || request.type == RefreshZoom ||
                request.type == RefreshDocument || request.type == RefreshOverviewNodeInfo);
      }
    };

    std::list<RefreshRequest> _pending_refreshes;
    base::Mutex _pending_refresh_mutex;

    base::RecMutex _block_user_interaction_mutex;

    WBContextModel *_model_context;
    WBContextSQLIDE *_sqlide_context;

    std::vector<WBComponent *> _components;

    WorkbenchImpl *_workbench;

    bec::Clipboard *_clipboard;

    ModelFile *_file;
    std::string _filename;
    // only used for comparing pointers
    grt::UndoAction *_save_point;

    TunnelManager *_tunnel_manager;

    ModelFile *_model_import_file;

    bool _force_sw_rendering;     // Command line switch.
    bool _force_opengl_rendering; // Command line switch.

    grt::ListRef<app_PaperType> get_paper_types(std::shared_ptr<grt::internal::Unserializer> unserializer);

    std::vector<grt::SlotHolder*> _messageHandlerList;

    void pushMessageHandler(grt::SlotHolder *slot);

    bool _other_connections_loaded;
    // setup
    void init_templates();
    void init_grt_tree(WBOptions *options, std::shared_ptr<grt::internal::Unserializer> unserializer);
    void init_plugins_grt(WBOptions *options);
    void init_plugin_groups_grt(WBOptions *options);
    void init_object_listeners_grt();
    void init_properties_grt(workbench_DocumentRef &doc);
    void init_rdbms_modules();

    void do_close_document(bool destroying);

    grt::ValueRef setup_context_grt(WBOptions *options);

    void set_default_options(grt::DictRef options);

    void load_app_options(bool update);

    bool auto_save_document();
    std::string get_auto_save_dir();

    void cleanup_options();

  public:
    void save_app_options();
    void save_connections();
    void save_instances();

  protected:
    void add_recent_file(const std::string &file);

    void load_app_state(std::shared_ptr<grt::internal::Unserializer> unserializer);
    void save_app_state();

    grt::ValueRef save_grt();

    grt::ValueRef execute_plugin_grt(const app_PluginRef &plugin, const grt::BaseListRef &args);
    void plugin_finished(const grt::ValueRef &result, const app_PluginRef &plugin);

    bool handle_message(const grt::Message &msg);

    void reset_document();
    void reset_listeners();

    void option_dict_changed(grt::internal::OwnedDict *dict = 0, bool added = false, const std::string &key = "");

  private:
    // for base::Observer
    virtual void handle_notification(const std::string &name, void *sender, std::map<std::string, std::string> &info);

  public:
    ModelFile *get_file() {
      return _file;
    }

    bool install_module_file(const std::string &path);
    bool uninstall_module(grt::Module *module);
    void run_script_file(const std::string &path);

  private:
    bool find_connection_password(const db_mgmt_ConnectionRef &conn, std::string &password);

    void *do_request_password(const std::string &title, const std::string &service, bool reset_password,
                              std::string *account, std::string *ret_password);
    void *do_find_connection_password(const std::string &hostId, const std::string &username,
                                      std::string *ret_password);

    void load_other_connections();

    void attempt_options_upgrade(xmlDocPtr xmldoc, const std::string &version);

    bool show_error(const std::string &title, const std::string &message);

    void setLogLevelFromGuiPreferences(const grt::DictRef &dict);

  public:
    std::string request_connection_password(const db_mgmt_ConnectionRef &conn, bool force_asking);

  public: // front end callbacks
    WBFrontendCallbacks *_frontendCallbacks;

    // Internal, used for gui plugins
    std::function<void(std::string, void *)> show_gui_plugin;

  private:
    void warnIfRunningOnUnsupportedOS();
  };

  struct GUILock {
    WBContext *_wb;

    GUILock(WBContext *wb, const std::string &message_title, const std::string &message) : _wb(wb) {
      mforms::Utilities::show_wait_message(message_title, message);
      _wb->block_user_interaction(true);
    }
    ~GUILock() {
      _wb->block_user_interaction(false);
      mforms::Utilities::hide_wait_message();
    }
  };
};
