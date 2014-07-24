using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Drawing;
using System.Windows.Forms.VisualStyles;
using System.Diagnostics;
using System.Drawing.Drawing2D;
using Aga.Controls.Tree.NodeControls;

namespace Aga.Controls.Tree
{
	public partial class TreeViewAdv
	{
		private void CreatePens()
		{
			CreateLinePen();
			CreateMarkPen();
      CreateGridPen(); // by mikel
		}

		private void CreateMarkPen()
		{
			GraphicsPath path = new GraphicsPath();
			path.AddLines(new Point[] { new Point(0, 0), new Point(1, 1), new Point(-1, 1), new Point(0, 0) });
			CustomLineCap cap = new CustomLineCap(null, path);
			cap.WidthScale = 1.0f;

			_markPen = new Pen(_dragDropMarkColor, _dragDropMarkWidth);
			_markPen.CustomStartCap = cap;
			_markPen.CustomEndCap = cap;
		}

		private void CreateLinePen()
		{
			_linePen = new Pen(_lineColor);
			_linePen.DashStyle = DashStyle.Dot;
		}

    // by mikel
    private void CreateGridPen()
    {
      _gridPen = new Pen(_gridColor);
    }

    protected override void OnPaint(PaintEventArgs e)
        {
            BeginPerformanceCount();

            DrawContext context = new DrawContext();
            context.Graphics = e.Graphics;
            e.Graphics.SmoothingMode = SmoothingMode.HighQuality;
            context.Font = this.Font;
            context.Enabled = Enabled;

            int y = 0;
            int gridHeight = 0;

            if (UseColumns)
            {
				DrawColumnHeaders(e.Graphics);
                y += ColumnHeaderHeight;
              // ml: this optimization doesn't work with only small invalid parts.
                if (Columns.Count == 0 /*|| e.ClipRectangle.Height <= y*/)
                    return;
            }

			int firstRowY = _rowLayout.GetRowBounds(FirstVisibleRow).Y;
            y -= firstRowY;

            e.Graphics.ResetTransform();
            e.Graphics.TranslateTransform(-OffsetX, y);
            Rectangle displayRect = DisplayRectangle;
            for (int row = FirstVisibleRow; row < RowCount; row++)
            {
                Rectangle rowRect = _rowLayout.GetRowBounds(row);
                rowRect.Width = displayRect.Width; // by mikel
                gridHeight += rowRect.Height;
                if (rowRect.Y + y > displayRect.Bottom)
                    break;
                else
                    DrawRow(e, ref context, row, rowRect);
            }

			if ((GridLineStyle & GridLineStyle.Vertical) == GridLineStyle.Vertical)
				DrawVerticalGridLines(e.Graphics, firstRowY);

			if (_dropPosition.Node != null && DragMode && HighlightDropPosition)
                DrawDropMark(e.Graphics);

            e.Graphics.ResetTransform();
            DrawScrollBarsBox(e.Graphics);

            if (DragMode && _dragBitmap != null)
                e.Graphics.DrawImage(_dragBitmap, PointToClient(MousePosition));

            EndPerformanceCount(e);
        }

		protected virtual void DrawRow(PaintEventArgs e, ref DrawContext context, int row, Rectangle rowRect)
		{
			TreeNodeAdv node = RowMap[row];
			context.DrawSelection = DrawSelectionMode.None;
			context.CurrentEditorOwner = _currentEditorOwner;
			if (DragMode)
			{
				if ((_dropPosition.Node == node) && _dropPosition.Position == NodePosition.Inside && HighlightDropPosition)
					context.DrawSelection = DrawSelectionMode.Active;
			}
			else
			{
				if (node.IsSelected && Focused)
					context.DrawSelection = DrawSelectionMode.Active;
				else if (node.IsSelected && !Focused && !HideSelection)
					context.DrawSelection = DrawSelectionMode.Inactive;
			}
			context.DrawFocus = Focused && CurrentNode == node;

      // custom draw by mikel
      Rectangle focusRect = new Rectangle(OffsetX, rowRect.Y, DisplayRectangle.Width, rowRect.Height);
      context.Bounds = focusRect;
      OnBeforeNodeDrawing(node, context);

      if (FullRowSelect)
			{
				context.DrawFocus = false;
				if (context.DrawSelection == DrawSelectionMode.Active || context.DrawSelection == DrawSelectionMode.Inactive)
				{
          // Handle high contrast mode separately.
          if (SystemInformation.HighContrast)
          {
            // ml: use the highlight color of the system scheme in high contrast mode.
            using (SolidBrush brush = new SolidBrush(SystemColors.Highlight))
              e.Graphics.FillRectangle(brush, focusRect);
          }
          else
          {
            RectangleF bounds = focusRect;
            bounds.X += 1.5f;
            bounds.Y -= 0.5f;
            bounds.Width -= 2;
            bounds.Height--;

            float cornerSize = 5;
            GraphicsPath fillPath = new GraphicsPath();
            if (isWin8OrAbove)
              fillPath.AddRectangle(bounds);
            else
            {
              fillPath.AddArc(bounds.Left, bounds.Top, cornerSize, cornerSize, 180, 90);
              fillPath.AddArc(bounds.Right - cornerSize, bounds.Top, cornerSize, cornerSize, -90, 90);
              fillPath.AddArc(bounds.Right - cornerSize, bounds.Bottom - cornerSize, cornerSize, cornerSize, 0, 90);
              fillPath.AddArc(bounds.Left, bounds.Bottom - cornerSize, cornerSize, cornerSize, 90, 90);
              fillPath.CloseAllFigures();
            }

            GraphicsPath outlinePath = new GraphicsPath();
            bounds.X -= 0.5f;
            bounds.Y += 0.5f;
            if (isWin8OrAbove)
              outlinePath.AddRectangle(bounds);
            else
            {
              outlinePath.AddArc(bounds.Left, bounds.Top, cornerSize, cornerSize, 180, 90);
              outlinePath.AddArc(bounds.Right - cornerSize, bounds.Top, cornerSize, cornerSize, -90, 90);
              outlinePath.AddArc(bounds.Right - cornerSize, bounds.Bottom - cornerSize, cornerSize, cornerSize, 0, 90);
              outlinePath.AddArc(bounds.Left, bounds.Bottom - cornerSize, cornerSize, cornerSize, 90, 90);
              outlinePath.CloseAllFigures();
            }

            if (context.DrawSelection == DrawSelectionMode.Active)
            {
              if (isWin8OrAbove)
              {
                using (SolidBrush brush = new SolidBrush(Color.FromArgb(0xFF, 0xD1, 0xE8, 0xFF)))
                  e.Graphics.FillPath(brush, fillPath);
                using (Pen pen = new Pen(Color.FromArgb(255, 0x6E, 0xC0, 0xE7)))
                  e.Graphics.DrawPath(pen, outlinePath);
              }
              else
              {
                using (LinearGradientBrush gradientBrush = new LinearGradientBrush(
                  new PointF(0, bounds.Top),
                  new PointF(0, bounds.Bottom),
                  Color.FromArgb(255, 0xF1, 0xF7, 0xFE),
                  Color.FromArgb(255, 0xCF, 0xE4, 0xFE)))
                  e.Graphics.FillPath(gradientBrush, fillPath);

                using (Pen pen = new Pen(Color.FromArgb(255, 0x83, 0xAC, 0xDD)))
                  e.Graphics.DrawPath(pen, outlinePath);
              }

              context.DrawSelection = DrawSelectionMode.FullRowSelect;
            }
            else
            {
              if (isWin8OrAbove)
              {
                using (SolidBrush brush = new SolidBrush(Color.FromArgb(0xFF, 0xF7, 0xF7, 0xF7)))
                  e.Graphics.FillPath(brush, fillPath);
                using (Pen pen = new Pen(Color.FromArgb(0xFF, 0xDE, 0xDE, 0xDE)))
                  e.Graphics.DrawPath(pen, outlinePath);
              }
              else
              {
                using (LinearGradientBrush gradientBrush = new LinearGradientBrush(
                new PointF(0, bounds.Top),
                new PointF(0, bounds.Bottom),
                Color.FromArgb(255, 0xF8, 0xF8, 0xF8),
                Color.FromArgb(255, 0xE5, 0xE5, 0xE5)))
                  e.Graphics.FillPath(gradientBrush, fillPath);

                using (Pen pen = new Pen(Color.FromArgb(255, 0xD9, 0xD9, 0xD9)))
                  e.Graphics.DrawPath(pen, outlinePath);
              }

              context.DrawSelection = DrawSelectionMode.None;
            }
          }
        }
			}
      
      // by mikez, mikel
      if ((GridLineStyle & GridLineStyle.Horizontal) == GridLineStyle.Horizontal)
        e.Graphics.DrawLine(_gridPen, 0, rowRect.Bottom, e.Graphics.ClipBounds.Right, rowRect.Bottom);

			if (ShowLines)
				DrawLines(e.Graphics, node, rowRect);

			DrawNode(node, context);

      // ml: added for overlay images.
      OnAfterNodeDrawing(node, context);
		}

		private void DrawVerticalGridLines(Graphics gr, int y)
		{
			int x = 0;
			foreach (TreeColumn c in Columns)
			{
				x += c.Width;

        // by mikel
        gr.DrawLine(_gridPen, x - 1, y, x - 1, gr.ClipBounds.Bottom);
			}
		}

		private void DrawColumnHeaders(Graphics gr)
		{
			ReorderColumnState reorder = Input as ReorderColumnState;
			int x = 0;
			TreeColumn.DrawBackground(gr, new Rectangle(0, 0, ClientRectangle.Width + 2, ColumnHeaderHeight - 1), false, false);
			gr.TranslateTransform(-OffsetX, 0);
			foreach (TreeColumn c in Columns)
			{
				if (c.IsVisible)
				{
					Rectangle rect = new Rectangle(x, 0, c.Width, ColumnHeaderHeight - 1);
					gr.SetClip(rect);
					bool pressed = ((Input is ClickColumnState || reorder != null) && ((Input as ColumnState).Column == c));
					c.Draw(gr, rect, Font, pressed, _hotColumn == c);
					gr.ResetClip();

					if (reorder != null && reorder.DropColumn == c)
						TreeColumn.DrawDropMark(gr, rect);

                    x += c.Width;
				}
			}

			if (reorder != null)
			{
				if (reorder.DropColumn == null)
					TreeColumn.DrawDropMark(gr, new Rectangle(x, 0, 0, ColumnHeaderHeight));
				gr.DrawImage(reorder.GhostImage, new Point(reorder.Location.X +  + reorder.DragOffset, reorder.Location.Y));
			}
		}

		public virtual void DrawNode(TreeNodeAdv node, DrawContext context)
		{
			foreach (NodeControlInfo item in GetNodeControls(node))
			{
				context.Bounds = item.Bounds;
				context.Graphics.SetClip(context.Bounds);
				item.Control.Draw(node, context);
				context.Graphics.ResetClip();
			}
		}

		private void DrawScrollBarsBox(Graphics gr)
		{
			Rectangle r1 = DisplayRectangle;
			Rectangle r2 = ClientRectangle;
			gr.FillRectangle(SystemBrushes.Control,
				new Rectangle(r1.Right, r1.Bottom, r2.Width - r1.Width, r2.Height - r1.Height));
		}

		private void DrawDropMark(Graphics gr)
		{
			if (_dropPosition.Position == NodePosition.Inside)
				return;

			Rectangle rect = GetNodeBounds(_dropPosition.Node);
			int right = DisplayRectangle.Right - LeftMargin + OffsetX;
			int y = rect.Y;
			if (_dropPosition.Position == NodePosition.After)
				y = rect.Bottom;
			gr.DrawLine(_markPen, rect.X, y, right, y);
		}

		private void DrawLines(Graphics gr, TreeNodeAdv node, Rectangle rowRect)
		{
			if (UseColumns && Columns.Count > 0)
				gr.SetClip(new Rectangle(0, rowRect.Y, Columns[0].Width, rowRect.Bottom));

			TreeNodeAdv curNode = node;
			while (curNode != _root && curNode != null)
			{
				int level = curNode.Level;
				int x = (level - 1) * _indent + NodePlusMinus.ImageSize / 2 + LeftMargin;
				int width = NodePlusMinus.Width - NodePlusMinus.ImageSize / 2;
				int y = rowRect.Y;
				int y2 = y + rowRect.Height;

				if (curNode == node)
				{
					int midy = y + rowRect.Height / 2;
					gr.DrawLine(_linePen, x, midy, x + width, midy);
					if (curNode.NextNode == null)
						y2 = y + rowRect.Height / 2;
				}

				if (node.Row == 0)
					y = rowRect.Height / 2;
				if (curNode.NextNode != null || curNode == node)
					gr.DrawLine(_linePen, x, y, x, y2);

				curNode = curNode.Parent;
			}

			gr.ResetClip();
		}

		#region Performance

		private float _totalTime;
		private int _paintCount;

		[Conditional("PERF_TEST")]
		private void BeginPerformanceCount()
		{
			_paintCount++;
			TimeCounter.Start();
		}

		[Conditional("PERF_TEST")]
		private void EndPerformanceCount(PaintEventArgs e)
		{
			float time = TimeCounter.Finish();
			_totalTime += time;
			string debugText = string.Format("FPS {0:0.0}; Avg. FPS {1:0.0}",
				1 / time, 1 / (_totalTime / _paintCount));
			e.Graphics.DrawString(debugText, Control.DefaultFont, Brushes.Gray,
				new PointF(DisplayRectangle.Width - 150, DisplayRectangle.Height - 20));
		}
		#endregion

	}
}
