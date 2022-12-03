/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "reporter.h"
#include "helpers.h"

#include "casmine.h"
#include "ansi-styles.h"

#include <iomanip>

using namespace casmine;
using namespace std::chrono;

using namespace std::string_literals;

#ifdef DISABLE_SPECIAL_CHARACTERS
  #define DISABLED_TEST "- "
  #define PENDING_TEST "* "
  #define FAILED_TEST "x"
  #define SUCCESSFUL_TEST "o"
#else
  #define DISABLED_TEST "⊘ "
  #define PENDING_TEST "* "
  #define FAILED_TEST "x"
  #define SUCCESSFUL_TEST "√"
#endif

#ifdef _MSC_VER
#undef GetObject
#endif

//----------------------------------------------------------------------------------------------------------------------

/**
 * Prints the given time in the smallest form that is meaningfull.
 */
std::string formatTime(microseconds time) {
  std::ostringstream o;
  o << std::fixed;
  o.precision(2);

  if (time < 1ms)
    o << time.count() << "µs";
  else if (time < 1s)
    o <<  time.count() / 1e3 << "ms";
  else if (time < 1min)
    o << time.count() / 1e6 << "s";
  else if (time < 1h) {
    o << time / 1min << "min ";
    time %= 1min;
    o << time / 1s << "s";
  } else {
    o << time / 1h << "h ";
    time %= 1h;
    o << time / 1min << "min";
  }

  return o.str();
}

//----------------- Reporter -------------------------------------------------------------------------------------------

Reporter::Reporter() {
}

//----------------------------------------------------------------------------------------------------------------------

void Reporter::casmineStarted(size_t totalSpecsDefined) {
  baseDir = CasmineContext::get()->baseDir();
  totalSpecCount = totalSpecsDefined;
};

//----------------------------------------------------------------------------------------------------------------------

void Reporter::moduleStarted(TestResult const& result, size_t specsDefined) {
  ++totalModuleCount;
  currentSpecsDefined = specsDefined;
  currentModuleFile = result.file;

  currentSpecCount = 0;
  currentSucceededSpecCount = 0;
  currentPendingSpecCount = 0;
  currentDisabledSpecCount = 0;
  currentExcludedSpecCount = 0;
};

//----------------------------------------------------------------------------------------------------------------------

void Reporter::specStarted(TestResult const& result) {
  ++currentSpecCount;
  currentSpecFile = result.file;
};

//----------------------------------------------------------------------------------------------------------------------

void Reporter::specDone(TestResult const& result) {
  totalCheckCount += result.total;
  totalSucceededCheckCount += result.succeeded;

  if (result.runType == ItemRuntimeSpec::Excluded) {
    ++totalExcludedSpecCount;
    ++currentExcludedSpecCount;
    return;
  }

  switch (result.runType) {
    case ItemRuntimeSpec::Disabled: {
      ++totalDisabledSpecCount;
      ++currentDisabledSpecCount;
      break;
    }

    default: {
      if (result.pending) {
        ++totalPendingSpecCount;
        ++currentPendingSpecCount;
      } else {
        bool failed = result.succeeded != result.total || !result.failures.empty();
        if (!failed) {
          ++totalSucceededSpecCount;
          ++currentSucceededSpecCount;
        } else {
          ++totalFailedSpecCount;
        }
      }

      break;
    }
  }
};

//----------------------------------------------------------------------------------------------------------------------

void Reporter::moduleDone(TestResult const& result) {
  if (result.pending) {
    ++modulesPending;
    return;
  }

  switch (result.runType) {
    case ItemRuntimeSpec::Excluded: {
      ++modulesExcluded;
      totalExcludedSpecCount += currentSpecsDefined;
      break;
    }

    case ItemRuntimeSpec::Disabled: {
      ++modulesDisabled;
      totalDisabledSpecCount += currentSpecsDefined;
      break;
    }

    default: {
      totalCheckCount += result.total; // Checks in before/after all/each.
      totalSucceededCheckCount += result.succeeded;

      bool failed = (currentSpecCount - currentPendingSpecCount - currentDisabledSpecCount - currentSucceededSpecCount
                     - currentExcludedSpecCount) != 0;
      failed = failed || !result.failures.empty();
      if (failed)
        ++modulesFailed;
      else
        ++modulesSucceeded;

      break;
    }
  }
};

//----------------------------------------------------------------------------------------------------------------------

void Reporter::casmineDone(microseconds duration) {
};

//----------------- ConsoleReporter ------------------------------------------------------------------------------------

void ConsoleReporter::casmineStarted(size_t totalSpecsDefined) {
  auto &configuration = CasmineContext::get()->configuration;
  if (configuration.GetObject().HasMember("reporting")) {
    auto &reporterConfig = configuration.GetObject()["reporting"];
    if (reporterConfig.HasMember("printDetailTimes"))
      _printDetailTimes = reporterConfig["printDetailTimes"].GetBool();
  }

  Reporter::casmineStarted(totalSpecsDefined);

  std::cout << clearScreen << styleBoldOn << styleUnderlineOn << "Running Workbench Unit and Integration Tests"
    << stylesReset << std::endl << std::flush;
};

//----------------------------------------------------------------------------------------------------------------------

void ConsoleReporter::moduleStarted(TestResult const& result, size_t specsDefined) {
  Reporter::moduleStarted(result, specsDefined);

  std::stringstream ss;
  ss << std::endl << styleBoldOn << result.name << styleBoldOff << " (";
  ss << relativePath(baseDir, result.file) << "):";
  _moduleIntro = ss.str();
};

//----------------------------------------------------------------------------------------------------------------------

void ConsoleReporter::specStarted(TestResult const& result) {
  Reporter::specStarted(result);

  // This method is only called if the module is not disabled/excluded. Hence we can now print the module intro.
  if (!_moduleIntro.empty()) {
    std::cout << _moduleIntro << std::flush;
    _moduleIntro.clear();
  }
};

//----------------------------------------------------------------------------------------------------------------------

void ConsoleReporter::specDone(TestResult const& result) {
  Reporter::specDone(result);

  if (result.runType == ItemRuntimeSpec::Excluded)
    return;

  std::string indent = std::string(2, ' ');
  std::cout << std::endl << indent;

  if (result.runType == ItemRuntimeSpec::Disabled) {
      std::cout << styleBlueOn << DISABLED_TEST << styleBlueOff << result.name << styleBlueOn << " (disabled)" << styleBlueOff;
  } else {
    if (result.pending) {
      std::cout << styleBlueOn << PENDING_TEST << styleBlueOff << styleDimOn << result.name << styleDimOff;
      std::cout << " (suspended because " << result.pendingReason << ")";
    } else {
      bool failed = result.succeeded != result.total || !result.failures.empty();
      if (!failed)
        std::cout << styleGreenOn << SUCCESSFUL_TEST << styleGreenOff;
      else
        std::cout << styleBoldOn << styleRedOn << FAILED_TEST << styleRedOff << styleBoldOff;

      std::cout << " " << styleDimOn << result.name << styleDimOff;
      if (result.total == 0) {
        std::cout << styleBoldOn << styleYellowOn << " (" << result.succeeded << " of " << result.total;
        std::cout << " suceeded)" << stylesReset;
      } else if (failed) {
        std::cout << styleBoldOn << styleRedOn << " (" << result.succeeded << " of " << result.total;
        std::cout << " suceeded)" << stylesReset;
      } else
        std::cout << " (" << result.succeeded << " of " << result.total << " suceeded)";

      if (_printDetailTimes)
        std::cout << ": " << styleCyanOn << formatTime(result.duration) << styleCyanOff;
    }
  }

  if (!result.failures.empty()) {
    std::cout << std::endl;
    for (auto &entry : result.failures) {
      std::cout << std::endl;

      std::cout << indent << styleRedOn << "File: " << relativePath(baseDir, entry.file) << std::endl;
      std::cout << indent << styleRedOn << "Error at line " << entry.line << ": ";

      std::vector<std::string> parts = splitBySet(entry.message, "\n");
      if (parts.size() == 1)
        std::cout << styleRedOn << parts[0] << std::endl;
      else {
        std::cout << std::endl;
        for (auto &part : parts)
          std::cout << styleRedOn << indent << indent << part << std::endl;
      }
    }
  }

  std::cout << stylesReset << std::flush;
};

//----------------------------------------------------------------------------------------------------------------------

void ConsoleReporter::moduleDone( TestResult const& result) {
  Reporter::moduleDone(result);

  if (result.runType == ItemRuntimeSpec::Excluded)
    return;

  // If no spec was executed (disabled or exception in before/after) then we have no module intro printed yet.
  if (!_moduleIntro.empty()) {
    std::cout << _moduleIntro;
    _moduleIntro.clear();
  }

  size_t failedCount = currentSpecCount - currentPendingSpecCount - currentDisabledSpecCount -
    currentSucceededSpecCount - currentExcludedSpecCount;
  bool failed = (failedCount != 0) || !result.failures.empty();
  if (currentSpecCount > 0 || (!result.failures.empty() && currentSpecCount == 0))
    std::cout << std::endl;

  if (result.pending) {
    std::cout << std::endl << styleBoldOn << "Test case " << styleBoldOff;
    std::cout << styleBlueOn << "suspended because " << result.pendingReason << styleBlueOff << std::endl;
  } else {
    switch (result.runType) {
      case ItemRuntimeSpec::Normal:
      case ItemRuntimeSpec::Focused:
      case ItemRuntimeSpec::Forced:{
        std::cout << styleBoldOn << "Test case " << styleBoldOff;
        if (!failed)
          std::cout << styleGreenOn << "successfully completed" << styleGreenOff;
        else
          std::cout << styleBoldOn << styleRedOn << "failed" << styleRedOff << styleBoldOff;

        std::cout << ": " << styleCyanOn << formatTime(result.duration) << styleCyanOff;
        std::cout << std::endl;
        break;
      }

      case ItemRuntimeSpec::Disabled:
        std::cout << styleBlueOn << " disabled" << styleBlueOff << std::endl;
        break;
      case ItemRuntimeSpec::Excluded: { // Handled above;
        break;
      }
    }
  }

  if (!result.failures.empty()) {
    std::cout << std::endl;
    for (auto &entry : result.failures) {
      if (currentModuleFile != entry.file)
        std::cout << styleRedOn << "File: " << relativePath(baseDir, entry.file) << std::endl;

      std::cout << styleRedOn << "Error at line " << entry.line << ": ";

      std::vector<std::string> parts = splitBySet(entry.message, "\n");
      for (auto &part : parts)
        std::cout << styleRedOn << part << std::endl;
    }
  }

  std::cout << stylesReset << std::flush;
};

//----------------------------------------------------------------------------------------------------------------------

void ConsoleReporter::casmineDone(microseconds duration) {
  std::cout << std::endl << "Finished tests after: " << styleCyanOn << formatTime(duration) << styleCyanOff;
  std::cout << std::endl << std::endl;

  size_t modulesExecuted = modulesSucceeded + modulesFailed;
  std::cout << styleBoldOn << modulesExecuted << " of " << totalModuleCount << styleBoldOff << " test cases executed";
  std::cout << " (" << modulesSucceeded << " succeeded, " << modulesFailed << " failed, ";
  std::cout << modulesPending << " pending, " << modulesDisabled << " disabled, ";
  std::cout << modulesExcluded << " excluded)" << std::endl;

  size_t specCount = totalFailedSpecCount + totalSucceededSpecCount;
  std::cout << styleBoldOn << specCount << " of " << totalSpecCount << styleBoldOff << " tests executed";

  std::cout << " (" << totalSucceededSpecCount << " succeeded, " << totalFailedSpecCount << " failed, ";
  std::cout << totalPendingSpecCount << " pending, " << totalDisabledSpecCount << " disabled, ";
  std::cout << totalExcludedSpecCount << " excluded)" << std::endl;

  std::cout << styleBoldOn << totalCheckCount << styleBoldOff << " checks executed";
  size_t failedChecks = totalCheckCount - totalSucceededCheckCount;
  if (failedChecks == 0)
    std::cout << styleGreenOn << " (" << failedChecks << " failed, 0%)" << stylesReset;
  else
    std::cout << styleRedOn << styleBoldOn << " (" << failedChecks << " failed, " << std::setprecision(2)
      << 100.0 * failedChecks / totalCheckCount << "%)" << stylesReset;
  std::cout << std::endl << std::endl;
};

//----------------- JsonReporter ---------------------------------------------------------------------------------------

static void removeLastComma(std::string &json) {
  if (json.empty())
    return;

  if (json.rfind(",\n") == json.size() - 2)
    json.erase(json.end() - 2);
  else if (json[json.size() - 1] == ',')
    json.erase(json.end() - 1);
}

//----------------------------------------------------------------------------------------------------------------------

static std::string escape(std::string const& s) {
  std::string result;
  result.reserve(s.size());
  for (auto ch : s) {
    char escape = 0;
    switch (ch) {
      case '"':
        escape = '"';
        break;
      case '\\':
        escape = '\\';
        break;
      case '\b':
        escape = 'b';
        break;
      case '\f':
        escape = 'f';
        break;
      case '\n':
        escape = 'n';
        break;
      case '\r':
        escape = 'r';
        break;
      case '\t':
        escape = 't';
        break;
      default:
        break;
    }

    if (escape) {
      result.push_back('\\');
      result.push_back(escape);
    } else
      result.push_back(ch);
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void JsonReporter::casmineStarted(size_t totalSpecsDefined) {
  Reporter::casmineStarted(totalSpecsDefined);

  _json = "{\n\t\"description\": \"casmine testing result file\",\n";
  _moduleJson = "\t\"test cases\": {\n";
}

//----------------------------------------------------------------------------------------------------------------------

void JsonReporter::moduleStarted(TestResult const& result, size_t specsDefined) {
  Reporter::moduleStarted(result, specsDefined);

  _moduleJson += "\t\t\"" + escape(result.name) + "\": {\n";
  _moduleJson += "\t\t\t\"file\": \"" + relativePath(baseDir, currentModuleFile) + "\",\n";
  _specJson = "";
}

//----------------------------------------------------------------------------------------------------------------------

void JsonReporter::specStarted(TestResult const& result) {
  Reporter::specStarted(result);

  _specJson += "\t\t\t\t\"" + escape(result.name) + "\": {\n";
}

//----------------------------------------------------------------------------------------------------------------------

void JsonReporter::specDone(TestResult const& result) {
  Reporter::specDone(result);

  static std::string detailIndent = "\t\t\t\t\t";
  static std::string errorIndent = "\t\t\t\t\t\t";

  if (currentModuleFile != result.file)
    _specJson += detailIndent + "\"file\": \"" + escape(relativePath(baseDir, result.file)) + "\",\n";
  _specJson += detailIndent + "\"status\": ";

  if (result.pending) {
    _specJson += "\"pending\",\n" + detailIndent + "\"reason\": \"" +
      escape(result.pendingReason) + "\"\n";
  } else {
    switch (result.runType) {
      case ItemRuntimeSpec::Excluded:
        _specJson += "\"excluded\",\n";
        break;

      case ItemRuntimeSpec::Disabled:
        _specJson += "\"disabled\",\n";
        break;

      default: {
        bool failed = result.succeeded != result.total || !result.failures.empty();
        if (failed)
          _specJson += "\"failed\",\n";
        else
          _specJson += "\"succeeded\",\n";

        _specJson += detailIndent + "\"checks executed\": " + std::to_string(result.total) + ",\n";
        _specJson += detailIndent + "\"checks succeeded\": " + std::to_string(result.succeeded) + ",\n";
        break;
      }
    }
  }

  if (!result.failures.empty()) {
    _specJson += detailIndent + "\"errors\": [\n";

    for (auto &entry : result.failures) {
      _specJson += errorIndent + "{\n";
      if (entry.file != currentSpecFile)
        _specJson += errorIndent + "\t\"file\": \"" + escape(relativePath(baseDir, entry.file)) + "\",\n";
      _specJson += errorIndent + "\t\"line\": " + std::to_string(entry.line) + ",\n";
      _specJson += errorIndent + "\t\"message\": \"" + escape(entry.message) + "\"\n";
      _specJson += errorIndent + "},\n";
    }

    removeLastComma(_specJson);
    _specJson += detailIndent + "]\n";
  }

  removeLastComma(_specJson);
  _specJson += "\t\t\t\t},\n";
}

//----------------------------------------------------------------------------------------------------------------------

void JsonReporter::moduleDone(TestResult const& result) {
  Reporter::moduleDone(result);

  _moduleJson += "\t\t\t\"status\": ";
  if (result.pending) {
    _moduleJson += "\"suspended\",\n\t\t\t\"reason\": \"" + result.pendingReason + "\",\n";
  } else {
    switch (result.runType) {
      case ItemRuntimeSpec::Normal:
      case ItemRuntimeSpec::Focused:
      case ItemRuntimeSpec::Forced: {
        size_t failedSpecCount = currentSpecCount - currentPendingSpecCount - currentDisabledSpecCount - currentSucceededSpecCount;
        bool failed = failedSpecCount > 0 || !result.failures.empty();
        _moduleJson += (failed ? "\"failed\"" : "\"succeeded\"") + ",\n"s;
        _moduleJson += "\t\t\t\"tests executed\": " + std::to_string(currentSpecCount) + ",\n";
        _moduleJson += "\t\t\t\"tests succeeded\": " + std::to_string(currentSucceededSpecCount) + ",\n";
        _moduleJson += "\t\t\t\"tests failed\": " + std::to_string(failedSpecCount) + ",\n";
        _moduleJson += "\t\t\t\"tests pending\": " + std::to_string(currentPendingSpecCount) + ",\n";
        _moduleJson += "\t\t\t\"tests disabled\": " + std::to_string(currentDisabledSpecCount) + ",\n";
        _moduleJson += "\t\t\t\"tests excluded\": " + std::to_string(currentExcludedSpecCount) + ",\n";
        break;
      }

      case ItemRuntimeSpec::Disabled:
        _moduleJson += " \"disabled\",\n";
        break;
      case ItemRuntimeSpec::Excluded: {
        _moduleJson += " \"excluded\",\n";
        break;
      }
    }
  }
  _moduleJson += "\t\t\t\"duration\": " + std::to_string(result.duration.count()) + ",\n";

  static std::string errorIndent = "\t\t\t\t";

  if (!result.failures.empty()) {
    _moduleJson += "\t\t\t\"errors\": [\n";

    for (auto &entry : result.failures) {
      _moduleJson += errorIndent + "{\n";
      if (entry.file != currentSpecFile)
        _moduleJson += errorIndent + "\t\"file\": \"" + escape(relativePath(baseDir, entry.file)) + "\",\n";
      _moduleJson += errorIndent + "\t\"line\": " + std::to_string(entry.line) + ",\n";
      _moduleJson += errorIndent + "\t\"message\": \"" + escape(entry.message) + "\"\n";
      _moduleJson += errorIndent + "},\n";
    }

    removeLastComma(_moduleJson);
    _moduleJson += "\t\t\t],\n";
  }

  removeLastComma(_specJson);
  _moduleJson += "\t\t\t\"tests\": {\n" + _specJson + "\t\t\t}\n\t\t},\n";
}

//----------------------------------------------------------------------------------------------------------------------

void JsonReporter::casmineDone(microseconds duration) {
  Reporter::casmineDone(duration);

  _json += "\t\"tests executed\": " + std::to_string(totalSpecCount) + ",\n";
  _json += "\t\"tests succeeded\": " + std::to_string(totalSucceededSpecCount) + ",\n";
  _json += "\t\"tests failed\": " + std::to_string(totalFailedSpecCount) + ",\n";
  _json += "\t\"tests pending\": " + std::to_string(totalPendingSpecCount) + ",\n";
  _json += "\t\"tests disabled\": " + std::to_string(totalDisabledSpecCount) + ",\n";
  _json += "\t\"tests excluded\": " + std::to_string(totalExcludedSpecCount) + ",\n";
  _json += "\t\"duration\": " + std::to_string(duration.count()) + ",\n";

  removeLastComma(_moduleJson);
  _json += _moduleJson + "\t}\n}\n";

  std::ofstream stream(baseDir + "/test-results.json", std::ios_base::out | std::ios_base::binary);
  stream << _json;
}

//----------------------------------------------------------------------------------------------------------------------
