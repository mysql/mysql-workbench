/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __CANVAS_VIEWER_H__
#define __CANVAS_VIEWER_H__

#include "Canvas.h"

namespace MySQL {
  namespace GUI {
    namespace Mdc {
    public
      ref class WindowsCanvasViewerPanel : System::Windows::Forms::Panel {
        System::Windows::Forms::ScrollBar ^ vScrollbar = nullptr;
        System::Windows::Forms::ScrollBar ^ hScrollbar = nullptr;

        BaseWindowsCanvasView ^ canvas = nullptr;
        bool canvasInitialized = false;

        bool scrolling = false;

        bool handleInput = false;
        System::Windows::Forms::ToolStripLabel ^ canvasFPSLabel = nullptr;

      public:
        WindowsCanvasViewerPanel();

        WindowsGLCanvasView ^ CreateGLCanvas(Form ^ ownerForm, bool handleInput);
        WindowsGDICanvasView ^ CreateGDICanvas(Form ^ ownerForm, bool handleInput);

        void FinalizeCanvas();

        property BaseWindowsCanvasView ^ Canvas { BaseWindowsCanvasView ^ get() { return canvas; } }

          property Form ^
          OwnerForm {
            Form ^ get() {
              if (canvas != nullptr)
                return canvas->GetOwnerForm();
              else
                return nullptr;
            } void set(Form ^ value) {
              if (canvas != nullptr)
                canvas->SetOwnerForm(value);
            }
          }

          property ScrollBar ^
          VScrollbar {
            ScrollBar ^ get() { return vScrollbar; } void set(ScrollBar ^ value) {
              vScrollbar = value;
            }
          }

          property ScrollBar ^
          HScrollbar {
            ScrollBar ^ get() { return hScrollbar; } void set(ScrollBar ^ value) {
              hScrollbar = value;
            }
          }

          protected : virtual void OnMouseMove(MouseEventArgs ^ e) override;
        virtual void OnMouseDown(MouseEventArgs ^ e) override;
        virtual void OnMouseUp(MouseEventArgs ^ e) override;
        virtual void OnMouseDoubleClick(MouseEventArgs ^ e) override;
        virtual void OnMouseWheel(MouseEventArgs ^ e) override;

        virtual void OnKeyDown(KeyEventArgs ^ e) override;

        virtual void OnKeyUp(KeyEventArgs ^ e) override;
        virtual void OnSizeChanged(EventArgs ^ e) override;
        virtual void OnPaintBackground(PaintEventArgs ^ e) override;
        virtual void OnPaint(PaintEventArgs ^ e) override;

      private:
        void OnNeedsRepaint(int x, int y, int w, int h);
        void OnViewportChanged();

        void ScrollablePanel_Click(Object ^ sender, EventArgs ^ e);
        void UpdateScrollbars();
        void UpdateScrollBarPositions();
        void UpdateScrollBarSizes();

        void DoMouseMove(MouseEventArgs ^ e);

      public:
        void HandleScroll(Object ^ sender, ScrollEventArgs ^ args);
      };

    public
      ref class WindowsCanvasViewer : System::Windows::Forms::Panel {
        WindowsCanvasViewerPanel ^ canvasPanel;

      public:
        WindowsCanvasViewer();

      public:
        property BaseWindowsCanvasView ^ Canvas { BaseWindowsCanvasView ^ get() { return canvasPanel->Canvas; } }

          property System::Windows::Forms::Form ^
          OwnerForm {
            System::Windows::Forms::Form ^
              get() { return canvasPanel->OwnerForm; } void set(System::Windows::Forms::Form ^ value) {
              canvasPanel->OwnerForm = value;
            }
          }

          property System::Windows::Forms::Panel ^
          CanvasPanel { System::Windows::Forms::Panel ^ get() { return canvasPanel; } }

          /// <summary>
          /// Initializes a new canvas viewer. Normally we use OpenGL for rendering, but this can be
          /// switched off by the user (either via the application options or via command line).
          /// The opposite force switch can override the sw rendering switch if the user really wants this.
          /// It is usually only used to override forced sw rendering for certain chip sets.
          /// </summary>
          /// <param name="ownerForm">The hosting WB form for the view.</param>
          /// <param name="handleInput">True if the view should act to user input (mouse/keyboard).</param>
          /// <returns>The newly created canvas view</returns>
          public : BaseWindowsCanvasView ^
                   CreateCanvasView(System::Windows::Forms::Form ^ ownerForm, bool handleInput,
                                    bool software_rendering_enforced, bool opengl_rendering_enforced);

        void FinalizeCanvas() {
          canvasPanel->FinalizeCanvas();
        }
      };
    };
  };
};

#endif