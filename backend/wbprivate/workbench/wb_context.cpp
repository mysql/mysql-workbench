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

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "wb_tunnel.h"

#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "sqlide/wb_sql_editor_form.h"

#include "wb_context.h"
#include "wb_context_ui.h"
#include "wb_model_file.h"

#include "wb_version.h"

#include "base/string_utilities.h"
#include "base/util_functions.h"
#include "base/scope_exit_trigger.h"

#include "grt/clipboard.h"
#include "grt/plugin_manager.h"

#include "cppdbc.h"

#include "wb_module.h"

#include "grts/structs.workbench.h"
#include "model/wb_context_model.h"
#include "model/wb_component_basic.h"
#include "model/wb_component_logical.h"
#include "model/wb_component_physical.h"

#include "sqlide/wb_context_sqlide.h"

#include "upgrade_helper.h"

#include "grtui/grtdb_connect_dialog.h"
#include "grtdb/db_helpers.h"

#include "interfaces/interfaces.h"
#include "grt/validation_manager.h"

#include "base/threaded_timer.h"
#include "base/log.h"
#include "base/drawing.h"

#include "mforms/mforms.h"
#include "mforms/menubar.h"
#include "mforms/toolbar.h"

DEFAULT_LOG_DOMAIN(DOMAIN_WB_CONTEXT)

#define PLUGIN_GROUP_PATH "/wb/registry/pluginGroups"
#define PLUGIN_LIST_PATH "/wb/registry/plugins"

#define TYPE_GROUP_FILE "data/db_datatype_groups.xml"

#define SERVER_INSTANCE_LIST "server_instances.xml"
#define FILE_CONNECTION_LIST "connections.xml"
#define FILE_OTHER_CONNECTION_LIST "other_connections.xml"

#define PAPER_LANDSCAPE "landscape"
#define PAPER_PORTRAIT "portrait"

// Options file.
#define OPTIONS_FILE_NAME "wb_options.xml"
#define OPTIONS_DOCUMENT_FORMAT "MySQL Workbench Options"
#define OPTIONS_DOCUMENT_VERSION "1.0.1"

// State file.
#define STATE_FILE_NAME "wb_state.xml"
#define STATE_DOCUMENT_FORMAT "MySQL Workbench Application State"
#define STATE_DOCUMENT_VERSION "1.0.0"

// Don't send a given refresh_request unless no new ones arrive in this time.
#define UI_REQUEST_THROTTLE 0.3

#define DEFAULT_UNDO_STACK_SIZE 10

// auto-save every 1 minute (default)
#define AUTO_SAVE_MODEL_INTERVAL (60)

#define AUTO_SAVE_SQLEDITOR_INTERVAL 10

#if defined(_MSC_VER) || defined(__APPLE__)
#define HAVE_BUNDLED_MYSQLDUMP
#endif

using namespace grt;
using namespace bec;
using namespace wb;
using namespace base;

static const char *argv0 = NULL;

//----------------------------------------------------------------------

#ifndef Constructor____

static base::Mutex option_mutex;

static void log_func(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data) {
  base::Logger::LogLevel level = base::Logger::LogLevel::Disabled;
  if (log_level & (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL))
    level = base::Logger::LogLevel::Error;
  else if (log_level & G_LOG_LEVEL_WARNING)
    level = base::Logger::LogLevel::Warning;
  else if (log_level & (G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO))
    level = base::Logger::LogLevel::Info;
  else if (log_level & (G_LOG_LEVEL_DEBUG))
    level = base::Logger::LogLevel::Debug;
  base::Logger::log(level, log_domain ? log_domain : "", "%s", std::string(message).append("\n").c_str());
#ifndef _MSC_VER
  g_log_default_handler(log_domain, log_level, message, user_data);
#endif
}

static grt::ValueRef get_app_option(const std::string &option, WBContext *wb) {
  base::MutexLock lock(option_mutex);

  if (option.empty())
    return wb->get_wb_options();

  if (wb->get_document().is_valid() && wb->get_document()->physicalModels().count()) {
    grt::DictRef model_options(wb->get_document()->physicalModels().get(0)->options());

    if (model_options.get_int("useglobal", 0))
      return wb->get_wb_options().get(option);
    if (model_options.has_key(option))
      return model_options.get(option);
  }
  return wb->get_wb_options().get(option);
}

static void set_app_option(const std::string &option, grt::ValueRef value, WBContext *wb) {
  base::MutexLock lock(option_mutex);

  if (wb->get_document().is_valid() && wb->get_document()->physicalModels().is_valid() &&
      wb->get_document()->physicalModels().count() > 0) {
    grt::DictRef model_options(wb->get_document()->physicalModels().get(0)->options());

    if (model_options.get_int("useglobal", 0)) {
      wb->get_wb_options().set(option, value);
      return;
    } else if (model_options.has_key(option)) {
      model_options.set(option, value);
      return;
    }
  }
  wb->get_wb_options().set(option, value);
}

//----------------- WBOptions ----------------------------------------------------------------------

static bool parse_loglevel(const std::string &line) {
  bool ret = false;

  ret = base::Logger::active_level(line);
  if (ret)
    printf("Logger set to level '%s'. '%s'\n", line.c_str(), base::Logger::get_state().c_str());

  return ret;
}

WBOptions::WBOptions(const std::string &appBinaryName)
  : binaryName(appBinaryName),
    force_sw_rendering(false),
    force_opengl_rendering(false),
    verbose(false),
    quit_when_done(false),
    testing(false),
    init_python(true),
    full_init(true),
    logLevelSet(false) {
  // Allocate this on heap so we silent gcc
  programOptions = new dataTypes::OptionsList();
  logDebug("Creating WBOptions\n");

#if defined(_MSC_VER) || defined(__APPLE__)
  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, 'h', "help",
                                                  "Show help options",
                                                  [this](const dataTypes::OptionEntry &entry, int *retval) {
                                                    std::cout << programOptions->getHelp(binaryName);
                                                    *retval = 0;
                                                    return false;
                                                  }));
#endif

#ifdef _MSC_VER
  programOptions->addEntry(
    dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, 0, "swrendering",
                           "Force the diagram canvas to use software rendering instead of OpenGL",
                           [this](const dataTypes::OptionEntry &entry, int *retval) {
                             force_sw_rendering = entry.value.logicalValue;
                             return true;
                           }));
#elif __APPLE__
#else
  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, 0,
                                                  "force-sw-render", "Force Xlib rendering",
                                                  [this](const dataTypes::OptionEntry &entry, int *retval) {
                                                    force_sw_rendering = entry.value.logicalValue;
                                                    return true;
                                                  }));

  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, 0,
                                                  "force-opengl-render", "Force OpenGL rendering",
                                                  [this](const dataTypes::OptionEntry &entry, int *retval) {
                                                    force_opengl_rendering = entry.value.logicalValue;
                                                    return true;
                                                  }));
#endif

  auto func = [this](const dataTypes::OptionEntry &entry, int *retval) {
    if (!entry.value.textValue.empty()) {
      open_at_startup_type = entry.longName;
      if (entry.longName == "query" || entry.longName == "admin")
        open_connection = entry.value.textValue;
      else
        open_at_startup = entry.value.textValue;
    }
    return true;
  };

  programOptions->addEntry(
    dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentText, "query",
                           "Open a query tab and ask for connection if nothing is specified.\n"
                           "If named connection is specified it will be opened,\n"
                           "else connection will be created based on the given connection string,",
                           func, "<connection>|<connection string>"));
  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentText, "admin",
                                                  "Open a administration tab to the named instance", func,
                                                  "<instance>"));
  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentText, "model",
                                                  "Open the given EER model file", func, "<model file>"));

  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical,
                                                  "upgrade-mysql-dbs", "Open a migration wizard tab",
                                                  [this](const dataTypes::OptionEntry &entry, int *retval) {
                                                    if (entry.value.logicalValue)
                                                      open_at_startup_type = entry.longName;
                                                    return true;
                                                  }));

  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, "migration",
                                                  "Open a migration wizard tab",
                                                  [this](const dataTypes::OptionEntry &entry, int *retval) {
                                                    if (entry.value.logicalValue)
                                                      open_at_startup_type = entry.longName;
                                                    return true;
                                                  }));

  programOptions->addEntry(dataTypes::OptionEntry(
    dataTypes::OptionArgumentType::OptionArgumentText, 0, "script",
    "Open the given SQL file in an connection, best in conjunction with a query parameter", func, "<sql file>"));

  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentText, "run-script",
                                                  "Execute Python code from a file",
                                                  [this](const dataTypes::OptionEntry &entry, int *retval) {
                                                    if (!entry.value.textValue.empty()) {
                                                      run_language = "python";
                                                      open_at_startup_type = "run-script";
                                                      open_at_startup = entry.value.textValue;
                                                    }
                                                    return true;
                                                  },
                                                  "<file>"));

  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentText, "run",
                                                  "Execute the given Python code",
                                                  [this](const dataTypes::OptionEntry &entry, int *retval) {
                                                    if (!entry.value.textValue.empty())
                                                      run_at_startup = entry.value.textValue;
                                                    return true;
                                                  },
                                                  "<code>"));

  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentText, "run-python",
                                                  "Execute Python code from a file",
                                                  [this](const dataTypes::OptionEntry &entry, int *retval) {
                                                    if (!entry.value.textValue.empty()) {
                                                      run_language = "python";
                                                      run_at_startup = entry.value.textValue;
                                                    }
                                                    return true;
                                                  },
                                                  "<code>"));

  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical,
                                                  "quit-when-done", "Quit Workbench when the script is done",
                                                  [this](const dataTypes::OptionEntry &entry, int *retval) {
                                                    quit_when_done = entry.value.logicalValue;
                                                    return true;
                                                  }));

  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, "log-to-stderr",
                                                  "Also log to stderr",
                                                  [](const dataTypes::OptionEntry &entry, int *retval) {
                                                    Logger::log_to_stderr(true);
                                                    return true;
                                                  }));

  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentText, "log-level",
                                                  "Valid levels are: error, warning, info, debug1, debug2, debug3",
                                                  [](const dataTypes::OptionEntry &entry, int *retval) {
                                                    if (!parse_loglevel(entry.value.textValue)) {
                                                      printf("Unable to parse log level value %s\n",
                                                             entry.value.textValue.c_str());
                                                      *retval = 1;
                                                      return false;
                                                    }
                                                    return true;
                                                  },
                                                  "<level>"));

  programOptions->addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentLogical, 'v', "verbose",
                                                  "Enable diagnostics output",
                                                  [this](const dataTypes::OptionEntry &entry, int *retval) {
                                                    verbose = entry.value.logicalValue;
                                                    return true;
                                                  }));

  programOptions->addEntry(dataTypes::OptionEntry(
    dataTypes::OptionArgumentType::OptionArgumentLogical, "version", "Show Workbench version number and exit",
    [](const dataTypes::OptionEntry &entry, int *retval) {
      if (entry.value.logicalValue) {
        const char *type = APP_EDITION_NAME;
        if (strcmp(APP_EDITION_NAME, "Community") == (0)) // Extra parens to silence warning.
          type = "CE";

        printf("MySQL Workbench %s (%s) %i.%i.%i %s build %i\n", type, APP_LICENSE_TYPE, APP_MAJOR_NUMBER,
               APP_MINOR_NUMBER, APP_RELEASE_NUMBER, APP_RELEASE_TYPE, APP_BUILD_NUMBER);
        *retval = 0;

        return false;
      } else
        return true;
    }));

  programOptions->addEntry(
    dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentText, "configdir",
                           "Specify configuration directory location, default is platform specific.",
                           [this](const dataTypes::OptionEntry &entry, int *retval) {
                             if (!entry.value.textValue.empty()) {
                               printf("Using %s as data directory.\n", entry.value.textValue.c_str());
                               user_data_dir = entry.value.textValue;
                             }
                             return true;
                           },
                           "<path>"));

  programOptions->addEntry(
    dataTypes::OptionEntry(dataTypes::OptionArgumentType::OptionArgumentText, "open",
                           "Open the given file at startup (deprecated, use script, model etc.)",
                           [this](const dataTypes::OptionEntry &entry, int *retval) {
                             if (!entry.value.textValue.empty()) {
                               printf(
                                 "Note: the \"open\" parameter is deprecated and will be removed in a future version"
                                 " of MySQL Workbench\n");
                               open_at_startup = entry.value.textValue;
                             }

                             return true;
                           }));
}

WBOptions::~WBOptions() {
  delete programOptions;
}

void WBOptions::analyzeCommandLineArguments() {
  auto entry = programOptions->getEntry("log-level");
  if (entry->value.textValue.empty()) {
    const char *log_setting = getenv("WB_LOG_LEVEL");
    if (log_setting == NULL) {
#if defined(_DEBUG) || defined(ENABLE_DEBUG)
      log_setting = "debug2";
#else
      log_setting = "info";
#endif
    } else
      logLevelSet = true;

    std::string level = base::tolower(log_setting);
    base::Logger::active_level(level);
  } else {
    logInfo("Logger set to level '%s'\n", base::Logger::active_level().c_str());
    base::Logger::setLogLevelSpecifiedByUser();
  }

  // Get last path and use it
  if (!programOptions->pathArgs.empty())
    open_at_startup = programOptions->pathArgs.back();
}
//----------------- WBContext ----------------------------------------------------------------------

extern void register_all_metaclasses();

WBContext::WBContext(bool verbose) : _frontendCallbacks(nullptr) {

  logDebug("Creating WBContext\n");

  _asked_for_saving = false;
  _model_context = nullptr;
  _sqlide_context = new WBContextSQLIDE();
  _file = nullptr;
  _save_point = nullptr;
  _tunnel_manager = nullptr;
  _model_import_file = nullptr;

  g_log_set_handler(NULL, (GLogLevelFlags)0xfffff, log_func, this);

  // register GRT object class implementations
  if (grt::GRT::get()->metaclassesNeedRegister()) {
    register_all_metaclasses();
  }

  _user_interaction_blocked = 0;
  block_user_interaction(true);
  _send_messages_to_shell = true;

  _initialization_finished = false;
  _attachments_changed = false;

  bec::GRTManager::get()->setVerbose(verbose);
  bec::GRTManager::get()->set_app_option_slots(std::bind(get_app_option, std::placeholders::_1, this),
                                    std::bind(set_app_option, std::placeholders::_1, std::placeholders::_2, this));
  bec::GRTManager::get()->update_plugin_arguments_pool =
    std::bind(&WBContext::update_plugin_arguments_pool, this, std::placeholders::_1);

  bec::GRTManager::get()->set_timeout_request_slot(
    std::bind(&WBContext::request_refresh, this, RefreshTimer, "", static_cast<NativeHandle>(0)));

  NotificationCenter::get()->add_observer(this, "GNDocumentOpened");

  // register interface classes
  register_interfaces();

  _clipboard = new bec::Clipboard();
  bec::GRTManager::get()->set_clipboard(_clipboard);
  scoped_connect(grt::GRT::get()->get_undo_manager()->signal_changed(),
                 std::bind(&WBContext::request_refresh, this, RefreshDocument, "", static_cast<NativeHandle>(0)));

  if (getenv("DEBUG_UNDO"))
    grt::GRT::get()->get_undo_manager()->enable_logging_to(&std::cout);

  _plugin_manager = bec::GRTManager::get()->get_plugin_manager();
  _plugin_manager->set_registry_paths(PLUGIN_LIST_PATH, PLUGIN_GROUP_PATH);

  // create and register the module for Workbench stuff
  _workbench = grt::GRT::get()->get_native_module<WorkbenchImpl>();
  _workbench->set_context(this);

  _components.push_back(new WBComponentBasic(this));
  _components.push_back(new WBComponentPhysical(this));
  _components.push_back(new WBComponentLogical(this));
}

WBContext::~WBContext() {
  for(auto const &it: _messageHandlerList) {
    grt::GRT::get()->removeMessageHandler(it);
  }
  _messageHandlerList.clear();

  NotificationCenter::get()->remove_observer(this);

  logDebug("Destroying WBContext\n");

  //{
  //  workbench_WorkbenchRef app(get_root());

  //  app.options().unref_tree();
  //  app.registry().unref_tree();
  //  app.info().unref_tree();
  //  app.unref_tree();
  //}

  delete _clipboard;
  _clipboard = 0;
  //  delete _plugin_manager; this is deleted by the GRT, since its a module

  _frontendCallbacks = nullptr;

  closeModelFile();

  // unset the log handler user data as the logger will be deleted
  // TODO: no longer needed since we have a static logger now.
  // g_log_set_handler(NULL, (GLogLevelFlags)0xfffff, log_func, NULL);

  std::vector<WBComponent *>::iterator it = _components.begin();
  std::vector<WBComponent *>::const_iterator last = _components.end();
  for (; last != it; ++it) {
    delete *it;
    *it = 0;
  }


  delete _sqlide_context;
  _sqlide_context = 0;
}

#endif // Constructor____

#ifndef Components____

WBComponent *WBContext::get_component_named(const std::string &name) {
  FOREACH_COMPONENT(_components, iter)
  if ((*iter)->get_name() == name)
    return (*iter);
  return 0;
}

void WBContext::foreach_component(const std::function<void(WBComponent *)> &slot) {
  FOREACH_COMPONENT(_components, iter)
  slot(*iter);
}

WBComponent *WBContext::get_component_handling(const model_ObjectRef &object) {
  FOREACH_COMPONENT(_components, iter)
  if ((*iter)->handles_figure(object))
    return *iter;
  return 0;
}

#endif // Components____

//--------------------------------------------------------------------------------------------------

bec::UIForm *WBContext::get_active_form() {
  return wb::WBContextUI::get()->get_active_form();
}

//--------------------------------------------------------------------------------------------------

bool wb::WBContext::is_commercial() {
  std::string edition = base::tolower(get_root()->info()->edition());
  return (edition == "commercial") || (edition == "development");
}

//--------------------------------------------------------------------------------------------------

bec::UIForm *WBContext::get_active_main_form() {
  return wb::WBContextUI::get()->get_active_main_form();
}

void WBContext::finalize() {
  // Stop any scheduled events, animations etc. before continuing.
  ThreadedTimer::stop();

  //_signal_app_closing();
  NotificationCenter::get()->send("GNAppClosing", 0);

  do_close_document(true);

  // don't save state if initialization isn't finished at this time (otherwise we're probably
  // quitting before all the old state was loaded and writing stuff back would just reset everything)
  if (_initialization_finished) {
    save_app_options();
    save_app_state();
    save_connections();
  }

  bec::GRTManager::get()->get_dispatcher()->shutdown();
  if (_tunnel_manager) {
    delete _tunnel_manager;
    _tunnel_manager = nullptr;
  }

  if (_model_context) {
    delete _model_context;
    _model_context = nullptr;
  }

  // clear slots to managed
  bec::GRTManager::get()->set_status_slot(std::function<void(std::string)>{});
  _plugin_manager->set_gui_plugin_callbacks(PluginManagerImpl::OpenGUIPluginSlot{},
    PluginManagerImpl::ShowGUIPluginSlot{}, PluginManagerImpl::CloseGUIPluginSlot{});
}

void WBContext::block_user_interaction(bool flag) {
  // Use a mutext to protect this whole function
  base::RecMutexLock _lock(_block_user_interaction_mutex);

  if (flag)
    _user_interaction_blocked++;
  else {
    if (_user_interaction_blocked > 0)
      _user_interaction_blocked--;
  }

  if (_user_interaction_blocked == 1 && flag) {
    if (_frontendCallbacks && _frontendCallbacks->lock_gui)
      _frontendCallbacks->lock_gui(true);
  } else if (_frontendCallbacks && _user_interaction_blocked == 0 && !flag) {
    if (_frontendCallbacks->lock_gui)
      _frontendCallbacks->lock_gui(false);
  }
}

#ifndef Setup____

//--------------------------------------------------------------------------------

bool WBContext::opengl_rendering_enforced() {
  return _force_opengl_rendering;
}

//--------------------------------------------------------------------------------------------------

bool WBContext::software_rendering_enforced() {
  bool result = false;

  if (!_force_opengl_rendering) {
    // See if the current adapter is one of those that we don't want to enable OpenGL by default.
    static std::string excluded_adapters[] = {
      "965",   // Mobile Intel(R) 965 Express chip set Family
      "82945G" // Intel(R) 82945G Express chip set Family
    };

    grt::StringListRef arguments(grt::Initialized);
    std::string videoAdapter = grt::StringRef::cast_from(_workbench->call_function("getVideoAdapter", arguments));
    for (unsigned int i = 0; i < sizeof(excluded_adapters) / sizeof(excluded_adapters[0]); i++)
      if (videoAdapter.find(excluded_adapters[i]) != std::string::npos) {
        result = true;
        break;
      }
  }

  // Setting from preferences.
  if (get_root()->options()->options().get_int("workbench:ForceSWRendering", 0) != 0)
    result = true;

  // Setting from command line overrides the preferences setting.
  if (_force_sw_rendering)
    result = true;

  return result;
}

//--------------------------------------------------------------------------------------------------

// Need to define a local function wrapper for the show_error call as sigc templates can neither handle
// overloaded functions nor default parameters.
bool WBContext::show_error(const std::string &title, const std::string &message) {
  logError("%s", (message + '\n').c_str());
  return mforms::Utilities::show_error(title, message, _("Close")) != 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Actually triggers the password find or user querying process for the given service. The given account
 * can be empty if the connection has no user set so we must be able to return it like the password.
 * Both, account as well as password must be allocated by the caller and passed on via references as
 * this function is called via the dispatcher.
 * For the same reason is the return value passed back as pointer, even though it's a bool.
 */
void *WBContext::do_request_password(const std::string &title, const std::string &service, bool force_asking,
                                     std::string *account, std::string *password) {
  bool ret = false;

  try {
    ret = mforms::Utilities::find_or_ask_for_password(title, service, *account, force_asking, *password);
  } catch (const std::exception &e) {
    show_error("Error Looking Up Password", e.what());
  }

  return (void *)ret;
}

void *WBContext::do_find_connection_password(const std::string &hostId, const std::string &username,
                                             std::string *ret_password) {
  bool ret = false;
  try {
    ret = mforms::Utilities::find_password(hostId, username, *ret_password);
  } catch (const std::exception &e) {
    show_error("Error Looking Up Password", e.what());
  }

  return (void *)ret;
}

bool WBContext::find_connection_password(const db_mgmt_ConnectionRef &conn, std::string &password) {
  /*
  return execute_in_main_thread<bool>("find_password",
                        std::bind(&WBContext::do_find_connection_password, this,
                                   conn->hostIdentifier().c_str(),
                                   conn->parameterValues().get_string("userName").c_str(),
                                   &password));*/
  void *ret = mforms::Utilities::perform_from_main_thread(
    std::bind(&WBContext::do_find_connection_password, this, conn->hostIdentifier(),
              conn->parameterValues().get_string("userName"), &password));
  if (ret)
    return true;
  return false;
}

// throws grt::user_cancelled
std::string WBContext::request_connection_password(const db_mgmt_ConnectionRef &conn, bool reset_password) {
  std::string password_tmp;
  std::string user_tmp = conn->parameterValues().get_string("userName");
  void *ret = mforms::Utilities::perform_from_main_thread(
    std::bind(&WBContext::do_request_password, this, _("Connect to MySQL Server"), conn->hostIdentifier(),
              reset_password, &user_tmp, &password_tmp));
  if (ret)
    return password_tmp;
  throw grt::user_cancelled("Canceled by user");
}

bool WBContext::init_(WBFrontendCallbacks *callbacks, WBOptions *options) {
  logInfo("WbContext::init\n");
  grt::ValueRef res;

  _force_opengl_rendering = options->force_opengl_rendering;
  _force_sw_rendering = options->force_sw_rendering;

  // Set callbacks
  _frontendCallbacks = callbacks;

  bec::GRTManager::get()->set_status_slot(_frontendCallbacks->show_status_text);
  grt::GRT::get()->setTesting(options->testing);

  // Already blocked (when constructed), but call it again so that lock_gui() gets called now.
  // Unlock not before a new document is created (see new_document()).
  block_user_interaction(true);
  block_user_interaction(false); // decrement the block counter to be back at 1

  // set the path for the options dictionary that modules should use to store their stuff
  grt::GRT::get()->set_global_module_data_path("/wb/customData");
  grt::GRT::get()->set_document_module_data_path("/wb/doc/customData");

  mforms::App::get()->setBaseDir(options->basedir);
  bec::GRTManager::get()->set_datadir(options->basedir);
  bec::GRTManager::get()->set_basedir(options->basedir);
  bec::GRTManager::get()->set_user_datadir(options->user_data_dir);
  bec::GRTManager::get()->cleanup_tmp_dir();

  mforms::App::get()->set_user_data_folder_path(options->user_data_dir);
  mforms::Utilities::set_message_answers_storage_path(
    base::makePath(options->user_data_dir, "mforms_remembered_dialog_responses"));

  bec::IconManager::get_instance()->set_basedir(options->basedir);

  // Setup image search paths
  std::string path;
  static const char *dirs[] = {"images",      "images/icons", "images/grt",     "images/grt/structs",
                               "images/png",
#ifdef _MSC_VER
                               "images/home",
#endif
                               "images/ui",   "images/sql",   "images/sql/mac", "",
                               NULL};

  for (unsigned int i = 0; dirs[i] != NULL; i++) {
    mdc::ImageManager::get_instance()->add_search_path(base::makePath(options->basedir, dirs[i]));
    bec::IconManager::get_instance()->add_search_path(dirs[i]);
  }

  std::string loader_module_path = options->plugin_search_path;

  _tunnel_manager = new TunnelManager();

  {
    // Set location of cdbc drivers and module loader (python etc) DLLs
    sql::DriverManager *dbc_driver_man = sql::DriverManager::getDriverManager();

    // set the tunnel factory
    if (_tunnel_manager)
      dbc_driver_man->setTunnelFactoryFunction(
        std::bind(&TunnelManager::createTunnel, _tunnel_manager, std::placeholders::_1));

    // set function to request connection password for user
    dbc_driver_man->setPasswordFindFunction(
      std::bind(&WBContext::find_connection_password, this, std::placeholders::_1, std::placeholders::_2));
    dbc_driver_man->setPasswordRequestFunction(
      std::bind(&WBContext::request_connection_password, this, std::placeholders::_1, std::placeholders::_2));

    mforms::Utilities::add_driver_shutdown_callback(std::bind(&sql::DriverManager::thread_cleanup, dbc_driver_man));

#ifdef _MSC_VER
    dbc_driver_man->set_driver_dir(options->basedir);
#elif defined(__APPLE__)
    dbc_driver_man->set_driver_dir(options->cdbc_driver_search_path);
#else
    if (getenv("DBC_DRIVER_PATH"))
      dbc_driver_man->set_driver_dir(getenv("DBC_DRIVER_PATH"));
    else
      dbc_driver_man->set_driver_dir(options->cdbc_driver_search_path);
#endif
  }

  // Set callbacks

  _plugin_manager->set_gui_plugin_callbacks(callbacks->open_editor, callbacks->show_editor, callbacks->hide_editor);

  pushMessageHandler(new grt::SlotHolder(std::bind(&WBContext::handle_message, this, std::placeholders::_1)));

  // Set options
  _datadir = options->basedir;
  _user_datadir = options->user_data_dir;

  std::string modules_path;
  std::string libraries_path;
  std::string user_modules_path;
  std::string user_scripts_path;
  std::string user_libraries_path;

  user_modules_path = base::makePath(options->user_data_dir, "modules");
  user_scripts_path = base::makePath(options->user_data_dir, "scripts");
  user_libraries_path = base::makePath(options->user_data_dir, "libraries");

  modules_path = options->module_search_path;
#ifdef _MSC_VER
  modules_path = base::pathlistPrepend(modules_path, ".");
#endif

  libraries_path = options->library_search_path;

  // create user_data_dir/modules dir if it does not exist yet
  if (!g_file_test(user_modules_path.c_str(), (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
    g_mkdir_with_parents(user_modules_path.c_str(), 0700);

  if (!g_file_test(user_scripts_path.c_str(), (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
    g_mkdir_with_parents(user_scripts_path.c_str(), 0700);

  if (!g_file_test(user_libraries_path.c_str(), (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
    g_mkdir_with_parents(user_libraries_path.c_str(), 0700);

  bec::GRTManager::get()->set_search_paths(
    modules_path, base::pathlistPrepend(options->struct_search_path, base::makePath(options->basedir, "structs")),
    libraries_path);

  bec::GRTManager::get()->set_user_extension_paths(user_modules_path, user_libraries_path, user_scripts_path);

  std::list<std::string> exts;
  exts.push_back(".grt");

  bec::GRTManager::get()->set_module_extensions(exts);
  // init major parts of the app
  get_sqlide_context()->init();

  _frontendCallbacks->show_status_text(_("Initializing GRT..."));
  // Initialize GRT Manager.
  bec::GRTManager::get()->initialize(options->init_python, loader_module_path);

  bec::GRTManager::get()->get_shell()->set_save_directory(options->user_data_dir);
  bec::GRTManager::get()->get_shell()->set_saves_history(200); // limit history to 200 commands

  // add some handy shortcuts to shell tree bookmarks list
  bec::GRTManager::get()->get_shell()->add_grt_tree_bookmark("/wb/doc/physicalModels/0/catalog");
  bec::GRTManager::get()->get_shell()->add_grt_tree_bookmark("/wb/doc/physicalModels/0/catalog/schemata/0/tables");
  bec::GRTManager::get()->get_shell()->add_grt_tree_bookmark("/wb/doc/physicalModels/0/diagrams/0");
  bec::GRTManager::get()->get_shell()->add_grt_tree_bookmark("/wb/doc/physicalModels/0/diagrams/0/figures");
  bec::GRTManager::get()->get_shell()->add_grt_tree_bookmark("/wb/sqlEditors");
  bec::GRTManager::get()->get_shell()->add_grt_tree_bookmark("/wb/migration");
  bec::GRTManager::get()->get_shell()->add_grt_tree_bookmark("/wb/migration/sourceCatalog");
  bec::GRTManager::get()->get_shell()->add_grt_tree_bookmark("/wb/migration/targetCatalog");
  bec::GRTManager::get()->get_shell()->add_grt_tree_bookmark("/wb/registry/plugins");

  grt::GRT::get()->send_output(strfmt("Looking for user plugins in %s\n", user_modules_path.c_str()));

  _frontendCallbacks->show_status_text(_("Initializing Workbench components..."));
  res = setup_context_grt(options);

  if (res.is_valid() && *grt::IntegerRef::cast_from(res) != 1)
    show_error(_("Initialization Error"),
               _("There was an error during initialization of Workbench, some functionality may not work."));

  logInfo("System info:\n %s\n", _workbench->getSystemInfo(true).c_str());

  // The GRT shell is now created on demand. No need to do this in advance (which might get us into
  // trouble on Windows, because the main window doesn't exist yet).
  try {
    bec::GRTManager::get()->initialize_shell(get_root()->options()->options().get_string("grtshell:ShellLanguage", "python"));
  } catch (std::exception &) {
    bec::GRTManager::get()->initialize_shell("python");
  }

  get_root()->options()->signal_dict_changed()->connect(std::bind(
    &WBContext::option_dict_changed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

  _send_messages_to_shell = false;

  return true;
}

//--------------------------------------------------------------------------------------------------

static bool output_to_stdout(const grt::Message &msg, void *sender) {
  if (msg.type == grt::OutputMsg) {
    printf("%s", msg.text.c_str());
    fflush(stdout);
  } else {
    fprintf(stderr, "%s", msg.format().c_str());
  }
  return true;
}

void WBContext::warnIfRunningOnUnsupportedOS() {
  std::string os = get_local_os_name();
  logDebug2("get_local_os_name() returned '%s'\n", os.c_str());

  if (!_workbench->isOsSupported(os)) {
    mforms::Utilities::show_message_and_remember(
      "Unsupported Operating System",

      "You are running Workbench on an unsupported operating system. "
      "While it may work for you just fine, it wasn't designed to run on your platform. "
      "Please keep this in mind if you run into problems.",

      "OK", "", "", "wb.supported_os_check.suppress_warning", "Don't show this message again");
  }
}

void WBContext::init_finish_(WBOptions *options) {
  // initialize plugins that have a initializer (start with builtins and then go through user plugins)
  // Initialization to be done ONLY when WB is first started.
  // This point is also reached when i.e. a document was opened by double clicking and a WB instance was already open
  // full_init is used to identify the initialization mode.
  if (options->full_init) {
    const std::vector<grt::Module *> &modules(grt::GRT::get()->get_modules());
    grt::BaseListRef args(true);

    for (std::vector<grt::Module *>::const_iterator it = modules.begin(); it != modules.end(); ++it) {
      if ((*it)->has_function("initialize0")) {
        logDebug("Calling %s.initialize0()...\n", (*it)->name().c_str());
        try {
          (*it)->call_function("initialize0", args);
        } catch (std::exception &e) {
          logError("Error calling %s.initialize0(): %s\n", (*it)->name().c_str(), e.what());
        }
      }
    }

    for (std::vector<grt::Module *>::const_iterator it = modules.begin(); it != modules.end(); ++it) {
      if ((*it)->has_function("initialize")) {
        logDebug("Calling %s.initialize()...\n", (*it)->name().c_str());
        try {
          (*it)->call_function("initialize", args);
        } catch (std::exception &e) {
          logError("Error calling %s.initialize(): %s\n", (*it)->name().c_str(), e.what());
        }
      }
    }
  }

  // open initial document when GUI init finishes
  std::string initial_file;
  if (!options->open_at_startup.empty())
    initial_file = options->open_at_startup;

  if (initial_file.empty() && get_wb_options().get_int("workbench.AutoReopenLastModel", 0)) {
    grt::StringListRef recentFiles(get_root()->options()->recentFiles());
    for (size_t i = 0; i < recentFiles.count(); i++) {
      initial_file = recentFiles.get(i);
      if (g_str_has_suffix(initial_file.c_str(), ".mwb"))
        break;
    }
    if (!g_str_has_suffix(initial_file.c_str(), ".mwb"))
      initial_file.clear();
  }

  if (!initial_file.empty()) {
    if (g_str_has_suffix(initial_file.c_str(), ".mwb") || options->open_at_startup_type == "model")
      open_document(initial_file);
    else if (g_str_has_suffix(initial_file.c_str(), ".sql") || g_str_has_suffix(initial_file.c_str(), ".dbquery") ||
             options->open_at_startup_type == "query")
      options->open_at_startup_type = "query";
    else if (g_str_has_suffix(initial_file.c_str(), ".py") && options->open_at_startup_type == "run-script")
      options->open_at_startup_type = "run-script";
    else if (g_str_has_suffix(initial_file.c_str(), ".py") && options->open_at_startup_type == "script") {
      options->open_at_startup_type = "run-script";
      logWarning("--script option is meant for SQL scripts, assuming --run-script was meant instead\n");
    } else
      logError("Unknown file type %s\n", initial_file.c_str());
  }

  block_user_interaction(false);

  // SSH tunnel manager is created on first creation of a connection.

  _frontendCallbacks->show_status_text(_("Ready."));

  // Avoid our runtime tests to lock up when a modal warning dialog is displayed.
  if (options->open_at_startup_type != "run-script")
    warnIfRunningOnUnsupportedOS();

  try {
    // execute action requested from command line
    if (options->open_at_startup_type == "query" || options->open_at_startup_type == "admin") {
      std::string connection_name = options->open_connection;
      db_mgmt_ConnectionRef conn;

      if (!connection_name.empty()) {
        conn = find_named_object_in_list(get_root()->rdbmsMgmt()->storedConns(), connection_name, true);

        if (!conn.is_valid()) {
          if (options->open_at_startup_type == "admin") // if --admin is specified, look for instances too, for
                                                        // backwards compatibility with Windows Notifier
          {
            db_mgmt_ServerInstanceRef instance(
              find_named_object_in_list(get_root()->rdbmsMgmt()->storedInstances(), connection_name, true));

            if (!instance.is_valid())
              logError("No instance or connection named %s was found\n", connection_name.c_str());
            else {
              logError("Falling back to instance called %s for --admin option\n", connection_name.c_str());
              conn = instance->connection();
            }
          } else // else we will try to parse the param as connection string
          {
            grt::BaseListRef args(true);
            args.ginsert(grt::StringRef(options->open_connection));

            grt::ValueRef val = grt::GRT::get()->call_module_function("PyWbUtils", "connectionFromString", args);
            if (db_mgmt_ConnectionRef::can_wrap(val)) {
              db_mgmt_ConnectionRef tmp = db_mgmt_ConnectionRef::cast_from(val);
              if (!tmp.is_valid())
                logInfo("Connection string was invalid...\n");
              else {
                logDebug("Found connection string [%s]...\n", tmp->name().c_str());
                conn = tmp;
              }
            }
          }
        }
      }

      // XXX: review this, if connection_name is empty we may try to open an invalid connection.
      if (conn.is_valid() || connection_name.empty()) {
        logInfo("Opening SQL Editor window to '%s'...\n", connection_name.c_str());
        try {
          if (options->open_at_startup_type == "admin")
            add_new_admin_window(conn);
          else {
            if (conn.is_valid())
              add_new_query_window(conn);
            else {
              if (!options->open_at_startup.empty())
                open_script_file(options->open_at_startup);
              else
                add_new_query_window();
            }
          }
        } catch (std::exception &e) {
          logError("Error opening SQL editor to '%s': %s\n", connection_name.c_str(), e.what());
          throw;
        }
      } else {
        std::string message = strfmt(_("Invalid connection name '%s' given for --query"), connection_name.c_str());
        logWarning("%s\n", message.c_str());
        throw std::runtime_error(message);
      }
    } else if (options->open_at_startup_type == "run-script") {
      std::string script_file = options->open_at_startup;

      pushMessageHandler(new grt::SlotHolder(std::bind(output_to_stdout, std::placeholders::_1, std::placeholders::_2)));
      bec::GRTManager::get()->get_shell()->run_script_file(script_file);
      grt::GRT::get()->popMessageHandler();
    } else if (options->open_at_startup_type == "migration") {
      logInfo("Opening Migration Wizard...\n");
      add_new_plugin_window("wb.migration.open", "Migration Wizard");
    } else if (options->open_at_startup_type == "upgrade-mysql-dbs") {
      logInfo("Opening Database Copy Wizard...\n");
      add_new_plugin_window("wb.db.copy.open", "Database Copy Wizard");
    }

    if (!options->run_at_startup.empty()) {
      std::string lang = options->run_language;
      if (lang.empty()) {
        lang = "python";
        pushMessageHandler(new grt::SlotHolder(std::bind(output_to_stdout, std::placeholders::_1, std::placeholders::_2)));
      }
      bec::GRTManager::get()->get_shell()->run_script(options->run_at_startup, lang);
      grt::GRT::get()->popMessageHandler();
    }
  } catch (std::runtime_error &e) {
    // Errors from script execution are logged to the command line so we don't need to print out
    // another message. Notify user about it in the UI if we are supposed to not quit when done.
    if (!options->quit_when_done) {
      show_error(_("Error executing startup action"),
                 _("An error occurred while "
                   "the application executed the given startup action. The returned error is:\n\n") +
                   std::string(e.what()) + _("\n\nSee also the output window."));
    }
  }

  _initialization_finished = true;

  if (options->quit_when_done && (!options->run_at_startup.empty() || options->open_at_startup_type == "script" ||
                                  options->open_at_startup_type == "run-script"))
    _frontendCallbacks->quit_application();
}

//--------------------------------------------------------------------------------------------------

void WBContext::pushMessageHandler(grt::SlotHolder *slot) {
  _messageHandlerList.push_back(slot);
  grt::GRT::get()->pushMessageHandler(slot);

}

//--------------------------------------------------------------------------------------------------

bool WBContext::handle_message(const grt::Message &msg) {
  // No need to log messages here. That happens already in the grt manager.

  if (_send_messages_to_shell) {
    bec::GRTManager::get()->get_shell()->handle_msg(msg);
    return true;
  } else {
    if (bec::GRTManager::get()->get_messages_list()) {
      bec::GRTManager::get()->get_messages_list()->handle_message(msg);
      return true;
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

void WBContext::init_rdbms_modules() {
  logDebug("Initializing rdbms modules\n");

  // Init MySQL first.
  grt::Module *module = grt::GRT::get()->get_module("DbMySQL");
  if (!module)
    throw std::logic_error("DbMySQL module not found");
  grt::BaseListRef args(true);
  module->call_function("initializeDBMSInfo", args);

  // Will done prior to Migration init, to speed up app startup.
  //grt::GRT::get()->initializeOtherRDBMS();
}

grt::ValueRef WBContext::setup_context_grt(WBOptions *options) {
  std::shared_ptr<grt::internal::Unserializer> unserializer = grt::GRT::get()->get_unserializer();
  // init the GRT tree nodes, set default options
  init_grt_tree(options, unserializer);

  // Load last application state. This will only load it into the grt tree.
  // Components that have stored their settings will later read those values and reapply them.
  // This must be done as early as possible to provide all other parts their last saved state
  // when they are loading/initializing.
  load_app_state(unserializer);

  init_plugin_groups_grt(options);

  init_plugins_grt(options);

  // Initialize RDBMS specific modules. must happen before connections are loaded.
  init_rdbms_modules();

  // Table templates can be initialized only after rdbms info because it needs column datatypes.
  init_templates();

  for(auto &it: _components) {
    it->setup_context_grt(options);
  }

  // App options must be loaded after everything else is initialized.
  load_app_options(false);

  // Rescan plugins so that list of disabled plugins is applied.
  _plugin_manager->rescan_plugins();

  return grt::IntegerRef(1);
}

void WBContext::init_templates() {
  // Default table templates list
  grt::DictRef options(get_root()->options()->options());
  if (!options.has_key("TableTemplates")) {
    grt::ListRef<db_Table> templates =
      grt::ListRef<db_Table>::cast_from(grt::GRT::get()->unserialize(base::makePath(get_datadir(), "data/table_templates.xml")));
    options.set("TableTemplates", templates);
  }
}

void WBContext::init_grt_tree(WBOptions *options, std::shared_ptr<grt::internal::Unserializer> unserializer) {
  grt::DictRef root(true);
  workbench_WorkbenchRef app(grt::Initialized);

  root.set("wb", app);

  // setup application subtree
  {
    app_InfoRef info(grt::Initialized);
    GrtVersionRef version(grt::Initialized);
    info->owner(app);

    version->majorNumber(APP_MAJOR_NUMBER);
    version->minorNumber(APP_MINOR_NUMBER);
    version->releaseNumber(APP_RELEASE_NUMBER);
    version->buildNumber(APP_BUILD_NUMBER);
    version->status(1);

    info->name("MySQL Workbench");
    info->version(version);
    info->copyright("Oracle and/or its affiliates");
    info->license(APP_LICENSE_TYPE);
    info->edition(APP_EDITION_NAME);
    app->info(info);
  }

  {
    app_OptionsRef options(grt::Initialized);
    options->owner(app);

    append_contents(options->paperTypes(), get_paper_types(unserializer));

    set_default_options(options->options());

    app->options(options);
  }

  {
    app_RegistryRef registry(grt::Initialized);
    registry->owner(app);
    registry->appDataDirectory(bec::GRTManager::get()->get_basedir());
    registry->appExecutablePath(argv0 ? argv0 : "");

    app->registry(registry);
  }

  // ------------------

  db_mgmt_ManagementRef mgmt_info(grt::Initialized);

  // load datatype groups from XML

  ListRef<db_DatatypeGroup> grouplist;

  grouplist = ListRef<db_DatatypeGroup>::cast_from(
    grt::GRT::get()->unserialize(base::makePath(options->basedir, TYPE_GROUP_FILE), unserializer));
  for (size_t c = grouplist.count(), i = 0; i < c; i++) {
    grouplist[i]->owner(mgmt_info);
    mgmt_info->datatypeGroups().insert(grouplist[i]);
  }
  app->rdbmsMgmt(mgmt_info);

  grt::GRT::get()->set_root(root);
}

void WBContext::init_plugin_groups_grt(WBOptions *options) {
  struct group_def {
    const char *category;
    const char *name;
    const char *accessibilityName;
  } std_groups[] = {{"Database", "Database", "Database"},
                    {"Catalog", "Editors", "Editors"},
                    {"Application", "Workbench", "Workbench"},
                    {"Model", "Validation", "Validation"},
                    {"Model", "Export", "Export"},

                    {"Home", "Home", "Home"},
                    {"Home", "Home/Connections", "Connections"},
                    {"Home", "Home/ModelFiles", "Model Files"},
                    {"Home", "Home/Instances", "Instances"},

                    {"Model", "Menu/Text", "Model Text"},
                    {"SQLEditor", "Menu/Text", "SQL Editor Text"},

                    {"Model", "Menu/Model", "Model"},
                    {"Model", "Menu/Utilities", "Utilities"},
                    {"Catalog", "Menu/Catalog", "Catalog"},
                    {"Catalog", "Menu/Objects", "Objects"},
                    {"Database", "Menu/Database", "Database"},

                    {"Utilities", "Filter", "Filter"},
                    {"Utilities", "Menu/Utilities", "Utilities"},

                    {"SQLEditor", "Menu/SQL/Editor", "SQL Editor"},
                    {"SQLEditor", "Menu/SQL/Script", "SQL Script"},
                    {"SQLEditor", "Menu/SQL/Utilities", "SQL Utilities"},

                    {"Others", "Menu/Ungrouped", "Others"}};

  std::map<std::string, app_PluginGroupRef> groups;

  grt::ListRef<app_PluginGroup> group_list = grt::ListRef<app_PluginGroup>::cast_from(grt::GRT::get()->get(PLUGIN_GROUP_PATH));

  for (unsigned int i = 0; i < sizeof(std_groups) / sizeof(group_def); i++) {
    app_PluginGroupRef group(grt::Initialized);
    group->category(std_groups[i].category);
    group->name(std_groups[i].name);
    group->accessibilityName(std_groups[i].accessibilityName);

    group_list.insert(group);

    groups[std_groups[i].name] = group;
  }
}

void WBContext::init_plugins_grt(WBOptions *options) {
  std::map<std::string, bool> scanned_dir_list;
  std::list<std::string> exts;

#if defined(_MSC_VER)
  exts.push_back(".wbp.be");
#endif
  exts.push_back(".wbp");

  // scan user plugins
  std::string plugin_path = normalize_path(base::makePath(options->user_data_dir, "plugins"));
  grt::GRT::get()->send_output(strfmt("Looking for user plugins in %s\n", plugin_path.c_str()));

  bec::GRTManager::get()->do_scan_modules(plugin_path, exts, false);
  scanned_dir_list[plugin_path] = true;

  std::vector<std::string> paths = base::split(options->plugin_search_path, G_SEARCHPATH_SEPARATOR_S);

  for (size_t c = paths.size(), i = 0; i < c; i++) {
    if (scanned_dir_list.find(paths[i]) == scanned_dir_list.end() &&
        g_file_test(paths[i].c_str(), G_FILE_TEST_IS_DIR)) {
      std::string full_path = normalize_path(base::makePath(options->user_data_dir, paths[i]));

      if (scanned_dir_list.find(full_path) == scanned_dir_list.end()) {
        grt::GRT::get()->send_output(strfmt("Looking for plugins in %s\n", full_path.c_str()));
        bec::GRTManager::get()->do_scan_modules(paths[i], exts, false);
      }
      scanned_dir_list[paths[i]] = true;
    }
  }

  _plugin_manager->rescan_plugins();

  ValidationManager::scan();
}

void WBContext::init_properties_grt(workbench_DocumentRef &doc) {
  app_DocumentInfoRef info(grt::Initialized);
  info->name("Properties");
  info->owner(doc);

  info->caption("New Model");
  info->version("1.0");
  info->project("Name of the project");
  info->dateCreated(fmttime(0, DATETIME_FMT));
  info->dateChanged(fmttime(0, DATETIME_FMT));
  info->author(g_get_real_name());

  doc->info(info);
}

static void set_default(grt::DictRef dict, const char *option, int value) {
  if (!dict.has_key(option))
    dict.gset(option, value);
}

static void set_default(grt::DictRef dict, const char *option, const std::string &value) {
  if (!dict.has_key(option) || option[0] == '@')
    dict.gset(option, value);
}

/**
 ****************************************************************************
 * @brief Sets Workbench specific options
 *
 * To get a configuration option, use GRTManager::get_app_option()
 *
 ****************************************************************************
 */
void WBContext::set_default_options(grt::DictRef options) {
  set_default(options, "workbench:ForceSWRendering", 0);
  set_default(options, "workbench:OSSHideMissing", 0);
  set_default(options, "workbench:UndoEntries", DEFAULT_UNDO_STACK_SIZE);
  set_default(options, "workbench:AutoSaveModelInterval", AUTO_SAVE_MODEL_INTERVAL);
  set_default(options, "workbench:AutoSaveSQLEditorInterval", AUTO_SAVE_SQLEDITOR_INTERVAL);
  set_default(options, "workbench.AutoReopenLastModel", 0);
  set_default(options, "workbench:SaveSQLWorkspaceOnClose", 1);
  set_default(options, "workbench:InternalSchema", ".mysqlworkbench");

  set_default(options, "workbench.physical:DeleteObjectConfirmation", "ask");

  set_default(options, "HomeScreen:HeadingMessage", 1); // By default display Welcome Message

  set_default(options, "grtshell:ShellLanguage", "python");
  set_default(options, "@grtshell:ShellLanguage/Items", "python");

  // URL of latest versions file (used by version updater)
  set_default(options, "VersionsFileURL", "http://wb.mysql.com/versions.php");

  // SQL parsing options
  set_default(options, "SqlIdentifiersCS", 1);
  set_default(options, "SqlMode", "");
  set_default(options, "SqlDelimiter", "$$");

  set_default(options, "SqlEditor::SyntaxCheck::MaxErrCount", 100);

  // All editors
  set_default(options, "Editor:TabIndentSpaces", 0);
  set_default(options, "Editor:TabWidth", 4);
  set_default(options, "Editor:IndentWidth", 4);

  // DB SQL editor
  set_default(options, "DbSqlEditor:SchemaTreeRestoreState", 1);
  set_default(options, "DbSqlEditor:CodeCompletionEnabled", 1);
  set_default(options, "DbSqlEditor:AutoStartCodeCompletion", 1);
  set_default(options, "DbSqlEditor:CodeCompletionUpperCaseKeywords", 0);
  set_default(options, "DbSqlEditor:ProgressStatusUpdateInterval", 500); // in ms
  set_default(options, "DbSqlEditor:KeepAliveInterval", 600);            // in seconds
  set_default(options, "DbSqlEditor:ReadTimeOut", 30);                  // in seconds
  set_default(options, "DbSqlEditor:ConnectionTimeOut", 60);             // in seconds
  set_default(options, "DbSqlEditor:MaxQuerySizeToHistory", 65536);
  set_default(options, "DbSqlEditor:ContinueOnError", 0); // continue running sql script bypassing failed statements
  set_default(options, "DbSqlEditor:AutocommitMode", 1);  // when enabled, each statement will be committed immediately
  set_default(options, "DbSqlEditor:IsDataChangesCommitWizardEnabled", 1);
  set_default(options, "DbSqlEditor:ShowSchemaTreeSchemaContents", 1);
  set_default(options, "DbSqlEditor:SafeUpdates", 1);
  set_default(options, "DbSqlEditor:ShowWarnings", 1);
  set_default(options, "DbSqlEditor:ReformatViewDDL", 1);
  set_default(options, "DbSqlEditor:OnlineDDLAlgorithm", "DEFAULT");
  set_default(options, "DbSqlEditor:OnlineDDLLock", "DEFAULT");

  set_default(options, "DbSqlEditor:DiscardUnsavedQueryTabs", 0);
  set_default(options, "DbSqlEditor:SQLCommentTypeForHotkey", "--");
  set_default(options, "DbSqlEditor:DisableAutomaticContextHelp", 1);

  set_default(options, "DbSqlEditor:Reformatter:UpcaseKeywords", 1);
  set_default(options, "DbSqlEditor::MaxResultsets", 50);

  // Migration
  set_default(options, "Migration:ConnectionTimeOut", 60); // in seconds

  // Recordset
  set_default(options, "Recordset:FloatingPointVisibleScale", 3);
  set_default(options, "Recordset:FieldValueTruncationThreshold", 256);
  set_default(options, "SqlEditor:LimitRows", 1);
  set_default(options, "SqlEditor:LimitRowsCount", 1000);
  set_default(options, "SqlEditor:PreserveRowFilter", 1);
  set_default(options, "SqlEditor:geographicLocationURL", "http://www.openstreetmap.org/?mlat=%LAT%&mlon=%LON%");

  // Name templates
  set_default(options, "PkColumnNameTemplate", "id%table%");
  set_default(options, "DefaultPkColumnType", "INT");

  set_default(options, "ColumnNameTemplate", "%table%col");
  set_default(options, "DefaultColumnType", "VARCHAR(45)");

  set_default(options, "FKNameTemplate", "fk_%stable%_%dtable%");
  set_default(options, "FKColumnNameTemplate", "%table%_%column%");
  set_default(options, "AuxTableTemplate", "%stable%_has_%dtable%");

  // Model Defaults
  set_default(options, "DefaultFigureNotation", "workbench/default");
  set_default(options, "DefaultConnectionNotation", "crowsfoot");
  set_default(options, "AlignToGrid", 0);

  set_default(options, "SynchronizeObjectColors", 1);

  // MySQL Defaults
  set_default(options, "DefaultTargetMySQLVersion", base::getVersion());

  set_default(options, "db.mysql.Table:tableEngine", "InnoDB");
  set_default(options, "SqlGenerator.Mysql:SQL_MODE", "ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION");
#ifdef HAVE_BUNDLED_MYSQLDUMP
  set_default(options, "mysqldump", "");
  set_default(options, "mysqlclient", "");
#else
  set_default(options, "mysqldump", "mysqldump");
  set_default(options, "mysqlclient", "mysql");
#endif

#ifdef _MSC_VER
  std::string homedir = mforms::Utilities::get_special_folder(mforms::Documents);
  set_default(options, "dumpdirectory", homedir + "\\dumps");
#else
  std::string homedir = "~";
  set_default(options, "dumpdirectory", homedir + "/dumps");
#endif

  // FK defaults
  set_default(options, "db.ForeignKey:deleteRule", "NO ACTION");
  set_default(options, "db.ForeignKey:updateRule", "NO ACTION");

  // Default Colors
  set_default(options, "workbench.model.Layer:Color", "#F0F1FE");
  set_default(options, "workbench.model.NoteFigure:Color", "#FEFDED");

  set_default(options, "workbench.physical.Diagram:DrawLineCrossings", 0);
  set_default(options, "workbench.physical.ObjectFigure:Expanded", 1);
  set_default(options, "workbench.physical.TableFigure:ShowColumnTypes", 1);
  set_default(options, "workbench.physical.TableFigure:ShowColumnFlags", 0);
  set_default(options, "workbench.physical.TableFigure:MaxColumnTypeLength", 20);
  set_default(options, "workbench.physical.TableFigure:MaxColumnsDisplayed", 30);
  set_default(options, "workbench.physical.RoutineGroupFigure:MaxRoutineNameLength", 20);

  set_default(options, "workbench.physical.TableFigure:Color", "#98BFDA");
  set_default(options, "workbench.physical.ViewFigure:Color", "#FEDE58");
  set_default(options, "workbench.physical.RoutineGroupFigure:Color", "#98D8A5");

  // Default Fonts

  set_default(options, "workbench.physical.FontSet:Name", "Default (Western)");

  set_default(options, "workbench.physical.TableFigure:TitleFont", DEFAULT_FONT_FAMILY " Bold 12");
  set_default(options, "workbench.physical.TableFigure:SectionFont", DEFAULT_FONT_FAMILY " Bold 11");
  set_default(options, "workbench.physical.TableFigure:ItemsFont", DEFAULT_FONT_FAMILY " 11");
  set_default(options, "workbench.physical.ViewFigure:TitleFont", DEFAULT_FONT_FAMILY " Bold 12");
  set_default(options, "workbench.physical.RoutineGroupFigure:TitleFont", DEFAULT_FONT_FAMILY " Bold 12");
  set_default(options, "workbench.physical.RoutineGroupFigure:ItemsFont", DEFAULT_FONT_FAMILY " 12");
  set_default(options, "workbench.physical.Connection:CaptionFont", DEFAULT_FONT_FAMILY " 11");
  set_default(options, "workbench.physical.Layer:TitleFont", DEFAULT_FONT_FAMILY " 11");
  set_default(options, "workbench.model.NoteFigure:TextFont", DEFAULT_FONT_FAMILY " 11");

#if defined(_MSC_VER)
  set_default(options, "workbench.general.Resultset:Font", DEFAULT_FONT_FAMILY " 8");
  if (get_local_os_name().find("Windows XP") != std::string::npos) {
    set_default(options, "workbench.general.Editor:Font", DEFAULT_MONOSPACE_FONT_FAMILY_ALT " 10");
    set_default(options, "workbench.scripting.ScriptingShell:Font", DEFAULT_MONOSPACE_FONT_FAMILY_ALT " 10");
    set_default(options, "workbench.scripting.ScriptingEditor:Font", DEFAULT_MONOSPACE_FONT_FAMILY_ALT " 10");
  } else {
    set_default(options, "workbench.general.Editor:Font", DEFAULT_MONOSPACE_FONT_FAMILY " 10");
    set_default(options, "workbench.scripting.ScriptingShell:Font", DEFAULT_MONOSPACE_FONT_FAMILY " 10");
    set_default(options, "workbench.scripting.ScriptingEditor:Font", DEFAULT_MONOSPACE_FONT_FAMILY " 10");
  }
#elif defined(__APPLE__)
  set_default(options, "workbench.general.Resultset:Font", DEFAULT_FONT_FAMILY " 11");
  set_default(options, "workbench.general.Editor:Font", DEFAULT_MONOSPACE_FONT_FAMILY " 13");
  set_default(options, "workbench.scripting.ScriptingShell:Font", DEFAULT_MONOSPACE_FONT_FAMILY " 13");
  set_default(options, "workbench.scripting.ScriptingEditor:Font", DEFAULT_MONOSPACE_FONT_FAMILY " 13");
#else
  set_default(options, "workbench.general.Resultset:Font", DEFAULT_FONT_FAMILY " 11");
  set_default(options, "workbench.general.Editor:Font", DEFAULT_MONOSPACE_FONT_FAMILY " 11");
  set_default(options, "workbench.scripting.ScriptingShell:Font", DEFAULT_MONOSPACE_FONT_FAMILY " 11");
  set_default(options, "workbench.scripting.ScriptingEditor:Font", DEFAULT_MONOSPACE_FONT_FAMILY " 11");
#endif

  std::string colors;
  colors = "#FFEEEC\n";
  colors += "#FEFDED\n";
  colors += "#EAFFE5\n";
  colors += "#ECFDFF\n";
  colors += "#F0F1FE\n";
  colors += "#FFEBFA\n";
  set_default(options, "workbench.model.Figure:ColorList", colors);

  colors = "#98BFDA\n";
  colors += "#FEDE58\n";
  colors += "#98D8A5\n";
  colors += "#FE9898\n";
  colors += "#FE98FE\n";
  colors += "#FFFFFF\n";
  set_default(options, "workbench.model.ObjectFigure:ColorList", colors);

  set_default(options, "@ColorScheme/Items",
              "System Default:0,Windows 7:1,Windows 8:2,Windows 8 (alternative):3,High Contrast:4");

  // Advanced options
  // Option can't be turned off by default. We've got a situation with connector c++ on windows
  // if tunnel will get closed just before calling mysql_ping() (keep alive) then ping will try to use invalid
  // mysql->net structure.
  set_default(options, "SSH:keepalive", 60);
  set_default(options, "SSH:connectTimeout", 10);
  set_default(options, "SSH:BufferSize", 10240);
  set_default(options, "SSH:maxFileSize", 100*ONE_MB);  // Set limit to 100MB by default.
  set_default(options, "SSH:logSize", 100*ONE_MB);  // Set limit to 100MB by default.

  set_default(options, "SSH:readWriteTimeout", 5);
  set_default(options, "SSH:commandTimeout", 1);
  set_default(options, "SSH:commandRetryCount", 3);

#ifndef _MSC_VER
  set_default(options, "SSH:pathtosshconfig", base::expand_tilde("~/.ssh/config"));
#endif

  // Other options
  set_default(options, "workbench.physical.Connection:ShowCaptions", 0);
  set_default(options, "workbench.physical.Connection:CenterCaptions", 0);

  // By the time we make it here, Logger's log level has already been set to default (whatever it may be).
  // NOTE that there's a cornercase we ignore: if user set --log-level or WB_LOG_LEVEL, this is what
  // Logger::active_level() will return instead.
  // But since set_default() only has effect the first time the Workbench is run, this shouldn't really matter for the
  // user while keeping our code simpler.
  set_default(options, "workbench.logger:LogLevel", base::Logger::active_level());
}

grt::ListRef<app_PaperType> WBContext::get_paper_types(std::shared_ptr<grt::internal::Unserializer> unserializer) {
  return grt::ListRef<app_PaperType>::cast_from(
    grt::GRT::get()->unserialize(base::makePath(get_datadir(), "data/paper_types.xml"), unserializer));
}

static void strip_options_dict(grt::DictRef dict) {
  std::vector<std::string> keys;
  {
    grt::DictRef::const_iterator iter = dict.begin();
    grt::DictRef::const_iterator end = dict.end();

    while (iter != end) {
      if (iter->first[0] == '@')
        keys.push_back(iter->first);
      ++iter;
    }
  }

  for (std::vector<std::string>::const_iterator end = keys.end(), iter = keys.begin(); iter != end; ++iter) {
    dict.remove(*iter);
  }
}

void WBContext::setLogLevelFromGuiPreferences(const grt::DictRef &dict) {
  // don't set if user already specified log level (via commmandline or shell env variable)
  if (base::Logger::wasLogLevelSpecifiedByUser())
    return;

  std::string currentLogLevel = base::Logger::active_level();
  std::string prefsLogLevel = dict.get_string("workbench.logger:LogLevel", currentLogLevel);

  if (currentLogLevel != prefsLogLevel) {
    bool ok = base::Logger::active_level(prefsLogLevel);
    if (ok)
      logInfo("Log level changed to '%s' according to UI option\n", prefsLogLevel.c_str());
    else
      assert(0);
  }
}

void WBContext::load_app_options(bool update) {
  // load ui related stuff (menus, toolbars etc)
  wb::WBContextUI::get()->load_app_options(update);

  // load saved options
  std::string options_xml = base::makePath(_user_datadir, OPTIONS_FILE_NAME);
  if (g_file_test(options_xml.c_str(), G_FILE_TEST_EXISTS)) {
    try {
      app_OptionsRef curOptions(get_root()->options());

      xmlDocPtr xmlDocument = grt::GRT::get()->load_xml(options_xml);
      if (!xmlDocument) {
        throw std::runtime_error(
          _("The file is not a valid MySQL Workbench options file.\n"
            "The file will skipped and settings are reset to their default values."));
      }

      base::ScopeExitTrigger free_on_leave(std::bind(xmlFreeDoc, xmlDocument));

      std::string doctype, version;
      grt::GRT::get()->get_xml_metainfo(xmlDocument, doctype, version);

      // Older option files without a version number are considered as 1.0.0 and
      // upgraded from there to latest version.
      if (version.empty())
        version = "1.0.0";
      else
        // Document format has been introduced in 1.0.1.
        if (doctype != OPTIONS_DOCUMENT_FORMAT) {
        throw std::runtime_error(
          _("The file is not a valid MySQL Workbench options file.\n"
            "The file will skipped and settings are reset to their default values."));
      }

      // Try to upgrade document at XML level.
      if (version != OPTIONS_DOCUMENT_VERSION)
        attempt_options_upgrade(xmlDocument, version);

      grt::ValueRef options_value = grt::GRT::get()->unserialize_xml(xmlDocument, options_xml);
      app_OptionsRef options(app_OptionsRef::cast_from(options_value));

      if (options.is_valid()) {
        // strip stuff that are not options
        strip_options_dict(options->options());
        strip_options_dict(options->commonOptions());

        // set loaded options dict
        grt::merge_contents(curOptions->options(), options->options(), true);

        grt::merge_contents(curOptions->commonOptions(), options->commonOptions(), true);

        setLogLevelFromGuiPreferences(options->options());

        // set loaded recent files list (if they exist)
        while (curOptions->recentFiles().count() > 0)
          curOptions->recentFiles().remove(0);
        for (grt::StringListRef::const_iterator file = options->recentFiles().begin();
             file != options->recentFiles().end(); ++file) {
          if (g_file_test((*file).c_str(), G_FILE_TEST_EXISTS))
            curOptions->recentFiles().insert(*file);
        }

        // set loaded disabled plugins list
        grt::replace_contents(curOptions->disabledPlugins(), options->disabledPlugins());

        // merge paper types (so we get custom types)
        grt::merge_contents_by_id(grt::ObjectListRef::cast_from(curOptions->paperTypes()),
                                  grt::ObjectListRef::cast_from(options->paperTypes()), false);
        grt::ListRef<app_PaperType> paperTypes = curOptions->paperTypes();

        for (size_t c = paperTypes.count(), i = 0; i < c; i++) {
          app_PaperTypeRef value(paperTypes[i]);

          if (value.is_valid())
            value->owner(curOptions);
        }

        // Load custom application colors and color scheme.
        base::ColorScheme scheme = base::ColorSchemeStandard;
        grt::ValueRef value = curOptions->options()["ColorScheme"];
        if (value.is_valid()) {
          grt::IntegerRef int_value = grt::IntegerRef::cast_from(value);
          scheme = (base::ColorScheme)*int_value;
        }
        base::Color::set_active_scheme(scheme);

        options->reset_references();
      }
    } catch (std::exception &exc) {
      mforms::Utilities::show_error(_("Error while loading options"),
                                    strfmt("The file '%s' could not be loaded: %s", options_xml.c_str(), exc.what()),
                                    _("Close"));
      grt::GRT::get()->send_error(strfmt("Error while loading '%s': %s", options_xml.c_str(), exc.what()));
    }
  } else {
    // No config file, so maybe initial start. Set at least a default color scheme.
    base::Color::set_active_scheme(base::ColorSchemeStandard);
  }

  // commit options
  option_dict_changed();

  cleanup_options();

  // load options specific for other parts of wb
  for(auto &it: _components) {
    it->load_app_options(update);
  }
  // load list of server instances
  db_mgmt_ManagementRef mgmt = get_root()->rdbmsMgmt();
  std::string inst_list_xml = base::makePath(get_user_datadir(), SERVER_INSTANCE_LIST);
  if (g_file_test(inst_list_xml.c_str(), G_FILE_TEST_EXISTS)) {
    try {
      grt::ListRef<db_mgmt_ServerInstance> list(
        grt::ListRef<db_mgmt_ServerInstance>::cast_from(grt::GRT::get()->unserialize(inst_list_xml)));

      if (list.is_valid()) {
        while (mgmt->storedInstances().count() > 0)
          mgmt->storedInstances().remove(0);
        for (size_t c = list.count(), i = 0; i < c; i++) {
          // starting from 5.2.16, passwords are not stored in the profile anymore

          mgmt->storedInstances().insert(list.get(i));
        }
      }
    } catch (std::exception &exc) {
      grt::GRT::get()->send_warning(strfmt("Error loading '%s': %s", inst_list_xml.c_str(), exc.what()));
    }
  }
}

void WBContext::load_other_connections() {
  // load list of non-MySQL connections
  unsigned int connection_count = 0;
  unsigned int total_connections = 0;
  db_mgmt_ManagementRef mgmt = get_root()->rdbmsMgmt();
  std::string conn_list_xml = base::makePath(get_user_datadir(), FILE_OTHER_CONNECTION_LIST);
  if (g_file_test(conn_list_xml.c_str(), G_FILE_TEST_EXISTS)) {
    try {
      grt::ListRef<db_mgmt_Connection> list(
        grt::ListRef<db_mgmt_Connection>::cast_from(grt::GRT::get()->unserialize(conn_list_xml)));
      total_connections = (int)list->count();
      if (list.is_valid()) {
        replace_contents(mgmt->otherStoredConns(), list);

        GRTLIST_FOREACH(db_mgmt_Connection, list, iter)
          (*iter)->owner(mgmt);
      }

      ++connection_count;
    } catch (std::exception &exc) {
      logError("Error loading %s: %s\n", conn_list_xml.c_str(), exc.what());
    }
  }

  logInfo("Loaded %u/%u new non-MySQL connections\n", connection_count, total_connections);
}

void WBContext::attempt_options_upgrade(xmlDocPtr xmldoc, const std::string &version) {
  std::vector<std::string> ver = base::split(version, ".");

  int major = base::atoi<int>(ver[0], 0);
  int minor = base::atoi<int>(ver[1], 0);
  int revision = base::atoi<int>(ver[2], 0);

  // Version * -> 1.0.1
  // Performed changes:
  //   * Removed formPositions tag from options tag.
  if (major == 1 && minor == 0 && revision == 0) {
    XMLTraverser xml(xmldoc);
    std::vector<xmlNodePtr> options_list(xml.scan_objects_of_type("app.Options"));

    for (size_t c = options_list.size(), i = 0; i < c; i++)
      xml.delete_object_item(options_list[i], "formPositions");

    revision = 1;
  }
}

void WBContext::save_app_options() {
  std::string options_file = base::makePath(_user_datadir, OPTIONS_FILE_NAME);
  app_OptionsRef options(get_root()->options());

  // set owner of options to nil so that it wont point to a bogus object when loading
  GrtObjectRef owner(options->owner());
  options->owner(GrtObjectRef());

  grt::GRT::get()->serialize(options, options_file + ".tmp", OPTIONS_DOCUMENT_FORMAT, OPTIONS_DOCUMENT_VERSION);
  g_remove(options_file.c_str());
  g_rename(std::string(options_file + ".tmp").c_str(), options_file.c_str());

  options->owner(owner);

  FOREACH_COMPONENT(_components, iter)
  (*iter)->save_app_options();
}

void WBContext::save_instances() {
  // save instance list
  db_mgmt_ManagementRef mgmt = get_root()->rdbmsMgmt();
  if (!mgmt.is_valid())
    return;
  std::string inst_list_xml = base::makePath(get_user_datadir(), SERVER_INSTANCE_LIST);
  grt::GRT::get()->serialize(mgmt->storedInstances(), inst_list_xml);
}

void WBContext::save_connections() {
  db_mgmt_ManagementRef mgmt = get_root()->rdbmsMgmt();
  if (!mgmt.is_valid()) {
    logError("Failed to save connections (Invalid RDBMS management reference).\n");
    return;
  }
  // save other connections list
  if (mgmt->otherStoredConns()->count()) {
    std::string conn_list_xml = base::makePath(get_user_datadir(), FILE_OTHER_CONNECTION_LIST);
    grt::GRT::get()->serialize(mgmt->otherStoredConns(), conn_list_xml);
    logDebug("Saved connection list (Non-MySQL: %u)\n", (unsigned int)mgmt->otherStoredConns()->count());
  }

  grt::GRT::get()->serialize(mgmt->storedConns(), base::makePath(get_user_datadir(), FILE_CONNECTION_LIST));
  logDebug("Saved connection list (MySQL: %u)\n", (unsigned int)mgmt->storedConns()->count());
}

//--------------------------------------------------------------------------------------------------

void WBContext::load_app_state(std::shared_ptr<grt::internal::Unserializer> unserializer) {
  // Load saved state.
  std::string state_xml = base::makePath(_user_datadir, STATE_FILE_NAME);
  if (g_file_test(state_xml.c_str(), G_FILE_TEST_EXISTS)) {
    xmlDocPtr xmlDocument = NULL;
    try {
      xmlDocument = grt::GRT::get()->load_xml(state_xml);
      base::ScopeExitTrigger free_on_leave(std::bind(xmlFreeDoc, xmlDocument));

      std::string doctype, version;
      grt::GRT::get()->get_xml_metainfo(xmlDocument, doctype, version);

      if (doctype != STATE_DOCUMENT_FORMAT) {
        throw std::runtime_error(
          _("The file is not a valid MySQL Workbench state file.\n"
            "The file will skipped and the application starts in its default state."));
      }

      grt::DictRef current_state(get_root()->state());
      grt::DictRef new_state = grt::DictRef::cast_from(grt::GRT::get()->unserialize_xml(xmlDocument, state_xml));

      // Store new state in grt tree.
      grt::merge_contents(current_state, new_state, true);
    } catch (std::exception &exc) {
      mforms::Utilities::show_error(_("Error while loading application state"),
                                    strfmt("The file '%s' could not be loaded: %s", state_xml.c_str(), exc.what()),
                                    _("Close"));
      grt::GRT::get()->send_warning(strfmt("Error while loading '%s': %s", state_xml.c_str(), exc.what()));
    }
  }

  // restore stuff from grt shell
  bec::GRTManager::get()->get_shell()->restore_state();
}

//--------------------------------------------------------------------------------------------------

void WBContext::save_app_state() {
  // Keep the current version number so we can compare on next startup if a new version was
  // launched the first time.
  std::string version = strfmt("%i.%i.%i", APP_MAJOR_NUMBER, APP_MINOR_NUMBER, APP_RELEASE_NUMBER);
  save_state("last-run-as", "global", version);

  std::string state_file = base::makePath(_user_datadir, STATE_FILE_NAME);
  grt::GRT::get()->serialize(get_root()->state(), state_file + ".tmp", STATE_DOCUMENT_FORMAT, STATE_DOCUMENT_VERSION);
  g_remove(state_file.c_str());
  g_rename(std::string(state_file + ".tmp").c_str(), state_file.c_str());

  try {
    bec::GRTManager::get()->get_shell()->store_state();
  } catch (std::exception &exc) {
    std::string message = base::strfmt("Error saving GRT shell state: %s", exc.what());
    grt::GRT::get()->send_error(message);
  }
}

//--------------------------------------------------------------------------------------------------

void WBContext::add_recent_file(const std::string &file) {
  grt::StringListRef recentFiles(get_root()->options()->recentFiles());
  recentFiles.remove_value(file);
  recentFiles.insert(file, 0);

  // TODO: Make the max number of files configurable.
  while (recentFiles.count() > 20)
    recentFiles.remove(20);
  save_app_options();

  wb::WBContextUI::get()->refresh_home_documents();
}

//--------------------------------------------------------------------------------------------------

void WBContext::option_dict_changed(grt::internal::OwnedDict *options, bool, const std::string &) {
  if (get_wb_options() == grt::DictRef(options)) {
    ssize_t undo_size = get_wb_options().get_int("workbench:UndoEntries", DEFAULT_UNDO_STACK_SIZE);

    if (undo_size == 0)
      undo_size = 1;

    grt::GRT::get()->get_undo_manager()->set_undo_limit(undo_size);
  }
}

#endif // Setup____

/**
 * Cancels all pending grt idle tasks and refreshes in the context. Useful to avoid crashes
 * or meaningless GUI overhead when closing down (parts of) WB.
 * Warning: canceling idle tasks unconditionally might lead to other problems, so use with extreme care.
 */
bool WBContext::cancel_idle_tasks() {
  bool result = bec::GRTManager::get()->cancel_idle_tasks();

  MutexLock lock(_pending_refresh_mutex);
  _pending_refreshes.clear();

  return result;
}

void WBContext::flush_idle_tasks(bool force) {
  try {
    bec::GRTManager::get()->perform_idle_tasks();

    if (_user_interaction_blocked) {
      return;
    }

    mdc::Timestamp now = mdc::get_time();

    std::list<RefreshRequest> refreshes;
    {
      MutexLock lock(_pending_refresh_mutex);
      // separate the requests that can be executed now
      std::list<RefreshRequest>::iterator iter = _pending_refreshes.begin();
      while (iter != _pending_refreshes.end()) {
        std::list<RefreshRequest>::iterator next = iter;
        ++next;

        if (force || (now - iter->timestamp >= UI_REQUEST_THROTTLE)) {
          refreshes.push_back(*iter);
          _pending_refreshes.erase(iter);
        }

        iter = next;
      }
    }

    // send the refresh requests
    for (std::list<RefreshRequest>::iterator iter = refreshes.begin(); iter != refreshes.end(); ++iter)
      _frontendCallbacks->refresh_gui(iter->type, iter->str, iter->ptr);
  } catch (std::exception &exc) {
    logException("WBContext: exception in flush idle task", exc);
  }
}

void WBContext::request_refresh(RefreshType type, const std::string &str, NativeHandle ptr) {
  MutexLock lock(_pending_refresh_mutex);

  mdc::Timestamp now = mdc::get_time();

  // check if dupe
  for (std::list<RefreshRequest>::iterator iter = _pending_refreshes.begin(); iter != _pending_refreshes.end();
       ++iter) {
    if (iter->type == type && iter->str == str && iter->ptr == ptr) {
      // if its a dupe, update the timestamp so that notifications are only sent when
      // there's no more fresh requests arriving
      iter->timestamp = now;
      return;
    }
  }

  RefreshRequest refresh;
  refresh.type = type;
  refresh.str = str;
  refresh.ptr = ptr;
  refresh.timestamp = now;

#if !defined(_MSC_VER) && !defined(__APPLE__)
  // XXX: check this requirement. Probably already fixed since this hack was added.

  // Do not remove the following refresh! W/o it linux version hangs at times.
  if (_frontendCallbacks->refresh_gui && _pending_refreshes.empty())
    _frontendCallbacks->refresh_gui(RefreshNeeded, "", (NativeHandle)0);
#endif

  _pending_refreshes.push_back(refresh);
}

#ifndef Document____

//--------------------------------------------------------------------------------
// Creating/Loading documents

void WBContext::new_document() {
  try {
    _frontendCallbacks->show_status_text(_("Creating new document..."));

    // Ask whether unsaved changes should be saved.
    if (has_unsaved_changes()) {
      int answer = mforms::Utilities::show_message(
        _("New Document"),
        _("Only one model can be open at a time. Do you want to save pending changes to the document?\n\n"
          "If you don't save your changes, they will be lost."),
        _("Save"), _("Cancel"), _("Don't Save"));
      if (answer == mforms::ResultOk) {
        if (!save_as(_filename))
          return;
      } else if (answer == mforms::ResultCancel)
        return;
    }

    block_user_interaction(true);

    // close the current document
    do_close_document(false);

    _model_context = new WBContextModel();

    // create an empty document and add a physical model to it
    workbench_DocumentRef doc(grt::Initialized);
    workbench_WorkbenchRef wb(get_root());
    wb->doc(doc);

    // mark the document as a global object, so that child objects have changes tracked for undo
    doc->mark_global();

    doc->owner(wb);

    {
      // setup default page settings
      app_PageSettingsRef page(grt::Initialized);

      page->owner(doc);
      page->paperType(grt::find_named_object_in_list(wb->options()->paperTypes(), "iso-a4"));
      if (!page->paperType().is_valid()) {
        // if there is no paperType available, create A4
        app_PaperTypeRef paperType(grt::Initialized);

        paperType->owner(page);
        paperType->name("iso-a4");
        paperType->caption("A4 (210 mm x 297 mm)");
        paperType->width(210.0);
        paperType->height(297.0);
        paperType->marginsSet(0);

        page->paperType(paperType);
      }

      page->marginTop(6.35);
      page->marginBottom(14.46);
      page->marginLeft(6.35);
      page->marginRight(6.35);
      page->orientation(PAPER_PORTRAIT);

      doc->pageSettings(page);
    }

    init_properties_grt(doc);

    _file = new ModelFile(get_auto_save_dir());

    scoped_connect(_file->signal_changed(),
                   std::bind(&WBContext::request_refresh, this, RefreshDocument, "", static_cast<NativeHandle>(0)));

    _file->create();
    bec::GRTManager::get()->set_db_file_path(_file->get_db_file_path());

    _filename = "";

    wb->docPath(_filename);

    _model_context->model_created(_file, doc);

    reset_document();

    _save_point = grt::GRT::get()->get_undo_manager()->get_latest_undo_action();
    request_refresh(RefreshDocument, "");

    bec::GRTManager::get()->run_once_when_idle(std::bind(_frontendCallbacks->perform_command, "reset_layout"));
    bec::GRTManager::get()->run_once_when_idle(std::bind(&WBContext::block_user_interaction, this, false));

    _frontendCallbacks->show_status_text(_("New document."));
  } catch (grt::grt_runtime_error &error) {
    show_error(error.what(), error.detail);
    _frontendCallbacks->show_status_text(_("Error creating new document."));
  }
}

/** Tells backend that frontend has finished preparing for a newly created or loaded model.
 Call as last thing done from the RefreshNewModel handler.
 */
void WBContext::new_model_finish() {
  _model_context->realize();
}

void WBContext::open_script_file(const std::string &file) {
  execute_in_main_thread("openscript", std::bind(&WBContextSQLIDE::open_document, _sqlide_context, file), false);
}

void WBContext::open_recent_document(int index) {
  if (index - 1 < (int)get_root()->options()->recentFiles().count()) {
    std::string file = get_root()->options()->recentFiles().get(index - 1);

    if (g_str_has_suffix(file.c_str(), ".mwb"))
      open_document(file);
    else
      open_script_file(file);
  }
}

bool WBContext::open_file_by_extension(const std::string &path, bool interactive) {
  if (g_str_has_suffix(path.c_str(), ".mwbplugin") || g_str_has_suffix(path.c_str(), ".mwbpluginz")) {
    // install plugin
    if (interactive)
      return wb::WBContextUI::get()->start_plugin_install(path);

    install_module_file(path);
    return true;
  } else if (g_str_has_suffix(path.c_str(), ".mwb")) {
    // open document
    return open_document(path);
  } else if (g_str_has_suffix(path.c_str(), ".sql")) {
    SqlEditorForm *form = _sqlide_context->get_active_sql_editor();
    if (form) {
      form->open_file(path, true);
      return true;
    }
    _sqlide_context->open_document(path);
    return false;
  } else {
    if (interactive) {
      show_error(_("Unrecognized File Type"),
                 base::strfmt(_("MySQL Workbench does not know how to open file %s"), path.c_str()));
    }
    return false;
  }
}

void WBContext::reset_document() {
  grt::GRT::get()->get_undo_manager()->reset();

  wb::WBContextUI::get()->reset();

  _clipboard->clear();
  _clipboard->set_content_description("");
  // refresh internal environment of loaders
  grt::GRT::get()->refresh_loaders();
}

int WBContext::closeModelFile() {
  if (_model_import_file) {
    delete _model_import_file;
    _model_import_file = 0;
  }
  return 0;
};

std::string WBContext::getTempDir() {
  if (_model_import_file)
    return _model_import_file->get_tempdir_path();
  return "";
}

std::string WBContext::getDbFilePath() {
  if (_model_import_file)
    return _model_import_file->get_db_file_path();
  return "";
};

workbench_DocumentRef WBContext::openModelFile(const std::string &file) {
  workbench_DocumentRef doc;
  closeModelFile();
  _model_import_file = new ModelFile(get_auto_save_dir());

  try {
    if (base::string_compare(file, get_filename(), false) == 0) {
      mforms::Utilities::show_message("Open Document",
                                      "Error while including another model. A model cannot be added to itself.", "OK");
      return doc;
    }
    _model_import_file->open(file);
    //    _manager->set_db_file_path(_file->get_db_file_path());

    doc = _model_import_file->retrieve_document();
  } catch (std::exception &exc) {
    show_exception(strfmt(_("Cannot open document '%s'."), file.c_str()), exc);
  }

  return doc;
};

bool WBContext::open_document(const std::string &file) {
  if (_model_context != NULL) {
    // A model is already loaded. Warn the user it will be closed. Ask for saving pending changes
    // if there are any.
    if (has_unsaved_changes()) {
      int answer = execute_in_main_thread<int>("check save changes",
                                               std::bind(mforms::Utilities::show_message, _("Open Document"),
                                                         _("Only one model can be open at a time. Do you wish to "
                                                           "save pending changes to the currently open model?\n\n"
                                                           "If you don't they will be lost."),
                                                         _("Save"), _("Cancel"), _("Don't Save")));

      if (answer == mforms::ResultOk) {
        if (!save_as(_filename))
          return false;
      } else if (answer == mforms::ResultCancel)
        return false;
    } else {
      int answer = execute_in_main_thread<int>(
        "replace document", std::bind(mforms::Utilities::show_message, _("Open Document"),
                                      _("Opening another model will close the currently open model.\n\n"
                                        "Do you wish to proceed opening it?"),
                                      _("Open"), _("Cancel"), ""));

      if (answer != mforms::ResultOk)
        return false;
    }

    execute_in_main_thread("close document", std::bind(&WBContext::do_close_document, this, false), true);
  }

  _frontendCallbacks->show_status_text(strfmt(_("Loading %s..."), file.c_str()));
  ValidationManager::clear();

  GUILock lock(this, _("Model file is being loaded"), strfmt(_("The model %s is loading now and will be available "
                                                               "in a moment.\n\n Please stand by..."),
                                                             file.c_str()));

  bec::GRTManager::get()->block_idle_tasks();

  workbench_DocumentRef doc;

  _model_context = new WBContextModel;

  FOREACH_COMPONENT(_components, iter)
  (*iter)->block_model_notifications();

  _file = new ModelFile(get_auto_save_dir());
  scoped_connect(_file->signal_changed(),
                 std::bind(&WBContext::request_refresh, this, RefreshDocument, "", static_cast<NativeHandle>(0)));

  try {
    _file->open(file);
    bec::GRTManager::get()->set_db_file_path(_file->get_db_file_path());

    doc = _file->retrieve_document();
  }
  /*  catch (grt::grt_runtime_exception &exc)
    {
      show_exception(strfmt(_("Cannot open document '%s'."), file.c_str()), exc);
      new_document();
      _manager->unblock_idle_tasks();

      FOREACH_COMPONENT(_components, iter)
        (*iter)->unblock_model_notifications();

      lock_gui(false);

      return false;
    }*/
  catch (std::exception &exc) {
    show_exception(strfmt(_("Cannot open document '%s'."), file.c_str()), exc);
    // if open fails, just let it fail new_document();
    bec::GRTManager::get()->unblock_idle_tasks();

    FOREACH_COMPONENT(_components, iter)
    (*iter)->unblock_model_notifications();

    return false;
  }

  std::list<std::string> warnings(_file->get_load_warnings());
  if (!warnings.empty()) {
    if (warnings.size() == 1) {
      mforms::Utilities::show_warning(
        _("Corrected Model File"),
        strfmt(_("The model in file '%s' contained a problem which was successfully recovered:\n %s"), file.c_str(),
               warnings.front().c_str()),
        _("Close"));
    } else {
      std::string msg =
        strfmt(_("The model in file '%s' contained problems which were successfully recovered:\n"), file.c_str());
      int i = 0;

      for (std::list<std::string>::const_iterator iter = warnings.begin(); iter != warnings.end(); ++iter) {
        if (i++ >= 2) {
          msg.append("(see log for more details)");
          break;
        }
        msg.append(" -").append(*iter).append("\n");
      }

      mforms::Utilities::show_warning(_("Corrected Model File"), msg, _("Close"));
    }

    grt::GRT::get()->send_output(
      base::strfmt("%i problems found and corrected during load of file '%s':\n", (int)warnings.size(), file.c_str()));
    for (std::list<std::string>::const_iterator iter = warnings.begin(); iter != warnings.end(); ++iter)
      grt::GRT::get()->send_output(base::strfmt(" - %s\n", iter->c_str()));

    _file->copy_file(file, file + ".beforefix");
    grt::GRT::get()->send_output(base::strfmt("Original file backed up to %s\n", (file + ".beforefix").c_str()));
  }

  // 5.0 -> 5.1 was done at 1.2.0, if document is from 5.0, make a backup
  std::vector<std::string> version_parts = base::split(_file->in_disk_document_version(), ".");
  if (version_parts.size() >= 2 && version_parts[0] == "1" && base::atoi<int>(version_parts[1], 0) <= 2) {
    std::string::size_type dot = file.rfind('.');
    std::string bakpath;
    if (file.substr(dot) == ".mwb")
      bakpath = file.substr(0, dot).append(".wb50.mwb");
    else
      bakpath = file + ".wb50.mwb";

    _file->copy_file(file, bakpath);

    // make backup
    grt::GRT::get()->send_info(strfmt("Model file is from 5.0, making backup to %s", bakpath.c_str()));
  } else if (version_parts.size() >= 2 && version_parts[0] == "1" && base::atoi<int>(version_parts[1], 0) <= 3) {
    std::string::size_type dot = file.rfind('.');
    std::string bakpath;
    if (file.substr(dot) == ".mwb")
      bakpath = file.substr(0, dot).append(".wb51.mwb");
    else
      bakpath = file + ".wb51.mwb";

    _file->copy_file(file, bakpath);

    // make backup
    grt::GRT::get()->send_info(strfmt("Model file is from 5.1, making backup to %s", bakpath.c_str()));
  }

  // add the file to the recent files list and sync the options file
  {
    NotificationInfo info;
    info["path"] = file;
    NotificationCenter::get()->send("GNDocumentOpened", 0, info);
  }

  workbench_WorkbenchRef wb(get_root());

  wb->doc(doc);
  doc->owner(wb);

  // mark the document as a global object, so that child objects have changes tracked for undo
  doc->mark_global();

  // check if paperType is properly set (if its null it could be a custom type
  // not available locally)
  if (!doc->pageSettings()->paperType().is_valid()) {
    doc->pageSettings()->paperType(grt::find_named_object_in_list(get_root()->options()->paperTypes(), "A4"));
  }

  FOREACH_COMPONENT(_components, iter)
  (*iter)->unblock_model_notifications();

  reset_document();

  _model_context->model_loaded(_file, doc);

  _filename = file;
  _save_point = grt::GRT::get()->get_undo_manager()->get_latest_undo_action();

  wb->docPath(_filename);

  request_refresh(RefreshDocument, "");

  _frontendCallbacks->show_status_text(_("Document loaded."));

  bec::GRTManager::get()->unblock_idle_tasks();

  if (_frontendCallbacks->perform_command)
    bec::GRTManager::get()->run_once_when_idle(std::bind(_frontendCallbacks->perform_command, "reset_layout"));

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Separate close confirmation and actual close action into two actions to allow better UI updates.
 * This method checks if the document can be closed. That is:
 *  - it is either unchanged
 *  - the user confirmed to save it (and it was saved).
 *
 * @result True if the document can be closed, false otherwise.
 */
bool WBContext::can_close_document() {
  if (!_asked_for_saving && has_unsaved_changes()) {
    int answer = execute_in_main_thread<int>("check save changes",
                                             std::bind(mforms::Utilities::show_message, _("Close Document"),
                                                       _("Do you want to save pending changes to the document?\n\n"
                                                         "If you don't save your changes, they will be lost."),
                                                       _("Save"), _("Cancel"), _("Don't Save")));
    if (answer == mforms::ResultOk) {
      if (!save_as(_filename))
        return false;
    } else if (answer == mforms::ResultCancel)
      return false;
    _asked_for_saving = true;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Closes the current document. This function can be called from any thread.
 *
 * If there are pending changes the user is asked to save them. If can_close_document was called before
 * then the user should not be bothered again about pending changes.
 *
 * XXX: this should finally be changed. can_close is the function which can cancel the closing process.
 *      When we reach here it's too late. This function is called from a destructor and hence cannot be cancelled.
 */
bool WBContext::close_document() {
  if (can_close_document()) {
    _asked_for_saving = false;

    block_user_interaction(true);

    // close the current document
    execute_in_main_thread("close document", std::bind(&WBContext::do_close_document, this, false), true);

    block_user_interaction(false);

    bec::GRTManager::get()->has_unsaved_changes(false);

    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

void WBContext::do_close_document(bool destroying) {
  // This method must only be called from the main thread.
  assert(bec::GRTManager::get()->in_main_thread());

  if (_model_context)
    _model_context->model_closed();

  if (!destroying && _frontendCallbacks->refresh_gui) {
    // close all open editors
    _frontendCallbacks->refresh_gui(RefreshCloseEditor, "", (NativeHandle)0);
  }

  ValidationManager::clear();

  delete _file;
  _file = 0;

  // reset undo manager before destroying views to make sure that old refs to
  // figures will be released and bridges will be deleted 1st
  grt::GRT::get()->get_undo_manager()->reset();
  _save_point = grt::GRT::get()->get_undo_manager()->get_latest_undo_action();

  FOREACH_COMPONENT(_components, iter)
  (*iter)->close_document();

  if (!destroying && _frontendCallbacks->refresh_gui) {
    // Cancel all pending model related events.
    _pending_refreshes.remove_if(CancelRefreshCandidate());

    _frontendCallbacks->refresh_gui(RefreshCloseDocument, "", (NativeHandle)0);
  }
}

/** Tells backend that the frontend has finished doing cleanup for document close.
 */
void WBContext::close_document_finish() {
  workbench_DocumentRef doc(get_document());

  _filename = "";
  get_root()->docPath("");

  if (_model_context)
    _model_context->unrealize();

  get_root()->doc(workbench_DocumentRef());

  delete _model_context;
  _model_context = 0;

  // reset circular references in the document once the app goes idle
  if (doc.is_valid())
    doc->reset_references();

  // reset once again just to be sure
  grt::GRT::get()->get_undo_manager()->reset();
  _save_point = grt::GRT::get()->get_undo_manager()->get_latest_undo_action();
}

//--------------------------------------------------------------------------------
// Saving

grt::ValueRef WBContext::save_grt() {
  // update last change timestamp
  app_DocumentInfoRef info(get_document()->info());

  info->dateChanged(fmttime(0, DATETIME_FMT));

  std::string zip_comment;
  try {
    workbench_DocumentRef doc(get_document());
    GrtObjectRef owner(doc->owner());
    doc->owner(GrtObjectRef()); // temporarily clear non-persistent owner
    _file->store_document(doc);
    doc->owner(owner);

    ListRef<db_Schema> schemata(doc->physicalModels()[0]->catalog()->schemata());
    if (schemata.count())
      zip_comment += "model-schemas: ";
    size_t last = schemata.count() - 1;
    for (size_t sc = schemata.count(), si = 0; si < sc; si++) {
      db_SchemaRef schema(schemata[si]);
      zip_comment += schema->name();
      if (si != last)
        zip_comment += ", ";
    }
    if (!zip_comment.empty())
      zip_comment += '\n';
  } catch (std::exception &exc) {
    show_exception(_("Could not store document data."), exc);
    return grt::ValueRef();
  }

  try {
    if (!_file->save_to(_filename, zip_comment))
      return grt::ValueRef();
  } catch (std::exception &exc) {
    show_exception(strfmt(_("Could not save document to %s"), _filename.c_str()), exc);
    return grt::ValueRef();
  }

  // add the file to the recent files list and sync the options file
  // add_recent_file(_filename);
  {
    NotificationInfo info;
    info["path"] = _filename;
    NotificationCenter::get()->send("GNDocumentOpened", 0, info);
  }

  bec::GRTManager::get()->has_unsaved_changes(false);
  _attachments_changed = false;

  _save_point = grt::GRT::get()->get_undo_manager()->get_latest_undo_action();
  request_refresh(RefreshDocument, "");

  return grt::IntegerRef(1);
}

std::string WBContext::get_filename() const {
  return _filename;
}

std::string WBContext::get_auto_save_dir() {
  return bec::GRTManager::get()->get_user_datadir();
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes outdated settings that were used previously, but are no longer needed.
 */
void WBContext::cleanup_options() {
  logDebug("Cleaning up old options\n");

  grt::DictRef options = get_root()->options()->options();
  options.remove("workbench.physical.ConnectionFigure:CaptionFont");
  options.remove("workbench.model.Layer:TitleFont");
  options.remove("workbench.model.NoteFigure:TitleFont");
  options.remove("workbench.physical:DeleteObjectConfirmation");
  options.remove("Sidebar:RightAligned");
}

//--------------------------------------------------------------------------------------------------

bool WBContext::save_as(const std::string &path) {
  if (_frontendCallbacks->refresh_gui)
    execute_in_main_thread("commit_changes",
                           std::bind(_frontendCallbacks->refresh_gui, RefreshFinishEdits, "", (NativeHandle)0), true);

  if (path.empty()) {
    std::string s = execute_in_main_thread<std::string>(
      "save", std::bind(_frontendCallbacks->show_file_dialog, "save", _("Save Model"), "mwb"));
    if (s.empty())
      return false;

    if (!base::hasSuffix(s, ".mwb"))
      s.append(".mwb");

    _filename = s;
  } else
    _filename = path;

  try {
    _frontendCallbacks->show_status_text(strfmt(_("Saving %s..."), _filename.c_str()));

    if (grt::IntegerRef::cast_from(save_grt()) == 1) {
      _frontendCallbacks->show_status_text(strfmt(_("%s saved."), _filename.c_str()));
      return true;
    } else
      _frontendCallbacks->show_status_text(_("Error saving document."));
  } catch (grt::grt_runtime_error &error) {
    show_exception(_("Error saving document"), error);
    _frontendCallbacks->show_status_text(_("Error saving document."));
  }
  return false;
}

bool WBContext::has_unsaved_changes() {
  if (bec::GRTManager::get()->has_unsaved_changes())
    return true;

  if (grt::GRT::get()->get_undo_manager()->get_latest_closed_undo_action() != _save_point)
    return true;

  if (_file && _file->has_unsaved_changes())
    return true;

  if (_attachments_changed)
    return true;

  return false;
}

bool WBContext::save_changes() {
  save_as(_filename);

  return !has_unsaved_changes();
}

#endif // Document____

#ifndef Plugins____

//--------------------------------------------------------------------------------
// Plugin Handling

void WBContext::update_plugin_arguments_pool(bec::ArgumentPool &args) {
  // value must be asked interactively
  if (args.find("app.PluginInputDefinition:string") == args.end()) {
    // don't add placeholder if it already has a value
    args["app.PluginInputDefinition:string"] = grt::StringRef("");
  }
  args["app.PluginFileInput::save"] = grt::StringRef("");
  args["app.PluginFileInput::open"] = grt::StringRef("");
  args["app.PluginFileInput:filename:save"] = grt::StringRef("");
  args["app.PluginFileInput:filename:open"] = grt::StringRef("");

  if (_model_context && _model_context->get_active_model(true).is_valid())
    return _model_context->update_plugin_arguments_pool(args);

  if (_sqlide_context->get_active_sql_editor())
    return _sqlide_context->update_plugin_arguments_pool(args);
}

void WBContext::report_bug(const std::string &errorInfo) {
  grt::Module *module;

  module = grt::GRT::get()->get_module("Workbench");

  if (!module)
    throw std::runtime_error("Workbench module not found");

  // Setst he parameters for the python plugin
  grt::BaseListRef args(true);
  args.ginsert(grt::StringRef(errorInfo));

  module->call_function("reportBug", args);
}

void WBContext::execute_plugin(const std::string &plugin_name, const ArgumentPool &defaults) {
  app_PluginRef plugin(_plugin_manager->get_plugin(plugin_name));

  if (!plugin.is_valid())
    throw grt::grt_runtime_error("Invalid plugin", "Invalid plugin " + plugin_name);

  ArgumentPool argpool(defaults);

  update_plugin_arguments_pool(argpool);

  app_PluginFileInputRef finput(argpool.needs_file_input(plugin));
  if (finput.is_valid()) {
    std::string fname;

    fname = _frontendCallbacks->show_file_dialog(finput->dialogType(), finput->dialogTitle(), finput->fileExtensions());
    if (fname.empty()) {
      _frontendCallbacks->show_status_text(_("Cancelled."));
      return;
    }
    argpool.add_file_input(finput, fname);
  }

  // build the argument list
  grt::BaseListRef fargs = argpool.build_argument_list(plugin);

  // internal plugins are executed directly in the main thread
  if (plugin->pluginType() == INTERNAL_PLUGIN_TYPE || plugin->pluginType() == STANDALONE_GUI_PLUGIN_TYPE) {
    grt::ValueRef result = execute_plugin_grt(plugin, fargs);
    plugin_finished(result, plugin);
  } else {
    bec::GRTManager::get()->execute_grt_task(strfmt(_("Performing %s..."), plugin->caption().c_str()),
                                  std::bind(&WBContext::execute_plugin_grt, this, plugin, fargs),
                                  std::bind(&WBContext::plugin_finished, this, std::placeholders::_1, plugin));
  }
}

grt::ValueRef WBContext::execute_plugin_grt(const app_PluginRef &plugin, const grt::BaseListRef &args) {
  grt::ValueRef result;

  if (plugin.is_instance(app_DocumentPlugin::static_class_name())) {
    throw std::logic_error("not implemented");
    /* FIXME
    app_DocumentPluginRef doc_plugin(app_DocumentPluginRef::cast_from(plugin));
    workbench_DocumentRef document(get_document());
    bool flag= false;

    for (size_t c= doc_plugin.documentStructNames().count(), i= 0; i < c; i++)
    {
      if (document.is_instance(*doc_plugin.documentStructNames().get(i)))
      {
        flag= true;
        break;
      }
    }

    if (flag)
    {
      _plugin_manager->open_plugin(plugin, document);
    }
    else
      throw grt::grt_runtime_error(_("Invalid document type for plugin."),
            _("The plugin cannot be executed because the current document type is not known to it."));
          */
  } else {
    GTimer *timer = g_timer_new();
    g_timer_start(timer);

    if (_model_context)
      _model_context->begin_plugin_exec();

    bec::GRTManager::get()->soft_lock_globals_tree();

    try {
      bool skip_undo = false;

      if (*plugin->pluginType() != "normal")
        skip_undo = true;

      grt::AutoUndo undo(skip_undo);
      std::string s = *plugin->pluginType();
      _plugin_manager->open_plugin(plugin, args);

      undo.end_or_cancel_if_empty(plugin->caption());

      // TODO: remove this obsolete code once it is confirmed that updating the catalog tree here is not necessary.
      // Btw: why is only the catalog tree updated here? It's not the right place here anyway.
      // request_refresh(RefreshSchemaList, "");
    } catch (const std::exception &exc) {
      grt::GRT::get()->send_error(strfmt("Error executing plugin %s: %s", plugin->name().c_str(), exc.what()));
      result = grt::StringRef(strfmt("%s", exc.what()));
    }

    bec::GRTManager::get()->soft_unlock_globals_tree();

    if (_model_context)
      _model_context->end_plugin_exec();

    g_timer_stop(timer);
    double elapsed = g_timer_elapsed(timer, NULL);
    g_timer_destroy(timer);

    grt::GRT::get()->send_verbose(strfmt("%s finished in %.2fs\n", plugin->name().c_str(), elapsed));
  }

  return result;
}

void WBContext::plugin_finished(const grt::ValueRef &result, const app_PluginRef &plugin) {
  if (*plugin->showProgress())
    _frontendCallbacks->show_status_text(strfmt(_("Execution of \"%s\" finished."), plugin->caption().c_str()));

  if (result.is_valid()) {
    std::string message = *grt::StringRef::cast_from(result);
    show_error(strfmt("Error during \"%s\"", plugin->caption().c_str()), message);
  }

  // request a refresh on the toolbars and menus in case some button state has changed
  bec::UIForm *form = get_active_main_form();
  if (form) {
    mforms::MenuBar *menu = form->get_menubar();
    if (menu)
      menu->validate();
    mforms::ToolBar *tbar = form->get_toolbar();
    if (tbar)
      tbar->validate();
  }
}

//--------------------------------------------------------------------------------
// Object Editors

void WBContext::close_gui_plugin(NativeHandle handle) {
  _plugin_manager->forget_gui_plugin_handle(handle);

  // TODO: really closing the plugin produces flicker when an existing editor is reused. Needs investigation.
  //_plugin_manager->close_and_forget_gui_plugin(handle);
}

void WBContext::register_builtin_plugins(grt::ListRef<app_Plugin> plugins) {
  _plugin_manager->register_plugins(plugins);
}

bool WBContext::activate_live_object(const GrtObjectRef &object) {
  try {
    return get_sqlide_context()->activate_live_object(object);
  } catch (grt::grt_runtime_error &exc) {
    show_exception(_("Activate Live Object"), exc);
  }
  return false;
}

#endif // Plugins____

#ifndef DB_Querying____

std::shared_ptr<SqlEditorForm> WBContext::add_new_query_window(const db_mgmt_ConnectionRef &targetConnection,
                                                               bool restore_session) {
  db_mgmt_ConnectionRef target(targetConnection);

  if (!target.is_valid()) {
    grtui::DbConnectionDialog dialog(get_root()->rdbmsMgmt());
    logDebug("No connection specified, showing connection selection dialog...\n");
    target = dialog.run();
    if (!target.is_valid()) {
      logDebug("Connection selection dialog was cancelled\n");
      _frontendCallbacks->show_status_text(_("Connection cancelled"));
      return SqlEditorForm::Ref();
    }
  }

  _frontendCallbacks->show_status_text(_("Opening SQL Editor..."));

  SqlEditorForm::Ref form;
  try {
    _frontendCallbacks->show_status_text(_("Connecting..."));

    form = get_sqlide_context()->create_connected_editor(target);

    if (form->connection_details().find("dbmsProductVersion") != form->connection_details().end()) {
      // check that we're connecting to a known and supported version of the server
      if (!bec::is_supported_mysql_version(form->connection_details()["dbmsProductVersion"])) {
        logError("Unsupported server version: %s %s\n", form->connection_details()["dbmsProductName"].c_str(),
                 form->connection_details()["dbmsProductVersion"].c_str());

        if (mforms::Utilities::show_message_and_remember(
              base::strfmt("Connection Warning (%s)", targetConnection->name().c_str()),
              base::strfmt(
                "Incompatible/nonstandard server version or connection protocol detected (%s).\n\n"
                "A connection to this database can be established but some MySQL Workbench features may not work "
                "properly since the database is not fully compatible with the supported versions of MySQL.\n\n"
                "MySQL Workbench is developed and tested for MySQL Server versions 5.6, 5.7 and 8.0.\n"
                "For MySQL Server older than 5.6, please use MySQL Workbench version 6.3.",
                bec::sanitize_server_version_number(form->connection_details()["dbmsProductVersion"]).c_str()),
              "Continue Anyway", "Cancel", "", "wb.supported_server_check.suppress_warning",
              "Don't show this message again") != mforms::ResultOk) {
          _frontendCallbacks->show_status_text(_("Unsupported server"));
          return SqlEditorForm::Ref();
        }
      }
    }

    save_connections(); // lastConnected time changed (and potentially the serverVersion).
  } catch (grt::user_cancelled &e) {
    if (target.is_valid())
      logInfo("Connection to %s cancelled by user: %s\n", target->name().c_str(), e.what());
    else
      logInfo("Connection cancelled by user: %s\n", e.what());

    _frontendCallbacks->show_status_text(_("Connection cancelled"));
    return SqlEditorForm::Ref();
  } catch (grt::server_denied &sd) {
    SqlEditorForm::report_connection_failure(sd, target);
    return SqlEditorForm::Ref();
  } catch (std::exception &exc) {
    SqlEditorForm::report_connection_failure(exc.what(), target);
    return SqlEditorForm::Ref();
  }

  try {
    _frontendCallbacks->create_main_form_view(WB_MAIN_VIEW_DB_QUERY, form);
  } catch (std::exception &exc) {
    _frontendCallbacks->show_status_text(_("Could not open SQL Editor."));

    show_error(_("Cannot Open SQL Editor"), strfmt(_("Error in frontend for SQL Editor: %s"), exc.what()));
    return SqlEditorForm::Ref();
  }

  // Restore the last workspace *after* the UI has setup the WQE frontend.
  if (restore_session)
    form->restore_last_workspace();

  _frontendCallbacks->show_status_text(_("SQL Editor Opened."));

  return form;
}

std::shared_ptr<SqlEditorForm> WBContext::add_new_query_window() {
  _frontendCallbacks->show_status_text(_("Opening SQL Editor..."));

  SqlEditorForm::Ref form;
  form = get_sqlide_context()->create_connected_editor(db_mgmt_ConnectionRef());

  try {
    _frontendCallbacks->create_main_form_view(WB_MAIN_VIEW_DB_QUERY, form);
  } catch (std::exception &exc) {
    _frontendCallbacks->show_status_text(_("Could not open SQL Editor."));

    show_error(_("Cannot Open SQL Editor"), strfmt(_("Error in frontend for SQL Editor: %s"), exc.what()));
    return SqlEditorForm::Ref();
  }

  _frontendCallbacks->show_status_text(_("SQL Editor Opened."));

  form->update_title();

  return form;
}

#endif // DB_Querying____

#ifndef Admin____

void WBContext::add_new_admin_window(const db_mgmt_ConnectionRef &target) {
  std::shared_ptr<SqlEditorForm> conn(add_new_query_window(target));
  if (conn) {
    grt::BaseListRef args(true);
    db_query_EditorRef editor(_sqlide_context->get_grt_editor_object(conn.get()));
    args.ginsert(editor);
    args.ginsert(grt::StringRef("admin_server_status"));
    grt::GRT::get()->call_module_function("WbAdmin", "openAdminSection", args);
  }
}

#endif // Admin__

#ifndef AutoStartPlugins____

void WBContext::add_new_plugin_window(const std::string &plugin_id, const std::string &caption) {
  _frontendCallbacks->show_status_text(strfmt(_("Starting %s Module..."), caption.c_str()));

  try {
    grt::BaseListRef args(AnyType);

    app_PluginRef plugin(_plugin_manager->get_plugin(plugin_id));

    if (plugin.is_valid())
      _plugin_manager->open_plugin(plugin, args);
    else {
      _frontendCallbacks->show_status_text(strfmt(_("%s plugin not found"), caption.c_str()));
      return;
    }
  } catch (std::exception &exc) {
    logError("Error opening %s: %s\n", caption.c_str(), exc.what());
    _frontendCallbacks->show_status_text(strfmt(_("Could not open %s: %s"), caption.c_str(), exc.what()));
    return;
  }
}

#endif // AutoStartPlugins____

#ifndef Utilities____
workbench_WorkbenchRef WBContext::get_root() {
  return workbench_WorkbenchRef::cast_from(grt::DictRef::cast_from(grt::GRT::get()->root()).get("wb"));
}

workbench_DocumentRef WBContext::get_document() {
  return workbench_DocumentRef::cast_from(get_root()->doc());
}

grt::DictRef WBContext::get_wb_options() {
  return get_root()->options()->options();
}

// XXX: we have mforms::Utilities::perform_from_main_thread.
void WBContext::execute_in_main_thread(const std::string &name, const std::function<void()> &function, bool wait) {
  bec::GRTManager::get()->get_dispatcher()->call_from_main_thread<void>(function, wait, false);
}

void WBContext::show_exception(const std::string &operation, const std::exception &exc) {
  const grt::grt_runtime_error *rt = dynamic_cast<const grt::grt_runtime_error *>(&exc);

  if (rt) {
    if (bec::GRTManager::get()->in_main_thread())
      show_error(operation, std::string(rt->what()) + "\n" + rt->detail);
    else
      bec::GRTManager::get()->run_once_when_idle(
        std::bind(&WBContext::show_error, this, operation, std::string(rt->what()) + "\n" + rt->detail));
  } else {
    if (bec::GRTManager::get()->in_main_thread())
      show_error(operation, exc.what());
    else
      bec::GRTManager::get()->run_once_when_idle(std::bind(&WBContext::show_error, this, operation, std::string(exc.what())));
  }
}

void WBContext::show_exception(const std::string &operation, const grt::grt_runtime_error &exc) {
  if (bec::GRTManager::get()->in_main_thread())
    show_error(operation, std::string(exc.what()) + "\n" + exc.detail);
  else
    bec::GRTManager::get()->run_once_when_idle(
      std::bind(&WBContext::show_error, this, operation, std::string(exc.what()) + "\n" + exc.detail));
}

#endif // Utilities____

bool WBContext::install_module_file(const std::string &path) {
  std::string module_dir = bec::GRTManager::get()->get_user_module_path();
  std::string target_path;
  std::string lang_extension;

  {
    std::string fname = base::basename(path);
    lang_extension = base::extension(fname);
    if (!lang_extension.empty())
      fname = base::strip_extension(fname);
    target_path = module_dir + "/" + fname;
  }

  if (lang_extension == ".py") {
    // python doesnt like . in middle of filename
    if (g_str_has_suffix(target_path.c_str(), ".grt"))
      target_path[target_path.length() - 4] = '_';
    else if (!g_str_has_suffix(target_path.c_str(), "_grt"))
      target_path.append("_grt");
  } else if (lang_extension == ".lua") {
    show_error("Install Plugin", "Lua is no longer supported in this version.");
  } else if (lang_extension == ".mwbpluginz") {
    lang_extension = ".mwbplugin";
  } else if (lang_extension == ".mwbplugin") {
    // do nothing
  } else {
    show_error("Install Plugin", strfmt("The file %s is not of a known plugin type.", path.c_str()));
    return false;
  }

  // add back the lang_extension
  target_path.append(lang_extension);

  if (module_dir.empty()) {
    show_error("Could Not Install Plugin", "User module install directory is not known");
    return false;
  }

  if (g_file_test(target_path.c_str(), G_FILE_TEST_EXISTS)) {
    logInfo("A plugin file named '%s' is already installed.\n", base::basename(path).c_str());
    if (mforms::Utilities::show_message(
          "Install Plugin", strfmt("A plugin file named '%s' is already installed, would you like to replace it?",
                                   base::basename(path).c_str()),
          "Replace", "Cancel", "") != mforms::ResultOk) {
      logInfo("Plugin replacment denied.\n");
      return false;
    }
    logInfo("Plugin replacment accepted.\n");
  }

  if (lang_extension == ".mwbplugin") {
    if (*path.rbegin() == 'z') {
      try {
        ModelFile::unpack_zip(path, module_dir);
      } catch (const std::exception &exc) {
        show_error("Could Not Install Plugin",
                   strfmt("Plugin %s could not be installed: %s", path.c_str(), exc.what()));
        return false;
      }
    } else {
      if (!copy_folder(path.c_str(), target_path.c_str())) {
        show_error("Could Not Install Plugin",
                   strfmt("Plugin %s could not be copied to modules folder.", path.c_str()));
        return false;
      }
    }
  } else if (!base::copyFile(path.c_str(), target_path.c_str())) {
    int err = errno;
    show_error("Could Not Install Plugin", g_strerror(err));
    grt::GRT::get()->send_output(
      strfmt("ERROR: could not copy module '%s' to '%s': %s\n", path.c_str(), target_path.c_str(), g_strerror(err)));
    return false;
  }

  std::string message = strfmt("Plugin %s installed.", path.c_str());
  logInfo("%s\n", message.c_str());
  _frontendCallbacks->show_status_text(message);
  mforms::Utilities::show_message(
    "Plugin Installed", strfmt("Plugin %s was installed, please restart Workbench to use it.", path.c_str()), "OK");

  grt::GRT::get()->send_output(strfmt("Copied module %s to '%s'\n", path.c_str(), target_path.c_str()));
  grt::GRT::get()->send_output("Please restart Workbench for the change to take effect.\n");

  return true;
}

bool WBContext::uninstall_module(grt::Module *module) {
  std::string path = module->path();
  if (path.empty()) {
    logWarning("Can't uninstall module %s\n", module->name().c_str());
    return false;
  }

  grt::StringListRef disabled_plugins(get_root()->options()->disabledPlugins());
  // remove all plugins from this module from the disabled list
  grt::ListRef<app_Plugin> pl(_plugin_manager->get_plugin_list());
  for (grt::ListRef<app_Plugin>::const_iterator p = pl.begin(); p != pl.end(); ++p) {
    if ((*p)->moduleName() == module->name())
      disabled_plugins.remove_value((*p)->name());
  }

  // unregister the module
  grt::GRT::get()->unregister_module(module);

  _plugin_manager->rescan_plugins();

  // delete the file
  if (module->is_bundle())
    path = module->bundle_path();

  auto ext = base::extension(path);
  if (ext == ".py") {
    std::string pyc = path + "c";
    // For python, we need to also remove the pyc file
    if (base::file_exists(pyc))
      mforms::Utilities::move_to_trash(pyc);
  }
  mforms::Utilities::move_to_trash(path);
  mforms::Utilities::move_to_trash(path + "c");

  return false;
}

void WBContext::run_script_file(const std::string &filename) {
  logDebug("Executing script %s...\n", filename.c_str());

  bec::GRTManager::get()->push_status_text(base::strfmt("Executing script %s...", filename.c_str()));

  grt::AutoUndo undo;

  try {
    bec::GRTManager::get()->get_shell()->run_script_file(filename);
  } catch (std::exception &exc) {
    undo.cancel();
    logError("Script failed: %s\n", exc.what());
    bec::GRTManager::get()->replace_status_text("Script execution failed");
    return;
  }

  undo.end_or_cancel_if_empty(strfmt("Execute Script %s", base::basename(filename).c_str()));

  logDebug("Script finished.\n");
  bec::GRTManager::get()->pop_status_text();
}

std::string WBContext::create_attached_file(const std::string &group, const std::string &tmpl) {
  if (group == "script")
    return _file->add_script_file(tmpl);
  else if (group == "note")
    return _file->add_note_file(tmpl);
  else
    throw std::invalid_argument("invalid attachment group name");
}

std::string WBContext::recreate_attached_file(const std::string &name, const std::string &data) {
  _file->undelete_file(name);
  _file->set_file_contents(name, data);
  return name;
}

void WBContext::save_attached_file_contents(const std::string &name, const char *data, size_t size) {
  _attachments_changed = true;
  _file->set_file_contents(name, data, size);
}

std::string WBContext::get_attached_file_contents(const std::string &name) {
  return _file->get_file_contents(name);
}

std::string WBContext::get_attached_file_tmp_path(const std::string &name) {
  return _file->get_path_for(name);
}

int WBContext::export_attached_file_contents(const std::string &name, const std::string &export_to) {
  try {
    _file->copy_file_to(name, export_to);
  } catch (grt::os_error &exc) {
    logWarning("Error exporting %s: %s\n", name.c_str(), exc.what());
    return 0;
  }
  return 1;
}

void WBContext::delete_attached_file(const std::string &name) {
  _file->delete_file(name);
}

/**
 * Returns the value for a state given by name as string, if it exists or the default value if not.
 */
std::string WBContext::read_state(const std::string &name, const std::string &domain,
                                  const std::string &default_value) {
  workbench_WorkbenchRef wb = get_root();
  grt::DictRef dict = wb->state();

  return dict.get_string(domain + ":" + name, default_value);
}

/**
 * Returns the value for a state given by name as int, if it exists or the default value if not.
 */
int WBContext::read_state(const std::string &name, const std::string &domain, const int &default_value) {
  grt::DictRef dict = get_root()->state();

  return (int)dict.get_int(domain + ":" + name, default_value);
}

/**
 * Returns the value for a state given by name as double, if it exists or the default value if not.
 */
double WBContext::read_state(const std::string &name, const std::string &domain, const double &default_value) {
  grt::DictRef dict = get_root()->state();

  return dict.get_double(domain + ":" + name, default_value);
}

/**
 * Returns the value for a state given by name as bool, if it exists or the default value if not.
 */
bool WBContext::read_state(const std::string &name, const std::string &domain, const bool &default_value) {
  grt::DictRef dict = get_root()->state();

  return dict.get_int(domain + ":" + name, default_value ? 1 : 0) == 1;
}

/**
 * Returns the value for a state given by name as ValueRef, if it exists or the default value if not.
 */
grt::ValueRef WBContext::read_state(const std::string &name, const std::string &domain) {
  grt::DictRef dict = get_root()->state();

  return dict.get(domain + ":" + name);
}

/**
 * Stores the given string state value in the grt tree.
 */
void WBContext::save_state(const std::string &name, const std::string &domain, const std::string &value) {
  grt::DictRef dict = get_root()->state();

  // Set new value for the given state name in that domain.
  dict.gset(domain + ":" + name, value);
}

/**
 * Stores the given int state value in the grt tree.
 */
void WBContext::save_state(const std::string &name, const std::string &domain, const int &value) {
  grt::DictRef dict = get_root()->state();

  // Set new value for the given state name in that domain.
  dict.gset(domain + ":" + name, value);
}

/**
 * Stores the given double state value in the grt tree.
 */
void WBContext::save_state(const std::string &name, const std::string &domain, const double &value) {
  grt::DictRef dict = get_root()->state();

  // Set new value for the given state name in that domain.
  dict.gset(domain + ":" + name, value);
}

/**
 * Stores the given bool state value in the grt tree.
 */
void WBContext::save_state(const std::string &name, const std::string &domain, const bool &value) {
  grt::DictRef dict = get_root()->state();

  // Set new value for the given state name in that domain.
  dict.gset(domain + ":" + name, value ? 1 : 0);
}

/**
 * Stores the given ValueRef state value in the grt tree.
 */
void WBContext::save_state(const std::string &name, const std::string &domain, grt::ValueRef value) {
  grt::DictRef dict = get_root()->state();

  // Set new value for the given state name in that domain.
  dict.set(domain + ":" + name, value);
}

//--------------------------------------------------------------------------------------------------

void WBContext::handle_notification(const std::string &name, void *sender, std::map<std::string, std::string> &info) {
  if (name == "GNDocumentOpened")
    add_recent_file(info["path"]);
}

//--------------------------------------------------------------------------------------------------

static struct RegisterNotifDocs_wb_context {
  RegisterNotifDocs_wb_context() {
    base::NotificationCenter::get()->register_notification("GNDocumentOpened", "modeling",
                                                           "Sent when a Workbench document file is opened.", "",
                                                           "path - path of the file that was opened");

    base::NotificationCenter::get()->register_notification("GNAppClosing", "application",
                                                           "Sent right before Workbench closes.", "", "");
  }
} initdocs_wb_context;
