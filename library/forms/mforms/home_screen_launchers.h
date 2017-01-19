/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/drawbox.h"
#include "mforms/menu.h"
#include <string>
#include "base/geometry.h"
#include "home_screen_helpers.h"

namespace mforms {
  class HomeScreen;
  //----------------- LauncherEntry ---------------------------------------------------------------

  typedef std::function<bool()> LauncherCallback;
  class LauncherEntry : public mforms::Accessible {
  public:
    std::string title;
    std::string title_shorted;
    std::string description;
    std::vector<std::string> descriptionLines;
    base::any object;

    base::Rect bounds;

    cairo_surface_t *icon;

    bool operator<(const LauncherEntry &other) const;
    //------ Accessibility Methods -----
    virtual std::string get_acc_name();
    virtual std::string get_acc_description();

    virtual Accessible::Role get_acc_role();
    virtual base::Rect get_acc_bounds();
    virtual std::string get_acc_default_action();
    LauncherEntry();
    LauncherEntry(const LauncherEntry &other);

    LauncherEntry &operator=(LauncherEntry &&other);
    virtual ~LauncherEntry();
  };

  //----------------- LaunchersSection ---------------------------------------------------------------

  class MFORMS_EXPORT LaunchersSection : public HomeScreenSection {
  private:
    HomeScreen *_owner;
    ssize_t _entries_per_row;

    typedef std::vector<LauncherEntry>::iterator LauncherIterator;
    std::vector<LauncherEntry> _launchers;

    mforms::Menu *_context_menu;
    mforms::Menu *_action_menu;

    ssize_t _hot_entry;
    ssize_t _active_entry;

    std::function<bool(int, int)> _accessible_click_handler;
    base::Rect _use_default_button_rect;
    base::Rect _launcher_heading_rect;

  public:
    const int LAUNCHERS_LEFT_PADDING = 40;
    const int LAUNCHERS_RIGHT_PADDING = 40;
    const int LAUNCHERS_TOP_PADDING = 64;
    const int LAUNCHERS_VERTICAL_SPACING = 26;
    const int LAUNCHERS_SPACING = 20;

    const int LAUNCHERS_ENTRY_WIDTH = 250; // No spacing horizontally.
    const int LAUNCHERS_ENTRY_HEIGHT = 60;
    const int LAUNCHERS_HEADING_SPACING = 10; // Spacing between a heading part and a separator.
    const int LAUNCHERS_TOP_BASELINE = 40;    // Vertical space from top border to title base line.

    LaunchersSection(HomeScreen *owner);

    virtual ~LaunchersSection();

    std::size_t entry_from_point(int x, int y);

    void drawEntry(cairo_t *cr, const LauncherEntry &entry, bool hot);
    void layout(cairo_t *cr);
    virtual void cancelOperation();
    virtual void setFocus();
    virtual bool canHandle(HomeScreenMenuType type);
    virtual void setContextMenu(mforms::Menu *menu, HomeScreenMenuType type);
    virtual void setContextMenuAction(mforms::Menu *menu, HomeScreenMenuType type);

    void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah);

    void addLauncher(const std::string &icon, const std::string &name, const std::string &description,
                     const base::any &obj);
    void clearLaunchers();
    virtual bool mouse_double_click(mforms::MouseButton button, int x, int y);
    virtual bool mouse_click(mforms::MouseButton button, int x, int y);
    bool mouse_leave();
    virtual bool mouse_move(mforms::MouseButton button, int x, int y);
    void handle_command(const std::string &command);
    virtual int get_acc_child_count();
    virtual Accessible *get_acc_child(int index);
    virtual Accessible::Role get_acc_role();
    virtual mforms::Accessible *hit_test(int x, int y);
  };

} /* namespace wb */
