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

#ifndef LIBRARY_BASE_DATA_TYPES_H_
#define LIBRARY_BASE_DATA_TYPES_H_

#include <string>
#include <vector>

namespace dataTypes {

enum ConnectionType {
  ConnectionClassic,
  ConnectionNode
};

enum EditorLanguage {
  EditorSql,
  EditorJavaScript,
  EditorPython
};

class BaseConnection {
public:
  std::string hostName;
  ssize_t port;
  std::string userName;
  std::string userPassword;
  BaseConnection() : port(0) {};
  BaseConnection(ssize_t p) : port(p) {};
  virtual ~BaseConnection() {};

  bool isValid() const {
    return (!hostName.empty() && !userName.empty());
  }

  std::string uri() const {
      return userName + "@" + hostName + ":" + std::to_string(port);
  }
};

class SshConnection : public BaseConnection {
public:
  std::string keyFile;
  SshConnection() : BaseConnection(22) { }
  virtual ~SshConnection() {};
};

class nodeConnection : public BaseConnection {
public:
  SshConnection ssh;
  std::string defaultSchema;
  ConnectionType type;
  EditorLanguage language;
  nodeConnection();
  virtual ~nodeConnection();
};

class XProject {
public:
  std::string name;
  nodeConnection connection;
  bool isValid() { return !name.empty(); };
};

class ProjectHolder {
public:
  std::string name;
  bool isGroup;
  bool isRoot;
  std::vector<ProjectHolder> children;
  XProject project;
  ProjectHolder() : isGroup(false), isRoot(false) {};
};

} /* namespace dataTypes */

#endif /* LIBRARY_BASE_DATA_TYPES_H_ */
