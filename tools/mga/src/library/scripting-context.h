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

#include "common.h"
#include "jsexport.h"

namespace mga {

  enum class ScriptingError : duk_int_t {
    Error = DUK_ERR_ERROR,
    Eval = DUK_ERR_EVAL_ERROR,
    Range = DUK_ERR_RANGE_ERROR,
    Reference = DUK_ERR_REFERENCE_ERROR,
    Syntax = DUK_ERR_SYNTAX_ERROR,
    Type = DUK_ERR_TYPE_ERROR,
    Uri = DUK_ERR_URI_ERROR
  };

  class JSClass : public JSObject {
  public:
    JSClass();
    JSClass(ScriptingContext *context, duk_idx_t index);

    JSObject getPrototype() const;
  };

  class EventEmitter;

  // A class to manage a duktape context.
  // Many of the member functions act directly on the underlying duktape context, that is, they expect their values
  // there and also push result to it (if any).
  class ScriptingContext {
    // Friend classes, because I don't want the internal duktape context to be accessible, except for those classes.
    friend class JSValueBase;
    friend class JSObject;
    friend class JSGlobalObject;
    friend class JSArray;
    friend class JSValues;
    friend class JSVariant;
    //friend class JSContext;

    friend class DebugAdapter;

  public:
    ScriptingContext();
    ~ScriptingContext();

    void bootstrap(std::string const& fileName) const;
    void evaluate(std::string const &input) const;

    // Run loop related functions.
    void expireTimers();
    void runImmediates();
    bool callbacksPending() const;

    JSObject loadJsonFile(std::string const& fileName);

    // Error handling.
    void checkForErrors() const;
    void throwScriptingError(ScriptingError error, std::string const& message) const;

    // Type/instance/property handling.
    JSClass getClass(std::string const& name);
    void defineClass(JSObject &target, std::string const& name, std::string const& baseClass, int argCount,
                     ConstructorFunction constructor, PrototypeDefCallback callback);
    JSObject createJsInstance(std::string const& name, VariantArray const& args);
    void runFunctionFromThis(std::string const& name, int argCount);
    void callConstructor(std::string const& className, VariantArray const& args);
    void establishInheritance();
    
    // Timers + callbacks.
    std::string setImmediate() const;
    bool clearImmediate() const;
    std::string setInterval() const;
    bool clearInterval() const;
    std::string setTimeout() const;
    bool clearTimeout() const;

    // Events.
    void emitEvent(std::string const& emitter, std::string const eventName,
                   std::initializer_list<JSVariant> const& values) const;

    void addEventListener(EventEmitter const* emitter, bool once) const;
    void emitEvent(EventEmitter const* emitter) const;
    void removeEventListeners(JSExport const* emitter) const;
    void eventNames(EventEmitter const* emitter) const;
    void eventListenerCount(EventEmitter const* emitter) const;
    void prependEventListener(EventEmitter const* emitter, bool once) const;
    void removeAllEventListeners(EventEmitter const* emitter) const;
    void removeEventListener(EventEmitter const* emitter) const;

    // Utility functions.
    std::string logOutput(const char *errorName = nullptr) const;
    void format() const;

    static void registerModule(std::string const& name, ModuleActivationFunction activation);

  protected:
    void getCleanedStackTrace() const;

    void pushValue(std::map<std::string, std::string> const& map) const;
    void pushValue(std::vector<std::string> const& array) const;

    std::string stashValue(duk_idx_t index) const;
    void unstashValue(std::string const& id) const;
    void usingStashedValue(std::string const& name, std::function<void ()> callback) const;

    void* getNativeReference() const;
    void setNativeReference(void *reference) const;

    void executeCallback(std::string const& id) const;

    // Internal event handling.
    bool loadEventsArray(bool create) const;
    std::string getAssignedEvent() const;
    void enumerateEventArray(std::function<bool (duk_idx_t)> callback) const;
    void removeListeners(std::string const& id, std::string const& event, std::set<duk_uarridx_t> indices) const;

    static ScriptingContext* fromDuktapeContext(duk_context *ctx);
    
  private:
    duk_context *_ctx;

    static bool _callingConstructor; // Set when running an inherited constructor.
    static std::map<std::string, ConstructorFunction> _constructorMap;

    // c-tor and d-tor trampolines for all registered classes.
    static duk_ret_t constructor(duk_context *ctx);
    static duk_ret_t destructor(duk_context *ctx);

    std::string createTimer(bool oneshot) const;
    bool deleteTimer() const;

    static std::string resolveFolder(ScriptingContext *context, std::string const& path);
    static std::set<std::string> moduleFolders(std::string const& path);
    static duk_ret_t resolveModule(duk_context* ctx);
    static duk_ret_t loadModule(duk_context* ctx);
  };
  
}
