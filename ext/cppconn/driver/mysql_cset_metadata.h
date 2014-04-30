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

#ifndef _MYSQL_CSET_METADATA_H_
#define _MYSQL_CSET_METADATA_H_

#include <cppconn/resultset.h>
#include <cppconn/resultset_metadata.h>
#include "mysql_private_iface.h"

namespace sql
{
namespace mysql
{
namespace util {template<class T> class my_shared_ptr; }; // forward declaration.
class MySQL_DebugLogger;

class MySQL_ConstructedResultSetMetaData : public sql::ResultSetMetaData
{
	const MySQL_ConstructedResultSet * parent;
	sql::mysql::util::my_shared_ptr< MySQL_DebugLogger > * logger;
public:
	MySQL_ConstructedResultSetMetaData(const MySQL_ConstructedResultSet * p, sql::mysql::util::my_shared_ptr< MySQL_DebugLogger > * l);
	virtual ~MySQL_ConstructedResultSetMetaData();

	std::string getCatalogName(unsigned int columnIndex);

	unsigned int getColumnCount();

	int getColumnDisplaySize(unsigned int columnIndex);

	std::string getColumnLabel(unsigned int columnIndex);

	std::string getColumnName(unsigned int columnIndex);

	int getColumnType(unsigned int columnIndex);

	std::string getColumnTypeName(unsigned int columnIndex);

	int getPrecision(unsigned int columnIndex);

	int getScale(unsigned int columnIndex);

	std::string getSchemaName(unsigned int columnIndex);

	std::string getTableName(unsigned int columnIndex);

	bool isAutoIncrement(unsigned int columnIndex);

	bool isCaseSensitive(unsigned int columnIndex);

	bool isCurrency(unsigned int columnIndex);

	bool isDefinitelyWritable(unsigned int columnIndex);

	int isNullable(unsigned int columnIndex);

	bool isReadOnly(unsigned int columnIndex);

	bool isSearchable(unsigned int columnIndex);

	bool isSigned(unsigned int columnIndex);

	bool isWritable(unsigned int columnIndex);

private:
	/* Prevent use of these */
	MySQL_ConstructedResultSetMetaData(const MySQL_ConstructedResultSetMetaData &);
	void operator=(MySQL_ConstructedResultSetMetaData &);
};


}; /* namespace mysql */
}; /* namespace sql */

#endif /* _MYSQL_CSET_METADATA_H_ */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
