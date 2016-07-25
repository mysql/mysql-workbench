/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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


// This file store general data types used between classic and X WB.

#pragma once

#include <string>
#include <vector>

#include "common.h"
#include "jsonparser.h"
#include <typeinfo>

namespace dataTypes {

enum ConnectionType {
  ConnectionClassic,
  ConnectionNode
};

JsonParser::JsonValue toJson(const ConnectionType &type);
void fromJson(const JsonParser::JsonValue &value, ConnectionType &type);

enum EditorLanguage {
  EditorSql,
  EditorJavaScript,
  EditorPython
};

JsonParser::JsonValue toJson(const EditorLanguage &lang);
void fromJson(const JsonParser::JsonValue &value, EditorLanguage &lang);

struct BASELIBRARY_PUBLIC_FUNC AppOptions
 {
   std::string basedir;
   std::string pluginSearchPath;
   std::string structSearchPath;
   std::string moduleSearchPath;
   std::string jsModuleSearchPath;
   std::string librarySearchPath;
   std::string cdbcDriverSearchPath;
   std::string userDataDir;
};

class BASELIBRARY_PUBLIC_FUNC BaseConnection {
private:
  std::string className = "BaseConnection";
public:
  std::string hostName;
  ssize_t port;
  std::string userName;
  std::string userPassword;
  BaseConnection() : port(0) {};
  BaseConnection(ssize_t p) : port(p) {};
  BaseConnection(const JsonParser::JsonValue &value);
  virtual ~BaseConnection() {};

  bool isValid() const {
    return (!hostName.empty() && !userName.empty());
  }

  std::string uri(bool withPassword = false) const;
  std::string hostIdentifier() const;

  virtual JsonParser::JsonValue toJson() const;
  virtual void fromJson(const JsonParser::JsonValue &value, const std::string &cName = "");

};

class BASELIBRARY_PUBLIC_FUNC SSHConnection : public BaseConnection {
private:
  std::string className = "SSHConnection";
public:
  std::string keyFile;
  SSHConnection() : BaseConnection(22) { }
  SSHConnection(const JsonParser::JsonValue &value);
  virtual ~SSHConnection() {};
  virtual JsonParser::JsonValue toJson() const;
  virtual void fromJson(const JsonParser::JsonValue &value, const std::string &cName = "");
};

class BASELIBRARY_PUBLIC_FUNC NodeConnection : public BaseConnection {
private:
  std::string className = "NodeConnection";

public:
  SSHConnection ssh;
  std::string defaultSchema;
  std::string uuid;
  ConnectionType type;
  EditorLanguage language;
  NodeConnection();
  NodeConnection(const JsonParser::JsonValue &value);
  virtual ~NodeConnection();
  virtual JsonParser::JsonValue toJson() const;
  virtual void fromJson(const JsonParser::JsonValue &value, const std::string &cName = "");
};

class BASELIBRARY_PUBLIC_FUNC XProject {
private:
  std::string className = "XProject";
public:
  bool placeholder;
  std::string name;
  NodeConnection connection;
  bool isValid() const { return !name.empty() && connection.isValid(); };
  XProject() : placeholder(false) {};
  XProject(const JsonParser::JsonValue &value);
  virtual ~XProject() {};
  JsonParser::JsonValue toJson() const;
  void fromJson(const JsonParser::JsonValue &value);

};

class BASELIBRARY_PUBLIC_FUNC ProjectHolder {
private:
  std::string className = "ProjectHolder";
public:
  std::string name;
  bool isGroup;
  bool isRoot;
  std::vector<ProjectHolder> children;
  XProject project;
  ProjectHolder() : isGroup(false), isRoot(false) {};
  ProjectHolder(const JsonParser::JsonValue &value);
  virtual ~ProjectHolder() {};
  JsonParser::JsonValue toJson() const;
  void fromJson(const JsonParser::JsonValue &value);
};

} /* namespace dataTypes */
