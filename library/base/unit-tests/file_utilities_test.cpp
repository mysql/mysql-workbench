/* 
 * Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "base/threading.h"
#include "wb_helpers.h"

using namespace base;

#ifdef _WIN32
#define FILE_SEPARATOR    "\\"
#define INVALID_NAME      "__test_file01*?"
#define RESERVED_NAME     "com1"
#define MKDIR             "mkdir "
#define READONLY          "attrib +R "
#define READWRITE         "attrib -R "
#else
#define FILE_SEPARATOR    "/"
#define INVALID_NAME      "__test_file01/?"
#define RESERVED_NAME     "."
#define MKDIR             "mkdir -p "
#define READONLY          "chmod 0444 "
#define READWRITE         "chmod 0777 "
#endif

#define TEST_DIR_NAME01  "__test_dir01"
#define TEST_DIR_NAME02  "__test_dir02"
#define TEST_FILE_NAME01 "__test_file01.txt"
#define TEST_FILE_NAME02 "__test_file02.txt"
#define TEST_FILE_NAME03 "__test_file03.txt"
#define TEST_FILE_NAME04 "__test_file04.txt"
#define TEST_FILE_STRIPPED_NAME02 "__test_file02"

#define FILE_PATTERN "__test_"
#define EMPTY_NAME ""

/*
 * Some tests on Windows specific behavior (like reserved file names, file sharing violation exceptions, etc) 
 * are desabled in other platforms
 * */

BEGIN_TEST_DATA_CLASS(file_utilities_test)

  protected:
    std::string too_long_name;
    std::string too_long_basename;
    std::string dir_unicode_name;
    std::string file_unicode_name;
    std::string file_unicode_basename;

TEST_DATA_CONSTRUCTOR(file_utilities_test)
{
  unsigned int i;

  dir_unicode_name.clear();
  dir_unicode_name += "__test_dir_";
  dir_unicode_name += "\xE3\x8A\xA8"; // Valid Unicode character.

  file_unicode_basename.clear();
  file_unicode_basename += "__test_file_";
  file_unicode_basename += "\xE3\x8F\xA3"; // Valid Unicode character.

  file_unicode_name.clear();
  file_unicode_name = file_unicode_basename;
  file_unicode_name += ".txt";

  too_long_basename.clear();
  for (i=0; i < 1000 ;i++)
      too_long_basename.append("x");

  too_long_name.clear();
  too_long_name = too_long_basename;
  too_long_name += ".txt";

  // Clean up any existing test directories/files
  remove_recursive(TEST_DIR_NAME01);
  remove_recursive(TEST_DIR_NAME02);
  remove(TEST_FILE_NAME01);
  remove(TEST_FILE_NAME02);
  remove(TEST_FILE_NAME03);
  remove(TEST_FILE_NAME04);
  remove(dir_unicode_name);
  remove(file_unicode_name);

  // Initialize the thread system
  if (!g_thread_supported ()) 
    base::threading_init();
}

TEST_DATA_DESTRUCTOR(file_utilities_test)
{

  // Clean up any remaining test directories/files
  remove_recursive(TEST_DIR_NAME01);
  remove_recursive(TEST_DIR_NAME02);
  remove(TEST_FILE_NAME01);
  remove(TEST_FILE_NAME02);
  remove(TEST_FILE_NAME03);
  remove(TEST_FILE_NAME04);
  remove(dir_unicode_name);
  remove(file_unicode_name);

}

END_TEST_DATA_CLASS;

TEST_MODULE(file_utilities_test, "file_utilities");

/*
 * Testing public API's
 * - create_directory(const std::string &path, int mode)
 * - remove(const std::string &path)
 */
TEST_FUNCTION(5)
{
    // Remove a non existing directory
    try
    {
      if (base::remove(TEST_DIR_NAME01))
      {
          // return true means dir already exists
          fail(strfmt("TEST 5.1: Directory \"%s\" already exists",TEST_DIR_NAME01));
      }
    }
    catch (base::file_error &exc)
    {
      throw grt::os_error(strfmt("Cannot remove directory for document: %s", exc.what()));
    }

    // Create a non existing directory
    try
    {
      if (!(base::create_directory(TEST_DIR_NAME01, 0700)))
      {
          // return false means dir already exists
          fail(strfmt("TEST 5.2: Directory \"%s\" already exists",TEST_DIR_NAME01));
      }
    }
    catch (base::file_error &exc)
    {
      throw grt::os_error(strfmt("Cannot create directory for document: %s", exc.what()));
    }

    // Create an already existing directory
    try
    {
      if (base::create_directory(TEST_DIR_NAME01, 0700))
      {
          // return true means dir does not exist
          fail(strfmt("TEST 5.3: Directory \"%s\" does not exist",TEST_DIR_NAME01));
      }
    }
    catch (base::file_error &exc)
    {
      throw grt::os_error(strfmt("Cannot create directory for document: %s", exc.what()));
    }

    // Remove an existing directory
    try
    {
      if (!base::remove(TEST_DIR_NAME01))
      {
          // return false means dir does not exist
          fail(strfmt("TEST 5.4: Directory \"%s\" does not exist",TEST_DIR_NAME01));
      }
    }
    catch (base::file_error &exc)
    {
      throw grt::os_error(strfmt("Cannot remove directory for document: %s", exc.what()));
    }

    // Remove a non existing file
    try
    {
      if (base::remove(TEST_FILE_NAME01))
      {
          // return true means file already exists
          fail(strfmt("TEST 5.5: File \"%s\" already exists",TEST_FILE_NAME01));
      }
    }
    catch (base::file_error &exc)
    {
      throw grt::os_error(strfmt("Cannot remove file for document: %s", exc.what()));
    }

    // Remove an existing file
    try
    {
      base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w+"); // Create file
      test_file_scoped.dispose(); // Close file

      if (!base::remove(TEST_FILE_NAME01))
      {
          // return false means file does not exist
          fail(strfmt("TEST 5.6: File \"%s\" does not exist",TEST_FILE_NAME01));
      }
    }
    catch (base::file_error &exc)
    {
      throw grt::os_error(strfmt("Cannot remove file for document: %s", exc.what()));
    }
}

/*
 * Testing public API's
 * - create_directory(const std::string &path, int mode)
 * - remove(const std::string &path)
 * -- Corner/Limit Values -- 
*/
TEST_FUNCTION(10)
{
    // Create a directory -- Invalid name
    try
    {
      base::create_directory(INVALID_NAME, 0700);
      fail(strfmt("TEST 10.1: Directory name \"%s\" did not throw an error",INVALID_NAME));
    }
    catch (const base::file_error &exc)
    {
        if (0 != std::string(exc.what()).find("Could not create directory "))
        {
          fail(strfmt("TEST 10.1: Directory name \"%s\" threw an unexpected error: %s",
              INVALID_NAME, exc.what()));
        }
    }
    catch(std::exception &exc)
    {
      fail(strfmt("TEST 10.1: Directory name \"%s\" threw an unexpected error: %s",
          INVALID_NAME, exc.what()));
    }

    // Create a directory -- Empty name
    try
    {
      base::create_directory(EMPTY_NAME, 0700);
      fail(strfmt("TEST 10.2: Directory name \"%s\" did not throw an error",EMPTY_NAME));
    }
    catch (const base::file_error &exc)
    {
        if (0 != std::string(exc.what()).find("Could not create directory "))
        {
          fail(strfmt("TEST 10.2: Directory name \"%s\" threw an unexpected error: %s",
              EMPTY_NAME, exc.what()));
        }
    }
    catch(std::exception &exc)
    {
      fail(strfmt("TEST 10.2: Directory name \"%s\" threw an unexpected error: %s",
          EMPTY_NAME, exc.what()));
    }

    // Create a directory -- Too long name
    try
    {
      base::create_directory(too_long_name, 0700);
      fail("TEST 10.3: Directory with too long name did not throw an error");
    }
    catch (const base::file_error &exc)
    {
        if (0 != std::string(exc.what()).find("Could not create directory "))
        {
          fail(strfmt("TEST 10.3: Directory with too long name threw an unexpected error: %s",exc.what()));
        }
    }
    catch(std::exception &exc)
    {
      fail(strfmt("TEST 10.3: Directory with too long name threw an unexpected error: %s",exc.what()));
    }

    // Create a directory -- Unicode name
    try
    {
      if (!(base::create_directory(dir_unicode_name, 0700)))
      {
          // return false means dir already exists
          fail(strfmt("TEST 10.4: Directory \"%s\" already exists",dir_unicode_name.c_str()));
      }
    }
    catch (base::file_error &exc)
    {
      throw grt::os_error(strfmt("Cannot create directory for document: %s", exc.what()));
    }

#ifdef _WIN32
    // Create a directory -- Reserved name. This will not fail in linux, returning false (already exists)
    try
    {
      base::create_directory(RESERVED_NAME, 0700);
      fail(strfmt("TEST 10.5: Directory name \"%s\" did not throw an error",RESERVED_NAME));
    }
    catch (const base::file_error &exc)
    {
        if (0 != std::string(exc.what()).find("Could not create directory "))
        {
          fail(strfmt("TEST 10.5: Directory name \"%s\" threw an unexpected error: %s",
              RESERVED_NAME, exc.what()));
        }
    }
    catch(std::exception &exc)
    {
      fail(strfmt("TEST 10.5: Directory name \"%s\" threw an unexpected error: %s",
          RESERVED_NAME, exc.what()));
    }
#endif

    // Create a file -- Unicode name
    try
    {
      base::FileHandle test_file_scoped(file_unicode_name.c_str(), "w+"); // Create file
      if (!file_exists(file_unicode_name))
      {
          // return false means file does not exist
          fail(strfmt("TEST 10.6: File creation of \"%s\" failed",file_unicode_name.c_str()));
      }
    }
    catch (base::file_error &exc)
    {
      throw grt::os_error(strfmt("Cannot create file for document: %s", exc.what()));
    }
#ifdef _WIN32
    // Remove a file/directory -- Invalid name
    try
    {
      base::remove(INVALID_NAME);
      fail(strfmt("TEST 10.7: Directory name \"%s\" did not throw an error",INVALID_NAME));

    }
    catch (const base::file_error &exc)
    {
        if (0 != std::string(exc.what()).find("Could not delete file "))
        {
          fail(strfmt("TEST 10.7: Directory name \"%s\" threw an unexpected error: %s",
              INVALID_NAME, exc.what()));
        }
    }
    catch(std::exception &exc)
    {
      fail(strfmt("TEST 10.7: Directory name \"%s\" threw an unexpected error: %s",
          INVALID_NAME, exc.what()));
    }

    // Remove a file/directory -- Empty name
    try
    {
      base::remove(EMPTY_NAME);
      fail(strfmt("TEST 10.8: File/Directory name \"%s\" did not throw an error",EMPTY_NAME));
    }
    catch (const base::file_error &exc)
    {
        if (0 != std::string(exc.what()).find("Could not delete file "))
        {
          fail(strfmt("TEST 10.8: Directory name \"%s\" threw an unexpected error: %s",
              EMPTY_NAME, exc.what()));
        }
    }
    catch(std::exception &exc)
    {
      fail(strfmt("TEST 10.8: Directory name \"%s\" threw an unexpected error: %s",
          EMPTY_NAME, exc.what()));
    }
#endif

    // Remove a file/directory -- Too long name
    try
    {
      base::remove(too_long_name);
      fail("TEST 10.9: File/Directory with too long name did not throw an error");
    }
    catch (const base::file_error & exc)
    {
        if (0 != std::string(exc.what()).find("Could not delete file "))
        {
          fail(strfmt("TEST 10.9: File/Directory with too long name threw an unexpected error: %s",exc.what()));
        }
    }
    catch(std::exception &exc)
    {
      fail(strfmt("TEST 10.9: File/Directory with too long name threw an unexpected error: %s",exc.what()));
    }

    // Remove an existing directory -- Unicode name
    try
    {
      if (!base::remove(dir_unicode_name))
      {
          // return false means dir does not exist
          fail(strfmt("TEST 10.10: Directory \"%s\" does not exist",dir_unicode_name.c_str()));
      }
    }
    catch (base::file_error &exc)
    {
      throw grt::os_error(strfmt("Cannot remove directory for document: %s", exc.what()));
    }

    // Remove an existing file -- Unicode name
    try
    {
      if (!base::remove(file_unicode_name))
      {
          // return false means file does not exist
          fail(strfmt("TEST 10.11: File \"%s\" does not exist",file_unicode_name.c_str()));
      }
    }
    catch (base::file_error &exc)
    {
      throw grt::os_error(strfmt("Cannot remove file for document: %s", exc.what()));
    }

    // Remove a file/directory -- Reserved name
    //try
    //{
    //  if (base::remove(RESERVED_NAME))
    //  {
    //      // return true means dir exists
    //      fail(strfmt("TEST 10.12: Directory \"%s\" exists",dir_unicode_name));
    //  }
    //}
    //catch (base::file_error &exc)
    //{
    //  throw grt::os_error(strfmt("Cannot remove directory for document: %s", exc.what()));
    //}

  // Clean leftover test files
  base::remove(dir_unicode_name);
  base::remove(file_unicode_name);
}

/*
 * Testing file_error public API's
 * - c-tor
 * - code()
 * - sys_code()
 */
TEST_FUNCTION(15)
{
  // Miscellaneous variables
  error_code test_result;
  error_code expected_result;
  //int int_test_result;
  //int int_expected_result;

  try
  {
      // Testing c-tor
      base::file_error c_tor_file_error("Error Test", 0);

      // Testing code()
      test_result = c_tor_file_error.code();
      expected_result = success;

      if(test_result != expected_result)
      {
        fail("TEST 15.1: Unexpected result calling function code()");
      }

      // Testing sys_code()
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // NOTE: THIS FUNCTION IS NOT IMPLEMENTED YET!!!!
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

      //int_test_result = c_tor_file_error.sys_code();
      //int_expected_result = 0;

      //if(int_test_result != int_expected_result)
      //{
      //  fail("TEST 15.2: Unexpected result calling function sys_code()");
      //}
  }
  catch (...)
  {
      throw grt::os_error(strfmt("Exception error"));
  }
}

/*
 * Testing file_error public API's
 * - c-tor
 * -- Corner/Limit Values -- 
 */
TEST_FUNCTION(20)
{
  try
  {
      // Testing c-tor -- Empty string
      base::file_error c_tor_file_error_empty(EMPTY_NAME, 0);

      // Testing c-tor -- Unicode string
      base::file_error c_tor_file_error_unicode(file_unicode_name, 0);
  }
  catch (...)
  {
      throw grt::os_error(strfmt("Exception error"));
  }
}

/*
 * Testing FileHandle public API's (operators)
 * - operator FILE *()
 * - operator bool()
 * - operator =(FileHandle &fh)
 * - operator ->()
 */
TEST_FUNCTION(25)
{
  try
  {
      // Miscellaneous variables
      base::FileHandle test_file01(TEST_FILE_NAME01, "w+");
      base::FileHandle test_file03(TEST_FILE_NAME03, "w+");
      base::FileHandle test_file04;

      FILE* test_result = NULL;
      FILE* expected_result = NULL;

      // Testing overridden operator bool()
      if (test_file01.operator bool())
      {
        fail("TEST 25.2: Unexpected result calling operator bool()");
      }

      test_file01.dispose();

      if (!test_file01.operator bool())
      {
        fail("TEST 25.3: Unexpected result calling operator bool()");
      }

      // Testing overridden operator =(FileHandle &fh)
      expected_result = test_file03.file();
      test_file04 = test_file03;
      test_result = test_file04.file();

      ensure_equals("TEST 25.4: Unexpected result calling operator =(FileHandle &fh)",test_result,expected_result);
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Clean leftover test files
  base::remove(TEST_FILE_NAME01);
  base::remove(TEST_FILE_NAME03);
}

/*
 * Testing FileHandle public API's
 * - swap(FileHandle &fh)
 * - file()
 * - dispose()
 */
TEST_FUNCTION(30)
{
  try
  {
      // Miscellaneous variables
      base::FileHandle test_file01(TEST_FILE_NAME01, "w+");
      base::FileHandle test_file02(TEST_FILE_NAME02, "w+");
      base::FileHandle test_file03;

      FILE* test_result = NULL;
      FILE* expected_result = NULL;

      // Testing swap(FileHandle &fh)
      expected_result = test_file02.file();
      test_file01.swap(test_file02);
      test_result = test_file01.file();

      ensure_equals("TEST 30.1: Unexpected result calling function swap(FileHandle &fh)",test_result,expected_result);

      // Testing file()
      ensure("TEST 30.2: Unexpected result calling function file()", test_file01.file() != NULL);

      // Testing dispose()
      test_file01.dispose();
      test_result = test_file01.file();
      expected_result = NULL;

      ensure_equals("TEST 30.3: Unexpected result calling function dispose()",test_result,expected_result);

      // Testing open_file(const char *filename, const char *mode, bool throw_on_fail= true)

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      try
      {
          // throw error (file does not exist)
          base::FileHandle test_file_scoped(TEST_FILE_NAME03, "r");
          fail("TEST 30.4: Unexpected result calling FileHandle c-tor");
      }
      catch(...)
      {
          //Nothing to do, just catch the error and continue
      }

      // Testing FileHandle c-tor
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle test_file03(TEST_FILE_NAME03, "r", false);
        test_result = test_file03.file();
        expected_result = NULL;

        ensure_equals("TEST 30.5: Unexpected result calling open_file()",test_result,expected_result);
      }
      catch(...)
      {
        fail("TEST 30.5: Unexpected error calling open_file()");
      }
      
      try
      {
        base::FileHandle test_file04(TEST_FILE_NAME04, "w+");
        test_result = test_file04.file();

        if(!test_result)
        {
          fail("TEST 30.6: Unexpected result calling FileHandle c-tor");
        }
      }
      catch(...)
      {
        fail("TEST 30.6: Unexpected error calling FileHandle c-tor");
      }

  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Clean leftover test files
  base::remove(TEST_FILE_NAME01);
  base::remove(TEST_FILE_NAME02);
  base::remove(TEST_FILE_NAME04);
}

/*
 * Testing FileHandle public API's
 * - swap(FileHandle &fh)
 * - file()
 * - dispose()
 * - open_file(const char *filename, const char *mode, bool throw_on_fail= true)
 * -- Read-only permissions -- 
 */
TEST_FUNCTION(33)
{
  // Miscellaneous variables
  FILE* test_result = NULL;
  FILE* expected_result = NULL;
  base::FileHandle test_file01(TEST_FILE_NAME01, "w+");
  base::FileHandle test_file02(TEST_FILE_NAME02, "w+");
  base::FileHandle test_file;

  std::string command_line;

  try
 {
      // Change file permissions to read-only
      command_line.clear();
      command_line.assign(READONLY);
      command_line.append(TEST_FILE_NAME01);

      system(command_line.c_str());
	
      command_line.clear();
      command_line.assign(READONLY);
      command_line.append(TEST_FILE_NAME02);

      system(command_line.c_str());
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  try
  {
      // Testing swap(FileHandle &fh)
      expected_result = test_file02.file();
      test_file01.swap(test_file02);
      test_result = test_file01.file();

      ensure_equals("TEST 33.1: Unexpected result calling function swap(FileHandle &fh)",test_result,expected_result);

      // Testing file()
      ensure("TEST 33.2: Unexpected result calling function file()", test_file01.file() != NULL);

      // Testing dispose()
      test_file01.dispose();
      test_result = test_file01.file();
      expected_result = NULL;

      ensure_equals("TEST 33.3: Unexpected result calling function dispose()",test_result,expected_result);

      // Testing open_file(const char *filename, const char *mode, bool throw_on_fail= true)

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      try
      {
          // throw error (file is read-only)
          base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w");
          fail(strfmt("TEST 33.4: Read-only file \"%s\" did not throw an error",TEST_FILE_NAME01));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 33.4: Read-only file \"%s\" threw an unexpected error: %s",
                  TEST_FILE_NAME01, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 33.4: Read-only file \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME01, exc.what()));
      }

      // test case for 'throw_on_fail' FALSE value
      try
      {
          // doesn't throw error (even if file is read-only)
          base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w", false);
          test_result = test_file.file();
          expected_result = NULL;

          if(test_result != expected_result)
          {
              fail("TEST 33.5: Unexpected result calling FileHandle c-tor");
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 33.5: Read-only file \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME01, exc.what()));
      }
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Change back permissions to read-write
  try
  {
      // Change file permissions to read-write
      command_line.clear();
      command_line.assign(READWRITE);
      command_line.append(TEST_FILE_NAME01);

      system(command_line.c_str());

      command_line.clear();
      command_line.assign(READWRITE);
      command_line.append(TEST_FILE_NAME02);

      system(command_line.c_str());
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Clean leftover test files
  test_file01.dispose();
  test_file02.dispose();
  base::remove(TEST_FILE_NAME01);
  base::remove(TEST_FILE_NAME02);
}

/*
 * Testing FileHandle public API's
 * - open_file(const char *filename, const char *mode, bool throw_on_fail= true)
 * -- Corner/Limit Values -- 
 */
TEST_FUNCTION(35)
{
  try
  {
          // Miscellaneous variables
      base::FileHandle test_file;

      // Testing open_file(const char *filename, const char *mode, bool throw_on_fail= true)

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Invalid name
      try
      {
          base::FileHandle test_file_scoped(INVALID_NAME, "r");
          fail(strfmt("TEST 35.1: File name \"%s\" did not throw an error",INVALID_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 35.1: File name \"%s\" threw an unexpected error: %s",
                  INVALID_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 35.1: File name \"%s\" threw an unexpected error: %s",
              INVALID_NAME, exc.what()));
      }
      
      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Empty name
      try
      {
          base::FileHandle test_file_scoped(EMPTY_NAME, "r");
          fail(strfmt("TEST 35.2: File name \"%s\" did not throw an error",EMPTY_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 35.2: File name \"%s\" threw an unexpected error: %s",
                  EMPTY_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 35.2: File name \"%s\" threw an unexpected error: %s",
              EMPTY_NAME, exc.what()));
      }

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Too long name
      try
      {
          base::FileHandle test_file_scoped(too_long_name.c_str(), "r");
          fail("TEST 35.3: File with too long name did not throw an error");
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 35.3: File with too long name threw an unexpected error: %s",exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 35.3: File with too long name threw an unexpected error: %s",exc.what()));
      }
      
#ifdef _WIN32
      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Unicode name (file doesn't exist)
      try
      {
          base::FileHandle test_file_scoped(file_unicode_name.c_str(), "r");
          fail("TEST 35.4: File with unicode name did not throw an error");

      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 35.4: File with unicode name threw an unexpected error: %s",exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 35.4: File with unicode name threw an unexpected error: %s",exc.what()));
      }
#endif
      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Reserved name
      //try
      //{
      //    test_file.open_file(RESERVED_NAME, "r");
      //    fail(strfmt("TEST 35.5: File name \"%s\" did not throw an error",RESERVED_NAME));
      //}
      //catch (const base::file_error &exc)
      //{
      //    if (0 != std::string(exc.what()).find("Failed to open file \""))
      //    {
      //        fail(strfmt("TEST 35.5: File name \"%s\" threw an unexpected error: %s",
      //            RESERVED_NAME, exc.what()));
      //    }
      //}
      //catch(std::exception &exc)
      //{
      //    fail(strfmt("TEST 35.5: File name \"%s\" threw an unexpected error: %s",
      //        RESERVED_NAME, exc.what()));
      //}

      // test case for 'throw_on_fail' FALSE value
      // -- Invalid name
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle test_file_scoped(INVALID_NAME, "r", false);
      }
      catch(...)
      {
        fail(strfmt("TEST 35.6: File name \"%s\" threw an error",INVALID_NAME));
      }

      // test case for 'throw_on_fail' FALSE value
      // -- Empty name
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle test_file_scoped(EMPTY_NAME, "r", false);
      }
      catch(...)
      {
        fail(strfmt("TEST 35.7: File name \"%s\" threw an error",EMPTY_NAME));
      }

      // test case for 'throw_on_fail' FALSE value
      // -- Too long name
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle test_file_scoped(too_long_name.c_str(), "r", false);
      }
      catch(...)
      {
        fail("TEST 35.8: File with too long name threw an error");
      }

      // test case for 'throw_on_fail' FALSE value
      // -- Unicode name
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle test_file_scoped(file_unicode_name.c_str(), "r", false);
      }
      catch(...)
      {
        fail("TEST 35.9: File with unicode name threw an error");
      }

      // test case for 'throw_on_fail' FALSE value
      // -- Reserved name
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle test_file_scoped(RESERVED_NAME, "r", false);
      }
      catch(...)
      {
        fail(strfmt("TEST 35.10: File name \"%s\" threw an error",INVALID_NAME));
      }

  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Clean leftover test files
  base::remove(file_unicode_name);

}

/*
 * Testing FileHandle c-tors & d-tors
 */
TEST_FUNCTION(40)
{
  try
  {
      // constructors & miscellaneous variables
      base::FileHandle c_tor_no_name;
      base::FileHandle c_tor_test_filename(TEST_FILE_NAME01, "w+");
      base::FileHandle c_tor_temp(TEST_FILE_NAME02, "w+");
      FILE* original_c_tor_temp_ptr = c_tor_temp.file();
      base::FileHandle c_tor_FileHandle_ref(c_tor_temp);

      // test case for empty name string
      FILE* test_result = c_tor_no_name.file();
      FILE* expected_result = NULL;

      ensure_equals("TEST 40.1: Unexpected result calling FileHandle constructor",test_result,expected_result);
  
      // test case for 'throw_on_fail' default value (i.e. TRUE)
      try
      {
        // throw error (file does not exist)
        base::FileHandle c_tor_throw_on_fail(TEST_FILE_NAME03, "r");
        fail("TEST 40.2: Unexpected result calling FileHandle constructor");
      }
      catch(...)
      {
          //Nothing to do, just catch the error and continue
      }

      // test case for 'throw_on_fail' FALSE value
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle c_tor_throw_on_fail(TEST_FILE_NAME03, "r", false);
        test_result = c_tor_throw_on_fail.file();
        expected_result = NULL;

        ensure_equals("TEST 40.3: Unexpected result calling FileHandle constructor",test_result,expected_result);
      }
      catch(...)
      {
        fail("TEST 40.3: Unexpected result calling FileHandle constructor");
      }

      // test case for FileHandle& value
      test_result = c_tor_FileHandle_ref.file();
      expected_result = original_c_tor_temp_ptr;
  
      ensure_equals("TEST 40.5: Unexpected result calling FileHandle constructor",test_result,expected_result);

      // test case for d-tor
      c_tor_FileHandle_ref.~FileHandle();
      test_result = c_tor_FileHandle_ref.file();
      expected_result = NULL;

      ensure_equals("TEST 40.6: Unexpected result calling FileHandle destructor",test_result,expected_result);
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Clean leftover test files
  base::remove(TEST_FILE_NAME01);
  base::remove(TEST_FILE_NAME02);
}

/*
 * Testing FileHandle c-tors
 * -- Corner/Limit Values -- 
 */
TEST_FUNCTION(45)
{
  try
  {
      // Testing FileHandle(const char *filename, const char *mode, bool throw_on_fail= true)
  
      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Invalid name
      try
      {
        base::FileHandle c_tor(INVALID_NAME, "r");
        fail(strfmt("TEST 45.1: File name \"%s\" did not throw an error",INVALID_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 45.1: File name \"%s\" threw an unexpected error: %s",
                  INVALID_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 45.1: File name \"%s\" threw an unexpected error: %s",
              INVALID_NAME, exc.what()));
      }

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Empty name
      try
      {
          base::FileHandle c_tor(EMPTY_NAME, "r");
          fail(strfmt("TEST 45.2: File name \"%s\" did not throw an error",EMPTY_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 45.2: File name \"%s\" threw an unexpected error: %s",
                  EMPTY_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 45.2: File name \"%s\" threw an unexpected error: %s",
              EMPTY_NAME, exc.what()));
      }

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Too long name
      try
      {
          base::FileHandle c_tor(too_long_name.c_str(), "r");
          fail("TEST 45.3: File with too long name did not throw an error");
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 45.3: File with too long name threw an unexpected error: %s",exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 45.3: File with too long name threw an unexpected error: %s",exc.what()));
      }
#ifdef _WIN32
      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Unicode name (file doesn't exist)
      try
      {
          base::FileHandle c_tor(file_unicode_name.c_str(), "r");
          fail("TEST 45.4: File with unicode name did not throw an error");

      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 45.4: File with unicode name threw an unexpected error: %s",exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 45.4: File with unicode name threw an unexpected error: %s",exc.what()));
      }

#if 0 // this test is not reliable, in some systems/setups this works fine and in some others it'll throw an exception

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      // -- Reserved name
      try
      {
        base::FileHandle c_tor(RESERVED_NAME, "w+");
        fail(strfmt("TEST 45.5.1: File name \"%s\" did not throw an error",RESERVED_NAME));

      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 45.5.2: File name \"%s\" threw an unexpected error: %s",
                  RESERVED_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 45.5.3: File name \"%s\" threw an unexpected error: %s",
              RESERVED_NAME, exc.what()));
      }
#endif
#endif
      // test case for 'throw_on_fail' FALSE value
      // -- Invalid name
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle c_tor(INVALID_NAME, "r", false);
      }
      catch(...)
      {
        fail(strfmt("TEST 45.6: File name \"%s\" threw an error",INVALID_NAME));
      }

      // test case for 'throw_on_fail' FALSE value
      // -- Empty name
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle c_tor(EMPTY_NAME, "r", false);
      }
      catch(...)
      {
        fail(strfmt("TEST 45.7: File name \"%s\" threw an error",EMPTY_NAME));
      }

      // test case for 'throw_on_fail' FALSE value
      // -- Too long name
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle c_tor(too_long_name.c_str(), "r", false);
      }
      catch(...)
      {
        fail("TEST 45.8: File with too long name threw an error");
      }

      // test case for 'throw_on_fail' FALSE value
      // -- Unicode name
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle c_tor(file_unicode_name.c_str(), "r", false);
      }
      catch(...)
      {
        fail("TEST 45.9: File with unicode name threw an error");
      }

      // test case for 'throw_on_fail' FALSE value
      // -- Reserved name
      try
      {
        // doesn't throw error (even if file does not exist)
        base::FileHandle c_tor(RESERVED_NAME, "r", false);
      }
      catch(...)
      {
        fail(strfmt("TEST 45.10: File name \"%s\" threw an error",RESERVED_NAME));
      }

  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Clean leftover test files
  base::remove(file_unicode_name);
}

/*
 * Testing public API's
 * - file_exists(const std::string &path)
 * - is_directory(const std::string &path)
 * - dirname(const std::string &path)
 * - rename(const std::string &from, const std::string &to)
 * - extension(const std::string &path)
 * - basename(const std::string &path)
 * - strip_extension(const std::string &path)
 * - remove_recursive(const std::string &path)
 */
TEST_FUNCTION(50)
{
  std::string command_line;
  std::string test_result;

  try
  {
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
  }
  catch (std::runtime_error &exc)
  {
      throw grt::os_error(strfmt("Runtime error: %s", exc.what()));
  }

  try
  {
      // Create file
      base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w+");
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  try
  {
      // Testing file_exists(const std::string &path) with a directory
      if (!base::file_exists(TEST_DIR_NAME01))
      {
          // return false means dir does not exist
          fail(strfmt("TEST 50.1: Directory \"%s\" does not exist",TEST_DIR_NAME01));
      }

      // Testing file_exists(const std::string &path) with a file
      if (!base::file_exists(TEST_FILE_NAME01))
      {
          // return false means file does not exist
          fail(strfmt("TEST 50.2: File \"%s\" does not exist",TEST_FILE_NAME01));
      }

      // Testing is_directory(const std::string &path)
      if (!base::is_directory(TEST_DIR_NAME01))
      {
          // return false means TEST_DIR_NAME01 is not a directory
          fail(strfmt("TEST 50.3: \"%s\" is not a directory",TEST_DIR_NAME01));
      }

      // Testing dirname(const std::string &path)
      test_result.clear();
      test_result = base::dirname(TEST_FILE_NAME01);
      ensure_equals("TEST 50.4: Unexpected result getting the directory name", test_result, ".");

      test_result.clear();
      test_result = base::dirname(TEST_DIR_NAME01 FILE_SEPARATOR TEST_DIR_NAME02);
      ensure_equals("TEST 50.5: Unexpected result getting the directory name", test_result, TEST_DIR_NAME01);

      // Testing rename(const std::string &from, const std::string &to)
      base::rename(TEST_FILE_NAME01, TEST_FILE_NAME02);

      if (base::file_exists(TEST_FILE_NAME01))
      {
          // return true means old file still exists
          fail(strfmt("TEST 50.6: File \"%s\" still exists",TEST_FILE_NAME01));
      }

      if (!base::file_exists(TEST_FILE_NAME02))
      {
          // return false means new file does not exist
          fail(strfmt("TEST 50.7: File \"%s\" does not exist",TEST_FILE_NAME02));
      }

      // Testing extension(const std::string &path)
      test_result.clear();
      test_result = base::extension(TEST_FILE_NAME02);
      ensure_equals("TEST 50.8: Unexpected result getting file extension", test_result, ".txt");

      // Testing basename(const std::string &path)
      test_result.clear();
      test_result = base::basename("." FILE_SEPARATOR TEST_FILE_NAME02);
      ensure_equals("TEST 50.9: Unexpected result getting file basename", test_result, TEST_FILE_NAME02);

      // Testing strip_extension(const std::string &path)
      test_result.clear();
      test_result = base::strip_extension(TEST_FILE_NAME02);
      ensure_equals("TEST 50.10: Unexpected result calling strip_extension()", test_result, TEST_FILE_STRIPPED_NAME02);

      // Testing remove_recursive(const std::string &path)
      if (!base::remove_recursive(TEST_DIR_NAME01))
      {
          // return false means dir does not exist
          fail(strfmt("TEST 50.11: Directory \"%s\" does not exist",TEST_DIR_NAME01));
      }
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Clean leftover test files
  remove_recursive(TEST_DIR_NAME01);
  remove_recursive(TEST_DIR_NAME02);
  remove(TEST_FILE_NAME01);
  remove(TEST_FILE_NAME02);
}

/*
 * Testing public API's
 * - file_exists(const std::string &path)
 * - is_directory(const std::string &path)
 * - dirname(const std::string &path)
 * - rename(const std::string &from, const std::string &to)
 * - extension(const std::string &path)
 * - basename(const std::string &path)
 * - strip_extension(const std::string &path)
 * - remove_recursive(const std::string &path)
 * -- Corner/Limit Values -- 
 */
TEST_FUNCTION(53)
{
  std::string command_line;
  std::string test_result;

  try
  {
      // Create subdirectory structure
      command_line.clear();
      command_line.assign(MKDIR);
      command_line.append(TEST_DIR_NAME01);
      command_line.append(FILE_SEPARATOR);
      command_line.append(TEST_DIR_NAME02);

      system(command_line.c_str());

  }
  catch (std::runtime_error &exc)
  {
      throw grt::os_error(strfmt("Runtime error: %s", exc.what()));
  }

  try
  {
      // Create file
      base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w+");
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  try
  {
      // Testing file_exists(const std::string &path)

      // -- Invalid name
      if (base::file_exists(INVALID_NAME))
      {
          // return true means file/dir exists
          fail(strfmt("TEST 53.1: 'file_exists()' returned TRUE for invalid name \"%s\""
              ,INVALID_NAME));
      }

      // -- Empty name
      if (base::file_exists(EMPTY_NAME))
      {
          // return true means file/dir exists
          fail(strfmt("TEST 53.2: 'file_exists()' returned TRUE for empty name \"%s\""
              ,EMPTY_NAME));
      }

      // -- Too long name
      if (base::file_exists(too_long_name.c_str()))
      {
          // return true means file/dir exists
          fail(strfmt("TEST 53.3: 'file_exists()' returned TRUE for a too long name"));
      }

      // -- Unicode name
      if (base::file_exists(file_unicode_name.c_str()))
      {
          // return true means file/dir exists
          fail(strfmt("TEST 53.4: 'file_exists()' returned TRUE for a unicode name"));
      }

      // Testing is_directory(const std::string &path)

      // -- Invalid name
      if (base::is_directory(INVALID_NAME))
      {
          // return true means directory exists
          fail(strfmt("TEST 53.5: 'is_directory()' returned TRUE for invalid name \"%s\""
              ,INVALID_NAME));
      }

      // -- Empty name
      if (base::is_directory(EMPTY_NAME))
      {
          // return true means file/dir exists
          fail(strfmt("TEST 53.6: 'is_directory()' returned TRUE for empty name \"%s\""
              ,EMPTY_NAME));
      }

      // -- Too long name
      if (base::is_directory(too_long_name.c_str()))
      {
          // return true means file/dir exists
          fail(strfmt("TEST 53.7: 'is_directory()' returned TRUE for a too long name"));
      }

      // -- Unicode name
      if (base::is_directory(file_unicode_name.c_str()))
      {
          // return true means file/dir exists
          fail(strfmt("TEST 53.8: 'is_directory()' returned TRUE for a unicode name"));
      }
      
#ifdef _WIN32
      // -- Reserved name
      if (base::is_directory(RESERVED_NAME))
      {
          // return true means directory exists
          fail(strfmt("TEST 53.9: 'is_directory()' returned TRUE for reserved name \"%s\""
              ,RESERVED_NAME));
      }

      // Testing dirname(const std::string &path)

      // -- Invalid name
      test_result.clear();
      test_result = base::dirname(INVALID_NAME);
      ensure_equals("TEST 53.10: Unexpected result getting the directory name", test_result, ".");
#endif
      // -- Empty name
      test_result.clear();
      test_result = base::dirname(EMPTY_NAME);
      ensure_equals("TEST 53.11: Unexpected result getting the directory name", test_result, ".");

      // -- Too long name
      test_result.clear();
      test_result = base::dirname(too_long_name.c_str());
      ensure_equals("TEST 53.12: Unexpected result getting the directory name", test_result, ".");

      // -- Unicode name
      test_result.clear();
      test_result = base::dirname(file_unicode_name.c_str());
      ensure_equals("TEST 53.13: Unexpected result getting the directory name", test_result, ".");

      // -- Reserved name
      test_result.clear();
      test_result = base::dirname(RESERVED_NAME);
      ensure_equals("TEST 53.14: Unexpected result getting the directory name", test_result, ".");

      // Testing rename(const std::string &from, const std::string &to)

      // -- Invalid name (1st parameter)
      try
      {
        base::rename(INVALID_NAME, TEST_FILE_NAME02);
        fail(strfmt("TEST 53.15: File name \"%s\" did not throw an error",INVALID_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.15: File name \"%s\" threw an unexpected error: %s",
                  INVALID_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.15: File name \"%s\" threw an unexpected error: %s",
              INVALID_NAME, exc.what()));
      }

      // -- Invalid name (2nd parameter)
      try
      {
        base::rename(TEST_FILE_NAME01, INVALID_NAME);
        fail(strfmt("TEST 53.16: File name \"%s\" did not throw an error",INVALID_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.16: File name \"%s\" threw an unexpected error: %s",
                  INVALID_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.16: File name \"%s\" threw an unexpected error: %s",
              INVALID_NAME, exc.what()));
      }

      // -- Invalid name (both parameters)
      try
      {
        base::rename(INVALID_NAME, INVALID_NAME);
        fail(strfmt("TEST 53.17: File name \"%s\" did not throw an error",INVALID_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.17: File name \"%s\" threw an unexpected error: %s",
                  INVALID_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.17: File name \"%s\" threw an unexpected error: %s",
              INVALID_NAME, exc.what()));
      }

      // -- Empty name (1st parameter)
      try
      {
        base::rename(EMPTY_NAME, TEST_FILE_NAME02);
        fail(strfmt("TEST 53.18: File name \"%s\" did not throw an error",EMPTY_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.18: File name \"%s\" threw an unexpected error: %s",
                  EMPTY_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.18: File name \"%s\" threw an unexpected error: %s",
              EMPTY_NAME, exc.what()));
      }

      // -- Empty name (2nd parameter)
      try
      {
        base::rename(TEST_FILE_NAME01, EMPTY_NAME);
        fail(strfmt("TEST 53.19: File name \"%s\" did not throw an error",EMPTY_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.19: File name \"%s\" threw an unexpected error: %s",
                  EMPTY_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.19: File name \"%s\" threw an unexpected error: %s",
              EMPTY_NAME, exc.what()));
      }

      // -- Empty name (both parameters)
      try
      {
        base::rename(EMPTY_NAME, EMPTY_NAME);
        fail(strfmt("TEST 53.20: File name \"%s\" did not throw an error",EMPTY_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.20: File name \"%s\" threw an unexpected error: %s",
                  EMPTY_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.20: File name \"%s\" threw an unexpected error: %s",
              EMPTY_NAME, exc.what()));
      }

      // -- Too long name (1st parameter)
      try
      {
        base::rename(too_long_name.c_str(), TEST_FILE_NAME02);
        fail("TEST 53.21: Too long file name did not throw an error");
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.21: Too long file name threw an unexpected error: %s",exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.21: Too long file name threw an unexpected error: %s",exc.what()));
      }

      // -- Too long name (2nd parameter)
      try
      {
        base::rename(TEST_FILE_NAME01, too_long_name.c_str());
        fail("TEST 53.22: Too long file name did not throw an error");
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.22: Too long file name threw an unexpected error: %s",exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.22: Too long file name threw an unexpected error: %s",exc.what()));
      }

      // -- Too long name (both parameters)
      try
      {
        base::rename(too_long_name.c_str(), too_long_name.c_str());
        fail("TEST 53.23: Too long file name did not throw an error");
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.23: Too long file name threw an unexpected error: %s",exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.23: Too long file name threw an unexpected error: %s",exc.what()));
      }

      // -- Reserved name (1st parameter)
      try
      {
        base::rename(RESERVED_NAME, TEST_FILE_NAME02);
        fail(strfmt("TEST 53.24: File name \"%s\" did not throw an error",RESERVED_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.24: File name \"%s\" threw an unexpected error: %s",
                  RESERVED_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.24: File name \"%s\" threw an unexpected error: %s",
              RESERVED_NAME, exc.what()));
      }

      // -- Reserved name (2nd parameter)
      try
      {
        base::rename(TEST_FILE_NAME01, RESERVED_NAME);
        fail(strfmt("TEST 53.25: File name \"%s\" did not throw an error",RESERVED_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.25: File name \"%s\" threw an unexpected error: %s",
                  RESERVED_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.25: File name \"%s\" threw an unexpected error: %s",
              RESERVED_NAME, exc.what()));
      }

      // -- Reserved name (both parameters)
      try
      {
        base::rename(RESERVED_NAME, RESERVED_NAME);
        fail(strfmt("TEST 53.26: File name \"%s\" did not throw an error",RESERVED_NAME));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.26: File name \"%s\" threw an unexpected error: %s",
                  RESERVED_NAME, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.26: File name \"%s\" threw an unexpected error: %s",
              RESERVED_NAME, exc.what()));
      }

      // -- Unicode name (change from ASCII to Unicode)
      base::rename(TEST_FILE_NAME01, file_unicode_name.c_str());

      if (base::file_exists(TEST_FILE_NAME01))
      {
          // return true means old file still exists
          fail(strfmt("TEST 53.27: File \"%s\" still exists",TEST_FILE_NAME01));
      }

      if (!base::file_exists(file_unicode_name.c_str()))
      {
          // return false means new file does not exist
          fail(strfmt("TEST 53.28: File \"%s\" does not exist",file_unicode_name.c_str()));
      }

      // -- Unicode name (change back from Unicode to ASCII)
      base::rename(file_unicode_name.c_str(), TEST_FILE_NAME01);

      if (base::file_exists(file_unicode_name.c_str()))
      {
          // return true means old file still exists
          fail(strfmt("TEST 53.29: File \"%s\" still exists",file_unicode_name.c_str()));
      }

      if (!base::file_exists(TEST_FILE_NAME01))
      {
          // return false means new file does not exist
          fail(strfmt("TEST 53.30: File \"%s\" does not exist",TEST_FILE_NAME01));
      }

      // -- Source File does not exist
      try
      {
        base::rename(TEST_FILE_NAME03, TEST_FILE_NAME02);
        fail(strfmt("TEST 53.31: Non-existing file \"%s\" did not throw an error",TEST_FILE_NAME03));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.31: Non-existing file \"%s\" threw an unexpected error: %s",
                  TEST_FILE_NAME03, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.31: Non-existing file \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME03, exc.what()));
      }
#ifdef _WIN32
      // -- Target File already exists
      try
      {
        // Create file
        base::FileHandle test_file_scoped(TEST_FILE_NAME03, "w+");

        base::rename(TEST_FILE_NAME01, TEST_FILE_NAME03);
        fail(strfmt("TEST 53.32: Already-existing file \"%s\" did not throw an error",TEST_FILE_NAME03));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.32: Already-existing file \"%s\" threw an unexpected error: %s",
                  TEST_FILE_NAME03, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.32: Already-existing file \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME03, exc.what()));
      }
#endif
      // -- Source & Target files are the same, and non-existing
      try
      {
        base::rename(TEST_FILE_NAME02, TEST_FILE_NAME02);
        fail(strfmt("TEST 53.33: Non-existing files \"%s\" did not throw an error",TEST_FILE_NAME02));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 53.33: Non-existing files \"%s\" threw an unexpected error: %s",
                  TEST_FILE_NAME02, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 53.33: Non-existing files \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME02, exc.what()));
      }

      // -- Source & Target files are the same, and both exist
      base::rename(TEST_FILE_NAME01, TEST_FILE_NAME01);

      if (!base::file_exists(TEST_FILE_NAME01))
      {
          // return false means new file does not exist
          fail(strfmt("TEST 53.34: File \"%s\" does not exist",TEST_FILE_NAME01));
      }

      // Testing extension(const std::string &path)

      // -- Invalid name
      test_result.clear();
      test_result = base::extension(INVALID_NAME);
      ensure_equals("TEST 53.35: Unexpected result getting file extension", test_result, "");

      // -- Empty name
      test_result.clear();
      test_result = base::extension(EMPTY_NAME);
      ensure_equals("TEST 53.36: Unexpected result getting file extension", test_result, "");

      // -- Too long name
      test_result.clear();
      test_result = base::extension(too_long_name.c_str());
      ensure_equals("TEST 53.37: Unexpected result getting file extension", test_result, ".txt");

      // -- Unicode name
      test_result.clear();
      test_result = base::extension(file_unicode_name.c_str());
      ensure_equals("TEST 53.38: Unexpected result getting file extension", test_result, ".txt");

#ifdef _WIN32
      // -- Reserved name
      test_result.clear();
      test_result = base::extension(RESERVED_NAME);
      ensure_equals("TEST 53.39: Unexpected result getting file extension", test_result, "");
#endif
      
      // -- No extension
      test_result.clear();
      test_result = base::extension("filename_no_ext");
      ensure_equals("TEST 53.40: Unexpected result getting file extension", test_result, "");

      // -- No extension with dot
      test_result.clear();
      test_result = base::extension("filename_with_no_extension.");
      ensure_equals("TEST 53.41: Unexpected result getting file extension", test_result, ".");

      // -- Only extension
      test_result.clear();
      test_result = base::extension(".txt");
      ensure_equals("TEST 53.42: Unexpected result getting file extension", test_result, ".txt");

      // -- Long extension
      test_result.clear();
      test_result = base::extension("basename.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
      ensure_equals("TEST 53.43: Unexpected result getting file extension", test_result, 
          ".xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

      // Testing strip_extension(const std::string &path)

      // -- Invalid name
      test_result.clear();
      test_result = base::strip_extension(INVALID_NAME);
      ensure_equals("TEST 53.44: Unexpected result calling strip_extension()", test_result, INVALID_NAME);

      // -- Empty name
      test_result.clear();
      test_result = base::strip_extension(EMPTY_NAME);
      ensure_equals("TEST 53.45: Unexpected result calling strip_extension()", test_result, EMPTY_NAME);

      // -- Too long name
      test_result.clear();
      test_result = base::strip_extension(too_long_name.c_str());
      ensure_equals("TEST 53.46: Unexpected result calling strip_extension()", test_result, 
          too_long_basename.c_str());

      // -- Unicode name
      test_result.clear();
      test_result = base::strip_extension(file_unicode_name.c_str());
      ensure_equals("TEST 53.47: Unexpected result calling strip_extension()", test_result, 
          file_unicode_basename.c_str());

#ifdef _WIN32
      // -- Reserved name
      test_result.clear();
      test_result = base::strip_extension(RESERVED_NAME);

      ensure_equals("TEST 53.48: Unexpected result calling strip_extension()", test_result, RESERVED_NAME);
#endif
      // -- No extension
      test_result.clear();
      test_result = base::strip_extension("filename_with_no_extension");
      ensure_equals("TEST 53.49: Unexpected result calling strip_extension()", test_result, 
          "filename_with_no_extension");

      // -- No extension with dot
      test_result.clear();
      test_result = base::strip_extension("filename_with_no_extension.");
      ensure_equals("TEST 53.50: Unexpected result calling strip_extension()", test_result, 
          "filename_with_no_extension");

      // -- Only extension
      test_result.clear();
      test_result = base::strip_extension(".txt");
      ensure_equals("TEST 53.51: Unexpected result calling strip_extension()", test_result, "");

      // -- Long extension
      test_result.clear();
      test_result = base::strip_extension("basename.xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
      ensure_equals("TEST 53.52: Unexpected result getting file extension", test_result, "basename");

      // Testing remove_recursive(const std::string &path)

      // -- Invalid name
      if (base::remove_recursive(INVALID_NAME))
      {
          // return true means dir exists
          fail(strfmt("TEST 53.53: Directory \"%s\" exists",INVALID_NAME));
      }

      // -- Empty name
      if (base::remove_recursive(EMPTY_NAME))
      {
          // return true means dir exists
          fail(strfmt("TEST 53.54: Directory \"%s\" exists",EMPTY_NAME));
      }

      // -- Too long name
      if (base::remove_recursive(too_long_name.c_str()))
      {
          // return true means dir exists
          fail(strfmt("TEST 53.55: Directory \"%s\" exists",too_long_name.c_str()));
      }

      // -- Unicode name
      base::rename(TEST_DIR_NAME01, dir_unicode_name.c_str());

      if (!base::remove_recursive(dir_unicode_name.c_str()))
      {
          // return false means dir does not exist
          fail("TEST 53.56: Directory with Unicode name does not exist");
      }
#ifdef _WIN32
      // -- Reserved name
      if (base::remove_recursive(RESERVED_NAME))
      {
          // return true means dir exists
          fail(strfmt("TEST 53.57: Directory \"%s\" exists",RESERVED_NAME));
      }
#endif
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Clean leftover test files
  remove_recursive(TEST_DIR_NAME01);
  remove_recursive(dir_unicode_name.c_str());
  remove(TEST_FILE_NAME01);
  remove(TEST_FILE_NAME02);
  remove(TEST_FILE_NAME03);
}

/*
 * Testing public API's
 * - rename(const std::string &from, const std::string &to)
 * - remove(const std::string &path)
 * - remove_recursive(const std::string &path)
 * -- Read-only permissions -- 
 */
#ifdef _WIN32
TEST_FUNCTION(54)
{
  std::string command_line;

  try
  {
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
  }
  catch (std::runtime_error &exc)
  {
      throw grt::os_error(strfmt("Runtime error: %s", exc.what()));
  }

  try
  {
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
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  try
  {
      // Rename a read-only file
      base::rename(TEST_FILE_NAME01, TEST_FILE_NAME02);

      if (base::file_exists(TEST_FILE_NAME01))
      {
          // return true means old file still exists
          fail(strfmt("TEST 54.1: File \"%s\" still exists",TEST_FILE_NAME01));
      }

      if (!base::file_exists(TEST_FILE_NAME02))
      {
          // return false means new file does not exist
          fail(strfmt("TEST 54.1: File \"%s\" does not exist",TEST_FILE_NAME02));
      }

      // Remove a read-only directory
      try
      {
          bool result = base::remove(TEST_DIR_NAME02);
          fail(strfmt("TEST 54.2: Read-only directory \"%s\" did not throw an error",TEST_DIR_NAME02));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not delete file "))
          {
              fail(strfmt("TEST 54.2: Read-only directory \"%s\" threw an unexpected error: %s",
                  TEST_DIR_NAME02, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 54.2: Read-only directory \"%s\" threw an unexpected error: %s",
              TEST_DIR_NAME02, exc.what()));
      }

      // Remove a read-only file
      try
      {
          base::remove(TEST_FILE_NAME02);
          fail(strfmt("TEST 54.3: Read-only file \"%s\" did not throw an error",TEST_FILE_NAME02));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not delete file "))
          {
              fail(strfmt("TEST 54.3: Read-only file \"%s\" threw an unexpected error: %s",
                  TEST_FILE_NAME02, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 54.3: Read-only file \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME02, exc.what()));
      }

      // Recursively remove a read-only directory structure
      if (!base::remove_recursive(TEST_DIR_NAME01))
      {
          // return false means dir does not exist
          fail(strfmt("TEST 54.4: Directory \"%s\" does not exist",TEST_DIR_NAME01));
      }
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Change back permissions to read-write
  try
  {
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
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Clean leftover test files
  base::remove_recursive(TEST_DIR_NAME01);
  base::remove_recursive(TEST_DIR_NAME02);
  base::remove(TEST_FILE_NAME01);
  base::remove(TEST_FILE_NAME02);
}
#endif

/*
 * Testing public API
 * - scan_for_files_matching(const std::string &pattern,
       bool recursive = false);
 */
TEST_FUNCTION(55)
{
  std::string command_line;

  try
  {
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
  }
  catch (std::runtime_error &exc)
  {
      throw grt::os_error(strfmt("Runtime error: %s", exc.what()));
  }

  try
  {
      // Create files
      base::FileHandle test_file01(TEST_FILE_NAME01, "w+");
      base::FileHandle test_file02(TEST_FILE_NAME02, "w+");
  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // test case for 'recursive' default value (i.e. FALSE)
  try
  {
      std::string search_pattern = "." FILE_SEPARATOR FILE_PATTERN "*";
      std::list<std::string> test_result = base::scan_for_files_matching(search_pattern);

      ensure("TEST 55.1: File matching returned an empty list",!test_result.empty());
      ensure_equals("TEST 55.2: Invalid list size", test_result.size(), 4U);

      while (!test_result.empty())
      {
          ensure("TEST 55.3: Invalid file matching", std::string::npos != test_result.front().find(FILE_PATTERN));
          test_result.pop_front();
      }

  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // test case for 'recursive' TRUE value
  try
  {
      std::string search_pattern = "." FILE_SEPARATOR FILE_PATTERN "*";
      std::list<std::string> test_result = base::scan_for_files_matching(search_pattern,true);

      ensure("TEST 55.1: File matching returned an empty list",!test_result.empty());
      ensure_equals("TEST 55.2: Invalid list size", test_result.size(), 5U);

      while (!test_result.empty())
      {
          ensure("TEST 55.3: Invalid file matching", std::string::npos != test_result.front().find(FILE_PATTERN));
          test_result.pop_front();
      }

  }
  catch (base::file_error &exc)
  {
      throw grt::os_error(strfmt("File error: %s", exc.what()));
  }

  // Clean leftover test files
  remove_recursive(TEST_DIR_NAME01);
  remove_recursive(TEST_DIR_NAME02);
  remove(TEST_FILE_NAME01);
  remove(TEST_FILE_NAME02);
}

/*
 * Testing file_locked_error public API's
 * - c-tors
 */
TEST_FUNCTION(60)
{
  try
  {
      // test cases for constructors
      try
      {
          base::file_locked_error error("File Locked Error Message");
          throw(error);
          fail("TEST 60.1: Invalid file_locked_error");
      }
      catch (const base::file_locked_error)
      {
          //Nothing to do, just catch the error and continue
      }

      try
      {
          base::file_locked_error first_error("File Locked Error Message");
          base::file_locked_error second_error(first_error);
          throw(second_error);
          fail("TEST 60.2: Invalid file_locked_error (copy-constructor)");
      }
      catch (const base::file_locked_error)
      {
          //Nothing to do, just catch the error and continue
      }

  }
  catch (std::exception &exc)
  {
      throw grt::os_error(strfmt("Runtime error: %s", exc.what()));
  }
}

/*
 * Testing LockFile public API's
 * - c-tor
 * - d-tor
 * - check(const std::string &path)
 */
TEST_FUNCTION(65)
{
  remove(TEST_FILE_NAME01);
  remove(TEST_FILE_NAME02);

  try
  {
      // test cases for constructor, check(const std::string &path)
      // with Status == LockedSelf and Status == NotLocked
      // and destructor
      try
      {
        {
          base::LockFile lock_file01(TEST_FILE_NAME01);
          base::FileHandle test_file02(TEST_FILE_NAME02, "w+");

          if (LockFile::check(TEST_FILE_NAME01) != LockFile::LockedSelf)
              fail(strfmt("TEST 65.1: File \"%s\" not locked",TEST_FILE_NAME01));

#ifndef _WIN32
          // Semantic issue with NotLocked for a plain file without content.
          // TODO: rework lock detection with other than base::LockFile instances.
          if (LockFile::check(TEST_FILE_NAME02) != LockFile::NotLocked)
              fail(strfmt("TEST 65.2: File \"%s\" locked",TEST_FILE_NAME02));
#endif
        }
        ensure("TEST 65.3: Failed d-tor call", !base::file_exists(TEST_FILE_NAME01));
      }
      catch (base::file_error &exc)
      {
          throw grt::os_error(strfmt("File error: %s", exc.what()));
      }
      catch (std::invalid_argument &exc)
      {
          throw grt::os_error(strfmt("Invalid argument error: %s", exc.what()));
      }
      catch (std::runtime_error &exc)
      {
          throw grt::os_error(strfmt("Runtime/file-locked error: %s", exc.what()));
      }
  }
  catch (std::exception &exc)
  {
    throw grt::os_error(strfmt("Runtime error: %s", exc.what()));
  }

  // Clean leftover test files
  remove(TEST_FILE_NAME01);
  remove(TEST_FILE_NAME02);
}

#ifdef _WIN32
/*
 * Child thread function
 */
gpointer _child_thread_func(gpointer data)
{
    // Miscellaneous variables
    FILE* test_result = NULL;
    FILE* expected_result = NULL;
    base::FileHandle test_file;

    //  If the tread thows an exception it will SIGABRT the process. So we'll catch the exception
    //  and return gracefully.
    try
    {
      // Testing rename(const std::string &from, const std::string &to)
      try
      {
          base::rename(TEST_FILE_NAME01, TEST_FILE_NAME02);
          fail(strfmt("TEST 70.1: Locked file \"%s\" did not throw an error",TEST_FILE_NAME01));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not rename file "))
          {
              fail(strfmt("TEST 70.1: Locked file \"%s\" threw an unexpected error: %s",
                  TEST_FILE_NAME01, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 70.1: Locked file \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME01, exc.what()));
      }

      // Testing remove(const std::string &path)
      try
      {
          base::remove(TEST_FILE_NAME01);
          fail(strfmt("TEST 70.2: Locked file \"%s\" did not throw an error",TEST_FILE_NAME01));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Could not delete file "))
          {
              fail(strfmt("TEST 70.2: Locked file \"%s\" threw an unexpected error: %s",
                  TEST_FILE_NAME01, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 70.2: Locked file \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME01, exc.what()));
      }

      // Testing open_file(const char *filename, const char *mode, bool throw_on_fail= true)

      // test case for 'throw_on_fail' default value (i.e. TRUE)
      try
      {
          // throw error (file is locked)
          base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w");
          fail(strfmt("TEST 70.3: Locked file \"%s\" did not throw an error",TEST_FILE_NAME01));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 70.3: Locked file \"%s\" threw an unexpected error: %s",
                  TEST_FILE_NAME01, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 70.3: Locked file \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME01, exc.what()));
      }

      // test case for 'throw_on_fail' FALSE value
      try
      {
          // doesn't throw error (even if file is locked)
          base::FileHandle test_file_scoped(TEST_FILE_NAME01, "w", false);
          test_result = test_file.file();
          expected_result = NULL;

          ensure_equals("TEST 70.4: Unexpected result calling FileHandle c-tor",test_result,expected_result);
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 70.4: Read-only file \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME01, exc.what()));
      }

      // Testing FileHandle c-tor
      try
      {
          base::FileHandle c_tor_test_filename(TEST_FILE_NAME01, "w+");
          fail(strfmt("TEST 70.5: Locked file \"%s\" did not throw an error",TEST_FILE_NAME01));
      }
      catch (const base::file_error &exc)
      {
          if (0 != std::string(exc.what()).find("Failed to open file \""))
          {
              fail(strfmt("TEST 70.5: Locked file \"%s\" threw an unexpected error: %s",
                  TEST_FILE_NAME01, exc.what()));
          }
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 70.5: Locked file \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME01, exc.what()));
      }

      // Testing LockFile c-tor
      try
      {
          base::LockFile lock_file(TEST_FILE_NAME01);
          fail(strfmt("TEST 70.6: Locked file \"%s\" did not throw an error",TEST_FILE_NAME01));
      }
      catch (const base::file_locked_error)
      {
          //Nothing to do, just catch the error and continue
      }
      catch(std::exception &exc)
      {
          fail(strfmt("TEST 70.6: Locked file \"%s\" threw an unexpected error: %s",
              TEST_FILE_NAME01, exc.what()));
      }

      // Testing LockFile::check in child thread
      try
      {
          if (LockFile::check(TEST_FILE_NAME01) != LockFile::LockedSelf)
              fail(strfmt("TEST 70.7: File \"%s\" not locked",TEST_FILE_NAME01));
      }
      catch (base::file_error &exc)
      {
          throw grt::os_error(strfmt("File error: %s", exc.what()));
      }
      catch (std::invalid_argument &exc)
      {
          throw grt::os_error(strfmt("Invalid argument error: %s", exc.what()));
      }
      catch (std::runtime_error &exc)
      {
          throw grt::os_error(strfmt("Runtime/file-locked error: %s", exc.what()));
      }
    }
    catch(std::exception &exc)
    {
      fail(exc.what());
    }
    return NULL;
}

/*
 * Testing public API's
 * - rename(const std::string &from, const std::string &to)
 * - remove(const std::string &path)
 * Testing FileHandle public API's
 * - open_file(const char *filename, const char *mode, bool throw_on_fail= true)
 * Testing FileHandle c-tor
 * -- Multi-threading locking -- 
 */
TEST_FUNCTION(70)
{
    // Miscellaneous variables
    GThread * _child_thread;

    try
    {
        // Create file
        base::FileHandle test_file01(TEST_FILE_NAME01, "w+");
    }
    catch (base::file_error &exc)
    {
        throw grt::os_error(strfmt("File error: %s", exc.what()));
    }

    // Lock the file
    base::LockFile lock_file(TEST_FILE_NAME01);

    // Testing LockFile::check in main thread
    try
    {
        if (LockFile::check(TEST_FILE_NAME01) != LockFile::LockedSelf)
            fail(strfmt("TEST 70.8: File \"%s\" not locked",TEST_FILE_NAME01));
    }
    catch (base::file_error &exc)
    {
        throw grt::os_error(strfmt("File error: %s", exc.what()));
    }
    catch (std::invalid_argument &exc)
    {
        throw grt::os_error(strfmt("Invalid argument error: %s", exc.what()));
    }
    catch (std::runtime_error &exc)
    {
        throw grt::os_error(strfmt("Runtime/file-locked error: %s", exc.what()));
    }

    // Kick off the child thread
    _child_thread = base::create_thread(_child_thread_func, this);
    // Wait for _main_thread to finish
    g_thread_join(_child_thread);
}
#endif


END_TESTS
