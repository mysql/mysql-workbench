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

    ref class LayoutBox;

  public
    ref class GtkBoxLayout abstract : public System::Windows::Forms::Layout::LayoutEngine {
    public:
      virtual System::Drawing::Size GetPreferredSize(LayoutBox ^ container, System::Drawing::Size proposedSize) = 0;
    };

    // Implements an extended flow layout behavior which mimics the horizontal GTK.box layout.
  public
    ref class HorizontalGtkBoxLayout : public GtkBoxLayout {
    public:
      System::Drawing::Size ComputeLayout(LayoutBox ^ box, System::Drawing::Size proposedSize, bool resizeChildren);
      virtual bool Layout(Object ^ container, System::Windows::Forms::LayoutEventArgs ^ arguments) override;
      virtual System::Drawing::Size GetPreferredSize(LayoutBox ^ container,
                                                     System::Drawing::Size proposedSize) override;
    };

    // Implements an extended flow layout behavior which mimics the vertical GTK.box layout.
  public
    ref class VerticalGtkBoxLayout : public GtkBoxLayout {
    public:
      System::Drawing::Size ComputeLayout(LayoutBox ^ box, System::Drawing::Size proposedSize, bool resizeChildren);
      virtual bool Layout(Object ^ container, System::Windows::Forms::LayoutEventArgs ^ arguments) override;
      virtual System::Drawing::Size GetPreferredSize(LayoutBox ^ container,
                                                     System::Drawing::Size proposedSize) override;
    };

    // Implements a GTK.box like control.
  public
    ref class LayoutBox : public System::Windows::Forms::Panel {
    private:
      System::Collections::Generic::Dictionary<System::Windows::Forms::Control ^, bool> expandInfo;
      System::Collections::Generic::Dictionary<System::Windows::Forms::Control ^, bool> fillInfo;

      GtkBoxLayout ^ layoutEngine;
      bool horizontal;
      bool homogeneous;
      int spacing;

    protected:
      virtual void OnPaintBackground(System::Windows::Forms::PaintEventArgs ^ args) override;

    public:
      LayoutBox();

      bool GetControlExpands(System::Windows::Forms::Control ^ ctl) {
        return expandInfo[ctl];
      }

      bool GetControlFills(System::Windows::Forms::Control ^ ctl) {
        return fillInfo[ctl];
      }

      virtual Drawing::Size GetPreferredSize(Drawing::Size proposedSize) override;

      void Add(System::Windows::Forms::Control ^ ctl, bool expands, bool fills) {
        ViewWrapper::set_layout_dirty(this, true);
        expandInfo[ctl] = expands;
        fillInfo[ctl] = fills;
        Controls->Add(ctl);
      }

      void Remove(System::Windows::Forms::Control ^ ctl) {
        Controls->Remove(ctl);
        expandInfo.Remove(ctl);
        fillInfo.Remove(ctl);
      }

      virtual property System::Windows::Forms::Layout::LayoutEngine ^
        LayoutEngine {
          System::Windows::Forms::Layout::LayoutEngine ^ get() override {
            if (layoutEngine == nullptr) {
              if (horizontal)
                layoutEngine = gcnew HorizontalGtkBoxLayout();
              else
                layoutEngine = gcnew VerticalGtkBoxLayout();
            }
            return layoutEngine;
          }
        }

        virtual property bool Homogeneous {
        bool get() {
          return homogeneous;
        }

        void set(bool value) {
          if (homogeneous != value) {
            ViewWrapper::set_layout_dirty(this, true);
            homogeneous = value;
            Refresh();
          }
        }
      }

      virtual property int Spacing {
        int get() {
          return spacing;
        }

        void set(int value) {
          if (spacing != value) {
            ViewWrapper::set_layout_dirty(this, true);
            spacing = value;
            Refresh();
          }
        }
      }

      virtual property bool Horizontal {
        bool get() {
          return horizontal;
        }

        void set(bool value) {
          if (horizontal != value) {
            horizontal = value;
            layoutEngine = nullptr;
            ViewWrapper::set_layout_dirty(this, true);
            Refresh();
          }
        }
      }
    };

  public
    class BoxWrapper : public ViewWrapper {
    protected:
      static bool create(mforms::Box *backend, bool horizontal);
      static void add(mforms::Box *backend, mforms::View *child, bool expand, bool fill);
      static void add_end(mforms::Box *backend, mforms::View *child, bool expand, bool fill);
      static void remove(mforms::Box *backend, mforms::View *child);
      static void set_spacing(mforms::Box *backend, int space);
      static void set_homogeneous(mforms::Box *backend, bool value);

    public:
      BoxWrapper(mforms::Box *box);

      static void init();
    };
  };
};
