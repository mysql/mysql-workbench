/* 
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MYSQL_PARSER_SERVICES_H_
#define _MYSQL_PARSER_SERVICES_H_

#include "wbpublic_public_interface.h"

/**
 * Defines an abstract interface for parser services. The actual implementation is done in a module.
 */
class WBPUBLICBACKEND_PUBLIC_FUNC MySQLParserServices
{
public:
  virtual int stopProcessing() = 0;
  virtual int determineStatementRanges(const char *sql, size_t length, const std::string &initial_delimiter, 
    std::vector<std::pair<size_t, size_t> > &ranges) = 0;
};


#endif // _MYSQL_PARSER_SERVICES_H_
