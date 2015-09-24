/*
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "mysql-parser.h"
#include "mysql-syntax-check.h"
#include "mysql-scanner.h"

using namespace parser;

//--------------------------------------------------------------------------------------------------

long short_version(const GrtVersionRef &version)
{
  ssize_t short_version;
  if (version.is_valid())
  {
    short_version = version->majorNumber() * 10000;
    if (version->minorNumber() > -1)
      short_version += version->minorNumber() * 100;
    else
      short_version += 500;
    if (version->releaseNumber() > -1)
      short_version += version->releaseNumber();
  }
  else
    short_version = 50501; // Assume some reasonable default (5.5.1).

  return (long)short_version;
}

//------------------ ParserContext -----------------------------------------------------------------

ParserContext::ParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version,
  bool case_sensitive)
{
  _version = version;
  _case_sensitive = case_sensitive;

  for (size_t i = 0; i < charsets->count(); i++)
    _filtered_charsets.insert(base::tolower(*charsets[i]->name()));

  long server_version = short_version(_version);
  update_filtered_charsets(server_version);

  // Both, parser and syntax checker are only a few hundreds of bytes in size (except for any
  // stored token strings or the AST), so we can always simply create both without serious memory
  // concerns (the syntax checker has no AST).
  _recognizer = new MySQLRecognizer(server_version, "", _filtered_charsets);
  _syntax_checker = new MySQLSyntaxChecker(server_version, "", _filtered_charsets);
}

//--------------------------------------------------------------------------------------------------

ParserContext::~ParserContext()
{
  delete _recognizer;
  delete _syntax_checker;
}

//--------------------------------------------------------------------------------------------------

void ParserContext::update_filtered_charsets(long version)
{
  if (version < 50503)
  {
    _filtered_charsets.erase("utf8mb4");
    _filtered_charsets.erase("utf16");
    _filtered_charsets.erase("utf32");
  }
  else
  {
    // Duplicates are automatically ignored.
    _filtered_charsets.insert("utf8mb4");
    _filtered_charsets.insert("utf16");
    _filtered_charsets.insert("utf32");
  }
}

//--------------------------------------------------------------------------------------------------

boost::shared_ptr<MySQLScanner> ParserContext::createScanner(const std::string &text)
{
  long server_version = short_version(_version);
  return boost::shared_ptr<MySQLScanner>(new MySQLScanner(text.c_str(), text.size(), true, server_version,
    _sql_mode, _filtered_charsets));
}

//--------------------------------------------------------------------------------------------------

boost::shared_ptr<MySQLQueryIdentifier> ParserContext::createQueryIdentifier()
{
  long version = short_version(_version);
  return boost::shared_ptr<MySQLQueryIdentifier>(new MySQLQueryIdentifier(version, _sql_mode, _filtered_charsets));
}

//--------------------------------------------------------------------------------------------------

void ParserContext::use_sql_mode(const std::string &mode)
{
  _sql_mode = mode;
  _recognizer->set_sql_mode(mode);
  _syntax_checker->set_sql_mode(mode);
}

//--------------------------------------------------------------------------------------------------

std::string ParserContext::get_sql_mode()
{
  return _sql_mode;
}

//--------------------------------------------------------------------------------------------------

void ParserContext::use_server_version(GrtVersionRef version)
{
  if (_version == version)
    return;

  _version = version;

  long server_version = short_version(_version);
  update_filtered_charsets(server_version);

  _recognizer->set_server_version(server_version);
  _syntax_checker->set_server_version(server_version);
}

//--------------------------------------------------------------------------------------------------

/**
* Returns a collection of errors from the last parser run. The start position is offset by the given
* value (used to adjust error position in a larger context).
*/
std::vector<ParserErrorEntry> ParserContext::get_errors_with_offset(size_t offset, bool for_syntax_check)
{
  std::vector<ParserErrorEntry> errors;

  MySQLRecognitionBase *recognizer = _recognizer;
  if (for_syntax_check)
    recognizer = _syntax_checker;
  if (recognizer->has_errors())
  {
    const std::vector<MySQLParserErrorInfo> error_info = recognizer->error_info();
    for (std::vector<MySQLParserErrorInfo>::const_iterator error_iterator = error_info.begin();
      error_iterator != error_info.end(); ++error_iterator)
    {
      ParserErrorEntry entry = { error_iterator->message, error_iterator->charOffset + offset,
        error_iterator->line, error_iterator->length };
      errors.push_back(entry);
    }
  }

  return errors;
}

//--------------------------------------------------------------------------------------------------

uint32_t ParserContext::get_keyword_token(const std::string &keyword)
{
  return _recognizer->get_keyword_token(keyword);
}

//--------------------------------------------------------------------------------------------------

/**
* Returns the pointer to the internal keyword strings array in the parser, so we can directly
* work with those keywords. The position in the keywords list corresponds to their token value.
* The first user defined token name starts at index 4.
*/
char ** ParserContext::get_token_name_list()
{
  return _recognizer->get_token_list();
}

//------------------ MySQLParserServices -----------------------------------------------------------

ParserContext::Ref MySQLParserServices::createParserContext(GrtCharacterSetsRef charsets,
  GrtVersionRef version, bool case_sensitive)
{
  boost::shared_ptr<ParserContext> result(new ParserContext(charsets, version, case_sensitive));

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
