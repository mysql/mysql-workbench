/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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


#include "SSHFileWrapper.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN("SSHFileWrapper")

ssh::SSHFileWrapper::SSHFileWrapper(std::shared_ptr<ssh::SSHSession> session, std::shared_ptr<SSHSftp> ftp, const std::string &path, const std::size_t maxFileLimit)
    : _session(session), _sftp(ftp), _maxFileLimit(maxFileLimit), _path(path) {
  _file = _sftp->open(path);
  logDebug3("Open file: %s\n", _path.c_str());
}

ssh::SSHFileWrapper::~SSHFileWrapper() {
  logDebug3("Close file: %s\n", _path.c_str());
  auto lock = _session->lockSession();
  sftp_close(_file);
}

grt::StringRef ssh::SSHFileWrapper::getPath() {
  return _path;
}

grt::StringRef ssh::SSHFileWrapper::read(const size_t length) {
  auto lock = _session->lockSession();
  std::vector<char> buffer;
  logDebug3("Resizing read buffer: %zu\n", length);
  buffer.resize(length);
  ssize_t nBytes = sftp_read(_file, buffer.data(), buffer.size());
  if (nBytes < 0)
    throw SSHSftpException(ssh_get_error(_file->sftp->session));

  std::string buff;
  buff.append(buffer.data(), nBytes);

  return buff;
}

grt::StringRef ssh::SSHFileWrapper::readline() {
  auto lock = _session->lockSession();
  std::string buff;
  size_t bytesCount = 0;
  char buffer;
  while (true) {
    ssize_t nBytes = sftp_read(_file, &buffer, 1);
    if (nBytes == 0)
      break;
    else if (nBytes < 0) {
      throw SSHSftpException(ssh_get_error(_file->sftp->session));
    }
    buff.append(nBytes, buffer);
    bytesCount += nBytes;
    if (buffer == '\n')
      break;

    if (bytesCount > _maxFileLimit) {
      throw SSHSftpException("Max file limit exceeded\n.");
    }
  }
  return buff;
}

grt::IntegerRef ssh::SSHFileWrapper::seek(const size_t offset) {
  auto lock = _session->lockSession();
  return sftp_seek64(_file, offset);
}

grt::IntegerRef ssh::SSHFileWrapper::tell() {
  auto lock = _session->lockSession();
  return (std::size_t)sftp_tell64(_file);
}
