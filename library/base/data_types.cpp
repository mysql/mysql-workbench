/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

// This file store general data types used between classic and X WB.

#include "base/data_types.h"
#include <typeinfo>

using namespace rapidjson;
using namespace std::string_literals;

namespace dataTypes {

  Value toJson(const ConnectionType &type) {
    switch (type) {
      case ConnectionClassic:
        return Value("ConnectionClassic");
      case ConnectionNode:
        return Value("ConnectionNode");
    }
    return Value();
  }

  //--------------------------------------------------------------------------------------------------

  void fromJson(const Value &value, ConnectionType &type) {
    if (value.GetString() == "ConnectionClassic"s)
      type = ConnectionClassic;
    else if (value.GetString() == "ConnectionNode"s)
      type = ConnectionNode;
    else
      throw std::bad_cast();
  }

  //--------------------------------------------------------------------------------------------------

  Value toJson(const EditorLanguage &lang) {
    switch (lang) {
      case EditorSql:
        return Value("EditorSql");
      case EditorJavaScript:
        return Value("EditorJavaScript");
      case EditorPython:
        return Value("EditorPython");
    }
    return Value();
  }

  //--------------------------------------------------------------------------------------------------

  void fromJson(const Value &value, EditorLanguage &lang) {
    if (value.GetString() == "EditorSql"s)
      lang = EditorSql;
    else if (value.GetString() == "EditorJavaScript"s)
      lang = EditorJavaScript;
    else if (value.GetString() == "EditorPython"s)
      lang = EditorPython;
    else
      throw std::bad_cast();
  }

  //--------------------------------------------------------------------------------------------------

  BaseConnection::BaseConnection(const Value &value) : port(0) {
    fromJson(value);
  }

  std::string BaseConnection::uri(bool withPassword) const {
    std::vector<std::string> v;

    v.push_back(hostName);
    v.push_back(std::to_string(port));
    v.push_back(userName);
    if (!userPassword.empty() && withPassword)
      v.push_back(userPassword);

    std::string uri;
    if (v.size() == 4) // if there's no pw, we will ask for it later
      uri = v[2] + ":" + v[3] + "@" + v[0] + ":" + v[1];
    else
      uri = v[2] + "@" + v[0] + ":" + v[1];
    return uri;
  }

  //--------------------------------------------------------------------------------------------------

  std::string BaseConnection::hostIdentifier() const {
    return hostName + ":" + std::to_string(port);
  }

  //--------------------------------------------------------------------------------------------------

  Value BaseConnection::toJson() const {
    Document document;
    Value o(kObjectType);
    o.AddMember("className", className, document.GetAllocator());
    o.AddMember("hostName", hostName, document.GetAllocator());
    o.AddMember("userName", userName, document.GetAllocator());
    o.AddMember("port", static_cast<unsigned>(port), document.GetAllocator());
    return o;
  }

  //--------------------------------------------------------------------------------------------------

  void BaseConnection::fromJson(const Value &value, const std::string &cName) {
    if (value["className"] == (cName.empty() ? className : cName))
      throw std::bad_cast();
    hostName = value["hostName"].GetString();
    userName = value["userName"].GetString();
    port = value["port"].GetInt();
  }

  //--------------------------------------------------------------------------------------------------

  SSHConnection::SSHConnection(const Value &value) : BaseConnection(22) {
    fromJson(value);
  }

  //--------------------------------------------------------------------------------------------------

  Value SSHConnection::toJson() const {
    Value o = BaseConnection::toJson();
    Document document;
    o.AddMember("className", className, document.GetAllocator());
    o.AddMember("keyFile", keyFile, document.GetAllocator());
    return o;
  }

  //--------------------------------------------------------------------------------------------------

  void SSHConnection::fromJson(const Value &value, const std::string &cName) {
    BaseConnection::fromJson(value, className);
    keyFile = value["keyFile"].GetString();
  }

  //--------------------------------------------------------------------------------------------------

  NodeConnection::NodeConnection() : BaseConnection(33060), type(ConnectionNode), language(EditorJavaScript) {
    // TODO Auto-generated constructor stub
  }

  //--------------------------------------------------------------------------------------------------

  NodeConnection::NodeConnection(const Value &value)
    : BaseConnection(33060), type(ConnectionNode), language(EditorJavaScript) {
    fromJson(value);
  }

  //--------------------------------------------------------------------------------------------------

  NodeConnection::~NodeConnection() {
    // TODO Auto-generated destructor stub
  }

  //--------------------------------------------------------------------------------------------------

  Value NodeConnection::toJson() const {
    Value o = BaseConnection::toJson();
    Document document;
    o.AddMember("className", className, document.GetAllocator());
    o.AddMember("defaultSchema", defaultSchema, document.GetAllocator());
    o.AddMember("uuid", uuid, document.GetAllocator());
    o.AddMember("type", dataTypes::toJson(type), document.GetAllocator());
    o.AddMember("language", dataTypes::toJson(language), document.GetAllocator());
    o.AddMember("ssh", ssh.toJson(), document.GetAllocator());
    return o;
  }

  //--------------------------------------------------------------------------------------------------

  void NodeConnection::fromJson(const Value &value, const std::string &cName) {
    BaseConnection::fromJson(value, className);
    uuid = value["uuid"].GetString();
    defaultSchema = value["defaultSchema"].GetString();
    ssh = SSHConnection(value["ssh"]);
    dataTypes::fromJson(value["type"], type);
    dataTypes::fromJson(value["language"], language);
  }

  //--------------------------------------------------------------------------------------------------

  XProject::XProject(const Value &value) : placeholder(false) {
    fromJson(value);
  }

  //--------------------------------------------------------------------------------------------------

  Value XProject::toJson() const {
    Document document;
    Value val;
    val.AddMember("className", className, document.GetAllocator());
    val.AddMember("name", name, document.GetAllocator());
    val.AddMember("connection", connection.toJson(), document.GetAllocator());
    return val;
  }

  //--------------------------------------------------------------------------------------------------

  void XProject::fromJson(const Value &value) {
    if (value["className"] == className)
      throw std::bad_cast();
    name = value["name"].GetString();
    connection = NodeConnection(value["connection"]);
  }

  //--------------------------------------------------------------------------------------------------

  ProjectHolder::ProjectHolder(const Value &value) {
    fromJson(value);
  }

  //--------------------------------------------------------------------------------------------------

  Value ProjectHolder::toJson() const {
    Value o;
    Document document;
    Document::AllocatorType &allocator = document.GetAllocator();
    o.AddMember("className", className, allocator);
    o.AddMember("isGroup", isGroup, allocator);
    o.AddMember("isRoot", isRoot, allocator);
    o.AddMember("project", project.toJson(), allocator);

    Value arr(kArrayType);
    for (auto it : children)
      arr.PushBack(it.toJson(), allocator);
    o.AddMember("children", arr, allocator);
    return o;
  }

  //--------------------------------------------------------------------------------------------------

  void ProjectHolder::fromJson(const Value &value) {
    Document doscument;
    if (value["className"].GetString() == className)
      throw std::bad_cast();

    isGroup = value["isGroup"].GetBool();
    isRoot = value["isRoot"].GetBool();
    project = XProject(value["project"]);
    for (auto &it : value["children"].GetArray())
      children.push_back(ProjectHolder(it));
  }

  //--------------------------------------------------------------------------------------------------

} /* namespace dataTypes */
