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

#include "types.h"
#include "utilities.h"
#include "platform.h"

#include "scripting-context.h"
#include "jsexport.h"

using namespace mga;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Special dump function for functions (which must be on the TOS). No recursion takes place here.
 */
static void dumpFunction(duk_context *ctx, std::ostream &target) {
  duk_get_prop_string(ctx, -1, "length");
  duk_int_t length = duk_get_int(ctx, -1);
  duk_pop(ctx);

  std::string name;
  duk_get_prop_string(ctx, -1, "name");
  if (!duk_is_undefined(ctx, -1))
    name = duk_get_string(ctx, -1);
  else
    if (duk_is_array(ctx, -1))
      name = "Array";
  if (name.empty())
    name = "<native>";
  duk_pop(ctx);

  target << name << "(";
  if (length == 0)
    target << " varags or 0 parameters)";
  else
    target << length << (length == 1 ? " parameter" : " parameters") << ")";
  target << std::endl;
}

//----------------------------------------------------------------------------------------------------------------------

static void recursiveDump(duk_context *ctx, size_t level, size_t maxLevel, std::ostream &target,
                          std::set<void *> &visited, unsigned enumFlags) {

  std::string indentation(2 * level, ' ');
  if (maxLevel > 0 && level > maxLevel) {
    target << indentation << "..." << std::endl;
    return;
  }

  void *p = duk_get_heapptr(ctx, -1);
  if (p == nullptr || visited.count(p) > 0)
    return;

  visited.insert(p);

  duk_enum(ctx, -1, enumFlags);

  while (duk_next(ctx, -1, 1)) {

    // stack [ ... enum key value ]
    std::string key = duk_get_string(ctx, -2);

    target << indentation << key << ": ";
    if (duk_is_function(ctx, -1)) // Includes constructors. TODO: what about properties on functions?
      //if (key == "constructor")
      dumpFunction(ctx, target);
    else {
      switch (duk_get_type(ctx, -1)) {
        case DUK_TYPE_STRING:
          target << "\"" << duk_to_string(ctx, -1) << "\"" << std::endl;
          break;
        case DUK_TYPE_OBJECT:
          if (duk_is_array(ctx, -1)) {
            target << "[" << std::endl;
            recursiveDump(ctx, level + 1, maxLevel, target, visited, enumFlags);
            target << indentation << "]" << std::endl;
          } else {
            target << "{" << std::endl;
            recursiveDump(ctx, level + 1, maxLevel, target, visited, enumFlags);
            target << indentation << "}" << std::endl;
          }
          break;
        default:
          target << duk_to_string(ctx, -1) << std::endl;
          break;
      }
    }

    duk_pop_2(ctx);
  }

  duk_pop(ctx); // Remove the enumerator.

}

//----------------- JSValueBase ----------------------------------------------------------------------------------------

JSValueBase::JSValueBase() {
}

//----------------------------------------------------------------------------------------------------------------------

JSValueBase::JSValueBase(ScriptingContext *context) : _context(context) {
}

//----------------------------------------------------------------------------------------------------------------------

JSValueBase::JSValueBase(ScriptingContext *context, duk_idx_t index) : _context(context) {
  initialize(index);
}

//----------------------------------------------------------------------------------------------------------------------

JSValueBase::JSValueBase(JSValueBase const& other) {
  if (other.isValid()) {
    _context = other._context;

    // Make a copy of the stashed value under a new id.
    _context->usingStashedValue(other._id, [&]() {
      duk_dup_top(_context->_ctx);
      _id = _context->stashValue(-1);
      duk_pop(_context->_ctx);
    });
  }
}

//----------------------------------------------------------------------------------------------------------------------

JSValueBase::JSValueBase(JSValueBase&& other) {
  if (other.isValid()) {
    _id = other._id;
    _context = other._context;

    other._context = nullptr;
  }
}

//----------------------------------------------------------------------------------------------------------------------

JSValueBase::~JSValueBase() {
  if (_context != nullptr)
  _context->unstashValue(_id);
}

//----------------------------------------------------------------------------------------------------------------------

JSValueBase JSValueBase::operator = (JSValueBase const& other) {
  if (_id == other._id)
    return *this;

  // Values cannot be copied/moved accross contexts, but we can change validity of this value.
  assert(_context == nullptr || other._context == nullptr || _context == other._context);

  if (_context != nullptr && !_id.empty())
    _context->unstashValue(_id);

  _context = other._context; // In case we haven't had a context yet.
  if (other.isValid()) {
    _context->usingStashedValue(other._id, [&]() {
      _id = _context->stashValue(-1);
    });
  }

  return *this;
}

//----------------------------------------------------------------------------------------------------------------------

JSValueBase JSValueBase::operator = (JSValueBase&& other) {
  if (_id == other._id)
    return *this;

  if (_context != nullptr)
    _context->unstashValue(_id);

  if (other.isValid()) {
    _id = other._id;
    _context = other._context;
    other._context = nullptr;
  } else {
    _context = nullptr;
  }

  return *this;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Pushes a reference to the managed object on the TOS. This is only useful when using the value as parameter
 * in value/property setters or as a result.
 */
void JSValueBase::push() const {
  checkValidity();

  duk_push_global_stash(_context->_ctx);
  duk_get_prop_string(_context->_ctx, -1, _id.c_str());
  duk_remove(_context->_ctx, -2); // Remove the stash from the stack.
}

//----------------------------------------------------------------------------------------------------------------------

std::string JSValueBase::dumpObject(bool showHidden, size_t maxDepth) const {
  std::string result;

  _context->usingStashedValue(_id, [&]() {
    unsigned enumFlags = DUK_ENUM_INCLUDE_SYMBOLS;
    if (showHidden)
      enumFlags |= DUK_ENUM_INCLUDE_NONENUMERABLE | DUK_ENUM_INCLUDE_HIDDEN;

    std::stringstream stream;
    std::set<void *> visited;

    bool isArray = duk_is_array(_context->_ctx, -1) != 0;
    bool isObject = !isArray && duk_is_object(_context->_ctx, -1);

    if (isArray)
      stream << "[" << std::endl;
    else if (isObject)
      stream << "{" << std::endl;

    recursiveDump(_context->_ctx, (isArray || isObject) ? 1 : 0, maxDepth, stream, visited, enumFlags);

    if (isArray)
      stream << "]" << std::endl;
    else if (isObject)
      stream << "}" << std::endl;
    
    result = stream.str();
  });  

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

std::string JSValues::dumpObject(size_t index, bool showHidden, size_t maxDepth) const {
  duk_dup(_context->_ctx, static_cast<duk_idx_t>(index));
  std::string result = JSArray(_context, -1).dumpObject(showHidden, maxDepth);
  duk_pop(_context->_ctx);
  
  return result;
}
//----------------------------------------------------------------------------------------------------------------------

void JSValueBase::initialize(duk_idx_t index) {
  if (_context != nullptr) {
    _id = _context->stashValue(index);
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Examine the TOS to see if that is of the expected type.
 */
static bool isType(duk_context *ctx, ValueType type) {
  switch (type) {
    case ValueType::Undefined:
      return duk_is_undefined(ctx, -1) != 0;
    case ValueType::Null:
      return duk_is_null(ctx, -1) != 0;
    case ValueType::Int:
    case ValueType::UInt:
    case ValueType::Double:
      return duk_is_number(ctx, -1) != 0;

    case ValueType::String:
      return duk_is_string(ctx, -1) != 0;

    case ValueType::Boolean:
      return duk_is_boolean(ctx, -1) != 0;

    case ValueType::Object:
    case ValueType::Json:
      return duk_is_object(ctx, -1) != 0;

    case ValueType::Array:
      return duk_is_array(ctx, -1) != 0;

    case ValueType::Pointer:
      return duk_is_pointer(ctx, -1) != 0;

    default:
      return false;
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Determines whether the value at the given property (or array index if property is empty) is of the required type.
 */
bool JSValueBase::isValueType(std::string const& property, duk_uarridx_t index, ValueType type) const {
  checkValidity();

  bool result = false;
  _context->usingStashedValue(_id, [&]() {
    if (property.empty()) {
      duk_uarridx_t length = static_cast<duk_uarridx_t>(duk_get_length(_context->_ctx, -1));
      if (index >= length)
        throw std::runtime_error("Array index out of range");

      duk_get_prop_index(_context->_ctx, -1, index);
    } else
      duk_get_prop_string(_context->_ctx, -1, property.c_str());

    result = isType(_context->_ctx, type);

    duk_pop(_context->_ctx);
  });

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * A dedicated checker for number types (which is in JS just that single type).
 */
bool JSValueBase::isNumberType(std::string const& property, duk_uarridx_t index) const {
  checkValidity();

  bool result = false;
  _context->usingStashedValue(_id, [&]() {
    if (property.empty()) {
      duk_uarridx_t length = static_cast<duk_uarridx_t>(duk_get_length(_context->_ctx, -1));
      if (index >= length)
        throw std::runtime_error("Array index out of range");

      duk_get_prop_index(_context->_ctx, -1, index);
    } else
      duk_get_prop_string(_context->_ctx, -1, property.c_str());

    result = duk_is_number(_context->_ctx, -1) != 0;

    duk_pop(_context->_ctx);
  });

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant JSValueBase::getValue(std::string const& property, duk_uarridx_t index, ValueType type, bool coerced,
                                JSVariant const& defaultValue) const {
  checkValidity();

  JSVariant result(defaultValue);
  _context->usingStashedValue(_id, [&]() {
    if (property.empty()) {
      duk_uarridx_t length = static_cast<duk_uarridx_t>(duk_get_length(_context->_ctx, -1));
      if (index >= length)
        throw std::runtime_error("Array index out of range");

      duk_get_prop_index(_context->_ctx, -1, index);
    } else
      duk_get_prop_string(_context->_ctx, -1, property.c_str());

    result = JSValueBase::getValue(_context, type, coerced, defaultValue);

    duk_pop(_context->_ctx);
  });

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void JSValueBase::checkValidity() const {
  if (_context == nullptr || _id.empty())
    throw std::runtime_error("Attempt to access an invalid JS object");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Get value from the TOS as the specified type. If Undefined is specified as type return whatever exists.
 * No coercion takes place in this case, nor does the require parameter has any effect.
 */
JSVariant JSValueBase::getValue(ScriptingContext *context, ValueType type, bool coerced,
                                JSVariant const& defaultValue) {
  switch (type) {
    case ValueType::Undefined: {
      switch (duk_get_type(context->_ctx, -1)) {
        case DUK_TYPE_BOOLEAN:
          return duk_get_boolean(context->_ctx, -1) != 0;
        case DUK_TYPE_NUMBER: {
          double result = duk_get_number(context->_ctx, -1);
          if (result == static_cast<int>(result))
            return static_cast<int>(result);
          if (result == static_cast<unsigned int>(result))
            return static_cast<unsigned int>(result);
          return result;
        }
        case DUK_TYPE_STRING: {
          size_t size;
          const char *text = duk_get_lstring(context->_ctx, -1, &size);
          return std::string(text, size);
        }
        case DUK_TYPE_OBJECT:
          if (duk_is_array(context->_ctx, -1)) {
            return JSArray(context, -1);
          } else {
            return JSObject(context, -1);
          }
        case DUK_TYPE_POINTER:
          return duk_get_pointer(context->_ctx, -1);
        case DUK_TYPE_NULL:
          return nullptr;

        default: // DUK_TYPE_UNDEFINED, DUK_TYPE_BUFFER, DUK_TYPE_LIGHTFUNC
          return defaultValue;
      }

      break;
    }

    case ValueType::Int:
      if (coerced)
        return duk_to_int(context->_ctx, -1);
      else if (duk_is_number(context->_ctx, -1))
        return duk_get_int(context->_ctx, -1);
      break;

    case ValueType::UInt:
      if (coerced)
        return duk_to_uint(context->_ctx, -1);
      else if (duk_is_number(context->_ctx, -1))
        return duk_get_uint(context->_ctx, -1);
      break;

    case ValueType::Double:
      if (coerced)
        return duk_to_number(context->_ctx, -1);
      else if (duk_is_number(context->_ctx, -1))
        return duk_get_number(context->_ctx, -1);
      break;

    case ValueType::String: {
      size_t size = 0;
      const char *text = nullptr;

      if (coerced)
        text = duk_to_lstring(context->_ctx, -1, &size);
      else if (duk_is_string(context->_ctx, -1))
        text = duk_get_lstring(context->_ctx, -1, &size);

      if (text != nullptr)
        return std::string(text, size);
      break;
    }

    case ValueType::Boolean:
      if (coerced)
        return duk_to_boolean(context->_ctx, -1) != 0;
      else if (duk_is_boolean(context->_ctx, -1))
        return duk_get_boolean(context->_ctx, -1) != 0;
      break;

    case ValueType::Object: {
      if (duk_is_object(context->_ctx, -1)) {
        return JSObject(context, -1);
      }
      break;
    }

    case ValueType::Array: {
      if (duk_is_array(context->_ctx, -1)) {
        return JSArray(context, -1);
      }
      break;
    }

    case ValueType::Pointer:
      if (coerced)
        return duk_to_pointer(context->_ctx, -1);
      else if (duk_is_pointer(context->_ctx, -1))
        return duk_get_pointer(context->_ctx, -1);
      break;

    case ValueType::Json: {
      if (duk_is_object(context->_ctx, -1)) {
        return duk_json_encode(context->_ctx, -1);
      }
      break;
    }

    case ValueType::Null: // Makes no sense.
      assert(false);
      break;
  }

  return defaultValue;
}

//----------------- JSObject -------------------------------------------------------------------------------------------

std::map<void *, std::map<std::string, FunctionCallback>> instanceFunctionMap;

using Accessors = struct {
  PropertyGetter getter;
  PropertySetter setter;
};
std::map<void *, std::map<std::string, Accessors>> instancePropertyMap;

//----------------------------------------------------------------------------------------------------------------------

JSObject::JSObject() : JSValueBase() {
}

//----------------------------------------------------------------------------------------------------------------------

JSObject::JSObject(ScriptingContext *context) : JSValueBase(context) {
  duk_push_object(context->_ctx);
  initialize(-1);
  duk_pop(context->_ctx);
}

//----------------------------------------------------------------------------------------------------------------------

JSObject::JSObject(ScriptingContext *context, duk_idx_t index) : JSValueBase(context, index) {
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the native C++ reference associated with this JS object (if there's any).
 */
JSExport * JSObject::getBacking() const {
  checkValidity();

  JSExport *result;
  _context->usingStashedValue(_id, [&]() {
    result = reinterpret_cast<JSExport *>(_context->getNativeReference());
  });

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void JSObject::setBacking(JSExport *object) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    _context->setNativeReference(object);
  });
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns all property names defined in this object (not inherited ones).
 */
std::set<std::string> JSObject::getPropertyKeys() const {
  std::set<std::string> result;

  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    duk_enum(_context->_ctx,  -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    while (duk_next(_context->_ctx, -1, false)) {
      result.insert(duk_safe_to_string(_context->_ctx, -1));
      duk_pop(_context->_ctx);
    }
    duk_pop(_context->_ctx);
  });

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

bool JSObject::is(ValueType type, std::string const& property) const {
  return isValueType(property, 0, type);
}

//----------------------------------------------------------------------------------------------------------------------

bool JSObject::isNumber(std::string const& property) const {
  return isNumberType(property, 0);
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant JSObject::as(ValueType type, std::string const& property) const {
  return getValue(property, 0, type, true, JSVariant());
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant JSObject::get(std::string const& property) const {
  return getValue(property, 0, ValueType::Undefined, false, JSVariant());
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant JSObject::get(std::string const& property, JSVariant const& defaultValue) const {
  return getValue(property, 0, ValueType::Undefined, false, defaultValue);
}

//----------------------------------------------------------------------------------------------------------------------

void JSObject::set(std::string const& property, JSVariant const& value) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    value.pushValue(_context);
    duk_put_prop_string(_context->_ctx, -2, property.c_str());
  });
}

//----------------------------------------------------------------------------------------------------------------------

void JSObject::defineProperty(std::string const& name, JSVariant const& value, bool readOnly) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    duk_push_string(_context->_ctx, name.c_str());
    value.pushValue(_context);

    duk_uint_t flags = DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE;
    if (readOnly)
      flags |= (DUK_READ_ONLY);
    duk_def_prop(_context->_ctx, -3, flags);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void JSObject::defineProperty(std::string const& name, std::map<std::string, std::string> const& value,
                              bool readOnly) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    duk_push_string(_context->_ctx, name.c_str());

    duk_idx_t objectIndex = duk_push_object(_context->_ctx);
    for (auto &entry : value) {
      duk_push_string(_context->_ctx, entry.second.c_str());
      duk_put_prop_string(_context->_ctx, objectIndex, entry.first.c_str());
    }

    duk_uint_t flags = DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE;
    if (readOnly)
      flags |= DUK_READ_ONLY;
    duk_def_prop(_context->_ctx, -3, flags);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void JSObject::defineProperty(std::string const& name, std::vector<std::string> const& value, bool readOnly) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    duk_push_string(_context->_ctx, name.c_str());

    duk_idx_t arrayIndex = duk_push_array(_context->_ctx);
    for (duk_uarridx_t i = 0; i < value.size(); ++i) {
      duk_push_string(_context->_ctx, value[i].c_str());
      duk_put_prop_index(_context->_ctx, arrayIndex, i);
    }

    duk_uint_t flags = DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE;
    if (readOnly)
      flags |= DUK_READ_ONLY;
    duk_def_prop(_context->_ctx, -3, flags);
  });
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Creates a virtual property with the given name (and key) + with the setter/getter. Setter and getter are optional
 * (pass nullptr if not wanted). If both are null you should set a value explicitely.
 */
void JSObject::defineVirtualProperty(std::string const& name, PropertyGetter getter, PropertySetter setter) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    duk_idx_t index = duk_normalize_index(_context->_ctx, -1);

    // Bind the property to the current instance (via its heap pointer).
    void *ptr = duk_get_heapptr(_context->_ctx, -1);
    auto &propertyMap = instancePropertyMap[ptr];

    if (propertyMap.find(name) != propertyMap.end())
      throw std::runtime_error("Duplicate property name '" + name + "'");
    propertyMap[name] = { getter, setter };

    duk_uint_t flags = DUK_DEFPROP_SET_ENUMERABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE;
    duk_push_string(_context->_ctx, name.c_str());

    if (getter != nullptr) {
      flags |= DUK_DEFPROP_HAVE_GETTER;
      duk_push_c_function(_context->_ctx, dispatchGetProperty, 1);

      duk_push_string(_context->_ctx, "ptr");
      duk_push_pointer(_context->_ctx, ptr);
      duk_def_prop(_context->_ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_READ_ONLY);
    }

    if (setter != nullptr) {
      flags |= DUK_DEFPROP_HAVE_SETTER;
      duk_push_c_function(_context->_ctx, dispatchSetProperty, 2);

      duk_push_string(_context->_ctx, "ptr");
      duk_push_pointer(_context->_ctx, ptr);
      duk_def_prop(_context->_ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_READ_ONLY);
    }

    duk_def_prop(_context->_ctx, index, flags);
  });
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * A virtual array is a special virtual property where not the property itself is backed by getters and setters but
 * the individual elements.
 */
void JSObject::defineVirtualArrayProperty(std::string const& name, StringArrayRef const& array, bool readOnly) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    duk_get_global_string(_context->_ctx, "Proxy");

    duk_idx_t objectIndex = duk_push_array(_context->_ctx);
    {
      _context->setNativeReference(new StringArrayRef(array));
      duk_push_c_function(_context->_ctx, stringArrayFinalizer, 1);
      duk_set_finalizer(_context->_ctx, objectIndex);
    }

    duk_idx_t handlerIndex = duk_push_object(_context->_ctx);
    {
      duk_push_c_lightfunc(_context->_ctx, stringArrayGetter, 3, 0, 0);
      duk_put_prop_string(_context->_ctx, handlerIndex, "get");
      duk_push_c_lightfunc(_context->_ctx, stringArraySetter, 4, 0, 0);
      duk_put_prop_string(_context->_ctx, handlerIndex, "set");
      duk_push_c_lightfunc(_context->_ctx, stringArrayOwnKeys, 1, 0, 0);
      duk_put_prop_string(_context->_ctx, handlerIndex, "ownKeys");
    }

    duk_new(_context->_ctx, 2);
    duk_put_prop_string(_context->_ctx, -2, name.c_str());

    if (readOnly) {
      duk_push_string(_context->_ctx, name.c_str());
      duk_def_prop(_context->_ctx, -2, DUK_READ_ONLY);
    }
  });
}

//----------------------------------------------------------------------------------------------------------------------

void JSObject::defineEnum(std::string const& name, ObjectDefCallback callback, bool readOnly) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    duk_push_string(_context->_ctx, name.c_str());

    JSObject object(_context);
    callback(object);

    object.push();

    duk_uint_t flags = DUK_DEFPROP_HAVE_VALUE;
    if (readOnly)
      flags |= DUK_READ_ONLY;
    duk_def_prop(_context->_ctx, -3, flags);

    // duk_freeze(_ctx, -1); not available yet
  });
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Register a callback function under one or more function names (aliases).
 */
void JSObject::defineFunction(std::set<std::string> const& names, int argCount, FunctionCallback callback) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    // Bind the function names to the current instance (via its heap pointer).
    void *ptr = duk_get_heapptr(_context->_ctx, -1);
    auto &functionMap = instanceFunctionMap[ptr];
    for (auto &name : names) {
      if (functionMap.find(name) != functionMap.end())
        throw std::runtime_error("Duplicate function name '" + name + "'");
      functionMap[name] = callback;

      duk_push_c_function(_context->_ctx, dispatchFunction, argCount);

      duk_push_string(_context->_ctx, "name");
      duk_push_string(_context->_ctx, name.c_str());
      duk_def_prop(_context->_ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_READ_ONLY);

      duk_push_string(_context->_ctx, "ptr");
      duk_push_pointer(_context->_ctx, ptr);
      duk_def_prop(_context->_ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_READ_ONLY);

      duk_put_prop_string(_context->_ctx, -2, name.c_str());
    }
  });
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Defines a new (nested) class on this object. For more details see the scripting context.
 */
void JSObject::defineClass(std::string const& name, std::string const& baseClass, int argCount,
                 ConstructorFunction constructor, PrototypeDefCallback callback) {
  _context->defineClass(*this, name, baseClass, argCount, constructor, callback);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns this object as string, which is something that depends on the JS type. For instance for a String
 * this call returns the content of that object.
 */
std::string JSObject::stringContent() const {
  checkValidity();

  std::string result;
  _context->usingStashedValue(_id, [&]() {
    size_t size;
    const char *text = duk_get_lstring(_context->_ctx, -1, &size);
    result = std::string(text, size);
  });

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Does a lookup of the given target and function name to find a callback. This can fail in which case
 * the result is null.
 */
FunctionCallback JSObject::lookupFunction(void *target, std::string const& name) {
  auto instanceIterator = instanceFunctionMap.find(target);
  if (instanceIterator != instanceFunctionMap.end()) {
    auto callbackIterator = instanceIterator->second.find(name);
    if (callbackIterator != instanceIterator->second.end())
      return callbackIterator->second;
  }

  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Central callback for all registerd functions.
 */
duk_ret_t JSObject::dispatchFunction(duk_context *ctx) {
  auto context = ScriptingContext::fromDuktapeContext(ctx);
  JSValues args(context); // Wraps the values currently on the value stack.

  duk_push_current_function(ctx);
  if (!duk_get_prop_string(ctx, -1, "name"))
    context->throwScriptingError(ScriptingError::Type, "Internal error: function without name");

  std::string name = duk_get_string(ctx, -1);
  duk_pop(ctx);

  // Get the heap pointer of the object (or prototype) on which this function has been defined.
  duk_get_prop_string(ctx, -1, "ptr");
  void *ptr = duk_get_pointer(ctx, -1);

  duk_pop_2(ctx);

  FunctionCallback callback = lookupFunction(ptr, name);
  if (!callback) {
    context->throwScriptingError(ScriptingError::Type, "Attempt to call an undefined function");
  }
  
  // Also get the backing object for the callback (can be null).
  duk_push_this(ctx);
  JSExport *me = reinterpret_cast<JSExport *>(context->getNativeReference());
  duk_pop(ctx);

  try {
    callback(me, args);

    if (args.haveResult())
      return 1;
  } catch (std::runtime_error &e) {
    context->throwScriptingError(ScriptingError::Error, e.what());
  }

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Ditto for property getters.
 */
duk_ret_t JSObject::dispatchGetProperty(duk_context *ctx) {
  // Duktape uses a non-standard approach for property setters and getters:
  // the name of the property is available as (additional) parameter.
  // stack: [ key ]
  std::string property = duk_get_string(ctx, -1);

  duk_push_current_function(ctx);
  duk_get_prop_string(ctx, -1, "ptr");
  void *ptr = duk_get_pointer(ctx, -1);
  duk_pop_2(ctx);

  auto context = ScriptingContext::fromDuktapeContext(ctx);
  duk_push_this(ctx);
  JSExport *me = reinterpret_cast<JSExport *>(context->getNativeReference());
  duk_pop(ctx);

  auto instanceIterator = instancePropertyMap.find(ptr);
  if (instanceIterator != instancePropertyMap.end()) {
    auto propertyIterator = instanceIterator->second.find(property);
    if (propertyIterator != instanceIterator->second.end()) {
      try {
        JSVariant result = propertyIterator->second.getter(context, me, property);
        result.pushValue(context);
        return 1;
      } catch (std::runtime_error &e) {
        context->throwScriptingError(ScriptingError::Error, e.what());
        return 0;
      }
    }
  }

  context->throwScriptingError(ScriptingError::Type, "Attempt to access a property not registered for this instance");

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Ditto for property setters.
 */
duk_ret_t JSObject::dispatchSetProperty(duk_context *ctx) {
  // stack: [ value key ]
  std::string property = duk_get_string(ctx, -1);

  auto context = ScriptingContext::fromDuktapeContext(ctx);
  JSVariant value;
  switch (duk_get_type(ctx, 0)) {
    case DUK_TYPE_UNDEFINED:
    case DUK_TYPE_NULL:
      break;
    case DUK_TYPE_BOOLEAN:
      value = duk_get_boolean(ctx, 0) != 0;
      break;
    case DUK_TYPE_NUMBER:
      value = duk_get_number(ctx, 0);
      break;
    case DUK_TYPE_STRING: {
      size_t size;
      const char *v = duk_get_lstring(ctx, 0, &size);
      value = std::string(v, size);
      break;
    }
    case DUK_TYPE_OBJECT:
      if (duk_is_array(ctx, 0))
        value = JSArray(ScriptingContext::fromDuktapeContext(ctx), 0);
      else
        value = JSObject(ScriptingContext::fromDuktapeContext(ctx), 0);
      break;
    case DUK_TYPE_POINTER:
      value = duk_get_pointer(ctx, 0);
      break;

    default:
      context->throwScriptingError(ScriptingError::Error, "Cannot handle a property of that type (" +
                                   std::to_string(duk_get_type(ctx, 0)) + ")");
  }

  duk_push_current_function(ctx);
  duk_get_prop_string(ctx, -1, "ptr");
  void *ptr = duk_get_pointer(ctx, -1);
  duk_pop_2(ctx);

  duk_push_this(ctx);
  JSExport *me = reinterpret_cast<JSExport *>(context->getNativeReference());
  duk_pop(ctx);

  auto instanceIterator = instancePropertyMap.find(ptr);
  if (instanceIterator != instancePropertyMap.end()) {
    auto propertyIterator = instanceIterator->second.find(property);
    if (propertyIterator != instanceIterator->second.end()) {
      try {
        propertyIterator->second.setter(context, me, property, value);
      } catch (std::runtime_error &e) {
        context->throwScriptingError(ScriptingError::Error, e.what());
      }
      return 0;
    }
  }

  context->throwScriptingError(ScriptingError::Type, "Attempt to access a property not registered for this instance");

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

duk_ret_t JSObject::stringArrayFinalizer(duk_context *ctx) {
  // stack: [ array ]
  ScriptingContext *context = ScriptingContext::fromDuktapeContext(ctx);
  auto *array = reinterpret_cast<StringArrayRef *>(context->getNativeReference());
  context->setNativeReference(nullptr);
  delete array;

  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * The getter trap for a string array proxy.
 */
duk_ret_t JSObject::stringArrayGetter(duk_context *ctx) {
  auto context = ScriptingContext::fromDuktapeContext(ctx);
  auto *array = reinterpret_cast<StringArrayRef *>(context->getNativeReference());

  // The getter is called either with an index or a property/function name. The index can be coerced to a string
  // (in fact standard JS behavior is to coerce numbers to strings for property access, but duktape has an optimized path).
  // So we first check for a number or a string that looks like a number.
  size_t index = INT_MAX;
  if (duk_is_number(ctx, -2))
    index = duk_get_uint(ctx, -2);
  else {
    const char *key = duk_get_string(ctx, -2);
    char *end = nullptr;
    unsigned long candidate = strtoul(key, &end, 10);
    if (*end == '\0')
      index = candidate;
  }

  if (index < INT_MAX && index < (*array)->size()) {
    duk_push_string(ctx, (*array)->at(index).c_str());
    return 1;
  }

  // No index found. Let the target handle the call then (except for .length).
  if (duk_is_string(ctx, -2)) {
    const char *key = duk_require_string(ctx, -2);
    if (strcmp(key, "length") == 0) {
      duk_push_int(ctx, (duk_int_t)(*array)->size());
      return 1;
    }

    duk_dup(ctx, -2); // Add name to stack again, will be replaced by the following call.
    duk_get_prop(ctx, -4);
    return 1;
  }

  return 0; // For `undefined`.
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * The setter trap for a string array proxy.
 */
duk_ret_t JSObject::stringArraySetter(duk_context *ctx) {
  auto context = ScriptingContext::fromDuktapeContext(ctx);
  auto *array = reinterpret_cast<StringArrayRef *>(context->getNativeReference());

  // stack: [... target key value proxy ]
  size_t index = INT_MAX;
  if (duk_is_number(ctx, -3)) {
    index = duk_get_uint(ctx, -3);
  } else {
    const char *key = duk_get_string(ctx, -3);
    if (strcmp(key, "length") == 0) // No need to set the size. We do that implicitly.
      return 1;

    char *end = nullptr;
    unsigned long candidate = strtoul(key, &end, 10);
    if (*end == '\0')
      index = candidate;
  }

  if (index < INT_MAX) {
    const char *value = duk_require_string(ctx, -2);
    if (index >= (*array)->size())
      (*array)->resize(index + 1);

    (**array)[index] = value;

    return 1; // For success.
  }

  return 0; // For failure.
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * The ownKeys trap for a string array proxy.
 */
duk_ret_t JSObject::stringArrayOwnKeys(duk_context *ctx) {
  auto context = ScriptingContext::fromDuktapeContext(ctx);
  auto *array = reinterpret_cast<StringArrayRef *>(context->getNativeReference());

  duk_push_array(ctx);
  duk_uarridx_t size = static_cast<duk_uarridx_t>((*array)->size());
  for (duk_uarridx_t i = 0; i < size; ++i) {
    duk_push_sprintf(ctx, "%d", static_cast<int>(i));
    duk_put_prop_index(ctx, -2, i);
  }

  duk_push_string(ctx, "length");
  duk_put_prop_index(ctx, -2, size);

  return 1; // For success.
}

//----------------- JSGlobalObject -------------------------------------------------------------------------------------

JSGlobalObject::JSGlobalObject(ScriptingContext *context) : JSObject() {
  _context = context;
  duk_push_global_object(context->_ctx);
  initialize(-1);
}

//----------------- JSArray --------------------------------------------------------------------------------------------

JSArray::JSArray() : JSValueBase() {
}

//----------------------------------------------------------------------------------------------------------------------

JSArray::JSArray(ScriptingContext *context) : JSValueBase(context) {
  duk_push_array(context->_ctx);
  initialize(-1);
  duk_pop(context->_ctx);
}

//----------------------------------------------------------------------------------------------------------------------

JSArray::JSArray(ScriptingContext *context, duk_idx_t index) : JSValueBase(context, index) {
}

//----------------------------------------------------------------------------------------------------------------------

size_t JSArray::size() const {
  checkValidity();

  size_t result = 0;
  _context->usingStashedValue(_id, [&]() {
    result = static_cast<size_t>(duk_get_length(_context->_ctx, -1));
  });

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

bool JSArray::is(ValueType type, size_t index) const {
  return isValueType("", static_cast<duk_uarridx_t>(index), type);
}

//----------------------------------------------------------------------------------------------------------------------

bool JSArray::isNumber(size_t index) const {
  return isNumberType("", static_cast<duk_uarridx_t>(index));
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant JSArray::as(ValueType type, size_t index) const {
  return getValue("", static_cast<duk_uarridx_t>(index), type, true, JSVariant());
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant JSArray::get(size_t index) const {
  return getValue("", static_cast<duk_uarridx_t>(index), ValueType::Undefined, false, JSVariant());
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant JSArray::get(size_t index, JSVariant const& defaultValue) const {
  return getValue("", static_cast<duk_uarridx_t>(index), ValueType::Undefined, false, defaultValue);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Changes the content of an existing array element.
 */
void JSArray::setValue(size_t index, JSVariant const& value) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    size_t length = static_cast<size_t>(duk_get_length(_context->_ctx, -1));
    if (index >= length)
      throw std::runtime_error("Array index out of range");

    value.pushValue(_context);
    duk_put_prop_index(_context->_ctx, -2, static_cast<duk_uarridx_t>(index));
  });
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Appends a new array element.
 */
void JSArray::addValue(JSVariant const& value) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    size_t length = duk_get_length(_context->_ctx, -1);
    value.pushValue(_context);
    duk_put_prop_index(_context->_ctx, -2, static_cast<duk_uarridx_t>(length));
  });
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Appends a list of new elements.
 */
void JSArray::addValues(std::initializer_list<JSVariant> const& values) const {
  checkValidity();

  _context->usingStashedValue(_id, [&]() {
    size_t length = duk_get_length(_context->_ctx, -1);
    for (auto &value : values) {
      value.pushValue(_context);
      duk_put_prop_index(_context->_ctx, -2, static_cast<duk_uarridx_t>(length++));
    }
  });
}

//----------------- JSValues ------------------------------------------------------------------------------------------

JSValues::JSValues(ScriptingContext *context) : _context(context) {
}

//----------------------------------------------------------------------------------------------------------------------

size_t JSValues::size() const {
  return static_cast<size_t>(duk_get_top(_context->_ctx));
}

//----------------------------------------------------------------------------------------------------------------------

bool JSValues::is(ValueType type, size_t index) const {
  if (index >= size())
    throw std::runtime_error("Result index out of range");

  duk_dup(_context->_ctx, static_cast<duk_idx_t>(index));
  bool result = isType(_context->_ctx, type);
  duk_pop((_context->_ctx));

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

bool JSValues::isNumber(size_t index) const {
  if (index >= size())
    return false;

  return duk_is_number(_context->_ctx, static_cast<duk_idx_t>(index)) != 0;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant JSValues::as(ValueType type, size_t index) const {
  if (index >= size())
    throw std::runtime_error("Result index out of range");

  duk_dup(_context->_ctx, static_cast<duk_idx_t>(index));
  JSVariant result = JSValueBase::getValue(_context, type, true, JSVariant());
  duk_pop(_context->_ctx);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant JSValues::get(size_t index) const {
  if (index >= size())
    throw std::runtime_error("Result index out of range");

  duk_dup(_context->_ctx, static_cast<duk_idx_t>(index));
  JSVariant result = JSValueBase::getValue(_context, ValueType::Undefined, false, JSVariant());
  duk_pop(_context->_ctx);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant JSValues::get(size_t index, JSVariant const& defaultValue) const {
  if (index >= size())
    throw std::runtime_error("Result index out of range");

  duk_dup(_context->_ctx, static_cast<duk_idx_t>(index));
  JSVariant result = JSValueBase::getValue(_context, ValueType::Undefined, false, defaultValue);
  duk_pop(_context->_ctx);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Rectangle JSValues::getRectangle(size_t index) const {
  duk_idx_t idx = static_cast<duk_idx_t>(index);
  if (duk_is_object(_context->_ctx, idx)) {
    duk_get_prop_string(_context->_ctx, idx, "x");
    int x = 0;
    if (duk_is_number(_context->_ctx, -1))
      x = duk_get_int(_context->_ctx, -1);
    duk_pop(_context->_ctx);

    duk_get_prop_string(_context->_ctx, idx, "y");
    int y = 0;
    if (duk_is_number(_context->_ctx, -1))
      y = duk_get_int(_context->_ctx, -1);
    duk_pop(_context->_ctx);

    duk_get_prop_string(_context->_ctx, idx, "width");
    int width = 0;
    if (duk_is_number(_context->_ctx, -1))
      width = duk_get_int(_context->_ctx, -1);
    duk_pop(_context->_ctx);

    duk_get_prop_string(_context->_ctx, idx, "height");
    int height = 0;
    if (duk_is_number(_context->_ctx, -1))
      height = duk_get_int(_context->_ctx, -1);
    duk_pop(_context->_ctx);

    return geometry::Rectangle(x, y, width, height);
  }

  return geometry::Rectangle();
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Size JSValues::getSize(size_t index) const {
  duk_idx_t idx = static_cast<duk_idx_t>(index);
  if (duk_is_object(_context->_ctx, idx)) {
    duk_get_prop_string(_context->_ctx, idx, "width");
    int width = 0;
    if (duk_is_number(_context->_ctx, -1))
      width = duk_get_int(_context->_ctx, -1);
    duk_pop(_context->_ctx);

    duk_get_prop_string(_context->_ctx, idx, "height");
    int height = 0;
    if (duk_is_number(_context->_ctx, -1))
      height = duk_get_int(_context->_ctx, -1);
    duk_pop(_context->_ctx);

    return geometry::Size(width, height);
  }

  return geometry::Size();
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Point JSValues::getPoint(size_t index) const {
  duk_idx_t idx = static_cast<duk_idx_t>(index);
  if (duk_is_object(_context->_ctx, idx)) {
    duk_get_prop_string(_context->_ctx, idx, "x");
    int x = 0;
    if (duk_is_number(_context->_ctx, -1))
      x = duk_get_int(_context->_ctx, -1);
    duk_pop(_context->_ctx);

    duk_get_prop_string(_context->_ctx, idx, "y");
    int y = 0;
    if (duk_is_number(_context->_ctx, -1))
      y = duk_get_int(_context->_ctx, -1);
    duk_pop(_context->_ctx);

    return geometry::Point(x, y);
  }

  return geometry::Point();
}

//----------------------------------------------------------------------------------------------------------------------

aal::TextRange JSValues::getTextRange(size_t index) const {
  duk_idx_t idx = static_cast<duk_idx_t>(index);
  if (duk_is_object(_context->_ctx, idx)) {
    duk_get_prop_string(_context->_ctx, idx, "start");
    unsigned start = 0;
    if (duk_is_number(_context->_ctx, -1))
      start = duk_get_uint(_context->_ctx, -1);
    duk_pop(_context->_ctx);

    duk_get_prop_string(_context->_ctx, idx, "end");
    unsigned end = 0;
    if (duk_is_number(_context->_ctx, -1))
      end = duk_get_uint(_context->_ctx, -1);
    duk_pop(_context->_ctx);

    return aal::TextRange(start, end);
  }

  return aal::TextRange();

}

//----------------------------------------------------------------------------------------------------------------------

void JSValues::removeValue(size_t index) const {
  duk_remove(_context->_ctx, static_cast<duk_idx_t>(index));
}

//----------------------------------------------------------------------------------------------------------------------

void JSValues::pushResult(JSVariant const& value) {
  _haveResult = true;
  value.pushValue(_context);
}

//----------------------------------------------------------------------------------------------------------------------

void JSValues::format() {
  _haveResult = true;
  _context->format();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the current JS "this" binding, which is only valid in certain situations when set by the JS engine
 * (e.g. when calling a registered callback function for methods/properties of a class).
 * The "this" binding can even explicitly be changed (e.g. via "Function.prototype.apply()") so use it with care!
 */
JSObject JSValues::getThis() const {
  duk_push_this(_context->_ctx);
  auto result = JSObject(_context, -1);
  duk_pop(_context->_ctx);

  return result;
}

//----------------- JSExport -------------------------------------------------------------------------------------------

JSExport::~JSExport() {
}

//----------------------------------------------------------------------------------------------------------------------

void JSExport::finalize(ScriptingContext *context) {
  std::ignore = context;
}

//----------------- JSVariant ------------------------------------------------------------------------------------------

JSVariant::JSVariant() : _intValue(0) {
  _type = ValueType::Undefined;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(std::nullptr_t) : _intValue(0) {
  _type = ValueType::Null;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(int value): _intValue(value) {
  _type = ValueType::Int;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(ssize_t value) : _doubleValue (static_cast<double>(value)) {
  _type = ValueType::Double;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(unsigned int value): _uintValue (value) {
  _type = ValueType::UInt;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(size_t value) : _doubleValue (static_cast<double>(value)) {
  _type = ValueType::Double;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(double value) : _doubleValue (value) {
  _type = ValueType::Double;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(std::string const& value, bool treatAsJson) : _stringValue(value) {
  if (treatAsJson)
    _type = ValueType::Json;
  else
    _type = ValueType::String;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(char const* value) : _stringValue(value) {
  _type = ValueType::String;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(bool value) : _boolValue(value) {
  _type = ValueType::Boolean;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(void *value) : _pointerValue(value) {
  _type = ValueType::Pointer;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(JSObject const& value) : _objectValue(value) {
  _type = ValueType::Object;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(JSArray const& value) : _arrayValue(value) {
  _type = ValueType::Array;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(SerializableObject const& value) : _stringValue(value.toJson()) {
  _type = ValueType::Json;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::JSVariant(JSVariant const& other) : _type(ValueType::Undefined) {
  assign(other);
}

//----------------------------------------------------------------------------------------------------------------------
                                                                                                                                                                                      
JSVariant::JSVariant(JSVariant&& other) : _type(ValueType::Undefined) {
  move(std::move(other));
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::~JSVariant() {
  cleanUp();
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant& JSVariant::operator = (JSVariant const& other) {
  assign(other);
  return *this;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant& JSVariant::operator = (JSVariant&& other) {
  move(std::move(other));
  return *this;
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::operator int() const {
  switch (_type) {
    case ValueType::Int:
      return _intValue;
    case ValueType::Boolean:
      return static_cast<int>(_boolValue);
    case ValueType::Double:
      if (static_cast<int>(_doubleValue) == _doubleValue)
        return static_cast<int>(_doubleValue);
      break;
    case ValueType::String:
      try {
        return std::stoi(_stringValue);
      } catch (...) {}
      break;
      // fallthrough
    default:
      break;
  }
  throw std::runtime_error("Bad cast");
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::operator unsigned int() const {
  switch (_type) {
    case ValueType::Int:
      if (_intValue >= 0)
        return (unsigned int)_intValue;
      break;
    case ValueType::UInt:
      return _uintValue;
    case ValueType::Boolean:
      return static_cast<unsigned int>(_boolValue);
    case ValueType::Double:
      if (static_cast<unsigned int>(_doubleValue) == _doubleValue)
        return static_cast<unsigned int>(_doubleValue);
      break;
    case ValueType::String: {
      try {
        int result = std::stoi(_stringValue);
        if (result >= 0)
          return (unsigned int)result;
      } catch (...) {}
      break;
    }
    default:
      break;
  }
  throw std::runtime_error("Bad cast");
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::operator double() const {
  switch (_type) {
    case ValueType::Int:
      return _intValue;
    case ValueType::UInt:
      return _uintValue;
    case ValueType::Double:
      return _doubleValue;
    case ValueType::Boolean:
      return _boolValue;
    case ValueType::String:
      try {
        return std::stod(_stringValue);
      } catch (...) {}
      break;
    default:
      break;
  }
  throw std::runtime_error("Bad cast");
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::operator std::string() const {
  switch (_type) {
    case ValueType::Int:
      return std::to_string(_intValue);
    case ValueType::UInt:
      return std::to_string(_uintValue);
    case ValueType::Double:
      return std::to_string(_doubleValue);
    case ValueType::String:
    case ValueType::Json:
      return _stringValue;
    case ValueType::Boolean:
      return std::to_string(_boolValue);
    default:
      throw std::runtime_error("Bad cast");
  }
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::operator bool() const {
  switch (_type) {
    case ValueType::Int:
      return _intValue != 0;
    case ValueType::UInt:
      return _uintValue != 0;
    case ValueType::Double:
      return _doubleValue != 0;
    case ValueType::Pointer:
      return _pointerValue != nullptr;
    case ValueType::Boolean:
      return _boolValue;
    case ValueType::Null:
    case ValueType::Undefined:
      return false;
    case ValueType::String:
    case ValueType::Json:
      return !_stringValue.empty();
    case ValueType::Object:
    case ValueType::Array:
      return true;
    default:
      throw std::runtime_error("Bad cast");
  }
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::operator JSObject() const {
  if (_type == ValueType::Object)
    return _objectValue;

  throw std::runtime_error("Bad cast");
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::operator JSArray() const {
  if (_type == ValueType::Array)
    return _arrayValue;

  throw std::runtime_error("Bad cast");
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::operator void*() const {
  if (_type == ValueType::Pointer)
    return _pointerValue;
  else if (_type == ValueType::Null) 
    return nullptr;
    
  throw std::runtime_error("Bad cast");
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::operator ssize_t() const {
  switch (_type) {
    case ValueType::Int:
      return _intValue;
    case ValueType::UInt:
      return _uintValue;
    case ValueType::Double:
      if (static_cast<ssize_t>(_doubleValue) == _doubleValue)
        return static_cast<ssize_t>(_doubleValue);
      break;
    case ValueType::Boolean:
      return _boolValue;
    case ValueType::String:
      try {
        return std::stoi(_stringValue);
      } catch (...) {}
      break;
    default:
      break;
  }
  throw std::runtime_error("Bad cast");
}

//----------------------------------------------------------------------------------------------------------------------

JSVariant::operator size_t() const {
  switch (_type) {
    case ValueType::Int:
      return static_cast<size_t>(_intValue);
    case ValueType::UInt:
      return _uintValue;
    case ValueType::Double: {
      if (static_cast<size_t>(_doubleValue) == _doubleValue)
        return static_cast<size_t>(_doubleValue);
      break;
    }
    case ValueType::Boolean:
      return _boolValue;
    case ValueType::String: {
      try {
        int result = std::stoi(_stringValue);
        if (result >= 0)
          return static_cast<unsigned int>(result);
      } catch (...) {}
      break;
    }
    default:
      break;
  }
  throw std::runtime_error("Bad cast");
}

//----------------------------------------------------------------------------------------------------------------------

bool JSVariant::is(ValueType type) const {
  return _type == type;
}

//----------------------------------------------------------------------------------------------------------------------

void JSVariant::pushValue(ScriptingContext const* context) const {
  switch (_type) {
    case ValueType::Undefined:
      duk_push_undefined(context->_ctx);
      break;
    case ValueType::Null:
      duk_push_null(context->_ctx);
      break;
    case ValueType::Int:
      duk_push_int(context->_ctx, _intValue);
      break;
    case ValueType::UInt:
      duk_push_uint(context->_ctx, _uintValue);
      break;
    case ValueType::Double:
      duk_push_number(context->_ctx, _doubleValue);
      break;
    case ValueType::String: {
      duk_push_lstring(context->_ctx, _stringValue.c_str(), _stringValue.size());
      break;
    }
    case ValueType::Boolean:
      duk_push_boolean(context->_ctx, _boolValue);
      break;
    case ValueType::Object:
      _objectValue.push();
      break;
    case ValueType::Array:
      _arrayValue.push();
      break;
    case ValueType::Pointer:
      duk_push_pointer(context->_ctx, _pointerValue);
      break;
    case ValueType::Json:  {
      duk_push_lstring(context->_ctx, _stringValue.c_str(), _stringValue.size());
      duk_json_decode(context->_ctx, -1);
      break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Used to do proper clean-up of the union member. After this call the type is reset to Undefined and the integer member
 * is active (with a value of 0).
 */
void JSVariant::cleanUp() {
  // Explicitly trigger non-trivial destructors.
  switch (_type) {
    case ValueType::String:
    case ValueType::Json:
      _stringValue.~basic_string();
      break;
    case ValueType::Object:
      _objectValue.~JSObject();
      break;
    case ValueType::Array:
      _arrayValue.~JSArray();
      break;
    default: // Nothing to do.
      break;
  }

  _type = ValueType::Undefined;
  _intValue = 0;
}

//----------------------------------------------------------------------------------------------------------------------

void JSVariant::assign(JSVariant const& other) {
  cleanUp();

  _type = other._type;
  switch (_type) {
    case ValueType::Int:
      _intValue = other._intValue;
      break;
    case ValueType::UInt:
      _uintValue = other._uintValue;
      break;
    case ValueType::Double:
      _doubleValue = other._doubleValue;
      break;
    case ValueType::String:
    case ValueType::Json: {
      std::string *p = new(&_stringValue) std::string;
      *p = other._stringValue;
      break;
    }
    case ValueType::Boolean:
      _boolValue = other._boolValue;
      break;
    case ValueType::Object: {
      JSObject *p = new(&_objectValue) JSObject;
      *p = other._objectValue;
      break;
    }
    case ValueType::Array: {
      JSArray *p = new(&_arrayValue) JSArray;
      *p = other._arrayValue;
      break;
    }
    case ValueType::Pointer:
      _pointerValue = other._pointerValue;
      break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void JSVariant::move(JSVariant&& other) {
  cleanUp();

  _type = other._type;
  switch (_type) {
    case ValueType::Int:
      _intValue = other._intValue;
      break;
    case ValueType::UInt:
      _uintValue = other._uintValue;
      break;
    case ValueType::Double:
      _doubleValue = other._doubleValue;
      break;
    case ValueType::String:
    case ValueType::Json: {
       std::string *p = new(&_stringValue) std::string;
      *p = std::move(other._stringValue);
      break;
    }
    case ValueType::Boolean:
      _boolValue = other._boolValue;
      break;
    case ValueType::Object: {
       JSObject *p = new(&_objectValue) JSObject;
      *p = std::move(other._objectValue);
      break;
    }
    case ValueType::Array: {
      JSArray *p = new(&_arrayValue) JSArray;
      *p = std::move(other._arrayValue);
      break;
    }
    case ValueType::Pointer:
      _pointerValue = other._pointerValue;
      break;
    default:
      break;
  }

  other.cleanUp(); // Reset the other value.
}

//----------------------------------------------------------------------------------------------------------------------
