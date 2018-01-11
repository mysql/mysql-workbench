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

#ifndef _WBCALLBACKS_H_
#define _WBCALLBACKS_H_

#include "GrtTemplates.h"
#include "DelegateWrapper.h"

#include "workbench/wb_context.h"

#pragma make_public(wb::WBFrontendCallbacks)

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices; // Needed for the [Out] keyword.

using namespace MySQL::GUI::Mdc;
using namespace MySQL::Grt;

namespace MySQL {
  namespace Workbench {

    struct wb::WBFrontendCallbacks;
  public
    enum class RefreshType {
      RefreshNeeded = wb::RefreshNeeded,

      RefreshNothing = wb::RefreshNothing,
      RefreshSchemaNoReload = wb::RefreshSchemaNoReload,
      RefreshNewDiagram = wb::RefreshNewDiagram,
      RefreshSelection = wb::RefreshSelection,
      RefreshCloseEditor = wb::RefreshCloseEditor,
      RefreshNewModel = wb::RefreshNewModel,
      RefreshOverviewNodeInfo = wb::RefreshOverviewNodeInfo,
      RefreshOverviewNodeChildren = wb::RefreshOverviewNodeChildren,
      RefreshDocument = wb::RefreshDocument,
      RefreshCloseDocument = wb::RefreshCloseDocument,
      RefreshZoom = wb::RefreshZoom,
      RefreshTimer = wb::RefreshTimer,
      RefreshFinishEdits = wb::RefreshFinishEdits,
    };

  public
    enum class GUIPluginFlags {
      ForceNewWindowFlag = ::bec::ForceNewWindowFlag,
      StandaloneWindowFlag = ::bec::StandaloneWindowFlag
    };

  public
    ref class WbFrontendCallbacks {
    public:
      // Delegates used by C#.
      delegate void VoidDelegate();
      delegate bool BoolDelegate();
      delegate void VoidBoolDelegate(bool f);
      delegate void VoidStrDelegate(String ^ str1);
      delegate void VoidStrStrDelegate(String ^ str1, String ^ str2);
      delegate String ^ StrStrStrStrDelegate(String ^ str1, String ^ str2, String ^ str3);
      delegate bool BoolStrStrStrDelegate(String ^ str1, String ^ str2, String ^ str3);
      delegate bool BoolStrStrStrStrDelegate(String ^ str0, String ^ str1, String ^ str2, String ^ str3);
      delegate bool BoolStrStrFloatDelegate(String ^ str1, String ^ str2, float f3);
      delegate bool BoolStrIntStrPtrDelegate(String ^ str, int i, [Out] String ^ % result);
      delegate BaseWindowsCanvasView ^ CanvasViewStringStringDelegate(String ^ str1, String ^ str2);
      delegate void VoidCanvasViewDelegate(BaseWindowsCanvasView ^ canvasView);
      delegate IntPtr IntPtrGRTManagerModuleStrStrGrtListFlagsDelegate(GrtManager ^ grtManager, GrtModule ^ module,
                                                                       String ^ str1, String ^ str2, GrtValue ^ grtlist,
                                                                       GUIPluginFlags flags);
      delegate void VoidIntPtrDelegate(IntPtr p2);
      delegate void VoidRefreshTypeStringIntPtrDelegate(RefreshType refresh, String ^ str, IntPtr ptr);

    private:
      wb::WBFrontendCallbacks* _callbacks;

      // ----------------------------------------------------------------------------
      // Wrapper delegates used by the C++.net wrapper

      void set_show_file_dialog(StrStrStrStrDelegate ^ dt);
      void set_show_status_text(VoidStrDelegate ^ dt);
      void set_create_diagram(CanvasViewStringStringDelegate ^ dt);
      void set_destroy_view(VoidCanvasViewDelegate ^ dt);
      void set_switched_view(VoidCanvasViewDelegate ^ dt);
      void set_tool_changed(VoidCanvasViewDelegate ^ dt);
      void set_open_editor(IntPtrGRTManagerModuleStrStrGrtListFlagsDelegate ^ dt);
      void set_show_editor(VoidIntPtrDelegate ^ dt);
      void set_hide_editor(VoidIntPtrDelegate ^ dt);
      void set_refresh_gui(VoidRefreshTypeStringIntPtrDelegate ^ dt);
      void set_perform_command(VoidStrDelegate ^ dt);
      void set_lock_gui(VoidBoolDelegate ^ dt);
      void set_quit_application(BoolDelegate ^ dt);

      // void ()
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate void VoidWrapperDelegate();
      typedef void (*WbFrontendCallbacks::VOID_CB)();

      // bool ()
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate void BoolWrapperDelegate();
      typedef bool (*WbFrontendCallbacks::BOOL_CB)();

      // void (bool)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate void VoidBoolWrapperDelegate(bool b);
      typedef void (*WbFrontendCallbacks::VOID_BOOL_CB)(bool b);

      // void (string)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate void VoidStrWrapperDelegate(
        const std::string& str1);
      typedef void (*WbFrontendCallbacks::VOID_STR_CB)(const std::string& str1);

      // void (string, std::string)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate void VoidStrStrWrapperDelegate(
        const std::string& str1, const std::string& str2);
      typedef void (*WbFrontendCallbacks::VOID_STR_STR_CB)(const std::string& str1, const std::string& str2);

      // std::string (string, std::string, std::string)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate std::string StrStrStrStrWrapperDelegate(
        const std::string& str1, const std::string& str2, const std::string& str3);
      typedef std::string (*WbFrontendCallbacks::STR_STR_STR_STR_CB)(const std::string& str1, const std::string& str2,
                                                                     const std::string& str3);

      // bool (string, std::string, std::string)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate bool BoolStrStrStrWrapperDelegate(
        const std::string& str1, const std::string& str2, const std::string& str3);
      typedef bool (*WbFrontendCallbacks::BOOL_STR_STR_STR_CB)(const std::string& str1, const std::string& str2,
                                                               const std::string& str3);

      // int (string, std::string, std::string, std::string, std::string)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate int IntStrStrStrStrStrWrapperDelegate(
        const std::string& title, const std::string& str1, const std::string& str2, const std::string& str3,
        const std::string& str4);
      typedef int (*WbFrontendCallbacks::INT_STR_STR_STR_STR_STR_CB)(const std::string& title, const std::string& str1,
                                                                     const std::string& str2, const std::string& str3,
                                                                     const std::string& str4);

      // bool (string, std::string, float)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate bool BoolStrStrFloatWrapperDelegate(
        const std::string& str1, const std::string& str2, float f3);
      typedef bool (*WbFrontendCallbacks::BOOL_STR_STR_FLOAT_CB)(const std::string& str1, const std::string& str2,
                                                                 float f3);

      // bool (string, int, std::string&)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate bool BoolStrIntStrPtrWrapperDelegate(
        const std::string& str, int, std::string& res);
      typedef bool (*WbFrontendCallbacks::BOOL_STR_INT_STRPTR_CB)(const std::string& str, int, std::string& res);

      // CanvasView * (model_DiagramRef&)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate::mdc::CanvasView*
      CanvasViewDiagramWrapperDelegate(const model_DiagramRef& diagram);
      typedef ::mdc::CanvasView* (*WbFrontendCallbacks::CANVASVIEW_DIAGRAM_CB)(const model_DiagramRef& diagram);

      // void (CanvasView *)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate void VoidCanvasViewWrapperDelegate(
        ::mdc::CanvasView* canvas_view);
      typedef void (*WbFrontendCallbacks::VOID_CANVASVIEW_CB)(::mdc::CanvasView* canvas_view);

      // _int64 (Module*, std::string, std::string, GrtList, GUIPluginFlags)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate uintptr_t
      IntGRTManagerModuleStrStrGrtListFlagsWrapperDelegate(grt::Module*, const std::string& str2,
                                                           const std::string& str3, const grt::BaseListRef& grt_list,
                                                           bec::GUIPluginFlags flags);
      typedef uintptr_t (*WbFrontendCallbacks::INT_GRTMANAGER_MODULE_STR_STR_GRTLIST_FLAGS_CB)(
        grt::Module* module, const std::string& str2, const std::string& str3, const grt::BaseListRef& grt_list,
        bec::GUIPluginFlags flags);

      // void (uintptr_t)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate void VoidIntPtrWrapperDelegate(
        uintptr_t native_handle);
      typedef void (*WbFrontendCallbacks::VOID_INT64_CB)(uintptr_t native_handle);

      // void (RefreshType, std::string, uintptr_t)
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate void
      VoidRefreshTypeStringIntPtrWrapperDelegate(::wb::RefreshType refresh, const std::string& str, uintptr_t ptr);
      typedef void (*WbFrontendCallbacks::VOID_REFRESHTYPE_STRING_INTPTR_CB)(::wb::RefreshType refresh,
                                                                             const std::string& str, uintptr_t ptr);

      // ----------------------------------------------------------------------------
      // Variables used to store the C# delegates and the C++.Net wrapper delegates
      // Intermediate callback that is called from C++ and calls the C# delegate

      StrStrStrStrDelegate ^ show_file_dialog_delegate;
      StrStrStrStrWrapperDelegate ^ show_file_dialog_wrapper_delegate;
      std::string show_file_dialog_wrapper(const std::string& str1, const std::string& str2, const std::string& str3);

      VoidStrDelegate ^ show_status_text_delegate;
      VoidStrWrapperDelegate ^ show_status_text_wrapper_delegate;
      void show_status_text_wrapper(const std::string& str1);

      BoolStrStrFloatDelegate ^ show_progress_delegate;
      BoolStrStrFloatWrapperDelegate ^ show_progress_wrapper_delegate;
      bool show_progress_wrapper(const std::string& str1, const std::string& str2, float f3);

      VoidStrDelegate ^ shell_output_delegate;
      VoidStrWrapperDelegate ^ shell_output_wrapper_delegate;
      void shell_output_wrapper(const std::string& str1);

      // CanvasView
      CanvasViewStringStringDelegate ^ create_diagram_delegate;
      CanvasViewDiagramWrapperDelegate ^ create_diagram_wrapper_delegate;
      ::mdc::CanvasView* create_diagram_wrapper(const model_DiagramRef& model);

      VoidCanvasViewDelegate ^ destroy_view_delegate;
      VoidCanvasViewWrapperDelegate ^ destroy_view_wrapper_delegate;
      void destroy_view_wrapper(::mdc::CanvasView* canvas_view);

      VoidCanvasViewDelegate ^ switched_view_delegate;
      VoidCanvasViewWrapperDelegate ^ switched_view_wrapper_delegate;
      void switched_view_wrapper(::mdc::CanvasView* canvas_view);

      VoidCanvasViewDelegate ^ tool_changed_delegate;
      VoidCanvasViewWrapperDelegate ^ tool_changed_wrapper_delegate;
      void tool_changed_wrapper(::mdc::CanvasView* canvas_view);

      // Editors
      IntPtrGRTManagerModuleStrStrGrtListFlagsDelegate ^ open_editor_delegate;
      IntGRTManagerModuleStrStrGrtListFlagsWrapperDelegate ^ open_editor_wrapper_delegate;
      uintptr_t open_editor_wrapper(grt::Module* module, const std::string& str2, const std::string& str3,
                                    const grt::BaseListRef& grt_list, bec::GUIPluginFlags flags);

      VoidIntPtrDelegate ^ show_editor_delegate;
      VoidIntPtrWrapperDelegate ^ show_editor_wrapper_delegate;
      void show_editor_wrapper(uintptr_t native_handle);

      VoidIntPtrDelegate ^ hide_editor_delegate;
      VoidIntPtrWrapperDelegate ^ hide_editor_wrapper_delegate;
      void hide_editor_wrapper(uintptr_t native_handle);

      // UI related callbacks.
      VoidRefreshTypeStringIntPtrDelegate ^ refresh_gui_delegate;
      VoidRefreshTypeStringIntPtrWrapperDelegate ^ refresh_gui_wrapper_delegate;
      void refresh_gui_wrapper(::wb::RefreshType refresh, const std::string& str, uintptr_t ptr);

      VoidStrDelegate ^ perform_command_delegate;
      VoidStrWrapperDelegate ^ perform_command_wrapper_delegate;
      void perform_command_wrapper(const std::string& command);

      VoidBoolDelegate ^ lock_gui_delegate;
      VoidBoolWrapperDelegate ^ lock_gui_wrapper_delegate;
      void lock_gui_wrapper(bool flag);

      BoolDelegate ^ quit_application_delegate;
      VoidWrapperDelegate ^ quit_application_wrapper_delegate;
      void quit_application_wrapper();

    public:
      WbFrontendCallbacks(StrStrStrStrDelegate ^ show_file_dialog, VoidStrDelegate ^ show_status_text,
                          BoolStrStrFloatDelegate ^ show_progress, CanvasViewStringStringDelegate ^ create_diagram,
                          VoidCanvasViewDelegate ^ destroy_view, VoidCanvasViewDelegate ^ switched_view,
                          VoidCanvasViewDelegate ^ tool_changed,
                          IntPtrGRTManagerModuleStrStrGrtListFlagsDelegate ^ open_editor,
                          VoidIntPtrDelegate ^ show_editor, VoidIntPtrDelegate ^ hide_editor,
                          VoidRefreshTypeStringIntPtrDelegate ^ refresh_gui, VoidBoolDelegate ^ lock_gui,
                          VoidStrDelegate ^ perform_command, BoolDelegate ^ quit_application);

      ~WbFrontendCallbacks();

      ::wb::WBFrontendCallbacks* get_callbacks();
    };
  };
}

#endif
