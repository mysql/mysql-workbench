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

#include "mforms/Panel.h"

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
  class MFORMS_EXPORT JsonView : public Panel
  {
  public:
    static JsonView* (*jsonViewFactory)(JsonViewType type);

    void setData(const std::string &text);
    const std::string &getData() const;
#ifndef SWIG
    static JsonView* createInstance(JsonViewType type = JsonTabControl);
    static void registerFactory(JsonView* (*create)(JsonViewType type));
    static bool __init;
    static bool initFactoryMethod();
#endif

  protected:
    JsonView();
    virtual ~JsonView();
    std::string _jsonText;

  private:
    virtual void setJson(const std::string &text) abstract;
    virtual const std::string &getJson() const abstract;
  };

  class CodeEditor;
  /// <summary>
  /// Json text view control class definition.
  /// <summary>
  class MFORMS_EXPORT JsonTextView : public JsonView
  {
  public:
    JsonTextView();
    virtual ~JsonTextView();
    void setData(const std::string &text);
    const std::string &getData() const;

  private:
    void init();
    std::shared_ptr<CodeEditor> _textEditor;
    virtual void setJson(const std::string& text) override;
    virtual const std::string &getJson() const override;
  };

  /// <summary>
  /// Json tree view control class definition.
  /// <summary>
  class TreeNodeView;
  class MFORMS_EXPORT JsonTreeView : public JsonView
  {
  public:
    JsonTreeView();
    virtual ~JsonTreeView();
    void setData(const std::string &text);
    const std::string &getData() const;

  private:
    virtual void setJson(const std::string& text) override;
    virtual const std::string& getJson() const override;
    std::shared_ptr<TreeNodeView> _treeView;
  };

  /// <summary>
  /// Json grid view control class definition.
  /// <summary>
  class MFORMS_EXPORT JsonGridView : public JsonView
  {
  public:
    JsonGridView();
    virtual ~JsonGridView();
    void setData(const std::string &text);
    const std::string &getData() const;

  private:
    virtual void setJson(const std::string& text) override;
    virtual const std::string &getJson() const override;
  };


  /// <summary>
  /// Json tab view control class definition.
  /// <summary>
  class TabView;
  class MFORMS_EXPORT JsonTabView : public JsonView
  {
  public:
    JsonTabView();
    virtual ~JsonTabView();
    void setData(const std::string &text);
    const std::string &getData() const;

  private:
    virtual void setJson(const std::string& text) override;
    virtual const std::string &getJson() const override;

    std::shared_ptr<JsonTextView> _textView;
    std::shared_ptr<JsonTreeView> _treeView;
    std::shared_ptr<JsonGridView> _gridView;
    std::shared_ptr<TabView> _tabView;
  };
};
