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

#pragma once

#include "common.h"

#include <map>
#include <vector>

namespace JsonParser {

  enum DataType { VBoolean, VString, VDouble, VInt64, VUint64, VObject, VArray, VEmpty };

  class JsonValue;
  class BASELIBRARY_PUBLIC_FUNC JsonObject {
  public:
    typedef std::map<std::string, JsonValue> Container;
    typedef Container::size_type SizeType;
    typedef Container::key_type KeyType;
    typedef Container::iterator Iterator;
    typedef Container::const_iterator ConstIterator;
    typedef Container::value_type ValueType;

    JsonObject();
    // move operations
    JsonObject(JsonObject &&val);
    JsonObject(const JsonObject &other);
    JsonObject &operator=(JsonObject &&val);
    JsonObject &operator=(const JsonObject &val);

    JsonValue &operator[](const std::string &name);

    // return iterator for begining of sequence
    Iterator begin();
    ConstIterator begin() const;
    ConstIterator cbegin() const;

    // return iterator for end of sequence
    Iterator end();
    ConstIterator end() const;
    ConstIterator cend() const;

    // return length of sequence
    SizeType size() const;
    Iterator find(const KeyType &key);
    ConstIterator find(const KeyType &key) const;

    // test if container is empty
    bool empty() const;

    void clear();
    Iterator erase(Iterator pos);
    Iterator erase(Iterator first, Iterator last);

    void insert(const KeyType &key, const JsonValue &value);
    JsonValue &get(const KeyType &key);
    const JsonValue &get(const KeyType &key) const;

  private:
    Container _data;
  };

  class BASELIBRARY_PUBLIC_FUNC JsonArray {
  public:
    // based on std::vector implementation
    typedef std::vector<JsonValue> Container;
    typedef Container::size_type SizeType;
    typedef Container::iterator Iterator;
    typedef Container::const_iterator ConstIterator;
    typedef Container::value_type ValueType;

    // Default constructor
    JsonArray();
    JsonArray(const JsonArray &other);
    JsonArray &operator=(const JsonArray &other);
    // move operations
    JsonArray(JsonArray &&other);
    JsonArray &operator=(JsonArray &&other);

    // subscript sequence with checking
    JsonValue &at(SizeType pos);
    const JsonValue &at(SizeType pos) const;

    // subscript sequence
    JsonValue &operator[](SizeType pos);
    const JsonValue &operator[](SizeType pos) const;

    // return iterator for begining of sequence
    Iterator begin();
    ConstIterator begin() const;
    ConstIterator cbegin() const;

    // return iterator for end of sequence
    Iterator end();
    ConstIterator end() const;
    ConstIterator cend() const;

    // return length of sequence
    SizeType size() const;

    // test if container is empty
    bool empty() const;
    void clear();
    Iterator erase(Iterator pos);
    Iterator erase(Iterator first, Iterator last);

    // insert value at pos
    Iterator insert(Iterator pos, const JsonValue &value);
    // insert element at end
    void pushBack(const ValueType &value);

  private:
    Container _data;
  };

  class BASELIBRARY_PUBLIC_FUNC JsonValue {
  public:
    JsonValue();
    ~JsonValue(){};
    JsonValue(const JsonValue &rhs);
    JsonValue &operator=(const JsonValue &rhs);
    JsonValue &operator=(JsonValue &&rhs);
    JsonValue(JsonValue &&rhs);

    explicit JsonValue(const std::string &val);
    explicit JsonValue(std::string &&val);
    explicit JsonValue(const char *val);
    explicit JsonValue(bool val);

#ifdef DEFINE_INT_FUNCTIONS
    explicit JsonValue(int val);
    explicit JsonValue(unsigned int val);
#endif
#ifdef DEFINE_UINT64_T_FUNCTIONS
    explicit JsonValue(int64_t val);
    explicit JsonValue(uint64_t val);
#endif
#ifdef DEFINE_SSIZE_T_FUNCTIONS
    explicit JsonValue(ssize_t val);
    explicit JsonValue(size_t val);
#endif

    explicit JsonValue(double val);
    explicit JsonValue(const JsonObject &val);
    explicit JsonValue(JsonObject &&val);
    explicit JsonValue(const JsonArray &val);
    explicit JsonValue(JsonArray &&val);

    // Cast and assignment. Throw if cast is not possible.
    operator const JsonObject &() const;
    operator JsonObject &(); // non-const version so you can change inner values of the object.
    const JsonObject &operator=(const JsonObject &other);
    operator const JsonArray &() const;
    operator JsonArray &(); // dito non-const version
    const JsonArray &operator=(const JsonArray &other);

#ifdef DEFINE_INT_FUNCTIONS
    operator int() const;
    operator unsigned int() const;
    int operator=(int other);
    unsigned int operator=(unsigned int other);
#endif
#ifdef DEFINE_UINT64_T_FUNCTIONS
    operator int64_t() const;
    operator uint64_t() const;
    int64_t operator=(int64_t other);
    uint64_t operator=(uint64_t other);
#endif
#ifdef DEFINE_SSIZE_T_FUNCTIONS
    operator ssize_t() const;
    operator size_t() const;
    ssize_t operator=(ssize_t other);
    size_t operator=(size_t other);
#endif

    operator double() const;
    double operator=(double other);

    operator bool() const;
    bool operator=(bool other);

    operator const std::string &() const;
    const std::string &operator=(const std::string &other);

    DataType getType() const;
    void setDeleted(bool flag);
    bool isDeleted() const;

    void clear();
    bool isValid();

  private:
    double _double;
    int64_t _integer64;
    uint64_t _uinteger64;
    bool _bool;
    std::string _string;
    JsonObject _object;
    JsonArray _array;

    DataType _type;
    bool _deleted;
    bool _isValid;
  };

#ifndef HAS_NOEXCEPT
  #if defined(_MSC_VER) 
    #define NOEXCEPT noexcept
  #elif defined(__APPLE__)
    #define NOEXCEPT _NOEXCEPT
  #else
    #ifndef _GLIBCXX_USE_NOEXCEPT
      #define NOEXCEPT throw()
    #else
      #define NOEXCEPT _GLIBCXX_USE_NOEXCEPT
    #endif
  #endif
  #define HAS_NOEXCEPT
#endif

#if defined(_MSC_VER)
// C4275 can be ignored in Visual C++ if you are deriving from a type in the Standard C++ Library
#pragma warning(push)
#pragma warning(disable : 4275)
#endif
  class BASELIBRARY_PUBLIC_FUNC ParserException : public std::exception {
  public:
    explicit ParserException(const std::string &message) : _msgText(message) {
    }
    explicit ParserException(const char *message) : _msgText(message) {
    }
    virtual ~ParserException() NOEXCEPT {
    }
    virtual const char *what() const NOEXCEPT {
      return _msgText.c_str();
    }

  private:
    std::string _msgText;
  };
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

  class BASELIBRARY_PUBLIC_FUNC JsonReader {
    struct JsonToken {
      enum JsonTokenType {
        JsonTokenString,
        JsonTokenNumber,
        JsonTokenBoolean,
        JsonTokenEmpty,
        JsonTokenObjectStart,
        JsonTokenObjectEnd,
        JsonTokenArrayStart,
        JsonTokenArrayEnd,
        JsonTokenNext,
        JsonTokenAssign,
      };

      JsonToken(JsonTokenType type, const std::string &value) : _type(type), _value(value) {
      }
      JsonTokenType getType() const {
        return _type;
      }
      const std::string &getValue() const {
        return _value;
      }

    private:
      JsonTokenType _type;
      std::string _value;
    };

  public:
    typedef std::vector<JsonToken> Tokens;
    typedef Tokens::const_iterator TokensConstIterator;
    static void read(const std::string &text, JsonValue &value);
    static void readFromFile(const std::string &path, JsonValue &value);
    explicit JsonReader(const std::string &text);

  private:
    char peek();
    bool eos();
    void eatWhitespace();
    void moveAhead();
    static bool isWhiteSpace(char c);
    std::string getJsonNumber();
    std::string getJsonString();
    bool match(const std::string &text);
    bool processToken(JsonToken::JsonTokenType type, bool skip = false, bool mustMatch = true);
    void checkJsonEmpty(const std::string &text = "null");
    std::string getJsonBoolean();
    void scan();
    void parse(JsonObject &obj);
    void parseNumber(JsonValue &value);
    void parseBoolean(JsonValue &value);
    void parseString(JsonValue &value);
    void parseEmpty(JsonValue &value);
    void parseObject(JsonValue &value);
    void parseArray(JsonValue &value);
    void parse(JsonValue &value);

    // members
    std::string _jsonText;
    std::string::size_type _actualPos;
    Tokens _tokens;
    TokensConstIterator _tokenIterator;
    TokensConstIterator _tokenEnd;
  };

  class BASELIBRARY_PUBLIC_FUNC JsonWriter {
  public:
    explicit JsonWriter(const JsonValue &value);
    static void write(std::string &text, const JsonValue &value);
    static void writeToFile(const std::string &path, const JsonValue &value);

  private:
    void toString(std::string &output);
    void generate(std::string &output);
    void write(const JsonValue &value);
    void write(const JsonObject &value);
    void write(const JsonArray &value);
    void write(const std::string &value);

    const JsonValue &_jsonValue;
    int _depth;
    std::string _output;
  };
};
