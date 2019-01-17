/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/accessibility.h"

#include "mforms/task_sidebar.h"
#include "mforms/drawbox.h"
#include "mforms/treeview.h"
#include "mforms/menubar.h"
#include "mforms/textentry.h"
#include "mforms/button.h"
#include "mforms/label.h"

#include "sqlide/wb_live_schema_tree.h"

#include "grt/grt_manager.h"

/**
 * Implementation of a sidebar in iTunes look and feel that can optionally show the content of a
 * database.
 */

namespace wb {

  class SimpleSidebar;
  class AdvancedSidebar;
  class SidebarSection;

  /**
    * The SidebarEntry class is a lean wrapper for an icon/label combination which can be used
    * to trigger an action. It needs a container to be useful (here the class SidebarSection).
    */
  class SidebarEntry : public base::Accessible {
  private:
    std::string _name;
    std::string _accessibilityName;
    std::string _title;
    cairo_surface_t* _icon;
    mforms::TaskEntryType _type;
    base::Rect _bounds;
    bool _enabled;
    boost::signals2::signal<void (const std::string &)> *_callback;

    SidebarSection *_owner;

  public:
    SidebarEntry(SidebarSection *owner, const std::string& name, const std::string& title,
                 const std::string& accessibilityName, const std::string& icon, mforms::TaskEntryType type,
                 boost::signals2::signal<void (const std::string &)> *callback);
    virtual ~SidebarEntry();

    void set_title(const std::string& title);
    void set_icon(const std::string& icon);
    void set_enabled(bool flag);

    void paint(cairo_t* cr, base::Rect bounds, bool hot, bool active, const base::Color& selection_color);
    bool contains(double x, double y);

    std::string title() {
      return _title;
    }
    std::string name() {
      return _name;
    }
    mforms::TaskEntryType type() {
      return _type;
    }
    bool enabled() const {
      return _enabled;
    }

    void execute() {
        (*_callback)(_name);
    }

    // ------ Accesibility Methods -----
    virtual std::string getAccessibilityDescription() override {
      return _accessibilityName;
    }

    virtual base::Accessible::Role getAccessibilityRole() override {
      return base::Accessible::PushButton;
    }

    virtual base::Rect getAccessibilityBounds() override {
      return _bounds;
    }

    virtual std::string getAccessibilityDefaultAction() override {
        return "click";
    }

    virtual void accessibilityDoDefaultAction() override {
        execute();
    }

  };

  class SidebarSection : public mforms::DrawBox {
    friend class SidebarEntry;
    
  private:
    struct Button : public base::Accessible {
      std::string _name;
      cairo_surface_t* icon;
      cairo_surface_t* alt_icon;
      std::string iconName, altIconName;
      int x, y;
      int bounds_width, bounds_height;
      base::Size size;
      bool hot;
      bool down;
      bool state;
      base::Rect bounds;

      Button(std::string const& name, std::string const& icon, std::string const& alt_icon);
      virtual ~Button();

      void draw(cairo_t* cr);
      void move(int x, int y);

      bool check_hit(ssize_t x, ssize_t y);

      // ------ Accesibility Methods -----
      virtual std::string getAccessibilityDescription() {
        return _name;
      }

      virtual base::Accessible::Role getAccessibilityRole() {
        return base::Accessible::PushButton;
      }

      virtual base::Rect getAccessibilityBounds() {
        return base::Rect(x, y, bounds_width, bounds_height);
      }
    };

    std::string _title;
    std::vector<Button*> _enabled_buttons;
    std::vector<SidebarEntry*> _entries;
    size_t _layout_width;
    size_t _layout_height;
    bool _expand_text_visible;
    int _expand_text_width;
    bool _expanded;
    bool _expand_text_active;
    bool _expandable;

    Button* _refresh_button;
    Button* _config_button;

    SidebarEntry* _selected_entry;
    SidebarEntry* _hot_entry;
    boost::signals2::signal<void(SidebarSection*)> _expanded_changed;

    cairo_t* _layout_context;
    cairo_surface_t* _layout_surface;
    double _last_width;

    SimpleSidebar *_owner;

  protected:
    void set_selected(SidebarEntry* entry);
    void create_context_for_layout();
    void layout(cairo_t* cr);
    SidebarEntry* entry_from_point(double x, double y);

  public:
    SidebarSection(SimpleSidebar* owner, const std::string& title, mforms::TaskSectionFlags flags);
    ~SidebarSection();

    int find_entry(const std::string& name);

    int add_entry(const std::string& name, const std::string& accessibilityName, const std::string& title,
                  const std::string& icon, mforms::TaskEntryType type);
    void set_entry_text(int index, const std::string& title);
    void set_entry_icon(int index, const std::string& icon);
    void set_entry_enabled(int index, bool enabled);
    void mark_busy(bool busy);
    void remove_entry(const std::string& entry);
    int entry_count() {
      return (int)_entries.size();
    }

    void clear();
    bool select(const std::string& title);

    SidebarEntry* selected() {
      return _selected_entry;
    }
    bool expanded() {
      return _expanded;
    }
    void toggle_expand();

    std::string title() {
      return _title;
    }

    virtual void repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) override;
    virtual bool mouse_leave() override;
    virtual bool mouse_move(mforms::MouseButton button, int x, int y) override;
    virtual bool mouse_down(mforms::MouseButton button, int x, int y) override;
    virtual bool mouse_click(mforms::MouseButton button, int x, int y) override;
    virtual bool mouse_up(mforms::MouseButton button, int x, int y) override;
    virtual base::Size getLayoutSize(base::Size proposedSize) override;

    virtual std::string getAccessibilityDescription() override {
      return _title;
    }

    virtual base::Accessible::Role getAccessibilityRole() override {
      return base::Accessible::OutlineItem;
    }

    virtual size_t getAccessibilityChildCount() override;
    virtual Accessible* getAccessibilityChild(size_t index) override;
    virtual base::Accessible* accessibilityHitTest(ssize_t x, ssize_t y) override;

    void clear_selection();

    boost::signals2::signal<void(SidebarSection*)>* expanded_changed() {
      return &_expanded_changed;
    }
  };

  class SimpleSidebar : public mforms::TaskSidebar, public base::Observer {
    friend class SidebarSection;
    friend class SidebarEntry;
    
  protected:
    std::vector<SidebarSection*> _sections;

    base::Color _activeTextColor;
    base::Color _inactiveTextColor;
    base::Color _selection_color;

    static mforms::TaskSidebar* create_instance();
    SimpleSidebar(); // Create the sidebar via its mforms alter ego TaskSidebar::create()

    virtual void updateColors();
    
    int find_section(const std::string& title);
    void handle_notification(const std::string& name, void* sender, base::NotificationInfo& info) override;
    void add_items_from_list(mforms::MenuBase& menu, const bec::MenuItemList& items);

  public:
    ~SimpleSidebar();

    virtual int add_section(const std::string& name, const std::string& accessibilityName, const std::string& title,
                            mforms::TaskSectionFlags flags = mforms::TaskSectionPlain) override;
    virtual void remove_section(const std::string& name) override;
    virtual int add_section_entry(const std::string& section_name, const std::string& name, const std::string& accessibilityName,
                                  const std::string& title, const std::string& icon, mforms::TaskEntryType type) override;
    virtual void set_section_entry_text(const std::string& entry_name, const std::string& title) override;
    virtual void set_section_entry_icon(const std::string& entry_name, const std::string& icon) override;
    virtual void set_section_entry_enabled(const std::string& entry_name, bool flag) override;
    virtual void mark_section_busy(const std::string& section_name, bool busy) override;

    virtual void remove_section_entry(const std::string& entry_name) override;
    virtual void set_collapse_states(const std::string& data) override;
    virtual std::string get_collapse_states() override;

    virtual void clear_sections() override;
    virtual void clear_section(const std::string& section_name) override;
    virtual void set_selection_color(const std::string& color) override;
    virtual void set_selection_color(const base::SystemColor color) override;
    const base::Color& selection_color() const {
      return _selection_color;
    }

    virtual int select_entry(const std::string& entry_name) override;
    virtual std::string selected_entry() override;
    virtual void clear_selection() override;

  private:
    static bool __init;
    static bool init_factory_method();
  };

  class AdvancedSidebar : public SimpleSidebar {
  private:
    typedef boost::signals2::signal<void(const std::string&)> SearchBoxChangedSignal;
    typedef boost::signals2::signal<void(void)> TreeNodeSelected;
    mforms::TreeView _new_schema_tree;
    mforms::TreeView _filtered_schema_tree;
    mforms::ContextMenu _tree_context_menu;
    mforms::Box _schema_search_box;
    mforms::TextEntry _schema_search_text;
    bool _remote_search_enabled;
    mforms::Label _schema_search_warning;
    mforms::Button _remote_search;
    wb::LiveSchemaTree* _schema_model;
    wb::LiveSchemaTree* _base_model;
    wb::LiveSchemaTree* _filtered_schema_model;
    boost::signals2::scoped_connection _activate_conn;
    SearchBoxChangedSignal _search_box_changed_signal;
    TreeNodeSelected _tree_node_selected;
    bool _is_model_owner; // True if the sidebar is the owner of the model.

    mforms::Box _schema_box; // Container for schema section and tree so we can hide it as a block.

    bec::GRTManager::Timer* _filterTimer;

    AdvancedSidebar(); // Create the sidebar via its mforms alter ego TaskSidebar::create()
  protected:
    static mforms::TaskSidebar* create_instance();

    void setup_schema_tree();
    virtual void updateColors() override;
    void on_show_menu(mforms::MenuItem* parent_item);
    void add_items_from_list(mforms::MenuBase& menu, const bec::MenuItemList& items);
    void handle_menu_command(const std::string& command);
    void on_search_text_changed_prepare();
    bool on_search_text_changed();
    void on_remote_search_clicked();
    void on_tree_node_selected();

  public:
    virtual ~AdvancedSidebar();

    virtual void mark_section_busy(const std::string& section_name, bool busy) override;

    virtual mforms::TreeView* get_schema_tree() override {
      return (_schema_model == _base_model) ? &_new_schema_tree : &_filtered_schema_tree;
    }
    virtual mforms::TextEntry* get_filter_entry() override {
      return &_schema_search_text;
    }
    void tool_action_clicked(const std::string& action);
    virtual void set_schema_model(wb::LiveSchemaTree* model) override;
    virtual void set_filtered_schema_model(wb::LiveSchemaTree* model) override;
    virtual void enable_server_search(bool enabled) override {
      _remote_search_enabled = enabled;
    }

    virtual boost::signals2::signal<void(const std::string&)>* signal_filter_changed() override {
      return &_search_box_changed_signal;
    }
    virtual boost::signals2::signal<void(void)>* tree_node_selected() override {
      return &_tree_node_selected;
    };

    virtual void expand_schema(int schema_index) override;

    virtual mforms::ContextMenu* get_context_menu() override {
      return &_tree_context_menu;
    }

  private:
    static bool __init;
    static bool init_factory_method();
  };

} // namespace wb
