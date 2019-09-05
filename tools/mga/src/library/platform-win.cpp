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

#include <psapi.h>

#include "platform.h"
#include "global.h"
#include "path.h"
#include "process.h"
#include "utilities.h"
#include "accessible.h"

#define COPYFILE_EXCL 1
#define COPYFILE_FICLONE 2
#define COPYFILE_FICLONE_FORCE 4

using namespace mga;
using namespace aal;

class WinStat : public Stat {
  friend Stat;
protected:
  virtual void initialize(const std::string &path, bool followSymlinks) override;
  virtual timespec getTimeSpec(TimeSpecs spec) const override;
};

//----------------------------------------------------------------------------------------------------------------------

class WinPlatform : public Platform {
public:
  virtual int launchApplication(std::string const& name, std::vector<std::string> const& params,
    bool newInstance, ShowState showState, std::map<std::string, std::string> const& env = {}) const override;

  virtual bool isRunning(int processID) const override;
  virtual int getPidByName(std::string const& name) const override;
  virtual std::string getTempDirName() const override;
  virtual bool terminate(int processID, bool force = false) const override;
  virtual void initialize(int argc, const char* argv[], char *envp[]) const override;
  virtual void exit(ExitCode code) const override;
  virtual void writeText(std::string const& text, bool error) const override;
  virtual void createFolder(std::string const& name) const override;
  virtual void removeFolder(std::string const& name) const override;
  virtual void removeFile(std::string const& name) const override;
  
  virtual geometry::Size WinPlatform::getImageResolution(std::string const& path) const override;
  virtual void defineOsConstants(ScriptingContext &context, JSObject &constants) const override;
  virtual void defineFsConstants(ScriptingContext &context, JSObject &constants) const override;
  virtual std::vector<Cpu> cpuInfo() const override;
  virtual size_t getFreeMem() const override;
  virtual size_t getTotalMem() const override;
  virtual std::string getHomeDir() const override;
  virtual std::string getHostName() const override;
  virtual std::map<std::string, std::vector<NetworkInterface>> networkInterfaces() const override;
  virtual Version getVersion() const override;
  virtual std::string getRelease() const override;
  virtual double getUptime() const override;
  virtual void loadAvg(double (&avg)[3]) const override;
  virtual UserInfo getCurrentUserInfo() const override;
  virtual UiToolkit getUiToolkit() const override;
  virtual std::string getClipboardText() const override;
  virtual void setClipboardText(const std::string &text) const override;
  virtual std::vector<Screen> getScreens() const override;
};

//----------------------------------------------------------------------------------------------------------------------

std::unique_ptr<Stat> Stat::get(const std::string &path, bool followSymlinks) {
  auto result = std::make_unique<WinStat>();
  result->initialize(path, followSymlinks);
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void WinStat::initialize(const std::string &path, bool followSymlinks) {
  std::ignore = followSymlinks;

  // For now we don't support symlinks on Windows. 
  if (_wstat(Utilities::s2ws(path).c_str(), &_buffer) != 0)
    throw std::runtime_error("Cannot create stats: " + Utilities::getLastError());
}

//----------------------------------------------------------------------------------------------------------------------

timespec WinStat::getTimeSpec(TimeSpecs spec) const {
  timespec result = { 0, 0 };
  switch(spec) {
    case TimeSpecs::atime:
      result.tv_sec = _buffer.st_atime;
      break;
    case TimeSpecs::mtime:
      result.tv_sec = _buffer.st_mtime;
      break;
    case TimeSpecs::ctime:
    case TimeSpecs::birthdate:
      result.tv_sec = _buffer.st_ctime;
      break;
    default:
      throw std::runtime_error("Invalid time specification");
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------


Platform& Platform::get() {
  static WinPlatform singleton;
  return singleton;
}

//----------------------------------------------------------------------------------------------------------------------

static int runNewInstance(std::wstring const& wexe, std::wstring const& wparam, ShowState showState) {
  SHELLEXECUTEINFO shellExeInfo;
  memset(&shellExeInfo, 0, sizeof(shellExeInfo));
  shellExeInfo.cbSize = sizeof(shellExeInfo);
  shellExeInfo.lpVerb = L"";
  shellExeInfo.lpFile = wexe.c_str();
  shellExeInfo.lpParameters = wparam.c_str();
  if (showState == ShowState::Hidden)
    shellExeInfo.nShow = SW_HIDE;
  else if (showState == ShowState::Maximized)
    shellExeInfo.nShow = SW_SHOWMAXIMIZED;
  else
    shellExeInfo.nShow = SW_NORMAL;
  shellExeInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  SetLastError(ERROR_SUCCESS);

  if (ShellExecuteEx(&shellExeInfo) == FALSE) {
    LPVOID msgBuf = NULL;
    DWORD lastErr = GetLastError();
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, lastErr,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msgBuf, 0, nullptr);
    std::wstring msg = (LPCTSTR)msgBuf;
    LocalFree(msgBuf);
    SetLastError(ERROR_SUCCESS);
    throw std::runtime_error(Utilities::ws2s(msg));
  }

  return GetProcessId(shellExeInfo.hProcess);
}

//----------------------------------------------------------------------------------------------------------------------

int WinPlatform::launchApplication(std::string const& name, std::vector<std::string> const& params,
                                   bool newInstance, ShowState showState,
                                   std::map<std::string, std::string> const& /*env*/) const {
  std::stringstream ss;
  std::copy(params.begin(), params.end(), std::ostream_iterator<std::string>(ss, " "));

  std::string path = Path::isAbsolute(name) ? "" : Process::cwd();
  if (path.empty()) {
    path = name;
  } else {
    path = path + "/" + name;
    path = Path::normalize(path);
  }

  std::wstring wexe = Utilities::s2ws(path);
  std::wstring wparam = Utilities::s2ws(ss.str());

  int processId = 0;
  if (!newInstance)
    processId = Accessible::getRunningProcess(wexe);
  if (processId == 0)
    processId = runNewInstance(wexe, wparam, showState);
  return processId;
}

//----------------------------------------------------------------------------------------------------------------------

bool WinPlatform::isRunning(int processID) const {
  HANDLE runningProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, false, processID);
  bool isRunning = runningProcessHandle != NULL;
  if (isRunning) {
    CloseHandle(runningProcessHandle);
  }
  return isRunning;
}

//----------------------------------------------------------------------------------------------------------------------

int WinPlatform::getPidByName(std::string const& name) const {
  auto processList = Accessible::getRunningProcessByName(Utilities::s2ws(name));
  size_t size = processList.size();
  if (size > 0) {
    if (size > 1) {
      std::cout << "\nFound " << size << " processes with name: " << name << std::endl;
    }
    return processList.at(0);
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

std::string WinPlatform::getTempDirName() const {
  TCHAR tempPathBuffer[MAX_PATH] = { 0 };

  // Gets the temp path env string (no guarantee it's a valid path).
  DWORD retVal = GetTempPath(MAX_PATH, tempPathBuffer);
  if (retVal > MAX_PATH || (retVal == 0)) {
    throw std::runtime_error("GetTempPath failed");
  }
  return Utilities::ws2s(tempPathBuffer);
}

//----------------------------------------------------------------------------------------------------------------------

bool WinPlatform::terminate(int processID, bool /*force*/) const {
  HANDLE runningProcessHandle = OpenProcess(PROCESS_TERMINATE, false, processID);
  bool result = TerminateProcess(runningProcessHandle, 1) == TRUE;
  CloseHandle(runningProcessHandle);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void WinPlatform::initialize(int argc, const char* argv[], char *envp[]) const {
  std::ignore = argc;
  std::ignore = argv;
  std::ignore = envp;

  // In order to properly display UTF-8 output, we have to explicitly set the console to that encoding.
  // Using a different encoding (e.g. UTF-16) doesn't work with certain IDEs like Visual Studio Code.
  SetConsoleOutputCP(CP_UTF8);

  WORD requestedVersion = MAKEWORD(2, 2);
  WSADATA data;
  WSAStartup(requestedVersion, &data);
}

//----------------------------------------------------------------------------------------------------------------------

void WinPlatform::exit(ExitCode code) const {
  std::ignore = code;

  // When debugging in Visual Studio the debugee is executed in an own shell window which is closed when it is finished.
  // That keeps us from seeing its output. By specifying the --wait command line parameter we will wait here before
  // closing the process.
  if (Process::waitBeforeExit) {
    std::cout << "\nExecution finished. Press any key.";
    while (!_kbhit())
      Sleep(100);
  };

  WSACleanup();
}

//----------------------------------------------------------------------------------------------------------------------

void WinPlatform::writeText(std::string const& text, bool error) const {
  if (error) {
    fputs(text.c_str(), stderr);
    fflush(stderr);
  } else {
    fputs(text.c_str(), stdout);
    fflush(stdout);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void WinPlatform::createFolder(std::string const& name) const {
  if (name.back() == ':') {
    UINT type = GetDriveType(Utilities::s2ws(name).c_str());
    if (type == DRIVE_UNKNOWN) {
      std::string message = "The drive type cannot be determined: '" + name;
      throw std::runtime_error(message);
    }
    return;
  }

  std::wstring converted = Utilities::s2ws(name);
  if (_wmkdir(converted.c_str()) != 0) {
    wchar_t *error = _wcserror(errno);
    std::string message = "Error while creating folder '" + name + "': ";
    throw std::runtime_error(message + Utilities::ws2s(error));
  }
}

//----------------------------------------------------------------------------------------------------------------------

void WinPlatform::removeFolder(std::string const& name) const {
  std::wstring converted = Utilities::s2ws(name);
  if (_wrmdir(converted.c_str()) != 0) {
    wchar_t *error = _wcserror(errno);
    std::string message = "Cannot remove folder '" + name + "': ";
    throw std::runtime_error(message + Utilities::ws2s(error));
  }
}

//----------------------------------------------------------------------------------------------------------------------

void WinPlatform::removeFile(std::string const& name) const {
  std::wstring converted = Utilities::s2ws(name);
  if (_wremove(converted.c_str()) != 0) {
    wchar_t *error = _wcserror(errno);
    std::string message = "Cannot remove file '" + name + "': ";
    throw std::runtime_error(message + Utilities::ws2s(error));
  }
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Size WinPlatform::getImageResolution(std::string const& path) const {
  int const CheckingPart = 24;
  std::ifstream is;
  is.open(path, std::ios::binary);
  is.seekg(0, std::ios::end);
  auto length = is.tellg();
  if (length < CheckingPart) {
    is.close();
    return {};
  }
  is.seekg(0, std::ios::beg);

  unsigned char buf[CheckingPart] = { 0 };
  is.read((char*)buf, sizeof(buf));
  // For JPEGs, we need to read the first 12 bytes of each chunk.
  if (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF && buf[3] == 0xE0 && buf[6] == 'J' && buf[7] == 'F' && buf[8] == 'I' && buf[9] == 'F') {
    long pos = 2;
    while (buf[2] == 0xFF) {
      if (buf[3] == 0xC0 || buf[3] == 0xC1 || buf[3] == 0xC2 || buf[3] == 0xC3 || buf[3] == 0xC9 || buf[3] == 0xCA || buf[3] == 0xCB)
        break;
      pos += 2 + (buf[4] << 8) + buf[5];
      if (pos + 12 > length) 
        break;
      is.seekg(pos, SEEK_SET);
      is.read((char*)buf + 2, CheckingPart - 2);
    }
  }
  is.close();

  geometry::Size size;
  if ((BYTE)buf[0] == 0xFF && (BYTE)buf[1] == 0xD8 && (BYTE)buf[2] == 0xFF) {
    size.height = (buf[7] << 8) + buf[8];
    size.width = (buf[9] << 8) + buf[10];
  } else if (buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F') {
    size.width = buf[6] + (buf[7] << 8);
    size.height = buf[8] + (buf[9] << 8);
  } else  if ((BYTE)buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G' && (BYTE)buf[4] == 0x0D &&
    (BYTE)buf[5] == 0x0A && (BYTE)buf[6] == 0x1A && (BYTE)buf[7] == 0x0A && buf[12] == 'I' && buf[13] == 'H' && buf[14] == 'D' && buf[15] == 'R') {
    size.width = (buf[16] << 24) + (buf[17] << 16) + (buf[18] << 8) + (buf[19] << 0);
    size.height = (buf[20] << 24) + (buf[21] << 16) + (buf[22] << 8) + (buf[23] << 0);
  }
  return size;
}

//----------------------------------------------------------------------------------------------------------------------

void WinPlatform::defineOsConstants(ScriptingContext &context, JSObject &constants) const {
  Platform::defineOsConstants(context, constants);

  // Need to define some additional error constants.
  JSObject errorNumbers = constants.get("errno");
  DEFINE_CONSTANT(errorNumbers, WSAEINTR);
  DEFINE_CONSTANT(errorNumbers, WSAEBADF);
  DEFINE_CONSTANT(errorNumbers, WSAEACCES);
  DEFINE_CONSTANT(errorNumbers, WSAEFAULT);
  DEFINE_CONSTANT(errorNumbers, WSAEINVAL);
  DEFINE_CONSTANT(errorNumbers, WSAEMFILE);
  DEFINE_CONSTANT(errorNumbers, WSAEWOULDBLOCK);
  DEFINE_CONSTANT(errorNumbers, WSAEINPROGRESS);
  DEFINE_CONSTANT(errorNumbers, WSAEALREADY);
  DEFINE_CONSTANT(errorNumbers, WSAENOTSOCK);
  DEFINE_CONSTANT(errorNumbers, WSAEDESTADDRREQ);
  DEFINE_CONSTANT(errorNumbers, WSAEMSGSIZE);
  DEFINE_CONSTANT(errorNumbers, WSAEPROTOTYPE);
  DEFINE_CONSTANT(errorNumbers, WSAENOPROTOOPT);
  DEFINE_CONSTANT(errorNumbers, WSAEPROTONOSUPPORT);
  DEFINE_CONSTANT(errorNumbers, WSAESOCKTNOSUPPORT);
  DEFINE_CONSTANT(errorNumbers, WSAEOPNOTSUPP);
  DEFINE_CONSTANT(errorNumbers, WSAEPFNOSUPPORT);
  DEFINE_CONSTANT(errorNumbers, WSAEAFNOSUPPORT);
  DEFINE_CONSTANT(errorNumbers, WSAEADDRINUSE);
  DEFINE_CONSTANT(errorNumbers, WSAEADDRNOTAVAIL);
  DEFINE_CONSTANT(errorNumbers, WSAENETDOWN);
  DEFINE_CONSTANT(errorNumbers, WSAENETUNREACH);
  DEFINE_CONSTANT(errorNumbers, WSAENETRESET);
  DEFINE_CONSTANT(errorNumbers, WSAECONNABORTED);
  DEFINE_CONSTANT(errorNumbers, WSAECONNRESET);
  DEFINE_CONSTANT(errorNumbers, WSAENOBUFS);
  DEFINE_CONSTANT(errorNumbers, WSAEISCONN);
  DEFINE_CONSTANT(errorNumbers, WSAENOTCONN);
  DEFINE_CONSTANT(errorNumbers, WSAESHUTDOWN);
  DEFINE_CONSTANT(errorNumbers, WSAETOOMANYREFS);
  DEFINE_CONSTANT(errorNumbers, WSAETIMEDOUT);
  DEFINE_CONSTANT(errorNumbers, WSAECONNREFUSED);
  DEFINE_CONSTANT(errorNumbers, WSAELOOP);
  DEFINE_CONSTANT(errorNumbers, WSAENAMETOOLONG);
  DEFINE_CONSTANT(errorNumbers, WSAEHOSTDOWN);
  DEFINE_CONSTANT(errorNumbers, WSAEHOSTUNREACH);
  DEFINE_CONSTANT(errorNumbers, WSAENOTEMPTY);
  DEFINE_CONSTANT(errorNumbers, WSAEPROCLIM);
  DEFINE_CONSTANT(errorNumbers, WSAEUSERS);
  DEFINE_CONSTANT(errorNumbers, WSAEDQUOT);
  DEFINE_CONSTANT(errorNumbers, WSAESTALE);
  DEFINE_CONSTANT(errorNumbers, WSAEREMOTE);
  DEFINE_CONSTANT(errorNumbers, WSASYSNOTREADY);
  DEFINE_CONSTANT(errorNumbers, WSAVERNOTSUPPORTED);
  DEFINE_CONSTANT(errorNumbers, WSANOTINITIALISED);
  DEFINE_CONSTANT(errorNumbers, WSAEDISCON);
  DEFINE_CONSTANT(errorNumbers, WSAENOMORE);
  DEFINE_CONSTANT(errorNumbers, WSAECANCELLED);
  DEFINE_CONSTANT(errorNumbers, WSAEINVALIDPROCTABLE);
  DEFINE_CONSTANT(errorNumbers, WSAEINVALIDPROVIDER);
  DEFINE_CONSTANT(errorNumbers, WSAEPROVIDERFAILEDINIT);
  DEFINE_CONSTANT(errorNumbers, WSASYSCALLFAILURE);
  DEFINE_CONSTANT(errorNumbers, WSASERVICE_NOT_FOUND);
  DEFINE_CONSTANT(errorNumbers, WSATYPE_NOT_FOUND);
  DEFINE_CONSTANT(errorNumbers, WSA_E_NO_MORE);
  DEFINE_CONSTANT(errorNumbers, WSA_E_CANCELLED);
  DEFINE_CONSTANT(errorNumbers, WSAEREFUSED);
}

//----------------------------------------------------------------------------------------------------------------------

void WinPlatform::defineFsConstants(ScriptingContext &context, JSObject &constants) const {
  Platform::defineFsConstants(context, constants);
  
  DEFINE_CONSTANT(constants, O_RDONLY);
  DEFINE_CONSTANT(constants, O_WRONLY);
  DEFINE_CONSTANT(constants, O_RDWR);
  DEFINE_CONSTANT(constants, S_IFMT);
  DEFINE_CONSTANT(constants, S_IFREG);
  DEFINE_CONSTANT(constants, S_IFDIR);
  DEFINE_CONSTANT(constants, S_IFCHR);
  DEFINE_CONSTANT(constants, S_IFLNK);
  DEFINE_CONSTANT(constants, O_CREAT);
  DEFINE_CONSTANT(constants, O_EXCL);
  DEFINE_CONSTANT(constants, O_TRUNC);
  DEFINE_CONSTANT(constants, O_APPEND);

  DEFINE_CONSTANT(constants, COPYFILE_EXCL);
  DEFINE_CONSTANT(constants, COPYFILE_FICLONE);
  DEFINE_CONSTANT(constants, COPYFILE_FICLONE_FORCE);
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<Cpu> WinPlatform::cpuInfo() const {
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  DWORD processorCount = systemInfo.dwNumberOfProcessors;

  ULONG processorInfoSize = processorCount * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);
  ULONG resultSize;
  auto *processorInfo = new SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[processorCount];
  NTSTATUS status = NtQuerySystemInformation(SystemProcessorPerformanceInformation, processorInfo, processorInfoSize,
    &resultSize);

  std::vector<Cpu> result;
  if (NT_SUCCESS(status)) {
    for (size_t i = 0; i < processorCount; ++i) {
      Cpu cpu;

      HKEY processorKey;
      std::wstring keyName = L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\" + std::to_wstring(i);
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName.c_str(), 0, KEY_QUERY_VALUE, &processorKey) == ERROR_SUCCESS) {
        
        DWORD speed;
        DWORD keySize = sizeof(speed);
        if (RegQueryValueEx(processorKey, L"~MHz", nullptr, nullptr, reinterpret_cast<BYTE*>(&speed), &keySize) == ERROR_SUCCESS) {
          cpu.speed = speed;
        }

        WCHAR brand[256];
        keySize = sizeof(brand);
        if (RegQueryValueEx(processorKey, L"ProcessorNameString", nullptr, nullptr, reinterpret_cast<BYTE*>(&brand),
          &keySize) == ERROR_SUCCESS) {
          cpu.model = Utilities::ws2s(brand);
        }
      }

      RegCloseKey(processorKey);

      cpu.user = processorInfo[i].UserTime.QuadPart / 10000;
      cpu.idle = processorInfo[i].IdleTime.QuadPart / 10000;
      cpu.sys = processorInfo[i].KernelTime.QuadPart - cpu.idle;

      // DPC and IRQ times are stored in the reserved area.
      cpu.irq = processorInfo[i].Reserved1[1].QuadPart / 10000;
      cpu.nice = 0;

      result.push_back(cpu);
    }
  }

  delete [] processorInfo;

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

size_t WinPlatform::getFreeMem() const {
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  if (GlobalMemoryStatusEx(&status) == TRUE)
    return status.ullAvailPhys;
  else
    throw std::runtime_error("GlobalMemoryStatusEx failed");
}

//----------------------------------------------------------------------------------------------------------------------

size_t WinPlatform::getTotalMem() const {
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  if (GlobalMemoryStatusEx(&status) == TRUE)
    return status.ullTotalPhys;
  else
    throw std::runtime_error("GlobalMemoryStatusEx failed");
}

//----------------------------------------------------------------------------------------------------------------------

std::string WinPlatform::getHomeDir() const {
  WCHAR path[MAX_PATH];
  if (GetEnvironmentVariable(L"USERPROFILE", path, MAX_PATH) > 0)
    return Utilities::ws2s(path);
  else
    throw std::runtime_error("GetEnvironmentVariable failed");
}

//----------------------------------------------------------------------------------------------------------------------

std::string WinPlatform::getHostName() const {
  WCHAR name[1024];
  if (GetHostNameW(name, 1024) == 0)
    return Utilities::ws2s(name);
  else
    throw std::runtime_error("GetHostNameW failed");
}

//----------------------------------------------------------------------------------------------------------------------

std::map<std::string, std::vector<NetworkInterface>> WinPlatform::networkInterfaces() const {
  std::map<std::string, std::vector<NetworkInterface>> result;

  // Getting the necessary buffer size does not follow usual conventions (passing a 0 and return the size).
  ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST;

  ULONG bufferSize = 8192;
  IP_ADAPTER_ADDRESSES *addressBuffer;
  while (true) {
    addressBuffer = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(malloc(bufferSize));
    ULONG error = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, addressBuffer, &bufferSize);
    if (error == ERROR_SUCCESS)
      break;

    free(addressBuffer);

    if (error != ERROR_BUFFER_OVERFLOW) {
      if (error == ERROR_ADDRESS_NOT_ASSOCIATED || error == ERROR_NO_DATA)
        // No error. We simply have not network interfaces configured.
        return result;

      throw std::runtime_error("GetAdaptersAddresses failed");
    }

    bufferSize *= 2;
  }
  
  for (IP_ADAPTER_ADDRESSES *adapter = addressBuffer; adapter != nullptr; adapter = adapter->Next) {
    if (adapter->OperStatus != IfOperStatusUp || adapter->FirstUnicastAddress == nullptr)
      continue;

    std::string nicName = Utilities::ws2s(adapter->FriendlyName);
    for (auto address = adapter->FirstUnicastAddress; address != nullptr; address = address->Next) {
      NetworkInterface nic;

      LPSOCKADDR socketAddress = address->Address.lpSockaddr;
      if (adapter->PhysicalAddressLength > 0) {
        std::string macAddress;
        for (size_t i = 0; i < adapter->PhysicalAddressLength; ++i)
          macAddress += Utilities::format("%.2X:", adapter->PhysicalAddress[i]);
        macAddress.resize(macAddress.size() - 1); // Remove last colon.
        nic.mac = macAddress;
      } else
        nic.mac = "00:00:00:00:00:00";

      nic.internal = (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK);

      UINT8 prefixLength = reinterpret_cast<IP_ADAPTER_UNICAST_ADDRESS_LH*>(address)->OnLinkPrefixLength;
      UINT8 remainingBits = prefixLength % 8;

      if (socketAddress->sa_family == AF_INET6) {
        auto ipv6SocketAddress = reinterpret_cast<struct sockaddr_in6 *>(socketAddress);

        char buffer[46];
        nic.address = inet_ntop(socketAddress->sa_family, &ipv6SocketAddress->sin6_addr, buffer, 46);

        IN6_ADDR networkMask;
        ZeroMemory(&networkMask, sizeof(IN6_ADDR));

        UINT8 byteCount = prefixLength / 8;
        memset(networkMask.u.Byte, 0xFF, byteCount); // Full bytes.
        if (remainingBits > 0)
          networkMask.u.Byte[byteCount] = 0xFF << (8 - remainingBits);
        nic.netmask = inet_ntop(socketAddress->sa_family, &networkMask, buffer, 46);

        nic.family = "IPv6";
        nic.scopeid = ipv6SocketAddress->sin6_scope_id;
      } else {
        auto ipv4SocketAddress = reinterpret_cast<struct sockaddr_in *>(socketAddress);

        char buffer[16];
        nic.address = inet_ntop(socketAddress->sa_family, &ipv4SocketAddress->sin_addr, buffer, 16);

        IN_ADDR networkMask;
        networkMask.s_addr = (prefixLength > 0) ? htonl(0xffffffff << (32 - prefixLength)) : 0;
        nic.netmask = inet_ntop(socketAddress->sa_family, &networkMask, buffer, 16);

        nic.family = "IPv4";
      }

      result[nicName].push_back(nic);
    }
  }

  free(addressBuffer);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

#pragma warning( disable: 4996 ) // GetVersionEx is deprecated.

std::string WinPlatform::getRelease() const {
  OSVERSIONINFO info;
  info.dwOSVersionInfoSize = sizeof(info);
  if (GetVersionEx(&info) == TRUE)
    return Utilities::format("%d.%d.%d", info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber);
  else
    throw std::runtime_error("GetVersionEx failed");
}

//----------------------------------------------------------------------------------------------------------------------

Version WinPlatform::getVersion() const {
  OSVERSIONINFO info;
  info.dwOSVersionInfoSize = sizeof(info);
  if (GetVersionEx(&info) == TRUE) {
    return { info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber };
  }
  else
    throw std::runtime_error("GetVersionEx failed");
}

//----------------------------------------------------------------------------------------------------------------------

#pragma warning( default: 4996 )

double WinPlatform::getUptime() const {
  return GetTickCount64() / 1000.0;
}

//----------------------------------------------------------------------------------------------------------------------

void WinPlatform::loadAvg(double (&avg)[3]) const {
  std::ignore = avg;
}


//----------------------------------------------------------------------------------------------------------------------

UserInfo WinPlatform::getCurrentUserInfo() const {
  UserInfo result;

  HANDLE token;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token) == TRUE) {
    TCHAR path[MAX_PATH];
    DWORD size = MAX_PATH;
    if (GetUserProfileDirectory(token, path, &size) == TRUE)
      result.homeDir = Utilities::ws2s(path);
  }
  CloseHandle(token);

  TCHAR buffer[UNLEN + 1];
  DWORD size = UNLEN;
  if (GetUserName(buffer, &size) == TRUE)
    result.userName = Utilities::ws2s(buffer);

  result.uid = -1;
  result.gid = -1;

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

UiToolkit WinPlatform::getUiToolkit() const {
  return UiToolkit::DotNet;
}

//----------------------------------------------------------------------------------------------------------------------

std::string WinPlatform::getClipboardText() const {
  return Accessible::getClipboardText();
}

//----------------------------------------------------------------------------------------------------------------------

void WinPlatform::setClipboardText(const std::string &content) const {
  if (!content.empty()) {
    Accessible::setClipboardText(content);
  }
}

//----------------------------------------------------------------------------------------------------------------------

BOOL MonitorEnumProc(HMONITOR hMonitor, HDC /*hdc*/, LPRECT /*prect*/, LPARAM data) {
  std::vector<Screen> *screens = reinterpret_cast<std::vector<Screen> *>(data);
  if (screens == nullptr)
    return false;

  MONITORINFOEX info;
  info.cbSize = sizeof(info);
  if (GetMonitorInfo(hMonitor, &info)) {
    Screen screen;
    screen.bounds.position = geometry::Point(0, 0);
    screen.bounds.size = geometry::Size(std::abs(info.rcMonitor.left - info.rcMonitor.right), std::abs(info.rcMonitor.top - info.rcMonitor.bottom));
    HDC dc = CreateDC(L"DISPLAY", info.szDevice, NULL, NULL);
    if (dc != nullptr) {
      int logicalScreenHeight = GetDeviceCaps(dc, VERTRES);
      int physicalScreenWidth = GetDeviceCaps(dc, DESKTOPHORZRES);
      int physicalScreenHeight = GetDeviceCaps(dc, DESKTOPVERTRES);
      screen.resolutionX = physicalScreenWidth;
      screen.resolutionY = physicalScreenHeight;
      float screenScalingFactor = (float)physicalScreenHeight / (float)logicalScreenHeight;
      screen.scaleFactor = screenScalingFactor;
      DeleteDC(dc);
    }
    screens->push_back(screen);
  }
  return true;
};

std::vector<Screen> WinPlatform::getScreens() const {
  std::vector<Screen> vec;
  if (EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&vec))) {
    return vec;
  }
  return {};  
}

//----------------------------------------------------------------------------------------------------------------------
