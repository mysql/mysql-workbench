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

namespace dataTypes {

  JsonParser::JsonValue toJson(const ConnectionType &type) {
    switch (type) {
      case ConnectionClassic:
        return JsonParser::JsonValue("ConnectionClassic");
      case ConnectionNode:
        return JsonParser::JsonValue("ConnectionNode");
    }
    return JsonParser::JsonValue();
  }

  //--------------------------------------------------------------------------------------------------

  void fromJson(const JsonParser::JsonValue &value, ConnectionType &type) {
    if ((std::string)value == "ConnectionClassic")
      type = ConnectionClassic;
    else if ((std::string)value == "ConnectionNode")
      type = ConnectionNode;
    else
      throw std::bad_cast();
  }

  //--------------------------------------------------------------------------------------------------

  JsonParser::JsonValue toJson(const EditorLanguage &lang) {
    switch (lang) {
      case EditorSql:
        return JsonParser::JsonValue("EditorSql");
      case EditorJavaScript:
        return JsonParser::JsonValue("EditorJavaScript");
      case EditorPython:
        return JsonParser::JsonValue("EditorPython");
    }
    return JsonParser::JsonValue();
  }

  //--------------------------------------------------------------------------------------------------

  void fromJson(const JsonParser::JsonValue &value, EditorLanguage &lang) {
    if ((std::string)value == "EditorSql")
      lang = EditorSql;
    else if ((std::string)value == "EditorJavaScript")
      lang = EditorJavaScript;
    else if ((std::string)value == "EditorPython")
      lang = EditorPython;
    else
      throw std::bad_cast();
  }

  //--------------------------------------------------------------------------------------------------

  BaseConnection::BaseConnection(const JsonParser::JsonValue &value) : port(0) {
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

  JsonParser::JsonValue BaseConnection::toJson() const {
    JsonParser::JsonObject o;
    o.insert("className", JsonParser::JsonValue(className));
    o.insert("hostName", JsonParser::JsonValue(hostName));
    o.insert("userName", JsonParser::JsonValue(userName));
    o.insert("port", JsonParser::JsonValue(port));
    return JsonParser::JsonValue(o);
  }

  //--------------------------------------------------------------------------------------------------

  void BaseConnection::fromJson(const JsonParser::JsonValue &value, const std::string &cName) {
    const JsonParser::JsonObject o = value; // May throw.
    if ((std::string)o.get("className") == (cName.empty() ? className : cName))
      throw std::bad_cast();
    hostName = (std::string)o.get("hostName");
    userName = (std::string)o.get("userName");
    port = (int)o.get("port");
  }

  //--------------------------------------------------------------------------------------------------

  SSHConnection::SSHConnection(const JsonParser::JsonValue &value) : BaseConnection(22) {
    fromJson(value);
  }

  //--------------------------------------------------------------------------------------------------

  JsonParser::JsonValue SSHConnection::toJson() const {
    JsonParser::JsonObject o = BaseConnection::toJson();
    o["className"] = JsonParser::JsonValue(className);
    o.insert("keyFile", JsonParser::JsonValue(keyFile));
    return JsonParser::JsonValue(o);
  }

  //--------------------------------------------------------------------------------------------------

  void SSHConnection::fromJson(const JsonParser::JsonValue &value, const std::string &cName) {
    BaseConnection::fromJson(value, className);
    const JsonParser::JsonObject o = value;
    keyFile = (std::string)o.get("keyFile");
  }

  //--------------------------------------------------------------------------------------------------

  NodeConnection::NodeConnection() : BaseConnection(33060), type(ConnectionNode), language(EditorJavaScript) {
    // TODO Auto-generated constructor stub
  }

  //--------------------------------------------------------------------------------------------------

  NodeConnection::NodeConnection(const JsonParser::JsonValue &value)
    : BaseConnection(33060), type(ConnectionNode), language(EditorJavaScript) {
    fromJson(value);
  }

  //--------------------------------------------------------------------------------------------------

  NodeConnection::~NodeConnection() {
    // TODO Auto-generated destructor stub
  }

  //--------------------------------------------------------------------------------------------------

  JsonParser::JsonValue NodeConnection::toJson() const {
    JsonParser::JsonObject o = BaseConnection::toJson();
    o["className"] = JsonParser::JsonValue(className);
    o.insert("defaultSchema", JsonParser::JsonValue(defaultSchema));
    o.insert("uuid", JsonParser::JsonValue(uuid));
    o.insert("type", dataTypes::toJson(type));
    o.insert("language", dataTypes::toJson(language));
    o.insert("ssh", ssh.toJson());
    return JsonParser::JsonValue(o);
  }

  //--------------------------------------------------------------------------------------------------

  void NodeConnection::fromJson(const JsonParser::JsonValue &value, const std::string &cName) {
    BaseConnection::fromJson(value, className);
    const JsonParser::JsonObject o = value;
    uuid = (std::string)o.get("uuid");
    defaultSchema = (std::string)o.get("defaultSchema");
    ssh = SSHConnection(o.get("ssh"));
    dataTypes::fromJson(o.get("type"), type);
    dataTypes::fromJson(o.get("language"), language);
  }

  //--------------------------------------------------------------------------------------------------

  XProject::XProject(const JsonParser::JsonValue &value) : placeholder(false) {
    fromJson(value);
  }

  //--------------------------------------------------------------------------------------------------

  JsonParser::JsonValue XProject::toJson() const {
    JsonParser::JsonObject o;
    o.insert("className", JsonParser::JsonValue(className));
    o.insert("name", JsonParser::JsonValue(name));
    o.insert("connection", connection.toJson());
    return JsonParser::JsonValue(o);
  }

  //--------------------------------------------------------------------------------------------------

  void XProject::fromJson(const JsonParser::JsonValue &value) {
    const JsonParser::JsonObject o = value; // May throw.
    if ((std::string)o.get("className") == className)
      throw std::bad_cast();
    name = (std::string)o.get("name");
    connection = NodeConnection(o.get("connection"));
  }

  //--------------------------------------------------------------------------------------------------

  ProjectHolder::ProjectHolder(const JsonParser::JsonValue &value) {
    fromJson(value);
  }

  //--------------------------------------------------------------------------------------------------

  JsonParser::JsonValue ProjectHolder::toJson() const {
    JsonParser::JsonObject o;
    o.insert("className", JsonParser::JsonValue(className));
    o.insert("isGroup", JsonParser::JsonValue(isGroup));
    o.insert("isRoot", JsonParser::JsonValue(isRoot));
    o.insert("project", JsonParser::JsonValue(project.toJson()));
    JsonParser::JsonArray arr;
    for (auto it : children)
      arr.pushBack(it.toJson());
    o.insert("children", JsonParser::JsonValue(arr));
    return JsonParser::JsonValue(o);
  }

  //--------------------------------------------------------------------------------------------------

  void ProjectHolder::fromJson(const JsonParser::JsonValue &value) {
    const JsonParser::JsonObject o = value; // May throw.
    if ((std::string)o.get("className") == className)
      throw std::bad_cast();

    isGroup = (bool)o.get("isGroup");
    isRoot = (bool)o.get("isRoot");
    project = XProject(o.get("project"));
    const JsonParser::JsonArray array = o.get("children");
    for (auto &it : array)
      children.push_back(ProjectHolder(it));
  }

  //--------------------------------------------------------------------------------------------------

} /* namespace dataTypes */
