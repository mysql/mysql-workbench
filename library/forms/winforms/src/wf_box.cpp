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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_box.h"
#include "wf_label.h"

using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

ref class ControlEntry {
public:
  Control ^ control;
  System::Drawing::Rectangle bounds;
  bool fills;
};

typedef List<ControlEntry ^> ControlList;

//--------------------------------------------------------------------------------------------------

/**
 * Adjusts all child bounds to fill their container vertically (in horizontal mode)
 * or horizontally (in vertical mode). Applies the given padding value accordingly.
 *
 * Note: containerSize must not contain any padding value.
 *
 */
void maximize_children(ControlList % list, bool horizontal, Size containerSize, int padding) {
  if (horizontal) {
    for each(ControlEntry ^ entry in list) {
        ViewWrapper::remove_auto_resize(entry->control, AutoResizeMode::ResizeVertical);
        entry->bounds.Y = padding;
        entry->bounds.Height = containerSize.Height;
      };
  } else {
    for each(ControlEntry ^ entry in list) {
        ViewWrapper::remove_auto_resize(entry->control, AutoResizeMode::ResizeHorizontal);
        entry->bounds.X = padding;

        // For labels determine the preferred height based on the new width (as they can be in auto wrap mode).
        if (is<WrapControlLabel>(entry->control)) {
          Size size = entry->control->GetPreferredSize(Size(containerSize.Width, 0));
          entry->bounds.Size = Size(containerSize.Width, size.Height);
        } else
          entry->bounds.Width = containerSize.Width;
      };
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Applies the computed bounds to each control. Adjust position if the control does not fill the given
 * space depending on the fill flag.
 */
void apply_bounds(ControlList % list, bool horizontal) {
  // Accumulate additional offsets which could be caused by control constraints (i.e. a control
  // does not resize to the size we gave it). We have to adjust the computed location of following controls then.
  int vertical_offset = 0;
  int horizontal_offset = 0;

  HDWP hdwp = BeginDeferWindowPos(list.Count);
  for each(ControlEntry ^ entry in list) {
      // The parameter horizontal tells us if the layout direction is horizontal or vertical.
      // Always resize orthogonal to the layout direction but resize in layout direction only if fill is set
      // or the computed size is smaller than the current size.
      ViewWrapper::remove_auto_resize(entry->control, AutoResizeMode::ResizeBoth);
      Drawing::Rectangle newBounds = entry->control->Bounds;

      if (horizontal) {
        // Workaround: usually, scaling a button vertically does not produce good results.
        // Buttons are better left at their normal height, so we switch that auto-scaling off.
        // In vertical layout the scaling is controllable by the fill flag.
        if (!is<Button>(entry->control))
          newBounds.Height = entry->bounds.Height;
        if (entry->fills || entry->bounds.Width < newBounds.Width)
          newBounds.Width = entry->bounds.Width;
      } else {
        newBounds.Width = entry->bounds.Width;
        if (entry->fills || entry->bounds.Height < newBounds.Height)
          newBounds.Height = entry->bounds.Height;
      }

      // Get the resulting control size and adjust the offset for following controls and the
      // middle alignment (for controls that don't resize).
      newBounds.X = entry->bounds.Left + horizontal_offset + (entry->bounds.Width - newBounds.Width) / 2;
      newBounds.Y = entry->bounds.Top + vertical_offset + (entry->bounds.Height - newBounds.Height) / 2;

      if (horizontal)
        horizontal_offset += newBounds.Width - entry->bounds.Width;
      else
        vertical_offset += newBounds.Height - entry->bounds.Height;

      if (hdwp == 0)
        entry->control->Bounds = newBounds;
      else {
        HDWP new_hdwp = DeferWindowPos(hdwp, (HWND)entry->control->Handle.ToPointer(), 0, newBounds.X, newBounds.Y,
                                       newBounds.Width, newBounds.Height, SWP_NOZORDER);
        if (new_hdwp == 0) {
          // Something went wrong, so finish what we started so far and continue with straight resize.
          EndDeferWindowPos(hdwp);
        }

        hdwp = new_hdwp;
      }
    }

  if (hdwp != 0)
    EndDeferWindowPos(hdwp);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the largest width of any control in both given lists.
 */
int get_largest_width(ControlList % list1, ControlList % list2) {
  int max = 0;
  for each(ControlEntry ^ entry in list1) {
      if (entry->bounds.Width > max)
        max = entry->bounds.Width;
    }
  for each(ControlEntry ^ entry in list2) {
      if (entry->bounds.Width > max)
        max = entry->bounds.Width;
    }

  return max;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the largest height of any control in both given lists.
 */
int get_largest_height(ControlList % list1, ControlList % list2) {
  int max = 0;
  for each(ControlEntry ^ entry in list1) {
      if (entry->bounds.Height > max)
        max = entry->bounds.Height;
    }
  for each(ControlEntry ^ entry in list2) {
      if (entry->bounds.Height > max)
        max = entry->bounds.Height;
    }

  return max;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called if the container is in "use all" mode and any remaining space in it is to be distributed
 * amongst all child controls that have the expand flag set.
 *
 * @param list The list of controls to expand.
 * @param fraction The amount of pixels the controls must be resized/moved.
 * @param mirrored Do the adjustment in a mirrored fashion (for right aligned controls).
 */
void expand_horizontally(ControlList % list, LayoutBox ^ box, int fraction, bool mirrored) {
  for (int i = 0; i < list.Count; i++) {
    if (box->GetControlExpands(list[i]->control)) {
      list[i]->bounds.Width += fraction;
      if (mirrored) {
        list[i]->bounds.X -= fraction;

        // Move all following controls by the same amount.
        for (int j = i + 1; j < list.Count; j++)
          list[j]->bounds.X -= fraction;
      } else {
        // Move all following controls by the same amount.
        for (int j = i + 1; j < list.Count; j++)
          list[j]->bounds.X += fraction;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Called if the container is in "use all" mode and any remaining space in it is to be distributed
 * amongst all child controls that have the expand flag set.
 *
 * @param list The list of controls to expand.
 * @param fraction The amount of pixels the controls must be resized/moved.
 * @param mirrored Do the adjustment in a mirrored fashion (for bottom aligned controls).
 */
void expand_vertically(ControlList % list, LayoutBox ^ box, int fraction, bool mirrored) {
  for (int i = 0; i < list.Count; i++) {
    if (box->GetControlExpands(list[i]->control)) {
      list[i]->bounds.Height += fraction;
      if (mirrored) {
        list[i]->bounds.Y -= fraction;

        // Move all following controls by the same amount.
        for (int j = i + 1; j < list.Count; j++)
          list[j]->bounds.Y -= fraction;
      } else {
        // Move all following controls by the same amount.
        for (int j = i + 1; j < list.Count; j++)
          list[j]->bounds.Y += fraction;
      }
    }
  }
}

//----------------- HorizontalGtkBoxLayout ---------------------------------------------------------

/**
 * Computes the entire layout of the box. This includes size and position of client controls.
 *
 * @param box The container to which this layouter belongs.
 * @param proposedSize The size to start from layouting. Since super ordinated controls may impose
 *                     a layout size we need to honor that (especially important for auto wrapping
 *                     labels).
 * @param resizeChildren Tells the function whether the computed client control bounds should be applied
 *                      (when doing a relayout) or not (when computing the preferred size).
 * @return The resulting size of the box.
 */
System::Drawing::Size HorizontalGtkBoxLayout::ComputeLayout(LayoutBox ^ box, System::Drawing::Size proposedSize,
                                                            bool resizeChildren) {
  ControlList left_aligned;
  ControlList right_aligned;

  int spacing = box->Spacing;
  int max_height = 0;
  int expand_count = 0;

  proposedSize.Width -= box->Padding.Horizontal;
  proposedSize.Height -= box->Padding.Vertical;

  // First part: setup and vertical position and size computation.
  for each(Control ^ control in box->Controls) {
      // Also here we should use reflection to get the true visibility state, but that's expensive.
      if (!control->Visible)
        continue;

      // Make a copy of the current bounds of the control.
      // This is used in the computation before we finally manipulate the control itself.
      ControlEntry ^ entry = gcnew ControlEntry();
      entry->control = control;
      entry->fills = box->GetControlFills(control);

      // Keep track of the tallest control, so we can adjust the container's height properly.
      ViewWrapper::set_full_auto_resize(control);

      bool use_min_width = ViewWrapper::use_min_width_for_layout(control);
      bool use_min_height = ViewWrapper::use_min_height_for_layout(control);
      if (use_min_width && use_min_height)
        entry->bounds.Size = control->MinimumSize;
      else {
        entry->bounds =
          System::Drawing::Rectangle(Point(0, 0), control->GetPreferredSize(Size(0, proposedSize.Height)));
        if (use_min_width)
          entry->bounds.Width = control->MinimumSize.Width;
        if (use_min_height)
          entry->bounds.Height = control->MinimumSize.Height;
      }

      // Weird behavior of combo boxes. They report a one pixel too small height (you cannot set them
      // to this smaller height, though). So this wrong value is messing up our computed overall height.
      if (is<ComboBox>(entry->control))
        entry->bounds.Height++;

      // Buttons have a very own understanding of their preferred size. Flat buttons always add 3 px padding
      // in addition to their normal Padding. System buttons ignore the image in size calculations.
      // We fix flat buttons for now as they can come with images only. Normal buttons usually have text.
      if (is<Button>(entry->control)) {
        Button ^ button = (Button ^)entry->control;
        if (button->FlatStyle == FlatStyle::Flat)
          entry->bounds = Drawing::Rectangle::Inflate(entry->bounds, -3, -3);
        else {
          // If a smaller size than the preferred size is set for a button use this instead.
          if (button->MinimumSize.Height > 0 && button->MinimumSize.Height < control->PreferredSize.Height)
            entry->bounds.Height = button->MinimumSize.Height;
        }
        if (entry->fills) {
          Button ^ btn = dynamic_cast<Button ^>(entry->control);
          btn->AutoSizeMode = AutoSizeMode::GrowAndShrink;
        }
      }

      // Sort control into the proper alignment list.
      if (control->Dock == DockStyle::Right)
        right_aligned.Add(entry);
      else
        left_aligned.Add(entry);

      // Keep track of the highest control, so we can adjust the container's height properly.
      if (max_height < entry->bounds.Height)
        max_height = entry->bounds.Height;

      // Count how many children have the expand flag set. This is needed for later computation.
      // Remove auto resizing in layout direction too, as this is mutual exclusive to expand mode.
      if (box->GetControlExpands(control))
        expand_count++;
    }

  // Adjust height of the container if it is too small or auto resizing is enabled.
  if (proposedSize.Height < max_height || ViewWrapper::can_auto_resize_vertically(box)) {
    proposedSize.Height = max_height;
    if (proposedSize.Height < box->MinimumSize.Height - box->Padding.Vertical)
      proposedSize.Height = box->MinimumSize.Height - box->Padding.Vertical;
  }

  // Go again through the child list and adjust the height of each child control as well as
  // compute their vertical position.
  maximize_children(left_aligned, true, proposedSize, box->Padding.Top);
  maximize_children(right_aligned, true, proposedSize, box->Padding.Top);

  // Second part: horizontal position and size computation.
  // We can have two special cases here: distributed and "use all" mode, but only if the container
  // is not set to auto resizing in horizontal direction (otherwise it adjusts itself to the content).
  int common_width = 0;

  int control_count = left_aligned.Count + right_aligned.Count;
  if (control_count > 0 && box->Homogeneous) {
    // In this mode we resize all controls so that they entirely fill the width of the container.
    // However, if any of the child controls has a width larger than the computed common width
    // instead use this largest width and increase the container width accordingly.
    common_width = (proposedSize.Width - (control_count - 1) * spacing) / control_count;
    int max = get_largest_width(left_aligned, right_aligned);
    if (max > common_width)
      common_width = max;
  }

  int offset = box->Padding.Left;

  int resulting_width = 0;
  for each(ControlEntry ^ entry in left_aligned) {
      // Consider either a common width or the individual widths of the controls here.
      entry->bounds.X = offset;
      if (common_width > 0)
        entry->bounds.Width = common_width;
      offset += entry->bounds.Width + spacing;
    }

  if (offset > box->Padding.Left) {
    // Remove the left padding we used for positioning. It's applied later.
    resulting_width = offset - box->Padding.Left;

    // Remove also the last spacing if there are no (visible) right aligned children.
    if (right_aligned.Count == 0)
      resulting_width -= spacing;
  }

  // For right aligned controls we first compute relative coordinates.
  offset = -box->Padding.Right;
  for each(ControlEntry ^ entry in right_aligned) {
      // Consider either a common width or the individual widths of the controls here.
      if (common_width > 0)
        entry->bounds.Width = common_width;
      entry->bounds.X = offset - entry->bounds.Width;
      offset -= entry->bounds.Width + spacing;
    }

  if (offset < -box->Padding.Right)
    // Remove one spacing we added too much above. Also remove the left padding we used for positioning.
    // The padding is applied later.
    resulting_width += -offset - spacing - box->Padding.Right;

  // Adjust width of the container if it is too small or auto resizing is enabled.
  if (proposedSize.Width < resulting_width || ViewWrapper::can_auto_resize_horizontally(box)) {
    proposedSize.Width = resulting_width;
    if (proposedSize.Width < box->MinimumSize.Width - box->Padding.Horizontal)
      proposedSize.Width = box->MinimumSize.Width - box->Padding.Horizontal;
  }

  if (resizeChildren) {
    // Distribute any free space amongst all child controls which have their expand flag set. This is
    // mutually exclusive with auto resizing in layout direction.
    // Though we don't need to test the auto resizing flag since the box's width would here already be set to
    // the resulting width we computed above if it were enabled.
    if (expand_count > 0 && proposedSize.Width > resulting_width) {
      int fraction = (proposedSize.Width - resulting_width) / expand_count;
      expand_horizontally(left_aligned, box, fraction, false);
      expand_horizontally(right_aligned, box, fraction, true);
    }

    // Compute the final position of the right aligned controls.
    proposedSize.Width += box->Padding.Horizontal;
    proposedSize.Height += box->Padding.Vertical;

    for each(ControlEntry ^ entry in right_aligned) entry->bounds.Offset(proposedSize.Width, 0);

    apply_bounds(left_aligned, true);
    apply_bounds(right_aligned, true);
  } else {
    proposedSize.Width += box->Padding.Horizontal;
    proposedSize.Height += box->Padding.Vertical;
  }

  return proposedSize;
}

//--------------------------------------------------------------------------------------------------

bool HorizontalGtkBoxLayout::Layout(Object ^ container, LayoutEventArgs ^ arguments) {
  LayoutBox ^ box = static_cast<LayoutBox ^>(container);
  if (!ViewWrapper::can_layout(box, arguments->AffectedProperty))
    return false;

  ViewWrapper::adjust_auto_resize_from_docking(box);
  System::Drawing::Size boxSize = ComputeLayout(box, box->Size, true);

  // Finally resize container if necessary.
  bool parentLayoutNeeded = !box->Size.Equals(boxSize);
  if (parentLayoutNeeded)
    ViewWrapper::resize_with_docking(box, boxSize);

  return parentLayoutNeeded;
}

//--------------------------------------------------------------------------------------------------

System::Drawing::Size HorizontalGtkBoxLayout::GetPreferredSize(LayoutBox ^ container,
                                                               System::Drawing::Size proposedSize) {
  proposedSize = ComputeLayout(container, proposedSize, false);
  return proposedSize;
}

//----------------- VerticalGtkBoxLayout ----------------------------------------------------------

/**
 * Computes the entire layout of the box. This includes size and position of client controls.
 *
 * @param box The container to which this layouter belongs.
 * @param proposedSize The size to start from layouting. Since super ordinated controls may impose
 *                     a layout size we need to honor that (especially important for auto wrapping
 *                     labels).
 * @param resizeChildren Tells the function whether the computed client control bounds should be applied
 *                       (when doing a relayout) or not (when computing the preferred size).
 * @return The resulting size of the box.
 */
System::Drawing::Size VerticalGtkBoxLayout::ComputeLayout(LayoutBox ^ box, System::Drawing::Size proposedSize,
                                                          bool resizeChildren) {
  ControlList top_aligned;
  ControlList bottom_aligned;

  int spacing = box->Spacing;
  int max_width = 0;
  int expand_count = 0;

  proposedSize.Width -= box->Padding.Horizontal;
  proposedSize.Height -= box->Padding.Vertical;

  // First part: setup vertical position and size computation.
  for each(Control ^ control in box->Controls) {
      if (!control->Visible)
        continue;

      // Make a copy of the current bounds of the control.
      // This is used in the computation before we finally manipulate the control itself.
      ControlEntry ^ entry = gcnew ControlEntry();
      entry->control = control;
      entry->fills = box->GetControlFills(control);

      // Sort control into the proper alignment list.
      if (control->Dock == DockStyle::Bottom)
        bottom_aligned.Add(entry);
      else
        top_aligned.Add(entry);

      // Keep track of the widest control, so we can adjust the container's width properly.
      ViewWrapper::set_full_auto_resize(control);

      bool use_min_width = ViewWrapper::use_min_width_for_layout(control);
      bool use_min_height = ViewWrapper::use_min_height_for_layout(control);
      if (use_min_width && use_min_height)
        entry->bounds.Size = control->MinimumSize;
      else {
        entry->bounds = System::Drawing::Rectangle(
          Point(0, 0), control->GetPreferredSize(System::Drawing::Size(proposedSize.Width, 0)));
        if (use_min_width)
          entry->bounds.Width = control->MinimumSize.Width;
        if (use_min_height)
          entry->bounds.Height = control->MinimumSize.Height;
      }

      // Weird behavior of combo boxes. They report a one pixel too small height (you cannot set them
      // to this smaller height, though). So this wrong value is messing up our computed overall height.
      if (is<ComboBox>(entry->control))
        entry->bounds.Height++;

      if (is<Button>(entry->control) && entry->fills) {
        Button ^ btn = dynamic_cast<Button ^>(entry->control);
        btn->AutoSizeMode = AutoSizeMode::GrowAndShrink;
      }

      if (max_width < entry->bounds.Width)
        max_width = entry->bounds.Width;

      // Count how many children have the expand flag set. This is needed for later computation.
      // Remove auto resizing in layout direction too, as this is mutual exclusive to expand mode.
      if (box->GetControlExpands(control))
        expand_count++;
    }

  // Adjust height of the container if it is too small or auto resizing is enabled.
  if (proposedSize.Width < max_width || ViewWrapper::can_auto_resize_horizontally(box)) {
    proposedSize.Width = max_width;
    if (proposedSize.Width < box->MinimumSize.Width - box->Padding.Horizontal)
      proposedSize.Width = box->MinimumSize.Width - box->Padding.Horizontal;
  }

  // Go again through the child list and adjust the width of each child control as well as
  // compute their vertical position. This will also determine if we need to adjust the overall
  // height, in case we have labels in the list.
  maximize_children(top_aligned, false, proposedSize, box->Padding.Left);
  maximize_children(bottom_aligned, false, proposedSize, box->Padding.Left);

  // Second part: horizontal position and size computation.
  // We can have two special cases here: distributed and "use all" mode, but only if the container
  // is not set to auto resizing in horizontal direction (otherwise it adjusts itself to the content).
  int common_height = 0;
  int control_count = top_aligned.Count + bottom_aligned.Count;
  if (control_count > 0 && box->Homogeneous) {
    // In this mode we resize all controls so that they entirely fill the width of the container.
    // However, if any of the child controls has a width larger than the computed common width
    // instead use this largest width and increase the container width accordingly.
    common_height = (proposedSize.Height - (control_count - 1) * spacing) / (control_count);
    int max = get_largest_height(top_aligned, bottom_aligned);
    if (max > common_height)
      common_height = max;
  }

  int offset = box->Padding.Top;

  int resulting_height = 0;
  for each(ControlEntry ^ entry in top_aligned) {
      // Consider either a common height or the individual widths of the controls here.
      entry->bounds.Y = offset;
      if (common_height > 0)
        entry->bounds.Height = common_height;
      offset += entry->bounds.Height + spacing;
    }

  if (offset > box->Padding.Top) {
    // Remove the top padding we used for positioning. It's applied later.
    resulting_height = offset - box->Padding.Top;

    // Remove also the last spacing if there are no bottom aligned children.
    if (bottom_aligned.Count == 0)
      resulting_height -= spacing;
  }

  // For bottom aligned controls we first compute relative coordinates.
  offset = -box->Padding.Bottom;
  for each(ControlEntry ^ entry in bottom_aligned) {
      // Consider either a common height or the individual widths of the controls here.
      if (common_height > 0)
        entry->bounds.Height = common_height;
      entry->bounds.Y = offset - entry->bounds.Height;
      offset -= entry->bounds.Height + spacing;
    }

  if (offset < -box->Padding.Bottom)
    // Remove one spacing we added too much above. Also remove the bottom padding we used for positioning.
    // The padding is applied later.
    resulting_height += -offset - spacing - box->Padding.Bottom;

  // Adjust height of the container if it is too small or auto resizing is enabled.
  if (proposedSize.Height < resulting_height || ViewWrapper::can_auto_resize_vertically(box)) {
    proposedSize.Height = resulting_height;
    if (proposedSize.Height < box->MinimumSize.Height - box->Padding.Vertical)
      proposedSize.Height = box->MinimumSize.Height - box->Padding.Vertical;
  }

  if (resizeChildren) {
    // Distribute any free space amongst all child controls which have their expand flag set. This is
    // mutually exclusive with auto resizing in layout direction.
    // Though we don't need to test the auto resizing flag since the box's width would here already be set to
    // the resulting width we computed above if it were enabled.
    if (expand_count > 0 && proposedSize.Height > resulting_height) {
      int fraction = (proposedSize.Height - resulting_height) / expand_count;
      expand_vertically(top_aligned, box, fraction, false);
      expand_vertically(bottom_aligned, box, fraction, true);
    }

    // Apply the padding value again and compute the final position of the bottom aligned controls.
    proposedSize.Width += box->Padding.Horizontal;
    proposedSize.Height += box->Padding.Vertical;

    for each(ControlEntry ^ entry in bottom_aligned) entry->bounds.Offset(0, proposedSize.Height);

    apply_bounds(top_aligned, false);
    apply_bounds(bottom_aligned, false);
  } else {
    proposedSize.Width += box->Padding.Horizontal;
    proposedSize.Height += box->Padding.Vertical;
  }

  return proposedSize;
}

//--------------------------------------------------------------------------------------------------

bool VerticalGtkBoxLayout::Layout(Object ^ container, LayoutEventArgs ^ arguments) {
  LayoutBox ^ box = static_cast<LayoutBox ^>(container);
  if (!ViewWrapper::can_layout(box, arguments->AffectedProperty))
    return false;

  ViewWrapper::adjust_auto_resize_from_docking(box);
  System::Drawing::Size boxSize = ComputeLayout(box, box->Size, true);

  // Finally resize container if necessary.
  bool parentLayoutNeeded = !box->Size.Equals(boxSize);
  if (parentLayoutNeeded)
    ViewWrapper::resize_with_docking(box, boxSize);

  return parentLayoutNeeded;
}

//-------------------------------------------------------------------------------------------------

System::Drawing::Size VerticalGtkBoxLayout::GetPreferredSize(LayoutBox ^ container,
                                                             System::Drawing::Size proposedSize) {
  return ComputeLayout(container, proposedSize, false);
}

//----------------- LayoutBox ----------------------------------------------------------------------

LayoutBox::LayoutBox() {
  SetStyle(ControlStyles::UserPaint, true);
  SetStyle(ControlStyles::AllPaintingInWmPaint, true);
  SetStyle(ControlStyles::DoubleBuffer, true);
  SetStyle(ControlStyles::SupportsTransparentBackColor, true);
  SetStyle(ControlStyles::OptimizedDoubleBuffer, true);
  UpdateStyles();

  horizontal = false;
  homogeneous = false;
  spacing = 0;
}

//--------------------------------------------------------------------------------------------------

System::Drawing::Size LayoutBox::GetPreferredSize(System::Drawing::Size proposedSize) {
  return layoutEngine->GetPreferredSize(this, proposedSize);
}

//--------------------------------------------------------------------------------------------------

void LayoutBox::OnPaintBackground(PaintEventArgs ^ args) {
  Panel::OnPaintBackground(args);

  BoxWrapper *wrapper = BoxWrapper::GetWrapper<BoxWrapper>(this);
  wrapper->DrawBackground(args);
}

//----------------- BoxWrapper -----------------------------------------------------------------------

BoxWrapper::BoxWrapper(mforms::Box *box) : ViewWrapper(box) {
}

//--------------------------------------------------------------------------------------------------

bool BoxWrapper::create(mforms::Box *backend, bool horizontal) {
  BoxWrapper *wrapper = new BoxWrapper(backend);
  LayoutBox ^ box = Create<LayoutBox>(backend, wrapper);
  box->Horizontal = horizontal;

  return true;
}

//-------------------------------------------------------------------------------------------------

void BoxWrapper::add(mforms::Box *backend, mforms::View *child, bool expand, bool fill) {
  Control ^ control = BoxWrapper::GetControl(child);
  if (backend->is_horizontal())
    control->Dock = DockStyle::Left;
  else
    control->Dock = DockStyle::Top;

  LayoutBox ^ box = BoxWrapper::GetManagedObject<LayoutBox>(backend);
  box->Add(control, expand, fill);
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void BoxWrapper::add_end(mforms::Box *backend, mforms::View *child, bool expand, bool fill) {
  Control ^ control = BoxWrapper::GetControl(child);
  if (backend->is_horizontal())
    control->Dock = DockStyle::Right;
  else
    control->Dock = DockStyle::Bottom;

  LayoutBox ^ box = BoxWrapper::GetManagedObject<LayoutBox>(backend);
  box->Add(control, expand, fill);
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void BoxWrapper::remove(mforms::Box *backend, mforms::View *child) {
  LayoutBox ^ box = BoxWrapper::GetManagedObject<LayoutBox>(backend);
  box->Remove(BoxWrapper::GetControl(child));
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void BoxWrapper::set_spacing(mforms::Box *backend, int space) {
  LayoutBox ^ box = BoxWrapper::GetManagedObject<LayoutBox>(backend);
  box->Spacing = space;
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void BoxWrapper::set_homogeneous(mforms::Box *backend, bool value) {
  LayoutBox ^ box = BoxWrapper::GetManagedObject<LayoutBox>(backend);
  box->Homogeneous = value;
  backend->set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

void BoxWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_box_impl.create = &BoxWrapper::create;
  f->_box_impl.add = &BoxWrapper::add;
  f->_box_impl.add_end = &BoxWrapper::add_end;
  f->_box_impl.remove = &BoxWrapper::remove;
  f->_box_impl.set_spacing = &BoxWrapper::set_spacing;
  f->_box_impl.set_homogeneous = &BoxWrapper::set_homogeneous;
}
