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

#include "mforms/jsonview.h"
#include <mforms/panel.h>
#include <mforms/treenodeview.h>
#include <mforms/gridview.h>
#include <mforms/code_editor.h>
#include <mforms/tabview.h>

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
*  @brief  %JsonObject move constructor.
*  @param val A %JsonObject of identical element and allocator types.
*/
JsonObject::JsonObject(JsonObject &&other) 
  : _data(std::move(other._data))
{
}

JsonObject &JsonObject::operator=(JsonObject &&other)
{
  _data = std::move(other._data);
  return *this;
}

// return iterator for begining of sequence
JsonObject::Iterator JsonObject::begin()
{
  return _data.begin();
}

JsonObject::ConstIterator JsonObject::begin() const
{
  return _data.begin();
}

JsonObject::ConstIterator JsonObject::cbegin() const
{
  return _data.begin();
}

JsonObject::Iterator JsonObject::end()
{
  return _data.end();
}

JsonObject::ConstIterator JsonObject::end() const
{
  return _data.end();
}

JsonObject::ConstIterator JsonObject::cend() const
{
  return _data.end();
}

JsonObject::SizeType JsonObject::size()
{
  return _data.size();
}

JsonObject::Iterator JsonObject::find(const KeyType& key)
{
  return _data.find(key);
}

JsonObject::ConstIterator JsonObject::find(const KeyType& key) const
{
  return _data.find(key);
}
// test if container is empty
bool JsonObject::empty() const
{
  return _data.empty();
}

void JsonObject::clear()
{
  _data.clear();
}

JsonObject::Iterator JsonObject::erase(Iterator it)
{
  return _data.erase(it);
}

JsonObject::Iterator JsonObject::erase(Iterator first, Iterator last)
{
  return _data.erase(first, last);
}

void JsonObject::insert(const KeyType &key, const JsonValue& value)
{
  _data[key] = value;
}

JsonValue &JsonObject::get(const KeyType &key)
{
    return _data[key];;
}

/// <summary>
/// Set JsonView factory method function.
/// </summary>
//bool JsonView::initFactoryMethod()
//{
//  registerFactory(&createInstance);
//  return true;
//}


// Default constructor
JsonArray::JsonArray()
{

}


// move operations
JsonArray::JsonArray(JsonArray &&other)
  : _data(std::move(other._data))
{
}

JsonArray &JsonArray::operator = (JsonArray &&other)
{
  _data = std::move(other._data);
  return *this;
}

// subscript sequence with checking
JsonValue& JsonArray::at(SizeType pos)
{
  assert(pos < _data.size());
  return _data.at(pos);
}
const JsonValue &JsonArray::at(SizeType pos) const
{
  assert(pos < _data.size());
  return _data.at(pos);
}

// subscript sequence
JsonValue &JsonArray::operator[](SizeType pos)
{
  assert(pos < _data.size());
  return _data[pos];
}


const JsonValue &JsonArray::operator[](SizeType pos) const
{
  assert(pos < _data.size());
  return _data[pos];
}


// return iterator for begining of sequence
JsonArray::Iterator JsonArray::begin()
{
  return _data.begin();
}


JsonArray::ConstIterator JsonArray::begin() const
{
  return _data.begin();
}

JsonArray::ConstIterator JsonArray::cbegin() const
{
  return _data.begin();
}

// return iterator for end of sequence
JsonArray::Iterator JsonArray::end()
{
  return _data.end();
}

JsonArray::ConstIterator JsonArray::end() const
{
  return _data.end();
}

JsonArray::ConstIterator JsonArray::cend() const
{
  return _data.end();
}

// return length of sequence
JsonArray::SizeType JsonArray::size()
{
  return _data.size();
}

// test if container is empty
bool JsonArray::empty() const
{
  return _data.empty();
}
void JsonArray::clear()
{
  _data.clear();
}


JsonArray::Iterator JsonArray::erase(Iterator pos)
{
  return _data.erase(pos);

}

JsonArray::Iterator JsonArray::erase(Iterator first, Iterator last)
{
  return _data.erase(first, last);
}

// insert value at pos
JsonArray::Iterator JsonArray::insert(Iterator pos, const JsonValue& value)
{
  return _data.insert(pos, value);
}

// insert element at end
void JsonArray::pushBack(const ValueType& value)
{
  _data.push_back(value);
}


JsonValue::JsonValue()
{
}

  // Copy operations
JsonValue::JsonValue(const JsonValue& rhs)
{}


JsonValue::JsonValue(JsonValue&& rhs)
{

}

JsonValue &JsonValue::operator=(const JsonValue& rhs)
{
  return *this;
}
// move operations
JsonValue& JsonValue::operator=(JsonValue&& rhs)
{
  return *this;
}


  // construct from std::string
JsonValue::JsonValue(const std::string& val)
{}
    
JsonValue::JsonValue(std::string&& val)
{}
  // construct from c string pointer
JsonValue::JsonValue(const char* val)
{}

  // construct from bool
JsonValue::JsonValue(bool val)
{}

// construct from int
JsonValue::JsonValue(int val)
{
}
// construct from double
JsonValue::JsonValue(double val)
{}
// construct from JsonObject
JsonValue::JsonValue(const JsonObject&& val)
{}
JsonValue::JsonValue(JsonObject&& val)
{}
// // construct from JsonArray
JsonValue::JsonValue(const JsonArray &val)
{}
JsonValue::JsonValue(const JsonArray&& val)
{}

// return type of value
DataType JsonValue::getType() const
{
  return _type;
}

// test for equality
bool JsonValue::operator==(const JsonValue &rhs) const
{

  return true;
}
// test for inequality
bool JsonValue::operator!=(const JsonValue &rhs) const
{
  return true;
}
// test if this < rhs
//bool JsonValue::operator <(const JsonValue &rhs) const
//{}
//// test if this <= rhs
//bool JsonValue::operator <=(const JsonValue &rhs) const
//{}
//// test if this > rhs
//bool JsonValue::operator >(const JsonValue &rhs) const
//{}
//// test if this >= rhs
//bool JsonValue::operator >=(const JsonValue &rhs) const
//{}

// Subscript operator, access by key
//JsonValue& JsonValue::operator[] (const std::string& key)
//{}
//const JsonValue& JsonValue::operator[] (const std::string& key) const
//{}
//
//// Subscript operator, access by index
//JsonValue& JsonValue::operator[] (JsonArray::SizeType idx)
//{}
//const JsonValue& JsonValue::operator[] (JsonArray::SizeType idx) const
//{}

// access function
double JsonValue::getDouble() const
{

  return _double;
}
void JsonValue::setDouble(double val)
{
  _double = val;

}

int JsonValue::getInt() const
{
  return (int)_double;
}
void JsonValue::setInt(int val)
{
  _double = val;
}

bool JsonValue::getBool() const
{
  return _bool;
}
void JsonValue::setBool(bool val)
{
  _bool = val;
}

const std::string& JsonValue::getString() const
{
  return _string;
}
void JsonValue::setString(const std::string& val)
{
  _string = val;
}

JsonValue::operator JsonObject() const
{ 
  return _object; 
}
const JsonObject& JsonValue::getObject() const
{
  return _object;
}
void JsonValue::setObject(const JsonObject& val)
{
  _object = val;
}

JsonValue::operator JsonArray() const
{
  if (_type == VAarray)
    return _array;
  else
    throw std::bad_typeid("Value is not of type array");
}
const JsonArray& JsonValue::getArray() const
{
  return _array;
}

void JsonValue::setArray(const JsonArray& val)
{
  _array = val;
}



/// <summary>
/// ctor.
/// </summary>
JsonBaseView::JsonBaseView() : Panel(TransparentPanel)
{
}

/// <summary>
/// dtor
/// </summary>
JsonBaseView::~JsonBaseView()
{
}

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
/// <summary>
/// ctor.
/// </summary>
JsonTextView::JsonTextView() 
  :  _textEditor(std::make_shared<CodeEditor>())
{
  init();
  scoped_connect(_textEditor->signal_changed(), boost::bind(&JsonTextView::textChanged, this));
}

void JsonTextView::textChanged()
{
  _signalChanged();
}

/// <summary>
/// dtor.
/// </summary>
JsonTextView::~JsonTextView()
{
}

/// <summary>
/// Init controls
/// </summary>
void JsonTextView::init()
{
  assert(_textEditor.get() != nullptr);
  _textEditor->set_language(mforms::LanguageNone);
  _textEditor->set_features(mforms::FeatureWrapText, true);
  _textEditor->set_features(mforms::FeatureReadOnly, false);
  add(_textEditor.get());
}



/**
* The content of a draw box is, by nature, drawn by the box itself, so we need to know what
* space the box needs. Overwritten by descendants. Subviews do not automatically add to the content
* size. If that's needed then additional computations are needed by the host.
*/

/// <summary>
/// ctor
/// </summary>
JsonTreeView::JsonTreeView()
{
}

/// <summary>
/// dtor
/// </summary>
JsonTreeView::~JsonTreeView()
{
}

/// <summary>
/// ctor
/// </summary>
JsonGridView::JsonGridView()
{
}

/// <summary>
/// dtor
/// </summary>
JsonGridView::~JsonGridView()
{
}

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

/// <summary>
/// ctor
/// </summary>
JsonTabView::JsonTabView() : Panel(TransparentPanel), _textView(std::make_shared<JsonTextView>()),
  _treeView(std::make_shared<JsonTreeView>()), _gridView(std::make_shared<JsonGridView>()), 
  _tabView(std::make_shared<TabView>(TabViewPalette))
{
  Setup();
}

/// <summary>
/// dtor
/// </summary>
JsonTabView::~JsonTabView()
{
}

void JsonTabView::setJson(const JsonParser::JsonValue& val)
{
}

//const JsonParser::JsonValue& JsonTabView::getJson() const
//{
//}

void JsonTabView::setText(const std::string& text)
{
  //_textView->set = text;
}

//const std::string& JsonTabView::getText() const
//{
// // return _textView;
//}

void JsonTabView::textViewTextChanged()
{
  int b = 3;
  b++;
}
