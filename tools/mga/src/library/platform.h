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

#pragma once

#include "common.h"
#include "context-management.h"
#include "process.h"

namespace mga {

  struct Cpu {
    std::string model; // If a platform can provide individual CPU models.
    size_t speed;

    // Times.
    size_t user = 0;
    size_t nice = 0;
    size_t sys = 0;
    size_t idle = 0;
    size_t irq = 0;
  };

  struct NetworkInterface {
    std::string address;
    std::string netmask;
    std::string family;
    std::string mac;
    bool internal = true;
    size_t scopeid = 0;
  };

  struct UserInfo {
    std::string userName;
    int uid;
    int gid;
    std::string shell;
    std::string homeDir;
  };

  struct Version {
    size_t major;
    size_t minor;
    size_t patch;
  };

  enum class UiToolkit {
    Unknown,
    DotNet,
    Cocoa,
    Gtk,
    Qt
  };

  class ScriptingContext;

  struct Stat {
    enum class TimeSpecs {
      atime,
      ctime,
      mtime,
      birthdate
    };
    
    struct internalTimeSpec {
      time_t secs;
      time_t nsecs;
    };
    
    int getDev() const { return _buffer.st_dev; }
    size_t getIno() const { return _buffer.st_ino; }
    uint16_t getMode() const { return _buffer.st_mode; }
    int getNLink() const { return _buffer.st_nlink; }
    size_t getUid() const { return _buffer.st_uid; }
    size_t getGid() const { return _buffer.st_gid; }
    int getRdev() const { return _buffer.st_rdev; }
    ssize_t getSize() const { return _buffer.st_size; }
    int getBlockkSize() const { return _blockSize; }
    ssize_t getBlocks() const { return _blocks; }
    
    double getTimeMS(TimeSpecs spec) const;
    std::string getTimeString(TimeSpecs spec) const;
    
    static std::unique_ptr<Stat> get(const std::string &path, bool followLinks);

    virtual ~Stat() {};
    
  protected:
#ifdef _MSC_VER
    struct _stat _buffer;
#else
    struct stat _buffer;
#endif
    ssize_t _blocks = 0;
    int _blockSize = 0;
    std::string _path;

    virtual void initialize(const std::string &path, bool followLinks) = 0;
    virtual timespec getTimeSpec(TimeSpecs spec) const = 0;
  };

  struct Screen {
    geometry::Rectangle bounds;
    size_t resolutionX;
    size_t resolutionY;
    double scaleFactor;
  };

  class Platform {
  public:
    static Platform& get();
    virtual ~Platform() {};

    virtual int launchApplication(std::string const& name, std::vector<std::string> const& params,  bool newInstance,
                                  ShowState showState, std::map<std::string, std::string> const& env = {}) const = 0;

    virtual int getPidByName(const std::string &name) const = 0;
    virtual std::string getTempDirName() const = 0;
    virtual bool terminate(int processID, bool force = false) const = 0;
    virtual void initialize(int, const char *[], char *[]) const {};
    virtual void exit(ExitCode) const {};
    virtual void runLoopRun(ScriptingContext &context) const;
    virtual bool isRunning(int processID) const { return false; };

    virtual void writeText(std::string const& text, bool error = false) const;
    virtual void createFolder(std::string const& name) const;
    virtual void removeFolder(std::string const& name) const;
    virtual void removeFile(std::string const& name) const;

    virtual void defineOsConstants(ScriptingContext &context, JSObject &constants) const;
    virtual void defineFsConstants(ScriptingContext &context, JSObject &constants) const;
    virtual geometry::Size getImageResolution(std::string const& path) const = 0;

    virtual std::vector<Cpu> cpuInfo() const = 0;
    virtual size_t getFreeMem() const = 0;
    virtual size_t getTotalMem() const = 0;
    virtual std::string getHomeDir() const;
    virtual std::string getHostName() const;
    virtual std::map<std::string, std::vector<NetworkInterface>> networkInterfaces() const = 0;
    virtual std::string getRelease() const;
    virtual Version getVersion() const = 0;
#if defined(__linux) || defined(__linux__) || defined(linux)
    virtual std::string getDistroName() const = 0;
#endif
    virtual double getUptime() const = 0;
    virtual UserInfo getCurrentUserInfo() const;
    virtual void loadAvg(double (&avg)[3]) const = 0;
    
    virtual UiToolkit getUiToolkit() const = 0;

    virtual std::string getClipboardText() const = 0;
    virtual void setClipboardText(const std::string &text) const = 0;
    virtual std::vector<Screen> getScreens() const = 0;
  };
}
