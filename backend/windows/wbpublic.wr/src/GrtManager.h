/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "ModelWrappers.h"
#include "GrtShell.h"
#include "GrtValueInspector.h"

namespace MySQL {
  namespace Grt {

  public
    ref class GrtManager {
    public:
      delegate bool BoolStringStringFloatDelegate(String ^ str1, String ^ str2, float f);

    private:
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate bool BoolStringStringFloatWrapperDelegate(
        const std::string& str1, const std::string& str2, float f);
      typedef bool (*GrtManager::BOOL_STRING_STRING_FLOAT_HANDLER_CB)(const std::string& str1, const std::string& str2,
                                                                      float f);

      GrtShell ^ managed_shell;
      GRT ^ managed_grt;
      BoolStringStringFloatDelegate ^ progress_callback_delegate;
      BoolStringStringFloatWrapperDelegate ^ progress_callback_wrapper_delegate;

      bool progress_callback_wrapper(const std::string& str1, const std::string& str2, float f) {
        return progress_callback_delegate(CppStringToNative(str1), CppStringToNative(str2), f);
      }

    public:
      GrtManager(bool threaded, bool verbose) : managed_shell(nullptr), managed_grt(nullptr) {
      }

      explicit GrtManager() : managed_grt(nullptr) {
      }

      virtual ~GrtManager() { /*delete inner;*/
      }

      /*::bec::GRTManager *get_unmanaged_object()
      {
        &::bec::GRTManager::get();
      }*/

      String ^
        get_app_option_string(String ^ option_name) {
          return CppStringToNative(::bec::GRTManager::get()->get_app_option_string(NativeToCppString(option_name)));
        }

        Font
        ^
        get_font_option(String ^ option_name) {
          String ^ font_string =
            CppStringToNative(::bec::GRTManager::get()->get_app_option_string(NativeToCppString(option_name)));
          return MySQL::Utilities::ControlUtilities::GetFont(font_string);
        }

        void set_module_extensions(List<String ^> ^ extensions) {
        ::bec::GRTManager::get()->set_module_extensions(NativeToCppStringList2(extensions));
      }

      void set_search_paths(String ^ module_sp, String ^ struct_sp, String ^ libs_sp) {
        ::bec::GRTManager::get()->set_search_paths(NativeToCppString(module_sp), NativeToCppString(struct_sp),
                                                   NativeToCppString(libs_sp));
      }

      GRT ^
        get_grt() {
          if (managed_grt == nullptr)
            managed_grt = gcnew GRT;
          return managed_grt;
        }

        void initialize() {
        ::bec::GRTManager::get()->initialize(true);
      }

      void perform_idle_tasks() {
        ::bec::GRTManager::get()->perform_idle_tasks();
      }

      void rescan_modules() {
        ::bec::GRTManager::get()->rescan_modules();
      }

      // shell
      GrtShell ^
        get_shell() {
          if (managed_shell == nullptr)
            managed_shell = gcnew GrtShell(::bec::GRTManager::get()->get_shell());
          return managed_shell;
        }

        void terminate() {
        bec::GRTManager::get()->terminate();
      };
      bool terminated() {
        return ::bec::GRTManager::get()->terminated();
      };
      void resetTermination() {
        ::bec::GRTManager::get()->reset_termination();
      };

      bool try_soft_lock_globals_tree() {
        return ::bec::GRTManager::get()->try_soft_lock_globals_tree();
      };
      void soft_lock_globals_tree() {
        ::bec::GRTManager::get()->soft_lock_globals_tree();
      };
      void soft_unlock_globals_tree() {
        ::bec::GRTManager::get()->soft_unlock_globals_tree();
      };
      bool is_globals_tree_locked() {
        return ::bec::GRTManager::get()->is_globals_tree_locked();
      };
    };

  } // namespace Grt
} // namespace MySQL