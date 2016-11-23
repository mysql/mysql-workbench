/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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
  class ConnectionInfoPopup;

  class MFORMS_EXPORT ConnectionsSection : public HomeScreenSection, public mforms::DropDelegate {
    friend class ConnectionEntry;
    friend class FolderBackEntry;
    friend class FolderEntry;
    friend class ConnectionInfoPopup;

  private:
    HomeScreen *_owner;

    cairo_surface_t *_folder_icon;
    cairo_surface_t *_mouse_over_icon;
    cairo_surface_t *_mouse_over2_icon;
    cairo_surface_t *_network_icon;
    cairo_surface_t *_ha_filter_icon;
    cairo_surface_t *_plus_icon;
    cairo_surface_t *_sakila_icon;
    cairo_surface_t *_schema_icon;
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
    bool _show_details; // If there's a hot connection this indicates if we just show the hot state or the connection
                        // details.

    ssize_t _drag_index; // The index of the entry that is being dragged.
    ssize_t _drop_index; // The index of the entry that is currently the drop target.
    mforms::DropPosition _drop_position;

    HomeAccessibleButton _add_button;
    HomeAccessibleButton _manage_button;
    HomeAccessibleButton _browseDocButton;
    HomeAccessibleButton _readBlogButton;
    HomeAccessibleButton _discussButton;


    base::Rect _info_button_rect;

    ConnectionInfoPopup *_info_popup;

    std::function<bool(int, int)> _accessible_click_handler;

    mforms::Box _search_box;
    mforms::TextEntry _search_text;

    base::Rect _mouse_down_position; // Used to determine if the user starts a drag/drop operation.

    bool _showWelcomeHeading;

    ConnectionVector &displayed_connections();

    void update_colors();

    void on_search_text_changed();
    void on_search_text_action(mforms::TextEntryAction action);
    ssize_t calculate_index_from_point(int x, int y);
    std::shared_ptr<ConnectionEntry> entry_from_point(int x, int y, bool &in_details_area);
    std::shared_ptr<ConnectionEntry> entry_from_index(ssize_t index);
    base::Rect bounds_for_entry(ssize_t index);
    std::string connectionIdFromIndex(ssize_t index);
    int drawHeading(cairo_t *cr);

    void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah);

    virtual bool mouse_down(mforms::MouseButton button, int x, int y);
    virtual bool mouse_up(mforms::MouseButton button, int x, int y);
    virtual bool mouse_double_click(mforms::MouseButton button, int x, int y);
    virtual bool mouse_click(mforms::MouseButton button, int x, int y);
    bool mouse_leave();
    virtual bool mouse_move(mforms::MouseButton button, int x, int y);

    void handle_command(const std::string &command);
    void handle_folder_command(const std::string &command);

    void menu_open();

    void show_info_popup();
    void hide_info_popup();
    void popup_closed();

    void change_to_folder(std::shared_ptr<FolderEntry> folder);

    virtual int get_acc_child_count();
    virtual Accessible *get_acc_child(int index);
    virtual std::string get_acc_name();
    virtual Accessible::Role get_acc_role();

    virtual mforms::Accessible *hit_test(int x, int y);
    bool do_tile_drag(ssize_t index, int x, int y);

    mforms::DragOperation drag_over(View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                    const std::vector<std::string> &formats);
    mforms::DragOperation files_dropped(View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                        const std::vector<std::string> &file_names);
    mforms::DragOperation data_dropped(mforms::View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                       void *data, const std::string &format);

  public:
    static const int CONNECTIONS_LEFT_PADDING = 40;
    static const int CONNECTIONS_RIGHT_PADDING =
      40; // The tile spacing right to the last tile in the row does not belong to this padding.
    static const int CONNECTIONS_TOP_PADDING = 75; // The vertical offset of the first visible shortcut entry->
    static const int CONNECTIONS_SPACING = 9;      // Vertical/horizontal space between entries.

    static const int CONNECTIONS_TILE_WIDTH = 241;
    static const int CONNECTIONS_TILE_HEIGHT = 91;

    ConnectionsSection(HomeScreen *owner);
    ~ConnectionsSection();
    void clear_connections(bool clear_state = true);
    void focus_search_box();
    void showWelcomeHeading(bool state = true);
    virtual void updateHeight();
    virtual void cancelOperation();
    virtual void setFocus();
    virtual bool canHandle(HomeScreenMenuType type);
    virtual void setContextMenu(mforms::Menu *menu, HomeScreenMenuType type);
    virtual void setContextMenuAction(mforms::Menu *menu, HomeScreenMenuType type);
    std::function<anyMap(const std::string &)> getConnectionInfoCallback;

    void addConnection(const std::string &connectionId, const std::string &title, const std::string &description,
                       const std::string &user, const std::string &schema);
  };
}
