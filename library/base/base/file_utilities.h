/* 
 * Copyright (c) 2011, 2015, Oracle and/or its affiliates. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _FILE_UTILITIES_H_
#define _FILE_UTILITIES_H_

#include "common.h"
#include <list>
#include <string>
#include <stdexcept>

namespace base
{
  enum error_code
  {
    success = 0,
    file_not_found = -1,
    already_exists = -2,
    access_denied = -3,
    other_error = -1000
  };
    
  #ifdef _WIN32
    #pragma warning(disable: 4275) // non dll-interface class used as base dll-interface class.
  #endif
  
  class BASELIBRARY_PUBLIC_FUNC file_error : public std::runtime_error
  {
    int sys_error_code;
  public:
    file_error(const std::string &text, int err);
    
    error_code code();
    int sys_code();
  };
  
  BASELIBRARY_PUBLIC_FUNC std::list<std::string> scan_for_files_matching(const std::string &pattern,
    bool recursive = false);

  class BASELIBRARY_PUBLIC_FUNC file_locked_error : public std::runtime_error
  {
  public:
    file_locked_error(const std::string &msg) : std::runtime_error(msg)
    {}
  };
  
  struct BASELIBRARY_PUBLIC_FUNC LockFile 
  {
#ifdef _WIN32
    #pragma warning(disable: 4251) // DLL interface required for std::string member.
    #pragma warning(disable: 4290) // C++ exception specification ignored.
    HANDLE handle;
#else
    int fd;
#endif

    std::string path;
    enum Status
    {
      LockedSelf, // lock file exists and is the process itself
      LockedOther, // lock file exists and its owner is running
      NotLocked,
    };
    
    LockFile(const std::string &path) throw (std::invalid_argument, std::runtime_error, file_locked_error);
    ~LockFile();
#undef check // there's a #define check in osx
    static Status check(const std::string &path);
  };


  class BASELIBRARY_PUBLIC_FUNC FileHandle
  {
    FILE *_file;
  public:
    FileHandle() : _file(NULL) {}
    FileHandle(const char *filename, const char *mode, bool throw_on_fail= true);
    FileHandle(FileHandle &fh) : _file(NULL) { swap(fh); }
    ~FileHandle() { dispose(); }
    void swap(FileHandle &fh);
    operator bool() const { return (!_file); }
    FileHandle & operator =(FileHandle &fh); // will pass ownership of FILE from assigned obj to this
    //  NOTE: Never close this handle, because it's managed by the FileHandle class.
    FILE * file() { return _file; }
    void dispose();

//  Removed because it's dangerous to use. If this object received a FILE * that is
//  controled by another object one of them will crash when disposing. So, this class
//  will contain the FILE * management within itself.
//     FileHandle(FILE *file) : _file(file) {}
//     operator FILE *() { return _file; }
//     FILE * operator ->() { return _file; }
//     static FILE * open_file(const char *filename, const char *mode, bool throw_on_fail= true);
//     static void close_file(FILE *f);
  };


  // creates the directory, returns false if the directory exists.. throws exception on error
  BASELIBRARY_PUBLIC_FUNC bool create_directory(const std::string &path, int mode, bool with_parents=false);
  
  BASELIBRARY_PUBLIC_FUNC bool remove(const std::string &path);
  BASELIBRARY_PUBLIC_FUNC bool tryRemove(const std::string &path);

  BASELIBRARY_PUBLIC_FUNC bool remove_recursive(const std::string &path);

  BASELIBRARY_PUBLIC_FUNC void rename(const std::string &from, const std::string &to);

  BASELIBRARY_PUBLIC_FUNC bool file_exists(const std::string &path);
  BASELIBRARY_PUBLIC_FUNC bool is_directory(const std::string &path);

  //BASELIBRARY_PUBLIC_FUNC std::string make_path(const std::string &path, const std::string &filename);
  
  // file.ext -> .ext
  BASELIBRARY_PUBLIC_FUNC std::string extension(const std::string &path);
  // returns . if no dirname in path
  BASELIBRARY_PUBLIC_FUNC std::string dirname(const std::string &path);
  // returns . if no filename in path
  BASELIBRARY_PUBLIC_FUNC std::string basename(const std::string &path);
  
  // file.ext -> file
  BASELIBRARY_PUBLIC_FUNC std::string strip_extension(const std::string &path);

  BASELIBRARY_PUBLIC_FUNC bool file_mtime(const std::string &path, time_t &mtime);

  BASELIBRARY_PUBLIC_FUNC std::string join_path(const char *prefix, ...);
};

#endif
