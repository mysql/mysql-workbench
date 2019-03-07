/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

using MySQL.Base;
using MySQL.Controls;
using MySQL.Utilities;

namespace MySQL.Grt.Db
{
  public partial class RecordsetView : Form, IRecordsetView
  {
    private RecordsetWrapper model;
    private GridView gridView;
    private int contextColumnIndex;
    private int contextRowIndex;
   // private Timer dataSearchTimer;

    public RecordsetView()
    {
      InitializeComponent();
    }

    static public MySQL.Base.IRecordsetView create(RecordsetWrapper recordset)
    {
      RecordsetView view = new RecordsetView();
      view.SetupRecordset(recordset);
      return view;
    }

    public void SetupRecordset(RecordsetWrapper recordset)
    {
      Font font = null;
      string fontName = recordset.getFont();
      float size = recordset.getFontSize();
      if(!string.IsNullOrEmpty(fontName) && size > 0)
        font = ControlUtilities.GetFont(fontName, size);
      gridView = new GridView(recordset, font);
   //   gridView.Dock = DockStyle.Fill;
      gridView.BorderStyle = BorderStyle.None;
      gridView.StandardTab = false; // Let Tab move the cursor to the next cell instead next control in parent tab order.
      gridView.AllowAutoResizeColumns = false; // this will mess up our custom sizing/column width saving

      recordset.register_edit_actions();

      ActionList actionList = recordset.action_list;
      actionList.register_action("record_wrap_vertical", ToggleGridWrapMode);
      actionList.register_action("record_sort_asc", SortAscending);
      actionList.register_action("record_sort_desc", SortDescending);
      actionList.register_action("record_del", DeleteCurrentRecord);
      actionList.register_action("record_add", AddNewRecord);
      actionList.register_action("record_edit", EditCurrentRecord);

      recordset.set_flush_ui_changes_cb(FlushUIChanges);

      recordset.set_update_selection_delegate(UpdateSelection);

      gridView.KeyDown += gridView_KeyDown;
      gridView.MouseDown += gridView_MouseDown;
      gridView.ColumnHeaderMouseClick += gridView_ColumnHeaderMouseClick;
      gridView.CellContextMenuStripNeeded += gridView_CellContextMenuStripNeeded;
      gridView.CellStateChanged += gridView_CellStateChanged;
      gridView.ColumnWidthChanged += gridView_ColumnWidthChanged;

      Model = recordset;
    }

    public bool Dirty
    {
      get { return model.has_pending_changes(); }
    }

    public GridView GridView
    {
      get { return gridView; }
    }

    public RecordsetWrapper Model
    {
      set
      {
        if (model != null)
          model.refresh_ui_cb(null);

        gridView.Model = value;
        model = value;
        model.refresh_ui_cb(RefreshGrid);
        model.set_rows_changed(ProcessRowChange);
        //model.add_tree_refresh_handler(ProcessTreeRefresh); Called on every cell change, so it's not good for full refreshs.
        gridView.ProcessModelChange();
      }
      get { return model; }
    }

    private void RefreshGrid()
    {
      if (Model.inserts_editor())
        ProcessModelChange();
      else
        gridView.Invalidate();
      gridView.ProcessModelRowsChange();
    }

    public int ProcessModelChange()
    {
      gridView.ProcessModelChange();
      return 0;
    }

    private void ProcessRowChange()
    {
      gridView.ProcessModelRowsChange();
    }

    #region Message handling

    private void gridView_KeyDown(object sender, System.Windows.Forms.KeyEventArgs e)
    {
      switch (e.KeyCode)
      {
        case Keys.Delete:
        case Keys.Back:
          {
            if (gridView.SelectedCells.Count == 1 && !gridView.ReadOnly)
            {
              DataGridViewCell cell = gridView.SelectedCells[0];
              model.set_field_null(new NodeIdWrapper(cell.RowIndex), cell.ColumnIndex);
              gridView.InvalidateCell(cell);
            }

          }
          break;
        default:
          break;
      }
    }

    private void closeButton_Click(object sender, EventArgs e)
    {
      if (model.can_close())
        model.close(); // This will remove the record set from the backend with a notification
                       // to remove the UI (handled in SqlIdeForm.cs).
    }

    private void gridView_CellContextMenuStripNeeded(object sender, DataGridViewCellContextMenuStripNeededEventArgs e)
    {
      contextColumnIndex = e.ColumnIndex;
      contextRowIndex = e.RowIndex;
      if (e.RowIndex == -1)
      {
        e.ContextMenuStrip = (e.ColumnIndex == -1) ? null : get_column_header_right_click_menu(contextColumnIndex);
      }
      else
      {
        int column = -1;
        List<int> rows = gridView.GetSelectedRowsCol(ref column);
        e.ContextMenuStrip = model.get_context_menu(rows, column);
      }
    }

    private void gridView_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
    {
      if (e.Button == MouseButtons.Right)
      {
        DataGridView.HitTestInfo info = gridView.HitTest(e.X, e.Y);
        if (info.ColumnIndex < 0 || info.RowIndex < 0)
          return;

        DataGridViewCell cell = gridView[info.ColumnIndex, info.RowIndex];
        if (!cell.Selected)
        {
          if (Control.ModifierKeys == Keys.None)
            gridView.ClearSelection();
          cell.Selected = true;
        }
      }
    }

    private void gridView_ColumnHeaderMouseClick(object sender, DataGridViewCellMouseEventArgs e)
    {
      if (MouseButtons.Left == e.Button)
      {
        SortOrder newSortOrder;

        gridView.EndEdit();

        // By holding the (left) Alt key we remove any sorting.
        if ((ModifierKeys & Keys.Alt) != 0)
          newSortOrder = SortOrder.None;
        else
        {
          switch (gridView.Columns[e.ColumnIndex].HeaderCell.SortGlyphDirection)
          {
            case SortOrder.Ascending:
              newSortOrder = SortOrder.Descending;
              break;
            case SortOrder.Descending:
              newSortOrder = SortOrder.None;
              break;
            default:
              newSortOrder = SortOrder.Ascending;
              break;
          }
        }
        bool retaining = (newSortOrder == SortOrder.None) || (ModifierKeys & Keys.Shift) != 0;
        SortByColumn(e.ColumnIndex, newSortOrder, retaining);
      }
    }

    /// <summary>
    /// Resorts the recordset for the given column and direction.
    /// </summary>
    /// <param name="columnIndex">The index of the column to sort.</param>
    /// <param name="direction">The sort direction.</param>
    /// <param name="retaining">Indicates if existing sorts for other columns should be kept or removed.</param>
    private void SortByColumn(int columnIndex, SortOrder direction, bool retaining)
    {
      gridView.Columns[columnIndex].HeaderCell.SortGlyphDirection = direction;
      switch (direction)
      {
        case SortOrder.Ascending:
          model.sort_by(columnIndex, 1, retaining);
          break;
        case SortOrder.Descending:
          model.sort_by(columnIndex, -1, retaining);
          break;
        default:
          model.sort_by(columnIndex, 0, retaining);
        break;
      }
    }

    void gridView_CellStateChanged(object sender, DataGridViewCellStateChangedEventArgs e)
    {
      if (e.StateChanged == DataGridViewElementStates.Selected)
      {
        if (e.Cell.Selected)
        {
          contextColumnIndex = e.Cell.ColumnIndex;
          contextRowIndex = e.Cell.RowIndex;
        }
        else if (gridView.SelectedCells.Count > 0)
        {
          DataGridViewCell cell = gridView.SelectedCells[gridView.SelectedCells.Count - 1];
          contextColumnIndex = cell.ColumnIndex;
          contextRowIndex = cell.RowIndex;
        }
        else
        {
          contextColumnIndex = -1;
          contextRowIndex = -1;
        }
      }
    }

    #endregion

    #region record set manipulations

    public void RefreshRecords()
    {
      model.refresh();
    }

    public void DiscardChanges()
    {
      gridView.CancelEdit();
      model.rollback();
    }

    public void FlushUIChanges()
    {
        // Apply any pending changes in the grid before writing them to model.
        gridView.EndEdit();
    }

    public void SaveChanges()
    {
      model.apply_changes();
    }

    /// <summary>
    /// Called from the backend when the selection (record or cell) was changed.
    /// Update the gridview for this
    /// </summary>
    public void UpdateSelection()
    {
      if (model.get_column_count() == 0)
        return;

      int row = model.edited_field_row();
      int column = model.edited_field_column();
      if (row < 0 || row >= gridView.Rows.Count || column < 0 || column >= gridView.Columns.Count)
        return;
      gridView.CurrentCell = gridView[model.edited_field_column(), model.edited_field_row()];
    }

    public void ToggleGridWrapMode()
    {
      gridView.WrapMode = !gridView.WrapMode;
    }

    public void EditCurrentRecord()
    {
      if (model.get_column_count() == 0)
        return;

      // Ensure we have a row selected that we can edit.
      if (gridView.SelectedRows.Count == 0)
        gridView.SetRowSelected(0);
      gridView.BeginEdit(true);
    }

    public void AddNewRecord()
    {
      if (model.get_column_count() == 0)
        return;

      gridView.Focus();
      gridView.SetRowSelected(gridView.RowCount - 1);
      if (gridView.Columns.Count > 0)
        gridView.BeginEdit(false);
    }

    public void DeleteCurrentRecord()
    {
      if (model.get_column_count() == 0)
        return;

      List<DataGridViewRow> selectedRows = gridView.GetSelectedRows();
      List<NodeIdWrapper> nodes = new List<NodeIdWrapper>(selectedRows.Count);
      foreach (DataGridViewRow row in selectedRows)
        nodes.Add(new NodeIdWrapper(row.Index));
      if (nodes.Count == 0 && gridView.IsCurrentCellInEditMode)
        nodes.Add(new NodeIdWrapper(gridView.CurrentCellAddress.Y));
      gridView.CancelEdit();
      model.delete_nodes(nodes);
      gridView.ProcessModelRowsChange();
      gridView.Update();
    }
    
    public void SortAscending()
    {
      if (contextColumnIndex == -1)
        return;
      SortByColumn(contextColumnIndex, SortOrder.Ascending, true);
    }

    public void SortDescending()
    {
      if (contextColumnIndex == -1)
        return;
      SortByColumn(contextColumnIndex, SortOrder.Descending, true);
    }

    #endregion

    #region IRecordsetView

    public Control control()
    {
      return gridView;
    }

    private IRecordsetView.ColumnResizeCallback column_resize_callback = null;
    void gridView_ColumnWidthChanged(object sender, DataGridViewColumnEventArgs e)
    {
      if (column_resize_callback != null)
        column_resize_callback(e.Column.Index);
    }

    public void set_column_resize_callback(IRecordsetView.ColumnResizeCallback callback)
    {
      column_resize_callback = callback;
    }

    private IRecordsetView.ColumnHeaderRightClickCallback get_column_header_right_click_menu;
    public void set_column_header_right_click_callback(IRecordsetView.ColumnHeaderRightClickCallback callback)
    {
      get_column_header_right_click_menu = callback;
    }

    public int get_column_count()
    {
      return gridView.ColumnCount;
    }

    public int get_column_width(int column)
    {
      return gridView.Columns[column].Width;
    }

    public void set_column_width(int column, int width)
    {
      gridView.Columns[column].Width = width;
    }

    public void update_columns()
    {
      gridView.Refresh();
    }

    public int current_cell_row()
    {
      if (gridView.CurrentCell != null)
        return gridView.CurrentCell.RowIndex;
      return -1;
    }

    public int current_cell_column()
    {
      return gridView.CurrentCell.ColumnIndex;
    }

    public void set_current_cell(int row, int column)
    {
      gridView.CurrentCell = gridView[row, column];
    }

    
    public void set_font(String font, float size, FontStyle style)
    {
      gridView.Font = new Font(font, size, style);
      // Don't auto resize rows here, as this might be very expensive (e.g. many rows, large cell values).
      gridView.AutoResizeColumnHeadersHeight();
    }
    
    public void set_column_header_indicator(int column, IRecordsetView.ColumnHeaderIndicator order)
    {
      switch (order)
      {
      case IRecordsetView.ColumnHeaderIndicator.NoOrder:
        gridView.Columns[column].HeaderCell.SortGlyphDirection = SortOrder.None;
        break;
      case IRecordsetView.ColumnHeaderIndicator.OrderAsc:
        gridView.Columns[column].HeaderCell.SortGlyphDirection = SortOrder.Ascending;
        break;
      case IRecordsetView.ColumnHeaderIndicator.OrderDesc:
        gridView.Columns[column].HeaderCell.SortGlyphDirection = SortOrder.Descending;
        break;
      }
    }
    #endregion
  }
}
