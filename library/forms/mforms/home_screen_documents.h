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
  //----------------- DocumentEntry ---------------------------------------------------------------

  class DocumentEntry : public mforms::Accessible {
  public:
    std::string path;
    time_t timestamp; // Last accessed as timestamp for sorting.

    std::string title;
    std::string title_shorted;
    std::string folder;
    std::string folder_shorted;
    std::string schemas;
    std::string schemas_shorted;
    std::string last_accessed;
    std::string size;

    base::Rect bounds;
    bool is_model;

    bool operator<(const DocumentEntry &other) const;
    //------ Accessibility Methods -----
    virtual std::string get_acc_name();
    virtual std::string get_acc_description();

    virtual Accessible::Role get_acc_role();
    virtual base::Rect get_acc_bounds();
    virtual std::string get_acc_default_action();
  };

  //----------------- DocumentsSection ---------------------------------------------------------------

  class MFORMS_EXPORT DocumentsSection : public HomeScreenSection {
  private:
    HomeScreen *_owner;

    cairo_surface_t *_model_icon;
    cairo_surface_t *_sql_icon;
    cairo_surface_t *_plus_icon;
    cairo_surface_t *_schema_icon;
    cairo_surface_t *_time_icon;
    cairo_surface_t *_folder_icon;
    cairo_surface_t *_size_icon;
    cairo_surface_t *_close_icon;
    cairo_surface_t *_open_icon;
    cairo_surface_t *_action_icon;
    float _backing_scale_when_icons_loaded;

    ssize_t _entries_per_row;

    bool _show_selection_message; // Additional info to let the user a connection (when opening a script).
    base::Rect _message_close_button_rect;

    typedef std::vector<DocumentEntry>::iterator DocumentIterator;
    std::vector<DocumentEntry> _documents;
    std::vector<DocumentEntry> _filtered_documents;

    mforms::Menu *_model_context_menu;
    mforms::Menu *_model_action_menu;

    ssize_t _hot_entry;
    ssize_t _active_entry;
    enum DisplayMode { Nothing, ModelsOnly, ScriptsOnly, Mixed } _display_mode;

    std::function<bool(int, int)> _accessible_click_handler;

    HomeAccessibleButton _add_button;
    HomeAccessibleButton _open_button;
    HomeAccessibleButton _action_button;

    base::Rect _close_button_rect;
    base::Rect _use_default_button_rect;

    DisplayMode _hot_heading;
    base::Rect _model_heading_rect;
    base::Rect _sql_heading_rect;
    base::Rect _mixed_heading_rect;
    std::string _pending_script;

  public:
    const int DOCUMENTS_LEFT_PADDING = 40;
    const int DOCUMENTS_RIGHT_PADDING = 40;
    const int DOCUMENTS_TOP_PADDING = 64;
    const int DOCUMENTS_VERTICAL_SPACING = 26;
    const int DOCUMENTS_SPACING = 20;

    const int DOCUMENTS_ENTRY_WIDTH = 250; // No spacing horizontally.
    const int DOCUMENTS_ENTRY_HEIGHT = 60;
    const int DOCUMENTS_HEADING_SPACING = 10; // Spacing between a heading part and a separator.
    const int DOCUMENTS_TOP_BASELINE = 40;    // Vertical space from top border to title base line.

    const int MESSAGE_WIDTH = 200;
    const int MESSAGE_HEIGHT = 75;

    const int POPUP_TIP_HEIGHT = 14;

    DocumentsSection(HomeScreen *owner);

    virtual ~DocumentsSection();

    std::size_t entry_from_point(int x, int y);

    /**
     * Draws and icon followed by the given text. The given position is that of the upper left corner
     * of the image.
     */
    void draw_icon_with_text(cairo_t *cr, int x, int y, cairo_surface_t *icon, const std::string &text);

    void draw_entry(cairo_t *cr, const DocumentEntry &entry, bool hot);
    void update_filtered_documents();
    void draw_selection_message(cairo_t *cr);
    void layout(cairo_t *cr);
    virtual void cancelOperation();
    virtual void setFocus();
    virtual bool canHandle(HomeScreenMenuType type);
    virtual void setContextMenu(mforms::Menu *menu, HomeScreenMenuType type);
    virtual void setContextMenuAction(mforms::Menu *menu, HomeScreenMenuType type);

    void load_icons();
    void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah);
    void add_document(const std::string &path, const time_t &time, const std::string schemas, long file_size);
    void clear_documents();
    virtual bool mouse_double_click(mforms::MouseButton button, int x, int y);
    virtual bool mouse_click(mforms::MouseButton button, int x, int y);
    bool mouse_leave();
    virtual bool mouse_move(mforms::MouseButton button, int x, int y);
    void handle_command(const std::string &command);
    void show_connection_select_message();
    void hide_connection_select_message();
    virtual int get_acc_child_count();
    virtual Accessible *get_acc_child(int index);
    virtual Accessible::Role get_acc_role();
    virtual mforms::Accessible *hit_test(int x, int y);
  };

} /* namespace wb */
