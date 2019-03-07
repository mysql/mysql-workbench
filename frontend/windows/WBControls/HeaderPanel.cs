/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

using MySQL.Utilities;

namespace MySQL.Controls
{
  public class HeaderPanel : System.Windows.Forms.Panel
  {
    /// <summary> 
	  /// Required designer variable.
	  /// </summary>
	  private System.ComponentModel.Container components = null;

    #region Construction and destruction

    public HeaderPanel()
	  {
		  // This call is required by the Windows.Forms Form Designer.
		  InitializeComponent();

		  // Some special styles for the control.
		  SetStyle(ControlStyles.UserPaint, true);
		  SetStyle(ControlStyles.AllPaintingInWmPaint, true);
		  SetStyle(ControlStyles.DoubleBuffer, true);
		  SetStyle(ControlStyles.SupportsTransparentBackColor, true);
		  SetStyle(ControlStyles.OptimizedDoubleBuffer, true);
      UpdateStyles();

      Padding = new Padding(0, 24, 0, 0);
      BackColor = Color.FromArgb(0xff, 0x49, 0x61, 0x84);
      ForeColor = Color.White;
    }

    #endregion

    #region Drawing the control and its parts.

    override protected void OnPaintBackground(PaintEventArgs e)
    {
      SolidBrush brush;
      if (ControlUtilities.IsHierarchyFocused(this))
        brush = new SolidBrush(HeaderColorFocused);
      else
        brush = new SolidBrush(HeaderColor);
      e.Graphics.FillRectangle(brush, e.ClipRectangle);
      brush.Dispose();
    }
    
    override protected void OnPaint(PaintEventArgs e)
    {
      base.OnPaint(e);
      DrawControl(e.Graphics);
    }

	  internal void DrawControl(Graphics g)
	  {
      RectangleF bounds = ClientRectangle;
      bounds.Height = Padding.Top;
      bounds.X = headerPadding.Left;
      bounds.Y = headerPadding.Top;
      bounds.Width -= headerPadding.Horizontal;
      bounds.Height -= headerPadding.Vertical;

      if (Text != null && Text.Length > 0)
      {
        using (StringFormat stringFormat = new StringFormat())
        {
          stringFormat.Alignment = StringAlignment.Near;
          stringFormat.LineAlignment = StringAlignment.Center;

          Brush brush;
          if (ControlUtilities.IsHierarchyFocused(this))
            brush = new SolidBrush(ForeColorFocused);
          else
            brush = new SolidBrush(ForeColor);
          g.DrawString(Text, Font, brush, bounds, stringFormat);
          brush.Dispose();

          // Exclude text area from bounds rectangle (for the pattern);
          SizeF size = g.MeasureString(Text, Font);
          bounds.X += (float)Math.Ceiling(size.Width) + 4;
          bounds.Width -= (float)Math.Ceiling(size.Width) + 4;
        }

        // As patter we draw 3 lines with specific dash pattern.
        Color patternColor;
        if (ControlUtilities.IsHierarchyFocused(this))
          patternColor = Color.FromArgb(75, ForeColorFocused);
        else
          patternColor = Color.FromArgb(0x99, 0x99, 0x99);
        using (Pen linePen = new Pen(patternColor, 1))
        {
          linePen.DashPattern = new float[] { 1f, 3f };

          PointF start = new PointF(bounds.Left + 2, bounds.Height / 2f - 2);
          PointF end = new PointF(bounds.Right, bounds.Height / 2f - 2);

          // Ensure we end with outer line's dots.
          int patternCount = 2 + (int)(end.X - start.X) / 4;
          start.X = end.X - 4 * patternCount + 3;

          g.DrawLine(linePen, start, end);
          start.Y += 4;
          end.Y += 4;
          g.DrawLine(linePen, start, end);

          start = new PointF(start.X + 2, start.Y - 2);
          end = new PointF(end.X - 2, end.Y - 2);
          g.DrawLine(linePen, start, end);
        }
      }
    }

    #endregion

    #region Event handling

    protected override void OnEnter(EventArgs e)
    {
      base.OnEnter(e);
      Invalidate();
    }

    protected override void OnLeave(EventArgs e)
    {
      base.OnLeave(e);
      Invalidate();
    }

    protected override void OnHandleCreated(EventArgs e)
    {
      base.OnHandleCreated(e);

      Form container = Application.OpenForms[0].TopLevelControl as Form;
      if (container != null)
      {
        container.Deactivate += new EventHandler(ActivationChanged);
        container.Activated += new EventHandler(ActivationChanged);
      }
    }

    protected override void OnHandleDestroyed(EventArgs e)
    {
      base.OnHandleDestroyed(e);

      // Unregister our activation listener from the application's main form.
      // This assumes the main form hasn't changed in the meantime (which should be the case in 99.99% of all cases).
      if (Application.OpenForms.Count > 0)
      {
        Form container = Application.OpenForms[0].TopLevelControl as Form;
        if (container != null)
        {
          container.Deactivate -= ActivationChanged;
          container.Activated -= ActivationChanged;
        }
      }
    }

    private void ActivationChanged(object sender, EventArgs e)
    {
      Invalidate();
    }

    #endregion

    #region Component Designer generated code

    /// <summary>
	  /// Required method for Designer support - do not modify 
	  /// the contents of this method with the code editor.
	  /// </summary>
	  private void InitializeComponent()
	  {
		  components = new System.ComponentModel.Container();
	  }

	  #endregion

    #region Properties

    private Padding headerPadding = new Padding(5, 0, 0, 0);
    public Padding HeaderPadding
    {
      get { return headerPadding; }
      set
      {
        headerPadding = value;
        Invalidate();
      }
    }

    private Color headerColor = Color.FromArgb(255, 73, 97, 132);
    public Color HeaderColor
    {
      get { return headerColor; }
      set
      {
        if (headerColor != value)
        {
          headerColor = value;
          Invalidate();
        }
      }
    }

    private Color headerColorFocused = Color.FromArgb(255, 73, 97, 132);
    public Color HeaderColorFocused
    {
      get { return headerColorFocused; }
      set
      {
        if (headerColorFocused != value)
        {
          headerColorFocused = value;
          Invalidate();
        }
      }
    }

    private Color foreColorFocused = Color.White;
    public Color ForeColorFocused
    {
      get { return foreColorFocused; }
      set
      {
        if (foreColorFocused != value)
        {
          foreColorFocused = value;
          Invalidate();
        }
      }
    }

    [EditorBrowsable(EditorBrowsableState.Always)]
    [Bindable(true)]
    [Browsable(true)]
    public override string Text { get; set; }

    #endregion
    
    #region Accessibility support

    protected override AccessibleObject CreateAccessibilityInstance()
    {
      return new HeaderPanelAccessibleObject();
    }

    private class HeaderPanelAccessibleObject : AccessibleObject
    {
      public override AccessibleRole Role
      {
        get { return AccessibleRole.Pane; }
      }

      public override String Name
      {
        get { return "HaderPanel"; }
      }
    }

    #endregion
  }
}
