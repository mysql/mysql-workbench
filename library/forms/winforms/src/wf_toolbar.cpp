/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_toolbar.h"

#include "base/log.h"
#include "base/string_utilities.h"

using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace System::IO;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Controls;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

//----------------- ToolStripExpander --------------------------------------------------------------

/**
  * Specialized separator item that makes all following items appear right aligned.
  */
ref class ToolStripExpander : public ToolStripSeparator {};

//----------------- ToolStripSegementedButton-------------------------------------------------------

/**
  * Specialized button that visually forms a group with neighbor segmented buttons
  * without any spacing between them, appearing so as one big button with several areas.
  */
ref class ToolStripSegmentedButton : public ToolStripButton {};

//----------------- MformsToolStripLayout ----------------------------------------------------------

/**
  * Computes the entire layout of the toolstrip (position of items).
  *
  * @param toolstrip What we are computing the layout for.
  * @param proposedSize The size to start from layouting. Since super ordinated controls may impose
  *                     a layout size we need to honor that (especially important for auto wrapping
  *                     labels).
  * @param preferredSizeOnly If true arrange compute only the minimal size needed, otherwise do a full layout.
  * @return The resulting size of the box.
  */
System::Drawing::Size MformsToolStripLayout::ComputeLayout(MformsToolStrip ^ toolstrip, Drawing::Size proposedSize,
                                                           bool preferredSizeOnly) {
  int itemsTotalWidth = 0;
  int maxHeight = 0;
  int expandCount = 0;

  proposedSize.Width -= toolstrip->Padding.Horizontal + toolstrip->GripMargin.Horizontal;
  if (proposedSize.Width < 0)
    proposedSize.Width = 0;

  proposedSize.Height = toolstrip->Size.Height - toolstrip->Padding.Vertical;
  if (proposedSize.Height < 0)
    proposedSize.Height = 0;

  // Determine total width of all items so we can distribute any remaining space over
  // all expander items.
  for each(ToolStripItem ^ item in toolstrip->Items) {
      if (!item->Visible)
        continue;

      if (item->GetType() != ToolStripSegmentedButton::typeid)
        item->Size = item->GetPreferredSize(proposedSize); // The size includes the item's margin.

      if (item->Height > maxHeight)
        maxHeight = item->Height;
      if (item->GetType() == ToolStripExpander::typeid)
        expandCount++; // Expanders have no width per se, but get all that's unused.
      else
        itemsTotalWidth += item->Width + item->Margin.Horizontal;
    }

  // Adjust width of the container if it is too small, but don't make the toolbar smaller than
  // the available space. It always stretches over the available space.
  if (proposedSize.Width < itemsTotalWidth)
    proposedSize.Width = itemsTotalWidth;

  // Same for the height.
  if (proposedSize.Height < maxHeight)
    proposedSize.Height = maxHeight;

  if (!preferredSizeOnly) {
    // Fraction of remaining space per expander. Due to integral pixel bounds + locations
    // we might end up with a number of unused pixel. These will be individually distributed over
    // all expanders as well.
    int expanderFraction = 0;
    int remainingSpace = 0;
    if (expandCount > 0) {
      expanderFraction = (proposedSize.Width - itemsTotalWidth) / expandCount;
      remainingSpace = proposedSize.Width - itemsTotalWidth - (expanderFraction * expandCount);
    }

    int offset = toolstrip->Padding.Left + toolstrip->GripMargin.Horizontal;

    for each(ToolStripItem ^ item in toolstrip->Items) {
        if (!item->Visible)
          continue;

        if (item->GetType() == ToolStripExpander::typeid) {
          offset += expanderFraction;
          if (remainingSpace > 0) {
            offset++;
            remainingSpace--;
          }
        } else {
          System::Drawing::Point itemLocation;
          offset += item->Margin.Left;

          // Vertically center the item within the bounds of the toolstrip (less its padding).
          itemLocation.Y = toolstrip->Padding.Top + (int)Math::Ceiling((proposedSize.Height - item->Height) / 2.0);
          itemLocation.X = offset;
          toolstrip->ApplyLocation(item, itemLocation);
          offset += item->Width + item->Margin.Right;
        }
      }
  }

  proposedSize.Width += toolstrip->Padding.Horizontal + toolstrip->GripMargin.Horizontal;
  proposedSize.Height += toolstrip->Padding.Vertical;

  return proposedSize;
}

//------------------------------------------------------------------------------------------------

bool MformsToolStripLayout::Layout(Object ^ container, LayoutEventArgs ^ arguments) {
  MformsToolStrip ^ toolstrip = dynamic_cast<MformsToolStrip ^>(container);
  if (!ViewWrapper::can_layout(toolstrip, arguments->AffectedProperty))
    return false;

  Drawing::Size toolStripSize = ComputeLayout(toolstrip, toolstrip->Size, false);

  // Finally resize container if necessary.
  bool parentLayoutNeeded = !toolstrip->Size.Equals(toolStripSize);
  if (parentLayoutNeeded && toolstrip->AutoSize)
    toolstrip->Size = toolStripSize;

  return parentLayoutNeeded;
}

//----------------- MformsToolStrip-----------------------------------------------------------------

Drawing::Size MformsToolStrip::GetPreferredSize(Drawing::Size proposedSize) {
  return layoutEngine->ComputeLayout(this, proposedSize, true);
}

//--------------------------------------------------------------------------------------------------

void MformsToolStrip::ApplyLocation(ToolStripItem ^ item, Drawing::Point location) {
  // SetItemLocation is protected and its visibility cannot be increased nor is it possible
  // to allow access to it via friend class or using clauses.
  SetItemLocation(item, location);
}

//--------------------------------------------------------------------------------------------------

ref class ToolbarItemEventTarget;

// Have to separate definition and declaration here as the wrapper and its event target
// depend on each other.
public
class ToolBarItemWrapper : public ObjectWrapper {
private:
  gcroot<ToolbarItemEventTarget ^> eventTarget;
  gcroot<Drawing::Image ^> normalImage;
  gcroot<Drawing::Image ^> activeImage;

public:
  ToolBarItemWrapper(mforms::ToolBarItem *item, mforms::ToolBarItemType type);
  ~ToolBarItemWrapper();

  void UpdateItemImage();
  void SetItemChecked(bool state);
  void Focus();
  void SetNormalImage(Drawing::Image ^ image);
  void SetActiveImage(Drawing::Image ^ image);
  void RegisterDropDown(ToolStripButton ^ button);
};

//----------------- ToolbarItemEventTarget ---------------------------------------------------------

public
ref class ToolbarItemEventTarget {
private:
  ToolBarItemWrapper *wrapper;
  bool destroyed;

public:
  ToolbarItemEventTarget(ToolBarItemWrapper *aWrapper) {
    wrapper = aWrapper;
    destroyed = false;
  }

  //------------------------------------------------------------------------------------------------

  ~ToolbarItemEventTarget() {
    destroyed = true;
  }

  //------------------------------------------------------------------------------------------------

  void OnItemActivation(Object ^ sender, System::EventArgs ^ e) {
    mforms::ToolBarItem *item = ObjectWrapper::GetBackend<mforms::ToolBarItem>(sender);
    if (item != NULL) {
      item->callback();
      if (!destroyed) // We might get deleted in the callback.
        wrapper->UpdateItemImage();
    }
  }

  //------------------------------------------------------------------------------------------------

  void OnColorItemActivation(Object ^ sender, System::EventArgs ^ e) {
    ToolStripButton ^ button = dynamic_cast<ToolStripButton ^>(sender);
    ToolStripDropDownButton ^ dropdown = dynamic_cast<ToolStripDropDownButton ^>(button->Tag);
    dropdown->Image = button->Image; // Show what was selected in the button too.
    dropdown->Text = button->Text;

    mforms::ToolBarItem *item = ObjectWrapper::GetBackend<mforms::ToolBarItem>(dropdown);
    if (item != NULL)
      item->callback();
  }

  //------------------------------------------------------------------------------------------------

  void OnKeyPress(Object ^ sender, KeyPressEventArgs ^ e) {
    if (e->KeyChar == '\r') {
      e->Handled = true;
      mforms::ToolBarItem *item = ObjectWrapper::GetBackend<mforms::ToolBarItem>(sender);
      if (item != NULL)
        item->callback();
    }
  }

  //------------------------------------------------------------------------------------------------
};

//----------------- ToolBarItemWrapper ----------------------------------------------------------------

ToolBarItemWrapper::ToolBarItemWrapper(mforms::ToolBarItem *backend, mforms::ToolBarItemType type)
  : ObjectWrapper(backend) {
  eventTarget = gcnew ToolbarItemEventTarget(this);

  ToolStripItem ^ item = nullptr;
  switch (type) {
    case mforms::ActionItem: {
      ToolStripButton ^ button = ToolBarItemWrapper::Create<ToolStripButton>(backend, this);
      button->Click += gcnew System::EventHandler(eventTarget, &ToolbarItemEventTarget::OnItemActivation);
      button->Margin = Padding(2, 2, 2, 2);
      item = button;
      break;
    }

    case mforms::TextActionItem: {
      ToolStripButton ^ button = ToolBarItemWrapper::Create<ToolStripButton>(backend, this);
      button->Click += gcnew System::EventHandler(eventTarget, &ToolbarItemEventTarget::OnItemActivation);
      button->ForeColor = Conversions::GetApplicationColor(ApplicationColor::AppColorPanelToolbar, true);
      item = button;
      break;
    }

    case mforms::TextEntryItem: {
      ToolStripTextBox ^ box = ViewWrapper::Create<ToolStripTextBox>(backend, this);
      box->Enabled = true;
      box->AutoSize = false;
      box->Size = Size(60, 20);
      box->TextBox->MaximumSize = Size(60, 20);
      box->KeyPress += gcnew KeyPressEventHandler(eventTarget, &ToolbarItemEventTarget::OnKeyPress);
      item = box;
      break;
    }

    case mforms::LabelItem: {
      ToolStripLabel ^ label = ToolBarItemWrapper::Create<ToolStripLabel>(backend, this);
      label->Font = gcnew Font(DEFAULT_FONT_FAMILY, 10, FontStyle::Regular, GraphicsUnit::Pixel);
      label->ForeColor = Conversions::GetApplicationColor(ApplicationColor::AppColorPanelToolbar, true);
      item = label;
      break;
    }

    case mforms::ToggleItem: {
      ToolStripButton ^ button = ToolBarItemWrapper::Create<ToolStripButton>(backend, this);
      button->DisplayStyle = ToolStripItemDisplayStyle::Image;
      button->Click += gcnew System::EventHandler(eventTarget, &ToolbarItemEventTarget::OnItemActivation);
      button->Margin = Padding(2, 2, 2, 2);
      button->CheckOnClick = true;
      item = button;
      break;
    }

    case mforms::SegmentedToggleItem: {
      ToolStripSegmentedButton ^ button = ToolBarItemWrapper::Create<ToolStripSegmentedButton>(backend, this);
      button->DisplayStyle = ToolStripItemDisplayStyle::Image;
      button->Click += gcnew System::EventHandler(eventTarget, &ToolbarItemEventTarget::OnItemActivation);
      button->Margin = Padding(0, 0, 0, 0);
      button->AutoSize = false;
      button->CheckOnClick = true;
      item = button;
      break;
    }

    case mforms::SearchFieldItem: {
      ToolStripTextBox ^ box = ViewWrapper::Create<ToolStripTextBox>(backend, this);
      box->Enabled = true;
      box->AutoSize = false;
      box->Size = Size(100, 20);
      box->TextBox->MaximumSize = Size(100, 20);
      box->KeyPress += gcnew KeyPressEventHandler(eventTarget, &ToolbarItemEventTarget::OnKeyPress);
      item = box;
      break;
    }

    case mforms::FlatSelectorItem: {
      ToolStripDropDownButton ^ selector = ViewWrapper::Create<ToolStripDropDownButton>(backend, this);
      ToolStripDropDown ^ dropdown = gcnew ToolStripDropDown();
      dropdown->BackColor = Color::DarkGray;
      selector->DropDown = dropdown;
      selector->DisplayStyle = ToolStripItemDisplayStyle::Text;
      selector->BackColor = Color::DarkGray;
      selector->Font = ControlUtilities::GetFont("Microsoft Sans Serif", 8.25f);
      selector->Margin = Padding(2, 3, 2, 3);
      item = selector;
      break;
    }
    case mforms::SelectorItem: {
      ToolStripComboBox ^ selector = ViewWrapper::Create<ToolStripComboBox>(backend, this);
      selector->DropDownStyle = ComboBoxStyle::DropDownList;
      selector->FlatStyle = FlatStyle::Popup;
      selector->Font = ControlUtilities::GetFont("Microsoft Sans Serif", 8.25f);
      selector->Margin = Padding(2, 3, 2, 3);
      selector->SelectedIndexChanged += gcnew EventHandler(eventTarget, &ToolbarItemEventTarget::OnItemActivation);
      item = selector;
      break;
    }

    case mforms::ColorSelectorItem: {
      ToolStripDropDownButton ^ selector = ViewWrapper::Create<ToolStripDropDownButton>(backend, this);
      ToolStripDropDown ^ dropdown = gcnew ToolStripDropDown();
      selector->DropDown = dropdown;
      selector->DisplayStyle = ToolStripItemDisplayStyle::Image;
      selector->ShowDropDownArrow = true;
      selector->DropDownDirection = ToolStripDropDownDirection::BelowRight;
      selector->AutoSize = false;
      selector->Size = Size(75, 21);
      item = selector;
      break;
    }

    case mforms::SeparatorItem:
      item = ViewWrapper::Create<ToolStripSeparator>(backend, this);
      break;

    case mforms::ExpanderItem:
      item = ViewWrapper::Create<ToolStripExpander>(backend, this);
      break;

    case mforms::TitleItem: {
      ToolStripLabel ^ label = ToolBarItemWrapper::Create<ToolStripLabel>(backend, this);
      label->Font = gcnew Font(DEFAULT_FONT_FAMILY, 10, FontStyle::Bold, GraphicsUnit::Pixel);
      label->ForeColor = ColorTranslator::FromHtml("#333333");
      item = label;
      break;
    }

    default:
      throw std::runtime_error(
        base::strfmt("Internal error: unimplemented toolbar item type requested (%i).", (int)type));
  }

  item->ImageScaling = ToolStripItemImageScaling::None;
  item->Name = CppStringToNative(backend->getInternalName());
}

//--------------------------------------------------------------------------------------------------

ToolBarItemWrapper::~ToolBarItemWrapper() {
  delete eventTarget;
  delete normalImage;
  delete activeImage;
}

//--------------------------------------------------------------------------------------------------

void ToolBarItemWrapper::UpdateItemImage() {
  ToolStripItem ^ item = GetManagedObject<ToolStripItem>();
  ToolStripButton ^ button = dynamic_cast<ToolStripButton ^>(item);
  bool isChecked = false;
  if (button != nullptr)
    isChecked = button->Checked;

  static Size borderSize(2, 2);
  if (item->GetType() == ToolStripSegmentedButton::typeid) {
    // For segmented buttons we use a trick to avoid the implicit border given by the default
    // implementation. Using a background image avoids it. We have to size the item
    // explicitly, though. Auto size must be off to make this work.
    if (isChecked) {
      item->BackgroundImage = activeImage;
      if (static_cast<Drawing::Image ^>(activeImage) != nullptr) {
        item->AutoSize = false;
        item->Size = activeImage->Size + button->Margin.Size + borderSize;
      }
    } else {
      item->BackgroundImage = normalImage;
      if (static_cast<Drawing::Image ^>(normalImage) != nullptr) {
        item->AutoSize = false;
        item->Size = normalImage->Size + button->Margin.Size + borderSize;
      }
    }
  } else {
    // Use the normal image also for checked items if no active image exists.
    if (isChecked && static_cast<Drawing::Image ^>(activeImage) != nullptr) {
      item->Image = activeImage;
      item->AutoSize = false;
      item->Size = activeImage->Size + button->Margin.Size + borderSize;
    } else {
      item->Image = normalImage;
      if (static_cast<Drawing::Image ^>(normalImage) != nullptr) {
        item->AutoSize = false;
        item->Size = normalImage->Size + button->Margin.Size + borderSize;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

void ToolBarItemWrapper::SetItemChecked(bool state) {
  ToolStripButton ^ button = GetManagedObject<ToolStripButton>();
  if (button != nullptr && button->Checked != state) {
    button->Checked = state;
    UpdateItemImage();
  }
}

//--------------------------------------------------------------------------------------------------

void ToolBarItemWrapper::Focus() {
  ToolStripItem ^ item = GetManagedObject<ToolStripItem>();
  ToolStripTextBox ^ textbox = dynamic_cast<ToolStripTextBox ^>(item);
  if (textbox != nullptr && textbox->CanSelect)
    textbox->Select();
}

//--------------------------------------------------------------------------------------------------

void ToolBarItemWrapper::SetNormalImage(Drawing::Image ^ image) {
  normalImage = image;
  UpdateItemImage();
}

//--------------------------------------------------------------------------------------------------

void ToolBarItemWrapper::SetActiveImage(Drawing::Image ^ image) {
  activeImage = image;
  UpdateItemImage();
}

void ToolBarItemWrapper::RegisterDropDown(ToolStripButton ^ button) {
  button->Click += gcnew EventHandler(eventTarget, &ToolbarItemEventTarget::OnColorItemActivation);
}

//----------------- ToolBarWrapper -----------------------------------------------------------------

ToolBarWrapper::ToolBarWrapper(mforms::ToolBar *toolbar) : ViewWrapper(toolbar) {
}

//--------------------------------------------------------------------------------------------------

bool ToolBarWrapper::create_tool_bar(mforms::ToolBar *backend, mforms::ToolBarType type) {
  ToolBarWrapper *wrapper = new ToolBarWrapper(backend);

  ToolStrip ^ toolstrip = nullptr;
  switch (type) {
    case mforms::MainToolBar:
      toolstrip = ToolBarWrapper::Create<MformsToolStrip>(backend, wrapper);
      toolstrip->Name = "MainToolBar";
      toolstrip->Renderer = gcnew FlatMainToolStripRenderer();
      toolstrip->Padding = Padding(2);
      toolstrip->GripMargin = Padding(4, 6, 4, 6);
      toolstrip->Margin = Padding(4, 5, 2, 5); // Used only for separator items. Does not influence the height.
      toolstrip->AutoSize = true;
      break;

    case mforms::SecondaryToolBar:
    case mforms::PaletteToolBar:
      toolstrip = ToolBarWrapper::Create<MformsToolStrip>(backend, wrapper);
      toolstrip->Name = "SecondaryToolBar";
      toolstrip->Renderer = gcnew FlatSubToolStripRenderer();
      toolstrip->GripStyle = ToolStripGripStyle::Hidden;
      toolstrip->GripMargin = Padding(2, 4, 2, 5);
      toolstrip->Padding = Padding(5, 0, 0, 0);
      toolstrip->AutoSize = false;
      toolstrip->Size = Size(100, 23);
      try {
        toolstrip->Font = ControlUtilities::GetFont("Microsoft Sans Serif", 8.25f);
      } catch (System::ArgumentException ^ e) {
        // Argument exception pops up when the system cannot find the Regular font style (corrupt font).
        logError("ToolBarWrapper::create_tool_bar failed. %s\n", e->Message);
      }

      toolstrip->BackColor = Conversions::GetApplicationColor(ApplicationColor::AppColorPanelToolbar, false);
      break;

    case mforms::OptionsToolBar:
      toolstrip = ViewWrapper::Create<MformsToolStrip>(backend, wrapper);
      toolstrip->Name = "OptionsToolBar";
      toolstrip->GripMargin = Padding(4, 6, 4, 6);
      toolstrip->Margin = Padding(4, 8, 2, 8);
      toolstrip->RenderMode = ToolStripRenderMode::ManagerRenderMode;
      toolstrip->AutoSize = true;
      break;

    case mforms::ToolPickerToolBar:
      toolstrip = ViewWrapper::Create<ToolStrip>(backend, wrapper);
      toolstrip->Name = "ToolPickerToolBar";
      toolstrip->BackColor = Conversions::GetApplicationColor(ApplicationColor::AppColorPanelToolbar, false);
      toolstrip->GripStyle = ToolStripGripStyle::Hidden;
      toolstrip->LayoutStyle = ToolStripLayoutStyle::VerticalStackWithOverflow;
      toolstrip->Padding = Padding(0);
      toolstrip->RenderMode = ToolStripRenderMode::ManagerRenderMode;
      toolstrip->AutoSize = true;
      break;

    default:
      throw std::runtime_error(base::strfmt("Internal error: unimplemented toolbar type requested (%i).", (int)type));
  }

  return toolstrip != nullptr;
}

//--------------------------------------------------------------------------------------------------

void ToolBarWrapper::insert_item(mforms::ToolBar *backend, int index, mforms::ToolBarItem *item) {
  ToolStrip ^ toolstrip = ToolBarWrapper::GetManagedObject<ToolStrip>(backend);
  ToolStripItem ^ native_item = ToolBarWrapper::GetManagedObject<ToolStripItem>(item);

  // Update the name of the control, so the accessibility layer has something to work with.
  native_item->Name = CppStringToNative(item->getInternalName());

  if (index < 0 || index >= toolstrip->Items->Count) {
    index = toolstrip->Items->Count;
    toolstrip->Items->Add(native_item);
  } else
    toolstrip->Items->Insert(index, native_item);
}

//--------------------------------------------------------------------------------------------------

void ToolBarWrapper::remove_item(mforms::ToolBar *backend, mforms::ToolBarItem *item) {
  ToolStrip ^ toolstrip = ToolBarWrapper::GetManagedObject<ToolStrip>(backend);
  ToolStripItem ^ native_item = ToolBarWrapper::GetManagedObject<ToolStripItem>(item);

  toolstrip->Items->Remove(native_item);
}

//--------------------------------------------------------------------------------------------------

bool ToolBarWrapper::create_tool_item(mforms::ToolBarItem *item, mforms::ToolBarItemType type) {
  // ToolBarItemWrapper will itself create the connections to the backend and its native object.
  ToolBarItemWrapper *toolbar_item = new ToolBarItemWrapper(item, type);
  return true;
}

//--------------------------------------------------------------------------------------------------

void ToolBarWrapper::set_item_icon(mforms::ToolBarItem *item, const std::string &path) {
  String ^ iconPath = CppStringToNative(path);
  if (File::Exists(iconPath)) {
    ToolBarItemWrapper *wrapper = item->get_data<ToolBarItemWrapper>();
    wrapper->SetNormalImage(Image::FromFile(iconPath));
  }
}

//--------------------------------------------------------------------------------------------------

void ToolBarWrapper::set_item_alt_icon(mforms::ToolBarItem *item, const std::string &path) {
  String ^ iconPath = CppStringToNative(path);
  if (File::Exists(iconPath)) {
    ToolBarItemWrapper *wrapper = item->get_data<ToolBarItemWrapper>();
    wrapper->SetActiveImage(Image::FromFile(iconPath));
  }
}

//--------------------------------------------------------------------------------------------------

void ToolBarWrapper::set_item_text(mforms::ToolBarItem *item, const std::string &text) {
  String ^ native_text = CppStringToNative(text);

  switch (item->get_type()) {
    case mforms::SelectorItem: {
      ToolStripComboBox ^ selector = ToolBarWrapper::GetManagedObject<ToolStripComboBox>(item);
      int index = selector->FindStringExact(native_text);

      // If not found, selects the first item
      // When there are items
      if (index < 0 && selector->Items->Count)
        index = 0;
      selector->SelectedIndex = index;

      break;
    }

    case mforms::ColorSelectorItem: {
      // If this item is a color selector then find the entry with the corresponding color
      // and use its image for the selector.
      ToolStripDropDownButton ^ dropDownButton = ToolBarWrapper::GetManagedObject<ToolStripDropDownButton>(item);
      bool found = false;
      for each(ToolStripButton ^ button in dropDownButton->DropDownItems) {
          if (button->Text == native_text) {
            dropDownButton->Image = button->Image;
            found = true;
            break;
          }
        }

      if (!found)
        dropDownButton->Image = create_color_image(dropDownButton->Text);

      break;
    }

    default: {
      ToolStripItem ^ stripItem = ToolBarWrapper::GetManagedObject<ToolStripItem>(item);
      stripItem->Text = native_text;
    }
  }
}

//--------------------------------------------------------------------------------------------------

std::string ToolBarWrapper::get_item_text(mforms::ToolBarItem *item) {
  ToolStripItem ^ native_item = ToolBarWrapper::GetManagedObject<ToolStripItem>(item);
  return NativeToCppString(native_item->Text);
}

//--------------------------------------------------------------------------------------------------

void ToolBarWrapper::set_item_name(mforms::ToolBarItem *item, const std::string &name) {
  ToolStripItem ^ native_item = ToolBarWrapper::GetManagedObject<ToolStripItem>(item);
  native_item->AccessibleName = CppStringToNative(name);
}

//--------------------------------------------------------------------------------------------------

void ToolBarWrapper::set_item_enabled(mforms::ToolBarItem *item, bool state) {
  ToolStripItem ^ native_item = ToolBarWrapper::GetManagedObject<ToolStripItem>(item);
  native_item->Enabled = state;
}

//--------------------------------------------------------------------------------------------------

bool ToolBarWrapper::get_item_enabled(mforms::ToolBarItem *item) {
  ToolStripItem ^ native_item = ToolBarWrapper::GetManagedObject<ToolStripItem>(item);
  return native_item->Enabled;
}

//--------------------------------------------------------------------------------------------------

void ToolBarWrapper::set_item_checked(mforms::ToolBarItem *item, bool state) {
  ToolBarItemWrapper *wrapper = item->get_data<ToolBarItemWrapper>();
  wrapper->SetItemChecked(state);
}

//--------------------------------------------------------------------------------------------------

bool ToolBarWrapper::get_item_checked(mforms::ToolBarItem *item) {
  ToolStripButton ^ button = ToolBarWrapper::GetManagedObject<ToolStripButton>(item);
  if (button != nullptr)
    return button->Checked;

  return false;
}

//--------------------------------------------------------------------------------------------------

void ToolBarWrapper::set_item_tooltip(mforms::ToolBarItem *item, const std::string &text) {
  ToolStripItem ^ native_item = ToolBarWrapper::GetManagedObject<ToolStripItem>(item);
  native_item->ToolTipText = CppStringToNative(text);
}

//--------------------------------------------------------------------------------------------------

void ToolBarWrapper::set_selector_items(mforms::ToolBarItem *item, const std::vector<std::string> &values) {
  switch (item->get_type()) {
    case mforms::SelectorItem: {
      ToolStripComboBox ^ combobox = ToolBarWrapper::GetManagedObject<ToolStripComboBox>(item);
      if (combobox != nullptr) {
        // Normal combobox.
        List<String ^> list = CppStringListToNative(values);
        combobox->Items->Clear();
        combobox->Items->AddRange(list.ToArray());
        if (combobox->Items->Count > 0)
          combobox->SelectedIndex = 0;
      }
      break;
    }
    case mforms::FlatSelectorItem: {
      ToolStripDropDownButton ^ selector = ToolBarWrapper::GetManagedObject<ToolStripDropDownButton>(item);
      if (selector != nullptr) {
        List<ToolStripButton ^> buttons = gcnew List<ToolStripButton ^>();
        for (std::vector<std::string>::const_iterator iterator = values.begin(); iterator != values.end(); iterator++) {
          String ^ text = CppStringToNative(*iterator);
          ToolStripButton ^ button = gcnew ToolStripButton();
          button->DisplayStyle = ToolStripItemDisplayStyle::Text;
          ToolBarItemWrapper *wrapper = item->get_data<ToolBarItemWrapper>();
          button->Text = text;
          button->Tag = selector;
          wrapper->RegisterDropDown(button);
          buttons.Add(button);
        }
        selector->DropDown->Items->Clear();
        selector->DropDown->Items->AddRange(buttons.ToArray());
      }
      break;
    }
    case mforms::ColorSelectorItem: {
      ToolStripDropDownButton ^ selector = ToolBarWrapper::GetManagedObject<ToolStripDropDownButton>(item);
      if (selector != nullptr) {
        // Color selector.
        Bitmap ^ selected = nullptr;
        List<ToolStripButton ^> buttons = gcnew List<ToolStripButton ^>();

        for (std::vector<std::string>::const_iterator iterator = values.begin(); iterator != values.end(); iterator++) {
          String ^ text = CppStringToNative(*iterator);
          ToolStripButton ^ button = gcnew ToolStripButton();
          button->DisplayStyle = ToolStripItemDisplayStyle::Image;

          button->Text = text;
          button->Image = create_color_image(text);
          button->Tag = selector;
          button->ImageScaling = ToolStripItemImageScaling::None;

          ToolBarItemWrapper *wrapper = item->get_data<ToolBarItemWrapper>();
          wrapper->RegisterDropDown(button);

          buttons.Add(button);
        }
        selector->DropDown->Items->Clear();
        selector->DropDown->Items->AddRange(buttons.ToArray());
      }
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

Bitmap ^ ToolBarWrapper::create_color_image(String ^ color) {
  SolidBrush ^ brush = gcnew SolidBrush(Color::White);
  Pen ^ pen = gcnew Pen(Color::LightGray);

  Bitmap ^ image = gcnew Bitmap(60, 15);
  Graphics ^ g = Graphics::FromImage(image);
  brush->Color = ColorTranslator::FromHtml(color);
  g->FillRectangle(brush, System::Drawing::Rectangle(1, 0, 58, 14));
  g->DrawRectangle(pen, System::Drawing::Rectangle(1, 0, 58, 14));

  return image;
}

//--------------------------------------------------------------------------------------------------

void ToolBarWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_tool_bar_impl.create_tool_bar = &ToolBarWrapper::create_tool_bar;
  f->_tool_bar_impl.insert_item = &ToolBarWrapper::insert_item;
  f->_tool_bar_impl.remove_item = &ToolBarWrapper::remove_item;
  f->_tool_bar_impl.create_tool_item = &ToolBarWrapper::create_tool_item;
  f->_tool_bar_impl.set_item_icon = &ToolBarWrapper::set_item_icon;
  f->_tool_bar_impl.set_item_alt_icon = &ToolBarWrapper::set_item_alt_icon;
  f->_tool_bar_impl.set_item_text = &ToolBarWrapper::set_item_text;
  f->_tool_bar_impl.get_item_text = &ToolBarWrapper::get_item_text;
  f->_tool_bar_impl.set_item_name = &ToolBarWrapper::set_item_name;
  f->_tool_bar_impl.set_item_enabled = &ToolBarWrapper::set_item_enabled;
  f->_tool_bar_impl.get_item_enabled = &ToolBarWrapper::get_item_enabled;
  f->_tool_bar_impl.set_item_checked = &ToolBarWrapper::set_item_checked;
  f->_tool_bar_impl.get_item_checked = &ToolBarWrapper::get_item_checked;
  f->_tool_bar_impl.set_item_tooltip = &ToolBarWrapper::set_item_tooltip;
  f->_tool_bar_impl.set_selector_items = &ToolBarWrapper::set_selector_items;
}
