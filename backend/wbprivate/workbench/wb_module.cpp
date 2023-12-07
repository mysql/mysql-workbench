/*
 * Copyright (c) 2007, 2022, Oracle and/or its affiliates.
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

#include <sstream>

#include "base/config_file.h"
#include "base/log.h"

#include "wb_module.h"
#include "wb_overview.h"
#include "model/wb_model_diagram_form.h"
#include "wb_context_ui.h"
#include "wb_context.h"
#include "model/wb_context_model.h"
#include "model/wb_component.h"

#include "mdc_back_layer.h"

#include "wbcanvas/model_figure_impl.h"

#include "user_defined_type_editor.h"
#include "preferences_form.h"
#include "document_properties_form.h"
#include "grt_shell_window.h"
#include "plugin_manager_window.h"

#include "server_instance_editor.h"
#include "grtui/grtdb_connection_editor.h"
#include "base/string_utilities.h"
#include "base/util_functions.h"
#include "grtdb/db_helpers.h"
#include "SSHSessionWrapper.h"
#include "wb_version.h"

using namespace wb;
using namespace grt;
using namespace base;

DEFAULT_LOG_DOMAIN(DOMAIN_WB_MODULE)

//--------------------------------------------------------------------------------------------------

WorkbenchImpl::WorkbenchImpl(CPPModuleLoader *loader) : super(loader), _wb(0), _is_other_dbms_initialized(false) {
#ifdef _MSC_VER
  _last_wmi_session_id = 1;
  _last_wmi_monitor_id = 1;
#endif
}

//--------------------------------------------------------------------------------------------------

WorkbenchImpl::~WorkbenchImpl() {
}

//--------------------------------------------------------------------------------------------------

void WorkbenchImpl::set_context(WBContext *wb) {
  _wb = wb;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a number of system parameters for use in the log and the debug output.
 */
std::string WorkbenchImpl::getSystemInfo(bool indent) {

#define ARCHITECTURE "64 bit"

#if defined(_MSC_VER)
  #define PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
  #define PLATFORM_NAME "macOS"
#else
  #define PLATFORM_NAME "Linux/Unix"
#endif

  app_InfoRef info(app_InfoRef::cast_from(grt::GRT::get()->get("/wb/info")));

  const char *tab = indent ? "\t" : "";
  std::string result = strfmt("%s%s %s (%s) for " PLATFORM_NAME " version %i.%i.%i %s build %i (%s)\n", tab,
                              info->name().c_str(), APP_EDITION_NAME, APP_LICENSE_TYPE, APP_MAJOR_NUMBER,
                              APP_MINOR_NUMBER, APP_RELEASE_NUMBER, APP_RELEASE_TYPE, APP_BUILD_NUMBER, ARCHITECTURE);
  result += strfmt("%sConfiguration Directory: %s\n", tab, bec::GRTManager::get()->get_user_datadir().c_str());
  result += strfmt("%sData Directory: %s\n", tab, bec::GRTManager::get()->get_basedir().c_str());

  int cver = cairo_version();
  result += strfmt("%sCairo Version: %i.%i.%i\n", tab, (cver / 10000) % 100, (cver / 100) % 100, cver % 100);

  result += strfmt("%sOS: %s\n", tab, get_local_os_name().c_str());
  result += strfmt("%sCPU: %s\n", tab, get_local_hardware_info().c_str());

  result += getFullVideoAdapterInfo(indent);

#ifdef _MSC_VER

  int locale_buffer_size = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SLANGUAGE, NULL, 0);
  if (locale_buffer_size > 0) {
    TCHAR *buffer = new TCHAR[locale_buffer_size];
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SLANGUAGE, buffer, locale_buffer_size);

    std::string converted = base::wstring_to_string(buffer);
    delete[] buffer;

    result += strfmt("%sCurrent user language: %s\n", tab, converted.c_str());
  } else {
    // Very unlikely the system cannot return the current user language, but just in case.
    result += strfmt("%sUnable to determine current user language.\n", tab);
  }

#elif defined(__APPLE__)
#else
  // get distro name/version from lsb_release
  {
    int rc;
    char *stdo;
    if (g_spawn_command_line_sync("lsb_release -d", &stdo, NULL, &rc, NULL) && stdo) {
      char *d = strchr(stdo, ':');
      if (d)
        result += strfmt("%sDistribution: %s\n", tab, g_strchug(d + 1));
      g_free(stdo);
    }
  }

  // try to find out if we're running in fips mode
  {
    bool fips_crypto = false;
    {
      std::ifstream fips_check;
      fips_check.open("/proc/sys/crypto/fips_enabled");
      if (fips_check.good()) {
        char val;
        fips_check >> val;
        fips_crypto = val == '1' ? true : false;
      }
    }
    bool fips_kernel = false;
    {
      std::ifstream fips_check;
      fips_check.open("/proc/cmdline");
      if (fips_check.good()) {
        std::string line;
        fips_check >> line;
        std::size_t found = line.find("fips=");
        if (found != std::string::npos) {
          if (found + 5 <= line.size() && line.substr(found + 5, 1) == "1")
            fips_kernel = 1;
        }
      }
    }
    result += strfmt("%sFips mode enabled: %s\n", tab, (fips_kernel || fips_crypto) ? "yes" : "no");

    // get env variables
    {
      int rc;
      char *stdo;
      if (g_spawn_command_line_sync("/usr/bin/env", &stdo, NULL, &rc, NULL) && stdo) {
        logDebug3("Environment variables:\n %s\n", stdo);
        g_free(stdo);
      }
    }
  }
#endif

  return result;
}

std::map<std::string, std::string> WorkbenchImpl::getSystemInfoMap() {
  std::map<std::string, std::string> result;
  int cver = cairo_version();

  result["edition"] = APP_EDITION_NAME;
  result["license"] = APP_LICENSE_TYPE;
  result["version"] = strfmt("%u.%u.%u", APP_MAJOR_NUMBER, APP_MINOR_NUMBER, APP_RELEASE_NUMBER);
  result["configuration directory"] = bec::GRTManager::get()->get_user_datadir();
  result["data directory"] = bec::GRTManager::get()->get_basedir();
  result["cairo version"] = strfmt("%u.%u.%u", (cver / 10000) % 100, (cver / 100) % 100, cver % 100);
  result["os"] = get_local_os_name();
  result["cpu"] = get_local_hardware_info();

  result["platform"] = PLATFORM_NAME;
#if __linux__
  result["distribution"] = result["os"];
#endif

  return result;
}

int WorkbenchImpl::isOsSupported(const std::string &os) {
  if (os.find("unknown") != std::string::npos) {
    logWarning("OS detection failed, skipping OS support check. OS string: '%s'\n", os.c_str());
    return true;
  }

  static std::vector<std::string> supportedOsList {
    "Ubuntu 22.04", "Ubuntu 22.10", "Ubuntu 23.04", "Ubuntu 23.10", "Debian 10",

    "Red Hat Enterprise Linux release 9",        // Oracle 9.0: Red Hat Enterprise Linux release 9.0 (Plow)
    "Fedora release 37", "Fedora release 38", "CentOS release 7",

    "Windows 10", "Windows Server 2016", "Windows Server 2019", 
    "Windows 11", "Windows Server 2022",

    "macOS 13", "macOS 14"
  };

  for (std::string s : supportedOsList) { 
    if (os.find(s) != std::string::npos) {
      logDebug2("OS '%s' is supported\n", os.c_str());
      return true;
    }
  }

  logWarning("OS not found on supported OS list. OS string: '%s'\n", os.c_str());
  return false;
}
//--------------------------------------------------------------------------------------------------

#define def_plugin(group, aName, type, aCaption, descr) \
  {                                                     \
    app_PluginRef plugin(grt::Initialized);             \
    plugin->name("wb." group "." aName);                \
    plugin->caption(aCaption);                          \
    plugin->description(descr);                         \
    plugin->moduleName("Workbench");                    \
    plugin->moduleFunctionName(aName);                  \
    plugin->pluginType(type);                           \
    plugin->groups().insert("Application/Workbench");   \
    list.insert(plugin);                                \
  }

#define def_object_plugin(group, klass, aName, aCaption, descr) \
  {                                                             \
    app_PluginRef plugin(grt::Initialized);                     \
    plugin->name("wb." group "." aName);                        \
    plugin->caption(aCaption);                                  \
    plugin->description(descr);                                 \
    plugin->moduleName("Workbench");                            \
    plugin->moduleFunctionName(aName);                          \
    plugin->pluginType(NORMAL_PLUGIN_TYPE);                     \
    plugin->groups().insert("Application/Workbench");           \
    app_PluginObjectInputRef input(grt::Initialized);           \
    input->owner(plugin);                                       \
    input->objectStructName(klass::static_class_name());        \
    plugin->inputValues().insert(input);                        \
    list.insert(plugin);                                        \
  }

#define def_view_plugin(group, aName, aCaption, descr)           \
  {                                                              \
    app_PluginRef plugin(grt::Initialized);                      \
    plugin->name("wb." group "." aName);                         \
    plugin->caption(aCaption);                                   \
    plugin->description(descr);                                  \
    plugin->moduleName("Workbench");                             \
    plugin->moduleFunctionName(aName);                           \
    plugin->pluginType(NORMAL_PLUGIN_TYPE);                      \
    plugin->groups().insert("Application/Workbench");            \
    app_PluginObjectInputRef input(grt::Initialized);            \
    input->owner(plugin);                                        \
    input->name("activeDiagram");                                \
    input->objectStructName(model_Diagram::static_class_name()); \
    plugin->inputValues().insert(input);                         \
    list.insert(plugin);                                         \
  }

#define def_model_plugin(group, aName, aCaption, descr)        \
  {                                                            \
    app_PluginRef plugin(grt::Initialized);                    \
    plugin->name("wb." group "." aName);                       \
    plugin->caption(aCaption);                                 \
    plugin->description(descr);                                \
    plugin->moduleName("Workbench");                           \
    plugin->moduleFunctionName(aName);                         \
    plugin->pluginType(NORMAL_PLUGIN_TYPE);                    \
    plugin->groups().insert("Application/Workbench");          \
    app_PluginObjectInputRef input(grt::Initialized);          \
    input->owner(plugin);                                      \
    input->name("activeModel");                                \
    input->objectStructName(model_Model::static_class_name()); \
    plugin->inputValues().insert(input);                       \
    list.insert(plugin);                                       \
  }

#define def_form_model_plugin(group, aName, aCaption, descr)   \
  {                                                            \
    app_PluginRef plugin(grt::Initialized);                    \
    plugin->name("wb." group "." aName);                       \
    plugin->caption(aCaption);                                 \
    plugin->description(descr);                                \
    plugin->moduleName("Workbench");                           \
    plugin->moduleFunctionName(aName);                         \
    plugin->pluginType(STANDALONE_GUI_PLUGIN_TYPE);            \
    plugin->groups().insert("Application/Workbench");          \
    app_PluginObjectInputRef input(grt::Initialized);          \
    input->owner(plugin);                                      \
    input->name("activeModel");                                \
    input->objectStructName(model_Model::static_class_name()); \
    plugin->inputValues().insert(input);                       \
    list.insert(plugin);                                       \
  }

#define def_form_plugin(group, aName, aCaption, descr) \
  {                                                    \
    app_PluginRef plugin(grt::Initialized);            \
    plugin->name("wb." group "." aName);               \
    plugin->caption(aCaption);                         \
    plugin->description(descr);                        \
    plugin->moduleName("Workbench");                   \
    plugin->moduleFunctionName(aName);                 \
    plugin->pluginType(STANDALONE_GUI_PLUGIN_TYPE);    \
    plugin->groups().insert("Application/Workbench");  \
    list.insert(plugin);                               \
  }

#define def_arg_plugin(group, aName, type, aCaption, descr) \
  {                                                         \
    app_PluginRef plugin(grt::Initialized);                 \
    app_PluginInputDefinitionRef pdef(grt::Initialized);    \
    plugin->name("wb." group "." aName);                    \
    plugin->caption(aCaption);                              \
    plugin->description(descr);                             \
    plugin->moduleName("Workbench");                        \
    plugin->moduleFunctionName(aName);                      \
    plugin->pluginType(type);                               \
    pdef->owner(plugin);                                    \
    pdef->name("string");                                   \
    plugin->inputValues().insert(pdef);                     \
    plugin->groups().insert("Application/Workbench");       \
    list.insert(plugin);                                    \
  }

#define def_model_arg_plugin(group, aName, type, aCaption, descr) \
  {                                                               \
    app_PluginRef plugin(grt::Initialized);                       \
    plugin->name("wb." group "." aName);                          \
    plugin->caption(aCaption);                                    \
    plugin->description(descr);                                   \
    plugin->moduleName("Workbench");                              \
    plugin->moduleFunctionName(aName);                            \
    plugin->pluginType(type);                                     \
    app_PluginInputDefinitionRef pdef(grt::Initialized);          \
    pdef->owner(plugin);                                          \
    pdef->name("string");                                         \
    plugin->inputValues().insert(pdef);                           \
    app_PluginObjectInputRef model(grt::Initialized);             \
    model->owner(plugin);                                         \
    model->name("activeModel");                                   \
    model->objectStructName(model_Model::static_class_name());    \
    plugin->inputValues().insert(model);                          \
    plugin->groups().insert("Application/Workbench");             \
    list.insert(plugin);                                          \
  }

#define def_file_plugin(group, aName, ptype, aCaption, descr, aDialogCaption, aType, aExtensions) \
  {                                                                                               \
    app_PluginRef plugin(grt::Initialized);                                                       \
    app_PluginFileInputRef pdef(grt::Initialized);                                                \
    plugin->name("wb." group "." aName);                                                          \
    plugin->caption(aCaption);                                                                    \
    plugin->description(descr);                                                                   \
    plugin->moduleName("Workbench");                                                              \
    plugin->moduleFunctionName(aName);                                                            \
    plugin->pluginType(ptype);                                                                    \
    pdef->owner(plugin);                                                                          \
    pdef->dialogTitle(aDialogCaption);                                                            \
    pdef->dialogType(aType);                                                                      \
    pdef->fileExtensions(aExtensions);                                                            \
    plugin->inputValues().insert(pdef);                                                           \
    plugin->groups().insert("Application/Workbench");                                             \
    list.insert(plugin);                                                                          \
  }

ListRef<app_Plugin> WorkbenchImpl::getPluginInfo() {
  ListRef<app_Plugin> list(true);

  def_plugin("file", "newDocument", INTERNAL_PLUGIN_TYPE, "New Model", "New Document");
  def_plugin("file", "newDocumentFromDB", INTERNAL_PLUGIN_TYPE, "Reverse Engineer Database", "Reverse Engineer");
  def_file_plugin("file", "openModel", INTERNAL_PLUGIN_TYPE, "Open Model", "Open Model from File", "Open Model", "open",
                  "mwb");
  def_plugin("file", "saveModel", INTERNAL_PLUGIN_TYPE, "Save Model", "Save Model to Current File");
  def_arg_plugin("file", "openRecentModel", INTERNAL_PLUGIN_TYPE, "Open Model", "Open Model");
  def_file_plugin("file", "saveModelAs", INTERNAL_PLUGIN_TYPE, "Save As", "Save Model to a New File", "Save Model",
                  "save", "mwb");
  def_plugin("file", "exit", INTERNAL_PLUGIN_TYPE, "Exit", "Exit Workbench");

  def_file_plugin("export", "exportPNG", STANDALONE_GUI_PLUGIN_TYPE, "Export as PNG", "Export Current Diagram as PNG",
                  "Export as PNG", "save", "png");
  def_file_plugin("export", "exportPDF", STANDALONE_GUI_PLUGIN_TYPE, "Export as PDF", "Export Current Diagram as PDF",
                  "Export as PDF", "save", "pdf");
  def_file_plugin("export", "exportSVG", STANDALONE_GUI_PLUGIN_TYPE, "Export as SVG", "Export Current Diagram as SVG",
                  "Export as SVG", "save", "svg");
  def_file_plugin("export", "exportPS", STANDALONE_GUI_PLUGIN_TYPE, "Export as PS",
                  "Export Current Diagram as PostScript", "Export as PostScript", "save", "ps");

  def_plugin("edit", "selectAll", INTERNAL_PLUGIN_TYPE, "Select All", "Select All Objects in Diagram");
  def_plugin("edit", "selectSimilar", INTERNAL_PLUGIN_TYPE, "Select Similar Figures",
             "Select Similar Figures in Diagram");
  def_plugin("edit", "selectConnected", INTERNAL_PLUGIN_TYPE, "Select Connected Figures",
             "Select Figures Connected to the Selected One");

  def_view_plugin("edit", "raiseSelection", "Bring to Front", "Bring Selected Object to Front");
  def_view_plugin("edit", "lowerSelection", "Send to Back", "Send Selected Object to Back");

  def_view_plugin("edit", "toggleGridAlign", "Align to Grid", "Align Objects to Grid");
  def_view_plugin("edit", "toggleGrid", "Toggle Grid", "Toggle Grid");
  def_view_plugin("edit", "togglePageGrid", "Toggle Page Guides", "Toggle Page Guides");
  def_view_plugin("edit", "toggleFKHighlight", "Toggle Relationship Highlight", "Toggle Relationship Highlight");

  def_view_plugin("edit", "editSelectedFigure", "Edit Selected Objects", "Edit Selected Objects");
  def_view_plugin("edit", "editSelectedFigureInNewWindow", "Edit Selected Objects in New Window",
                  "Edit Selected Objects in New Window");

  def_object_plugin("edit", GrtObject, "editObject", "Edit Selected Object", "Edit Selected Object");
  def_object_plugin("edit", GrtObject, "editObjectInNewWindow", "Edit Selected Object in New Window",
                    "Edit Selected Object in New Window");

  def_plugin("edit", "goToNextSelected", INTERNAL_PLUGIN_TYPE, "Go to Next Selected", "Go to Next Selected Object");
  def_plugin("edit", "goToPreviousSelected", INTERNAL_PLUGIN_TYPE, "Go to Previous Selected",
             "Go to Previous Selected Object");

  def_plugin("view", "zoomIn", INTERNAL_PLUGIN_TYPE, "Zoom In", "Zoom In Diagram");
  def_plugin("view", "zoomOut", INTERNAL_PLUGIN_TYPE, "Zoom Out", "Zoom Out Diagram");
  def_plugin("view", "zoomDefault", INTERNAL_PLUGIN_TYPE, "Zoom 100%", "Zoom Back Diagram to Default");

  def_arg_plugin("view", "goToMarker", INTERNAL_PLUGIN_TYPE, "Go to Marker", "Go to Previously Set Diagram Marker");
  def_arg_plugin("view", "setMarker", INTERNAL_PLUGIN_TYPE, "Set a Marker",
                 "Set a Diagram Marker to the Current Location");

  def_model_arg_plugin("view", "setFigureNotation", INTERNAL_PLUGIN_TYPE, "Set Objects Notation",
                       "Set Object Notation");
  def_model_arg_plugin("view", "setRelationshipNotation", INTERNAL_PLUGIN_TYPE, "Set Relationships Notation",
                       "Set Relationship Notation");

  def_model_plugin("model", "newDiagram", "Add New Diagram", "Add a New Diagram to the Model");

  def_file_plugin("tools", "runScriptFile", STANDALONE_GUI_PLUGIN_TYPE, "Run Script File",
                  "Select a Script File to Execute", "Open Script and Execute", "open", "py");
  def_file_plugin("tools", "installModuleFile", STANDALONE_GUI_PLUGIN_TYPE, "Install Plugin",
                  "Select a Module or Plugin File to Install", "Select Module to Install", "open",
                  "py,mwbplugin,mwbpluginz");

  def_form_model_plugin("form", "showUserTypeEditor", "User Types Editor", "Open User Defined Types Editor");
  def_form_model_plugin("form", "showModelOptions", "Show Model Options", "Open Model Options Window");
  def_form_model_plugin("form", "showDocumentProperties", "Document Properties", "Open Document Properties Window");
  def_form_plugin("form", "showOptions", "Preferences", "Open Options Window");
  def_form_plugin("form", "showConnectionManager", "Manage Database Connections", "Open DB Connection Manager");
  def_form_plugin("form", "showInstanceManager", "Manage Server Instance Profiles", "Open Server Profile Manager");
  def_form_plugin("form", "showQueryConnectDialog", "Query Database...", "Connect to and Query a Database Server");
  def_form_plugin("form", "showGRTShell", "Show GRT Shell...", "Show Workbench Script Development Shell");
  def_plugin("form", "newGRTFile", STANDALONE_GUI_PLUGIN_TYPE, "New Script File...",
             "Create a new Workbench script/plugin file");
  def_plugin("form", "openGRTFile", STANDALONE_GUI_PLUGIN_TYPE, "Open Script File...",
             "Open an existing Workbench script/plugin file");
  def_form_plugin("form", "showPluginManager", "Plugin Manager...", "Show Workbench Plugin Manager Window");
  def_arg_plugin("form", "reportBug", STANDALONE_GUI_PLUGIN_TYPE, "Report Bug...", "Show Report Bug Window");

  def_plugin("debug", "debugValidateGRT", NORMAL_PLUGIN_TYPE, "Validate GRT Tree", "Validate Consistency of GRT Tree");

  return list;
}

int WorkbenchImpl::copyToClipboard(const std::string &astr) {
  bec::GRTManager::get()->get_dispatcher()->call_from_main_thread<void>(
    std::bind(mforms::Utilities::set_clipboard_text, astr), true, false);

  return 1;
}

int WorkbenchImpl::hasUnsavedChanges() {
  return _wb->has_unsaved_changes() ? 1 : 0;
}

int WorkbenchImpl::newDocument() {
  _wb->new_document();

  return 0;
}

int WorkbenchImpl::newDocumentFromDB() {
  // if there is a model open, do plain reveng, otherwise create one 1st

  if (!_wb->get_document().is_valid())
    _wb->new_document();

  grt::Module *module = grt::GRT::get()->get_module("MySQLDbModule");

  if (module == NULL)
    throw std::logic_error("Internal error: can't find Workbench DB module.");

  grt::BaseListRef args(true);
  args.ginsert(_wb->get_document()->physicalModels()[0]->catalog());
  grt::IntegerRef resultRef = grt::IntegerRef::cast_from(module->call_function("runDbImportWizard", args));

  return (int)*resultRef;
}

int WorkbenchImpl::openModel(const std::string &filename) {
  _wb->open_document(filename);

  return 0;
}

int WorkbenchImpl::openRecentModel(const std::string &index) {
  _wb->open_recent_document(base::atoi<int>(index, 0));

  return 0;
}

int WorkbenchImpl::saveModel() {
  _wb->save_as(_wb->get_filename());

  return 0;
}

int WorkbenchImpl::saveModelAs(const std::string &filename) {
  _wb->save_as(base::appendExtensionIfNeeded(filename, ".mwb"));

  return 0;
}

int WorkbenchImpl::exportPNG(const std::string &filename) {
  _wb->get_model_context()->export_png(base::appendExtensionIfNeeded(filename, ".png"));

  return 0;
}

int WorkbenchImpl::exportPDF(const std::string &filename) {
  _wb->get_model_context()->export_pdf(base::appendExtensionIfNeeded(filename, ".pdf"));

  return 0;
}

int WorkbenchImpl::exportSVG(const std::string &filename) {
  _wb->get_model_context()->export_svg(base::appendExtensionIfNeeded(filename, ".svg"));

  return 0;
}

int WorkbenchImpl::exportPS(const std::string &filename) {
  _wb->get_model_context()->export_ps(base::appendExtensionIfNeeded(filename, ".ps"));

  return 0;
}

int WorkbenchImpl::activateDiagram(const model_DiagramRef &diagram) {
  _wb->get_model_context()->switch_diagram(diagram);
  return 0;
}

int WorkbenchImpl::exportDiagramToPng(const model_DiagramRef &diagram, const std::string &filename) {
  _wb->get_model_context()->exportPng(diagram, filename);
  return 0;
}

static void quit() {
  if (wb::WBContextUI::get()->request_quit())
    wb::WBContextUI::get()->perform_quit();
}

int WorkbenchImpl::exit() {
  bec::GRTManager::get()->get_dispatcher()->call_from_main_thread<void>(std::bind(quit), false, false);

  return 0;
}

int WorkbenchImpl::selectAll() {
  if (dynamic_cast<ModelDiagramForm *>(_wb->get_active_form())) {
    _wb->get_active_form()->select_all();
  }
  return 0;
}

int WorkbenchImpl::selectSimilar() {
  if (!dynamic_cast<ModelDiagramForm *>(_wb->get_active_form()))
    return 0;

  model_DiagramRef view(dynamic_cast<ModelDiagramForm *>(_wb->get_active_form())->get_model_diagram());
  std::string figure_type;

  if (view->selection().count() != 1)
    return 0;

  model_ObjectRef object(view->selection().get(0));

  figure_type = object.class_name();

  view->unselectAll();

  if (model_FigureRef::can_wrap(object)) {
    for (size_t c = view->figures().count(), i = 0; i < c; i++) {
      model_FigureRef figure(view->figures().get(i));

      if (figure.is_instance(figure_type))
        view->selectObject(figure);
    }
  } else if (model_ConnectionRef::can_wrap(object)) {
    for (size_t c = view->connections().count(), i = 0; i < c; i++) {
      model_ConnectionRef conn(view->connections().get(i));

      if (conn.is_instance(figure_type))
        view->selectObject(conn);
    }
  } else if (model_LayerRef::can_wrap(object)) {
    for (size_t c = view->layers().count(), i = 0; i < c; i++) {
      model_LayerRef layer(view->layers().get(i));

      if (layer.is_instance(figure_type))
        view->selectObject(layer);
    }
  }

  return 0;
}

int WorkbenchImpl::selectConnected() {
  if (!dynamic_cast<ModelDiagramForm *>(_wb->get_active_form()))
    return 0;

  model_DiagramRef view(dynamic_cast<ModelDiagramForm *>(_wb->get_active_form())->get_model_diagram());
  std::string figure_type;
  model_FigureRef figure;

  if (view->selection().count() != 1)
    return 0;

  if (model_FigureRef::can_wrap(view->selection().get(0)))
    figure = model_FigureRef::cast_from(view->selection().get(0));

  if (figure.is_valid()) {
    std::set<std::string> added;

    added.insert(figure.id());

    for (size_t c = view->connections().count(), i = 0; i < c; i++) {
      model_ConnectionRef conn(view->connections().get(i));

      if (conn->startFigure() == figure) {
        if (added.find(conn->endFigure()->id()) == added.end()) {
          added.insert(conn->endFigure()->id());
          view->selectObject(conn->endFigure());
        }
      } else if (conn->endFigure() == figure) {
        if (added.find(conn->startFigure()->id()) == added.end()) {
          added.insert(conn->startFigure()->id());
          view->selectObject(conn->startFigure());
        }
      }
    }
  }

  return 0;
}

static void activate_object(WBComponent *compo, const model_ObjectRef &object, bool newwindow) {
  if (compo->handles_figure(object))
    compo->activate_canvas_object(object, newwindow);
}

int WorkbenchImpl::editSelectedFigure(const model_DiagramRef &view) {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(_wb->get_active_form());
  if (form) {
    ListRef<model_Object> list(form->get_selection());

    for (size_t c = list.count(), i = 0; i < c; i++) {
      _wb->foreach_component(std::bind(activate_object, std::placeholders::_1, list.get(i), false));
    }
  }
  return 0;
}

int WorkbenchImpl::editSelectedFigureInNewWindow(const model_DiagramRef &view) {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(_wb->get_active_form());
  if (form) {
    ListRef<model_Object> list(form->get_selection());

    for (size_t c = list.count(), i = 0; i < c; i++) {
      _wb->foreach_component(std::bind(activate_object, std::placeholders::_1, list.get(i), true));
    }
  }
  return 0;
}

int WorkbenchImpl::editObject(const GrtObjectRef &object) {
  bec::GRTManager::get()->open_object_editor(object, bec::NoFlags);
  return 0;
}

int WorkbenchImpl::editObjectInNewWindow(const GrtObjectRef &object) {
  bec::GRTManager::get()->open_object_editor(object, bec::ForceNewWindowFlag);
  return 0;
}

int WorkbenchImpl::goToNextSelected() {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(_wb->get_active_form());
  if (!form)
    return 0;

  model_DiagramRef view(form->get_model_diagram());

  if (view->selection().count() == 0)
    return 0;

  for (size_t c = view->selection().count(), i = 0; i < c; i++) {
    model_Figure::ImplData *figure = model_FigureRef::cast_from(view->selection().get(i))->get_data();
    if (figure && figure->get_canvas_item()) {
      if (form->get_view()->get_focused_item() == figure->get_canvas_item()) {
        if (i < view->selection().count() - 1) {
          form->focus_and_make_visible(view->selection().get(i + 1), false);
          return 0;
        }
        break;
      }
    }
  }

  // focus 1st
  form->focus_and_make_visible(view->selection().get(0), false);

  return 0;
}

int WorkbenchImpl::goToPreviousSelected() {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(_wb->get_active_form());
  if (!form)
    return 0;

  model_DiagramRef view(form->get_model_diagram());
  if (view->selection().count() == 0)
    return 0;

  for (size_t c = view->selection().count(), i = 0; i < c; i++) {
    model_Figure::ImplData *figure = model_FigureRef::cast_from(view->selection().get(i))->get_data();
    if (figure && figure->get_canvas_item()) {
      if (form->get_view()->get_focused_item() == figure->get_canvas_item()) {
        if (i > 0) {
          form->focus_and_make_visible(view->selection().get(i - 1), false);
          return 0;
        }
        break;
      }
    }
  }
  form->focus_and_make_visible(view->selection().get(view->selection().count() - 1), false);

  return 0;
}

int WorkbenchImpl::newDiagram(const model_ModelRef &model) {
  model->addNewDiagram(false);

  return 0;
}

// canvas manipulation

int WorkbenchImpl::raiseSelection(const model_DiagramRef &view) {
  for (size_t c = view->selection().count(), i = 0; i < c; i++) {
    if (view->selection().get(i).is_instance(model_Figure::static_class_name())) {
      model_FigureRef figure(model_FigureRef::cast_from(view->selection()[i]));

      figure->layer()->raiseFigure(figure);
    }
  }
  return 0;
}

int WorkbenchImpl::lowerSelection(const model_DiagramRef &view) {
  for (size_t c = view->selection().count(), i = 0; i < c; i++) {
    if (view->selection().get(i).is_instance(model_Figure::static_class_name())) {
      model_FigureRef figure(model_FigureRef::cast_from(view->selection()[i]));

      figure->layer()->lowerFigure(figure);
    }
  }
  return 0;
}

int WorkbenchImpl::toggleGridAlign(const model_DiagramRef &view) {
  ModelDiagramForm *form = _wb->get_model_context()->get_diagram_form_for_diagram_id(view->id());
  if (form) {
    form->get_view()->set_grid_snapping(!form->get_view()->get_grid_snapping());

    bec::GRTManager::get()->set_app_option("AlignToGrid",
                                           grt::IntegerRef(form->get_view()->get_grid_snapping() ? 1 : 0));
  }
  return 0;
}

int WorkbenchImpl::toggleGrid(const model_DiagramRef &view) {
  ModelDiagramForm *form = _wb->get_model_context()->get_diagram_form_for_diagram_id(view->id());
  if (form) {
    form->get_view()->get_background_layer()->set_grid_visible(
      !form->get_view()->get_background_layer()->get_grid_visible());

    view->options().gset("ShowGrid", form->get_view()->get_background_layer()->get_grid_visible() ? 1 : 0);
  }
  return 0;
}

int WorkbenchImpl::togglePageGrid(const model_DiagramRef &view) {
  ModelDiagramForm *form = _wb->get_model_context()->get_diagram_form_for_diagram_id(view->id());
  if (form) {
    form->get_view()->get_background_layer()->set_paper_visible(
      !form->get_view()->get_background_layer()->get_paper_visible());

    view->options().gset("ShowPageGrid", form->get_view()->get_background_layer()->get_paper_visible() ? 1 : 0);
  }
  return 0;
}

int WorkbenchImpl::toggleFKHighlight(const model_DiagramRef &view) {
  ModelDiagramForm *form = _wb->get_model_context()->get_diagram_form_for_diagram_id(view->id());
  if (form) {
    form->set_highlight_fks(!form->get_highlight_fks());

    view->options().gset("ShowFKHighlight", form->get_highlight_fks() ? 1 : 0);
  }
  return 0;
}

int WorkbenchImpl::goToMarker(const std::string &marker) {
  model_ModelRef model(_wb->get_model_context()->get_active_model(true));

  if (model.is_valid()) {
    model_MarkerRef mk;

    for (size_t c = model->markers().count(), i = 0; i < c; i++)
      if (*model->markers().get(i)->name() == marker) {
        mk = model->markers().get(i);
        break;
      }

    if (mk.is_valid()) {
      model_DiagramRef diagram = model_DiagramRef::cast_from(mk->diagram());
      diagram->zoom(mk->zoom());
      diagram->x(mk->x());
      diagram->y(mk->y());

      bec::GRTManager::get()->get_dispatcher()->call_from_main_thread<void>(
        std::bind(&WBContextModel::switch_diagram, _wb->get_model_context(), diagram), false, false);
    }
  }

  return 0;
}

int WorkbenchImpl::setMarker(const std::string &marker) {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(wb::WBContextUI::get()->get_active_main_form());

  if (form) {
    model_MarkerRef mk(grt::Initialized);

    model_ModelRef model(form->get_model_diagram()->owner());

    for (size_t c = model->markers().count(), i = 0; i < c; i++)
      if (*model->markers().get(i)->name() == marker) {
        model->markers().remove(i);
        break;
      }

    mk->owner(model);
    mk->name(marker);
    mk->diagram(form->get_model_diagram());
    mk->zoom(form->get_view()->get_zoom());
    mk->x(form->get_view()->get_viewport().left());
    mk->y(form->get_view()->get_viewport().top());

    model->markers().insert(mk);
  }
  return 0;
}

template <class C>
grt::Ref<C> get_parent_for_object(const GrtObjectRef &object) {
  GrtObjectRef obj = object;
  while (obj.is_valid() && !obj.is_instance(C::static_class_name()))
    obj = obj->owner();
  return grt::Ref<C>::cast_from(obj);
}

int WorkbenchImpl::highlightFigure(const model_ObjectRef &figure) {
  if (figure.is_valid()) {
    model_DiagramRef view;

    if (figure.is_instance<model_Diagram>())
      view = model_DiagramRef::cast_from(figure);
    else
      view = get_parent_for_object<model_Diagram>(figure);

    if (view.is_valid()) {
      ModelDiagramForm *form = _wb->get_model_context()->get_diagram_form_for_diagram_id(view.id());

      if (form) {
        _wb->_frontendCallbacks->switched_view(form->get_view());
        form->focus_and_make_visible(model_FigureRef::cast_from(figure), true);
      }
    }
  }
  return 0;
}

int WorkbenchImpl::setFigureNotation(const std::string &name, workbench_physical_ModelRef model) {
  //  model_ModelRef model(wb::WBContextUI::get()->get_active_model(true));

  if (model.is_valid() && model.is_instance<workbench_physical_Model>())
    workbench_physical_ModelRef::cast_from(model)->figureNotation(name);
  _wb->get_wb_options().set("DefaultFigureNotation", grt::StringRef(name));

  return 0;
}

int WorkbenchImpl::setRelationshipNotation(const std::string &name, workbench_physical_ModelRef model) {
  //  model_ModelRef model(wb::WBContextUI::get()->get_active_model(true));

  if (model.is_valid() && model.is_instance<workbench_physical_Model>())
    workbench_physical_ModelRef::cast_from(model)->connectionNotation(name);
  _wb->get_wb_options().set("DefaultConnectionNotation", grt::StringRef(name));
  return 0;
}

int WorkbenchImpl::zoomIn() {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(_wb->get_active_main_form());
  if (!form)
    return 0;

  form->zoom_in();
  return 0;
}

int WorkbenchImpl::zoomOut() {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(_wb->get_active_main_form());
  if (!form)
    return 0;

  form->zoom_out();

  return 0;
}

int WorkbenchImpl::zoomDefault() {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(_wb->get_active_main_form());
  if (!form)
    return 0;

  model_DiagramRef view(form->get_model_diagram());

  view->zoom(1.0);

  return 0;
}

int WorkbenchImpl::startTrackingUndo() {
  grt::GRT::get()->begin_undoable_action();
  return 0;
}

int WorkbenchImpl::finishTrackingUndo(const std::string &description) {
  grt::GRT::get()->end_undoable_action(description);
  return 0;
}

int WorkbenchImpl::cancelTrackingUndo() {
  grt::GRT::get()->cancel_undoable_action();
  return 0;
}

int WorkbenchImpl::addUndoListAdd(const BaseListRef &list) {
  grt::GRT::get()->get_undo_manager()->add_undo(new grt::UndoListInsertAction(list));
  return 0;
}

int WorkbenchImpl::addUndoListRemove(const BaseListRef &list, int index) {
  grt::GRT::get()->get_undo_manager()->add_undo(new grt::UndoListRemoveAction(list, index));
  return 0;
}

int WorkbenchImpl::addUndoObjectChange(const ObjectRef &object, const std::string &member) {
  grt::GRT::get()->get_undo_manager()->add_undo(new grt::UndoObjectChangeAction(object, member));
  return 0;
}

int WorkbenchImpl::addUndoDictSet(const DictRef &dict, const std::string &key) {
  grt::GRT::get()->get_undo_manager()->add_undo(new grt::UndoDictSetAction(dict, key));
  return 0;
}

int WorkbenchImpl::beginUndoGroup() {
  grt::GRT::get()->get_undo_manager()->begin_undo_group();
  return 0;
}

int WorkbenchImpl::endUndoGroup() {
  grt::GRT::get()->get_undo_manager()->end_undo_group();
  return 0;
}

int WorkbenchImpl::setUndoDescription(const std::string &text) {
  grt::GRT::get()->get_undo_manager()->set_action_description(text);
  return 0;
}

std::string WorkbenchImpl::createAttachedFile(const std::string &group, const std::string &tmpl) {
  return _wb->create_attached_file(group, tmpl);
}

int WorkbenchImpl::setAttachedFileContents(const std::string &filename, const std::string &text) {
  _wb->save_attached_file_contents(filename, text.data(), text.size());
  return 0;
}

std::string WorkbenchImpl::getAttachedFileContents(const std::string &filename) {
  return _wb->get_attached_file_contents(filename);
}

std::string WorkbenchImpl::getAttachedFileTmpPath(const std::string &filename) {
  return _wb->get_attached_file_tmp_path(filename);
}

int WorkbenchImpl::exportAttachedFileContents(const std::string &filename, const std::string &export_to) {
  return _wb->export_attached_file_contents(filename, export_to);
}

workbench_DocumentRef WorkbenchImpl::openModelFile(const std::string &path) {
  return _wb->openModelFile(path);
}

int WorkbenchImpl::closeModelFile() {
  return _wb->closeModelFile();
}

std::string WorkbenchImpl::getTempDir() {
  return _wb->getTempDir();
}

std::string WorkbenchImpl::getDbFilePath() {
  return _wb->getDbFilePath();
}

int WorkbenchImpl::runScriptFile(const std::string &filename) {
  _wb->run_script_file(filename);

  return 0;
}

int WorkbenchImpl::installModuleFile(const std::string &filename) {
  _wb->install_module_file(filename);
  return 0;
}

static int traverse_value(const ObjectRef &owner, const std::string &member, const ValueRef &value);

static bool traverse_member(const MetaClass::Member *member, const ObjectRef &owner, const ObjectRef &object) {
  std::string k = member->name;
  ValueRef v = object->get_member(k);

  if (!v.is_valid()) {
    if ((member->type.base.type == ListType) || (member->type.base.type == DictType))
      grt::GRT::get()->send_output(strfmt("%s[%s] (type: %s, name: '%s', id: %s), has NULL list or dict member: '%s'\n",
                                          owner.class_name().c_str(), k.c_str(), object.class_name().c_str(),
                                          object.get_string_member("name").c_str(), object.id().c_str(), k.c_str()));
  }

  if (k == "owner") {
    if (ObjectRef::cast_from(v) != owner) {
      if (!v.is_valid())
        grt::GRT::get()->send_output(strfmt(
          "%s[%s] (type: %s, name: '%s', id: %s), has no owner set\n", owner.class_name().c_str(), member->name.c_str(),
          object.class_name().c_str(), object->get_string_member("name").c_str(), object.id().c_str()));
      else
        grt::GRT::get()->send_output(
          strfmt("%s[%s] (type: %s, name: '%s', id: %s), has bad owner (or missing attr:dontfollow)\n",
                 owner.class_name().c_str(), member->name.c_str(), object.class_name().c_str(),
                 object->get_string_member("name").c_str(), object.id().c_str()));
    }
  }

  if (member->owned_object)
    traverse_value(object, k, v);

  return true;
}

static int traverse_value(const ObjectRef &owner, const std::string &member, const ValueRef &value) {
  switch (value.type()) {
    case DictType: {
      DictRef dict(DictRef::cast_from(value));

      for (DictRef::const_iterator iter = dict.begin(); iter != dict.end(); ++iter) {
        std::string k = iter->first;
        ValueRef v = iter->second;

        traverse_value(owner, k, v);
      }
    } break;
    case ListType: {
      BaseListRef list(BaseListRef::cast_from(value));

      for (size_t c = list.count(), i = 0; i < c; i++) {
        ValueRef v;

        v = list.get(i);

        traverse_value(owner, strfmt("%i", (int)i), v);
      }
    } break;
    case ObjectType: {
      ObjectRef object(ObjectRef::cast_from(value));
      MetaClass *gstruct = object->get_metaclass();

      gstruct->foreach_member(std::bind(traverse_member, std::placeholders::_1, owner, object));
    } break;

    default:
      break;
  }
  return 0;
}

int WorkbenchImpl::debugValidateGRT() {
  ValueRef root(grt::GRT::get()->root());
  ObjectRef owner;

  logDebug3("Validating GRT Tree...\n");

  // make sure that all nodes have their owner set to their parent object
  // make sure that all refs that are not owned are marked dontfollow
  traverse_value(owner, "root", root);

  logDebug3("GRT Tree Validation Finished.\n");

  return 0;
}

//--------------------------------------------------------------------------------------------------

int WorkbenchImpl::refreshHomeConnections() {
  wb::WBContextUI::get()->refresh_home_connections();
  return 0;
}

//--------------------------------------------------------------------------------------------------

int WorkbenchImpl::confirm(const std::string &title, const std::string &caption) {
  return bec::GRTManager::get()->get_dispatcher()->call_from_main_thread<int>(
    std::bind(mforms::Utilities::show_message, title, caption, _("OK"), _("Cancel"), ""), true, false);
}

std::string WorkbenchImpl::requestFileOpen(const std::string &caption, const std::string &extensions) {
  return bec::GRTManager::get()->get_dispatcher()->call_from_main_thread<std::string>(
    std::bind(_wb->_frontendCallbacks->show_file_dialog, "open", caption, extensions), true, false);
}

std::string WorkbenchImpl::requestFileSave(const std::string &caption, const std::string &extensions) {
  return bec::GRTManager::get()->get_dispatcher()->call_from_main_thread<std::string>(
    std::bind(_wb->_frontendCallbacks->show_file_dialog, "save", caption, extensions), true, false);
}

//--------------------------------------------------------------------------------------------------

int WorkbenchImpl::showUserTypeEditor(const workbench_physical_ModelRef &model) {
  if (_wb->get_model_context())
    _wb->get_model_context()->show_user_type_editor(model);

  return 0;
}

int WorkbenchImpl::showGRTShell() {
  wb::WBContextUI::get()->get_shell_window()->show();

  return 0;
}

int WorkbenchImpl::newGRTFile() {
  wb::WBContextUI::get()->get_shell_window()->show();
  wb::WBContextUI::get()->get_shell_window()->add_new_script();

  return 0;
}

int WorkbenchImpl::openGRTFile() {
  wb::WBContextUI::get()->get_shell_window()->show();
  wb::WBContextUI::get()->get_shell_window()->open_script_file();

  return 0;
}

int WorkbenchImpl::showDocumentProperties() {
  DocumentPropertiesForm props;

  props.show();

  return 0;
}

int WorkbenchImpl::showModelOptions(const workbench_physical_ModelRef &model) {
  PreferencesForm prefs(model);
  prefs.show();

  return 0;
}

int WorkbenchImpl::showOptions() {
  PreferencesForm prefs;
  prefs.show();

  return 0;
}

int WorkbenchImpl::reportBug(const std::string error_info) {
  unsigned short os_id = 1;
  std::map<std::string, std::string> sys_info = getSystemInfoMap();

  std::string os_details = sys_info["os"];

  if (sys_info["platform"] == "Linux/Unix") {
    os_id = 5;
    os_details = sys_info["distribution"];
  } else if (sys_info["platform"] == "macOS")
    os_id = 6;
  else if (sys_info["platform"] == "Windows")
    os_id = 7;

  std::ostringstream stream;
  stream << "http://bugs.mysql.com/report.php"
         << "?"
         << "in[status]="
         << "Open"
         << "&"
         << "in[php_version]=" << sys_info["version"] << "&"
         << "in[os]=" << os_id << "&"
         << "in[os_details]=" << os_details << "&"
         << "in[tags]="
         << "WBBugReporter"
         << "&"
         << "in[really]="
         << "0"
         << "&"
         << "in[ldesc]="
         << "----"
         << "[For better reports, please attach the log file after submitting. You can find it in "
         << base::Logger::log_filename() << "]";

  mforms::Utilities::open_url(stream.str());
  return 0;
}

int WorkbenchImpl::showConnectionManager() {
  grtui::DbConnectionEditor editor(_wb->get_root()->rdbmsMgmt());
  _wb->_frontendCallbacks->show_status_text("Connection Manager Opened.");
  editor.run();
  _wb->_frontendCallbacks->show_status_text("");
  wb::WBContextUI::get()->refresh_home_connections();
  _wb->save_connections();

  return 0;
}

int WorkbenchImpl::showInstanceManager() {
  ServerInstanceEditor editor(_wb->get_root()->rdbmsMgmt());
  _wb->_frontendCallbacks->show_status_text("Server Profile Manager Opened.");
  db_mgmt_ServerInstanceRef instance(editor.run());
  _wb->_frontendCallbacks->show_status_text("");
  // save instance list now
  _wb->save_instances();
  return 0;
}

int WorkbenchImpl::showInstanceManagerFor(const db_mgmt_ConnectionRef &conn) {
  ServerInstanceEditor editor(_wb->get_root()->rdbmsMgmt());
  _wb->_frontendCallbacks->show_status_text("Server Profile Manager Opened.");
  db_mgmt_ServerInstanceRef instance(editor.run(conn, true));
  _wb->_frontendCallbacks->show_status_text("");
  // save instance list now
  _wb->save_instances();
  return 0;
}

int WorkbenchImpl::saveConnections() {
  _wb->save_connections();
  return 0;
}

int WorkbenchImpl::saveInstances() {
  _wb->save_instances();
  return 0;
}

int WorkbenchImpl::showQueryConnectDialog() {
  _wb->add_new_query_window(db_mgmt_ConnectionRef());

  return 0;
}

int WorkbenchImpl::showPluginManager() {
  PluginManagerWindow pm(_wb);

  pm.run();
  return 0;
}

//--------------------------------------------------------------------------------------------------

#ifdef _MSC_VER

/**
 * Opens a new wmi session, which is essentially a connection to a computer that can later be used to
 * query that box, manipulate service states or monitor values.
 *
 * @server The name or IP address of the machine to connect to. Leave empty for localhost.
 * @user The user name for the connection. Leave empty for localhost.
 * @password The password for the given user. Ignored when connecting to localhost.
 * @return A unique id for the new session. Use that for any further call and don't forgot to close the session
 *         once you don't need it anymore or you will get a memory leak. Returns 0 on error.
 */
int WorkbenchImpl::wmiOpenSession(const std::string server, const std::string &user, const std::string &password) {
  logDebug2("Opening wmi session\n");

  wmi::WmiServices *services = new wmi::WmiServices(server, user, password);
  _wmi_sessions[++_last_wmi_session_id] = services;
#ifdef DEBUG
  _thread_for_wmi_session[_last_wmi_session_id] = g_thread_self();
#endif

  return _last_wmi_session_id;
}

//--------------------------------------------------------------------------------------------------

#ifdef DEBUG
#define check_wmi_thread(session)                                                              \
  {                                                                                            \
    if (_thread_for_wmi_session.find(session) != _thread_for_wmi_session.end()) {              \
      if (g_thread_self() != _thread_for_wmi_session[session]) {                               \
        logWarning("%s called from invalid thread for WMI session %i", __FUNCTION__, session); \
        throw std::logic_error("WMI access from invalid thread");                              \
      }                                                                                        \
    }                                                                                          \
  }
#else
#define check_wmi_thread(s)
#endif

//--------------------------------------------------------------------------------------------------

/**
 * Closes the wmi session opened with wmiOpenSession. Necessary to avoid memory leaks. If there are
 * pending monitors for that session they will be closed implicitly.
 *
 * @param session The session that should be closed.
 * @return 1 if the session was successfully closed, -1 if the session is invalid.
 */
int WorkbenchImpl::wmiCloseSession(int session) {
  logDebug2("Closing wmi session\n");
  if (_wmi_sessions.find(session) == _wmi_sessions.end())
    return -1;

  logDebug2("Closing all wmi monitors\n");
  wmi::WmiServices *services = _wmi_sessions[session];
  std::map<int, wmi::WmiMonitor *>::iterator next, i = _wmi_monitors.begin();
  while (i != _wmi_monitors.end()) {
    next = i;
    ++next;
    if (i->second->owner() == services)
      wmiStopMonitoring(i->first);
    i = next;
  }
  delete services;

  _wmi_sessions.erase(session);

  return 1;
}

//--------------------------------------------------------------------------------------------------

/**
 * Executes a query against the given server session. The query must be in WQL format.
 *
 * @param session The session to work against.
 * @param query The query to execute. For further info see
 * http://msdn.microsoft.com/en-us/library/aa394606%28VS.85%29.aspx.
 * @return A list of GRT dicts containing the objects returned by the query, that is, name/value pairs
 *         of object properties.
 */
grt::DictListRef WorkbenchImpl::wmiQuery(int session, const std::string &query) {
  logDebug2("Running a wmi query\n");
  if (_wmi_sessions.find(session) == _wmi_sessions.end()) {
    logWarning("Attempt to run a wmi query against non-existing session\n");
    return grt::DictListRef();
  }

  check_wmi_thread(session);

  wmi::WmiServices *services = _wmi_sessions[session];

  return services->query(query);
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to query or control a service on a target machine.
 *
 * @param session The session describing the target connection.
 * @param service The name of the service on that machine to work with.
 * @param action The action to be executed. Allowed values are: status, start, stop.
 * @return A result describing the outcome of the action. Can be:
 *   - completed
 *   - error
 *   - already-running
 *   - already-stopped
 *   - already-starting
 *   - already-stopping
 *   - stopping
 *   - starting
 */
std::string WorkbenchImpl::wmiServiceControl(int session, const std::string &service, const std::string &action) {
  logDebug2("Running wmi service control command\n");
  if (_wmi_sessions.find(session) == _wmi_sessions.end())
    return "error - Invalid wmi session";

  check_wmi_thread(session);

  wmi::WmiServices *services = _wmi_sessions[session];

  return services->serviceControl(service, action);
}

//--------------------------------------------------------------------------------------------------

/**
 * Queries the target machine for certain system statistics.
 *
 * @param session The session describing the target connection.
 * @param what The value to query. Can be: TotalVisibleMemorySize, FreePhysicalMemory.
 * @return The asked for value. If what is invalid then the result is simply 0. The returned value
 *         is formatted as string to cater for different types of return values.
 */
std::string WorkbenchImpl::wmiSystemStat(int session, const std::string &what) {
  logDebug2("Running wmi system statistics query\n");
  if (_wmi_sessions.find(session) == _wmi_sessions.end())
    return "error - Invalid wmi session";

  check_wmi_thread(session);

  wmi::WmiServices *services = _wmi_sessions[session];

  return services->systemStat(what);
}

//--------------------------------------------------------------------------------------------------

/**
 * Starts a monitoring session for a given connection. It will set up a monitoring context to allow
 * quick queries for a single value. The monitor must be freed with wmiStopMonitoring to avoid errors and
 * memory leaks.
 *
 * @param session The session describing the target connection.
 * @param what The property/value to monitor. Supported values are: LoadPercentage.
 * @return A unique id describing the new monitor.
 */
int WorkbenchImpl::wmiStartMonitoring(int session, const std::string &what) {
  logDebug2("Starting new wmi monitor\n");
  if (_wmi_sessions.find(session) == _wmi_sessions.end())
    return -1;

  check_wmi_thread(session);

  wmi::WmiServices *services = _wmi_sessions[session];
  wmi::WmiMonitor *monitor = services->startMonitoring(what);
  _wmi_monitors[++_last_wmi_monitor_id] = monitor;

  logDebug2("Wmi monitor with id %d created\n", _last_wmi_monitor_id);
  return _last_wmi_monitor_id;
}

//--------------------------------------------------------------------------------------------------

/**
 * Reads the current value of the monitored property.
 *
 * @param monitor The monitor set up with wmiStartMonitoring.
 * @return The current value formatted as string.
 */
std::string WorkbenchImpl::wmiReadValue(int monitor_id) {
  logDebug3("Reading wmi value for monitor: %d\n", monitor_id);
  if (_wmi_monitors.find(monitor_id) == _wmi_monitors.end()) {
    logWarning("Attempt to read monitor value for non-existing monitor\n");
    return "error - Invalid wmi session";
  }

  wmi::WmiMonitor *monitor = _wmi_monitors[monitor_id];

  return monitor->readValue();
}

//--------------------------------------------------------------------------------------------------

/**
 * Stops the giving monitor. After this call the monitor is no longer valid.
 *
 * @param monitor The monitor to stop.
 * @return -1 if monitor_id is invalid, else 1
 */
int WorkbenchImpl::wmiStopMonitoring(int monitor_id) {
  logDebug2("Stopping wmi monitor %d\n", monitor_id);
  if (_wmi_monitors.find(monitor_id) == _wmi_monitors.end()) {
    logWarning("Attempt to stop non-existing wmi monitor\n");
    return -1;
  }

  wmi::WmiMonitor *monitor = _wmi_monitors[monitor_id];

  delete monitor;
  _wmi_monitors.erase(monitor_id);

  return 1;
}

#endif

//--------------------------------------------------------------------------------------------------

static const char *DEFAULT_RDBMS_ID = "com.mysql.rdbms.mysql";

/**
 * Creates a new connection ref and adds it to the stored connections collection.
 * The new connection is also returned.
 */
db_mgmt_ConnectionRef WorkbenchImpl::create_connection(const std::string &host, const std::string &user,
                                                       const std::string socket_or_pipe_name, int can_use_networking,
                                                       int can_use_socket_or_pipe, int port, const std::string &name) {
  logDebug("Creating new connection (%s) to host %s:%d for user %s (socket/pipe: %s)\n", name.c_str(), host.c_str(),
           port, user.c_str(), socket_or_pipe_name.c_str());

  db_mgmt_RdbmsRef rdbms = find_object_in_list(_wb->get_root()->rdbmsMgmt()->rdbms(), DEFAULT_RDBMS_ID);
  grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());

  db_mgmt_ConnectionRef connection = db_mgmt_ConnectionRef(grt::Initialized);
  db_mgmt_DriverRef driver;
  if (can_use_networking)
    driver = rdbms->defaultDriver();
  else {
    // If networking is enabled but sockets/named pipes aren't enabled then the server
    // is actually not accessible. We play nice though and use the default driver in that case.
    if (can_use_socket_or_pipe) {
      driver = find_object_in_list(rdbms->drivers(), "com.mysql.rdbms.mysql.driver.native_socket");
      if (!driver.is_valid())
        driver = rdbms->defaultDriver();
      else
        connection->parameterValues().gset("socket", socket_or_pipe_name);
    } else
      driver = rdbms->defaultDriver();
  }

  connection->driver(driver);
  connection->name(name);

  connection->parameterValues().gset("hostName", host);
  connection->parameterValues().gset("userName", user);
  connection->parameterValues().gset("port", port);

  connection->hostIdentifier(bec::get_host_identifier_for_connection(connection));
  connections.insert(connection);

  logDebug("Done creating new connection\n");

  return connection;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a list of Dicts with data for each locally installed MySQL server.
 */
grt::DictListRef WorkbenchImpl::getLocalServerList() {
  logDebug("Reading locally installed MySQL servers\n");

  grt::DictListRef entries;
#ifdef _MSC_VER
  try {
    int session = wmiOpenSession("", "", "");
    entries =
      wmiQuery(session, "select * from Win32_Service where (Name like \"%mysql%\" or DisplayName like \"%mysql%\")");
    wmiCloseSession(session);
  } catch (std::exception const &e) {
    // If for some reason (e.g. insufficient rights) the retrieval fails then we return an empty list.
    logError("Unable to locate installed MySQL Servers : %s.\n", e.what());
  } catch (...) {
    logError("Unable to locate installed MySQL Servers.\n");
  }

#else
  entries = grt::DictListRef(grt::Initialized);

  char *stdo = NULL;
  char *ster = NULL;
  int rc = 0;
  GError *err = NULL;
#ifdef __APPLE__
  std::string cmd = "/bin/sh -c \"ps -ec | grep \\\"mysqld\\b\\\" | awk '{ print $1 }' | xargs ps -ww -o args= -p \"";
#else
  std::string cmd =
    "/bin/sh -c \"ps -ec | grep \\\"mysqld\\b\\\" | awk '{ print $1 }' | xargs -r ps -ww -o args= -p \"";
#endif
  if (g_spawn_command_line_sync(cmd.c_str(), &stdo, &ster, &rc, &err) && stdo) {
    std::string processes(stdo);

    std::vector<std::string> servers = base::split(processes, "\n");
    std::vector<std::string>::iterator index, end = servers.end();

    for (index = servers.begin(); index != end; index++) {
      DictRef server(true);
      std::string command = *index;
      if (command.length()) {
        server.set("PathName", StringRef(command));
        entries.insert(server);
      }
    }
  }

  if (stdo)
    g_free(stdo);

  if (err) {
    logWarning("Error looking for installed servers, error %d : %s\n", err->code, err->message);
    g_error_free(err);
  }

  if (ster != NULL && strlen(ster) > 0)
    logError("stderr from process list %s\n", ster);

  g_free(ster);

#endif

  logDebug("Found %li installed MySQL servers\n", entries.is_valid() ? (long)entries.count() : -1);

  return entries;
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates a list of new connections to all local servers found.
 */
int WorkbenchImpl::createConnectionsFromLocalServers() {
  grt::DictListRef servers = getLocalServerList();
  if (!servers.is_valid())
    return -1;

  db_mgmt_RdbmsRef rdbms = find_object_in_list(_wb->get_root()->rdbmsMgmt()->rdbms(), DEFAULT_RDBMS_ID);
  grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
  size_t count = servers->count();
  for (size_t i = 0; i < count; i++) {
    grt::DictRef server(servers[i]);

    std::string service_display_name = server.get_string("DisplayName", "invalid");
    std::string path = server.get_string("PathName", "invalid");
    std::string config_file = base::extract_option_from_command_line("--defaults-file", path);

    if (g_file_test(config_file.c_str(), G_FILE_TEST_EXISTS)) {
      ConfigurationFile key_file(config_file, base::AutoCreateSections | base::AutoCreateKeys);

      // Early out if the default section is not there.
      if (!key_file.has_section("mysqld"))
        continue;

      bool can_use_networking = 1;
      bool can_use_socket_or_pipe = 0;
      std::string socket_or_pipe_name;

      int port = key_file.get_int("port", "mysqld");
      if (port == INT_MIN)
        port = 3306;
      if (key_file.has_key("skip-networking", "mysqld"))
        can_use_networking = 0;
      if (key_file.has_key("enable-named-pipe", "mysqld"))
        can_use_socket_or_pipe = 1;

      if (can_use_socket_or_pipe) {
        socket_or_pipe_name = key_file.get_value("socket", "mysqld");
        if (socket_or_pipe_name.size() == 0)
          socket_or_pipe_name = "MySQL";
      }

      create_connection("localhost", "root", socket_or_pipe_name, can_use_networking, can_use_socket_or_pipe, port,
                        _("Local ") + service_display_name);
    }
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates a list of server instance entries for all local servers found.
 */
int WorkbenchImpl::createInstancesFromLocalServers() {
  int found_instances = 0;
  try {
    grt::DictListRef servers = getLocalServerList();
    if (!servers.is_valid())
      return -1;

    db_mgmt_RdbmsRef rdbms = find_object_in_list(_wb->get_root()->rdbmsMgmt()->rdbms(), DEFAULT_RDBMS_ID);
    grt::ListRef<db_mgmt_ServerInstance> instances(_wb->get_root()->rdbmsMgmt()->storedInstances());
    size_t count = servers->count();
    for (size_t i = 0; i < count; i++) {
      grt::DictRef entry(servers[i]);

      std::string display_name = entry.get_string("DisplayName", "invalid");
      std::string service_name = entry.get_string("Name", "invalid");
      std::string path = entry.get_string("PathName", "invalid");

      // Takes the parameters from the command line
      std::string config_file = base::extract_option_from_command_line("--defaults-file", path);
      std::string socket_or_pipe_name = base::extract_option_from_command_line("--socket", path);
      std::string str_port = base::extract_option_from_command_line("--port", path);

      ssize_t port = INT_MIN;
      if (str_port.length())
        port = base::atoi<int>(str_port, 0);

      bool can_use_networking = true;
      if (path.find("--skip-networking") != std::string::npos)
        can_use_networking = false;

      bool can_use_socket_or_pipe = false;
      if (path.find("--enable-named-pipe") != std::string::npos)
        can_use_socket_or_pipe = true;

      // Creates the server instance
      db_mgmt_ServerInstanceRef instance(grt::Initialized);

      // If the configuration file is part of the command call
      // Will take the parameters from there if were not available on the command call
      if (g_file_test(config_file.c_str(), G_FILE_TEST_EXISTS)) {
        ConfigurationFile key_file(config_file, base::AutoCreateSections | base::AutoCreateKeys);

        // Early out if the default section is not there.
        if (!key_file.has_section("mysqld"))
          continue;

        // Gets the port if not already retrieved from the command line
        if (port == INT_MIN)
          port = key_file.get_int("port", "mysqld");

        if (can_use_networking && key_file.has_key("skip-networking", "mysqld"))
          can_use_networking = false;

        if (!can_use_socket_or_pipe && key_file.has_key("enable-named-pipe", "mysqld"))
          can_use_socket_or_pipe = true;

        if (socket_or_pipe_name.size() == 0)
          socket_or_pipe_name = key_file.get_value("socket", "mysqld");

        // When a valid config path is identified it will be locked on the
        // connection editor
        instance->serverInfo().gset("sys.config.path.lock", 1);
      }

      // If the port was not found, uses the default port
      if (port == INT_MIN)
        port = 3306;

      // If the socket was not found uses a default value
      if (socket_or_pipe_name.size() == 0)
        socket_or_pipe_name = "MySQL";

      instance->owner(_wb->get_root()->rdbmsMgmt());

#ifdef _MSC_VER
      instance->serverInfo().gset("sys.system", "Windows");
      instance->serverInfo().gset("windowsAdmin", 1);
      instance->loginInfo().gset("wmi.userName", ""); // Only used for remote connections.
      instance->loginInfo().gset("wmi.hostName", ""); // Only used for remote connections.
      instance->serverInfo().gset("sys.mysqld.service_name", service_name);
#elif defined(__APPLE__)
      instance->serverInfo().gset("sys.system", "macOS");
#else
      instance->serverInfo().gset("sys.system", "Linux");
#endif
      instance->serverInfo().gset("setupPending", 1);

      // If the display name is invalid will create one using the port
      if (display_name == "invalid")
        display_name = std::to_string(port);

      instance->name("Local " + display_name);

      instance->serverInfo().gset("sys.preset", "Custom");

      instance->serverInfo().gset("sys.config.path", config_file);
      instance->serverInfo().gset("sys.config.section", "mysqld");

      // Finally look up a connection or create one we can use for this instance.
      db_mgmt_ConnectionRef connection;
      grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
      for (grt::ListRef<db_mgmt_Connection>::const_iterator end = connections.end(), iterator = connections.begin();
           iterator != end; ++iterator) {
        grt::DictRef parameters((*iterator)->parameterValues());

        // Must be a localhost connection.
        std::string parameter = parameters.get_string("hostName");
        if (!parameter.empty() && parameter != "localhost" && parameter != "127.0.0.1")
          continue;

        // For now we only consider connections with a root user. We might later add support
        // for any user.
        parameter = parameters.get_string("userName");
        if (parameter != "root")
          continue;

        if (can_use_networking) {
          ssize_t other_port = parameters.get_int("port");
          if (other_port != port)
            continue;

          // Only native tcp/ip network connections for now.
          if ((*iterator)->driver()->id() != "com.mysql.rdbms.mysql.driver.native")
            continue;
        } else if (can_use_socket_or_pipe) {
          parameter = parameters.get_string("socket");
          if (parameter.size() == 0)
            parameter = "MySQL"; // Default pipe/socket name.

          if (parameter != socket_or_pipe_name)
            continue;

          // Only native socket/pipe connections for now.
          if ((*iterator)->driver()->id() != "com.mysql.rdbms.mysql.driver.native_socket")
            continue;
        }

        // All parameters are ok. This is a connection we can use.
        connection = *iterator;
        break;
      }

      // If we did not find a connection for this instance then create a new one.
      if (!connection.is_valid())
        connection = create_connection("localhost", "root", socket_or_pipe_name, can_use_networking,
                                       can_use_socket_or_pipe, (int)port, _("Local instance ") + display_name);

      instance->connection(connection);
      instances.insert(instance);
      ++found_instances;
    }
  } catch (std::exception &exc) {
    // If for some reason (e.g. insufficient rights) the wmi session throws an exception
    // we don't want to see it. We simply do not create the list in that case.
    logWarning("Error auto-detecting server instance: %s\n", exc.what());
  }

  return found_instances;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a short string describing the currently active video adapter, especially the used chipset.
 */
std::string WorkbenchImpl::getVideoAdapter() {
  logDebug("Attempting to determine the current video adaptor and its properties\n");
  std::string result = _("Unknown");
  try {
#ifdef _MSC_VER
    int session = wmiOpenSession("", "", "");
    grt::DictListRef entries = wmiQuery(session, "select Availability, VideoProcessor from Win32_VideoController");
    wmiCloseSession(session);

    size_t count = entries->count();
    logDebug("Found %d adapters\n", count);
    for (size_t i = 0; i < count; i++) {
      grt::DictRef entry(entries[i]);

      if (entry.get_string("Availability", "0") != "3")
        continue;

      // Return the first active adapter we find.
      result = entry.get_string("VideoProcessor", "Unknown video processor");
      break;
    }
#else
// TODO: other OSes go here.
#endif
  } catch (...) {
    // If for some reason (e.g. insufficient rights) the retrieval fails then we return an empty list.
    logError("Attempt failed to determine the current video adapter\n");
  }

  logDebug("Done scan for the current video adapter\n");

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns all available info about the currently active video adapter in human readable format.
 */
std::string WorkbenchImpl::getFullVideoAdapterInfo(bool indent) {
  std::stringstream result;
  std::string tab = indent ? "\t" : "";
  try {
#ifdef _MSC_VER
    int session = wmiOpenSession("", "", "");
    grt::DictListRef entries = wmiQuery(session, "select * from Win32_VideoController");
    wmiCloseSession(session);

    // Try finding the first video controller that is marked as being online. This doesn't work
    // reliably all the time, though. So if we don't find such a controller we explicitly ask for
    // the first one in the system, which should give most of the time the proper result.
    size_t count = entries->count();
    grt::DictRef entry;

    for (size_t i = 0; i < count; i++) {
      entry = entries[i];
      if (entry.get_string("Availability", "0") == "3")
        break;
    }

    if (!entry.is_valid() && count > 0)
      entry = entries[0];

    if (entry.is_valid()) {
      result << tab << "Active video adapter " << entry.get_string("Caption", "unknown") << '\n';
      std::string value = entry.get_string("AdapterRAM", "");
      if (!value.empty()) {
        int64_t size = base::atoi<int64_t>(value, (int64_t)0) / 1024 / 1024;
        result << tab << "Installed video RAM: " << size << " MB\n";
      } else
        result << tab << "Installed video RAM: unknown\n";

      result << tab << "Current video mode: " << entry.get_string("VideoModeDescription", "unknown") << '\n';

      value = entry.get_string("CurrentBitsPerPixel", "");
      if (value.size() > 0)
        result << tab << "Used bit depth: " << value << '\n';
      else
        result << tab << "Used bit depth: unknown\n";

      result << tab << "Driver version: " << entry.get_string("DriverVersion", "unknown") << '\n';
      result << tab << "Installed display drivers: " << entry.get_string("InstalledDisplayDrivers", "unknown") << '\n';
    } else {
      result << "No video adapter info available\n";
    }
#else
    // TODO: other OSes go here.
    result << "No video adapter info available\n";
#endif
  } catch (...) {
    // If for some reason (e.g. insufficient rights) the retrieval fails then we return an empty list.
    result << "No video adapter info available\n";
  }
  return result.str();
}

int WorkbenchImpl::initializeOtherRDBMS() {
  if (_is_other_dbms_initialized)
    return 0;
  _is_other_dbms_initialized = true;
  grt::GRT::get()->send_output("Initializing rdbms modules\n");

  // Init MySQL first.
  grt::Module *mysql_module = grt::GRT::get()->get_module("DbMySQL"); // already loaded on startup
  grt::BaseListRef args(true);

  // init other RDBMS
  bool failed = false;
  const std::vector<grt::Module *> &modules(grt::GRT::get()->get_modules());
  for (std::vector<grt::Module *>::const_iterator m = modules.begin(); m != modules.end(); ++m) {
    if ((*m)->has_function("initializeDBMSInfo") && *m != mysql_module) {
      grt::GRT::get()->send_output(strfmt("Initializing %s rdbms info\n", (*m)->name().c_str()));
      try {
        (*m)->call_function("initializeDBMSInfo", args);
      } catch (std::exception &) {
        failed = true;
      }
    }
  }
  if (failed)
    logWarning("Support for one or more RDBMS sources have failed.\n");

  _wb->load_other_connections();

  return 1;
}

db_mgmt_SSHConnectionRef WorkbenchImpl::createSSHSession(const grt::ObjectRef &val) {

  if (!db_mgmt_ConnectionRef::can_wrap(val) && !db_mgmt_ServerInstanceRef::can_wrap(val)) {
    logError("Invalid argument, Connection or ServerInstace is required.\n");
    return db_mgmt_SSHConnectionRef();
  }

  try {
    db_mgmt_SSHConnectionRef object(grt::Initialized);
    object->owner(wb::WBContextUI::get()->get_wb()->get_root());
    object->name("SSHSession");
    if (db_mgmt_ConnectionRef::can_wrap(val))
      object->set_data(new ssh::SSHSessionWrapper(db_mgmt_ConnectionRef::cast_from(val)));
    else
      object->set_data(new ssh::SSHSessionWrapper(db_mgmt_ServerInstanceRef::cast_from(val)));

    return object;
  } catch (std::runtime_error &) {
    logError("Unable to create db_mgmt_SSHConnectionRef object.\n");
  }
  return db_mgmt_SSHConnectionRef();
}

/**
* Removes a connection from the stored connections list along with all associated data
* (including its server instance entry).
*/
int WorkbenchImpl::deleteConnection(const db_mgmt_ConnectionRef &connection) {
  grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
  grt::ListRef<db_mgmt_ServerInstance> instances = _wb->get_root()->rdbmsMgmt()->storedInstances();

  // Remove all associated server instances.
  for (ssize_t i = instances.count() - 1; i >= 0; --i) {
    db_mgmt_ServerInstanceRef instance(instances[i]);
    if (instance->connection() == connection)
      instances->remove(i);
  }

  // Remove password associated with this connection (if stored in keychain/vault). Check first
  // this service/username combination isn't used anymore by other connections.
  bool credentials_still_used = false;
  grt::DictRef parameter_values = connection->parameterValues();
  std::string host = connection->hostIdentifier();
  std::string user = parameter_values.get_string("userName");
  for (grt::ListRef<db_mgmt_Connection>::const_iterator i = connections.begin(); i != connections.end(); ++i) {
    if (*i != connection) {
      grt::DictRef current_parameters = (*i)->parameterValues();
      if (host == *(*i)->hostIdentifier() && user == current_parameters.get_string("userName")) {
        credentials_still_used = true;
        break;
      }
    }
  }
  if (!credentials_still_used) {
    try {
      mforms::Utilities::forget_password(host, user);
    } catch (std::exception &exc) {
      logWarning("Could not clear password: %s\n", exc.what());
    }
  }

  connections->remove(connection);

  return 0;
}

int WorkbenchImpl::deleteConnectionGroup(const std::string &group) {
  size_t group_length = group.length();

  std::vector<db_mgmt_ConnectionRef> candidates;
  grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());

  ssize_t index = connections.count() - 1;
  while (index >= 0) {
    std::string name = connections[index]->name();

    if (name.compare(0, group_length, group) == 0)
      candidates.push_back(connections[index]);

    index--;
  }

  for (std::vector<db_mgmt_ConnectionRef>::const_iterator iterator = candidates.begin(); iterator != candidates.end();
       ++iterator)
    deleteConnection(*iterator);

  return 0;
}
//--------------------------------------------------------------------------------------------------
