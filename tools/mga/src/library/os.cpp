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

#include "scripting-context.h"
#include "filesystem.h"
#include "platform.h"

#include "os.h"

using namespace mga;

//----------------------------------------------------------------------------------------------------------------------

#ifdef _MSC_VER

static inline void cpuid(uint32_t selector, int *data) {
  __cpuid(data, selector);
}

#else

static inline void cpuid(uint32_t selector, int *data) {
  asm("cpuid": "=a" (data[0]), "=b" (data[1]), "=c" (data[2]), "=d" (data[3])
      : "a"(selector), "b" (0), "c" (0), "d" (0));
}

#endif

#define EAX 0
#define EBX 1
#define ECX 2
#define EDX 3

//----------------------------------------------------------------------------------------------------------------------

void OS::activate(ScriptingContext &context, JSObject &exports) {
  exports.defineFunction({ "arch" }, 0, [](JSExport *, JSValues &args) {
#ifdef _MSC_VER
  #ifdef _WIN64
    args.pushResult("x64");
  #else
    args.pushResult("x86");
  #endif
#elif __x86_64__ || __ppc64__
    args.pushResult("x64");
#else
    args.pushResult("x86");
#endif
  });

  exports.defineFunction({ "cpus" }, 0, [](JSExport *, JSValues &args) {
    int registers[4];

    cpuid(0, registers);

    // Determine vendor.
    std::string vendor(reinterpret_cast<char *>(&registers[EBX]), 4);
    vendor += std::string(reinterpret_cast<char *>(&registers[EDX]), 4);
    vendor += std::string(reinterpret_cast<char *>(&registers[ECX]), 4);

    cpuid(0x80000000, registers);
    uint32_t maxExtension = static_cast<uint32_t>(registers[EAX]);

    // Determine brand.
    std::string brand;
    if (maxExtension >= 0x80000004) {
      char temp[48]; // Max brand length is 48 (and implicitly null terminated).

      cpuid(0x80000002, registers);
      memcpy(temp, reinterpret_cast<char *>(&registers[EAX]), 16);
      cpuid(0x80000003, registers);
      memcpy(&temp[16], reinterpret_cast<char *>(&registers[EAX]), 16);
      cpuid(0x80000004, registers);
      memcpy(&temp[32], reinterpret_cast<char *>(&registers[EAX]), 16);

      brand = temp;
    }
    if (brand.empty())
      brand = vendor;

    JSArray cpus(args.context());
    for (auto &cpu : Platform::get().cpuInfo()) {
      JSObject entry(args.context());
      if (!cpu.model.empty())
        entry.defineProperty("model", cpu.model);
      else
        entry.defineProperty("model", brand);

      entry.defineProperty("speed", cpu.speed);

      JSObject times(args.context());
      times.defineProperty("user", static_cast<unsigned>(cpu.user));
      times.defineProperty("nice", static_cast<unsigned>(cpu.nice), "nice");
      times.defineProperty("sys", static_cast<unsigned>(cpu.sys), "sys");
      times.defineProperty("idle", static_cast<unsigned>(cpu.idle), "idle");
      times.defineProperty("irq", static_cast<unsigned>(cpu.irq), "irq");

      entry.defineProperty("times", times);
      cpus.addValue(entry);
    }

    args.pushResult(cpus);
  });

  exports.defineFunction({ "endianness" }, 0, [](JSExport *, JSValues &args) {
    union {
      uint16_t value = 0xAB00;
      uint8_t bytes[2];
    } value;

    if (value.bytes[1] == 0xAB)
      args.pushResult("LE");
    else
      args.pushResult("BE");
  });

  exports.defineFunction({ "freemem" }, 0, [](JSExport *, JSValues &args) {
    args.pushResult(static_cast<double>(Platform::get().getFreeMem()));
  });

  exports.defineFunction({ "homedir" }, 0, [](JSExport *, JSValues &args) {
    args.pushResult(Platform::get().getHomeDir());
  });

  exports.defineFunction({ "hostname" }, 0, [](JSExport *, JSValues &args) {
    args.pushResult(Platform::get().getHostName());
  });

  exports.defineFunction({ "loadavg" }, 0, [](JSExport *, JSValues &args) {
    double avg[3] = { 0, 0, 0 };
    Platform::get().loadAvg(avg);

    JSArray result(args.context());
    for (size_t i = 0; i < 3; ++i) {
      result.addValue(avg[i]);
    }
    args.pushResult(result);
  });

  exports.defineFunction({ "networkInterfaces" }, 0, [](JSExport *, JSValues &args) {
    JSObject nics(args.context());

    for (auto &entry : Platform::get().networkInterfaces()) {
      JSArray nicList(args.context());
      nics.defineProperty(entry.first, nicList);

      for (auto &nic : entry.second) {
        JSObject jsNic(args.context());

        jsNic.defineProperty("address", nic.address);
        jsNic.defineProperty("netmask", nic.netmask);
        jsNic.defineProperty("family", nic.family);
        jsNic.defineProperty("mac", nic.mac);
        jsNic.defineProperty("internal", nic.internal);
        jsNic.defineProperty("scopeid", nic.scopeid);

        nicList.addValue(jsNic);
      }
    }

    args.pushResult(nics);
  });

  exports.defineFunction({ "platform" }, 0, [](JSExport *, JSValues &args) {
#ifdef _MSC_VER
    args.pushResult("win32");
#elif __APPLE__
    args.pushResult("darwin");
#else
    args.pushResult("linux");
#endif
  });

  exports.defineFunction({ "release" }, 0, [](JSExport *, JSValues &args) {
    args.pushResult(Platform::get().getRelease());
  });

  // This is not in node.js. An addition to return the OS version.
  exports.defineFunction({ "version" }, 0, [](JSExport *, JSValues &args) {
    Version version = Platform::get().getVersion();
    JSObject result(args.context());
    result.defineProperty("major", version.major);
    result.defineProperty("minor", version.minor);
    result.defineProperty("path", version.patch);
    args.pushResult(result);
  });

#if defined(__linux) || defined(__linux__) || defined(linux)
  exports.defineFunction({ "distro" }, 0, [](JSExport *, JSValues &args) {
    args.pushResult(Platform::get().getDistroName());
  });
#endif

  exports.defineFunction({ "tmpdir" }, 0, [](JSExport *, JSValues &args) {
    args.pushResult(Platform::get().getTempDirName());
  });

  exports.defineFunction({ "totalmem" }, 0, [](JSExport *, JSValues &args) {
    args.pushResult(static_cast<double>(Platform::get().getTotalMem()));
  });

  exports.defineFunction({ "type" }, 0, [](JSExport *, JSValues &args) {
#ifdef _MSC_VER
    args.pushResult("Windows_NT");
#elif __APPLE__
    args.pushResult("Darwin");
#else
    args.pushResult("Linux");
#endif
  });

  exports.defineFunction({ "uptime" }, 0, [](JSExport *, JSValues &args) {
    args.pushResult(Platform::get().getUptime());
  });

  exports.defineFunction({ "userInfo" }, 0, [](JSExport *, JSValues &args) {
    JSObject result(args.context());
    UserInfo info = Platform::get().getCurrentUserInfo();

    result.defineProperty("username", info.userName);
    result.defineProperty("uid", info.uid);
    result.defineProperty("gid", info.gid);
    result.defineProperty("shell", info.shell);
    result.defineProperty("homedir", info.homeDir);

    args.pushResult(result);
  });

  exports.defineFunction({  "getPidByName" }, 1, [](JSExport *, JSValues &args) {
    if (args.is(ValueType::String, 0)) {
      args.pushResult(Platform::get().getPidByName(args.get(0)));
    } else {
      args.context()->throwScriptingError(ScriptingError::Error, "Unhandled argument type");
    }
  });

  exports.defineFunction({ "terminate" }, 2, [](JSExport *, JSValues &args) {
    if (args.is(ValueType::Int, 0) && args.is(ValueType::Boolean, 1)) {
      args.pushResult(Platform::get().terminate(args.get(0), args.get(1)));
    } else {
      args.context()->throwScriptingError(ScriptingError::Error, "Unhandled argument type");
    }
  });

  exports.defineFunction({ "launchApplication" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    size_t applicationArgCount = args.size() - 2;
    std::vector<std::string> params;

    if (args.is(ValueType::String, 0)) {
       std::string name = args.get(0);

       bool newInstance = false;
       ShowState showState = ShowState::Normal;
       std::map<std::string, std::string> envVars;
       if (args.is(ValueType::Object, 1)) {
         JSObject options = args.get(1);

         int value = options.get("showState", static_cast<int>(ShowState::Normal));
         showState = static_cast<ShowState>(value);
         newInstance = options.get("newInstance", false);

         JSObject envp = options.get("env", JSObject());
         if (envp.isValid()) {
           auto keys = envp.getPropertyKeys();
           for(auto it: keys) {
             envVars[it] = (std::string)envp.get(it);
           }
         }
      }

      if (applicationArgCount > 0) {
        for (size_t i = 0; i < applicationArgCount; ++i) {
          params.push_back(args.get(i + 2));
        }
      }

      args.pushResult(Platform::get().launchApplication(name, params, newInstance, showState, envVars));

    } else {
      args.context()->throwScriptingError(ScriptingError::Error, "Unhandled argument type");
    }
  });

#ifdef _MSC_VER
  exports.defineProperty("EOL", "\r\n");
#else
  exports.defineProperty("EOL", "\n");
#endif

  JSObject constants(&context);
  Platform::get().defineOsConstants(context, constants);
  exports.defineProperty("constants", constants);
}

//----------------------------------------------------------------------------------------------------------------------

bool OS::_registered = []() {
  ScriptingContext::registerModule("os", &OS::activate);
  return true;
}();
