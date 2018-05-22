/*
 * Copyright (c) 2017, 2018 Oracle and/or its affiliates. All rights reserved.
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

#pragma once
#include <grts/structs.db.mgmt.h>

#include "wbpublic_public_interface.h"

//------------------------------------------------------------------------------------------------
class WBPUBLICBACKEND_PUBLIC_FUNC db_mgmt_SSHConnection::ImplData {
public:
  ImplData();
  virtual ~ImplData();
  virtual void disconnect() = 0;
  virtual grt::IntegerRef isConnected() = 0;
  virtual grt::IntegerRef connect() = 0;
  virtual grt::DictRef executeCommand(const std::string &text) = 0;
  virtual grt::DictRef executeSudoCommand(const std::string &text, const std::string &user) = 0;
  virtual grt::IntegerRef cd(const std::string &directory) = 0;
  virtual void get(const std::string &src, const std::string &dest) = 0;
  virtual grt::StringRef getContent(const std::string &src) = 0;
  virtual grt::DictListRef ls(const std::string &path) = 0;
  virtual void mkdir(const std::string &directory) = 0;
  virtual db_mgmt_SSHFileRef open(const std::string &path) = 0;
  virtual void put(const std::string &src, const std::string &dest) = 0;
  virtual grt::StringRef pwd() = 0;
  virtual void rmdir(const std::string &directory) = 0;
  virtual void setContent(const std::string &path, const std::string &content) = 0;
  virtual grt::DictRef stat(const std::string &path) = 0;
  virtual void unlink(const std::string &file) = 0;
  virtual grt::IntegerRef fileExists(const std::string &path) = 0;
};
