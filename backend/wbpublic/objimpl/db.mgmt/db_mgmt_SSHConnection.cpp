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

#include <grts/structs.db.mgmt.h>

#include "db_mgmt_SSHConnection.h"
#include <grtpp_util.h>

//------------------------------------------------------------------------------------------------

db_mgmt_SSHConnection::ImplData::ImplData() {

}

//------------------------------------------------------------------------------------------------

db_mgmt_SSHConnection::ImplData::~ImplData() {
}

//------------------------------------------------------------------------------------------------

void db_mgmt_SSHConnection::init() {
//  if (!_data) _data= new db_mgmt_SSHConnection::ImplData();
}

//------------------------------------------------------------------------------------------------

db_mgmt_SSHConnection::~db_mgmt_SSHConnection() {
  delete _data;
}

//------------------------------------------------------------------------------------------------

void db_mgmt_SSHConnection::set_data(ImplData *data) {
  _data = data;
}

//------------------------------------------------------------------------------------------------

void db_mgmt_SSHConnection::disconnect() {
  if (_data)
    _data->disconnect();
}

grt::IntegerRef db_mgmt_SSHConnection::isConnected() {
  if (_data)
    return _data->isConnected();
  return 0;
}

//------------------------------------------------------------------------------------------------

grt::IntegerRef db_mgmt_SSHConnection::connect() {
  if (_data)
    return _data->connect();
  return -1;
}

//------------------------------------------------------------------------------------------------

grt::DictRef db_mgmt_SSHConnection::executeCommand(const std::string &text) {
  if (_data)
    return _data->executeCommand(text);
  grt::DictRef dict(true);
  dict.gset("stdout", "");
  dict.gset("stderr", "");
  dict.gset("stderr", -1);
  return dict;
}

//------------------------------------------------------------------------------------------------

grt::DictRef db_mgmt_SSHConnection::executeSudoCommand(const std::string &text, const std::string &user) {
  if (_data)
    return _data->executeSudoCommand(text, user);
  grt::DictRef dict(true);
  dict.gset("stdout", "");
  dict.gset("stderr", "");
  dict.gset("stderr", -1);
  return dict;
}

//------------------------------------------------------------------------------------------------

grt::IntegerRef db_mgmt_SSHConnection::cd(const std::string &directory) {
  if (_data)
    return _data->cd(directory);
  return 0;
}

//------------------------------------------------------------------------------------------------

void db_mgmt_SSHConnection::get(const std::string &src, const std::string &dest) {
  if (_data)
    _data->get(src, dest);
}

//------------------------------------------------------------------------------------------------

grt::StringRef db_mgmt_SSHConnection::getContent(const std::string &src) {
  if (_data)
    return _data->getContent(src);
  return "";
}

//------------------------------------------------------------------------------------------------

grt::DictListRef db_mgmt_SSHConnection::ls(const std::string &path) {
  if (_data)
    return _data->ls(path);
  return grt::DictListRef();
}

//------------------------------------------------------------------------------------------------

void db_mgmt_SSHConnection::mkdir(const std::string &directory) {
  if (_data)
    _data->mkdir(directory);
}

//------------------------------------------------------------------------------------------------

db_mgmt_SSHFileRef db_mgmt_SSHConnection::open(const std::string &path) {
  if (_data)
    return _data->open(path);
  return db_mgmt_SSHFileRef();
}

//------------------------------------------------------------------------------------------------

void db_mgmt_SSHConnection::put(const std::string &src, const std::string &dest) {
  if (_data)
    _data->put(src, dest);
}

//------------------------------------------------------------------------------------------------

grt::StringRef db_mgmt_SSHConnection::pwd() {
  if (_data)
    return _data->pwd();
  return "";
}

//------------------------------------------------------------------------------------------------

void db_mgmt_SSHConnection::rmdir(const std::string &directory) {
  if (_data)
    _data->rmdir(directory);
}

//------------------------------------------------------------------------------------------------

void db_mgmt_SSHConnection::setContent(const std::string &path, const std::string &content) {
  if (_data)
    _data->setContent(path, content);
}

//------------------------------------------------------------------------------------------------

grt::DictRef db_mgmt_SSHConnection::stat(const std::string &path) {
  if (_data)
    return _data->stat(path);
  return grt::DictRef();
}

//------------------------------------------------------------------------------------------------

grt::IntegerRef db_mgmt_SSHConnection::fileExists(const std::string &path) {
  if (_data)
    return _data->fileExists(path);

  return grt::IntegerRef();
}

//------------------------------------------------------------------------------------------------

void db_mgmt_SSHConnection::unlink(const std::string &file) {
  if (_data)
    _data->unlink(file);
}

//------------------------------------------------------------------------------------------------

