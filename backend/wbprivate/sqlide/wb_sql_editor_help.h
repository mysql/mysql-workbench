/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

// Helper class to find context sensitive help based on a statement and a position in it.

namespace help {

class MYSQLWBBACKEND_PUBLIC_FUNC HelpContext {
public:
  HelpContext(GrtCharacterSetsRef charsets, const std::string &sqlMode, long serverVersion);
  ~HelpContext();

private:
  friend class DbSqlEditorContextHelp;
  
  class Private;
  Private *_d;
};

class MYSQLWBBACKEND_PUBLIC_FUNC DbSqlEditorContextHelp // Made public for tests only.
{
public:
  static DbSqlEditorContextHelp* get();

  bool helpTextForTopic(const std::string &topic, std::string &title, std::string &text);
  std::string helpTopicFromPosition(HelpContext *context, const std::string &query, std::pair<size_t, size_t> caret);

protected:
  DbSqlEditorContextHelp() {};

  bool topicExists(const std::string &topic);
};

} // namespace help
