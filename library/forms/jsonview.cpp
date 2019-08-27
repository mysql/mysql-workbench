/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <set>
#include <sstream>
#include <cctype>
#include <future>

#ifdef __APPLE__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"
#endif

#include <boost/date_time.hpp>

#ifdef __APPLE__
#pragma GCC diagnostic pop
#endif

#include "mforms/jsonview.h"
#include "mforms/panel.h"
#include "mforms/treeview.h"
#include "mforms/code_editor.h"
#include "mforms/tabview.h"
#include "mforms/menubar.h"
#include "mforms/button.h"
#include "mforms/label.h"
#include "mforms/textentry.h"

#include "base/string_utilities.h"

#undef min

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

using namespace mforms;
using namespace rapidjson;

namespace ph = std::placeholders;
namespace bt = boost::posix_time;

// JSON Data structures implementation

//--------------------------------------------------------------------------------------------------

// JSON Control Implementation

static void findNode(TreeNodeRef parent, const std::string &text, JsonTreeBaseView::TreeNodeVectorMap &found) {
  if (parent.is_valid()) {
    auto node = parent;
    if (base::contains_string(node->get_string(1), text, false))
      found[text].push_back(node);
    int count = node->count();
    for (int i = 0; i < count; ++i) {
      TreeNodeRef child(node->get_child(i));
      if (child)
        findNode(child, text, found);
    }
  }
}

//--------------------------------------------------------------------------------------------------

static std::string getParseErrorText(ParseErrorCode code) {
  std::string text = "No error.";
  switch (code) {
    case kParseErrorDocumentEmpty:
      text = "The document is empty.";
      break;
    case kParseErrorDocumentRootNotSingular:
      text = "The document root must not follow by other values.";
      break;
    case kParseErrorValueInvalid:
      text = "Invalid value.";
      break;
    case kParseErrorObjectMissName:
      text = "Missing a name for object member.";
      break;
    case kParseErrorObjectMissColon:
      text = "Missing a colon after a name of object member.";
      break;
    case kParseErrorObjectMissCommaOrCurlyBracket:
      text = "Missing a comma or '}' after an object member.";
      break;
    case kParseErrorArrayMissCommaOrSquareBracket:
      text = "Missing a comma or ']' after an array element.";
      break;
    case kParseErrorStringUnicodeEscapeInvalidHex:
      text = "Incorrect hex digit after \\u escape in string.";
      break;
    case kParseErrorStringUnicodeSurrogateInvalid:
      text = "The surrogate pair in string is invalid.";
      break;
    case kParseErrorStringEscapeInvalid:
      text = "Invalid escape character in string.";
      break;
    case kParseErrorStringMissQuotationMark:
      text = "Missing a closing quotation mark in string.";
      break;
    case kParseErrorStringInvalidEncoding:
      text = "Invalid encoding in string.";
      break;
    case kParseErrorNumberTooBig:
      text = "Number too big to be stored in double.";
      break;
    case kParseErrorNumberMissFraction:
      text = "Miss fraction part in number.";
      break;
    case kParseErrorNumberMissExponent:
      text = "Miss exponent in number.";
      break;
    case kParseErrorTermination:
      text = "Parsing was terminated.";
      break;
    case kParseErrorUnspecificSyntaxError:
      text = "Unspecific syntax error.";
      break;
    case kParseErrorNone:
    default:
      text = "No error";
  }
  return text;
}

//--------------------------------------------------------------------------------------------------

JsonInputDlg::JsonInputDlg(mforms::Form *owner, bool showTextEntry)
  : mforms::Form(owner, mforms::FormResizable),
    _textEditor(manage(new CodeEditor())),
    _save(nullptr),
    _cancel(nullptr),
    _textEntry(nullptr),
    _validated(false) {
  setup(showTextEntry);
}

//--------------------------------------------------------------------------------------------------

JsonInputDlg::~JsonInputDlg() {
}

//--------------------------------------------------------------------------------------------------

std::string JsonInputDlg::objectName() const {
  return (_textEntry != NULL) ? _textEntry->get_string_value() : "";
}

//--------------------------------------------------------------------------------------------------

const std::string &JsonInputDlg::text() const {
  return _text;
}

//--------------------------------------------------------------------------------------------------

const rapidjson::Value &JsonInputDlg::data() const {
  return _value;
}

//--------------------------------------------------------------------------------------------------

bool JsonInputDlg::run() {
  return run_modal(NULL, _cancel);
}

//--------------------------------------------------------------------------------------------------

void JsonInputDlg::setup(bool showTextEntry) {
  Box *box = manage(new Box(false));
  Box *hbox = manage(new Box(true));
  Button *check = manage(new Button());

  if (showTextEntry) {
    Box *textEntryBox = manage(new Box(true));
    textEntryBox->set_padding(12);
    textEntryBox->set_spacing(12);
    Label *nameDescription = manage(new Label("Object name:"));
    _textEntry = manage(new TextEntry());
    textEntryBox->add(nameDescription, false, false);
    textEntryBox->add(_textEntry, true, true);
    box->add(textEntryBox, false, true);
  }
  _cancel = manage(new Button());
  _save = manage(new Button());

  set_title("JSON Editor");
  set_content(box);
  box->set_padding(12);
  box->set_spacing(12);
  _textEditor->set_language(mforms::LanguageJson);
  _textEditor->set_features(mforms::FeatureWrapText, false);
  _textEditor->set_features(mforms::FeatureReadOnly, false);
  box->add(_textEditor, true, true);
  box->add(hbox, false, true);
  hbox->add_end(_cancel, false, true);
  hbox->add_end(_save, false, true);
  hbox->add_end(check, false, true);
  hbox->set_spacing(12);
  check->set_text("Validate");
  _save->set_text("Save");
  _save->set_enabled(false);
  _cancel->set_text("Cancel");
  scoped_connect(check->signal_clicked(), std::bind(&JsonInputDlg::validate, this));
  scoped_connect(_save->signal_clicked(), std::bind(&JsonInputDlg::save, this));
  scoped_connect(_textEditor->signal_changed(),
                 std::bind(&JsonInputDlg::editorContentChanged, this, std::placeholders::_1, std::placeholders::_2,
                           std::placeholders::_3, std::placeholders::_4));
  set_size(800, 500);
  center();
}

//--------------------------------------------------------------------------------------------------

void JsonInputDlg::save() {
  if (_textEntry) {
    auto text = _textEntry->get_string_value();
    if (text.empty() && _textEntry->is_enabled()) {
      mforms::Utilities::show_error("JSON Editor.", "The field 'name' can not be empty", "Ok");
      return;
    }
  }
  end_modal(true);
}

//--------------------------------------------------------------------------------------------------

void JsonInputDlg::setText(const std::string &text, bool readonly) {
  if (_textEntry) {
    _textEntry->set_value(text);
    _textEntry->set_enabled(!readonly);
  }
}

//--------------------------------------------------------------------------------------------------

void JsonInputDlg::setJson(const Value &json) {
  Document d;
  d.CopyFrom(json, d.GetAllocator());
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  d.Accept(writer);

  _textEditor->set_text(buffer.GetString());
}

//--------------------------------------------------------------------------------------------------

void JsonInputDlg::validate() {
  auto text = _textEditor->get_text(false);
  if (text.empty())
    return;

  _document.Parse(text);
  _save->set_enabled(true);
  _validated = true;
  _value.CopyFrom(_document, _document.GetAllocator());
  _text = _textEditor->get_string_value();

  if (_document.HasParseError())
    mforms::Utilities::show_error(
      "JSON check.", base::strfmt("Validation failed: '%s'", getParseErrorText(_document.GetParseError()).c_str()), "Ok");
}

//--------------------------------------------------------------------------------------------------

void JsonInputDlg::editorContentChanged(Sci_Position /*position*/, Sci_Position /*length*/,
                                        Sci_Position /*numberOfLines*/, bool /*inserted*/) {
  _save->set_enabled(false);
  _validated = false;
  _text = "";
  _value = Value();
}

//--------------------------------------------------------------------------------------------------

JsonBaseView::JsonBaseView(Document &doc) : Panel(TransparentPanel), _document(doc) {
}

//--------------------------------------------------------------------------------------------------

bool JsonBaseView::isDateTime(const std::string &text) {
  static std::string validChars = "0123456789-.: ";
  if (text.find_first_not_of(validChars) != std::string::npos)
    return false;

  bt::time_input_facet *isoFacet = new bt::time_input_facet();
  isoFacet->set_iso_format();
  bt::time_input_facet *extendedIsoFacet = new bt::time_input_facet();
  isoFacet->set_iso_extended_format();
  static const std::locale formats[] = {
    std::locale(std::locale::classic(), isoFacet),
    std::locale(std::locale::classic(), extendedIsoFacet),
    std::locale(std::locale::classic(), new bt::time_input_facet("%Y-%m-%d %H:%M:%S")),
    std::locale(std::locale::classic(), new bt::time_input_facet("%Y/%m/%d %H:%M:%S")),
    std::locale(std::locale::classic(), new bt::time_input_facet("%d.%m.%Y %H:%M:%S")),
    std::locale(std::locale::classic(), new bt::time_input_facet("%Y-%m-%d"))
  };
  static const size_t formatCounts = sizeof(formats) / sizeof(formats[0]);

  bt::ptime pt;
  bool ret = false;
  for (size_t i = 0; i < formatCounts; ++i) {
    std::istringstream is(text);
    is.imbue(formats[i]);
    is >> pt;
    if (pt != bt::ptime()) {
      ret = true;
      break;
    }
  }
  return ret;
}

//--------------------------------------------------------------------------------------------------

JsonBaseView::~JsonBaseView() {
}

//--------------------------------------------------------------------------------------------------

boost::signals2::signal<void(bool)> *JsonBaseView::dataChanged() {
  return &_dataChanged;
}

//--------------------------------------------------------------------------------------------------

void JsonBaseView::clear() {
}

//--------------------------------------------------------------------------------------------------

JsonTreeBaseView::JsonTreeBaseView(rapidjson::Document &doc) : JsonBaseView(doc), _useFilter(false), _searchIdx(0) {
  _contextMenu = mforms::manage(new mforms::ContextMenu());
  _contextMenu->signal_will_show()->connect(std::bind(&JsonTreeBaseView::prepareMenu, this));
}

//--------------------------------------------------------------------------------------------------

void JsonTreeBaseView::prepareMenu() {
  if (_contextMenu) {
    _contextMenu->remove_all();
    auto node = _treeView->get_selected_node();
    if (!node.is_valid())
      return;
    auto *data = dynamic_cast<JsonValueNodeData *>(node->get_data());
    if (data != NULL) {
      auto &jv = data->getData();
      bool showAddModify = true;
      if (!jv.IsObject() && !jv.IsArray())
        showAddModify = false;

      auto *item = mforms::manage(new mforms::MenuItem("Add new value"));
      item->set_name("Add New Document");
      item->setInternalName("add_new_doc");
      item->signal_clicked()->connect(std::bind(&JsonTreeBaseView::handleMenuCommand, this, item->getInternalName()));
      item->set_enabled(showAddModify);
      _contextMenu->add_item(item);

      item = mforms::manage(new mforms::MenuItem("Delete JSON"));
      item->set_name("Delete Document");
      item->setInternalName("delete_doc");
      item->signal_clicked()->connect(std::bind(&JsonTreeBaseView::handleMenuCommand, this, item->getInternalName()));
      _contextMenu->add_item(item);

      item = mforms::manage(new mforms::MenuItem("Modify JSON"));
      item->set_name("Modify Document");
      item->setInternalName("modify_doc");
      item->signal_clicked()->connect(std::bind(&JsonTreeBaseView::handleMenuCommand, this, item->getInternalName()));
      item->set_enabled(showAddModify);
      _contextMenu->add_item(item);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void JsonTreeBaseView::handleMenuCommand(const std::string &command) {
  auto node = _treeView->get_selected_node();
  if (command == "add_new_doc") {
    openInputJsonWindow(node);
    return;
  }
  if (command == "delete_doc") {
    auto data = dynamic_cast<JsonValueNodeData *>(node->get_data());
    if (data != nullptr) {
      auto &jv = data->getData();
      auto parent = node->get_parent();
      if (parent != nullptr) {
        auto parentData = dynamic_cast<JsonValueNodeData *>(parent->get_data());
        if (parentData != nullptr) {
          auto &value = parentData->getData();
          if (value.IsArray()) {
            for (auto &item : value.GetArray()) {
              if (item == jv) {
                value.Erase(&item);
                break;
              }
            }
          } else if (value.IsObject()) {
            for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
              if (it->value == jv) {
                value.RemoveMember(it);
                break;
              }
            }
          }
        }
      }
      node->set_data(nullptr); // This will explicitly delete the data.
    }
    node->remove_from_parent();
    _dataChanged(false);
    return;
  }
  if (command == "modify_doc") {
    openInputJsonWindow(node, true);
  }
}

//--------------------------------------------------------------------------------------------------

void JsonTreeBaseView::openInputJsonWindow(TreeNodeRef node, bool updateMode /*= false*/) {
  auto data = dynamic_cast<JsonValueNodeData *>(node->get_data());
  if (data != nullptr) {
    auto &jv = data->getData();
    JsonInputDlg dlg(_treeView->get_parent_form(), jv.IsObject());
    if (updateMode) {
      if (jv.IsObject()) {
        auto tag = node->get_tag();
        dlg.setText(tag, true);
      }
      dlg.setJson(jv);
    }
    if (dlg.run()) {
      Value value;
      value.CopyFrom(dlg.data(), _document.GetAllocator());
      auto objectName = dlg.objectName();
      switch (jv.GetType()) {
        case kObjectType: {
          jv.AddMember(Value(objectName, _document.GetAllocator()), value, _document.GetAllocator());
          if (updateMode) {
            node->remove_children();
          }
          auto newNode = (updateMode) ? node : node->add_child();
          generateTree(objectName.empty() ? jv : jv[objectName], 0, newNode);
          newNode->set_string(0, objectName + "{" + std::to_string(jv.MemberCount()) + "}");
          newNode->set_tag(objectName);
          _dataChanged(false);
          break;
        }
        case kArrayType: {
          if (updateMode) {
            jv.Clear();
            node->remove_children();
            jv.CopyFrom(value, _document.GetAllocator());
          } else {
            jv.PushBack(value, _document.GetAllocator());
          }
          auto newNode = (updateMode) ? node : node->add_child();
          generateTree((updateMode) ? jv : *(jv.End()-1), 0, newNode);
          newNode->set_string(0, objectName + "[" + std::to_string(jv.Size()) + "]");
          _dataChanged(false);
          break;
        }
        default:
          break;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

JsonTreeBaseView::~JsonTreeBaseView() {
}

//--------------------------------------------------------------------------------------------------

void JsonTreeBaseView::generateStringInTree(rapidjson::Value &value, int columnId, TreeNodeRef node) {
  auto text = value.GetString();
  setStringData(columnId, node, text);
  node->set_data(new JsonTreeBaseView::JsonValueNodeData(value));
  node->expand();
}

//--------------------------------------------------------------------------------------------------

void JsonTreeBaseView::highlightMatchNode(const std::string &text, bool backward) {
  if (_textToFind != text) {
    _textToFind = text;
    _searchIdx = 0;
  }
  bool needSearch = false;
  auto it = _viewFindResult.find(text);
  if (it != _viewFindResult.end()) {
    if (_searchIdx >= it->second.size())
      _searchIdx = 0;
    auto node = it->second[_searchIdx];
    if (base::contains_string(node->get_string(1), text, false)) {
      _treeView->select_node(node);
      _treeView->scrollToNode(node);
      _searchIdx++;
    } else {
      _viewFindResult.erase(text);
      needSearch = true;
    }
  } else
    needSearch = true;
  if (needSearch) {
    _searchIdx = 0;
    auto node = _treeView->get_selected_node();
    if (!node.is_valid())
      node = _treeView->root_node();

    findNode(node, text, _viewFindResult);
    auto it = _viewFindResult.find(text);
    if (it != _viewFindResult.end()) {
      auto node = it->second[_searchIdx];
      _treeView->select_node(node);
      _treeView->scrollToNode(node);
      _treeView->focus();
    }
  }
}

//--------------------------------------------------------------------------------------------------

void JsonTreeBaseView::collectParents(TreeNodeRef node, std::list<TreeNodeRef> &parents) {
  auto parent = node->get_parent();
  if (parent->is_valid()) {
    parents.push_back(parent);
    collectParents(parent, parents);
  }
}

//--------------------------------------------------------------------------------------------------

void JsonTreeBaseView::reCreateTree(Value &value) {
  _useFilter = false;
  _treeView->clear();
  auto node = _treeView->root_node()->add_child();
  _treeView->BeginUpdate();
  Value o(kObjectType);
  o.CopyFrom(value, _document.GetAllocator());
  generateTree(value, 0, node);
  _treeView->EndUpdate();
}

//--------------------------------------------------------------------------------------------------

bool JsonTreeBaseView::filterView(const std::string &text, rapidjson::Value &value) {
  auto selectedNode = _treeView->get_selected_node();
  if (!selectedNode.is_valid())
    selectedNode = _treeView->root_node();
  TreeNodeVectorMap viewFilterResult;
  findNode(selectedNode, text, viewFilterResult);
  auto it = viewFilterResult.find(text);
  if (it != viewFilterResult.end()) {
    std::shared_ptr<TreeNodeList> branch(new TreeNodeList);
    for (auto node : it->second) {
      branch->push_back(node);
      collectParents(node, *branch);
    }
    _filterGuard.clear();
    auto actualNode = _treeView->root_node();
    while (!branch->empty()) {
      auto node = branch->back();
      branch->pop_back();
      auto data = dynamic_cast<JsonValueNodeData *>(node->get_data());
      if (data != nullptr) {
        auto &jv = data->getData();
        if (_filterGuard.count(&jv))
          continue;
        _filterGuard.insert(&jv);
      }
    }
    _useFilter = true;
    _treeView->clear();
    generateTree(value, 0, _treeView->root_node());
  }
  return _useFilter;
}

//--------------------------------------------------------------------------------------------------

void JsonTreeBaseView::generateTree(rapidjson::Value &value, int columnId, TreeNodeRef node, bool addNew) {
  switch (value.GetType()) {
    case kNumberType:
      generateNumberInTree(value, columnId, node);
      break;
    case kFalseType:
    case kTrueType:
      generateBoolInTree(value, columnId, node);
      break;
    case kStringType:
      generateStringInTree(value, columnId, node);
      break;
    case kObjectType:
      generateObjectInTree(value, columnId, node, addNew);
      break;
    case kArrayType:
      generateArrayInTree(value, columnId, node);
      break;
    case kNullType:
      generateNullInTree(value, columnId, node);
      break;
    default:
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void JsonTreeBaseView::setCellValue(mforms::TreeNodeRef node, int column, const std::string &value) {
  auto data = dynamic_cast<JsonValueNodeData *>(node->get_data());
  bool setData = false;
  if (data != nullptr) {
    std::stringstream buffer;
    double number = 0;
    auto &storedValue = data->getData();
    switch (storedValue.GetType()) {
      case kNumberType:
        if (!base::is_number(value))
          break;
        buffer << value;
        buffer >> number;
        storedValue = Value(number).Move();
        setData = true;
        break;
      case kTrueType:
        storedValue = Value(true).Move();
        setData = true;
        break;
      case kFalseType:
        storedValue = Value(false).Move();
        setData = true;
        break;
      case kStringType:
        storedValue = Value(value, _document.GetAllocator()).Move();
        setStringData(column, node, value);
        setData = true;
        break;
      default:
        break;
    }
  }
  if (setData) {
    node->set_string(column, value);
    _dataChanged(false);
  }
}

//--------------------------------------------------------------------------------------------------

JsonTextView::JsonTextView(Document &doc)
  : JsonBaseView(doc), _textEditor(manage(new CodeEditor())), _modified(false), _position(0) {
  init();
}

//--------------------------------------------------------------------------------------------------

void JsonTextView::setText(const std::string &jsonText, bool validateJson /*= true*/) {
  _textEditor->set_value(jsonText.c_str());
  if (validateJson)
    validate();
  _text = jsonText;
}

//--------------------------------------------------------------------------------------------------

const Value &JsonTextView::getJson() const {
  return _json;
}

//--------------------------------------------------------------------------------------------------

const std::string &JsonTextView::getText() const {
  return _text;
}

//--------------------------------------------------------------------------------------------------

JsonTextView::~JsonTextView() {
}

//--------------------------------------------------------------------------------------------------

void JsonTextView::clear() {
  _textEditor->set_value("");
}

//--------------------------------------------------------------------------------------------------

void JsonTextView::init() {
  assert(_textEditor != NULL);
  _textEditor->set_language(mforms::LanguageJson);
  _textEditor->set_features(mforms::FeatureWrapText, false);
  _textEditor->set_features(mforms::FeatureReadOnly, false);
  scoped_connect(_textEditor->signal_changed(),
                 [this](Sci_Position position, Sci_Position length, Sci_Position numberOfLines, bool inserted) {
                   editorContentChanged(position, length, numberOfLines, inserted);
                 });
  scoped_connect(_textEditor->signal_dwell(),
                 [this](bool started, size_t position, int x, int y) { dwellEvent(started, position, x, y); });
  Box *box = manage(new Box(false));
  box->set_padding(5);
  box->set_spacing(5);
  box->add(_textEditor, true, true);
  add(box);
}

//--------------------------------------------------------------------------------------------------

void JsonTextView::editorContentChanged(Sci_Position position, Sci_Position length, Sci_Position numberOfLines,
                                        bool inserted) {
  if (_stopTextProcessing)
    _stopTextProcessing();
  _modified = true;
  _position = position;
  _text = _textEditor->get_text(false);
  if (_startTextProcessing) {
    _startTextProcessing([this]() -> bool {
      _dataChanged(true);
      return false;
    });
  } else
    _dataChanged(true);
}

//--------------------------------------------------------------------------------------------------

bool JsonTextView::validate() {
  bool ret = true;

  if (_modified) {
    std::future<std::string> validateFuture = std::async(std::launch::async, [&, this]() -> std::string {
      _document.Parse(_text);
      if (_document.HasParseError()) {
        return getParseErrorText(_document.GetParseError());
      } else {
        _json.CopyFrom(_document, _document.GetAllocator());
      }
      return "";
    });
    validateFuture.wait();
    auto text = validateFuture.get();
    if (text.empty()) {
      _textEditor->remove_markup(LineMarkupAll, -1);
      _textEditor->remove_indicator(mforms::RangeIndicatorError, 0, _textEditor->text_length());
      _errorEntry.clear();
      _modified = false;
      ret = true;
    } else {
      int line = (int)_textEditor->line_from_position(_position);
      _textEditor->show_markup(LineMarkupError, line);
      std::size_t posBegin = _textEditor->position_from_line(line);
      posBegin = _text.find_first_not_of(" \t\r\n", posBegin);
      std::size_t posEnd = _text.find_first_of("\n\r", posBegin + 1);
      _textEditor->show_indicator(mforms::RangeIndicatorError, posBegin, posEnd - posBegin);
      _errorEntry.push_back(JsonErrorEntry{ text, posBegin, posEnd - posBegin });
      ret = false;
    }
  }
  return ret;
}

//--------------------------------------------------------------------------------------------------

void JsonTextView::dwellEvent(bool started, size_t position, int x, int y) {
  if (started) {
    if (_textEditor->indicator_at(position) == mforms::RangeIndicatorError) {
      auto end = _errorEntry.cend();
      auto it = std::find_if(_errorEntry.cbegin(), end, [&](const JsonErrorEntry &value) {
        return value.pos <= position && position <= value.pos + value.length;
      });
      if (it != end)
        _textEditor->show_calltip(true, position, it->text);
    }
  } else
    _textEditor->show_calltip(false, 0, "");
}

//--------------------------------------------------------------------------------------------------

void JsonTextView::findAndHighlightText(const std::string &text, bool backward /*= false*/) {
  _textEditor->find_and_highlight_text(text, mforms::FindDefault, true, backward);
}

//--------------------------------------------------------------------------------------------------

JsonTreeView::JsonTreeView(Document &doc) : JsonTreeBaseView(doc) {
  _treeView = manage(new mforms::TreeView(mforms::TreeAltRowColors | mforms::TreeShowRowLines |
                                          mforms::TreeShowColumnLines | mforms::TreeNoBorder));
  _treeView->add_column(IconStringColumnType, "Key", 150, false, true);
  _treeView->add_column(StringLTColumnType, "Value", 200, true, true);
  _treeView->add_column(StringLTColumnType, "Type", 200, false, true);
  _treeView->end_columns();
  _treeView->set_cell_edit_handler(std::bind(&JsonTreeBaseView::setCellValue, this, ph::_1, ph::_2, ph::_3));
  _treeView->set_selection_mode(TreeSelectSingle);
  _treeView->set_context_menu(_contextMenu);
  init();
}

//--------------------------------------------------------------------------------------------------

void JsonTreeView::init() {
  assert(_treeView != nullptr);
  add(_treeView);
}

//--------------------------------------------------------------------------------------------------

JsonTreeView::~JsonTreeView() {
  _treeView->clear();
}

//--------------------------------------------------------------------------------------------------

void JsonTreeView::clear() {
  _treeView->clear();
  _viewFindResult.clear();
  _textToFind = "";
  _searchIdx = 0;
  _useFilter = false;
}

//--------------------------------------------------------------------------------------------------

void JsonTreeView::setJson(rapidjson::Value &value) {
  clear();
  auto node = _treeView->root_node()->add_child();
  generateTree(value, 0, node);
}

//--------------------------------------------------------------------------------------------------

void JsonTreeView::appendJson(rapidjson::Value &value) {
  TreeNodeRef node = _treeView->root_node();
  _viewFindResult.clear();
  _textToFind = "";
  _searchIdx = 0;
  generateTree(value, 0, node);
}

//--------------------------------------------------------------------------------------------------

void JsonTreeView::generateObjectInTree(rapidjson::Value &value, int /*columnId*/, TreeNodeRef node, bool addNew) {
  if (_useFilter && _filterGuard.count(&value) == 0)
    return;
  size_t size = 0;
  node->set_data(new JsonTreeBaseView::JsonValueNodeData(value));

  for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
    std::string text = it->name.GetString();
    std::stringstream textSize;
    switch (it->value.GetType()) {
      case kArrayType: {
        auto &arrayVal = it->value;
        size = arrayVal.Size();
        node->set_tag(text);
        textSize << size;
        text += "[";
        text += textSize.str();
        text += "]";
        break;
      }
      case kObjectType: {
        auto &objectVal = it->value;
        size = objectVal.MemberCount();
        textSize << size;
        text += "{";
        text += textSize.str();
        text += "}";
        break;
      }
      default:
        break;
    }
    auto node2 = (addNew) ? node->add_child() : node;
    if (addNew) {
      node->set_icon_path(0, "JS_Datatype_Object.png");
      std::string name = node->get_string(0);
      if (name.empty())
        node->set_string(0, "<unnamed>");
      node->set_string(1, "");
      node->set_string(2, "Object");
    }
    node2->set_string(0, text);
    node2->set_tag(text);
    generateTree(it->value, 1, node2);
    node2->expand();
  }
}

//--------------------------------------------------------------------------------------------------

void JsonTreeView::generateArrayInTree(rapidjson::Value &value, int /*columnId*/, TreeNodeRef node) {
  if (_useFilter && _filterGuard.count(&value) == 0)
    return;

  node->set_icon_path(0, "JS_Datatype_Array.png");
  std::string name = node->get_string(0);
  if (name.empty())
    node->set_string(0, "<unnamed>");
  node->set_string(1, "");
  node->set_string(2, "Array");
  std::string tagName = node->get_tag();
  node->set_data(new JsonTreeBaseView::JsonValueNodeData(value));

  int index = 0;
  for (auto &v : value.GetArray()) {
    if (_useFilter && _filterGuard.count(&v) == 0)
      continue;
    auto arrrayNode = node->add_child();
    bool addNew = false;
    if (v.GetType() == kArrayType || v.GetType() == kObjectType)
      addNew = true;
    std::string keyName = tagName.empty() ? "key[%d]" : tagName + "[%d]";
    arrrayNode->set_string(0, base::strfmt(keyName.c_str(), index));
    arrrayNode->set_string(1, "");
    generateTree(v, 1, arrrayNode, addNew);
    index++;
  }
  node->expand();
}

//--------------------------------------------------------------------------------------------------

void JsonTreeView::generateBoolInTree(rapidjson::Value &value, int /*columnId*/, TreeNodeRef node) {
  node->set_icon_path(0, "JS_Datatype_Bool.png");
  node->set_attributes(1, mforms::TextAttributes("#4b4a4c", false, false));
  node->set_bool(1, value.GetBool());
  node->set_string(2, "Boolean");
  node->set_data(new JsonTreeBaseView::JsonValueNodeData(value));
  node->expand();
}

//--------------------------------------------------------------------------------------------------

void JsonTreeView::generateNumberInTree(rapidjson::Value &value, int /*columnId*/, TreeNodeRef node) {
  node->set_icon_path(0, "JS_Datatype_Number.png");
  node->set_attributes(1, mforms::TextAttributes("#4b4a4c", false, false));
  if (value.IsDouble()) {
    node->set_string(1, std::to_string(value.GetDouble()));
    node->set_string(2, "Double");
  } else if (value.IsInt64()) {
    node->set_string(1, std::to_string(value.GetInt64()));
    node->set_string(2, "Long Integer");
  } else if (value.IsUint64()) {
    node->set_string(1, std::to_string(value.GetUint64()));
    node->set_string(2, "Unsigned Long Integer");
  }
  node->set_data(new JsonTreeBaseView::JsonValueNodeData(value));
  node->expand();
}

//--------------------------------------------------------------------------------------------------

void JsonTreeView::generateNullInTree(rapidjson::Value &value, int /*columnId*/, TreeNodeRef node) {
  node->set_icon_path(0, "JS_Datatype_Null.png");
  node->set_string(0, "null");
  node->set_string(1, "");
  node->set_string(2, "Null");
  node->set_data(new JsonTreeBaseView::JsonValueNodeData(value));
  node->expand();
}

//--------------------------------------------------------------------------------------------------

void JsonTreeView::setStringData(int /*columnId*/, TreeNodeRef node, const std::string &text) {
  /*if (isDateTime(text))
  {
    node->set_icon_path(0, "JS_Datatype_Date.png");
    node->set_string(2, "Date/Time");
  }
  else*/
  {
    node->set_icon_path(0, "JS_Datatype_String.png");
    node->set_string(2, "String");
  }
  node->set_attributes(1, mforms::TextAttributes("#4b4a4c", false, false));
  node->set_string(1, text.c_str());
}

//--------------------------------------------------------------------------------------------------

JsonGridView::JsonGridView(Document &doc)
  : JsonTreeBaseView(doc),
    _level(0),
    _headerAdded(false),
    _noNameColId(-1),
    _columnIndex(0),
    _rowNum(1),
    _actualParent(20) {
  init();
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Init flat grid view
 *
 * Based of readed JSON data control function initialize mforms control TreNodeView
 */
void JsonGridView::init() {
  _treeView = manage(new mforms::TreeView(mforms::TreeAltRowColors | mforms::TreeShowRowLines |
                                          mforms::TreeShowColumnLines | mforms::TreeNoBorder));
  assert(_treeView != nullptr);
  _treeView->add_column(StringLTColumnType, "", 30, false, false);
  _treeView->set_cell_edit_handler(
    std::bind(&JsonGridView::setCellValue, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _treeView->signal_node_activated()->connect(
    std::bind(&JsonGridView::nodeActivated, this, std::placeholders::_1, std::placeholders::_2));

  _treeView->set_selection_mode(TreeSelectSingle);
  _treeView->set_context_menu(_contextMenu);

  _goUpButton = manage(new Button());
  _goUpButton->set_text("Back <<<");
  _goUpButton->set_enabled(false);
  scoped_connect(_goUpButton->signal_clicked(), std::bind(&JsonGridView::goUp, this));

  _content = manage(new Box(false));
  _content->add(_treeView, true, true);

  Box *hbox = manage(new Box(true));
  hbox->add_end(_goUpButton, false, false);
  hbox->set_size(-1, 30);
  _content->add(hbox, false, false);
  add(_content);
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::goUp() {
  if (_level <= 0 || _actualParent.empty())
    return;
  rapidjson::Value *value = _actualParent.at(_level - 1);
  if (value == NULL)
    return;
  setJson(*value);
  if (--_level <= 0)
    _goUpButton->set_enabled(false);
}

//--------------------------------------------------------------------------------------------------

JsonGridView::~JsonGridView() {
  _treeView->clear();
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::clear() {
  _treeView->clear();
  _viewFindResult.clear();
  _textToFind = "";
  _searchIdx = 0;
  _useFilter = false;
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::setJson(rapidjson::Value &value) {
  clear();
  _rowNum = 1;
  if (!_headerAdded) {
    _columnIndex = 1;
    _level = 0;
    _noNameColId = -1;
    generateColumnNames(value);
    _treeView->end_columns();
    _headerAdded = true;
  }
  if (_level >= (int)_actualParent.size())
    _actualParent.resize(_actualParent.size() * 2);
  _actualParent[_level] = &value;
  TreeNodeRef node = _treeView->root_node();
  generateTree(value, 0, node);
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::reCreateTree(rapidjson::Value &value) {
  remove(_content);
  init();
  _headerAdded = false;
  _colNameToColId.clear();
  setJson(value);
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::addColumn(int size, Type type, Value *value, const std::string &name) {
  switch (type) {
    case kArrayType:
    case kObjectType:
      _treeView->add_column(IconStringColumnType, name, size, false, true);
      break;

    case kNumberType: {
      if (value != nullptr && (value->IsFloat() || value->IsDouble())) {
        _treeView->add_column(FloatColumnType, name, size, true, true);
      } else {
        _treeView->add_column(LongIntegerColumnType, name, size, true, true);
      }
      break;
    }

    case kTrueType:
    case kFalseType:
    case kStringType:
    case kNullType:
    default:
      _treeView->add_column(IconStringColumnType, name, size, true, true);
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::generateColumnNames(rapidjson::Value &value) {
  if (_level != 0)
    return;
  switch (value.GetType()) {
    case kObjectType: {
      for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
        if (_colNameToColId.count(it->name.GetString()) == 1)
          continue;
        addColumn(100, it->value.GetType(), &value, it->name.GetString());
        _colNameToColId[it->name.GetString()] = _columnIndex++;
        if (it->value.GetType() == kObjectType || it->value.GetType() == kArrayType)
          generateColumnNames(it->value);
      }
      break;
    }
    case kArrayType: {
      for (auto &item : value.GetArray()) {
        if (item.GetType() == kObjectType) {
          Value &obj = item;
          for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
            if (_colNameToColId.count(it->name.GetString()) == 1)
              continue;
            addColumn(100, it->value.GetType(), &value, it->name.GetString());
            _colNameToColId[it->name.GetString()] = _columnIndex++;
            if (it->value.GetType() == kObjectType || it->value.GetType() == kArrayType)
              generateColumnNames(it->value);
          }
        } else {
          if (_noNameColId > 0)
            continue;
          addColumn(100, kStringType, nullptr, "");
          _noNameColId = _columnIndex++;
        }
        if (item.GetType() == kObjectType || item.GetType() == kArrayType)
          generateColumnNames(item);
      }
      break;
    }
    default:
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::setCellValue(mforms::TreeNodeRef node, int column, const std::string &value) {
  JsonValueNodeData *data = dynamic_cast<JsonValueNodeData *>(node->get_data());
  if (data == NULL)
    return;

  const std::map<std::string, int>::const_iterator it =
    std::find_if(_colNameToColId.begin(), _colNameToColId.end(),
                 [&column](const std::pair<std::string, int> &elem) { return column == elem.second; });

  if (it == _colNameToColId.end() || it->first.empty())
    return;

  std::string key = it->first;
  rapidjson::Value &valData = data->getData();
  if (valData.FindMember(it->first) == valData.MemberEnd())
    return;

  rapidjson::Value &storedValue = valData[key];
  if (data != NULL) {
    std::stringstream buffer;
    double number = 0;
    int64_t number2 = 0;
    uint64_t number3 = 0;
    long number4 = 0;
    bool retBool = false;
    switch (storedValue.GetType()) {
      case kNumberType: {
        if (!base::is_number(value))
          break;
        if (storedValue.IsDouble()) {
          buffer << value;
          buffer >> number;
          storedValue = number;
          node->set_float(column, number);
        } else if (storedValue.IsInt64() || storedValue.IsInt()) {
          buffer << value;
          buffer >> number2;
          storedValue.SetInt64(number2);
          node->set_long(column, number2);
        } else if (storedValue.IsUint64()) {
          buffer << value;
          buffer >> number3;
          storedValue.SetUint64(number3);
          node->set_float(column, (double)number3);
        } else {
          buffer << value;
          buffer >> number4;
          storedValue.SetInt(number4);
          node->set_long(column, number4);
        }
        break;
      }
      case kTrueType:
      case kFalseType:
        if (!base::isBool(value))
          break;
        buffer << value;
        buffer >> std::boolalpha >> retBool;
        storedValue.SetBool(retBool);
        node->set_bool(column, retBool);
        _dataChanged(false);
        break;
      case kStringType:
        storedValue.SetString(value, _document.GetAllocator());
        setStringData(column, node, value);
        node->set_string(column, value);
        _dataChanged(false);
        break;
      default:
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::openInputJsonWindow(rapidjson::Value &value) {
  JsonInputDlg dlg(_treeView->get_parent_form(), false);
  dlg.setJson(value);
  if (dlg.run()) {
    value.CopyFrom(dlg.data(), _document.GetAllocator());
    _actualParent[_level] = &value;
    reCreateTree(*_actualParent.at(0));
    setJson(*_actualParent.at(_level));
    _dataChanged(false);
  }
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::handleMenuCommand(const std::string &command) {
  rapidjson::Value *parent = _actualParent.at(_level);
  if (parent == nullptr)
    return;
  if (command == "add_new_doc" || command == "modify_doc") {
    openInputJsonWindow(*parent);
    return;
  }
  if (command == "delete_doc") {
    TreeNodeRef node = _treeView->get_selected_node();
    if (!node.is_valid())
      return;
    auto *data = dynamic_cast<JsonValueNodeData *>(node->get_data());
    if (data != nullptr) {
      rapidjson::Value &jv = data->getData();
      if (parent->IsArray()) {
        for (auto &item : parent->GetArray()) {
          if (item == jv) {
            parent->Erase(&item);
            break;
          }
        }
      } else if (parent->IsObject()) {
        parent->RemoveAllMembers();
      }
      node->set_data(nullptr); // This will explicitly delete the data.
    }
    node->remove_from_parent();
    _dataChanged(false);
  }
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::appendJson(rapidjson::Value & /*value*/) {
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::generateObjectInTree(rapidjson::Value &value, int columnId, TreeNodeRef node, bool addNew) {
  auto child = node;
  if (addNew)
    child = node->add_child();
  size_t size = 0;
  std::stringstream textSize;
  child->set_data(new JsonTreeBaseView::JsonValueNodeData(value));
  node->set_string(0, std::to_string(_rowNum++));
  for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
    std::string text = it->name.GetString();
    if (_colNameToColId.count(text) == 0)
      continue;
    int index = _colNameToColId[text];
    switch (it->value.GetType()) {
      case kArrayType: {
        auto const& arrayVal = it->value.GetArray();
        size = arrayVal.Size();
        textSize << size;
        text = "Array [";
        text += textSize.str();
        text += "]";
        child->set_icon_path(index, "JS_Datatype_Array.png");
        child->set_string(index, text);
        break;
      }
      case kObjectType: {
        auto &objectVal = it->value;
        size = objectVal.MemberCount();
        textSize << size;
        text = "Object {";
        text += textSize.str();
        text += "}";
        child->set_icon_path(index, "JS_Datatype_Object.png");
        child->set_string(index, text);
        break;
      }
      case kNumberType:
        generateNumberInTree(it->value, index, child);
        break;
      case kTrueType:
      case kFalseType:
        generateBoolInTree(it->value, index, child);
        break;
      case kStringType:
        setStringData(index, child, it->value.GetString());
        break;
      case kNullType:
        generateNullInTree(it->value, index, child);
        break;
      default:
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::generateArrayInTree(rapidjson::Value &value, int /*columnId*/, TreeNodeRef /*node*/) {
  auto const& arrayType = value.GetArray();
  for (auto &item : arrayType) {
    mforms::TreeNodeRef arrrayNode = _treeView->root_node()->add_child();
    arrrayNode->set_string(0, std::to_string(_rowNum++));
    switch (item.GetType()) {
      case kArrayType: {
        auto const& arrayVal = item.GetArray();
        size_t size = arrayVal.Size();
        std::stringstream textSize;
        textSize << size;
        std::string text = "Array [";
        text += textSize.str();
        text += "]";
        arrrayNode->set_icon_path(_noNameColId, "JS_Datatype_Array.png");
        arrrayNode->set_string(_noNameColId, text);
        arrrayNode->set_data(new JsonTreeBaseView::JsonValueNodeData(item));
        break;
      }
      case kObjectType:
        _rowNum--;
        generateObjectInTree(item, 0, arrrayNode, false);
        break;
      case kNumberType:
        generateNumberInTree(item, _noNameColId, arrrayNode);
        arrrayNode->set_data(new JsonTreeBaseView::JsonValueNodeData(item));
        break;
      case kTrueType:
      case kFalseType:
        generateBoolInTree(item, _noNameColId, arrrayNode);
        arrrayNode->set_data(new JsonTreeBaseView::JsonValueNodeData(item));
        break;
      case kStringType:
        setStringData(_noNameColId, arrrayNode, item.GetString());
        arrrayNode->set_data(new JsonTreeBaseView::JsonValueNodeData(item));
        break;
      case kNullType:
        generateNullInTree(item, _noNameColId, arrrayNode);
        arrrayNode->set_data(new JsonTreeBaseView::JsonValueNodeData(item));
        break;
      default:
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::nodeActivated(TreeNodeRef node, int column) {
  if (column > 0) {
    JsonValueNodeData *data = dynamic_cast<JsonValueNodeData *>(node->get_data());
    if (!data)
      return;

    rapidjson::Value &storedValue = data->getData();
    if (storedValue.GetType() == kObjectType) {
      const std::map<std::string, int>::const_iterator it =
        std::find_if(_colNameToColId.begin(), _colNameToColId.end(),
                     [&column](const std::pair<std::string, int> &elem) { return column == elem.second; });

      if (it != _colNameToColId.end() && storedValue.FindMember(it->first) != storedValue.MemberEnd()) {
        rapidjson::Value &clickedValue = storedValue[it->first];
        if (clickedValue.GetType() != kObjectType && clickedValue.GetType() != kArrayType)
          return;
        _level++;
        setJson(clickedValue);
        _goUpButton->set_enabled(true);
      }
    }
    if (storedValue.GetType() == kArrayType) {
      _level++;
      setJson(storedValue);
      _goUpButton->set_enabled(true);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::generateBoolInTree(rapidjson::Value &value, int columnId, TreeNodeRef node) {
  node->set_bool(columnId, value.GetBool());
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::generateNumberInTree(rapidjson::Value &value, int columnId, TreeNodeRef node) {
  if (value.IsDouble()) {
    node->set_float(columnId, value.GetDouble());
  } else if (value.IsInt64()) {
    node->set_long(columnId, value.GetInt64());
  } else if (value.IsUint64()) {
    node->set_long(columnId, value.GetUint64());
  } else if (value.IsNumber()) {
    node->set_long(columnId, value.GetInt());
  }
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::generateNullInTree(rapidjson::Value &value, int columnId, TreeNodeRef node) {
  node->set_string(columnId, "null");
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::setStringData(int columnId, TreeNodeRef node, const std::string &text) {
  if (isDateTime(text))
    node->set_icon_path(0, "JS_Datatype_Date.png");
  node->set_attributes(columnId, mforms::TextAttributes("#4b4a4c", false, false));
  node->set_string(columnId, text.c_str());
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::Setup() {
  assert(_tabView != NULL);
  _tabView->set_name("JSON Editor");
  _tabId.textTabId = _tabView->add_page(_textView, "Text");
  _tabId.treeViewTabId = _tabView->add_page(_treeView, "Tree");
  _tabId.gridViewTabId = _tabView->add_page(_gridView, "Grid");
  add(_tabView);

  scoped_connect(_textView->dataChanged(), std::bind(&JsonTabView::dataChanged, this, std::placeholders::_1));
  scoped_connect(_treeView->dataChanged(), std::bind(&JsonTabView::dataChanged, this, std::placeholders::_1));
  scoped_connect(_gridView->dataChanged(), std::bind(&JsonTabView::dataChanged, this, std::placeholders::_1));
  scoped_connect(_tabView->signal_tab_changed(), std::bind(&JsonTabView::tabChanged, this));
}

//--------------------------------------------------------------------------------------------------

JsonTabView::JsonTabView(bool tabLess, JsonTabViewType defaultView)
  : Panel(TransparentPanel),
    _textView(manage(new JsonTextView(_document))),
    _treeView(manage(new JsonTreeView(_document))),
    _gridView(manage(new JsonGridView(_document))),
    _tabView(manage(new TabView(tabLess ? TabViewTabless : TabViewPalette))),
    _updating(false),
    _defaultView(defaultView) {
  Setup();
}

//--------------------------------------------------------------------------------------------------

JsonTabView::~JsonTabView() {
}

//--------------------------------------------------------------------------------------------------
void JsonTabView::setJson(const rapidjson::Value &value) {
  Document d;
  _json.CopyFrom(value, d.GetAllocator());
  _ident = 0;
  _updating = true;
  d.CopyFrom(_json, d.GetAllocator());
  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  d.Accept(writer);
  _jsonText = buffer.GetString();

  _updateView = { true, true, true };
  switch (_defaultView) {
    case JsonTabView::TabText:
      _textView->setText(_jsonText, false);
      _updateView.textViewUpdate = false;
      break;
    case JsonTabView::TabTree:
      _treeView->setJson(_json);
      _updateView.treeViewUpdate = false;
      break;
    case JsonTabView::TabGrid:
      _gridView->setJson(_json);
      _updateView.gridViewUpdate = false;
      break;
  }
  switchTab(_defaultView);
  _updating = false;
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::setText(const std::string &text, bool validate) {
  _jsonText = text;
  _textView->setText(text, validate);
  _updateView.textViewUpdate = false;
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::tabChanged() {
  int tabId = _tabView->get_active_tab();
  if (tabId == _tabId.textTabId && _updateView.textViewUpdate) {
    _updating = true;
    _textView->setText(_jsonText);
    _updateView.textViewUpdate = false;
    _updating = false;
    _dataChanged(_jsonText);
  } else if (tabId == _tabId.treeViewTabId && _updateView.treeViewUpdate) {
    _treeView->reCreateTree(_json);
    _updateView.treeViewUpdate = false;
    _dataChanged(_jsonText);
  } else if (tabId == _tabId.gridViewTabId && _updateView.gridViewUpdate) {
    _gridView->reCreateTree(_json);
    _updateView.gridViewUpdate = false;
    _dataChanged(_jsonText);
  }
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::dataChanged(bool forceUpdate) {
  if (_updating)
    return;
  int tabId = _tabView->get_active_tab();
  if (tabId != _tabId.textTabId) {
    _document.CopyFrom(_json, _document.GetAllocator());
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    _document.Accept(writer);
    _jsonText = buffer.GetString();
  } else {
    if (_textView->validate()) {
      _jsonText = _textView->getText();
      _json.CopyFrom(_textView->getJson(), _document.GetAllocator());
    } else
      return;
  }
  _updateView.textViewUpdate = tabId != _tabId.textTabId;
  _updateView.treeViewUpdate = tabId != _tabId.treeViewTabId;
  _updateView.gridViewUpdate = tabId != _tabId.gridViewTabId;

  _dataChanged(_jsonText);
}

//--------------------------------------------------------------------------------------------------

boost::signals2::signal<void(const std::string &text)> *JsonTabView::editorDataChanged() {
  return &_dataChanged;
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::clear() {
  _jsonText.clear();
  _textView->clear();
  _treeView->clear();
  _gridView->clear();
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::highlightMatch(const std::string &text) {
  _matchText = text;
  int tabId = _tabView->get_active_tab();
  if (tabId == _tabId.textTabId) {
    _textView->findAndHighlightText(text);
  } else if (tabId == _tabId.treeViewTabId) {
    _treeView->highlightMatchNode(text);
  } else if (tabId == _tabId.gridViewTabId) {
    _gridView->highlightMatchNode(text);
  }
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::highlightNextMatch() {
  int tabId = _tabView->get_active_tab();
  if (tabId == _tabId.textTabId && !_matchText.empty()) {
    _textView->findAndHighlightText(_matchText);
  } else if (tabId == _tabId.treeViewTabId && !_matchText.empty()) {
    _treeView->highlightMatchNode(_matchText);
  } else if (tabId == _tabId.gridViewTabId && !_matchText.empty()) {
    _gridView->highlightMatchNode(_matchText);
  }
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::highlightPreviousMatch() {
  int tabId = _tabView->get_active_tab();
  if (tabId == _tabId.textTabId && !_matchText.empty()) {
    _textView->findAndHighlightText(_matchText, true);
  } else if (tabId == _tabId.treeViewTabId && !_matchText.empty()) {
    _treeView->highlightMatchNode(_matchText, true);
  } else if (tabId == _tabId.gridViewTabId && !_matchText.empty()) {
    _gridView->highlightMatchNode(_matchText, true);
  }
}

//--------------------------------------------------------------------------------------------------

bool JsonTabView::filterView(const std::string &text) {
  int tabId = _tabView->get_active_tab();
  bool ret = false;
  if (tabId == _tabId.textTabId) {
    return false; // no filtering for text view
  } else if (tabId == _tabId.treeViewTabId) {
    ret = _treeView->filterView(text, _json);
  } else if (tabId == _tabId.gridViewTabId) {
    ret = _gridView->filterView(text, _json);
  }
  return ret;
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::restoreOrginalResult() {
  int tabId = _tabView->get_active_tab();
  if (tabId == _tabId.textTabId) {
    return;
  } else if (tabId == _tabId.treeViewTabId) {
    _treeView->reCreateTree(_json);
  } else if (tabId == _tabId.gridViewTabId) {
    _gridView->reCreateTree(_json);
  }
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::switchTab(JsonTabViewType tab) const {
  switch (tab) {
    case JsonTabViewType::TabText:
      _tabView->set_active_tab(_tabId.textTabId);
      break;
    case JsonTabViewType::TabTree:
      _tabView->set_active_tab(_tabId.treeViewTabId);
      break;
    case JsonTabViewType::TabGrid:
      _tabView->set_active_tab(_tabId.gridViewTabId);
      break;
    default:
      _tabView->set_active_tab(_tabId.textTabId);
  }
}

//--------------------------------------------------------------------------------------------------

JsonTabView::JsonTabViewType JsonTabView::getActiveTab() const {
  int tabId = _tabView->get_active_tab();
  if (tabId == _tabId.textTabId)
    return JsonTabViewType::TabText;
  else if (tabId == _tabId.treeViewTabId)
    return JsonTabViewType::TabTree;
  else
    return JsonTabViewType::TabGrid;
}

//--------------------------------------------------------------------------------------------------

const std::string &JsonTabView::text() const {
  return _jsonText;
}

//--------------------------------------------------------------------------------------------------

const rapidjson::Value &JsonTabView::json() const {
  return _json;
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::setTextProcessingStartHandler(std::function<void(std::function<bool()>)> callback) {
  if (_textView)
    _textView->_startTextProcessing = callback;
}

//--------------------------------------------------------------------------------------------------

void JsonTabView::setTextProcessingStopHandler(std::function<void()> callabck) {
  if (_textView)
    _textView->_stopTextProcessing = callabck;
}

//--------------------------------------------------------------------------------------------------
