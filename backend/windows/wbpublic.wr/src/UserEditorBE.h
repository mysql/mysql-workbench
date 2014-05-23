/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __USER_EDITOR_H__
#define __USER_EDITOR_H__

#include "DBObjectEditorWrapper.h"
#include "RoleTreeBE.h"
#include "GrtTemplates.h"
#include "grtdb/editor_user.h"

namespace MySQL {
namespace Grt {
namespace Db {

public ref class UserEditorBE : public BaseEditorWrapper
{
protected:
  UserEditorBE(::bec::UserEditorBE *inn)
    : BaseEditorWrapper(inn)
  {}

public:
  UserEditorBE::UserEditorBE(MySQL::Grt::GrtManager^ grtm, MySQL::Grt::GrtValue^ arglist)
  : BaseEditorWrapper(
        new bec::UserEditorBE(grtm->get_unmanaged_object(), 
        db_UserRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0))
        )
      )
  {
  }

  ::bec::UserEditorBE *get_unmanaged_object()
  { return static_cast<::bec::UserEditorBE *>(inner); }

  void set_name(String ^name)
  { get_unmanaged_object()->set_name(NativeToCppString(name)); }

  String^ get_name()
  { return CppStringToNative(get_unmanaged_object()->get_name()); }

  void set_password(String^ pass)
  { get_unmanaged_object()->set_password(NativeToCppString(pass)); }

  String^ get_password()
  { return gcnew String(get_unmanaged_object()->get_password().c_str()); }

  void set_comment(String^ comment)
  { get_unmanaged_object()->set_comment(NativeToCppString(comment)); }

  String^ get_comment()
  { return CppStringToNative(get_unmanaged_object()->get_comment()); }


  RoleTreeBE^ get_role_tree()
  {
    return gcnew RoleTreeBE(get_unmanaged_object()->get_role_tree());
  }

  void add_role(String^ pass)
  { get_unmanaged_object()->add_role(NativeToCppString(pass)); }

  void remove_role(String^ pass)
  { get_unmanaged_object()->remove_role(NativeToCppString(pass)); }

  List<String^>^ get_roles()
  {
    return CppStringListToNative(get_unmanaged_object()->get_roles());
  }
};

} // namespace Db
} // namespace Grt
} // namespace MySQL

#endif // __USER_EDITOR_H__