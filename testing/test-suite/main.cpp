/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "ansi-styles.h"

#include "string_utilities.h"
#include "file_utilities.h"
#include "data_types.h"
#include "wb_version.h"

#include "helpers.h"

#include "casmine.h"

using namespace casmine;

//----------------------------------------------------------------------------------------------------------------------

template<typename T>
std::string printWithDefault(T value, std::string const& type) {
  std::stringstream ss;
  ss << value;

  if (ss.str().empty())
    return "<<no value for " + type + ">>";
  return ss.str();
};

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, const char *argv[]) {

  std::cout << "Casmine testing MySQL Workbench " << printWithDefault(APP_MAJOR_NUMBER, "major version") << ".";
  std::cout << printWithDefault(APP_MINOR_NUMBER, "minor version") << ".";
  std::cout << printWithDefault(APP_RELEASE_NUMBER, "release number");
  std::cout << " (" << printWithDefault(APP_RELEASE_TYPE, "release type") << ", ";
  std::cout << printWithDefault(APP_LICENSE_TYPE, "license type") << ", ";
  std::cout << printWithDefault(APP_EDITION_NAME, "edition name") << ")" << std::endl;

  auto context = CasmineContext::get();
  dataTypes::OptionsList opts;
  std::string configFile;

  opts.addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentLogical,
    'h', "help", "Show recognized options",
    [&](const dataTypes::OptionEntry &entry, int *retval) {
      std::cout << opts.getHelp(argv[0]);
      *retval = 0;
      return false;
    })
  );

  opts.addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentText,
    "config", "Sets the config file",
    [&](const dataTypes::OptionEntry &entry, int *retval) {
        configFile = entry.value.textValue;
        return true;
      }, "<file>")
  );

  opts.addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentLogical,
    0, "continue-on-exception", "Casmine will continue test execution even after catching an exception",
    [&](const dataTypes::OptionEntry &entry, int *retval) {
      context->settings["continueOnException"] = entry.value.logicalValue;
      return true;
    }));

  opts.addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentLogical,
    0, "verbose", "Casmine will be printing status messages",
    [&](const dataTypes::OptionEntry &entry, int *retval) {
      context->settings["verbose"] = entry.value.logicalValue;
      return true;
    })
  );

  opts.addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentLogical,
    0, "no-colors", "Do not use terminal colors, even if supported",
    [&](const dataTypes::OptionEntry &entry, int *retval) {
      context->settings["no-colors"] = entry.value.logicalValue;
      return true;
    })
  );

  opts.addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentLogical,
    0, "list-specs", "List specs that can be selectivly launched",
    [&](const dataTypes::OptionEntry &entry, int *retval) {
      std::cout << "Spec list: "<<std::endl;
      for(const auto &it: CasmineContext::get()->getSpecList()) {
        std::cout << "\t" << it << std::endl;
      }
      *retval = 0;
      return false;
    })
  );

  opts.addEntry(dataTypes::OptionEntry(dataTypes::OptionArgumentLogical,
    'o', "only", "Run only specs specified as the last arguments, use 'list-specs' to see available specs.",
    [&](const dataTypes::OptionEntry &entry, int *retval) {
      context->settings["only-selected"] = entry.value.logicalValue;
      return true;
    })
  );

  // Command line parameter parsing.
  int rc = 0;
  try {
    if (!opts.parse(std::vector<std::string>(argv + 1, argv + argc), rc)) {
      return rc;
    }
  } catch (std::runtime_error &re) {
    std::cerr << re.what() << std::endl;
    return 1;
  }

  if (std::get<bool>(CasmineContext::get()->settings["only-selected"])) {
    if (opts.pathArgs.empty()) {
      std::cerr << "Option 'only' requires that you pass at least one spec." << std::endl;
      return 1;
    }
    // We need to check if the spec name that has been passed, is a valid one
    std::vector<std::string> diff;
    auto tmpList = CasmineContext::get()->getSpecList();
    std::set<std::string> args(opts.pathArgs.begin(), opts.pathArgs.end());
    std::set<std::string> list(tmpList.begin(), tmpList.end());
    std::set_difference(args.begin(), args.end(), list.begin(), list.end(), std::inserter(diff, diff.begin()));

    if (!diff.empty()) {
      std::cout << "The following specs could not be found:" << std::endl;
      for (const auto &it: diff) {
        std::cout << "\t '" << it << "'" << std::endl;
      }
      return 1;
    }
    CasmineContext::get()->forceSpecList(opts.pathArgs);

  }

  // Determine and load the configuration file. The search order is:
  // - application parameter
  // - CASMINE_CONFIG_FILE env var
  // - local dir
  // - home dir
  std::ifstream configStream;
  auto openConfig = [&](std::string const& name, bool failAllowed) -> bool {
    if (configStream.is_open())
      return true;

    auto configFile = expandPath(name);
    if (!configFile.empty()) {
      configStream.open(configFile);
      if (!configStream.good() && !failAllowed) {
        std::cerr << "Error while opening the configuration file " << configFile << ": " << strerror(errno) << std::endl;
        return false;
      }
    }

    return true;
  };

  if (!openConfig(configFile, false))
    return 1;

  if (!openConfig(casmine::getEnvVar("CASMINE_CONFIG_FILE"), false))
      return 1;

  if (!openConfig("casmine-config.json", true))
    return 1;

#ifdef _MSC_VER
  if (!openConfig("%HOMEDRIVE%/%HOMEPATH%/casmine-config.json", true))
    return 1;
#else
  if (!openConfig("$HOME/casmine-config.json", true))
    return 1;
#endif

  if (configStream.good()) {
    rapidjson::IStreamWrapper streamWrapper(configStream);

    rapidjson::ParseResult parseResult = context->configuration.ParseStream(streamWrapper);
    if (parseResult.IsError()) {
      const RAPIDJSON_ERROR_CHARTYPE *message = GetParseError_En(parseResult.Code());
      std::cerr << "Error while parsing the configuration file: " << message << std::endl;
      return 1;
    }

    if (!context->configuration.IsObject()) {
      std::cerr << "The configuration file has not the correct format" << std::endl;
      return 1;
    }
  } else {
    std::cerr << std::endl;
    std::cerr << "Error: no configuration was specified and no file was found either in the home or the current folder";
    std:: cerr << std::endl;
    return 1;
  }

  if (CasmineContext::get()->settings.count("no-colors") > 0
      && !std::get<bool>(CasmineContext::get()->settings["no-colors"]))
    initAnsiStyles();

#ifdef _MSC_VER
  SetConsoleOutputCP(CP_UTF8);
#endif

  context->runTests(base::cwd());

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------
