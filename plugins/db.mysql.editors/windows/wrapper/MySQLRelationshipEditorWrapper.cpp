/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "MySQLRelationshipEditorWrapper.h"

#include "ConvUtils.h"

using namespace MySQL::Grt::Db;

//--------------------------------------------------------------------------------------------------

MySQLRelationshipEditorWrapper::MySQLRelationshipEditorWrapper(MySQL::Grt::GrtValue ^ arglist)
  : BaseEditorWrapper(new RelationshipEditorBE(workbench_physical_ConnectionRef::cast_from(
      grt::BaseListRef::cast_from(arglist->get_unmanaged_object()).get(0)))) {
}

//--------------------------------------------------------------------------------------------------

MySQLRelationshipEditorWrapper::~MySQLRelationshipEditorWrapper() {
  delete inner; // We created it.
}

//--------------------------------------------------------------------------------------------------

RelationshipEditorBE *MySQLRelationshipEditorWrapper::get_unmanaged_object() {
  return static_cast<::RelationshipEditorBE *>(inner);
}

//--------------------------------------------------------------------------------------------------

void MySQLRelationshipEditorWrapper::set_caption(String ^ caption) {
  get_unmanaged_object()->set_caption(NativeToCppString(caption));
}

//--------------------------------------------------------------------------------------------------

String ^ MySQLRelationshipEditorWrapper::get_caption() {
  return CppStringToNative(get_unmanaged_object()->get_caption());
}

//--------------------------------------------------------------------------------------------------

String ^ MySQLRelationshipEditorWrapper::get_caption_long() {
  return CppStringToNative(get_unmanaged_object()->get_caption_long());
}

//--------------------------------------------------------------------------------------------------

void MySQLRelationshipEditorWrapper::set_extra_caption(String ^ caption) {
  get_unmanaged_object()->set_extra_caption(NativeToCppString(caption));
}

//--------------------------------------------------------------------------------------------------

String ^ MySQLRelationshipEditorWrapper::get_extra_caption() {
  return CppStringToNative(get_unmanaged_object()->get_extra_caption());
}

//--------------------------------------------------------------------------------------------------

String ^ MySQLRelationshipEditorWrapper::get_extra_caption_long() {
  return CppStringToNative(get_unmanaged_object()->get_extra_caption_long());
}

//--------------------------------------------------------------------------------------------------

String ^ MySQLRelationshipEditorWrapper::get_right_table_name() {
  return CppStringToNative(get_unmanaged_object()->get_right_table_name());
}

//--------------------------------------------------------------------------------------------------

String ^ MySQLRelationshipEditorWrapper::get_left_table_name() {
  return CppStringToNative(get_unmanaged_object()->get_left_table_name());
}

//--------------------------------------------------------------------------------------------------

String ^ MySQLRelationshipEditorWrapper::get_right_table_info() {
  return CppStringToNative(get_unmanaged_object()->get_right_table_info());
}

//--------------------------------------------------------------------------------------------------

String ^ MySQLRelationshipEditorWrapper::get_left_table_info() {
  return CppStringToNative(get_unmanaged_object()->get_left_table_info());
}

//--------------------------------------------------------------------------------------------------

String ^ MySQLRelationshipEditorWrapper::get_left_table_fk() {
  return CppStringToNative(get_unmanaged_object()->get_left_table_fk());
}

//--------------------------------------------------------------------------------------------------

void MySQLRelationshipEditorWrapper::set_left_mandatory(bool flag) {
  get_unmanaged_object()->set_left_mandatory(flag);
}

//--------------------------------------------------------------------------------------------------

bool MySQLRelationshipEditorWrapper::get_left_mandatory() {
  return get_unmanaged_object()->get_left_mandatory();
}

//--------------------------------------------------------------------------------------------------

void MySQLRelationshipEditorWrapper::set_right_mandatory(bool flag) {
  get_unmanaged_object()->set_right_mandatory(flag);
}

//--------------------------------------------------------------------------------------------------

bool MySQLRelationshipEditorWrapper::get_right_mandatory() {
  return get_unmanaged_object()->get_right_mandatory();
}

//--------------------------------------------------------------------------------------------------

void MySQLRelationshipEditorWrapper::set_to_many(bool flag) {
  get_unmanaged_object()->set_to_many(flag);
}

//--------------------------------------------------------------------------------------------------

bool MySQLRelationshipEditorWrapper::get_to_many() {
  return get_unmanaged_object()->get_to_many();
}

//--------------------------------------------------------------------------------------------------

void MySQLRelationshipEditorWrapper::set_comment(String ^ comment) {
  get_unmanaged_object()->set_comment(NativeToCppString(comment));
}

//--------------------------------------------------------------------------------------------------

String ^ MySQLRelationshipEditorWrapper::get_comment() {
  return CppStringToNative(get_unmanaged_object()->get_comment());
}

//--------------------------------------------------------------------------------------------------

RelationshipVisibilityType MySQLRelationshipEditorWrapper::get_visibility() {
  return (RelationshipVisibilityType)get_unmanaged_object()->get_visibility();
}

//--------------------------------------------------------------------------------------------------

bool MySQLRelationshipEditorWrapper::get_is_identifying() {
  return get_unmanaged_object()->get_is_identifying();
}

//--------------------------------------------------------------------------------------------------

void MySQLRelationshipEditorWrapper::set_visibility(RelationshipVisibilityType v) {
  get_unmanaged_object()->set_visibility((RelationshipEditorBE::VisibilityType)v);
}

//--------------------------------------------------------------------------------------------------

void MySQLRelationshipEditorWrapper::open_editor_for_left_table() {
  get_unmanaged_object()->open_editor_for_left_table();
}

//--------------------------------------------------------------------------------------------------

void MySQLRelationshipEditorWrapper::open_editor_for_right_table() {
  get_unmanaged_object()->open_editor_for_right_table();
}

//--------------------------------------------------------------------------------------------------

void MySQLRelationshipEditorWrapper::set_is_identifying(bool identifying) {
  get_unmanaged_object()->set_is_identifying(identifying);
}

//--------------------------------------------------------------------------------------------------
