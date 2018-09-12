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

#include "utilities.h"
#include "scripting-context.h"

#include "events.h"

using namespace mga;

//----------------- EventEmitter ---------------------------------------------------------------------------------------

void EventEmitter::registerInContext(ScriptingContext &context, JSObject &exports) {
  std::ignore = context;
  exports.defineClass("EventEmitter", "", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = args;
    instance->setBacking(new EventEmitter());
  }, [](JSObject &prototype) {
    prototype.defineFunction({ "addListener", "on" }, 2, [](JSExport *emitter, JSValues &args) {
      args.context()->addEventListener(dynamic_cast<EventEmitter *>(emitter), false);
    });
    prototype.defineFunction({ "emit" }, JSExport::VarArgs, [](JSExport *emitter, JSValues &args) {
      args.context()->emitEvent(dynamic_cast<EventEmitter *>(emitter));
    });
    prototype.defineFunction({ "eventNames" }, 0, [](JSExport *emitter, JSValues &args) {
      args.context()->eventNames(dynamic_cast<EventEmitter *>(emitter));
      args.haveResult(true); // Must set this explicitly here, as the result is already pushed.
    });
    prototype.defineFunction({ "getMaxListeners" }, 0, [](JSExport *emitter, JSValues &args) {
      size_t count = dynamic_cast<EventEmitter *>(emitter)->maxListeners;
      args.pushResult(static_cast<int>(count));
    });
    prototype.defineFunction({ "listenerCount" }, 1, [](JSExport *emitter, JSValues &args) {
      args.context()->eventListenerCount(dynamic_cast<EventEmitter *>(emitter));
      args.haveResult(true);
    });
    prototype.defineFunction({ "once" }, 2, [](JSExport *emitter, JSValues &args) {
      args.context()->addEventListener(dynamic_cast<EventEmitter *>(emitter), true);
    });
    prototype.defineFunction({ "prependListener" }, 2, [](JSExport *emitter, JSValues &args) {
      args.context()->prependEventListener(dynamic_cast<EventEmitter *>(emitter), false);
    });
    prototype.defineFunction({ "prependOnceListener" }, 2, [](JSExport *emitter, JSValues &args) {
      args.context()->prependEventListener(dynamic_cast<EventEmitter *>(emitter), true);
    });
    prototype.defineFunction({ "removeAllListeners" }, 1, [](JSExport *emitter, JSValues &args) {
      args.context()->removeAllEventListeners(dynamic_cast<EventEmitter *>(emitter));
    });
    prototype.defineFunction({ "removeListener" }, 2, [](JSExport *emitter, JSValues &args) {
      args.context()->removeEventListener(dynamic_cast<EventEmitter *>(emitter));
    });
    prototype.defineFunction({ "setMaxListeners" }, 1, [](JSExport *emitter, JSValues &args) {
      unsigned count = args.as(ValueType::UInt, 0);
      dynamic_cast<EventEmitter *>(emitter)->maxListeners = count;
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

void EventEmitter::finalize(ScriptingContext *context) {
  // Remove all listener entries for this emitter.
  context->removeAllEventListeners(this);
}

//----------------- Events ---------------------------------------------------------------------------------------------

void Events::activate(ScriptingContext &context, JSObject &exports) {
  EventEmitter::registerInContext(context, exports);
}

//----------------------------------------------------------------------------------------------------------------------

bool Events::_registered = []() {
  ScriptingContext::registerModule("events", &Events::activate);
  return true;
}();
