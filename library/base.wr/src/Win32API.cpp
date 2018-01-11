/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "Win32API.h"
#include "base/file_functions.h"
#include "ConvUtils.h"

using namespace System;
using namespace System::IO;
using namespace System::Windows::Forms;

using namespace MySQL::Utilities::SysUtils;

#pragma comment(lib, "User32.lib")

//--------------------------------------------------------------------------------------------------

// Clipboard operations for editable combo boxes.
void Win32Api::Undo(ComboBox ^ box) {
  // Clipboard commands are only supported by editable combo boxes.
  if (box->DropDownStyle == ComboBoxStyle::DropDownList)
    return;

  COMBOBOXINFO info;
  info.cbSize = sizeof(info);
  LRESULT res = SendMessage((HWND)box->Handle.ToPointer(), CB_GETCOMBOBOXINFO, 0, (LPARAM)&info);
  if (res != 0)
    SendMessage(info.hwndItem, WM_UNDO, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void Win32Api::Cut(ComboBox ^ box) {
  if (box->DropDownStyle == ComboBoxStyle::DropDownList)
    return;

  COMBOBOXINFO info;
  info.cbSize = sizeof(info);
  LRESULT res = SendMessage((HWND)box->Handle.ToPointer(), CB_GETCOMBOBOXINFO, 0, (LPARAM)&info);
  if (res != 0)
    SendMessage(info.hwndItem, WM_CUT, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void Win32Api::Copy(ComboBox ^ box) {
  if (box->DropDownStyle == ComboBoxStyle::DropDownList)
    return;

  COMBOBOXINFO info;
  info.cbSize = sizeof(info);
  LRESULT res = SendMessage((HWND)box->Handle.ToPointer(), CB_GETCOMBOBOXINFO, 0, (LPARAM)&info);
  if (res != 0)
    SendMessage(info.hwndItem, WM_COPY, 0, 0);
}

//--------------------------------------------------------------------------------------------------

void Win32Api::Paste(ComboBox ^ box) {
  if (box->DropDownStyle == ComboBoxStyle::DropDownList)
    return;

  COMBOBOXINFO info;
  info.cbSize = sizeof(info);
  LRESULT res = SendMessage((HWND)box->Handle.ToPointer(), CB_GETCOMBOBOXINFO, 0, (LPARAM)&info);
  if (res != 0)
    SendMessage(info.hwndItem, WM_PASTE, 0, 0);
}

//--------------------------------------------------------------------------------------------------

bool Win32Api::CanUndo(ComboBox ^ box) {
  if (box->DropDownStyle == ComboBoxStyle::DropDownList)
    return false;

  COMBOBOXINFO info;
  info.cbSize = sizeof(info);
  LRESULT res = SendMessage((HWND)box->Handle.ToPointer(), CB_GETCOMBOBOXINFO, 0, (LPARAM)&info);
  if (res != 0)
    return SendMessage(info.hwndItem, EM_CANUNDO, 0, 0) != 0;

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Unblocks the given file, by removing the alternate data stream "Zone.Identifier" from it,
 * which is added by the system when the file was downloaded from a remote computer.
 */
bool Win32Api::UnblockFile(String ^ file) {
  String ^ streamName = file + ":Zone.Identifier";
  std::string nativeName = NativeToCppString(streamName);
  return base_remove(nativeName) == 0;
}

//--------------------------------------------------------------------------------------------------

/// <summary>
/// Unblocks all our files that might have been blocked when the user downloaded them
/// as zip archive. Even though this touches 1500+ files it is still extremely fast, even
/// if no lock exists anymore. Testing for an existing lock takes probably more time than just
/// trying to remove it (even if it doesn't exist).
/// </summary>
bool Win32Api::UnblockWorkbenchFiles(String ^ folder) {
  bool result = true;
  for each(String ^ subFolder in Directory::GetDirectories(folder)) result &= UnblockWorkbenchFiles(subFolder);

  for each(String ^ file in Directory::GetFiles(folder)) result &= UnblockFile(file);

  return result;
}

//--------------------------------------------------------------------------------------------------

static HANDLE hStdOut = 0;
static int stdOutIndex = -1;
static FILE *stdOutFile = NULL, originalStdOutFile;

/**
 * Redirects the stdout handle to the parent process console, so we can start WB from
 * command line without opening a separate console window, but still get all cout output
 * (e.g. via printf) there.
 * Note: since this works with static vars it is essentially a one-timer. Don't call this
 *       function more than once!
 */
bool Win32Api::RedirectConsole() {
  AttachConsole(ATTACH_PARENT_PROCESS);

  stdOutIndex = _open_osfhandle((INT_PTR)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
  if (stdOutIndex < 0)
    return false;

  stdOutFile = _fdopen(stdOutIndex, "w");

  if (stdOutFile == NULL)
    return false;

  originalStdOutFile = *stdout;
  *stdout = *stdOutFile;

  std::cout.clear();
  return true;
}

//--------------------------------------------------------------------------------------------------

void Win32Api::ReleaseConsole() {
  FreeConsole();
  if (stdOutFile != NULL)
    fclose(stdOutFile);
  //_close(stdOutIndex); freed already at this point.
  *stdout = originalStdOutFile;
}