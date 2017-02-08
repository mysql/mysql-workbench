/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "test.h"
#include "mforms/jsonview.h"
#include "wb_helpers.h"

using namespace mforms;
using namespace JsonParser;

BEGIN_TEST_DATA_CLASS(mforms_json_test)
private:
WBTester _tester; // Initializes the mforms stub with good values (among others).
END_TEST_DATA_CLASS;

TEST_MODULE(mforms_json_test, "mforms json paser testing");

//--------------------------------------------------------------------------------------------------

TEST_FUNCTION(1) {
  std::string json =
    "{\"menu\":{\"id\":\"file\",\"value\":\"File\",\"popup\":{\"menuitem\":[{\"value\":\"New\",\"onclick\""
    ":\"CreateNewDoc()\"},{\"value\":\"Open\",\"onclick\":\"OpenDoc()\"},{\"value\":\"Close\",\"onclick\":\""
    "CloseDoc()\"},{\"value\":true,\"onclick\":null},{\"value\":false,\"onclick\":true}]}}}";
  // test reader
  JsonParser::JsonValue value;
  try {
    JsonParser::JsonReader::read(json, value);
  } catch (JsonParser::ParserException &ex) {
    g_message("Parse error: '%s'", ex.what());
    throw;
  }

  // test writer
  std::string str;
  JsonParser::JsonWriter::write(str, value);

  JsonParser::JsonValue value2;
  try {
    JsonParser::JsonReader::read(str, value2);
  } catch (JsonParser::ParserException &ex) {
    g_message("Parse error: '%s'", ex.what());
    throw;
  }
}

TEST_FUNCTION(5) {
  std::string json =
    "{\"menu\":{\"header\":\"SVGViewer\",\"items\":[{\"id\":\"Open\"},{\"id\":\"OpenNew\""
    ",\"label\":\"OpenNew\"},null,{\"id\":\"ZoomIn\",\"label\":\"ZoomIn\""
    "},{\"id\":\"ZoomOut\",\"label\":\"ZoomOut\"},1,2,3,{\"id\":\"OriginalView\",\"label\":\""
    "OriginalView\"},null,{\"id\":\"Quality\"},{\"id\":\"Pause\"},{\"id\":\"Mute\""
    "},null,{\"id\":\"Find\",\"label\":\"Find...\"},{\"id\":\"FindAgain\",\"label\":\"FindAgain\"},{\""
    "id\":\"Copy\"},{\"id\":\"CopyAgain\",\"label\":\"CopyAgain\"},{\"id\":\"CopySVG\",\"label\":\""
    "CopySVG\"},{\"id\":\"ViewSVG\",\"label\":\"ViewSVG\"},{\"id\":\"ViewSource\",\"label\":\"ViewSource\""
    "},{\"id\":\"SaveAs\",\"label\":\"SaveAs\"},null,{\"id\":\"Help\"},{\"id\":\"About\",\"label\":"
    "\"AboutAdobeCVGViewer...\"}]}}";
  // test reader
  JsonParser::JsonValue value;
  try {
    JsonParser::JsonReader::read(json, value);
  } catch (JsonParser::ParserException &ex) {
    g_message("Parse error: '%s'", ex.what());
    throw;
  }

  // test writer
  std::string str;
  JsonParser::JsonWriter::write(str, value);

  JsonParser::JsonValue value2;
  try {
    JsonParser::JsonReader::read(str, value2);
  } catch (JsonParser::ParserException &ex) {
    g_message("Parse error: '%s'", ex.what());
    throw;
  }
}

TEST_FUNCTION(10) {
  /* json structure
  {
    "colorsArray" : [{
      "name":"red",
      "hexValue":"#f00"
      },
      {
      "name":"green",
      "hexValue":"#0f0"
      },
      {
      "name":"blue",
      "hexValue":"#00f"
      },
      {
      "name":"cyan",
      "hexValue":"#0ff"
      },
      {
      "name":"magenta",
      "hexValue":"#f0f"
      },
      {
      "name":"yellow",
      "hexValue":"#ff0"
      },
      {
      "name":"black",
      "hexValue":"#000"
      }
    ]
 }*/

  JsonArray jsonArray;
  JsonObject obj1;
  obj1["name"] = JsonValue("red");
  obj1["hexValue"] = JsonValue("#f00");
  JsonObject obj2;
  obj2["name"] = JsonValue("green");
  obj2["hexValue"] = JsonValue("#0f0");
  JsonObject obj3;
  obj3["name"] = JsonValue("blue");
  obj3["hexValue"] = JsonValue("#00f");

  jsonArray.pushBack(JsonValue(obj1));
  jsonArray.pushBack(JsonValue(obj2));
  jsonArray.pushBack(JsonValue(obj3));

  JsonObject object;
  object["colorsArray"] = JsonValue(jsonArray);
  JsonValue value(object);

  // try cast JsonObject to double
  bool exceptionThrown = false;
  try {
    double WB_UNUSED number = (double)value;
  } catch (const std::bad_cast &) {
    exceptionThrown = true;
  }
  ensure_true("bad cast should be thrown", exceptionThrown);
  const JsonObject &objRoot = (const JsonObject &)value;

  // check getter
  auto value1 = objRoot.get("colorsArray");

  auto jsArray = (const JsonArray &)value1;
  ensure_true("It should be JsonArray and it should contains tree elements", jsArray.size() == 3);
  for (auto &entry : jsonArray) {
    ensure_true("Array should contains JsonObjects", entry.getType() == VObject);
    auto value2 = (const JsonObject &)entry;
    ensure_true("Every object in array should contains two elemants", value2.size() == 2);
  }
}

TEST_FUNCTION(15) {
  std::string missingComma = "[1, 2 3]";
  // test reader with incomplete JSON
  bool exceptionThrown = false;
  JsonParser::JsonValue value;
  try {
    JsonParser::JsonReader::read(missingComma, value);
  } catch (JsonParser::ParserException &) {
    exceptionThrown = true;
  }
  ensure_true("Exception should be thrown", exceptionThrown);

  exceptionThrown = false;
  std::string badDocument = "[true, false, true, [ss]]s ssss";
  JsonParser::JsonValue value2;
  try {
    JsonParser::JsonReader::read(missingComma, value2);
  } catch (JsonParser::ParserException &) {
    exceptionThrown = true;
  }
  ensure_true("Exception should be thrown", exceptionThrown);
}

//--------------------------------------------------------------------------------------------------

END_TESTS;

//--------------------------------------------------------------------------------------------------
;
