/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "test.h"
#include "grt/parse_utils.h"
#include "base/string_utilities.h"

using namespace bec;

TEST_MODULE(be_common_utils, "common utility functions");

TEST_FUNCTION(1) {
  std::list<std::string> foo_bar_baz;
  std::list<std::string> foo_bar_NULL;
  std::list<std::string> foo_quote;
  std::list<std::string> NULL_;

  foo_bar_baz.push_back("'foo'");
  foo_bar_baz.push_back("'bar'");
  foo_bar_baz.push_back("'baz'");
  foo_bar_baz.push_back("'bla'");

  foo_bar_NULL.push_back("'foo'");
  foo_bar_NULL.push_back("'bar'");
  foo_bar_NULL.push_back("NULL");

  foo_quote.push_back("'foo\\''");
  foo_quote.push_back("'bar'");

  NULL_.push_back("NULL");

#define CHECK(str, expected)                                          \
  {                                                                   \
    std::list<std::string> tokens;                                    \
    ensure(str, bec::tokenize_string_list(str, '\'', false, tokens)); \
    /*std::equal(tokens.begin(), tokens.end(), expected.begin());*/   \
  }

#define CHECK_FAIL(str, nounquoted)                                    \
  {                                                                    \
    std::list<std::string> tokens;                                     \
    ensure(str, !tokenize_string_list(str, '\'', nounquoted, tokens)); \
  }

  CHECK("'foo','bar', 'baz' ,'bla'", foo_bar_baz);
  CHECK("'foo' ,'bar','baz' , 'bla'", foo_bar_baz);

  CHECK("'foo' ,'bar','baz' , 'bla'", foo_bar_baz);
  CHECK("'foo', 'bar', NULL", foo_bar_NULL);
  CHECK("NULL", NULL_);
  CHECK("'foo\\'', 'bar'", foo_quote);

  CHECK_FAIL("'foo' bar", false);
  CHECK_FAIL("NULL NULL", false);
  CHECK_FAIL("'foo' 'bar'", false);
  CHECK_FAIL("'foo", false);
  CHECK_FAIL("'foo', 'bar", false);
  CHECK_FAIL("'foo', bar", true);
  CHECK_FAIL("'foo', NULL", true);
}

END_TESTS
