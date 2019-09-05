/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "platform.h"
#include "filesystem.h"
#include "global.h"
#include "scripting-context.h"

#include "utilities.h"

#define COPYFILE_EXCL 1

using namespace mga;
using namespace geometry;

class MacStat : public Stat {
  friend Stat;
protected:
  virtual void initialize(const std::string &path, bool followLinks) override;
  virtual timespec getTimeSpec(TimeSpecs spec) const override;
};

//----------------------------------------------------------------------------------------------------------------------

class MacPlatform : public Platform {
public:
  virtual ~MacPlatform() {};

  virtual int launchApplication(const std::string &name, const std::vector<std::string> &params, bool newInstance,
                                ShowState showState, std::map<std::string, std::string> const& env) const override;

  virtual int getPidByName(const std::string &name) const override;
  virtual std::string getTempDirName() const override;
  virtual bool terminate(int processID, bool force = false) const override;
  virtual void runLoopRun(ScriptingContext &context) const override;
  virtual bool isRunning(int processID) const override;

  virtual std::vector<Cpu> cpuInfo() const override;
  virtual size_t getFreeMem() const override;
  virtual size_t getTotalMem() const override;
  virtual std::map<std::string, std::vector<NetworkInterface>> networkInterfaces() const override;
  virtual Version getVersion() const override;
  virtual double getUptime() const override;
  virtual geometry::Size getImageResolution(std::string const& path) const override;
  virtual void defineOsConstants(ScriptingContext &context, JSObject &constants) const override;
  virtual void defineFsConstants(ScriptingContext &context, JSObject &constants) const override;
  virtual void loadAvg(double (&avg)[3]) const override;
  virtual UiToolkit getUiToolkit() const override;
  virtual std::string getClipboardText() const override;
  virtual void setClipboardText(const std::string &text) const override;
  virtual std::vector<Screen> getScreens() const override;
};

//----------------------------------------------------------------------------------------------------------------------

std::unique_ptr<Stat> Stat::get(const std::string &path, bool followLinks) {
  MacStat *result = new MacStat();
  result->initialize(path, followLinks);
  return std::unique_ptr<Stat>(result);
}

//----------------------------------------------------------------------------------------------------------------------

void MacStat::initialize(const std::string &path, bool followLinks) {
  _path = path;
  if (followLinks) {
    if (stat(path.c_str(), &_buffer) != 0)
      throw std::runtime_error("Cannot create stats: " + Utilities::getLastError());
  } else {
    if (lstat(path.c_str(), &_buffer) != 0)
      throw std::runtime_error("Cannot create stats: " + Utilities::getLastError());
  }

  _blockSize = _buffer.st_blksize;
  _blocks = _buffer.st_blocks;
}

//----------------------------------------------------------------------------------------------------------------------

timespec MacStat::getTimeSpec(TimeSpecs spec) const {
  switch(spec) {
    case TimeSpecs::atime:
      return _buffer.st_atimespec;
    case TimeSpecs::mtime:
      return _buffer.st_mtimespec;
    case TimeSpecs::ctime:
    case TimeSpecs::birthdate:
      return _buffer.st_ctimespec;
    default:
      throw std::runtime_error("Invalid time specification");
  }
  return timespec();

}

//----------------------------------------------------------------------------------------------------------------------

Platform& Platform::get() {
  static MacPlatform singleton;
  return singleton;
}

//----------------------------------------------------------------------------------------------------------------------

int MacPlatform::launchApplication(const std::string &name, const std::vector<std::string> &params, bool newInstance,
                                   ShowState showState, std::map<std::string, std::string> const& env) const {
  NSString *path = [NSString stringWithUTF8String: name.c_str()];

  NSMutableArray *args = [NSMutableArray new];
  for (size_t i = 0; i < params.size(); ++i)
    [args addObject: [NSString stringWithUTF8String: params[i].c_str()]];

  NSError *error = nil;
  NSWorkspaceLaunchOptions options = NSWorkspaceLaunchWithoutAddingToRecents;
  if (newInstance)
    options |= NSWorkspaceLaunchNewInstance;
  if (showState == ShowState::Hidden)
    options |= NSWorkspaceLaunchAndHide;
  else if (showState == ShowState::HideOthers)
    options |= NSWorkspaceLaunchAndHideOthers;

  auto environment = [NSMutableDictionary new];
  for (auto &pair : env) {
    environment[[NSString stringWithUTF8String: pair.first.c_str()]] = [NSString stringWithUTF8String: pair.second.c_str()];
  }

  auto configuration = @{
    NSWorkspaceLaunchConfigurationEnvironment: environment,
    NSWorkspaceLaunchConfigurationArguments: args
  };
  NSRunningApplication *application =
    [NSWorkspace.sharedWorkspace launchApplicationAtURL: [NSURL fileURLWithPath: path]
                                                options: options
                                          configuration: configuration
                                                  error: &error];
  if (error != nil)
    throw std::runtime_error(error.localizedDescription.UTF8String);

  if (showState != ShowState::Hidden)
    [application activateWithOptions: NSApplicationActivateAllWindows];

  return application.processIdentifier;
}

//----------------------------------------------------------------------------------------------------------------------

int MacPlatform::getPidByName(const std::string &name) const {
  NSArray<NSRunningApplication *> *applications = [NSWorkspace.sharedWorkspace runningApplications];
  NSString *appName = [NSString stringWithUTF8String: name.c_str()];
  for (NSRunningApplication *application : applications) {
    if ([application.localizedName isEqualToString: appName])
      return application.processIdentifier;
  }

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

std::string MacPlatform::getTempDirName() const {
  // Not really system conformal, but we want Node.js compatibility.
  const char* temp = getenv("TMPDIR");
  if (temp != nullptr)
    return temp;
  return [NSTemporaryDirectory() UTF8String];
}

//----------------------------------------------------------------------------------------------------------------------

bool MacPlatform::terminate(int processID, bool force) const {
  NSRunningApplication *application = [NSRunningApplication runningApplicationWithProcessIdentifier: processID];
  if (application != nil && application != NSRunningApplication.currentApplication) {
    bool terminationInProgress = false;
    if (force)
      terminationInProgress =  [application forceTerminate];
    else
      terminationInProgress =  [application terminate];
    if (terminationInProgress) {
      // Wait for completion of the termination.
      while (!application.terminated && [NSRunLoop.currentRunLoop runMode: NSDefaultRunLoopMode
                                                               beforeDate: [NSDate.date dateByAddingTimeInterval: 0.010]])
        ;

      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void MacPlatform::runLoopRun(ScriptingContext &context) const {
  // Create the shared application object to allow working with stuff that requires it
  // (like borderless windows).
  [NSApplication sharedApplication];

  while (!JSContext::stopRunloop && [NSRunLoop.currentRunLoop runMode: NSDefaultRunLoopMode
                                                           beforeDate: [NSDate.date dateByAddingTimeInterval: 0.010]]) {
    context.expireTimers();

    // TODO: file + sockets.

    context.runImmediates();

    if (!context.callbacksPending())
      break;
  };
}

//----------------------------------------------------------------------------------------------------------------------

bool MacPlatform::isRunning(int processID) const {
  NSRunningApplication *application = [NSRunningApplication runningApplicationWithProcessIdentifier: processID];
  return application != nil;
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<Cpu> MacPlatform::cpuInfo() const {
  std::vector<Cpu> result;

  uint64_t cpuSpeed;
  size_t size = sizeof(cpuSpeed);
  if (sysctlbyname("hw.cpufrequency", &cpuSpeed, &size, nullptr, 0) != 0)
    cpuSpeed = 0;

  natural_t cpuCount;
  processor_cpu_load_info_data_t *info;
  mach_msg_type_number_t infoCount;
  kern_return_t kernelResult = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpuCount,
                                                   reinterpret_cast<processor_info_array_t*>(&info), &infoCount);
  if (kernelResult != KERN_SUCCESS) {
    return result;
  }

  size_t ticks = static_cast<size_t>(sysconf(_SC_CLK_TCK));
  size_t multiplier = 1000L / ticks;

  for (natural_t i = 0; i < cpuCount; ++i) {
    Cpu cpu { "", cpuSpeed / 1000000,
      (info[i].cpu_ticks[0]) * multiplier,
      (info[i].cpu_ticks[3]) * multiplier,
      (info[i].cpu_ticks[1]) * multiplier,
      (info[i].cpu_ticks[2]) * multiplier,
      0 // No irq info.
    };
    result.push_back(cpu);
  }

  vm_deallocate(mach_task_self(), (vm_address_t)info, infoCount);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

size_t MacPlatform::getFreeMem() const {
  vm_statistics_data_t info;
  mach_msg_type_number_t count = sizeof(info) / sizeof(integer_t);

  if (host_statistics(mach_host_self(), HOST_VM_INFO, reinterpret_cast<host_info_t>(&info), &count) != KERN_SUCCESS) {
    return 0;
  }

  return info.free_count * static_cast<natural_t>(sysconf(_SC_PAGESIZE));
}

//----------------------------------------------------------------------------------------------------------------------

std::map<std::string, std::vector<NetworkInterface>> MacPlatform::networkInterfaces() const {
  std::map<std::string, std::vector<NetworkInterface>> result;

  struct ifaddrs *addresses;
  if (getifaddrs(&addresses))
    return result;

  // First collect mac addresses for each interface (identified by name).
  // We use this map later when creating the result interface instances.
  std::map<std::string, std::string> macAddresses;
  for (ifaddrs *run = addresses; run != nullptr; run = run->ifa_next) {
    if (run->ifa_addr->sa_family == AF_LINK) {
      struct sockaddr_dl *sa_addr = reinterpret_cast<struct sockaddr_dl *>(run->ifa_addr);
      auto macAddress = reinterpret_cast<unsigned char *>(LLADDR(sa_addr));
      macAddresses[run->ifa_name] = Utilities::format("%02x:%02x:%02x:%02x:%02x:%02x",
                                                  *macAddress, *(macAddress + 1), *(macAddress + 2),
                                                  *(macAddress + 3), *(macAddress + 4), *(macAddress + 5));
    }
  }

  for (ifaddrs *run = addresses; run != nullptr; run = run->ifa_next) {
    if ((run->ifa_flags & IFF_UP) == 0 || (run->ifa_flags & IFF_RUNNING) == 0)
      continue;

    if (run->ifa_addr == nullptr || run->ifa_addr->sa_family == AF_LINK)
      continue;

    NetworkInterface interface;
    std::string name = run->ifa_name;
    interface.family = run->ifa_addr->sa_family == AF_INET6 ? "IPv6" : "IPv4";

    char buffer[INET6_ADDRSTRLEN];
    if (getnameinfo(run->ifa_addr, sizeof(run->ifa_addr), buffer, sizeof(buffer), 0, 0, NI_NUMERICHOST) == 0) {
      interface.address = buffer;

      // Remove the interface name appendix if there's any. We already have the interface name separated.
      size_t position = interface.address.find("%");
      if (position != std::string::npos)
        interface.address.resize(position);
    }

    if (getnameinfo(run->ifa_netmask, sizeof(run->ifa_netmask), buffer, sizeof(buffer), 0, 0, NI_NUMERICHOST) == 0)
      interface.netmask = buffer;
    interface.internal = (run->ifa_flags & IFF_LOOPBACK) != 0;

    auto macIterator = macAddresses.find(run->ifa_name);
    if (macIterator == macAddresses.end())
      interface.mac = "00::00::00::00::00::00";
    else
      interface.mac = macIterator->second;

    if (run->ifa_addr->sa_family == AF_INET6) {
      auto ip6Addr = reinterpret_cast<struct sockaddr_in6 *>(run->ifa_addr);
      interface.scopeid = ip6Addr->sin6_scope_id;
    }

    result[name].push_back(interface);
  }

  freeifaddrs(addresses);

  return result;
}

//---------------------------------------------------------------------------------------------------------------------

size_t MacPlatform::getTotalMem() const {
  uint64_t result;
  int which[] = { CTL_HW, HW_MEMSIZE };
  size_t size = sizeof(result);

  if (sysctl(which, 2, &result, &size, nullptr, 0) != 0)
    return 0;

  return result;
}

//---------------------------------------------------------------------------------------------------------------------

void MacPlatform::loadAvg(double (&avg)[3]) const {
  struct loadavg info;
  size_t size = sizeof(info);
  int which[] = { CTL_VM, VM_LOADAVG };

  if (sysctl(which, 2, &info, &size, nullptr, 0) >= 0) {
    avg[0] = static_cast<double>(info.ldavg[0]) / info.fscale;
    avg[1] = static_cast<double>(info.ldavg[1]) / info.fscale;
    avg[2] = static_cast<double>(info.ldavg[2]) / info.fscale;
  }
}

//----------------------------------------------------------------------------------------------------------------------

Version MacPlatform::getVersion() const {
  NSOperatingSystemVersion version = NSProcessInfo.processInfo.operatingSystemVersion;
  return {
    static_cast<size_t>(version.majorVersion),
    static_cast<size_t>(version.minorVersion),
    static_cast<size_t>(version.patchVersion)
  };
}

//----------------------------------------------------------------------------------------------------------------------

double MacPlatform::getUptime() const {
  struct timeval info;
  size_t size = sizeof(info);
  static int which[] = { CTL_KERN, KERN_BOOTTIME };

  if (sysctl(which, 2, &info, &size, nullptr, 0) != 0)
    return 0;

  time_t now = time(NULL);
  return now - info.tv_sec;
}

//----------------------------------------------------------------------------------------------------------------------

void MacPlatform::defineOsConstants(ScriptingContext &context, JSObject &constants) const {
  Platform::defineOsConstants(context, constants);

  JSObject signals = constants.get("signals");

  DEFINE_CONSTANT(signals, SIGHUP);
  DEFINE_CONSTANT(signals, SIGQUIT);
  DEFINE_CONSTANT(signals, SIGTRAP);
  DEFINE_CONSTANT(signals, SIGIOT);
  DEFINE_CONSTANT(signals, SIGBUS);
  DEFINE_CONSTANT(signals, SIGKILL);
  DEFINE_CONSTANT(signals, SIGUSR1);
  DEFINE_CONSTANT(signals, SIGUSR2);
  DEFINE_CONSTANT(signals, SIGPIPE);
  DEFINE_CONSTANT(signals, SIGALRM);
  DEFINE_CONSTANT(signals, SIGCHLD);
  DEFINE_CONSTANT(signals, SIGCONT);
  DEFINE_CONSTANT(signals, SIGSTOP);
  DEFINE_CONSTANT(signals, SIGTSTP);
  DEFINE_CONSTANT(signals, SIGTTIN);
  DEFINE_CONSTANT(signals, SIGTTOU);
  DEFINE_CONSTANT(signals, SIGURG);
  DEFINE_CONSTANT(signals, SIGXCPU);
  DEFINE_CONSTANT(signals, SIGXFSZ);
  DEFINE_CONSTANT(signals, SIGVTALRM);
  DEFINE_CONSTANT(signals, SIGPROF);
  DEFINE_CONSTANT(signals, SIGWINCH);
  DEFINE_CONSTANT(signals, SIGIO);
  DEFINE_CONSTANT(signals, SIGSYS);

  JSObject errorNumbers = constants.get("errno");

  DEFINE_CONSTANT(errorNumbers, EDQUOT);
  DEFINE_CONSTANT(errorNumbers, EMULTIHOP);
  DEFINE_CONSTANT(errorNumbers, ESTALE);
}

//----------------------------------------------------------------------------------------------------------------------

void MacPlatform::defineFsConstants(ScriptingContext &context, JSObject &constants) const {
  Platform::defineFsConstants(context, constants);

  DEFINE_CONSTANT(constants, O_RDONLY);
  DEFINE_CONSTANT(constants, O_WRONLY);
  DEFINE_CONSTANT(constants, O_RDWR);
  DEFINE_CONSTANT(constants, O_CREAT);
  DEFINE_CONSTANT(constants, O_EXCL);
  DEFINE_CONSTANT(constants, O_NOCTTY);
  DEFINE_CONSTANT(constants, O_TRUNC);
  DEFINE_CONSTANT(constants, O_APPEND);
  DEFINE_CONSTANT(constants, O_DIRECTORY);
  DEFINE_CONSTANT(constants, O_NOFOLLOW);
  DEFINE_CONSTANT(constants, O_SYNC);
  DEFINE_CONSTANT(constants, O_DSYNC);
  DEFINE_CONSTANT(constants, O_SYMLINK);
  DEFINE_CONSTANT(constants, O_NONBLOCK);

  DEFINE_CONSTANT(constants, S_IFMT);
  DEFINE_CONSTANT(constants, S_IFREG);
  DEFINE_CONSTANT(constants, S_IFDIR);
  DEFINE_CONSTANT(constants, S_IFCHR);
  DEFINE_CONSTANT(constants, S_IFBLK);
  DEFINE_CONSTANT(constants, S_IFIFO);
  DEFINE_CONSTANT(constants, S_IFLNK);
  DEFINE_CONSTANT(constants, S_IFSOCK);
  DEFINE_CONSTANT(constants, S_IRWXU);
  DEFINE_CONSTANT(constants, S_IRUSR);
  DEFINE_CONSTANT(constants, S_IWUSR);
  DEFINE_CONSTANT(constants, S_IXUSR);
  DEFINE_CONSTANT(constants, S_IRWXG);
  DEFINE_CONSTANT(constants, S_IRGRP);
  DEFINE_CONSTANT(constants, S_IWGRP);
  DEFINE_CONSTANT(constants, S_IXGRP);
  DEFINE_CONSTANT(constants, S_IRWXO);
  DEFINE_CONSTANT(constants, S_IROTH);
  DEFINE_CONSTANT(constants, S_IWOTH);
  DEFINE_CONSTANT(constants, S_IXOTH);

  DEFINE_CONSTANT(constants, COPYFILE_EXCL);
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Size MacPlatform::getImageResolution(std::string const& path) const {
  geometry::Size size(0, 0);

  NSURL *imageFileURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
  CGImageSourceRef imageSource = CGImageSourceCreateWithURL((CFURLRef)imageFileURL, NULL);
  if (imageSource == NULL) {
    // Error loading image
    return size;
  }

  NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:
                           [NSNumber numberWithBool: NO], (__bridge NSString *)kCGImageSourceShouldCache,
                           nil];
  CFDictionaryRef imageProperties = CGImageSourceCopyPropertiesAtIndex(imageSource, 0, (CFDictionaryRef)options);
  if (imageProperties) {
    NSNumber *width = (NSNumber *)CFDictionaryGetValue(imageProperties, kCGImagePropertyPixelWidth);
    NSNumber *height = (NSNumber *)CFDictionaryGetValue(imageProperties, kCGImagePropertyPixelHeight);
    size.width = [width intValue];
    size.height = [height intValue];
    CFRelease(imageProperties);
  }
  CFRelease(imageSource);

  return size;
}

//----------------------------------------------------------------------------------------------------------------------

UiToolkit MacPlatform::getUiToolkit() const {
  return UiToolkit::Cocoa;
}

//----------------------------------------------------------------------------------------------------------------------

std::string MacPlatform::getClipboardText() const {
  NSPasteboard *pasteBoard = NSPasteboard.generalPasteboard;
  return [pasteBoard stringForType: NSPasteboardTypeString].UTF8String ?: "";
}

//----------------------------------------------------------------------------------------------------------------------

void MacPlatform::setClipboardText(const std::string &text) const {
  NSPasteboard *pasteBoard = NSPasteboard.generalPasteboard;
  [pasteBoard declareTypes: @[NSPasteboardTypeString] owner: nil];
  [pasteBoard setString: @(text.c_str()) forType: NSPasteboardTypeString];
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<Screen> MacPlatform::getScreens() const {
  std::vector<Screen> result;

  // Coordinates in cocoa are vertically flipped compared to those used by the accessibility implementation.
  // Both use logical coordinates, however, which is important for retina screens or scaled content.
  // We have to convert screen coordinates so that the screen rect is still correctly positioned wrt the main screen
  // (which defines the (0, 0) origin at its upper left corner).
  NSRect mainFrame = NSScreen.mainScreen.frame;
  for (NSScreen *screen in NSScreen.screens) {
    NSRect frame = screen.frame;
    auto device = screen.deviceDescription;
    double scaleFactor = static_cast<double>(screen.backingScaleFactor);

    Rectangle bounds { static_cast<int>(frame.origin.x),
      static_cast<int>(mainFrame.size.height - frame.size.height - frame.origin.y),
      static_cast<int>(frame.size.width), static_cast<int>(frame.size.height) };

    NSSize resolution = [device[NSDeviceResolution] sizeValue];
    Screen entry { bounds, static_cast<size_t>(resolution.width), static_cast<size_t>(resolution.height), scaleFactor };
    result.push_back(entry);
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------
