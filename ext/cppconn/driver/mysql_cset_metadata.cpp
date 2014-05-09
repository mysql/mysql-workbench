/* Copyright (C) 2007 - 2008 MySQL AB, 2008 Sun Microsystems, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   There are special exceptions to the terms and conditions of the GPL
   as it is applied to this software. View the full text of the
   exception in file EXCEPTIONS-CONNECTOR-C++ in the directory of this
   software distribution.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _WIN32
#include <stdlib.h>
#endif	//	_WIN32

#include "mysql_constructed_resultset.h"
#include "mysql_cset_metadata.h"

#include "mysql_debug.h"
#include "mysql_util.h"

namespace sql
{
namespace mysql
{

/* {{{ MySQL_ConstructedResultSetMetaData::MySQL_ConstructedResultSetMetaData() -I- */
MySQL_ConstructedResultSetMetaData::MySQL_ConstructedResultSetMetaData(const MySQL_ConstructedResultSet * p,
																	   sql::mysql::util::my_shared_ptr< MySQL_DebugLogger > * l)
  : parent(p), logger(l? l->getReference():NULL)
{
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::~MySQL_ConstructedResultSetMetaData() -I- */
MySQL_ConstructedResultSetMetaData::~MySQL_ConstructedResultSetMetaData()
{
	/* Don't remove the block or we can get into problems with logger */
	{
		CPP_ENTER("MySQL_ConstructedResultSetMetaData::~MySQL_ConstructedResultSetMetaData");
		CPP_INFO_FMT("this=%p", this);
	}
	logger->freeReference();
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::getCatalogName() -I- */
std::string
MySQL_ConstructedResultSetMetaData::getCatalogName(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::getCatalogName");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return "";
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::getColumnCount() -I- */
unsigned int
MySQL_ConstructedResultSetMetaData::getColumnCount()
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::getColumnCount");
	CPP_INFO_FMT("this=%p", this);
	CPP_INFO_FMT("column_count=%d", parent->num_fields);
	return parent->num_fields;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::getColumnDisplaySize() -U- */
int
MySQL_ConstructedResultSetMetaData::getColumnDisplaySize(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::getColumnDisplaySize");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSetMetaData::getColumnDisplaySize()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::getColumnLabel() -I- */
std::string
MySQL_ConstructedResultSetMetaData::getColumnLabel(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::getColumnLabel");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return parent->field_index_to_name_map[columnIndex - 1];
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::getColumnName() -I- */
std::string
MySQL_ConstructedResultSetMetaData::getColumnName(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::getColumnName");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return parent->field_index_to_name_map[columnIndex - 1];
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::getColumnType() -I- */
int
MySQL_ConstructedResultSetMetaData::getColumnType(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::getColumnType");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return MYSQL_TYPE_VARCHAR;
//	return mysql_fetch_field_direct(result->get(), columnIndex - 1)->type;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::getColumnTypeName() -I- */
std::string
MySQL_ConstructedResultSetMetaData::getColumnTypeName(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::getColumnTypeName");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return "VARCHAR";
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::getPrecision() -U- */
int
MySQL_ConstructedResultSetMetaData::getPrecision(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::getPrecision");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSetMetaData::getPrecision()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::getScale() -U- */
int
MySQL_ConstructedResultSetMetaData::getScale(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::getScale");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSetMetaData::getScale()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::getSchemaName() -I- */
std::string
MySQL_ConstructedResultSetMetaData::getSchemaName(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::getSchemaName");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return "";
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::getTableName() -I- */
std::string
MySQL_ConstructedResultSetMetaData::getTableName(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::getTableName");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return "";
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::isAutoIncrement() -I- */
bool
MySQL_ConstructedResultSetMetaData::isAutoIncrement(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::isAutoIncrement");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return false;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::isCaseSensitive() -I- */
bool
MySQL_ConstructedResultSetMetaData::isCaseSensitive(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::isCaseSensitive");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return "true";
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::isCurrency() -I- */
bool
MySQL_ConstructedResultSetMetaData::isCurrency(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::isCurrency");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return false;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::isDefinitelyWritable() -I- */
bool
MySQL_ConstructedResultSetMetaData::isDefinitelyWritable(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::isDefinitelyWritable");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return isWritable(columnIndex);
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::isNullable() -I- */
int
MySQL_ConstructedResultSetMetaData::isNullable(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::isNullable");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return false;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::isReadOnly() -I- */
bool
MySQL_ConstructedResultSetMetaData::isReadOnly(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::isReadOnly");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	/* We consider we connect to >= 40100 - else, we can't say */
	return true;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::isSearchable() -I- */
bool
MySQL_ConstructedResultSetMetaData::isSearchable(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::isSearchable");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return true;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::isSigned() -I- */
bool
MySQL_ConstructedResultSetMetaData::isSigned(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::isSigned");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return false;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSetMetaData::isWritable() -I- */
bool
MySQL_ConstructedResultSetMetaData::isWritable(unsigned int columnIndex)
{
	CPP_ENTER("MySQL_ConstructedResultSetMetaData::isWritable");
	CPP_INFO_FMT("this=%p", this);
	if (columnIndex == 0 || columnIndex > parent->num_fields) {
		throw sql::InvalidArgumentException("Invalid value for columnIndex");
	}
	return !isReadOnly(columnIndex);
}
/* }}} */


}; /* namespace mysql */
}; /* namespace sql */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
