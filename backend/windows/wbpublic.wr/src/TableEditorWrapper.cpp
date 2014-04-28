/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "ConvUtils.h"
#include "Grt.h"
#include "GrtTemplates.h"
#include "DelegateWrapper.h"
#include "GrtManager.h"

#include "ActionList.h"
#include "GrtThreadedTaskWrapper.h"
#include "TableEditorWrapper.h"
#include "mforms/view.h"

namespace MySQL {
namespace Grt {
namespace Db {

TableColumnsListWrapper::TableColumnsListWrapper(bec::TableColumnsListBE *inn)
  : ListModelWrapper(inn)
{}

void TableColumnsListWrapper::reorder_many(List<int> ^rows, int nindex)
{
  std::vector<size_t> nlist;

  for (int i= 0; i < rows->Count; i++)
    nlist.push_back(rows[i]);

  get_unmanaged_object()->reorder_many(nlist, nindex);
}

bool TableColumnsListWrapper::get_row(NodeIdWrapper^ node,
             [Out] String^ %name,
             [Out] String^ %type,
             [Out] bool^ %ispk,
             [Out] bool^ %notnull,
             [Out] bool^ %unique,
             [Out] bool^ %isbinary,
             [Out] bool^ %isunsigned,
             [Out] bool^ %iszerofill,
             [Out] String^ %flags,
             [Out] String^ %defvalue,
             [Out] String^ %charset,
             [Out] String^ %collation,
             [Out] String^ %comment)
{
  std::string name_str= NativeToCppString(name);
  std::string type_str= NativeToCppString(type);
  std::string flags_str= NativeToCppString(flags);
  std::string defvalue_str= NativeToCppString(defvalue);
  std::string charset_str= NativeToCppString(charset);
  std::string collation_str= NativeToCppString(collation);
  std::string comment_str= NativeToCppString(comment);

  bool local_ispk;
  bool local_notnull;
  bool local_unique;
  bool local_isbinary;
  bool local_isunsigned;
  bool local_iszerofill;

  bool retval= get_unmanaged_object()->get_row(*node->get_unmanaged_object(), 
    name_str, 
    type_str, 
    local_ispk, 
    local_notnull, 
    local_unique, 
    local_isbinary,
    local_isunsigned,
    local_iszerofill,
    flags_str, 
    defvalue_str, 
    charset_str, 
    collation_str,
    comment_str);

  *ispk= local_ispk;
  *notnull= local_notnull;
  *unique= local_unique;
  *isbinary= local_isbinary;
  *isunsigned= local_isunsigned;
  *iszerofill= local_iszerofill;

  name= CppStringToNative(name_str);
  type= CppStringToNative(type_str);
  flags= CppStringToNative(flags_str);
  defvalue= CppStringToNative(defvalue_str);
  charset= CppStringToNative(charset_str);
  collation= CppStringToNative(collation_str);
  comment= CppStringToNative(comment_str);

  return retval;
}

IndexColumnsListWrapper::IndexColumnsListWrapper(IndexListWrapper^ owner)
  : ListModelWrapper(new bec::IndexColumnsListBE(owner->get_unmanaged_object()))
{}

IndexColumnsListWrapper::IndexColumnsListWrapper(bec::IndexColumnsListBE *inn)
  : ListModelWrapper(inn)
{}

void IndexColumnsListWrapper::set_column_enabled(NodeIdWrapper^ node, bool flag)
{
 get_unmanaged_object()->set_column_enabled(*node->get_unmanaged_object(), flag); 
}

bool IndexColumnsListWrapper::get_column_enabled(NodeIdWrapper^ node)
{
  return get_unmanaged_object()->get_column_enabled(*node->get_unmanaged_object()); 
}

int IndexColumnsListWrapper::get_max_order_index()
{
  return (int)get_unmanaged_object()->get_max_order_index();
}


IndexListWrapper::IndexListWrapper(TableEditorWrapper^ owner)
  : ListModelWrapper(new bec::IndexListBE(owner->get_unmanaged_object()))
{}

IndexListWrapper::IndexListWrapper(bec::IndexListBE *inn)
  : ListModelWrapper(inn)
{}
  
IndexColumnsListWrapper^ IndexListWrapper::get_columns() 
{ return gcnew IndexColumnsListWrapper(get_unmanaged_object()->get_columns()); }

void IndexListWrapper::select_index(NodeIdWrapper^ node)
{ get_unmanaged_object()->select_index(*node->get_unmanaged_object()); }

FKConstraintColumnsListWrapper::FKConstraintColumnsListWrapper(FKConstraintListWrapper^ owner)
  : ListModelWrapper(new bec::FKConstraintColumnsListBE(owner->get_unmanaged_object()))
{}

FKConstraintColumnsListWrapper::FKConstraintColumnsListWrapper(bec::FKConstraintColumnsListBE *inn)
  : ListModelWrapper(inn)
{}

List<String^>^ FKConstraintColumnsListWrapper::get_ref_columns_list(NodeIdWrapper^ node, bool filtered)
{
  return CppStringListToNative(
    static_cast<bec::FKConstraintColumnsListBE *>(inner)->get_ref_columns_list(*node->get_unmanaged_object(), filtered));
}

bool FKConstraintColumnsListWrapper::set_column_is_fk(NodeIdWrapper^ node, bool flag)
{
  return get_unmanaged_object()->set_column_is_fk(*node->get_unmanaged_object(), flag); 
}

bool FKConstraintColumnsListWrapper::get_column_is_fk(NodeIdWrapper^ node)
{
  return get_unmanaged_object()->get_column_is_fk(*node->get_unmanaged_object()); 
}

FKConstraintListWrapper::FKConstraintListWrapper(TableEditorWrapper^ owner)
  : ListModelWrapper(new bec::FKConstraintListBE(owner->get_unmanaged_object()))
{}

FKConstraintListWrapper::FKConstraintListWrapper(bec::FKConstraintListBE *inn)
  : ListModelWrapper(inn)
{}

/*
NodeId^ FKConstraintListWrapper::add_column(String^ column_name)
{ return gcnew NodeId(&get_unmanaged_object()->add_column(NativeToCppString(column_name))); }*/

void FKConstraintListWrapper::select_fk(NodeIdWrapper^ node)
{ get_unmanaged_object()->select_fk(*node->get_unmanaged_object()); }


FKConstraintColumnsListWrapper^ FKConstraintListWrapper::get_columns()
{ return gcnew FKConstraintColumnsListWrapper(get_unmanaged_object()->get_columns()); }



IndexListWrapper^ TableEditorWrapper::get_indexes() 
{ return gcnew IndexListWrapper(get_unmanaged_object()->get_indexes()); }

FKConstraintListWrapper^ TableEditorWrapper::get_fks() 
{ return gcnew FKConstraintListWrapper(get_unmanaged_object()->get_fks()); }


Control ^TableEditorWrapper::get_inserts_panel(Control ^grid)
{
  mforms::View *view = get_unmanaged_object()->create_inserts_panel(MySQL::Forms::Native::wrapper_for_control(grid));

  return dynamic_cast<Control ^>(MySQL::Forms::ObjectMapper::GetManagedComponent(view));
}

// table options
//...

// column editing
NodeIdWrapper^ TableEditorWrapper::add_column(String^ name)
{ return gcnew NodeIdWrapper(&get_unmanaged_object()->add_column(NativeToCppString(name))); }

void TableEditorWrapper::remove_column(NodeIdWrapper^ node)
{ get_unmanaged_object()->remove_column(*node->get_unmanaged_object()); }

//db_Column get_column_with_name(const std::string &name);

// fk editing
NodeIdWrapper^ TableEditorWrapper::add_fk(String^ name)
{
  return gcnew NodeIdWrapper(&get_unmanaged_object()->add_fk(NativeToCppString(name)));
}

void TableEditorWrapper::remove_fk(NodeIdWrapper^ node)
{ get_unmanaged_object()->remove_fk(*node->get_unmanaged_object()); }

NodeIdWrapper^ TableEditorWrapper::add_fk_with_columns(List<NodeIdWrapper ^> ^columns)
{
  std::vector<bec::NodeId> node_vec= ObjectListToCppVector<NodeIdWrapper, bec::NodeId>(columns);
  return gcnew NodeIdWrapper(&get_unmanaged_object()->add_fk_with_columns(node_vec));
}

// index editing
NodeIdWrapper^ TableEditorWrapper::add_index(String^ name)
{ return gcnew NodeIdWrapper(&get_unmanaged_object()->add_index(NativeToCppString(name))); }

void TableEditorWrapper::remove_index(NodeIdWrapper^ node)
{ get_unmanaged_object()->remove_index(*node->get_unmanaged_object(), false); }

NodeIdWrapper^ TableEditorWrapper::add_index_with_columns(List<NodeIdWrapper ^> ^columns)
{
  std::vector<bec::NodeId> node_vec= ObjectListToCppVector<NodeIdWrapper, bec::NodeId>(columns);
  return gcnew NodeIdWrapper(&get_unmanaged_object()->add_index_with_columns(node_vec));
}

List<String^>^ TableEditorWrapper::get_index_types()
{  
  return CppStringListToNative(
    static_cast<bec::TableEditorBE *>(inner)->get_index_types());
}


} // namespace Db
} // namespace Grt
} // namespace MySQL
