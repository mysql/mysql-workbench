/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include "grt/parse_utils.h"
#include "base/string_utilities.h"

#include "casmine.h"

namespace {

$ModuleEnvironment() {};

using namespace bec;

$describe("Common utility functions") {
  $it("String tokenization", [&](){

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

    auto check = [](std::string str, std::list<std::string> expected) {
      std::list<std::string> tokens;
      $expect(bec::tokenize_string_list(str, '\'', false, tokens)).toBeTrue();
    };

    auto check_fail = [](std::string str, bool nounquoted) {
      std::list<std::string> tokens;
      $expect(tokenize_string_list(str, '\'', nounquoted, tokens)).toBeFalse();
    };

    check("'foo','bar', 'baz' ,'bla'", foo_bar_baz);
    check("'foo' ,'bar','baz' , 'bla'", foo_bar_baz);

    check("'foo' ,'bar','baz' , 'bla'", foo_bar_baz);
    check("'foo', 'bar', NULL", foo_bar_NULL);
    check("NULL", NULL_);
    check("'foo\\'', 'bar'", foo_quote);

    check_fail("'foo' bar", false);
    check_fail("NULL NULL", false);
    check_fail("'foo' 'bar'", false);
    check_fail("'foo", false);
    check_fail("'foo', 'bar", false);
    check_fail("'foo', bar", true);
    check_fail("'foo', NULL", true);
  });
}
  
}
