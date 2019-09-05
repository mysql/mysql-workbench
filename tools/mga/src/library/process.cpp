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

#include "utilities.h"
#include "streams.h"

#include "platform.h"
#include "path.h"
#include "scripting-context.h"

#include "process.h"

using namespace mga;

//----------------- Process --------------------------------------------------------------------------------------------

static StringArrayRef arguments;
static std::map<std::string, std::string> environment;

ExitCode Process::exitCode = ExitCode::Success;
bool Process::waitBeforeExit = false;

void Process::initialize(int argc, const char* argv[], char *envp[]) {
  arguments = std::make_shared<StringArray>();
  for (int i = 0; i < argc; ++i)
    arguments->push_back(argv[i]);

  size_t i = 0;
  while (envp && envp[i] != nullptr) {
    auto parts = Utilities::split(envp[i++], "=");
    if (parts.size() > 1)
      environment[parts[0]] = parts[1];
  }

  Platform::get().initialize(argc, argv, envp);

}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Changes the current work directory.
 */
void Process::chdir(std::string const& path) {
  // TODO: should be moved to platform layer (and use Unicode variant on Windows).
#ifdef _MSC_VER
  int result = ::_wchdir(Utilities::s2ws(path).c_str());
#else
  int result = ::chdir(path.c_str());
#endif
  if (result)
    throw std::runtime_error("could not change directory (" + Utilities::getLastError() + ")");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the current work directory.
 */
std::string Process::cwd() {
#ifdef _MSC_VER
  wchar_t widePath[FILENAME_MAX + 1];
  ::_wgetcwd(widePath, FILENAME_MAX);

  return Path::normalize(Utilities::ws2s(widePath));

#else
  char currentPath[FILENAME_MAX + 1];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
  ::getcwd(currentPath, FILENAME_MAX);

  return currentPath;
#pragma GCC diagnostic pop
#endif
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the environment variables for this process.
 */
std::map<std::string, std::string> Process::env() {
  return environment;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Exists the current process with the given exit code (default is the stored exit code).
 */
void Process::exit(ExitCode code) {
  Platform::get().exit(code);
#ifndef _MSC_VER
  ::exit(static_cast<int>(code));
#endif // ! 
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Called when the current process is about to end. This call will trigger the "exit" event in JS.
 */
void Process::onExit(ScriptingContext &context) {
  context.emitEvent("process", "exit", { static_cast<int>(exitCode) });
}

//----------------------------------------------------------------------------------------------------------------------

void Process::activate(ScriptingContext &context, JSObject &exports) {
  // The events + streams modules must be activated already.
  exports.defineClass("Process", "EventEmitter", 0, [](JSObject *process, JSValues &args) {
    std::ignore = args;
    process->setBacking(new Process());
  }, [&](JSObject &prototype) {
    // Publish the entire env map as property on our module.
    prototype.defineProperty("env", environment);
    prototype.defineVirtualArrayProperty("argv", arguments, false);

    // execArgv and argv0 contain node specific values, which we do not support atm.
    prototype.defineProperty("execPath", (*arguments)[0]);

#ifdef _MSC_VER
    prototype.defineProperty("platform", "win32");
    prototype.defineProperty("EOL", "\r\n");
#elif __APPLE__
    prototype.defineProperty("platform", "macOS");
    prototype.defineProperty("EOL", "\n");
#else
    prototype.defineProperty("platform", "linux");
    prototype.defineProperty("EOL", "\n");
#endif

    JSObject values(&context);
    values.defineProperty("node", "8.12.0");
    values.defineProperty("unicode", "9.0.0");
    prototype.defineProperty("versions", values);

    prototype.defineProperty("title", "MySQL GUI Automator");
    prototype.defineProperty("version", "0.1.0");

    prototype.defineVirtualProperty("exitCode", [](ScriptingContext *, JSExport *, std::string const& name) {
      std::ignore = name;
      JSVariant result(static_cast<unsigned int>(Process::exitCode));
      return result;
    }, [](ScriptingContext *, JSExport *, std::string const& name, JSVariant value) {
      std::ignore = name;
      int t = value;
      Process::exitCode = static_cast<ExitCode>(t);
    });
    
    prototype.defineFunction( { "chdir" }, 1, [](JSExport *, JSValues &args) {
      std::string path = args.get(0);
      Process::chdir(path);
    });

    prototype.defineFunction( { "cwd", "pwd" }, 1, [](JSExport *, JSValues &args) {
      args.pushResult(Process::cwd());
    });

    prototype.defineFunction( { "exit" }, 1, [](JSExport *, JSValues &args) {
      if (args.size() > 0) { // The exit code is optional.
        int code = args.as(ValueType::Int, 0);
        exitCode = static_cast<ExitCode>(code);
      }

      onExit(*args.context());
      Process::exit(); // Never returns.
    });

  });

  // Create a singleton instance of the Process class and add stdout/stderr properties to it.
  // TODO: use TTY instead, as is done in Node.js.
  JSGlobalObject globals(&context);
  JSObject process = context.createJsInstance("Process", {});
  globals.defineProperty("process", process);

  // Create "Writable" instances for these properties.
  // We use a simplified approach here and only implement the _write method to print console output.
  JSObject writer = context.createJsInstance("Writable", {});
  writer.defineFunction({ "_write" }, 3, [](JSExport *, JSValues &args) {
    // parameters: chunk?, encoding?, callback?, chunk can be a string or a buffer.
    std::string chunk = args.get(0, "");

    // We ignore encoding and callback atm. UTF-8 is expected all the time and we don't write asynchronously.
    Platform::get().writeText(chunk, false);
  });

  writer.defineProperty("isTTY", true); // TODO: implement non-tty situations (e.g. when redirected to a stream).
  process.defineProperty("stdout", writer);

  writer = context.createJsInstance("Writable", {});
  writer.defineFunction({ "_write" }, 3, [](JSExport *, JSValues &args) {
    // parameters: chunk?, encoding?, callback?, chunk can be a string or a buffer.
    std::string chunk = args.get(0, "");
    Platform::get().writeText(chunk, true);
  });

  process.defineProperty("stderr", writer);
}

//----------------------------------------------------------------------------------------------------------------------

bool Process::_registered = []() {
  ScriptingContext::registerModule("process", &Process::activate);
  return true;
}();
