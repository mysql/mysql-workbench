/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <mforms/box.h>
#include <mforms/app.h>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace mforms {
  class ToolBarItem;
  class ToolBar;

  enum ToolBarType
  {
    MainToolBar,
    SecondaryToolBar,
    ToolPickerToolBar, // only toggle and separator items are required to work
    OptionsToolBar,
    PaletteToolBar
  };
    
  enum ToolBarItemType
  {
    LabelItem,
    ActionItem,
    TextActionItem,
    ToggleItem,
    SegmentedToggleItem,
    SearchFieldItem,
    SelectorItem,
    ColorSelectorItem,
    SeparatorItem,
    ExpanderItem,
    ImageBoxItem,
#if !defined(_WIN32) && !defined(__APPLE__)
    TitleItem = LabelItem // TODO implement specific TitleItem (bold with #333333 color) for Linux
#else
    TitleItem
#endif
  };

#ifndef SWIG
  struct MFORMS_EXPORT ToolBarImplPtrs
  {
    bool (*create_tool_bar)(ToolBar *item, ToolBarType type);
    void (*insert_item)(ToolBar *toolbar, int index, ToolBarItem *item);
    void (*remove_item)(ToolBar *toolbar, ToolBarItem *item);

    bool (*create_tool_item)(ToolBarItem *item, ToolBarItemType type);
    void (*set_item_icon)(ToolBarItem *item, const std::string&);
    void (*set_item_alt_icon)(ToolBarItem *item, const std::string&);
    void (*set_item_text)(ToolBarItem *item, const std::string&);
    std::string (*get_item_text)(ToolBarItem *item);
    void (*set_item_enabled)(ToolBarItem *item, bool);
    bool (*get_item_enabled)(ToolBarItem *item);
    void (*set_item_checked)(ToolBarItem *item, bool);
    bool (*get_item_checked)(ToolBarItem *item);
    void (*set_item_tooltip)(ToolBarItem *item, const std::string&);

    // For selector items only.
    void (*set_selector_items)(ToolBarItem* item, const std::vector<std::string>& values);
  }; 
#endif

  // base abstract class for menuitem and menubase
  class MFORMS_EXPORT ToolBar : public Container
  {
  protected:    
    ToolBarImplPtrs *_impl;
    std::vector<ToolBarItem*> _items;
    ToolBarType _type;
    
  public:
    typedef boost::shared_ptr<ToolBar>  Ptr;
    ToolBar(ToolBarType type = MainToolBar);
    virtual ~ToolBar();
    
    ToolBarType get_type() { return _type; }
    
    std::vector<ToolBarItem*> &get_items() { return _items; }
    
    ToolBarItem *find_item(const std::string &name);
    
    void set_item_enabled(const std::string &name, bool flag);
    void set_item_checked(const std::string &name, bool flag);
    bool get_item_checked(const std::string &name);

    void add_item(ToolBarItem *item);
    void insert_item(int index, ToolBarItem *item);
    void remove_all();
    void remove_item(ToolBarItem *item);

    ToolBarItem *add_separator_item(const std::string &name="");

    void validate();
  };
  
  class MFORMS_EXPORT ToolBarItem : public Object
  {
  friend class ToolBar;
  public:
    ToolBarItem(ToolBarItemType type = ActionItem, const bool expandable = false);
    ToolBarItemType get_type() { return _type; }

    bool get_expandable() const { return _expandable; }

    // Caption for labels, selected entry in drop down items, search text in search fields.
    void set_text(const std::string &text);
    std::string get_text();

    void set_tooltip(const std::string &text);

    void set_icon(const std::string &path);
    std::string get_icon() { return _icon; };
    void set_alt_icon(const std::string &path);
    std::string get_alt_icon() { return _alt_icon; };

    void set_enabled(bool flag);
    bool get_enabled();

    void set_checked(bool flag);
    bool get_checked();

    boost::signals2::signal<void (ToolBarItem*)>* signal_activated() { return &_clicked_signal; }
    
    void set_name(const std::string &name);
    std::string get_name() { return _name; }

    void set_selector_items(const std::vector<std::string>& values);

    void set_validator(const boost::function<bool ()> &slot);
    void set_search_handler(const boost::function<void (const std::string&)> &slot);
    
  public:
    void callback();
    void validate();
    void search(const std::string&);
  private:
    ToolBarImplPtrs *_impl;
    std::string _name;
    std::string _icon;
    std::string _alt_icon;
    ToolBarItemType _type;
    bool _updating;
    const bool _expandable;
    boost::signals2::signal<void (ToolBarItem*)> _clicked_signal;
    boost::function<bool ()> _validate;
    boost::function<void (const std::string&)> _search;
  };
};
