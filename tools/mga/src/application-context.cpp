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

#include "common.h"
#include "global.h"
#include "utilities.h"
#include "filesystem.h"
#include "path.h"
#include "process.h"
#include "context-management.h"
#include "db.h"
#include "accessible.h"
#include "application-context.h"

using namespace mga;

//----------------------------------------------------------------------------------------------------------------------

ApplicationContext& ApplicationContext::get()
{
  static ApplicationContext backend;
  return backend;
}

//----------------------------------------------------------------------------------------------------------------------

ApplicationContext::ApplicationContext() : _debugMode(false)
{
}

//----------------------------------------------------------------------------------------------------------------------

ApplicationContext::~ApplicationContext()
{
}

//----------------------------------------------------------------------------------------------------------------------

void ApplicationContext::showHelp() {
  std::cout << "mga [OPTION]... FILE" << std::endl << std::endl;
  std::cout << "FILE\t- path to json config file." << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "\t--debugger\t\t- enable debug mode" << std::endl;
  std::cout << "\t--wait\t\t\t- wait on exit" << std::endl << std::endl;
  std::cout << "\t--help\t\t\t- display help message and exit" << std::endl;
}

//----------------------------------------------------------------------------------------------------------------------

ExitCode ApplicationContext::initialize() {
  if (!aal::Accessible::accessibilitySetup()) {
    std::cerr << "Fatal: could not acquire permission for MGA to use accessibility for automation." << std::endl;
    return ExitCode::Other;
  }

  // By default we need to set the default minimal C locale.
  setlocale(LC_ALL, "C");
  std::locale::global(std::locale("C"));
  std::cout.imbue(std::locale("C"));
  std::cerr.imbue(std::locale("C"));
  
  // Keep the current directory, so we can restore it before exit.
  _currentDir = Process::cwd();

  return ExitCode::Success;
}

//----------------------------------------------------------------------------------------------------------------------

ExitCode ApplicationContext::parseParams(int argc, const char **argv, char **envp) {

  _debugMode = false;

  Process::initialize(argc, argv, envp);

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "--debugger") {
      _debugMode = true;
      continue;
    }
    else if (arg == "--wait") {
      Process::waitBeforeExit = true;
      continue;
    }
    else if (arg == "--help") {
      showHelp();
      return ExitCode::Success;
    }
    else if (Utilities::hasSuffix(arg, ".json")) {
      _configFile = arg;
      continue;
    }
    else {
      std::cerr << "Unknown argument: " << arg << std::endl;
      showHelp();
      return ExitCode::Other;
    }
  }

  if (_configFile.empty()) {
    // Try to load it from the user home directory.
#ifdef _MSC_VER
    std::string configLocation = getenv("HOMEPATH");
    configLocation.append("\\mga.json");
#else
    std::string configLocation = getenv("HOME");
    configLocation.append("/.mga.json");
#endif
    std::ofstream checkConfigFile(configLocation.c_str(), std::ios::in);
    if (checkConfigFile.good())
      _configFile = configLocation;
    else {
      std::cerr << "Error: missing path to json config file" << std::endl;
      return ExitCode::Other;
    }
  }
  return ExitCode::Success;
}

//----------------------------------------------------------------------------------------------------------------------

void ApplicationContext::run() {
  JSContext context(_debugMode);
  try {
    context.initialize(_configFile);
    context.runEventLoop();
  } catch (std::runtime_error &ex) {
    std::cerr << ex.what() << std::endl;
    Process::exitCode = ExitCode::Other;
  }
  
  try {
    context.onExit();
  }
  catch (...) {
  }
}

//----------------------------------------------------------------------------------------------------------------------

ExitCode ApplicationContext::shutDown() {
  // Go back to where we started from.
  Process::chdir(_currentDir);
  Db::threadCleanup();
  Process::exit();
  return Process::exitCode; // Won't be executed.
}

//----------------------------------------------------------------------------------------------------------------------
