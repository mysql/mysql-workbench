/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/box.h"
#include "mforms/drawbox.h"
#include "mforms/textentry.h"
#include "mforms/mforms.h"
#include "base/data_types.h"
#include "home_screen_helpers.h"

namespace mforms
{
  class XConnectionEntry;
  class XFolderBackEntry;
  class XFolderEntry;
  class XConnectionInfoPopup;
  class HomeScreen;

  class MFORMS_EXPORT XConnectionsSection : public HomeScreenSection, public mforms::DropDelegate
  {
    friend class XConnectionEntry;
    friend class XFolderBackEntry;
    friend class XFolderEntry;

  public:
    typedef std::vector<std::shared_ptr<XConnectionEntry>> XConnectionVector;

  private:
    HomeScreen *_owner;

    cairo_surface_t* _folder_icon;
    cairo_surface_t* _mouse_over_icon;
    cairo_surface_t* _mouse_over2_icon;
    cairo_surface_t* _plus_icon;
    cairo_surface_t* _manage_icon;
    cairo_surface_t* _xTileIcon;

    base::Color _titleColor;
    base::Color _descriptionColor;
    base::Color _folderTitleColor;
    base::Color _backgroundColor;
    base::Color _backgroundColorHot;
    base::Color _folderBackgroundColor;
    base::Color _folderBackgroundColorHot;
    base::Color _backTileBackgroundColor;
    base::Color _backTileBackgroundColorHot;

    std::shared_ptr<XFolderEntry> _active_folder;     // The folder entry that is currently active.
    std::string _active_folder_title_before_refresh_start;

    typedef XConnectionVector::iterator XConnectionIterator;
    XConnectionVector _connections;
    XConnectionVector _filtered_connections;
    bool _filtered;

    mforms::Menu *_connection_context_menu;
    mforms::Menu *_folder_context_menu;
    mforms::Menu *_generic_context_menu;

    std::shared_ptr<XConnectionEntry> _hot_entry; // The connection entry under the mouse.
    std::shared_ptr<XConnectionEntry> _entry_for_menu; // The entry that was hot when the context menu was opened.
    bool _showDetails;      // If there's a hot connection this indicates if we just show the hot state or the connection details.

    ssize_t _dragIndex;     // The index of the entry that is being dragged.
    ssize_t _dropIndex;     // The index of the entry that is currently the drop target.
    mforms::DropPosition _dropPosition;

    HomeAccessibleButton _addButton;
    HomeAccessibleButton _manageButton;
    HomeAccessibleButton _learnButton;
    HomeAccessibleButton _tutorialButton;
    HomeAccessibleButton _useTraditionalButton;

    base::Rect _info_button_rect;

    XConnectionInfoPopup *_info_popup;

    boost::function <bool (int, int)> _accessible_click_handler;

    base::Rect _mouse_down_position; // Used to determine if the user starts a drag/drop operation.

    XConnectionVector &displayed_connections();

    void update_colors();

    ssize_t calculate_index_from_point(int x, int y);
    std::shared_ptr<XConnectionEntry> entry_from_point(int x, int y, bool &in_details_area);
    std::shared_ptr<XConnectionEntry> entry_from_index(ssize_t index);
    base::Rect bounds_for_entry(ssize_t index);

    dataTypes::XProject projectFromIndex(ssize_t index);

    int drawHeading(cairo_t *cr);

    void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah);

    int calculateHeight();

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

    void change_to_folder(std::shared_ptr<XFolderEntry> folder);

    virtual int get_acc_child_count();
    virtual Accessible* get_acc_child(int index);
    virtual std::string get_acc_name();
    virtual Accessible::Role get_acc_role();

    virtual mforms::Accessible* hit_test(int x, int y);
    bool do_tile_drag(ssize_t index, int x, int y);

    mforms::DragOperation drag_over(View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                    const std::vector<std::string> &formats);
    mforms::DragOperation files_dropped(View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                        const std::vector<std::string> &file_names);
    mforms::DragOperation data_dropped(mforms::View *sender, base::Point p,
                                       mforms::DragOperation allowedOperations, void *data, const std::string &format);

  public:
    static const int CONNECTIONS_LEFT_PADDING = 40;
    static const int CONNECTIONS_RIGHT_PADDING = 40; // The tile spacing right to the last tile in the row does not belong to this padding.
    static const int CONNECTIONS_TOP_PADDING = 75; // The vertical offset of the first visible shortcut entry.
    static const int CONNECTIONS_SPACING = 9; // Vertical/horizontal space between entries.

    static const int CONNECTIONS_TILE_WIDTH = 300;
    static const int CONNECTIONS_TILE_HEIGHT = 85;

    XConnectionsSection(HomeScreen *owner);
    ~XConnectionsSection();
    void clear_connections(bool clear_state = true);

    std::shared_ptr<XFolderEntry> createFolder(const dataTypes::ProjectHolder &holder);
    std::shared_ptr<XConnectionEntry> createConnection(const dataTypes::XProject &project);
    void loadProjects(const dataTypes::ProjectHolder &holder);
    void loadProjects(const dataTypes::ProjectHolder &holder, XConnectionVector &children);

    virtual void updateHeight();
    virtual void cancelOperation();
    virtual void setFocus();
    virtual bool canHandle(HomeScreenMenuType type);
    virtual void setContextMenu(mforms::Menu *menu, HomeScreenMenuType type);
    virtual void setContextMenuAction(mforms::Menu *menu, HomeScreenMenuType type);
  };
}
