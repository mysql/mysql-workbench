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

#pragma once

#include "base/jsonparser.h"
#include "mforms/form.h"
#include "mforms/panel.h"
#include "mforms/treeview.h"

#include <set>
#include <functional>

/**
 * @brief A Json view tab control with tree diffrent view text, tree and grid.
 *
 */
namespace mforms {
  /**
   * @brief Json view base class definition.
   */
  class JsonBaseView : public Panel {
  public:
    JsonBaseView();
    virtual ~JsonBaseView();
    void highlightMatch(const std::string &text);
    boost::signals2::signal<void(bool)> *dataChanged();

  protected:
    virtual void clear() = 0;
    boost::signals2::signal<void(bool)> _dataChanged;
    bool isDateTime(const std::string &text);
  };

  /**
  * @brief Dialog for adding JSON.
  */
  class CodeEditor;
  class TextEntry;
  class JsonInputDlg : public mforms::Form {
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
    CodeEditor *_textEditor;
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
  class JsonTextView : public JsonBaseView {
  public:
    JsonTextView();
    virtual ~JsonTextView();
    void setText(const std::string &jsonText, bool validateJson = true);
    virtual void clear();
    void findAndHighlightText(const std::string &text, bool backward = false);
    const JsonParser::JsonValue &getJson() const;
    const std::string &getText() const;
    bool validate();
    std::function<void()> _stopTextProcessing;
    std::function<void(std::function<bool()>)> _startTextProcessing;

  private:
    struct JsonErrorEntry {
      std::string text;
      std::size_t pos;
      std::size_t length;
    };
    void init();
    void editorContentChanged(int position, int length, int numberOfLines, bool inserted);
    void dwellEvent(bool started, size_t position, int x, int y);

    CodeEditor *_textEditor;
    bool _modified;
    std::string _text;
    int _position;
    JsonParser::JsonValue _json;
    std::vector<JsonErrorEntry> _errorEntry;
  };

  class JsonTreeBaseView : public JsonBaseView {
  public:
    typedef std::list<TreeNodeRef> TreeNodeList;
    typedef std::vector<TreeNodeRef> TreeNodeVactor;
    typedef std::map<std::string, TreeNodeVactor> TreeNodeVectorMap;
    struct JsonValueNodeData : public mforms::TreeNodeData {
      JsonValueNodeData(JsonParser::JsonValue &value) : _jsonValue(value) {
      }
      JsonParser::JsonValue &getData() {
        return _jsonValue;
      }
      ~JsonValueNodeData() {
      }

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
    void generateTree(JsonParser::JsonValue &value, int columnId, mforms::TreeNodeRef node, bool addNew = true);
    virtual void generateArrayInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node) = 0;
    virtual void generateObjectInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node, bool addNew) = 0;
    virtual void generateNumberInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node) = 0;
    virtual void generateBoolInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node) = 0;
    virtual void generateNullInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node) = 0;
    virtual void setStringData(int columnId, TreeNodeRef node, const std::string &text) = 0;

    void generateStringInTree(JsonParser::JsonValue &value, int idx, TreeNodeRef node);
    void collectParents(TreeNodeRef node, TreeNodeList &parents);
    static std::string getNodeIconPath(JsonNodeIcons icon);

    TreeNodeVectorMap _viewFindResult;
    std::set<JsonParser::JsonValue *> _filterGuard;
    bool _useFilter;
    std::string _textToFind;
    size_t _searchIdx;

    TreeView *_treeView;
    ContextMenu *_contextMenu;

  private:
    void prepareMenu();
    virtual void handleMenuCommand(const std::string &command);
    void openInputJsonWindow(TreeNodeRef node, bool updateMode = false);
  };

  /**
   * @brief Json grid view control class definition.
   */
  class JsonTreeView : public JsonTreeBaseView {
  public:
    JsonTreeView();
    virtual ~JsonTreeView();
    void setJson(JsonParser::JsonValue &val);
    void appendJson(JsonParser::JsonValue &val);
    virtual void clear();

  private:
    void init();
    virtual void generateArrayInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node);
    virtual void generateObjectInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node, bool addNew);
    virtual void generateNumberInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node);
    virtual void generateBoolInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node);
    virtual void generateNullInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node);
    virtual void setStringData(int columnId, TreeNodeRef node, const std::string &text);
  };

  /**
     * @brief Json grid view control class definition.
     */
  class JsonGridView : public JsonTreeBaseView {
  public:
    typedef JsonParser::JsonObject::Iterator JsonObjectIter;
    typedef JsonParser::JsonArray::Iterator JsonArrayIter;
    JsonGridView();
    virtual ~JsonGridView();
    void setJson(JsonParser::JsonValue &val);
    void appendJson(JsonParser::JsonValue &val);
    virtual void clear();
    void reCreateTree(JsonParser::JsonValue &value);

  private:
    void init();
    void generateColumnNames(JsonParser::JsonValue &value);
    void addColumn(int size, JsonParser::DataType type, const std::string &name);
    void nodeActivated(TreeNodeRef row, int column);
    void setCellValue(mforms::TreeNodeRef node, int column, const std::string &value);
    void goUp();

    virtual void generateArrayInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node);
    virtual void generateObjectInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node, bool addNew);
    virtual void generateNumberInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node);
    virtual void generateBoolInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node);
    virtual void generateNullInTree(JsonParser::JsonValue &value, int columnId, TreeNodeRef node);
    virtual void setStringData(int columnId, TreeNodeRef node, const std::string &text);

    virtual void handleMenuCommand(const std::string &command);
    void openInputJsonWindow(JsonParser::JsonValue &value);

    int _level;
    bool _headerAdded;
    int _noNameColId;
    int _columnIndex;
    int _rowNum;
    std::vector<JsonParser::JsonValue *> _actualParent;
    std::map<std::string, int> _colNameToColId;
    Button *_goUpButton;
    Box *_content;
  };

  /**
   * @brief Json tab view control class definition.
   */
  class TabView;
  class MFORMS_EXPORT JsonTabView : public Panel {
  public:
    typedef std::shared_ptr<JsonParser::JsonValue> JsonValuePtr;
    enum JsonTabViewType { TabText, TabTree, TabGrid };
    void Setup();
    JsonTabView(bool tabLess = false, JsonTabViewType defaultView = TabText);
    ~JsonTabView();

    void setJson(const JsonParser::JsonValue &val);
    void setText(const std::string &text, bool validate = true);
    void append2(const std::string &text);
    void tabChanged();
    void dataChanged(bool forceUpdate);
    void clear();
    void highlightMatch(const std::string &text);
    void highlightNextMatch();
    void highlightPreviousMatch();
    bool filterView(const std::string &text);
    void restoreOrginalResult();
    void switchTab(JsonTabViewType tab) const;
    JsonTabViewType getActiveTab() const;
    boost::signals2::signal<void(const std::string &text)> *editorDataChanged();
    const std::string &text() const;
    const JsonParser::JsonValue &json() const;

    void setTextProcessingStartHandler(std::function<void(std::function<bool()>)>);
    void setTextProcessingStopHandler(std::function<void()>);

  private:
    JsonTextView *_textView;
    JsonTreeView *_treeView;
    JsonGridView *_gridView;
    TabView *_tabView;
    std::string _jsonText;
    JsonValuePtr _json;
    int _ident;
    struct {
      int textTabId;
      int treeViewTabId;
      int gridViewTabId;
    } _tabId;
    struct {
      bool textViewUpdate;
      bool treeViewUpdate;
      bool gridViewUpdate;
    } _updateView;
    bool _updating;
    std::string _matchText;
    boost::signals2::signal<void(const std::string &text)> _dataChanged;
    JsonTabViewType _defaultView;
  };
};
