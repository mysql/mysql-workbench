/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "home_screen.h"

#include "mforms/box.h"
#include "mforms/drawbox.h"
#include "mforms/textentry.h"
#include "home_screen_helpers.h"

namespace mforms {
  class Menu;
  class ConnectionEntry;
  class FolderBackEntry;
  class FolderEntry;

  class MFORMS_EXPORT ConnectionsWelcomeScreen : public mforms::DrawBox {
  public:
    ConnectionsWelcomeScreen(HomeScreen *owner);
    virtual ~ConnectionsWelcomeScreen();

    virtual base::Size getLayoutSize(base::Size proposedSize) override;
    
    virtual base::Accessible::Role getAccessibilityRole() override;
    virtual std::string getAccessibilityTitle() override;
    virtual std::string getAccessibilityDescription() override;
    virtual std::string getAccessibilityValue() override;
    virtual size_t getAccessibilityChildCount() override;
    virtual Accessible* getAccessibilityChild(size_t index) override;
    virtual base::Rect getAccessibilityBounds() override;
    virtual Accessible* accessibilityHitTest(ssize_t x, ssize_t y) override;

    void updateColors();
    void updateIcons();

  private:
    int _totalHeight = 100; // Arbitrary initial value, til our computation is done.

    HomeScreen *_owner;
    HomeAccessibleButton _closeHomeScreenButton;
    HomeAccessibleButton _browseDocButton;
    HomeAccessibleButton _readBlogButton;
    HomeAccessibleButton _discussButton;

    cairo_surface_t *_closeIcon;

    base::Color _textColor;

    std::string _heading;
    std::vector<std::string> _content;
    
    virtual void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) override;
    virtual bool mouse_click(mforms::MouseButton button, int x, int y) override;
  };

  class MFORMS_EXPORT ConnectionsSection : public HomeScreenSection, public mforms::DropDelegate {
    friend class ConnectionEntry;
    friend class FolderBackEntry;
    friend class FolderEntry;

  private:
    HomeScreen *_owner;

    cairo_surface_t *_folder_icon;
    cairo_surface_t *_network_icon;
    cairo_surface_t *_plus_icon;
    cairo_surface_t *_sakila_icon;
    cairo_surface_t *_user_icon;
    cairo_surface_t *_manage_icon;

    base::Color _titleColor;
    base::Color _folderTitleColor;
    base::Color _backgroundColor;
    base::Color _backgroundColorHot;
    base::Color _folderBackgroundColor;
    base::Color _folderBackgroundColorHot;
    base::Color _backTileBackgroundColor;
    base::Color _backTileBackgroundColorHot;

    std::shared_ptr<FolderEntry> _active_folder; // The folder entry that is currently active.
    std::string _active_folder_title_before_refresh_start;

    typedef std::vector<std::shared_ptr<ConnectionEntry> > ConnectionVector;
    typedef ConnectionVector::iterator ConnectionIterator;
    ConnectionVector _connections;
    ConnectionVector _filtered_connections;
    bool _filtered;

    mforms::Menu *_connection_context_menu;
    mforms::Menu *_folder_context_menu;
    mforms::Menu *_generic_context_menu;

    std::shared_ptr<ConnectionEntry> _hot_entry;      // The connection entry under the mouse.
    std::shared_ptr<ConnectionEntry> _entry_for_menu; // The entry that was hot when the context menu was opened.

    ssize_t _drag_index; // The index of the entry that is being dragged.
    ssize_t _drop_index; // The index of the entry that is currently the drop target.
    mforms::DropPosition _drop_position;

    HomeAccessibleButton _add_button;
    HomeAccessibleButton _manage_button;
    HomeAccessibleButton _rescanButton;

    mforms::Box _search_box;
    mforms::TextEntry _search_text;

    base::Rect _mouse_down_position; // Used to determine if the user starts a drag/drop operation.

    bool _showWelcomeHeading;

    ConnectionsWelcomeScreen *_welcomeScreen;
    mforms::Box *_container;

    ConnectionVector const& displayed_connections() const;

    virtual void updateColors() override;
    virtual void updateIcons() override;

    void on_search_text_changed();
    void on_search_text_action(mforms::TextEntryAction action);
    ssize_t calculate_index_from_point(int x, int y);
    std::shared_ptr<ConnectionEntry> entry_from_point(int x, int y) const;
    std::shared_ptr<ConnectionEntry> entry_from_index(ssize_t index) const;
    base::Rect bounds_for_entry(size_t index, size_t width);
    std::string connectionIdFromIndex(ssize_t index);

    void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) override;

    virtual bool mouse_down(mforms::MouseButton button, int x, int y) override;
    virtual bool mouse_up(mforms::MouseButton button, int x, int y) override;
    virtual bool mouse_double_click(mforms::MouseButton button, int x, int y) override;
    virtual bool mouse_click(mforms::MouseButton button, int x, int y) override;
    virtual bool mouse_leave() override;
    virtual bool mouse_move(mforms::MouseButton button, int x, int y) override;

    void handle_command(const std::string &command);
    void handle_folder_command(const std::string &command);

    void menu_open();

    void change_to_folder(std::shared_ptr<FolderEntry> folder);

    virtual std::string getAccessibilityTitle() override;
    virtual size_t getAccessibilityChildCount() override;
    virtual Accessible* getAccessibilityChild(size_t index) override;
    virtual Accessible::Role getAccessibilityRole() override;

    virtual base::Accessible* accessibilityHitTest(ssize_t x, ssize_t y) override;
    bool do_tile_drag(ssize_t index, int x, int y);

    mforms::DragOperation drag_over(View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                    const std::vector<std::string> &formats) override;
    mforms::DragOperation files_dropped(View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                        const std::vector<std::string> &file_names) override;
    mforms::DragOperation data_dropped(mforms::View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                       void *data, const std::string &format) override;

  public:
    static const int CONNECTIONS_LEFT_PADDING = 20;
    static const int CONNECTIONS_RIGHT_PADDING = 20;  // The tile spacing right to the last tile in the row does not
                                                      // belong to this padding.
    static const int CONNECTIONS_TOP_PADDING = 75;    // The vertical offset of the first visible shortcut entry.
    static const int CONNECTIONS_BOTTOM_PADDING = 20; // The vertical distance after the last tile line.
    static const int CONNECTIONS_SPACING = 9;         // Vertical/horizontal space between entries.

    static const int CONNECTIONS_TILE_WIDTH = 241;
    static const int CONNECTIONS_TILE_HEIGHT = 91;

    ConnectionsSection(HomeScreen *owner);
    ~ConnectionsSection();

    void clear_connections(bool clear_state = true);
    void focus_search_box();
    void showWelcomeHeading(bool state = true);

    virtual base::Size getLayoutSize(base::Size proposedSize) override;
    virtual const char* getTitle() override;
    virtual void cancelOperation() override;
    virtual void setFocus() override;
    virtual bool canHandle(HomeScreenMenuType type) override;
    virtual void setContextMenu(mforms::Menu *menu, HomeScreenMenuType type) override;
    virtual void setContextMenuAction(mforms::Menu *menu, HomeScreenMenuType type) override;

    std::function<anyMap(const std::string &)> getConnectionInfoCallback;

    void addConnection(const std::string &connectionId, const std::string &title, const std::string &description,
                       const std::string &user, const std::string &schema);

    void updateFocusableAreas();
    bool setFocusOnEntry(ConnectionEntry const* entry);

    virtual mforms::View *getContainer() override;
    virtual mforms::View *get_parent() const override;
  };
}
