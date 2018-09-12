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

#include <vector>
#include <functional>
#include <memory>

namespace base {
  class any;
}

namespace mga {

  using StringArray = std::vector<std::string>;
  using StringArrayRef = std::shared_ptr<StringArray>;

  class ScriptingContext;
  class JSObject;
  class JSClass;
  class JSValues;
  class JSExport;
  class JSVariant;

  enum ExitCode {
    Success = 0,
    ScriptError,
    RunLoopError,
    Other,
    CompletionFailure
  };

  using ModuleActivationFunction = std::function<void (ScriptingContext &, JSObject &exports)>;

  using FunctionCallback = std::function<void (JSExport *, JSValues &)>;
  using ObjectDefCallback = std::function<void (JSObject &)>;
  using ClassDefCallback = std::function<void (JSClass &)>;
  using PrototypeDefCallback = std::function<void (JSObject &)>;

  using ConstructorFunction = std::function<void (JSObject *, JSValues &)>;

  using PropertyGetter = std::function<JSVariant (ScriptingContext *, JSExport *, std::string const&)>;
  using PropertySetter = std::function<void (ScriptingContext *, JSExport *, std::string const&, JSVariant const&)>;

  // Interface for serialization of a (simple) object to a (json) string.
  class SerializableObject {
  public:
    virtual std::string toJson() const = 0;
  };

} // namespace mga
