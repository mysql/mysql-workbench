/*
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _HOME_SCREEN_CONNECTIONS_H_
#define _HOME_SCREEN_CONNECTIONS_H_

#include "home_screen.h"

#include "mforms/drawbox.h"
#include "mforms/textentry.h"
#include <boost/shared_ptr.hpp>

namespace mforms
{
  class Menu;
};

namespace wb
{
  class ConnectionEntry;
  class FolderBackEntry;
  class FolderEntry;
  class FabricFolderEntry;
  class FabricManagedConnectionEntry;
  class FabricServerEntry;
  class ConnectionInfoPopup;

  class ConnectionsSection: public mforms::DrawBox, public mforms::DropDelegate
  {
    friend class ConnectionEntry;
    friend class FolderBackEntry;
    friend class FolderEntry;
    friend class FabricFolderEntry;
    friend class FabricManagedConnectionEntry;
    friend class FabricServerEntry;

  private:
    HomeScreen *_owner;

    cairo_surface_t* _folder_icon;
    cairo_surface_t* _mouse_over_icon;
    cairo_surface_t* _mouse_over2_icon;
    cairo_surface_t* _network_icon;
    cairo_surface_t* _ha_filter_icon;
    cairo_surface_t* _managed_status_icon;
    cairo_surface_t* _page_down_icon;
    cairo_surface_t* _page_up_icon;
    cairo_surface_t* _plus_icon;
    cairo_surface_t* _sakila_icon;
    cairo_surface_t* _fabric_icon;
    cairo_surface_t* _schema_icon;
    cairo_surface_t* _user_icon;
    cairo_surface_t* _manage_icon;

    base::Color _tile_bk_color1;
    base::Color _tile_bk_color2;
    base::Color _fabric_tile_bk_color;
    base::Color _managed_primary_tile_bk_color;
    base::Color _managed_secondary_tile_bk_color;
    base::Color _managed_faulty_tile_bk_color;
    base::Color _managed_spare_tile_bk_color;
    base::Color _folder_tile_bk_color;
    base::Color _back_tile_bk_color;

    base::Color _tile_bk_color1_hl;
    base::Color _tile_bk_color2_hl;
    base::Color _folder_tile_bk_color_hl;
    base::Color _fabric_tile_bk_color_hl;
    base::Color _managed_primary_tile_bk_color_hl;
    base::Color _managed_secondary_tile_bk_color_hl;
    base::Color _managed_faulty_tile_bk_color_hl;
    base::Color _managed_spare_tile_bk_color_hl;
    base::Color _back_tile_bk_color_hl;

    ssize_t _page_start;        // Index into the list where root display starts.
    boost::shared_ptr<FolderEntry> _active_folder;     // The folder entry that is currently active.
    std::string _active_folder_title_before_refresh_start;
    // for the paging hack...
    ssize_t _next_page_start;
    std::list<ssize_t> _prev_page_start; // a stack of previous page start indexes
  
    ssize_t _page_start_backup; // Copy of the current page start when we go into a folder (for restauration).
    ssize_t _next_page_start_backup;
    std::list<ssize_t> _prev_page_start_backup; // a stack of previous page start indexes

    typedef std::vector<boost::shared_ptr<ConnectionEntry> > ConnectionVector;
    typedef ConnectionVector::iterator ConnectionIterator;
    ConnectionVector _connections;
    ConnectionVector _filtered_connections;
    bool _filtered;

    mforms::Menu *_connection_context_menu;
    mforms::Menu *_fabric_context_menu;
    mforms::Menu *_folder_context_menu;
    mforms::Menu *_generic_context_menu;

    boost::shared_ptr<ConnectionEntry> _hot_entry; // The connection entry under the mouse.
    boost::shared_ptr<ConnectionEntry> _entry_for_menu; // The entry that was hot when the context menu was opened.
    bool _show_details;      // If there's a hot connection this indicates if we just show the hot state or the connection details.

    ssize_t _drag_index;     // The index of the entry that is being dragged.
    ssize_t _drop_index;     // The index of the entry that is currently the drop target.
    mforms::DropPosition _drop_position;

    HomeAccessibleButton _add_button;
    HomeAccessibleButton _manage_button;
    HomeAccessibleButton _page_up_button;
    HomeAccessibleButton _page_down_button;

    base::Rect _info_button_rect;

    ConnectionInfoPopup *_info_popup;

    boost::function <bool (int, int)> _accessible_click_handler;

    mforms::Box _search_box;
    mforms::TextEntry _search_text;

    base::Rect _mouse_down_position; // Used to determine if the user starts a drag/drop operation.

    ConnectionVector &displayed_connections();

    void update_colors();
    bool is_managed_connection(int index);

    void on_search_text_changed();
    void on_search_text_action(mforms::TextEntryAction action);
    ssize_t calculate_index_from_point(int x, int y);
    boost::shared_ptr<ConnectionEntry> entry_from_point(int x, int y, bool &in_details_area);
    boost::shared_ptr<ConnectionEntry> entry_from_index(ssize_t index);
    base::Rect bounds_for_entry(int index);
    db_mgmt_ConnectionRef connection_from_index(ssize_t index);

    void draw_paging_part(cairo_t *cr, int current_page, int pages, bool high_contrast);

    void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah);

    virtual bool mouse_down(mforms::MouseButton button, int x, int y);
    virtual bool mouse_up(mforms::MouseButton button, int x, int y);
    virtual bool mouse_double_click(mforms::MouseButton button, int x, int y);
    virtual bool mouse_click(mforms::MouseButton button, int x, int y);
    bool mouse_leave();
    virtual bool mouse_move(mforms::MouseButton button, int x, int y);

    void handle_command(const std::string &command);
    void handle_folder_command(const std::string &command, bool is_fabric);

    void menu_open();

    void show_info_popup();
    void hide_info_popup();
    void popup_closed();

    void cancel_operation();

    void change_to_folder(boost::shared_ptr<FolderEntry> folder);

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
    ConnectionsSection(HomeScreen *owner);
    ~ConnectionsSection();
    void clear_connections(bool clear_state = true);
    void focus_search_box();

    void add_connection(const db_mgmt_ConnectionRef &connection, const std::string &title,
                        const std::string &description, const std::string &user, const std::string &schema);

    void set_context_menu(mforms::Menu *menu, HomeScreenMenuType type);
  };
}
#endif
