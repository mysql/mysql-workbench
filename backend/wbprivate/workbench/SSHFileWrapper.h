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


#pragma once

#include "grt.h"
#include "SSHSftp.h"
#include "objimpl/db.mgmt/db_mgmt_SSHFile.h"
#include "wb_backend_public_interface.h"

namespace ssh {
  class SSHSession;
  class MYSQLWBBACKEND_PUBLIC_FUNC SSHFileWrapper : public db_mgmt_SSHFile::ImplData {
    std::shared_ptr<SSHSession> _session;
    std::shared_ptr<SSHSftp> _sftp;
    sftp_file _file;
    std::size_t _maxFileLimit;
    std::string _path;
  public:
    SSHFileWrapper(std::shared_ptr<SSHSession> session, std::shared_ptr<SSHSftp> ftp, const std::string &path, const std::size_t maxFileLimit);
    virtual ~SSHFileWrapper();
    virtual grt::StringRef getPath();
    virtual grt::StringRef read(const size_t length);
    virtual grt::StringRef readline();
    virtual grt::IntegerRef seek(const size_t offset);
    virtual grt::IntegerRef tell();
  };
}  /* namespace ssh */
