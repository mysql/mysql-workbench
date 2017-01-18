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

#include <windows.h>
#include <psapi.h>
#include "base/mem_stat.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN("base")

#ifdef _WIN32

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
