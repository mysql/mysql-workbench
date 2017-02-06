/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <set>
#include <sstream>
#include <cctype>
#include <future>

#include <boost/date_time.hpp>

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

using namespace mforms;
using namespace JsonParser;

namespace ph = std::placeholders;
namespace bt = boost::posix_time;

// JSON Data structures implementation

//--------------------------------------------------------------------------------------------------

// JSON Control Implementation

/**
 * @brief Find node in tree recursively.
 *
 * parent Parent node reference
 * text Text to find.
 * founded Map reference to save results.
 */
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

/**
 * @brief Get JSON Object name.
 *
 * @returns Obect name.
 */
std::string JsonInputDlg::objectName() const {
  return (_textEntry != NULL) ? _textEntry->get_string_value() : "";
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Get JSON data in text format.
 *
 * @returns JSON text.
 */
const std::string &JsonInputDlg::text() const {
  return _text;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Get JSON value.
 *
 * @returns JSON value.
 */
const JsonParser::JsonValue &JsonInputDlg::data() const {
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

/**
* @brief Set text.
*
* @text Text to set.
* @readOnly Set control read only.
*/
void JsonInputDlg::setText(const std::string &text, bool readonly) {
  if (_textEntry) {
    _textEntry->set_value(text);
    _textEntry->set_enabled(!readonly);
  }
}

//--------------------------------------------------------------------------------------------------

/**
* @brief Set JSON value.
*
* @json JSON to set.
*/
void JsonInputDlg::setJson(const JsonValue &json) {
  std::string text;
  JsonWriter::write(text, json);
  _textEditor->set_text(text.c_str());
}

//--------------------------------------------------------------------------------------------------

/**
  * @brief Validate json text.
  *
  */
void JsonInputDlg::validate() {
  auto text = _textEditor->get_text(false);
  if (text.empty())
    return;
  try {
    JsonParser::JsonValue value;
    JsonParser::JsonReader::read(text, value);
    _save->set_enabled(true);
    _validated = true;
    _value = value;
    _text = _textEditor->get_string_value();
  } catch (ParserException &ex) {
    mforms::Utilities::show_error("JSON check.", base::strfmt("Validation failed: '%s'", ex.what()), "Ok");
  }
}

//--------------------------------------------------------------------------------------------------

/**
  * @brief Content is edited
  *
  * @position The (byte) position in the text where the change happened.
  * @length The length of the change (in bytes).
  * @numberOfLines The number of lines which have been added (if positive) or removed (if negative).
  * @inserted True if text was inserted.
  */
void JsonInputDlg::editorContentChanged(int /*position*/, int /*length*/, int /*numberOfLines*/, bool /*inserted*/) {
  _save->set_enabled(false);
  _validated = false;
  _text = "";
  _value = JsonValue();
}

//--------------------------------------------------------------------------------------------------

JsonBaseView::JsonBaseView() : Panel(TransparentPanel) {
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
    std::locale(std::locale::classic(), new bt::time_input_facet("%Y-%m-%d"))};
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

/**
 * @brief Signal emitted when JSON data was modified.
 *
 * @return Signal funtion pointer.
 */
boost::signals2::signal<void(bool)> *JsonBaseView::dataChanged() {
  return &_dataChanged;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Clear control.
 *
 */
void JsonBaseView::clear() {
}

//--------------------------------------------------------------------------------------------------

JsonTreeBaseView::JsonTreeBaseView() : _useFilter(false), _searchIdx(0) {
  _contextMenu = mforms::manage(new mforms::ContextMenu());
  _contextMenu->signal_will_show()->connect(std::bind(&JsonTreeBaseView::prepareMenu, this));
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Setup context manu.
 *
 **/
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
      if (jv.getType() != VObject && jv.getType() != VArray)
        showAddModify = false;

      auto *item = mforms::manage(new mforms::MenuItem("Add new value"));
      item->set_name("add_new_doc");
      item->signal_clicked()->connect(std::bind(&JsonTreeBaseView::handleMenuCommand, this, item->get_name()));
      item->set_enabled(showAddModify);
      _contextMenu->add_item(item);

      item = mforms::manage(new mforms::MenuItem("Delete JSON"));
      item->set_name("delete_doc");
      item->signal_clicked()->connect(std::bind(&JsonTreeBaseView::handleMenuCommand, this, item->get_name()));
      _contextMenu->add_item(item);

      item = mforms::manage(new mforms::MenuItem("Modify JSON"));
      item->set_name("modify_doc");
      item->signal_clicked()->connect(std::bind(&JsonTreeBaseView::handleMenuCommand, this, item->get_name()));
      item->set_enabled(showAddModify);
      _contextMenu->add_item(item);
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Handle mennu command message.
 *
 * @command Command text.
 * @node Selected tree node.
 */
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
      jv.setDeleted(true);
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

/**
* @brief Handle mennu command message.
*
* @command Command text.
* @node Selected tree node.
*/
void JsonTreeBaseView::openInputJsonWindow(TreeNodeRef node, bool updateMode /*= false*/) {
  auto data = dynamic_cast<JsonValueNodeData *>(node->get_data());
  if (data != nullptr) {
    auto &jv = data->getData();
    bool isObject = jv.getType() == VObject;
    JsonInputDlg dlg(_treeView->get_parent_form(), isObject);
    if (updateMode) {
      if (isObject) {
        auto tag = node->get_tag();
        dlg.setText(tag, true);
      }
      dlg.setJson(jv);
    }
    if (dlg.run()) {
      auto value = dlg.data();
      auto objectName = dlg.objectName();
      switch (jv.getType()) {
        case VObject: {
          JsonObject &obj = (JsonObject &)jv;
          if (updateMode) {
            if (objectName.empty())
              jv = value;
            else
              obj[objectName] = value;
            node->remove_children();
          } else
            obj.insert(objectName, value);
          auto newNode = (updateMode) ? node : node->add_child();
          generateTree(objectName.empty() ? jv : obj[objectName], 0, newNode);
          newNode->set_string(0, objectName + "{" + std::to_string(obj.size()) + "}");
          newNode->set_tag(objectName);
          _dataChanged(false);
          break;
        }
        case VArray: {
          JsonArray &array = (JsonArray &)jv;
          if (updateMode) {
            array.clear();
            node->remove_children();
            if (value.getType() == VArray)
              array = (JsonArray)value;
            else
              array.pushBack(value);
          } else
            array.pushBack(value);
          size_t size = array.size();
          auto newNode = (updateMode) ? node : node->add_child();
          generateTree((updateMode) ? jv : array[size - 1], 0, newNode);
          newNode->set_string(0, objectName + "[" + std::to_string(array.size()) + "]");
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

/**
 * @brief Insert string value to the tree.
 *
 * @param value JsonValue to put in tree.
 * @param node Tree node reference.
 */
void JsonTreeBaseView::generateStringInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node) {
  const std::string &text = (std::string)value;
  setStringData(columnId, node, text);
  node->set_data(new JsonTreeBaseView::JsonValueNodeData(value));
  node->expand();
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Highlight matches in tree view.
 *
 * @param text Text to find.
 * @param backward Search backward.
 */
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

/**
 * @brief Find parents.
 *
 * @param node Tree node reference.
 * @param parents List of returned parents.
 */
void JsonTreeBaseView::collectParents(TreeNodeRef node, std::list<TreeNodeRef> &parents) {
  auto parent = node->get_parent();
  if (parent->is_valid()) {
    parents.push_back(parent);
    collectParents(parent, parents);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Re-create tree.
 *
 * @param value JSON value reference.
 */
void JsonTreeBaseView::reCreateTree(JsonValue &value) {
  _useFilter = false;
  _treeView->clear();
  auto node = _treeView->root_node()->add_child();
  _treeView->BeginUpdate();
  generateTree(value, 0, node);
  _treeView->EndUpdate();
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Filter tree view.
 *
 * @param text Text to find.
 * @param value JSON value reference.
 */
bool JsonTreeBaseView::filterView(const std::string &text, JsonParser::JsonValue &value) {
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

/**
 * @brief Fill tree control.
 *
 * @param value JsonValue to show in tree.
 * @param node Tree node reference.
 * @param addNew True if child node should be created.
 */
void JsonTreeBaseView::generateTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node, bool addNew) {
  if (value.isDeleted())
    return;
  switch (value.getType()) {
    case VDouble:
    case VInt64:
    case VUint64:
      generateNumberInTree(value, columnId, node);
      break;
    case VBoolean:
      generateBoolInTree(value, columnId, node);
      break;
    case VString:
      generateStringInTree(value, columnId, node);
      break;
    case VObject:
      generateObjectInTree(value, columnId, node, addNew);
      break;
    case VArray:
      generateArrayInTree(value, columnId, node);
      break;
    case VEmpty:
      generateNullInTree(value, columnId, node);
      break;
    default:
      break;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief callback to handle changed made by user to tree cells.
 *
 * @param node Tree node reference, which was edited.
 * @param column Column number which was edited.
 * @param value New string value to set.
 */
void JsonTreeBaseView::setCellValue(mforms::TreeNodeRef node, int column, const std::string &value) {
  auto data = dynamic_cast<JsonValueNodeData *>(node->get_data());
  bool setData = false;
  if (data != nullptr) {
    std::stringstream buffer;
    double number = 0;
    int64_t number2 = 0;
    uint64_t number3 = 0;
    bool retBool = false;
    auto &storedValue = data->getData();
    switch (storedValue.getType()) {
      case VDouble:
        if (!base::is_number(value))
          break;
        buffer << value;
        buffer >> number;
        storedValue = number;
        setData = true;
        break;
      case VInt64:
        if (!base::is_number(value))
          break;
        buffer << value;
        buffer >> number2;
        storedValue = number2;
        setData = true;
        break;
      case VUint64:
        if (!base::is_number(value))
          break;
        buffer << value;
        buffer >> number3;
        storedValue = number3;
        setData = true;
        break;
      case VBoolean:
        if (!base::isBool(value))
          break;
        buffer << value;
        buffer >> std::boolalpha >> retBool;
        storedValue = retBool;
        setData = true;
        break;
      case VString:
        storedValue = value;
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

JsonTextView::JsonTextView() : _textEditor(manage(new CodeEditor())), _modified(false), _position(0) {
  init();
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Fill text in control
 *
 * @param jsonText A string that contains the JSON text data to set.
 */
void JsonTextView::setText(const std::string &jsonText, bool validateJson /*= true*/) {
  _textEditor->set_value(jsonText.c_str());
  if (validateJson)
    validate();
  _text = jsonText;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Get Json.
 *
 * @return Json value const reference.
 */
const JsonValue &JsonTextView::getJson() const {
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

/**
 * @brief Clear control.
 *
 */
void JsonTextView::clear() {
  _textEditor->set_value("");
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Init controls in text tab control.
 *
 */
void JsonTextView::init() {
  assert(_textEditor != NULL);
  _textEditor->set_language(mforms::LanguageJson);
  _textEditor->set_features(mforms::FeatureWrapText, false);
  _textEditor->set_features(mforms::FeatureReadOnly, false);
  scoped_connect(_textEditor->signal_changed(), [this](int position, int length, int numberOfLines, bool inserted) {
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

/**
 * @brief Content is edited
 *
 * @position The (byte) position in the text where the change happened.
 * @length The length of the change (in bytes).
 * @numberOfLines The number of lines which have been added (if positive) or removed (if negative).
 * @inserted True if text was inserted.
 */
void JsonTextView::editorContentChanged(int position, int length, int numberOfLines, bool inserted) {
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

/**
 * @brief Signal emitted when the validate was clicked.
 */
bool JsonTextView::validate() {
  bool ret = true;

  if (_modified) {
    std::future<std::string> validateFuture = std::async(std::launch::async, [&, this]() -> std::string {
      try {
        JsonParser::JsonValue value;
        JsonParser::JsonReader::read(_text, value);
        _json = value;
      } catch (ParserException &ex) {
        return ex.what();
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
      _errorEntry.push_back(JsonErrorEntry{text, posBegin, posEnd - posBegin});
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

JsonTreeView::JsonTreeView() {
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

/**
 * @brief Init tree/grid view
 *
 * Based of readed JSON data control function initialize mforms control TreNodeView
 */
void JsonTreeView::init() {
  assert(_treeView != NULL);
  add(_treeView);
}

//--------------------------------------------------------------------------------------------------

JsonTreeView::~JsonTreeView() {
  _treeView->clear();
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Clear control.
 *
 */
void JsonTreeView::clear() {
  _treeView->clear();
  _viewFindResult.clear();
  _textToFind = "";
  _searchIdx = 0;
  _useFilter = false;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Add the JSON data to the control.
 *
 * @param value A JsonValue object to show in control.
 */
void JsonTreeView::setJson(JsonParser::JsonValue &value) {
  clear();
  auto node = _treeView->root_node()->add_child();
  generateTree(value, 0, node);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Append Json data to the control.
 *
 * @param value A JsonValue object to show in control.
 */
void JsonTreeView::appendJson(JsonParser::JsonValue &value) {
  TreeNodeRef node = _treeView->root_node();
  _viewFindResult.clear();
  _textToFind = "";
  _searchIdx = 0;
  generateTree(value, 0, node);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Insert object value to the tree
 *
 * @param value JsonValue to put in tree
 * @param node Tree node reference
 * @param addNew If true add as child node
 */
void JsonTreeView::generateObjectInTree(JsonParser::JsonValue &value, int /*columnId*/, TreeNodeRef node, bool addNew) {
  if (_useFilter && _filterGuard.count(&value) == 0)
    return;
  auto &object = (JsonObject &)value;
  size_t size = 0;
  auto end = object.end();
  node->set_data(new JsonTreeBaseView::JsonValueNodeData(value));
  for (JsonObject::Iterator it = object.begin(); it != end; ++it) {
    auto text = it->first;
    std::stringstream textSize;
    switch (it->second.getType()) {
      case VArray: {
        auto &arrayVal = (JsonArray &)it->second;
        size = arrayVal.size();
        node->set_tag(it->first);
        textSize << size;
        text += "[";
        text += textSize.str();
        text += "]";
        break;
      }
      case VObject: {
        auto &objectVal = (JsonObject &)it->second;
        size = objectVal.size();
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
    node2->set_tag(it->first);
    generateTree(it->second, 1, node2);
    node2->expand();
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Insert array value to the tree
 *
 * @param value JsonValue to put in tree
 * @param node Tree node reference
 * @param addNew If true add as child node
 */
void JsonTreeView::generateArrayInTree(JsonParser::JsonValue &value, int /*columnId*/, TreeNodeRef node) {
  if (_useFilter && _filterGuard.count(&value) == 0)
    return;

  auto &arrayType = (JsonArray &)value;

  node->set_icon_path(0, "JS_Datatype_Array.png");
  std::string name = node->get_string(0);
  if (name.empty())
    node->set_string(0, "<unnamed>");
  node->set_string(1, "");
  node->set_string(2, "Array");
  std::string tagName = node->get_tag();
  node->set_data(new JsonTreeBaseView::JsonValueNodeData(value));

  auto end = arrayType.end();
  int index = 0;
  for (auto it = arrayType.begin(); it != end; ++it, ++index) {
    if (_useFilter && _filterGuard.count(&*it) == 0)
      continue;
    auto arrrayNode = node->add_child();
    bool addNew = false;
    if (it->getType() == VArray || it->getType() == VObject)
      addNew = true;
    std::string keyName = tagName.empty() ? "key[%d]" : tagName + "[%d]";
    arrrayNode->set_string(0, base::strfmt(keyName.c_str(), index));
    arrrayNode->set_string(1, "");
    generateTree(*it, 1, arrrayNode, addNew);
  }
  node->expand();
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Insert bool value to the tree
 *
 * @param value JsonValue to put in tree
 * @param node Tree node reference
 */
void JsonTreeView::generateBoolInTree(JsonParser::JsonValue &value, int /*columnId*/, TreeNodeRef node) {
  node->set_icon_path(0, "JS_Datatype_Bool.png");
  node->set_attributes(1, mforms::TextAttributes("#4b4a4c", false, false));
  node->set_bool(1, (bool)value);
  node->set_string(2, "Boolean");
  node->set_data(new JsonTreeBaseView::JsonValueNodeData(value));
  node->expand();
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Insert double value to the tree
 *
 * @param value JsonValue to put in tree
 * @param node Tree node reference
 */
void JsonTreeView::generateNumberInTree(JsonParser::JsonValue &value, int /*columnId*/, TreeNodeRef node) {
  node->set_icon_path(0, "JS_Datatype_Number.png");
  node->set_attributes(1, mforms::TextAttributes("#4b4a4c", false, false));
  switch (value.getType()) {
    case VDouble:
      node->set_string(1, std::to_string((double)value));
      node->set_string(2, "Double");
      break;
    case VInt64:
      node->set_string(1, std::to_string((int64_t)value));
      node->set_string(2, "Long Integer");
      break;
    case VUint64:
      node->set_string(1, std::to_string((uint64_t)value));
      node->set_string(2, "Unsigned Long Integer");
      break;
    default:
      break;
  }
  node->set_data(new JsonTreeBaseView::JsonValueNodeData(value));
  node->expand();
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Insert null value to the tree
 *
 * @param node Tree node reference
 */
void JsonTreeView::generateNullInTree(JsonParser::JsonValue &value, int /*columnId*/, TreeNodeRef node) {
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

JsonGridView::JsonGridView()
  : _level(0), _headerAdded(false), _noNameColId(-1), _columnIndex(0), _rowNum(1), _actualParent(20) {
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
  _content->add(hbox, false, false);
  add(_content);
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::goUp() {
  if (_level <= 0 || _actualParent.empty())
    return;
  JsonParser::JsonValue *value = _actualParent.at(_level - 1);
  if (value == NULL)
    return;
  setJson(*value);
  _level--;
}

//--------------------------------------------------------------------------------------------------

JsonGridView::~JsonGridView() {
  _treeView->clear();
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Clear control.
 *
 */
void JsonGridView::clear() {
  _treeView->clear();
  _viewFindResult.clear();
  _textToFind = "";
  _searchIdx = 0;
  _useFilter = false;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Add the JSON data to the control.
 *
 * @param value A JsonValue object to show in control.
 */
void JsonGridView::setJson(JsonParser::JsonValue &value) {
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

void JsonGridView::reCreateTree(JsonParser::JsonValue &value) {
  remove(_content);
  init();
  _headerAdded = false;
  _colNameToColId.clear();
  setJson(value);
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::addColumn(int size, JsonParser::DataType type, const std::string &name) {
  switch (type) {
    case VArray:
    case VObject:
      _treeView->add_column(IconStringColumnType, name, size, false, true);
      break;
    case VInt64:
      _treeView->add_column(LongIntegerColumnType, name, size, true, true);
      break;
    case VUint64:
    case VDouble:
      _treeView->add_column(FloatColumnType, name, size, true, true);
      break;
    case VBoolean:
    case VString:
    case VEmpty:
    default:
      _treeView->add_column(StringColumnType, name, size, true, true);
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::generateColumnNames(JsonParser::JsonValue &value) {
  if (_level != 0)
    return;
  switch (value.getType()) {
    case VObject: {
      JsonObject &obj = (JsonObject &)value;
      JsonObjectIter end = obj.end();
      for (JsonObjectIter it = obj.begin(); it != end; ++it) {
        if (_colNameToColId.count(it->first) == 1)
          continue;
        addColumn(100, it->second.getType(), it->first);
        _colNameToColId[it->first] = _columnIndex++;
        if (it->second.getType() == VObject || it->second.getType() == VArray)
          generateColumnNames(it->second);
      }
      break;
    }
    case VArray: {
      JsonArray &jarray = (JsonArray &)value;

      JsonArrayIter end = jarray.end();
      for (JsonArrayIter it = jarray.begin(); it != end; ++it) {
        if (it->getType() == VObject) {
          JsonObject &obj = (JsonObject &)*it;
          JsonObjectIter end = obj.end();
          for (JsonObjectIter it = obj.begin(); it != end; ++it) {
            if (_colNameToColId.count(it->first) == 1)
              continue;
            addColumn(100, it->second.getType(), it->first);
            _colNameToColId[it->first] = _columnIndex++;
            if (it->second.getType() == VObject || it->second.getType() == VArray)
              generateColumnNames(it->second);
          }
        } else {
          if (_noNameColId > 0)
            continue;
          addColumn(100, VString, "");
          _noNameColId = _columnIndex++;
        }
        if (it->getType() == VObject || it->getType() == VArray)
          generateColumnNames(*it);
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

  std::string key;
  if (it != _colNameToColId.end())
    key = it->first;
  JsonParser::JsonValue &storedValue = (!key.empty() ? ((JsonObject &)data->getData())[key] : data->getData());
  if (data != NULL) {
    std::stringstream buffer;
    double number = 0;
    int64_t number2 = 0;
    uint64_t number3 = 0;
    bool retBool = false;
    switch (storedValue.getType()) {
      case VDouble:
        if (!base::is_number(value))
          break;
        buffer << value;
        buffer >> number;
        storedValue = number;
        node->set_float(column, number);
        _dataChanged(false);
      case VInt64:
        if (!base::is_number(value))
          break;
        buffer << value;
        buffer >> number2;
        storedValue = number2;
        node->set_long(column, number2);
        _dataChanged(false);
        break;
      case VUint64:
        if (!base::is_number(value))
          break;
        buffer << value;
        buffer >> number3;
        storedValue = number3;
        node->set_float(column, (double)number3);
        _dataChanged(false);
        break;
      case VBoolean:
        if (!base::isBool(value))
          break;
        buffer << value;
        buffer >> std::boolalpha >> retBool;
        storedValue = retBool;
        node->set_bool(column, retBool);
        _dataChanged(false);
        break;
      case VString:
        storedValue = value;
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

void JsonGridView::openInputJsonWindow(JsonParser::JsonValue &value) {
  JsonInputDlg dlg(_treeView->get_parent_form(), false);
  dlg.setJson(value);
  if (dlg.run()) {
    value = dlg.data();
    _actualParent[_level] = &value;
    reCreateTree(*_actualParent.at(0));
    setJson(*_actualParent.at(_level));
    _dataChanged(false);
  }
}

//--------------------------------------------------------------------------------------------------

void JsonGridView::handleMenuCommand(const std::string &command) {
  JsonParser::JsonValue *value = _actualParent.at(_level);
  if (value == NULL)
    return;
  if (command == "add_new_doc" || command == "modify_doc") {
    openInputJsonWindow(*value);
    return;
  }
  if (command == "delete_doc") {
    TreeNodeRef node = _treeView->get_selected_node();
    if (!node.is_valid())
      return;
    auto *data = dynamic_cast<JsonValueNodeData *>(node->get_data());
    if (data != nullptr) {
      JsonParser::JsonValue &jv = data->getData();
      jv.setDeleted(true);
      node->set_data(nullptr); // This will explicitly delete the data.
    }
    node->remove_from_parent();
    _dataChanged(false);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Append JSON data to the control.
 *
 * @param value A JsonValue object to show in control.
 */
void JsonGridView::appendJson(JsonParser::JsonValue & /*value*/) {
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Insert object value to the tree
 *
 * @param value JsonValue to put in tree
 * @param node Tree node reference
 * @param addNew If true add as child node
 */
void JsonGridView::generateObjectInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node, bool addNew) {
  if (value.isDeleted())
    return;
  auto child = node;
  if (addNew)
    child = node->add_child();
  auto &object = (JsonObject &)value;
  size_t size = 0;
  std::stringstream textSize;
  auto end = object.end();
  child->set_data(new JsonTreeBaseView::JsonValueNodeData(value));
  node->set_string(0, std::to_string(_rowNum++));
  for (auto it = object.begin(); it != end; ++it) {
    if (it->second.isDeleted())
      continue;
    std::string text = it->first;
    if (_colNameToColId.count(text) == 0)
      continue;
    int index = _colNameToColId[text];
    switch (it->second.getType()) {
      case VArray: {
        auto &arrayVal = (JsonArray &)it->second;
        size = arrayVal.size();
        textSize << size;
        text = "Array [";
        text += textSize.str();
        text += "]";
        child->set_icon_path(index, "JS_Datatype_Array.png");
        child->set_string(index, text);
        break;
      }
      case VObject: {
        auto &objectVal = (JsonObject &)it->second;
        size = objectVal.size();
        textSize << size;
        text = "Object {";
        text += textSize.str();
        text += "}";
        child->set_icon_path(index, "JS_Datatype_Object.png");
        child->set_string(index, text);
        break;
      }
      case VDouble:
      case VInt64:
      case VUint64:
        generateNumberInTree(it->second, index, child);
        break;
      case VBoolean:
        generateBoolInTree(it->second, index, child);
        break;
      case VString:
        setStringData(index, child, it->second);
        break;
      case VEmpty:
        generateNullInTree(it->second, index, child);
        break;
      default:
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Insert array value to the tree
 *
 * @param value JsonValue to put in tree
 * @param node Tree node reference
 * @param addNew If true add as child node
 */
void JsonGridView::generateArrayInTree(JsonParser::JsonValue &value, int /*columnId*/, TreeNodeRef /*node*/) {
  if (value.isDeleted())
    return;

  auto &arrayType = (JsonArray &)value;
  auto end = arrayType.end();
  for (auto it = arrayType.begin(); it != end; ++it) {
    if (it->isDeleted())
      return;
    mforms::TreeNodeRef arrrayNode = _treeView->root_node()->add_child();
    arrrayNode->set_string(0, std::to_string(_rowNum++));
    switch (it->getType()) {
      case VArray: {
        JsonArray &arrayVal = (JsonArray &)*it;
        size_t size = arrayVal.size();
        std::stringstream textSize;
        textSize << size;
        std::string text = "Array [";
        text += textSize.str();
        text += "]";
        arrrayNode->set_icon_path(_noNameColId, "JS_Datatype_Array.png");
        arrrayNode->set_string(_noNameColId, text);
        arrrayNode->set_data(new JsonTreeBaseView::JsonValueNodeData(*it));
        break;
      }
      case VObject:
        _rowNum--;
        generateObjectInTree(*it, 0, arrrayNode, false);
        break;
      case VDouble:
      case VInt64:
      case VUint64:
        generateNumberInTree(*it, _noNameColId, arrrayNode);
        arrrayNode->set_data(new JsonTreeBaseView::JsonValueNodeData(*it));
        break;
      case VBoolean:
        generateBoolInTree(*it, _noNameColId, arrrayNode);
        arrrayNode->set_data(new JsonTreeBaseView::JsonValueNodeData(*it));
        break;
      case VString:
        setStringData(_noNameColId, arrrayNode, *it);
        arrrayNode->set_data(new JsonTreeBaseView::JsonValueNodeData(*it));
        break;
      case VEmpty:
        generateNullInTree(*it, _noNameColId, arrrayNode);
        arrrayNode->set_data(new JsonTreeBaseView::JsonValueNodeData(*it));
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

    JsonParser::JsonValue &storedValue = data->getData();
    if (storedValue.getType() == VObject) {
      const std::map<std::string, int>::const_iterator it =
        std::find_if(_colNameToColId.begin(), _colNameToColId.end(),
                     [&column](const std::pair<std::string, int> &elem) { return column == elem.second; });

      if (it != _colNameToColId.end()) {
        JsonParser::JsonValue &clickedValue = ((JsonObject &)storedValue)[it->first];
        if (clickedValue.getType() != VObject && clickedValue.getType() != VArray)
          return;
        _level++;
        setJson(clickedValue);
        _goUpButton->set_enabled(true);
      }
    }
    if (storedValue.getType() == VArray) {
      _level++;
      setJson(storedValue);
      _goUpButton->set_enabled(true);
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Insert bool value to the tree
 *
 * @param value JsonValue to put in tree
 * @param node Tree node reference
 */
void JsonGridView::generateBoolInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node) {
  node->set_bool(columnId, (bool)value);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Insert double value to the tree
 *
 * @param value JsonValue to put in tree
 * @param node Tree node reference
 */
void JsonGridView::generateNumberInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node) {
  switch (value.getType()) {
    case VDouble:
      node->set_float(columnId, (double)value);
      break;
    case VInt64:
      node->set_long(columnId, (int64_t)value);
      break;
    case VUint64:
      node->set_long(columnId, (uint64_t)value);
      break;
    default:
      break;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Insert null value to the tree
 *
 * @param node Tree node reference
 */
void JsonGridView::generateNullInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node) {
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
  _tabView->set_name("json_editor:tab");
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
    _textView(manage(new JsonTextView())),
    _treeView(manage(new JsonTreeView())),
    _gridView(manage(new JsonGridView())),
    _tabView(manage(new TabView(tabLess ? TabViewTabless : TabViewPalette))),
    _updating(false),
    _defaultView(defaultView) {
  Setup();
}

//--------------------------------------------------------------------------------------------------

JsonTabView::~JsonTabView() {
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Add the JSON data to the control.
 *
 * @param value A JsonValue object that contains the JSON text data to set.
 */
void JsonTabView::setJson(const JsonParser::JsonValue &value) {
  _json = std::make_shared<JsonParser::JsonValue>(std::move(value));
  _ident = 0;
  _updating = true;
  JsonWriter::write(_jsonText, value);
  _updateView = {true, true, true};
  switch (_defaultView) {
    case JsonTabView::TabText:
      _textView->setText(_jsonText, false);
      _updateView.textViewUpdate = false;
      break;
    case JsonTabView::TabTree:
      _treeView->setJson(*_json);
      _updateView.treeViewUpdate = false;
      break;
    case JsonTabView::TabGrid:
      _gridView->setJson(*_json);
      _updateView.gridViewUpdate = false;
      break;
  }
  switchTab(_defaultView);
  _updating = false;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Set JSON text.
 *
 * @param text String to set in control.
 */
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
    _treeView->reCreateTree(*_json);
    _updateView.treeViewUpdate = false;
    _dataChanged(_jsonText);
  } else if (tabId == _tabId.gridViewTabId && _updateView.gridViewUpdate) {
    _gridView->reCreateTree(*_json);
    _updateView.gridViewUpdate = false;
    _dataChanged(_jsonText);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Signal emitted when the tab is switched by user.
 *
 */
void JsonTabView::dataChanged(bool forceUpdate) {
  if (_updating)
    return;
  int tabId = _tabView->get_active_tab();
  if (tabId != _tabId.textTabId)
    JsonWriter::write(_jsonText, *_json);
  else {
    if (_textView->validate()) {
      _jsonText = _textView->getText();
      _json.reset(new JsonValue(_textView->getJson()));
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

/**
 * @brief Clear control
 *
 */
void JsonTabView::clear() {
  _jsonText.clear();
  _textView->clear();
  _treeView->clear();
  _gridView->clear();
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Highlight match in control.
 *
 * text Text to find.
 */
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

/**
 * @brief Highlight next matches in JSON view.
 *
 */
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

/**
 * @brief Highlight next match in JSON view.
 *
 */
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

/**
 * @brief Highlight match in control.
 *
 * text Text to find.
 */
bool JsonTabView::filterView(const std::string &text) {
  int tabId = _tabView->get_active_tab();
  bool ret = false;
  if (tabId == _tabId.textTabId) {
    return false; // no filtering for text view
  } else if (tabId == _tabId.treeViewTabId) {
    ret = _treeView->filterView(text, *_json);
  } else if (tabId == _tabId.gridViewTabId) {
    ret = _gridView->filterView(text, *_json);
  }
  return ret;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Disable filtering.
 *
 * text Text to find.
 */
void JsonTabView::restoreOrginalResult() {
  int tabId = _tabView->get_active_tab();
  if (tabId == _tabId.textTabId) {
    return;
  } else if (tabId == _tabId.treeViewTabId) {
    _treeView->reCreateTree(*_json);
  } else if (tabId == _tabId.gridViewTabId) {
    _gridView->reCreateTree(*_json);
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

const JsonParser::JsonValue &JsonTabView::json() const {
  return *_json;
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
