/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __STOREDNOTEEDITOR_H__
#define __STOREDNOTEEDITOR_H__

#include "wb_editor_storednote.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"

#include "mforms/code_editor.h"
#include "base/string_utilities.h"

#pragma make_public(::StoredNoteEditorBE)

using namespace MySQL::Grt;
using namespace System;
using namespace System::Collections::Generic;

namespace MySQL {
namespace Grt {

public delegate void TextChangeDelegate(int line, int linesAdded);

public ref class StoredNoteEditorBE : public BaseEditorWrapper
{
private:
  bool _is_script;

protected:
  StoredNoteEditorBE(::StoredNoteEditorBE *inn)
    : BaseEditorWrapper(inn)
  {}

public:

  StoredNoteEditorBE::StoredNoteEditorBE(MySQL::Grt::GrtManager^ grtm, MySQL::Grt::GrtValue^ arglist)
  : BaseEditorWrapper(
      new ::StoredNoteEditorBE(grtm->get_unmanaged_object(), 
      GrtStoredNoteRef::cast_from(grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0))
          )
      )
  {
    grt::ValueRef value = grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0);
    _is_script = grt::ObjectRef::cast_from(value).is_instance("db.Script");
  }

  StoredNoteEditorBE::~StoredNoteEditorBE()
  {
  }

  ::StoredNoteEditorBE *get_unmanaged_object()
  { return static_cast<::StoredNoteEditorBE *>(inner); }

  void commit_changes()
  {
    get_unmanaged_object()->commit_changes();
  }

  void load_text()
  {
    get_unmanaged_object()->load_text();
  }

  void set_name(String ^name)
  {
    get_unmanaged_object()->set_name(NativeToCppString(name));
  }

  String^ get_name()
  {
    return CppStringToNative(get_unmanaged_object()->get_name());
  }

  bool is_sql_script()
  {
    return _is_script;
  }

};

} // namespace Grt
} // namespace MySQL

#endif // __STOREDNOTEEDITOR_H__