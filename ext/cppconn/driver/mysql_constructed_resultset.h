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

#ifndef _MYSQL_CONSTRUCTED_RESULTSET_H_
#define _MYSQL_CONSTRUCTED_RESULTSET_H_

#include <cppconn/resultset.h>
#include "mysql_res_wrapper.h"

#include "mysql_private_iface.h"

namespace sql
{
namespace mysql
{
namespace util {template<class T> class my_shared_ptr; }; // forward declaration.

class MySQL_DebugLogger;

class MySQL_ConstructedResultSet : public sql::ResultSet
{
public:
	typedef std::list<std::string> StringList;

	MySQL_ConstructedResultSet(const StringList& fn, const StringList& rset, sql::mysql::util::my_shared_ptr< MySQL_DebugLogger > * l);
	virtual ~MySQL_ConstructedResultSet();

	bool absolute(int row);

	void afterLast();

	void beforeFirst();

	void cancelRowUpdates();

	void clearWarnings();

	void close();

	unsigned int findColumn(const std::string& columnLabel) const;

	bool first();

	bool getBoolean(unsigned int columnIndex) const;

	bool getBoolean(const std::string& columnLabel) const;
	
	int getConcurrency();

	std::string getCursorName();

	// Get the given column as double
	double getDouble(unsigned int columnIndex) const;

	double getDouble(const std::string& columnLabel) const;

	int getFetchDirection();
	int getFetchSize();
	int getHoldability();

	// Get the given column as int
	int getInt(unsigned int columnIndex) const;

	int getInt(const std::string& columnLabel) const;

	// Get the given column as int
	long long getLong(unsigned int columnIndex) const;

	long long getLong(const std::string& columnLabel) const;

	sql::ResultSetMetaData * getMetaData() const;

	size_t getRow() const;

	sql::RowID * getRowId(unsigned int columnIndex);
	sql::RowID * getRowId(const std::string & columnLabel);

	const sql::Statement * getStatement() const;

	// Get the given column as string
	std::string getString(unsigned int columnIndex) const;

	std::string getString(const std::string& columnLabel) const;

	void getWarnings();

	void insertRow();

	bool isAfterLast() const;

	bool isBeforeFirst() const;

	bool isClosed() const;

	bool isFirst() const;

	// Retrieves whether the cursor is on the last row of this sql::ResultSet object.
	bool isLast() const;

	bool isNull(unsigned int columnIndex) const;

	bool isNull(const std::string& columnLabel) const;

	bool last();

	void moveToCurrentRow();

	void moveToInsertRow();

	bool next();

	bool previous();

	void refreshRow();

	bool relative(int rows);	
	bool rowDeleted();

	bool rowInserted();

	bool rowUpdated();

	size_t rowsCount() const;

	void setFetchSize(size_t rows);

	bool wasNull() const;

protected:
	void checkValid() const;
	void seek();

public:

	unsigned int num_fields;
	StringList rs;
	bool started;

	typedef std::map< std::string, int > FieldNameIndexMap;
	typedef std::pair< std::string, int > FieldNameIndexPair;

	FieldNameIndexMap field_name_to_index_map;
	std::string * field_index_to_name_map;
	StringList::iterator current_record;

	my_ulonglong num_rows;
	my_ulonglong row_position; /* 0 = before first row, 1 - first row, 'num_rows + 1' - after last row */

	bool is_closed;

protected:
	sql::mysql::util::my_shared_ptr< MySQL_DebugLogger > * logger;

private:
	/* Prevent use of these */
	MySQL_ConstructedResultSet(const MySQL_ConstructedResultSet &);
	void operator=(MySQL_ConstructedResultSet &);
};

}; /* namespace mysql */
}; /* namespace sql */
#endif // _MYSQL_CONSTRUCTED_RESULTSET_H_

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
