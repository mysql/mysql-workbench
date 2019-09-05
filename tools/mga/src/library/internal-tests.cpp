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

#include "utilities.h"
#include "scripting-context.h"

#include "internal-tests.h"
#include <iostream>

using namespace mga;

InternalTests::TestStatistics InternalTests::_context(__FILE__);

#define expect(value)                       \
  InternalTests::_context._line = __LINE__; \
  InternalTests::makeExpect(value)

#define it(description, func) InternalTests::it(description, func)
#define describe(description, func) InternalTests::describe(description, func)

//----------------------------------------------------------------------------------------------------------------------

void InternalTests::activate(ScriptingContext &context, JSObject &exports) {
  std::ignore = context;

  exports.defineFunction({"executeInternalTests"}, 0, [&](JSExport *, JSValues &args) {
    ScriptingContext *currentContext = args.context();

    describe("JSObject tests:", [&]() {
      it("defineProperty", [&]() {
        JSObject object(currentContext);

        object.defineProperty("prop1", 1234, false);
        object.defineProperty("prop2", "some text", false);

        expect(object.getPropertyKeys().size()).toBe(2);

        expect((int)object.get("prop1")).toBe(1234);
        expect((std::string)object.get("prop2")).toBe("some text");
        expect(object.dumpObject(true, 10)).toContain("prop1: 1234");
      });
    });

    describe("JSValue tests:", [&]() {
      it("size", [&]() {
        //  TODO: Not very well tested, but lacking a good way to test it, since we only have 1 call to these tests
        //     expect(args.size()).toBe(0);
      });
    });

    describe("JSVariant tests:", [&]() {
      std::string jsonObjectString = "{\"propA\": 123, \"propB\": \"some text\"}";
      it("move constructor", [&]() {
        JSVariant first("some text");

        expect((std::string)first).toBe("some text");

        JSVariant second(std::move(first));
        expect((std::string)second).toBe("some text");
        expect(first.is(ValueType::Undefined)).toBe(true);
      });

      it("assign", [&]() {
        JSVariant value1("some text");
        JSVariant value2(-1234);
        JSVariant value3((unsigned int)1234);
        JSObject value4(currentContext);
        JSArray value5(currentContext);

        JSObject value4_1(currentContext);
        JSArray value5_1(currentContext);

        //  TODO: These tests are not going as expected
        //     value4.defineVirtualProperty("myProp1", [](JSExport *, std::string const&)  -> JSVariant { return 0; },
        //     PropertySetter());
        //     value4_1 = value4;
        //     expect(value4_1.toString()).toBe("");

        value5.addValue(1234);
        value5.addValue(5678);
        value5_1 = value5;

        expect((std::string)JSVariant(value1)).toBe("some text");
        expect((int)JSVariant(value2)).toBe(-1234);
        expect((unsigned int)JSVariant(value3)).toBe(1234);
        expect((int)value5_1.get(0)).toBe(1234);
        expect((int)value5_1.get(1)).toBe(5678);
      });

      it("cast to int", [&]() {
        expect((int)JSVariant(0)).toBe(0);
        expect((int)JSVariant(-1234)).toBe(-1234);
        expect((int)JSVariant(1234)).toBe(1234);
        expect((int)JSVariant(false)).toBe(0);
        expect((int)JSVariant(true)).toBe(1);

        expect([]() { std::ignore = (int)JSVariant("aaa"); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (int)JSVariant(123.45); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (int)JSVariant(nullptr); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (int)JSVariant({1, 2, 3, 4}); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (int)JSVariant(JSVariant()); }).toThrowError("Bad cast");
        expect([&]() { std::ignore = (int)JSVariant(jsonObjectString, true); }).toThrowError("Bad cast");
      });

      it("cast to unsigned int", [&]() {
        expect((unsigned int)JSVariant(0)).toBe(0);
        expect((unsigned int)JSVariant(1234)).toBe(1234);
        expect((unsigned int)JSVariant(false)).toBe(0);
        expect((unsigned int)JSVariant(true)).toBe(1);

        expect([]() { std::ignore = (unsigned int)JSVariant(-1234); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (unsigned int)JSVariant("aaa"); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (unsigned int)JSVariant(123.45); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (unsigned int)JSVariant(nullptr); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (unsigned int)JSVariant({1, 2, 3, 4}); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (unsigned int)JSVariant(JSVariant()); }).toThrowError("Bad cast");
        expect([&]() { std::ignore = (unsigned int)JSVariant(jsonObjectString, true); }).toThrowError("Bad cast");
      });

      it("cast to double", [&]() {
        expect((double)JSVariant(0)).toBe(0);
        expect((double)JSVariant(1234)).toBe(1234.0);
        expect((double)JSVariant(false)).toBe(0);
        expect((double)JSVariant(true)).toBe(1.0);
        expect((double)JSVariant(123.45)).toBe(123.45);
        expect((double)JSVariant(-123.45)).toBe(-123.45);

        expect([]() { std::ignore = (double)JSVariant("aaa"); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (double)JSVariant(nullptr); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (double)JSVariant({1, 2, 3, 4}); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (double)JSVariant(JSVariant()); }).toThrowError("Bad cast");
        expect([&]() { std::ignore = (double)JSVariant(jsonObjectString, true); }).toThrowError("Bad cast");
      });

      it("cast to string", [&]() {
        expect((std::string)JSVariant(0)).toBe("0");
        expect((std::string)JSVariant(1234)).toBe("1234");
        expect((std::string)JSVariant(false)).toBe("0");
        expect((std::string)JSVariant(true)).toBe("1");
        expect((std::string)JSVariant(123.45)).toBe("123.450000");
        expect((std::string)JSVariant(-123.45)).toBe("-123.450000");
        expect((std::string)JSVariant("aaa")).toBe("aaa");
        expect((std::string)JSVariant(jsonObjectString, true)).toBe(jsonObjectString);

        //  This fails and it also throws a weird value
        //     expect((std::string)JSVariant({1, 2, 3, 4})).toThrowError("Bad cast");

        expect([]() { std::ignore = (std::string) JSVariant(nullptr); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (std::string) JSVariant(JSVariant()); }).toThrowError("Bad cast");
      });

      it("cast to JSArray", [&]() {
        expect([]() { std::ignore = (JSArray) JSVariant(0); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSArray) JSVariant(1234); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSArray) JSVariant(false); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSArray) JSVariant(true); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSArray) JSVariant(123.45); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSArray) JSVariant(-123.45); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSArray) JSVariant("aaa"); }).toThrowError("Bad cast");
        expect([&]() { std::ignore = (JSArray) JSVariant(jsonObjectString, true); }).toThrowError("Bad cast");

        //  This fails and it also throws a weird value
        //     expect((std::string)JSVariant({1, 2, 3, 4})).toThrowError("Bad cast");

        expect([]() { std::ignore = (JSArray) JSVariant(nullptr); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSArray) JSVariant(JSVariant()); }).toThrowError("Bad cast");
      });

      it("cast to JSObject", [&]() {
        expect([]() { std::ignore = (JSObject) JSVariant(0); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSObject) JSVariant(1234); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSObject) JSVariant(false); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSObject) JSVariant(true); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSObject) JSVariant(123.45); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSObject) JSVariant(-123.45); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSObject) JSVariant("aaa"); }).toThrowError("Bad cast");
        expect([&]() { std::ignore = (JSObject) JSVariant(jsonObjectString, true); }).toThrowError("Bad cast");

        //  This fails and it also throws a weird value
        //     expect((std::string)JSVariant({1, 2, 3, 4})).toThrowError("Bad cast");

        expect([]() { std::ignore = (JSObject) JSVariant(nullptr); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (JSObject) JSVariant(JSVariant()); }).toThrowError("Bad cast");
      });

      it("cast to boolean", [&]() {
        expect((bool)JSVariant(0)).toBe(false);
        expect((bool)JSVariant(1234)).toBe(true);
        expect((bool)JSVariant(false)).toBe(false);
        expect((bool)JSVariant(true)).toBe(true);
        expect((bool)JSVariant(-1234)).toBe(true);
        expect((bool)JSVariant(123.45)).toBe(true);
        expect((bool)JSVariant(nullptr)).toBe(false);
        expect((bool)JSVariant()).toBe(false);
        expect((bool)JSVariant("")).toBe(false);
        expect((bool)JSVariant("aaa")).toBe(true);
        expect((bool)JSVariant({1, 2, 3, 4})).toBe(true);
        expect((bool)JSVariant(jsonObjectString, true)).toBe(true);
      });

      it("cast to ssize_t", [&]() {
        expect((ssize_t)JSVariant(0)).toBe(0);
        expect((ssize_t)JSVariant(-1234)).toBe(-1234);
        expect((ssize_t)JSVariant(1234)).toBe(1234);
        expect((ssize_t)JSVariant(false)).toBe(0);
        expect((ssize_t)JSVariant(true)).toBe(1);

        expect([]() { std::ignore = (ssize_t)JSVariant("aaa"); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (ssize_t)JSVariant(123.45); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (ssize_t)JSVariant(nullptr); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (ssize_t)JSVariant({1, 2, 3, 4}); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (ssize_t)JSVariant(JSVariant()); }).toThrowError("Bad cast");
        expect([&]() { std::ignore = (ssize_t)JSVariant(jsonObjectString, true); }).toThrowError("Bad cast");
      });

      it("cast to size_t", [&]() {
        expect((size_t)JSVariant(0)).toBe(0);
        expect((size_t)JSVariant(1234)).toBe(1234);
        expect((size_t)JSVariant(false)).toBe(0);
        expect((size_t)JSVariant(true)).toBe(1);
        expect((size_t)JSVariant(-1234)).toBe(-1234);

        expect([]() { std::ignore = (size_t)JSVariant("aaa"); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (size_t)JSVariant(123.45); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (size_t)JSVariant(nullptr); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (size_t)JSVariant({1, 2, 3, 4}); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (size_t)JSVariant(JSVariant()); }).toThrowError("Bad cast");
        expect([&]() { std::ignore = (size_t)JSVariant(jsonObjectString, true); }).toThrowError("Bad cast");
      });

      it("cast to pointer", [&]() {
        expect((void *)JSVariant(nullptr)).toBe(nullptr);

        expect([]() { std::ignore = (void *)JSVariant(0); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (void *)JSVariant(1234); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (void *)JSVariant(false); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (void *)JSVariant(true); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (void *)JSVariant(123.45); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (void *)JSVariant(-123.45); }).toThrowError("Bad cast");
        expect([]() { std::ignore = (void *)JSVariant("aaa"); }).toThrowError("Bad cast");
        expect([&]() { std::ignore = (void *)JSVariant(jsonObjectString, true); }).toThrowError("Bad cast");

        //  This fails and it also throws a weird value
        //     expect((std::string)JSVariant({1, 2, 3, 4})).toThrowError("Bad cast");

        expect([]() { std::ignore = (void *)JSVariant(JSVariant()); }).toThrowError("Bad cast");
      });
    });

    describe("JSArray tests:", [&]() {
      it("empty", [&]() {
        JSArray array(currentContext);
        expect(array.empty()).toBe(true);
        array.addValue(1);
        expect(array.empty()).toBe(false);
      });

      it("is", [&]() {
        JSArray array(currentContext);
        array.addValues({1, "some text", 3.0, JSVariant(), nullptr, true});

        expect(array.is(ValueType::Int, 0)).toBe(true);
        expect(array.is(ValueType::Int, 1)).toBe(false);
        expect(array.is(ValueType::Int, 2)).toBe(true);
        expect(array.is(ValueType::Int, 3)).toBe(false);
        expect(array.is(ValueType::Int, 4)).toBe(false);
        expect(array.is(ValueType::Int, 5)).toBe(false);

        expect(array.is(ValueType::String, 0)).toBe(false);
        expect(array.is(ValueType::String, 1)).toBe(true);
        expect(array.is(ValueType::String, 2)).toBe(false);
        expect(array.is(ValueType::String, 3)).toBe(false);
        expect(array.is(ValueType::String, 4)).toBe(false);
        expect(array.is(ValueType::String, 5)).toBe(false);

        expect(array.is(ValueType::Double, 0)).toBe(true);
        expect(array.is(ValueType::Double, 1)).toBe(false);
        expect(array.is(ValueType::Double, 2)).toBe(true);
        expect(array.is(ValueType::Double, 3)).toBe(false);
        expect(array.is(ValueType::Double, 4)).toBe(false);
        expect(array.is(ValueType::Double, 5)).toBe(false);

        expect(array.is(ValueType::Undefined, 0)).toBe(false);
        expect(array.is(ValueType::Undefined, 1)).toBe(false);
        expect(array.is(ValueType::Undefined, 2)).toBe(false);
        expect(array.is(ValueType::Undefined, 3)).toBe(true);
        expect(array.is(ValueType::Undefined, 4)).toBe(false);
        expect(array.is(ValueType::Undefined, 5)).toBe(false);

        expect(array.is(ValueType::Null, 0)).toBe(false);
        expect(array.is(ValueType::Null, 1)).toBe(false);
        expect(array.is(ValueType::Null, 2)).toBe(false);
        expect(array.is(ValueType::Null, 3)).toBe(false);
        expect(array.is(ValueType::Null, 4)).toBe(true);
        expect(array.is(ValueType::Null, 5)).toBe(false);

        expect(array.is(ValueType::Boolean, 0)).toBe(false);
        expect(array.is(ValueType::Boolean, 1)).toBe(false);
        expect(array.is(ValueType::Boolean, 2)).toBe(false);
        expect(array.is(ValueType::Boolean, 3)).toBe(false);
        expect(array.is(ValueType::Boolean, 4)).toBe(false);
        expect(array.is(ValueType::Boolean, 5)).toBe(true);

      });

      it("addValue", [&]() {
        JSArray array(currentContext);

        expect(array.empty()).toBe(true);

        array.addValue(1234);
        array.addValue("some text");
        array.addValue(nullptr);
        array.addValue(JSVariant());
        array.addValue(false);
        array.addValue(1.234);

        expect((int)array.get(0)).toBe(1234);
        expect((std::string)array.get(1)).toBe("some text");
        //  TODO: how to test this here...?
        //     expect((void *)array.get(2)).toBe(nullptr);
        expect(array.get(3).is(ValueType::Undefined)).toBe(true);
        expect((bool)array.get(4)).toBe(false);
        expect((double)array.get(5)).toBe(1.234);

        expect([&]() { array.get(6); }).toThrowError("Array index out of range");
        expect([&]() { array.get(7); }).toThrowError("Array index out of range");
        expect([&]() { array.get(100000); }).toThrowError("Array index out of range");
      });

      it("addValues", [&]() {
        JSArray array(currentContext);

        array.addValues({1234, "some text", nullptr, JSVariant(), false, 1.234});

        expect((int)array.get(0)).toBe(1234);
        expect((std::string)array.get(1)).toBe("some text");
        //  TODO: how to test this here...?
        //     expect((void *)array.get(2)).toBe(nullptr);
        expect(array.get(3).is(ValueType::Undefined)).toBe(true);
        expect((bool)array.get(4)).toBe(false);
        expect((double)array.get(5)).toBe(1.234);

        expect([&]() { array.get(6); }).toThrowError("Array index out of range");
        expect([&]() { array.get(7); }).toThrowError("Array index out of range");
        expect([&]() { array.get(1000000); }).toThrowError("Array index out of range");
      });

      it("setValue", [&]() {
        JSArray array(currentContext);

        array.addValue(1234);
        array.addValue("some text");

        expect((int)array.get(0)).toBe(1234);
        expect((std::string)array.get(1)).toBe("some text");

        array.setValue(0, "some text");
        array.setValue(1, 1234);

        expect((std::string)array.get(0)).toBe("some text");
        expect((int)array.get(1)).toBe(1234);
      });

      it("get", [&]() {
        JSArray array(currentContext);

        array.addValue(1234);
        array.addValue("some text");

        expect((int)array.get(0)).toBe(1234);
        expect((std::string)array.get(1)).toBe("some text");

        //  TODO: We should be able to get the default value, here...
        //     expect((std::string)array.get(2, "undefined variable")).toBe("undefined variable");
      });
    });

    describe("Utilities:", [&]() {
      it("normalize", []() {
        std::string str = "some text";
        expect(Utilities::normalize(str, (NormalizationForm)-1)).toBe(str);
      });

      it("hasSuffix", []() {
        std::string str1 = "string";
        std::string str2 = "some text lives inside this string";
        std::string str3 = "not exist";

        expect(Utilities::hasSuffix(str2, str1)).toBe(true);
        expect(Utilities::hasSuffix(str1, str2)).toBe(false);
        expect(Utilities::hasSuffix(str1, str3)).toBe(false);
      });
    });
    
    auto testResults = InternalTests::_context;
    
    JSObject result(currentContext);
    JSArray describeArray(currentContext);

    result.defineProperty("total", testResults.getTotals()._total);
    result.defineProperty("succeed", testResults.getTotals()._succeeded);
    result.defineProperty("result", testResults.getTotals().allOk());
    result.defineProperty("describes", describeArray);
    
    for (auto pair1 : testResults._stats) {
      JSObject currentDescribe(currentContext);
      JSArray itArray(currentContext);
      
      describeArray.addValue(currentDescribe);

      currentDescribe.defineProperty("name", pair1.first);
      currentDescribe.defineProperty("its", itArray);
      
      for(auto pair2 : pair1.second) {
        TestCounters &counters = pair2.second;
        JSObject itResults(currentContext);
        JSArray errors(currentContext);

        itResults.defineProperty("total", counters._total);
        itResults.defineProperty("succeed", counters._succeeded);
        itResults.defineProperty("result", counters.allOk());
        itResults.defineProperty("errors", errors);
        
        for (auto error : counters._failures) {
          JSObject currentError(currentContext);
          currentError.defineProperty("filename", error._filename);
          currentError.defineProperty("line", error._line);
          currentError.defineProperty("message", error._message);
          
          errors.addValue(currentError);
        }
        
        itArray.addValue(itResults);
      }
    }

    args.pushResult(result);
  });
  
}

//----------------------------------------------------------------------------------------------------------------------

bool InternalTests::_registered = []() {
  ScriptingContext::registerModule("tests", &InternalTests::activate);
  return true;
}();
