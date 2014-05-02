#include "stdafx.h"

#include <grts/structs.db.query.h>

#include <grtpp_util.h>

#include "db_query_Resultset.h"

//================================================================================
// db_query_EditableResultset


class WBPUBLICBACKEND_PUBLIC_FUNC db_query_EditableResultset::ImplData : public WBRecordsetResultset
{
public:
  ImplData(db_query_EditableResultsetRef aself, boost::shared_ptr<Recordset> rset)
  : WBRecordsetResultset(aself, rset)
  {
  }
};


db_query_EditableResultsetRef WBPUBLICBACKEND_PUBLIC_FUNC grtwrap_editablerecordset(GrtObjectRef owner, Recordset::Ref rset)
{
  db_query_EditableResultsetRef object(owner.get_grt());
  
  db_query_EditableResultset::ImplData *data= new db_query_EditableResultset::ImplData(object, rset);
  
  object->owner(owner);
  
  object->set_data(data);
  
  return object;  
}



void db_query_EditableResultset::init()
{
}

db_query_EditableResultset::~db_query_EditableResultset()
{
  // data is shared and deleted by parent class
  //delete _data;
}


void db_query_EditableResultset::set_data(ImplData *data)
{
  _data= data;
  db_query_Resultset::set_data(data);
}

grt::IntegerRef db_query_EditableResultset::setFieldNull(long column)
{
  if (_data && column >= 0 && column < _data->recordset->get_column_count() &&
      _data->recordset->set_field_null(bec::NodeId(_data->currentRow()), (int)column))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);  
}


grt::IntegerRef db_query_EditableResultset::setFieldNullByName(const std::string &column)
{
  if (_data && _data->column_by_name.find(column) != _data->column_by_name.end() &&
      _data->recordset->set_field_null(bec::NodeId(_data->currentRow()), _data->column_by_name[column]))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);  
}

grt::IntegerRef db_query_EditableResultset::setFloatFieldValue(long column, double value)
{
  if (_data && column >= 0 && column < _data->recordset->get_column_count() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), (int)column, value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}


grt::IntegerRef db_query_EditableResultset::setFloatFieldValueByName(const std::string &column, double value)
{
  if (_data && _data->column_by_name.find(column) != _data->column_by_name.end() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), _data->column_by_name[column], value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}


grt::IntegerRef db_query_EditableResultset::setIntFieldValue(long column, long value)
{
  if (_data && column >= 0 && column < _data->recordset->get_column_count() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), (int)column, (long long)value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}


grt::IntegerRef db_query_EditableResultset::setIntFieldValueByName(const std::string &column, long value)
{
  if (_data && _data->column_by_name.find(column) != _data->column_by_name.end() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), _data->column_by_name[column], (long long)value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);  
}


grt::IntegerRef db_query_EditableResultset::setStringFieldValue(long column, const std::string &value)
{
  if (_data && column >= 0 && column < _data->recordset->get_column_count() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), (int)column, value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);
}


grt::IntegerRef db_query_EditableResultset::setStringFieldValueByName(const std::string &column, const std::string &value)
{
  if (_data && _data->column_by_name.find(column) != _data->column_by_name.end() &&
      _data->recordset->set_field(bec::NodeId(_data->currentRow()), _data->column_by_name[column], value))
    return grt::IntegerRef(1);
  return grt::IntegerRef(0);  
}


grt::IntegerRef db_query_EditableResultset::applyChanges()
{
  if (_data)
    _data->recordset->apply_changes_();
  
  return grt::IntegerRef(0);
}


grt::IntegerRef db_query_EditableResultset::revertChanges()
{
  if (_data)
  {
    _data->recordset->rollback();
    
    if ((int) _data->cursor >= _data->recordset->count())
      _data->cursor= _data->recordset->count()-1;
  }
  return grt::IntegerRef(0);
}

grt::IntegerRef db_query_EditableResultset::addNewRow()
{
  if (_data)
  {
    _data->cursor= _data->recordset->count()-1;
  
    return grt::IntegerRef((grt::IntegerRef::storage_type)_data->cursor);
  }
  return grt::IntegerRef(0);
}


grt::IntegerRef db_query_EditableResultset::deleteRow(long row)
{
  return grt::IntegerRef(_data ? _data->recordset->delete_node(row) : 0);
}

grt::IntegerRef db_query_EditableResultset::loadFieldValueFromFile(long column, const std::string &file)
{
  if (_data && column >= 0 && column < _data->recordset->get_column_count())
  {
    _data->recordset->load_from_file(bec::NodeId(_data->cursor), (int)column, file);
    return grt::IntegerRef(1);
  }
  return grt::IntegerRef(0);
}

