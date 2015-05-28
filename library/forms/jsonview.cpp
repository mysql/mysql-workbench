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

JsonView* (*JsonView::jsonViewFactory)(JsonViewType type) = nullptr;

// implicitly initialize the adv. sidebar
bool JsonView::__init = JsonView::initFactoryMethod();

/// <summary>
/// Set JsonView factory method function.
/// </summary>
bool JsonView::initFactoryMethod()
{
  registerFactory(&createInstance);
  return true;
}

/// <summary>
/// ctor.
/// </summary>
JsonView::JsonView() : Panel(TransparentPanel)
{
}

/// <summary>
/// dtor
/// </summary>
JsonView::~JsonView()
{
}

/// <summary>
/// The Json data as string to add to the control.
/// </summary>
/// <param name="text">A string that contains the json text data to set.</param>
void JsonView::setJson(const std::string &text)
{
}

/// <summary>
/// Retrieves data from the Json control in the text format
/// <summary>
/// <returns>Returns a string that represents the current control data.</returns>
const std::string &JsonView::getJson() const
{
  return _jsonText;
}

/// <summary>
/// The Json data as string to add to the control.
/// </summary>
/// <param name="text">A string that contains the json text data to set.</param>
void JsonView::setData(const std::string &text)
{
}

/// <summary>
/// Retrieves data from the Json control in the text format
/// <summary>
/// <returns>Returns a string that represents the current control data.</returns>
const std::string &JsonView::getData() const
{
  return getJson();
}

/// <summary>
/// Create JsonView control using factory pattern.
/// </summary>
/// <param name="jsonData">JSON data.</param>
/// <returns>pointer to jsonview control.</returns>
JsonView *JsonView::createInstance(JsonViewType type /*= JsonTabControl*/)
{
  switch (type)
  {
  case JsonTabControl:
    return new JsonTabView();
  case JsonTextControl:
    return new JsonTextView();
  case JsonTreeControl:
    return new JsonTreeView();
  case JsonGridControl:
    return new JsonGridView();
  default:
    throw new std::exception("no construction method found");
  }
}

/// <summary>
/// Register factory for JsonView control.
/// </summary>
/// <param name="create">JSON data.</param>
void JsonView::registerFactory(JsonView* (*create)(JsonViewType type))
{
  jsonViewFactory = create;
}

/// <summary>
/// The Json data as string to add to the control.
/// </summary>
/// <param name="text">A string that contains the json text data to set.</param>
void JsonTextView::setData(const std::string &text)
{
}

/// <summary>
/// Retrieves data from the Json control in the text format
/// <summary>
/// <returns>Returns a string that represents the current control data.</returns>
const std::string &JsonTextView::getData() const
{
  return getJson();
}

/// <summary>
/// ctor.
/// </summary>
JsonTextView::JsonTextView() 
  :  _textEditor(std::make_shared<CodeEditor>())
{
  init();
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
  cache_view(_textEditor.get());
}

/// <summary>
/// The Json data as string to add to the control.
/// </summary>
/// <param name="text">A string that contains the json text data to set.</param>
void JsonTextView::setJson(const std::string &text)
{
}

/// <summary>
/// Retrieves data from the Json control in the text format
/// <summary>
/// <returns>Returns a string that represents the current control data.</returns>
const std::string &JsonTextView::getJson() const
{
  return _jsonText;
}

/// <summary>
/// The Json data as string to add to the control.
/// </summary>
/// <param name="text">A string that contains the json text data to set.</param>
void JsonTreeView::setData(const std::string &text)
{
}

/// <summary>
/// Retrieves data from the Json control in the text format
/// <summary>
/// <returns>Returns a string that represents the current control data.</returns>
const std::string &JsonTreeView::getData() const
{
  return getJson();
}

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
/// The Json data as string to add to the control.
/// </summary>
/// <param name="text">A string that contains the json text data to set.</param>
void JsonTreeView::setJson(const std::string &text)
{
}

const std::string &JsonTreeView::getJson() const
{
  return _jsonText;
}

/// <summary>
/// The Json data as string to add to the control.
/// </summary>
/// <param name="text">A string that contains the json text data to set.</param>
void JsonGridView::setData(const std::string &text)
{
}

/// <summary>
/// Retrieves data from the Json control in the text format
/// <summary>
/// <returns>Returns a string that represents the current control data.</returns>
const std::string &JsonGridView::getData() const
{
  return getJson();
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

/// <summary>
/// The Json data as string to add to the control.
/// </summary>
/// <param name="text">A string that contains the json text data to set.</param>
void JsonGridView::setJson(const std::string& text)
{
}

/// <summary>
/// Retrieves data from the Json control in the text format
/// <summary>
/// <returns>Returns a string that represents the current control data.</returns>
const std::string &JsonGridView::getJson() const
{
  return _jsonText;
}

/// <summary>
/// The Json data as string to add to the control.
/// </summary>
/// <param name="text">A string that contains the json text data to set.</param>
void JsonTabView::setData(const std::string &text)
{
}

/// <summary>
/// Retrieves data from the Json control in the text format
/// <summary>
/// <returns>Returns a string that represents the current control data.</returns>
const std::string &JsonTabView::getData() const
{
  return getJson();
}

/// <summary>
/// ctor
/// </summary>
JsonTabView::JsonTabView() : _textView(std::make_shared<JsonTextView>()),
  _treeView(std::make_shared<JsonTreeView>()), _gridView(std::make_shared<JsonGridView>()), 
  _tabView(std::make_shared<TabView>(TabViewPalette))
{
  assert(_tabView.get() != NULL);
  _tabView->set_name("json_editor:tab");
  _tabView->add_page(manage(_textView.get()), "Text");
  _tabView->add_page(manage(_treeView.get()), "Tree");
  _tabView->add_page(manage(_gridView.get()), "Grid");
  cache_view(_tabView.get());
}

/// <summary>
/// dtor
/// </summary>
JsonTabView::~JsonTabView()
{
}

/// <summary>
/// The Json data as string to add to the control.
/// </summary>
/// <param name="text">A string that contains the json text data to set.</param>
void JsonTabView::setJson(const std::string &text)
{
  _jsonText = text;
}

/// <summary>
/// Retrieves data from the Json control in the text format
/// <summary>
/// <returns>Returns a string that represents the current control data.</returns>
const std::string &JsonTabView::getJson() const
{
  return _jsonText;
}