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

#include "mysql_constructed_resultset.h"
#include "mysql_cset_metadata.h"

#if !defined(_WIN32) && !defined(_WIN64)
#include <stdlib.h>
#else
#define atoll(x) _atoi64((x))
#endif	//	_WIN32


#include "mysql_debug.h"
#include "mysql_util.h"

namespace sql
{
namespace mysql
{

extern char * cppmysql_utf8_strup(const char *src, size_t srclen);


/* {{{ MySQL_ConstructedResultSet::MySQL_ConstructedResultSet() -I- */
MySQL_ConstructedResultSet::MySQL_ConstructedResultSet(const StringList& fn, const StringList& rset, sql::mysql::util::my_shared_ptr< MySQL_DebugLogger > * l)
  : rs(rset), started(false), row_position(0), is_closed(false), logger(l? l->getReference():NULL)
{
	CPP_ENTER("MySQL_ConstructedResultSet::MySQL_ConstructedResultSet");
	CPP_INFO_FMT("metadata.size=%d resultset.size=%d", fn.size(), rset.size());
	current_record = rs.begin();
	num_fields = static_cast<int>(fn.size());

	if (fn.size()) {
		num_rows =  rset.size() / fn.size();
	} else {
		num_rows = 0;
	}

	field_index_to_name_map = new std::string[num_fields];

	unsigned int idx = 0;
	for (StringList::const_iterator it = fn.begin(), e = fn.end(); it != e; it++, idx++) {
		char *tmp = cppmysql_utf8_strup(it->c_str(), 0);
		field_name_to_index_map[std::string(tmp)] = idx;
		field_index_to_name_map[idx] = std::string(tmp);
		free(tmp);
	}
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::~MySQL_ConstructedResultSet() -I- */
MySQL_ConstructedResultSet::~MySQL_ConstructedResultSet()
{
	/* Don't remove the block or we can get into problems with logger */
	{
		CPP_ENTER("MySQL_ConstructedResultSet::~MySQL_ConstructedResultSet");
		if (!isClosed()) {
			close();
		}
	}
	logger->freeReference();
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::seek() -I- */
inline void MySQL_ConstructedResultSet::seek()
{
	CPP_ENTER("MySQL_ConstructedResultSet::seek");
	current_record = rs.begin();
	/* i should be signed, or when row_position is 0 `i` will overflow */
	for (long long i = row_position - 1; i > 0; --i) {
		unsigned int j = num_fields;
		while (j--) {
			current_record++;
		}
	}
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::absolute() -I- */
bool
MySQL_ConstructedResultSet::absolute(int row)
{
	CPP_ENTER("MySQL_ConstructedResultSet::absolute");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	if (row > 0) {
		if (row > (int) num_rows) {
			afterLast();
		} else {
			row_position = row;
			seek();
			return true;
		}
	} else if (row < 0) {
		if ((-row) > (int) num_rows) {
			beforeFirst();
		} else {
			row_position = num_rows - (-row)  + 1;
			seek();
			return true;
		}
	} else {
		/* According to the JDBC book, absolute(0) means before the result set */
		beforeFirst();
	}
	return (row_position > 0 && row_position < (num_rows + 1));
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::afterLast() -I- */
void
MySQL_ConstructedResultSet::afterLast()
{
	CPP_ENTER("MySQL_ConstructedResultSet::afterLast");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	row_position = num_rows + 1;
	seek();
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::beforeFirst() -I- */
void
MySQL_ConstructedResultSet::beforeFirst()
{
	CPP_ENTER("MySQL_ConstructedResultSet::beforeFirst");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	row_position = 0;
	seek();
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::cancelRowUpdates() -U- */
void
MySQL_ConstructedResultSet::cancelRowUpdates()
{
	CPP_ENTER("MySQL_ConstructedResultSet::cancelRowUpdates");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::cancelRowUpdates()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::checkValid() -I- */
void
MySQL_ConstructedResultSet::checkValid() const
{
	CPP_ENTER("MySQL_ConstructedResultSet::checkValid");
	if (isClosed()) {
		throw sql::InvalidInstanceException("ResultSet has been closed");
	}
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::clearWarnings() -U- */
void
MySQL_ConstructedResultSet::clearWarnings()
{
	CPP_ENTER("MySQL_ConstructedResultSet::clearWarnings");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::clearWarnings()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::close() -I- */
void
MySQL_ConstructedResultSet::close()
{
	CPP_ENTER("MySQL_ConstructedResultSet::close");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	delete [] field_index_to_name_map;
	is_closed = true;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::findColumn() -I- */
unsigned int
MySQL_ConstructedResultSet::findColumn(const std::string& columnLabel) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::columnLabel");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	char * tmp = cppmysql_utf8_strup(columnLabel.c_str(), 0);
	FieldNameIndexMap::const_iterator iter = field_name_to_index_map.find(tmp);
	free(tmp);

	if (iter == field_name_to_index_map.end()) {
		return 0;
	}
	/* findColumn returns 1-based indexes */
	return iter->second + 1;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::first() -I- */
bool
MySQL_ConstructedResultSet::first()
{
	CPP_ENTER("MySQL_ConstructedResultSet::first");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	if (num_rows) {
		row_position = 1;
		seek();
	}
	return num_rows != 0;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getBoolean() -I- */
bool
MySQL_ConstructedResultSet::getBoolean(unsigned int columnIndex) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getBoolean(int)");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	if (isBeforeFirst() || isAfterLast()) {
		throw sql::InvalidArgumentException("MySQL_ConstructedResultSet::getString: can't fetch because not on result set");
	}
	return getInt(columnIndex) != 0;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getBoolean() -I- */
bool
MySQL_ConstructedResultSet::getBoolean(const std::string& columnLabel) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getBoolean(string)");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	return getInt(columnLabel) != 0;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getConcurrency() -U- */
int
MySQL_ConstructedResultSet::getConcurrency()
{
	CPP_ENTER("MySQL_ConstructedResultSet::getConcurrency");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::getConcurrency()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getCursorName() -U- */
std::string
MySQL_ConstructedResultSet::getCursorName()
{
	CPP_ENTER("MySQL_ConstructedResultSet::getCursorName");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::getCursorName()");
}
/* }}} */


// Get the given column as double
/* {{{ MySQL_ConstructedResultSet::getDouble() -I- */
double
MySQL_ConstructedResultSet::getDouble(unsigned int columnIndex) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getDouble(int)");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	/* Don't columnIndex--, as we use it in the while loop later */
	if (columnIndex > num_fields || columnIndex == 0) {
		throw sql::InvalidArgumentException("MySQL_ConstructedResultSet::getDouble: invalid value of 'columnIndex'");
	}
	if (isBeforeFirst() || isAfterLast()) {
		throw sql::InvalidArgumentException("MySQL_ConstructedResultSet::getDouble: can't fetch because not on result set");
	}

	StringList::iterator f = current_record;

	while (--columnIndex) {
		f++;
	}

	return atof(f->c_str());
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getDouble() -I- */
double
MySQL_ConstructedResultSet::getDouble(const std::string& columnLabel) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getDouble(string)");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	return getDouble(findColumn(columnLabel));
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getFetchDirection() -U- */
int
MySQL_ConstructedResultSet::getFetchDirection()
{
	CPP_ENTER("MySQL_ConstructedResultSet::getFetchDirection");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::getFetchDirection()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getFetchSize() -U- */
int
MySQL_ConstructedResultSet::getFetchSize()
{
	CPP_ENTER("MySQL_ConstructedResultSet::getFetchSize");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::getFetchSize()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getHoldability() -U- */
int
MySQL_ConstructedResultSet::getHoldability()
{
	CPP_ENTER("MySQL_ConstructedResultSet::getHoldability");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::getHoldability()");
}
/* }}} */


// Get the given column as int
/* {{{ MySQL_ConstructedResultSet::getInt() -I- */
int
MySQL_ConstructedResultSet::getInt(unsigned int columnIndex) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getInt(int)");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	/* Don't columnIndex--, as we use it in the while loop later */
	if (columnIndex > num_fields || columnIndex == 0) {
		throw sql::InvalidArgumentException("MySQL_ConstructedResultSet::getInt: invalid value of 'columnIndex'");
	}
	if (isBeforeFirst() || isAfterLast()) {
		throw sql::InvalidArgumentException("MySQL_ConstructedResultSet::getInt: can't fetch because not on result set");
	}

	StringList::iterator f = current_record;

	while (--columnIndex) {
		f++;
	}

	return atoi(f->c_str());
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getInt() -I- */
int
MySQL_ConstructedResultSet::getInt(const std::string& columnLabel) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getInt(string)");
	return getInt(findColumn(columnLabel));
}
/* }}} */


// Get the given column as int
/* {{{ MySQL_ConstructedResultSet::getLong() -I- */
long long
MySQL_ConstructedResultSet::getLong(unsigned int columnIndex) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getLong(int)");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	/* Don't columnIndex--, as we use it in the while loop later */
	if (columnIndex > num_fields || columnIndex == 0) {
		throw sql::InvalidArgumentException("MySQL_ConstructedResultSet::getLong: invalid value of 'columnIndex'");
	}
	if (isBeforeFirst() || isAfterLast()) {
		throw sql::InvalidArgumentException("MySQL_ConstructedResultSet::getLong: can't fetch because not on result set");
	}

	StringList::iterator f = current_record;

	while (--columnIndex) {
		f++;
	}

	return atoll(f->c_str());
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getLong() -I- */
long long
MySQL_ConstructedResultSet::getLong(const std::string& columnLabel) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getLong(string)");
	return getLong(findColumn(columnLabel));
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getMetaData() -I- */
sql::ResultSetMetaData *
MySQL_ConstructedResultSet::getMetaData() const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getMetaData");
	return new MySQL_ConstructedResultSetMetaData(this, logger);
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getRow() -I- */
size_t
MySQL_ConstructedResultSet::getRow() const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getRow");
	/* row_position is 0 based */
	return static_cast<size_t> (row_position);
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getRowId() -U- */
sql::RowID *
MySQL_ConstructedResultSet::getRowId(unsigned int)
{
	CPP_ENTER("MySQL_ConstructedResultSet::getRowId");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::getRowId()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getRowId() -U- */
sql::RowID *
MySQL_ConstructedResultSet::getRowId(const std::string &)
{
	CPP_ENTER("MySQL_ConstructedResultSet::getRowId");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::getRowId()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getStatement() -I- */
const Statement *
MySQL_ConstructedResultSet::getStatement() const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getStatement");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	return NULL; /* This is a constructed result set - no statement -> NULL */
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getString() -I- */
std::string
MySQL_ConstructedResultSet::getString(unsigned int columnIndex) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getString(int)");
	CPP_INFO_FMT("this=%p", this);
	CPP_INFO_FMT("columnIndex=%u", columnIndex);
	checkValid();
	/* Don't columnIndex--, as we use it in the while loop later */
	if (columnIndex > num_fields || columnIndex == 0) {
		throw sql::InvalidArgumentException("MySQL_ConstructedResultSet::getString: invalid value of 'columnIndex'");
	}
	if (isBeforeFirst() || isAfterLast()) {
		throw sql::InvalidArgumentException("MySQL_ConstructedResultSet::getString: can't fetch because not on result set");
	}

	StringList::iterator f = current_record;
	while (--columnIndex) {
		f++;
	}
	CPP_INFO_FMT("value=%d", f->length());
//	CPP_INFO_FMT("value=%*s", f->length(), f->c_str());
	return *f;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getString() -I- */
std::string
MySQL_ConstructedResultSet::getString(const std::string& columnLabel) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::getString(string)");
	return getString(findColumn(columnLabel));
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::getWarnings() -U- */
void
MySQL_ConstructedResultSet::getWarnings()
{
	CPP_ENTER("MySQL_ConstructedResultSet::getWarnings");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::getWarnings()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::insertRow() -U- */
void
MySQL_ConstructedResultSet::insertRow()
{
	CPP_ENTER("MySQL_ConstructedResultSet::insertRow");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	 /* TODO - We don't support inserting anyway */
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::insertRow()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::isAfterLast() -I- */
bool
MySQL_ConstructedResultSet::isAfterLast() const
{
	CPP_ENTER("MySQL_ConstructedResultSet::isAfterLast");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	return (row_position == num_rows + 1);
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::isBeforeFirst() -I- */
bool
MySQL_ConstructedResultSet::isBeforeFirst() const
{
	CPP_ENTER("MySQL_ConstructedResultSet::isBeforeFirst");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	return (row_position == 0);
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::isClosed() -I- */
bool
MySQL_ConstructedResultSet::isClosed() const
{
	CPP_ENTER("MySQL_ConstructedResultSet::isClosed");
	return is_closed;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::isFirst() -I- */
bool
MySQL_ConstructedResultSet::isFirst() const
{
	CPP_ENTER("MySQL_ConstructedResultSet::isFirst");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	/* OR current_record == rs.begin() */
	return (row_position == 1);
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::isLast() -I- */
bool
MySQL_ConstructedResultSet::isLast() const
{
	CPP_ENTER("MySQL_ConstructedResultSet::isLast");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	/* OR current_record == rs.end() */
	return (row_position == num_rows);
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::isLast() -I- */
bool
MySQL_ConstructedResultSet::isNull(unsigned int columnIndex) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::isNull(int)");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	/* internally zero based */
	columnIndex--;
	if (columnIndex >= num_fields) {
		throw sql::InvalidArgumentException("MySQL_ConstructedResultSet::isNull: invalid value of 'columnIndex'");
	}
	return false;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::isNull() -I- */
bool
MySQL_ConstructedResultSet::isNull(const std::string& columnLabel) const
{
	CPP_ENTER("MySQL_ConstructedResultSet::isNull(string)");
	return isNull(findColumn(columnLabel));
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::last() -I- */
bool
MySQL_ConstructedResultSet::last()
{
	CPP_ENTER("MySQL_ConstructedResultSet::last");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	if (num_rows) {
		row_position = num_rows;
		seek();
	}
	return num_rows? true:false;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::moveToCurrentRow() -U- */
void
MySQL_ConstructedResultSet::moveToCurrentRow()
{
	CPP_ENTER("MySQL_ConstructedResultSet::moveToCurrentRow");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::moveToCurrentRow()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::moveToInsertRow() -U- */
void
MySQL_ConstructedResultSet::moveToInsertRow()
{
	CPP_ENTER("MySQL_ConstructedResultSet::moveToInsertRow");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::moveToInsertRow()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::next() -I- */
bool
MySQL_ConstructedResultSet::next()
{
	CPP_ENTER("MySQL_ConstructedResultSet::next");
	checkValid();
	bool ret = false;
	if (row_position == num_rows) {
		afterLast();
	} else if (row_position == 0) {
		first();
		ret = true;
	} else if (row_position > 0 && row_position < num_rows) {
		int i = num_fields;
		while (i--) {
			current_record++;
		}
		row_position++;
		ret = true;
	}
	CPP_INFO_FMT("row_position=%llu num_rows=%llu", row_position, num_rows);
	return ret;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::previous() -I- */
bool
MySQL_ConstructedResultSet::previous()
{
	CPP_ENTER("MySQL_ConstructedResultSet::previous");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	if (row_position == 0) {
		return false;
	} else if (row_position == 1) {
		beforeFirst();
		return false;
	} else if (row_position > 1) {
		row_position--;
		int i = num_fields;
		while (i--) {
			current_record--;
		}
		return true;
	}
	throw sql::SQLException("Impossible");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::refreshRow() -U- */
void
MySQL_ConstructedResultSet::refreshRow()
{
	CPP_ENTER("MySQL_ConstructedResultSet::refreshRow");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ResultSet::refreshRow()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::relative() -I- */
bool
MySQL_ConstructedResultSet::relative(int rows)
{
	CPP_ENTER("MySQL_ConstructedResultSet::relative");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	if (rows != 0) {
		if ((row_position + rows) > num_rows || (row_position + rows) < 1) {
			rows > 0? afterLast(): beforeFirst(); /* after last or before first */
		} else {
			row_position += rows;
			seek();
		}
	}

	return (row_position < (num_rows + 1) || row_position > 0);
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::rowDeleted() -U- */
bool
MySQL_ConstructedResultSet::rowDeleted()
{
	CPP_ENTER("MySQL_ConstructedResultSet::rowDeleted");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::rowDeleted()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::rowInserted() -U- */
bool
MySQL_ConstructedResultSet::rowInserted()
{
	CPP_ENTER("MySQL_ConstructedResultSet::rowInserted");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::rowInserted()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::rowUpdated() -U- */
bool
MySQL_ConstructedResultSet::rowUpdated()
{
	CPP_ENTER("MySQL_ConstructedResultSet::rowUpdated");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ConstructedResultSet::rowUpdated()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::rowsCount() -I- */
size_t
MySQL_ConstructedResultSet::rowsCount() const
{
	CPP_ENTER("MySQL_ConstructedResultSet::rowsCount");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	return rs.size() / num_fields;
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::setFetchSize() -U- */
void
MySQL_ConstructedResultSet::setFetchSize(size_t /* rows */) 
{
	CPP_ENTER("MySQL_ConstructedResultSet::setFetchSize");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	throw sql::MethodNotImplementedException("MySQL_ResultSet::rowDeleted()");
}
/* }}} */


/* {{{ MySQL_ConstructedResultSet::wasNull() -I- */
bool
MySQL_ConstructedResultSet::wasNull() const
{
	CPP_ENTER("MySQL_ConstructedResultSet::wasNull");
	CPP_INFO_FMT("this=%p", this);
	checkValid();
	return false;
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
