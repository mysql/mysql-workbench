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
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;
using System.Windows.Forms;

using MySQL.Utilities;
using MySQL.Utilities.SysUtils;

namespace MySQL.Controls
{
	/// <summary>
	/// Summary description for FlatTabControl.
	/// </summary>
	[ToolboxBitmap(typeof(System.Windows.Forms.TabControl))]

  public class FlatTabControl : System.Windows.Forms.TabControl
	{
    // Summary: determines position and look of the tabs.
    public enum TabStyleType
    {
      NoTabs,         // Don't show any tabs.
      TopNormal,      // Tabs standing on the content above the top line.
      BottomNormal,   // Tabs hanging down below the content.
      TopTransparent, // Tabs standing on the content, like TopNormal. Different colors, though.
                      // Background of space not covered by a tab is transparent. Different outline too.
    }

    public enum CloseButtonVisiblity
    {
      ShowButton,
      HideButton,
      InheritVisibility,
    }

    private const String tabRelayoutMessageName = "WindowsForms12_TabBaseReLayout";
    private uint tabRelayoutMessageID;

    // TabInfo keeps all necessary layout information.
    private class TabInfo
    {
      internal TabPage page;
      internal bool isValid;
      internal CloseButtonVisiblity closeButtonVisibility; // For individual close buttons.
      internal bool isBusy; // Show a busy indicator is to be shown for that tab (in place of the close button).
      internal Rectangle tabArea;
      internal Rectangle buttonArea;

      internal TabInfo()
      {
        this.page = null;
        isValid = false;
        closeButtonVisibility = CloseButtonVisiblity.InheritVisibility;
      }
    }

    /// <summary> 
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

    private ImageList leftRightImages = null;
    private Bitmap darkCloseButton = null;
    private Bitmap lightCloseButton = null;
    private Dictionary<string, Bitmap> busyIndicators = new Dictionary<string, Bitmap>();
    private SubClass scroller = null;
    private Form activationTracker = null;

    internal Control auxView = null;

    // For top-transparent there is no tab background color.
    private Color topTransparentTabColor = Color.White;
    private Color topTransparentTabTextColor = Color.Black;

    private Color topNormalTabTextColor = Color.Black;
    private Color topNormalTabSelectedFocusedColor = Color.Blue;
    private Color topNormalTabSelectedFocusedTextColor = Color.White;
    private Color topNormalTabSelectedUnfocusedColor = Color.Gray;
    private Color topNormalTabSelectedUnfocusedTextColor = Color.White;

    private Color unselectedTabColor = Color.Gray; // Normal top + bottom style. Also for uncovered areas.

    private Color bottomTabTextColor = Color.Black;
    private Color bottomTabSelectedColor = Color.White;
    private Color bottomTabSelectedTextColor = Color.Black;

    private int glowSize = 12; // The size of the glow behind text on glass tabs.

    private bool showCloseButton = true; // Central flag for close buttons. Can be overridden by each page.
    private bool showFocusState = true;
    private bool hideWhenEmpty = false;

    // Painting/Layouting
    private TabStyleType tabStyle = TabStyleType.TopNormal;
    private List<TabInfo> layoutInfo = new List<TabInfo>();
    private int scrollOffset;
    private int pendingScrollIntoViewIndex = -1; // Set when scrollIntoView was called while we were minimized.

    // Some layout constants.
    private const int buttonSpacing = 5; // Number of pixels between text and close button.
    private const int scrollerSpacing = 5; // Distance of last tab and the scroller when scrolled to the end.
    private const int scrollScaleFactor = 5; // Number of pixel we scroll on a single scroll click.

    // The padding within the tabs.
    private Padding itemPadding = new Padding(6, 0, 6, 0);

    // The padding around the tab pages in the tab control.
    private Padding contentPadding = new Padding();

    // Maximum tab size.
    private int maxTabSize = 200;

    // Mouse handling.
    private int lastTabHit = -1;
    private Point lastClickPosition;
    private bool buttonHit = false;
    
    // Tab Switch Shortcut
    private bool useDefaultTabSwitchKey = true;

    #region Construction and destruction

    public FlatTabControl()
		{
			// This call is required by the Windows.Forms Form Designer.
			InitializeComponent();

			// Some special styles for the control.
			SetStyle(ControlStyles.UserPaint, true);
			SetStyle(ControlStyles.AllPaintingInWmPaint, true);
			SetStyle(ControlStyles.DoubleBuffer, true);
			//SetStyle(ControlStyles.ResizeRedraw, true); // Doesn't really work. Need Invalidate() call in OnResize too.
			SetStyle(ControlStyles.SupportsTransparentBackColor, true);
			SetStyle(ControlStyles.OptimizedDoubleBuffer, true);
      UpdateStyles();

      BackgroundColor = Color.FromKnownColor(KnownColor.Control);
			leftRightImages = new ImageList();

			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(FlatTabControl));
			Bitmap icons = ((System.Drawing.Bitmap)(resources.GetObject("TabIcons")));
			if (icons != null)
			{
				icons.MakeTransparent(Color.White);
				leftRightImages.Images.AddStrip(icons);
			}

      // Both buttons really should have the same size.
      darkCloseButton = ((System.Drawing.Bitmap)(resources.GetObject("tab_close_dark")));
      lightCloseButton = ((System.Drawing.Bitmap)(resources.GetObject("tab_close_light")));

      CanReorderTabs = false;
      AllowDrop = false;
    }

    protected override void OnCreateControl()
    {
      base.OnCreateControl();

      FindScroller();

      // Hacker alarm: in order to allow proper scroller customization/setup we have to listen to
      //               a message registered by the tab control, which is obviously doing the scroller
      //               (up/down) setup. We need this to do our own setup. Regular Windows messages
      //               like WM_WINDOWPOSCHANGED are not sufficient.
      tabRelayoutMessageID = Win32.RegisterWindowMessage(tabRelayoutMessageName);
    }

    /// <summary> 
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose(bool disposing)
		{
			if (disposing)
			{
				if (components != null)
				{
					components.Dispose();
				}

				leftRightImages.Dispose();
        darkCloseButton.Dispose();
        lightCloseButton.Dispose();

        foreach (Bitmap image in busyIndicators.Values)
        {
          // Must be the same delegate we used to start the animation with!
          ImageAnimator.StopAnimate(image, new EventHandler(BusyAnimationStep));
        }
      }
			base.Dispose(disposing);
		}

    #endregion

    #region Native Windows code needed for some adjustments

    protected override void WndProc(ref System.Windows.Forms.Message m)
    {
      switch ((WM) m.Msg)
      {
        case (WM)TCM.ADJUSTRECT:
        {
          // We need to adjust the display rectangle, which directly determines where the content is shown.
          Win32.RECT rectangle = (Win32.RECT) m.GetLParam(typeof(Win32.RECT));

          // The "larger" value indicates the direction of the computation.
          // A 1 means from display rectangle to window rectangle. A 0 the other way around.
          int topOffset = 0;
          if (tabStyle == TabStyleType.TopNormal || tabStyle == TabStyleType.TopTransparent)
            topOffset = ItemSize.Height;
          int bottomOffset = 0;
          if (tabStyle == TabStyleType.BottomNormal)
            bottomOffset = ItemSize.Height;

          int larger = m.WParam.ToInt32();
          if (larger == 0)
          {
            rectangle.Left += Margin.Left + contentPadding.Left;
            rectangle.Right -= Margin.Right + contentPadding.Right;
            rectangle.Top += Margin.Top + ContentPadding.Top + topOffset;
            rectangle.Bottom -= Margin.Bottom + ContentPadding.Bottom + bottomOffset;
          }
          else
          {
            rectangle.Left -= Margin.Left + contentPadding.Left;
            rectangle.Right += Margin.Right + contentPadding.Right;
            rectangle.Top -= Margin.Top + contentPadding.Top + topOffset;
            rectangle.Bottom += Margin.Bottom + contentPadding.Bottom + bottomOffset;
          }

          Marshal.StructureToPtr(rectangle, m.LParam, true); 

          break;
        }

        case WM.HSCROLL:
          switch ((SB) Win32.LoWord(m.WParam.ToInt32()))
          {
            case SB.THUMBPOSITION:
              scrollOffset = -scrollScaleFactor * Win32.HiWord(m.WParam.ToInt32());
              Invalidate();
              break;
          }
          m.Result = new IntPtr(1);
          break;

        case WM.WINDOWPOSCHANGED:
          base.WndProc(ref m);
          UpdateScroller();
          if (tabStyle == TabStyleType.BottomNormal)
            AdjustLayoutInfo(null);
          break;

        case WM.SIZE:
          base.WndProc(ref m);
          if (m.WParam.ToInt32() == Win32.SIZE_RESTORED && m.LParam.ToInt32() != 0)
          {
            // Window has been resized.
            if (pendingScrollIntoViewIndex > -1)
              ScrollIntoView(pendingScrollIntoViewIndex);
            pendingScrollIntoViewIndex = -1;
          }
          break;

        case WM.NCHITTEST:
        {
          // Declare everything as being part of the client area, so we get mouse events for that.
          m.Result = new IntPtr(1); // HT_CLIENT
          break;
        }

        case WM.LBUTTONDOWN:
        {
          // Handle this message directly to keep the base class from doing unwanted things.
          uint lparam = (uint)m.LParam.ToInt32();
          int x = Win32.LoWord(lparam);
          int y = Win32.HiWord(lparam);
          MouseEventArgs args = new MouseEventArgs(MouseButtons.Left, 1, x, y, 0);
          OnMouseDown(args);
          m.Result = IntPtr.Zero;

          break;
        }
        default:
          base.WndProc(ref m);

          // The tab relayout message is a message registered by the original tab control
          // and obviously the one where the tab control also sets the position of the scroller.
          // Since we have our own view of how that should be done we listen to this message to
          // make our own adjustments.
          if (tabRelayoutMessageID != 0 && m.Msg == tabRelayoutMessageID)
            UpdateScroller();
          break;
      }
    }

    private int ScrollerWndProc(ref Message m)
    {
      switch ((WM) m.Msg)
      {
        case WM.ERASEBKGND:
          m.Result = new IntPtr(1);
          break;
        
        case WM.PAINT:
          {
            IntPtr hDC = Win32.GetDC(m.HWnd);
            Graphics g = Graphics.FromHdc(hDC);
            DrawScroller(g);
            g.Dispose();
            Win32.ReleaseDC(m.HWnd, hDC);

            Win32.RECT clientArea = new Win32.RECT();
            Win32.GetClientRect(m.HWnd, ref clientArea);
            Win32.ValidateRect(m.HWnd, ref clientArea);

            // return 0 (processed)
            m.Result = IntPtr.Zero;

          }
          return 1;

        case (WM) UDM.SETRANGE:
          // The systab control sends this message when it wants to update the scroller, however
          // that works diametrically opposed to our own intents, so it is simply switched off here.
          // Our implementation uses UDM_SETRANGE32.
          m.Result = new IntPtr(1);
          return 1;

      }

      return 0;
    }

    protected void RecalculateFrame()
    {
      // Trigger a resize event which will make the base control send the recalculate message.
      Width += 1;
      Width -= 1;
    }

    #endregion

    #region Drawing the control and its parts.

    /// <summary>
    /// Load drawing colors once application has finished setup or the user changed the color scheme.
    /// </summary>
    public void UpdateColors()
    {
      topTransparentTabColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainTab, false);
      topTransparentTabTextColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainTab, true);

      topNormalTabTextColor = Conversions.GetApplicationColor(ApplicationColor.AppColorTabUnselected, true);
      topNormalTabSelectedFocusedColor = Conversions.GetApplicationColor(ApplicationColor.AppColorTopTabSelectedFocused, false);
      topNormalTabSelectedFocusedTextColor = Conversions.GetApplicationColor(ApplicationColor.AppColorTopTabSelectedFocused, true);
      topNormalTabSelectedUnfocusedColor = Conversions.GetApplicationColor(ApplicationColor.AppColorTopTabSelectedUnfocused, false);
      topNormalTabSelectedUnfocusedTextColor = Conversions.GetApplicationColor(ApplicationColor.AppColorTopTabSelectedUnfocused, true);

      unselectedTabColor = Conversions.GetApplicationColor(ApplicationColor.AppColorTabUnselected, false);

      bottomTabTextColor = Conversions.GetApplicationColor(ApplicationColor.AppColorTabUnselected, true);
      bottomTabSelectedColor = Conversions.GetApplicationColor(ApplicationColor.AppColorBottomTabSelected, false);
      bottomTabSelectedTextColor = Conversions.GetApplicationColor(ApplicationColor.AppColorBottomTabSelected, true);

      Invalidate();
    }

    protected override void OnPaintBackground(PaintEventArgs e)
    {
      // Do not erase the background. We either use a transparent background or completely
      // draw it in the DrawControl function.
    }

    protected override void OnPaint(PaintEventArgs e)
	{
      base.OnPaint(e); 
  			
      DrawControl(e.Graphics);
	}

		internal void DrawControl(Graphics g)
		{
			if (!Visible)
				return;

      g.SmoothingMode = SmoothingMode.AntiAlias;

      bool drawFocused = showFocusState && ControlUtilities.IsHierarchyFocused(this);
      RectangleF clientArea = ClientRectangle;
      clientArea.Offset(-0.5f, -0.5f);

			// Fill client area with the control's background color
      // (everything outside the content rectangle, e.g. including margin).
      // Exclude the transparent part if TopTransparent is set as tab style (though only if
      // we are running on Aero).
      float topSpace = 0;
      if (tabStyle == TabStyleType.TopTransparent && ControlUtilities.IsCompositionEnabled())
      {
        topSpace = ItemSize.Height + Margin.Top;
        clientArea.Y += topSpace;
        clientArea.Height -= topSpace;
      }

      if (BackgroundColor != Color.Transparent)
      {
        using (Brush brush = new SolidBrush(BackgroundColor))
          g.FillRectangle(brush, clientArea);
      }

      // Fill the tab control area within margins. We might have already excluded the top part.
      clientArea.X += Margin.Left;
      clientArea.Width -= Margin.Horizontal;
      if (topSpace == 0)
      {
        clientArea.Y += Margin.Top;
        clientArea.Height -= Margin.Vertical;
      }
      else
        clientArea.Height -= Margin.Bottom;

      using (SolidBrush brush = new SolidBrush(unselectedTabColor))
        g.FillRectangle(brush, clientArea);

      // Some additional handling for certain tab styles.
      switch (tabStyle)
      {
        case TabStyleType.TopNormal:
          {
            // Draw the separator line.
            if (SelectedTab != null)
            {
              clientArea.Y += ItemSize.Height;
              clientArea.Height = ContentPadding.Top;
              Color color;
              if (drawFocused)
                color = topNormalTabSelectedFocusedColor;
              else
                color = topNormalTabSelectedUnfocusedColor;
              using (SolidBrush brush = new SolidBrush(color))
                g.FillRectangle(brush, clientArea);
            }

            break;
          }
      }

      if (TabPages.Count == 0 || tabStyle == TabStyleType.NoTabs)
        return;

      // Draw only tabs which lie at least partially in the visible area.
      Rectangle clipRect = new Rectangle(Margin.Left, Margin.Top, ClientSize.Width - Margin.Horizontal,
        ItemSize.Height);
      if (tabStyle == TabStyleType.BottomNormal)
        clipRect.Y = ClientSize.Height - Margin.Bottom - ItemSize.Height;
      g.SetClip(clipRect);

      int leftBorder = Margin.Left - scrollOffset;
      float rightBorder = clientArea.Width - Margin.Horizontal;

      if (SelectedIndex >= 0)
      {
        // Draw in two rounds. The selected tab first and then all others.
        // Each tab drawing excludes its drawn area from the clip region so it can't be overdrawn.
        ValidateTab(SelectedIndex);
        if (layoutInfo[SelectedIndex].tabArea.Right > leftBorder || layoutInfo[SelectedIndex].tabArea.Left < rightBorder)
          DrawTab(g, SelectedIndex, drawFocused);
      }
      for (int i = 0; i < TabCount; i++)
      {
        if (i != SelectedIndex)
        {
          ValidateTab(i);
          if (layoutInfo[i].tabArea.Right > leftBorder || layoutInfo[i].tabArea.Left < rightBorder)
            DrawTab(g, i, drawFocused);
        }
      }
    }

    internal void DrawTab(Graphics g, int index, bool drawFocused)
    {
	    RectangleF bounds = GetTabRect(index);

      TabPage page = TabPages[index];
      bool isSelected = (SelectedIndex == index);

      // For high quality drawing with antialiased lines we need to specify line and gradient coordinates
      // which lie between two pixels (not exactly *on* one, so adjust by a half pixel offset for these cases.
      bounds.X -= 0.5f;
      bounds.Y -= 0.5f;

      GraphicsPath outline = null;
      if (isSelected || tabStyle == TabStyleType.TopTransparent)
      {
        outline = GetTabOutline(bounds, isSelected);
        Color tabColor;

        switch (tabStyle)
        {
          case TabStyleType.BottomNormal:
            tabColor = bottomTabSelectedColor;
            break;

          case TabStyleType.TopTransparent:
            if (isSelected)
              tabColor = topTransparentTabColor;
            else
              tabColor = Color.FromArgb(128, topTransparentTabColor);
            break;

          default: // Top normal.
            if (isSelected)
            {
              if (drawFocused)
                tabColor = topNormalTabSelectedFocusedColor;
              else
                tabColor = topNormalTabSelectedUnfocusedColor;
            }
            else
              tabColor = topNormalTabTextColor;
            break;
        }

        using (SolidBrush brush = new SolidBrush(tabColor))
          g.FillPath(brush, outline);
      }

      bool drawingOnGlass = (tabStyle == TabStyleType.TopTransparent && !isSelected &&
                             ControlUtilities.IsCompositionEnabled());

      // Tab icon. Compute position here but draw after the text (to draw it over the text glow
      // if necessary).
      bool drawImage = false;
      Point imagePosition = new Point(0, 0);
      if ((page.ImageIndex >= 0) && (ImageList != null) && (ImageList.Images.Count > page.ImageIndex) &&
        (ImageList.Images[page.ImageIndex] != null))
      {
        drawImage = true;
        int iconSpacing = 4;

        imagePosition = new Point((int) bounds.X + itemPadding.Left, (int) bounds.Y);
				
        int adjustment = ImageList.ImageSize.Width + iconSpacing;

        imagePosition.Y += ((int)bounds.Height - ImageList.ImageSize.Height) / 2;
        bounds.X += adjustment;
        bounds.Width -= adjustment;
      }

      bool buttonSpaceNeeded = IsCloseButtonVisible(index) || (layoutInfo[index].isBusy);
      Rectangle buttonRect = GetTabButtonRect(index);
      
      // Tab text.
      if (page.Text.Length > 0)
      {
        using (StringFormat stringFormat = new StringFormat())
        {
          stringFormat.Alignment = StringAlignment.Near;
          stringFormat.Trimming = StringTrimming.EllipsisCharacter;
          stringFormat.FormatFlags = StringFormatFlags.NoWrap;
          stringFormat.LineAlignment = StringAlignment.Center;

          // Create format flags variant of the StringFormat for Aero rendering.
          TextFormatFlags formatFlags = TextFormatFlags.EndEllipsis | TextFormatFlags.VerticalCenter |
            TextFormatFlags.SingleLine;

          Brush brush;
          switch (tabStyle)
          {
            case TabStyleType.TopTransparent:
              brush = new SolidBrush(topTransparentTabTextColor);
              break;

            case TabStyleType.BottomNormal:
              if (isSelected)
                brush = new SolidBrush(bottomTabSelectedTextColor);
              else
                brush = new SolidBrush(bottomTabTextColor);
              break;

            default: // Top tab normal.
              if (isSelected)
              {
                if (drawFocused)
                  brush = new SolidBrush(topNormalTabSelectedFocusedTextColor);
                else
                  brush = new SolidBrush(topNormalTabSelectedUnfocusedTextColor);

              }
              else
                brush = new SolidBrush(topNormalTabTextColor);
              break;
          }

          int buttonPart = 0;
          if (buttonSpaceNeeded)
            buttonPart = buttonSpacing + buttonRect.Width;

          // When computing the left padding take the necessary glow around text into account, so we
          // don't add too much extra space.
          if (drawingOnGlass)
          {
            int leftSpace = itemPadding.Left - (renderWithGlow ? glowSize : 0) + 1;
            int rightSpace = itemPadding.Right > glowSize ? itemPadding.Right - glowSize : 0;
            bounds.X += leftSpace;
            bounds.Width -= buttonPart + leftSpace + rightSpace;
          }
          else
          {
            bounds.X += itemPadding.Left;
            bounds.Width -= itemPadding.Horizontal + buttonPart;
          }
          bounds.Height -= itemPadding.Vertical;

          Font font = this.Font;
          if (Conversions.InHighContrastMode())
            font = new Font(font, font.Style | FontStyle.Bold);

          // For writing text on a (semi) transparent surface (e.g. when using Aero)
          // normal text output routines will mess up the ClearType/anti aliasing extra pixels
          // or the text color (sometimes both together). Hence we use a path to draw the text.
          if (tabStyle != TabStyleType.TopTransparent || isSelected)
          {
            // Since GraphicsPath.AddString renders text significantly different we use it only
            // if we really need to.
            g.DrawString(page.Text, font, brush, bounds, stringFormat);
          }
          else
          {
            if (ControlUtilities.IsCompositionEnabled())
            {
              Rectangle intBounds = Rectangle.Ceiling(bounds);
              Win32.DrawTextOnGlass(g, page.Text, font, intBounds, Color.Black, formatFlags, renderWithGlow);
            }
            else
              using (GraphicsPath textPath = new GraphicsPath())
              {
                float emSize = g.DpiY * font.SizeInPoints / 72;
                if (tabStyle == TabStyleType.TopTransparent)
                  bounds.Y--; // Account for outline border.
                textPath.AddString(page.Text, font.FontFamily, (int)font.Style, emSize, bounds, stringFormat);
                g.FillPath(brush, textPath);
              }
          }
          brush.Dispose();
        }
      }

      if (drawImage)
        g.DrawImageUnscaled(ImageList.Images[page.ImageIndex], imagePosition);

      // Finally the close button if this tab is active (or we use the TopTransparent style or
      // the tab is marked as busy).
      if ((isSelected || tabStyle == TabStyleType.TopTransparent || layoutInfo[index].isBusy) &&
        buttonSpaceNeeded)
      {
        if (layoutInfo[index].isBusy)
        {
          Bitmap indicator;
          switch (tabStyle)
          {
            case TabStyleType.TopNormal:
              if (isSelected)
              {
                if (drawFocused)
                  indicator = GetBusyIndicatorForStyle("blue");
                else
                  indicator = GetBusyIndicatorForStyle("white");
              }
              else
                indicator = GetBusyIndicatorForStyle("white");
              break;

            default:
              indicator = GetBusyIndicatorForStyle("white");
              break;
          }
          ImageAnimator.UpdateFrames(indicator);
          g.DrawImageUnscaled(indicator, buttonRect);
        }
        else
        {
          switch (tabStyle)
          {
            case TabStyleType.TopTransparent:
              g.DrawImageUnscaled(darkCloseButton, buttonRect);
              break;
            case TabStyleType.TopNormal:
              {
                bool drawDark = isSelected && drawFocused;
                if (Conversions.UseWin8Drawing())
                  drawDark = false;
                else
                  if (Conversions.InHighContrastMode())
                    drawDark = !drawDark;
                if (drawDark)
                  g.DrawImageUnscaled(darkCloseButton, buttonRect);
                else
                  g.DrawImageUnscaled(lightCloseButton, buttonRect);
                break;
              }
            case TabStyleType.BottomNormal:
              g.DrawImageUnscaled(darkCloseButton, buttonRect);
              break;
          }
        }
      }
      if (outline != null)
      {
        // The Widen operation discards the inner part, so we need to compose the result.
        GraphicsPath inner = (GraphicsPath)outline.Clone();
        Pen pen = new Pen(Color.White, 1.5f);
        outline.Widen(pen);
        outline.AddPath(inner, true);
        g.ExcludeClip(new Region(outline));

        inner.Dispose();
        outline.Dispose();
        pen.Dispose();
      }
		}

    /// <summary>
    /// Creates the outline of a tab depending on its style.
    /// </summary>
    /// <param name="bounds">The bounding rectangle for the tab.</param>
    /// <returns>The outline of the tab as path, ready to be filled.</returns>
    private GraphicsPath GetTabOutline(RectangleF recBounds, bool isSelected)
    {
      GraphicsPath path = new GraphicsPath();
      float cornerSize = 6.5f;

      switch (tabStyle)
      {
        case TabStyleType.TopNormal:
          path.AddLine(recBounds.Left, recBounds.Bottom, recBounds.Left, recBounds.Top);
          path.AddLine(recBounds.Left, recBounds.Top, recBounds.Right, recBounds.Top);
          path.AddLine(recBounds.Right, recBounds.Top, recBounds.Right, recBounds.Bottom);
          path.AddLine(recBounds.Right, recBounds.Bottom, recBounds.Left, recBounds.Bottom);
          break;

        case TabStyleType.BottomNormal:
          path.AddLine(recBounds.Left, recBounds.Top, recBounds.Right, recBounds.Top);
          path.AddLine(recBounds.Right, recBounds.Top, recBounds.Right, recBounds.Bottom);
          path.AddLine(recBounds.Right, recBounds.Bottom, recBounds.Left, recBounds.Bottom);
          path.AddLine(recBounds.Left, recBounds.Bottom, recBounds.Left, recBounds.Top);
          break;

        case TabStyleType.TopTransparent:
          float distance = isSelected ? 0 : 1;
          path.AddLine(recBounds.Left, recBounds.Bottom - distance, recBounds.Left, recBounds.Top + cornerSize);
          path.AddArc(recBounds.Left, recBounds.Top, cornerSize, cornerSize, 180, 90);
          path.AddArc(recBounds.Right - cornerSize - 8, recBounds.Top, 1.5f * cornerSize, cornerSize, -90, 60);
          path.AddLine(recBounds.Right + 14, recBounds.Bottom - distance, recBounds.Left, recBounds.Bottom - distance);
          break;
      }

      return path;
    }

    /// <summary>
    /// Returns the rectangle of the given tab for drawing.
    /// </summary>
    /// <param name="index"></param>
    /// <returns></returns>
    new public Rectangle GetTabRect(int index)
    {
      ValidateTab(index);

      Rectangle area = layoutInfo[index].tabArea;
      area.Offset(scrollOffset, 0);
      return area;
    }

    /// <summary>
    /// Returns the rectangle of the close button for drawing.
    /// </summary>
    /// <param name="index"></param>
    /// <returns></returns>
    public Rectangle GetTabButtonRect(int index)
    {
      ValidateTab(index);

      Rectangle area = layoutInfo[index].buttonArea;
      area.Offset(scrollOffset, 0);
      return area;
    }

    /// <summary>
    /// Checks if the busy animation for the given style has been loaded already. If not
    /// the image is loaded and the animation for it started.
    /// </summary>
    /// <returns>The image registered as busy animation.</returns>
    internal Bitmap GetBusyIndicatorForStyle(string type)
    {
      if (!busyIndicators.ContainsKey(type))
      {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(FlatTabControl));
        switch (type)
        {
          case "blue":
            busyIndicators[type] = ((System.Drawing.Bitmap)(resources.GetObject("busy-indicator-blue")));
            break;
          case "darkblue":
            busyIndicators[type] = ((System.Drawing.Bitmap)(resources.GetObject("busy-indicator-darkblue")));
            break;
          case "yellow":
            busyIndicators[type] = ((System.Drawing.Bitmap)(resources.GetObject("busy-indicator-yellow")));
            break;
          default:
            busyIndicators[type] = ((System.Drawing.Bitmap)(resources.GetObject("busy-indicator-white")));
            break;
        }
        ImageAnimator.Animate(busyIndicators[type], new EventHandler(BusyAnimationStep));
      }
      return busyIndicators[type];
    }

		internal void DrawScroller(Graphics g)
		{
			if ((leftRightImages == null) || (leftRightImages.Images.Count != 4))
				return;

			Rectangle clientArea = this.ClientRectangle;

      Win32.RECT r0 = new Win32.RECT();
			Win32.GetClientRect(scroller.Handle, ref r0);

      // Fill background of the scroller. Decrease width and height by 1 to
      // account for right and bottom borders which should not be used in drawing.
      r0.Right--;
      r0.Bottom--;
      Rectangle scrollerRect = new Rectangle(r0.Location, r0.Size);
			using (Brush br = new SolidBrush(SystemColors.Control))
        g.FillRectangle(br, scrollerRect);
			
      // Make a small outer border.
			using (Pen border = new Pen(SystemColors.ControlDark))
      {
        Rectangle rborder = scrollerRect;
        rborder.Inflate(-1, -1);
        g.DrawRectangle(border, rborder);
      }

			int nMiddle = (r0.Width / 2);
			int nTop = (r0.Height - leftRightImages.ImageSize.Height) / 2 + 1;
			int nLeft = (nMiddle - leftRightImages.ImageSize.Width) / 2;

			Rectangle r1 = new Rectangle(new Point(nLeft, nTop), leftRightImages.ImageSize);
			Rectangle r2 = new Rectangle(new Point(nMiddle + nLeft, nTop), leftRightImages.ImageSize);

			// Finally draw the buttons.
			Image img = leftRightImages.Images[1];
			if (img != null)
			{
				if (TabCount > 0)
				{
					Rectangle r3 = this.GetTabRect(0);
					if (r3.Left < clientArea.Left)
						g.DrawImage(img, r1);
					else
					{
						img = leftRightImages.Images[3];
						if (img != null)
							g.DrawImage(img, r1);
					}
				}
			}

			img = leftRightImages.Images[0];
			if (img != null)
			{
				if (this.TabCount > 0)
				{
					Rectangle r3 = this.GetTabRect(this.TabCount - 1);
					if (r3.Right > (clientArea.Width - r0.Width))
						g.DrawImage(img, r2);
					else
					{
						img = leftRightImages.Images[2];
						if (img != null)
							g.DrawImage(img, r2);
					}
				}
			}
		}

    /// <summary>
    /// Searches for a child control of type UpDown32, which the standard tab control
    /// maintains and we want to customize.
    /// </summary>
    private void FindScroller()
    {
      // Find the UpDown control.
      IntPtr pWnd = Win32.GetWindow(this.Handle, Win32.GW_CHILD);

      while (pWnd != IntPtr.Zero)
      {
        // Get the window class name
        char[] className = new char[33];

        int length = Win32.GetClassName(pWnd, className, 32);
        string s = new string(className, 0, length);

        if (s == "msctls_updown32")
        {
          scroller = new SubClass(pWnd, true);
          scroller.SubClassedWndProc += new SubClass.SubClassWndProcEventHandler(ScrollerWndProc);
          break;
        }

        pWnd = Win32.GetWindow(pWnd, Win32.GW_HWNDNEXT);
      }
    }

    private void UpdateScroller()
    {
      // Seems the scroll is sometimes re-created, so also check for a 0 handle.
      if (scroller == null || scroller.Handle == IntPtr.Zero)
        FindScroller();

      if (scroller != null)
      {
        if (tabStyle == TabStyleType.NoTabs)
        {
          scrollOffset = 0;
          Win32.ShowWindow(scroller.Handle, (int)SW.HIDE);
          return;
        }

        Win32.RECT rect = new Win32.RECT();
        Win32.GetClientRect(scroller.Handle, ref rect);
        int totalTabWidth = GetTotalTabWidth();

        int availableWidth = ClientSize.Width - Margin.Horizontal;
        if (totalTabWidth > availableWidth)
        {
          // Decrease available space also by that used of the scroller now.
          availableWidth -= rect.Width;
          Win32.ShowWindow(scroller.Handle, (int)SW.SHOWNOACTIVATE);

          // The scroll range is the number of steps (scaled pixels) we need to scroll over the full tab space.
          int scrollRange = (int) Math.Ceiling((totalTabWidth - availableWidth) / (float) scrollScaleFactor) +
            (int) ((scrollerSpacing / (float) scrollScaleFactor));
          Win32.SendMessage(scroller.Handle, (int) UDM.SETRANGE32, IntPtr.Zero, new IntPtr(scrollRange));

          // Make sure the scroll position is still ok before setting it in the scroller.
          // Always scroll so that the right border stays close to the scroller.
          Rectangle bounds = GetTabRect(TabCount - 1);
          if (bounds.Right < ClientSize.Width - Margin.Right - scrollerSpacing - rect.Width)
            scrollOffset += ClientSize.Width - Margin.Right - scrollerSpacing - rect.Width - bounds.Right;
          Win32.SendMessage(scroller.Handle, (int)UDM.SETPOS32, IntPtr.Zero, new IntPtr(-scrollOffset / scrollScaleFactor));

          // Disable scroll acceleration by setting a single scroll increment.
          Win32.UDACCEL acceleration = new Win32.UDACCEL();
          acceleration.nInc = 5;
          acceleration.nSec = 0;
          IntPtr parameter = Marshal.AllocHGlobal(Marshal.SizeOf(acceleration));
          Marshal.StructureToPtr(acceleration, parameter, true);
          Win32.SendMessage(scroller.Handle, (int)UDM.SETACCEL, new IntPtr(1), parameter);
          Marshal.FreeHGlobal(parameter);
        }
        else
        {
          scrollOffset = 0;
          Win32.ShowWindow(scroller.Handle, (int)SW.HIDE);
        }

        Win32.InvalidateRect(scroller.Handle, ref rect, true);
        int scrollerWidth = rect.Width;
        rect.Left = Width - scrollerWidth - Margin.Right;
        rect.Right = rect.Left + scrollerWidth;

        if (tabStyle == TabStyleType.BottomNormal)
          rect.Top = ClientSize.Height - ItemSize.Height - Margin.Bottom;
        else
          rect.Top = Margin.Top;

        rect.Bottom = rect.Top + ItemSize.Height;
        Win32.MoveWindow(scroller.Handle, rect.Left, rect.Top, rect.Width, rect.Height, true);
      }
    }

    /// <summary>
    /// Computes the overall size of all tabs.
    /// </summary>
    /// <returns></returns>
    private int GetTotalTabWidth()
    {
      int result = 0;
      for (int i = 0; i < TabCount; i++)
        result += GetTabRect(i).Width;

      return result;
    }

    #endregion

    #region Event handling

    protected override void OnResize(EventArgs e)
    {
      base.OnResize(e);
      if (auxView != null)
      {
        auxView.Top = Top + Height - auxView.Height;
        auxView.Left = Left + Width - auxView.Width;
      }
    }

    protected override void OnHandleCreated(EventArgs e)
    {
      base.OnHandleCreated(e);

      RegisterForActivation(this, new EventArgs());
    }

    /// <summary>
    /// Triggered when the form we use to track activation changes is disposed. If we get this notification
    /// it means we are still alive, which can only be true if we have been re-parented in the meantime.
    /// Hence query the current top level control and use this as new tracker.
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void RegisterForActivation(object sender, EventArgs e)
    {
      activationTracker = TopLevelControl as Form;
      if (activationTracker != null)
      {
        activationTracker.Deactivate += new EventHandler(ActivationChanged);
        activationTracker.Activated += new EventHandler(ActivationChanged);
        activationTracker.Disposed += new EventHandler(RegisterForActivation);
      }
    }

    protected override void OnHandleDestroyed(EventArgs e)
    {
      base.OnHandleDestroyed(e);

      // Unregister our activation listener from our host form.
      if (activationTracker != null)
      {
        activationTracker.Deactivate -= ActivationChanged;
        activationTracker.Activated -= ActivationChanged;
      }
    }

    private void ActivationChanged(object sender, EventArgs e)
    {
      Invalidate();
    }

    override protected void OnControlAdded(ControlEventArgs e)
		{
      base.OnControlAdded(e);

      if (e.Control is TabPage)
      {
        TabPage page = e.Control as TabPage;
        page.TextChanged += new EventHandler(PageTextChanged);
        if (tabStyle == TabStyleType.BottomNormal)
          page.BackColor = Color.White;

        layoutInfo.Insert(TabPages.IndexOf(page), new TabInfo());
        AdjustLayoutInfo(page);
        UpdateScroller();

        Show();

        // If this is the first page then it is automatically selected,
        // but doesn't send a selection change event. Hence we do it manually.
        if (TabCount == 1)
          OnSelectedIndexChanged(null);
      }
		}

		override protected void OnControlRemoved(ControlEventArgs e)
		{
      base.OnControlRemoved(e);

      if (e.Control is TabPage)
      {
        TabPage page = e.Control as TabPage;
        page.TextChanged -= PageTextChanged;

        // The page is still in the tab view and removed when execution returns to the caller.
        // So we cannot use our normal handling and have to invalidate following pages here.
        int index = TabPages.IndexOf(page);
        layoutInfo.RemoveAt(index);
        while (index < layoutInfo.Count)
          layoutInfo[index++].isValid = false;
        UpdateScroller();

        if (!DesignMode && hideWhenEmpty && TabCount == 1)
          Hide();
      }
		}

		override protected void OnSelectedIndexChanged(EventArgs e)
		{
      base.OnSelectedIndexChanged(e);

      ScrollIntoView(SelectedIndex);
      if (scroller != null)
      {
        Win32.RECT clientArea = new Win32.RECT();
        Win32.GetClientRect(scroller.Handle, ref clientArea);
        Win32.InvalidateRect(scroller.Handle, ref clientArea, false);
      }
			Invalidate();	// We need to update border and background colors.
		}

    override protected void OnMouseDown(MouseEventArgs e)
    {
      switch (e.Button)
      {
        case MouseButtons.Left:
          Focus();
          lastTabHit = TabIndexFromPosition(e.Location);
          if (lastTabHit > -1)
          {
            lastClickPosition = e.Location;
            if (SelectedIndex != lastTabHit)
              SelectedIndex = lastTabHit;
            else
              buttonHit = GetTabButtonRect(lastTabHit).Contains(e.Location);
          }
          break;

        default:
          base.OnMouseDown(e);
          break;
      }
    }

    protected override void OnMouseUp(MouseEventArgs e)
    {
      switch (e.Button)
      {
        case MouseButtons.Left:
          if (lastTabHit > -1)
          {
            if (buttonHit && GetTabButtonRect(lastTabHit).Contains(e.Location) &&
              (TabCount > 1 || CanCloseLastTab))
            {
              // Close tab if the application agrees.
              TabPage page = TabPages[lastTabHit];
              CloseTabPage(page);
            }
          }
          break;

        case MouseButtons.Right:
          {
            int tab = TabIndexFromPosition(e.Location);
            if (tab > -1)
            {
              OnTabShowMenu(new TabMenuEventArgs(TabPages[tab], tab, PointToScreen(e.Location)));
            }
          }
          break;

        default:
          base.OnMouseUp(e);
          break;
      }
    }

    protected override void OnMouseMove(MouseEventArgs e)
    {
      switch (e.Button)
      {
        case MouseButtons.Left:
          if (lastTabHit > -1 && !buttonHit && CanReorderTabs &&
            (Math.Abs(lastClickPosition.X - e.Location.X) > 5 || Math.Abs(lastClickPosition.Y - e.Location.Y) > 5))
          {
            TabPage page = TabPages[lastTabHit];
            if (DoDragDrop(page, DragDropEffects.Move) == DragDropEffects.Move)
            {
              int newIndex = TabPages.IndexOf(page);
              OnTabMoved(new TabMovedEventArgs(page, lastTabHit, newIndex));
            }
          }
          else
            base.OnMouseMove(e);
          break;

        default:
          base.OnMouseMove(e);
          break;
      }
    }

    protected override void OnDragOver(DragEventArgs e)
    {
      // See if a tab page is being dragged.
      if (e.Data.GetData(typeof(TabPage)) == null)
      {
        base.OnDragOver(e);
        return;
      }

      TabPage dragTab = (TabPage) e.Data.GetData(typeof(TabPage));
      int dragTabIndex = TabPages.IndexOf(dragTab);
      if (dragTabIndex < 0)
      {
        // Tab page doesn't belong to this view. Ignore it.
        e.Effect = DragDropEffects.None;
        return;
      }

      // Hover over a tab?
      int hoverIndex = TabIndexFromPosition(PointToClient(new Point(e.X, e.Y)));
      if (hoverIndex < 0)
      {
        e.Effect = DragDropEffects.None;
        return;
      }

      e.Effect = DragDropEffects.Move;

      // Do this check after we set the drop effect so we don't get a block cursor
      // when we already moved a page and still hover with the mouse over that.
      if (dragTabIndex == hoverIndex)
        return;

      Rectangle dragTabRect = GetTabRect(dragTabIndex);
      Rectangle hoverTabRect = GetTabRect(hoverIndex);

      if (dragTabRect.Width < hoverTabRect.Width)
      {
        Point tcLocation = PointToScreen(Location);

        if (dragTabIndex < hoverIndex)
        {
          if ((e.X - tcLocation.X) > ((hoverTabRect.X + hoverTabRect.Width) - dragTabRect.Width))
            MovePage(dragTab, hoverIndex);
        }
        else
          if (dragTabIndex > hoverIndex)
          {
            if ((e.X - tcLocation.X) < (hoverTabRect.X + dragTabRect.Width))
              MovePage(dragTab, hoverIndex);
          }
      }
      else
        MovePage(dragTab, hoverIndex);
    }

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

    protected override void OnSizeChanged(EventArgs e)
    {
      if (IsHandleCreated)
      {
        this.BeginInvoke((MethodInvoker) delegate
        {
          base.OnSizeChanged(e);
        });
      }
    }

    void PageTextChanged(object sender, EventArgs e)
    {
      AdjustLayoutInfo(sender as TabPage);
    }

    public event EventHandler<TabClosingEventArgs> TabClosing;
    protected internal virtual void OnTabClosing(TabClosingEventArgs args)
    {
      if (TabClosing != null)
        TabClosing(this, args);
    }

    public event EventHandler<TabClosedEventArgs> TabClosed;
    protected internal virtual void OnTabClosed(TabClosedEventArgs args)
    {
      if (TabClosed != null)
        TabClosed(this, args);
    }

    public event EventHandler<TabMovingEventArgs> TabMoving;
    protected internal virtual void OnTabMoving(TabMovingEventArgs args)
    {
      if (TabMoving != null)
        TabMoving(this, args);
    }

    public event EventHandler<TabMovedEventArgs> TabMoved;
    protected internal virtual void OnTabMoved(TabMovedEventArgs args)
    {
      if (TabMoved != null)
        TabMoved(this, args);
    }

    public event EventHandler<TabMenuEventArgs> TabShowMenu;
    protected internal virtual void OnTabShowMenu(TabMenuEventArgs args)
    {
      if (TabShowMenu != null)
        TabShowMenu(this, args);
    }

    protected override void OnKeyDown(KeyEventArgs ke)
    {
      if (ke.KeyData == (Keys.Control | Keys.Tab))
        if (!useDefaultTabSwitchKey)
          return;

      base.OnKeyDown(ke);
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

    #region Other implementation

    /// <summary>
    /// Determines if the the close button for a given tab should be shown.
    /// </summary>
    private bool IsCloseButtonVisible(int index)
    {
      return (layoutInfo[index].closeButtonVisibility == CloseButtonVisiblity.ShowButton) ||
        (layoutInfo[index].closeButtonVisibility == CloseButtonVisiblity.InheritVisibility && showCloseButton);
    }

    /// <summary>
    /// Adjusts the size of the layout info list and marks all entries that follow the given page
    /// as invalid, so that they re-compute their layout next time it is required.
    /// </summary>
    private void AdjustLayoutInfo(TabPage page)
    {
      // Since add and remove of entries in the layoutInfo list does not consider where changes
      // happened it's better to adjust the list in advance where possible.
      if (layoutInfo.Count > TabCount)
        layoutInfo.RemoveRange(TabCount, layoutInfo.Count - TabCount);

      for (int i = 0; i < TabCount; i++)
      {
        if (i >= layoutInfo.Count)
          layoutInfo.Add(new TabInfo());
        layoutInfo[i].page = TabPages[i];
      }

      bool invalidate = (page == null) ? true : false;
      foreach (TabInfo entry in layoutInfo)
      {
        // Invalidate this page and all following.
        if (entry.page == page)
          invalidate = true;
        if (invalidate)
          entry.isValid = false;
      }
    }

    /// <summary>
    /// Recomputes layout information for this tab if not yet done.
    /// </summary>
    private void ValidateTab(int index)
    {
      if (index >= layoutInfo.Count)
        AdjustLayoutInfo(TabPages[index]);

      if (!layoutInfo[index].isValid)
      {
        Graphics g = CreateGraphics();

        // Find first valid entry before the given one.
        // We have to validate all entries after that until this one.
        int currentIndex = index;
        while (currentIndex >= 0 && !layoutInfo[currentIndex].isValid)
          currentIndex--;

        int offsetX = Margin.Left;
        if (currentIndex > -1)
          offsetX = layoutInfo[currentIndex].tabArea.Right;
        int offsetY = Margin.Top;
        if (tabStyle == TabStyleType.BottomNormal)
          offsetY = ClientSize.Height - ItemSize.Height - Margin.Bottom;
        while (++currentIndex <= index)
        {
          TabInfo info = layoutInfo[currentIndex];
          int textWidth = 0;

          if (info.page.Text.Length > 0)
          {
            Font font = this.Font;
            if (Conversions.InHighContrastMode())
              font = new Font(font, font.Style | FontStyle.Bold);
            SizeF measuredTextSize = g.MeasureString(info.page.Text, font);
            textWidth = (int)Math.Ceiling(measuredTextSize.Width);
          }

          int buttonPart = 0;
          bool buttonSpaceNeeded = IsCloseButtonVisible(currentIndex) || info.isBusy;
          Size buttonSize;
          if (info.isBusy)
          {
            // Assuming here all busy indicator images are of the same size.
            // Not a strong limitation actually, since all tabs are of same height too.
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(FlatTabControl));
            Bitmap image = ((System.Drawing.Bitmap)(resources.GetObject("busy-indicator-blue")));
            buttonSize = image.Size;
          }
          else
            buttonSize = new Size(darkCloseButton.Width, darkCloseButton.Height);
          if (buttonSpaceNeeded)
            buttonPart = buttonSpacing + buttonSize.Width;
          int tabWidth = itemPadding.Horizontal + textWidth + buttonPart;
          if ((info.page.ImageIndex >= 0) && (ImageList != null) && (ImageList.Images.Count > info.page.ImageIndex) &&
            (ImageList.Images[info.page.ImageIndex] != null))
          {
            tabWidth += ImageList.ImageSize.Width;
            
            // Add 4 px spacing if there is something after the icon.
            if (buttonSpaceNeeded || textWidth > 0)
              tabWidth += 4;
          }

          if (ItemSize.Width > tabWidth)
            tabWidth = ItemSize.Width;
          if (tabWidth > maxTabSize)
            tabWidth = maxTabSize;
          info.tabArea = new Rectangle(offsetX, offsetY, tabWidth, ItemSize.Height);
          offsetX = info.tabArea.Right;

          if (buttonSpaceNeeded)
          {
            int buttonOffset = offsetY + (info.tabArea.Height - buttonSize.Height) / 2 + 1;
            info.buttonArea = new Rectangle(offsetX - itemPadding.Right - buttonSize.Width, buttonOffset,
              buttonSize.Width, buttonSize.Height);
          }
          else
            info.buttonArea = Rectangle.Empty;

          info.isValid = true;
        }

        g.Dispose();
      }
    }

    /// <summary>
    /// Allows to set individual close buttons for a tab.
    /// </summary>
    public void SetCloseButtonVisibility(int index, CloseButtonVisiblity visibility)
    {
      AdjustLayoutInfo(TabPages[index]);
      layoutInfo[index].closeButtonVisibility = visibility;
    }

    /// <summary>
    /// Mark individual tabs as busy causing them to show a busy indicator instead of the close button.
    /// </summary>
    public void SetBusy(int index, bool busy)
    {
      if (layoutInfo[index].isBusy != busy)
      {
        if (!busy)
        {
          layoutInfo[index].isBusy = false;
          AdjustLayoutInfo(TabPages[index]);
        }
        else
        {
          AdjustLayoutInfo(TabPages[index]);
          layoutInfo[index].isBusy = true;
        }
        Invalidate();
      }
    }

    private void BusyAnimationStep(object sender, EventArgs args)
    {
      foreach (TabInfo info in layoutInfo)
        if (info.isBusy)
        {
          Rectangle area = info.tabArea;
          area.Offset(scrollOffset, 0);
          Invalidate(area, false);
        }
    }

    /// <summary>
    /// Does the actual work to close a tab page, sending out appropriate events and updating
    /// internal structures. Returns true if the page was actually closed.
    /// </summary>
    public bool CloseTabPage(TabPage page)
    {
      if (page == null)
        return false;
      if (TabCount > 1 || CanCloseLastTab)
      {
        TabClosingEventArgs args = new TabClosingEventArgs(page, true, TabPages.IndexOf(page));
        OnTabClosing(args);
        if (args.canClose)
        {
          // The page could be removed in the OnTabClosing event, so prepare for that.
          int tabIndex = TabPages.IndexOf(page);
          if (tabIndex > -1)
          {
            SetBusy(tabIndex, false);

            // Remove this tab and select its predecessor if possible.
            if (SelectedIndex > 0)
              SelectedIndex = SelectedIndex - 1;
            lastTabHit = -1;
            TabPages.Remove(page);
          }

          AdjustLayoutInfo(null);
          Update();

          OnTabClosed(new TabClosedEventArgs(page));

          // In the case OnTabClosed did not dispose of the page do it now.
          if (!page.IsDisposed)
            page.Dispose();

          return true;
        }
      }
      return false;
    }

    /// <summary>
    /// Determines if the given position lies within the bounds of a tab and, if so, returns the tab's
    /// index to the caller.
    /// </summary>
    /// <param name="point">The position to check for in local coordinates.</param>
    /// <returns>The index of the tab found or -1 if none.</returns>
    public int TabIndexFromPosition(Point point)
    {
      for (int i = 0; i < TabCount; i++)
        if (GetTabRect(i).Contains(point))
          return i;
      return -1;
    }

    /// <summary>
    /// Adjusts the scroll offset so that the given tab is in the visible area.
    /// </summary>
    /// <param name="index">The index of the tab to make visible.</param>
    /// <returns>True if an adjustment was necessary, otherwise False.</returns>
    public bool ScrollIntoView(int index)
    {
      bool result = false;
      if (index < 0 || index >= TabCount)
        return result;

      // If our bounds are empty then the application is likely minimized and we delay setting
      // the horizontal offset until we are restored.
      if (Width == 0 && Height == 0)
      {
        pendingScrollIntoViewIndex = index;
        return result;
      }

      Rectangle area = GetTabRect(index);
      if (area.Left < 0)
      {
        scrollOffset -= area.Left;
        result = true;
      }
      else
      {
        Win32.RECT rect = new Win32.RECT();
        if (scroller != null)
          Win32.GetClientRect(scroller.Handle, ref rect);

        int rightBorder = ClientSize.Width - Margin.Right - rect.Width - scrollerSpacing;
        if (area.Right > rightBorder)
        {
          scrollOffset -= area.Right - rightBorder;
          result = true;
        }
      }

      if (result && scroller != null)
        Win32.SendMessage(scroller.Handle, (int)UDM.SETPOS32, IntPtr.Zero, new IntPtr(-scrollOffset / scrollScaleFactor));
      return result;
    }

    /// <summary>
    /// Moves the given page to the new index.
    /// </summary>
    private void MovePage(TabPage page, int newIndex)
    {
      int oldIndex = TabPages.IndexOf(page);

      TabMovingEventArgs args = new TabMovingEventArgs(page, oldIndex, newIndex);
      OnTabMoving(args);
      if (args.Cancel)
        return;

      bool selectTab = (oldIndex == SelectedIndex);

      // The simple approach here would be to remove the page and insert it at the new location.
      // However this will produce heavy flickering, so we move the pages one by one instead.
      // While moving the pages also move their corresponding tab info to keep current states.
      TabInfo oldInfo = layoutInfo[oldIndex];
      if (oldIndex < newIndex)
      {
        for (int index = oldIndex; index < newIndex; index++)
        {
          TabPages[index] = TabPages[index + 1];
          layoutInfo[index] = layoutInfo[index + 1];
        }
        TabPages[newIndex] = page;
        layoutInfo[newIndex] = oldInfo;
        AdjustLayoutInfo(TabPages[oldIndex]);
      }
      else
      {
        for (int index = oldIndex; index > newIndex; index--)
        {
          TabPages[index] = TabPages[index - 1];
          layoutInfo[index] = layoutInfo[index - 1];
        }
        TabPages[newIndex] = page;
        layoutInfo[newIndex] = oldInfo;
        AdjustLayoutInfo(page);
      }

      // Keep the tab selected which was selected before.
      if (selectTab)
        SelectedIndex = newIndex;

      Invalidate();
    }

    // In order to simplify general handling of page content (which is very often just a container
    // with the actual content) we introduce here the concept of documents.
    // A document is the content of a tab. When using this concept you should only have one control on each
    // tab to make this work. Otherwise use the tab pages directly and handle search etc. yourself.
    // See also ITabDocument and TabDocument and the Documents property below.

    /// <summary>
    /// Scans the given control and all its children to find a docked ITabDocument.
    /// Due to various docking structures the document can be anywhere in the hierarchy.
    /// </summary>
    /// <param name="control"></param>
    /// <returns></returns>
    private ITabDocument DocumentFromHierarchy(Control control)
    {
      if (control is ITabDocument)
        return control as ITabDocument;

      foreach (Control child in control.Controls)
      {
        ITabDocument result = DocumentFromHierarchy(child);
        if (result != null)
          return result;
      }

      return null;
    }

    /// <summary>
    /// Returns the document (i.e. the first control on a tab page) whose page has the given title.
    /// </summary>
    /// <param name="text"></param>
    /// <returns></returns>
    public ITabDocument FindDocument(string text)
    {
      foreach (TabPage page in TabPages)
        if (page.Text == text)
        {
          ITabDocument result = DocumentFromHierarchy(page);
          if (result != null)
            return result;
        }

      return null;
    }

    /// <summary>
    /// Returns the document stored on the tab page with the given index or null if there is none.
    /// </summary>
    public ITabDocument DocumentFromIndex(int index)
    {
      return DocumentFromPage(TabPages[index]);
    }

    /// <summary>
    /// Returns the index for the given document or -1 if not found.
    /// </summary>
    public int IndexFromDocument(ITabDocument document)
    {
      int i = 0;
      foreach (TabPage page in TabPages)
      {
        if (DocumentFromHierarchy(page) == document)
          return i;
        i++;
      }

      return -1;
    }

    /// <summary>
    /// Returns the document stored on the given tab page or null if there is none.
    /// </summary>
    public ITabDocument DocumentFromPage(TabPage page)
    {
      return DocumentFromHierarchy(page);
    }

    /// <summary>
    /// Checks if the document is the first control on any of our pages.
    /// </summary>
    /// <returns>True if the document was found, otherwise false.</returns>
    public bool HasDocument(ITabDocument document)
    {
      foreach (TabPage page in TabPages)
        if (DocumentFromHierarchy(page) == document)
          return true;

      return false;
    }

    /// <summary>
    /// Adds the given document to a new tab page and returns the index of that new page.
    /// </summary>
    /// <param name="document"></param>
    /// <returns></returns>
    public int AddDocument(ITabDocument document)
    {
      TabPage page = new TabPage();
      TabDocument control = document as TabDocument;
      control.SetHost(page);
      page.Controls.Add(control);
      control.Dock = DockStyle.Fill;
      control.Margin = new Padding(0);
      control.Padding = new Padding(0);
      control.Show();
      TabPages.Add(page);

      return TabPages.Count - 1;
    }

    /// <summary>
    /// Removes a previously added document from this control. This method removes the hosting tab,
    /// regardless of other content on it. The document itself is not freed.
    /// </summary>
    /// <param name="document"></param>
    public void RemoveDocument(ITabDocument document)
    {
      foreach (TabPage page in TabPages)
        if (DocumentFromHierarchy(page) == document)
        {
          page.Controls.Clear();
          TabPages.Remove(page);

          return;
        }
    }

    /// <summary>
    /// Tries to close the given document by sending out TabClosing. If that returns true
    /// the document is closed and its hosting tab removed.
    /// </summary>
    /// <returns>True if the document was closed, otherwise false.</returns>
    public bool CloseDocument(ITabDocument document)
    {
      foreach (TabPage page in TabPages)
        if (DocumentFromHierarchy(page) == document)
          return CloseTabPage(page);

      return false;
    }

    /// <summary>
    /// Returns the list of documents in an array, e.g. to allow manipulation of this list in a loop.
    /// </summary>
    /// <returns></returns>
    public ITabDocument[] DocumentsToArray()
    {
      ITabDocument[] result = new ITabDocument[TabCount];
      int i = 0;
      foreach (ITabDocument content in Documents)
        result[i++] = content;

      return result;
    }

    #endregion

    #region Properties

    /// <summary>
    /// Returns the currently active document if there is one.
    /// </summary>
    public ITabDocument ActiveDocument
    {
      get
      {
        if (SelectedTab != null)
          return DocumentFromHierarchy(SelectedTab);
        return null;
      }
    }

    public Control AuxControl
    {
      get
      {
        return auxView;
      }

      set
      {
        if (auxView != value)
        {
          auxView = value;
          //auxView.BackColor = Color.Transparent; Transparent doesn't work as expected on Win.
          auxView.Dock = DockStyle.None;
          Parent.Controls.Add(auxView);
          Parent.Controls.SetChildIndex(auxView, 0);
        }
      }
    }

    /// <summary>
    /// Helper enumeration that returns the control which represents an ITabDocument on each page
    /// or null if there is none on the specific page.
    /// </summary>
    /// <returns></returns>
    public IEnumerable<ITabDocument> Documents
    {
      get
      {
        foreach (TabPage page in TabPages)
          yield return DocumentFromHierarchy(page);
      }
    }

		public TabStyleType TabStyle
		{
			get { return tabStyle; }
			set 
      {
        if (value != tabStyle)
        {
          tabStyle = value;
          RecalculateFrame();
          AdjustLayoutInfo(null);
          UpdateScroller();
        }
      }
		}

    public Padding ItemPadding
    {
      get { return itemPadding; }
      set
      {
        itemPadding = value;
        AdjustLayoutInfo(null);
        Invalidate();
      }
    }

    public Padding ContentPadding
    {
      get { return contentPadding; }
      set
      {
        contentPadding = value;
        AdjustLayoutInfo(null);
        Invalidate();
      }
    }

    [Browsable(true)]
    public bool ShowCloseButton
    {
      get { return showCloseButton; }
      set
      {
        if (value != showCloseButton)
        {
          showCloseButton = value;
          RecalculateFrame();
          AdjustLayoutInfo(null);
          Invalidate();
        }
      }
    }

    [Browsable(true)]
    public bool ShowFocusState
    {
      get { return showFocusState; }
      set
      {
        if (value != showFocusState)
        {
          showFocusState = value;
          Invalidate();
        }
      }
    }

    [Browsable(true)]
    public bool CanCloseLastTab { get; set; }

    [Browsable(true)]
    public bool HideWhenEmpty
    {
      get { return hideWhenEmpty; }
      set
      {
        if (value != hideWhenEmpty)
        {
          hideWhenEmpty = value;
          if (!DesignMode && hideWhenEmpty && TabCount == 0)
            Hide();
          else
            Show();
        }
      }
    }

    [Browsable(true)]
    public int MaxTabSize
    {
      get { return maxTabSize; }
      set
      {
        if (value < ItemSize.Width)
          value = ItemSize.Width;
        if (value != maxTabSize)
        {
          maxTabSize = value;
          RecalculateFrame();
          AdjustLayoutInfo(null);
          Invalidate();
        }
      }
    }

    [Browsable(true)]
    public bool CanReorderTabs { get; set; }

    bool renderWithGlow = true;

    [Browsable(true)]
    public bool RenderWithGlow
    {
      get { return renderWithGlow; }
      set
      {
        if (value != renderWithGlow)
        {
          renderWithGlow = value;
          Invalidate();
        }
      }
    }

    private Color backColor;

    [Browsable(true)]
    public Color BackgroundColor
    {
      get { return backColor; }
      set
      {
        if (value != backColor)
        {
          backColor = value;
          Invalidate();
        }
      }
    }

    [Browsable(true)]
    public bool DefaultTabSwitch
    {
      get { return useDefaultTabSwitchKey; }
      set { useDefaultTabSwitchKey = value; }
    }

    // With the switch to .NET 4.0 client profile we have to separate runtime
    // and design time code.
    [Editor("MySQL.Controls.FlatTabControlCollectionEditor", "UITypeEditor")]
    public new TabPageCollection TabPages
    {
      get
      {
        return base.TabPages;
      }
    }

    #endregion
	}

  //------------------------------------------------------------------------------------------------

  // In order to aid managing documents on tab pages (instead of plain controls) we define an interface and 
  // a base class which implements it, to be used by the application, if it wants to maintain
  // the "document metaphor".
  public interface ITabDocument
  {
    void Activate();
    void Close();

    String TabText { get; set; }
    Control Content { get; }
    int ToolbarHeight { get; }
  }

  public class TabDocument : Form, ITabDocument
  {
    private TabPage host = null;
    private String tabText = "";
    private String toolTipText = "";

    public TabDocument()
    {
      FormBorderStyle = FormBorderStyle.None;
      TopLevel = false;
    }

    new virtual public void Show()
    {
      base.Show();
    }

    new virtual public void Activate()
    {
      Show();

      base.Activate();
      (host.Parent as TabControl).SelectedTab = host;
    }

    public void SetHost(TabPage host)
    {
      if (this.host != host)
      {
        if (this.host != null)
        {
          FlatTabControl tabControl = this.host.Parent as FlatTabControl;
          if (tabControl != null)
          {
            // Remove the tab page this document was docked to if it gets moved to another one.
            this.host.Controls.Clear();
            tabControl.TabPages.Remove(this.host);
            tabControl.OnTabClosed(new TabClosedEventArgs(this.host));
          }
        }

        this.host = host;
        if (host != null)
        {
          host.Text = tabText;
          host.ToolTipText = toolTipText;
        }
      }
    }

    protected override void Dispose(bool disposing)
    {
      host = null;
      base.Dispose(disposing);
    }

    /// <summary>
    /// Called when the document is closed already. Just remove its tab page.
    /// No need to trigger tab closing and closed events.
    /// </summary>
    override protected void OnFormClosed(FormClosedEventArgs e)
    {
      if (host != null && host.Parent != null)
      {
        host.Controls.Clear();
        (host.Parent as FlatTabControl).TabPages.Remove(host);
      }
      base.OnFormClosed(e);
    }

    public virtual String TabText
    {
      get { return tabText; }
      set
      {
        tabText = value;
        if (host != null)
          host.Text = value;
      }
    }

    public virtual Control Content
    {
      get { return this; }
    }

    public int ToolbarHeight
    {
      get {
        if (Controls.Count > 0 && Controls[0] is Panel)
        {
          // If this tab document has a menu and/or toolbar then it is placed on a panel
          // in the root of this document.
          Panel container = Controls[0] as Panel;
          if (container.Controls.Count > 0 && (container.Controls[0] is ToolStrip))
            return container.Height;
        }
        return 0;
      }
    }

    public virtual String ToolTipText
    {
      get { return toolTipText; }
      set
      {
        toolTipText = value;
        if (host != null)
          host.ToolTipText = value;
      }
    }
  }

  #region Event argument classes

  public class TabClosingEventArgs : EventArgs
  {
    public int index;
    public TabPage page;
    public bool canClose;

    public TabClosingEventArgs(TabPage page, bool canClose, int index)
    {
      this.page = page;
      this.canClose = canClose;
      this.index = index;
    }
  }

  public class TabClosedEventArgs : EventArgs
  {
    public TabPage page;

    public TabClosedEventArgs(TabPage page)
    {
      this.page = page;
    }
  }

  public class TabMenuEventArgs : EventArgs
  {
    public TabPage page;
    public int pageIndex;
    public Point location;

    public TabMenuEventArgs(TabPage page, int index, Point pos)
    {
      this.page = page;
      this.pageIndex = index;
      this.location = pos;
    }
  };

  public class TabMovedEventArgs : EventArgs
  {
    public int FromIndex, ToIndex;
    public TabPage MovedPage;

    public TabMovedEventArgs(TabPage page, int from, int to)
    {
      MovedPage = page;
      FromIndex = from;
      ToIndex = to;
    }
  }

  public class TabMovingEventArgs : EventArgs
  {
    public int FromIndex, ToIndex;
    public TabPage MovedPage;
    public bool Cancel;

    public TabMovingEventArgs(TabPage page, int from, int to)
    {
      MovedPage = page;
      FromIndex = from;
      ToIndex = to;
      Cancel = false;
    }
  }

  #endregion

}
