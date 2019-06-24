/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "sqlide/wb_sql_editor_form.h"

#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "workbench/wb_overview.h"
#include "ConvUtils.h"
#include "GrtTemplates.h"
#include "Overview.h"
#include "Wb.h"

#include "SQLEditorFormWrapper.h"

#include "mforms/menubar.h"
#include "mforms/appview.h"

using namespace System::IO;
using namespace System::Reflection;
using namespace System::Text;
using namespace System::Threading;
using namespace System::Windows::Forms;

using namespace Aga::Controls::Tree;

using namespace base;
using namespace wb;

using namespace MySQL::Base;
using namespace MySQL::Forms;
using namespace MySQL::Workbench;

//--------------------------------------------------------------------------------------------------

WbOptions::WbOptions(String ^ baseDir, String ^ userDir, bool full_init) {
  String ^ exeName = Path::GetFileName(Assembly::GetEntryAssembly()->Location);
  inner = new wb::WBOptions(NativeToCppStringRaw(exeName));
  inner->basedir = NativeToCppStringRaw(baseDir);
  inner->plugin_search_path = inner->basedir;
  inner->module_search_path = inner->basedir + "/modules";
  inner->struct_search_path = "";
  inner->library_search_path = inner->basedir;

  if (!inner->user_data_dir.empty()) {
    if (!base::is_directory(inner->user_data_dir)) {
      try {
        if (!base::copyDirectoryRecursive(NativeToCppStringRaw(userDir), inner->user_data_dir)) {
          Logger::LogError("WBContext managed", String::Format("Unable to prepare new config directory: {0} \n",
                                                               CppStringToNative(inner->user_data_dir)));
        }
      } catch (std::exception &exc) {
        Logger::LogError(
          "WBContext managed",
          String::Format(
            "There was a problem preparing new config directory. Falling back to default one. The error was: {0}\n",
            CppStringToNative(exc.what())));
        inner->user_data_dir = NativeToCppStringRaw(userDir);
      }
    }
  } else
    inner->user_data_dir = NativeToCppStringRaw(userDir);

  inner->full_init = full_init;
}

//--------------------------------------------------------------------------------------------------

bool WbOptions::parse_args(array<String ^> ^ args, String ^ app_path) {
  // Convert to UTF8 and keep the returned arrays as long as we are parsing that.
  // Add the application's path at the tip as the parse routine wants it so.
  array<array<unsigned char> ^> ^ managed_utf8 = gcnew array<array<unsigned char> ^>(args->Length + 1);
  managed_utf8[0] = Encoding::UTF8->GetBytes(app_path);
  for (int i = 0; i < args->Length; i++)
    managed_utf8[i + 1] = Encoding::UTF8->GetBytes(args[i]);

  // Collect the managed string arrays into a c-like char* for parsing.
  char **arguments = new char *[managed_utf8->Length];
  bool ret = true;
  try {
    for (int i = 0; i < managed_utf8->Length; i++) {
      pin_ptr<unsigned char> chars = &managed_utf8[i][0];
      arguments[i] = (char *)chars;
    }

    int rc = 0;
    std::vector<std::string> arguments(arguments + 1, arguments + managed_utf8->Length);
    if (!inner->programOptions->parse(arguments, rc)) {
      Logger::LogInfo("WBContext managed", String::Format("Exiting with rc {0} after parsing arguments\n", rc));
      ret = false;
    }
    inner->analyzeCommandLineArguments();
  } catch (std::exception &exc) {
    Logger::LogInfo("WBContext managed",
                    String::Format("Exiting with error message: {0}\n", CppStringToNative(exc.what())));
    ret = false;
  } finally {
    delete arguments;
  }

  return ret;
}

//--------------------------------------------------------------------------------------------------

String ^ WbOptions::OpenAtStartup::get() {
  return CppStringToNative(inner->open_at_startup);
}

//--------------------------------------------------------------------------------------------------

String ^ WbOptions::OpenAtStartupType::get() {
  return CppStringToNative(inner->open_at_startup_type);
}

//--------------------------------------------------------------------------------------------------

void WbOptions::analyzeCommandLineArguments() {
  inner->analyzeCommandLineArguments();
}

//----------------- WbContext ----------------------------------------------------------------------

WbContext::WbContext(bool verbose) {
  physical_overview = nullptr;
  _wbContextUi = new WbContextUiHolder;
  Logger::LogDebug("WBContext managed", 1, "Creating WbContext\n");
}

//--------------------------------------------------------------------------------------------------

WbContext::~WbContext() {
  // Don't delete the inner object. It's a singleton.
  delete physical_overview;
  delete _wbContextUi;
  Logger::LogDebug("WBContext managed", 1, "Destroying WbContext\n");
}

//--------------------------------------------------------------------------------------------------

bool WbContext::init(WbFrontendCallbacks ^ callbacks, WbOptions ^ options,
                     VoidStrUIFormDelegate ^ create_main_form_view) {
  set_create_main_form_view(callbacks, create_main_form_view);
  return WBContextUI::get()->init(callbacks->get_callbacks(), options->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

GrtManager ^ WbContext::get_grt_manager() {
  if (manager == nullptr)
    manager = gcnew GrtManager();

  return manager;
}

//--------------------------------------------------------------------------------------------------

void WbContext::add_frontend_commands(List<String ^> ^ commands) {
  WBContextUI::get()->get_command_ui()->add_frontend_commands(NativeToCppStringList2(commands));
}

//--------------------------------------------------------------------------------------------------

void WbContext::remove_frontend_commands(List<String ^> ^ commands) {
  WBContextUI::get()->get_command_ui()->remove_frontend_commands(NativeToCppStringList2(commands));
}

//--------------------------------------------------------------------------------------------------

void WbContext::activate_command(String ^ name) {
  std::string command = NativeToCppString(name);
  WBContextUI::get()->get_command_ui()->activate_command(command);
}

//--------------------------------------------------------------------------------------------------

Overview ^ WbContext::get_physical_overview() {
  if (physical_overview == nullptr)
    physical_overview = gcnew Overview(WBContextUI::get()->get_physical_overview());
  return physical_overview;
}

//--------------------------------------------------------------------------------------------------

TreeViewAdv ^ WbContext::get_history_tree() {
  // Note: this and the other get_* function below leak memory (the created trees).
  //       This will be solved once the entire sidebars are managed by the backend.
  return dynamic_cast<TreeViewAdv ^>(
    ObjectMapper::GetManagedComponent(WBContextUI::get()->get_wb()->get_model_context()->create_history_tree()));
}

//--------------------------------------------------------------------------------------------------

TreeViewAdv ^ WbContext::get_usertypes_tree() {
  return dynamic_cast<TreeViewAdv ^>(
    ObjectMapper::GetManagedComponent(WBContextUI::get()->get_wb()->get_model_context()->create_user_type_list()));
}

//--------------------------------------------------------------------------------------------------

MySQL::Grt::GrtValueInspector ^ WbContext::get_inspector_for_selection(UIForm ^ form, [Out] List<String ^> ^ % items) {
  std::vector<std::string> vitems;
  bec::ValueInspectorBE *insp =
    WBContextUI::get()->create_inspector_for_selection(form->get_unmanaged_object(), vitems);
  if (!insp)
    insp = WBContextUI::get()->create_inspector_for_selection(vitems);
  if (insp) {
    items = CppStringListToNative(vitems);
    return gcnew::MySQL::Grt::GrtValueInspector(insp, true);
  }
  return nullptr;
}

//--------------------------------------------------------------------------------------------------

bool WbContext::are_lists_equal(GrtValue ^ v1, GrtValue ^ v2) {
  if ((nullptr == v1) || (nullptr == v2))
    return (v1 == v2);
  ::grt::ValueRef v1_(v1->get_unmanaged_object());
  ::grt::ValueRef v2_(v2->get_unmanaged_object());
  ::grt::ObjectListRef l1_ = ::grt::ObjectListRef::cast_from(v1_);
  ::grt::ObjectListRef l2_ = ::grt::ObjectListRef::cast_from(v2_);

  return ::grt::compare_list_contents(l1_, l2_);
}

//--------------------------------------------------------------------------------------------------

String ^ WbContext::get_description_for_selection(UIForm ^ form, [Out] GrtValue ^ % activeObjList,
                                                  [Out] List<String ^> ^ % items) {
  std::vector<std::string> vitems;
  ::grt::ListRef<GrtObject> activeObjList_;
  std::string val =
    WBContextUI::get()->get_description_for_selection(form->get_unmanaged_object(), activeObjList_, vitems);
  activeObjList = gcnew GrtValue(activeObjList_);
  items = CppStringListToNative(vitems);
  return CppStringToNative(val);
}

//--------------------------------------------------------------------------------------------------

String ^ WbContext::get_description_for_selection([Out] GrtValue ^ % activeObjList, [Out] List<String ^> ^ % items) {
  std::vector<std::string> vitems;
  ::grt::ListRef<GrtObject> activeObjList_;
  std::string val = WBContextUI::get()->get_description_for_selection(activeObjList_, vitems);
  activeObjList = gcnew GrtValue(activeObjList_);
  items = CppStringListToNative(vitems);
  return CppStringToNative(val);
}

//--------------------------------------------------------------------------------------------------

void WbContext::set_description_for_selection(GrtValue ^ activeObjList, String ^ val) {
  std::string val_ = NativeToCppString(val);
  ::grt::ListRef<GrtObject> activeObjList_(::grt::ListRef<GrtObject>::cast_from(activeObjList->get_unmanaged_object()));
  WBContextUI::get()->set_description_for_selection(activeObjList_, val_);
}

//--------------------------------------------------------------------------------------------------

MySQL::Workbench::ModelDiagramFormWrapper ^ WbContext::get_diagram_form_for_diagram(String ^ id) {
  return gcnew ModelDiagramFormWrapper(
    WBContextUI::get()->get_wb()->get_model_context()->get_diagram_form_for_diagram_id(NativeToCppString(id)));
}

//--------------------------------------------------------------------------------------------------

void WbContext::set_active_form(UIForm ^ uiform) {
  if (uiform == nullptr)
    WBContextUI::get()->set_active_form(0);
  else
    WBContextUI::get()->set_active_form(uiform->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

void WbContext::set_active_form_from_appview(MySQL::Forms::AppViewDockContent ^ form) {
  if (form == nullptr) {
    Logger::LogDebug("WBContext managed", 2, "Activating appview 0\n");
    WBContextUI::get()->set_active_form(0);
  } else {
    Logger::LogDebug("WBContext managed", 2, String::Format("Activating appview {0}\n", form->GetAppViewIdentifier()));
    WBContextUI::get()->set_active_form(form->GetBackend());
  }
}

//--------------------------------------------------------------------------------------------------

String ^ WbContext::get_active_context() {
  return CppStringToNative(WBContextUI::get()->get_active_context());
}

//--------------------------------------------------------------------------------------------------

void WbContext::close_gui_plugin(IntPtr handle) {
  WBContextUI::get()->get_wb()->close_gui_plugin((NativeHandle)handle.ToPointer());
}

//--------------------------------------------------------------------------------------------------

void WbContext::execute_plugin(String ^ name) {
  WBContextUI::get()->get_wb()->execute_plugin(NativeToCppStringRaw(name));
}

//--------------------------------------------------------------------------------------------------

void WbContext::report_bug(String ^ errorInfo) {
  WBContextUI::get()->get_wb()->report_bug(NativeToCppStringRaw(errorInfo));
}

//--------------------------------------------------------------------------------------------------

String ^ WbContext::read_state(String ^ name, String ^ domain, String ^ default_value) {
  std::string value = WBContextUI::get()->get_wb()->read_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain),
                                                               NativeToCppString(default_value));

  return CppStringToNative(value);
}

//--------------------------------------------------------------------------------------------------

int WbContext::read_state(String ^ name, String ^ domain, const int default_value) {
  int value =
    WBContextUI::get()->get_wb()->read_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), default_value);

  return value;
}

//--------------------------------------------------------------------------------------------------

double WbContext::read_state(String ^ name, String ^ domain, const double default_value) {
  double value =
    WBContextUI::get()->get_wb()->read_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), default_value);

  return value;
}

//--------------------------------------------------------------------------------------------------

bool WbContext::read_state(String ^ name, String ^ domain, const bool default_value) {
  bool value =
    WBContextUI::get()->get_wb()->read_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), default_value);

  return value;
}

//--------------------------------------------------------------------------------------------------

void WbContext::save_state(String ^ name, String ^ domain, String ^ value) {
  WBContextUI::get()->get_wb()->save_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain),
                                           NativeToCppString(value));
}

//--------------------------------------------------------------------------------------------------

void WbContext::save_state(String ^ name, String ^ domain, const int value) {
  WBContextUI::get()->get_wb()->save_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), value);
}

//--------------------------------------------------------------------------------------------------

void WbContext::save_state(String ^ name, String ^ domain, const double value) {
  WBContextUI::get()->get_wb()->save_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), value);
}

//--------------------------------------------------------------------------------------------------

void WbContext::save_state(String ^ name, String ^ domain, const bool value) {
  WBContextUI::get()->get_wb()->save_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), value);
}

//--------------------------------------------------------------------------------------------------

String ^ WbContext::read_option_value(String ^ model, String ^ key, String ^ default_value) {
  std::string raw_result;
  if (WBContextUI::get()->get_wb_options_value(NativeToCppStringRaw(model), NativeToCppStringRaw(key), raw_result))
    return CppStringToNativeRaw(raw_result);
  else
    return default_value;
}

//--------------------------------------------------------------------------------------------------

void WbContext::set_create_main_form_view(MySQL::Workbench::WbFrontendCallbacks ^ cbacks, VoidStrUIFormDelegate ^ dt) {
  create_main_form_view_delegate = dt;
  create_main_form_view_wrapper_delegate =
    gcnew VoidStrUIFormWrapperDelegate(this, &WbContext::create_main_form_view_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(create_main_form_view_wrapper_delegate);
  VOID_STR_UIFORM_CB cb = static_cast<VOID_STR_UIFORM_CB>(ip.ToPointer());

  ::wb::WBFrontendCallbacks *callbacks = cbacks->get_callbacks();
  callbacks->create_main_form_view = cb;
}

//--------------------------------------------------------------------------------------------------

void WbContext::create_main_form_view_wrapper(const std::string &view_name, std::shared_ptr<bec::UIForm> form_be) {
  String ^ name = CppStringToNativeRaw(view_name);
  Logger::LogDebug("WBContext managed", 1, String::Format("Creating UI wrapper {0}\n", name));

  UIForm ^ form = nullptr;

  if (0 == view_name.compare(WB_MAIN_VIEW_DB_QUERY)) {
    std::shared_ptr<::SqlEditorForm> ref(std::static_pointer_cast<::SqlEditorForm>(form_be));
    form = gcnew MySQL::GUI::Workbench::SqlEditorFormWrapper(&ref);
  }

  create_main_form_view_delegate(name, form);
}

//--------------------------------------------------------------------------------------------------

MenuStrip ^ WbContext::menu_for_form(MySQL::Base::UIForm ^ form) {
  mforms::MenuBar *menu = NULL;

  if (form != nullptr)
    menu = form->get_unmanaged_object()->get_menubar();
  if (menu == NULL)
    menu = WBContextUI::get()->get_command_ui()->create_menubar_for_context("");

  if (menu == NULL)
    return nullptr;

  return dynamic_cast<MenuStrip ^>(ObjectMapper::GetManagedComponent(menu));
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the menu strip for the appview that is docked on the given dock content object.
 * If there's no menu yet a standard one is created.
 */
MenuStrip ^ WbContext::menu_for_appview(MySQL::Forms::AppViewDockContent ^ content) {
  if (content == nullptr)
    return nullptr;

  MenuStrip ^ menuStrip = content->GetMenuBar();
  if (menuStrip != nullptr)
    return menuStrip;

  auto backend = content->GetBackend();
  mforms::MenuBar *menu = WBContextUI::get()->get_command_ui()->create_menubar_for_context(
    backend->is_main_form() ? backend->get_form_context_name() : "");
  if (menu == NULL)
    return nullptr;

  backend->set_menubar(menu);
  return content->GetMenuBar();
}

//--------------------------------------------------------------------------------------------------

void WbContext::validate_menu_for_form(MySQL::Base::UIForm ^ form) {
  mforms::MenuBar *menu = NULL;

  if (form != nullptr)
    menu = form->get_unmanaged_object()->get_menubar();
  if (menu != NULL)
    menu->validate();
}

//--------------------------------------------------------------------------------------------------

ToolStrip ^ WbContext::toolbar_for_form(MySQL::Base::UIForm ^ form) {
  mforms::ToolBar *toolbar = NULL;

  if (form != nullptr)
    toolbar = form->get_unmanaged_object()->get_toolbar();
  if (toolbar == NULL)
    return nullptr;

  return dynamic_cast<ToolStrip ^>(ObjectMapper::GetManagedComponent(toolbar));
}

//--------------------------------------------------------------------------------------------------

Control ^ WbContext::shared_secondary_sidebar() {
  mforms::View *sidebar = WBContextUI::get()->get_wb()->get_model_context()->shared_secondary_sidebar();
  if (sidebar == NULL)
    return nullptr;

  return dynamic_cast<Control ^>(ObjectMapper::GetManagedComponent(sidebar));
}

//--------------------------------------------------------------------------------------------------

/**
 * Searches the toolbar of the given form for a search box and makes it active.
 */
void WbContext::focus_search_box(MySQL::Base::UIForm ^ form) {
  mforms::ToolBar *toolbar = NULL;

  if (form != nullptr)
    toolbar = form->get_unmanaged_object()->get_toolbar();
  if (toolbar == NULL)
    return;

  mforms::ToolBarItem *searchbox = toolbar->find_item("find");
  if (searchbox != NULL) {
    ToolStripTextBox ^ textbox = dynamic_cast<ToolStripTextBox ^>(ObjectMapper::GetManagedComponent(searchbox));
    textbox->Select();
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Searches the toolbar of the given form for a search box and returns its content.
 * Usually used in response of a search action.
 * XXX: once the command ui can pass on search strings this function can go.
 */
String ^ WbContext::get_search_string(MySQL::Base::UIForm ^ form) {
  mforms::ToolBar *toolbar = NULL;

  if (form != nullptr)
    toolbar = form->get_unmanaged_object()->get_toolbar();
  if (toolbar == NULL)
    return "";

  mforms::ToolBarItem *searchBox = toolbar->find_item("find");
  if (searchBox != NULL)
    return CppStringToNativeRaw(searchBox->get_text());

  return "";
}

//--------------------------------------------------------------------------------------------------

String ^ WbContext::get_title() {
  return CppStringToNative(WBContextUI::get()->get_title());
}

//--------------------------------------------------------------------------------------------------

bool WbContext::has_unsaved_changes() {
  return WBContextUI::get()->get_wb()->has_unsaved_changes();
}

//--------------------------------------------------------------------------------------------------

void WbContext::open_document(String ^ file) {
  WBContextUI::get()->get_wb()->open_document(NativeToCppString(file));
}

//--------------------------------------------------------------------------------------------------

bool WbContext::save_changes() {
  return WBContextUI::get()->get_wb()->save_changes();
}

//--------------------------------------------------------------------------------------------------

void WbContext::flush_idle_tasks(bool force) {
  try {
    WBContextUI::get()->get_wb()->flush_idle_tasks(force);
  } catch (std::exception *ex) {
    throw gcnew MySQL::Grt::BackendException(ex);
  } catch (std::exception &ex) {
    throw gcnew MySQL::Grt::BackendException(&ex);
  } catch (...) {
    throw; // Don't wrap the exception into something completely useless. The original exception
           // often is converted automatically into a proper managed exception.
  }
}

//--------------------------------------------------------------------------------------------------

double WbContext::delay_for_next_timer() {
  return bec::GRTManager::get()->delay_for_next_timeout();
}

//--------------------------------------------------------------------------------------------------

void WbContext::flush_timers() {
  return bec::GRTManager::get()->flush_timers();
}

//--------------------------------------------------------------------------------------------------
