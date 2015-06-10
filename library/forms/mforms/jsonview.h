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

#pragma once
#include "mforms/panel.h"

namespace JsonParser {
  
  enum DataType { VInt, VBoolean, VString, VDouble, VObject, VArray, VEmpty };
  class JsonValue;
  class MFORMS_EXPORT JsonObject
  {
  public:
    typedef std::map<std::string, JsonValue> Container;
    typedef Container::size_type SizeType;
    typedef Container::key_type KeyType;
    typedef Container::iterator Iterator;
    typedef Container::const_iterator ConstIterator;
    typedef Container::value_type ValueType;

    JsonObject();
    // move operations
    JsonObject(JsonObject&& val);
    JsonObject(const JsonObject& other) : _data(other._data) { }
    JsonObject &operator=(JsonObject &&val);
    JsonObject &operator=(const JsonObject &val) { return *this; }

    // return iterator for begining of sequence
    Iterator begin();
    ConstIterator begin() const;
    ConstIterator cbegin() const;

    // return iterator for end of sequence
    Iterator end();
    ConstIterator end() const;
    ConstIterator cend() const;

    // return length of sequence
    SizeType size();
    Iterator find(const KeyType& key);
    ConstIterator find(const KeyType& key) const;

    // test if container is empty
    bool empty() const;

    void clear();
    Iterator erase(Iterator pos);
    Iterator erase(Iterator first, Iterator last);

    void insert(const KeyType &key, const JsonValue& value);
    JsonValue &get(const KeyType &key);

  private:
    Container _data;
  };

  class MFORMS_EXPORT JsonArray
  {
  public:
    // based on std::vector implementation
    typedef std::vector<JsonValue> Container;
    typedef Container::size_type SizeType;
    typedef Container::iterator Iterator;
    typedef Container::const_iterator ConstIterator;
    typedef Container::value_type ValueType;

    // Default constructor
    JsonArray();
    JsonArray(const JsonArray &other) : _data(other._data) { }
    JsonArray& operator=(const JsonArray &other)
    {
      _data = other._data;
      return *this;
    }
    // move operations
    JsonArray(JsonArray &&other);
    JsonArray& operator=(JsonArray &&other);

    // subscript sequence with checking
    JsonValue& at(SizeType pos);
    const JsonValue& at(SizeType pos) const;

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
    SizeType size();

    // test if container is empty
    bool empty() const;
    void clear();
    Iterator erase(Iterator pos);
    Iterator erase(Iterator first, Iterator last);

    // insert value at pos
    Iterator insert(Iterator pos, const JsonValue& value);
    // insert count * value at pos
    //void insert(Iterator pos, SizeType count, const JsonValue& value);

    // insert element at end
    void pushBack(const ValueType& value);

  private:
    Container _data;
  };

  class MFORMS_EXPORT JsonValue
  {
    
  public:
    JsonValue();
    JsonValue(const JsonValue& rhs);
    JsonValue& operator=(const JsonValue& rhs);
    JsonValue& operator=(JsonValue&& rhs);
    JsonValue(JsonValue&& rhs);

    explicit JsonValue(const std::string& val);
    explicit JsonValue(std::string&& val);
    explicit JsonValue(const char* val);
    explicit JsonValue(bool val);
    explicit JsonValue(int val);
    explicit JsonValue(double val);
    explicit JsonValue(const JsonObject& val);
    explicit JsonValue(JsonObject&& val);
    explicit JsonValue(const JsonArray &val);
    explicit JsonValue(JsonArray &&val);

    // access function
    double getDouble() const;
    int getInt() const;
    void setNumber(double val);

    bool getBool() const;
    void setBool(bool val);

    const std::string& getString() const;
    void setString(const std::string& val);

    operator JsonObject () const;
    JsonObject& getObject();
    void setObject(const JsonObject& val);

    operator JsonArray () const;
    JsonArray& getArray();
    void setArray(const JsonArray& val);

    void setType(DataType type);
    DataType getType() const;

  private:
    double _double;
    bool _bool;
    std::string _string;
    JsonObject _object;
    JsonArray _array;
    DataType _type;
  };

  class MFORMS_EXPORT ParserException : public std::exception
  {
  public:
    ParserException(const std::string& message) : std::exception(message.c_str()) {}
  };

  class MFORMS_EXPORT JsonReader : public boost::noncopyable
  {
    struct JsonToken
    {
      enum JsonTokenType { JsonTokenString, JsonTokenNumber, JsonTokenBoolean, JsonTokenEmpty, JsonTokenObjectStart,
        JsonTokenObjectEnd, JsonTokenArrayStart, JsonTokenArrayEnd, JsonTokenNext, JsonTokenAssign, };
      JsonToken(JsonTokenType type, const std::string& value) : _type(type), _value(value) { }
      JsonTokenType getType() const { return _type;  }
      const std::string &getValue() const { return _value; }
    private:
      JsonTokenType _type;
      std::string _value;
    };
  public:
    typedef std::vector<JsonToken> Tokens;
    typedef Tokens::const_iterator TokensConstIterator;
    static void read(const std::string &str, JsonValue &value);
    explicit JsonReader(const std::string &str);

  private:
    char peek();
    bool eos();
    void eatWhitespace();
    void moveAhead();
    static bool isWhiteSpace(char c);
    std::string getJsonNumber();
    std::string getJsonString();
    bool match(const std::string &text);
    bool match(JsonToken::JsonTokenType type, bool skip = false, bool mustMach = true);
    void checkJsonNull();
    std::string getJsonBoolean();
    void scan();
    void parse(JsonObject &obj);
    void parseNumber(JsonValue& value);
    void parseBoolean(JsonValue& value);
    void parseString(JsonValue& value);
    void parseEmpty(JsonValue& value);
    void parseObject(JsonValue& value);
    void parseArray(JsonValue& value);
    void parse(JsonValue &value);

    // members
    std::string _jsonText;
    std::string::size_type _actualPos;
    Tokens _tokens;
    TokensConstIterator _tokenIterator;
    TokensConstIterator _tokenEnd;
  };

  class MFORMS_EXPORT JsonWriter : public boost::noncopyable
  {
    //not implemented yet
    static bool write(const std::string &/*str*/, JsonValue &/*value*/)
    {
      return true;
    }
  };
};

/**
* @brief A Json view tab control with tree diffrent view text, tree and grid.
*
**/
namespace mforms {
   /**
   * @brief Json view base class definition.
   **/
  class JsonBaseView : public Panel
  {
  public:
    JsonBaseView();
    virtual ~JsonBaseView();
    boost::signals2::signal<void()>* signalChanged();
  protected:
    boost::signals2::signal<void()> _signalChanged;
  };

  /**
  * @brief Json text view control class definition.
  **/
  class CodeEditor;
  class JsonTextView : public JsonBaseView
  {
  public:
    JsonTextView();
    virtual ~JsonTextView();
    void textChanged();

  private:
    void init();
    std::shared_ptr<CodeEditor> _textEditor;
  };

  /**
  * @brief Json tree view control class definition.
  **/
  class TreeNodeView;
  class JsonTreeView : public JsonBaseView
  {
  public:
    JsonTreeView();
    virtual ~JsonTreeView();

  private:
    std::shared_ptr<TreeNodeView> _treeView;
  };

  /**
  * @brief Json grid view control class definition.
  **/
  class JsonGridView : public JsonBaseView
  {
  public:
    JsonGridView();
    virtual ~JsonGridView();
  };

  /**
  * @brief Json tab view control class definition.
  **/
  class TabView;
  class MFORMS_EXPORT JsonTabView : public Panel
  {
  public:
    void Setup();
    JsonTabView();
    ~JsonTabView();

    void setJson(const JsonParser::JsonValue &val);
    //const JsonParser::JsonValue &getJson() const;
    void setText(const std::string &text);
   // const std::string &getText() const;

  private:
    void textViewTextChanged();
    std::shared_ptr<JsonTextView> _textView;
    std::shared_ptr<JsonTreeView> _treeView;
    std::shared_ptr<JsonGridView> _gridView;
    std::shared_ptr<TabView> _tabView;
    std::string _jsonText;
    JsonParser::JsonValue _json;
  };
};
