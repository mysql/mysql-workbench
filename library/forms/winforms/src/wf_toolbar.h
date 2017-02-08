/**
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

namespace MySQL {
  namespace Forms {

    ref class MformsToolStrip;

    /**
     * Custom layout engine to allow to use expander and segmented tool strip items.
     * This engine only supports horizontal toolstrips.
     */
  private
    ref class MformsToolStripLayout : public System::Windows::Forms::Layout::LayoutEngine {
    public:
      System::Drawing::Size ComputeLayout(MformsToolStrip ^ toolstrip, System::Drawing::Size proposedSize,
                                          bool preferredSizeOnly);
      virtual bool Layout(Object ^ container, System::Windows::Forms::LayoutEventArgs ^ arguments) override;
    };

  private
    ref class MformsToolStrip : public System::Windows::Forms::ToolStrip {
    private:
      MformsToolStripLayout ^ layoutEngine;

    public:
      virtual System::Drawing::Size GetPreferredSize(System::Drawing::Size proposedSize) override;
      void ApplyLocation(System::Windows::Forms::ToolStripItem ^ item, Drawing::Point location);

      virtual property System::Windows::Forms::Layout::LayoutEngine ^ LayoutEngine {
        System::Windows::Forms::Layout::LayoutEngine ^ get() override {
          if (layoutEngine == nullptr)
            layoutEngine = gcnew MformsToolStripLayout();
          return layoutEngine;
        }
      }
    };

    /**
     * Managed wrapper for an mforms ToolBar.
     */
  public
    class ToolBarWrapper : public ViewWrapper {
    protected:
      ToolBarWrapper(mforms::ToolBar *toolbar);

      static bool create_tool_bar(mforms::ToolBar *backend, mforms::ToolBarType type);
      static void insert_item(mforms::ToolBar *backend, int index, mforms::ToolBarItem *item);
      static void remove_item(mforms::ToolBar *backend, mforms::ToolBarItem *item);

      static bool create_tool_item(mforms::ToolBarItem *item, mforms::ToolBarItemType type);
      static void set_item_icon(mforms::ToolBarItem *item, const std::string &path);
      static void set_item_alt_icon(mforms::ToolBarItem *item, const std::string &path);
      static void set_item_text(mforms::ToolBarItem *item, const std::string &text);
      static std::string get_item_text(mforms::ToolBarItem *item);
      static void set_item_name(mforms::ToolBarItem *item, const std::string &);
      static void set_item_enabled(mforms::ToolBarItem *item, bool state);
      static bool get_item_enabled(mforms::ToolBarItem *item);
      static void set_item_checked(mforms::ToolBarItem *item, bool state);
      static bool get_item_checked(mforms::ToolBarItem *item);
      static void set_item_tooltip(mforms::ToolBarItem *item, const std::string &text);

      // For selector items only.
      static void set_selector_items(mforms::ToolBarItem *item, const std::vector<std::string> &values);

      static Drawing::Bitmap ^ create_color_image(String ^ color);

    public:
      static void init();
    };
  };
};
