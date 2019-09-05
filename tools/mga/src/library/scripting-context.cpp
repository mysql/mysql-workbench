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

#include "duk_module_node.h"

#include "filesystem.h"
#include "path.h"
#include "utilities.h"
#include "platform.h"

#include "glob.h"
#include "child_process.h"
#include "db.h"
#include "os.h"

#include "scripting-context.h"

using namespace mga;
using namespace std::chrono;

#define MIN_DELAY  1 // 1ms, depends also on the speed at which the run loop is executed.

// Timer + runloop implementation modeled after the duktape runloop example.
#define MIN_WAIT 1.0
#define MAX_WAIT 60000.0
#define MAX_FDS  256

std::map<std::string, ConstructorFunction> ScriptingContext::_constructorMap;
bool ScriptingContext::_callingConstructor = false;

//----------------- JSClass --------------------------------------------------------------------------------------------

JSClass::JSClass() : JSObject() {
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * JSClass represents a constructor function in JS, which is a special object with a prototype for inheritance.
 */
JSClass::JSClass(ScriptingContext *context, duk_idx_t index) : JSObject(context, index) {
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the prototype for this class.
 */
JSObject JSClass::getPrototype() const {
  return get("prototype");
}

//----------------- ScriptingContext -----------------------------------------------------------------------------------

static void normalizeFilename(duk_context *ctx) {
  std::string filename = duk_require_string(ctx, -1);
  filename = Path::resolve({ filename });
  duk_push_string(ctx, filename.c_str());
  duk_replace(ctx, -2);
}

//----------------------------------------------------------------------------------------------------------------------

ScriptingContext::ScriptingContext() {
  _ctx = duk_create_heap_default();
  assert(_ctx != nullptr);

  duk_push_global_stash(_ctx);

  // Store a reference of this object in the duktape context.
  duk_push_pointer(_ctx, this);
  duk_put_prop_string(_ctx, -2, "context");

  // Some objects for work data.
  duk_push_bare_object(_ctx);
  duk_put_prop_string(_ctx, -2, "callbacks");
  duk_push_bare_object(_ctx);
  duk_put_prop_string(_ctx, -2, "events");
  duk_push_bare_object(_ctx);
  duk_put_prop_string(_ctx, -2, "classes");

  duk_pop(_ctx);

  // Store duktape's global object under the global property "global".
  duk_push_global_object(_ctx);
  duk_put_global_string(_ctx, "global");

  // Module loading support.
  duk_push_object(_ctx);
  duk_push_c_function(_ctx, resolveModule, JSExport::VarArgs);
  duk_put_prop_string(_ctx, -2, "resolve");
  duk_push_c_function(_ctx, loadModule, JSExport::VarArgs);
  duk_put_prop_string(_ctx, -2, "load");
  duk_module_node_init(_ctx, normalizeFilename);
}

//----------------------------------------------------------------------------------------------------------------------

ScriptingContext::~ScriptingContext() {
  duk_destroy_heap(_ctx);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Runs the initial (main) script in this context.
 */
void ScriptingContext::bootstrap(std::string const& fileName) const {
  // Insert script into a global field. It will run in an initial timer callback trigger from the run loop.
  duk_push_global_object(_ctx);
  std::string text = FS::contentFromFile(fileName);
  duk_push_string(_ctx, fileName.c_str());
  duk_pcompile_string_filename(_ctx, DUK_COMPILE_SHEBANG, text.c_str());
  checkForErrors();

  duk_put_prop_string(_ctx, -2, "_START");
  duk_pop(_ctx);
  duk_eval_string(_ctx, "setTimeout(function() { _START(); }, 0);");
  duk_pop(_ctx);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Allows to evaluate JS code (e.g. to inject functions etc.)
 */
void ScriptingContext::evaluate(std::string const &input) const {
  duk_push_string(_ctx, input.c_str());
  duk_peval(_ctx);
  checkForErrors();
}

//----------------------------------------------------------------------------------------------------------------------

// A struct for timer management held internally.
struct Timer {
  std::string id;
  bool oneshoot;
  bool done;

  milliseconds target;
  milliseconds delay;

  Timer(std::string id_, bool oneshoot_, bool done_, milliseconds target_, milliseconds delay_)
    : id(id_), oneshoot(oneshoot_), done(done_), target(target_), delay(delay_) {}
};

static std::vector<Timer> timers;
static std::vector<Timer> newTimers; // Newly created timers while we expire timers.
static duk_uarridx_t nextCallbackId = 1;

static std::set<std::string> immediates;
static std::set<std::string> newImmediates; // Newly created immediates while running existing ones.

//----------------------------------------------------------------------------------------------------------------------

/**
 * Called after running timers and immediates to merge in new callbacks that have been created while running either
 * a timer or an immediate.
 */
static void mergeNewCallbacks() {
  if (!newTimers.empty()) {
    for (auto &timer : newTimers)
      timers.push_back(timer);

    // Sort timers by decreasing target time.
    std::sort(timers.begin(), timers.end(), [](Timer const& lhs, Timer const& rhs) {
      return rhs.target < lhs.target;
    });

    newTimers.clear();
  }

  if (!newImmediates.empty()) {
    for (auto &immediate : newImmediates)
      immediates.insert(immediate);
    newImmediates.clear();
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Walks the timer list and executes callbacks of the timers whose time has come. Removes outdated timers and adds
 * new ones created by the executed callbacks.
 */
void ScriptingContext::expireTimers() {
  while (true) {
    if (timers.empty())
      break;

    // The timer at the end of the timer list is the next one to fire.
    milliseconds now = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch();
    if (timers.back().target > now)
      break;

    // Reschedule this timer.
    if (timers.back().oneshoot)
      timers.back().done = true;
    else
      timers.back().target = now + timers.back().delay;

    // Load + execute timer callback. Note: no explicit this binding.
    executeCallback(timers.back().id);

    // Finally remove all finished timers.
    timers.erase(std::remove_if(timers.begin(), timers.end(), [&](Timer const& timer) {
      if (timer.done)
        duk_del_prop_string(_ctx, -1, timer.id.c_str());

      return timer.done;
    }), timers.end());

    // A timer was rescheduled. Need to move it to it's correct place in the list.
    std::sort(timers.begin(), timers.end(), [](Timer const& lhs, Timer const& rhs) {
      return rhs.target < lhs.target;
    });

    duk_debugger_cooperate(_ctx);
  }

  // Once we are done with the normal timer list merge in new timers.
  // Those will be handled in the next loop.
  mergeNewCallbacks();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Runs all callbacks registerd with setImmediate.
 */
void ScriptingContext::runImmediates() {
  if (!immediates.empty()) {
    for (auto &id : immediates) {
      executeCallback(id);

      // Finally remove the callback object from the stash.
      duk_del_prop_string(_ctx, -1, id.c_str());
      
      duk_debugger_cooperate(_ctx);
    }
  }

  immediates.clear();
  mergeNewCallbacks();
}

//----------------------------------------------------------------------------------------------------------------------

bool ScriptingContext::callbacksPending() const {
  return !timers.empty() || !immediates.empty();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Loads the given JSON file as object and returns it. Throws a JS error if parsing failed.
 */
JSObject ScriptingContext::loadJsonFile(std::string const& fileName) {
  std::string config = FS::contentFromFile(fileName);
  try {
    duk_get_global_string(_ctx, "Duktape");
    duk_push_string(_ctx, "dec");
    duk_push_string(_ctx, "jx");
    duk_push_string(_ctx, config.c_str());
    duk_call_prop(_ctx, -4, 2);
  } catch (...) {
    checkForErrors();
  }
  
  auto result = JSObject(this, -1);
  duk_pop(_ctx);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Checks if the TOS contains a JS error object and throws a C++ exception for it. This is particularly useful
 * when executing code in safe mode (pcall).
 */
void ScriptingContext::checkForErrors() const {
  if (duk_is_error(_ctx, -1)) {
    // See if there's a stack trace. That includes the actual error message so we can use
    // that as exception message. Otherwise just get what is on stack top.
    std::string error;
    if (duk_has_prop_string(_ctx, -1, "stack")) {
      duk_get_prop_string(_ctx, -1, "stack"); // Puts stack trace on the stack.
      getCleanedStackTrace();
      error = duk_require_string(_ctx, -1);
    } else {
      error = duk_safe_to_string(_ctx, -1);
    }
    duk_pop(_ctx); // Remove error from stack.

    throw std::runtime_error(error);
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Creates an error in the scripting enironment (which acts like a normal exception on C++ side). 
 */
void ScriptingContext::throwScriptingError(ScriptingError error, std::string const& message) const {
  duk_push_error_object_raw(_ctx, static_cast<duk_int_t>(error), nullptr, 0, "%s", message.c_str());
  duk_get_prop_string(_ctx, -1, "stack");
  getCleanedStackTrace();
  duk_put_prop_string(_ctx, -2, "stack");
  duk_throw(_ctx);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the class specified by the given name.
 */
JSClass ScriptingContext::getClass(std::string const& name) {
  JSClass result;

  // First try standard JS class lookup.
  duk_push_global_object(_ctx);
  if (duk_has_prop_string(_ctx, -1, name.c_str())) {
    duk_get_prop_string(_ctx, -1, name.c_str());
    result = JSClass(this, -1);
    duk_pop(_ctx);
  } else {
    duk_push_global_stash(_ctx);
    duk_get_prop_string(_ctx, -1, "classes");
    duk_get_prop_string(_ctx, -1, name.c_str());
    if (duk_is_undefined(_ctx, -1))
      throwScriptingError(ScriptingError::Type, "Attempt to use undefined class '" + name + "'");

    result = JSClass(this, -1);
    duk_pop_3(_ctx);
  }
  duk_pop(_ctx);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Defines a new JS "class" (which can be used as any JS class, e.g. instantiated via "new MyClass()").
 * The actual functionality of this class can be defined in the prototype callback which is called as part of this
 * definition process.
 *
 * @param target The object on which to define the class (can be the global JS object or a module/namespace/class).
 * @param name The name of the class to define.
 * @param baseClass The name of a class the new class derives from. Can be left empty if "Object" is the base class.
 * @param argCount The number of arguments for the constructor.
 * @param constructor A function to call when an instance of this class is being created.
 * @param callback A function to call for seting up the prototype object for this class.
 */
void ScriptingContext::defineClass(JSObject &target, std::string const& name, std::string const& baseClass,
                                   int argCount, ConstructorFunction constructor, PrototypeDefCallback callback) {
  if (_constructorMap.find(name) != _constructorMap.end())
    throw std::runtime_error("Duplicate class name '" + name + "'");

  duk_push_c_function(_ctx, ScriptingContext::constructor, argCount);
  duk_push_string(_ctx, "name");
  duk_push_string(_ctx, name.c_str());
  duk_def_prop(_ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_READ_ONLY); // Cannot be changed.

  // Stash the new class, so we can find it later (for inheritance or manual instantiation).
  // We can do this as we use this method only for internal class references (otherwise we would need to "require()"
  // the module where the class is exported from etc.).
  duk_push_global_stash(_ctx);
  duk_get_prop_string(_ctx, -1, "classes");
  duk_dup(_ctx, -3);
  duk_put_prop_string(_ctx, -2, name.c_str());
  duk_pop_2(_ctx);

  // Create a prototype for the new class and let the caller configure it.
  JSObject prototype(this);
  callback(prototype);
  prototype.push();

  if (!baseClass.empty() && baseClass != "Object") {
    // stack: [ ... constructor derived-prototype ]
    JSClass base = getClass(baseClass);
    base.push();
    duk_get_prop_string(_ctx, -1, "prototype"); // stack: [ ... constructor derived-prototype base base-prototype ]
    duk_remove(_ctx, -2); // stack: [ ... constructor derived-prototype base-prototype ]
    duk_set_prototype(_ctx, -2); // Does derived.prototype.__proto__ = base.prototype.
  }

  // stack: [ ... constructor prototype ]
  duk_put_prop_string(_ctx, -2, "prototype");

  // Finally store the constructor function under the class name on the given target.
  target.defineProperty(name, JSClass(this, -1));
  duk_pop(_ctx);

  _constructorMap[name] = constructor;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Creates a new instance of a JS class.
 *
 * @param name The name of the class to create.
 * @param args The constructor arguments.
 */
JSObject ScriptingContext::createJsInstance(std::string const& name, VariantArray const& args) {
  JSClass clazz = getClass(name);
  clazz.push();

  if (args.empty())
    duk_new(_ctx, 0);
  else {
    for (size_t i = 0; i < args.size(); ++i)
      args[i].pushValue(this);
    duk_new(_ctx, static_cast<duk_int_t>(args.size()));
  }

  JSObject result(this, -1);
  duk_pop(_ctx);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * This method allows to call a function property of the current this binding. It must hence be called only from a
 * JS function callback (where the this binding is set).
 * The parameters for the function are all on the value stack already.
 */
void ScriptingContext::runFunctionFromThis(std::string const& name, int argCount) {
  duk_idx_t firstIndex = duk_get_top(_ctx) - argCount;

  // Insert the this binding before the arguments.
  duk_push_this(_ctx);
  duk_insert(_ctx, firstIndex);

  if (duk_has_prop_string(_ctx, firstIndex, name.c_str())) { // Does this function exist?
    // If so insert the function's name between this binding and arguments.
    duk_push_string(_ctx, name.c_str());
    duk_insert(_ctx, firstIndex + 1);

    // Now we are ready to run.
    duk_call_prop(_ctx, firstIndex, argCount);
  } else
    duk_remove(_ctx, firstIndex);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * This method is a helper to implement inheritance in JS classes with a native constructor. It allows to call
 * the constructor of the base class.
 */
void ScriptingContext::callConstructor(std::string const& className, VariantArray const& args) {
  JSClass clazz = getClass(className);
  clazz.push();

  // When calling the inherited constructor we do that like with any other method, however this will
  // not set the constructor flag in duktape, breaking so our check for a constructor call.
  // Instead we use an own flag here, just for this single purpose.
  _callingConstructor = true;
  try {
    duk_push_this(_ctx);
    if (args.empty())
      duk_call_method(_ctx, 0);
    else {
      for (size_t i = 0; i < args.size(); ++i)
        args[i].pushValue(this);
      duk_call_method(_ctx, static_cast<duk_int_t>(args.size()));
    }
    duk_pop(_ctx); // Remove result from constructor call.
    _callingConstructor = false;
  } catch (...) {
    _callingConstructor = false;
    throw;
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * A helper to set up the inheritance chain between two JS classes. Expects constructor and super constructor
 * on the stack.
 */
void ScriptingContext::establishInheritance() {
  // stack: [ ... derived base ]
  duk_get_prop_string(_ctx, -2, "prototype"); // stack: [ ... derived base derived-prototype ]
  duk_get_prop_string(_ctx, -2, "prototype"); // stack: [ ... derived base derived-prototype base-prototype ]
  duk_set_prototype(_ctx, -2); // Does derived.prototype.__proto__ = base.prototype.
  duk_pop(_ctx);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Central constructor function called by duktape which triggers the individual constructor functions specified
 * for each registered class.
 */
duk_ret_t ScriptingContext::constructor(duk_context *ctx) {
  auto *context = fromDuktapeContext(ctx);
  if (!duk_is_constructor_call(ctx) && !_callingConstructor) {
    duk_push_current_function(ctx);
    std::string name = "<unknown>";
    if (duk_get_prop_string(ctx, -1, "name"))
      name = duk_get_string(ctx, -1);

    duk_pop_2(ctx);
    context->throwScriptingError(ScriptingError::Error, name + " must be created with the new operator");
  }

  duk_push_current_function(ctx);
  if (!duk_get_prop_string(ctx, -1, "name"))
    context->throwScriptingError(ScriptingError::Type, "Internal error: constructor without name");

  std::string name = duk_get_string(ctx, -1);
  duk_pop_2(ctx);

  duk_push_this(ctx);

  // Store the destructor function for this JS object.
  duk_push_c_function(ctx, destructor, 1);
  duk_set_finalizer(ctx, -2);

  JSObject object(context, -1);
  duk_pop(ctx);

  JSValues args(context);
  _constructorMap[name](&object, args);

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Ditto for the destructor.
 * Note: sometimes there is a backing class and sometimes there's none.
 */
duk_ret_t ScriptingContext::destructor(duk_context *ctx) {
  // stack: [ object ]
  duk_get_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("data"));
  JSExport *object = reinterpret_cast<JSExport *>(duk_get_pointer(ctx, -1));
  duk_pop(ctx);

  if (object != nullptr) {
    object->finalize(fromDuktapeContext(ctx));

    duk_push_pointer(ctx, nullptr);
    duk_put_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("data"));

    delete object;
  }

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Takes a callback and parameters from the duktape stack and puts it on the global stash.
 */
static void createCallbackEntry(duk_context *ctx, std::string const& callbackId, duk_uarridx_t paramOffset) {
  // stack: [ callback ... <varargs> ]

  // The callback is a JS function and must be stored in a JS object to stay alive (same for its parameters).
  // Create a callback object to hold all that.
  duk_uarridx_t argCount = static_cast<duk_uarridx_t>(duk_get_top(ctx)) - paramOffset;

  duk_push_bare_object(ctx);
  duk_dup(ctx, 0);
  duk_put_prop_string(ctx, -2, "callback");
  duk_push_array(ctx);
  for (duk_uarridx_t i = 0; i < argCount; ++i) {
    duk_dup(ctx, static_cast<duk_int_t>(i + paramOffset));
    duk_put_prop_index(ctx, -2, i);
  };
  duk_put_prop_string(ctx, -2, "arguments");

  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, "callbacks");
  duk_remove(ctx, -2); // Remove global stash.
  duk_insert(ctx, -2); // stack: [ callback ... <varargs> callbacks callback-object ]
  duk_put_prop_string(ctx, -2, callbackId.c_str());  // callbacks[callback-id] = callback-object

  duk_pop(ctx); // Pop callbacks.
}

//----------------------------------------------------------------------------------------------------------------------

std::string ScriptingContext::createTimer(bool oneshot) const {
  // stack: [ callback delay <varargs> ]
  duk_require_function(_ctx, 0);
  duk_double_t d = duk_is_number(_ctx, 1) ? duk_get_number(_ctx, 1) : 0;
  if (d < MIN_DELAY)
    d = MIN_DELAY;

  milliseconds delay { static_cast<size_t>(d) };
  milliseconds now = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch();

  std::string id = std::to_string(nextCallbackId++);
  newTimers.emplace_back(id, oneshot, false, now + delay, delay);
  createCallbackEntry(_ctx, id, 2);

  return id;
}

//----------------------------------------------------------------------------------------------------------------------

bool ScriptingContext::deleteTimer() const {
  // stack: [ timer-id ]
  std::string id = duk_to_string(_ctx, 0);

  // Check first the pending timers list.
  auto iterator = std::find_if(newTimers.begin(), newTimers.end(), [&](Timer const& timer) {
    return timer.id == id;
  });

  if (iterator != newTimers.end()) { // A pending timer can directly be removed.
    newTimers.erase(iterator);
    usingStashedValue("callbacks", [&]() {
      duk_del_prop_string(_ctx, -1, id.c_str());
    });

    return true;
  }

  iterator = std::find_if(timers.begin(), timers.end(), [&](Timer const& timer) {
    return timer.id == id;
  });

  if (iterator != timers.end()) {
    iterator->done = true;
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

std::string ScriptingContext::setImmediate() const {
  // stack: [ callback <varargs> ]
  duk_require_function(_ctx, 0);

  std::string id = std::to_string(nextCallbackId++);
  newImmediates.insert(id);
  createCallbackEntry(_ctx, id, 1);

  return id;
}

//----------------------------------------------------------------------------------------------------------------------

bool ScriptingContext::clearImmediate() const {
  // stack: [ immediate-id ]
  std::string id = duk_to_string(_ctx, -1);

  bool removeCallback = false;
  if (immediates.count(id) > 0) {
    immediates.erase(id);
    removeCallback = true;
  } else if (newImmediates.count(id) > 0) {
    newImmediates.erase(id);
    removeCallback = true;
  }

  if (removeCallback) {
    usingStashedValue("callbacks", [&]() {
      duk_del_prop_string(_ctx, -1, id.c_str());
    });
  }

  return removeCallback;
}

//----------------------------------------------------------------------------------------------------------------------

std::string ScriptingContext::setInterval() const {
  return createTimer(false);
}

//----------------------------------------------------------------------------------------------------------------------

bool ScriptingContext::clearInterval() const {
  return deleteTimer();
}

//----------------------------------------------------------------------------------------------------------------------

std::string ScriptingContext::setTimeout() const {
  return createTimer(true);
}

//----------------------------------------------------------------------------------------------------------------------

bool ScriptingContext::clearTimeout() const {
  return deleteTimer();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Sends an event with the given name + parameters and the emitter as caller (which must be a global variable and an
 * EventEmitter instance).
 *
 * This method is used for events triggered from C++.
 */
void ScriptingContext::emitEvent(std::string const& emitter, std::string const eventName,
                                 std::initializer_list<JSVariant> const& values) const {
  duk_get_global_string(_ctx, emitter.c_str());
  duk_get_prop_string(_ctx, -1, "emit");
  duk_dup(_ctx, -2);  // The `this` binding for this call.
  duk_push_string(_ctx, eventName.c_str());

  for (auto &value : values)
    value.pushValue(this);

  duk_pcall_method(_ctx, static_cast<duk_idx_t>(values.size() + 1));
  duk_pop_2(_ctx);
}

//----------------------------------------------------------------------------------------------------------------------

void ScriptingContext::addEventListener(EventEmitter const* emitter, bool once) const {
  // stack: [... event-name listener ]
  if (duk_is_function(_ctx, -1)) {
    std::string event = duk_to_string(_ctx, -2); // A string or a symbol. We coerce both to a string.
    duk_idx_t functionIndex = duk_normalize_index(_ctx, -1);

    duk_push_sprintf(_ctx, "%p", emitter);
    loadEventsArray(true);

    duk_size_t length = duk_get_length(_ctx, -1);
    duk_idx_t arrayIndex = duk_normalize_index(_ctx, -1);

    // Create new object in the events array.
    duk_push_object(_ctx);
    duk_push_string(_ctx, event.c_str());
    duk_put_prop_string(_ctx, -2, "event");
    duk_push_boolean(_ctx, once);
    duk_put_prop_string(_ctx, -2, "once");
    duk_dup(_ctx, functionIndex);
    duk_put_prop_string(_ctx, -2, "listener");

    duk_put_prop_index(_ctx, arrayIndex, static_cast<duk_uarridx_t>(length));

  } else {
    throwScriptingError(ScriptingError::Error, "Listener value is not a callable function");
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Sends an event as specified on the stack.
 *
 * This method is used for events triggered from JS.
 */
void ScriptingContext::emitEvent(EventEmitter const* emitter) const {
  // stack: [ event-name <varargs> ]
  duk_int_t argCount = duk_get_top(_ctx) - 1; // The number of args for the event callback (without the event name).
  std::string event = duk_require_string(_ctx, 0); // A string or a symbol. We coerce both to a string.

  duk_push_sprintf(_ctx, "%p", emitter);
  std::string key = duk_get_string(_ctx, -1);
  if (!loadEventsArray(false))
    return;

  std::set<duk_uarridx_t> removalCandidates;
  enumerateEventArray([&](duk_uarridx_t index) -> bool {
    if (duk_is_undefined(_ctx, -1))
      return true;

    // stack: [ event-name <varargs> array listener-object ]
    if (getAssignedEvent() == event) {
      duk_get_prop_string(_ctx, -1, "once");
      bool once = duk_get_boolean(_ctx, -1) != 0;
      duk_pop(_ctx);

      // Create a callable stack sequence (duplicate parameters) and trigger the callback.
      duk_get_prop_string(_ctx, -1, "listener");
      duk_push_this(_ctx);
      for (duk_idx_t i = 0; i < argCount; ++i) {
        duk_dup(_ctx, i + 1);
      }

      // stack: [ event-name <varargs> array listener-object listener this-binding <varargs> ]
      duk_int_t result = duk_pcall_method(_ctx, argCount);
      if (result != DUK_EXEC_SUCCESS) {
        throwScriptingError(ScriptingError::Error, Utilities::format("callback failed, %s", duk_safe_to_string(_ctx, -1)));
      } else if (once) {
        removalCandidates.insert(index);
      }
      duk_pop(_ctx);
    }

    return true;
  });

  if (!removalCandidates.empty())
    removeListeners(key, event, removalCandidates);
}

//----------------------------------------------------------------------------------------------------------------------

void ScriptingContext::eventNames(EventEmitter const* emitter) const {
  duk_push_sprintf(_ctx, "%p", emitter);
  if (!loadEventsArray(false))
    return;

  std::set<std::string> names;
  enumerateEventArray([&](duk_idx_t) -> bool {
    names.insert(getAssignedEvent());

    return true;
  });

  duk_idx_t arrayIndex = duk_push_array(_ctx);
  duk_uarridx_t index = 0;
  for (auto &name : names) {
    duk_push_string(_ctx, name.c_str());
    duk_put_prop_index(_ctx, arrayIndex, index++);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ScriptingContext::eventListenerCount(EventEmitter const* emitter) const {
  std::string event = duk_get_string(_ctx, -1);

  duk_push_sprintf(_ctx, "%p", emitter);
  if (!loadEventsArray(false)) {
    duk_push_int(_ctx, 0);
    return;
  }

  duk_int_t count = 0;
  enumerateEventArray([&](duk_idx_t) -> bool {
    if (getAssignedEvent() == event)
      ++count;

    return true;
  });

  duk_push_int(_ctx, count);
}

//----------------------------------------------------------------------------------------------------------------------

void ScriptingContext::prependEventListener(EventEmitter const* emitter, bool once) const {
  // stack: [... event-name listener ]
  if (duk_is_function(_ctx, -1)) {
    std::string event = duk_to_string(_ctx, -2); // A string or a symbol. We coerce both to a string.
    duk_idx_t listenerIndex = duk_normalize_index(_ctx, -1);

    duk_push_sprintf(_ctx, "%p", emitter);
    std::string key = duk_get_string(_ctx, -1);
    loadEventsArray(true);

    duk_size_t length = duk_get_length(_ctx, -1);
    duk_idx_t sourceArrayIndex = duk_normalize_index(_ctx, -1);
    duk_idx_t targetArrayIndex = duk_push_array(_ctx);

    // Create new object in the new events array.
    duk_push_object(_ctx);
    duk_push_string(_ctx, event.c_str());
    duk_put_prop_string(_ctx, -2, "event");
    duk_push_boolean(_ctx, once);
    duk_put_prop_string(_ctx, -2, "once");
    duk_dup(_ctx, listenerIndex);
    duk_put_prop_string(_ctx, -2, "listener");
    duk_put_prop_index(_ctx, targetArrayIndex, 0);

    // Copy all other events.
    for (duk_uarridx_t index = 0; index < length; ++index) {
      duk_get_prop_index(_ctx, sourceArrayIndex, index);
      duk_put_prop_index(_ctx, targetArrayIndex, index + 1);
    }

    // Finally replace the original array with the new one.
    duk_push_global_stash(_ctx);
    duk_get_prop_string(_ctx, -1, "events");
    duk_remove(_ctx, -2);

    duk_swap_top(_ctx, -2);
    duk_put_prop_string(_ctx, -2, key.c_str());
  } else {
    throwScriptingError(ScriptingError::Error, "Listener value is not a callable function");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ScriptingContext::removeAllEventListeners(EventEmitter const* emitter) const {
  // stack: [ event-name ]
  std::string event;
  if (duk_get_top(_ctx) > 0 && duk_is_string(_ctx, 0)) { // The event name is optional.
    event = duk_get_string(_ctx, -1);
  }

  duk_push_sprintf(_ctx, "%p", emitter);
  std::string key = duk_get_string(_ctx, -1);
  duk_pop(_ctx);

  removeListeners(key, event, {});
}

//----------------------------------------------------------------------------------------------------------------------

void ScriptingContext::removeEventListener(EventEmitter const* emitter) const {
  // stack: [ event-name listener ]
  std::string event = duk_require_string(_ctx, -2);
  if (!duk_is_function(_ctx, -1)) {
    throwScriptingError(ScriptingError::Type, "callable required (stack index -1)");
    return;
  }

  duk_push_sprintf(_ctx, "%p", emitter);
  std::string key = duk_get_string(_ctx, -1);
  if (!loadEventsArray(false))
    return;

  std::set<duk_uarridx_t> indices;

  duk_normalize_index(_ctx, -1);
  enumerateEventArray([&](duk_uarridx_t index) -> bool {
    if (getAssignedEvent() != event)
      return true;

    duk_get_prop_string(_ctx, -1, "listener");
    if (duk_samevalue(_ctx, 1, -1)) {
      indices.insert(index);
      return false;
    }

    return true;
  });

  if (!indices.empty())
    removeListeners(key, "", indices);
}

//----------------------------------------------------------------------------------------------------------------------

std::string ScriptingContext::logOutput(const char *errorName) const {
  // stack: [ message? <varargs> ]
  format();

  if (errorName != nullptr) {
    duk_push_error_object(_ctx, DUK_ERR_ERROR, "%s", duk_require_string(_ctx, -1));
    duk_push_string(_ctx, "name");
    duk_push_string(_ctx, errorName);
    duk_def_prop(_ctx, -3, DUK_DEFPROP_FORCE | DUK_DEFPROP_HAVE_VALUE);
    duk_get_prop_string(_ctx, -1, "stack");
    getCleanedStackTrace();
  }

  size_t size;
  const char *text = duk_to_lstring(_ctx, -1, &size);
  std::string result(text, size);

  return result + "\n";
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * A very simple format function, similar to printf, which does not support any of the flags, width, precision or length
 * parameters that printf can handle.
 * Modeled after https://github.com/nodejs/node/blob/master/lib/util.js#L65.
 *
 * Expects all parameters on the stack and returns the formatted string as new TOS.
 */
void ScriptingContext::format() const {

  // Stack can vary here. There can be no format string or the format string doesn't contain (enough) format specifiers
  // for all parameters. In that case they are simply stringified and appended to the result string.
  // stack: [ format-string? <varargs> ]
  duk_int_t top = duk_get_top(_ctx);
  if (top == 0) {
    duk_push_string(_ctx, "");
    return;
  }

  std::string result;

  if (top == 1 || !duk_is_string(_ctx, 0)) {
    // No format string or no additional parameters.
    for (duk_int_t i = 0; i < top; ++i) {
      if (!result.empty())
        result += " ";
      result += duk_to_string(_ctx, i);
    }
  } else {
    std::string format = duk_get_string(_ctx, 0);
    duk_int_t currentArgument = 1;
    size_t lastPosition = 0;

    for (size_t i = 0; i < format.size(); ) {
      if (format[i] == '%' && (i + 1 < format.size())) {
        switch (format[i + 1]) {
          case 'd':
          case 'f':
            if (currentArgument >= top)
              break;
            if (lastPosition < i)
              result += format.substr(lastPosition, i - lastPosition);

            // Make the argument (might be a string, object, null, number etc.) into a number.
            // Then convert that to a string for the result.
            duk_to_number(_ctx, currentArgument);
            result += duk_to_string(_ctx, currentArgument++);
            lastPosition = i = i + 2;
            continue;

          case 'j':
            if (currentArgument >= top)
              break;
            if (lastPosition < i)
              result += format.substr(lastPosition, i - lastPosition);

            result += duk_json_encode(_ctx, currentArgument++);
            lastPosition = i = i + 2;
            continue;

          case 's':
            if (currentArgument >= top)
              break;
            if (lastPosition < i)
              result += format.substr(lastPosition, i - lastPosition);
            result += duk_get_string(_ctx, currentArgument++);
            lastPosition = i = i + 2;
            continue;

          case '%':
            if (lastPosition < i)
              result += format.substr(lastPosition, i - lastPosition);
            result += '%';
            lastPosition = i = i + 2;
            continue;
        }
      }
      ++i;
    }

    if (lastPosition == 0)
      result = format;
    else if (lastPosition < format.size())
      result += format.substr(lastPosition);

    while (currentArgument < top) {
      result += std::string(" ") + duk_to_string(_ctx, currentArgument++);
    }
  }
  
  duk_push_lstring(_ctx, result.c_str(), result.size());
}

//----------------------------------------------------------------------------------------------------------------------

static std::map<std::string, ModuleActivationFunction>& getInternalModules() {
  static std::map<std::string, ModuleActivationFunction> modules;

  return modules;
}

//----------------------------------------------------------------------------------------------------------------------

void ScriptingContext::registerModule(std::string const& name, ModuleActivationFunction activation) {
  getInternalModules()[name] = activation;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * On call there must be a stack trace string on the TOS. It will be processed and replaced by this function.
 * Some of the internal duktape flags and all C++ code references are removed.
 */
void ScriptingContext::getCleanedStackTrace() const {
  std::string error = duk_safe_to_string(_ctx, -1);
  duk_pop(_ctx); // Remove stack trace from stack.

  // Clean up the stack trace a bit. There's some stuff in it that isn't interesting for JS.
  std::vector<std::string> lines = Utilities::split(error);
  lines.erase(std::remove_if(lines.begin(), lines.end(), [](std::string const& line) {
    return (line.find(".cpp") != std::string::npos) || (line.find("duktape") != std::string::npos);
  }), lines.end());

  for (auto &line : lines) {
    size_t pos = line.find("native");
    if (pos != std::string::npos)
      line = line.substr(0, pos + 7);
    else {
      pos = line.find_last_of(")");
      if (pos != std::string::npos)
        line = line.substr(0, pos + 1);
      else {
        pos = line.find("light");
        if (pos != std::string::npos)
          line = line.substr(0, pos + 1);
      }
    }
  }
  error = Utilities::concat(lines);
  duk_push_string(_ctx, error.c_str());
}

//----------------------------------------------------------------------------------------------------------------------

void ScriptingContext::pushValue(std::map<std::string, std::string> const& map) const {
  duk_idx_t objectIndex = duk_push_object(_ctx);
  for (auto &entry : map) {
    duk_push_string(_ctx, entry.second.c_str());
    duk_put_prop_string(_ctx, objectIndex, entry.first.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ScriptingContext::pushValue(std::vector<std::string> const& array) const {
  duk_idx_t arrayIndex = duk_push_array(_ctx);
  for (duk_uarridx_t i = 0; i < array.size(); ++i) {
    duk_push_string(_ctx, array[i].c_str());
    duk_put_prop_index(_ctx, arrayIndex, i);
  }
}

//----------------------------------------------------------------------------------------------------------------------

static size_t nextStashId = 0;

/**
 * Pins a value by copying a reference to the global stash, and returns a key for later access.
 */
std::string ScriptingContext::stashValue(duk_idx_t index) const {
  index = duk_normalize_index(_ctx, index);

  duk_push_global_stash(_ctx);
  std::string result = std::to_string(nextStashId++);
  duk_push_string(_ctx, result.c_str());
  duk_dup(_ctx, index);
  duk_put_prop(_ctx, -3);

  duk_pop(_ctx); // Remove global stash.

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Removes a previously stashed value.
 */
void ScriptingContext::unstashValue(std::string const& id) const {
  duk_push_global_stash(_ctx);
  duk_del_prop_string(_ctx, -1, id.c_str());
  duk_pop(_ctx);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Creates an execution block for a stashed value.
 */
void ScriptingContext::usingStashedValue(std::string const& name, std::function<void ()> callback) const {
  duk_push_global_stash(_ctx);
  duk_get_prop_string(_ctx, -1, name.c_str());
  if (duk_is_undefined(_ctx, -1))
    throwScriptingError(ScriptingError::Error, "Attempt to access unknown stashed object");

  callback();

  duk_pop_2(_ctx);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the native reference for the TOS object (if there's any set).
 */
void* ScriptingContext::getNativeReference() const {
  if (!duk_is_object(_ctx, -1)) // Can be object, array, function, buffer.
    return nullptr;

  void* result = nullptr;
  duk_get_prop_string(_ctx, -1, DUK_HIDDEN_SYMBOL("data"));
  if (duk_is_pointer(_ctx, -1))
    result = duk_get_pointer(_ctx, -1);
  duk_pop(_ctx);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Stores a native reference in the TOS object.
 */
void ScriptingContext::setNativeReference(void *reference) const {
  if (!duk_is_object(_ctx, -1))
    return;

  duk_push_pointer(_ctx, reference);
  duk_put_prop_string(_ctx, -2, DUK_HIDDEN_SYMBOL("data"));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Executes the callback with the given id and returns a status for that.
 */
void ScriptingContext::executeCallback(std::string const& id) const {
  duk_int_t result;
  usingStashedValue("callbacks", [&]() {
    duk_get_prop_string(_ctx, -1, id.c_str());
    if (duk_is_undefined(_ctx, -1))
      throw std::runtime_error("Internal error: couldn't find the callback with the id: " + id);

    duk_get_prop_string(_ctx, -1, "arguments");
    duk_idx_t argumentsIndex = duk_normalize_index(_ctx, -1);
    duk_uarridx_t argCount = static_cast<duk_uarridx_t>(duk_get_length(_ctx, -1));
    duk_get_prop_string(_ctx, -2, "callback");

    for (duk_uarridx_t i = 0; i < argCount; ++i)
      duk_get_prop_index(_ctx, argumentsIndex, i);

    // stack: [... callback-object callback <varargs> ]
    result = duk_pcall(_ctx, static_cast<duk_idx_t>(argCount));
    if (result != DUK_EXEC_SUCCESS)
      checkForErrors();

    duk_pop_3(_ctx); // Remove result from pcall, the arguments array and the callback-object.
  });
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Loads the list of events for a given emitter and returns true if that was successfull.
 * If the array doesn't exist yet and `create` is true it will be created.
 * The id for the emitter is on the TOS on enter.
 *
 * On return nothing changed on the stack if the result is false, otherwise the array replaces the TOS element.
 */
bool ScriptingContext::loadEventsArray(bool create) const {
  // The event callback cache is organized as an object with keys coming from native event emitter instances and
  // values being a single list of listener objects (members: event name, once flag, actual listener).
  duk_push_global_stash(_ctx);
  duk_get_prop_string(_ctx, -1, "events");
  duk_remove(_ctx, -2); // Remove stash.
  duk_dup(_ctx, -2); // Duplicate emitter id.

  duk_get_prop(_ctx, -2);
  if (!duk_is_array(_ctx, -1)) {
    duk_pop(_ctx); // Remove invalid value.
    if (!create) {
      duk_pop(_ctx); // Remove also "events".
      return false;
    }

    // Create new array, duplicate it as result and store it under the emitter id in the events object.
    duk_push_array(_ctx);
    duk_dup(_ctx, -3); // Duplicate id again.
    duk_dup(_ctx, -2);
    duk_put_prop(_ctx, -4);

    // The array is now on the top.
  }

  duk_remove(_ctx, -2); // Remove "events" object ...
  duk_remove(_ctx, -2); // and the original emitter id.

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the value of the "event" property of the TOS object.
 */
std::string ScriptingContext::getAssignedEvent() const {
  duk_get_prop_string(_ctx, -1, "event");
  std::string event = duk_require_string(_ctx, -1);
  duk_pop(_ctx);

  return event;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Iterates over the array on the TOS by loading each of its objects and then calls the callback for it.
 *
 * The callback should return true if it wants the iteration to continue, otherwise the iteration is interrupted.
 *
 * Note: the callback must not change the stack before the TOS on its invocation in order to be able to continue
 *       the iteration here.
 */
void ScriptingContext::enumerateEventArray(std::function<bool (duk_idx_t)> callback) const {
  duk_idx_t length = static_cast<duk_idx_t>(duk_get_length(_ctx, -1));
  duk_idx_t arrayIndex = duk_normalize_index(_ctx, -1);

  for (duk_idx_t i = 0; i < length; ++i) {
    duk_get_prop_index(_ctx, arrayIndex, static_cast<duk_uarridx_t>(i));
    if (!callback(i)) {
      duk_pop(_ctx);
      break;
    }

    duk_pop(_ctx);
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Removes listeners from the list for the given id. The indices directly address the entries in the list
 * without considering which event they are assigned to. If given the event parameter is ignored.
 * If the list of indices is empty all listeners for the given event are removed or, if that is empty, the entire
 * listener list is cleared.
 *
 * After removal all remaining entries are reindexed, that is, they are moved to the proper array object keys.
 */
void ScriptingContext::removeListeners(std::string const& id, std::string const& event,
                                       std::set<duk_uarridx_t> indices) const {

  // Removal is implemented by creating a new array with the remaining elements and replacing the listener entry
  // for the given id with that.
  duk_push_global_stash(_ctx);
  duk_get_prop_string(_ctx, -1, "events");
  duk_remove(_ctx, -2);

  duk_int_t eventsIndex = duk_normalize_index(_ctx, -1);
  if (duk_is_undefined(_ctx, -1)) {
    duk_pop(_ctx);
    return;
  }

  duk_get_prop_string(_ctx, eventsIndex, id.c_str());
  duk_int_t sourceArrayIndex = duk_normalize_index(_ctx, -1);
  if (!duk_is_array(_ctx, sourceArrayIndex)) {
    duk_pop_2(_ctx);
    return;
  }

  // stack: [ ... events-object events-list ]
  if (indices.empty()) {
    if (event.empty()) {
      duk_pop(_ctx);
      duk_del_prop_string(_ctx, eventsIndex, id.c_str());

      duk_pop(_ctx);
      return;
    }

    enumerateEventArray([&](duk_uarridx_t index) -> bool {
      if (getAssignedEvent() == event)
        indices.insert(index);

      return true;
    });

  }

  if (!indices.empty()) {
    duk_size_t length = duk_get_length(_ctx, -1);

    // Create a new array as target.
    duk_int_t targetArrayIndex = duk_push_array(_ctx);

    duk_uarridx_t targetIndex = 0;
    for (duk_uarridx_t index = 0; index < length; ++index) {
      if (indices.count(index) == 0) {
        duk_get_prop_index(_ctx, sourceArrayIndex, index);
        duk_put_prop_index(_ctx, targetArrayIndex, targetIndex++);
      }
    }

    // Finally replace the stored array.
    duk_put_prop_string(_ctx, eventsIndex, id.c_str());
  }

  duk_pop_2(_ctx);
}

//----------------------------------------------------------------------------------------------------------------------

ScriptingContext* ScriptingContext::fromDuktapeContext(duk_context *ctx) {
  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, "context");
  auto context = reinterpret_cast<ScriptingContext *>(duk_get_pointer(ctx, -1));
  duk_pop_2(ctx);

  return context;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * See if the given path can be resolved to an existing file we support.
 * Returns the resolved name if something was found, otherwise an empty string.
 */
static std::string resolveFile(std::string const& path) {
  if (FS::isFile(path))
    return path;

  if (FS::isFile(path + ".js"))
    return path + ".js";

  if (FS::isFile(path + ".json"))
    return path + ".json";

  return "";
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * See if the given path contains an index file (either .js or .json).
 * Returns the resolved name if something was found, otherwise an empty string.
 */
static std::string resolveIndex(std::string const& path) {
  if (!FS::isDir(path))
    return "";

  std::string temp = Path::join({ path, "index.js" });
  if (FS::isFile(temp))
    return temp;

  temp = Path::join({ path, "index.json" });
  if (FS::isFile(temp))
    return temp;

  return "";
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * See if given path can be resolved to a folder containing either a package.json file with a reference to the file to
 * load or an index (.js/.json) file.
 * Returns the resolved name if something was found, otherwise an empty string.
 */
std::string ScriptingContext::resolveFolder(ScriptingContext *context, std::string const& path) {
  if (!FS::isDir(path))
    return "";

  std::string packagePath = Path::join({ path, "package.json" });
  if (FS::isFile(packagePath)) {
    JSObject json = context->loadJsonFile(packagePath);
    std::string mainPath = json.get("main", "");
    if (!mainPath.empty()) {
      mainPath = Path::join({ path, mainPath });
      std::string main = resolveFile(mainPath);
      if (!main.empty())
        return main;

      main = resolveIndex(mainPath);
      if (!main.empty())
        return main;
    }

    // If there is no "main" entry or it couldn't be resolved continue with other processing.
  }

  return resolveIndex(path);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Collects a list of possible node_modules folders.
 */
std::set<std::string> ScriptingContext::moduleFolders(std::string const& path) {
  std::set<std::string> result;
  std::vector<std::string> parts = Utilities::splitBySet(path, R"(/\)");

  for (ssize_t i = static_cast<ssize_t>(parts.size()) - 1; i >= 0; --i) {
    if (parts[static_cast<size_t>(i)] == "node_modules")
      continue;

    std::vector<std::string> slice(parts.begin(), parts.begin() + i);
    slice.push_back("node_modules");
    result.insert(Path::join(slice));
  }
  result.insert("node_modules");

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Part of the module loading machinery. JS interfacing is done by the duk_module_node code.
 * But we have to do the file work here. On the stack we get the value passed to `require()` as a "module ID" and
 * the ID of the calling script (which is empty for the main script).
 */
duk_ret_t ScriptingContext::resolveModule(duk_context *ctx) {
  // stack: [ requested_id parent_id ]
  std::string requestedID = duk_get_string(ctx, 0);
  std::string callingID = duk_get_string(ctx, 1);
  std::string parentPath = FS::isDir(callingID) ? callingID : Path::dirname(callingID);

  // Module resolution strategy in Node.js style: https://nodejs.org/api/modules.html#modules_all_together
  auto modules = getInternalModules();
  if (modules.find(requestedID) != modules.end()) {
    duk_push_string(ctx, requestedID.c_str());
    return 1;
  }

  ScriptingContext *context = ScriptingContext::fromDuktapeContext(ctx);
  std::string resolvedID;
  std::string cwd = Process::cwd();

  try {
    if (Path::isAbsolute(requestedID) || Utilities::hasPrefix(requestedID, ".")) {
      std::string temp;
      if (Path::isAbsolute(requestedID)) {
        temp = Path::relative(cwd, requestedID);
      } else
        temp = Path::join({ parentPath, requestedID });

      resolvedID = resolveFile(temp);
      if (resolvedID.empty())
        resolvedID = resolveFolder(context, temp);
    }
  } catch (std::runtime_error &e) {
    // Triggered for parse errors in package.json.
    context->throwScriptingError(ScriptingError::Syntax, e.what());
    return 0;
  }

  // No files found so far. Check node modules.
  if (resolvedID.empty()) {
    for (auto &folder : moduleFolders(parentPath)) {
      std::string path = Path::join({ folder, requestedID });
      std::string temp = resolveFile(path);
      if (!temp.empty()) {
        resolvedID = temp;
        break;
      }

      temp = resolveFolder(context, path);
      if (!temp.empty()) {
        resolvedID = temp;
        break;
      }
    }
  }

  // Still not found? Last attempt: include folders.
  if (resolvedID.empty()) {
    duk_get_global_string(ctx, "mga");
    duk_get_prop_string(ctx, -1, "config");
    duk_get_prop_string(ctx, -1, "includes");

    if (duk_is_array(ctx, -1)) {
      duk_uarridx_t length = static_cast<duk_uarridx_t>(duk_get_length(ctx, -1));

      for (duk_uarridx_t i = 0; i < length; ++i) {
        duk_get_prop_index(ctx, -1, i);
        std::string include = duk_require_string(ctx, -1);
        duk_pop(ctx);

        std::string path = Path::join({ include, requestedID });
        std::string temp = resolveFile(path);
        if (!temp.empty()) {
          resolvedID = Path::relative(cwd, temp);
          break;
        }

        temp = resolveFolder(context, path);
        if (!temp.empty()) {
          resolvedID = Path::relative(cwd, temp);
          break;
        }
      }
    }
    duk_pop_3(ctx);
  }

  if (resolvedID.empty()) {
    context->throwScriptingError(ScriptingError::Error, Utilities::format("Cannot resolve module %s", requestedID.c_str()));
    return 0;
  }

  duk_push_string(ctx, resolvedID.c_str());
  return 1;  // Use result on stack.
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Here we actually load the file content depending on the file type.
 */
duk_ret_t ScriptingContext::loadModule(duk_context *ctx) {
  // stack: [ resolved_id exports module ]
  std::string requestedID = duk_get_string(ctx, -3);

  auto context = fromDuktapeContext(ctx);

  auto modules = getInternalModules();
  auto iterator = modules.find(requestedID);
  if (iterator != modules.end()) {
    // Activate the module. Must return a module object to export.
    JSObject exports(context, -2);
    iterator->second(*context, exports);

    return 0;
  }

  std::string extension = Utilities::toLower(Path::extname(requestedID));
  duk_dup(ctx, -3);
  duk_put_prop_string(ctx, -2, "filename");
  try {
    std::string text = FS::contentFromFile(requestedID);
    duk_push_string(ctx, text.c_str());
    if (extension == ".json")
      duk_json_decode(ctx, -1);

    return 1; // Use result on stack.
  } catch (std::runtime_error &e) {
    context->throwScriptingError(ScriptingError::Uri,
                                 Utilities::format("Cannot load module %s, %s", requestedID.c_str(), e.what()));

    return 0; // Undefined.
  }
}

//----------------------------------------------------------------------------------------------------------------------

