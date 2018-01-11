/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grts/structs.db.query.h"
#include "grtpp_util.h"
#include "db_query_Resultset.h"

class WBPUBLICBACKEND_PUBLIC_FUNC db_query_EditableResultset::ImplData : public WBRecordsetResultset {
public:
  ImplData(db_query_EditableResultsetRef aself, std::shared_ptr<Recordset> rset) : WBRecordsetResultset(aself, rset) {
  }
};

db_query_EditableResultsetRef WBPUBLICBACKEND_PUBLIC_FUNC grtwrap_editablerecordset(GrtObjectRef owner,
                                                                                    Recordset::Ref rset) {
  db_query_EditableResultsetRef object(grt::Initialized);

  db_query_EditableResultset::ImplData *data = new db_query_EditableResultset::ImplData(object, rset);

  object->owner(owner);

  object->set_data(data);

  return object;
}

void db_query_EditableResultset::init() {
}

db_query_EditableResultset::~db_query_EditableResultset() {
  // data is shared and deleted by parent class
  // delete _data;
}

void db_query_EditableResultset::set_data(ImplData *data) {
  _data = data;
  db_query_Resultset::set_data(data);
}

grt::IntegerRef db_query_EditableResultset::setFieldNull(ssize_t column) {
  if (_data && column >= 0 && (size_t)column < _data->recordset->get_column_count() &&
      _data->recordset->set_field_null(bec::NodeId(_data->currentRow()), column))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::setFieldNullByName(const std::string &column) {
  if (_data && _data->column_by_name.find(column) != _data->column_by_name.end() &&
      _data->recordset->set_field_null(bec::NodeId(_data->currentRow()), _data->column_by_name[column]))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::setFloatFieldValue(ssize_t column, double value) {
  if (_data && column >= 0 && (size_t)column < _data->recordset->get_column_count() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), column, value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::setFloatFieldValueByName(const std::string &column, double value) {
  if (_data && _data->column_by_name.find(column) != _data->column_by_name.end() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), _data->column_by_name[column], value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::setIntFieldValue(ssize_t column, ssize_t value) {
  if (_data && column >= 0 && (size_t)column < _data->recordset->get_column_count() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), column, value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::setIntFieldValueByName(const std::string &column, ssize_t value) {
  if (_data && _data->column_by_name.find(column) != _data->column_by_name.end() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), _data->column_by_name[column], value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::setStringFieldValue(ssize_t column, const std::string &value) {
  if (_data && column >= 0 && (size_t)column < _data->recordset->get_column_count() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), column, value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::setStringFieldValueByName(const std::string &column,
                                                                      const std::string &value) {
  if (_data && _data->column_by_name.find(column) != _data->column_by_name.end() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), _data->column_by_name[column], value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::applyChanges() {
  if (_data)
    _data->recordset->apply_changes_();

  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::revertChanges() {
  if (_data) {
    _data->recordset->rollback();

    if (_data->cursor >= _data->recordset->count())
      _data->cursor = _data->recordset->count() - 1;
  }
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::addNewRow() {
  if (_data) {
    _data->cursor = _data->recordset->count() - 1;
    if (_data->recordset->rows_changed)
      _data->recordset->rows_changed();
    return grt::IntegerRef((grt::IntegerRef::storage_type)_data->cursor);
  }
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::deleteRow(ssize_t row) {
  return grt::IntegerRef(_data ? _data->recordset->delete_node(row) : 0);
}

grt::IntegerRef db_query_EditableResultset::loadFieldValueFromFile(ssize_t column, const std::string &file) {
  if (_data && column >= 0 && (size_t)column < _data->recordset->get_column_count()) {
    _data->recordset->load_from_file(bec::NodeId(_data->cursor), column, file);
    return grt::IntegerRef(1);
  }
  return grt::IntegerRef(0);
}
