/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/jsonparser.h"
#include "base/string_utilities.h"
#include <set>
#include <assert.h>
#include <typeinfo>
#include <algorithm>
#include <sstream>
#include <fstream>

namespace JsonParser {
  JsonObject::JsonObject() {
  }

  /**
   * @brief  Move constructor.
   * @param other A JsonObject of identical element and allocator types.
   */
  JsonObject::JsonObject(JsonObject &&other) : _data(std::move(other._data)) {
  }

  /**
   * @brief Move assignment operator.
   * @param other A JsonObject of identical element and allocator types.
   *
   * @return returns the reference to assigned value.
   */
  JsonObject &JsonObject::operator=(JsonObject &&other) {
    _data = std::move(other._data);
    return *this;
  }

  /**
  * @brief Copy constructor.
  * @param other a JsonObject of identical element and allocator types.
  */
  JsonObject::JsonObject(const JsonObject &other) : _data(other._data) {
  }

  /**
   * @brief Assignment operator.
   * @param other a JsonObject of identical element and allocator types.
   *
   * @return returns the reference to assigned value.
   */
  JsonObject &JsonObject::operator=(const JsonObject &other) {
    // if (*this != other)
    _data = other._data;
    return *this;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns a read/write iterator.
   *
   * Returns a read/write iterator that points to the first
   * element in the JsonObject container. Iteration is done in accessing order according to the key.
   *
   * @return iterator for begining of sequence.
   */
  JsonObject::Iterator JsonObject::begin() {
    return _data.begin();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns a readonly iterator.
   *
   * Returns a readonly iterator that points to the first
   * element in the JsonObject container. Iteration is done in accessing order according to the key.
   *
   * @return iterator for begining of sequence.
   */
  JsonObject::ConstIterator JsonObject::begin() const {
    return _data.begin();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns a readonly iterator.
   *
   * Returns a readonly iterator that points to the first
   * element in the JsonObject container. Iteration is done in accessing order according to the key.
   *
   * @return iterator for begining of sequence.
   */
  JsonObject::ConstIterator JsonObject::cbegin() const {
    return _data.begin();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns iterator for end of sequence.
   *
   * Returns a read/write iterator that points to one past the last pair in the JsonObject container.
   *
   * @return iterator for end of sequence.
   */
  JsonObject::Iterator JsonObject::end() {
    return _data.end();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns a readonly iterator.
   *
   * Returns a readonly (const) iterator that points to one past the last pair in the JsonObject container.
   *
   * @return iterator for end of sequence.
   */
  JsonObject::ConstIterator JsonObject::end() const {
    return _data.end();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns a readonly iterator.
   *
   * Returns a readonly (const) iterator that points to one past the last pair in the JsonObject container.
   *
   * @return iterator for end of sequence.
   */
  JsonObject::ConstIterator JsonObject::cend() const {
    return _data.end();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns the size of the JsonObject container.
   *
   * @return the size of the JsonObject container.
   */
  JsonObject::SizeType JsonObject::size() const {
    return _data.size();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Tries to find an element in a JsonObject container.
   *
   * @param key Kay to be located in the JsonObject container.
   * @return Iterator pointing to element, or end() if not found.
   */
  JsonObject::Iterator JsonObject::find(const KeyType &key) {
    return _data.find(key);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Tries to find an element in a JsonObject container.
   *
   * @param key Kay to be located in the JsonObject container.
   * @return Read only iterator pointing to element, or end() if not found.
   */
  JsonObject::ConstIterator JsonObject::find(const KeyType &key) const {
    return _data.find(key);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Test if container is empty.
   *
   * @return returns true if the JsonObject container is empty.
   */
  bool JsonObject::empty() const {
    return _data.empty();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Erases all elements in a JsonObject container.
   *
   */
  void JsonObject::clear() {
    _data.clear();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Erases an element from a JsonObject container.
   *
   * @param it An iterator pointing to the element to be erased.
   * @return An iterator pointing to the element immediately following
   *         a position prior to the element being erased.If no such
   *         element exists, end() is returned.
   */
  JsonObject::Iterator JsonObject::erase(Iterator it) {
    return _data.erase(it);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Erases an element from a JsonObject container.
   *
   * @param first An iterator pointing to the start of the range to be erased.
   * @param last An iterator pointing to the end of the range to be erased.
   * @return An iterator pointing to the element immediately following
   *         a position prior to the element being erased.If no such
   *         element exists, end() is returned.
   */
  JsonObject::Iterator JsonObject::erase(Iterator first, Iterator last) {
    return _data.erase(first, last);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Attempts to insert a JSON value into the container.
   *
   * @param  key  The key for which json data should be stored.
   * @param  value  JsonValue to be inserted.
   */
  void JsonObject::insert(const KeyType &key, const JsonValue &value) {
    _data[key] = value;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Access to the data..
   *
   * @param name The key for which JSON data should be accesed.
   * @return A reference to the data whose key is equivalent to 'key', if
   *         such a data is present in the JSON container. If no such data is present
   *         new key is created
   */
  JsonValue &JsonObject::operator[](const std::string &name) {
    return _data[name];
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Access to JsonObject container data.
   *
   * @param key The key for which data should be retrieved.
   * @return A reference to the data whose key is equivalent to 'key', if
   *         such a data is present in the JSON container. If no such data is present
   *         std::out_of_range is thrown.
   */
  JsonValue &JsonObject::get(const KeyType &key) {
    if (_data.count(key) == 0)
      throw std::out_of_range(base::strfmt("no element '%s' found in container", key.c_str()));
    return _data[key];
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Access to JsonObject container data.
   *
   * @param key The key for which data should be retrieved.
   * @return  A const reference to the data whose key is equivalent to 'key', if
   *          such a data is present in the JSON container. If no such data is present
   *          std::out_of_range is thrown.
   */
  const JsonValue &JsonObject::get(const KeyType &key) const {
    if (_data.count(key) == 0)
      throw std::out_of_range(base::strfmt("no element '%s' found in container", key.c_str()));
    return _data.at(key);
  }

  //--------------------------------------------------------------------------------------------------

  JsonArray::JsonArray() {
  }

  /**
   * @brief  Move constructor.
   * @param other A JsonArray of identical element and allocator types.
   */
  JsonArray::JsonArray(JsonArray &&other) : _data(std::move(other._data)) {
  }

  /**
   * @brief Move assignment operator.
   * @param other A JsonArray of identical element and allocator types.
   *
   * @return returns the reference to assigned value.
   */
  JsonArray &JsonArray::operator=(JsonArray &&other) {
    _data = std::move(other._data);
    return *this;
  }

  /**
   * @brief Copy constructor.
   * @param other a JsonObject of identical element and allocator types.
   */
  JsonArray::JsonArray(const JsonArray &other) : _data(other._data) {
  }

  /**
   * @brief Assignment operator.
   * @param other a JsonArray of identical element and allocator types.
   *
   * @return returns the reference to assigned value.
   */
  JsonArray &JsonArray::operator=(const JsonArray &other) {
    _data = other._data;
    return *this;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief  Access to the data.
   * @param pos The index of the element for which data should be
   *            accessed.
   * @return Read/write reference to data stored in continer.
   *         If no such data is present std::out_of_range is thrown.
   */
  JsonValue &JsonArray::at(SizeType pos) {
    if (pos > _data.size())
      throw std::out_of_range(base::strfmt("Index '%lu' is out of range.", pos));
    return _data.at(pos);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Access to the data.
   * @param pos The index of the element for which data should be
   *        accessed.
   * @return Read only reference to data stored in continer.
   *         If no such data is present std::out_of_range is thrown.
   */
  const JsonValue &JsonArray::at(SizeType pos) const {
    if (pos > _data.size())
      throw std::out_of_range(base::strfmt("Index '%lu' is out of range.", pos));
    return _data.at(pos);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief  Access to the data.
   * @param pos The index of the element for which data should be accessed.
   * @return Read/Write reference to data stored in continer.
   *
   *  This operator allows for easy, array-style, data access.
   */
  JsonValue &JsonArray::operator[](SizeType pos) {
    assert(pos < _data.size());
    return _data[pos];
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief  Access to the data.
   * @param pos The index of the element for which data should be accessed.
   * @return Read only reference to data stored in continer.
   *
   *  This operator allows for easy, array-style, data access.
   */
  const JsonValue &JsonArray::operator[](SizeType pos) const {
    assert(pos < _data.size());
    return _data[pos];
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns a read/write iterator.
   *
   * Returns a read/write iterator that points to the first
   * element in the JsonArray container.
   *
   * @return iterator for begining of sequence.
   */
  JsonArray::Iterator JsonArray::begin() {
    return _data.begin();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns a rad only iterator.
   *
   * Returns a read only iterator that points to the first
   * element in the JsonArray container.
   *
   * @return iterator for begining of sequence.
   */
  JsonArray::ConstIterator JsonArray::begin() const {
    return _data.begin();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns a rad only iterator.
   *
   * Returns a read only iterator that points to the first
   * element in the JsonArray container.
   *
   * @return iterator for begining of sequence.
   */
  JsonArray::ConstIterator JsonArray::cbegin() const {
    return _data.begin();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns iterator for end of sequence.
   *
   * Returns a read/write iterator that points to one past the last pair in the JsonArray container.
   *
   * @return iterator for end of sequence.
   */
  JsonArray::Iterator JsonArray::end() {
    return _data.end();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns iterator for end of sequence.
   *
   * Returns a read only iterator that points to one past the last pair in the JsonArray container.
   *
   * @return iterator for end of sequence.
   */
  JsonArray::ConstIterator JsonArray::end() const {
    return _data.end();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns iterator for end of sequence.
   *
   * Returns a read only iterator that points to one past the last pair in the JsonArray container.
   *
   * @return iterator for end of sequence.
   */
  JsonArray::ConstIterator JsonArray::cend() const {
    return _data.end();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Get size of JsonArray.
   *
   * @return return length of sequence.
   */
  JsonArray::SizeType JsonArray::size() const {
    return _data.size();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Test if container is empty.
   *
   * @return returns true if the JsonArray container is empty.
   */
  bool JsonArray::empty() const {
    return _data.empty();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Erases all elements in a JsonArray container.
   *
   */
  void JsonArray::clear() {
    _data.clear();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Erases an element from a JsonArray container.
   *
   * @param it An iterator pointing to the element to be erased.
   * @return An iterator pointing to the element immediately following
   *         a position prior to the element being erased. If no such
   *         element exists, end() is returned.
   */
  JsonArray::Iterator JsonArray::erase(Iterator pos) {
    return _data.erase(pos);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Erases an element from a JsonArray container.
   *
   * @param first An iterator pointing to the start of the range to be erased.
   * @param last An iterator pointing to the end of the range to be erased.
   * @return An iterator pointing to the element immediately following
   *         a position prior to the element being erased.If no such
   *         element exists, end() is returned.
   */
  JsonArray::Iterator JsonArray::erase(Iterator first, Iterator last) {
    return _data.erase(first, last);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Attempts to insert value at pos.
   * @param pos A iterator to the JsonArray where insert value.
   * @param value JsonValue to be inserted.
   *
   * @return An iterator that points to the inserted data.
   */
  JsonArray::Iterator JsonArray::insert(Iterator pos, const JsonValue &value) {
    return _data.insert(pos, value);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Add data to the end of the container.
   * @param value JsonValue to be inserted.
   *
   */
  void JsonArray::pushBack(const ValueType &value) {
    _data.push_back(value);
  }

  //----------------- JsonValue ----------------------------------------------------------------------

  JsonValue::JsonValue()
    : _double(0), _integer64(0), _uinteger64(0), _bool(false), _type(VEmpty), _deleted(false), _isValid(false) {
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief  Copy constructor.
   * @param other A JsonValue of identical element and allocator types.
   */
  JsonValue::JsonValue(const JsonValue &rhs)
    : _double(rhs._double),
      _integer64(rhs._integer64),
      _uinteger64(rhs._uinteger64),
      _bool(rhs._bool),
      _string(rhs._string),
      _object(rhs._object),
      _array(rhs._array),
      _type(rhs._type),
      _deleted(rhs._deleted),
      _isValid(rhs._isValid) {
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief  Move constructor.
   * @param other A JsonValue of identical element and allocator types.
   */
  JsonValue::JsonValue(JsonValue &&rhs)
    : _double(rhs._double),
      _integer64(rhs._integer64),
      _uinteger64(rhs._uinteger64),
      _bool(rhs._bool),
      _string(std::move(rhs._string)),
      _object(std::move(rhs._object)),
      _array(std::move(rhs._array)),
      _type(rhs._type),
      _deleted(rhs._deleted),
      _isValid(rhs._isValid) {
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Assignment operator.
   * @param other A JsonValue of identical element and allocator types.
   *
   * @return returns the reference to assigned value.
   */
  JsonValue &JsonValue::operator=(const JsonValue &rhs) {
    _double = rhs._double;
    _integer64 = rhs._integer64;
    _uinteger64 = rhs._uinteger64;
    _bool = rhs._bool;
    _string = rhs._string;
    _object = rhs._object;
    _array = rhs._array;
    _type = rhs._type;
    _deleted = rhs._deleted;
    _isValid = rhs._isValid;

    return *this;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Move assignment operator.
   * @param other A JsonValue of identical element and allocator types.
   *
   * @return returns the reference to assigned value.
   */
  JsonValue &JsonValue::operator=(JsonValue &&rhs) {
    _double = rhs._double;
    _integer64 = rhs._integer64;
    _uinteger64 = rhs._uinteger64;
    _bool = rhs._bool;
    _string = std::move(rhs._string);
    _object = std::move(rhs._object);
    _array = std::move(rhs._array);
    _type = rhs._type;
    _deleted = rhs._deleted;
    _isValid = rhs._isValid;

    return *this;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(const std::string &val) : JsonValue() {
    _isValid = true;
    _string = val;
    _type = VString;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(std::string &&val) : JsonValue() {
    _isValid = true;
    _string = std::move(val);
    _type = VString;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(const char *val) : JsonValue() {
    _isValid = true;
    _string = std::string(val != nullptr ? val : "");
    _type = VString;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(bool val) : JsonValue() {
    _isValid = true;
    _bool = val;
    _type = VBoolean;
  }

//--------------------------------------------------------------------------------------------------

#ifdef DEFINE_INT_FUNCTIONS

  JsonValue::JsonValue(int val) : JsonValue() {
    _isValid = true;
    _integer64 = val;
    _type = VInt64;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(unsigned int val) : JsonValue() {
    _isValid = true;
    _uinteger64 = val;
    _type = VUint64;
  }

#endif

//--------------------------------------------------------------------------------------------------

#ifdef DEFINE_UINT64_T_FUNCTIONS

  JsonValue::JsonValue(int64_t val) : JsonValue() {
    _isValid = true;
    _integer64 = val;
    _type = VInt64;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(uint64_t val) : JsonValue() {
    _isValid = true;
    _uinteger64 = val;
    _type = VUint64;
  }

#endif

//--------------------------------------------------------------------------------------------------

#ifdef DEFINE_SSIZE_T_FUNCTIONS

  JsonValue::JsonValue(ssize_t val) : JsonValue() {
    _isValid = true;
    _integer64 = val;
    _type = VInt64;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(size_t val) : JsonValue() {
    _isValid = true;
    _uinteger64 = val;
    _type = VUint64;
  }

#endif

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(double val) : JsonValue() {
    _isValid = true;
    _double = val;
    _type = VDouble;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(const JsonObject &val) : JsonValue() {
    _isValid = true;
    _object = val;
    _type = VObject;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(JsonObject &&val) : JsonValue() {
    _isValid = true;
    _object = std::move(val);
    _type = VObject;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(const JsonArray &val) : JsonValue() {
    _isValid = true;
    _array = val;
    _type = VArray;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::JsonValue(JsonArray &&val) : JsonValue() {
    _isValid = true;
    _array = std::move(val);
    _type = VArray;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Implicit cast to jsonObject element type. Throw if its not possible.
   *
   */
  JsonValue::operator const JsonObject &() const {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VObject)
      throw std::bad_cast();
    return _object;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::operator JsonObject &() {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VObject)
      throw std::bad_cast();
    return _object;
  }

  //--------------------------------------------------------------------------------------------------

  const JsonObject &JsonValue::operator=(const JsonObject &other) {
    _isValid = true;
    _type = VObject;
    _object = other;
    return other;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::operator const JsonArray &() const {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VArray)
      throw std::bad_cast();
    return _array;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::operator JsonArray &() {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VArray)
      throw std::bad_cast();
    return _array;
  }

  //--------------------------------------------------------------------------------------------------

  const JsonArray &JsonValue::operator=(const JsonArray &other) {
    _isValid = true;
    _type = VArray;
    _array = other;
    return other;
  }

//--------------------------------------------------------------------------------------------------

#ifdef DEFINE_INT_FUNCTIONS

  JsonValue::operator int() const {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VInt64)
      throw std::bad_cast();
    return (int)_integer64;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::operator unsigned int() const {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VInt64)
      throw std::bad_cast();
    return (unsigned int)_uinteger64;
  }

  //--------------------------------------------------------------------------------------------------

  int JsonValue::operator=(int other) {
    _isValid = true;
    _type = VInt64;
    _integer64 = other;
    return other;
  }

  //--------------------------------------------------------------------------------------------------

  unsigned int JsonValue::operator=(unsigned int other) {
    _isValid = true;
    _type = VUint64;
    _uinteger64 = other;
    return other;
  }

#endif

//--------------------------------------------------------------------------------------------------

#ifdef DEFINE_UINT64_T_FUNCTIONS

  JsonValue::operator int64_t() const {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VInt64)
      throw std::bad_cast();
    return _integer64;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::operator uint64_t() const {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VUint64)
      throw std::bad_cast();
    return _uinteger64;
  }

  //--------------------------------------------------------------------------------------------------

  int64_t JsonValue::operator=(int64_t other) {
    _isValid = true;
    _type = VInt64;
    _integer64 = other;
    return other;
  }

  //--------------------------------------------------------------------------------------------------

  uint64_t JsonValue::operator=(uint64_t other) {
    _isValid = true;
    _type = VUint64;
    _uinteger64 = other;
    return other;
  }

#endif

//--------------------------------------------------------------------------------------------------

#ifdef DEFINE_SSIZE_T_FUNCTIONS

  JsonValue::operator ssize_t() const {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VInt64)
      throw std::bad_cast();
    return (ssize_t)_integer64;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::operator size_t() const {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VUint64)
      throw std::bad_cast();
    return (size_t)_uinteger64;
  }

  //--------------------------------------------------------------------------------------------------

  ssize_t JsonValue::operator=(ssize_t other) {
    _isValid = true;
    _type = VInt64;
    _integer64 = other;
    return other;
  }

  //--------------------------------------------------------------------------------------------------

  size_t JsonValue::operator=(size_t other) {
    _isValid = true;
    _type = VUint64;
    _uinteger64 = other;
    return other;
  }

#endif

  //--------------------------------------------------------------------------------------------------

  JsonValue::operator double() const {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VDouble)
      throw std::bad_cast();
    return _double;
  }

  //--------------------------------------------------------------------------------------------------

  double JsonValue::operator=(double other) {
    _isValid = true;
    _type = VDouble;
    _double = other;
    return other;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::operator bool() const {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VBoolean)
      throw std::bad_cast();
    return _bool;
  }

  //--------------------------------------------------------------------------------------------------

  bool JsonValue::operator=(bool other) {
    _isValid = true;
    _type = VBoolean;
    _bool = other;
    return other;
  }

  //--------------------------------------------------------------------------------------------------

  JsonValue::operator const std::string &() const {
    if (!_isValid)
      throw std::runtime_error("Accessing uninitialized JSON value");

    if (_type != VString)
      throw std::bad_cast();
    return _string;
  }

  //--------------------------------------------------------------------------------------------------

  const std::string &JsonValue::operator=(const std::string &other) {
    _isValid = true;
    _type = VString;
    _string = other;
    return other;
  }

  //--------------------------------------------------------------------------------------------------

  DataType JsonValue::getType() const {
    return _type;
  }

  //--------------------------------------------------------------------------------------------------

  void JsonValue::setDeleted(bool flag) {
    _deleted = flag;
  }

  //--------------------------------------------------------------------------------------------------

  bool JsonValue::isDeleted() const {
    return _deleted;
  }

  //--------------------------------------------------------------------------------------------------

  void JsonValue::clear() {
    _isValid = false;
    _type = VEmpty;
    _double = 0;
    _integer64 = 0;
    _uinteger64 = 0;
    _bool = false;
    _string = "";
    _object = JsonObject();
    _array = JsonArray();
  }

  //--------------------------------------------------------------------------------------------------

  bool JsonValue::isValid() {
    return _isValid;
  }

  //----------------- JsonReader ---------------------------------------------------------------------

  // JSON reader implementation

  /**
   * @brief Constructor
   *        Construct JsonReader from string
   *
   * @param Value string reference containing JSON data.
   */
  JsonReader::JsonReader(const std::string &value) : _jsonText(value), _actualPos(0) {
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Returns the next available character but does not consume it.
   *
   * @return An char representing the next character to be read, or 0 if there are no characters to be read.
   */
  char JsonReader::peek() {
    return (_actualPos < _jsonText.length()) ? _jsonText[_actualPos] : static_cast<char>(0);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Check if the end of a string has been reached
   *
   * @param
   * @return Returns a bool value true when the end of a string has been reached
   */
  bool JsonReader::eos() {
    return _actualPos == _jsonText.length();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Indicates whether the specified  character is categorized as white space.
   *
   * @param c The character to evaluate.
   * @return true if c is white space; otherwise, false.
   */
  bool JsonReader::isWhiteSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief skip white spaces.
   *
   */
  void JsonReader::eatWhitespace() {
    while (isWhiteSpace(peek()))
      moveAhead();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Consume actual character and move the next available.
   *
   */
  void JsonReader::moveAhead() {
    _actualPos = std::min(_actualPos + 1, _jsonText.length());
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Try to parse JSON data.
   *
   * @param text String to parse.
   * @param value Parsed JSON value.
   */
  void JsonReader::read(const std::string &text, JsonValue &value) {
    JsonReader reader(text);
    reader.scan();
    reader.parse(value);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Try to read JSON data from file.
   * @param path JSON data Filepath
   * @param value Parsed JSON value.
   * @return JsonValue
   */

  void JsonReader::readFromFile(const std::string &path, JsonValue &value) {
    std::string str = base::getTextFileContent(path);
    if (str.empty())
      return;
    read(str, value);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Scan every character in JSON data.
   *
   */
  void JsonReader::scan() {
    while (!eos()) {
      eatWhitespace();
      std::string value;
      auto type = JsonToken::JsonTokenEmpty;
      char chr = peek();
      switch (chr) {
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
          checkJsonEmpty();
          type = JsonToken::JsonTokenEmpty;
          break;
        case 'u':
          checkJsonEmpty("undefined"); // only valid in java script, it is not valid JSON value according to
                                       // www.json.org
          type = JsonToken::JsonTokenEmpty;
          break;

        case 0:
          moveAhead();
          break;
        default:
          throw ParserException(std::string("Unexpected start sequence: ") + chr); // @@FIXMEE
      }
      _tokens.push_back(JsonToken(type, value));
    }
    _tokenIterator = _tokens.cbegin();
    _tokenEnd = _tokens.cend();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Check if JSON data contains given text starting with actual reader position.
   *
   * @param text String to check.
   * @return true if text match to readed text from JSON data member; otherwise, false.
   */
  bool JsonReader::match(const std::string &text) {
    bool match = !text.empty();
    auto cend = text.cend();
    std::for_each(text.cbegin(), cend, [&match, this](char value) {
      if (eos() || value != peek()) {
        match = false;
        return;
      }
      moveAhead();
    });
    return match;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Parse JSON string.
   *
   * @return Parsed value.
   */
  std::string JsonReader::getJsonString() {
    moveAhead();
    std::string string;
    while (eos() == false && peek() != '"') {
      char currentChar = peek();
      moveAhead();
      if (currentChar == '\\' && !eos()) {
        currentChar = peek();
        moveAhead();
        switch (currentChar) {
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
      } else {
        string.push_back(currentChar);
      }
    }
    if (!match("\""))
      throw ParserException(std::string("Expected: \" "));
    return string;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Parse JSON null literal.
   *
   * @param text String to check.
   */
  void JsonReader::checkJsonEmpty(const std::string &text /* = "null" */) {
    std::string emptyString;
    for (size_t i = 0; i < text.size() && !eos(); ++i) {
      char ch = peek();
      if (std::isspace(ch))
        break;
      emptyString += ch;
      moveAhead();
    }
    if (emptyString.compare(text) != 0)
      throw ParserException(std::string("Unexpected token: ") + emptyString);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Get a string literal from JSON data buffer.
   *
   * @return returns the parsed bool as string.
   */
  std::string JsonReader::getJsonBoolean() {
    const int size = peek() == 'f' ? 5 : 4;
    std::string boolString;
    for (int i = 0; i < size && !eos(); ++i) {
      boolString += peek();
      moveAhead();
    }
    if (boolString == "true" && boolString == "false")
      throw ParserException(std::string("Unexpected token: ") + boolString);

    return boolString;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Get a string literal from JSON data buffer.
   *
   * @return returns the parsed number as string.
   */
  std::string JsonReader::getJsonNumber() {
    std::set<char> numericChars = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', 'e', 'E', '-', '+'};
    std::string number;
    while (eos() == false && numericChars.count(peek()) == 1) {
      number += peek();
      moveAhead();
    }
    return number;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Check the specified token in current buffer position
   *
   * @param token Token to check.
   * @param skip Skip this token.
   * @param mustMach True if specified thoken must macht, otherweise exception is thrown.
   */
  bool JsonReader::processToken(JsonToken::JsonTokenType type, bool skip /*= false*/, bool mustMatch /* = true*/) {
    bool ret = _tokenIterator != _tokenEnd && _tokenIterator->getType() == type;
    if (!ret && mustMatch) {
      std::string message;
      if (_tokenIterator != _tokenEnd)
        throw ParserException("Unexpected token: " + _tokenIterator->getValue());
      else
        throw ParserException("Incomplete JSON data");
    }
    if (skip && ret) {
      ++_tokenIterator;
      ret = _tokenIterator != _tokenEnd;
    }
    return ret;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Parse a JsonObject from Json data buffer.
   *
   * @param value JsonValue reference where to store parsed JsonObject.
   */
  void JsonReader::parse(JsonObject &obj) {
    bool go = processToken(JsonToken::JsonTokenObjectStart, true) &&
              _tokenIterator->getType() != JsonToken::JsonTokenObjectStart;
    while (go) {
      // the member name
      processToken(JsonToken::JsonTokenString, false, false);
      if (_tokenIterator->getType() == JsonToken::JsonTokenObjectEnd)
        break;
      std::string name = _tokenIterator->getValue();
      ++_tokenIterator;

      // assign separator
      processToken(JsonToken::JsonTokenAssign, true);

      JsonValue value;
      // json value
      parse(value);
      if (obj.find(name) != obj.end())
        throw ParserException(std::string("Duplicate member: ") + name);

      obj.insert(name, value);

      go = processToken(JsonToken::JsonTokenNext, true, false);
    }
    processToken(JsonToken::JsonTokenObjectEnd, true);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Parses a string, and returns a floating point number or integer.
   *
   * @param value JsonValue reference where to store parsed number.
   */
  void JsonReader::parseNumber(JsonValue &value) {
    std::stringstream buffer;
    buffer << _tokenIterator->getValue();
    double number = 0;
    buffer >> number;
    double intpart = 0;
    if (modf(number, &intpart) == 0.0)
      value = (ssize_t)number;
    else
      value = number;
    ++_tokenIterator;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Parses a string, and returns a bool.
   *
   * @param value JsonValue reference where to store parsed bool value.
   */
  void JsonReader::parseBoolean(JsonValue &value) {
    value = (_tokenIterator->getValue() == "true" ? true : false);
    ++_tokenIterator;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Parse a string literal from JSON data buffer.
   *
   * @param value JsonValue reference where to store parsed string.
   */
  void JsonReader::parseString(JsonValue &value) {
    value = _tokenIterator->getValue();
    ++_tokenIterator;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Parse a empty string literal from JSON data buffer.
   *
   * @param value JsonValue reference where to store parsed string.
   */
  void JsonReader::parseEmpty(JsonValue &value) {
    value.clear();
    ++_tokenIterator;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Parse a JsonObject from JSON data buffer.
   *
   * @param value JsonValue reference where to store parsed JsonObject.
   */
  void JsonReader::parseObject(JsonValue &value) {
    JsonObject object;
    parse(object);
    value = object;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Parse a JsonArray from JSON data buffer.
   *
   * @param value JsonValue reference where to store parsed JsonArray.
   */
  void JsonReader::parseArray(JsonValue &value) {
    JsonArray array_;
    bool go =
      processToken(JsonToken::JsonTokenArrayStart, true);
    if (processToken(JsonToken::JsonTokenArrayEnd, false, false))
      go = false;
    while (go) {
      JsonValue innerValue;
      parse(innerValue);
      array_.pushBack(innerValue);
      go = processToken(JsonToken::JsonTokenNext, true, false);
    }
    processToken(JsonToken::JsonTokenArrayEnd, true);
    value = array_;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief parse
   *
   * @param reference to JSON value to be parsed
   */
  void JsonReader::parse(JsonValue &value) {
    if (_tokenIterator == _tokenEnd)
      throw ParserException("Unexpected json data end.");
    auto type = _tokenIterator->getType();
    switch (type) {
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

  //--------------------------------------------------------------------------------------------------

  // JSON writer implementation

  /**
   * @brief Construtor
   *        Construct JsonWriter from JsonValue
   *
   * @param value JsonValue reference
   */
  JsonWriter::JsonWriter(const JsonValue &value) : _jsonValue(value), _depth(0) {
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Write JsonValue into string.
   *
   * @param text String reference to store JSON text data.
   * @param value JsonValue to be stored into text.
   */
  void JsonWriter::write(std::string &text, const JsonValue &value) {
    JsonWriter writer(value);
    writer.toString(text);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Write JsonValue into file.
   *
   * @param text Filepath to store JSON text data.
   * @param value JsonValue to be stored into file.
   */
  void JsonWriter::writeToFile(const std::string &path, const JsonValue &value) {
    std::string data;
    write(data, value);
    std::ofstream file(path, std::ios::trunc);
    file << data;
    file.close();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Generate text representation of JsonObject.
   *
   * @param output String reference to store json text data.
   */
  void JsonWriter::toString(std::string &output) {
    generate(output);
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Generate text representation of JsonObject.
   *
   * @param output String reference to store JSON text data.
   */
  void JsonWriter::generate(std::string &output) {
    write(_jsonValue);
    output = _output;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Write JsonValue into string.
   *
   * @param value JsonValue to store in JSON text data.
   */
  void JsonWriter::write(const JsonValue &value) {
    if (value.isDeleted())
      return;
    switch (value.getType()) {
      case VBoolean:
        _output += value ? "true" : "false";
        break;
      case VString:
        write((std::string)value);
        break;
      case VDouble:
        _output += std::to_string((double)value);
        break;
      case VInt64:
        _output += std::to_string((int64_t)value);
        break;
      case VUint64:
        _output += std::to_string((uint64_t)value);
        break;
      case VObject:
        write((JsonObject)value);
        break;
      case VArray:
        write((JsonArray)value);
        break;
      case VEmpty:
        _output += "null";
        break;
      default:
        break;
    }
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Write JsonObject value.
   *
   * @param value JsonObject to store in JSON text data.
   */
  void JsonWriter::write(const JsonObject &value) {
    _output += "{";
    ++_depth;
    auto end = value.cend();
    auto finalIter = end;
    if (!value.empty()) {
      _output += "\n";
      --finalIter;
    }
    for (auto it = value.cbegin(); it != end; ++it) {
      if (it->second.isDeleted())
        continue;
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

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Write JsonArray value.
   *
   * @param value JsonArray to store in JSON text data.
   */
  void JsonWriter::write(const JsonArray &value) {
    _output += "[";
    ++_depth;
    auto end = value.cend();
    auto finalIter = end;
    if (!value.empty()) {
      _output += "\n";
      --finalIter;
    }
    for (auto it = value.cbegin(); it != end; ++it) {
      if (it->isDeleted())
        continue;
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

  //--------------------------------------------------------------------------------------------------

  /**
   * @brief Write string value.
   *
   * @param value String to store in JSON text.
   */
  void JsonWriter::write(const std::string &value) {
    _output += '"';
    _output += base::escape_json_string(value);
    _output += '"';
  }

}; /* namespace JsonParser */
