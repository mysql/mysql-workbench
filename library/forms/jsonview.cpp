/* 
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include <memory>
#include <set>
#include <sstream>

#include "mforms/jsonview.h"
#include <mforms/panel.h>
#include <mforms/treenodeview.h>
//#include <mforms/gridview.h>
#include <mforms/code_editor.h>
#include <mforms/tabview.h>
#include <stub/stub_utilities.h>
#include <base/string_utilities.h>

using namespace mforms;
using namespace JsonParser;

/**
 * Implementation of the json parser view, which is the base for most of the visual controls in mforms.
 */


/**
*  @brief  Default constructor creates empty element.
*/
JsonObject::JsonObject()
{
}

/**
*  @brief  Move constructor.
*  @param other A JsonObject of identical element and allocator types.
*/
JsonObject::JsonObject(JsonObject &&other) 
  : _data(std::move(other._data))
{
}

/**
*  @brief  Dfault constructor creates empty element.
*/
JsonObject &JsonObject::operator=(JsonObject &&other)
{
  _data = std::move(other._data);
  return *this;
}

/**
* @brief Returns a read/write iterator
* 
* Returns a read/write iterator that points to the first
* element in the JsonObjeect container. Iteration is done in accessing order according to the key.
*
* @return iterator for begining of sequence
**/
JsonObject::Iterator JsonObject::begin()
{
  return _data.begin();
}

/**
* @brief Returns a readonly iterator
*
* Returns a readonly iterator that points to the first
* element in the JsonObjeect container. Iteration is done in accessing order according to the key.
*
* @return iterator for begining of sequence
**/
JsonObject::ConstIterator JsonObject::begin() const
{
  return _data.begin();
}

/**
* @brief Returns a readonly iterator
*
* Returns a readonly iterator that points to the first
* element in the JsonObjeect container. Iteration is done in accessing order according to the key.
*
* @return iterator for begining of sequence
**/
JsonObject::ConstIterator JsonObject::cbegin() const
{
  return _data.begin();
}

/**
* @brief Returns iterator for end of sequence
*
* Returns a readonly iterator that points to one past the last pairin the JsonObjeect container.
*
* @return iterator for end of sequence
**/
JsonObject::Iterator JsonObject::end()
{
  return _data.end();
}

/**
* @brief
*
* @param
* @return
**/JsonObject::ConstIterator JsonObject::end() const
{
  return _data.end();
}

/**
* @brief
*
* @param
* @return
**/JsonObject::ConstIterator JsonObject::cend() const
{
  return _data.end();
}

/**
* @brief
*
* @param
* @return
**/JsonObject::SizeType JsonObject::size()
{
  return _data.size();
}

/**
* @brief
*
* @param
* @return
**/JsonObject::Iterator JsonObject::find(const KeyType& key)
{
  return _data.find(key);
}

JsonObject::JsonObject(const JsonObject& other) : _data(other._data)
{
  
}

JsonObject &JsonObject::operator=(const JsonObject &other)
{ 
  //if (*this != other)
  _data = other._data;
  return *this; 
}

/**
* @brief
*
* @param
* @return
**/JsonObject::ConstIterator JsonObject::find(const KeyType& key) const
{
  return _data.find(key);
}

/**
* @brief
*
* @param
* @return
**/
// test if container is empty
bool JsonObject::empty() const
{
  return _data.empty();
}

/**
* @brief
*
* @param
* @return
**/void JsonObject::clear()
{
  _data.clear();
}

/**
* @brief
*
* @param
* @return
**/
JsonObject::Iterator JsonObject::erase(Iterator it)
{
  return _data.erase(it);
}

/**
* @brief
*
* @param
* @return
**/
JsonObject::Iterator JsonObject::erase(Iterator first, Iterator last)
{
  return _data.erase(first, last);
}

/**
* @brief
*
* @param
* @return
**/
void JsonObject::insert(const KeyType &key, const JsonValue& value)
{
  _data[key] = value;
}

/**
* @brief
*
* @param
* @return
**/
JsonValue &JsonObject::get(const KeyType &key)
{
    return _data[key];;
}

// Default constructor
/**
* @brief
*
* @param
* @return
**/
JsonArray::JsonArray()
{
}


// move operations
/**
* @brief
*
* @param
* @return
**/
JsonArray::JsonArray(JsonArray &&other)
  : _data(std::move(other._data))
{
}

/**
* @brief
*
* @param
* @return
**/
JsonArray &JsonArray::operator =(JsonArray &&other)
{
  _data = std::move(other._data);
  return *this;
}

/**
* @brief
*
* @param
* @return
**/
// subscript sequence with checking
JsonValue& JsonArray::at(SizeType pos)
{
  assert(pos < _data.size());
  return _data.at(pos);
}

/**
* @brief
*
* @param
* @return
**/
const JsonValue &JsonArray::at(SizeType pos) const
{
  assert(pos < _data.size());
  return _data.at(pos);
}

/**
* @brief
*
* @param
* @return
**/
// subscript sequence
JsonValue &JsonArray::operator[](SizeType pos)
{
  assert(pos < _data.size());
  return _data[pos];
}

/**
* @brief
*
* @param
* @return
**/
const JsonValue &JsonArray::operator[](SizeType pos) const
{
  assert(pos < _data.size());
  return _data[pos];
}


/**
* @brief
*
* @param
* @return
**/
// return iterator for begining of sequence
JsonArray::Iterator JsonArray::begin()
{
  return _data.begin();
}

/**
* @brief
*
* @param
* @return
**/
JsonArray::ConstIterator JsonArray::begin() const
{
  return _data.begin();
}

/**
* @brief
*
* @param
* @return
**/
JsonArray::ConstIterator JsonArray::cbegin() const
{
  return _data.begin();
}

/**
* @brief
*
* @param
* @return
**/
// return iterator for end of sequence
JsonArray::Iterator JsonArray::end()
{
  return _data.end();
}

/**
* @brief
*
* @param
* @return
**/
JsonArray::ConstIterator JsonArray::end() const
{
  return _data.end();
}

/**
* @brief
*
* @param
* @return
**/
JsonArray::ConstIterator JsonArray::cend() const
{
  return _data.end();
}

/**
* @brief
*
* @param
* @return
**/
// return length of sequence
JsonArray::SizeType JsonArray::size()
{
  return _data.size();
}

/**
* @brief
*
* @param
* @return
**/
// test if container is empty
bool JsonArray::empty() const
{
  return _data.empty();
}

/**
* @brief
*
* @param
* @return
**/
void JsonArray::clear()
{
  _data.clear();
}


/**
* @brief
*
* @param
* @return
**/
JsonArray::Iterator JsonArray::erase(Iterator pos)
{
  return _data.erase(pos);

}

/**
* @brief
*
* @param
* @return
**/
JsonArray::Iterator JsonArray::erase(Iterator first, Iterator last)
{
  return _data.erase(first, last);
}

/**
* @brief
*
* @param
* @return
**/
// insert value at pos
JsonArray::Iterator JsonArray::insert(Iterator pos, const JsonValue& value)
{
  return _data.insert(pos, value);
}

/**
* @brief
*
* @param
* @return
**/
// insert element at end
void JsonArray::pushBack(const ValueType& value)
{
  _data.push_back(value);
}

/**
* @brief
*
* @param
* @return
**/
JsonValue::JsonValue()
  : _double(0), _bool(false), _type(VEmpty)
{
}

/**
* @brief
*
* @param
* @return
**/
  // Copy operations
JsonValue::JsonValue(const JsonValue& rhs)
   : _double(rhs._double), _bool(rhs._bool), _string(rhs._string), _object(rhs._object),  
   _array(rhs._array), _type(rhs._type)
{
}


/**
* @brief
*
* @param
* @return
**/
JsonValue::JsonValue(JsonValue&& rhs)
   : _double(rhs._double), _bool(rhs._bool), _string(std::move(rhs._string)), _object(std::move(rhs._object)),
   _array(std::move(rhs._array)), _type(rhs._type)
{
}

/**
* @brief
*
* @param
* @return
**/
JsonValue &JsonValue::operator=(const JsonValue& rhs)
{
  _double = rhs._double;
  _bool = rhs._bool;
  _string = rhs._string;
  _object = rhs._object;
  _array = rhs._array;
  _type = rhs._type;
  return *this;
}

/**
* @brief
*
* @param
* @return
**/
// move operations
JsonValue& JsonValue::operator=(JsonValue&& rhs)
{
  _double = rhs._double;
  _bool = rhs._bool;
  _string = std::move(rhs._string);
  _object = std::move(rhs._object);
  _array = std::move(rhs._array);
  _type = rhs._type;
  return *this;
}

/**
* @brief
*
* @param
* @return
**/
JsonValue::JsonValue(const std::string& val)
   : _double(0), _bool(false), _string(val), _type(VString)
{
}
    
/**
* @brief
*
* @param
* @return
**/
JsonValue::JsonValue(std::string&& val)
  : _double(0), _bool(false), _string(std::move(val)), _type(VString)
{
}

/**
* @brief
*
* @param
* @return
**/
JsonValue::JsonValue(const char* val)
  : _double(0), _bool(false), _type(VString)
{
  _string = std::string(val != nullptr ? val : "");
}


/**
* @brief construct from bool
*
* @param
* @return
**/
  // 
JsonValue::JsonValue(bool val)
  : _double(0), _bool(val), _type(VBoolean)
{
}

// construct from int
/**
* @brief construct from bool
*
* @param
* @return
**/
JsonValue::JsonValue(int val)
  : _double(val), _bool(false), _type(VInt)
{
}

/**
* @brief construct from bool
*
* @param
* @return
**/
// construct from double
JsonValue::JsonValue(double val)
  : _double(val), _bool(false), _type(VDouble)
{
}

/**
* @brief
*
* @param
* @return
**/
JsonValue::JsonValue(const JsonObject &val)
  : _double(0), _bool(false), _object(val), _type(VObject)
{
}

/**
* @brief
*
* @param
* @return
**/
JsonValue::JsonValue(JsonObject&& val)
  : _double(0), _bool(false), _object(std::move(val)), _type(VObject)
{
}

/**
* @brief
*
* @param
* @return
**/
// // construct from JsonArray
JsonValue::JsonValue(const JsonArray &val)
  : _double(0), _bool(false), _array(val), _type(VObject)
{
}

/**
* @brief
*
* @param
* @return
**/
JsonValue::JsonValue(JsonArray &&val)
  : _double(0), _bool(false), _array(std::move(val)), _type(VObject)
{
}

/**
* @brief
*
* @param
* @return
**/
// return type of value
DataType JsonValue::getType() const
{
  return _type;
}

/**
* @brief
*
* @param
* @return
**/
// access function
double JsonValue::getDouble() const
{

  return _double;
}

/**
* @brief
*
* @param
* @return
**/
void JsonValue::setNumber(double val)
{
  _double = val;
}

/**
* @brief
*
* @param
* @return
**/
int JsonValue::getInt() const
{
  return static_cast<int>(_double);
}

/**
* @brief
*
* @param
* @return
**/
bool JsonValue::getBool() const
{
  return _bool;
}

/**
* @brief
*
* @param
* @return
**/
void JsonValue::setBool(bool val)
{
  _bool = val;
}

/**
* @brief
*
* @param
* @return
**/
const std::string& JsonValue::getString() const
{
  return _string;
}

/**
* @brief
*
* @param
* @return
**/
void JsonValue::setString(const std::string& val)
{
  _string = val;
}

/**
* @brief
*
* @param
* @return
**/
JsonObject &JsonValue::getObject()
{
  return _object;
}

/**
* @brief
*
* @param
* @return
**/
const JsonObject& JsonValue::getObject() const
{
  return _object;
}

/**
* @brief
*
* @param
* @return
**/
void JsonValue::setObject(const JsonObject& val)
{
  _object = val;
}

/**
* @brief
*
* @param
* @return
**/
JsonArray &JsonValue::getArray()
{
  return _array;
}

/**
* @brief
*
* @param
* @return
**/
const JsonArray& JsonValue::getArray() const
{
  return _array;
}

/**
* @brief
*
* @param
* @return
**/
void JsonValue::setType(DataType type)
{
   _type = type;
}

/**
* @brief
*
* @param
* @return
**/
void JsonValue::setArray(const JsonArray& val)
{
  _array = val;
}

/**
* @brief
*
* @param
* @return
**/
JsonReader::JsonReader(const std::string &str) 
  : _jsonText(str), _actualPos(0)
{
}

/**
* @brief
*
* @param
* @return
**/
char JsonReader::peek()
{
  return (_actualPos < _jsonText.length()) ? _jsonText[_actualPos] : static_cast<char>(0);
}

/**
* @brief
*
* @param
* @return
**/
bool JsonReader::eos()
{
  return _actualPos == _jsonText.length();
}

/**
* @brief
*
* @param
* @return
**/
bool JsonReader::isWhiteSpace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/**
* @brief
*
* @param
* @return
**/
void JsonReader::eatWhitespace()
{
  while(isWhiteSpace(peek()))
    moveAhead();
}

/**
* @brief
*
* @param
* @return
**/
void JsonReader::moveAhead()
{
  _actualPos = std::min(_actualPos + 1, _jsonText.length());
}

/**
* @brief
*
* @param
* @return
**/
void JsonReader::read(const std::string &str, JsonValue &value)
{
  JsonReader reader(str);
  reader.scan();
  reader.parse(value);
}


/**
* @brief
*
* @param
* @return
**/
void JsonReader::scan()
{
  while (!eos())
  {
    eatWhitespace();
    std::string value;
    auto type = JsonToken::JsonTokenEmpty;
    auto chr = peek();
    switch (chr)
    {
    case '{':
      value += chr;
      type = JsonToken::JsonTokenObjectStart;
      moveAhead();
      break;

    case '}':
      value += chr;
      type = JsonToken::JsonTokenObjectEnd;
      moveAhead();
      break;

    case '[':
      value += chr;
      type = JsonToken::JsonTokenArrayStart;
      moveAhead();
      break;

    case ']':
      value += chr;
      type = JsonToken::JsonTokenArrayEnd;
      moveAhead();
      break;

    case ',':
      value += chr;
      type = JsonToken::JsonTokenNext;
      moveAhead();
      break;

    case ':':
      value += chr;
      type = JsonToken::JsonTokenAssign;
      moveAhead();
      break;

    case '"':
      value = getJsonString();
      type = JsonToken::JsonTokenString;
      break;

    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      value = getJsonNumber();
      type = JsonToken::JsonTokenNumber;
      break;
    case 't':
    case 'f':
      value = getJsonBoolean();
      type = JsonToken::JsonTokenBoolean;
      break;
    case 'n':
      checkJsonNull();
      type = JsonToken::JsonTokenEmpty;
      break;
    case 0:
      break;
    default:
      throw ParserException(std::string("Unexpected start sequence: ") + chr); // @@FIXMEE
    }
    _tokens.push_back(JsonToken(type, value));
  }
  _tokenIterator = _tokens.cbegin();
  _tokenEnd = _tokens.cend();
}

/**
* @brief
*
* @param
* @return
**/
bool JsonReader::match(const std::string &text)
{
  bool match = !text.empty();
  auto cend = text.cend();
  std::for_each(text.cbegin(), cend, [&match, this](char value)
  {
    if (eos() || value != peek())
    {
      match = false;
      return;
    }
    moveAhead();
  });
  return match;
}

/**
* @brief
*
* @param
* @return
**/
std::string JsonReader::getJsonString()
{
  moveAhead();
  std::string string;
  while (eos() == false && peek() != '"')
  {
    char currentChar = peek();
    moveAhead();
    if (currentChar == '\\' && !eos())
    {
      currentChar = peek();
      moveAhead();
      switch (currentChar)
      {
      case '/': 
      case '"':
      case '\\':
        string += currentChar;
        break;
      case 'b':
        string += '\b';
        break;
      case 'f':
        string += '\f';
        break;
      case 'n':
        string += '\n';
        break;
      case 'r':
        string += '\r';
        break;
      case 't':
        string += '\t';
        break;
      /*case 'u':
        string += '\x';*/
      default:
          throw ParserException(std::string("Unrecognized escape sequence: \\") + currentChar);
      }
    }
    else
    {
      string.push_back(currentChar);
    }
  }
  if (!match("\""))
    throw ParserException(std::string("Expected: \" "));
  return string;
}

/**
* @brief
*
* @param
* @return
**/
void JsonReader::checkJsonNull()
{
  const auto size = 4;
  std::string nullString;
  for (auto i = 0; i < size && !eos(); ++i)
  {
    nullString += peek();
    moveAhead();
  }
  if (nullString.compare("null") != 0)
    throw ParserException(std::string("Unexpected token: ") + nullString);
}

/**
* @brief
*
* @param
* @return
**/
std::string JsonReader::getJsonBoolean()
{
  const auto size = peek() == 'f' ? 5 : 4;
  std::string boolString;
  for (auto i = 0; i < size && !eos(); ++i)
  {
    boolString += peek();
    moveAhead();
  }
  if (boolString.compare("true") != 0 && boolString.compare("false") != 0)
    throw ParserException(std::string("Unexpected token: ") + boolString);

  return boolString;
}


/**
* @brief
*
* @param
* @return
**/
std::string JsonReader::getJsonNumber()
{
  std::set<char> numericChars = {'0','1','2','3','4','5','6','7','8','9','.','e','E','-','+'};
  std::string number;
  while (eos() == false && numericChars.count(peek()) == 1)
  {
    number += peek();
    moveAhead();
  }
  return number;
}

/**
* @brief
*
* @param
* @return
**/
bool JsonReader::match(JsonToken::JsonTokenType type, bool skip /*= false*/, bool mustMatch/* = true*/)
{
  auto ret = _tokenIterator != _tokenEnd && _tokenIterator->getType() == type;
  if (!ret && mustMatch)
  {
    std::string message;
    if (_tokenIterator != _tokenEnd)
      message = std::string("Unexpected token: ") + _tokenIterator->getValue();
    else
    message = std::string("Not compleated json data");
    throw ParserException(message);
  }
  if (skip && ret)
  {
    ++_tokenIterator;
    ret = _tokenIterator != _tokenEnd;
  }
  return ret;
}

/**
* @brief
*
* @param
* @return
**/
void JsonReader::parse(JsonObject &obj)
{
  bool go = match(JsonToken::JsonTokenObjectStart, true) && _tokenIterator->getType() != JsonToken::JsonTokenObjectStart;
  while (go)
  {
    // the member name
    match(JsonToken::JsonTokenString);
    std::string name = _tokenIterator->getValue();
    ++_tokenIterator;

    // assign separator
    match(JsonToken::JsonTokenAssign, true);

    JsonValue value;
    // json value
    parse(value);
    if (obj.find(name) != obj.end())
      throw ParserException(std::string("Duplicate member: ") + name);

    obj.insert(name, value);

    go = match(JsonToken::JsonTokenNext, true, false);
  }
  match(JsonToken::JsonTokenObjectEnd, true);
}

/**
* @brief
*
* @param
* @return
**/
void JsonReader::parseNumber(JsonValue& value)
{
   std::stringstream buffer;
   buffer << _tokenIterator->getValue();
   double number = 0;
   buffer >> number;
   double intpart = 0;
   if (std::modf(number, &intpart) == 0.0)
      value.setType(VInt);
   else
      value.setType(VDouble);
   value.setNumber(number);
   ++_tokenIterator;
}

/**
* @brief
*
* @param
* @return
**/
void JsonReader::parseBoolean(JsonValue& value)
{
   auto boolean = (_tokenIterator->getValue() == "true" ? true : false);
   value.setBool(boolean);
   value.setType(VBoolean);
   ++_tokenIterator;
}

/**
* @brief
*
* @param
* @return
**/
void JsonReader::parseString(JsonValue& value)
{
   value.setString(_tokenIterator->getValue());
   value.setType(VString);
   ++_tokenIterator;
}

/**
* @brief
*
* @param
* @return
**/
void JsonReader::parseEmpty(JsonValue& value)
{
   value.setType(VEmpty);
   ++_tokenIterator;
}

/**
* @brief
*
* @param
* @return
**/
void JsonReader::parseObject(JsonValue& value)
{
   value.setType(VObject);
   JsonObject &object = value.getObject();
   parse(object);
   //value.setObject(object);
}

/**
* @brief
*
* @param
* @return
**/
void JsonReader::parseArray(JsonValue& value)
{
   value.setType(VArray);
   JsonArray &array = value.getArray();
   bool go = match(JsonToken::JsonTokenArrayStart, true) && _tokenIterator->getType() != JsonToken::JsonTokenArrayStart;
   while (go)
   {
      JsonValue value;
      parse(value);
      array.pushBack(value);
      go = match(JsonToken::JsonTokenNext, true, false);
   }
   match(JsonToken::JsonTokenArrayEnd, true);
}

/**
* @brief parse 
* @param reference to json value to be parsed
**/
void JsonReader::parse(JsonValue &value)
{
  if (_tokenIterator == _tokenEnd)
     throw ParserException("Unexpected json data end.");
  auto type = _tokenIterator->getType();
  switch (type)
  {
  case JsonToken::JsonTokenString: 
    parseString(value);
    break;
  case JsonToken::JsonTokenNumber:
    parseNumber(value);
    break;
  case JsonToken::JsonTokenBoolean:
    parseBoolean(value);
    break;
  case JsonToken::JsonTokenEmpty: 
    parseEmpty(value);
    break;
  case JsonToken::JsonTokenObjectStart: 
    parseObject(value);
    break;
  case JsonToken::JsonTokenArrayStart: 
    parseArray(value);
    break;
  default: 
     throw ParserException(std::string("Unexpected token: ") + _tokenIterator->getValue());
  }
}


/**
* -------------------------------------------------------------------------------------------------
*   JSON writer implementation 
* -------------------------------------------------------------------------------------------------
*/

JsonWriter::JsonWriter(const JsonValue& value)
  : _jsonValue(value), _depth(0)
{
}

/**
* @brief
*
* @param
* @return
**/
void JsonWriter::write(std::string& str, const JsonValue& value)
{
  JsonWriter writer(value);
  writer.toString(str);
}

/**
* @brief
*
* @param
* @return
**/
void JsonWriter::toString(std::string& output)
{
  generate(output);
}

/**
* @brief
*
* @param
* @return
**/
void JsonWriter::generate(std::string& output)
{
  write(_jsonValue);
  output = _output;
}

/**
* @brief
*
* @param
* @return
**/
void JsonWriter::write(const JsonValue& value)
{
  switch(value.getType())
  {
  case VInt: 
    _output += std::to_string(value.getInt());
    break;
  case VBoolean: 
    _output += value.getBool() ? "true" : "false";
    break;
  case VString: 
    write(value.getString());
    break;
  case VDouble: 
    _output += std::to_string(value.getDouble());
    break;
  case VObject:
    write(value.getObject());
    break;
  case VArray:
    write(value.getArray());
    break;
  case VEmpty:
    _output += "null";
    break;
  default: break;
  }
}

/**
* @brief
*
* @param
* @return
**/
void JsonWriter::write(const JsonObject& value)
{
  _output += "{";
  ++_depth;
  auto end = value.cend();
  auto finalIter = end;
  if (!value.empty())
  {
    _output += "\n";
    --finalIter;
  }
  for (auto it = value.cbegin();  it != end; ++it)
  {
    _output += std::string(_depth, '\t');
    write(it->first);
    _output += " : ";
    write(it->second);
    if (it != finalIter)
      _output += ",";
    _output += "\n";
  }
  --_depth;
  _output += std::string(_depth, '\t');
  _output += "}";
}

/**
* @brief
*
* @param
* @return
**/
void JsonWriter::write(const JsonArray& value)
{
  _output += "[";
  ++_depth;
  auto end = value.cend();
  auto finalIter = end;
  if (!value.empty())
  {
    _output += "\n";
    --finalIter;
  }
  for (auto it = value.cbegin(); it != end; ++it)
  {
    _output += std::string(_depth, '\t');
    write(*it);
    if (it != finalIter)
      _output += ",";
    _output += "\n";
  }
  --_depth;
  _output += std::string(_depth, '\t');
  _output += "]";
}

/**
* @brief
*
* @param
* @return
**/
void JsonWriter::write(const std::string& value)
{
  _output += '"';
  for (auto character : value)
  {
    switch (character)
    {
    case '"':
      _output += "\\\"";
      break;
    case '\\':
      _output += "\\\\";
      break;
    case '\b':
      _output += "\\b";
      break;
    case '\f':
      _output += "\\f";
      break;
    case '\n':
      _output += "\\n";
      break;
    case '\r':
      _output += "\\r";
      break;
    case '\t':
      _output += "\\t";
      break;
    //case '\x':
    //  _output += "\\u";
    //  break;
    default:
      _output += character;
      break;
    }
  }
  _output += '"';
}

/**
* @brief default constructor.
*
**/
JsonBaseView::JsonBaseView() : Panel(TransparentPanel)
{
}

/**
* @brief default destructor.
*
**/
JsonBaseView::~JsonBaseView()
{
}

/**
* @brief
*
* @param
* @return
**/
//static std::string JsonBaseView::getNodeIconPath(JsonBaseView::JsonNodeIcons icon);
//{
//  bec::IconId iconid;
//  switch (icon)
//  {
//  case JsonObjectIcon:
//    iconid = bec::IconManager::get_instance()->get_icon_id("db.Table.many.$.png", bec::Icon16);
//    break;
//  case JsonArrayIcon:
//    iconid = bec::IconManager::get_instance()->get_icon_id("db.Table.$.png", bec::Icon16);
//    break;
//  case JsonStringIcon:
//    iconid = bec::IconManager::get_instance()->get_icon_id("db.View.many.$.png", bec::Icon16);
//    break;
//  case JsonNumericIcon:
//    iconid = bec::IconManager::get_instance()->get_icon_id("db.View.$.png", bec::Icon16);
//    break;
//  case JsonNullIcon:
//    iconid = bec::IconManager::get_instance()->get_icon_id("db.Routine.many.$.png", bec::Icon16);
//    break;
//  default:
//    return "";
//  }
//}

/**
* @brief
*
* @param
* @return
**/
boost::signals2::signal<void()>* JsonBaseView::signalChanged()
{
  return &_signalChanged;
}

/// <summary>
/// The Json data as string to add to the control.
/// </summary>
/// <param name="text">A string that contains the json text data to set.</param>
//void JsonBaseView::setText(const std::string &text)
//{
//  //_jsonText = text;
//}

/// <summary>
/// The Json data to add to the control.
/// </summary>
/// <param name="text">A JsonValue object that contains the json text data to set.</param>
//void JsonBaseView::setJson(const JsonValue &val)
//{
//  //_json = val;
//}

/// <summary>
/// Retrieves data from the Json control in the text format
/// <summary>
/// <returns>Returns a string that represents the current control data.</returns>
//const JsonValue &JsonBaseView::getJson() const
//{
//  //return _json;
//}
//
/// <summary>
/// Retrieves data from the Json control in the text format
/// <summary>
/// <returns>Returns a string that represents the current control data.</returns>
//const std::string &JsonBaseView::getText() const
//{
// // return _jsonText;
//}
//

/**
* @brief Constructor
*
**/
JsonTextView::JsonTextView() 
  :  _textEditor(std::make_shared<CodeEditor>())
{
  init();
  scoped_connect(_textEditor->signal_changed(), boost::bind(&JsonTextView::textChanged, this));
}

/**
* @brief
*
* @param
* @return
**/
void JsonTextView::textChanged()
{
  _signalChanged();
}

/**
* @brief
*
* @param
* @return
**/
void JsonTextView::setText(const std::string& jsonText)
{
  _textEditor->set_value(jsonText.c_str());
  _textEditor->set_features(mforms::FeatureReadOnly, false);
}

/**
* @brief Destructor.
*
**/
JsonTextView::~JsonTextView()
{
}

/**
* @brief Init controls
*
**/
void JsonTextView::init()
{
  assert(_textEditor.get() != nullptr);
  _textEditor->set_language(mforms::LanguageJson);
  _textEditor->set_features(mforms::FeatureWrapText, false);
  add(_textEditor.get());
}


/**
* @brief Constructor
*
**/
JsonTreeView::JsonTreeView()
   : _treeView(std::make_shared<mforms::TreeNodeView>(TreeNoBorder | TreeShowColumnLines))
{
  _treeView->add_column(IconStringColumnType, "", 150, false, true);
  _treeView->add_column(IconStringColumnType, "Value", 200, false, true);
  _treeView->end_columns();
  init();
}

/**
* @brief Destructor
*
**/
JsonTreeView::~JsonTreeView()
{
}

/**
* @brief Init tree view
*
* Based of readed json data control function initialize mforms control TreNodeView
**/
void JsonTreeView::init()
{
  assert(_treeView.get() != nullptr);
  add(_treeView.get());
}


void JsonTreeView::setJson(const JsonParser::JsonValue& value)
{
  _treeView->clear();
  TreeNodeRef node = _treeView->root_node();
  generateTree(value, node);
}


void JsonTreeView::generateObjectInTree(const JsonParser::JsonValue& value, TreeNodeRef node, bool addNew)
{
  auto object = value.getObject();
  size_t size = 0;
  std::stringstream textSize;
  auto end = object.cend();
  for (auto it = object.cbegin(); it != end; ++it)
  {
    auto text = it->first;
    switch (it->second.getType())
    {
    case VArray:
    {
      auto arrayVal = it->second.getArray();
      size = arrayVal.size();
      textSize << size;
      text += "[";
      text += textSize.str();
      text += "]";
      break;
    }
    case VObject:
    {
      auto objectVal = it->second.getObject();
      size = objectVal.size();
      textSize << size;
      text += "{";
      text += textSize.str();
      text += "}";
      break;
    }
    }
    mforms::TreeNodeRef node2 = (addNew) ? node->add_child() : node;
    node->set_icon_path(0, "json_obj.png");
    node2->set_string(0, text);
    node2->set_string(1, "");
    generateTree(it->second, node2);
    node2->expand();
  }
}


void JsonTreeView::generateArrayInTree(const JsonParser::JsonValue& value, TreeNodeRef node, bool addNew)
{
  auto arrayType = value.getArray();
  auto size = arrayType.size();
  std::stringstream textSize;
  textSize << size;
  auto text = "array [" + textSize.str() + "]";
  mforms::TreeNodeRef node2 = node->add_child();
  node->set_icon_path(0, "json_arr.png");
  node2->set_string(0, text);
  node2->set_string(1, "");
  auto end = arrayType.cend();
  int idx = 0;
  for (auto it = arrayType.cbegin(); it != end; ++it, ++idx)
  {
    mforms::TreeNodeRef arrrayNode = node2->add_child();
    bool addNew = false;
    if (it->getType() == VArray || it->getType() == VObject)
      addNew = true;
    arrrayNode->set_string(0, base::strfmt("[%d] {%d}", idx, arrayType.size()));
    arrrayNode->set_string(1, "");
    generateTree(*it, arrrayNode, addNew);
  }
  node2->expand();
}


void JsonTreeView::generateTree(const JsonParser::JsonValue &value, TreeNodeRef node, bool addNew)
{
  switch (value.getType())
  {
  case VInt:
    node->set_icon_path(0, "json_nmb.png");
    node->set_attributes(1, mforms::TextAttributes("#99cc66", false, true));
    node->set_int(1, value.getInt());
    node->expand();
    break;
  case VBoolean:
    node->set_icon_path(0, "json_nmb.png");
    node->set_attributes(1, mforms::TextAttributes("#0099ff", true, true));
    node->set_bool(1, value.getBool());
    node->expand();
    break;
  case VString:
    node->set_icon_path(0, "json_str.png");
    node->set_attributes(1, mforms::TextAttributes("#cc9966", false, false));
    node->set_string(1, value.getString());
    node->expand();
    break;
  case VDouble:
    node->set_icon_path(0, "json_nmb.png");
    node->set_attributes(1, mforms::TextAttributes("#99cc66", false, true));
    node->set_float(1, value.getDouble());
    node->expand();
    break;
  case VObject:
    generateObjectInTree(value, node, addNew);
    break;
  case VArray:
    generateArrayInTree(value, node, addNew);
    break;
  case VEmpty:
    node->set_icon_path(0, "json_null.png");
    node->set_string(0, "<<null>>");
    node->set_string(1, "");
    node->expand();
    break;
  default: break;
  }
}

/**
* @brief
*
* @param
* @return
**/
JsonGridView::JsonGridView()
{
}

/**
* @brief
*
* @param
* @return
**/
JsonGridView::~JsonGridView()
{
}


///**
//* @brief
//*
//* @param
//* @return
//**/
//void JsonTabView::generateTextOutput(const JsonValue &value)
//{
//  switch (value.getType())
//  {
//  case VInt:
//    _jsonText += std::to_string(value.getInt());
//    break;
//  case VBoolean:
//    _jsonText += value.getBool() ? "true" : "false";
//    break;
//  case VString:
//    _jsonText += "\"";
//    _jsonText += value.getString();
//    _jsonText += "\"";
//    break;
//  case VDouble:
//    _jsonText += std::to_string(value.getDouble());
//    break;
//  case VObject:
//    {
//      auto object = value.getObject();
//      //_jsonText += "(O) \"object\"\n";
//      ++_ident;
//      auto end = object.cend();
//      if (!object.empty())
//        _jsonText += "\n";
//      for (auto it = object.cbegin(); it != end; ++it)
//      {
//        _jsonText += std::string(_ident, '\t');
//        _jsonText += "\"";
//        _jsonText += it->first;
//        _jsonText += "\"";
//        _jsonText += " : ";
//        generateTextOutput(it->second);
//        _jsonText += "\n";
//      }
//      --_ident;
//    }
//    break;
//  case VArray:
//    {
//      auto jsonArray = value.getArray();
//      ++_ident;
//      auto end = jsonArray.cend();
//      if (!jsonArray.empty())
//        _jsonText += "\n";
//      auto idx = 0;
//      for (auto it = jsonArray.cbegin(); it != end; ++it, ++idx)
//      {
//        _jsonText += std::string(_ident, '\t');
//        std::stringstream tmpIdx;
//        tmpIdx << idx;
//        _jsonText += "[";
//        _jsonText += tmpIdx.str();
//        _jsonText += "] ";
//        generateTextOutput(*it);
//        _jsonText += "\n";
//      }
//      --_ident;
//    }
//
//    break;
//  case VEmpty:
//    _jsonText += "<<null>>";
//    break;
//  default: break;
//  }
//}

/**
* @brief
*
* @param
* @return
**/
void JsonTabView::Setup()
{
  assert(_tabView.get() != NULL);
  _tabView->set_name("json_editor:tab");
  _tabView->add_page(manage(_textView.get()), "Text");
  _tabView->add_page(manage(_treeView.get()), "Tree");
  _tabView->add_page(manage(_gridView.get()), "Grid");
  add(_tabView.get());
  scoped_connect(_textView->signalChanged(), boost::bind(&JsonTabView::textViewTextChanged, this));
}

/**
* @brief
*
* @param
* @return
**/
JsonTabView::JsonTabView() : Panel(TransparentPanel), _textView(std::make_shared<JsonTextView>()),
  _treeView(std::make_shared<JsonTreeView>()), _gridView(std::make_shared<JsonGridView>()), 
  _tabView(std::make_shared<TabView>(TabViewPalette))
{
  Setup();
}

/**
* @brief
*
* @param
* @return
**/
JsonTabView::~JsonTabView()
{
}

/**
* @brief
*
* @param
* @return
**/
void JsonTabView::setJson(const JsonParser::JsonValue& value)
{
  _json = value;
  _ident = 0;
  JsonWriter::write(_jsonText, value);
  _textView->setText(_jsonText);
  _treeView->setJson(value);
}

/**
* @brief
*
* @param
* @return
**/void JsonTabView::setText(const std::string& text)
{
  _jsonText = text;
}

/**
* @brief
*
* @param
* @return
**/
void JsonTabView::textViewTextChanged()
{
}
