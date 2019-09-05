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

#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/metadata.h>

#include "dbresult.h"

#include "utilities.h"
#include "scripting-context.h"
#include "filesystem.h"
#include "path.h"

#include "db.h"

using namespace mga;

//------------------ DbConnection --------------------------------------------------------------------------------------

void DbConnection::registerInContext(ScriptingContext &context, JSObject &exports) {
  std::ignore = context;
  exports.defineClass("DbConnection", "", 1, [](JSObject *instance, JSValues &args) {
    void *value = args.get(0);
    auto backendConnection = reinterpret_cast<DbConnection *>(value);
    instance->setBacking(backendConnection);
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("currentSchema", getter, setter);
    prototype.defineVirtualProperty("autoCommit", getter, setter);
    prototype.defineVirtualProperty("catalog", getter, setter);
    prototype.defineVirtualProperty("readOnly", getter, setter);
    prototype.defineVirtualProperty("valid", getter, setter);
    prototype.defineVirtualProperty("connected", getter, nullptr);
    prototype.defineVirtualProperty("clientInfo", getter, nullptr);
    prototype.defineVirtualProperty("version", getter, nullptr);

    prototype.defineFunction({ "disconnect" }, 0, [](JSExport *native, JSValues &args) {
      std::ignore = args;
      auto me = dynamic_cast<DbConnection *>(native);
      me->disconnect();
    });

    prototype.defineFunction({ "query" }, 1, [](JSExport *native, JSValues &args) {
      auto me = dynamic_cast<DbConnection *>(native);

      std::string query = args.as(ValueType::String, 0);
      try {
        auto res = me->query(query);
        args.pushResult(args.context()->createJsInstance("DbResult", { res }));
      } catch (std::exception &exc) {
        args.context()->throwScriptingError(ScriptingError::Error, exc.what());
      }
    });

    prototype.defineFunction({ "executeScript" }, 3, [](JSExport *native, JSValues &args) {
      auto me = dynamic_cast<DbConnection *>(native);

      std::string script = args.as(ValueType::String, 0);
      std::string delimiter = args.get(1, ";");
      bool ignoreErrors = args.get(2, false);
      try {
        me->executeScript(script, delimiter, ignoreErrors);
      } catch (std::exception &exc) {
        args.context()->throwScriptingError(ScriptingError::Error, exc.what());
      }
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

DbConnection::DbConnection(std::unique_ptr<sql::Connection> conn) : _connection(std::move(conn)) {
}

//----------------------------------------------------------------------------------------------------------------------

void DbConnection::disconnect() {
  if (_connection) {
    _connection->close();
    _connection.release();
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool DbConnection::isConnected() {
  if (!_connection)
    return false;

  return !_connection->isClosed();
}

//----------------------------------------------------------------------------------------------------------------------

// Describes a single statement out of a list in a string.
typedef struct {
  size_t line;   // The line number of the statement.
  size_t start;  // The byte start offset of the statement.
  size_t length; // The length of the statements in bytes.
} StatementRange;

static const unsigned char *skipLeadingWhitespace(const unsigned char *head, const unsigned char *tail) {
  while (head < tail && *head <= ' ')
    head++;
  return head;
}

//----------------------------------------------------------------------------------------------------------------------

static bool isLineBreak(const unsigned char *head, const unsigned char *line_break) {
  if (*line_break == '\0')
    return false;

  while (*head != '\0' && *line_break != '\0' && *head == *line_break) {
    head++;
    line_break++;
  }
  return *line_break == '\0';
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * A statement splitter to take a list of sql statements and split them into individual statements,
 * return their position and length in the original string (instead the copied strings).
 */
static size_t determineStatementRanges(const char *sql, size_t length, const std::string &initialDelimiter,
                                       std::vector<StatementRange> &ranges, const std::string &lineBreak) {

  static const unsigned char keyword[] = "delimiter";

  std::string delimiter = initialDelimiter.empty() ? ";" : initialDelimiter;
  const unsigned char *delimiterHead = reinterpret_cast<const unsigned char *>(delimiter.c_str());

  const unsigned char *start = reinterpret_cast<const unsigned char *>(sql);
  const unsigned char *head = start;
  const unsigned char *tail = head;
  const unsigned char *end = head + length;
  const unsigned char *newLine = reinterpret_cast<const unsigned char *>(lineBreak.c_str());

  size_t currentLine = 0;
  size_t statementStart = 0;
  bool haveContent = false; // Set when anything else but comments were found for the current statement.

  while (tail < end) {
    switch (*tail) {
      case '/': { // Possible multi line comment or hidden (conditional) command.
        if (*(tail + 1) == '*') {
          tail += 2;
          bool isHiddenCommand = (*tail == '!');
          while (true) {
            while (tail < end && *tail != '*') {
              if (isLineBreak(tail, newLine))
                ++currentLine;
              tail++;
            }

            if (tail == end) // Unfinished comment.
              break;
            else {
              if (*++tail == '/') {
                tail++; // Skip the slash too.
                break;
              }
            }
          }

          if (isHiddenCommand)
            haveContent = true;
          if (!haveContent) {
            head = tail; // Skip over the comment.
            statementStart = currentLine;
          }

        } else
          tail++;

        break;
      }

      case '-': { // Possible single line comment.
        const unsigned char *end_char = tail + 2;
        if (*(tail + 1) == '-' && (*end_char == ' ' || *end_char == '\t' || isLineBreak(end_char, newLine))) {
          // Skip everything until the end of the line.
          tail += 2;
          while (tail < end && !isLineBreak(tail, newLine))
            tail++;

          if (!haveContent) {
            head = tail;
            statementStart = currentLine;
          }
        } else
          tail++;

        break;
      }

      case '#': { // MySQL single line comment.
        while (tail < end && !isLineBreak(tail, newLine))
          tail++;

        if (!haveContent) {
          head = tail;
          statementStart = currentLine;
        }

        break;
      }

      case '"':
      case '\'':
      case '`': { // Quoted string/id. Skip this in a local loop.
        haveContent = true;
        unsigned char quote = *tail++;
        while (tail < end && *tail != quote) {
          // Skip any escaped character too.
          if (*tail == '\\')
            tail++;
          tail++;
        }
        if (*tail == quote)
          tail++; // Skip trailing quote char if one was there.

        break;
      }

      case 'd':
      case 'D': {
        haveContent = true;

        // Possible start of the keyword DELIMITER. Must be at the start of the text or a character,
        // which is not part of a regular MySQL identifier (0-9, A-Z, a-z, _, $, \u0080-\uffff).
        unsigned char previous = tail > start ? *(tail - 1) : 0;
        bool is_identifier_char = previous >= 0x80 || (previous >= '0' && previous <= '9') ||
        ((previous | 0x20) >= 'a' && (previous | 0x20) <= 'z') || previous == '$' ||
        previous == '_';
        if (tail == start || !is_identifier_char) {
          const unsigned char *run = tail + 1;
          const unsigned char *kw = keyword + 1;
          int count = 9;
          while (count-- > 1 && (*run++ | 0x20) == *kw++)
            ;
          if (count == 0 && *run == ' ') {
            // Delimiter keyword found. Get the new delimiter (everything until the end of the line).
            tail = run++;
            while (run < end && !isLineBreak(run, newLine))
              ++run;
            delimiter = Utilities::trim(std::string(reinterpret_cast<const char *>(tail), static_cast<size_t>(run - tail)));
            delimiterHead = reinterpret_cast<const unsigned char *>(delimiter.c_str());

            // Skip over the delimiter statement and any following line breaks.
            while (isLineBreak(run, newLine)) {
              ++currentLine;
              ++run;
            }
            tail = run;
            head = tail;
            statementStart = currentLine;
          } else
            ++tail;
        } else
          ++tail;

        break;
      }

      default:
        if (isLineBreak(tail, newLine)) {
          ++currentLine;
          if (!haveContent)
            ++statementStart;
        }

        if (*tail > ' ')
          haveContent = true;
        tail++;
        break;
    }

    if (*tail == *delimiterHead) {
      // Found possible start of the delimiter. Check if it really is.
      size_t count = delimiter.size();
      if (count == 1) {
        // Most common case. Trim the statement and check if it is not empty before adding the range.
        head = skipLeadingWhitespace(head, tail);
        if (head < tail)
          ranges.push_back({ statementStart, static_cast<size_t>(head - start), static_cast<size_t>(tail - head) });
        head = ++tail;
        statementStart = currentLine;
        haveContent = false;
      } else {
        const unsigned char *run = tail + 1;
        const unsigned char *del = delimiterHead + 1;
        while (count-- > 1 && (*run++ == *del++))
          ;

        if (count == 0) {
          // Multi char delimiter is complete. Tail still points to the start of the delimiter.
          // Run points to the first character after the delimiter.
          head = skipLeadingWhitespace(head, tail);
          if (head < tail)
            ranges.push_back({ statementStart, static_cast<size_t>(head - start), static_cast<size_t>(tail - head) });
          tail = run;
          head = run;
          statementStart = currentLine;
          haveContent = false;
        }
      }
    }
  }

  // Add remaining text to the range list.
  head = skipLeadingWhitespace(head, tail);
  if (head < tail)
    ranges.push_back({ statementStart, static_cast<size_t>(head - start), static_cast<size_t>(tail - head) });

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

DbResult* DbConnection::query(const std::string &query) {
  if (!isConnected())
    throw std::runtime_error("Not connected");

  auto result = new DbResult(_connection->createStatement(), query);
  if (!result->isValid()) {
    auto msg = result->getError();
    delete result;
    throw std::runtime_error(msg);
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void DbConnection::executeScript(const std::string &script, const std::string &delimiter, bool ignoreErrors) {
  if (!isConnected())
    throw std::runtime_error("MySQL connection is not active");

  doExecuteScript(script, delimiter, ignoreErrors);
}

//----------------------------------------------------------------------------------------------------------------------

void DbConnection::doExecuteScript(const std::string &script, const std::string &delimiter, bool ignoreErrors) {
  auto content = FS::contentFromFile(script);

  std::vector<StatementRange> ranges;
  determineStatementRanges(content.c_str(), content.size(), delimiter, ranges, "\n");

  std::unique_ptr<sql::Statement> statement(_connection->createStatement());
  for (auto &range : ranges) {
    auto query = std::string(content.c_str() + range.start, range.length);
    try {
      if (Utilities::trimLeft(query).find("source ") == 0) {
        auto sourceFile = Path::join({ Path::dirname(script), Utilities::trim(query.substr(7, query.size())) });
        doExecuteScript(sourceFile, delimiter, ignoreErrors);
        continue;
      }

      statement->execute(query);
    } catch (sql::SQLException &) {
      if (!ignoreErrors)
        throw;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::map<std::string, size_t> propertyMap = {
  { "currentSchema", 0 },
  { "autoCommit", 1 },
  { "catalog", 2 },
  { "readOnly", 3 },
  { "valid", 4 },
  { "connected", 5 },
  { "clientInfo", 6 },
  { "version", 7 }
};

/**
 * Getter for our virtual properties.
 */
JSVariant DbConnection::getter(ScriptingContext *, JSExport *instance, std::string const& key) {
  auto me = dynamic_cast<DbConnection *>(instance);

  size_t index = propertyMap[key];

  if (index == 5) {
    return me->isConnected();
  }

  if (index != 5 && !me->isConnected()) // 5 is "connected"
    throw std::runtime_error("Not connected");

  switch (index) { // Can only be what we specified on registration.
    case 0: {
      return me->_connection->getSchema().c_str();
    }

    case 1: {
      return me->_connection->getAutoCommit();
    }

    case 2: {
      return me->_connection->getCatalog().c_str();
    }

    case 3: {
      return me->_connection->isReadOnly();
    }

    case 4: {
      return me->_connection->isValid();
    }

    case 5: // Handled above.
      break;

    case 6: {
      sql::Driver *driver = me->_connection->getDriver();
      std::string info = Utilities::format("%s, %i.%i.%i", driver->getName().c_str(), driver->getMajorVersion(),
                                       driver->getMinorVersion(), driver->getPatchVersion());
      return info;
    }

    case 7: {
      sql::DatabaseMetaData *metaData(me->_connection->getMetaData());
      std::string version = metaData->getDatabaseProductVersion();
      return version;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Setter for our virtual properties.
 */
void DbConnection::setter(ScriptingContext *, JSExport *instance, std::string const& key, JSVariant const& value) {
  size_t index = propertyMap[key];

  auto me = dynamic_cast<DbConnection *>(instance);

  if (!me->isConnected())
    throw std::runtime_error("Not connected");

  switch (index) {
    case 0: {
      std::string t = value;
      me->_connection->setSchema(static_cast<std::string>(value));
      break;
    }

    case 1:
      me->_connection->setAutoCommit(value);
      break;

    case 2: {
      me->_connection->setCatalog(static_cast<std::string>(value));
      break;
    }

    case 3:
      me->_connection->setReadOnly(value);
      break;

    case 4: // No setter for these.
    case 5:
    case 6:
      break;

  }
}

//----------------------------------------------------------------------------------------------------------------------

void DbConnection::finalize(ScriptingContext *context) {
  std::ignore = context;

  disconnect();
}

//------------------ Db ------------------------------------------------------------------------------------------------

sql::Driver* Db::mysqlDriver = nullptr;

void Db::establishConnection(JSExport *instance, JSValues &args) {
  std::ignore = instance;
  if (!args.is(ValueType::Object, 0))
    args.context()->throwScriptingError(ScriptingError::Type, "ConnectionOptions expected");
  JSObject options = args.as(ValueType::Object, 0);

  // Take over all settings.
  sql::ConnectOptionsMap opts;
  try {
    for (auto &property : options.getPropertyKeys()) {
      auto value = options.get(property);
      if (value.is(ValueType::Boolean))
        opts[property] = static_cast<bool>(value);
      else if (value.is(ValueType::String))
        opts[property] = static_cast<std::string>(value);
      else
        opts[property] = static_cast<int>(value);
    }

    if (opts.find("OPT_CHARSET_NAME") == opts.end()) {
      opts["OPT_CHARSET_NAME"] = "utf8";
    }

    if (Db::mysqlDriver == nullptr)
      Db::mysqlDriver = get_driver_instance();

    std::unique_ptr<sql::Connection> ptr(Db::mysqlDriver->connect(opts));
    DbConnection *connection = new DbConnection(std::move(ptr));
    args.pushResult(args.context()->createJsInstance("DbConnection", { connection }));
  } catch (sql::SQLException &se) {
    args.context()->throwScriptingError(ScriptingError::Error, se.what());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Db::activate(ScriptingContext &context, JSObject &exports) {
  exports.defineFunction({ "connect" }, 1, establishConnection);

  DbConnection::registerInContext(context, exports);
  DbResult::registerInContext(context, exports);
}

//----------------------------------------------------------------------------------------------------------------------

void Db::threadCleanup() {
  if (Db::mysqlDriver != nullptr)
    Db::mysqlDriver->threadEnd();
}

//----------------------------------------------------------------------------------------------------------------------

bool Db::_registered = []() {
  ScriptingContext::registerModule("mysql", &Db::activate);
  return true;
}();

