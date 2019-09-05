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

#include <signal.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <glibmm/fileutils.h>
#include <glibmm/spawn.h>
#include <glibmm/vectorutils.h>
#include <glibmm/miscutils.h>
#include <giomm/appinfo.h>
#include <giomm/file.h>

#include <gtkmm.h>
#include <gdkmm/wrap_init.h>
#include <gtkmm/wrap_init.h>
#include <atkmm/wrap_init.h>
#include <cstring>
#include <algorithm>

#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <sys/utsname.h>
//  Undefine Success so it doesn't interfeer with our ExitCode::Success definition. we don't need this one anyway.
#undef Success


GMainContext *mainGlibContext;


#include "platform.h"
#include "global.h"
#include "utilities.h"
#include "path.h"
#include "utilities.h"
#include "process.h"

using namespace mga;

class LinuxStat : public Stat {
  friend Stat;
protected:
  virtual void initialize(const std::string &path, bool followSymlinks) override;
  virtual timespec getTimeSpec(TimeSpecs spec) const override;
public:
  virtual ~LinuxStat() {};
};

//----------------------------------------------------------------------------------------------------------------------

class LinuxPlatform : public Platform {
public:
  virtual ~LinuxPlatform();
  virtual int launchApplication(const std::string &name, const std::vector<std::string> &params,
    bool newInstance, ShowState showState, std::map<std::string, std::string> const& env = {}) const override;
    
  virtual int getPidByName(const std::string &name) const override;
  virtual std::string getTempDirName() const override;
  virtual bool terminate(int processID, bool force = false) const override;
  virtual void initialize(int argc, const char* argv[], char *envp[]) const override;
  virtual void exit(ExitCode code) const override;
  virtual void runLoopRun(ScriptingContext &context) const override;
  virtual bool isRunning(int processID) const override;
  
  virtual geometry::Size getImageResolution(std::string const& path) const override;
  virtual void defineOsConstants(ScriptingContext &context, JSObject &constants) const override;
  virtual void defineFsConstants(ScriptingContext &context, JSObject &constants) const override;
  virtual std::vector<Cpu> cpuInfo() const override;
  virtual size_t getFreeMem() const override;
  virtual size_t getTotalMem() const override;
  virtual void loadAvg(double (&avg)[3]) const override;
  virtual double getUptime() const override;
  virtual std::map<std::string, std::vector<NetworkInterface>> networkInterfaces() const override;
  virtual Version getVersion() const override;
  virtual std::string getDistroName() const override;
  virtual UiToolkit getUiToolkit() const override;
  virtual std::string getClipboardText() const override;
  virtual void setClipboardText(const std::string &text) const override;
  std::vector<mga::Screen> getScreens() const override;
};

//----------------------------------------------------------------------------------------------------------------------

std::unique_ptr<Stat> Stat::get(const std::string &path, bool followSymlinks) {
  auto result = std::make_unique<LinuxStat>();
  result->initialize(path, followSymlinks);
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void LinuxStat::initialize(const std::string &path, bool followSymlinks) {
  if (followSymlinks) {
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

timespec LinuxStat::getTimeSpec(TimeSpecs spec) const {
  switch(spec) {
    case TimeSpecs::atime:
      return _buffer.st_atim;
    case TimeSpecs::mtime:
      return _buffer.st_mtim;
    case TimeSpecs::ctime:
    case TimeSpecs::birthdate:
      return _buffer.st_ctim;
    default:
      throw std::runtime_error("Invalid time specification");
  }
  return timespec();
}

//----------------------------------------------------------------------------------------------------------------------

Platform& Platform::get() {
  static LinuxPlatform singleton;
  return singleton;
}

//----------------------------------------------------------------------------------------------------------------------

static std::string getBasename(const std::string &cmdline) {
  std::string result = cmdline;

  auto pos = result.find('\0');
  if (pos != std::string::npos)
    result.resize(pos);

  char *dn = g_path_get_basename(result.c_str());
  result = dn;
  g_free(dn);
  return result;
}
//----------------------------------------------------------------------------------------------------------------------

static int isRunningByName(const std::string &name) {
  Glib::Dir dir("/proc");
  for (const auto &elem: dir) {
    try {
      auto file = Utilities::concat({ "/proc", elem, "task", elem, "cmdline" }, "/");
      auto cmdline = getBasename(Glib::file_get_contents(file));
      if (cmdline.compare(getBasename(name)) == 0) {
        return std::stoi(elem);
      }
    } catch (Glib::FileError &ferror) {
      continue;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

LinuxPlatform::~LinuxPlatform() {
  Glib::wrap_register_cleanup();
  Glib::Error::register_cleanup();
}

int LinuxPlatform::launchApplication(const std::string &name, const std::vector<std::string> &params,
                                     bool newInstance, ShowState showState,
                                     std::map<std::string, std::string> const& env) const {

  if (!newInstance) {
    Glib::Pid pid = isRunningByName(name);
    if (pid != 0)
      return pid;
  }
  auto tmpParams = params;
  tmpParams.insert(tmpParams.begin(), name);

  auto e = g_get_environ();
  auto envp = Glib::ArrayHandler<std::string>::array_to_vector(e, Glib::OWNERSHIP_NONE);

  if (env.empty()) {
    envp.erase(std::remove_if(envp.begin(), envp.end(), [](const std::string &str) {
      return str.find("LD_PRELOAD") != std::string::npos;
    }), envp.end());
  } else {
    envp.clear();
    for(auto it: env) {
      envp.push_back(std::string(it.first+"="+it.second));
    }
  }

  g_strfreev(e);
  Glib::Pid pid = 0;
  try {
    Glib::spawn_async(Glib::get_current_dir(), tmpParams, envp, Glib::SPAWN_SEARCH_PATH, sigc::slot<void>(), &pid);
  } catch (Glib::SpawnError &serr) {
    throw std::runtime_error(serr.what());
  }
  return pid;
}

//----------------------------------------------------------------------------------------------------------------------

int LinuxPlatform::getPidByName(const std::string &name) const {
  return isRunningByName(name);
}

//----------------------------------------------------------------------------------------------------------------------

std::string LinuxPlatform::getTempDirName() const {
  return g_get_tmp_dir();
}

//----------------------------------------------------------------------------------------------------------------------

bool LinuxPlatform::terminate(int processID, bool force) const {
  pid_t pid = processID;
  if (pid != 0) {
    int err = kill(pid, force ? SIGKILL : SIGTERM);
    return (err == 0);
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void LinuxPlatform::initialize(int argc, const char* argv[], char *envp[]) const {
  gtk_disable_setlocale();
  gtk_init(0, 0);
  Glib::init();
  Gio::init();
  Gdk::wrap_init();
  Gtk::wrap_init();
  Atk::wrap_init();

  mainGlibContext = g_main_context_default();
  if (!g_main_context_acquire (mainGlibContext))
    throw std::runtime_error("Can't acquire global glib context");
}

//----------------------------------------------------------------------------------------------------------------------

void LinuxPlatform::exit(ExitCode code) const {
  g_main_context_release (mainGlibContext);
}

//----------------------------------------------------------------------------------------------------------------------

void LinuxPlatform::runLoopRun(ScriptingContext &context) const {
  while (!JSContext::stopRunloop) {
    g_main_context_iteration(mainGlibContext, false);

    context.expireTimers();

    // TODO: file + sockets.

    context.runImmediates();

    if (!context.callbacksPending())
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool LinuxPlatform::isRunning(int processID) const {
  pid_t pid = processID;
  if (pid != 0) {
    int err = kill(pid, 0);
    return (err == 0);
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void LinuxPlatform::defineOsConstants(ScriptingContext &context, JSObject &constants) const {
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

  DEFINE_CONSTANT(signals, SIGSTKFLT);
  DEFINE_CONSTANT(signals, SIGPOLL);
  DEFINE_CONSTANT(signals, SIGPWR);

  JSObject errorNumbers = constants.get("errno");
  
  DEFINE_CONSTANT(errorNumbers, EDQUOT);
  DEFINE_CONSTANT(errorNumbers, EMULTIHOP);
  DEFINE_CONSTANT(errorNumbers, ESTALE);
}

//----------------------------------------------------------------------------------------------------------------------

void LinuxPlatform::defineFsConstants(ScriptingContext &context, JSObject &constants) const {
  Platform::defineFsConstants(context, constants);
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Size LinuxPlatform::getImageResolution(std::string const& path) const {
  auto img = Gdk::Pixbuf::create_from_file(path);
  return geometry::Size(img->get_width(), img->get_height());
}

//----------------------------------------------------------------------------------------------------------------------

static void readProcTimes(std::vector<Cpu> &result) {
  unsigned long clockTicks = sysconf(_SC_CLK_TCK);
  std::istringstream input(Glib::file_get_contents("/proc/stat"));

  unsigned long user;
  unsigned long nice;
  unsigned long sys;
  unsigned long idle;
  unsigned long irq;

  unsigned int cpuCount = result.size();
  unsigned int cpuIter = 0;
  for(std::string line; std::getline(input, line); ) {
    if (cpuIter >= cpuCount)
      break;

    std::string cpuPrefix = "cpu" + std::to_string(cpuIter)+" ";
    if (std::strncmp(line.c_str(), cpuPrefix.c_str(), cpuPrefix.size()))
    {
      std::string cpuTime = line.substr(cpuPrefix.size(), std::string::npos);
      if (sscanf(cpuTime.c_str(), "%lu %lu %lu %lu %*u %lu", &user, &nice, &sys, &idle, &irq) != 5)
        throw std::runtime_error("Unable to read cpu times");
      result[cpuIter].irq = irq * clockTicks;
      result[cpuIter].idle = idle * clockTicks;
      result[cpuIter].sys = sys * clockTicks;
      result[cpuIter].nice = nice * clockTicks;
      result[cpuIter].user = user * clockTicks;
      cpuIter++;
    }
  }
}

std::vector<Cpu> LinuxPlatform::cpuInfo() const {

  std::vector<Cpu> result;
  unsigned int cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
  result.resize(cpuCount);

  const std::string modelPrefix = "model name\t:";
  const std::string speedPrefix = "cpu MHz\t\t:";

  unsigned int cpuIter = 0;
  std::istringstream input(Glib::file_get_contents("/proc/cpuinfo"));
  for(std::string line; std::getline(input, line); ) {
    if (cpuIter >= cpuCount)
      break;

    if (std::strncmp(line.c_str(), modelPrefix.c_str(), modelPrefix.size()) == 0)
      result[cpuIter].model = line.substr(modelPrefix.size(), std::string::npos);

    if (std::strncmp(line.c_str(), speedPrefix.c_str(), speedPrefix.size()) == 0) {
      result[cpuIter].speed = std::atoi(line.substr(speedPrefix.size(), std::string::npos).c_str());
      ++cpuIter;
    }
  }

  readProcTimes(result);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

size_t LinuxPlatform::getFreeMem() const {
  struct sysinfo info;
  ::sysinfo(&info);
  return info.freeram;
}

//----------------------------------------------------------------------------------------------------------------------

size_t LinuxPlatform::getTotalMem() const {
  struct sysinfo info;
  ::sysinfo(&info);
  return info.totalram;
}

//----------------------------------------------------------------------------------------------------------------------

void LinuxPlatform::loadAvg(double (&avg)[3]) const {
  getloadavg(avg, 3);
}

static inline bool fileExists(const std::string& fileName) {
  struct stat buff;
  return stat(fileName.c_str(), &buff)==0;
}

//----------------------------------------------------------------------------------------------------------------------

static std::string getSystemInfo(const std::string &prefix) {
  if (fileExists("/etc/os-release"))
  {
    std::istringstream input(Glib::file_get_contents("/etc/os-release"));
    for(std::string line; std::getline(input, line); ) {
      if (std::strncmp(line.c_str(), prefix.c_str(), prefix.size()) == 0) {
            auto value = line.substr(prefix.size(), std::string::npos);
            if(!value.empty()) {
              if (value.front() == '"' && value.back() == '"')
                return value.substr(1, value.size() -2);
            }
            return value;
      }
    }
  } else {
   throw std::runtime_error("Unable to obtain system information, missing /etc/os-release file.");
  }
  return {};
}

//----------------------------------------------------------------------------------------------------------------------

Version LinuxPlatform::getVersion() const {
  auto stringVersion = getSystemInfo("VERSION_ID=");
  auto splitVersion = Utilities::split(stringVersion, ".");
  Version ver = {0, 0, 0};
  if (splitVersion.size() > 0)
    ver.major = std::atoi(splitVersion[0].c_str());

  if (splitVersion.size() > 1)
    ver.minor = std::atoi(splitVersion[1].c_str());

  if (splitVersion.size() > 2)
    ver.patch = std::atoi(splitVersion[2].c_str());

  return ver;

}

//----------------------------------------------------------------------------------------------------------------------

std::string LinuxPlatform::getDistroName() const {
  return getSystemInfo("NAME=");
}

//----------------------------------------------------------------------------------------------------------------------

double LinuxPlatform::getUptime() const {
  struct sysinfo info;
  ::sysinfo(&info);
  return info.uptime;
}

//----------------------------------------------------------------------------------------------------------------------

std::map<std::string, std::vector<NetworkInterface>> LinuxPlatform::networkInterfaces() const {
  std::map<std::string, std::vector<NetworkInterface>> result;
  
  ifaddrs *interfaces = nullptr;

  int success = getifaddrs(&interfaces);
  
  if (success == -1)
    throw std::runtime_error("Unable to get the available network interfacess: " + Utilities::getLastError());
  
  for(; interfaces != nullptr; interfaces = interfaces->ifa_next) {
    if (interfaces->ifa_addr == nullptr || interfaces->ifa_addr->sa_family != PF_INET || interfaces->ifa_addr->sa_family != AF_INET)
      continue;

    NetworkInterface iface;
    iface.address = inet_ntoa(reinterpret_cast<sockaddr_in *>(interfaces->ifa_addr)->sin_addr);
    iface.netmask = inet_ntoa(reinterpret_cast<sockaddr_in *>(interfaces->ifa_netmask)->sin_addr);
    iface.internal = (interfaces->ifa_flags & IFF_LOOPBACK) != 0;

    if (interfaces->ifa_addr->sa_family == AF_INET6) {
      iface.scopeid = reinterpret_cast<struct sockaddr_in6 *>(interfaces->ifa_addr)->sin6_scope_id;
      iface.family = "IPv6";
    } else
      iface.family = "IPv4";
    
    struct ifreq ifr;
    strcpy(ifr.ifr_name, interfaces->ifa_name);
    int soc = socket(AF_INET, SOCK_DGRAM, 0);
    ioctl(soc, SIOCGIFHWADDR, &ifr);
    unsigned char *macptr = reinterpret_cast<unsigned char *>(ifr.ifr_hwaddr.sa_data);
    iface.mac = Utilities::format("%02x:%02x:%02x:%02x:%02x:%02x", *macptr, *(macptr+1), *(macptr+2), *(macptr+3), *(macptr+4), *(macptr+5));

    result[interfaces->ifa_name].push_back(iface);
  }
  
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

UiToolkit LinuxPlatform::getUiToolkit() const {
  std::map<std::string, std::string> env = Process::env();
  std::string currentDesktop = env["XDG_CURRENT_DESKTOP"];
  auto searchInVector = [](const std::vector<std::string> &v, const std::string &s) {
    return std::find_if(v.begin(), v.end(), [&](auto &str) {
              return str == s;
           }) != v.end();
  };

  std::transform(currentDesktop.begin(), currentDesktop.end(), currentDesktop.begin(), ::tolower);
  
  if (searchInVector(std::vector<std::string>({"kde", "lxqt"}), currentDesktop))
    return UiToolkit::Qt;

  if (searchInVector(std::vector<std::string>({"ubuntu:gnome", "unity", "gnome", "xfce", "lxde"}), currentDesktop))
    return UiToolkit::Gtk;
  
  return UiToolkit::Unknown;
}

//----------------------------------------------------------------------------------------------------------------------

std::string LinuxPlatform::getClipboardText() const {
  return Gtk::Clipboard::get()->wait_for_text();
}

//----------------------------------------------------------------------------------------------------------------------

void LinuxPlatform::setClipboardText(const std::string &text) const {
  Gtk::Clipboard::get()->set_text(text);
  // Because of MGA nature, we have to explicitly store()
  // the text so other apps won't hang waiting for mga to finish.
  Gtk::Clipboard::get()->store();
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<mga::Screen> LinuxPlatform::getScreens() const {
  std::vector<mga::Screen> result;
  ::Display *display = XOpenDisplay(NULL); // create a connection to X
  
  if (display == nullptr)
    throw std::runtime_error("There was a problem creating a X server connection");
  
  int count = 0;
  XineramaScreenInfo *info = XineramaQueryScreens(display, &count);
  
  if (info == nullptr)
    throw std::runtime_error("Failed to query screens");
  
  for (int index = 0; index < count; ++index) {
    double scalingFactor = 1.0; //  TODO: find out the right scaling factor from xlib
    geometry::Rectangle bounds = { static_cast<int>(info[index].x_org), 
                                   static_cast<int>(info[index].y_org), 
                                   static_cast<int>(info[index].width), 
                                   static_cast<int>(info[index].height) };
    mga::Screen entry = { bounds, static_cast<size_t>(info[index].width), 
                                  static_cast<size_t>(info[index].height), scalingFactor };
    result.push_back(entry);
  }
  
  XFree(info);
  XCloseDisplay(display); // disconnect this app from X server 

  return result;
}

//----------------------------------------------------------------------------------------------------------------------
