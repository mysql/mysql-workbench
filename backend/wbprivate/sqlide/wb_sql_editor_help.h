/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <thread>
#include "rapidjson/document.h"

// Helper class to find context sensitive help based on a statement and a position in it.

namespace JsonParser {
  class JsonObject;
}

namespace help {

  class MYSQLWBBACKEND_PUBLIC_FUNC HelpContext {
  public:
    HelpContext(GrtCharacterSetsRef charsets, const std::string &sqlMode, long serverVersion);
    ~HelpContext();

    long serverVersion() const;

  private:
    friend class DbSqlEditorContextHelp;

    class Private;
    Private *_d;
  };

  // Exported ony for public for tests.
  class MYSQLWBBACKEND_PUBLIC_FUNC DbSqlEditorContextHelp {
  public:
    static DbSqlEditorContextHelp *get();

    void waitForLoading();

    bool helpTextForTopic(HelpContext *helpContext, const std::string &topic, std::string &text);
    std::string helpTopicFromPosition(HelpContext *helpContext, const std::string &query, size_t caretPosition);

  protected:
    std::thread loaderThread;
    std::map<std::string, std::string> pageMap;
    std::map<long, std::set<std::string>> helpTopics;               // Quick lookup for help topics per server version.
    std::map<long, std::map<std::string, std::string>> helpContent; // Help text from a topic (also per version).

    DbSqlEditorContextHelp();
    ~DbSqlEditorContextHelp();

    std::string createHelpTextFromJson(long version, rapidjson::Value const &json);
    bool topicExists(long serverVersion, const std::string &topic);
  };

} // namespace help
