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
#include "mforms/treenodeview.h"

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
    JsonObject(const JsonObject &other);
    JsonObject &operator=(JsonObject &&val);
    JsonObject &operator=(const JsonObject &val);

    JsonValue& operator [](const std::string& name);

    //bool operator !=(const JsonObject &rhs);

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
    SizeType size();

    // test if container is empty
    bool empty() const;
    void clear();
    Iterator erase(Iterator pos);
    Iterator erase(Iterator first, Iterator last);

    // insert value at pos
    Iterator insert(Iterator pos, const JsonValue &value);
    // insert count * value at pos
    //void insert(Iterator pos, SizeType count, const JsonValue &value);

    // insert element at end
    void pushBack(const ValueType &value);

  private:
    Container _data;
  };

  class MFORMS_EXPORT JsonValue
  {
    
  public:
    JsonValue();
    JsonValue(const JsonValue &rhs);
    JsonValue &operator=(const JsonValue &rhs);
    JsonValue &operator=(JsonValue&& rhs);
    JsonValue(JsonValue&& rhs);

    explicit JsonValue(const std::string &val);
    explicit JsonValue(std::string&& val);
    explicit JsonValue(const char *val);
    explicit JsonValue(bool val);
    explicit JsonValue(int val);
    explicit JsonValue(double val);
    explicit JsonValue(const JsonObject &val);
    explicit JsonValue(JsonObject&& val);
    explicit JsonValue(const JsonArray &val);
    explicit JsonValue(JsonArray &&val);

    // implicit cast to actual element type. throw if its not possible
    operator const JsonObject& () const;
    operator const JsonArray& () const;
    operator const int () const;
    operator const double () const;
    operator const bool () const;
    operator const std::string& () const;

    // access function
    double getDouble() const;
    int getInt() const;
    void setNumber(double val);

    bool getBool() const;
    void setBool(bool val);
    const std::string &getString() const;
    void setString(const std::string &val);
    JsonObject &getObject();
    const JsonObject &getObject() const;
    void setObject(const JsonObject &val);
    JsonArray &getArray();
    const JsonArray &getArray() const;
    void setArray(const JsonArray &val);

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

#ifdef _WIN32
# pragma warning(disable: 4275) // non dll-interface class used as base dll-interface class.
#endif

  class MFORMS_EXPORT ParserException : public std::runtime_error
  {
  public:
    explicit ParserException(const std::string &message) : std::runtime_error(message.c_str()) {}
  };

  class MFORMS_EXPORT JsonReader : public boost::noncopyable
  {
    struct JsonToken
    {
      enum JsonTokenType { JsonTokenString, JsonTokenNumber, JsonTokenBoolean, JsonTokenEmpty, JsonTokenObjectStart,
        JsonTokenObjectEnd, JsonTokenArrayStart, JsonTokenArrayEnd, JsonTokenNext, JsonTokenAssign, };
      JsonToken(JsonTokenType type, const std::string &value) : _type(type), _value(value) { }
      JsonTokenType getType() const { return _type;  }
      const std::string &getValue() const { return _value; }
    private:
      JsonTokenType _type;
      std::string _value;
    };
  public:
    typedef std::vector<JsonToken> Tokens;
    typedef Tokens::const_iterator TokensConstIterator;
    static void read(const std::string &text, JsonValue &value);
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
    bool match(JsonToken::JsonTokenType type, bool skip = false, bool mustMach = true);
    void checkJsonNull();
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

  class MFORMS_EXPORT JsonWriter : public boost::noncopyable
  {
  public:
    explicit JsonWriter(const JsonValue &value);
    static void write(std::string &text, const JsonValue &value);

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
     struct JsonValueNodeData : public mforms::TreeNodeData
     {
        JsonValueNodeData(JsonParser::JsonValue &value) : _jsonValue(value) {}
        JsonParser::JsonValue &getData() { return _jsonValue; }
     private:
        JsonParser::JsonValue &_jsonValue;
     };
    enum JsonNodeIcons { JsonObjectIcon, JsonArrayIcon, JsonStringIcon, JsonNumericIcon, JsonNullIcon };
    JsonBaseView();
    virtual ~JsonBaseView();
    void setCellValue(mforms::TreeNodeRef node, int column, const std::string &value);
    boost::signals2::signal<void()>* signalChanged();

  protected:
    void generateTree(JsonParser::JsonValue &value, mforms::TreeNodeRef node, bool addNew = true);
    virtual void generateArrayInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateObjectInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateIntInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateBoolInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateStringInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateDoubleInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateNullInTree(TreeNodeRef node);

    boost::signals2::signal<void()> _signalChanged;
    static std::string getNodeIconPath(JsonNodeIcons icon);
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
    void setText(const std::string &jsonText);

  private:
    void init();
    std::shared_ptr<CodeEditor> _textEditor;
  };

  /**
  * @brief Json tree view control class definition.
  **/
  class JsonTreeView : public JsonBaseView
  {
  public:
    JsonTreeView();
    virtual ~JsonTreeView();
    void setJson(JsonParser::JsonValue &val);

  private:
    void init();
    virtual void generateArrayInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateObjectInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateIntInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateBoolInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateStringInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateDoubleInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateNullInTree(TreeNodeRef node);
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
    void setJson(JsonParser::JsonValue &val);

  private:
    void init();
    virtual void generateArrayInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateObjectInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateIntInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateBoolInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateStringInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateDoubleInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateNullInTree(TreeNodeRef node);
    std::shared_ptr<TreeNodeView> _gridView;
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
    void textViewTextChanged();

  private:
    std::shared_ptr<JsonTextView> _textView;
    std::shared_ptr<JsonTreeView> _treeView;
    std::shared_ptr<JsonGridView> _gridView;
    std::shared_ptr<TabView> _tabView;
    std::string _jsonText;
    JsonParser::JsonValue _json;
    int _ident;
  };
};
