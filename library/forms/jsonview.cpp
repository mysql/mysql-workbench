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
*  @brief Move assignment operator.
*  @param other A JsonObject of identical element and allocator types.
* 
*  @return returns the reference to assigned value.
*/
JsonObject &JsonObject::operator=(JsonObject &&other)
{
  _data = std::move(other._data);
  return *this;
}

/**
*  @brief Copy constructor.
*  @param other a JsonObject of identical element and allocator types.
*/
JsonObject::JsonObject(const JsonObject& other) : _data(other._data)
{
}

/**
*  @brief Assignment operator.
*  @param other a JsonObject of identical element and allocator types.
*
*  @return returns the reference to assigned value.
*/
JsonObject &JsonObject::operator=(const JsonObject &other)
{
  //if (*this != other)
  _data = other._data;
  return *this;
}

/**
* @brief Returns a read/write iterator.
* 
* Returns a read/write iterator that points to the first
* element in the JsonObject container. Iteration is done in accessing order according to the key.
*
* @return iterator for begining of sequence.
**/
JsonObject::Iterator JsonObject::begin()
{
  return _data.begin();
}

/**
* @brief Returns a readonly iterator.
*
* Returns a readonly iterator that points to the first
* element in the JsonObject container. Iteration is done in accessing order according to the key.
*
* @return iterator for begining of sequence.
**/
JsonObject::ConstIterator JsonObject::begin() const
{
  return _data.begin();
}

/**
* @brief Returns a readonly iterator.
*
* Returns a readonly iterator that points to the first
* element in the JsonObject container. Iteration is done in accessing order according to the key.
*
* @return iterator for begining of sequence.
**/
JsonObject::ConstIterator JsonObject::cbegin() const
{
  return _data.begin();
}

/**
* @brief Returns iterator for end of sequence.
*
* Returns a read/write iterator that points to one past the last pair in the JsonObject container.
*
* @return iterator for end of sequence.
**/
JsonObject::Iterator JsonObject::end()
{
  return _data.end();
}

/**
* @brief Returns a readonly iterator.
*
* Returns a readonly (const) iterator that points to one past the last pair in the JsonObject container.
*
* @return iterator for end of sequence.
**/
JsonObject::ConstIterator JsonObject::end() const
{
  return _data.end();
}

/**
* @brief Returns a readonly iterator.
*
* Returns a readonly (const) iterator that points to one past the last pair in the JsonObject container.
*
* @return iterator for end of sequence.
**/
JsonObject::ConstIterator JsonObject::cend() const
{
  return _data.end();
}

/**
* @brief Returns the size of the JsonObject container.
*
* @return the size of the JsonObject container.
**/
JsonObject::SizeType JsonObject::size()
{
  return _data.size();
}

/**
* @brief Tries to find an element in a JsonObject container.
*
* @param key Kay to be located in the JsonObject container.
* @return Iterator pointing to element, or end() if not found.
**/
JsonObject::Iterator JsonObject::find(const KeyType& key)
{
  return _data.find(key);
}


/**
* @brief Tries to find an element in a JsonObject container.
*
* @param key Kay to be located in the JsonObject container.
* @return Read only iterator pointing to element, or end() if not found.
**/
JsonObject::ConstIterator JsonObject::find(const KeyType& key) const
{
  return _data.find(key);
}

/**
* @brief Test if container is empty.
*
* @return returns true if the JsonObject container is empty.
**/
bool JsonObject::empty() const
{
  return _data.empty();
}

/**
* @brief Erases all elements in a JsonObject container.
*
*/
void JsonObject::clear()
{
  _data.clear();
}

/**
* @brief Erases an element from a JsonObject container.
*
* @param it An iterator pointing to the element to be erased.
* @return An iterator pointing to the element immediately following
*         a position prior to the element being erased.If no such
*         element exists, end() is returned.
**/
JsonObject::Iterator JsonObject::erase(Iterator it)
{
  return _data.erase(it);
}

/**
* @brief Erases an element from a JsonObject container.
*
* @param first An iterator pointing to the start of the range to be erased.
* @param last An iterator pointing to the end of the range to be erased.
* @return An iterator pointing to the element immediately following
*         a position prior to the element being erased.If no such
*         element exists, end() is returned.
*/
JsonObject::Iterator JsonObject::erase(Iterator first, Iterator last)
{
  return _data.erase(first, last);
}

/**
*  @brief Attempts to insert a Json value into the container.
*  @param  key  The key for which json data should be stored.
*  @param  value  JsonValue to be inserted.
**/
void JsonObject::insert(const KeyType &key, const JsonValue& value)
{
  _data[key] = value;
}

/**
* @brief Access to JsonObject container data.
*
* @param key The key for which data should be retrieved.
* @return  A reference to the data whose key is equivalent to 'key', if
*          such a data is present in the Json container. If no such data is present
*          std::out_of_range is thrown.
*/
JsonValue &JsonObject::get(const KeyType &key)
{
  if (_data.count(key) == 0)
    throw std::out_of_range(base::strfmt("no element '%s' found in caontainer", key.c_str()));
  return _data[key];
}

/**
* @brief Default constructor.
*
**/
JsonArray::JsonArray()
{
}

/**
*  @brief  Move constructor.
*  @param other A JsonArray of identical element and allocator types.
*/
JsonArray::JsonArray(JsonArray &&other)
  : _data(std::move(other._data))
{
}

/**
*  @brief Move assignment operator.
*  @param other A JsonArray of identical element and allocator types.
*
*  @return returns the reference to assigned value.
**/
JsonArray &JsonArray::operator=(JsonArray &&other)
{
  _data = std::move(other._data);
  return *this;
}

/**
*  @brief Copy constructor.
*  @param other a JsonObject of identical element and allocator types.
*/
JsonArray::JsonArray(const JsonArray &other) : _data(other._data)
{
}

/**
*  @brief Assignment operator.
*  @param other a JsonArray of identical element and allocator types.
*
*  @return returns the reference to assigned value.
**/
JsonArray& JsonArray::operator=(const JsonArray &other)
{
  _data = other._data;
  return *this;
}

/**
*  @brief  Access to the data.
*  @param pos The index of the element for which data should be
*         accessed.
*  @return Read/write reference to data stored in continer. 
*          If no such data is present std::out_of_range is thrown.
*/
JsonValue& JsonArray::at(SizeType pos)
{
  if (pos > _data.size())
    throw std::out_of_range(base::strfmt("Index '%d' is out of range.", pos));
  return _data.at(pos);
}

/**
*  @brief  Access to the data.
*  @param pos The index of the element for which data should be
*         accessed.
*  @return Read only reference to data stored in continer.
*          If no such data is present std::out_of_range is thrown.
**/
const JsonValue &JsonArray::at(SizeType pos) const
{
  if (pos > _data.size())
    throw std::out_of_range(base::strfmt("Index '%d' is out of range.", pos));
  return _data.at(pos);
}

/**
*  @brief  Access to the data.
*  @param pos The index of the element for which data should be accessed.
*  @return Read/Write reference to data stored in continer.
*
*  This operator allows for easy, array-style, data access.
*/
JsonValue &JsonArray::operator[](SizeType pos)
{
  assert(pos < _data.size());
  return _data[pos];
}

/**
*  @brief  Access to the data.
*  @param pos The index of the element for which data should be accessed.
*  @return Read only reference to data stored in continer.
*
*  This operator allows for easy, array-style, data access.
**/
const JsonValue &JsonArray::operator[](SizeType pos) const
{
  assert(pos < _data.size());
  return _data[pos];
}


/**
* @brief Returns a read/write iterator.
*
* Returns a read/write iterator that points to the first
* element in the JsonArray container.
*
* @return iterator for begining of sequence.
*/
JsonArray::Iterator JsonArray::begin()
{
  return _data.begin();
}

/**
* @brief Returns a rad only iterator.
*
* Returns a read only iterator that points to the first
* element in the JsonArray container.
*
* @return iterator for begining of sequence.
**/
JsonArray::ConstIterator JsonArray::begin() const
{
  return _data.begin();
}

/**
* @brief Returns a rad only iterator.
*
* Returns a read only iterator that points to the first
* element in the JsonArray container.
*
* @return iterator for begining of sequence.
*/
JsonArray::ConstIterator JsonArray::cbegin() const
{
  return _data.begin();
}

/**
* @brief Returns iterator for end of sequence.
*
* Returns a read/write iterator that points to one past the last pair in the JsonArray container.
*
* @return iterator for end of sequence.
**/
JsonArray::Iterator JsonArray::end()
{
  return _data.end();
}

/**
* @brief Returns iterator for end of sequence.
*
* Returns a read only iterator that points to one past the last pair in the JsonArray container.
*
* @return iterator for end of sequence.
*/
JsonArray::ConstIterator JsonArray::end() const
{
  return _data.end();
}

/**
* @brief Returns iterator for end of sequence.
*
* Returns a read only iterator that points to one past the last pair in the JsonArray container.
*
* @return iterator for end of sequence.
**/
JsonArray::ConstIterator JsonArray::cend() const
{
  return _data.end();
}

/**
* @brief Get size of JsonArray.
*
* @return return length of sequence.
**/
JsonArray::SizeType JsonArray::size()
{
  return _data.size();
}

/**
* @brief Test if container is empty.
*
* @return returns true if the JsonArray container is empty.
**/
bool JsonArray::empty() const
{
  return _data.empty();
}

/**
* @brief Erases all elements in a JsonArray container.
*
**/
void JsonArray::clear()
{
  _data.clear();
}

/**
* @brief Erases an element from a JsonArray container.
*
* @param it An iterator pointing to the element to be erased.
* @return An iterator pointing to the element immediately following
*         a position prior to the element being erased. If no such
*         element exists, end() is returned.
*/
JsonArray::Iterator JsonArray::erase(Iterator pos)
{
  return _data.erase(pos);

}

/**
* @brief Erases an element from a JsonArray container.
*
* @param first An iterator pointing to the start of the range to be erased.
* @param last An iterator pointing to the end of the range to be erased.
* @return An iterator pointing to the element immediately following
*         a position prior to the element being erased.If no such
*         element exists, end() is returned.
**/
JsonArray::Iterator JsonArray::erase(Iterator first, Iterator last)
{
  return _data.erase(first, last);
}

/**
*  @brief Attempts to insert value at pos.
*  @param  pos  A iterator to the JsonArray where insert value.
*  @param  value  JsonValue to be inserted.
*
*  @return An iterator that points to the inserted data.
*/
JsonArray::Iterator JsonArray::insert(Iterator pos, const JsonValue& value)
{
  return _data.insert(pos, value);
}

/**
*  @brief Add data to the end of the container.
   @param  value  JsonValue to be inserted.
*
**/
void JsonArray::pushBack(const ValueType& value)
{
  _data.push_back(value);
}

/**
* @brief default destructor.
*
**/
JsonValue::JsonValue()
  : _double(0), _bool(false), _type(VEmpty)
{
}

/**
*  @brief  Copy constructor.
*  @param other A JsonValue of identical element and allocator types.
**/
JsonValue::JsonValue(const JsonValue& rhs)
   : _double(rhs._double), _bool(rhs._bool), _string(rhs._string), _object(rhs._object),  
   _array(rhs._array), _type(rhs._type)
{
}


/**
*  @brief  Move constructor.
*  @param other A JsonValue of identical element and allocator types.
*/
JsonValue::JsonValue(JsonValue&& rhs)
   : _double(rhs._double), _bool(rhs._bool), _string(std::move(rhs._string)), _object(std::move(rhs._object)),
   _array(std::move(rhs._array)), _type(rhs._type)
{
}

/**
*  @brief Assignment operator.
*  @param other A JsonValue of identical element and allocator types.
*
*  @return returns the reference to assigned value.
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
*  @brief Move assignment operator.
*  @param other A JsonValue of identical element and allocator types.
*
*  @return returns the reference to assigned value.
*/
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
*  @brief  Construct JsonValue from string.
*  @param val A string value.
**/
JsonValue::JsonValue(const std::string& val)
   : _double(0), _bool(false), _string(val), _type(VString)
{
}

/**
*  @brief  Construct JsonValue from string.
*  @param val A string value.
**/
JsonValue::JsonValue(std::string&& val)
  : _double(0), _bool(false), _string(std::move(val)), _type(VString)
{
}

/**
*  @brief  Construct JsonValue from string.
*  @param val A string const value.
**/
JsonValue::JsonValue(const char* val)
  : _double(0), _bool(false), _type(VString)
{
  _string = std::string(val != nullptr ? val : "");
}

/**
*  @brief Construct JsonValue from bool.
*  @param val A bool value.
**/
JsonValue::JsonValue(bool val)
  : _double(0), _bool(val), _type(VBoolean)
{
}

/**
*  @brief Construct JsonValue from int.
*  @param val A int value.
**/
JsonValue::JsonValue(int val)
  : _double(val), _bool(false), _type(VInt)
{
}

/**
*  @brief Construct JsonValue from double.
*  @param val A double value.
**/
JsonValue::JsonValue(double val)
  : _double(val), _bool(false), _type(VDouble)
{
}

/**
*  @brief Construct JsonValue from JsonObject value.
*  @param val A JsonObject value.
**/
JsonValue::JsonValue(const JsonObject &val)
  : _double(0), _bool(false), _object(val), _type(VObject)
{
}

/**
*  @brief Construct JsonValue from JsonObject value.
*  @param val A JsonObject value.
**/
JsonValue::JsonValue(JsonObject&& val)
  : _double(0), _bool(false), _object(std::move(val)), _type(VObject)
{
}

/**
*  @brief Construct JsonValue from JsonArray value.
*  @param val A JsonArray value.
**/
JsonValue::JsonValue(const JsonArray &val)
  : _double(0), _bool(false), _array(val), _type(VObject)
{
}

/**
*  @brief Construct JsonValue from JsonArray value.
*  @param val A JsonArray value.
**/
JsonValue::JsonValue(JsonArray &&val)
  : _double(0), _bool(false), _array(std::move(val)), _type(VObject)
{
}

/**
* @brief Access to type of value.
*
* @return return type of value.
**/
DataType JsonValue::getType() const
{
  return _type;
}

/**
* @brief Access to the double value.
*
* @return return double value.
**/
double JsonValue::getDouble() const
{
  return _double;
}

/**
*  @brief Set number value.
*  @param val A double value to set.
**/
void JsonValue::setNumber(double val)
{
  _double = val;
}

/**
* @brief Access to the int value.
*
* @return return int value.
**/
int JsonValue::getInt() const
{
  return static_cast<int>(_double);
}

/**
* @brief Access to the bool value.
*
* @return return bool value.
**/
bool JsonValue::getBool() const
{
  return _bool;
}

/**
*  @brief Set bool value.
*  @param val A bool value to set.
**/
void JsonValue::setBool(bool val)
{
  _bool = val;
}

/**
* @brief Access to the string value.
*
* @return return const reference to string value.
**/
const std::string& JsonValue::getString() const
{
  return _string;
}

/**
*  @brief Set string value.
*  @param val A string reference to set.
**/
void JsonValue::setString(const std::string& val)
{
  _string = val;
}

/**
* @brief Access to the JsonObject value.
*
* @return return reference to the JsonObject value.
**/
JsonObject &JsonValue::getObject()
{
  return _object;
}

/**
* @brief Access to the JsonObject value.
*
* @return return const reference to the JsonObject value.
**/
const JsonObject& JsonValue::getObject() const
{
  return _object;
}

/**
*  @brief Set JsonObject value.
*  @param val A reference to JsonObject to set.
**/
void JsonValue::setObject(const JsonObject& val)
{
  _object = val;
}

/**
* @brief Access to the JsonArray value.
*
* @return return reference to the JsonObject value.
**/
JsonArray &JsonValue::getArray()
{
  return _array;
}

/**
*  @brief Set setArray value.
*  @param val A reference to setArray to set.
**/
void JsonValue::setArray(const JsonArray& val)
{
  _array = val;
}

/**
* @brief Access to the double value.
*
* @return return double value.
**/
const JsonArray& JsonValue::getArray() const
{
  return _array;
}

/**
*  @brief Set JsonValue type.
*  @param type A type to set.
**/
void JsonValue::setType(DataType type)
{
   _type = type;
}


/**
* -------------------------------------------------------------------------------------------------
*   JSON reader implementation
* -------------------------------------------------------------------------------------------------
*/

/**
* @brief Construtor
*        Construct JsonReader from string
*
* @param value string reference contaning Json data
**/
JsonReader::JsonReader(const std::string &value) 
  : _jsonText(value), _actualPos(0)
{
}

/**
* @brief Returns the next available character but does not consume it.
*
* @return An char representing the next character to be read, or 0 if there are no characters to be read.
**/
char JsonReader::peek()
{
  return (_actualPos < _jsonText.length()) ? _jsonText[_actualPos] : static_cast<char>(0);
}

/**
* @brief Check if the end of a string has been reached
*
* @param
* @return Returns a bool value true when the end of a string has been reached
**/
bool JsonReader::eos()
{
  return _actualPos == _jsonText.length();
}

/**
* @brief Indicates whether the specified  character is categorized as white space.
*
* @param c The character to evaluate. 
* @return true if c is white space; otherwise, false.
**/
bool JsonReader::isWhiteSpace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/**
* @brief skip white spaces 
*
**/
void JsonReader::eatWhitespace()
{
  while(isWhiteSpace(peek()))
    moveAhead();
}

/**
* @brief Consume actual character and move the next available.
*
**/
void JsonReader::moveAhead()
{
  _actualPos = std::min(_actualPos + 1, _jsonText.length());
}

/**
* @brief Try to parse Json data
*
* @param str String to parse.
* @param value Parsed Json value.
**/
void JsonReader::read(const std::string &str, JsonValue &value)
{
  JsonReader reader(str);
  reader.scan();
  reader.parse(value);
}


/**
* @brief Scan every character in Json data
*
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
* @brief Check if Json data contains given text starting with actual reader position
*
* @param text String to check
* @return true if text match to readed text from Json data member; otherwise, false.
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
* @brief Parse json string a
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
   if (modf(number, &intpart) == 0.0)
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
boost::signals2::signal<void()>* JsonBaseView::signalChanged()
{
  return &_signalChanged;
}

void JsonBaseView::generateObjectInTree(const JsonParser::JsonValue& value, TreeNodeRef node, bool addNew)
{
}

void JsonBaseView::generateArrayInTree(const JsonParser::JsonValue& value, TreeNodeRef node, bool addNew)
{
}

void JsonBaseView::generateIntInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
}


void JsonBaseView::generateBoolInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
}


void JsonBaseView::generateStringInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
}

void JsonBaseView::generateDoubleInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
}

void JsonBaseView::generateNullInTree(TreeNodeRef node)
{
}

void JsonBaseView::generateTree(const JsonParser::JsonValue &value, TreeNodeRef node, bool addNew)
{
  switch (value.getType())
  {
  case VInt:
    generateIntInTree(value, node);
    break;
  case VBoolean:
    generateBoolInTree(value, node);    break;
  case VString:
    generateStringInTree(value, node);
    break;
  case VDouble:
    generateDoubleInTree(value, node);
    break;
  case VObject:
    generateObjectInTree(value, node, addNew);
    break;
  case VArray:
    generateArrayInTree(value, node, addNew);
    break;
  case VEmpty:
    generateNullInTree(node);
    break;
  default: 
    break;
  }
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
* @brief Default constructor
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


/**
* @brief Add Json data to the control.
*
* @param value A JsonValue object that contains the json text data to set.
**/
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


void JsonTreeView::generateIntInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
  node->set_icon_path(0, "json_nmb.png");
  node->set_attributes(1, mforms::TextAttributes("#99cc66", false, true));
  node->set_int(1, value.getInt());
  node->expand();
}


void JsonTreeView::generateBoolInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
  node->set_icon_path(0, "json_nmb.png");
  node->set_attributes(1, mforms::TextAttributes("#0099ff", true, true));
  node->set_bool(1, value.getBool());
  node->expand();
}


void JsonTreeView::generateStringInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
  node->set_icon_path(0, "json_str.png");
  node->set_attributes(1, mforms::TextAttributes("#cc9966", false, false));
  node->set_string(1, value.getString());
  node->expand();
}

void JsonTreeView::generateDoubleInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
    node->set_icon_path(0, "json_nmb.png");
    node->set_attributes(1, mforms::TextAttributes("#99cc66", false, true));
    node->set_float(1, value.getDouble());
    node->expand();
}
  
void JsonTreeView::generateNullInTree(TreeNodeRef node)
{
    node->set_icon_path(0, "json_null.png");
    node->set_string(0, "<<null>>");
    node->set_string(1, "");
    node->expand();
}

/**
* @brief Constructor
*
**/
JsonGridView::JsonGridView()
  : _gridView(std::make_shared<mforms::TreeNodeView>(mforms::TreeAltRowColors | mforms::TreeShowRowLines | mforms::TreeShowColumnLines | mforms::TreeNoBorder))
{
  _gridView->add_column(IconStringColumnType, "Key", 150, false, true);
  _gridView->add_column(IconStringColumnType, "Value", 200, false, true);
  _gridView->add_column(IconStringColumnType, "Type", 200, false, true);
  _gridView->end_columns();
  init();
}



/**
* @brief Init tree view
*
* Based of readed json data control function initialize mforms control TreNodeView
**/
void JsonGridView::init()
{
  assert(_gridView.get() != nullptr);
  add(_gridView.get());
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


/**
* @brief Add the Json data to the control.
*
* @param value A JsonValue object that contains the json text data to set.
**/
void JsonGridView::setJson(const JsonParser::JsonValue& value)
{
  _gridView->clear();
  TreeNodeRef node = _gridView->root_node()->add_child();
  generateTree(value, node);
}


void JsonGridView::generateObjectInTree(const JsonParser::JsonValue& value, TreeNodeRef node, bool addNew)
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
    node2->set_string(2, "Object");
    generateTree(it->second, node2);
    node2->expand();
  }
}


void JsonGridView::generateArrayInTree(const JsonParser::JsonValue& value, TreeNodeRef node, bool addNew)
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
  node2->set_string(2, "Array");
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


void JsonGridView::generateIntInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
  node->set_icon_path(0, "json_nmb.png");
  node->set_attributes(1, mforms::TextAttributes("#99cc66", false, true));
  node->set_int(1, value.getInt());
  node->set_string(2, "Integer");
  node->expand();
}


void JsonGridView::generateBoolInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
  node->set_icon_path(0, "json_nmb.png");
  node->set_attributes(1, mforms::TextAttributes("#0099ff", true, true));
  node->set_bool(1, value.getBool());
  node->set_string(2, "Boolean");
  node->expand();
}


void JsonGridView::generateStringInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
  node->set_icon_path(0, "json_str.png");
  node->set_attributes(1, mforms::TextAttributes("#cc9966", false, false));
  node->set_string(1, value.getString());
  node->set_string(2, "String");
  node->expand();
}

void JsonGridView::generateDoubleInTree(const JsonParser::JsonValue& value, TreeNodeRef node)
{
  node->set_icon_path(0, "json_nmb.png");
  node->set_attributes(1, mforms::TextAttributes("#99cc66", false, true));
  node->set_float(1, value.getDouble());
  node->set_string(2, "Double");
  node->expand();
}

void JsonGridView::generateNullInTree(TreeNodeRef node)
{
  node->set_icon_path(0, "json_null.png");
  node->set_string(0, "<<null>>");
  node->set_string(1, "");
  node->set_string(2, "null");
  node->expand();
}


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
* @brief Add the Json data to the control.
*
* @param value A JsonValue object that contains the json text data to set.
**/
void JsonTabView::setJson(const JsonParser::JsonValue& value)
{
  _json = value;
  _ident = 0;
  JsonWriter::write(_jsonText, value);
  _textView->setText(_jsonText);
  _treeView->setJson(value);
  _gridView->setJson(value);
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
