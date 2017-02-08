/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

  private
    ref class FillLayout : public System::Windows::Forms::Layout::LayoutEngine {
    public:
      System::Drawing::Size ComputeLayout(System::Windows::Forms::Control ^ control, Drawing::Size proposedSize,
                                          bool resizeChildren);
      virtual bool Layout(Object ^ container, System::Windows::Forms::LayoutEventArgs ^ arguments) override;
      System::Drawing::Size GetPreferredSize(System::Windows::Forms::Control ^ control, Drawing::Size proposedSize);
    };

  private
    interface class ValueSetter {
      virtual void ApplyContentBounds(const Drawing::Rectangle % bounds);
    };

  private
    delegate System::Void ApplyBoundsDelegate(const Drawing::Rectangle % bounds);

    /**
     * A group box with a fill layout.
     */
  public
    ref class FillGroupBox : public System::Windows::Forms::GroupBox, ValueSetter {
    private:
      FillLayout ^ layoutEngine;

    protected:
      virtual void OnPaintBackground(System::Windows::Forms::PaintEventArgs ^ args) override;

    public:
      FillGroupBox();

      virtual System::Drawing::Size GetPreferredSize(System::Drawing::Size proposedSize) override;

      virtual property System::Windows::Forms::Layout::LayoutEngine ^
        LayoutEngine {
          System::Windows::Forms::Layout::LayoutEngine ^ get() override {
            if (layoutEngine == nullptr)
              layoutEngine = gcnew FillLayout();

            return layoutEngine;
          }
        }

        virtual void
        ApplyContentBounds(const System::Drawing::Rectangle % bounds) {
        if (Controls->Count > 0)
          Controls[0]->Bounds = bounds;
      };
    };

    /**
     * A panel with a fill layout.
     */
  public
    ref class FillPanel : public System::Windows::Forms::Panel, ValueSetter {
    private:
      FillLayout ^ layoutEngine;

    protected:
      virtual void OnPaintBackground(System::Windows::Forms::PaintEventArgs ^ args) override;

    public:
      FillPanel();

      virtual System::Drawing::Size GetPreferredSize(Drawing::Size proposedSize) override;

      virtual property System::Windows::Forms::Layout::LayoutEngine ^
        LayoutEngine {
          System::Windows::Forms::Layout::LayoutEngine ^ get() override {
            if (layoutEngine == nullptr)
              layoutEngine = gcnew FillLayout();

            return layoutEngine;
          }
        }

        virtual void
        ApplyContentBounds(const Drawing::Rectangle % bounds) {
        if (Controls->Count > 0)
          Controls[0]->Bounds = bounds;
      };
    };

    /**
     * A header panel with a fill layout.
     */
  public
    ref class HeaderFillPanel : public MySQL::Controls::HeaderPanel, ValueSetter {
    private:
      FillLayout ^ layoutEngine;
      System::Drawing::Bitmap ^ background;

    public:
      HeaderFillPanel();

      virtual Drawing::Size GetPreferredSize(Drawing::Size proposedSize) override;

      virtual property System::Windows::Forms::Layout::LayoutEngine ^
        LayoutEngine {
          System::Windows::Forms::Layout::LayoutEngine ^ get() override {
            if (layoutEngine == nullptr)
              layoutEngine = gcnew FillLayout();

            return layoutEngine;
          }
        }

        virtual void
        ApplyContentBounds(const Drawing::Rectangle % bounds) {
        if (Controls->Count > 0)
          Controls[0]->Bounds = bounds;
      };
    };

  public
    class PanelWrapper : public ViewWrapper {
    private:
      mforms::View *child;
      mforms::PanelType type;

    protected:
      PanelWrapper(mforms::View *backend);

      static bool create(mforms::Panel *backend, mforms::PanelType panelType);
      static void set_title(mforms::Panel *backend, const std::string &title);
      static void set_back_color(mforms::Panel *backend, const std::string &color);
      static void add(mforms::Panel *backend, mforms::View *view);
      static void set_active(mforms::Panel *backend, bool value);
      static bool get_active(mforms::Panel *backend);
      static void remove(mforms::Panel *backend, mforms::View *view);

    public:
      virtual void set_title(const std::string &title);
      virtual void set_back_color(const std::string &color);
      virtual void add(mforms::View *view);
      virtual void set_active(bool value);
      virtual bool get_active();
      virtual void remove(mforms::View *view);
      virtual void remove();

      static void init();
    };
  };
};
