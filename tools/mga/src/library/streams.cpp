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

#include "streams.h"

using namespace mga;

//----------------- Stream ---------------------------------------------------------------------------------------------

void Stream::registerInContext(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("Stream", "EventEmitter", 0, 
    [](JSObject *process, JSValues &args) { 
      std::ignore = process;
      std::ignore = args;
    }, [](JSObject &prototype) {
      std::ignore = prototype;
    });
}

//----------------- ReadableStream -------------------------------------------------------------------------------------

void ReadableStream::registerInContext(ScriptingContext &context, JSObject &exports) {
  std::ignore = context;
  exports.defineClass("Readable", "Stream", 0, 
    [](JSObject *process, JSValues &args) {
      std::ignore = process;
      std::ignore = args;
    }, [](JSObject &prototype) {
      std::ignore = prototype;
    });
}

//----------------- WritableStream -------------------------------------------------------------------------------------

void WritableStream::registerInContext(ScriptingContext &context, JSObject &exports) {
  std::ignore = context;
  exports.defineClass("Writable", "Stream", 0, [](JSObject *process, JSValues &args) {
    std::ignore = process;
    std::ignore = args;
  }, [](JSObject &prototype) {
    // Currently we only have very limited functionality. Just as much as is required to implement stdout and
    // stderr in Process.
    prototype.defineFunction( { "cork" }, 0, [](JSExport *, JSValues &args) {
      std::ignore = args;
    });
    prototype.defineFunction( { "end" }, 3, [](JSExport *, JSValues &args) {
      std::ignore = args;
    });
    prototype.defineFunction( { "setDefaultEncoding" }, 1, [](JSExport *, JSValues &args) {
      std::ignore = args;
    });
    prototype.defineFunction( { "uncork" }, 0, [](JSExport *, JSValues &args) {
      std::ignore = args;
    });
    prototype.defineFunction( { "write" }, 3, [](JSExport *writer, JSValues &args) {
      std::ignore = writer;
      args.context()->runFunctionFromThis("_write", static_cast<int>(args.size()));
    });
    prototype.defineFunction( { "destroy" }, 1, [](JSExport *, JSValues &args) {
      std::ignore = args;
    });
  });
}

//----------------- DuplexStream ---------------------------------------------------------------------------------------

void DuplexStream::registerInContext(ScriptingContext &context, JSObject &exports) {
  std::ignore = context;
  exports.defineClass("Duplex", "Stream", 0, 
    [](JSObject *process, JSValues &args) {
      std::ignore = process;
      std::ignore = args;
    }, [](JSObject &prototype) {
      std::ignore = prototype;
    });
}

//----------------- TransformStream ------------------------------------------------------------------------------------

void TransformStream::registerInContext(ScriptingContext &context, JSObject &exports) {
  std::ignore = context;
  exports.defineClass("Transform", "Stream", 0, 
    [](JSObject *process, JSValues &args) {
    std::ignore = process;
    std::ignore = args;
    }, [](JSObject &prototype) {
      std::ignore = prototype;
    });
}

//----------------- Streams --------------------------------------------------------------------------------------------

void Streams::activate(ScriptingContext &context, JSObject &exports) {
  // The events module must be active at this point.
  Stream::registerInContext(context, exports);
  ReadableStream::registerInContext(context, exports);
  WritableStream::registerInContext(context, exports);
  DuplexStream::registerInContext(context, exports);
  TransformStream::registerInContext(context, exports);
}

//----------------------------------------------------------------------------------------------------------------------

bool Streams::_registered = []() {
  ScriptingContext::registerModule("stream", &Streams::activate);
  return true;
}();
