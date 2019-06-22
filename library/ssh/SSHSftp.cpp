/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/log.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include <fcntl.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <vector>
#include "SSHSftp.h"

DEFAULT_LOG_DOMAIN("SSHSftp")

namespace ssh {

  SSHSftp::SSHSftp(std::shared_ptr<SSHSession> session, std::size_t maxFileSize)
      : _session(session), _maxFileLimit(maxFileSize) {

    auto lock = _session->lockSession();
    _sftp = sftp_new(_session->getSession()->getCSession());
    if (_sftp == nullptr)
      throw SSHSftpException(_session->getSession()->getError());

    auto rc = sftp_init(_sftp);
    throwOnError(rc);

    char* path = sftp_canonicalize_path(_sftp, ".");
    if (path == nullptr)
      throw SSHSftpException(_session->getSession()->getError());
    std::string s(path);
    _path = base::split(s, "/", -1);
    // check if first element isn't empty, if so we remove it
    if (_path.front().empty())
      _path.erase(_path.begin());
  }

  void SSHSftp::throwOnError(int rc) const {
    if (rc != SSH_OK)
      throw SSHSftpException(getSftpErrorDescription(sftp_get_error(_sftp)));
  }

  static void cleanPath(std::vector<std::string> &path) {
    auto it = std::remove_if(path.begin(), path.end(), [] (const std::string& val) {
      return val == "";
    }); 
    path.erase(it, path.end());
  }

  static std::string getHumanSftpError(int err) {
    switch(err) {
      case SSH_FX_OK:
        return "There was no error";

      case SSH_FX_EOF:
        return "End of file.";

      case SSH_FX_NO_SUCH_FILE:
        return "File doesn't exist.";

      case SSH_FX_PERMISSION_DENIED:
        return "Permission denied";

      case SSH_FX_FAILURE:
        return "Just a failure.";

      case SSH_FX_BAD_MESSAGE:
        return "Server sent garbage.";

      case SSH_FX_NO_CONNECTION:
        return "There's no connection";

      case SSH_FX_CONNECTION_LOST:
        return "Connection has been lost.";

      case SSH_FX_OP_UNSUPPORTED:
        return "Server doesn't support this operation.";

      case SSH_FX_INVALID_HANDLE:
        return "Invalid file handle.";

      case SSH_FX_NO_SUCH_PATH:
        return "The path to file or directory doesn't exist.";

      case SSH_FX_FILE_ALREADY_EXISTS:
        return "File already exists.";

      case SSH_FX_WRITE_PROTECT:
        return "Filesystem is write protected";

      case SSH_FX_NO_MEDIA:
        return "No media in remote drive";

      default:
        return "The error means nothing.";
    }
  }

  std::string SSHSftp::createRemotePath(const std::string &path) const {
    if (path.empty())
      return "";

    if (path[0] == '/') {  //this is absolute path
      auto tmpPath = base::split(path, "/", -1); // but we need to check the last part of it
      if (tmpPath.back() == ".." ) {
        tmpPath.pop_back();
        tmpPath.pop_back();
      }
      else if (tmpPath.back() == ".") {
        tmpPath.pop_back();
      }
      if (!tmpPath.empty() && tmpPath.front() == "")
        tmpPath.erase(tmpPath.begin());

      return "/" + base::join(tmpPath, "/");
    } else {
      std::vector<std::string> tmpPath(_path.begin(), _path.end());
      if (path == "..") {
        tmpPath.pop_back();
      } else if (path == ".") {
        // noop
      } else {
        tmpPath.push_back(path);
      }
      return "/" + base::join(tmpPath, "/");
    }
    return "/";
  }

  SSHSftp::~SSHSftp() {
    auto lock = _session->lockSession();
    sftp_free(_sftp);
  }

  sftp_file SSHSftp::open(const std::string &path) const {
    sftp_file file = sftp_open(_sftp, createRemotePath(path).c_str(), O_RDONLY, 0);
    if (file == nullptr)
      throw SSHSftpException(_session->getSession()->getError());
    return file;
  }

  void SSHSftp::mkdir(const std::string &dirname, unsigned int mode) {
    auto lock = _session->lockSession();
    auto rc = sftp_mkdir(_sftp, dirname.c_str(), mode);
    if (rc != SSH_OK) {
      auto error = sftp_get_error(_sftp);
      if (error != SSH_FX_FILE_ALREADY_EXISTS)
        throw SSHSftpException(getSftpErrorDescription(error));
      else
        throw SSHSftpException(_session->getSession()->getError());
    }
  }

  void SSHSftp::rmdir(const std::string &dirname) {
    auto lock = _session->lockSession();
    auto rc = sftp_rmdir(_sftp, createRemotePath(dirname).c_str());
    throwOnError(rc);
  }

  void SSHSftp::unlink(const std::string &file) {
    auto lock = _session->lockSession();
    auto rc = sftp_unlink(_sftp, createRemotePath(file).c_str());
    throwOnError(rc);
  }

  SftpStatAttrib SSHSftp::stat(const std::string &path) {
    auto lock = _session->lockSession();
    sftp_attributes info = sftp_stat(_sftp, createRemotePath(path).c_str());
    if (info == nullptr)
      throw SSHSftpException(getSftpErrorDescription(sftp_get_error(_sftp)));

    SftpStatAttrib stat;
    stat.atime = info->atime64;
    stat.mtime = info->mtime64;
    stat.gid = info->gid;
    stat.uid = info->uid;
    stat.size = info->size;
    if (info->name != nullptr)
      stat.name = info->name;
    else
      stat.name = base::basename(path);
    stat.isDir = info->type == SSH_FILEXFER_TYPE_DIRECTORY;
    sftp_attributes_free(info);
    return stat;
  }

  struct ftpFile {
    sftp_file ptr;
    ftpFile(const sftp_file &f) : ptr(f) {};
  };

  using ftpFileUniqueDeleter = std::unique_ptr<ftpFile, std::function<void(ftpFile*)>>;

  ftpFileUniqueDeleter createPtr(sftp_file _file) {
    return ftpFileUniqueDeleter(new ftpFile(_file), [](ftpFile* f) {
      if (f->ptr != nullptr)
        sftp_close(f->ptr);
      delete f;
    });
  }

  void SSHSftp::get(const std::string &src, const std::string &dest) const {
    auto lock = _session->lockSession();
    sftp_file file = sftp_open(_sftp, createRemotePath(src).c_str(), O_RDONLY, 0);
    if (file == nullptr)
      throw SSHSftpException(_session->getSession()->getError());

    base::FileHandle fileHandle;
    try {
      fileHandle = base::FileHandle(dest, "w+", true);
    } catch (base::file_error &fe) {
      throw SSHSftpException(fe.what());
    }

    char buffer[16384];
    while (true) {
      ssize_t nBytes = sftp_read(file, buffer, sizeof(buffer));
      if (nBytes == 0)
        break;
      else if (nBytes < 0) {
        sftp_close(file);
        throw SSHSftpException(_session->getSession()->getError());
      }

      std::size_t nWritten = fwrite(buffer, sizeof(char), nBytes, fileHandle.file());
      if (nBytes > 0 && nWritten != (std::size_t) nBytes) {
        sftp_close(file);
        throw SSHSftpException("Error writing file");
      }
    }

    int rc = sftp_close(file);
    if (rc != SSH_OK)
      throw SSHSftpException(_session->getSession()->getError());
  }

  void SSHSftp::setContent(const std::string &path, const std::string &data) const {
    logDebug3("Set file content: %s\n", path.c_str());
    auto lock = _session->lockSession();
    auto file = createPtr(sftp_open(_sftp, createRemotePath(path).c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU));
    if (file->ptr == nullptr)
      throw SSHSftpException(_session->getSession()->getError());

    ssize_t nWritten = sftp_write(file->ptr, data.c_str(), data.size());
    if (nWritten > 0 && (std::size_t) nWritten != data.size()) {
      throw SSHSftpException("Error writing file");
    }
    logDebug3("File content succesfully saved: %s\n", path.c_str());
  }

  void SSHSftp::put(const std::string &src, const std::string &dest) const {
    auto lock = _session->lockSession();
    auto file = createPtr(sftp_open(_sftp, createRemotePath(src).c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU));

    if (file->ptr == nullptr)
      throw SSHSftpException(_session->getSession()->getError());

    base::FileHandle fileHandle;
    try {
      fileHandle = base::FileHandle(src, "w+", true);
    } catch (base::file_error &fe) {
      throw SSHSftpException(fe.what());
    }

    std::size_t buffSize = 16384;
    std::vector<char> buff;
    buff.reserve(buffSize);
    while (true) {

      std::size_t nBytes = fread(buff.data(), sizeof(char), buffSize, fileHandle.file());
      if (nBytes == buffSize) {
        ssize_t nWritten = sftp_write(file->ptr, buff.data(), buffSize);
        if (nWritten > 0 && (std::size_t) nWritten != buffSize) {
          throw SSHSftpException("Error writing file");
        }
      } else if (feof(fileHandle.file())) {
        ssize_t nWritten = sftp_write(file->ptr, buff.data(), nBytes);
        if (nWritten > 0 && (std::size_t) nWritten != nBytes) {
          throw SSHSftpException("Error writing file");
        }
      } else {
        throw SSHSftpException("Error reading file");
      }
    }
  }

  std::string SSHSftp::getContent(const std::string &src) const {
    auto lock = _session->lockSession();
    auto file = createPtr(sftp_open(_sftp, createRemotePath(src).c_str(), O_RDONLY, 0));
    if (file->ptr == nullptr)
      throw SSHSftpException(_session->getSession()->getError());

    std::string buff;
    size_t bytesCount = 0;
    char buffer[16384];
    while (true) {
      ssize_t nBytes = sftp_read(file->ptr, buffer, sizeof(buffer));
      if (nBytes == 0)
        break;
      else if (nBytes < 0) {
        throw SSHSftpException(_session->getSession()->getError());
      }
      buff.append(buffer, nBytes);
      bytesCount += nBytes;
      if (bytesCount > _maxFileLimit) {
        throw SSHSftpException("Max file limit exceeded\n.");
      }

    }

    return buff;
  }

  void SSHSftp::setMaxFileLimit(std::size_t limit) {
    _maxFileLimit = limit;
  }

  int SSHSftp::cd(const std::string &dirname) {
    auto lock = _session->lockSession();
    if (dirname.empty())
      return false;

    sftp_dir dir = sftp_opendir(_sftp, createRemotePath(dirname).c_str());
    if (dir == nullptr) {
      int err = sftp_get_error(_sftp);
      logError("Can't change directory: %s. Error was: %s\n", createRemotePath(dirname).c_str(), getHumanSftpError(err).c_str());
      if (err == SSH_FX_NO_SUCH_FILE)
        return -1;

      if (err == SSH_FX_PERMISSION_DENIED)
        return -2;

      return 0;
    }

    auto tmpPath = base::split(createRemotePath(dirname), "/", -1);
    cleanPath(tmpPath);
    _path = tmpPath;
    if (!_path.empty() && _path.front().empty())
      _path.erase(_path.begin());

    if (sftp_closedir(dir))
      throw SSHSftpException(_session->getSession()->getError());

    return 1;
  }

  std::vector<SftpStatAttrib> SSHSftp::ls(const std::string &dirname) const {
    auto lock = _session->lockSession();
    std::vector<SftpStatAttrib> entries;
    if (dirname.empty())
      return entries;

    auto dirHandle = sftp_opendir(_sftp, createRemotePath(dirname).c_str());
    if (dirHandle == nullptr) {
      throw SSHSftpException(_session->getSession()->getError());
    }

    sftp_attributes file;
    /* reading the whole directory, file by file */
    while ((file = sftp_readdir(_sftp, dirHandle))) {
      if (file->type == SSH_FILEXFER_TYPE_DIRECTORY || file->type == SSH_FILEXFER_TYPE_REGULAR)
        entries.push_back(
            { file->size, file->uid, file->gid, file->atime64, file->mtime64, file->name, file->type
                == SSH_FILEXFER_TYPE_DIRECTORY });
      sftp_attributes_free(file);
    }

    if (!sftp_dir_eof(dirHandle)) {
      sftp_closedir(dirHandle);
      throw SSHSftpException(_session->getSession()->getError());
    }

    if (sftp_closedir(dirHandle))
      throw SSHSftpException(_session->getSession()->getError());

    return entries;
  }

  std::string SSHSftp::pwd() const {
    return "/" + base::join(_path, "/");
  }

  bool SSHSftp::fileExists(const std::string &path) const {
    auto lock = _session->lockSession();
    sftp_attributes info = sftp_stat(_sftp, createRemotePath(path).c_str());
    if (info == nullptr) {
      auto error = sftp_get_error(_sftp);
      if (SSH_FX_NO_SUCH_FILE == error)
        return false;

      throw SSHSftpException(getSftpErrorDescription(error));
    }

    bool isFile = info->type == SSH_FILEXFER_TYPE_REGULAR;
    sftp_attributes_free(info);
    return isFile;
  }

} /* namespace ssh */

