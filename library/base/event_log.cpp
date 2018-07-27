/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/event_log.h"
#include "base/string_utilities.h"
#ifdef _MSC_VER
#include "Objbase.h"

#pragma comment(lib, "wevtapi.lib")

EventLogReader::EventLogReader(const std::string &query,
                               const std::function<void(std::map<std::string, std::string> &output)> &printResults)
  : _query(query), _printResultsCallback(printResults), _position(0) {
}

/**
 * @brief Set last view position.
 */
void EventLogReader::SetPosition(long position) {
  _position = position;
}

/**
 * @brief Get the list of events.
 */
void EventLogReader::ReadEvents() {
  DWORD status = ERROR_SUCCESS;
  std::wstring buf = base::string_to_wstring(_query);
  EVT_HANDLE results = EvtQuery(nullptr, nullptr, buf.c_str(), EvtQueryChannelPath | EvtQueryTolerateQueryErrors);
  if (results == nullptr)
    return;

  if (!EvtSeek(results, _position, nullptr, 0, EvtSeekRelativeToFirst))
    return;

  if (PrintQueryStatuses(results) == ERROR_SUCCESS)
    PrintResults(results);

  if (results)
    EvtClose(results);
}

/**
 * @brief Get the list of paths in the query and the status for each path. Return
 *        the sum of the statuses, so the caller can decide whether to enumerate
          the results.

 * @param results The handle to a query or subscription result set that
 *                the EvtQuery function or the EvtSubscribe function returns.
 *
 * @return returns Status code.
 */
DWORD EventLogReader::PrintQueryStatuses(EVT_HANDLE results) {
  DWORD status = ERROR_SUCCESS;
  PEVT_VARIANT paths = nullptr;
  PEVT_VARIANT statuses = nullptr;
  do {
    if ((status = GetQueryStatusProperty(EvtQueryNames, results, paths)) != ERROR_SUCCESS)
      break;
    if ((status = GetQueryStatusProperty(EvtQueryStatuses, results, statuses)) != ERROR_SUCCESS)
      break;
    for (DWORD i = 0; i < paths->Count; i++)
      status += statuses->UInt32Arr[i];
  } while (false);
  if (paths != nullptr)
    free(paths);
  if (statuses != nullptr)
    free(statuses);
  return status;
}

/**
 * @brief Enumerate all the events in the result set.

 * @param results The handle to a query or subscription result set that
 *                the EvtQuery function or the EvtSubscribe function returns.
 *
 * @return returns Status code.
 */
DWORD EventLogReader::PrintResults(EVT_HANDLE results) {
  DWORD status = ERROR_SUCCESS;
  static const int ArraySize = 10;

  DWORD returned = 0;
  EVT_HANDLE events[ArraySize] = {0};
  bool run = true;
  while (run) {
    // Get a block of events from the result set.
    if (!EvtNext(results, ArraySize, events, INFINITE, 0, &returned))
      break;
    // For each event, call the PrintEvent function which renders the
    // event for display. PrintEvent is shown in RenderingEvents.
    for (DWORD i = 0; i < returned; i++) {
      _position++;
      status = PrintEvent(events[i]);
      if (status == ERROR_SUCCESS) {
        EvtClose(events[i]);
        events[i] = nullptr;
      } else {
        run = false;
        break;
      }
    }
  }
  for (DWORD i = 0; i < returned; i++) {
    if (events[i] != nullptr)
      EvtClose(events[i]);
  }
  return status;
}

/**
 * @brief Get the list of paths specified in the query or the list of status values
 *        for each path.
 * @param id The identifier of the query information to retrieve.
 * @param results The handle to a query or subscription result set.
 * @param property A caller-allocated buffer that will receive the query information.
 *                 The buffer contains an EVT_VARIANT object.
 *
 * @return returns Print event status.
 */
DWORD EventLogReader::GetQueryStatusProperty(EVT_QUERY_PROPERTY_ID id, EVT_HANDLE results, PEVT_VARIANT &property) {
  DWORD status = ERROR_SUCCESS;
  DWORD bufferSize = 0;
  DWORD bufferUsed = 0;
  if (!EvtGetQueryInfo(results, id, bufferSize, property, &bufferUsed)) {
    status = GetLastError();
    if (ERROR_INSUFFICIENT_BUFFER == status) {
      bufferSize = bufferUsed;
      property = (PEVT_VARIANT)malloc(bufferSize);
      if (property != nullptr) {
        EvtGetQueryInfo(results, id, bufferSize, property, &bufferUsed);
        status = GetLastError();
      } else {
        status = ERROR_OUTOFMEMORY;
      }
    }
  }
  return status;
}

/**
 * @brief Enumerate all the events in the result set.
 * @param eventHandle A handle to an event.
 *
 * @return returns Print event status.
 */
DWORD EventLogReader::PrintEvent(EVT_HANDLE eventHandle) {
  EVT_HANDLE context = nullptr;
  PEVT_VARIANT renderedValues = nullptr;
  DWORD status = ERROR_SUCCESS;
  do {
    // Identify the components of the event that you want to render. In this case,
    // render the system section of the event.
    context = EvtCreateRenderContext(0, nullptr, EvtRenderContextSystem);
    if (context == nullptr) {
      status = GetLastError();
      break;
    }
    // When you render the user data or system section of the event, you must specify
    // the EvtRenderEventValues flag. The function returns an array of variant values
    // for each element in the user data or system section of the event. For user data
    // or event data, the values are returned in the same order as the elements are
    // defined in the event. For system data, the values are returned in the order defined
    // in the EVT_SYSTEM_PROPERTY_ID enumeration.
    DWORD bufferSize = 0;
    DWORD bufferUsed = 0;
    DWORD propertyCount = 0;
    if (!EvtRender(context, eventHandle, EvtRenderEventValues, bufferSize, renderedValues, &bufferUsed,
                   &propertyCount)) {
      status = GetLastError();
      if (status == ERROR_INSUFFICIENT_BUFFER) {
        bufferSize = bufferUsed;
        renderedValues = (PEVT_VARIANT)malloc(bufferSize);
        if (renderedValues != nullptr) {
          EvtRender(context, eventHandle, EvtRenderEventValues, bufferSize, renderedValues, &bufferUsed,
                    &propertyCount);
          status = GetLastError();
        } else {
          status = ERROR_OUTOFMEMORY;
          break;
        }
      }
      if (status != ERROR_SUCCESS)
        break;
    }
    std::map<std::string, std::string> eventData;

    std::wstring tempBuf =
      (renderedValues[EvtSystemProviderName].StringVal) ? renderedValues[EvtSystemProviderName].StringVal : L"";
    eventData["providername"] = base::wstring_to_string(tempBuf);
    if (renderedValues[EvtSystemProviderGuid].GuidVal != nullptr) {
      WCHAR guid[50] = {0};
      StringFromGUID2(*(renderedValues[EvtSystemProviderGuid].GuidVal), guid, sizeof(guid) / sizeof(WCHAR));
      eventData["providerguid"] = base::wstring_to_string(guid);
    }

    DWORD eventId = renderedValues[EvtSystemEventID].UInt16Val;
    if (renderedValues[EvtSystemQualifiers].Type == EvtVarTypeNull)
      eventId = MAKELONG(renderedValues[EvtSystemEventID].UInt16Val, renderedValues[EvtSystemQualifiers].UInt16Val);
    char buf[1024] = {0};
    snprintf(buf, sizeof(buf), "%lu", eventId);
    eventData["eventid"] = buf;

    snprintf(buf, sizeof(buf), "%u",
             (renderedValues[EvtSystemVersion].Type == EvtVarTypeNull) ? 0 : renderedValues[EvtSystemVersion].ByteVal);
    eventData["version"] = buf;

    snprintf(buf, sizeof(buf), "%u",
             (renderedValues[EvtSystemLevel].Type == EvtVarTypeNull) ? 0 : renderedValues[EvtSystemLevel].ByteVal);
    eventData["level"] = buf;

    snprintf(buf, sizeof(buf), "%hu",
             (renderedValues[EvtSystemTask].Type == EvtVarTypeNull) ? 0 : renderedValues[EvtSystemTask].ByteVal);
    eventData["task"] = buf;

    snprintf(buf, sizeof(buf), "%u",
             (renderedValues[EvtSystemOpcode].Type == EvtVarTypeNull) ? 0 : renderedValues[EvtSystemOpcode].UInt16Val);
    eventData["opcode"] = buf;

    snprintf(buf, sizeof(buf), "0x%I64x", (renderedValues[EvtSystemKeywords].Type == EvtVarTypeNull)
                                            ? 0
                                            : renderedValues[EvtSystemOpcode].UInt64Val);
    eventData["keywords"] = buf;

    ULONGLONG ullTimeStamp = renderedValues[EvtSystemTimeCreated].FileTimeVal;
    FILETIME ft;
    ft.dwHighDateTime = (DWORD)((ullTimeStamp >> 32) & 0xFFFFFFFF);
    ft.dwLowDateTime = (DWORD)(ullTimeStamp & 0xFFFFFFFF);
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    ULONGLONG ullNanoseconds =
      (ullTimeStamp % 10000000) * 100; // Display nanoseconds instead of milliseconds for higher resolution
    snprintf(buf, sizeof(buf), "%02d/%02d/%02d %02d:%02d:%02d.%I64u", st.wMonth, st.wDay, st.wYear, st.wHour,
             st.wMinute, st.wSecond, ullNanoseconds);
    eventData["timecreated"] = buf;

    snprintf(buf, sizeof(buf), "%I64u", renderedValues[EvtSystemEventRecordId].UInt64Val);
    eventData["eventrecordid"] = buf;

    if (renderedValues[EvtSystemActivityID].Type != EvtVarTypeNull) {
      WCHAR guid[50] = {0};
      StringFromGUID2(*(renderedValues[EvtSystemActivityID].GuidVal), guid, sizeof(guid) / sizeof(WCHAR));
      ;
      eventData["activityid"] = base::wstring_to_string(guid);
    }

    if (renderedValues[EvtSystemRelatedActivityID].Type != EvtVarTypeNull) {
      WCHAR guid[50] = {0};
      StringFromGUID2(*(renderedValues[EvtSystemRelatedActivityID].GuidVal), guid, sizeof(guid) / sizeof(WCHAR));
      ;
      eventData["relatedactivityid"] = base::wstring_to_string(guid);
    }

    snprintf(buf, sizeof(buf), "%lu", renderedValues[EvtSystemProcessID].UInt32Val);
    eventData["processid"] = buf;

    snprintf(buf, sizeof(buf), "%lu", renderedValues[EvtSystemThreadID].UInt32Val);
    eventData["threadid"] = buf;

    tempBuf =
      (renderedValues[EvtSystemChannel].Type == EvtVarTypeNull) ? renderedValues[EvtSystemChannel].StringVal : L"";
    eventData["channel"] = base::wstring_to_string(tempBuf);

    eventData["computer"] = base::wstring_to_string(renderedValues[EvtSystemComputer].StringVal);

    if (renderedValues[EvtSystemUserID].Type != EvtVarTypeNull) {
      LPWSTR pwsSid = nullptr;
      if (ConvertSidToStringSid(renderedValues[EvtSystemUserID].SidVal, &pwsSid)) {
        eventData["secuserid"] = base::wstring_to_string(pwsSid);
        LocalFree(pwsSid);
      }
    }
    // Get the handle to the provider's metadata that contains the message strings.
    EVT_HANDLE providerMetadata =
      EvtOpenPublisherMetadata(nullptr, renderedValues[EvtSystemProviderName].StringVal, nullptr, 0, 0);
    if (providerMetadata == nullptr)
      break;
    eventData["message"] = GetMessageString(providerMetadata, eventHandle);
    _printResultsCallback(eventData);
  } while (false);

  if (context)
    EvtClose(context);
  if (renderedValues)
    free(renderedValues);
  return status;
}

/**
 * @brief Gets the specified message string from the event. If the event does not
 *        contain the specified message, the function returns empty string.
 * @param metadata A handle to the provider's metadata that the EvtOpenPublisherMetadata function returns.
 *                 The handle acts as a formatting context for the event or message identifier
 * @param eventHandle A handle to an event.
 *
 * @return returns message string from the event.
 */
std::string EventLogReader::GetMessageString(EVT_HANDLE metadata, EVT_HANDLE eventHandle) {
  DWORD bufferSize = 0;
  DWORD bufferUsed = 0;
  DWORD status = 0;
  std::vector<WCHAR> buffer;
  if (!EvtFormatMessage(metadata, eventHandle, 0, 0, nullptr, EvtFormatMessageEvent, bufferSize, nullptr,
                        &bufferUsed)) {
    status = GetLastError();
    if (ERROR_INSUFFICIENT_BUFFER == status) {
      bufferSize = bufferUsed;
      buffer.resize(bufferUsed);
      EvtFormatMessage(metadata, eventHandle, 0, 0, nullptr, EvtFormatMessageEvent, bufferSize, &buffer[0],
                       &bufferUsed);
    }
  }
  std::wstring temp(buffer.begin(), buffer.end());
  return base::wstring_to_string(temp);
}

#endif
