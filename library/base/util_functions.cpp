/*
 * Copyright (c) 2005, 2021, Oracle and/or its affiliates. All rights reserved.
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

#include <glib/gstdio.h>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <vector>

#include "base/log.h"
#include "base/common.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "workbench/wb_version.h"

// Windows includes
#ifdef _MSC_VER
#include <windows.h>
#include <direct.h>
#include <tchar.h>
#include <strsafe.h>
#else
// unix/linux includes
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/time.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/utsname.h> // uname()
#include <fcntl.h>

#define SIZE_T size_t

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <sys/sysctl.h>
#include <mach/machine.h>
#endif

#include "base/file_functions.h"
#include "base/util_functions.h"

DEFAULT_LOG_DOMAIN(DOMAIN_BASE)

struct hardware_info {
  std::string _cpu;
  std::string _clock;
  unsigned int _cpu_count;
  std::int64_t _memory_in_bytes;
};

//----------------------------------------------------------------------------------------------------------------------

char *auto_line_break(const char *txt, unsigned int width, char sep) {
  char *dst = (char *)g_malloc((width + 2) * 80);
  unsigned int i, o = 0, p = 0, w = 0, l = (unsigned int)strlen(txt);

  for (i = 0; i < l;) {
    w++;

    if (w > width) {
#if defined(_MSC_VER)
      dst[p + o] = '\r';
      dst[p + o + 1] = '\n';

      o += 1;
#else
      dst[p + o] = '\n';
#endif
      i = p + 1;
      w = 0;
    } else {
      dst[i + o] = txt[i];

      if (txt[i] == sep)
        p = i;

      i++;
    }
  }

  dst[i + o] = 0;

  return dst;
}

//----------------------------------------------------------------------------------------------------------------------

int str_is_numeric(const char *str) {
  unsigned int len = (unsigned int)strlen(str);
  unsigned int i;

  for (i = 0; i < len; i++)
    if (g_ascii_digit_value(str[i]) == -1)
      return 0;

  return 1;
}

//----------------------------------------------------------------------------------------------------------------------

char *str_toupper(char *str) {
  char *s = str;

  while (*s) {
    *s = (char)toupper(*s);
    s++;
  }
  return str;
}

//----------------------------------------------------------------------------------------------------------------------

#if defined(_MSC_VER)

#define BUFSIZE 256
#define VER_SUITE_WH_SERVER 0x00008000

int get_value_from_registry(HKEY root_key, const char *sub_key, const char *key, const char *def, char *value,
                            int target_size) {
  HKEY hSubKey;
  DWORD dwType;
  DWORD dwSize;
  LONG retval;
  unsigned char Buffer[512];

  // Explicitly link to ANSI version, we are using ANSI key names only here.
  if ((retval = RegOpenKeyExA(root_key, sub_key, 0, KEY_READ, &hSubKey)) == ERROR_SUCCESS) {
    dwSize = 512;
    if ((RegQueryValueExA(hSubKey, key, NULL, &dwType, Buffer, &dwSize)) == ERROR_SUCCESS) {
      if (dwType == REG_DWORD) {
        sprintf_s(value, target_size, "%d", Buffer[0] + Buffer[1] * 256 + Buffer[2] * 65536 + Buffer[3] * 16777216);
      } else {
        StringCchCopy((LPTSTR)value, BUFSIZE, (LPTSTR)Buffer);
      }
    } else {
      StringCchCopy((LPTSTR)value, BUFSIZE, (LPTSTR)def);
    }

    return 0;
  } else {
    StringCchCopy((LPTSTR)value, BUFSIZE, (LPTSTR) "");
    return retval;
  }
}

//----------------------------------------------------------------------------------------------------------------------

int set_value_to_registry(HKEY root_key, const char *sub_key, const char *key, const char *value) {
  HKEY hSubKey;
  LONG retval;
  DWORD dwDispo;

  if ((retval = RegCreateKeyExA(root_key, sub_key, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey,
                                &dwDispo)) == ERROR_SUCCESS) {
    retval = RegSetValueExA(hSubKey, key, 0, REG_SZ, (const BYTE *)value, (DWORD)strlen(value) + 1);

    if (retval != ERROR_SUCCESS) {
      return GetLastError();
    } else {
      return 0;
    }
  } else {
    return -1;
  }
}

//----------------------------------------------------------------------------------------------------------------------

typedef void(WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL(WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

std::string get_local_os_name() {
  std::string result;
  char buffer[BUFSIZE];

  std::string product_name;
  if (get_value_from_registry(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName", "",
                              buffer, BUFSIZE) == 0)
    product_name = buffer;

  std::string csd_version;
  if (get_value_from_registry(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "CSDVersion", "",
                              buffer, BUFSIZE) == 0)
    csd_version = buffer;

  if (!product_name.empty()) {
    result = (base::hasPrefix(product_name, "Microsoft") ? "" : "Microsoft ") + product_name;
    if (!csd_version.empty())
      result += " " + csd_version;
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

std::string get_local_hardware_info() {
  char *hardware_string;
  SYSTEM_INFO sysinfo;
  MEMORYSTATUSEX memstat;
  char processor_name[BUFSIZE];
  char processor_mhz[BUFSIZE];
  char total_phys_ram[BUFSIZE];
  DWORDLONG total_phys_ram_val;
  int target_size;

  GetSystemInfo(&sysinfo);

  memstat.dwLength = sizeof(memstat);
  GlobalMemoryStatusEx(&memstat);

  ZeroMemory(processor_mhz, sizeof(processor_mhz));
  ZeroMemory(processor_name, sizeof(processor_name));

  get_value_from_registry(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "~MHz", "",
                          processor_mhz, BUFSIZE);

  get_value_from_registry(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                          "ProcessorNameString", "", processor_name, BUFSIZE);

  if (!processor_name[0]) {
    get_value_from_registry(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "Identifier", "",
                            processor_name, BUFSIZE);
    sprintf_s(processor_name, BUFSIZE, "%s %s", processor_name, processor_mhz);
  }

  base::trim(processor_name);

  total_phys_ram_val = memstat.ullTotalPhys;
  if (total_phys_ram_val >= 1072410624 / 1.9) {
    sprintf_s(total_phys_ram, BUFSIZE, "%1.1f GiB RAM", (double)total_phys_ram_val / 1072410624.0);
  } else if (total_phys_ram_val >= 1047276 / 1.9) {
    sprintf_s(total_phys_ram, BUFSIZE, "%1.0f MiB RAM", (double)total_phys_ram_val / 1047276.0);
  } else {
    sprintf_s(total_phys_ram, BUFSIZE, "%lld B RAM", total_phys_ram_val);
  }

  target_size = 16 + (int)strlen(processor_name) + (int)strlen(total_phys_ram);
  hardware_string = (char *)g_malloc(target_size);

  if (sysinfo.dwNumberOfProcessors > 1) {
    sprintf_s(hardware_string, target_size, "%dx %s, %s", sysinfo.dwNumberOfProcessors, processor_name, total_phys_ram);
  } else {
    sprintf_s(hardware_string, target_size, "%s, %s", processor_name, total_phys_ram);
  }

  return hardware_string;
}

#else

#if defined(__APPLE__) && defined(__MACH__)

std::string get_local_os_name() {
  struct utsname info;

  if (uname(&info) < 0)
    return "unknown";

  char *dot_position = strstr(info.release, ".");
  if (dot_position == NULL)
    return "unknown";

  *dot_position = 0;
  int version = base::atoi<int>(info.release, 0);
  switch (version) {
    case 23:
      return std::string("macOS 14.x Sonoma ") + info.machine;
    case 22:
      return std::string("macOS 13.x Ventura ") + info.machine;
    case 21:
      return std::string("macOS 12.x Monterey ") + info.machine;
    case 20:
      return std::string("macOS 11.x Big Sur ") + info.machine;
    case 19:
      return std::string("macOS 10.15.x Catalina ") + info.machine;
  }
  return "unknown";
}

static const char *get_cpu_type_name(int cpu_type, int cpu_subtype) {
  switch (cpu_type) {
    case CPU_TYPE_I386:
      switch (cpu_subtype) {
        case CPU_SUBTYPE_386:
          return "80386";
        case CPU_SUBTYPE_486:
          return "80486";
        case CPU_SUBTYPE_486SX:
          return "80486SX";
        case CPU_SUBTYPE_PENT:
          return "Pentium";
        case CPU_SUBTYPE_PENTPRO:
          return "Pentium Pro";
        case CPU_SUBTYPE_PENTII_M3:
          return "Pentium II";
        case CPU_SUBTYPE_PENTII_M5:
          return "Pentium II";
#ifdef CPU_SUBTYPE_CELERON
        case CPU_SUBTYPE_CELERON:
          return "Celeron";
#endif
#ifdef CPU_SUBTYPE_CELERON_MOBILE
        case CPU_SUBTYPE_CELERON_MOBILE:
          return "Celeron Mobile";
#endif
#ifdef CPU_SUBTYPE_PENTIUM_3
        case CPU_SUBTYPE_PENTIUM_3:
          return "Pentium III";
#endif
#ifdef CPU_SUBTYPE_PENTIUM_3_M
        case CPU_SUBTYPE_PENTIUM_3_M:
          return "Pentium III M";
#endif
#ifdef CPU_SUBTYPE_PENTIUM_3_XEON
        case CPU_SUBTYPE_PENTIUM_3_XEON:
          return "Pentium III Xeon";
#endif
#ifdef CPU_SUBTYPE_PENTIUM_M
        case CPU_SUBTYPE_PENTIUM_M:
          return "Pentium M";
#endif
#ifdef CPU_SUBTYPE_PENTIUM_4
        case CPU_SUBTYPE_PENTIUM_4:
          return "Pentium 4";
#endif
#ifdef CPU_SUBTYPE_PENTIUM_4_M
        case CPU_SUBTYPE_PENTIUM_4_M:
          return "Pentium 4M";
#endif
#ifdef CPU_SUBTYPE_ITANIUM
        case CPU_SUBTYPE_ITANIUM:
          return "Itanium";
#endif
#ifdef CPU_SUBTYPE_ITANIUM_2
        case CPU_SUBTYPE_ITANIUM_2:
          return "Itanium 2";
#endif
#ifdef CPU_SUBTYPE_XEON
        case CPU_SUBTYPE_XEON:
          return "Xeon";
#endif
#ifdef CPU_SUBTYPE_XEON_MP
        case CPU_SUBTYPE_XEON_MP:
          return "Xeon MP";
#endif
        default:
          return "x86";
      }
      break;
    case CPU_TYPE_POWERPC:
      switch (cpu_subtype) {
        case CPU_SUBTYPE_POWERPC_601:
          return "PowerPC 601";
        case CPU_SUBTYPE_POWERPC_602:
          return "PowerPC 602";
        case CPU_SUBTYPE_POWERPC_603:
          return "PowerPC 603";
        case CPU_SUBTYPE_POWERPC_603e:
          return "PowerPC 603e";
        case CPU_SUBTYPE_POWERPC_603ev:
          return "PowerPC 603ev";
        case CPU_SUBTYPE_POWERPC_604:
          return "PowerPC 604";
        case CPU_SUBTYPE_POWERPC_604e:
          return "PowerPC 604e";
        case CPU_SUBTYPE_POWERPC_620:
          return "PowerPC 620";
        case CPU_SUBTYPE_POWERPC_750:
          return "PowerPC G3";
        case CPU_SUBTYPE_POWERPC_7400:
          return "PowerPC G4";
        case CPU_SUBTYPE_POWERPC_7450:
          return "PowerPC G4";
        case CPU_SUBTYPE_POWERPC_970:
          return "PowerPC G5";
        default:
          return "PowerPC";
      }
      break;
  }
  return "Unknown";
}

//----------------------------------------------------------------------------------------------------------------------

// macOS
static int _get_hardware_info(hardware_info &info) {
  int mib[2];
  size_t length;
  int tmp;
  char *mclass;
  int cpu_type, cpu_subtype;

  if (sysctlbyname("machdep.cpu.brand_string", NULL, &length, NULL, 0) != -1) {
    char *cpu = (char *)g_malloc(length + 1);
    sysctlbyname("machdep.cpu.brand_string", cpu, &length, NULL, 0);
    info._cpu = base::trim(cpu, " \n");
    g_free(cpu);
  } else {
    // get machine class
    mib[0] = CTL_HW;
    mib[1] = HW_MACHINE;
    sysctl(mib, 2, NULL, &length, NULL, 0);
    mclass = (char *)g_malloc(length * sizeof(char *));
    sysctl(mib, 2, mclass, &length, NULL, 0);

    // get machine arch
    length = sizeof(cpu_type);
    sysctlbyname("hw.cputype", &cpu_type, &length, NULL, 0);
    length = sizeof(cpu_subtype);
    sysctlbyname("hw.cpusubtype", &cpu_subtype, &length, NULL, 0);

    info._cpu = base::strfmt("%s (%s)", mclass, get_cpu_type_name(cpu_type, cpu_subtype));
    g_free(mclass);

    // get cpu clock
    mib[0] = CTL_HW;
    mib[1] = HW_CPU_FREQ;
    length = sizeof(tmp);
    if (sysctl(mib, 2, &tmp, &length, NULL, 0) < 0)
      info._clock = base::strfmt("?");
    else
      info._clock = base::strfmt("%.01f", (double)tmp / 1000000.0);
  }

  // get cpu count
  mib[0] = CTL_HW;
  mib[1] = HW_NCPU;
  length = sizeof(info._cpu_count);
  if (sysctl(mib, 2, &info._cpu_count, &length, NULL, 0) < 0)
    info._cpu_count = 1;

  // get memory size
  info._memory_in_bytes = get_physical_memory_size();

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

#else

// Linux/Unix version

std::string get_local_os_name() {
  auto is_debian_based = [](struct utsname &info) -> bool {
    return strstr(info.version, "Ubuntu") or strstr(info.version, "Debian");
  };

  // get distro name and version - Debian-based systems
  auto get_lsb_release_param = [](char param) -> std::string {
    char cmd[] = "lsb_release -_";
    cmd[sizeof(cmd) - 2] = param; // replace _ with param

    int rc;
    char *stdo;
    std::string result;
    GError *error;

    if (g_spawn_command_line_sync(cmd, &stdo, NULL, &rc, &error) && stdo) {
      char *d = strchr(stdo, ':');
      if (d)
        result = base::trim(g_strchug(d + 1));
      g_free(stdo);
      return result;
    } else {
      logError("Error executing lsb_release -%c: %s\n", param, error->message);
      return std::string("unknown");
    }

  };

  // get distro name and version - Red-Hat-based systems
  auto cat_redhat_release = []() -> std::string {
    std::ifstream is;
    try {
      is.open("/etc/redhat-release", std::ifstream::in);
      char buf[256];
      is.getline(buf, 256);
      return std::string(buf);
    } catch (const std::ios_base::failure &e) {
      logError("Error reading /etc/redhat-release: %s\n", e.what());
      return std::string("unknown");
    }
  };

  struct utsname info;

  if (uname(&info) < 0)
    return "unknown";
  else if (is_debian_based(info))
    return get_lsb_release_param('i') + ' ' + get_lsb_release_param('r') + ' ' + info.machine;
  else
    return cat_redhat_release() + ' ' + info.machine;
}

//----------------------------------------------------------------------------------------------------------------------

static int _get_hardware_info(hardware_info &info) {
  FILE *proc;
  char line[256];

  // fetch processor info from /proc/cpuinfo

  proc = fopen("/proc/cpuinfo", "r");
  if (!proc) {
    return -1;
  }

  info._cpu_count = 0;
  while (!feof(proc)) {
    if (!fgets(line, sizeof(line), proc))
      break;

    if (base::hasPrefix(line, "model name")) {
      info._cpu_count++;
      info._cpu = base::trim(base::split(line, ":")[1], " \n");
    } else if (base::hasPrefix(line, "cpu MHz")) {
      info._clock = base::trim(base::split(line, ":")[1], " \n");
    }
  }
  fclose(proc);

  info._memory_in_bytes = get_physical_memory_size();

  return 0;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
std::string get_local_hardware_info() {
  std::stringstream hardware_string;
  hardware_info info;

  _get_hardware_info(info);

  if (info._cpu_count > 1)
    hardware_string << info._cpu_count << "x ";

  hardware_string << info._cpu;

  if (!info._clock.empty())
    hardware_string << " (" << info._clock << "MHz)";

  hardware_string << " - " << base::sizefmt(info._memory_in_bytes, false) << " RAM";

  return hardware_string.str();
}

#endif

//----------------------------------------------------------------------------------------------------------------------

std::int64_t get_physical_memory_size() {
#if defined(_MSC_VER)
  MEMORYSTATUS memstat;

  GlobalMemoryStatus(&memstat);

  return memstat.dwTotalPhys;
#elif defined(__APPLE__)
  std::uint64_t mem64;
  int mib[2];
  int mem32;
  size_t length;
  mib[0] = CTL_HW;
  mib[1] = HW_MEMSIZE;
  length = sizeof(mem64);
  if (sysctl(mib, 2, &mem64, &length, NULL, 0) < 0) {
    mib[0] = CTL_HW;
    mib[1] = HW_PHYSMEM;
    length = sizeof(mem32);
    sysctl(mib, 2, &mem32, &length, NULL, 0);
    mem64 = mem32;
  }

  return mem64;
#else
  FILE *proc;
  std::int64_t mem64;
  mem64 = 0;
  // fetch physical memory info from /proc/meminfo
  proc = fopen("/proc/meminfo", "r");
  if (proc) {
    char line[1024];
    char *ptr, *end;

    while (fgets(line, sizeof(line), proc)) {
      if (strncasecmp(line, "MemTotal:", sizeof("MemTotal:") - 1) == 0) {
        char *line_end = line + strlen(line);
        ptr = strchr(line, ':') + 1;
        while (*ptr && *ptr == ' ')
          ptr++;
        end = strchr(ptr, ' ');
        if (end)
          *end = 0;
        if (end < line_end)
          end++;
        if (strstr(end, "gB") || strstr(end, "GB"))
          mem64 = strtoul(base::trim(ptr).c_str(), NULL, 10) * 1024 * 1024 * 1024LL;
        else if (strstr(end, "mB") || strstr(end, "MB"))
          mem64 = strtoul(base::trim(ptr).c_str(), NULL, 10) * 1024 * 1024LL;
        else if (strstr(end, "kB") || strstr(end, "KB"))
          mem64 = strtoul(base::trim(ptr).c_str(), NULL, 10) * 1024LL;
        else
          mem64 = strtoul(base::trim(ptr).c_str(), NULL, 10);
        break;
      }
    }
    fclose(proc);
  }
  return mem64;

#endif
}

//----------------------------------------------------------------------------------------------------------------------

std::int64_t get_file_size(const char *filename) {
#if _MSC_VER
  DWORD dwSizeLow;
  DWORD dwSizeHigh = 0;
  HANDLE hfile;
  std::wstring name = base::string_to_wstring(filename);

  hfile = CreateFile(name.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL, NULL);

  if (hfile != INVALID_HANDLE_VALUE) {
    dwSizeLow = GetFileSize(hfile, &dwSizeHigh);

    CloseHandle(hfile);

    if ((dwSizeLow == INVALID_FILE_SIZE) && (GetLastError()) != NO_ERROR) {
      return -1;
    } else {
      return (((std::int64_t)dwSizeHigh << 32) + dwSizeLow);
    }
  } else
    return -1;

#else  //! WINDOWS
  struct stat buf;
  char *local_filename;

  if (!(local_filename = g_filename_from_utf8(filename, -1, NULL, NULL, NULL)))
    return -1;

  if (stat(local_filename, &buf) < 0) {
    g_free(local_filename);
    return -1;
  }
  g_free(local_filename);
  return buf.st_size;
#endif //! WINDOWS
}

// note, needle has to be ascii!
char *strcasestr_len(const char *haystack, int haystack_len, const char *needle) {
  gssize needle_len = (gssize)strlen(needle);
  int i;

  if (needle_len > haystack_len)
    return NULL;

  i = 0;
  while (i <= haystack_len - needle_len) {
    if (g_ascii_strncasecmp(needle, haystack + i, needle_len) == 0)
      return (char *)haystack + i;
    i++;
  }
  return NULL;
}

//----------------------------------------------------------------------------------------------------------------------

const char *strfindword(const char *str, const char *word) {
  const char *result = NULL;
  const char *ptr;
  size_t wordlen = strlen(word);

  ptr = str;
  for (;;) {
    // find match
    ptr = strcasestr_len(ptr, (int)strlen(ptr), word);
    if (!ptr)
      break;

    // check if its acceptable
    if ((ptr == str || !isalnum(*(ptr - 1))) &&                   // space or any other non-alpha-numeric before
        (!isalnum(*(ptr + wordlen)) || *(ptr + wordlen) == '\0')) // space or any other non-alpha-numeric after
    {
      result = ptr;
      break;
    };
    ptr += wordlen;
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

// TODO: move to file_functions
/**
 * Copies all files non-recursively from source to target. Target will be created on the fly.
 */
int copy_folder(const char *source_folder, const char *target_folder) {
  const char *entry;
  GDir *dir;

  // Create target folder.
  if (!g_file_test(target_folder, G_FILE_TEST_IS_DIR))
    if (g_mkdir(target_folder, 0700) < 0)
      return 0;

  dir = g_dir_open(source_folder, 0, NULL);
  if (dir) {
    while ((entry = g_dir_read_name(dir)) != NULL) {
      char *source = g_build_filename(source_folder, entry, NULL);
      char *target = g_build_filename(target_folder, entry, NULL);
      if (!base::copyFile(source, target)) {
        logWarning("Could not copy file %s to %s: %s\n", source, target, g_strerror(errno));
        g_free(source);
        g_free(target);
        g_dir_close(dir);
        return 0;
      }
      g_free(source);
      g_free(target);
    }
    g_dir_close(dir);
  } else {
    logWarning("Could not open directory %s\n", source_folder);
    return 0;
  }
  return 1;
}

//--------------------------------------------------------------------------------------------------

namespace base {

  double timestamp() {
#if defined(_MSC_VER)
    return (double)GetTickCount() / 1000.0;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double seconds = tv.tv_sec;
    double fraction = tv.tv_usec / (double)(1000000);
    return seconds + fraction;
#endif
  }

  //--------------------------------------------------------------------------------------------------

  std::string fmttime(time_t t, const char *fmt) {
    char date[100];
#ifdef _MSC_VER
    errno_t err;
#else
    int err;
#endif
    struct tm newtime;

    if (t == 0)
      time(&t);

#ifdef _MSC_VER
    err = localtime_s(&newtime, &t);
#else
    localtime_r(&t, &newtime);
    err = 0;
#endif

    if (!err)
      strftime(date, sizeof(date), fmt, &newtime);
    else
      date[0] = 0;

    return date;
  }


  BASELIBRARY_PUBLIC_FUNC std::string getVersion(void) {
    return strfmt("%u.%u.%u", APP_MAJOR_NUMBER, APP_MINOR_NUMBER, APP_RELEASE_NUMBER);
  }


} // namespace base

//--------------------------------------------------------------------------------------------------

