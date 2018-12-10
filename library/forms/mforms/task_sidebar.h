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

/**
 * mforms interface to the advanced sidebar, which implements an iTunes like interface.
 */
#include <mforms/box.h>
#include <mforms/app.h>

#include <vector>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace wb {
  class LiveSchemaTree; // The tree model for the schema treeview.
};

namespace mforms {

  class ContextMenu;
  class TreeView;
  class TaskSidebar;
  class TextEntry;

  enum TaskSectionFlags {
    TaskSectionPlain = 0,
    TaskSectionRefreshable = 1,
    TaskSectionCollapsible = 2,
    TaskSectionShowConfigButton = 0x10,
  };

#ifndef SWIG
  inline TaskSectionFlags operator|(TaskSectionFlags a, TaskSectionFlags b) {
    return (TaskSectionFlags)((int)a | (int)b);
  }
#endif

  enum TaskEntryType {
    TaskEntrySelectableItem,
    TaskEntryLink,
    TaskEntryAlwaysActiveLink, // same as TaskEntryLink, but will send action even if disabled
    TaskEntryPlainItem
  };

  class MFORMS_EXPORT TaskSidebar : public Box {
  private:
    boost::signals2::signal<void(const std::string&)> _on_section_command;

  protected:
    TaskSidebar();

  public:
    static TaskSidebar* create(const std::string& type); // Create an instance from the factory

  public:
    virtual int add_section(const std::string& name, const std::string& accessbilityName,
                            const std::string& title, TaskSectionFlags flags = TaskSectionPlain) = 0;
    virtual void remove_section(const std::string& name) = 0;
    virtual int add_section_entry(const std::string& section_name, const std::string& entry_name, const std::string& accessibilityName,
                                  const std::string& title, const std::string& icon, TaskEntryType type) = 0;
    virtual void set_section_entry_text(const std::string& entry_name, const std::string& title) = 0;
    virtual void set_section_entry_icon(const std::string& entry_name, const std::string& icon) = 0;
    virtual void set_section_entry_enabled(const std::string& entry_name, bool flag) = 0;
    virtual void mark_section_busy(const std::string& section_name, bool busy) = 0;
    virtual void remove_section_entry(const std::string& entry_name) = 0;

    virtual void set_collapse_states(const std::string& data) = 0;
    virtual std::string get_collapse_states() = 0;

    virtual void clear_sections() = 0;
    virtual void clear_section(const std::string& section_name) = 0;
    virtual void set_selection_color(const std::string& color) = 0;
    virtual void set_selection_color(const base::SystemColor color) = 0;

    virtual int select_entry(const std::string& entry_name) = 0;
    virtual std::string selected_entry() = 0;
    virtual void clear_selection() = 0;

    boost::signals2::signal<void(const std::string&)>* on_section_command() {
      return &_on_section_command;
    };

  public: // Only for advanced sidebar
    virtual void set_schema_model(wb::LiveSchemaTree* model) {
    }
    virtual void set_filtered_schema_model(wb::LiveSchemaTree* model) {
    }
    virtual void enable_server_search(bool enabled) {
    }

    virtual TreeView* get_schema_tree() {
      return NULL;
    }
    virtual TextEntry* get_filter_entry() {
      return NULL;
    }

    virtual void expand_schema(int schema_index) {
    }

    virtual boost::signals2::signal<void(const std::string&)>* signal_filter_changed() {
      return NULL;
    }
    virtual boost::signals2::signal<void(void)>* tree_node_selected() {
      return NULL;
    }

    virtual mforms::ContextMenu* get_context_menu() {
      return NULL;
    }

#ifndef SWIG
    static void register_factory(const std::string& type, TaskSidebar* (*create)());
#endif
  private:
  };
}

#endif // !DOXYGEN_SHOULD_SKIP_THIS
