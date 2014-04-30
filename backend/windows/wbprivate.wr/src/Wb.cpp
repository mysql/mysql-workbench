/* 
 * Copyright (c) 2010, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "sqlide/wb_sql_editor_form.h"

#include "base/string_utilities.h"
#include "workbench/wb_overview.h"
#include "ConvUtils.h"
#include "GrtTemplates.h"
#include "Overview.h"
#include "Wb.h"

#include "SQLEditorFormWrapper.h"

#include "mforms/menubar.h"
#include "mforms/appview.h"

using namespace System::Text;
using namespace System::Threading;
using namespace System::Windows::Forms;

using namespace Aga::Controls::Tree;

using namespace base;

using namespace MySQL::Base;
using namespace MySQL::Forms;
using namespace MySQL::Workbench;

//--------------------------------------------------------------------------------------------------

WbOptions::WbOptions(String^ baseDir, String^ userDir)
{
  inner = new wb::WBOptions();
  inner->basedir = NativeToCppStringRaw(baseDir);
  inner->plugin_search_path = inner->basedir;
  inner->module_search_path = inner->basedir + "/modules";
  inner->struct_search_path = "";
  inner->library_search_path = inner->basedir;
  inner->user_data_dir = NativeToCppStringRaw(userDir);
}

//--------------------------------------------------------------------------------------------------

bool WbOptions::parse_args(array<String^>^ args, String^ app_path)
{
  // Convert to UTF8 and keep the returned arrays as long as we are parsing that.
  // Add the application's path at the tip as the parse routine wants it so.
  array<array<unsigned char>^>^ managed_utf8 = gcnew array<array<unsigned char>^>(args->Length + 1);
  managed_utf8[0] = Encoding::UTF8->GetBytes(app_path);
  for (int i = 0; i < args->Length; i++)
    managed_utf8[i + 1] = Encoding::UTF8->GetBytes(args[i]);

  // Collect the managed string arrays into a c-like char* for parsing.
  char** arguments = new char*[managed_utf8->Length];
  try
  {
    for (int i = 0; i < managed_utf8->Length; i++)
    {
      pin_ptr<unsigned char> chars = &managed_utf8[i][0];
      arguments[i] = (char*)chars;
    }

    return inner->parse_args(arguments, managed_utf8->Length);
  }
  finally
  {
    delete arguments;
  }
  
  return true;
}

//--------------------------------------------------------------------------------------------------

String^ WbOptions::OpenAtStartup::get()
{
  return CppStringToNative(inner->open_at_startup);
}

//--------------------------------------------------------------------------------------------------

String^ WbOptions::OpenAtStartupType::get()
{
  return CppStringToNative(inner->open_at_startup_type);
}

//----------------- WbContext ----------------------------------------------------------------------

WbContext::WbContext(bool verbose)
{
  inner = new ::wb::WBContextUI(verbose);
  manager = nullptr;
  physical_overview = nullptr;

  Logger::LogDebug("WBContext managed", 1, "Creating WbContext\n");
}

//--------------------------------------------------------------------------------------------------

WbContext::~WbContext()
{
  delete physical_overview;
  delete inner;
  delete manager;

  Logger::LogDebug("WBContext managed", 1, "Destroying WbContext\n");
}

//--------------------------------------------------------------------------------------------------

bool WbContext::init(WbFrontendCallbacks ^callbacks, WbOptions ^options,
  VoidStrUIFormDelegate^ create_main_form_view)
{
  set_create_main_form_view(callbacks, create_main_form_view);
  return inner->init(
    callbacks->get_callbacks(), 
    options->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

GrtManager^ WbContext::get_grt_manager()
{
  if (manager == nullptr)
    manager= gcnew GrtManager(inner->get_wb()->get_grt_manager());

  return manager; 
}

//--------------------------------------------------------------------------------------------------

void WbContext::add_frontend_commands(List<String^>^ commands)
{
  inner->get_command_ui()->add_frontend_commands(NativeToCppStringList2(commands));
}

//--------------------------------------------------------------------------------------------------

void WbContext::remove_frontend_commands(List<String^>^ commands)
{
  inner->get_command_ui()->remove_frontend_commands(NativeToCppStringList2(commands));
}

//--------------------------------------------------------------------------------------------------

void WbContext::activate_command(String^ name)
{
  std::string command= NativeToCppString(name);
  inner->get_command_ui()->activate_command(command);
}

//--------------------------------------------------------------------------------------------------

Overview^ WbContext::get_physical_overview()
{
  if (physical_overview == nullptr)
    physical_overview= gcnew Overview(inner->get_physical_overview());
  return physical_overview;
}

//--------------------------------------------------------------------------------------------------

TreeViewAdv^ WbContext::get_catalog_tree()
{
  // Note: this and the other 2 get_* functions below leak memory (the created trees).
  //       This will be solved once the entire sidebars are managed by the backend.
  return dynamic_cast<TreeViewAdv ^>(ObjectMapper::GetManagedComponent(inner->get_wb()->get_model_context()->create_catalog_tree()));
}

//--------------------------------------------------------------------------------------------------

TreeViewAdv^ WbContext::get_history_tree()
{
  return dynamic_cast<TreeViewAdv ^>(ObjectMapper::GetManagedComponent(inner->get_wb()->get_model_context()->create_history_tree()));
}

//--------------------------------------------------------------------------------------------------

TreeViewAdv^ WbContext::get_usertypes_tree()
{
  return dynamic_cast<TreeViewAdv ^>(ObjectMapper::GetManagedComponent(inner->get_wb()->get_model_context()->create_user_type_list()));
} 

//--------------------------------------------------------------------------------------------------

MySQL::Grt::GrtValueInspector^ WbContext::get_inspector_for_selection(UIForm^ form, [Out] List<String^>^ %items)
{
  std::vector<std::string> vitems;
  bec::ValueInspectorBE *insp = 
    inner->create_inspector_for_selection(form->get_unmanaged_object(), vitems);
  if (!insp)
    insp = inner->create_inspector_for_selection(vitems);
  if (insp)
  {
    items= CppStringListToNative(vitems);
    return gcnew ::MySQL::Grt::GrtValueInspector(insp, true);
  }
  return nullptr;
}

//--------------------------------------------------------------------------------------------------

bool WbContext::are_lists_equal(GrtValue^ v1, GrtValue^ v2)
{
  if ((nullptr == v1) || (nullptr == v2))
    return (v1 == v2);
  ::grt::ValueRef v1_(v1->get_unmanaged_object());
  ::grt::ValueRef v2_(v2->get_unmanaged_object());
  ::grt::ObjectListRef l1_= ::grt::ObjectListRef::cast_from(v1_);
  ::grt::ObjectListRef l2_= ::grt::ObjectListRef::cast_from(v2_);

  return ::grt::compare_list_contents(l1_, l2_);
}

//--------------------------------------------------------------------------------------------------

String^ WbContext::get_description_for_selection(UIForm^ form, [Out] GrtValue^ %activeObjList, [Out] List<String^>^ %items)
{
  std::vector<std::string> vitems;
  ::grt::ListRef<GrtObject> activeObjList_;
  std::string val= 
    inner->get_description_for_selection(form->get_unmanaged_object(), activeObjList_, vitems);
  activeObjList= gcnew GrtValue(activeObjList_);
  items= CppStringListToNative(vitems);
  return CppStringToNative(val);
}

//--------------------------------------------------------------------------------------------------

String^ WbContext::get_description_for_selection([Out] GrtValue^ %activeObjList, [Out] List<String^>^ %items)
{
  std::vector<std::string> vitems;
  ::grt::ListRef<GrtObject> activeObjList_;
  std::string val= inner->get_description_for_selection(activeObjList_, vitems);
  activeObjList= gcnew GrtValue(activeObjList_);
  items= CppStringListToNative(vitems);
  return CppStringToNative(val);
}

//--------------------------------------------------------------------------------------------------

void WbContext::set_description_for_selection(GrtValue^ activeObjList, String^ val)
{
  std::string val_= NativeToCppString(val);
  ::grt::ListRef<GrtObject> activeObjList_(::grt::ListRef<GrtObject>::cast_from(activeObjList->get_unmanaged_object()));
  inner->set_description_for_selection(activeObjList_, val_);
}

//--------------------------------------------------------------------------------------------------

MySQL::Workbench::ModelDiagramFormWrapper^ WbContext::get_diagram_form_for_diagram(String^ id)
{
  return gcnew ModelDiagramFormWrapper(inner->get_wb()->get_model_context()->get_diagram_form_for_diagram_id(NativeToCppString(id)));
}


//--------------------------------------------------------------------------------------------------

void WbContext::set_active_form(UIForm^ uiform)
{
  if (uiform == nullptr)
    inner->set_active_form(0);
  else
    inner->set_active_form(uiform->get_unmanaged_object());
}

//--------------------------------------------------------------------------------------------------

void WbContext::set_active_form_from_appview(MySQL::Forms::AppViewDockContent ^form)
{
  if (form == nullptr)
  {
    Logger::LogDebug("WBContext managed", 2, "Activating appview 0\n");
    inner->set_active_form(0);
  }
  else
  {
    Logger::LogDebug("WBContext managed", 2, String::Format("Activating appview {0}\n",
      form->GetAppViewIdentifier()));
    inner->set_active_form(form->GetBackend());
  }
}

//--------------------------------------------------------------------------------------------------

String^ WbContext::get_active_context()
{
  return CppStringToNative(inner->get_active_context());
}

//--------------------------------------------------------------------------------------------------

void WbContext::show_output()
{
  inner->show_output();
}

//--------------------------------------------------------------------------------------------------

void WbContext::close_gui_plugin(IntPtr handle)
{
  inner->get_wb()->close_gui_plugin((NativeHandle)handle.ToPointer());
}

//--------------------------------------------------------------------------------------------------

void WbContext::execute_plugin(String^ name)
{
  inner->get_wb()->execute_plugin(NativeToCppStringRaw(name));
}

//--------------------------------------------------------------------------------------------------

void WbContext::report_bug(String^ errorInfo)
{
  inner->get_wb()->report_bug(NativeToCppStringRaw(errorInfo));
}

//--------------------------------------------------------------------------------------------------

String^ WbContext::read_state(String^ name, String^ domain, String^ default_value)
{
  std::string value= inner->get_wb()->read_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), 
    NativeToCppString(default_value));
  
  return CppStringToNative(value);
}

//--------------------------------------------------------------------------------------------------

int WbContext::read_state(String^ name, String^ domain, const int default_value)
{
  int value= inner->get_wb()->read_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), 
    default_value);
  
  return value;
}

//--------------------------------------------------------------------------------------------------

double WbContext::read_state(String^ name, String^ domain, const double default_value)
{
  double value= inner->get_wb()->read_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), 
    default_value);
  
  return value;
}

//--------------------------------------------------------------------------------------------------

bool WbContext::read_state(String^ name, String^ domain, const bool default_value)
{
  bool value= inner->get_wb()->read_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), 
    default_value);
  
  return value;
}

//--------------------------------------------------------------------------------------------------

void WbContext::save_state(String^ name, String^ domain, String^ value)
{
  inner->get_wb()->save_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), 
    NativeToCppString(value));
}

//--------------------------------------------------------------------------------------------------

void WbContext::save_state(String^ name, String^ domain, const int value)
{
  inner->get_wb()->save_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), value);
}

//--------------------------------------------------------------------------------------------------

void WbContext::save_state(String^ name, String^ domain, const double value)
{
  inner->get_wb()->save_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), value);
}

//--------------------------------------------------------------------------------------------------

void WbContext::save_state(String^ name, String^ domain, const bool value)
{
  inner->get_wb()->save_state(NativeToCppStringRaw(name), NativeToCppStringRaw(domain), value);
}

//--------------------------------------------------------------------------------------------------

String^ WbContext::read_option_value(String^ model, String^ key, String^ default_value)
{
  std::string raw_result;
  if (inner->get_wb_options_value(NativeToCppStringRaw(model), NativeToCppStringRaw(key), raw_result))
    return CppStringToNativeRaw(raw_result);
  else
    return default_value;
}

//--------------------------------------------------------------------------------------------------

void WbContext::set_create_main_form_view(MySQL::Workbench::WbFrontendCallbacks^ cbacks, VoidStrUIFormDelegate^ dt)
{
  create_main_form_view_delegate= dt;
  create_main_form_view_wrapper_delegate=
    gcnew VoidStrUIFormWrapperDelegate(this, &WbContext::create_main_form_view_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(create_main_form_view_wrapper_delegate);
  VOID_STR_UIFORM_CB cb = static_cast<VOID_STR_UIFORM_CB>(ip.ToPointer());

  ::wb::WBFrontendCallbacks* callbacks = cbacks->get_callbacks();
  callbacks->create_main_form_view= std::ptr_fun(cb);
}

//--------------------------------------------------------------------------------------------------

void WbContext::create_main_form_view_wrapper(const std::string& view_name, boost::shared_ptr<bec::UIForm> form_be)
{
  String^ name = CppStringToNativeRaw(view_name);
  Logger::LogDebug("WBContext managed", 1, String::Format("Creating UI wrapper {0}\n", name));

  UIForm ^form = nullptr;

  if (0 == view_name.compare(WB_MAIN_VIEW_DB_QUERY))
  {
    boost::shared_ptr<::SqlEditorForm> ref(boost::static_pointer_cast<::SqlEditorForm>(form_be));
    form= gcnew MySQL::GUI::Workbench::SqlEditorFormWrapper(&ref);
  }

  create_main_form_view_delegate(name, form);
}

//--------------------------------------------------------------------------------------------------

MenuStrip^ WbContext::menu_for_form(MySQL::Base::UIForm^ form)
{
  mforms::MenuBar *menu = NULL;
  
  if (form != nullptr)
    menu = form->get_unmanaged_object()->get_menubar();
  if (menu == NULL)
    menu = inner->get_command_ui()->create_menubar_for_context("");

  if (menu == NULL)
    return nullptr;

  return dynamic_cast<MenuStrip ^>(ObjectMapper::GetManagedComponent(menu));
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the menu strip for the appview that is docked on the given dock content object.
 * If there's no menu yet a standard one is created.
 */
MenuStrip^ WbContext::menu_for_appview(MySQL::Forms::AppViewDockContent ^content)
{
  if (content == nullptr)
    return nullptr;

  MenuStrip ^menuStrip = content->GetMenuBar();
  if (menuStrip != nullptr)
    return menuStrip;

  mforms::MenuBar *menu = inner->get_command_ui()->create_menubar_for_context("");
  if (menu == NULL)
    return nullptr;

  content->GetBackend()->set_menubar(menu);
  return content->GetMenuBar();
}

//--------------------------------------------------------------------------------------------------

void WbContext::validate_menu_for_form(MySQL::Base::UIForm^ form)
{
  mforms::MenuBar* menu = NULL;

  if (form != nullptr)
    menu = form->get_unmanaged_object()->get_menubar();
  if (menu != NULL)
    menu->validate();
}

//--------------------------------------------------------------------------------------------------

ToolStrip^ WbContext::toolbar_for_form(MySQL::Base::UIForm ^form)
{
  mforms::ToolBar *toolbar = NULL;
  
  if (form != nullptr)
    toolbar = form->get_unmanaged_object()->get_toolbar();
  if (toolbar == NULL)
    return nullptr;

  return dynamic_cast<ToolStrip ^>(ObjectMapper::GetManagedComponent(toolbar));
}

//--------------------------------------------------------------------------------------------------

Control ^WbContext::shared_secondary_sidebar()
{
  mforms::View *sidebar = inner->get_wb()->get_model_context()->shared_secondary_sidebar();
  if (sidebar == NULL)
    return nullptr;

  return dynamic_cast<Control^>(ObjectMapper::GetManagedComponent(sidebar));
}

//--------------------------------------------------------------------------------------------------

/**
 * Searches the toolbar of the given form for a search box and makes it active.
 */
void WbContext::focus_search_box(MySQL::Base::UIForm^ form)
{
  mforms::ToolBar *toolbar = NULL;

  if (form != nullptr)
    toolbar = form->get_unmanaged_object()->get_toolbar();
  if (toolbar == NULL)
    return;

  mforms::ToolBarItem *searchbox = toolbar->find_item("find");
  if (searchbox != NULL)
  {
    ToolStripTextBox ^textbox = dynamic_cast<ToolStripTextBox^>(ObjectMapper::GetManagedComponent(searchbox));
    textbox->Select();
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Searches the toolbar of the given form for a search box and returns its content.
 * Usually used in response of a search action.
 * XXX: once the command ui can pass on search strings this function can go.
 */
String^ WbContext::get_search_string(MySQL::Base::UIForm^ form)
{
  mforms::ToolBar* toolbar = NULL;

  if (form != nullptr)
    toolbar = form->get_unmanaged_object()->get_toolbar();
  if (toolbar == NULL)
    return "";

  mforms::ToolBarItem* searchBox = toolbar->find_item("find");
  if (searchBox != NULL)
    return CppStringToNativeRaw(searchBox->get_text());

  return "";
}

//--------------------------------------------------------------------------------------------------

String^ WbContext::get_title()
{
  return CppStringToNative(inner->get_title());
}

//--------------------------------------------------------------------------------------------------

bool WbContext::has_unsaved_changes()
{
  return inner->get_wb()->has_unsaved_changes();
}

//--------------------------------------------------------------------------------------------------

void WbContext::open_document(String^ file)
{
  inner->get_wb()->open_document(NativeToCppString(file));
}

//--------------------------------------------------------------------------------------------------

bool WbContext::save_changes()
{
  return inner->get_wb()->save_changes();
}

//--------------------------------------------------------------------------------------------------

void WbContext::flush_idle_tasks()
{
  try 
  {
    inner->get_wb()->flush_idle_tasks(); 
  }
  catch(std::exception *ex)
  {
    throw gcnew BackendException(ex);
  }
  catch(std::exception &ex)
  {
    throw gcnew BackendException(&ex);
  }
  catch(...)
  {
    throw; // Don't wrap the exception into something completely useless. The original exception
           // often is converted automatically into a proper managed exception.
  }
}

//--------------------------------------------------------------------------------------------------

double WbContext::delay_for_next_timer()
{
  return inner->get_wb()->get_grt_manager()->delay_for_next_timeout();
}

//--------------------------------------------------------------------------------------------------

void WbContext::flush_timers()
{
  return inner->get_wb()->get_grt_manager()->flush_timers();
}

//--------------------------------------------------------------------------------------------------
