/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <map>

#include "common.h"
#include "rapidjson/document.h"
#include <typeinfo>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace dataTypes {

  enum ConnectionType { ConnectionClassic, ConnectionNode };

  rapidjson::Value toJson(const ConnectionType &type);
  void fromJson(const rapidjson::Value &value, ConnectionType &type);

  enum EditorLanguage { EditorSql, EditorJavaScript, EditorPython };

  rapidjson::Value toJson(const EditorLanguage &lang);
  void fromJson(const rapidjson::Value &value, EditorLanguage &lang);

  struct BASELIBRARY_PUBLIC_FUNC AppOptions {
    std::string basedir;
    std::string pluginSearchPath;
    std::string structSearchPath;
    std::string moduleSearchPath;
    std::string jsModuleSearchPath;
    std::string librarySearchPath;
    std::string cdbcDriverSearchPath;
    std::string userDataDir;
  };

  enum OptionArgumentType { OptionArgumentNumeric, OptionArgumentText, OptionArgumentLogical, OptionArgumentFilename };

  class BASELIBRARY_PUBLIC_FUNC OptionEntry {
  public:
    struct mixType {
      bool logicalValue;
      std::string textValue;
      int numericValue;
      OptionArgumentType type;

      mixType() : logicalValue(false), textValue(""), numericValue(0), type(OptionArgumentLogical) {
      }
      mixType(int i) : logicalValue(false), textValue(""), numericValue(i), type(OptionArgumentNumeric) {
      }
      mixType(const std::string &s) : logicalValue(false), textValue(s), numericValue(0), type(OptionArgumentText) {
      }
      mixType(bool b) : logicalValue(b), textValue(""), numericValue(0), type(OptionArgumentLogical) {
      }

      int &operator=(int &&other) {
        if (type != OptionArgumentNumeric)
          throw std::runtime_error("Can't mix types");
        numericValue = other;
        return numericValue;
      }

      std::string &operator=(std::string &&other) {
        if (type != OptionArgumentText && type != OptionArgumentFilename)
          throw std::runtime_error("Can't mix types");
        textValue = std::move(other);
        return textValue;
      }

      bool &operator=(bool &&other) {
        if (type != OptionArgumentLogical)
          throw std::runtime_error("Can't mix types");
        logicalValue = other;
        return logicalValue;
      }

      ~mixType() {
      }
    };

    char shortName;
    std::string longName;
    std::string argName;
    std::string description;
    mixType value;
    typedef std::function<bool(const OptionEntry &, int *retval)> optionCb;
    optionCb callback;
    OptionEntry(OptionArgumentType argType, const std::string &l, const std::string &d, optionCb cb = nullptr,
                const std::string &a = "")
      : shortName(0), longName(l), argName(a), description(d), callback(cb) {
      value.type = argType;
      switch (argType) {
        case OptionArgumentNumeric:
          value.numericValue = 0;
          break;
        case OptionArgumentLogical:
          value.logicalValue = false;
          break;
        case OptionArgumentText:
        case OptionArgumentFilename:
          break;
      }
    }

    OptionEntry(OptionArgumentType argType, char s, const std::string &l, const std::string &d, optionCb cb = nullptr,
                const std::string &a = "")
      : shortName(s), longName(l), argName(a), description(d), callback(cb) {
      value.type = argType;
      switch (argType) {
        case OptionArgumentNumeric:
          value.numericValue = 0;
          break;
        case OptionArgumentLogical:
          value.logicalValue = false;
          break;
        case OptionArgumentText:
        case OptionArgumentFilename:
          break;
      }
    }
  };

  class BASELIBRARY_PUBLIC_FUNC ArgumentParser {
    const char *ptr;

  public:
    ArgumentParser(const std::string &line) : ptr(line.data()){};

    std::string getArgName() {
      if (std::strncmp(ptr, "--", sizeof("--") - 1) != 0) {
        // This means it's not an argument name
        return std::string();
      }
      ptr += 2;
      std::string argName;
      while (true) {
        if (*ptr == ' ' || *ptr == '=' || *ptr == '\0')
          break;
        else
          argName.push_back(*ptr);
        ++ptr;
      }
      return argName;
    }
    std::string getArgValue() {
      std::string retVal;
      do {
        if (*ptr == '\0')
          return retVal;
        if (*ptr != ' ' && *ptr != '=') {
          retVal.assign(ptr);
          return retVal;
        }
        ++ptr;
      } while (true);

      return retVal;
    }
  };

  class BASELIBRARY_PUBLIC_FUNC OptionsList {
  public:
    typedef std::map<std::string, OptionEntry> entryList;
    OptionsList(){};
    void addEntry(const OptionEntry &entry) {
      _list.insert({entry.longName, entry});
    }

    entryList *getEntries() {
      return &_list;
    }

    OptionEntry *getEntry(const std::string name) {
      auto it = _list.find(name);
      if (it != _list.end())
        return &it->second;
      return nullptr;
    }
    std::vector<std::string> pathArgs;

    bool parse(const std::vector<std::string> &args, int &retVal) {
      for (auto it = args.begin(); it != args.end(); it++) {
        ArgumentParser a(*it);

        std::string name = a.getArgName();
        if (!name.empty()) {
          auto optIt = _list.find(name);
          if (optIt == _list.end())
            throw std::runtime_error("Unknown argument");

          if (optIt->second.value.type != OptionArgumentLogical) {
            if (!a.getArgValue().empty()) {
              if (!setArgumentValue(optIt->second, a.getArgValue(), &retVal))
                return false;
            } else {
              auto nextIt = it;
              do {
                ++nextIt;
                if (nextIt == args.end())
                  throw std::runtime_error("Argument is missing value");
                ArgumentParser nextArg(*nextIt);
                if (!nextArg.getArgName().empty())
                  throw std::runtime_error("Argument is missing value");

                if (!setArgumentValue(optIt->second, *nextIt, &retVal))
                  return false;

                ++it;
                break;

              } while (nextIt != args.end());
            }
          } else {
            optIt->second.value.logicalValue = true;
            if (optIt->second.callback) {
              if (!optIt->second.callback(optIt->second, &retVal))
                return false;
            }
          }
        } else
          pathArgs.push_back(*it);
      }
      return true;
    }

    std::string getHelp(const std::string &binaryName) {
      std::stringstream ss;
      ss << binaryName;
      ss << " [<options>] [<name of a model file or sql script>]";
      ss << std::endl;
      ss << "Options:";
      ss << std::endl;
      for (auto &o : _list) {
        auto entry = o.second;
        std::string param;
        if (entry.shortName != 0) {
          param += "-";
          param += entry.shortName;
          if (!entry.argName.empty())
            param += " " + entry.argName;
          param += ", ";
        }

        param += "--" + entry.longName;
        if (!entry.argName.empty())
          param += " " + entry.argName;

        ss << '\t' << std::setw(30) << std::left << param;
        ss << std::setw(20) << entry.description;
        ss << std::endl;
      }
      ss << std::endl;
      return ss.str();
    }

  protected:
    entryList _list;
    bool setArgumentValue(OptionEntry &entry, const std::string &val, int *retval) {
      switch (entry.value.type) {
        case OptionArgumentNumeric:
          entry.value.numericValue = atoi((val).c_str());
          break;
        case OptionArgumentFilename:
        case OptionArgumentText:
          entry.value.textValue = val;
          break;
        default:
          throw std::runtime_error("Unhandled value type");
      }

      if (entry.callback)
        return entry.callback(entry, retval);

      return true;
    }
  };

  class BASELIBRARY_PUBLIC_FUNC BaseConnection {
  private:
    std::string className = "BaseConnection";

  public:
    std::string hostName;
    ssize_t port;
    std::string userName;
    std::string userPassword;
    BaseConnection() : port(0){};
    BaseConnection(ssize_t p) : port(p){};
    BaseConnection(const rapidjson::Value &value);
    virtual ~BaseConnection(){};

    bool isValid() const {
      return (!hostName.empty() && !userName.empty());
    }

    std::string uri(bool withPassword = false) const;
    std::string hostIdentifier() const;

    virtual rapidjson::Value toJson() const;
    virtual void fromJson(const rapidjson::Value &value, const std::string &cName = "");
  };

  class BASELIBRARY_PUBLIC_FUNC SSHConnection : public BaseConnection {
  private:
    std::string className = "SSHConnection";

  public:
    std::string keyFile;
    SSHConnection() : BaseConnection(22) {
    }
    SSHConnection(const rapidjson::Value &value);
    virtual ~SSHConnection(){};
    virtual rapidjson::Value toJson() const;
    virtual void fromJson(const rapidjson::Value &value, const std::string &cName = "");
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
    NodeConnection(const rapidjson::Value &value);
    virtual ~NodeConnection();
    virtual rapidjson::Value toJson() const;
    virtual void fromJson(const rapidjson::Value &value, const std::string &cName = "");
  };

  class BASELIBRARY_PUBLIC_FUNC XProject {
  private:
    std::string className = "XProject";

  public:
    bool placeholder;
    std::string name;
    NodeConnection connection;
    bool isValid() const {
      return !name.empty() && connection.isValid();
    };
    XProject() : placeholder(false){};
    XProject(const rapidjson::Value &value);
    virtual ~XProject(){};
    rapidjson::Value toJson() const;
    void fromJson(const rapidjson::Value &value);
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
    ProjectHolder() : isGroup(false), isRoot(false){};
    ProjectHolder(const rapidjson::Value &value);
    virtual ~ProjectHolder(){};
    rapidjson::Value toJson() const;
    void fromJson(const rapidjson::Value &value);
  };

} /* namespace dataTypes */
