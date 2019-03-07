/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

using System.Drawing;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Drawing.Drawing2D;
using System.Drawing.Text;

namespace MySQL.Controls
{
  /// <summary>
  /// Helper class to convert text flags or to draw certain backgrounds.
  /// </summary>
  public class ToolStripHelper
  {
    public static StringFormat FlagsToStringFormat(TextFormatFlags flags)
    {
      StringFormat result = new StringFormat();

      // Horizontal Alignment.
      if ((flags & TextFormatFlags.HorizontalCenter) == TextFormatFlags.HorizontalCenter)
        result.Alignment = StringAlignment.Center;
      else
        if ((flags & TextFormatFlags.Right) == TextFormatFlags.Right)
          result.Alignment = StringAlignment.Far;
        else
          result.Alignment = StringAlignment.Near;

      // Vertical Alignment.
      if ((flags & TextFormatFlags.Bottom) == TextFormatFlags.Bottom)
        result.LineAlignment = StringAlignment.Far;
      else
        if ((flags & TextFormatFlags.VerticalCenter) == TextFormatFlags.VerticalCenter)
          result.LineAlignment = StringAlignment.Center;
        else
          result.LineAlignment = StringAlignment.Near;

      // Ellipsis.
      if ((flags & TextFormatFlags.EndEllipsis) == TextFormatFlags.EndEllipsis)
        result.Trimming = StringTrimming.EllipsisCharacter;
      else
        if ((flags & TextFormatFlags.PathEllipsis) == TextFormatFlags.PathEllipsis)
          result.Trimming = StringTrimming.EllipsisPath;
        else
          if ((flags & TextFormatFlags.WordEllipsis) == TextFormatFlags.WordEllipsis)
            result.Trimming = StringTrimming.EllipsisWord;
          else
            result.Trimming = StringTrimming.None; // StringTrimming.Character;

      // Shortcuts.
      if ((flags & TextFormatFlags.NoPrefix) == TextFormatFlags.NoPrefix)
        result.HotkeyPrefix = HotkeyPrefix.None;
      else
        if ((flags & TextFormatFlags.HidePrefix) == TextFormatFlags.HidePrefix)
          result.HotkeyPrefix = HotkeyPrefix.Hide;
        else
          result.HotkeyPrefix = HotkeyPrefix.Show;
      
      // Text Padding.
      if ((flags & TextFormatFlags.NoPadding) == TextFormatFlags.NoPadding)
        result.FormatFlags |= StringFormatFlags.FitBlackBox;

      // Text Wrapping.
      if ((flags & TextFormatFlags.SingleLine) == TextFormatFlags.SingleLine)
        result.FormatFlags |= StringFormatFlags.NoWrap;
      else
        if ((flags & TextFormatFlags.TextBoxControl) == TextFormatFlags.TextBoxControl)
          result.FormatFlags |= StringFormatFlags.LineLimit;
      
      // Other Flags.
      if ((flags & TextFormatFlags.RightToLeft) == TextFormatFlags.RightToLeft)
        result.FormatFlags |= StringFormatFlags.DirectionRightToLeft;
      if ((flags & TextFormatFlags.NoClipping) == TextFormatFlags.NoClipping)
        result.FormatFlags |= StringFormatFlags.NoClip;
      
      return result;
    }

    public static void MenuPanelPaint(Graphics g, RectangleF area)
    {
      g.SmoothingMode = SmoothingMode.HighQuality;

      // In order to perfectly draw the gradient exactly how we want it we have to use
      // inter-pixel coordinates. 
      area.Offset(-0.5f, -0.5f);

      ColorBlend blend = new ColorBlend();
      if (Conversions.InHighContrastMode())
        blend.Colors = new Color[] {
          Color.FromArgb(0xff, 0xff, 0xff, 0xff),
          Color.FromArgb(0xff, 0xff, 0xff, 0xff)
        };
      else
        blend.Colors = new Color[] {
          Color.FromArgb(0xff, 0xd6, 0xdc, 0xe5),
          Color.FromArgb(0xff, 0xc2, 0xcb, 0xda),
          Color.FromArgb(0xff, 0xad, 0xba, 0xce),
          Color.FromArgb(0xff, 0xad, 0xba, 0xce)
        };

      blend.Positions = new float[] { 0, 0.2f, 0.45f, 1 };

      using (LinearGradientBrush gradientBrush = new LinearGradientBrush(
        new RectangleF(0, 0, 1, area.Height),
        Color.Black, Color.White, LinearGradientMode.Vertical))
      {
        gradientBrush.InterpolationColors = blend;
        g.FillRectangle(gradientBrush, area);
      }

      // Bottom line
      using (Pen pen = new Pen(Color.FromArgb(0xff, 0x99, 0xa7, 0xb3)))
        g.DrawLine(pen, area.Left, area.Bottom - 0.5f, area.Right - 1, area.Bottom - 0.5f);

      // Left + Right border gradient.
      using (LinearGradientBrush gradientBrush = new LinearGradientBrush(
        new RectangleF(0, 0, 1, area.Bottom),
        Color.FromArgb(0xff, 0x92, 0x9b, 0xa2), Color.FromArgb(0xff, 0x99, 0xa7, 0xb3), LinearGradientMode.Vertical))
      {
        using (Pen pen = new Pen(gradientBrush, 1))
        {
          g.DrawLine(pen, 0, area.Top, 0, area.Bottom);
          g.DrawLine(pen, area.Right - 0.5f, area.Top, area.Right - 0.5f, area.Bottom);
        }
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  public class FlatMainToolStripRenderer : ToolStripRenderer
	{
    protected override void OnRenderToolStripBackground(ToolStripRenderEventArgs e)
    {
      using (SolidBrush brush = new SolidBrush(e.ToolStrip.BackColor))
        e.Graphics.FillRectangle(brush, e.AffectedBounds);
    }

    /// <summary>
    /// Used to render the toolstrip's grip handle.
    /// </summary>
    protected override void OnRenderGrip(ToolStripGripRenderEventArgs e)
    {
      using (Pen linePen = new Pen(Color.FromArgb(98, 113, 140), 2))
      {
        linePen.DashPattern = new float[] { 1f, 1f };

        // Top position is determined by the toolstrip's padding, the inner border and the grip's size,
        // which we shorten by 8 pixels to make it more appealing.
        Point gripStart = new Point(e.GripBounds.X, e.GripBounds.Y + e.ToolStrip.GripMargin.Top);
        Point gripEnd = new Point(gripStart.X, e.GripBounds.Y + e.GripBounds.Height - e.ToolStrip.GripMargin.Bottom);
        e.Graphics.DrawLine(linePen, gripStart, gripEnd);
      }
    }

    protected override void OnRenderSeparator(ToolStripSeparatorRenderEventArgs e)
    {
      using (Pen linePen = new Pen(Color.FromArgb(131, 146, 165)))
      {
        linePen.DashPattern = new float[] { 1f, 1f };

        Point lineStart = new Point(e.Item.Bounds.Width / 2, e.ToolStrip.Margin.Top);
        Point lineEnd = new Point(e.Item.Bounds.Width / 2, e.Item.Bounds.Height - e.ToolStrip.Margin.Bottom);
        e.Graphics.DrawLine(linePen, lineStart, lineEnd);
      }
    }

    protected override void OnRenderItemText(ToolStripItemTextRenderEventArgs e)
    {
      // In the case the renderer is used for a control sitting on an Aero glass window (Vista+)
      // we have to draw the text manually, as it would appear semi-transparently otherwise.
      Color color = Color.FromArgb(255, e.TextColor.R, e.TextColor.G, e.TextColor.B);
      using (SolidBrush brush = new SolidBrush(color))
        e.Graphics.DrawString(e.Text, e.TextFont, brush, e.TextRectangle);
    }

    protected override void OnRenderButtonBackground(ToolStripItemRenderEventArgs e)
    {
      if (e.Item.BackgroundImage != null)
        e.Graphics.DrawImageUnscaledAndClipped(e.Item.BackgroundImage,
          new Rectangle(0, 0, e.Item.Width, e.Item.Height));
    }
  }

  //------------------------------------------------------------------------------------------------

  public class FlatSubToolStripRenderer : ToolStripRenderer
  {
    protected override void OnRenderToolStripBackground(ToolStripRenderEventArgs e)
    {
      using (SolidBrush brush = new SolidBrush(e.ToolStrip.BackColor))
        e.Graphics.FillRectangle(brush, e.AffectedBounds);
    }

    /// <summary>
    /// Used to render the toolstrip's grip handle.
    /// </summary>
    protected override void OnRenderGrip(ToolStripGripRenderEventArgs e)
    {
      using (Pen linePen = new Pen(Color.FromArgb(98, 113, 140), 2))
      {
        linePen.DashPattern = new float[] { 1f, 1f };

        // Top position is determined by the toolstrip's padding, the inner border and the grip's size,
        // which we shorten by 8 pixels to make it more appealing.
        Point gripStart = new Point(e.GripBounds.X, e.GripBounds.Y + e.ToolStrip.GripMargin.Top);
        Point gripEnd = new Point(gripStart.X, e.GripBounds.Y + e.GripBounds.Height - e.ToolStrip.GripMargin.Bottom);
        e.Graphics.DrawLine(linePen, gripStart, gripEnd);
      }
    }

    protected override void OnRenderSeparator(ToolStripSeparatorRenderEventArgs e)
    {
      using (Pen linePen = new Pen(Color.FromArgb(131, 146, 165)))
      {
        Point lineStart = new Point(e.Item.Bounds.Width / 2, e.ToolStrip.GripMargin.Top);
        Point lineEnd = new Point(e.Item.Bounds.Width / 2, e.Item.Bounds.Height - e.ToolStrip.GripMargin.Bottom);
        e.Graphics.DrawLine(linePen, lineStart, lineEnd);
      }
    }

  }
  
  //------------------------------------------------------------------------------------------------

  /// <summary>
  /// A special menu strip renderer to make the background show through.
  /// </summary>
  public class TransparentMenuStripRenderer : ToolStripRenderer 
  {
    public Bitmap Logo { get; set; }

    private ColorBlend selectionGradient;

    public TransparentMenuStripRenderer()
    {
      selectionGradient = new ColorBlend();
      if (Conversions.InHighContrastMode())
      {
        selectionGradient.Colors = new Color[] {
          Color.FromArgb(0xff, 0xff, 0xff, 0xff),
          Color.FromArgb(0xff, 0xff, 0xff, 0xff)
        };

        selectionGradient.Positions = new float[] {
          0, 1
        };

      }
      else
      {
        selectionGradient.Colors = new Color[] {
          Color.FromArgb(0xff, 0xff, 0xfc, 0xf2),
          Color.FromArgb(0xff, 0xff, 0xf3, 0xcf),
          Color.FromArgb(0xff, 0xff, 0xec, 0xb5),
          Color.FromArgb(0xff, 0xff, 0xec, 0xb5)
        };

      selectionGradient.Positions = new float[] {
          0, 0.45f, 0.48f, 1
        };
      }
    }

    protected override void OnRenderMenuItemBackground(ToolStripItemRenderEventArgs e)
    {
      base.OnRenderMenuItemBackground(e);

      if (e.Item.Enabled)
      {
        bool highlight = e.Item.IsOnDropDown && e.Item.Selected;
        if (!highlight)
          highlight = e.Item.Selected || ((ToolStripMenuItem)e.Item).DropDown.Visible;

        if (highlight)
        {
          RectangleF bounds = new RectangleF(e.Item.Margin.Left + 0.5f, 0.5f,
            e.Item.Width - e.Item.Margin.Horizontal - 2, e.Item.Height - 2);

          e.Graphics.SmoothingMode = SmoothingMode.HighQuality;

          // Is it a dropped down submenu?
          if (!e.Item.IsOnDropDown && ((ToolStripMenuItem)e.Item).DropDown.Visible)
          {
            using (GraphicsPath outline = new GraphicsPath())
            {
              float cornerSize = 2;
              outline.AddLine(bounds.Left, bounds.Bottom, bounds.Left, bounds.Top + cornerSize);
              outline.AddArc(bounds.Left, bounds.Top, cornerSize, cornerSize, -180, 90);
              outline.AddArc(bounds.Right - cornerSize - 1, bounds.Top, cornerSize, cornerSize, -90, 90);
              outline.AddLine(bounds.Right - 1, bounds.Top + cornerSize, bounds.Right - 1, bounds.Bottom);

              if (Conversions.InHighContrastMode())
                e.Graphics.FillPath(Brushes.White, outline);
              else
                using (SolidBrush brush = new SolidBrush(Color.FromArgb(0xff, 0xe9, 0xec, 0xee)))
                  e.Graphics.FillPath(brush, outline);

              bounds = new RectangleF(e.Item.Margin.Left, 0, e.Item.Width - e.Item.Margin.Horizontal, e.Item.Height);
            }
            using (GraphicsPath outline = new GraphicsPath())
            {
              float cornerSize = 3;
              outline.AddLine(bounds.Left, bounds.Bottom - 1, bounds.Left, bounds.Top + cornerSize);
              outline.AddArc(bounds.Left, bounds.Top, cornerSize, cornerSize, -180, 90);
              outline.AddArc(bounds.Right - cornerSize - 2, bounds.Top, cornerSize, cornerSize, -90, 90);
              outline.AddLine(bounds.Right - 2, bounds.Bottom - 1, bounds.Right - 2, bounds.Bottom - 1);
              using (Pen pen = new Pen(Color.FromArgb(0xff, 0x9b, 0xa7, 0xb7)))
                e.Graphics.DrawPath(pen, outline);
            }
          }
          else
          {
            bounds.Inflate(-2, 0);
            if (Conversions.InHighContrastMode())
              e.Graphics.FillRectangle(Brushes.Black, bounds);
            else
            {
              using (LinearGradientBrush gradientBrush = new LinearGradientBrush(
                bounds,
                Color.Black, Color.White,
                LinearGradientMode.Vertical))
              {
                gradientBrush.InterpolationColors = selectionGradient;

                using (GraphicsPath outline = new GraphicsPath())
                {
                  float cornerSize = 2;
                  outline.AddArc(bounds.Left, bounds.Bottom - cornerSize, cornerSize, cornerSize, 90, 90);
                  outline.AddArc(bounds.Left, bounds.Top, cornerSize, cornerSize, -180, 90);
                  outline.AddArc(bounds.Right - cornerSize, bounds.Top, cornerSize, cornerSize, -90, 90);
                  outline.AddArc(bounds.Right - cornerSize, bounds.Bottom - cornerSize,
                    cornerSize, cornerSize, 0, 90);

                  e.Graphics.FillPath(gradientBrush, outline);

                  bounds = new RectangleF(e.Item.Margin.Left + 3, 0, e.Item.Width - e.Item.Margin.Horizontal - 4,
                    e.Item.Height);
                }
                using (GraphicsPath outline = new GraphicsPath())
                {
                  float cornerSize = 3;
                  outline.AddArc(bounds.Left, bounds.Bottom - cornerSize - 1, cornerSize, cornerSize, 90, 90);
                  outline.AddArc(bounds.Left, bounds.Top, cornerSize, cornerSize, -180, 90);
                  outline.AddArc(bounds.Right - cornerSize - 2, bounds.Top, cornerSize, cornerSize, -90, 90);
                  outline.AddArc(bounds.Right - cornerSize - 2, bounds.Bottom - cornerSize - 1,
                    cornerSize, cornerSize, 0, 90);
                  outline.CloseAllFigures();
                  using (Pen pen = new Pen(Color.FromArgb(0xff, 0xe5, 0xc3, 0x65)))
                    e.Graphics.DrawPath(pen, outline);
                }
              }
            }
          }
        }
      }
    }

    protected override void OnRenderToolStripBackground(ToolStripRenderEventArgs e)
    {
      if (e.ToolStrip.IsDropDown)
      {
        Color startColor = Color.FromArgb(0xff, 0xe9, 0xec, 0xee);
        Color endColor = Color.FromArgb(0xff, 0xd0, 0xd7, 0xe2);
        if (Conversions.InHighContrastMode())
          startColor = endColor = Color.White;
        using (LinearGradientBrush gradientBrush = new LinearGradientBrush(
          e.AffectedBounds, startColor, endColor,
          LinearGradientMode.Vertical))
          e.Graphics.FillRectangle(gradientBrush, e.AffectedBounds);
      }
      else
      {
        // Makes the transparent strip no longer transparent, but something with 
        // transparency is no longer working anymore.
        using (SolidBrush brush = new SolidBrush(e.ToolStrip.BackColor))
          e.Graphics.FillRectangle(brush, e.AffectedBounds);
      }

      if (Logo != null && e.ToolStrip.Dock == DockStyle.Top)
      {
        int x = e.ToolStrip.Width - Logo.Width - 8;
        int y = 6;
        e.Graphics.DrawImageUnscaled(Logo, new Point(x, y));
      }
    }

    protected override void OnRenderItemText(ToolStripItemTextRenderEventArgs e)
    {
      StringFormat format = ToolStripHelper.FlagsToStringFormat(e.TextFormat);
      format.FormatFlags |= StringFormatFlags.NoWrap;

      // In the case the renderer is used for a control sitting on an Aero glass window (Vista+)
      // we have to draw the text manually, as it would appear semi-transparently otherwise.
      Color color;
      Font font = e.TextFont;
      if (Conversions.InHighContrastMode())
      {
        bool selected = e.Item.Selected;
        if (!e.Item.IsOnDropDown && ((ToolStripMenuItem)e.Item).DropDown.Visible)
          selected = false;
        color = selected ? Color.White : Color.Black;
        if (!e.Item.Enabled)
          color = Color.Green;
        font = new Font(font, font.Style | FontStyle.Bold);
      }
      else
      {
        color = Color.Gray;
        if (e.Item.Enabled)
          color = Color.Black;
      }
      using (SolidBrush brush = new SolidBrush(color))
        e.Graphics.DrawString(e.Text, font, brush, e.TextRectangle, format);
    }

    protected override void OnRenderToolStripBorder(ToolStripRenderEventArgs e)
    {
      if (e.ToolStrip.IsDropDown)
      {
        Rectangle bounds = e.AffectedBounds;
        bounds.Width--;
        bounds.Height--;
        using (Pen pen = new Pen(Color.FromArgb(0xff, 0x9b, 0xa7, 0xb7)))
        {
          e.Graphics.DrawLine(pen, e.ConnectedArea.Right - 1, bounds.Top, bounds.Right, bounds.Top);
          e.Graphics.DrawLine(pen, bounds.Right, bounds.Top, bounds.Right, bounds.Bottom);
          e.Graphics.DrawLine(pen, bounds.Right, bounds.Bottom, bounds.Left, bounds.Bottom);
          e.Graphics.DrawLine(pen, bounds.Left, bounds.Bottom, bounds.Left, bounds.Top);
        }
      }
    }

    protected override void OnRenderSeparator(ToolStripSeparatorRenderEventArgs e)
    {
      // Currently we render only horizontal separators (as this renderer is for menus only).
      PointF start = new Point(e.Item.Margin.Left, e.Item.Height / 2 );
      PointF end = new Point(e.Item.Width - e.Item.Margin.Right, e.Item.Height / 2);
      using (Pen pen = new Pen(Color.FromArgb(0xff, 0xbe, 0xc3, 0xcb)))
        e.Graphics.DrawLine(pen, start, end);
    }

    protected override void OnRenderImageMargin(ToolStripRenderEventArgs e)
    {
      Color color = Color.FromArgb(0xff, 0xe9, 0xec, 0xee);
      if (Conversions.InHighContrastMode())
        color = Color.White;
      using (SolidBrush brush = new SolidBrush(color))
        e.Graphics.FillRectangle(brush, e.AffectedBounds);
    }

    protected override void OnRenderArrow(ToolStripArrowRenderEventArgs e)
    {
      if (Conversions.InHighContrastMode() && e.Item.Selected)
        e.ArrowColor = Color.White;
      else
        e.ArrowColor = Color.Black;
      base.OnRenderArrow(e);
    }
  }

//--------------------------------------------------------------------------------------------------

  /// <summary>
  /// A special menu strip renderer to make the background show through.
  /// </summary>
  public class Win8MenuStripRenderer : ToolStripRenderer
  {
    public Bitmap Logo { get; set; }

    public Win8MenuStripRenderer()
    {

    }

    protected override void OnRenderMenuItemBackground(ToolStripItemRenderEventArgs e)
    {
      base.OnRenderMenuItemBackground(e);

      if (e.Item.Enabled)
      {
        bool highlight = e.Item.IsOnDropDown && e.Item.Selected;
        if (!highlight)
          highlight = e.Item.Selected || ((ToolStripMenuItem)e.Item).DropDown.Visible;

        if (highlight)
        {
          RectangleF bounds = new RectangleF(0.5f, 0.5f, e.Item.Width - 1, e.Item.Height - 1);

          e.Graphics.SmoothingMode = SmoothingMode.HighQuality;

          // Simple solid background and border if dropped down.
          if (!e.Item.IsOnDropDown && ((ToolStripMenuItem)e.Item).DropDown.Visible)
          {
            using (SolidBrush brush = new SolidBrush(Color.FromArgb(0xe7, 0xe8, 0xe9)))
              e.Graphics.FillRectangle(brush, bounds);

            bounds.X = 0;
            bounds.Y = 0;

            using (GraphicsPath outline = new GraphicsPath())
            {
              outline.AddLine(bounds.Left, bounds.Bottom, bounds.Left, bounds.Top);
              outline.AddLine(bounds.Left, bounds.Top, bounds.Right, bounds.Top);
              outline.AddLine(bounds.Right, bounds.Top, bounds.Right, bounds.Bottom);
              using (Pen pen = new Pen(Color.FromArgb(0xcb, 0xcd, 0xda)))
                e.Graphics.DrawPath(pen, outline);
            }
          }
          else
          {
            if (e.Item.IsOnDropDown)
            {
              bounds.X += 3;
              bounds.Width -= 5;
            }
            using (SolidBrush brush = new SolidBrush(Color.FromArgb(0xd1, 0xe2, 0xf2)))
              e.Graphics.FillRectangle(brush, bounds);

            bounds.X -= 0.5f;
            bounds.Y = 0;
            using (Pen pen = new Pen(Color.FromArgb(0x78, 0xae, 0xe5)))
              e.Graphics.DrawRectangle(pen, bounds.X, bounds.Y, bounds.Width, bounds.Height);
          }
        }
      }
    }

    protected override void OnRenderToolStripBackground(ToolStripRenderEventArgs e)
    {
      if (e.ToolStrip.IsDropDown)
      {
        using (SolidBrush brush = new SolidBrush(Color.FromArgb(0xe7, 0xe8, 0xec)))
          e.Graphics.FillRectangle(brush, e.AffectedBounds);
      }
      else
      {
        using (SolidBrush brush = new SolidBrush(e.ToolStrip.BackColor))
          e.Graphics.FillRectangle(brush, e.AffectedBounds);
      }


      if (Logo != null && e.ToolStrip.Dock == DockStyle.Top)
      {
        int x = e.ToolStrip.Width - Logo.Width - 8;
        int y = 6;
        e.Graphics.DrawImageUnscaled(Logo, new Point(x, y));
      }
    }

    protected override void OnRenderItemText(ToolStripItemTextRenderEventArgs e)
    {
      StringFormat format = ToolStripHelper.FlagsToStringFormat(e.TextFormat);
      format.FormatFlags |= StringFormatFlags.NoWrap;

      // In the case the renderer is used for a control sitting on an Aero glass window (Vista+)
      // we have to draw the text manually, as it would appear semi-transparently otherwise.
      Color color = Color.FromArgb(0xa2, 0xa4, 0xa5);
      if (e.Item.Enabled)
        color = Color.FromArgb(0x1e, 0x1e, 0x1e);
      using (SolidBrush brush = new SolidBrush(color))
        e.Graphics.DrawString(e.Text, e.TextFont, brush, e.TextRectangle, format);
    }

    protected override void OnRenderToolStripBorder(ToolStripRenderEventArgs e)
    {
      if (e.ToolStrip.IsDropDown)
      {
        Rectangle bounds = e.AffectedBounds;
        bounds.Width--;
        bounds.Height--;
        using (Pen pen = new Pen(Color.FromArgb(0xcc, 0xce, 0xdb)))
        {
          e.Graphics.DrawLine(pen, e.ConnectedArea.Right, bounds.Top, bounds.Right, bounds.Top);
          e.Graphics.DrawLine(pen, bounds.Right, bounds.Top, bounds.Right, bounds.Bottom);
          e.Graphics.DrawLine(pen, bounds.Right, bounds.Bottom, bounds.Left, bounds.Bottom);
          e.Graphics.DrawLine(pen, bounds.Left, bounds.Bottom, bounds.Left, bounds.Top);
        }
      }
    }

    protected override void OnRenderSeparator(ToolStripSeparatorRenderEventArgs e)
    {
      // Currently we render only horizontal separators (as this renderer is for menus only).
      PointF start = new Point(e.Item.Margin.Left, e.Item.Height / 2 );
      PointF end = new Point(e.Item.Width - e.Item.Margin.Right, e.Item.Height / 2);
      using (Pen pen = new Pen(Color.FromArgb(0xcc, 0xce, 0xdb)))
        e.Graphics.DrawLine(pen, start, end);
    }

    protected override void OnRenderImageMargin(ToolStripRenderEventArgs e)
    {
      using (SolidBrush brush = new SolidBrush(Color.FromArgb(0xe7, 0xe8, 0xec)))
        e.Graphics.FillRectangle(brush, e.AffectedBounds);
    }

    protected override void OnRenderArrow(ToolStripArrowRenderEventArgs e)
    {
      e.ArrowColor = Color.FromArgb(0x00, 0x79, 0xcb);
      base.OnRenderArrow(e);
    }
  }
}
