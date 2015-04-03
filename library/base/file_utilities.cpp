/* 
 * Copyright (c) 2010, 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/file_functions.h"

#include <stdexcept>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#endif
#include <algorithm>

namespace base {
  
  std::string format_file_error(const std::string &text, int err)
  {
#ifdef _WIN32
    return strfmt("%s: error code %i", text.c_str(), err);
#else
    return strfmt("%s: %s", text.c_str(), strerror(err));
#endif
  }

  file_error::file_error(const std::string &text, int err)
  : std::runtime_error(format_file_error(text, err)), sys_error_code(err)
  {
  }
  
  error_code file_error::code()
  {
#ifdef _WIN32
    switch (sys_error_code)
    {
      case 0:
        return success;
      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:
        return file_not_found;
      case ERROR_ALREADY_EXISTS:
        return already_exists;
      case ERROR_ACCESS_DENIED:
        return access_denied;
      default:
        return other_error;
    }
#else
    switch (sys_error_code)
    {
      case 0:
        return success;
      case ENOENT:
        return file_not_found;
      case EEXIST:
        return already_exists;
      case EACCES:
        return access_denied;
      default:
        return other_error;
    }
#endif
  }
  
//--------------------------------------------------------------------------------------------------

  std::list<std::string> scan_for_files_matching(const std::string &pattern, bool recursive)
  {
    std::list<std::string> matches;

    std::string path = dirname(pattern);
    if (!g_file_test(path.c_str(), G_FILE_TEST_EXISTS))
    {
      return matches;
    }

    std::string pure_pattern = pattern.substr(path.size() + 1);

    std::string bname = basename(pattern);
    GPatternSpec *pat = g_pattern_spec_new(bname.c_str());
    GDir *dir;
    {
      GError *err = NULL;
      dir = g_dir_open(path.empty() ? "." : path.c_str(), 0, &err);
      if (!dir)
      {
        std::string msg = strfmt("can't open %s: %s", !path.empty() ? path.c_str() : ".", err->message);
        g_error_free(err);
        g_pattern_spec_free(pat);
        throw std::runtime_error(msg);
      }
    }
    const gchar *filename;
    while ((filename = g_dir_read_name(dir)))
    {
      std::string full_path = strfmt("%s%s%s", path.c_str(), G_DIR_SEPARATOR_S, filename);
      if (g_pattern_match_string(pat, filename))
        matches.push_back(full_path);

      if (recursive && g_file_test(full_path.c_str(), G_FILE_TEST_IS_DIR))
      {
        std::string subpattern = strfmt("%s%s%s", full_path.c_str(), G_DIR_SEPARATOR_S, pure_pattern.c_str());
        std::list<std::string> submatches = scan_for_files_matching(subpattern, true);
        if (submatches.size() > 0)
          matches.insert(matches.end(), submatches.begin(), submatches.end());
      }
    }
    g_dir_close(dir);
    g_pattern_spec_free(pat);
    return matches;
  }

//--------------------------------------------------------------------------------------------------

#ifdef _WIN32
  LockFile::LockFile(const std::string &path) throw (std::invalid_argument, std::runtime_error, file_locked_error)
    : path(path), handle(0)
  { 
    std::wstring wpath(string_to_wstring(path));
    if (path.empty()) throw std::invalid_argument("invalid path");

    handle = CreateFileW(wpath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
      OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
      if (GetLastError() == ERROR_SHARING_VIOLATION)
        throw file_locked_error("File already locked");
      throw std::runtime_error(strfmt("Error creating lock file (%i)", GetLastError()));
    }

    char buffer[32];
    sprintf_s(buffer, "%i", GetCurrentProcessId());
    DWORD bytes_written;

    if (!WriteFile(handle, buffer, (DWORD)strlen(buffer), &bytes_written, NULL) || bytes_written != strlen(buffer))
    {
      CloseHandle(handle);
      DeleteFileW(wpath.c_str());
      throw std::runtime_error("Could not write to lock file");
    }
  }
  
  LockFile::~LockFile()
  {
    if (handle)
      CloseHandle(handle);
    DeleteFileW(string_to_wstring(path).c_str());
  }
  
  LockFile::Status LockFile::check(const std::string &path)
  {
    // Can we open the file in exclusive mode?
    std::wstring wpath = string_to_wstring(path);
    HANDLE h = CreateFile(wpath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h != INVALID_HANDLE_VALUE)
    {
      CloseHandle(h);
      return NotLocked;
    }

    switch (GetLastError())
    {
    case ERROR_SHARING_VIOLATION:
      // If file cannot be opened for writing it's open somewhere else.
      // Try to open it and read the first bytes to see if that corresponds to our process id.
      h = CreateFileW(wpath.c_str(), 
            GENERIC_READ, 
            FILE_SHARE_WRITE | FILE_SHARE_READ, 
            NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      if (h != INVALID_HANDLE_VALUE)
      {
        char buffer[32];
        DWORD bytes_read;
        if (ReadFile(h, buffer, sizeof(buffer), &bytes_read, NULL))
        {
          CloseHandle(h);
          buffer[bytes_read] = 0;
          if (base::atoi<int>(buffer, -1) != GetCurrentProcessId())
            return LockedOther; // TODO: this unreliable. Any non-lock file would qualify here (which is wrong).
          return LockedSelf;
        }
        CloseHandle(h);
        return LockedOther;
      }

      // If the file cannot be read assume its locked by another process.
      return LockedOther;

    case ERROR_FILE_NOT_FOUND:
      return NotLocked;

    case ERROR_PATH_NOT_FOUND:
      throw std::invalid_argument("Invalid path");

    default:
      throw std::runtime_error(strfmt("Unknown error while checking file %s for locks (%i)", path.c_str(), GetLastError()));
    }
  }
#else
  LockFile::LockFile(const std::string &apath) throw (std::invalid_argument, std::runtime_error, file_locked_error)
  : path(apath)
  { 
    if (path.empty()) throw std::invalid_argument("invalid path");
    
    fd = open(path.c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
    if (fd < 0)
    {
      // this could mean lock exists, that it's a dangling file or that it's currently being locked by some other process/thread
      // we just go on and try to lock the file if the file already exists
      if (errno == ENOENT || errno == ENOTDIR)
        throw std::invalid_argument("invalid path");
        
      throw std::runtime_error(strfmt("%s creating lock file", g_strerror(errno)));
    }
    if (flock(fd, LOCK_EX|LOCK_NB) < 0)
    {
      close(fd);
      fd = -1;
      if (errno == EWOULDBLOCK)
        throw file_locked_error("file already locked");

      throw std::runtime_error(strfmt("%s while locking file", g_strerror(errno)));
    }

    ftruncate(fd, 0);

    char pid[32];
    snprintf(pid, sizeof(pid), "%i", getpid());
    if (write(fd, pid, strlen(pid)+1) < 0)
    {
      close(fd);
      throw std::runtime_error(strfmt("%s while locking file", g_strerror(errno)));
    }
  }
  
  LockFile::~LockFile()
  {
    if (fd >= 0)
      close(fd);
    unlink(path.c_str());
  }
  
  LockFile::Status LockFile::check(const std::string &path)
  {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
      return NotLocked;

    if (flock(fd, LOCK_EX|LOCK_NB) < 0)
    {
      char pid[32];
      // couldn't lock file, check if we own it ourselves
      int c = read(fd, pid, sizeof(pid)-1);
      close(fd);
      if (c < 0)
        return LockedOther;
      pid[c] = 0;
      if (base::atoi<int>(pid, -1) != getpid())
        return LockedOther;
      return LockedSelf;
    }
    else // nobody holds a lock on the file, so this is a leftover
    {
      flock(fd, LOCK_UN);
      close(fd);
      return NotLocked;
    }
  }
#endif


  bool create_directory(const std::string &path, int mode, bool with_parents)
  {
#ifdef _WIN32
    SetLastError(0);
    if (!CreateDirectoryW(path_from_utf8(path).c_str(), NULL))
    {
      DWORD error = GetLastError();
      if (error == ERROR_ALREADY_EXISTS)
        return false;

      if (with_parents)
      {
        if (error == ERROR_PATH_NOT_FOUND)
        {
          // create parents too
          std::string tmp = path;
          std::list<std::string> stack;
          while (!tmp.empty() && !file_exists(tmp))
          {
            stack.push_back(tmp);
            tmp = dirname(tmp);
          }
          if (tmp.empty()) // path is invalid
            throw file_error(strfmt("Could not create directory %s",
                              path.c_str()), ERROR_PATH_NOT_FOUND);

          while (!stack.empty())
          {
            if (!CreateDirectoryW(path_from_utf8(stack.back()).c_str(), NULL))
              throw file_error(strfmt("Could not create directory %s",
                                      path.c_str()), GetLastError());
            stack.pop_back();
          }
          return true;
        }
      }

      throw file_error(strfmt("Could not create directory %s", path.c_str()), GetLastError());
    }
#else
    if (with_parents)
    {
      if (g_mkdir_with_parents(path_from_utf8(path).c_str(), mode) < 0)
        throw file_error(strfmt("Could not create directory %s",
                                path.c_str()), errno);
    }
    else
    {
      // do not use g_mkdir_with_parents here, as we rely on the specific behaviour of g_mkdir (g_mkdir_with_parents doesnt fail if dir exists)
      if (g_mkdir(path_from_utf8(path).c_str(), mode) < 0)
      {
        if (errno == EEXIST)
          return false;
        throw file_error(strfmt("Could not create directory %s",
                                path.c_str()), errno);
      }
    }
#endif
    return true;
  }
  
  void rename(const std::string &from, const std::string &to)
  {
#ifdef _WIN32
    if (!MoveFile(path_from_utf8(from).c_str(), path_from_utf8(to).c_str()))
      throw file_error(strfmt("Could not rename file %s to %s", from.c_str(), to.c_str()), GetLastError());
#else
    if (::g_rename(path_from_utf8(from).c_str(), path_from_utf8(to).c_str()) < 0)
      throw file_error(strfmt("Could not rename file %s to %s", from.c_str(), to.c_str()), errno);
#endif
  }

  bool remove_recursive(const std::string &path)
  {
    GError *error= NULL;
    GDir* dir;
    const char *dir_entry;
    gchar *entry_path;
  
    dir= g_dir_open(path.c_str(), 0, &error);
    if (!dir && error)
      return false;
  
    while ((dir_entry= g_dir_read_name(dir)))
    {
      entry_path= g_build_filename(path.c_str(), dir_entry, NULL);
      if (g_file_test(entry_path, G_FILE_TEST_IS_DIR))
        (void) remove_recursive(entry_path);
      else
        (void) ::g_remove(entry_path);
      g_free(entry_path);
    }
  
    (void) g_rmdir(path.c_str());
  
    g_dir_close(dir);
    return true;
  }
  
  //--------------------------------------------------------------------------------------------------
  
  /**
   * Deletes file or folder.
   * Returns false if the object doesn't exist and throws an exception on error.
   */
  bool remove(const std::string &path)
  {
#ifdef _WIN32
    if (is_directory(path))
    {
      if (!RemoveDirectoryW(path_from_utf8(path).c_str()))
      {
        if (GetLastError() == ERROR_FILE_NOT_FOUND
          || GetLastError() == ERROR_PATH_NOT_FOUND)
          return false;
        throw file_error(strfmt("Could not delete file %s", path.c_str()), GetLastError());
      }
    }
    else
    {
      if (!DeleteFileW(path_from_utf8(path).c_str()))
      {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
          return false;
        throw file_error(strfmt("Could not delete file %s", path.c_str()), GetLastError());
      }
    }
#else
    if (::g_remove(path_from_utf8(path).c_str()) < 0)
    {
      if (errno == ENOENT)
        return false;
      throw file_error(strfmt("Could not delete file %s", path.c_str()), errno);
    }
#endif
    return true;
  }
  
  //--------------------------------------------------------------------------------------------------

  /**
   * Tries to delete a file or folder.
   * No exception is thrown if that fails. Returns true on success, otherwise false.
   */
  bool tryRemove(const std::string &path)
  {
#ifdef _WIN32
    if (is_directory(path))
      return RemoveDirectory(path_from_utf8(path).c_str()) == TRUE;
    else
      return DeleteFile(path_from_utf8(path).c_str()) == TRUE;
#else
    return ::g_remove(path_from_utf8(path).c_str()) == 0;
#endif
  }

  //--------------------------------------------------------------------------------------------------

  bool file_exists(const std::string &path)
  {
    char *f = g_filename_from_utf8(path.c_str(), -1, NULL, NULL, NULL);
    if (g_file_test(f, G_FILE_TEST_EXISTS))
    {
      g_free(f);
      return true;
    }
    g_free(f);
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  bool is_directory(const std::string &path)
  {
    char *f = g_filename_from_utf8(path.c_str(), -1, NULL, NULL, NULL);
    if (g_file_test(f, G_FILE_TEST_IS_DIR))
    {
      g_free(f);
      return true;
    }
    g_free(f);
    return false;
  }
  
  //--------------------------------------------------------------------------------------------------

  std::string extension(const std::string &path)
  {
    std::string::size_type p = path.rfind('.');
    if (p != std::string::npos)
    {
      std::string ext(path.substr(p));
      if (ext.find('/') != std::string::npos || ext.find('\\') != std::string::npos)
        return "";
      return ext;
    }
    return "";
  }
  
  std::string dirname(const std::string &path)
  {
    char *dn = g_path_get_dirname(path.c_str());
    std::string tmp(dn);
    g_free(dn);
    return tmp;
  }
  
  std::string basename(const std::string &path)
  {
    char *dn = g_path_get_basename(path.c_str());
    std::string tmp(dn);
    g_free(dn);
    return tmp;
  }
  
  std::string strip_extension(const std::string &path)
  {
    std::string ext;
    if (!(ext = extension(path)).empty())
    {
      return path.substr(0, path.size() - ext.size());
    }
    return path;
  }

FileHandle::FileHandle(const char *filename, const char *mode, bool throw_on_fail)
: _file(NULL) 
{ 
    _file = base_fopen(filename, mode);
    if (!_file && throw_on_fail)
      throw file_error(std::string("Failed to open file \"").append(filename).append("\""), errno);
}

  FileHandle & FileHandle::operator =(FileHandle &fh)
  {
    dispose();
    swap(fh);
    return *this;
  }
 
  void FileHandle::swap(FileHandle &fh)
  {
    std::swap(_file, fh._file);
  }
 
  void FileHandle::dispose()
  {
    if (_file)
    {
      ::fclose(_file);
      _file= NULL;
    }
  }

/**
 * Returns the last modification time of the given file.
 */
bool file_mtime(const std::string &path, time_t &mtime)
{
#ifdef _WIN32
  struct _stat stbuf;
#else
  struct stat stbuf;
#endif

  if (base_stat(path.c_str(), &stbuf) == 0)
  {
#ifdef __APPLE__
    mtime = (time_t)stbuf.st_mtimespec.tv_sec;
#else
    mtime = stbuf.st_mtime;
#endif
    return true;
  }
  return false;
}

std::string join_path(const char *prefix, ...)
{
  std::string path = prefix;
  char wrong_path_separator = G_DIR_SEPARATOR == '/' ? '\\' : '/';
  std::replace(path.begin(), path.end(), wrong_path_separator, G_DIR_SEPARATOR);
  std::string arg = const_cast<char*>(prefix);
  va_list ap;
  va_start(ap, prefix);
  while (!arg.empty())
  {
    arg = va_arg(ap, char*);
    if (!arg.empty())
    {
      if (path[path.size() - 1] == G_DIR_SEPARATOR)
        path += arg;
      else
        path += G_DIR_SEPARATOR + arg;
    }
  }
  va_end(ap);

  return path;
}

};
