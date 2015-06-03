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
  
  enum DataType
  {
    VInt,
    VString,
    VDouble,
    VOobject,
    VAarray,
    VEmpty
  };

  template <typename T> class JsonTrivialType;
  typedef JsonTrivialType<double> Number;
  typedef JsonTrivialType<bool> Boolean;
  typedef JsonTrivialType<std::string> String;

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
    JsonObject& operator=(JsonObject&& val);

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

    // move operations
    JsonArray(JsonArray &&other);
    JsonArray& operator=(JsonArray &&other);

    // subscript sequence with checking
    JsonArray& at(SizeType pos);
    const JsonArray& at(SizeType pos) const;

    // subscript sequence
    JsonArray &operator[](SizeType pos);
    const JsonArray &operator[](SizeType pos) const;


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

    // test for equality
    bool operator==(const JsonArray &rhs) const;
    // test for inequality
    bool operator!=(const JsonArray &rhs) const;
    // test if this < rhs
    bool operator <(const JsonArray &rhs) const;
    // test if this <= rhs
    bool operator <=(const JsonArray &rhs) const;
    // test if this > rhs
    bool operator >(const JsonArray &rhs) const;
    // test if this >= rhs
    bool operator >=(const JsonArray &rhs) const;

    // insert value at pos
    Iterator insert(Iterator pos, const JsonValue& value);

    // insert count * value at pos
    void insert(Iterator pos, SizeType count, const JsonValue& value);

    // insert element at end
    void pushBack(const ValueType& value);

  private:
    Container data_;
  };

  template <typename T>
  class JsonTrivialType
  {
  public:
    JsonTrivialType(const T& t = JsonTrivialType());

    operator JsonTrivialType&();
    operator const JsonTrivialType&() const;

    T& Value();
    const T& Value() const;

    bool operator == (const JsonTrivialType<T>& trivial) const;

  private:
    T _value;
  };

  class MFORMS_EXPORT JsonValue
  {
    // JSON value
    
    public:
      // Default constructor
      JsonValue();
      // Copy operations
      JsonValue(const JsonValue& rhs);
      JsonValue& operator=(const JsonValue& rhs);
      // move operations
      JsonValue& operator=(JsonValue&& rhs);
      JsonValue(JsonValue&& rhs);

      // construct from std::string
      JsonValue::JsonValue(const std::string& val);
      JsonValue::JsonValue(std::string&& val);
      // construct from c string pointer
      JsonValue::JsonValue(const char* val);
      // construct from bool
      JsonValue::JsonValue(bool val);
      // construct from int
      JsonValue::JsonValue(int val);
      // construct from double
      JsonValue::JsonValue(double val);
      // construct from JsonObject
      JsonValue::JsonValue(const JsonObject&& val);
      JsonValue::JsonValue(JsonObject&& val);
      // // construct from JsonArray
      JsonValue::JsonValue(const JsonArray &val);
      JsonValue::JsonValue(const JsonArray&& val);

      // return type of value
      DataType GetType() const;

      // test for equality
      bool operator==(const JsonValue &rhs) const;
      // test for inequality
      bool operator!=(const JsonValue &rhs) const;
      // test if this < rhs
      bool operator <(const JsonValue &rhs) const;
      // test if this <= rhs
      bool operator <=(const JsonValue &rhs) const;
      // test if this > rhs
      bool operator >(const JsonValue &rhs) const;
      // test if this >= rhs
      bool operator >=(const JsonValue &rhs) const;

      // Subscript operator, access by key
      JsonValue& operator[] (const std::string& key);
      const JsonValue& operator[] (const std::string& key) const;

      // Subscript operator, access by index
      JsonValue& operator[] (JsonArray::SizeType idx);
      const JsonValue& operator[] (JsonArray::SizeType idx) const;

      // access function
      int GetDouble() const;
      void SetDouble(int val);

      int GetInt() const;
      void SetInt(int val);

      bool GetBool() const;
      void SetBool(bool val);

      const std::string& GetString() const;
      void SetString(const std::string& val);

      operator JsonObject () const { return object_; }
      const JsonObject& GetObject() const;
      void SetObject(const JsonObject& val);

      operator JsonArray () const;
      const JsonArray& GetArray() const;
      void SetArray(const JsonArray& val);

    private:
      Number double_;
      Boolean bool_;
      String string_;
      JsonObject object_;
      JsonArray array_;

      DataType type_;
    };
 
};


namespace mforms {
  enum JsonViewType
  {
    /// <summary> 
    /// Json editor with tree tabs texeditor, treeview, gridview
    /// </summary>
    JsonTabControl = 0,
    /// <summary> 
    /// One tab JSON text ditor
    /// </summary>
    JsonTextControl,
    /// <summary> 
    /// One tab JSON tree view editor
    /// </summary>
    JsonTreeControl,
    /// <summary> 
    /// One tab JSON grid view editor
    /// </summary>
    JsonGridControl,
    
  };

  /// <summary>
  /// Json view base class definition.
  /// <summary>
  class JsonBaseView : public Panel
  {
  public:
    JsonBaseView();
    virtual ~JsonBaseView();
    boost::signals2::signal<void()>* signalChanged();
    //void setText(const std::string &text);
    ///void setJson(const JsonParser::JsonValue &val);
    //const std::string &getText() const;
    //const JsonParser::JsonValue &getJson() const;

  protected:
    boost::signals2::signal<void()> _signalChanged;
  };

  class CodeEditor;
  /// <summary>
  /// Json text view control class definition.
  /// <summary>
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

  /// <summary>
  /// Json tree view control class definition.
  /// <summary>
  class TreeNodeView;
  class JsonTreeView : public JsonBaseView
  {
  public:
    JsonTreeView();
    virtual ~JsonTreeView();

  private:
    std::shared_ptr<TreeNodeView> _treeView;
  };

  /// <summary>
  /// Json grid view control class definition.
  /// <summary>
  class JsonGridView : public JsonBaseView
  {
  public:
    JsonGridView();
    virtual ~JsonGridView();
  };


  /// <summary>
  /// Json tab view control class definition.
  /// <summary>
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
