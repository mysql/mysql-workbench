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

#include "base/log.h"
#include "base/string_utilities.h"
#include "base/drawing.h"

#include "wf_base.h"
#include "wf_view.h"
#include "wf_app.h"
#include "wf_code_editor.h"
#include "wf_drawbox.h"

using namespace System::Drawing;
using namespace System::IO;
using namespace System::Windows::Forms;
using namespace System::Runtime::InteropServices;

using namespace MySQL;
using namespace MySQL::Controls;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;
using namespace MySQL::Utilities::SysUtils;

using namespace HtmlRenderer;
using namespace Aga::Controls::Tree;

typedef System::Runtime::InteropServices::ComTypes::IDataObject SystemIDataObject;

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

//--------------------------------------------------------------------------------------------------

/**
 * Converts Windows specific mouse button identifiers mforms identifiers.
 */
static mforms::MouseButton convert_mouse_button(MouseButtons button) {
  switch (button) {
    case MouseButtons::Left:
      return mforms::MouseButtonLeft;
    case MouseButtons::Right:
      return mforms::MouseButtonRight;
    case MouseButtons::Middle:
      return mforms::MouseButtonOther;
    default:
      return mforms::MouseButtonNone;
  }
}

//----------------- ViewEventTarget ----------------------------------------------------------------

// A helper class to have a target for managed events.
ref class MySQL::Forms::ViewEventTarget {
private:
  mforms::View *view;
  mforms::DropDelegate *target;

  Control ^ control;
  MySQL::Utilities::IDropTargetHelper ^ helper;

public:
  ViewEventTarget(mforms::View *theView, Control ^ theControl) {
    view = theView;
    target = NULL;
    control = theControl;
    helper = nullptr;
  }

  //------------------------------------------------------------------------------------------------

  void SetDropTarget(mforms::DropDelegate *theTarget) {
    target = theTarget;
  }

  //------------------------------------------------------------------------------------------------

  void Resize(System::Object ^ sender, EventArgs ^ e) {
    view->resize();
  }

  //------------------------------------------------------------------------------------------------

  void Enter(System::Object ^ sender, EventArgs ^ e) {
    view->focus_changed();
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Converts drag event key states flags to mforms constants.
   * This is not the same as converting KeyData (like in wf_textbox.cpp).
   */
  mforms::ModifierKey GetModifiers(int keyState) {
    mforms::ModifierKey modifiers = mforms::ModifierNoModifier;
    if ((keyState & 8) == 8)
      modifiers = modifiers | mforms::ModifierControl;
    if ((keyState & 32) == 32)
      modifiers = modifiers | mforms::ModifierAlt;
    if ((keyState & 4) == 4)
      modifiers = modifiers | mforms::ModifierShift;

    return modifiers;
  }

  //------------------------------------------------------------------------------------------------

  void HandleMouseDown(System::Object ^ sender, MouseEventArgs ^ e) {
    Control ^ control = dynamic_cast<Control ^>(sender);
    if (control == nullptr)
      return;

    control->Focus();
    view->mouse_down(convert_mouse_button(e->Button), e->X, e->Y);
  }

  //------------------------------------------------------------------------------------------------

  void HandleMouseUp(System::Object ^ sender, MouseEventArgs ^ e) {
    view->mouse_up(convert_mouse_button(e->Button), e->X, e->Y);
  }

  //------------------------------------------------------------------------------------------------

  void HandleMouseClick(System::Object ^ sender, MouseEventArgs ^ e) {
    view->mouse_click(convert_mouse_button(e->Button), e->X, e->Y);
  }

  //------------------------------------------------------------------------------------------------

  void HandleMouseDoubleClick(System::Object ^ sender, MouseEventArgs ^ e) {
    view->mouse_double_click(convert_mouse_button(e->Button), e->X, e->Y);
  }

  //------------------------------------------------------------------------------------------------

  void HandleMouseMove(System::Object ^ sender, MouseEventArgs ^ e) {
    view->mouse_move(convert_mouse_button(e->Button), e->X, e->Y);
  }

  //------------------------------------------------------------------------------------------------

  void HandleMouseLeave(System::Object ^ sender, EventArgs ^ e) {
    view->mouse_leave();
  }

  //------------------------------------------------------------------------------------------------

  void HandleMouseEnter(System::Object ^ sender, EventArgs ^ e) {
    view->mouse_enter();
  }

  //------------------------------------------------------------------------------------------------

  void HandlerFocusIn(System::Object ^sender, EventArgs ^e) {
    mforms::View *ownedView = ViewWrapper::GetBackend<mforms::View>(sender);
    if (ownedView != nullptr) {
      ownedView->focusIn();
    }
  }

  //------------------------------------------------------------------------------------------------

  void HandlerFocusOut(System::Object ^sender, EventArgs ^e) {
    mforms::View *ownedView = ViewWrapper::GetBackend<mforms::View>(sender);
    if (ownedView != nullptr) {
      ownedView->focusOut();
    }
  }

  //------------------------------------------------------------------------------------------------

  void HandlerPreviewKeyKeyDown(System::Object ^sender, PreviewKeyDownEventArgs ^e) {
    if (e->KeyCode == Keys::Tab) {
      mforms::View *ownedView = ViewWrapper::GetBackend<mforms::View>(sender);
      if (ownedView != nullptr) {
        e->IsInputKey = ownedView->keyPress(GetKeys(e->KeyCode), ViewWrapper::GetModifiers(e->KeyData));
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  void HandlerKeyDown(System::Object ^sender, KeyEventArgs ^e) {
    mforms::View *ownedView = ViewWrapper::GetBackend<mforms::View>(sender);
    if (e->KeyCode == Keys::Tab) {
      e->Handled = true;
    } else {
      mforms::View *ownedView = ViewWrapper::GetBackend<mforms::View>(sender);
      if (ownedView != nullptr) {
        e->Handled = ownedView->keyPress(GetKeys(e->KeyCode), ViewWrapper::GetModifiers(e->KeyData));
      }
    }
  }
  
  //------------------------------------------------------------------------------------------------


  void HandlerKeyUp(System::Object ^sender, KeyEventArgs ^e) {
    mforms::View *ownedView = ViewWrapper::GetBackend<mforms::View>(sender);
    if (ownedView != nullptr) {
      ownedView->keyRelease(GetKeys(e->KeyCode), ViewWrapper::GetModifiers(e->KeyData));
    }
  }

  //------------------------------------------------------------------------------------------------

  mforms::KeyCode GetKeys(System::Windows::Forms::Keys arg) {
    mforms::KeyCode code = mforms::KeyUnkown;
    switch (arg) {
    case Keys::Home:
      code = mforms::KeyHome;
      break;
    case Keys::End:
      code = mforms::KeyEnd;
      break;
    case Keys::PageUp:
      code = mforms::KeyPrevious;
      break;
    case Keys::PageDown:
      code = mforms::KeyNext;
      break;
    case Keys::Up:
      code = mforms::KeyUp;
      break;
    case Keys::Down:
      code = mforms::KeyDown;
      break;
    case Keys::Return:
      code = mforms::KeyReturn;
      break;
    case Keys::Tab:
      code = mforms::KeyTab;
      break;
    case Keys::Apps:
      code = mforms::KeyMenu;
      break;
    case  Keys::F1:
      code = mforms::KeyF1;
      break;
    case  Keys::F2:
      code = mforms::KeyF2;
      break;
    case  Keys::F3:
      code = mforms::KeyF3;
      break;
    case  Keys::F4:
      code = mforms::KeyF4;
      break;
    case  Keys::F5:
      code = mforms::KeyF5;
      break;
    case  Keys::F6:
      code = mforms::KeyF6;
      break;
    case  Keys::F7:
      code = mforms::KeyF7;
      break;
    case  Keys::F8:
      code = mforms::KeyF8;
      break;
    case  Keys::F9:
      code = mforms::KeyF9;
      break;
    case  Keys::F10:
      code = mforms::KeyF10;
      break;
    case  Keys::F11:
      code = mforms::KeyF11;
      break;
    case  Keys::F12:
      code = mforms::KeyF12;
      break;
    case Keys::LShiftKey:
    case Keys::RShiftKey:
    case Keys::LControlKey:
    case Keys::RControlKey:
    case Keys::LWin:
    case Keys::RWin:
      code = mforms::KeyModifierOnly;
      break;
    default:
      if (arg >= Keys::A && arg <= Keys::Z)
        code = mforms::KeyChar;
      break;
    }
    return code;
  }

  //------------------------------------------------------------------------------------------------

  std::vector<std::string> get_available_formats(System::Windows::Forms::IDataObject ^ data) {
    std::vector<std::string> result;
    array<String ^> ^ formats = data->GetFormats();

    // Translate known formats to the backend type, copy others as is and let the receiver decide.
    for (int i = 0; i < formats->Length; ++i) {
      if (formats[i] == DataFormats::FileDrop)
        result.push_back(mforms::DragFormatFileName);
      else if (formats[i] ==
               DataFormats::UnicodeText) // This includes other text that can be converted to Unicode text.
        result.push_back(mforms::DragFormatText);
      else
        result.push_back(NativeToCppString(formats[i]));
    }

    return result;
  }

  //------------------------------------------------------------------------------------------------

  void HandleDragEnter(System::Object ^ sender, DragEventArgs ^ e) {
    Control ^ control = dynamic_cast<Control ^>(sender);
    if (control != nullptr) {
      if (e->Data->GetDataPresent("DragImageBits")) {
        // If a drag image was registered we can use the drag drop helper.
        helper = (MySQL::Utilities::IDropTargetHelper ^)gcnew MySQL::Utilities::DragDropHelper();
        Win32::POINT point(e->X, e->Y);
        helper->DragEnter(IntPtr::Zero, (SystemIDataObject ^)e->Data, point, (int)e->Effect);
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  void HandleDragOver(System::Object ^ sender, DragEventArgs ^ e) {
    e->Effect = DragDropEffects::None;
    Control ^ control = dynamic_cast<Control ^>(sender);
    if (control != nullptr) {
      if (target != NULL) {
        Drawing::Point point = control->PointToClient(Point(e->X, e->Y));

        mforms::View *origin = ViewWrapper::source_view_from_data(e->Data);

        mforms::DragOperation operation = mforms::DragOperationNone;
        if ((e->AllowedEffect & DragDropEffects::Copy) == DragDropEffects::Copy)
          operation = operation | mforms::DragOperationCopy;
        if ((e->AllowedEffect & DragDropEffects::Move) == DragDropEffects::Move)
          operation = operation | mforms::DragOperationMove;

        operation = target->drag_over(origin, base::Point(point.X, point.Y), operation, get_available_formats(e->Data));

        if ((operation & mforms::DragOperationCopy) != 0)
          e->Effect = DragDropEffects::Copy;
        if ((operation & mforms::DragOperationMove) != 0)
          e->Effect = e->Effect | DragDropEffects::Move;
      } else
        e->Effect = DragDropEffects::None;

      if (helper != nullptr) {
        Win32::POINT point;
        point.x = e->X;
        point.y = e->Y;
        helper->DragOver(point, (int)e->Effect);
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  void HandleDragDrop(System::Object ^ sender, DragEventArgs ^ e) {
    e->Effect = DragDropEffects::None;
    Control ^ control = dynamic_cast<Control ^>(sender);
    if (control != nullptr) {
      if (target != NULL) {
        mforms::View *origin = NULL;
        if (e->Data->GetDataPresent(DRAG_SOURCE_FORMAT_NAME)) {
          IntPtr ^ ref = (IntPtr ^)e->Data->GetData(DRAG_SOURCE_FORMAT_NAME);
          if (ref != nullptr)
            origin = (mforms::View *)ref->ToPointer();
        }

        Drawing::Point point = control->PointToClient(Point(e->X, e->Y));
        std::vector<std::string> formats = get_available_formats(e->Data);

        mforms::DragOperation operation;
        mforms::DragOperation allowedOperations = mforms::DragOperationNone;
        if ((e->AllowedEffect & DragDropEffects::Copy) == DragDropEffects::Copy)
          allowedOperations = allowedOperations | mforms::DragOperationCopy;
        if ((e->AllowedEffect & DragDropEffects::Move) == DragDropEffects::Move)
          allowedOperations = allowedOperations | mforms::DragOperationMove;

        for (size_t i = 0; i < formats.size(); ++i) {
          if (formats[i] == mforms::DragFormatFileName) {
            array<String ^> ^ names = dynamic_cast<array<String ^> ^>(e->Data->GetData(DataFormats::FileDrop));
            if (names != nullptr) {
              std::vector<std::string> file_names;
              for each(String ^ name in names) file_names.push_back(NativeToCppStringRaw(name));
              operation = target->files_dropped(origin, base::Point(point.X, point.Y), allowedOperations, file_names);
              break; // TODO: the breaks in the loop allow only for a single drop format, even though the view.h
                     // description says differently.
            }
          } else if (formats[i] == mforms::DragFormatText) {
            String ^ text = (String ^)e->Data->GetData(DataFormats::UnicodeText);
            operation = target->text_dropped(origin, base::Point(point.X, point.Y), allowedOperations,
                                             NativeToCppStringRaw(text));
            break;
          } else {
            // Forward unknown data only if we at least know it is from us.
            DataWrapper ^ wrapper = dynamic_cast<DataWrapper ^>(e->Data->GetData(e->Data->GetFormats()[(int)i], false));
            if (wrapper != nullptr) {
              operation = target->data_dropped(origin, base::Point(point.X, point.Y), allowedOperations,
                                               wrapper->GetData(), formats[i]);
              break;
            }
          }
        }

        if ((operation & mforms::DragOperationCopy) != 0)
          e->Effect = DragDropEffects::Copy;
        if ((operation & mforms::DragOperationMove) != 0)
          e->Effect = e->Effect | DragDropEffects::Move;
      }

      if (helper != nullptr) {
        Win32::POINT point;
        point.x = e->X;
        point.y = e->Y;
        helper->Drop((SystemIDataObject ^)e->Data, point, (int)e->Effect);
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  void HandleDragLeave(System::Object ^ sender, EventArgs ^ e) {
    if (helper != nullptr) {
      MySQL::Utilities::IDropTargetHelper ^ helper =
        (MySQL::Utilities::IDropTargetHelper ^)gcnew MySQL::Utilities::DragDropHelper();
      helper->DragLeave();

      delete helper;
      helper = nullptr;
    }
  }

  //------------------------------------------------------------------------------------------------

  void Relayout() {
    // Not really an event handler but a target for a threaded invocation.
    control->PerformLayout(control, "Bounds");
  }
};

//----------------- ViewWrapper --------------------------------------------------------------------

ViewWrapper::ViewWrapper(mforms::View *view) : ObjectWrapper(view) {
  tooltip = nullptr;
  backgroundImage = nullptr;
  eventTarget = nullptr;
  layoutSuspended = false;
  _resize_mode = AutoResizeMode::ResizeBoth;
  backgroundImageAlignment = mforms::NoAlign;
}

//--------------------------------------------------------------------------------------------------

void ViewWrapper::destroy(mforms::View *backend) {
  // Not needed anymore.
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines, depending on the type of the given control, if the layout process should use the control's
 * minimum or its preferred size.
 * This is necessary because there is a discrepancy between what is considered the minimum size for layouting
 * and the same size for Windows controls. The latter is usually just 0. Instead the preferred size comes close
 * to what we need in the layouting process. Unfortunately, some controls just return their current size
 * (even if they are empty) or the size they need to fit their full text (text box).
 */
bool ViewWrapper::use_min_width_for_layout(Control ^ control) {
  TextBox ^ text = dynamic_cast<TextBox ^>(control);
  bool needsMinWidth = (text != nullptr) && (text->Multiline);
  if (!needsMinWidth) {
    needsMinWidth =
      is<TabControl>(control) ||
      (Panel::typeid ==
       control->GetType()) // Only the standard panel, not our derivation or we will cause an endless loop.
      || is<ComboBox>(control) || is<TreeViewAdv>(control) || is<ListBox>(control) || is<SplitContainer>(control) ||
      is<HtmlLabel>(control) || is<HtmlPanel>(control) ||
      is<ProgressBar>(control) || is<ScintillaControl>(control) || is<DataGridView>(control) || is<TextBox>(control);
  }

  return needsMinWidth;
}

//--------------------------------------------------------------------------------------------------

bool ViewWrapper::use_min_height_for_layout(Control ^ control) {
  TextBox ^ text = dynamic_cast<TextBox ^>(control);
  bool needsMinHeight = (text != nullptr) && (text->Multiline);
  if (!needsMinHeight) {
    needsMinHeight = is<TabControl>(control) || (Panel::typeid == control->GetType()) || is<ListBox>(control) ||
                     is<TreeViewAdv>(control) || is<SplitContainer>(control) ||
                     is<ScintillaControl>(control) || is<DataGridView>(control);
  }

  return needsMinHeight;
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes the given mode from the auto resize settings of the given control.
 */
void ViewWrapper::remove_auto_resize(Control ^ control, AutoResizeMode mode) {
  ViewWrapper *wrapper = GetWrapper<ViewWrapper>(control);
  if (wrapper == NULL)
    return;

  switch (mode) {
    case AutoResizeMode::ResizeBoth:
      wrapper->_resize_mode = AutoResizeMode::ResizeNone;
      break;
    case AutoResizeMode::ResizeHorizontal:
      if (wrapper->_resize_mode == AutoResizeMode::ResizeBoth)
        wrapper->_resize_mode = AutoResizeMode::ResizeVertical;
      else
        wrapper->_resize_mode = AutoResizeMode::ResizeNone;
      break;
    case AutoResizeMode::ResizeVertical:
      if (wrapper->_resize_mode == AutoResizeMode::ResizeBoth)
        wrapper->_resize_mode = AutoResizeMode::ResizeHorizontal;
      else
        wrapper->_resize_mode = AutoResizeMode::ResizeNone;
      break;
  }
}

//-------------------------------------------------------------------------------------------------

AutoResizeMode ViewWrapper::get_auto_resize(Control ^ control) {
  ViewWrapper *wrapper = GetWrapper<ViewWrapper>(control);
  if (wrapper == NULL) // Can only happen for a control that wasn't created by mforms.
    return AutoResizeMode::ResizeNone;
  return wrapper->_resize_mode;
}

//-------------------------------------------------------------------------------------------------

/**
 * Determines if the given control can be resized in horizontal direction.
 */
bool ViewWrapper::can_auto_resize_horizontally(Control ^ control) {
  ViewWrapper *wrapper = GetWrapper<ViewWrapper>(control);
  if (wrapper == NULL)
    return false;
  AutoResizeMode mode = wrapper->_resize_mode;

  return (mode == AutoResizeMode::ResizeHorizontal) || (mode == AutoResizeMode::ResizeBoth);
}

//-------------------------------------------------------------------------------------------------

/**
 * Determines if the given control can be resized in vertical direction.
 */
bool ViewWrapper::can_auto_resize_vertically(Control ^ control) {
  ViewWrapper *wrapper = GetWrapper<ViewWrapper>(control);
  if (wrapper == NULL)
    return false;
  AutoResizeMode mode = wrapper->_resize_mode;

  return (mode == AutoResizeMode::ResizeVertical) || (mode == AutoResizeMode::ResizeBoth);
}

//-------------------------------------------------------------------------------------------------

/**
 * Enables resizing of the control in both directions.
 */
void ViewWrapper::set_full_auto_resize(Control ^ control) {
  ViewWrapper *wrapper = GetWrapper<ViewWrapper>(control);
  if (wrapper == NULL)
    return;
  wrapper->_resize_mode = AutoResizeMode::ResizeBoth;
}

//-------------------------------------------------------------------------------------------------

/**
 * Sets resize mode whatever the caller specified.
 */
void ViewWrapper::set_auto_resize(Control ^ control, AutoResizeMode mode) {
  ViewWrapper *wrapper = GetWrapper<ViewWrapper>(control);
  if (wrapper == NULL)
    return;
  wrapper->_resize_mode = mode;
}

//-------------------------------------------------------------------------------------------------

bool ViewWrapper::is_layout_dirty(Control ^ control) {
  mforms::View *view = ViewWrapper::GetBackend<mforms::View>(control);
  if (view == NULL)
    return false;
  return view->is_layout_dirty();
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_layout_dirty(Control ^ control, bool value) {
  mforms::View *view = ViewWrapper::GetBackend<mforms::View>(control);
  if (view != NULL)
    view->set_layout_dirty(value);
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::show(mforms::View *backend, bool show) {
  ViewWrapper *wrapper = backend->get_data<ViewWrapper>();
  Control ^ control = wrapper->GetControl();

  // Create the control's window handle if not yet done. This is necessary because otherwise
  // when setting a control to hidden and its window handle is not created, it will be moved to the end
  // of it's parent Controls list, messing up our layout.
  if (!control->IsHandleCreated)
    control->Handle;

  // Use reflection to get the true visible keyState. The Visible property returns the value for the
  // control itself as well as all its parents.
  bool visible =
    (bool)Control::typeid
      ->GetMethod("GetState", System::Reflection::BindingFlags::Instance | System::Reflection::BindingFlags::NonPublic)
      ->Invoke(control, gcnew array<Object ^>{2});
  if (show != visible && backend->get_parent() != NULL) {
    if (wrapper->layoutSuspended)
      backend->get_parent()->set_layout_dirty(true);
    else
      control->PerformLayout(control, "Visible");
  }

  control->Visible = show;
}

//-------------------------------------------------------------------------------------------------

int ViewWrapper::get_width(const mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  return control->Width;
}

//-------------------------------------------------------------------------------------------------

int ViewWrapper::get_height(const mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  return control->Height;
}

//-------------------------------------------------------------------------------------------------

int ViewWrapper::get_preferred_width(mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  return control->PreferredSize.Width;
}

//-------------------------------------------------------------------------------------------------

int ViewWrapper::get_preferred_height(mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  return control->PreferredSize.Height;
}

//-------------------------------------------------------------------------------------------------

int ViewWrapper::get_x(const mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  return control->Location.X;
}

//-------------------------------------------------------------------------------------------------

int ViewWrapper::get_y(const mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  return control->Location.Y;
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_size(mforms::View *backend, int w, int h) {
  Control ^ control = GetManagedObject<Control>(backend);
  Size newSize = control->Size;
  Size newMinSize = control->MinimumSize;
  if (w >= 0) {
    newSize.Width = w;
    newMinSize.Width = w;
  }
  if (h >= 0) {
    newSize.Height = h;
    newMinSize.Height = h;
  }

  // Setting the min size here is a trick to tell layouters that we want a fixed size
  // (not the preferred size).
  control->MinimumSize = newMinSize;
  control->Size = newSize;
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_min_size(mforms::View *backend, int w, int h) {
  Control ^ control = GetManagedObject<Control>(backend);
  Size newMinSize = control->MinimumSize;
  if (w >= 0)
    newMinSize.Width = w;
  if (h >= 0)
    newMinSize.Height = h;

  control->MinimumSize = newMinSize;
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_padding(mforms::View *backend, int left, int top, int right, int bottom) {
  ViewWrapper *wrapper = backend->get_data<ViewWrapper>();
  wrapper->set_padding(left, top, right, bottom);
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_position(mforms::View *backend, int x, int y) {
  Control ^ control = GetManagedObject<Control>(backend);
  control->Location = Point(x, y);
  if (backend->get_parent() != NULL)
    backend->get_parent()->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

std::pair<int, int> ViewWrapper::client_to_screen(mforms::View *backend, int x, int y) {
  Control ^ control = GetManagedObject<Control>(backend);
  System::Drawing::Point location = System::Drawing::Point(x, y);
  location = control->PointToScreen(location);
  return std::make_pair(location.X, location.Y);
}

//-------------------------------------------------------------------------------------------------

std::pair<int, int> ViewWrapper::screen_to_client(mforms::View *backend, int x, int y) {
  Control ^ control = GetManagedObject<Control>(backend);
  System::Drawing::Point location = System::Drawing::Point(x, y);
  location = control->PointToClient(location);
  return std::make_pair(location.X, location.Y);
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::relayout(mforms::View *backend) {
  ViewWrapper *wrapper = backend->get_data<ViewWrapper>();
  Control ^ control = wrapper->GetControl();
  if (control->InvokeRequired)
    control->BeginInvoke(gcnew MethodInvoker(wrapper->eventTarget, &ViewEventTarget::Relayout));
  else
    control->PerformLayout(control, "Bounds");
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_enabled(mforms::View *backend, bool flag) {
  Control ^ control = GetManagedObject<Control>(backend);
  control->Enabled = flag;
}

//-------------------------------------------------------------------------------------------------

bool ViewWrapper::is_enabled(mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  return control->Enabled;
}

//-------------------------------------------------------------------------------------------------

mforms::View *ViewWrapper::find_subview(mforms::View *backend, std::string &name) {
  return NULL;
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_name(mforms::View *backend, const std::string &text) {
  Control ^ control = GetManagedObject<Control>(backend);
  control->Name = CppStringToNative(text);
  control->AccessibleName = control->Name;
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_needs_repaint(mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  control->Invalidate();
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::suspend_layout(mforms::View *backend, bool flag) {
  ViewWrapper *wrapper = backend->get_data<ViewWrapper>();
  wrapper->layoutSuspended = flag;

  Control ^ control = wrapper->GetControl();
  if (flag)
    control->SuspendLayout();
  else {
    control->ResumeLayout();
    if (backend->is_layout_dirty())
      control->PerformLayout(control, "Bounds");
  }
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_front_color(mforms::View *backend, const std::string &color) {
  ViewWrapper *wrapper = backend->get_data<ViewWrapper>();
  wrapper->set_front_color(CppStringToNativeRaw(color));
}

//-------------------------------------------------------------------------------------------------

std::string ViewWrapper::get_front_color(mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  Color ^ color = control->ForeColor;
  if (color == nullptr)
    return "#000000";
  return base::strfmt("#%02x%02x%02x", color->R, color->G, color->B);
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_back_color(mforms::View *backend, const std::string &color) {
  FlatTabControl ^ tabcontrol = GetManagedObject<FlatTabControl>(backend);
  if (tabcontrol != nullptr) {
    if (color.empty())
      tabcontrol->BackgroundColor = Color::Transparent;
    else
      tabcontrol->BackgroundColor = System::Drawing::ColorTranslator::FromHtml(CppStringToNativeRaw(color));
    return;
  }

  Control ^ control = GetManagedObject<Control>(backend);
  if (color.empty()) {
    // Use an empty string as indicator to reset the background color.
    control->ResetBackColor();
    if (is<TreeViewAdv>(control))
      control->BackColor = SystemColors::Window;
    else if (is<ButtonBase>(control))
      ((ButtonBase ^)control)->UseVisualStyleBackColor = true;
    else if (is<TabPage>(control))
      ((TabPage ^)control)->UseVisualStyleBackColor = true;
  } else
    control->BackColor = ColorTranslator::FromHtml(CppStringToNativeRaw(color));
}

//-------------------------------------------------------------------------------------------------

std::string ViewWrapper::get_back_color(mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  Color ^ color = control->BackColor;
  if (color == nullptr)
    return "#000000";
  return base::strfmt("#%02x%02x%02x", color->R, color->G, color->B);
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_back_image(mforms::View *backend, const std::string &path, mforms::Alignment align) {
  ViewWrapper *wrapper = backend->get_data<ViewWrapper>();
  String ^ native_path = AppWrapper::get_image_path(CppStringToNative(path));
  if (File::Exists(native_path)) {
    wrapper->backgroundImage = Image::FromFile(native_path, true);
    wrapper->backgroundImageAlignment = align;
  }
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::focus(mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  System::Windows::Forms::Form ^ form = control->FindForm();
  if (form != nullptr)
    form->ActiveControl = control;
  else if (control->CanSelect)
    control->Select();
}

//-------------------------------------------------------------------------------------------------

bool ViewWrapper::has_focus(mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  return control->Focused;
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_tooltip(mforms::View *backend, const std::string &text) {
  ViewWrapper *wrapper = backend->get_data<ViewWrapper>();
  Control ^ control = wrapper->GetControl();
  if (static_cast<ToolTip ^>(wrapper->tooltip) == nullptr) {
    wrapper->tooltip = gcnew ToolTip();
    wrapper->tooltip->AutoPopDelay = 10000; // 10 sec. show time
    wrapper->tooltip->InitialDelay = 500;   // 500 ms before showing the tooltip if none was visible before.
    wrapper->tooltip->ReshowDelay = 250;    // 250 ms before showing the tooltip if another one was active.
    wrapper->tooltip->ShowAlways = true;    // Show tooltip even if the control is not active.
  }
  wrapper->tooltip->SetToolTip(control, CppStringToNative(text));
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_font(const std::string &fontDescription) {
  Control ^ control = GetManagedObject<Control>();

  std::string font;
  float size;
  bool bold;
  bool italic;
  base::parse_font_description(fontDescription, font, size, bold, italic);

  FontStyle style = FontStyle::Regular;
  if (bold)
    style = (FontStyle)(style | FontStyle::Bold);
  if (italic)
    style = (FontStyle)(style | FontStyle::Italic);

  try {
    // Font size in points.
    control->Font = ControlUtilities::GetFont(CppStringToNativeRaw(font), size, style);
  } catch (System::ArgumentException ^ e) {
    // Argument exception pops up when the system cannot find the Regular font style (corrupt font).
    logError("ViewWrapper::set_font failed. %s\n", e->Message);
  }
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_font(mforms::View *backend, const std::string &text) {
  ViewWrapper *wrapper = backend->get_data<ViewWrapper>();
  wrapper->set_font(text);
}

//-------------------------------------------------------------------------------------------------

bool ViewWrapper::is_shown(mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  return control->Visible;
}

//-------------------------------------------------------------------------------------------------

bool ViewWrapper::is_fully_visible(mforms::View *backend) {
  Control ^ control = GetManagedObject<Control>(backend);
  while (control->Visible) {
    if (control->Parent == nullptr)
      return true;
    control = control->Parent;
  };

  return false;
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::register_drop_formats(mforms::View *backend, mforms::DropDelegate *target,
                                        const std::vector<std::string> &formats) {
  Control ^ control = GetManagedObject<Control>(backend);
  control->AllowDrop =
    !formats.empty(); // On Windows we don't need to preregister formats like we do on the other platforms.

  ViewWrapper *wrapper = backend->get_data<ViewWrapper>();
  ViewEventTarget ^ nativeTarget = wrapper->eventTarget;
  if (nativeTarget == nullptr) {
    // Not set up for general drag/drop. But for file drops we can register via a separate path
    // which is necessary anyway for controls that do low level drag/drop (like Scintilla),
    // disabling so managed drag/drop.
    // Wrappers that can handle file drops have to do processing on their own (via WM_DROPFILES).
    if (formats.size() == 1 && formats[0] == mforms::DragFormatFileName)
      wrapper->register_file_drop(target);
    else
      throw gcnew System::Exception("Attempt to set a drop target on a view that is not enabled for drag and drop");
  } else
    nativeTarget->SetDropTarget(target);
}

//-------------------------------------------------------------------------------------------------

mforms::DragOperation ViewWrapper::drag_text(mforms::View *backend, mforms::DragDetails details,
                                             const std::string &text) {
  Control ^ control = GetManagedObject<Control>(backend);
  System::Windows::Forms::DataObject ^ dataObject =
    gcnew System::Windows::Forms::DataObject(gcnew MySQL::Utilities::DataObject());
  WBIDataObjectExtensions::SetDataEx(dataObject, DataFormats::UnicodeText, CppStringToNative(text));

  // Store the backend pointer in the data object, so we can distinguish between internal and
  // external drag sources.
  WBIDataObjectExtensions::SetDataEx(dataObject, DRAG_SOURCE_FORMAT_NAME, gcnew IntPtr(backend));

  DragDropEffects effects = DragDropEffects::None;
  if ((details.allowedOperations & mforms::DragOperationCopy) != 0)
    effects = effects | DragDropEffects::Copy;
  if ((details.allowedOperations & mforms::DragOperationMove) != 0)
    effects = effects | DragDropEffects::Move;

  effects = control->DoDragDrop(dataObject, effects);
  delete dataObject;

  if ((effects & DragDropEffects::Copy) == DragDropEffects::Copy)
    return mforms::DragOperationCopy;
  if ((effects & DragDropEffects::Move) == DragDropEffects::Move)
    return mforms::DragOperationMove;

  return mforms::DragOperationNone;
}

//-------------------------------------------------------------------------------------------------

mforms::DragOperation ViewWrapper::drag_data(mforms::View *backend, mforms::DragDetails details, void *data,
                                             const std::string &format) {
  Control ^ control = GetManagedObject<Control>(backend);
  DataWrapper ^ wrapper = gcnew DataWrapper(data);
  System::Windows::Forms::DataObject ^ dataObject =
    gcnew System::Windows::Forms::DataObject(gcnew MySQL::Utilities::DataObject());
  WBIDataObjectExtensions::SetDataEx(dataObject, CppStringToNativeRaw(format), wrapper);

  WBIDataObjectExtensions::SetDataEx(dataObject, DRAG_SOURCE_FORMAT_NAME, gcnew IntPtr(backend));

  DragDropEffects effects = DragDropEffects::None;
  if ((details.allowedOperations & mforms::DragOperationCopy) != 0)
    effects = effects | DragDropEffects::Copy;
  if ((details.allowedOperations & mforms::DragOperationMove) != 0)
    effects = effects | DragDropEffects::Move;

  if (details.image != NULL)
    SetDragImage(dataObject, details);
  bool flag = dataObject->GetDataPresent(CppStringToNativeRaw(format));

  effects = control->DoDragDrop(dataObject, effects);
  delete dataObject;
  if ((effects & DragDropEffects::Copy) == DragDropEffects::Copy)
    return mforms::DragOperationCopy;
  if ((effects & DragDropEffects::Move) == DragDropEffects::Move)
    return mforms::DragOperationMove;

  return mforms::DragOperationNone;
}

//-------------------------------------------------------------------------------------------------

mforms::DropPosition ViewWrapper::get_drop_position(mforms::View *backend) {
  ViewWrapper *wrapper = backend->get_data<ViewWrapper>();
  return wrapper->get_drop_position();
}

//-------------------------------------------------------------------------------------------------

/**
*	Tries to get a reference to the source backend view in a drag operation, if there's one.
*/
mforms::View *ViewWrapper::source_view_from_data(System::Windows::Forms::IDataObject ^ data) {
  if (data->GetDataPresent(DRAG_SOURCE_FORMAT_NAME)) {
    IntPtr ^ ref = (IntPtr ^)data->GetData(DRAG_SOURCE_FORMAT_NAME);
    if (ref != nullptr)
      return (mforms::View *)ref->ToPointer();
  }
  return NULL;
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates and sets a drag image from the given details for the data object.
 */
void ViewWrapper::SetDragImage(System::Windows::Forms::DataObject ^ data, mforms::DragDetails details) {
  BITMAPINFO bmpInfo = {0};
  bmpInfo.bmiHeader.biSize = sizeof(bmpInfo);
  bmpInfo.bmiHeader.biWidth = cairo_image_surface_get_width(details.image);
  bmpInfo.bmiHeader.biHeight = cairo_image_surface_get_height(details.image);
  bmpInfo.bmiHeader.biPlanes = 1;
  bmpInfo.bmiHeader.biBitCount = 32;
  bmpInfo.bmiHeader.biCompression = BI_RGB;

  HDC dc = GetDC(0);
  HDC workingDC = CreateCompatibleDC(dc);
  ReleaseDC(0, dc);

  LPVOID bits;
  HBITMAP hbmp = CreateDIBSection(workingDC, &bmpInfo, DIB_RGB_COLORS, &bits, 0, 0);

  cairo_surface_t *bitmapSurface =
    cairo_image_surface_create_for_data((unsigned char *)bits, CAIRO_FORMAT_ARGB32, bmpInfo.bmiHeader.biWidth,
                                        bmpInfo.bmiHeader.biHeight, bmpInfo.bmiHeader.biWidth * 4);

  if (bitmapSurface) {
    cairo_t *cr = cairo_create(bitmapSurface);
    cairo_surface_destroy(bitmapSurface);

    // Windows DIB sections as create here are flipped. Correct this with a mirror matrix.
    cairo_matrix_t matrix;
    cairo_matrix_init(&matrix, 1.0, 0.0, 0.0, -1.0, 0.0, bmpInfo.bmiHeader.biHeight);
    cairo_set_matrix(cr, &matrix);

    // Draw the provided source image into the temporary cairo context which will in turn
    // act on the DIB section.
    cairo_set_source_surface(cr, details.image, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);

    Win32::ShDragImage shdi;
    shdi.sizeDragImage.cx = cairo_image_surface_get_width(details.image);
    shdi.sizeDragImage.cy = cairo_image_surface_get_height(details.image);
    shdi.ptOffset.x = (long)details.hotspot.x;
    shdi.ptOffset.y = (long)details.hotspot.y;
    shdi.hbmpDragImage = IntPtr(hbmp);
    shdi.crColorKey = 0xFFFFFFFF;

    MySQL::Utilities::IDragSourceHelper ^ helper = (MySQL::Utilities::IDragSourceHelper ^)gcnew DragDropHelper();
    helper->InitializeFromBitmap(shdi, data);
  }
}

//-------------------------------------------------------------------------------------------------

/**
 * Resizes the given control according to its dock keyState. Sizes of the control which were not changed
 * are also reset in the given Size structure to allow for proper child layouting.
 */
void ViewWrapper::resize_with_docking(Control ^ control, System::Drawing::Size &size) {
  // If the control is docked somewhere resizing must be restricted not to destroy the docked
  // appearance.
  // Do not resize if the control has the fill dock style. In that case it must stay as it is.
  if (control->Dock == DockStyle::Fill)
    return;

  if (control->Dock == DockStyle::Left || control->Dock == DockStyle::Right)
    size.Height = control->Size.Height;
  else if (control->Dock == DockStyle::Top || control->Dock == DockStyle::Bottom)
    size.Width = control->Size.Width;

  control->Size = size;
  set_layout_dirty(control, true);
}

//-------------------------------------------------------------------------------------------------

/**
 * Removes auto resizing flags if a control is limited in resizing by docking.
 */
void ViewWrapper::adjust_auto_resize_from_docking(Control ^ control) {
  if (control->Dock == DockStyle::Fill)
    set_auto_resize(control, AutoResizeMode::ResizeNone);
  else if (control->Dock == DockStyle::Left || control->Dock == DockStyle::Right)
    remove_auto_resize(control, AutoResizeMode::ResizeHorizontal);
  else if (control->Dock == DockStyle::Top || control->Dock == DockStyle::Bottom)
    remove_auto_resize(control, AutoResizeMode::ResizeVertical);
}

//-------------------------------------------------------------------------------------------------

/**
 * Quick check if it makes sense to start layouting the given control.
 */
bool ViewWrapper::can_layout(Control ^ control, String ^ reason) {
  // Limit layouting to certain property changes.
  if (reason != nullptr) {
    if (reason != "Padding" && reason != "Bounds" && reason != "Visible" && reason != "Parent")
      return false;
  }

  /* Opting out based on a not created handle is a good optimization. Unfortunately, sometimes it
     prevents some forms (e.g. wizards) from layouting properly. So we have to disable this for now.
  if (!control->IsHandleCreated)
    return false;
  */

  if (control->Tag != nullptr) {
    mforms::View *backend = GetBackend<mforms::View>(control);
    if (backend->is_destroying())
      return false;
  }

  return true;
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::flush_events(mforms::View *) {
  Application::DoEvents();
}

//-------------------------------------------------------------------------------------------------

/**
 * Draws the background image with the earlier set layout, if there's one.
 */
void ViewWrapper::DrawBackground(PaintEventArgs ^ args) {
  if (static_cast<Drawing::Image ^>(backgroundImage) != nullptr) {
    int left = 0;
    int top = 0;
    Control ^ control = GetControl();

    // Horizontal alignment.
    switch (backgroundImageAlignment) {
      case mforms::BottomCenter:
      case mforms::MiddleCenter:
      case mforms::TopCenter:
        left = (control->Width - backgroundImage->Width) / 2;
        break;

      case mforms::BottomRight:
      case mforms::MiddleRight:
      case mforms::TopRight:
        left = control->Width - backgroundImage->Width;
        break;
    }

    // Vertical alignment.
    switch (backgroundImageAlignment) {
      case mforms::MiddleLeft:
      case mforms::MiddleCenter:
      case mforms::MiddleRight:
        top = (control->Height - backgroundImage->Height) / 2;
        break;

      case mforms::BottomLeft:
      case mforms::BottomCenter:
      case mforms::BottomRight:
        top = control->Height - backgroundImage->Height;
        break;
    }

    args->Graphics->DrawImage(backgroundImage, Point(left, top));
  }
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::set_resize_mode(AutoResizeMode mode) {
  _resize_mode = mode;
}

//-------------------------------------------------------------------------------------------------

void ViewWrapper::Initialize() {
  Control ^ control = GetControl();

  // Can be null, e.g. for non-control objects like dialogs.
  if (control != nullptr) {
    control->AutoSize = false;
    control->Font = gcnew Font(DEFAULT_FONT_FAMILY, DEFAULT_FONT_SIZE, FontStyle::Regular, GraphicsUnit::Pixel);

    bool hookEvents = is<TabControl>(control) || (Panel::typeid == control->GetType()) || is<TreeViewAdv>(control) ||
                      is<CanvasControl>(control)
      // Don't add ScintillaControl here. It has its own drag/drop handling.
      ;

    if (hookEvents) {
      mforms::View *view = GetBackend<mforms::View>(control);
      eventTarget = gcnew ViewEventTarget(view, control);
      control->Resize += gcnew EventHandler(eventTarget, &ViewEventTarget::Resize);
      control->Enter += gcnew EventHandler(eventTarget, &ViewEventTarget::Enter);

      control->MouseDown += gcnew MouseEventHandler(eventTarget, &ViewEventTarget::HandleMouseDown);
      control->MouseUp += gcnew MouseEventHandler(eventTarget, &ViewEventTarget::HandleMouseUp);
      control->MouseMove += gcnew MouseEventHandler(eventTarget, &ViewEventTarget::HandleMouseMove);
      control->MouseClick += gcnew MouseEventHandler(eventTarget, &ViewEventTarget::HandleMouseClick);
      control->MouseDoubleClick += gcnew MouseEventHandler(eventTarget, &ViewEventTarget::HandleMouseDoubleClick);
      control->MouseEnter += gcnew EventHandler(eventTarget, &ViewEventTarget::HandleMouseEnter);
      control->MouseLeave += gcnew EventHandler(eventTarget, &ViewEventTarget::HandleMouseLeave);

      control->DragEnter += gcnew DragEventHandler(eventTarget, &ViewEventTarget::HandleDragEnter);
      control->DragDrop += gcnew DragEventHandler(eventTarget, &ViewEventTarget::HandleDragDrop);
      control->DragOver += gcnew DragEventHandler(eventTarget, &ViewEventTarget::HandleDragOver);
      control->DragLeave += gcnew EventHandler(eventTarget, &ViewEventTarget::HandleDragLeave);
      control->GotFocus += gcnew EventHandler(eventTarget, &ViewEventTarget::HandlerFocusIn);
      control->LostFocus += gcnew EventHandler(eventTarget, &ViewEventTarget::HandlerFocusOut);
      control->PreviewKeyDown += gcnew PreviewKeyDownEventHandler(eventTarget, &ViewEventTarget::HandlerPreviewKeyKeyDown);
      control->KeyDown += gcnew KeyEventHandler(eventTarget, &ViewEventTarget::HandlerKeyDown);
      control->KeyUp += gcnew KeyEventHandler(eventTarget, &ViewEventTarget::HandlerKeyUp);
    }
  }
};

//--------------------------------------------------------------------------------------------------

void ViewWrapper::set_front_color(String ^ color) {
  Control ^ control = GetManagedObject<Control>();
  control->ForeColor = System::Drawing::ColorTranslator::FromHtml(color);
}

//--------------------------------------------------------------------------------------------------

void ViewWrapper::set_padding(int left, int top, int right, int bottom) {
  Control ^ control = GetManagedObject<Control>();
  control->Padding = System::Windows::Forms::Padding(left, top, right, bottom);
}

//--------------------------------------------------------------------------------------------------

mforms::ModifierKey ViewWrapper::GetModifiers(Keys keyData) {
  mforms::ModifierKey modifiers = mforms::ModifierNoModifier;
  if ((keyData & Keys::Control) == Keys::Control)
    modifiers = modifiers | mforms::ModifierControl;
  if ((keyData & Keys::Alt) == Keys::Alt)
    modifiers = modifiers | mforms::ModifierAlt;
  if ((keyData & Keys::Shift) == Keys::Shift)
    modifiers = modifiers | mforms::ModifierShift;
  if ((keyData & Keys::LWin) == Keys::LWin)
    modifiers = modifiers | mforms::ModifierCommand;
  if ((keyData & Keys::RWin) == Keys::RWin)
    modifiers = modifiers | mforms::ModifierCommand;
  return modifiers;
}

//--------------------------------------------------------------------------------------------------

void ViewWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_view_impl.destroy = &destroy;
  f->_view_impl.get_x = &get_x;
  f->_view_impl.get_y = &get_y;
  f->_view_impl.get_width = &get_width;
  f->_view_impl.get_height = &get_height;
  f->_view_impl.get_preferred_width = &get_preferred_width;
  f->_view_impl.get_preferred_height = &get_preferred_height;
  f->_view_impl.set_position = &set_position;
  f->_view_impl.set_size = &set_size;
  f->_view_impl.set_min_size = &set_min_size;
  f->_view_impl.set_padding = &set_padding;
  f->_view_impl.screen_to_client = &screen_to_client;
  f->_view_impl.client_to_screen = &client_to_screen;

  f->_view_impl.show = &show;
  f->_view_impl.set_enabled = &set_enabled;
  f->_view_impl.is_enabled = &is_enabled;
  f->_view_impl.set_name = &set_name;
  f->_view_impl.relayout = &relayout;
  f->_view_impl.set_needs_repaint = &set_needs_repaint;
  f->_view_impl.set_tooltip = &set_tooltip;
  f->_view_impl.set_font = &set_font;
  f->_view_impl.is_shown = &is_shown;
  f->_view_impl.is_fully_visible = &is_fully_visible;
  f->_view_impl.suspend_layout = &suspend_layout;
  f->_view_impl.set_front_color = &set_front_color;
  f->_view_impl.get_front_color = &get_front_color;
  f->_view_impl.set_back_color = &set_back_color;
  f->_view_impl.get_back_color = &get_back_color;
  f->_view_impl.set_back_image = &set_back_image;
  f->_view_impl.flush_events = &flush_events;
  f->_view_impl.focus = &focus;
  f->_view_impl.has_focus = &has_focus;

  f->_view_impl.register_drop_formats = &register_drop_formats;
  f->_view_impl.drag_text = &drag_text;
  f->_view_impl.drag_data = &drag_data;
  f->_view_impl.get_drop_position = &get_drop_position;
}

//-------------------------------------------------------------------------------------------------
