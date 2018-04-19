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

#ifdef _MSC_VER

#include <windows.h>
#include <psapi.h>
#include "base/mem_stat.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN("base")

using namespace base;
MEMORYSTATUSEX memInfo;
PROCESS_MEMORY_COUNTERS_EX pmc;

//--------------------------------------------------------------------------------------------------

void MemUsage::StartCounting() {
  memset(&memInfo, 0, sizeof memInfo);
  memset(&pmc, 0, sizeof pmc);
  memInfo.dwLength = sizeof MEMORYSTATUSEX;
  GlobalMemoryStatusEx(&memInfo);
  GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS)&pmc, sizeof(pmc));
}

//--------------------------------------------------------------------------------------------------

void MemUsage::PrintUsage() {
  logDebug("=========================== Memory usage before tests ===========================\n");
  DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
  logDebug("Total Physical Memory(RAM): %s Bytes, %s MB\n", std::to_string(totalPhysMem).c_str(),
           std::to_string(totalPhysMem / (1024 * 1024)).c_str());
  DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
  logDebug("Physical Memory currently used : %s Bytes, %s MB\n", std::to_string(physMemUsed).c_str(),
           std::to_string(physMemUsed / (1024 * 1024)).c_str());
  SIZE_T physMemUsedByMe = pmc.WorkingSetSize;
  logDebug("Physical Memory currently used by current process : %s Bytes, %s MB\n",
           std::to_string(physMemUsedByMe).c_str(), std::to_string(physMemUsedByMe / (1024 * 1024)).c_str());

  memset(&memInfo, 0, sizeof memInfo);
  memset(&pmc, 0, sizeof pmc);
  memInfo.dwLength = sizeof MEMORYSTATUSEX;
  GlobalMemoryStatusEx(&memInfo);
  logDebug("=========================== Memory usage after tests ===========================\n");
  totalPhysMem = memInfo.ullTotalPhys;
  // logDebug("Total Physical Memory(RAM): %s Bytes, %s MB\n", base::to_string(totalPhysMem).c_str(),
  // base::to_string(totalPhysMem / (1024 * 1024)).c_str());
  physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
  logDebug("Physical Memory currently used : %s Bytes, %s MB\n", std::to_string(physMemUsed).c_str(),
           std::to_string(physMemUsed / (1024 * 1024)).c_str());
  GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS)&pmc, sizeof(pmc));
  SIZE_T physMemUsedByMeAfter = pmc.WorkingSetSize - physMemUsedByMe;
  physMemUsedByMe = pmc.WorkingSetSize;
  logDebug("Physical Memory currently used by current process : %s Bytes, %s MB\n",
           std::to_string(physMemUsedByMe).c_str(), std::to_string(physMemUsedByMe / (1024 * 1024)).c_str());
  logDebug("Difference : %s Bytes, %s MB\n", std::to_string(physMemUsedByMeAfter).c_str(),
           std::to_string(physMemUsedByMeAfter / (1024 * 1024)).c_str());
}

//--------------------------------------------------------------------------------------------------

#endif
