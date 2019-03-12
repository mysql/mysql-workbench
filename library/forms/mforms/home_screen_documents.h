/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/drawbox.h"
#include "mforms/menu.h"
#include <string>
#include "base/geometry.h"
#include "home_screen_helpers.h"

namespace mforms {
  class HomeScreen;

  //----------------- DocumentEntry ---------------------------------------------------------------

  class DocumentEntry : public base::Accessible {
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

    std::function<bool(int, int)> default_handler;

    bool operator<(const DocumentEntry &other) const;

    void setTitle(const std::string &t);

    //------ Accessibility Methods -----
    virtual std::string getAccessibilityDescription() override;
    virtual Accessible::Role getAccessibilityRole() override;

    virtual base::Rect getAccessibilityBounds() override;
    virtual std::string getAccessibilityDefaultAction() override;
    virtual void accessibilityDoDefaultAction() override;
  };

  //----------------- DocumentsSection ---------------------------------------------------------------

  class MFORMS_EXPORT DocumentsSection : public HomeScreenSection {
  private:
    HomeScreen *_owner = nullptr;

    cairo_surface_t *_model_icon = nullptr;
    cairo_surface_t *_sql_icon = nullptr;
    cairo_surface_t *_plus_icon = nullptr;
    cairo_surface_t *_schema_icon = nullptr;
    cairo_surface_t *_time_icon = nullptr;
    cairo_surface_t *_folder_icon = nullptr;
    cairo_surface_t *_size_icon = nullptr;
    cairo_surface_t *_close_icon = nullptr;
    cairo_surface_t *_open_icon = nullptr;
    cairo_surface_t *_action_icon = nullptr;

    base::Color _textColor;
    
    ssize_t _entries_per_row = 0;

    bool _show_selection_message = false; // Additional info to let the user select a connection (when opening a script).
    base::Rect _message_close_button_rect;

    typedef std::vector<DocumentEntry>::iterator DocumentIterator;
    std::vector<DocumentEntry> _documents;
    std::vector<DocumentEntry> _filtered_documents;

    mforms::Menu *_model_context_menu = nullptr;
    mforms::Menu *_model_action_menu = nullptr;

    ssize_t _hot_entry = -1;
    ssize_t _active_entry = -1;
    enum DisplayMode { Nothing, ModelsOnly, ScriptsOnly, Mixed } _display_mode = ModelsOnly;

    HomeAccessibleButton _add_button;
    HomeAccessibleButton _open_button;
    HomeAccessibleButton _action_button;

    base::Rect _close_button_rect;
    base::Rect _use_default_button_rect;

    DisplayMode _hot_heading = Nothing;
    base::Rect _model_heading_rect;
    base::Rect _sql_heading_rect;
    base::Rect _mixed_heading_rect;
    std::string _pending_script;

    bool accessibleHandler(int x, int y);
    void deleteIcons();

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
    virtual const char* getTitle() override;
    virtual void cancelOperation() override;
    virtual void setFocus() override;
    virtual bool canHandle(HomeScreenMenuType type) override;
    virtual void setContextMenu(mforms::Menu *menu, HomeScreenMenuType type) override;
    virtual void setContextMenuAction(mforms::Menu *menu, HomeScreenMenuType type) override;

    virtual void updateColors() override;
    virtual void updateIcons() override;

    virtual void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) override;
    void add_document(const std::string &path, const time_t &time, const std::string schemas, long file_size);
    void clear_documents();

    virtual bool mouse_double_click(mforms::MouseButton button, int x, int y) override;
    virtual bool mouse_click(mforms::MouseButton button, int x, int y) override;
    virtual bool mouse_leave() override;
    virtual bool mouse_move(mforms::MouseButton button, int x, int y) override;

    void handle_command(const std::string &command);
    void show_connection_select_message();
    void hide_connection_select_message();

    virtual size_t getAccessibilityChildCount() override;
    virtual Accessible* getAccessibilityChild(size_t index) override;
    virtual Accessible::Role getAccessibilityRole() override;
    virtual base::Accessible* accessibilityHitTest(ssize_t x, ssize_t y) override;
  };

} /* namespace wb */
