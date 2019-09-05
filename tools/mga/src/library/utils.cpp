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

#include "utils.h"
#include "utilities.h"
#include "scripting-context.h"
#include "platform.h"

#include "utf8proc.h"

using namespace mga;

//----------------------------------------------------------------------------------------------------------------------

void Utils::activate(ScriptingContext &context, JSObject &exports) {
  exports.defineFunction({ "format" }, JSExport::VarArgs, [](JSExport *, JSValues &args) {
    // parameters: format-string? <varargs>
    args.format();
  });

  exports.defineFunction({ "inherit" }, 2, [](JSExport *, JSValues &args) {
    // parameters: constructor, superConstructor
    args.context()->establishInheritance();
  });

  exports.defineFunction({ "inspect" }, 2, [](JSExport *, JSValues &args) {
    // parameters: object, options
    bool showHidden = false;
    size_t maxLevel = 2;

    if (args.is(ValueType::Object, 1)) {
      JSObject options = args.get(1);
      showHidden = options.get("showHidden", false);
      maxLevel = options.get("maxLevel", 2);
    }
    std::string dump = args.dumpObject(0, showHidden, maxLevel);
    args.pushResult(dump);
  });

  exports.defineFunction({ "getImageResolution" }, 1, [](JSExport *, JSValues &args) {
    // parameters: path
    std::string path = args.get(0);
    args.pushResult(Platform::get().getImageResolution(path));
  });

  JSClass string = context.getClass("String");
  string.getPrototype().defineFunction({ "normalize" }, 1, [](JSExport *, JSValues &args) {
    // parameters: format-string?
    static std::map<std::string, NormalizationForm> formMap = {
      { "NFC", NormalizationForm::NFC },
      { "NFD", NormalizationForm::NFD },
      { "NFKC", NormalizationForm::NFKC },
      { "NFKD", NormalizationForm::NFKD },
    };

    std::string form = args.get(0, "NFC");
    auto iterator = formMap.find(form);
    if (iterator == formMap.end())
      args.context()->throwScriptingError(ScriptingError::Range, "Invalid normalization form specified");

    std::string text = args.getThis().stringContent();
    args.pushResult(Utilities::normalize(text, iterator->second));
  });
}

//----------------------------------------------------------------------------------------------------------------------

bool Utils::_registered = []() {
  ScriptingContext::registerModule("util", &Utils::activate);
  return true;
}();

//----------------------------------------------------------------------------------------------------------------------
