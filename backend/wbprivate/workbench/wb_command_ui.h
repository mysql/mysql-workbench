/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_COMMAND_UI_H_
#define _WB_COMMAND_UI_H_

#include "wb_backend_public_interface.h"

#include "base/trackable.h"

#include "grt/grt_manager.h"
#include "grts/structs.app.h"
#include "mdc_events.h"

namespace mforms {
  class MenuBar;
  class MenuItem;
  class ToolBar;
  class ToolBarItem;
};

namespace wb {
  struct ParsedCommand;
  class WBContext;

  struct WBShortcut {
    std::string shortcut;
    mdc::KeyInfo key;
    mdc::EventState modifiers;

    std::string name;
    std::string command;
  };

  // Manager menus and toolbars.
  class MYSQLWBBACKEND_PUBLIC_FUNC CommandUI : public base::trackable {
    //    friend class WBContextUI;

    struct BuiltinCommand {
      std::function<void()> execute;
      std::function<bool()> validate;
    };

    WBContext *_wb;
    // dynamic UI stuff
    grt::ListRef<app_ShortcutItem> _shortcuts;

    std::map<std::string, BuiltinCommand> _builtin_commands;
    boost::signals2::signal<void()> _validate_edit_menu_items;
    bec::ArgumentPool _argpool;
    bool _include_se;

    bool validate_command_item(const app_CommandItemRef &item, const ParsedCommand &cmd);
    void update_item_state(const app_ToolbarItemRef &item, const ParsedCommand &cmd, mforms::ToolBarItem *tb_item);
    void update_item_state(const app_CommandItemRef &item, const ParsedCommand &cmd, mforms::MenuItem *menu_item);

    void append_shortcut_items(const grt::ListRef<app_ShortcutItem> &plist, const std::string &context,
                               std::vector<WBShortcut> *items);

    bool execute_builtin_command(const std::string &name);
    bool validate_builtin_command(const std::string &name);
    bool validate_plugin_command(app_PluginRef plugin);

  private:
    void add_recent_menu(mforms::MenuItem *parent);
    void add_plugins_menu_items(mforms::MenuItem *parent, const std::string &group);
    void add_plugins_menu(mforms::MenuItem *parent, const std::string &context);
    void add_menu_items_for_context(const std::string &context, mforms::MenuItem *parent, const app_MenuItemRef &menu);
    void add_scripts_menu(mforms::MenuItem *parent);

    void menu_will_show(mforms::MenuItem *parent);

  public:
    mforms::MenuBar *create_menubar_for_context(const std::string &context);

    void revalidate_menu_bar(mforms::MenuBar *menu);
    void revalidate_edit_menu_items();

  public:
    CommandUI(WBContext *wb);

    void clearBuildInCommands();

    mforms::ToolBar *create_toolbar(const std::string &toolbar_file);
    mforms::ToolBar *create_toolbar(const std::string &toolbar_file,
                                    const std::function<void(std::string)> &activate_slot);

    void load_data();

    void activate_command(const std::string &command);
    bool activate_command(const std::string &command, bec::ArgumentPool argpool);

    std::vector<WBShortcut> get_shortcuts_for_context(const std::string &context);

    void add_frontend_commands(const std::list<std::string> &commands);
    void remove_frontend_commands(const std::list<std::string> &commands);
    void add_builtin_command(const std::string &name, const std::function<void()> &slot,
                             const std::function<bool()> &validate = std::function<bool()>());
    void remove_builtin_command(const std::string &name);
  };
};

#endif /* _WB_COMMAND_UI_H_ */
