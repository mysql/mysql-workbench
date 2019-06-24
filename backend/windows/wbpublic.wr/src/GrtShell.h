/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __GRT_SHELL_H__
#define __GRT_SHELL_H__

namespace MySQL {
  namespace Grt {

  public
    ref class GrtShell {
    public:
      delegate void VoidStringDelegate(String ^ str);

    private:
      [UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)] delegate void VoidStringWrapperDelegate(
        const std::string& str);
      typedef void (*GrtShell::VOID_STRING_HANDLER_CB)(const std::string& str);

      ::bec::ShellBE* inner;

      VoidStringDelegate ^ ready_handler_delegate;
      VoidStringWrapperDelegate ^ ready_handler_wrapper_delegate;

      void ready_handler_wrapper(const std::string& str) {
        ready_handler_delegate(CppStringToNative(str));
      }

      VoidStringDelegate ^ output_handler_delegate;
      VoidStringWrapperDelegate ^ output_handler_wrapper_delegate;

      void output_handler_wrapper(const std::string& str) {
        output_handler_delegate(CppStringToNative(str));
      }

    public:
      // GrtShell(GRT^ grt, GRTDispatcher *dispatcher);

      GrtShell(::bec::ShellBE* inn) : inner(inn) {
      }

      ~GrtShell() {
      }

      void set_save_directory(String ^ path) {
        inner->set_save_directory(NativeToCppString(path));
      }

      void start() {
        inner->start();
      }

      void process_line_async(String ^ line) {
        inner->process_line_async(NativeToCppString(line));
      }

      bool previous_history_line(String ^ current_line, [Out] String ^ % line) {
        std::string s1;
        bool retval = inner->previous_history_line(NativeToCppString(current_line), s1);
        line = CppStringToNative(s1);
        return retval;
      }

      bool next_history_line([Out] String ^ % line) {
        std::string s1;
        bool retval = inner->next_history_line(s1);
        line = CppStringToNative(s1);
        return retval;
      }

      void reset_history_position() {
        inner->reset_history_position();
      }

      void write_line(String ^ line) {
        inner->write_line(NativeToCppString(line));
      }

      void write(String ^ text) {
        inner->write_line(NativeToCppString(text));
      }

      // void writef(const char *fmt, ...);

      void set_ready_handler(VoidStringDelegate ^ dt) {
        ready_handler_delegate = dt;
        ready_handler_wrapper_delegate = gcnew VoidStringWrapperDelegate(this, &GrtShell::ready_handler_wrapper);
        IntPtr ip = Marshal::GetFunctionPointerForDelegate(ready_handler_wrapper_delegate);
        VOID_STRING_HANDLER_CB cb = static_cast<VOID_STRING_HANDLER_CB>(ip.ToPointer());
        inner->set_ready_handler(cb);
      }

      void set_saves_history(bool flag) {
        inner->set_saves_history(flag);
      }

      String ^ get_snippet_data() { return CppStringToNative(inner->get_snippet_data()); }

        void set_snippet_data(String ^ data) {
        inner->set_snippet_data(NativeToCppString(data));
      }

      void store_history() {
        inner->store_state();
      }

      void restore_history() {
        inner->restore_state();
      }

      List<String ^> ^ get_grt_tree_bookmarks() { return CppStringListToNative(inner->get_grt_tree_bookmarks()); }

      // GList *complete_word(const std::string &prefix, std::string &nprefix);
    };

  } // namespace Grt
} // namespace MySQL

#endif // __GRT_SHELL_H__
