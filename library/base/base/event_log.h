/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifdef _WIN32

#include "common.h"

struct BASELIBRARY_PUBLIC_FUNC EventLogReader {
  EventLogReader(const std::string &query,
                 const std::function<void(std::map<std::string, std::string> &output)> &printResults);
  void ReadEvents();
  void SetPosition(long position);

private:
  DWORD PrintResults(EVT_HANDLE results);
  DWORD PrintEvent(EVT_HANDLE hEvent);
  DWORD GetQueryStatusProperty(EVT_QUERY_PROPERTY_ID Id, EVT_HANDLE hResults, PEVT_VARIANT &pProperty);
  DWORD PrintQueryStatuses(EVT_HANDLE hResults);
  std::string GetMessageString(EVT_HANDLE metadata, EVT_HANDLE eventHandle);

  std::function<void(std::map<std::string, std::string> &output)> _printResultsCallback;
  std::string _query;
  long _position;
};

#endif
