/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __GRT_MANAGER_H__
#define __GRT_MANAGER_H__

#include "ModelWrappers.h"
#include "GrtShell.h"
#include "GrtValueInspector.h"

namespace MySQL {
  namespace Grt {

    public ref class GrtManager 
    {
    public:
      delegate bool BoolStringStringFloatDelegate(String^ str1, String^ str2, float f);

    private:
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)]
      delegate bool BoolStringStringFloatWrapperDelegate(const std::string& str1, const std::string& str2, float f);
      typedef bool (*GrtManager::BOOL_STRING_STRING_FLOAT_HANDLER_CB)(const std::string& str1, const std::string& str2, float f);

      ::bec::GRTManager *inner;
      GrtShell^ managed_shell;
      GRT^ managed_grt;
      BoolStringStringFloatDelegate^ progress_callback_delegate;
      BoolStringStringFloatWrapperDelegate^ progress_callback_wrapper_delegate;
      
      bool progress_callback_wrapper(const std::string& str1, const std::string& str2, float f)
      {
        return progress_callback_delegate(CppStringToNative(str1), CppStringToNative(str2), f);
      }
      
    public:
      GrtManager(bool threaded, bool verbose)
        : inner(new ::bec::GRTManager(threaded, verbose)), managed_shell(nullptr), managed_grt(nullptr)
      {}

      GrtManager(::bec::GRTManager *grt_manager)
        : inner(grt_manager), managed_shell(nullptr), managed_grt(nullptr)
      {}

      explicit GrtManager()
        : inner(nullptr), managed_shell(nullptr), managed_grt(nullptr)
      {}

      virtual ~GrtManager()
      { /*delete inner;*/ }

      ::bec::GRTManager *get_unmanaged_object()
      { return static_cast<::bec::GRTManager *>(inner); }

      String^ get_app_option_string(String^ option_name)
      { return CppStringToNative(inner->get_app_option_string(NativeToCppString(option_name))); }

      Font^ get_font_option(String^ option_name)
      {
        String^ font_string= CppStringToNative(inner->get_app_option_string(NativeToCppString(option_name)));
        return MySQL::Utilities::ControlUtilities::GetFont(font_string);
      }

      void set_module_extensions(List<String^>^ extensions)
      {
        inner->set_module_extensions(NativeToCppStringList2(extensions));
      }

      void set_search_paths(String^ module_sp, String^ struct_sp, String^ libs_sp)
      {
        inner->set_search_paths(
          NativeToCppString(module_sp),
          NativeToCppString(struct_sp),
          NativeToCppString(libs_sp));
      }

      GRT^ get_grt() 
      { 
        if(managed_grt == nullptr)
          managed_grt= gcnew GRT(inner->get_grt()); 
        return managed_grt;
      }

      void initialize()
      { inner->initialize(true); }

      void perform_idle_tasks()
      { inner->perform_idle_tasks(); }

      void rescan_modules()
      { inner->rescan_modules(); }

      // shell
      GrtShell^ get_shell()
      { 
        if(managed_shell == nullptr)
          managed_shell= gcnew GrtShell(inner->get_shell());
        return managed_shell; 
      }

      void terminate() { inner->terminate(); };
      bool terminated() { return inner->terminated(); };
      void resetTermination() { inner->reset_termination(); };

      bool try_soft_lock_globals_tree() { return inner->try_soft_lock_globals_tree(); };
      void soft_lock_globals_tree() { inner->soft_lock_globals_tree(); };
      void soft_unlock_globals_tree() { inner->soft_unlock_globals_tree(); };
      bool is_globals_tree_locked() { return inner->is_globals_tree_locked(); };

    };

  } // namespace Grt
} // namespace MySQL

#endif // __GRT_MANAGER_H__