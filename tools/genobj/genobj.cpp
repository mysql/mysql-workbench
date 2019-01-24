/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _MSC_VER
#include <iostream>

#include "base/log.h"

#include "grt.h"
#include "grtpp_helper.h"

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char **argv) {
  if (argc < 5) {
    std::cout << "\nNot enough parameters given. Syntax:\n" << std::endl;
    std::cout << "  genobj <structs-file> <structs-dir> <output-dir> <impl-output-dir>\n" << std::endl;
    return -1;
  }

  base::Logger(".");

  std::string structs_file = argv[1];
  std::string structs_dir = argv[2];
  std::string output_dir = argv[3];
  std::string impl_output_dir = argv[4];

  std::multimap<std::string, std::string> requires;

  std::cout << "Reading structs from '" << structs_dir << "', outputing classes to '" << output_dir << "'\n" << std::endl;

  grt::GRT::get()->scan_metaclasses_in(structs_dir, &requires);
  grt::GRT::get()->end_loading_metaclasses(false);

  grt::helper::generate_struct_code(structs_file, output_dir, impl_output_dir, requires);

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------
