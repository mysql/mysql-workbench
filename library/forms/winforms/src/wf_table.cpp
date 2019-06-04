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
#include "wf_table.h"
#include "wf_label.h"

using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace System::Windows::Forms;
using namespace System::Windows::Forms::Layout;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

ref class CellEntry {
public:
  Control ^ control;
  bool isVisible;
  System::Drawing::Rectangle bounds;
  int leftAttachment;
  int rightAttachment;
  int topAttachment;
  int bottomAttachment;
  bool horizontalExpand;
  bool verticalExpand;
  bool horizontalFill;
  bool verticalFill;
};

typedef List<CellEntry ^> CellList;

ref class GtkTableLayout : public LayoutEngine {
public:
  virtual bool Layout(Object ^ container, LayoutEventArgs ^ arguments) override;
};

//--------------------------------------------------------------------------------------------------

/**
 * Applies the computed bounds to each control. Adjust position if the control does not fill the given
 * space depending on the fill flag.
 *
 * @param list The list of cell entries with their bounds to be applied.
 */
void apply_bounds(CellList % list) {
  HDWP hdwp = BeginDeferWindowPos(list.Count);
  for each(CellEntry ^ entry in list) {
      if (!entry->isVisible)
        continue;

      ViewWrapper::remove_auto_resize(entry->control, AutoResizeMode::ResizeBoth);
      Drawing::Rectangle newBounds = entry->control->Bounds;

      // Resize the control to fill the available space if it is larger than that or
      // the fill flag is set.
      if (entry->horizontalFill || entry->bounds.Width < newBounds.Width)
        newBounds.Width = entry->bounds.Width;
      if (entry->verticalFill || entry->bounds.Height < newBounds.Height)
        newBounds.Height = entry->bounds.Height;

      newBounds.X = entry->bounds.Left + (entry->bounds.Width - newBounds.Width) / 2;
      newBounds.Y = entry->bounds.Top + (entry->bounds.Height - newBounds.Height) / 2;

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
 * Helper method to compare two cell entries by their column span size.
 */
int CompareColumnSpan(CellEntry ^ entry1, CellEntry ^ entry2) {
  if (entry1 == nullptr) {
    if (entry2 == nullptr)
      return 0; // Both entries are null, so they are equal.
    else
      return -1; // entry2 is greater since it is not null, but entry1 is.
  } else {
    if (entry2 == nullptr)
      return 1; // entry1 is not null, but entry2 is, so entry1 is greater.
    else {
      // Now to the real work. Both entries are valid.
      int difference =
        (entry1->rightAttachment - entry1->leftAttachment) - (entry2->rightAttachment - entry2->leftAttachment);
      return (difference > 0) ? 1 : ((difference < 0) ? -1 : 0);
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
* Helper method to compare two cell entries by their row span size.
*/
int CompareRowSpan(CellEntry ^ entry1, CellEntry ^ entry2) {
  if (entry1 == nullptr) {
    if (entry2 == nullptr)
      return 0; // Both entries are null, so they are equal.
    else
      return -1; // entry2 is greater since it is not null, but entry1 is.
  } else {
    if (entry2 == nullptr)
      return 1; // entry1 is not null, but entry2 is, so entry1 is greater.
    else {
      // Now to the real work. Both entries are valid.
      int difference =
        (entry1->bottomAttachment - entry1->topAttachment) - (entry2->bottomAttachment - entry2->topAttachment);
      return (difference > 0) ? 1 : ((difference < 0) ? -1 : 0);
    }
  }
}

//----------------- Table ---------------------------------------------------------------------------

ref class GtkTableLayout;

/**
  * Implements a GTK.table like control. In order to make the table layout working we have to derive
  * a new class which manages (and returns by request) an instance of the table layout engine.
  * It also manages some additional information which is used by the layout process.
  */
public
ref class MySQL::Forms::Table : public Panel {
private:
  GtkTableLayout ^ layoutEngine;

  CellList content;
  bool homogeneous;
  int columnSpacing;
  int rowSpacing;
  int columnCount;
  int rowCount;

public:
  Table() {
    homogeneous = false;
    columnSpacing = 0;
    rowSpacing = 0;
    columnCount = 0;
    rowCount = 0;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Computes the entire layout of the table. This includes size and position of client controls.
   *
   * @param proposedSize The size to start from layouting. Since super ordinated controls may impose
   *                     a layout size we need to honor that (especially important for auto wrapping
   *                     labels).
   * @param resizeChildren Tells the function whether the computed client control bounds should be applied
   *                       (when doing a relayout) or not (when computing the preferred size).
   * @return The resulting size of the table.
   */
  System::Drawing::Size ComputeLayout(System::Drawing::Size proposedSize, bool resizeChildren) {
    // Layouting the grid goes like this:
    // * Compute all row heights + column widths.
    // * Apply the resulting cell sizes to all attached children.
    // * Adjust the size of the container if necessary.

    // To compute all row heights and widths do:
    // 1) For each cell entry
    // 2)   Keep the expand state for all cells it covers.
    // 3)   Compute the base cell sizes for all cells it covers (e.g. for the width: take the control width and
    // distribute
    //      it evenly over all cells covered from left to right attachment).
    // 4)   Compare the cell size with what has been computed overall so far. Replace any cell size for
    //      the control which is larger than what is stored so far by that larger value.

    // If in homogeneous mode do:
    // Find the tallest row and apply its height to all other rows so they are all at the same height.
    // Similar for columns.

    // If the sum of all rows is smaller then the current container height distribute the difference evenly
    // over all children with expand flag set.
    // Similar for columns.

    // If not in homogeneous mode do:
    // 1) If the sum of all widths is smaller than the control width then distribute the remaining
    //     space over all columns for which the expand flag is set.
    // 2) Same for all rows.

    array<int> ^ heights = gcnew array<int>(RowCount);
    for (int i = 0; i < RowCount; i++)
      heights[i] = 0;
    array<int> ^ widths = gcnew array<int>(ColumnCount);
    for (int i = 0; i < ColumnCount; i++)
      widths[i] = 0;

    array<bool> ^ verticalExpandState = gcnew array<bool>(RowCount);
    for (int i = 0; i < RowCount; i++)
      verticalExpandState[i] = false;
    array<bool> ^ horizontalExpandState = gcnew array<bool>(ColumnCount);
    for (int i = 0; i < ColumnCount; i++)
      horizontalExpandState[i] = false;

    if (!HorizontalCenter)
      proposedSize.Width -= Padding.Horizontal;
    if (!VerticalCenter)
      proposedSize.Height -= Padding.Vertical;
    bool useHorizontalCentering = resizeChildren && HorizontalCenter;
    bool useVerticalCentering = resizeChildren && VerticalCenter;

    System::Drawing::Size newSize = System::Drawing::Size::Empty;

    // First round: sort list for increasing column span count, so we can process smallest entries first.
    content.Sort(gcnew Comparison<CellEntry ^>(CompareColumnSpan));

    // Go for each cell entry and apply its preferred size to the proper cells,
    // after visibility state and bounds are set.
    // Keep expand states so we can apply them later.
    for each(CellEntry ^ entry in content) {
        entry->isVisible = entry->control->Visible && (entry->rightAttachment > entry->leftAttachment) &&
                           (entry->bottomAttachment > entry->topAttachment) && !entry->control->IsDisposed &&
                           !entry->control->Disposing;
        if (!entry->isVisible)
          continue;

        // Check if the width of the entry is larger than what we have already.
        // While we are at it, keep the expand state in the associated cells.
        // However, if the current entry is expanding and covers a column which is already set to expand
        // then don't apply expansion, as we only want to expand those columns.
        int currentWidth = 0;
        bool doExpand = entry->horizontalExpand;
        if (doExpand) {
          useHorizontalCentering = false; // Expansion disables auto centering.
          for (int i = entry->leftAttachment; i < entry->rightAttachment; i++) {
            if (horizontalExpandState[i]) {
              doExpand = false;
              break;
            }
          }
        }
        for (int i = entry->leftAttachment; i < entry->rightAttachment; i++) {
          currentWidth += widths[i];
          if (doExpand)
            horizontalExpandState[i] = true;
        }

        ViewWrapper::set_full_auto_resize(entry->control);

        bool use_min_width = ViewWrapper::use_min_width_for_layout(entry->control);
        bool use_min_height = ViewWrapper::use_min_height_for_layout(entry->control);
        if (use_min_width && use_min_height)
          entry->bounds.Size = entry->control->MinimumSize;
        else {
          entry->bounds = System::Drawing::Rectangle(
            Point(0, 0), entry->control->GetPreferredSize(System::Drawing::Size(currentWidth, 0)));
          if (use_min_width)
            entry->bounds.Width = entry->control->MinimumSize.Width;
          if (use_min_height)
            entry->bounds.Height = entry->control->MinimumSize.Height;
        }

        // Weird behavior of combo boxes. They report a one pixel too small height (you cannot set them
        // to this smaller height, though). So this wrong value is messing up our computed overall height.
        if (is<ComboBox>(entry->control))
          entry->bounds.Height++;

        // Set all cells to the computed partial size if it is larger than what was found so far.
        // On the way apply the expand flag to all cells that are covered by that entry.

        // If the width of the entry is larger then distribute the difference to all cells it covers.
        if (entry->bounds.Width > currentWidth) {
          // The fraction is a per-cell value and computed by an integer div (we cannot add partial pixels).
          // Hence we might have a rest, which is less than the span size. Distribute this rest over all spanned cell
          // too.
          int fraction = (entry->bounds.Width - currentWidth) / (entry->rightAttachment - entry->leftAttachment);
          int rest = (entry->bounds.Width - currentWidth) % (entry->rightAttachment - entry->leftAttachment);

          for (int i = entry->leftAttachment; i < entry->rightAttachment; i++) {
            widths[i] += fraction;
            if (rest > 0) {
              widths[i]++;
              rest--;
            }
          }
        }
      }

    // Once we got the minimal width we need to compute the real width as the height computation depends
    // on the final column widths (e.g. for wrapping labels that change their height depending on their width).
    // Handle homogeneous mode.
    if (Homogeneous) {
      int max = 0;
      for (int i = 0; i < ColumnCount; i++)
        if (widths[i] > max)
          max = widths[i];

      for (int i = 0; i < ColumnCount; i++)
        widths[i] = max;
    }

    // Compute overall width and handle expanded entries.
    for (int i = 0; i < ColumnCount; i++)
      newSize.Width += widths[i];
    newSize.Width += (ColumnCount - 1) * ColumnSpacing;

    // Do auto sizing the table if enabled. Apply minimal bounds in any case.
    if (newSize.Width > proposedSize.Width ||
        (ViewWrapper::can_auto_resize_horizontally(this) && !useHorizontalCentering)) {
      proposedSize.Width = newSize.Width;
      if (proposedSize.Width < MinimumSize.Width - Padding.Horizontal)
        proposedSize.Width = MinimumSize.Width - Padding.Horizontal;
    }

    // Handle expansion of cells.
    if (resizeChildren) {
      if (!useHorizontalCentering && (newSize.Width < proposedSize.Width)) {
        int expandCount = 0;
        for (int i = 0; i < ColumnCount; i++)
          if (horizontalExpandState[i])
            expandCount++;
        if (expandCount > 0) {
          int fraction = (proposedSize.Width - newSize.Width) / expandCount;
          for (int i = 0; i < ColumnCount; i++)
            if (horizontalExpandState[i])
              widths[i] += fraction;
        }
      }
    }

    // Second round: Now that we have all final widths compute the heights. Start with sorting the entries
    // list for increasing row span count, so we can process smallest entries first.
    content.Sort(gcnew Comparison<CellEntry ^>(CompareRowSpan));

    // Go for each cell entry and apply its preferred size to the proper cells.
    // Keep expand states so we can apply them later.
    for each(CellEntry ^ entry in content) {
        // Visibility state and bounds where already determined in the first round.
        if (!entry->isVisible)
          continue;

        // Set all cells to the computed partial size if it is larger than what was found so far.
        // On the way apply the expand flag to all cells that are covered by that entry.

        // Check if the height of the entry is larger than what we have already.
        // Same expansion handling here as for horizontal expansion.
        int currentHeight = 0;
        bool doExpand = entry->verticalExpand;
        if (doExpand) {
          useVerticalCentering = false; // Expansion disables auto centering.
          for (int i = entry->topAttachment; i < entry->bottomAttachment; i++) {
            if (verticalExpandState[i]) {
              doExpand = false;
              break;
            }
          }
        }
        for (int i = entry->topAttachment; i < entry->bottomAttachment; i++) {
          currentHeight += heights[i];
          if (doExpand)
            verticalExpandState[i] = true;
        }

        // For controls that change height depending on their width we need another preferred size computation.
        // Currently this applies only for wrapping labels.
        if (is<WrapControlLabel>(entry->control)) {
          int currentWidth = 0;
          for (int i = entry->leftAttachment; i < entry->rightAttachment; i++)
            currentWidth += widths[i];
          entry->bounds = System::Drawing::Rectangle(
            Point(0, 0), entry->control->GetPreferredSize(System::Drawing::Size(currentWidth, 0)));
        }

        // If the height of the entry is larger then distribute the difference to all cells it covers.
        if (entry->bounds.Height > currentHeight) {
          int fraction = (entry->bounds.Height - currentHeight) / (entry->bottomAttachment - entry->topAttachment);
          int rest = (entry->bounds.Height - currentHeight) % (entry->bottomAttachment - entry->topAttachment);

          for (int i = entry->topAttachment; i < entry->bottomAttachment; i++) {
            heights[i] += fraction;
            if (rest > 0) {
              heights[i]++;
              rest--;
            }
          }
        }
      }

    // Handle homogeneous mode.
    if (Homogeneous) {
      int max = 0;
      for (int i = 0; i < RowCount; i++)
        if (heights[i] > max)
          max = heights[i];

      for (int i = 0; i < RowCount; i++)
        heights[i] = max;
    }

    // Compute overall size and handle expanded entries.
    for (int i = 0; i < RowCount; i++)
      newSize.Height += heights[i];
    newSize.Height += (RowCount - 1) * RowSpacing;

    // Do auto sizing the table if enabled. Apply minimal bounds in any case.
    if (newSize.Height > proposedSize.Height ||
        (ViewWrapper::can_auto_resize_vertically(this) && !useVerticalCentering)) {
      proposedSize.Height = newSize.Height;
      if (proposedSize.Height < MinimumSize.Height - Padding.Vertical)
        proposedSize.Height = MinimumSize.Height - Padding.Vertical;
    }

    // Handle expansion of cells (vertical case). Since this can happen only if the new size is
    // less than the proposed size it does not matter for pure size computation (in that case we
    // have already our target size). Hence do it only if we are actually layouting the table.
    if (resizeChildren) {
      if (!useVerticalCentering && (newSize.Height < proposedSize.Height)) {
        int expandCount = 0;
        for (int i = 0; i < RowCount; i++)
          if (verticalExpandState[i])
            expandCount++;
        if (expandCount > 0) {
          int fraction = (proposedSize.Height - newSize.Height) / expandCount;
          for (int i = 0; i < RowCount; i++)
            if (verticalExpandState[i])
              heights[i] += fraction;
        }
      }

      // Compute target bounds from cell sizes. Compute one more column/row used as right/bottom border.
      array<int> ^ rowStarts = gcnew array<int>(RowCount + 1);
      rowStarts[0] = Padding.Top;
      if (useVerticalCentering && (newSize.Height < proposedSize.Height))
        rowStarts[0] = (proposedSize.Height - newSize.Height) / 2;

      for (int i = 1; i <= RowCount; i++)
        rowStarts[i] = rowStarts[i - 1] + heights[i - 1] + RowSpacing;

      array<int> ^ columnStarts = gcnew array<int>(ColumnCount + 1);
      columnStarts[0] = Padding.Left;
      if (useHorizontalCentering && (newSize.Width < proposedSize.Width))
        columnStarts[0] = (proposedSize.Width - newSize.Width) / 2;

      for (int i = 1; i <= ColumnCount; i++)
        columnStarts[i] = columnStarts[i - 1] + widths[i - 1] + ColumnSpacing;

      for each(CellEntry ^ entry in content) {
          if (!entry->isVisible)
            continue;

          entry->bounds.X = columnStarts[entry->leftAttachment];
          entry->bounds.Y = rowStarts[entry->topAttachment];
          entry->bounds.Width =
            columnStarts[entry->rightAttachment] - columnStarts[entry->leftAttachment] - ColumnSpacing;
          entry->bounds.Height = rowStarts[entry->bottomAttachment] - rowStarts[entry->topAttachment] - RowSpacing;
        }

      // Apply target bounds to cell content.
      apply_bounds(content);
    }

    if (!HorizontalCenter)
      proposedSize.Width += Padding.Horizontal;
    if (!VerticalCenter)
      proposedSize.Height += Padding.Vertical;

    return proposedSize;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Returns the preferred size of the table (that is, the minimal size that can cover all content).
   *
   * @param proposedSize The base size to be used for layout. The resulting size won't usually go
   *                     below this size.
   *
   * @return The preferred size of this table.
   */
  virtual System::Drawing::Size GetPreferredSize(System::Drawing::Size proposedSize) override {
    return ComputeLayout(proposedSize, false);
  }

  //-------------------------------------------------------------------------------------------------

  void Add(Control ^ control, int left, int right, int top, int bottom, int flags) {
    if (left >= ColumnCount || right > ColumnCount)
      throw gcnew IndexOutOfRangeException("Adding control to table at an invalid column position");
    if (top >= RowCount || bottom > RowCount)
      throw gcnew IndexOutOfRangeException("Adding control to table at an invalid row position");

    ViewWrapper::set_layout_dirty(this, true);

    CellEntry ^ entry = gcnew CellEntry();
    entry->control = control;

    // Do some sanity checks here to avoid later trouble.
    // Upper limits are not checked as this can change with the number of columns,
    // even after the table was filled.
    if (left < 0)
      left = 0;
    if (left > right) {
      int temp = left;
      left = right;
      right = temp;
    }
    if (left == right)
      right++;
    if (top < 0)
      top = 0;
    if (top > bottom) {
      int temp = top;
      top = bottom;
      bottom = temp;
    }
    if (top == bottom)
      bottom++;
    entry->leftAttachment = left;
    entry->rightAttachment = right;
    entry->topAttachment = top;
    entry->bottomAttachment = bottom;
    entry->horizontalExpand = (flags & mforms::HExpandFlag) != 0;
    entry->verticalExpand = (flags & mforms::VExpandFlag) != 0;
    entry->horizontalFill = (flags & mforms::HFillFlag) != 0;
    entry->verticalFill = (flags & mforms::VFillFlag) != 0;
    content.Add(entry);     // for us
    Controls->Add(control); // for Windows
  }

  //-------------------------------------------------------------------------------------------------

  void Remove(Control ^ control) {
    ViewWrapper::set_layout_dirty(this, true);
    Controls->Remove(control);

    for each(CellEntry ^ entry in content) {
        if (entry->control == control) {
          content.Remove(entry);
          break;
        }
      }
  }

  //--------------------------------------------------------------------------------------------------

  virtual property System::Windows::Forms::Layout::LayoutEngine ^
    LayoutEngine {
      System::Windows::Forms::Layout::LayoutEngine ^ get() override {
        if (layoutEngine == nullptr)
          layoutEngine = gcnew GtkTableLayout();

        return layoutEngine;
      }
    }

    //--------------------------------------------------------------------------------------------------

    property bool Homogeneous {
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

  //--------------------------------------------------------------------------------------------------

  property int RowSpacing {
    int get() {
      return rowSpacing;
    }

    void set(int value) {
      if (rowSpacing != value) {
        ViewWrapper::set_layout_dirty(this, true);
        rowSpacing = value;
        Refresh();
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  property int ColumnSpacing {
    int get() {
      return columnSpacing;
    }

    void set(int value) {
      if (columnSpacing != value) {
        ViewWrapper::set_layout_dirty(this, true);
        columnSpacing = value;
        Refresh();
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  property int RowCount {
    int get() {
      return rowCount;
    }

    void set(int value) {
      if (rowCount != value) {
        ViewWrapper::set_layout_dirty(this, true);
        rowCount = value;
        Refresh();
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  property int ColumnCount {
    int get() {
      return columnCount;
    }

    void set(int value) {
      if (columnCount != value) {
        ViewWrapper::set_layout_dirty(this, true);
        columnCount = value;
        Refresh();
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  property bool HorizontalCenter;
  property bool VerticalCenter;
};

//----------------- GtkTableLayout ----------------------------------------------------------------

/**
  * This is the main layout method of the GTK table layout engine. It is triggered automatically
  * by the .NET framework, whenever a need appears to re-layout the tables content (usually when child controls
  * are added or removed, or when the table is resized, e.g. as part of the layout process of its parent).
  *
  * @param container The control which must be laid out.
  * @param arguments A number of arguments which are supposed to control the layouting process. Not used currently.
  *
  * @return True if the parent should re-layout itself (e.g. due to a size change of this table) or false, if not.
  */
bool GtkTableLayout::Layout(Object ^ container, LayoutEventArgs ^ arguments) {
  Table ^ table = (Table ^)container;
  if (!ViewWrapper::can_layout(table, arguments->AffectedProperty))
    return false;

  if (table->RowCount > 0 && table->ColumnCount > 0) {
    ViewWrapper::adjust_auto_resize_from_docking(table);
    System::Drawing::Size tableSize = table->ComputeLayout(table->Size, true);

    bool parentLayoutNeeded = !table->Size.Equals(tableSize);
    if (parentLayoutNeeded)
      ViewWrapper::resize_with_docking(table, tableSize);

    return parentLayoutNeeded;
  }
  return false;
}

//----------------- TableWrapper -----------------------------------------------------------------------

TableWrapper::TableWrapper(mforms::View *view) : ViewWrapper(view) {
}

//-------------------------------------------------------------------------------------------------

void TableWrapper::set_padding(int left, int top, int right, int bottom) {
  // Depending on what is specified as padding we apply a dynamic padding (centering so the content).
  Table ^ table = GetManagedObject<Table>();
  table->HorizontalCenter = (left < 0 || right < 0);
  table->VerticalCenter = (top < 0 || bottom < 0);

  ViewWrapper::set_padding(left, top, right, bottom);
}

//-------------------------------------------------------------------------------------------------

bool TableWrapper::create(mforms::Table *backend) {
  TableWrapper *wrapper = new TableWrapper(backend);
  TableWrapper::Create<Table>(backend, wrapper);
  return true;
}

//-------------------------------------------------------------------------------------------------

void TableWrapper::add(mforms::Table *backend, mforms::View *child, int left, int right, int top, int bottom,
                       int flags) {
  Table ^ table = TableWrapper::GetManagedObject<Table>(backend);
  table->Add(TableWrapper::GetControl(child), left, right, top, bottom, flags);
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void TableWrapper::remove(mforms::Table *backend, mforms::View *child) {
  Table ^ table = TableWrapper::GetManagedObject<Table>(backend);
  table->Remove(TableWrapper::GetControl(child));
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void TableWrapper::set_row_count(mforms::Table *backend, int count) {
  TableWrapper::GetManagedObject<Table>(backend)->RowCount = count;
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void TableWrapper::set_column_count(mforms::Table *backend, int count) {
  TableWrapper::GetManagedObject<Table>(backend)->ColumnCount = count;
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void TableWrapper::set_row_spacing(mforms::Table *backend, int space) {
  TableWrapper::GetManagedObject<Table>(backend)->RowSpacing = space;
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void TableWrapper::set_column_spacing(mforms::Table *backend, int space) {
  TableWrapper::GetManagedObject<Table>(backend)->ColumnSpacing = space;
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void TableWrapper::set_homogeneous(mforms::Table *backend, bool value) {
  TableWrapper::GetManagedObject<Table>(backend)->Homogeneous = value;
  backend->set_layout_dirty(true);
}

//-------------------------------------------------------------------------------------------------

void TableWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_table_impl.create = &TableWrapper::create;
  f->_table_impl.set_row_count = &TableWrapper::set_row_count;
  f->_table_impl.set_column_count = &TableWrapper::set_column_count;
  f->_table_impl.add = &TableWrapper::add;
  f->_table_impl.remove = &TableWrapper::remove;
  f->_table_impl.set_row_spacing = &TableWrapper::set_row_spacing;
  f->_table_impl.set_column_spacing = &TableWrapper::set_column_spacing;
  f->_table_impl.set_homogeneous = &TableWrapper::set_homogeneous;
}

//-------------------------------------------------------------------------------------------------
