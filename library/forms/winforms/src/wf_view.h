/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#define DRAG_SOURCE_FORMAT_NAME "com.mysql.workbench.drag-source"

namespace MySQL {
  namespace Forms {

  public
    enum class AutoResizeMode { ResizeNone, ResizeVertical, ResizeHorizontal, ResizeBoth };

    ref class ViewEventTarget;

  public
    class ViewWrapper : public ObjectWrapper {
    private:
      gcroot<System::Windows::Forms::ToolTip ^> tooltip;
      gcroot<Drawing::Image ^> backgroundImage;
      gcroot<ViewEventTarget ^> eventTarget;

      mforms::Alignment backgroundImageAlignment;
      bool layoutSuspended;
      AutoResizeMode _resize_mode; // Used to constrain certain layout operations.
    protected:
      ViewWrapper(mforms::View *view);

      static void destroy(mforms::View *backend);
      static void show(mforms::View *backend, bool show);
      static int get_width(const mforms::View *backend);
      static int get_height(const mforms::View *backend);
      static int get_preferred_width(mforms::View *backend);
      static int get_preferred_height(mforms::View *backend);
      static int get_x(const mforms::View *backend);
      static int get_y(const mforms::View *backend);
      static void set_size(mforms::View *backend, int w, int h);
      static void set_min_size(mforms::View *backend, int w, int h);
      static void set_padding(mforms::View *backend, int left, int top, int right, int bottom);
      static void set_position(mforms::View *backend, int x, int y);
      static std::pair<int, int> client_to_screen(mforms::View *backend, int x, int y);
      static std::pair<int, int> screen_to_client(mforms::View *backend, int x, int y);

      static void set_enabled(mforms::View *backend, bool flag);
      static bool is_enabled(mforms::View *backend);
      static mforms::View *find_subview(mforms::View *backend, std::string &name);
      static void set_name(mforms::View *backend, const std::string &text);
      static void relayout(mforms::View *backend);
      static void set_needs_repaint(mforms::View *backend);
      static void set_tooltip(mforms::View *backend, const std::string &text);
      static void set_font(mforms::View *backend, const std::string &text);
      static bool is_shown(mforms::View *backend);
      static bool is_fully_visible(mforms::View *backend);
      static void suspend_layout(mforms::View *backend, bool flag);
      static void set_front_color(mforms::View *backend, const std::string &color);
      static std::string get_front_color(mforms::View *backend);
      static void set_back_color(mforms::View *backend, const std::string &color);
      static std::string get_back_color(mforms::View *backend);
      static void set_back_image(mforms::View *backend, const std::string &path, mforms::Alignment alignment);
      static void flush_events(mforms::View *backend);

      static void register_drop_formats(mforms::View *backend, mforms::DropDelegate *target,
                                        const std::vector<std::string> &formats);
      static mforms::DragOperation drag_text(mforms::View *backend, mforms::DragDetails details,
                                             const std::string &text);
      static mforms::DragOperation drag_data(mforms::View *backend, mforms::DragDetails details, void *data,
                                             const std::string &format);
      static mforms::DropPosition get_drop_position(mforms::View *backend);

      static void SetDragImage(System::Windows::Forms::DataObject ^ data, mforms::DragDetails details);

      static void focus(mforms::View *backend);
      static bool has_focus(mforms::View *backend);

      virtual void Initialize();

      virtual void set_front_color(String ^ color);
      virtual void set_padding(int left, int top, int right, int bottom);
      virtual void set_font(const std::string &fontDescription);

      virtual void register_file_drop(mforms::DropDelegate *target){};
      virtual mforms::DropPosition get_drop_position() {
        return mforms::DropPositionUnknown;
      };

    public:
      // Only containers allow drawing a background (restriction imposed by other platforms)
      // so we simulate this here by triggering background drawing only for those classes.
      void DrawBackground(System::Windows::Forms::PaintEventArgs ^ args);
      void set_resize_mode(AutoResizeMode mode);

      // Utility functions need for event handlers.
      static bool use_min_width_for_layout(System::Windows::Forms::Control ^ control);
      static bool use_min_height_for_layout(System::Windows::Forms::Control ^ control);
      static void remove_auto_resize(System::Windows::Forms::Control ^ control, AutoResizeMode mode);
      static AutoResizeMode get_auto_resize(System::Windows::Forms::Control ^ control);
      static bool can_auto_resize_vertically(System::Windows::Forms::Control ^ control);
      static void set_full_auto_resize(System::Windows::Forms::Control ^ control);
      static bool can_auto_resize_horizontally(System::Windows::Forms::Control ^ control);
      static void set_auto_resize(System::Windows::Forms::Control ^ control, AutoResizeMode mode);
      static bool is_layout_dirty(System::Windows::Forms::Control ^ control);
      static void set_layout_dirty(System::Windows::Forms::Control ^ control, bool value);
      static void resize_with_docking(System::Windows::Forms::Control ^ control, System::Drawing::Size &size);
      static void adjust_auto_resize_from_docking(System::Windows::Forms::Control ^ control);
      static bool can_layout(System::Windows::Forms::Control ^ control, String ^ reason);

      static mforms::View *source_view_from_data(System::Windows::Forms::IDataObject ^ data);
      static mforms::ModifierKey GetModifiers(System::Windows::Forms::Keys keyData);

      static void init();
    };
  };
};
