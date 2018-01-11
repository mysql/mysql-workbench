/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "WbCallbacks.h"

#include "mdc.h"
#include "GrtTemplates.h"

using namespace base;

using namespace MySQL::Workbench;

WbFrontendCallbacks::WbFrontendCallbacks(StrStrStrStrDelegate ^ show_file_dialog, VoidStrDelegate ^ show_status_text,
                                         BoolStrStrFloatDelegate ^ show_progress,
                                         CanvasViewStringStringDelegate ^ create_diagram,
                                         VoidCanvasViewDelegate ^ destroy_view, VoidCanvasViewDelegate ^ switched_view,
                                         VoidCanvasViewDelegate ^ tool_changed,
                                         IntPtrGRTManagerModuleStrStrGrtListFlagsDelegate ^ open_editor,
                                         VoidIntPtrDelegate ^ show_editor, VoidIntPtrDelegate ^ hide_editor,
                                         VoidRefreshTypeStringIntPtrDelegate ^ refresh_gui, VoidBoolDelegate ^ lock_gui,
                                         VoidStrDelegate ^ perform_command, BoolDelegate ^ quit_application)
  : _callbacks(new ::wb::WBFrontendCallbacks()) {
  Logger::LogDebug("WBCallbacks", 1, "Creating callbacks wrapper\n");

  set_show_file_dialog(show_file_dialog);
  set_show_status_text(show_status_text);

  set_create_diagram(create_diagram);
  set_destroy_view(destroy_view);
  set_switched_view(switched_view);
  set_tool_changed(tool_changed);

  set_open_editor(open_editor);
  set_show_editor(show_editor);
  set_hide_editor(hide_editor);

  set_refresh_gui(refresh_gui);
  set_perform_command(perform_command);

  set_lock_gui(lock_gui);

  set_quit_application(quit_application);
}

WbFrontendCallbacks::~WbFrontendCallbacks() {
  delete _callbacks;
}

//--------------------------------------------------------------------------------------------------

::wb::WBFrontendCallbacks* WbFrontendCallbacks::get_callbacks() {
  return _callbacks;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_show_file_dialog(StrStrStrStrDelegate ^ dt) {
  show_file_dialog_delegate = dt;
  show_file_dialog_wrapper_delegate =
    gcnew StrStrStrStrWrapperDelegate(this, &WbFrontendCallbacks::show_file_dialog_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(show_file_dialog_wrapper_delegate);
  STR_STR_STR_STR_CB cb = static_cast<STR_STR_STR_STR_CB>(ip.ToPointer());
  _callbacks->show_file_dialog = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_show_status_text(VoidStrDelegate ^ dt) {
  show_status_text_delegate = dt;
  show_status_text_wrapper_delegate =
    gcnew VoidStrWrapperDelegate(this, &WbFrontendCallbacks::show_status_text_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(show_status_text_wrapper_delegate);
  VOID_STR_CB cb = static_cast<VOID_STR_CB>(ip.ToPointer());
  _callbacks->show_status_text = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_create_diagram(CanvasViewStringStringDelegate ^ dt) {
  create_diagram_delegate = dt;
  create_diagram_wrapper_delegate =
    gcnew CanvasViewDiagramWrapperDelegate(this, &WbFrontendCallbacks::create_diagram_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(create_diagram_wrapper_delegate);
  CANVASVIEW_DIAGRAM_CB cb = static_cast<CANVASVIEW_DIAGRAM_CB>(ip.ToPointer());
  _callbacks->create_diagram = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_destroy_view(VoidCanvasViewDelegate ^ dt) {
  destroy_view_delegate = dt;
  destroy_view_wrapper_delegate = gcnew VoidCanvasViewWrapperDelegate(this, &WbFrontendCallbacks::destroy_view_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(destroy_view_wrapper_delegate);
  VOID_CANVASVIEW_CB cb = static_cast<VOID_CANVASVIEW_CB>(ip.ToPointer());
  _callbacks->destroy_view = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_switched_view(VoidCanvasViewDelegate ^ dt) {
  switched_view_delegate = dt;
  switched_view_wrapper_delegate =
    gcnew VoidCanvasViewWrapperDelegate(this, &WbFrontendCallbacks::switched_view_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(switched_view_wrapper_delegate);
  VOID_CANVASVIEW_CB cb = static_cast<VOID_CANVASVIEW_CB>(ip.ToPointer());
  _callbacks->switched_view = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_tool_changed(VoidCanvasViewDelegate ^ dt) {
  tool_changed_delegate = dt;
  tool_changed_wrapper_delegate = gcnew VoidCanvasViewWrapperDelegate(this, &WbFrontendCallbacks::tool_changed_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(tool_changed_wrapper_delegate);
  VOID_CANVASVIEW_CB cb = static_cast<VOID_CANVASVIEW_CB>(ip.ToPointer());
  _callbacks->tool_changed = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_open_editor(IntPtrGRTManagerModuleStrStrGrtListFlagsDelegate ^ dt) {
  open_editor_delegate = dt;
  open_editor_wrapper_delegate =
    gcnew IntGRTManagerModuleStrStrGrtListFlagsWrapperDelegate(this, &WbFrontendCallbacks::open_editor_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(open_editor_wrapper_delegate);
  INT_GRTMANAGER_MODULE_STR_STR_GRTLIST_FLAGS_CB cb =
    static_cast<INT_GRTMANAGER_MODULE_STR_STR_GRTLIST_FLAGS_CB>(ip.ToPointer());
  _callbacks->open_editor = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_show_editor(VoidIntPtrDelegate ^ dt) {
  show_editor_delegate = dt;
  show_editor_wrapper_delegate = gcnew VoidIntPtrWrapperDelegate(this, &WbFrontendCallbacks::show_editor_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(show_editor_wrapper_delegate);
  VOID_INT64_CB cb = static_cast<VOID_INT64_CB>(ip.ToPointer());
  _callbacks->show_editor = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_hide_editor(VoidIntPtrDelegate ^ dt) {
  hide_editor_delegate = dt;
  hide_editor_wrapper_delegate = gcnew VoidIntPtrWrapperDelegate(this, &WbFrontendCallbacks::hide_editor_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(hide_editor_wrapper_delegate);
  VOID_INT64_CB cb = static_cast<VOID_INT64_CB>(ip.ToPointer());
  _callbacks->hide_editor = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_refresh_gui(VoidRefreshTypeStringIntPtrDelegate ^ dt) {
  refresh_gui_delegate = dt;
  refresh_gui_wrapper_delegate =
    gcnew VoidRefreshTypeStringIntPtrWrapperDelegate(this, &WbFrontendCallbacks::refresh_gui_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(refresh_gui_wrapper_delegate);
  VOID_REFRESHTYPE_STRING_INTPTR_CB cb = static_cast<VOID_REFRESHTYPE_STRING_INTPTR_CB>(ip.ToPointer());
  _callbacks->refresh_gui = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_perform_command(VoidStrDelegate ^ dt) {
  perform_command_delegate = dt;
  perform_command_wrapper_delegate = gcnew VoidStrWrapperDelegate(this, &WbFrontendCallbacks::perform_command_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(perform_command_wrapper_delegate);
  VOID_STR_CB cb = static_cast<VOID_STR_CB>(ip.ToPointer());
  _callbacks->perform_command = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_lock_gui(VoidBoolDelegate ^ dt) {
  lock_gui_delegate = dt;
  lock_gui_wrapper_delegate = gcnew VoidBoolWrapperDelegate(this, &WbFrontendCallbacks::lock_gui_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(lock_gui_wrapper_delegate);
  VOID_BOOL_CB cb = static_cast<VOID_BOOL_CB>(ip.ToPointer());
  _callbacks->lock_gui = cb;
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::set_quit_application(BoolDelegate ^ dt) {
  quit_application_delegate = dt;
  quit_application_wrapper_delegate = gcnew VoidWrapperDelegate(this, &WbFrontendCallbacks::quit_application_wrapper);
  IntPtr ip = Marshal::GetFunctionPointerForDelegate(quit_application_wrapper_delegate);
  BOOL_CB cb = static_cast<BOOL_CB>(ip.ToPointer());
  _callbacks->quit_application = cb;
}

//--------------------------------------------------------------------------------------------------

std::string WbFrontendCallbacks::show_file_dialog_wrapper(const std::string& str1, const std::string& str2,
                                                          const std::string& str3) {
  return NativeToCppString(
    show_file_dialog_delegate(CppStringToNative(str1), CppStringToNative(str2), CppStringToNative(str3)));
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::show_status_text_wrapper(const std::string& str1) {
  show_status_text_delegate(CppStringToNative(str1));
}

//--------------------------------------------------------------------------------------------------

bool WbFrontendCallbacks::show_progress_wrapper(const std::string& str1, const std::string& str2, float f3) {
  return show_progress_delegate(CppStringToNative(str1), CppStringToNative(str2), f3);
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::shell_output_wrapper(const std::string& str1) {
  shell_output_delegate(CppStringToNative(str1));
}

//--------------------------------------------------------------------------------------------------

::mdc::CanvasView* WbFrontendCallbacks::create_diagram_wrapper(const model_DiagramRef& model) {
  BaseWindowsCanvasView ^ cv = create_diagram_delegate(CppStringToNative(model->id()),
                                                       CppStringToNative(model->name()) /*, IntPtr(&model.content())*/);
  return cv->get_unmanaged_object();
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::destroy_view_wrapper(::mdc::CanvasView* canvas_view) {
  BaseWindowsCanvasView ^ wcv = BaseWindowsCanvasView::GetFromFixedId((IntPtr)canvas_view->get_user_data());
  destroy_view_delegate(wcv);
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::switched_view_wrapper(::mdc::CanvasView* canvas_view) {
  BaseWindowsCanvasView ^ wcv = BaseWindowsCanvasView::GetFromFixedId((IntPtr)canvas_view->get_user_data());
  switched_view_delegate(wcv);
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::tool_changed_wrapper(::mdc::CanvasView* canvas_view) {
  BaseWindowsCanvasView ^ wcv = BaseWindowsCanvasView::GetFromFixedId((IntPtr)canvas_view->get_user_data());
  tool_changed_delegate(wcv);
}

//--------------------------------------------------------------------------------------------------

uintptr_t WbFrontendCallbacks::open_editor_wrapper(grt::Module* module, const std::string& str2,
                                                   const std::string& str3, const grt::BaseListRef& grt_list,
                                                   bec::GUIPluginFlags flags) {
  return (uintptr_t)open_editor_delegate(gcnew GrtManager(), gcnew GrtModule(module), CppStringToNative(str2),
                                         CppStringToNative(str3), gcnew GrtValue(grt_list), (GUIPluginFlags)flags)
    .ToPointer();
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::show_editor_wrapper(uintptr_t native_handle) {
  show_editor_delegate(IntPtr((void*)native_handle));
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::hide_editor_wrapper(uintptr_t native_handle) {
  hide_editor_delegate(IntPtr((void*)native_handle));
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::refresh_gui_wrapper(::wb::RefreshType refresh, const std::string& str, uintptr_t ptr) {
  refresh_gui_delegate((RefreshType)refresh, CppStringToNative(str), IntPtr((void*)ptr));
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::perform_command_wrapper(const std::string& command) {
  perform_command_delegate(CppStringToNative(command));
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::lock_gui_wrapper(bool flag) {
  lock_gui_delegate(flag);
}

//--------------------------------------------------------------------------------------------------

void WbFrontendCallbacks::quit_application_wrapper() {
  quit_application_delegate();
}
