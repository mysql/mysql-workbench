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

#pragma once

#include "rapidjson/document.h"
#include "mforms/form.h"
#include "mforms/panel.h"
#include "mforms/treeview.h"

#include "Scintilla.h"

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
    JsonBaseView(rapidjson::Document &doc);
    virtual ~JsonBaseView();
    void highlightMatch(const std::string &text);
    boost::signals2::signal<void(bool)> *dataChanged();

  protected:
    virtual void clear() = 0;
    boost::signals2::signal<void(bool)> _dataChanged;
    bool isDateTime(const std::string &text);
    rapidjson::Document &_document;
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
    const rapidjson::Value &data() const;
    std::string objectName() const;
    void setText(const std::string &text, bool readonly);
    void setJson(const rapidjson::Value &json);
    bool run();

  private:
    rapidjson::Value _value;
    rapidjson::Document _document;
    std::string _text;
    CodeEditor *_textEditor;
    Button *_save;
    Button *_cancel;
    TextEntry *_textEntry;
    bool _validated;

    void setup(bool showTextEntry);
    void validate();
    void save();
    void editorContentChanged(Sci_Position position, Sci_Position length, Sci_Position numberOfLines, bool inserted);
  };

  /**
   * @brief Json text view control class definition.
   */
  class Label;
  class JsonTextView : public JsonBaseView {
  public:
    JsonTextView(rapidjson::Document &doc);
    virtual ~JsonTextView();
    void setText(const std::string &jsonText, bool validateJson = true);
    virtual void clear();
    void findAndHighlightText(const std::string &text, bool backward = false);
    const rapidjson::Value &getJson() const;
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
    void editorContentChanged(Sci_Position position, Sci_Position length, Sci_Position numberOfLines, bool inserted);
    void dwellEvent(bool started, size_t position, int x, int y);

    CodeEditor *_textEditor;
    bool _modified;
    std::string _text;
    Sci_Position _position;
    rapidjson::Value _json;
    std::vector<JsonErrorEntry> _errorEntry;
  };

  class JsonTreeBaseView : public JsonBaseView {
  public:
    typedef std::list<TreeNodeRef> TreeNodeList;
    typedef std::vector<TreeNodeRef> TreeNodeVactor;
    typedef std::map<std::string, TreeNodeVactor> TreeNodeVectorMap;
    struct JsonValueNodeData : public mforms::TreeNodeData {
      JsonValueNodeData(rapidjson::Value &value) : _jsonValue(value), type(value.GetType()) {
      }
      rapidjson::Value& getData() {
        return _jsonValue;
      }
      ~JsonValueNodeData() {
      }

    private:
      rapidjson::Value &_jsonValue;
      int type;
    };
    JsonTreeBaseView(rapidjson::Document &doc);
    virtual ~JsonTreeBaseView();
    enum JsonNodeIcons { JsonObjectIcon, JsonArrayIcon, JsonStringIcon, JsonNumericIcon, JsonNullIcon };
    void setCellValue(mforms::TreeNodeRef node, int column, const std::string &value);
    void highlightMatchNode(const std::string &text, bool bacward = false);
    bool filterView(const std::string &text, rapidjson::Value &value);
    void reCreateTree(rapidjson::Value &value);

  protected:
    void generateTree(rapidjson::Value &value, int columnId, mforms::TreeNodeRef node, bool addNew = true);
    virtual void generateArrayInTree(rapidjson::Value &value, int columnId, TreeNodeRef node) = 0;
    virtual void generateObjectInTree(rapidjson::Value &value, int columnId, TreeNodeRef node, bool addNew) = 0;
    virtual void generateNumberInTree(rapidjson::Value &value, int columnId, TreeNodeRef node) = 0;
    virtual void generateBoolInTree(rapidjson::Value &value, int columnId, TreeNodeRef node) = 0;
    virtual void generateNullInTree(rapidjson::Value &value, int columnId, TreeNodeRef node) = 0;
    virtual void setStringData(int columnId, TreeNodeRef node, const std::string &text) = 0;

    void generateStringInTree(rapidjson::Value &value, int idx, TreeNodeRef node);
    void collectParents(TreeNodeRef node, TreeNodeList &parents);
    static std::string getNodeIconPath(JsonNodeIcons icon);

    TreeNodeVectorMap _viewFindResult;
    std::set<rapidjson::Value *> _filterGuard;
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
    JsonTreeView(rapidjson::Document &doc);
    virtual ~JsonTreeView();
    void setJson(rapidjson::Value &val);
    void appendJson(rapidjson::Value &val);
    virtual void clear();

  private:
    void init();
    virtual void generateArrayInTree(rapidjson::Value &value, int columnId, TreeNodeRef node);
    virtual void generateObjectInTree(rapidjson::Value &value, int columnId, TreeNodeRef node, bool addNew);
    virtual void generateNumberInTree(rapidjson::Value &value, int columnId, TreeNodeRef node);
    virtual void generateBoolInTree(rapidjson::Value &value, int columnId, TreeNodeRef node);
    virtual void generateNullInTree(rapidjson::Value &value, int columnId, TreeNodeRef node);
    virtual void setStringData(int columnId, TreeNodeRef node, const std::string &text);
  };

  /**
   * @brief Json grid view control class definition.
   */
  class JsonGridView : public JsonTreeBaseView {
  public:
    JsonGridView(rapidjson::Document &doc);
    virtual ~JsonGridView();
    void setJson(rapidjson::Value &val);
    void appendJson(rapidjson::Value &val);
    virtual void clear();
    void reCreateTree(rapidjson::Value &value);

  private:
    void init();
    void generateColumnNames(rapidjson::Value &value);
    void addColumn(int size, rapidjson::Type type, rapidjson::Value *value, const std::string &name);
    void nodeActivated(TreeNodeRef row, int column);
    void setCellValue(mforms::TreeNodeRef node, int column, const std::string &value);
    void goUp();

    virtual void generateArrayInTree(rapidjson::Value &value, int columnId, TreeNodeRef node);
    virtual void generateObjectInTree(rapidjson::Value &value, int columnId, TreeNodeRef node, bool addNew);
    virtual void generateNumberInTree(rapidjson::Value &value, int columnId, TreeNodeRef node);
    virtual void generateBoolInTree(rapidjson::Value &value, int columnId, TreeNodeRef node);
    virtual void generateNullInTree(rapidjson::Value &value, int columnId, TreeNodeRef node);
    virtual void setStringData(int columnId, TreeNodeRef node, const std::string &text);

    virtual void handleMenuCommand(const std::string &command);
    void openInputJsonWindow(rapidjson::Value &value);

    int _level;
    bool _headerAdded;
    int _noNameColId;
    int _columnIndex;
    int _rowNum;
    std::vector<rapidjson::Value *> _actualParent;
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
    enum JsonTabViewType { TabText, TabTree, TabGrid };
    void Setup();
    JsonTabView(bool tabLess = false, JsonTabViewType defaultView = TabText);
    ~JsonTabView();

    void setJson(const rapidjson::Value &val);
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
    const rapidjson::Value &json() const;

    void setTextProcessingStartHandler(std::function<void(std::function<bool()>)>);
    void setTextProcessingStopHandler(std::function<void()>);

  private:
    JsonTextView *_textView;
    JsonTreeView *_treeView;
    JsonGridView *_gridView;
    TabView *_tabView;
    std::string _jsonText;
    rapidjson::Value _json;
    rapidjson::Document _document;
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
}; // namespace mforms
