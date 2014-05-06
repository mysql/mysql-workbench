/* 
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql_parser_services.h"

#include "base/string_utilities.h"

using namespace parser;

//------------------ ParserContext -----------------------------------------------------------------

ParserContext::ParserContext(db_mgmt_RdbmsRef rdbms)
{
  std::set<std::string> charsets;
  grt::ListRef<db_CharacterSet> list = rdbms->characterSets();
  for (size_t i = 0; i < list->count(); i++)
    charsets.insert(base::tolower(*list[i]->name()));

  // 3 character sets were added in version 5.5.3. Remove them from the list if the current version
  // is lower than that.
  GrtVersionRef version = rdbms->version();
  size_t server_version;
  if (version.is_valid())
    server_version = (unsigned)(version->majorNumber() * 10000 + version->minorNumber() * 100 + version->releaseNumber());
  else
    server_version = 50501; // Assume some reasonable default (5.5.1).
  if (server_version < 50503)
  {
    charsets.erase("utf8mb4");
    charsets.erase("utf16");
    charsets.erase("utf32");
  }
  
  _recognizer = new MySQLRecognizer((long)server_version, "", charsets);
}

//--------------------------------------------------------------------------------------------------

ParserContext::~ParserContext()
{
  delete _recognizer;
}

//--------------------------------------------------------------------------------------------------

void ParserContext::use_sql_mode(const std::string &mode)
{
  _recognizer->set_sql_mode(mode);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a collection of errors from the last parser run. The start position is offset by the given
 * value (used to adjust error position in a larger context).
 */
std::vector<ParserErrorEntry> ParserContext::get_errors_with_offset(size_t offset)
{
  std::vector<ParserErrorEntry> errors;

  if (_recognizer->has_errors())
  {
    const std::vector<MySQLParserErrorInfo> error_info = _recognizer->error_info();
    for (std::vector<MySQLParserErrorInfo>::const_iterator error_iterator = error_info.begin();
         error_iterator != error_info.end(); ++error_iterator)
    {
      ParserErrorEntry entry = {error_iterator->message, error_iterator->charOffset + offset,
        error_iterator->line, error_iterator->length};
      errors.push_back(entry);
    }
  }

  return errors;
}

//------------------ MySQLParserServices -----------------------------------------------------------

ParserContext::Ref MySQLParserServices::createParserContext(db_mgmt_RdbmsRef rdbms)
{
  boost::shared_ptr<ParserContext> result(new ParserContext(rdbms));

  return result;
}

//--------------------------------------------------------------------------------------------------

MySQLParserServices::Ref MySQLParserServices::get(grt::GRT *grt)
{
  MySQLParserServices::Ref module = dynamic_cast<MySQLParserServices::Ref>(grt->get_module("MySQLParserServices"));
  if (!module)
		throw std::runtime_error("Can't get MySQLParserServices module.");
  return module;
}

//--------------------------------------------------------------------------------------------------
