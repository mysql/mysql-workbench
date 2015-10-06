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
#include "mforms/treeview.h"
#include <algorithm>
#include <set>
#include <boost/tuple/tuple.hpp>

namespace JsonParser {

  enum DataType { VInt, VBoolean, VString, VDouble, VInt64, VUint64, VObject, VArray, VEmpty };
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
    JsonObject(const JsonObject &other);
    JsonObject &operator=(const JsonObject &val);

    JsonValue &operator [](const std::string &name);

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
    void erase(Iterator pos);
    void erase(Iterator first, Iterator last);

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
    Iterator insert(Iterator pos, const JsonValue &value);
    // insert element at end
    void pushBack(const ValueType &value);

  private:
    Container _data;
  };

  class MFORMS_EXPORT JsonValue
  {

  public:
    JsonValue();
    ~JsonValue() {};
    JsonValue(const JsonValue &rhs);
    JsonValue &operator=(const JsonValue &rhs);

    explicit JsonValue(const std::string &val);
    explicit JsonValue(const char *val);
    explicit JsonValue(bool val);
    explicit JsonValue(int val);
    explicit JsonValue(int64_t val);
    explicit JsonValue(uint64_t val);
    explicit JsonValue(double val);
    explicit JsonValue(const JsonObject &val);
    explicit JsonValue(const JsonArray &val);

    // implicit cast to actual element type. throw if its not possible
    operator const JsonObject & () const;
    operator const JsonArray & () const;
    operator int () const;
    operator double () const;
    operator int64_t() const;
    operator uint64_t() const;
    operator bool () const;
    operator const std::string & () const;

    // access function
    double getDouble() const;
    int getInt() const;
    void setNumber(double val);
    int64_t getInt64() const;
    void setInt64(int64_t val);
    uint64_t getUint64() const;
    void setUint64(uint64_t val);
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
    void setDeleted(bool flag);
    bool isDeleted() const;

  private:
    double _double;
    int64_t _integer64;
    uint64_t _uint64;
    bool _bool;
    std::string _string;
    JsonObject _object;
    JsonArray _array;
    DataType _type;
    bool _deleted;
  };

#if defined(_WIN32) || defined(__APPLE__)
  #define NOEXCEPT _NOEXCEPT
#else
  #ifndef _GLIBCXX_USE_NOEXCEPT
    #define NOEXCEPT throw()
  #else  
    #define NOEXCEPT _GLIBCXX_USE_NOEXCEPT
  #endif
#endif
  class MFORMS_EXPORT ParserException : public std::exception
  {
  public:
    explicit ParserException(const std::string &message) : _msgText(message) {}
    explicit ParserException(const char *message) : _msgText(message) {}
    virtual ~ParserException() NOEXCEPT {}
    virtual const char *what() const NOEXCEPT { return _msgText.c_str(); }
  private:
    std::string _msgText;
  };

  class MFORMS_EXPORT JsonReader
  {
    struct JsonToken
    {
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

  class MFORMS_EXPORT JsonWriter
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
 */
namespace mforms {
  /**
   * @brief Json view base class definition.
   */
  class JsonBaseView : public Panel
  {
  public:
    JsonBaseView();
    virtual ~JsonBaseView();
    void highlightMatch(const std::string &text);
    boost::signals2::signal<void(bool)>* dataChanged();

  protected:
    virtual void clear() = 0;
    boost::signals2::signal<void(bool)> _dataChanged;
  };

  /**
  * @brief Dialog for adding JSON.
  */
  class CodeEditor;
  class TextEntry;
  class JsonInputDlg : public mforms::Form
  {
  public:
    JsonInputDlg(mforms::Form *owner, bool showTextEntry);
    virtual ~JsonInputDlg();
    const std::string &text() const;
    const JsonParser::JsonValue &data() const;
    std::string objectName() const;
    void setText(const std::string &text, bool readonly);
    void setJson(const JsonParser::JsonValue &json);
    bool run();

  private:
    JsonParser::JsonValue _value;
    std::string _text;
    boost::shared_ptr<CodeEditor> _textEditor;
    Button *_save;
    Button *_cancel;
    TextEntry *_textEntry;
    bool _validated;

    void setup(bool showTextEntry);
    void validate();
    void save();
    void editorContentChanged(int position, int length, int numberOfLines, bool inserted);
  };

  /**
   * @brief Json text view control class definition.
   */
  class Label;
  class JsonTextView : public JsonBaseView
  {
  public:
    JsonTextView();
    virtual ~JsonTextView();
    void setText(const std::string &jsonText);
    virtual void clear();
    void findAndHighlightText(const std::string &text, bool backward = false);
    const JsonParser::JsonValue &getJson() const;

  private:
    void init();
    void editorContentChanged(int position, int length, int numberOfLines, bool inserted);
    virtual void validate();

    boost::shared_ptr<CodeEditor> _textEditor;
    boost::shared_ptr<Label> _validationResult;
    bool _modified;
    std::string _text;
    JsonParser::JsonValue _json;
  };

  class JsonTreeBaseView : public JsonBaseView
  {
  public:
    typedef std::list<TreeNodeRef> TreeNodeList;
    typedef std::vector<TreeNodeRef> TreeNodeVactor;
    typedef std::map <std::string, TreeNodeVactor> TreeNodeVectorMap;
    struct JsonValueNodeData : public mforms::TreeNodeData
    {
      JsonValueNodeData(JsonParser::JsonValue &value) : _jsonValue(value) {}
      JsonParser::JsonValue &getData() { return _jsonValue; }
      ~JsonValueNodeData() {}
    private:
      JsonParser::JsonValue &_jsonValue;
    };
    JsonTreeBaseView();
    virtual ~JsonTreeBaseView();
    enum JsonNodeIcons { JsonObjectIcon, JsonArrayIcon, JsonStringIcon, JsonNumericIcon, JsonNullIcon };
    void setCellValue(mforms::TreeNodeRef node, int column, const std::string &value);
    void highlightMatchNode(const std::string &text, bool bacward = false);
    bool filterView(const std::string &text, JsonParser::JsonValue &value);
    void reCreateTree(JsonParser::JsonValue &value);

  protected:
    void generateTree(JsonParser::JsonValue &value, mforms::TreeNodeRef node, bool addNew = true);
    virtual void generateArrayInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateObjectInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateNumberInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateBoolInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateStringInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateNullInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    void collectParents(TreeNodeRef node, TreeNodeList &parents);
    static std::string getNodeIconPath(JsonNodeIcons icon);
    TreeNodeVectorMap _viewFindResult;
    std::set<JsonParser::JsonValue*> _filterGuard;
    bool _useFilter;
    std::string _textToFind;
    size_t _searchIdx;
    boost::shared_ptr<TreeView> _treeView;
    mforms::ContextMenu *_contextMenu;

  private:
    void prepareMenu();
    void handleMenuCommand(const std::string &command);
    void openInputJsonWindow(TreeNodeRef node, bool updateMode = false);
  };

  /**
   * @brief Json tree view control class definition.
   */
  class JsonTreeView : public JsonTreeBaseView
  {
  public:
    JsonTreeView();
    virtual ~JsonTreeView();
    void setJson(JsonParser::JsonValue &val);
    void appendJson(JsonParser::JsonValue &val);
    virtual void clear();

  private:
    void init();
    virtual void generateArrayInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateObjectInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateNumberInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateBoolInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateStringInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateNullInTree(JsonParser::JsonValue &value, TreeNodeRef node);
  };

  /**
   * @brief Json grid view control class definition.
   */
  class JsonGridView : public JsonTreeBaseView
  {
  public:
    JsonGridView();
    virtual ~JsonGridView();
    void setJson(JsonParser::JsonValue &val);
    void appendJson(JsonParser::JsonValue &val);
    virtual void clear();

  private:
    void init();
    virtual void generateArrayInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateObjectInTree(JsonParser::JsonValue &value, TreeNodeRef node, bool addNew);
    virtual void generateNumberInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateBoolInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateStringInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    virtual void generateNullInTree(JsonParser::JsonValue &value, TreeNodeRef node);
    boost::shared_ptr<TreeView> _gridView;
  };

  /**
   * @brief Json tab view control class definition.
   */
  class TabView;
  class MFORMS_EXPORT JsonTabView : public Panel
  {
  public:
    typedef boost::shared_ptr<JsonParser::JsonValue> JsonValuePtr;
    void Setup();
    JsonTabView();
    ~JsonTabView();

    void setJson(const JsonParser::JsonValue &val);
    void setText(const std::string &text);
    void append(const std::string &text);
    void tabChanged();
    void dataChanged(bool forceUpdate);
    void clear();
    void highlightMatch(const std::string &text);
    void highlightNextMatch();
    void highlightPreviousMatch();
    bool filterView(const std::string &text);
    void restoreOrginalResult();

  private:
    boost::shared_ptr<JsonTextView> _textView;
    boost::shared_ptr<JsonTreeView> _treeView;
    boost::shared_ptr<JsonGridView> _gridView;
    boost::shared_ptr<TabView> _tabView;
    std::string _jsonText;
    JsonValuePtr _json;
    int _ident;
    boost::tuple<int, int, int> _tabId;
    std::string _matchText;
  };
};
