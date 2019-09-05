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

#pragma once

#include "duktape.h"

#include "common.h"
#include "types.h"
#include "geometry.h"
#include "textrange.h"

namespace mga {

#define DEFINE_CONSTANT(target, constant) target.defineProperty(#constant, JSVariant(constant));

  enum class ValueType {
    Undefined,
    Null,
    Boolean,
    Int,
    UInt,
    Double,
    String,
    Object,
    Array,
    Pointer,
    Json    
  };

  class JSVariant;

  // Wrapper classes for JS values.
  class JSValueBase {
    friend class JSValues;
    
  public:
    JSValueBase();
    JSValueBase(ScriptingContext *context);
    JSValueBase(ScriptingContext *context, duk_idx_t index);
    JSValueBase(JSValueBase const& other);
    JSValueBase(JSValueBase&& other);
    virtual ~JSValueBase();

    JSValueBase operator = (JSValueBase const& other);
    JSValueBase operator = (JSValueBase&& other);

    bool isValid() const { return _context != nullptr; }

    void push() const;

    std::string dumpObject(bool showHidden, size_t maxDepth) const;
  protected:
    ScriptingContext *_context = nullptr;
    std::string _id;

    void initialize(duk_idx_t index);

    bool isValueType(std::string const& property, duk_uarridx_t index, ValueType type) const;
    bool isNumberType(std::string const& property, duk_uarridx_t index) const;
    JSVariant getValue(std::string const& property, duk_uarridx_t index, ValueType type, bool coerced,
                       JSVariant const& defaultValue) const;

    void checkValidity() const;

  private:
    static JSVariant getValue(ScriptingContext *context, ValueType type, bool coerced,
                              JSVariant const& defaultValue);
  };

  class JSArray;

  class JSObject : public JSValueBase {
  public:
    JSObject();                                           // For an invalid object.
    JSObject(ScriptingContext *context);                  // For a new object.
    JSObject(ScriptingContext *context, duk_idx_t index); // For an existing object (on the stack).

    JSExport* getBacking() const;
    void setBacking(JSExport *object) const;

    // General value getters and setters.
    std::set<std::string> getPropertyKeys() const;

    bool is(ValueType type, std::string const& property) const;
    bool isNumber(std::string const& property) const;

    JSVariant as(ValueType type, std::string const& property) const;
    JSVariant get(std::string const& property) const;
    JSVariant get(std::string const& property, JSVariant const& defaultValue) const;

    void set(std::string const& property, JSVariant const& value) const;

    // Functionality of an object as instance or prototype of a class.
    void defineProperty(std::string const& name, JSVariant const& value, bool readOnly = true) const;
    void defineProperty(std::string const& name, std::map<std::string, std::string> const& value,
                        bool readOnly = true) const;
    void defineProperty(std::string const& name, std::vector<std::string> const& value, bool readOnly = true) const;
    void defineVirtualProperty(std::string const& name, PropertyGetter getter, PropertySetter setter) const;
    void defineVirtualArrayProperty(std::string const& name, StringArrayRef const& array, bool readOnly) const;
    void defineEnum(std::string const& name, ObjectDefCallback callback, bool readOnly = true) const;

    void defineFunction(std::set<std::string> const& names, int argCount, FunctionCallback callback) const;

    void defineClass(std::string const& name, std::string const& baseClass, int argCount,
                     ConstructorFunction constructor, PrototypeDefCallback callback);

    std::string stringContent() const;

  private:
    static FunctionCallback lookupFunction(void *target, std::string const& name);

    static duk_ret_t dispatchFunction(duk_context *ctx);
    static duk_ret_t dispatchGetProperty(duk_context *ctx);
    static duk_ret_t dispatchSetProperty(duk_context *ctx);

    static duk_ret_t stringArrayFinalizer(duk_context *ctx);
    static duk_ret_t stringArrayGetter(duk_context *ctx);
    static duk_ret_t stringArraySetter(duk_context *ctx);
    static duk_ret_t stringArrayOwnKeys(duk_context *ctx);
  };

  // A special object instance representing the scripting context root (which is not visible in JS, but everything
  // defined on it is visible everywhere).
  class JSGlobalObject : public JSObject {
  public:
    JSGlobalObject(ScriptingContext *context);
  };

  class JSArray : public JSValueBase {
  public:
    JSArray();
    JSArray(ScriptingContext *context);
    JSArray(ScriptingContext *context, duk_idx_t index);

    size_t size() const;
    bool empty() const { return size() == 0; }

    bool is(ValueType type, size_t index) const;
    bool isNumber(size_t index) const;

    JSVariant as(ValueType type, size_t index) const;
    JSVariant get(size_t index) const;
    JSVariant get(size_t index, JSVariant const& defaultValue) const;

    void setValue(size_t index, JSVariant const& value) const;
    void addValue(JSVariant const& value) const;
    void addValues(std::initializer_list<JSVariant> const& values) const;
  };

  // A wrapper of a collection of values (e.g. parameters).
  // Elements are accessed via an index (0..size() - 1), in the order they were pushed in JS.
  class JSValues {
  public:
    JSValues(ScriptingContext *context);

    size_t size() const;
    std::string dumpObject(size_t index, bool showHidden, size_t maxDepth) const;

    bool is(ValueType type, size_t index) const;
    bool isNumber(size_t index) const;

    JSVariant as(ValueType type, size_t index) const;
    JSVariant get(size_t index) const;
    JSVariant get(size_t index, JSVariant const& defaultValue) const;

    geometry::Rectangle getRectangle(size_t index) const;
    geometry::Size getSize(size_t index) const;
    geometry::Point getPoint(size_t index) const;
    aal::TextRange getTextRange(size_t index) const;

    void removeValue(size_t index) const;

    bool haveResult() const { return _haveResult; }
    void haveResult(bool flag) { _haveResult = flag; }

    void pushResult(JSVariant const& value);

    void format();
    JSObject getThis() const;
    ScriptingContext* context() { return _context; }

  private:
    bool _haveResult = false;
    ScriptingContext *_context;
  };

  // All classes that are exported to JS must derive from this class.
  class JSExport {
  public:
    static const int VarArgs = DUK_VARARGS; // Use for variable parameter counts.

    virtual ~JSExport();

    template<typename T>
    static T* reference(JSObject const& object) {
      return dynamic_cast<T *>(object.getBacking());
    }

    virtual void finalize(ScriptingContext *context);
  };

  // A wrapper for a value whose type can only be what's defined in ValueType (except for the Any type).
  class JSVariant {
  public:
    JSVariant();
    JSVariant(std::nullptr_t);
    JSVariant(int value);
    JSVariant(ssize_t value); // Will be converted to double.
    JSVariant(unsigned int value);
    JSVariant(size_t value); // Ditto.
    JSVariant(double value);
    JSVariant(std::string const& value, bool treatAsJson = false);
    JSVariant(char const* value);
    JSVariant(bool value);
    JSVariant(void *value);
    JSVariant(JSObject const& value);
    JSVariant(JSArray const& value);
    JSVariant(SerializableObject const& value);

    JSVariant(JSVariant const& other);
    JSVariant(JSVariant&& other);

    ~JSVariant();

    JSVariant& operator = (JSVariant const& other);
    JSVariant& operator = (JSVariant&& other);

    bool isValid() { return _type != ValueType::Undefined && _type != ValueType::Null; }

    // These operators convert to the target type if that doesn't has impact on the precision,
    // e.g. int -> double, bool -> int etc.
    operator int() const;
    operator unsigned int() const;
    operator double() const;
    operator std::string() const;
    operator bool() const;
    operator JSObject() const;
    operator JSArray() const;
    operator void*() const;

    operator ssize_t() const;
    operator size_t() const;

    bool is(ValueType type) const;
    void pushValue(ScriptingContext const* context) const;

  private:
    ValueType _type;
    union {
      int _intValue;
      unsigned int _uintValue;
      double _doubleValue;
      std::string _stringValue;
      bool _boolValue;
      void *_pointerValue;

      JSObject _objectValue;
      JSArray _arrayValue;
    };

    void cleanUp();

    void assign(JSVariant const& other);
    void move(JSVariant&& other);
  };

  using VariantArray = std::vector<JSVariant>;

} // namespace mga
