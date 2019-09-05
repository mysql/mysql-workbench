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

#include "mforms/code_editor.h"
#include "stub/stub_mforms.h"

#include "wb_test_helpers.h"
#include "casmine.h"

using namespace mforms;

namespace {

$ModuleEnvironment() {};

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
};

$describe("mforms code editor testing") {

  $beforeAll([this]() {
    data->tester.reset(new WorkbenchTester());
  });

  $it("Editor config loading + MySQL config values", []() {
    // Read a pretty standard code editor configuration file (MySQL section). Actual values don't matter (they
    // vary anyway) but there must be some base values in there and the xml must be read successfully.
    CodeEditorConfig config(mforms::LanguageMySQL);
    $expect(config.get_languages().size()).toBeGreaterThan(0U, "No language nodes found");

    // TODO: change log implementation so that we can easily check for new log entries.

    // Keywords.
    std::map<std::string, std::string> keywords = config.get_keywords();
    $expect(keywords.size()).toBeGreaterThan(0U, "Couldn't read keywords");
    $expect(keywords.find("Keywords")).Not.toEqual(keywords.end(), "Keyword list missing");
    $expect(keywords["Keywords"].empty()).toBeFalse("Keyword list empty");
    $expect(keywords.find("Procedure keywords")).Not.toEqual(keywords.end(), "Procedure keyword list missing");
    $expect(keywords["Procedure keywords"].empty()).toBeFalse("Procedure keyword list empty");
    $expect(keywords.find("User Keywords 1")).Not.toEqual(keywords.end(), "User keyword list 1 missing");
    $expect(keywords["User Keywords 1"].empty()).toBeFalse("User keyword list 1 empty");

    // Properties.
    std::map<std::string, std::string> properties = config.get_properties();
    $expect(properties.empty()).toBeFalse("Couldn't read properties");

    // Settings.
    std::map<std::string, std::string> settings = config.get_settings();
    $expect(settings.empty()).toBeFalse("Couldn't read settings");

    // Styles.
    std::map<int, std::map<std::string, std::string> > styles = config.get_styles();
    $expect(styles.empty()).toBeFalse("Couldn't read styles");

    // Pick some entries, just to check sub map.
    std::map<std::string, std::string> &values = styles[22]; // SCE_MYSQL_KEYWORD
    $expect(values.empty()).toBeFalse("Wrong number of style values found");
    $expect(values["fore-color"].empty()).toBeTrue("Old style color entry found");
    $expect(values["bold"].empty()).toBeFalse("Missing bold style for MySQL keywords");

    values = styles[22]; // SCE_MYSQL_PLACEHOLDER
    $expect(values.size()).toBeGreaterThanOrEqual(5U, "Wrong number of style values found");
    $expect(values["fore-color"].empty()).toBeTrue("Old style color entry found");
    $expect(values["fore-color-light"].empty()).toBeFalse("Missing fore-color-light");
    $expect(values["fore-color-dark"].empty()).toBeFalse("Missing fore-color-dark");
    $expect(values["back-color-light"].empty()).toBeFalse("Missing back-color-light");
    $expect(values["back-color-dark"].empty()).toBeFalse("Missing back-color-dark");
    $expect(values["bold"].empty()).toBeFalse("Missing bold style for MySQL keywords");
  });

  $it("Python config values", []() {
    // Another config check, this time for python.
    CodeEditorConfig config(mforms::LanguagePython);
    $expect(config.get_languages().empty()).toBeFalse("No language nodes found");

    // Keywords.
    std::map<std::string, std::string> keywords = config.get_keywords();
    $expect(keywords.empty()).toBeFalse("Couldn't read keywords");
    $expect(keywords.find("Keywords")).Not.toEqual(keywords.end(), "Keyword list missing");
    $expect(keywords["Keywords"].empty()).toBeFalse("Keyword list empty");

    $expect(keywords["Keywords"].find("continue")).Not.toBe(std::string::npos, "Python keyword \"continue\" not in keyword list");

    // Properties.
    std::map<std::string, std::string> properties = config.get_properties();
    $expect(properties.empty()).toBeFalse("Couldn't read properties");

    // Settings.
    std::map<std::string, std::string> settings = config.get_settings();
    $expect(settings.empty()).toBeFalse("Couldn't read settings");

    // Styles.
    std::map<int, std::map<std::string, std::string> > styles = config.get_styles();
    $expect(styles.empty()).toBeFalse("Couldn't read styles");

    // Pick some entries, just to check sub map.
    std::map<std::string, std::string> values = styles[8]; // Python class name.
    $expect(values.size()).toBeGreaterThan(1U, "Invalid style set for Python class names");
    $expect(values["fore-color"].empty()).toBeTrue("Old style color entry found");
    $expect(values["fore-color-light"].empty()).toBeFalse("Missing fore color style for Python class names");
    $expect(values["bold"].empty()).toBeFalse("Missing bold style for Python class names");
  });

}

}
