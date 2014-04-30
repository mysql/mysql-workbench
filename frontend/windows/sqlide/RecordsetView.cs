/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

using MySQL.Base;
using MySQL.Controls;
using MySQL.GUI.Workbench;
using MySQL.Workbench;

namespace MySQL.Grt.Db
{
  public partial class RecordsetView : Form
  {
    private RecordsetWrapper model;
    private GridView gridView;
    private ColumnFilterDialog columnFilterDialog = new ColumnFilterDialog();
    private int contextColumnIndex;
    private int contextRowIndex;
   // private Timer dataSearchTimer;
    private Image columnHeaderFilterImage;

    public RecordsetView()
    {
      InitializeComponent();

    //  dataSearchTimer = new Timer();
    //  dataSearchTimer.Interval = 500;
    //  dataSearchTimer.Tick += new EventHandler(OnDataSearchApply);
    }

    public void SetupRecordset(RecordsetWrapper recordset, bool override_apply)
    {
      gridView = new GridView(recordset);
   //   gridView.Dock = DockStyle.Fill;
      gridView.BorderStyle = BorderStyle.None;
      gridView.StandardTab = false; // Let Tab move the cursor to the next cell instead next control in parent tab order.

      recordset.register_edit_actions();

      ActionList actionList = recordset.action_list;
      actionList.register_action("record_wrap_vertical", ToggleGridWrapMode);
      actionList.register_action("record_sort_asc", SortAscending);
      actionList.register_action("record_sort_desc", SortDescending);
      actionList.register_action("record_del", DeleteCurrentRecord);
      actionList.register_action("record_add", AddNewRecord);
      actionList.register_action("record_edit", EditCurrentRecord);

      if (override_apply)
        recordset.set_apply_changes(SaveChanges);
      recordset.set_update_selection_delegate(UpdateSelection);

      gridView.KeyDown += gridView_KeyDown;
      gridView.MouseDown += gridView_MouseDown;
      gridView.ColumnHeaderMouseClick += gridView_ColumnHeaderMouseClick;
      gridView.CellContextMenuStripNeeded += gridView_CellContextMenuStripNeeded;
      gridView.CellStateChanged += gridView_CellStateChanged;
      gridView.CellPainting += gridView_CellPainting;

      gridView.AdditionalColumnWidth += gridView_AdditionalColumnWidth;

      // cache icon used for marking columns with applied filters
      {
        int iconId = recordset.column_filter_icon_id();
        columnHeaderFilterImage = GrtIconManager.get_instance().get_icon(iconId);
      }

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
              model.set_field_null(new NodeId(cell.RowIndex), cell.ColumnIndex);
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
        e.ContextMenuStrip = (e.ColumnIndex == -1) ? null : columnHeaderContextMenuStrip;
        resetAllColumnFiltersToolStripMenuItem.Enabled = model.has_column_filters();
        resetColumnFilterToolStripMenuItem.Enabled = model.has_column_filter(contextColumnIndex);
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

    private void setColumnFilterToolStripMenuItem_Click(object sender, EventArgs e)
    {
      columnFilterDialog.ColumnName = model.get_column_caption(contextColumnIndex);
      columnFilterDialog.FilterExpression = model.get_column_filter_expr(contextColumnIndex);
      columnFilterDialog.Location = new Point(columnHeaderContextMenuStrip.Left, columnHeaderContextMenuStrip.Top);
      columnFilterDialog.ShowDialog();
      if (columnFilterDialog.DialogResult == DialogResult.OK)
      {
        model.set_column_filter(contextColumnIndex, columnFilterDialog.FilterExpression);
      }
    }

    private void resetColumnFilterToolStripMenuItem_Click(object sender, EventArgs e)
    {
      model.reset_column_filter(contextColumnIndex);
    }

    private void resetAllColumnFiltersToolStripMenuItem_Click(object sender, EventArgs e)
    {
      model.reset_column_filters();
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

    private int gridView_AdditionalColumnWidth(DataGridViewColumn column)
    {
      int additionalColumnWidth = 0;
      if (column.Index >= 0)
        if (model.has_column_filter(column.Index))
          additionalColumnWidth += columnHeaderFilterImage.Size.Width + 2;
      return additionalColumnWidth;
    }

    private void gridView_CellPainting(object sender, DataGridViewCellPaintingEventArgs e)
    {
      if ((0 <= e.RowIndex) || (-1 == e.ColumnIndex))
        return;

      if (!model.has_column_filter(e.ColumnIndex))
        return;

      e.PaintBackground(e.ClipBounds, true);

      Rectangle glyphRect = new Rectangle(new Point(0, 0), columnHeaderFilterImage.Size);
      glyphRect.X = e.CellBounds.Right - glyphRect.Width - 2;
      if (gridView.Columns[e.ColumnIndex].HeaderCell.SortGlyphDirection != SortOrder.None)
        glyphRect.X -= glyphRect.Width;
      if (glyphRect.X < 0)
        glyphRect.X = 0;
      glyphRect.Y = e.ClipBounds.Top + (e.ClipBounds.Height - glyphRect.Height) / 2;
      if (glyphRect.Y < 0)
        glyphRect.Y = 0;

      Rectangle contentRect = e.ClipBounds;
      if (contentRect.Width > (e.CellBounds.Width - glyphRect.Width))
        contentRect.Width = e.CellBounds.Width - glyphRect.Width;
      e.PaintContent(contentRect);
      e.Graphics.DrawImageUnscaledAndClipped(columnHeaderFilterImage, glyphRect);

      e.Handled = true;
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

    public void SaveChanges()
    {
      // Apply any pending changes in the grid before writing them to model.
      gridView.EndEdit();
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
      List<NodeId> nodes = new List<NodeId>(selectedRows.Count);
      foreach (DataGridViewRow row in selectedRows)
        nodes.Add(new NodeId(row.Index));
      if (nodes.Count == 0 && gridView.IsCurrentCellInEditMode)
        nodes.Add(new NodeId(gridView.CurrentCellAddress.Y));
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

  }
}
