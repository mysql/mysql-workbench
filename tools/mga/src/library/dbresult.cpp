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

#include "utilities.h"
#include "scripting-context.h"

#include "dbresult.h"
#include <cppconn/exception.h>

using namespace mga;

//----------------------------------------------------------------------------------------------------------------------

static std::string getColumnType(int const& t) {
  std::string type;
  switch (t) {
    case sql::DataType::UNKNOWN:
      type = "unknown";
      break;

    case sql::DataType::BIT:
    case sql::DataType::TINYINT:
    case sql::DataType::SMALLINT:
    case sql::DataType::MEDIUMINT:
    case sql::DataType::INTEGER:
    case sql::DataType::BIGINT:
      type = "numeric";
      break;

    case sql::DataType::REAL:
    case sql::DataType::DOUBLE:
      type = "numeric";
      break;

    case sql::DataType::DECIMAL:
    case sql::DataType::NUMERIC:
      type = "numeric";
      break;

    case sql::DataType::CHAR:
    case sql::DataType::VARCHAR:
      type = "string";
      break;

    case sql::DataType::BINARY:
    case sql::DataType::VARBINARY:
    case sql::DataType::LONGVARCHAR:
    case sql::DataType::LONGVARBINARY:
      type = "blob";
      break;

    case sql::DataType::TIMESTAMP:
      type = "string";
      break;
    case sql::DataType::DATE:
      type = "string";
      break;
    case sql::DataType::TIME:
      type = "string";
      break;

    case sql::DataType::YEAR:
      type = "numeric";
      break;
    case sql::DataType::GEOMETRY:
      type = "string";
      break;
    case sql::DataType::ENUM:
    case sql::DataType::SET:
      type = "string";
      break;
    case sql::DataType::JSON:
      type = "json";
      break;
    case sql::DataType::SQLNULL:
      type = "null";
      break;
    default:
      type = "unknown";
      break;
  }
  return type;
}

//----------------------------------------------------------------------------------------------------------------------

DbResult::~DbResult() {
}

//----------------------------------------------------------------------------------------------------------------------

void DbResult::registerInContext(ScriptingContext &context, JSObject &exports) {
  std::ignore = context;
  exports.defineClass("DbResult", "", 1, [](JSObject *instance, JSValues &args) {
    void *value = args.get(0);
    instance->setBacking(reinterpret_cast<DbResult *>(value));
  }, [](JSObject &prototype) {
    prototype.defineFunction({ "next" }, 0, [](JSExport *instance, JSValues &args) {
      auto *me = dynamic_cast<DbResult *>(instance);
      if (!me->_result)
        return;

      args.pushResult(me->_result->next());
    });

    prototype.defineFunction({ "getMoreResults" }, 0, [](JSExport *instance, JSValues &args) {
      auto *me = dynamic_cast<DbResult *>(instance);
      if (!me->_result)
        return;

      if (me->_statement->getMoreResults()) {
        me->_result = std::unique_ptr<sql::ResultSet>(me->_statement->getResultSet());
        me->_metaData = me->_result->getMetaData();
        args.pushResult(true);
      } else {
        args.pushResult(false);
      }
    });

    prototype.defineFunction({ "fetchObject" }, 0, [](JSExport *instance, JSValues &args) {
      auto *me = dynamic_cast<DbResult *>(instance);
      if (!me->_result)
        return;

      if (me->_result->isBeforeFirst())
        args.context()->throwScriptingError(ScriptingError::Error,
          "Can't fetch object because not on result set, missed next()?");
      else {
        JSObject object(args.context());

        for (unsigned int i = 1; i <= me->_metaData->getColumnCount(); ++i) {
          std::string property = me->_metaData->getColumnLabel(i);
          object.set(property, me->getResultValue(i));
        }

        args.pushResult(object);
      }
    });

    prototype.defineFunction({ "fetchArray" }, 0, [](JSExport *instance, JSValues &args) {
      auto *me = dynamic_cast<DbResult *>(instance);
      if (!me->_result)
        return;

      if (me->_result->isBeforeFirst())
        args.context()->throwScriptingError(ScriptingError::Error,
          "Can't fetch array because not on result set, missed next()?");
      else {
        JSArray array(args.context());

        for (unsigned int i = 1; i <= me->_metaData->getColumnCount(); ++i) {
          std::string property = me->_metaData->getColumnLabel(i);
          array.addValue(me->getResultValue(i));
        }

        args.pushResult(array);
      }
    });

    prototype.defineFunction({ "getColumnLabels" }, 0, [](JSExport *instance, JSValues &args) {
      auto *me = dynamic_cast<DbResult *>(instance);
      if (!me->_result)
        return;

      try {
        JSArray array(args.context());

        for (unsigned int i = 1; i <= me->_metaData->getColumnCount(); ++i) {
          array.addValue(me->_metaData->getColumnLabel(i).c_str());
        }
        args.pushResult(array);
      } catch (std::exception &exc) {
        args.context()->throwScriptingError(ScriptingError::Error, exc.what());
      }
    });

    prototype.defineFunction({ "getColumnNames" }, 0, [](JSExport *instance, JSValues &args) {
      auto *me = dynamic_cast<DbResult *>(instance);
      if (!me->_result)
        return;

      try {
        JSArray array(args.context());

        for (unsigned int i = 1; i <= me->_metaData->getColumnCount(); ++i) {
          array.addValue(me->_metaData->getColumnName(i).c_str());
        }
        args.pushResult(array);
      } catch (std::exception &exc) {
        args.context()->throwScriptingError(ScriptingError::Error, exc.what());
      }
    });

    prototype.defineFunction({ "getUpdateCount" }, 0, [](JSExport *instance, JSValues &args) {
      auto *me = dynamic_cast<DbResult *>(instance);

      args.pushResult(static_cast<unsigned>(me->_statement->getUpdateCount()));
    });

    prototype.defineFunction({ "getLastInsertId" }, 0, [](JSExport *instance, JSValues &args) {
      auto *me = dynamic_cast<DbResult *>(instance);

      std::unique_ptr<sql::ResultSet> result(me->_statement->executeQuery("SELECT LAST_INSERT_ID()"));
      if (result->next())
        args.pushResult(static_cast<unsigned>(result->getInt(1)));
      else
        args.pushResult(0);
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

bool DbResult::isValid() {
  return !_invalid;
}

//----------------------------------------------------------------------------------------------------------------------

std::string DbResult::getError() {
  return _errorMessage;
}

//----------------------------------------------------------------------------------------------------------------------

DbResult::DbResult(sql::Statement *statement, const std::string &query) {
  _invalid = false;
  _metaData = nullptr;
  _statement = std::unique_ptr<sql::Statement>(statement);
  try {
    if (_statement->execute(query)) {
      _result = std::unique_ptr<sql::ResultSet>(_statement->getResultSet());
      _metaData = _result->getMetaData();
    }
  } catch (sql::SQLException &exc) {
    _invalid = true;
    _errorMessage = exc.what();
  }
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant DbResult::getResultValue(unsigned index) const {
  auto colType = getColumnType(_metaData->getColumnType(index));
  if (colType == "string")
    return _result->getString(index).c_str();
  else if (colType == "numeric")
    return static_cast<double>(_result->getDouble(index));
  else if (colType == "json" || colType == "blob")
    return JSVariant(_result->getString(index).c_str(), colType == "json");
  else if (colType == "null")
    return JSVariant(nullptr);

  return JSVariant();
}

//----------------------------------------------------------------------------------------------------------------------
