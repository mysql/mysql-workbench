/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <mforms/box.h>
#include <mforms/app.h>
#include <vector>

namespace mforms {
  class ToolBarItem;
  class ToolBar;

  enum ToolBarType {
    MainToolBar,       // The application toolbar.
    SecondaryToolBar,  // Any other toolbar not handled by the special variants below (e.g. on sidebars).
    ToolPickerToolBar, // The special vertical toolpicker toolbar for modeling.
    OptionsToolBar,    // For parts which involve a light UI (e.g. toolbar over SQL editors + result sets).
    PaletteToolBar     // TODO: not sure why we need that. A toolbar that doesn't differ from SecondaryToolBar.
  };

  enum ToolBarItemType {
    LabelItem,
    ActionItem,
    TextActionItem,
    TextEntryItem,
    ToggleItem,
    SegmentedToggleItem,
    SearchFieldItem,
    SelectorItem,
    FlatSelectorItem,
    ColorSelectorItem,
    SeparatorItem,
    ExpanderItem,
    ImageBoxItem,
    TitleItem,
  };

#ifndef SWIG
  struct MFORMS_EXPORT ToolBarImplPtrs {
    bool (*create_tool_bar)(ToolBar *item, ToolBarType type);
    void (*insert_item)(ToolBar *toolbar, int index, ToolBarItem *item);
    void (*remove_item)(ToolBar *toolbar, ToolBarItem *item);

    bool (*create_tool_item)(ToolBarItem *item, ToolBarItemType type);
    void (*set_item_icon)(ToolBarItem *item, const std::string &);
    void (*set_item_alt_icon)(ToolBarItem *item, const std::string &);
    void (*set_item_text)(ToolBarItem *item, const std::string &);
    // XXX: implement on Windows and Mac
    std::string (*get_item_text)(ToolBarItem *item);
    void (*set_item_name)(ToolBarItem *item, const std::string &);
    void (*set_item_enabled)(ToolBarItem *item, bool);
    bool (*get_item_enabled)(ToolBarItem *item);
    void (*set_item_checked)(ToolBarItem *item, bool);
    bool (*get_item_checked)(ToolBarItem *item);
    void (*set_item_tooltip)(ToolBarItem *item, const std::string &);

    // For selector items only.
    void (*set_selector_items)(ToolBarItem *item, const std::vector<std::string> &values);
  };
#endif

  // base abstract class for menuitem and menubase
  class MFORMS_EXPORT ToolBar : public Container {
  protected:
    ToolBarImplPtrs *_impl;
    std::vector<ToolBarItem *> _items;
    ToolBarType _type;

  public:
    typedef std::shared_ptr<ToolBar> Ptr;
    ToolBar(ToolBarType type = MainToolBar);
    virtual ~ToolBar();

    ToolBarType get_type() {
      return _type;
    }

    std::vector<ToolBarItem *> &get_items() {
      return _items;
    }

    ToolBarItem *find_item(const std::string &name);

    void set_item_enabled(const std::string &name, bool flag);
    void set_item_checked(const std::string &name, bool flag);
    bool get_item_checked(const std::string &name);

    void add_item(ToolBarItem *item);
    void insert_item(int index, ToolBarItem *item);
    void remove_all();
    void remove_item(ToolBarItem *item);

    ToolBarItem *add_separator_item(const std::string &name = "");

    void validate();
  };

  class MFORMS_EXPORT ToolBarItem : public Object {
    friend class ToolBar;

  public:
    ToolBarItem(ToolBarItemType type = ActionItem);
    ToolBarItemType get_type() {
      return _type;
    }

    // Caption for labels, selected entry in drop down items, search text in search fields.
    void set_text(const std::string &text);
    std::string get_text();

    void set_tooltip(const std::string &text);

    void set_icon(const std::string &path);
    std::string get_icon() {
      return _icon;
    }
    void set_alt_icon(const std::string &path);
    std::string get_alt_icon() {
      return _alt_icon;
    }

    void set_enabled(bool flag);
    bool get_enabled();

    void set_checked(bool flag);
    bool get_checked();

    boost::signals2::signal<void(ToolBarItem *)> *signal_activated() {
      return &_clicked_signal;
    }

    void setInternalName(const std::string &name) {
        _internalName = name;
    }
    std::string getInternalName() const {
      return _internalName;
    }
    void set_name(const std::string &name);

    void set_selector_items(const std::vector<std::string> &values);

    void set_validator(const std::function<bool()> &slot);
    void set_search_handler(const std::function<void(const std::string &)> &slot);

  public:
    void callback();
    void validate();
    void search(const std::string &);

  private:
    ToolBarImplPtrs *_impl;
    std::string _internalName;
    std::string _icon;
    std::string _alt_icon;
    ToolBarItemType _type;
    bool _updating;
    boost::signals2::signal<void(ToolBarItem *)> _clicked_signal;
    std::function<bool()> _validate;
    std::function<void(const std::string &)> _search;
  };
}
