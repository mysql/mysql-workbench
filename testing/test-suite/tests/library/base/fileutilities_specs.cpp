/*
 * Copyright (c) 2018, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/file_utilities.h"
#include "grt.h"

#include "casmine.h"

#ifdef _MSC_VER
#define FILE_SEPARATOR "\\"
#define INVALID_NAME "__test_file01*?"
#define RESERVED_NAME "com1"
#define MKDIR "mkdir "
#define READONLY "attrib +R "
#define READWRITE "attrib -R "
#else
#define FILE_SEPARATOR "/"
#define INVALID_NAME "__test_file01/?"
#define RESERVED_NAME "."
#define MKDIR "mkdir -p "
#define READONLY "chmod 0444 "
#define READWRITE "chmod 0777 "
#endif

#define TEST_DIR_NAME01 "__test_dir01"
#define TEST_DIR_NAME02 "__test_dir02"
#define TEST_FILE_NAME01 "__test_file01.txt"
#define TEST_FILE_NAME02 "__test_file02.txt"
#define TEST_FILE_NAME03 "__test_file03.txt"
#define TEST_FILE_NAME04 "__test_file04.txt"
#define TEST_FILE_STRIPPED_NAME02 "__test_file02"

#define FILE_PATTERN "__test_"
#define EMPTY_NAME ""

namespace {

$ModuleEnvironment() {};

$TestData {
  std::string too_long_name;
  std::string too_long_basename;
  std::string dir_unicode_name;
  std::string file_unicode_name;
  std::string file_unicode_basename;
};

$describe("file utilities") {

  $beforeAll([this]() {
    unsigned int i;

    data->dir_unicode_name.clear();
    data->dir_unicode_name += "__test_dir_";
    data->dir_unicode_name += "\xE3\x8A\xA8"; // Valid Unicode character.

    data->file_unicode_basename.clear();
    data->file_unicode_basename += "__test_file_";
    data->file_unicode_basename += "\xE3\x8F\xA3"; // Valid Unicode character.

    data->file_unicode_name.clear();
    data->file_unicode_name = data->file_unicode_basename;
    data->file_unicode_name += ".txt";

    data->too_long_basename.clear();
    for (i = 0; i < 1000; i++)
      data->too_long_basename.append("x");

    data->too_long_name.clear();
    data->too_long_name = data->too_long_basename;
    data->too_long_name += ".txt";

    // Clean up any existing test directories/files
    base::remove_recursive(TEST_DIR_NAME01);
    base::remove_recursive(TEST_DIR_NAME02);
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME02);
    base::remove(TEST_FILE_NAME03);
    base::remove(TEST_FILE_NAME04);
    base::remove(data->dir_unicode_name);
    base::remove(data->file_unicode_name);
  });

  $afterAll([this]() {
    // Clean up any remaining test directories/files
    base::remove_recursive(TEST_DIR_NAME01);
    base::remove_recursive(TEST_DIR_NAME02);
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME02);
    base::remove(TEST_FILE_NAME03);
    base::remove(TEST_FILE_NAME04);
    base::remove(data->dir_unicode_name);
    base::remove(data->file_unicode_name);
  });

  // Testing public API's
  // - create_directory(const std::string &path, int mode)
  // - remove(const std::string &path)
  $it("Remove a non existing directory", [&]() {
    $expect([]() {
      $expect(base::remove(TEST_DIR_NAME01)).toBeFalse();
    }).Not.toThrow();

    // Create a non existing directory
    $expect([]() {
      $expect(base::create_directory(TEST_DIR_NAME01, 0700)).toBeTrue();
    }).Not.toThrow();

    // Create an already existing directory
    $expect([]() {
      $expect(base::create_directory(TEST_DIR_NAME01, 0700)).toBeFalse();
    }).Not.toThrow();

    // Remove an existing directory
    $expect([]() {
      $expect(base::remove(TEST_DIR_NAME01)).toBeTrue();
    }).Not.toThrow();

    // Remove a non existing file
    $expect([]() {
      $expect(base::remove(TEST_FILE_NAME01)).toBeFalse();
    }).Not.toThrow();

    // Remove an existing file
    $expect([]() {
      base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w+"); // Create file
      test_file_scoped.dispose();                                // Close file

      $expect(base::remove(TEST_FILE_NAME01)).toBeTrue();
    }).Not.toThrow();
  });

  // Testing public API's
  // - create_directory(const std::string &path, int mode)
  // - remove(const std::string &path)
  // -- Corner/Limit Values --
  $it("Create a directory", [this]() {
    // Create a directory -- Invalid name
    $expect([]() { base::create_directory(INVALID_NAME, 0700); }).toThrowError<base::file_error>("^Could not create directory.*");

    // Create a directory -- Empty name
    $expect([]() { base::create_directory(EMPTY_NAME, 0700); }).toThrowError<base::file_error>("^Could not create directory.*");

    // Create a directory -- Too long name
    $expect([this]() { base::create_directory(data->too_long_name, 0700); }).toThrowError<base::file_error>("^Could not create directory.*");

    // Create a directory -- Unicode name
    $expect([this]() {
      $expect(base::create_directory(data->dir_unicode_name, 0700)).toBeTrue();
    }).Not.toThrow();

#ifdef _MSC_VER
    // Create a directory -- Reserved name. This will not fail in linux, returning false (already exists)
    $expect([]() {
      $expect(base::create_directory(RESERVED_NAME, 0700)).toBeTrue();
    }).Not.toThrow();
#endif

    // Create a file -- Unicode name
    $expect([this]() {
      base::FileHandle test_file_scoped(data->file_unicode_name, "w+"); // Create file
      $expect(base::file_exists(data->file_unicode_name)).toBeTrue();
    }).Not.toThrow();
#ifdef _MSC_VER
    // Remove a file/directory -- Invalid name
    $expect([]() { base::remove(INVALID_NAME); }).toThrow(base::file_error("Could not delete file", 0));

    // Remove a file/directory -- Empty name
    $expect([]() { base::remove(EMPTY_NAME); }).toThrow(base::file_error("Could not delete file", 0));
#endif

    // Remove a file/directory -- Too long name
    $expect([this]() { base::remove(data->too_long_name); }).toThrowError<base::file_error>("^Could not delete file.*");

    // Remove an existing directory -- Unicode name
    $expect([this]() {
      $expect(base::remove(data->dir_unicode_name)).toBeTrue();
    }).Not.toThrow();

    // Remove an existing file -- Unicode name
    $expect([this]() {
      $expect(base::remove(data->file_unicode_name)).toBeTrue();
    }).Not.toThrow();

    // Remove a file/directory -- Reserved name
    // try
    //{
    //  if (base::remove(RESERVED_NAME))
    //  {
    //      // return true means dir exists
    //      fail(strfmt("TEST 10.12: Directory \"%s\" exists",dir_unicode_name));
    //  }
    //}
    // catch (base::file_error &exc)
    //{
    //  throw grt::os_error(strfmt("Cannot remove directory for document: %s", exc.what()));
    //}

    // Clean leftover test files
    base::remove(data->dir_unicode_name);
    base::remove(data->file_unicode_name);
  });

  // Testing file_error public API's
  // - c-tor
  // - code()
  // - sys_code()
  $it("Miscellaneous variables", [&]() {
    // Miscellaneous variables
    base::error_code test_result;
    base::error_code expected_result;
    // int int_test_result;
    // int int_expected_result;

    $expect([&]() {
      // Testing c-tor
      base::file_error c_tor_file_error("Error Test", 0);

      // Testing code()
      test_result = c_tor_file_error.code();
      expected_result = base::success;

      $expect((int)test_result).toBe(expected_result);

      // Testing sys_code()
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // NOTE: THIS FUNCTION IS NOT IMPLEMENTED YET!!!!
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

      // int_test_result = c_tor_file_error.sys_code();
      // int_expected_result = 0;

      // if(int_test_result != int_expected_result)
      //{
      //  fail("TEST 15.2: Unexpected result calling function sys_code()");
      //}
    }).Not.toThrow();
  });

  // Testing file_error public API's
  // - c-tor
  // -- Corner/Limit Values --
  $it("file_error::file_error", [this]() {
    $expect([this]() {
      // Testing c-tor -- Empty string
      base::file_error c_tor_file_error_empty(EMPTY_NAME, 0);

      // Testing c-tor -- Unicode string
      base::file_error c_tor_file_error_unicode(data->file_unicode_name, 0);
    }).Not.toThrow();
  });

  // Testing FileHandle public API's (operators)
  // - operator FILE *()
  //  - operator bool()
  // - operator =(FileHandle &fh)
  // - operator ->()
  $it("FileHandle operators", [&]() {
    $expect([]() {
      // Miscellaneous variables
      base::FileHandle test_file01(TEST_FILE_NAME01, "w+");
      base::FileHandle test_file03(TEST_FILE_NAME03, "w+");
      base::FileHandle test_file04;

      FILE *test_result = NULL;
      FILE *expected_result = NULL;

      // Testing overridden operator bool()
      $expect(test_file01.operator bool()).toBeFalse();

      test_file01.dispose();

      $expect(test_file01.operator bool()).toBeTrue();

      // Testing overridden operator =(FileHandle &fh)
      expected_result = test_file03.file();
      test_file04 = test_file03;
      test_result = test_file04.file();

      $expect(test_result).toBe(expected_result);
    }).Not.toThrow();

    // Clean leftover test files
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME03);
  });

  // Testing FileHandle public API's
  // - swap(FileHandle &fh)
  // - file()
  // - dispose()
  $it("FileHandle swap, file and dispose", []() {
    $expect([]() {
      // Miscellaneous variables
      base::FileHandle test_file01(TEST_FILE_NAME01, "w+");
      base::FileHandle test_file02(TEST_FILE_NAME02, "w+");
      base::FileHandle test_file03;

      FILE *test_result = nullptr;
      FILE *expected_result = nullptr;

      // Testing swap(FileHandle &fh)
      expected_result = test_file02.file();
      test_file01.swap(test_file02);
      test_result = test_file01.file();

      $expect(test_result).toBe(expected_result);

      // Testing file()
      $expect(test_file01.file()).Not.toBe(nullptr);

      // Testing dispose()
      test_file01.dispose();
      test_result = test_file01.file();
      expected_result = nullptr;

      $expect(test_result).toBe(expected_result);

      // Testing open_file(const char *filename, const char *mode, bool throw_on_fail= true)

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      $expect([]() { base::FileHandle test_file_scoped(TEST_FILE_NAME03, "r"); }).toThrow();

      // Testing FileHandle c-tor
      $expect([]() {
        base::FileHandle test_file03(TEST_FILE_NAME03, "r", false);
        $expect(test_file03.file()).toBe(nullptr);
      }).Not.toThrow();

      $expect([]() {
        base::FileHandle test_file04(TEST_FILE_NAME04, "w+");
        $expect(test_file04.file()).Not.toBe(nullptr);
      }).Not.toThrow();

    }).Not.toThrow();

    // Clean leftover test files
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME02);
    base::remove(TEST_FILE_NAME04);
  });


  //  Testing FileHandle public API's
  // - swap(FileHandle &fh)
  // - file()
  // - dispose()
  // - open_file(const char *filename, const char *mode, bool throw_on_fail= true)
  // -- Read-only permissions --
  $it("FileHandle swap, file dispose, open_file on read only permissions", [&]() {
    // Miscellaneous variables
    FILE *test_result = NULL;
    FILE *expected_result = NULL;
    base::FileHandle test_file01(TEST_FILE_NAME01, "w+");
    base::FileHandle test_file02(TEST_FILE_NAME02, "w+");
    base::FileHandle test_file;

    std::string command_line;

    $expect([&]() {
      // Change file permissions to read-only
      command_line.clear();
      command_line.assign(READONLY);
      command_line.append(TEST_FILE_NAME01);

      system(command_line.c_str());

      command_line.clear();
      command_line.assign(READONLY);
      command_line.append(TEST_FILE_NAME02);

      system(command_line.c_str());
    }).Not.toThrow();

    $expect([&]() {
      // Testing swap(FileHandle &fh)
      expected_result = test_file02.file();
      test_file01.swap(test_file02);
      test_result = test_file01.file();

      $expect(test_result).toBe(expected_result);

      // Testing file()
      $expect(test_file01.file()).Not.toBe(nullptr);

      // Testing dispose()
      test_file01.dispose();
      test_result = test_file01.file();

      $expect(test_result).toBe(nullptr);

      // Testing open_file(const char *filename, const char *mode, bool throw_on_fail= true)

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      $expect([]() { base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w"); }).toThrowError<std::exception>("^Failed to open file.*");

      // test case for 'throw_on_fail' FALSE value
      $expect([&]() {
        base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w", false);
        $expect(test_file.file()).toBe(nullptr);
      }).Not.toThrow();
    }).Not.toThrow();

    // Change back permissions to read-write
    $expect([&]() {
      // Change file permissions to read-write
      command_line.clear();
      command_line.assign(READWRITE);
      command_line.append(TEST_FILE_NAME01);

      system(command_line.c_str());

      command_line.clear();
      command_line.assign(READWRITE);
      command_line.append(TEST_FILE_NAME02);

      system(command_line.c_str());
    }).Not.toThrow();

    // Clean leftover test files
    test_file01.dispose();
    test_file02.dispose();
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME02);
  });

  // Testing FileHandle public API's
  // - open_file(const char *filename, const char *mode, bool throw_on_fail= true)
  // -- Corner/Limit Values --
  $it("open_file", [this]() {
    $expect([this]() {
      // Miscellaneous variables
      base::FileHandle test_file;

      // Testing open_file(const char *filename, const char *mode, bool throw_on_fail= true)

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Invalid name
      $expect([]() { base::FileHandle test_file_scoped(INVALID_NAME, "r"); }).toThrowError<std::exception>("^Failed to open file.*");

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Empty name
      $expect([]() { base::FileHandle test_file_scoped(EMPTY_NAME, "r"); }).toThrowError<std::exception>("^Failed to open file.*");

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Too long name
      $expect([this]() { base::FileHandle test_file_scoped(data->too_long_name.c_str(), "r"); }).toThrowError<std::exception>("^Failed to open file.*");

#ifdef _MSC_VER
      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Unicode name (file doesn't exist)
      $expect([this]() { base::FileHandle test_file_scoped(data->file_unicode_name.c_str(), "r"); }).toThrowError<std::exception>("^Failed to open file.*");
#endif

      // test case for 'throw_on_fail' FALSE value
      // -- Invalid name
      $expect([]() { base::FileHandle test_file_scoped(INVALID_NAME, "r", false); }).Not.toThrow();

      // test case for 'throw_on_fail' FALSE value
      // -- Empty name
      $expect([]() { base::FileHandle test_file_scoped(EMPTY_NAME, "r", false); }).Not.toThrow();

      // test case for 'throw_on_fail' FALSE value
      // -- Too long name
      $expect([this]() { base::FileHandle test_file_scoped(data->too_long_name.c_str(), "r", false); }).Not.toThrow();

      // test case for 'throw_on_fail' FALSE value
      // -- Unicode name
      $expect([this]() { base::FileHandle test_file_scoped(data->file_unicode_name.c_str(), "r", false); }).Not.toThrow();

      // test case for 'throw_on_fail' FALSE value
      // -- Reserved name
      $expect([]() { base::FileHandle test_file_scoped(RESERVED_NAME, "r", false); }).Not.toThrow();
    }).Not.toThrow();

    // Clean leftover test files
    base::remove(data->file_unicode_name);
  });

  // Testing FileHandle c-tors & d-tors
  $it("FileHandle ctors and dtors", [&]() {
    $expect([]() {
      // constructors & miscellaneous variables
      base::FileHandle c_tor_no_name;
      base::FileHandle c_tor_test_filename(TEST_FILE_NAME01, "w+");
      base::FileHandle c_tor_temp(TEST_FILE_NAME02, "w+");
      FILE *original_c_tor_temp_ptr = c_tor_temp.file();
      base::FileHandle *c_tor_FileHandle_ref = new base::FileHandle(c_tor_temp);

      // test case for empty name string
      FILE *test_result = c_tor_no_name.file();

      $expect(test_result).toBe(nullptr);

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      $expect([]() { base::FileHandle c_tor_throw_on_fail(TEST_FILE_NAME03, "r"); }).toThrow();

      // test case for 'throw_on_fail' FALSE value
      $expect([]() {
        // doesn't throw error (even if file does not exist)
        base::FileHandle c_tor_throw_on_fail(TEST_FILE_NAME03, "r", false);
        $expect(c_tor_throw_on_fail.file()).toBe(nullptr);
      }).Not.toThrow();

      // test case for FileHandle& value
      $expect(c_tor_FileHandle_ref->file()).toBe(original_c_tor_temp_ptr);

      // test case for d-tor
      c_tor_FileHandle_ref->~FileHandle();
      $expect(c_tor_FileHandle_ref->file()).toBe(nullptr);
    }).Not.toThrow();

    // Clean leftover test files
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME02);
  });

  // Testing FileHandle c-tors
  // -- Corner/Limit Values --
  $it("FileHandle ctors and dtors", [this]() {
    $expect([this]() {
      // Testing FileHandle(const char *filename, const char *mode, bool throw_on_fail= true)

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Invalid name
      $expect([]() { base::FileHandle c_tor(INVALID_NAME, "r"); }).toThrowError<std::exception>("^Failed to open file.*");

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Empty name
      $expect([]() { base::FileHandle c_tor(EMPTY_NAME, "r"); }).toThrowError<std::exception>("^Failed to open file.*");

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Too long name
      $expect([this]() { base::FileHandle c_tor(data->too_long_name.c_str(), "r"); }).toThrowError<std::exception>("^Failed to open file.*");
#ifdef _MSC_VER
      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Unicode name (file doesn't exist)
      $expect([]() { base::FileHandle c_tor(file_unicode_name.c_str(), "r"); }).toThrowError<std::exception>("^Failed to open file.*");
#endif
      // test case for 'throw_on_fail' FALSE value
      // -- Invalid name
      $expect([]() { base::FileHandle c_tor(INVALID_NAME, "r", false); }).Not.toThrow();

      // test case for 'throw_on_fail' FALSE value
      // -- Empty name
      $expect([]() { base::FileHandle c_tor(EMPTY_NAME, "r", false); }).Not.toThrow();

      // test case for 'throw_on_fail' FALSE value
      // -- Too long name
      $expect([this]() { base::FileHandle c_tor(data->too_long_name.c_str(), "r", false); }).Not.toThrow();

      // test case for 'throw_on_fail' FALSE value
      // -- Unicode name
      $expect([this]() { base::FileHandle c_tor(data->file_unicode_name.c_str(), "r", false); }).Not.toThrow();

      // test case for 'throw_on_fail' FALSE value
      // -- Reserved name
      $expect([]() { base::FileHandle c_tor(RESERVED_NAME, "r", false); }).Not.toThrow();
    }).Not.toThrow();

    // Clean leftover test files
    base::remove(data->file_unicode_name);
  });

  // Testing public API's
  // - file_exists(const std::string &path)
  // - is_directory(const std::string &path)
  // - dirname(const std::string &path)
  // - rename(const std::string &from, const std::string &to)
  // - extension(const std::string &path)
  // - basename(const std::string &path)
  // - strip_extension(const std::string &path)
  // - remove_recursive(const std::string &path)
  $it("FileHandle operating methods", [&]() {
    std::string command_line;
    std::string test_result;

    $expect([&]() {
      // Create subdirectory structure
      command_line.clear();
      command_line.assign(MKDIR);
      command_line.append(TEST_DIR_NAME01);
      command_line.append(FILE_SEPARATOR);
      command_line.append(TEST_DIR_NAME02);

      system(command_line.c_str());

      // Create directory
      command_line.clear();
      command_line.assign(MKDIR);
      command_line.append(TEST_DIR_NAME02);

      system(command_line.c_str());
    }).Not.toThrow();

    $expect([]() { base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w+"); }).Not.toThrow();

    $expect([&]() {
      // Testing file_exists(const std::string &path) with a directory
      $expect(base::file_exists(TEST_DIR_NAME01)).toBeTrue();

      // Testing file_exists(const std::string &path) with a file
      $expect(base::file_exists(TEST_FILE_NAME01)).toBeTrue();

      // Testing is_directory(const std::string &path)
      $expect(base::is_directory(TEST_DIR_NAME01)).toBeTrue();

      // Testing dirname(const std::string &path)
      test_result.clear();
      test_result = base::dirname(TEST_FILE_NAME01);
      $expect(test_result).toBe(".");

      test_result.clear();
      test_result = base::dirname(TEST_DIR_NAME01 FILE_SEPARATOR TEST_DIR_NAME02);
      $expect(test_result).toBe(TEST_DIR_NAME01);

      // Testing rename(const std::string &from, const std::string &to)
      base::rename(TEST_FILE_NAME01, TEST_FILE_NAME02);

      $expect(base::file_exists(TEST_FILE_NAME01)).toBeFalse();

      $expect(base::file_exists(TEST_FILE_NAME02)).toBeTrue();

      // Testing extension(const std::string &path)
      test_result.clear();
      test_result = base::extension(TEST_FILE_NAME02);
      $expect(test_result).toBe(".txt");

      // Testing basename(const std::string &path)
      test_result.clear();
      test_result = base::basename("." FILE_SEPARATOR TEST_FILE_NAME02);
      $expect(test_result).toBe(TEST_FILE_NAME02);

      // Testing strip_extension(const std::string &path)
      test_result.clear();
      test_result = base::strip_extension(TEST_FILE_NAME02);
      $expect(test_result).toBe(TEST_FILE_STRIPPED_NAME02);

      // Testing remove_recursive(const std::string &path)
      $expect(base::remove_recursive(TEST_DIR_NAME01)).toBeTrue();
    }).Not.toThrow();

    // Clean leftover test files
    base::remove_recursive(TEST_DIR_NAME01);
    base::remove_recursive(TEST_DIR_NAME02);
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME02);
  });

  // Testing public API's
  // - file_exists(const std::string &path)
  // - is_directory(const std::string &path)
  // - dirname(const std::string &path)
  // - rename(const std::string &from, const std::string &to)
  // - extension(const std::string &path)
  // - basename(const std::string &path)
  // - strip_extension(const std::string &path)
  // - remove_recursive(const std::string &path)
  // -- Corner/Limit Values --
  $it("FileHandle operating methods (corner tests)", [this]() {
    std::string command_line;
    std::string test_result;

    $expect([&]() {
      // Create subdirectory structure
      command_line.clear();
      command_line.assign(MKDIR);
      command_line.append(TEST_DIR_NAME01);
      command_line.append(FILE_SEPARATOR);
      command_line.append(TEST_DIR_NAME02);

      system(command_line.c_str());
    }).Not.toThrow();

    $expect([&]() { base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w+"); }).Not.toThrow();

    $expect([this]() {
      // Testing file_exists(const std::string &path)
      $expect(base::file_exists(INVALID_NAME)).toBeFalse();
      $expect(base::file_exists(EMPTY_NAME)).toBeFalse();
      $expect(base::file_exists(data->too_long_name.c_str())).toBeFalse();
      $expect(base::file_exists(data->file_unicode_name.c_str())).toBeFalse();

      // Testing is_directory(const std::string &path)
      $expect(base::is_directory(INVALID_NAME)).toBeFalse();
      $expect(base::is_directory(EMPTY_NAME)).toBeFalse();
      $expect(base::is_directory(data->too_long_name.c_str())).toBeFalse();
      $expect(base::is_directory(data->file_unicode_name.c_str())).toBeFalse();

#ifdef _MSC_VER
      $expect(base::is_directory(RESERVED_NAME)).toBeFalse();

      // Testing dirname(const std::string &path)
      $expect(base::dirname(INVALID_NAME)).toBe(".");
#endif

      $expect(base::dirname(EMPTY_NAME)).toBe(".");
      $expect(base::dirname(data->too_long_name.c_str())).toBe(".");
      $expect(base::dirname(data->file_unicode_name.c_str())).toBe(".");
      $expect(base::dirname(RESERVED_NAME)).toBe(".");

      // Testing rename(const std::string &from, const std::string &to)
      $expect([]() { base::rename(INVALID_NAME, TEST_FILE_NAME02); }).toThrowError<std::exception>("^Could not rename file.*");
      $expect([]() { base::rename(TEST_FILE_NAME01, INVALID_NAME); }).toThrowError<std::exception>("^Could not rename file.*");
      $expect([]() { base::rename(INVALID_NAME, INVALID_NAME); }).toThrowError<std::exception>("^Could not rename file.*");
      $expect([]() { base::rename(EMPTY_NAME, TEST_FILE_NAME02); }).toThrowError<std::exception>("^Could not rename file.*");
      $expect([]() { base::rename(TEST_FILE_NAME01, EMPTY_NAME); }).toThrowError<std::exception>("^Could not rename file.*");
      $expect([]() { base::rename(EMPTY_NAME, EMPTY_NAME); }).toThrowError<std::exception>("^Could not rename file.*");
      $expect([this]() { base::rename(data->too_long_name.c_str(), TEST_FILE_NAME02); }).toThrowError<std::exception>("^Could not rename file.*");
      $expect([this]() { base::rename(TEST_FILE_NAME01, data->too_long_name.c_str()); }).toThrowError<std::exception>("^Could not rename file.*");
      $expect([this]() { base::rename(data->too_long_name.c_str(), data->too_long_name.c_str()); }).toThrowError<std::exception>("^Could not rename file.*");
      $expect([]() { base::rename(RESERVED_NAME, TEST_FILE_NAME02); }).toThrowError<std::exception>("^Could not rename file.*");
      $expect([]() { base::rename(TEST_FILE_NAME01, RESERVED_NAME); }).toThrowError<std::exception>("^Could not rename file.*");
      $expect([]() { base::rename(RESERVED_NAME, RESERVED_NAME); }).toThrowError<std::exception>("^Could not rename file.*");

      base::rename(TEST_FILE_NAME01, data->file_unicode_name);

      $expect(base::file_exists(TEST_FILE_NAME01)).toBeFalse();
      $expect(base::file_exists(data->file_unicode_name)).toBeTrue();

      // -- Unicode name (change back from Unicode to ASCII)
      base::rename(data->file_unicode_name.c_str(), TEST_FILE_NAME01);

      $expect(base::file_exists(data->file_unicode_name)).toBeFalse();
      $expect(base::file_exists(TEST_FILE_NAME01)).toBeTrue();

      // -- Source File does not exist
      $expect([]() { base::rename(TEST_FILE_NAME03, TEST_FILE_NAME02); }).toThrowError<std::exception>("^Could not rename file.*");
#ifdef _MSC_VER
      // -- Target File already exists
      $expect([]() {
        // Create file
        base::FileHandle test_file_scoped(TEST_FILE_NAME03, "w+");
        base::rename(TEST_FILE_NAME01, TEST_FILE_NAME03);
      }).toThrowError<std::exception>("^Could not rename file.*");
#endif
      // -- Source & Target files are the same, and non-existing
      $expect([]() { base::rename(TEST_FILE_NAME02, TEST_FILE_NAME02); }).toThrowError<std::exception>("^Could not rename file.*");

      // -- Source & Target files are the same, and both exist
      base::rename(TEST_FILE_NAME01, TEST_FILE_NAME01);

      $expect(base::file_exists(TEST_FILE_NAME01)).toBeTrue();

      // Testing extension(const std::string &path)
      $expect(base::extension(INVALID_NAME)).toBe("");
      $expect(base::extension(EMPTY_NAME)).toBe("");
      $expect(base::extension(data->too_long_name.c_str())).toBe(".txt");
      $expect(base::extension(data->file_unicode_name.c_str())).toBe(".txt");

#ifdef _MSC_VER
      $expect(base::extension(RESERVED_NAME)).toBe("");
#endif

      $expect(base::extension("filename_no_ext")).toBe("");
      $expect(base::extension("filename_with_no_extension.")).toBe(".");
      $expect(base::extension(".txt")).toBe(".txt");
      $expect(base::extension("basename.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")).toBe(".xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

      // Testing strip_extension(const std::string &path)

      $expect(base::strip_extension(INVALID_NAME)).toBe(INVALID_NAME);
      $expect(base::strip_extension(EMPTY_NAME)).toBe(EMPTY_NAME);
      $expect(base::strip_extension(data->too_long_name)).toBe(data->too_long_basename);
      $expect(base::strip_extension(data->file_unicode_name)).toBe(data->file_unicode_basename);

#ifdef _MSC_VER
      $expect(base::strip_extension(RESERVED_NAME)).toBe(RESERVED_NAME);
#endif

      $expect(base::strip_extension("filename_with_no_extension")).toBe("filename_with_no_extension");
      $expect(base::strip_extension("filename_with_no_extension.")).toBe("filename_with_no_extension");
      $expect(base::strip_extension(".txt")).toBe("");
      $expect(base::strip_extension("basename.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")).toBe("basename");

      // Testing remove_recursive(const std::string &path)

      $expect(base::remove_recursive(INVALID_NAME)).toBeFalse();
      $expect(base::remove_recursive(EMPTY_NAME)).toBeFalse();
      $expect(base::remove_recursive(data->too_long_name)).toBeFalse();

      base::rename(TEST_DIR_NAME01, data->dir_unicode_name);

      $expect(base::remove_recursive(data->dir_unicode_name.c_str())).toBeTrue();
#ifdef _MSC_VER
      // -- Reserved name
      $expect(base::remove_recursive(RESERVED_NAME)).toBeFalse();
#endif
    }).Not.toThrow();


    // Clean leftover test files
    base::remove_recursive(TEST_DIR_NAME01);
    base::remove_recursive(data->dir_unicode_name);
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME02);
    base::remove(TEST_FILE_NAME03);
  });

#if 0
  // Testing public API's
  // - rename(const std::string &from, const std::string &to)
  // - remove(const std::string &path)
  // - remove_recursive(const std::string &path)
  // -- Read-only permissions --
#ifdef _MSC_VER
  TEST_FUNCTION(54) {
    std::string command_line;

    try {
      // Create subdirectory structure
      command_line.clear();
      command_line.assign(MKDIR);
      command_line.append(TEST_DIR_NAME01);
      command_line.append(FILE_SEPARATOR);
      command_line.append(TEST_DIR_NAME02);

      system(command_line.c_str());

      // Create directory
      command_line.clear();
      command_line.assign(MKDIR);
      command_line.append(TEST_DIR_NAME02);

      system(command_line.c_str());
    } catch (std::runtime_error &exc) {
      throw grt::os_error(strfmt("Runtime error: %s", exc.what()));
    }

    try {
      // Create file
      base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w+");

      // Change file permission to read-only
      command_line.clear();
      command_line.assign(READONLY);
      command_line.append(TEST_FILE_NAME01);

      system(command_line.c_str());

      // Change directories permission to read-only
      command_line.clear();
      command_line.assign(READONLY);
      command_line.append(TEST_DIR_NAME01);

      system(command_line.c_str());

      command_line.clear();
      command_line.assign(READONLY);
      command_line.append(TEST_DIR_NAME02);

      system(command_line.c_str());
    } catch (base::file_error &exc) {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
    }

    try {
      // Rename a read-only file
      base::rename(TEST_FILE_NAME01, TEST_FILE_NAME02);

      if (base::file_exists(TEST_FILE_NAME01)) {
        // return true means old file still exists
        fail(strfmt("TEST 54.1: File \"%s\" still exists", TEST_FILE_NAME01));
      }

      if (!base::file_exists(TEST_FILE_NAME02)) {
        // return false means new file does not exist
        fail(strfmt("TEST 54.1: File \"%s\" does not exist", TEST_FILE_NAME02));
      }

      // Remove a read-only directory
      try {
        bool result = base::remove(TEST_DIR_NAME02);
        fail(strfmt("TEST 54.2: Read-only directory \"%s\" did not throw an error", TEST_DIR_NAME02));
      } catch (const base::file_error &exc) {
        if (0 != std::string(exc.what()).find("Could not delete file ")) {
          fail(
               strfmt("TEST 54.2: Read-only directory \"%s\" threw an unexpected error: %s", TEST_DIR_NAME02, exc.what()));
        }
      } catch (std::exception &exc) {
        fail(strfmt("TEST 54.2: Read-only directory \"%s\" threw an unexpected error: %s", TEST_DIR_NAME02, exc.what()));
      }

      // Remove a read-only file
      try {
        base::remove(TEST_FILE_NAME02);
        fail(strfmt("TEST 54.3: Read-only file \"%s\" did not throw an error", TEST_FILE_NAME02));
      } catch (const base::file_error &exc) {
        if (0 != std::string(exc.what()).find("Could not delete file ")) {
          fail(strfmt("TEST 54.3: Read-only file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME02, exc.what()));
        }
      } catch (std::exception &exc) {
        fail(strfmt("TEST 54.3: Read-only file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME02, exc.what()));
      }

      // Recursively remove a read-only directory structure
      if (!base::remove_recursive(TEST_DIR_NAME01)) {
        // return false means dir does not exist
        fail(strfmt("TEST 54.4: Directory \"%s\" does not exist", TEST_DIR_NAME01));
      }
    } catch (base::file_error &exc) {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
    }

    // Change back permissions to read-write
    try {
      // Change file permission to read-write
      command_line.clear();
      command_line.assign(READWRITE);
      command_line.append(TEST_FILE_NAME02);

      system(command_line.c_str());

      // Change directories permission to read-only
      command_line.clear();
      command_line.assign(READWRITE);
      command_line.append(TEST_DIR_NAME01);

      system(command_line.c_str());

      command_line.clear();
      command_line.assign(READWRITE);
      command_line.append(TEST_DIR_NAME02);

      system(command_line.c_str());
    } catch (base::file_error &exc) {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
    }

    // Clean leftover test files
    base::remove_recursive(TEST_DIR_NAME01);
    base::remove_recursive(TEST_DIR_NAME02);
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME02);
  }
#endif
#endif

  //  Testing public API
  // - scan_for_files_matching(const std::string &pattern,
  $it("scan_for_files_matching", [&]() {
    std::string command_line;

    $expect([&]() {
      // Create subdirectory structure
      command_line.clear();
      command_line.assign(MKDIR);
      command_line.append(TEST_DIR_NAME01);
      command_line.append(FILE_SEPARATOR);
      command_line.append(TEST_DIR_NAME02);

      system(command_line.c_str());

      // Create directory
      command_line.clear();
      command_line.assign(MKDIR);
      command_line.append(TEST_DIR_NAME02);

      system(command_line.c_str());
    }).Not.toThrow();

    $expect([]() {
      // Create files
      base::FileHandle test_file01(TEST_FILE_NAME01, "w+");
      base::FileHandle test_file02(TEST_FILE_NAME02, "w+");
    }).Not.toThrow();

    // test case for 'recursive' default value (i.e. FALSE)
    $expect([]() {
      std::string search_pattern = "." FILE_SEPARATOR FILE_PATTERN "*";
      std::list<std::string> test_result = base::scan_for_files_matching(search_pattern);

      $expect(test_result.empty()).toBeFalse();
      $expect(test_result.size()).toBe(4U);

      while (!test_result.empty()) {
        $expect(test_result.front().find(FILE_PATTERN)).Not.toBe(std::string::npos);
        test_result.pop_front();
      }
    }).Not.toThrow();

    // test case for 'recursive' TRUE value
    $expect([]() {
      std::string search_pattern = "." FILE_SEPARATOR FILE_PATTERN "*";
      std::list<std::string> test_result = base::scan_for_files_matching(search_pattern, true);

      $expect(test_result.empty()).toBeFalse();
      $expect(test_result.size()).toBe(5U);

      while (!test_result.empty()) {
        $expect(test_result.front().find(FILE_PATTERN)).Not.toBe(std::string::npos);
        test_result.pop_front();
      }
    }).Not.toThrow();

    // Clean leftover test files
    base::remove_recursive(TEST_DIR_NAME01);
    base::remove_recursive(TEST_DIR_NAME02);
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME02);
  });

  // Testing file_locked_error public API's
  // - c-tors
  $it("file_locked_error - ctors", [&]() {
    $expect([]() {
      // ml: very questionable what is tested here.
      // test cases for constructors
      $expect([]() {
        throw base::file_locked_error("File Locked Error Message");
      }).toThrowError<base::file_locked_error>("File Locked Error Message");

      $expect([]() {
        base::file_locked_error first_error("File Locked Error Message");
        throw base::file_locked_error(first_error);
      }).toThrowError<base::file_locked_error>("File Locked Error Message");
    }).Not.toThrow();
  });

  // Testing LockFile public API's
  // - c-tor
  // - d-tor
  // - check(const std::string &path)
  $it("LockFile - ctors, dtors and check", [&]() {
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME02);

    // test cases for constructor, check(const std::string &path)
    // with Status == LockedSelf and Status == NotLocked
    // and destructor
    $expect([]() {
      {
        base::LockFile lock_file01(TEST_FILE_NAME01);
        base::FileHandle test_file02(TEST_FILE_NAME02, "w+");

        $expect(base::LockFile::check(TEST_FILE_NAME01)).toBe(base::LockFile::LockedSelf);
#ifndef _MSC_VER
        // Semantic issue with NotLocked for a plain file without content.
        // TODO: rework lock detection with other than base::LockFile instances.
        $expect(base::LockFile::check(TEST_FILE_NAME02)).toBe(base::LockFile::NotLocked);
#endif
      }
      $expect(base::file_exists(TEST_FILE_NAME01)).toBeFalse();
    }).Not.toThrow();


    // Clean leftover test files
    base::remove(TEST_FILE_NAME01);
    base::remove(TEST_FILE_NAME02);
  });

#if 0
#ifdef _MSC_VER
  // Child thread function
  gpointer _child_thread_func(gpointer data) {
    // Miscellaneous variables
    FILE *test_result = NULL;
    FILE *expected_result = NULL;
    base::FileHandle test_file;

    //  If the tread thows an exception it will SIGABRT the process. So we'll catch the exception
    //  and return gracefully.
    try {
      // Testing rename(const std::string &from, const std::string &to)
      try {
        base::rename(TEST_FILE_NAME01, TEST_FILE_NAME02);
        fail(strfmt("TEST 70.1: Locked file \"%s\" did not throw an error", TEST_FILE_NAME01));
      } catch (const base::file_error &exc) {
        if (0 != std::string(exc.what()).find("Could not rename file ")) {
          fail(strfmt("TEST 70.1: Locked file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME01, exc.what()));
        }
      } catch (std::exception &exc) {
        fail(strfmt("TEST 70.1: Locked file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME01, exc.what()));
      }

      // Testing remove(const std::string &path)
      try {
        base::remove(TEST_FILE_NAME01);
        fail(strfmt("TEST 70.2: Locked file \"%s\" did not throw an error", TEST_FILE_NAME01));
      } catch (const base::file_error &exc) {
        if (0 != std::string(exc.what()).find("Could not delete file ")) {
          fail(strfmt("TEST 70.2: Locked file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME01, exc.what()));
        }
      } catch (std::exception &exc) {
        fail(strfmt("TEST 70.2: Locked file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME01, exc.what()));
      }

      // Testing open_file(const char *filename, const char *mode, bool throw_on_fail= true)

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      try {
        // throw error (file is locked)
        base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w");
        fail(strfmt("TEST 70.3: Locked file \"%s\" did not throw an error", TEST_FILE_NAME01));
      } catch (const base::file_error &exc) {
        if (0 != std::string(exc.what()).find("Failed to open file \"")) {
          fail(strfmt("TEST 70.3: Locked file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME01, exc.what()));
        }
      } catch (std::exception &exc) {
        fail(strfmt("TEST 70.3: Locked file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME01, exc.what()));
      }

      // test case for 'throw_on_fail' FALSE value
      try {
        // doesn't throw error (even if file is locked)
        base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w", false);
        test_result = test_file.file();
        expected_result = NULL;

        ensure_equals("TEST 70.4: Unexpected result calling FileHandle c-tor", test_result, expected_result);
      } catch (std::exception &exc) {
        fail(strfmt("TEST 70.4: Read-only file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME01, exc.what()));
      }

      // Testing FileHandle c-tor
      try {
        base::FileHandle c_tor_test_filename(TEST_FILE_NAME01, "w+");
        fail(strfmt("TEST 70.5: Locked file \"%s\" did not throw an error", TEST_FILE_NAME01));
      } catch (const base::file_error &exc) {
        if (0 != std::string(exc.what()).find("Failed to open file \"")) {
          fail(strfmt("TEST 70.5: Locked file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME01, exc.what()));
        }
      } catch (std::exception &exc) {
        fail(strfmt("TEST 70.5: Locked file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME01, exc.what()));
      }

      // Testing LockFile c-tor
      try {
        base::LockFile lock_file(TEST_FILE_NAME01);
        fail(strfmt("TEST 70.6: Locked file \"%s\" did not throw an error", TEST_FILE_NAME01));
      } catch (const base::file_locked_error) {
        // Nothing to do, just catch the error and continue
      } catch (std::exception &exc) {
        fail(strfmt("TEST 70.6: Locked file \"%s\" threw an unexpected error: %s", TEST_FILE_NAME01, exc.what()));
      }

      // Testing LockFile::check in child thread
      try {
        if (LockFile::check(TEST_FILE_NAME01) != LockFile::LockedSelf)
          fail(strfmt("TEST 70.7: File \"%s\" not locked", TEST_FILE_NAME01));
      } catch (base::file_error &exc) {
        throw grt::os_error(strfmt("File error: %s", exc.what()));
      } catch (std::invalid_argument &exc) {
        throw grt::os_error(strfmt("Invalid argument error: %s", exc.what()));
      } catch (std::runtime_error &exc) {
        throw grt::os_error(strfmt("Runtime/file-locked error: %s", exc.what()));
      }
    } catch (std::exception &exc) {
      fail(exc.what());
    }
    return NULL;
  }

  // Testing public API's
  // - rename(const std::string &from, const std::string &to)
  // - remove(const std::string &path)
  // Testing FileHandle public API's
  // - open_file(const char *filename, const char *mode, bool throw_on_fail= true)
  // Testing FileHandle c-tor
  // -- Multi-threading locking --
  TEST_FUNCTION(70) {
    // Miscellaneous variables
    GThread *_child_thread;

    try {
      // Create file
      base::FileHandle test_file01(TEST_FILE_NAME01, "w+");
    } catch (base::file_error &exc) {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
    }

    // Lock the file
    base::LockFile lock_file(TEST_FILE_NAME01);

    // Testing LockFile::check in main thread
    try {
      if (LockFile::check(TEST_FILE_NAME01) != LockFile::LockedSelf)
        fail(strfmt("TEST 70.8: File \"%s\" not locked", TEST_FILE_NAME01));
    } catch (base::file_error &exc) {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
    } catch (std::invalid_argument &exc) {
      throw grt::os_error(strfmt("Invalid argument error: %s", exc.what()));
    } catch (std::runtime_error &exc) {
      throw grt::os_error(strfmt("Runtime/file-locked error: %s", exc.what()));
    }

    // Kick off the child thread
    _child_thread = base::create_thread(_child_thread_func, this);
    // Wait for _main_thread to finish
    g_thread_join(_child_thread);
  }
#endif
#endif

  $it("Tests for relativePath", [&]() {
    // .
    $expect(base::relativePath("", "")).toBe("");
    $expect(base::relativePath("/", "")).toBe("");
    $expect(base::relativePath("", "/")).toBe("/");
    $expect(base::relativePath("", "\\")).toBe("\\");
    $expect(base::relativePath("/////////////////////////", "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\")).toBe("../../../../../../../../../../");
    $expect(base::relativePath("/abc/def", "\\abc\\def")).toBe("");
    $expect(base::relativePath("/abc/def", "/abc/def/ghi")).toBe("ghi");
    $expect(base::relativePath("/abc/def/ghi", "/abc/def/")).toBe("../");

    // Long names without sub paths.
    std::string basePath(50000, 'x');
    std::string pathToMakeRelative = "y";

    $expect(base::relativePath(basePath, pathToMakeRelative)).toBe(pathToMakeRelative);
    $expect(base::relativePath(pathToMakeRelative, basePath)).toBe(basePath);

    // Many (short) subpaths.
    basePath = "";
    for (size_t i = 0; i < 30000; ++i)
      basePath += "/abc";
    $expect(base::relativePath(basePath, "")).toBe("");
    $expect(base::relativePath(basePath, "abc")).toBe("abc");
    $expect(base::relativePath(basePath, basePath)).toBe("");

    pathToMakeRelative = "";
    for (size_t i = 0; i < 29999; ++i)
      pathToMakeRelative += "../";
    $expect(base::relativePath(basePath, "/abc")).toBe(pathToMakeRelative);

    $expect(base::relativePath("🍏🍎🍐/ЀЁЂ/ᚋᚌᚍ/last", "/last")).toBe("/last");
    $expect(base::relativePath("/🍏🍎🍐/ЀЁЂ\\ᚋᚌᚍ\\last", "\\last")).toBe("../../../../last");
    $expect(base::relativePath("🍏🍎🍐/ЀЁЂ\\ᚋᚌᚍ/last", "🍏🍎🍐")).toBe("../../../");

    // Case sensitivity.
#ifdef _MSC_VER
    $expect(base::relativePath("/🍏🍎🍐/ЀЁЂ\\ᚋᚌᚍ\\last", "\\Last").toBe("../../../../Last");
    $expect(base::relativePath("/XYZ/🍏🍎🍐/ЀЁЂ\\ᚋᚌᚍ\\last", "\\xyz\\Last").toBe("../../../../Last");
#else
    $expect(base::relativePath("/🍏🍎🍐/ЀЁЂ\\ᚋᚌᚍ\\last", "\\Last")).toBe("../../../../Last");
    $expect(base::relativePath("/XYZ/🍏🍎🍐/ЀЁЂ\\ᚋᚌᚍ\\last", "\\xyz\\Last")).toBe("../../../../../xyz/Last");
#endif

    // Win specific, but nonetheless working on any platform.
    $expect(base::relativePath("C:\\abc/def/ghi", "C:/abc/def/")).toBe("../");
    $expect(base::relativePath("C:\\abc/def/ghi", "D:/abc/def/")).toBe("D:/abc/def/");
    });
}

}
