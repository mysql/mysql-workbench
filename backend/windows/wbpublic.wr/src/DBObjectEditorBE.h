/* 
 * Copyright (c) 2010, 2013 Oracle and/or its affiliates. All rights reserved.
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

#ifndef __DB_OBJECT_EDITOR_H__
#define __DB_OBJECT_EDITOR_H__

#include "grtdb/editor_dbobject.h"
#include "Grt.h"
#include "GrtTemplates.h"
#include "BaseEditorWrapper.h"
#include "DelegateWrapper.h"


#ifdef _MSC_VER
using namespace MySQL::Grt;
using namespace System;
using namespace System::Collections::Generic;
#endif

namespace MySQL {
namespace Grt {
namespace Db {

public ref class DBObjectEditorBE : public BaseEditorWrapper
{
protected:

  DBObjectEditorBE(::bec::DBObjectEditorBE *inn)
    : BaseEditorWrapper(inn)
  {}

public:
  ::bec::DBObjectEditorBE *get_unmanaged_object()
  { return static_cast<::bec::DBObjectEditorBE *>(inner); }

  String^ get_name()
  { return CppStringToNative(get_unmanaged_object()->get_name()); }
  
  void set_name(String^ name)
  { get_unmanaged_object()->set_name(NativeToCppString(name)); }

  String^ get_comment()
  { return CppStringToNative(get_unmanaged_object()->get_comment()); }

  void set_comment(String^ descr)
  { get_unmanaged_object()->set_comment(NativeToCppString(descr)); }

  String^ get_sql()
  { return CppStringToNative(get_unmanaged_object()->get_sql()); }

  bool is_sql_commented()
  { return get_unmanaged_object()->is_sql_commented(); }

  void set_sql_commented(bool flag)
  {
    get_unmanaged_object()->set_sql_commented(flag);
  }

  // Helpers
  GrtValue^ get_catalog()
  {
    return gcnew GrtValue(static_cast<::bec::DBObjectEditorBE *>(inner)->get_catalog());
  }

  GrtValue^ get_schema()
  {
    return gcnew GrtValue(static_cast<::bec::DBObjectEditorBE *>(inner)->get_schema());
  }

  String^ get_schema_name()
  { return CppStringToNative(get_unmanaged_object()->get_schema_name()); }

  List<String^>^ get_all_table_names()
  {
    return CppStringListToNative(
      static_cast<::bec::DBObjectEditorBE *>(inner)->get_all_table_names());
  }

  List<String^>^ get_schema_table_names()
  {
    return CppStringListToNative(
      static_cast<::bec::DBObjectEditorBE *>(inner)->get_schema_table_names());
  }

  List<String^>^ get_table_column_names(String ^table_name)
  {
    List<String^>^ val= CppStringListToNative(
      static_cast<::bec::DBObjectEditorBE *>(inner)->get_table_column_names(NativeToCppString(table_name)));
    return val;
  }

  List<String^>^ get_table_column_names(GrtValue^ table)
  {
    return CppStringListToNative(
      static_cast<::bec::DBObjectEditorBE *>(inner)->get_table_column_names(db_TableRef::cast_from(table->get_unmanaged_object())));
  }

  List<String^>^ get_charset_collation_list()
  {
    return CppStringListToNative(
      static_cast<::bec::DBObjectEditorBE *>(inner)->get_charset_collation_list());
  }

  delegate void VoidStringListDelegate(List<String^>^);
  void set_sql_parser_log_cb(VoidStringListDelegate^ dt)
  {
    on_parse_sql_log_delegate= dt;
    on_parse_sql_log_wrapper_delegate= 
      gcnew VoidStringVectorWrapperDelegate(this, &DBObjectEditorBE::on_parse_sql_log_wrapper);
    IntPtr ip = Marshal::GetFunctionPointerForDelegate(on_parse_sql_log_wrapper_delegate);
    VOID_STRINGVECTOR_CB cb = static_cast<VOID_STRINGVECTOR_CB>(ip.ToPointer());
    get_unmanaged_object()->set_sql_parser_log_cb(cb);
  }

  typedef DelegateSlot4<
    int, int,
    int, int,
    int, int,
    int, int,
    std::string, String^> ParseErrorDelegate;

  void set_sql_parser_err_cb(ParseErrorDelegate::ManagedDelegate ^cb)
  {
    on_parse_err_delegate= gcnew ParseErrorDelegate(cb);
    get_unmanaged_object()->set_sql_parser_err_cb(on_parse_err_delegate->get_slot());
  }

private:
  VoidStringListDelegate^ on_parse_sql_log_delegate;
  ParseErrorDelegate^ on_parse_err_delegate;

  delegate void VoidStringVectorWrapperDelegate(std::vector<std::string>&);
  VoidStringVectorWrapperDelegate^ on_parse_sql_log_wrapper_delegate;

  typedef void (*DBObjectEditorBE::VOID_STRINGVECTOR_CB)(const std::vector<std::string>&);
  void on_parse_sql_log_wrapper(std::vector<std::string>& stringvec)
  { on_parse_sql_log_delegate(CppStringListToNative(stringvec)); }
};

} // namespace Db
} // namespace Grt
} // namespace MySQL

#endif // __DB_OBJECT_EDITOR_H__
