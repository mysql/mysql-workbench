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
using System.Text.RegularExpressions;
using System.Windows.Forms;

using MySQL.Grt;
using MySQL.Utilities.Properties;

namespace MySQL.Controls
{
  public class CustomDataGridViewTextBoxColumn : DataGridViewTextBoxColumn
  {
    public delegate int AdditionalColumnWidthDelegate(DataGridViewColumn sender);
    public AdditionalColumnWidthDelegate AdditionalColumnWidth;
    public override int GetPreferredWidth(DataGridViewAutoSizeColumnMode autoSizeColumnMode, bool fixedHeight)
    {
      int preferredWidth = base.GetPreferredWidth(autoSizeColumnMode, fixedHeight);
      if (AdditionalColumnWidth != null)
        preferredWidth += AdditionalColumnWidth(this);
      return preferredWidth;
    }

    private void InitializeComponent()
    {

    }
  }

  public class GridView : DataGridView
  {
    private bool refreshing = false;
    private Bitmap fieldNullBitmap;
    private Bitmap fieldBlobBitmap;
    private bool wrapMode = false;
    private bool autoScroll = false;
    private bool allowAutoResizeColumns = true;

    // Row main and alternate colors.
    private Color gridColor = Color.FromArgb(255, 255, 255);
    private Color gridAlternateRowColor = Color.FromArgb(237, 243, 253);
    private Bitmap mainBusyIcon = Resources.busy_indicator_white;
    private Bitmap alternateBusyIcon = Resources.busy_indicator_lightblue;

    public CustomDataGridViewTextBoxColumn.AdditionalColumnWidthDelegate AdditionalColumnWidth;

    // Message types (marked in the first column). Values taken from logging backend.
    private enum MessageType
    {
      ErrorMsg,
      WarningMsg,
      NoteMsg,
      OKMsg,
      EditMsg,
      BusyMsg
    };

    protected override void Dispose(bool disposing)
    {
      if (Model != null)
      {
        Model.Dispose();
        Model = null;
      }

      // Workaround for bug in DataGridView. When being in edit mode while the grid is disposed
      // DataGridView tries to access a not existing edit control internally, which crashes.
      CancelEdit();
      if (Columns.Count > 0)
        Columns.Clear();

      base.Dispose(disposing);
    }

    public GridView(GridModelWrapper model, Font font = null)
    {
      fieldNullBitmap = Resources.field_overlay_null;
      fieldBlobBitmap = Resources.field_overlay_blob;

      if(font != null)
        setRowHeight(font);

      Model = model;

      Dock = DockStyle.Fill;
      BorderStyle = BorderStyle.None;
      BackgroundColor = SystemColors.Window;

      EditMode = DataGridViewEditMode.EditOnKeystrokeOrF2;
      VirtualMode = true;
      AllowUserToOrderColumns = true;
      AllowUserToResizeRows = false;
      ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
      ShowCellToolTips = true;
      SelectionMode = DataGridViewSelectionMode.CellSelect;

      RowHeadersWidth -= 15;

			// Set grid color
			GridColor = gridColor;
    }

    private void setRowHeight(Font font) 
    {
      using(Graphics g = CreateGraphics()) 
      {
        SizeF textSize = g.MeasureString("setRowHeight", font);
        RowTemplate.Height = (int)(Math.Ceiling(textSize.Height) + 2);
      }
    }

    public GridModelWrapper Model { get; set; }

    public void ProcessModelChange()
    {
      if (Disposing || IsDisposed)
        return;

      refreshing = true;

      ReadOnly = Model.is_readonly();

      CustomDataGridViewTextBoxColumn[] columns = new CustomDataGridViewTextBoxColumn[Model.get_column_count()];
      for (int i = 0; i < Model.get_column_count(); i++)
      {
        CustomDataGridViewTextBoxColumn column= new CustomDataGridViewTextBoxColumn();
        column.AdditionalColumnWidth = AdditionalColumnWidth;
        column.HeaderText = Model.get_column_caption(i);
        Type columnValueType;
        switch (Model.get_column_type(i))
        {
          case GridModelWrapper.ColumnType.DatetimeType:
            columnValueType = typeof(DateTime);//! needs corresponding set_field or conversion to string. see PushValue
            column.DefaultCellStyle.Alignment = DataGridViewContentAlignment.TopLeft;
            break;
          case GridModelWrapper.ColumnType.NumericType:
            columnValueType = typeof(long);
            column.DefaultCellStyle.Alignment = DataGridViewContentAlignment.TopRight;
            break;
          case GridModelWrapper.ColumnType.FloatType:
            columnValueType = typeof(double);
            column.DefaultCellStyle.Alignment = DataGridViewContentAlignment.TopRight;
            break;
          default:
            columnValueType = typeof(string);
            column.DefaultCellStyle.Alignment = DataGridViewContentAlignment.TopLeft;
            break;
        }
        if (null != columnValueType)
          column.ValueType = columnValueType;

        column.SortMode = DataGridViewColumnSortMode.Programmatic;
        column.FillWeight = 0.001F;

        columns[i]= column;
      }

      try
      {
        Columns.Clear();
      }
      catch (System.InvalidOperationException)
      {
        CancelEdit();
        Columns.Clear();
      }

      Columns.AddRange(columns);

      // restore sorting glyphs
      {
        List<int> indexes;
        List<int> orders;
        Model.sort_columns(out indexes, out orders);
        int l = 0;
        foreach (int n in indexes)
        {
          DataGridViewColumn column = Columns[n];
          column.HeaderCell.SortGlyphDirection = (orders[l] == 1) ? SortOrder.Ascending : SortOrder.Descending;
          ++l;
        }
      }

      refreshing = false;
      ProcessModelRowsChange();
    }


    public void ProcessModelRowsChange()
    {
      if (Disposing || IsDisposed)
        return;

      refreshing = true;

      RowCount = Model.count();
      if (allowAutoResizeColumns)
        AutoResizeColumns(DataGridViewAutoSizeColumnsMode.DisplayedCells, true);
      if (wrapMode)
        AutoResizeRows(DataGridViewAutoSizeRowsMode.DisplayedCells, true);

      if (AutoScroll && (RowCount > 0))
        CurrentCell = this[0, Rows.Count - 1];

      ClearSelection();
      Invalidate();
      refreshing = false;
    }


    public void SetRowSelected(int rowIndex)
    {
      if (0 == RowCount)
        return;
      DataGridViewRow row = Rows[rowIndex];
      if (0 == row.Cells.Count)
        return;
      int columnIndex = (null != CurrentCell) ? CurrentCell.ColumnIndex : 0;
      CurrentCell = row.Cells[columnIndex];

      if (0 == rowIndex)
        VerticalScrollBar.Value = VerticalScrollBar.Minimum;
      else if ((RowCount-1) == rowIndex)
        VerticalScrollBar.Value = VerticalScrollBar.Maximum;
    }

    #region Properties

    public bool AllowAutoResizeColumns
    {
      get { return allowAutoResizeColumns; }
      set { allowAutoResizeColumns = value; }
    }


    public bool WrapMode
    {
      get { return wrapMode; }
      set
      {
        if (value == wrapMode)
          return;
        wrapMode = value;
        if (wrapMode)
          RowPrePaint += new DataGridViewRowPrePaintEventHandler(OnRowPrePaint);
        else
          RowPrePaint -= OnRowPrePaint;
        DefaultCellStyle.WrapMode = wrapMode ? DataGridViewTriState.True : DataGridViewTriState.False;
      }
    }

    public bool AutoScroll
    {
      get { return autoScroll; }
      set { autoScroll = value; }
    }

    /// <summary>
    /// Determines if anything can be copied from the grid to the clipboard.
    /// </summary>
    public bool CanCopy
    {
      get { return SelectedCells.Count > 0; }
    }

    #endregion

    #region Message handling

    private void OnRowPrePaint(Object sender, DataGridViewRowPrePaintEventArgs e)
    {
      AutoResizeRow(e.RowIndex);
    }


    protected override void OnCellPainting(DataGridViewCellPaintingEventArgs e)
    {
      base.OnCellPainting(e);

      if (e.ColumnIndex < 0 || e.RowIndex < 0)
        return;

      NodeIdWrapper NodeIdWrapper = new NodeIdWrapper(e.RowIndex);
      Bitmap icon = null;
      Rectangle rect = e.CellBounds;

      if (e.ColumnIndex == 0)
      {
        // Check if we have to draw a busy indicator.
        int type;
        Model.get_field(NodeIdWrapper, 0, out type);
        if ((MessageType) type == MessageType.BusyMsg)
        {
          icon = (e.RowIndex % 2 == 1) ? Resources.busy_indicator_lightblue : Resources.busy_indicator_white;
          ImageAnimator.UpdateFrames();
          rect.X += (rect.Width - icon.Width) / 2;
          rect.Y += (rect.Height - icon.Height) / 2;
          rect.Size = icon.Size;
        }
      }

      if (icon == null)
      {
        int iconId = Model.get_field_icon(NodeIdWrapper, e.ColumnIndex, IconSize.Icon16);
        icon = IconManagerWrapper.get_instance().get_icon(iconId);
        if (icon != null)
        {
          rect.Size = icon.Size;

          // Horizontal alignment.
          switch (e.CellStyle.Alignment)
          {
            case DataGridViewContentAlignment.TopRight:
            case DataGridViewContentAlignment.MiddleRight:
            case DataGridViewContentAlignment.BottomRight:
              rect.X += e.CellBounds.Width - rect.Width;
              rect.X -= 1; // cell border width (required only for right alignment)
              break;
            case DataGridViewContentAlignment.TopCenter:
            case DataGridViewContentAlignment.MiddleCenter:
            case DataGridViewContentAlignment.BottomCenter:
              rect.X += (e.CellBounds.Width - rect.Width) / 2;
              break;
          }

          // Vertical alignment.
          switch (e.CellStyle.Alignment)
          {
            case DataGridViewContentAlignment.MiddleLeft:
            case DataGridViewContentAlignment.MiddleCenter:
            case DataGridViewContentAlignment.MiddleRight:
              rect.Y += (e.CellBounds.Height - rect.Height) / 2;
              break;
            case DataGridViewContentAlignment.BottomLeft:
            case DataGridViewContentAlignment.BottomCenter:
            case DataGridViewContentAlignment.BottomRight:
              rect.Y += e.CellBounds.Height - rect.Height;
              break;
          }
        }
      }

      if (icon != null)
      {
        e.PaintBackground(e.CellBounds, true);
        e.Graphics.DrawImageUnscaledAndClipped(icon, rect);
        e.Handled = true;
      }
    }

    protected override void OnCellFormatting(DataGridViewCellFormattingEventArgs e)
    {
      base.OnCellFormatting(e);

			// Alter background color for every odd row
      if (1 == e.RowIndex % 2)
				e.CellStyle.BackColor = gridAlternateRowColor;
    }

    protected override void OnReadOnlyChanged(EventArgs e)
    {
      base.OnReadOnlyChanged(e);

      AllowUserToAddRows = !ReadOnly;
      AllowUserToDeleteRows = !ReadOnly;
    }

    protected override void OnRowsRemoved(DataGridViewRowsRemovedEventArgs e)
    {
      base.OnRowsRemoved(e);

      if (refreshing || Model == null)
        return;
      for (int i = 0; i < e.RowCount; i++)
        Model.delete_node(new NodeIdWrapper(e.RowIndex));
    }

    protected override void OnNewRowNeeded(DataGridViewRowEventArgs e)
    {
      base.OnNewRowNeeded(e);

      if (refreshing)
        return;
    }

    protected override void OnCancelRowEdit(QuestionEventArgs e)
    {
      base.OnCancelRowEdit(e);
      if (Model != null)
        e.Response = (Model.count() < RowCount);
    }

    protected override void OnCellBeginEdit(DataGridViewCellCancelEventArgs e)
    {
      if (e.ColumnIndex >= Model.get_column_count())
      {
        e.Cancel = true;
        return;
      }

      base.OnCellBeginEdit(e);

      switch (Model.get_column_type(e.ColumnIndex))
      {
        case GridModelWrapper.ColumnType.BlobType:
          e.Cancel = true;
          break;
      }
    }

    protected override void OnCurrentCellChanged(EventArgs e)
    {
        // Do not be tempted to change this to OnSelectionChanged as
        // there sometimes the value of CurrentCell is the previous
        // value and not the new value
        base.OnCurrentCellChanged(e);

        if (Model == null || refreshing)
            return;

        if (CurrentCell != null)
            Model.set_edited_field(CurrentCell.RowIndex, CurrentCell.ColumnIndex);
        else
            Model.set_edited_field(0, 0);
    }

    protected override void OnCellValuePushed(DataGridViewCellValueEventArgs e)
    {
      base.OnCellValuePushed(e);

      if (refreshing)
        return;

      if (null == e.Value)
      {
        if (typeof(string) != Columns[e.ColumnIndex].ValueType)
        {
          Model.set_field_null(new NodeIdWrapper(e.RowIndex), e.ColumnIndex);
        }
        else
          e.Value = "";
      }

      if (null != e.Value)
      {
        Type t = e.Value.GetType();
        if (typeof(string) == t)
          Model.set_field(new NodeIdWrapper(e.RowIndex), e.ColumnIndex, (string)e.Value);
        else if (typeof(double) == t)
          Model.set_field(new NodeIdWrapper(e.RowIndex), e.ColumnIndex, (double)e.Value);
        else if (typeof(int) == t)
          Model.set_field(new NodeIdWrapper(e.RowIndex), e.ColumnIndex, (int)e.Value);
        else if (typeof(long) == t)
          Model.set_field(new NodeIdWrapper(e.RowIndex), e.ColumnIndex, (long)e.Value);
      }
    }

    protected override void OnCellValueNeeded(DataGridViewCellValueEventArgs e)
    {
      base.OnCellValueNeeded(e);

      if (Model == null)
        return;

      String value;
      Model.get_field_repr(new NodeIdWrapper(e.RowIndex), e.ColumnIndex, out value);
      e.Value = value.Replace('\n', ' ');
    }

    protected override void OnCellToolTipTextNeeded(DataGridViewCellToolTipTextNeededEventArgs e)
    {
      base.OnCellToolTipTextNeeded(e);

      if (wrapMode || (0 > e.RowIndex) || (0 > e.ColumnIndex))
        return;

      String value;
      Model.get_field_repr(new NodeIdWrapper(e.RowIndex), e.ColumnIndex, out value);

      DataGridViewCell cell = Rows[e.RowIndex].Cells[e.ColumnIndex];
      if ((cell.Size.Width < cell.PreferredSize.Width)
          || (cell.Size.Height < cell.PreferredSize.Height))
      {
        e.ToolTipText = value;
      }
      else
      {
        Regex r = new Regex("\n");
        Match m = r.Match(value);
        if (m.Success)
          e.ToolTipText = value;
      }
    }

    protected override void OnDataError(bool displayErrorDialogIfNoHandler, DataGridViewDataErrorEventArgs e)
    {
      base.OnDataError(displayErrorDialogIfNoHandler, e);

      e.Cancel = true;
    }

    protected override void OnCellLeave(DataGridViewCellEventArgs e)
    {
      base.OnCellLeave(e);

      if (IsCurrentCellInEditMode)
        EditMode = DataGridViewEditMode.EditOnEnter;
      else
        EditMode = DataGridViewEditMode.EditOnKeystrokeOrF2;
    }

    protected override void OnCellClick(DataGridViewCellEventArgs e)
    {
      // Selection mode on the grid will change depending on the grid configuration.
      // If there row headers are enabled the behavior will be dynamic:
      // - Full Row Selection: if the selected cell was the row header
      // - Cell Selection: otherwise
      // If there are no row headers the selection mode will remain as configured on the GridView
      DataGridViewSelectionMode newMode = SelectionMode;
      if (RowHeadersVisible)
      {
        if (e.ColumnIndex == -1)
          newMode = DataGridViewSelectionMode.FullRowSelect;
        else
          newMode = DataGridViewSelectionMode.CellSelect;
      }

      if (newMode == SelectionMode || e.RowIndex == -1)
        base.OnCellClick(e);
      else
      {
        // Manually handle cell clicks if we have to switch the selection mode or we need a second click.
        SelectionMode = newMode;
        if (newMode == DataGridViewSelectionMode.CellSelect)
        {
          CurrentCell = this[e.ColumnIndex, e.RowIndex];
          CurrentCell.Selected = true;
        }
        else
          Rows[e.RowIndex].Selected = true;
      }
    }

    protected override bool ProcessDataGridViewKey(KeyEventArgs e)
    {
      switch (e.KeyCode)
      {
        case Keys.Home:
          FirstDisplayedScrollingColumnIndex = 0;
          break;

        case Keys.End:
          FirstDisplayedScrollingColumnIndex = Columns.Count - 1;
          break;
          
        case Keys.F2: // Explicitly handle F2 as the grid doesn't select the cell context.
          SelectionMode = DataGridViewSelectionMode.CellSelect;
          BeginEdit(true);
          return true;
      }
      return base.ProcessDataGridViewKey(e);
    }

    protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
    {
      Keys key = keyData & Keys.KeyCode;

      if (CurrentCell != null && CurrentCell.IsInEditMode)
      {
        switch (key)
        {
          case Keys.Escape:
            CancelEdit();
            EndEdit();
            EditMode = DataGridViewEditMode.EditOnKeystrokeOrF2;
            return true;
        }
      } 
      
      return base.ProcessCmdKey(ref msg, keyData);
    }

    #endregion

    public List<NodeIdWrapper> SelectedNodes()
    {
      List<DataGridViewRow> selectedRows = GetSelectedRows();
      List<NodeIdWrapper> nodes = new List<NodeIdWrapper>(selectedRows.Count);
      foreach (DataGridViewRow row in selectedRows)
        nodes.Add(new NodeIdWrapper(row.Index));
      return nodes;
    }

    public List<DataGridViewRow> GetSelectedRows()
    {
      SortedDictionary<int, int> rowIndeces = new SortedDictionary<int, int>();
      int indecesEnd = RowCount;
      if (!ReadOnly)
        --indecesEnd;
      foreach (DataGridViewCell cell in SelectedCells)
      {
        int rowIndex = cell.RowIndex;
        if (rowIndex < indecesEnd)
          rowIndeces[rowIndex] = 0;
      }
      List<DataGridViewRow> res = new List<DataGridViewRow>(rowIndeces.Count);
      foreach (KeyValuePair<int, int> pair in rowIndeces)
        res.Add(Rows[pair.Key]);
      return res;
    }

    public List<int> GetSelectedRowsCol(ref int column)
    {
      List<DataGridViewRow> rows = GetSelectedRows();
      List<int> indexes = new List<int>(rows.Count);
      foreach (DataGridViewRow row in rows)
        indexes.Add(row.Index);

      column = (SelectedColumns.Count > 0) ? SelectedColumns[SelectedColumns.Count - 1].Index : -1;
      if (column == -1)
      {
        if (SelectedCells.Count > 0 && !SelectedCells[0].OwningRow.Selected) // At least one selected cell, not in full row selection.
        {
          // Return a column value only if all selected cells belong to the same column.
          column = SelectedCells[0].ColumnIndex;
          foreach (DataGridViewCell cell in SelectedCells)
            if (cell.ColumnIndex != column)
            {
              column = -1;
              break;
            }
        }
      }
      return indexes;
    }

    /// <summary>
    /// Copies the currently selected cells to the clipboard.
    /// </summary>
    public void Copy()
    {
      IDataObject data = GetClipboardContent();
      Clipboard.SetDataObject(data);
    }

  }
}
