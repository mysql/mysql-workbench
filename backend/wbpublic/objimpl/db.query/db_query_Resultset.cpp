/* 
 * Copyright (c) 2010, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <grts/structs.db.query.h>
#include <grtpp_util.h>
#include "sqlide/recordset_be.h"
#include "db_query_Resultset.h"
#include <my_config.h>
#include "copytable.h"

//================================================================================
// db_query_Resultset
db_query_Resultset::ImplData::ImplData(db_query_ResultsetRef aself)
: self(dynamic_cast<db_query_Resultset*>(aself.valueptr()))
{
}


db_query_Resultset::ImplData::~ImplData()
{
}

//================================================================================


WBRecordsetResultset::WBRecordsetResultset(db_query_ResultsetRef aself, boost::shared_ptr<Recordset> rset)
: db_query_Resultset::ImplData(aself), cursor(0), recordset(rset)
{
  const size_t last_column = recordset->get_column_count();
  for (size_t i = 0; i < last_column; i++)
  {
    column_by_name[recordset->get_column_caption(i)]= i;
    
    std::string type;
    switch (recordset->get_column_type(i))
    {
      case bec::GridModel::UnknownType: type= "unknown"; break;
      case bec::GridModel::StringType: type= "string"; break;
      case bec::GridModel::NumericType: type= "numeric"; break;
      case bec::GridModel::FloatType: type= "float"; break;
      case bec::GridModel::DatetimeType: type= "datetime"; break;
      case bec::GridModel::BlobType: type= "blob"; break;
    }
    
    db_query_ResultsetColumnRef column(aself->get_grt());
    
    column->owner(aself);
    column->name(recordset->get_column_caption(i));
    column->columnType(type);
    
    self->columns().insert(column);
  }
}

grt::StringRef WBRecordsetResultset::sql() const
{
  return grt::StringRef(recordset->generator_query());
}

grt::IntegerRef WBRecordsetResultset::currentRow() const
{
  return grt::IntegerRef((long)cursor);
}

grt::IntegerRef WBRecordsetResultset::rowCount() const
{
  return grt::IntegerRef(recordset->count());
}

grt::DoubleRef WBRecordsetResultset::floatFieldValue(ssize_t column)
{
  double value;
  if (column >= 0 && (size_t)column < recordset->get_column_count())
  {
    if (recordset->get_field(cursor, column, value))
      return grt::DoubleRef(value);
  }
  else
    throw std::invalid_argument(base::strfmt("invalid column %li for resultset", (long)column).c_str());
  return grt::DoubleRef(0.0);
}


grt::DoubleRef WBRecordsetResultset::floatFieldValueByName(const std::string &column)
{
  double value;
  if (column_by_name.find(column) != column_by_name.end())
  {
    if (recordset->get_field(cursor, column_by_name[column], value))
      return grt::DoubleRef(value);
  }
  throw std::invalid_argument(base::strfmt("invalid column %s for resultset", column.c_str()).c_str());
  return grt::DoubleRef(0.0);
}


grt::IntegerRef WBRecordsetResultset::goToFirstRow()
{
  cursor= 0;
  return grt::IntegerRef( cursor < recordset->count());
}


grt::IntegerRef WBRecordsetResultset::goToLastRow()
{
  if (recordset->count() > 0)
  {
    cursor= recordset->count()-1;
    return grt::IntegerRef(1);
  }
  return grt::IntegerRef(0);
}


grt::IntegerRef WBRecordsetResultset::goToRow(ssize_t row)
{
  if (row >= 0 && (size_t)row < recordset->count())
  {
    cursor= row;
    return grt::IntegerRef(1);
  }
  return grt::IntegerRef(0);
}


grt::IntegerRef WBRecordsetResultset::intFieldValue(ssize_t column)
{
  ssize_t value;
  if (column >= 0 && (size_t)column < recordset->get_column_count())
  {
    if (recordset->get_field(bec::NodeId(cursor), column, value))
      return grt::IntegerRef(value);
  }
  else
    throw std::invalid_argument(base::strfmt("invalid column %li for resultset", (long)column).c_str());
  return grt::IntegerRef(0);
}


grt::IntegerRef WBRecordsetResultset::intFieldValueByName(const std::string &column)
{
  ssize_t value;
  if (column_by_name.find(column) != column_by_name.end())
  {
    if (recordset->get_field(bec::NodeId(cursor), column_by_name[column], value))
      return grt::IntegerRef(value);
  }
  throw std::invalid_argument(base::strfmt("invalid column %s for resultset", column.c_str()).c_str());
  return grt::IntegerRef(0);
}


grt::IntegerRef WBRecordsetResultset::nextRow()
{
  if ( cursor < recordset->count() - 1)
  {
    ++cursor;
    return grt::IntegerRef(1);
  }
  return grt::IntegerRef(0);
}


grt::IntegerRef WBRecordsetResultset::previousRow()
{
  if (cursor > 0)
  {
    --cursor;
    return grt::IntegerRef(1);
  }
  return grt::IntegerRef(0);
}


void WBRecordsetResultset::refresh()
{
  recordset->refresh();
}


grt::StringRef WBRecordsetResultset::stringFieldValue(ssize_t column)
{
  std::string value;
  if (column >= 0 && (size_t)column < recordset->get_column_count())
  {
    if (recordset->get_field_repr_no_truncate(bec::NodeId(cursor), column, value))
      return grt::StringRef(value);
  }
  else
    throw std::invalid_argument(base::strfmt("invalid column %li for resultset", (long)column).c_str());
  return grt::StringRef(); // NULL
}


grt::StringRef WBRecordsetResultset::stringFieldValueByName(const std::string &column)
{
  std::string value;
  if (column_by_name.find(column) != column_by_name.end())
  {
    if (recordset->get_field_repr_no_truncate(bec::NodeId(cursor), column_by_name[column], value))
      return grt::StringRef(value);
  }
  throw std::invalid_argument(base::strfmt("invalid column %s for resultset", column.c_str()).c_str());
  return grt::StringRef(); // NULL
}

grt::IntegerRef WBRecordsetResultset::saveFieldValueToFile(ssize_t column, const std::string &file)
{
  if (column >= 0 && (size_t)column < recordset->get_column_count())
  {
    recordset->save_to_file(bec::NodeId(cursor), column, file);
    return grt::IntegerRef(1);
  }
  return grt::IntegerRef(0);
}

//================================================================================

class WBPUBLICBACKEND_PUBLIC_FUNC CPPResultsetResultset : public db_query_Resultset::ImplData
{
  boost::shared_ptr<sql::ResultSet> recordset;
public:
  CPPResultsetResultset(db_query_ResultsetRef aself, boost::shared_ptr<sql::ResultSet> rset)
  : ImplData(aself), recordset(rset)
  {
    sql::ResultSetMetaData* meta(recordset->getMetaData());
    const int last_column= meta->getColumnCount();
    for (int i = 1; i <= last_column; i++)
    {
      column_by_name[meta->getColumnLabel(i)] = i;

      std::string type;
      switch (meta->getColumnType(i))
      {
        case sql::DataType::UNKNOWN:
          type= "unknown";
          break;
        case sql::DataType::BIT:
        case sql::DataType::TINYINT:
        case sql::DataType::SMALLINT:
        case sql::DataType::MEDIUMINT:
        case sql::DataType::INTEGER:
        case sql::DataType::BIGINT:
          type= "numeric";
          break;

        case sql::DataType::REAL:
        case sql::DataType::DOUBLE:
          type= "numeric";
          break;

        case sql::DataType::DECIMAL:
        case sql::DataType::NUMERIC:
          type= "string";
          break;

        case sql::DataType::CHAR:
        case sql::DataType::VARCHAR:
          type= "string";
          break;

        case sql::DataType::BINARY:
        case sql::DataType::VARBINARY:
        case sql::DataType::LONGVARCHAR:
        case sql::DataType::LONGVARBINARY:
          type= "blob";
          break;

        case sql::DataType::TIMESTAMP:
          type= "string";
          break;
        case sql::DataType::DATE:
          type= "string";
          break;
        case sql::DataType::TIME:
          type= "numeric";
          break;

        case sql::DataType::YEAR:
          type= "numeric";
          break;
        case sql::DataType::GEOMETRY:
          type= "string";
          break;
        case sql::DataType::ENUM:
        case sql::DataType::SET:
          type= "string";
          break;
#if MYSQL_CHECK_VERSION(5, 7, 0)
        case sql::DataType::JSON:
          type = "json";
          break;
#endif
        case sql::DataType::SQLNULL:
          type = "null";
          break;          
      }

      db_query_ResultsetColumnRef column(aself->get_grt());

      column->owner(aself);
      column->name(std::string(meta->getColumnLabel(i)));
      column->columnType(type);

      self->columns().insert(column);
    }
  }

  virtual grt::StringRef sql() const
  {
    return grt::StringRef("");
  }

  virtual grt::IntegerRef currentRow() const
  {
    return grt::IntegerRef((long)recordset->getRow());
  }

  virtual grt::IntegerRef rowCount() const
  {
    return grt::IntegerRef(recordset->rowsCount());
  }

  virtual grt::DoubleRef floatFieldValue(ssize_t column)
  {
    if (column >= 0 && column < (ssize_t)column_by_name.size())
      return grt::DoubleRef(recordset->getDouble((uint32_t)column + 1)); // Hard coded to 32bit, <sigh>.
    throw std::invalid_argument(base::strfmt("invalid column %li for resultset", (long)column).c_str());
    return grt::DoubleRef(0.0);
  }


  virtual grt::DoubleRef floatFieldValueByName(const std::string &column)
  {
    if (column_by_name.find(column) != column_by_name.end())
    {
      return grt::DoubleRef(recordset->getDouble((long)column_by_name[column]));
    }
    throw std::invalid_argument(base::strfmt("invalid column %s for resultset", column.c_str()).c_str());
    return grt::DoubleRef(0.0);
  }


  virtual grt::IntegerRef goToFirstRow()
  {
    return grt::IntegerRef(recordset->first());
  }


  virtual grt::IntegerRef goToLastRow()
  {
    return grt::IntegerRef(recordset->last());
  }


  virtual grt::IntegerRef goToRow(ssize_t row)
  {
    return grt::IntegerRef(recordset->absolute((int)row));
  }


  virtual grt::IntegerRef intFieldValue(ssize_t column)
  {
    if (column >= 0 && column < (long)column_by_name.size())
      return grt::IntegerRef(recordset->getInt((uint32_t)column + 1));
    throw std::invalid_argument(base::strfmt("invalid column %li for resultset", (long)column).c_str());
    return grt::IntegerRef(0);
  }


  virtual grt::IntegerRef intFieldValueByName(const std::string &column)
  {
    if (column_by_name.find(column) != column_by_name.end())
    {
      return grt::IntegerRef(recordset->getInt((uint32_t)column_by_name[column]));
    }
    throw std::invalid_argument(base::strfmt("invalid column %s for resultset", column.c_str()).c_str());
    return grt::IntegerRef(0);
  }


  virtual grt::IntegerRef nextRow()
  {
    return grt::IntegerRef(recordset->next());
  }


  virtual grt::IntegerRef previousRow()
  {
    return grt::IntegerRef(recordset->previous());
  }


  virtual void refresh()
  {
  }


  virtual grt::StringRef stringFieldValue(ssize_t column)
  {
    if (column >= 0 && column < (long)column_by_name.size())
      return grt::StringRef(recordset->getString((uint32_t)column + 1));
    throw std::invalid_argument(base::strfmt("invalid column %li for resultset", (long)column).c_str());
    return grt::StringRef(); // NULL
  }


  virtual grt::StringRef stringFieldValueByName(const std::string &column)
  {
    if (column_by_name.find(column) != column_by_name.end())
    {
      return grt::StringRef(recordset->getString((uint32_t)column_by_name[column]));
    }
    throw std::invalid_argument(base::strfmt("invalid column %s for resultset", column.c_str()).c_str());
    return grt::StringRef(); // NULL
  }
  
  virtual grt::IntegerRef saveFieldValueToFile(ssize_t column, const std::string &file)
  {
    return grt::IntegerRef(0);
  }
};

//================================================================================


db_query_ResultsetRef grtwrap_recordset(GrtObjectRef owner, Recordset::Ref rset)
{
  db_query_ResultsetRef object(owner.get_grt());

  db_query_Resultset::ImplData *data= new WBRecordsetResultset(object, rset);
  
  object->owner(owner);

  object->set_data(data);

  return object;
}


db_query_ResultsetRef grtwrap_recordset(GrtObjectRef owner, boost::shared_ptr<sql::ResultSet> rset)
{
  db_query_ResultsetRef object(owner.get_grt());

  db_query_Resultset::ImplData *data = new CPPResultsetResultset(object, rset);

  object->owner(owner);

  object->set_data(data);

  return object;
}



void db_query_Resultset::init()
{
  // _data init is delayed and done by grtwrap_recordset
}

db_query_Resultset::~db_query_Resultset()
{
  delete _data;
}


void db_query_Resultset::set_data(ImplData *data)
{
  _data= data;
}

grt::IntegerRef db_query_Resultset::currentRow() const
{
  if (_data)
    return _data->currentRow();
  return grt::IntegerRef(0);
}

grt::StringRef db_query_Resultset::sql() const
{
  return _data ? _data->sql() : grt::StringRef();
}

grt::IntegerRef db_query_Resultset::rowCount() const
{
  return _data ? _data->rowCount() : grt::IntegerRef(0);
}

grt::DoubleRef db_query_Resultset::floatFieldValue(ssize_t column)
{
  return _data ? _data->floatFieldValue(column) : grt::DoubleRef(0.0);
}


grt::DoubleRef db_query_Resultset::floatFieldValueByName(const std::string &column)
{
  return _data ? _data->floatFieldValueByName(column) : grt::DoubleRef(0.0);
}


grt::IntegerRef db_query_Resultset::goToFirstRow()
{
  return _data ? _data->goToFirstRow() : grt::IntegerRef(0);
}


grt::IntegerRef db_query_Resultset::goToLastRow()
{
  return _data ? _data->goToLastRow() : grt::IntegerRef(0);
}


grt::IntegerRef db_query_Resultset::goToRow(ssize_t row)
{
  return _data ? _data->goToRow(row) : grt::IntegerRef(0);
}


grt::IntegerRef db_query_Resultset::intFieldValue(ssize_t column)
{
  return _data ? _data->intFieldValue(column) : grt::IntegerRef(0);
}


grt::IntegerRef db_query_Resultset::intFieldValueByName(const std::string &column)
{
  return _data ? _data->intFieldValueByName(column) : grt::IntegerRef(0);
}


grt::IntegerRef db_query_Resultset::nextRow()
{
  return _data ? _data->nextRow() : grt::IntegerRef(0);
}


grt::IntegerRef db_query_Resultset::previousRow()
{
  return _data ? _data->previousRow() : grt::IntegerRef(0);
}


grt::IntegerRef db_query_Resultset::refresh()
{
  if (_data)
    _data->refresh();

  return grt::IntegerRef(0);
}


grt::StringRef db_query_Resultset::stringFieldValue(ssize_t column)
{
  return _data ? _data->stringFieldValue(column) : grt::StringRef(); // NULL
}


grt::StringRef db_query_Resultset::stringFieldValueByName(const std::string &column)
{
  return _data ? _data->stringFieldValueByName(column) : grt::StringRef(); // NULL
}


grt::IntegerRef db_query_Resultset::saveFieldValueToFile(ssize_t column, const std::string &file)
{
  return _data ? _data->saveFieldValueToFile(column, file) : grt::IntegerRef(0);
}
