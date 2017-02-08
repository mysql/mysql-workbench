/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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
    ref class DrawBoxLayout : public System::Windows::Forms::Layout::LayoutEngine {
    public:
      virtual bool Layout(Object ^ sender, System::Windows::Forms::LayoutEventArgs ^ arguments) override;
    };

  public
    ref class WBControlAccessibleObject : System::Windows::Forms::Control::ControlAccessibleObject {
    private:
      mforms::Accessible *backend;

    public:
      WBControlAccessibleObject(System::Windows::Forms::Control ^ owner, mforms::Accessible *backendOwner);

      property String ^ Name { virtual String ^ get() override; };
      property String ^ Description { virtual String ^ get() override; };
      property System::Windows::Forms::AccessibleRole Role {
        virtual System::Windows::Forms::AccessibleRole get() override;
      };
      property String ^ Value { virtual String ^ get() override; };
      property String ^ DefaultAction { virtual String ^ get() override; };

      virtual int GetChildCount() override;
      virtual System::Windows::Forms::AccessibleObject ^ GetChild(int index) override;
      virtual System::Windows::Forms::AccessibleObject ^ HitTest(int x, int y) override;
      virtual void DoDefaultAction() override;
    };

  public
    ref class WBAccessibleObject : System::Windows::Forms::AccessibleObject {
    private:
      mforms::Accessible *backend;
      WBControlAccessibleObject ^ parent;

    public:
      WBAccessibleObject(mforms::Accessible *back, WBControlAccessibleObject ^ parent_control);

      property String ^ Name { virtual String ^ get() override; };
      property String ^ Description { virtual String ^ get() override; };
      property System::Windows::Forms::AccessibleRole Role {
        virtual System::Windows::Forms::AccessibleRole get() override;
      };
      property System::Drawing::Rectangle Bounds {
        virtual System::Drawing::Rectangle get() override;
      };
      property String ^ DefaultAction { virtual String ^ get() override; };
      property String ^ Value { virtual String ^ get() override; };

      virtual int GetChildCount() override;
      virtual System::Windows::Forms::AccessibleObject ^ GetChild(int index) override;
      virtual System::Windows::Forms::AccessibleObject ^ HitTest(int x, int y) override;
      virtual void DoDefaultAction() override;
    };

    // A helper class to set a few things which are only accessible from a descendant.
  private
    ref class CanvasControl : System::Windows::Forms::Control {
    private:
      mforms::DrawBox *backend;

      DrawBoxLayout ^ layoutEngine;
      Collections::Generic::Dictionary<System::Windows::Forms::Control ^, int> alignments;

    public:
      CanvasControl::CanvasControl();
      virtual System::Drawing::Size GetPreferredSize(System::Drawing::Size proposedSize) override;
      void Add(System::Windows::Forms::Control ^ control, mforms::Alignment alignment);
      void Remove(System::Windows::Forms::Control ^ control);
      void Move(System::Windows::Forms::Control ^ control, int x, int y);
      mforms::Alignment GetAlignment(System::Windows::Forms::Control ^ control);
      void SetBackend(mforms::DrawBox *backend);
      void DoRepaint();

      virtual void OnKeyDown(System::Windows::Forms::KeyEventArgs ^ args) override;
      virtual void OnPaint(System::Windows::Forms::PaintEventArgs ^ args) override;

      virtual property System::Windows::Forms::Layout::LayoutEngine ^
        LayoutEngine { System::Windows::Forms::Layout::LayoutEngine ^ get() override; }

        virtual System::Windows::Forms::AccessibleObject ^
        CreateAccessibilityInstance() override;
    };

  public
    class DrawBoxWrapper : public ViewWrapper {
    protected:
      DrawBoxWrapper(mforms::DrawBox *backend);

      static bool create(mforms::DrawBox *backend);
      static void set_needs_repaint(mforms::DrawBox *backend);
      static void add(mforms::DrawBox *backend, mforms::View *view, mforms::Alignment alignment);
      static void remove(mforms::DrawBox *backend, mforms::View *view);
      static void move(mforms::DrawBox *backend, mforms::View *view, int x, int y);

      void OnRepaint(System::Object ^ sender, System::Windows::Forms::PaintEventArgs ^ e);
      void OnKeyDown(System::Object ^ sender, System::Windows::Forms::KeyEventArgs ^ e);

    public:
      static void init();
    };
  };
};
