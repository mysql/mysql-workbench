/* 
 * Copyright (c) 2005, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <glib/gstdio.h>
#include <cstdio>
#include <pcre.h>
#include <sstream>

#include "base/common.h"
#include "base/string_utilities.h"

// Windows includes
#ifdef _WIN32
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

// MacOS X
#if defined(__APPLE__) && defined(__MACH__)
  #include <sys/sysctl.h>
  #include <mach/machine.h>
#endif

#include "base/file_functions.h"
#include "base/util_functions.h"


struct hardware_info
{
  std::string _cpu;
  std::string _clock;
  unsigned int _cpu_count;
  boost::int64_t _memory_in_bytes;
};

//----------------------------------------------------------------------------------------------------------------------

static void __sappend(char **str, int *ressize, int *reslen, const char *sbegin, int count)
{
  if (*reslen + count + 1 >= *ressize)
  {
    *ressize+= count + 100;
    *str= (char*) g_realloc(*str, *ressize);
  }
#ifdef _WIN32
  strncpy_s(*str + *reslen, *reslen, sbegin, count);
#else
  strncpy(*str + *reslen, sbegin, count);
#endif

  *reslen += count;
  (*str)[*reslen]= 0;
}

//----------------------------------------------------------------------------------------------------------------------

char *str_g_subst(const char *str, const char *search, const char *replace)
{
  int ressize, reslen;
  int replsize= (int)strlen(replace);
  int searchlen= (int)strlen(search);
  char *res;
  const char *bptr, *ptr;

  g_return_val_if_fail(str != NULL, g_strdup(str));
  if(str[0] == '\0')
  {
    return g_strdup(str);
  }
  g_return_val_if_fail(search != NULL && *search, g_strdup(str));
  g_return_val_if_fail(replace != NULL, g_strdup(str));

  ressize= (int)strlen(str)+1;
  reslen= 0;
  res= (char*) g_malloc(ressize);

  bptr= str;

  while ((ptr= strstr(bptr, search)) != NULL)
  {
    __sappend(&res, &ressize, &reslen, bptr, (int)(ptr-bptr));
    bptr= ptr+searchlen;
    __sappend(&res, &ressize, &reslen, replace, replsize);
  }
  __sappend(&res, &ressize, &reslen, bptr, (int)strlen(bptr));

  return res;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Replaces the search string with the replace string
 * in the given str
 *
 * @str becomes invalid after calling this
 * function!
 *
 * Return value: Newly allocated result string
 **/
char *str_g_replace(char *str, const char *search, const char *replace)
{
  char *res;

  if (!replace)
    replace= "";

  res= str_g_subst(str, search, replace);
  g_free(str);
  return res;
}

//----------------------------------------------------------------------------------------------------------------------

char *auto_line_break(const char *txt, unsigned int width, char sep)
{
  char *dst= (char*) g_malloc((width + 2) * 80);
  unsigned int i, o= 0, p= 0, w= 0, l= (unsigned int)strlen(txt);

  for (i= 0; i < l;)
  {
    w++;

    if (w > width)
    {
#if defined(__WIN__) || defined(_WIN32) || defined(_WIN64)
      dst[p + o]= '\r';
      dst[p + o + 1]= '\n';

      o+= 1;
#else
      dst[p + o]= '\n';
#endif
      i= p + 1;
      w= 0;
    }
    else
    {
      dst[i + o]= txt[i];

      if (txt[i] == sep)
        p= i;

        i++;
    }
  }

  dst[i + o]= 0;

  return dst;
}

//----------------------------------------------------------------------------------------------------------------------

int str_is_numeric(const char *str)
{
  unsigned int len= (unsigned int)strlen(str);
  unsigned int i;

  for (i= 0; i < len; i++)
    if(g_ascii_digit_value(str[i]) == -1)
      return 0;

  return 1;
}

//----------------------------------------------------------------------------------------------------------------------

char *str_toupper(char *str)
{
  char *s= str;

  while (*s)
  {
    *s= toupper(*s);
    s++;
  }
  return str;
}

//----------------------------------------------------------------------------------------------------------------------

#if defined(__WIN__) || defined(_WIN32) || defined(_WIN64)

#define BUFSIZE 256
#define VER_SUITE_WH_SERVER 0x00008000

int get_value_from_registry(HKEY root_key, const char *sub_key, const char *key, const char *def,
                            char *value, int target_size)
{
  HKEY hSubKey;
  DWORD dwType;
  DWORD dwSize;
  LONG retval;
  unsigned char Buffer[512];

  // Explicitly link to ANSI version, we are using ANSI key names only here.
  if ((retval = RegOpenKeyExA(root_key, sub_key, 0, KEY_READ, &hSubKey)) == ERROR_SUCCESS)
  {
    dwSize = 512;
    if ((RegQueryValueExA(hSubKey, key, NULL, &dwType, Buffer, &dwSize)) == ERROR_SUCCESS)
    {
      if(dwType==REG_DWORD)
      {
        sprintf_s(value, target_size, "%d", Buffer[0] + Buffer[1] * 256 + Buffer[2] * 65536 + Buffer[3] * 16777216);
      }
      else
      {
        StringCchCopy((LPTSTR)value, BUFSIZE, (LPTSTR)Buffer);
      }
    }
    else
    {
      StringCchCopy((LPTSTR)value, BUFSIZE, (LPTSTR)def);
    }

    return 0;
  }
  else
  {
    StringCchCopy((LPTSTR)value, BUFSIZE, (LPTSTR)"");
    return retval;
  }
}

//----------------------------------------------------------------------------------------------------------------------

int set_value_to_registry(HKEY root_key, const char *sub_key, const char *key, const char *value)
{
  HKEY hSubKey;
  LONG retval;
  DWORD dwDispo;

  if ((retval= RegCreateKeyExA(root_key, sub_key, 0, "",
    REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hSubKey, &dwDispo))==ERROR_SUCCESS)
  {
    retval = RegSetValueExA(hSubKey, key, 0, REG_SZ, (const BYTE*)value, (DWORD)strlen(value)+1);

    if(retval != ERROR_SUCCESS)
    {
       return GetLastError();
    } 
    else 
    {
      return 0;
    }
  }
  else
  {
    return -1;
  }
}

//----------------------------------------------------------------------------------------------------------------------
                            
typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

std::string get_local_os_name()
{
  std::string result;
  char buffer[BUFSIZE];

  std::string product_name;
  if (get_value_from_registry(HKEY_LOCAL_MACHINE,
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName", "", buffer, BUFSIZE) == 0)
    product_name = buffer;

  std::string csd_version;
  if (get_value_from_registry(HKEY_LOCAL_MACHINE,
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "CSDVersion", "", buffer, BUFSIZE) == 0)
    csd_version = buffer;

  if (!product_name.empty())
  {
    result = (base::starts_with(product_name, "Microsoft") ? "" : "Microsoft ") + product_name;
    if (!csd_version.empty())
      result += " " + csd_version;
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

std::string get_local_hardware_info()
{
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

  get_value_from_registry(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString", "",
    processor_name, BUFSIZE);

  if(!processor_name[0])
  {
    get_value_from_registry(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "Identifier", "",
      processor_name, BUFSIZE);
    sprintf_s(processor_name, BUFSIZE, "%s %s", processor_name, processor_mhz);
  }

  base::trim(processor_name);

  total_phys_ram_val= memstat.ullTotalPhys;
  if(total_phys_ram_val >= 1072410624 / 1.9)
  {
    sprintf_s(total_phys_ram, BUFSIZE, "%1.1f GiB RAM", (double)total_phys_ram_val / 1072410624.0);
  }
  else if(total_phys_ram_val >= 1047276 / 1.9)
  {
    sprintf_s(total_phys_ram, BUFSIZE, "%1.0f MiB RAM", (double)total_phys_ram_val / 1047276.0);
  }
  else
  {
    sprintf_s(total_phys_ram, BUFSIZE, "%d B RAM", total_phys_ram_val);
  }
   
  target_size= 16 + (int)strlen(processor_name) + (int)strlen(total_phys_ram);
  hardware_string = (char*) g_malloc(target_size);

  if(sysinfo.dwNumberOfProcessors>1) 
  {
    sprintf_s(hardware_string, target_size, "%dx %s, %s", sysinfo.dwNumberOfProcessors, processor_name, total_phys_ram);
  }
  else
  {
    sprintf_s(hardware_string, target_size, "%s, %s", processor_name, total_phys_ram);
  }

  return hardware_string;

}

#else /* !__WIN__ */

#if defined(__APPLE__) && defined(__MACH__)

std::string get_local_os_name()
{
  struct utsname info;

  if (uname(&info) < 0)
    return "unknown";

  char * dot_position = strstr(info.release, ".");
  if (dot_position == NULL)
    return "unknown";

  *dot_position = 0;
  int version = atoi(info.release);
  switch (version) {
    case 13:
      return "OS X 10.9.x Mavericks";
    case 12:
      return "OS X 10.8.x Mountain Lion";
    case 11:
      return "OS X 10.7.x Lion";
    case 10:
      return "OS X 10.6.x Snow Leopard";
    case 9:
      return "OS X 10.5.x Leopard"; // For completeness. Not that WB would actually run on this or lower :-)
    case 8:
      return "OS X 10.4.x Tiger";
    case 7:
      return "OS X 10.3.x Panther";
    case 6:
      return "OS X 10.2.x Jaguar";
    case 5:
      return "OS X 10.1.x Puma";
  }
  return "unknown";
}


static const char *get_cpu_type_name(int cpu_type, int cpu_subtype)
{
  switch (cpu_type)
  {
    case CPU_TYPE_I386:
      switch (cpu_subtype)
      {
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
      switch (cpu_subtype)
      {
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

// MacOS X
static int _get_hardware_info(hardware_info &info)
{
  int mib[2];
  size_t length;
  int tmp;
  char *mclass;
  int cpu_type, cpu_subtype;
  
  if (sysctlbyname("machdep.cpu.brand_string", NULL, &length, NULL, 0) != -1)
  {
    char *cpu = (char*)g_malloc(length+1);
    sysctlbyname("machdep.cpu.brand_string", cpu, &length, NULL, 0);
    info._cpu = base::trim(cpu, " \n");
    g_free(cpu);
  }
  else
  {
    // get machine class
    mib[0]= CTL_HW;
    mib[1]= HW_MACHINE;
    sysctl(mib, 2, NULL, &length, NULL, 0);
    mclass= (char*)g_malloc(length*sizeof(char*));
    sysctl(mib, 2, mclass, &length, NULL, 0);
    
    // get machine arch
    length= sizeof(cpu_type);
    sysctlbyname("hw.cputype", &cpu_type, &length, NULL, 0);
    length= sizeof(cpu_subtype);
    sysctlbyname("hw.cpusubtype", &cpu_subtype, &length, NULL, 0);
    
    info._cpu = base::strfmt("%s (%s)", mclass, get_cpu_type_name(cpu_type, cpu_subtype));
    g_free(mclass);

    // get cpu clock
    mib[0]= CTL_HW;
    mib[1]= HW_CPU_FREQ;
    length= sizeof(tmp);
    if (sysctl(mib, 2, &tmp, &length, NULL, 0) < 0)
      info._clock = base::strfmt("?");  
    else
      info._clock = base::strfmt("%.01f", (double)tmp/1000000.0);
  }
  
  // get cpu count
  mib[0]= CTL_HW;
  mib[1]= HW_NCPU;
  length= sizeof(info._cpu_count);
  if (sysctl(mib, 2, &info._cpu_count, &length, NULL, 0) < 0)
    info._cpu_count = 1;

  // get memory size
  info._memory_in_bytes = get_physical_memory_size();

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

#else

// Linux/Unix version

std::string get_local_os_name()
{
  struct utsname info;

  if (uname(&info) < 0)
    return "unknown";

  return base::strfmt("%s %s", info.sysname, info.release);
}

//----------------------------------------------------------------------------------------------------------------------

static int _get_hardware_info(hardware_info &info)
{
  FILE *proc;
  char line[256];

  // fetch processor info from /proc/cpuinfo
  
  proc= fopen("/proc/cpuinfo", "r");
  if (!proc) 
  {
    return -1;
  }

  info._cpu_count = 0;
  while (!feof(proc)) 
  {
    if (!fgets(line, sizeof(line), proc))
      break;
    
    if (base::starts_with(line,"model name")) 
    {
      info._cpu_count++;
      info._cpu = base::trim(base::split(line, ":")[1], " \n");
    } 
    else if (base::starts_with(line,"cpu MHz"))
    {
      info._clock = base::trim( base::split(line, ":")[1], " \n");
    }
  }
  fclose(proc);
  
  info._memory_in_bytes = get_physical_memory_size();

  return 0;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
std::string get_local_hardware_info()
{
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

boost::int64_t get_physical_memory_size()
{
#if defined(__WIN__) || defined(_WIN32) || defined(_WIN64)
  MEMORYSTATUS memstat;
  
  GlobalMemoryStatus(&memstat);
  
  return memstat.dwTotalPhys;
#elif defined(__APPLE__)
  boost::uint64_t mem64;
  int mib[2];
  int mem32;
  size_t length;
  mib[0]= CTL_HW;
  mib[1]= HW_MEMSIZE;
  length= sizeof(mem64);
  if (sysctl(mib, 2, &mem64, &length, NULL, 0) < 0)
  {
    mib[0]= CTL_HW;
    mib[1]= HW_PHYSMEM;
    length= sizeof(mem32);
    sysctl(mib, 2, &mem32, &length, NULL, 0);
    mem64= mem32;
  }
  
  return mem64;
#else
  FILE *proc;
  boost::int64_t mem64;
  mem64= 0;
  // fetch physical memory info from /proc/meminfo
  proc= fopen("/proc/meminfo", "r");
  if (proc)
  {
    char line[1024];
    char *ptr, *end;

    while (fgets(line, sizeof(line), proc))
    {
      if (strncasecmp(line, "MemTotal:", sizeof("MemTotal:")-1)==0)
      {
        char *line_end= line+strlen(line);
        ptr= strchr(line, ':')+1;
        while (*ptr && *ptr==' ') ptr++;
        end= strchr(ptr, ' ');
        if (end)
          *end= 0;
        if (end < line_end)
          end++;
        if (strstr(end, "gB") || strstr(end, "GB"))
          mem64= strtoul(base::trim(ptr).c_str(), NULL, 10)*1024*1024*1024LL;
        else if (strstr(end, "mB") || strstr(end, "MB"))
          mem64= strtoul(base::trim(ptr).c_str(), NULL, 10)*1024*1024LL;
        else if (strstr(end, "kB") || strstr(end, "KB"))
          mem64= strtoul(base::trim(ptr).c_str(), NULL, 10)*1024LL;
        else
          mem64= strtoul(base::trim(ptr).c_str(), NULL, 10);
        break;
      }
    }
    fclose(proc);
  }
  else
  {
    g_warning("Memory stats retrieval not implemented for this system");
  }
  return mem64;

#endif
}

//----------------------------------------------------------------------------------------------------------------------

boost::int64_t get_file_size(const char *filename)
{
#if _WIN32
  DWORD dwSizeLow;
  DWORD dwSizeHigh = 0;
  HANDLE hfile;
  std::wstring name = base::string_to_wstring(filename);

  hfile = CreateFile(name.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hfile != INVALID_HANDLE_VALUE) 
  {
    dwSizeLow= GetFileSize(hfile, &dwSizeHigh);

    CloseHandle(hfile);

    if((dwSizeLow==INVALID_FILE_SIZE)&&(GetLastError()) != NO_ERROR )
    { 
      return -1;
    }
    else
    {
      return (((boost::int64_t) dwSizeHigh << 32) + dwSizeLow);
    }
  }
  else
    return -1;

#else //!WINDOWS
  struct stat buf;
  char *local_filename;

  if (! (local_filename= g_filename_from_utf8(filename,-1,NULL,NULL,NULL)))
    return -1;

  if (stat(local_filename, &buf) < 0) {
    g_free(local_filename);
    return -1;
  }
  g_free(local_filename);
  return buf.st_size;
#endif //!WINDOWS
}

// note, needle has to be ascii!
char *strcasestr_len(const char *haystack, int haystack_len, const char *needle)
{
  gssize needle_len= (gssize)strlen(needle);
  int i;

  if (needle_len > haystack_len)
    return NULL;

  i= 0;
  while (i <= haystack_len - needle_len)
  {
    if (g_ascii_strncasecmp(needle, haystack+i, needle_len)==0)
      return (char *)haystack+i;
    i++;
  }
  return NULL;
}

//----------------------------------------------------------------------------------------------------------------------

#define O_VECTOR_COUNT 64 // max # of ()*2+2

char * get_value_from_text_ex_opt(const char *txt, int txt_length,
                                  const char *regexpr,
                                  unsigned int substring_nr,
                                  int options_for_exec)
{
  pcre *pcre_exp;
  const char *error_str;
  int erroffset;
  int o_vector[O_VECTOR_COUNT];
  int rc;
  const char *ret_val;
  char *value= NULL;

  if(txt && *txt)
  {
    pcre_exp= pcre_compile(regexpr, PCRE_CASELESS, &error_str, &erroffset, NULL);
    if (pcre_exp)
    {
      if ((rc= pcre_exec(pcre_exp, NULL, txt, txt_length, 0, 
                          options_for_exec, o_vector, O_VECTOR_COUNT) ) > 0)
      {
        if (o_vector[substring_nr * 2] != -1)
        {
          pcre_get_substring(txt, o_vector, rc, substring_nr, &ret_val);

          value= g_strdup(ret_val);

          pcre_free_substring((char*)ret_val);
        }
      }

      pcre_free(pcre_exp);
    }
  }

  return value;
}

//----------------------------------------------------------------------------------------------------------------------

char * get_value_from_text_ex(const char *txt, int txt_length,
                              const char *regexpr, unsigned int substring_nr)
{
  return get_value_from_text_ex_opt(txt,txt_length,regexpr,substring_nr,0);
}

//----------------------------------------------------------------------------------------------------------------------

const char *strfindword(const char *str, const char *word)
{
  const char* result = NULL;
  const char *ptr;
  size_t wordlen= strlen(word);

  ptr= str;
  for (;;)
  {
    // find match
    ptr= strcasestr_len(ptr, (int)strlen(ptr), word);
    if (!ptr)
      break;

    // check if its acceptable
    if ((ptr == str || !isalnum(*(ptr - 1))) && // space or any other non-alpha-numeric before
      (!isalnum(*(ptr + wordlen)) || *(ptr + wordlen) == '\0')) // space or any other non-alpha-numeric after
    {
      result= ptr;
      break;
    };
    ptr+= wordlen;
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

// TODO: move to file_functions
/**
 * Copies a file whose name is given by source to target. File names are expected to be encoded
 * as UTF-8.
 *
 * @param source The name + path of the file to copy.
 * @param target The name + path where the new file should be copied to.
 * @return 1 if the operation was successfull, otherwise 0.
 */
int copy_file(const char* source, const char* target)
{
#ifdef _WIN32
  {
    const int cch_buf= MAX_PATH;
    WCHAR src_path[MAX_PATH];
    WCHAR dest_path[MAX_PATH];

    MultiByteToWideChar(CP_UTF8, 0, source, -1, src_path, cch_buf);
    MultiByteToWideChar(CP_UTF8, 0, target, -1, dest_path, cch_buf);

    if (!CopyFileW(src_path, dest_path, 0))
      return 0;
    return 1;
  }
#else
  // copy contents of original archive to new
  char buffer[1024*4];
  size_t c;
  FILE *in, *out;
  in= base_fopen(source, "r");
  if (!in) 
    return 0;

  out= base_fopen(target, "w+");
  if (!out) 
  { 
    fclose(in); 
    return 0;
  }

  while ((c= fread(buffer, 1, sizeof(buffer), in)) > 0 && c != (size_t)-1)
  {
    if (fwrite(buffer, 1, c, out) < c)
    {
      int e= errno;
      fclose(in);
      fclose(out);
      errno= e;
      return 0;
    }
  }
  fclose(in);
  fclose(out);

  return 1;
#endif
}

//----------------------------------------------------------------------------------------------------------------------

// TODO: move to file_functions
/**
 * Copies all files non-recursively from source to target. Target will be created on the fly.
 */
int copy_folder(const char *source_folder, const char *target_folder)
{
  const char *entry;
  GDir *dir;

  // Create target folder.
  if (!g_file_test(target_folder, G_FILE_TEST_IS_DIR))
    if (g_mkdir(target_folder, 0700) < 0)
      return 0;
  
  dir= g_dir_open(source_folder, 0, NULL);
  if (dir)
  {
    while ((entry= g_dir_read_name(dir)) != NULL)
    {
      char* source= g_build_filename(source_folder, entry, NULL);
      char* target= g_build_filename(target_folder, entry, NULL);
      if (!copy_file(source, target))
      {
        g_warning("Could not copy file %s to %s: %s", source, target, 
                  g_strerror(errno));
        g_free(source);
        g_free(target);
        g_dir_close(dir);
        return 0;
      }
      g_free(source);
      g_free(target);
    }
    g_dir_close(dir);
  }
  else
  {
    g_warning("Could not open directory %s", source_folder);
    return 0;
  }
  return 1;
}

//--------------------------------------------------------------------------------------------------

double timestamp()
{
#if defined(__WIN__) || defined(_WIN32) || defined(_WIN64)
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
