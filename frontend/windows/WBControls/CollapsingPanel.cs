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

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;
using System.Windows.Forms.Layout;

using MySQL.Controls.Properties;
using MySQL.Utilities.SysUtils;

namespace MySQL.Utilities
{
  #region Delegates

  public delegate int TabCountHandler(object sender);
  public delegate bool TabInfoHandler(object sender, int index,
    out int iconId, out string caption, out string description);
  public delegate bool TabChangingHandler(object sender, int index);
  public delegate void TabChangedHandler(object sender, int index);
  public delegate void CustomButtonEvent(object sender, int index);

  #endregion

  public enum CollapsingPanelDisplayMode
  {
    Normal,         // Show only the header, no tabs.
    HeaderAndTab,   // ???
    TabsOnly,       // Show only the tabs, no header.
    HeaderWithTabs, // Show both, header and tabs.
    NoHeader        // Show neither header nor tabs.
  };

  public enum CollapsingPanelStyle { Convex, Flat };

  public class CollapsingPanel : Panel
  {
    #region Local classes

    // Holds data for a single custom button on the header.
    private class CustomButton
    {
      public Rectangle boundingBox;
      public Bitmap bitmap;

      public CustomButton(Bitmap bitmap, Rectangle bounds)
      {
        this.bitmap = bitmap;
        boundingBox = bounds;
      }

      public void Paint(Graphics graphics, bool active)
      {
        int x = boundingBox.Left;
        int y = boundingBox.Top;

        // Draw background first if the button is active.
        if (active)
        {
          Bitmap background = Resources.collapsing_panel_grid_bg;
          int bgX = x + (boundingBox.Width - background.Width) / 2;
          int bgY = y + (boundingBox.Height - background.Height) / 2;
          graphics.DrawImageUnscaled(background, bgX, bgY);

          // Move the button image down and right a bit to create the "pressed" effect.
          x++;
          y++;
        }

        // Center images in the available space.
        x += (boundingBox.Width - bitmap.Width) / 2;
        y += (boundingBox.Height - bitmap.Height) / 2;
        graphics.DrawImageUnscaled(bitmap, x, y);

        // Draw a left border line if not active.
        if (!active)
        {
          Bitmap boundary = Resources.collapsing_panel_header_tab_left_flat;
          graphics.DrawImageUnscaledAndClipped(boundary, new Rectangle(x, y, boundary.Width, boundingBox.Height));
        }
      }
    }

    #endregion

    // ----------------------------------------------------------------------------------------------------------------

    #region Member Variables

    /// <summary> 
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    // Drawing resources.
    private Brush fontDarkBrush = new SolidBrush(Color.FromArgb(30, 30, 30));

    private Font headerFont = ControlUtilities.GetFont("Trebuchet MS", 11, FontStyle.Bold);
    private Font tabHeaderCaptionFont = ControlUtilities.GetFont("Trebuchet MS", 11, FontStyle.Bold);
    private Font tabHeaderDescriptionFont = ControlUtilities.GetFont("Trebuchet MS", 7);

    // Custom buttons. They are painted in the header, right aligned, left to the +/- buttons,
    // from left to right (index 0 is at the left side).
    private List<CustomButton> customButtons = new List<CustomButton>();
    private CustomButton activeCustomButton = null;

    // General properties.
    private string headerCaption = "Caption";

    private bool expanded = true;
    private int computedExpandedHeight = 0;
    private int manualHeight = 0;
    private static int descriptionSpace = 3;
    private int headerHeight = 23 + descriptionSpace;
    private int headerHeightTabsOnly = 63;
    private int headerSpace = 5;
    private int buttonOffset = 0;
    private int tabPaintOffset = 0;

    private CollapsingPanelDisplayMode displayMode = CollapsingPanelDisplayMode.Normal;
    private CollapsingPanelStyle style = CollapsingPanelStyle.Flat;
    private bool disableExpansion = false;
    private bool disableExpansionIcon = false;

    private int tabsHeight = 56;
    private int selectedTabIndex = 0;
    private ImageList tabHeaderImageList;
    private List<int> cachedTabWidths = new List<int>();
    private bool doPreviousControlResize = false;
    private Point mouseDownPoint;
    private int prevControlHeight;
    private bool displayTabActionButtons = false;
    private bool displayCustomButtons = false;
    private bool resizable = false;
    
    private Rectangle popupButtonBounds;
    private ContextMenu tabMenu;

    private ColumnLayout layoutEngine;

    #endregion

    #region Constructors

    public CollapsingPanel()
    {
      InitializeComponent();

      // Set double buffer
      SetStyle(
            ControlStyles.UserPaint |
            ControlStyles.AllPaintingInWmPaint |
            ControlStyles.OptimizedDoubleBuffer, true);
    }

    public CollapsingPanel(string Caption,
      CollapsingPanelDisplayMode InitialDisplayMode,
      CollapsingPanelStyle InitialStyle, bool InitialExpandState, int initialHeight)
      : this()
    {
      headerCaption = Caption;

      displayMode = InitialDisplayMode;
      style = InitialStyle;
      manualHeight = initialHeight;
      Expanded = InitialExpandState;

      tabMenu = new ContextMenu();
      tabMenu.Popup += new EventHandler(TabPopupEventHandler);
    }

    /// <summary> 
    /// Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
      if (disposing && (components != null))
      {
        components.Dispose();
      }

      headerFont.Dispose();
      tabHeaderCaptionFont.Dispose();
      tabHeaderDescriptionFont.Dispose();

      base.Dispose(disposing);
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

    #region Events

    private void TabPopupEventHandler(System.Object sender, System.EventArgs e)
    {
      // Clear all previously added MenuItems.
      tabMenu.MenuItems.Clear();

      for (int i = 0; i < cachedTabWidths.Count; i++)
      {
        string caption = "";
        string description = "";
        int iconId = -1;
        if (TabInfoNeeded != null)
        {
          if (!TabInfoNeeded(this, i, out iconId, out caption, out description))
            continue;
        }
        else
        {
          caption = "Caption";
          description = "Description";
        }

        MenuItem item = new MenuItem(caption);
        item.Click += new EventHandler(tabMenuItemClick);
        item.Checked = selectedTabIndex == i;
        tabMenu.MenuItems.Add(item);
      }
    }

    void tabMenuItemClick(object sender, EventArgs e)
    {
      MenuItem item = (MenuItem)sender;
      SelectedTabIndex = item.Index;
    }

    protected override void OnControlAdded(ControlEventArgs e)
    {
      base.OnControlAdded(e);
      PerformLayout();
    }

    protected override void OnControlRemoved(ControlEventArgs e)
    {
      base.OnControlRemoved(e);
      PerformLayout();
    }

    protected override void OnPaint(System.Windows.Forms.PaintEventArgs e)
    {
      base.OnPaint(e);

      Graphics g = e.Graphics;
      bool showHeader = (DisplayMode != CollapsingPanelDisplayMode.TabsOnly) &&
        (DisplayMode != CollapsingPanelDisplayMode.NoHeader);
      bool showTabs = (DisplayMode == CollapsingPanelDisplayMode.HeaderAndTab) ||
        (DisplayMode == CollapsingPanelDisplayMode.HeaderWithTabs) ||
        (DisplayMode == CollapsingPanelDisplayMode.TabsOnly);

      // Draw the header background
      if (showHeader)
      {
        if (style != CollapsingPanelStyle.Flat ||
          DisplayMode == CollapsingPanelDisplayMode.HeaderWithTabs)
        {
          Bitmap headerBg = style == CollapsingPanelStyle.Convex ?
            Resources.collapsing_panel_header_bg : Resources.collapsing_panel_header_bg_flat;

          g.DrawImageUnscaledAndClipped(headerBg,
            new Rectangle(0, 0, Width + 30, headerBg.Height));
        }
        else
        {
          using (SolidBrush background = new SolidBrush(Color.FromArgb(245, 245, 245)))
            g.FillRectangle(background, new Rectangle(0, 0, Width, headerHeight));

          // Draw tab action buttons
          if (displayTabActionButtons)
          {
            g.DrawImageUnscaledAndClipped(Resources.collapsing_panel_header_tab_add,
              new Rectangle(Width - Resources.collapsing_panel_header_tab_add.Width -
                  Resources.collapsing_panel_header_tab_del.Width,
                0,
                Resources.collapsing_panel_header_tab_add.Width,
                Resources.collapsing_panel_header_tab_add.Height));

            g.DrawImageUnscaledAndClipped(Resources.collapsing_panel_header_tab_del,
              new Rectangle(Width - Resources.collapsing_panel_header_tab_del.Width,
                0,
                Resources.collapsing_panel_header_tab_del.Width,
                Resources.collapsing_panel_header_tab_del.Height));
          }

          // Draw tab action buttons
          if (displayCustomButtons)
          {
            foreach (CustomButton button in customButtons)
              button.Paint(g, activeCustomButton == button);
          }
        }
      }

      // If the header is expanded
      if (Expanded)
      {
        // Draw the Minus icon
        if (!disableExpansionIcon && !disableExpansion && showHeader)
        {
          Bitmap icon = (style == CollapsingPanelStyle.Convex) ?
            Resources.collapsing_panel_minus : Resources.collapsing_panel_minus_flat;
          g.DrawImageUnscaled(icon, 10, (headerHeight - icon.Height) / 2);
        }

        // Draw shadow
        if (showHeader)
        {
          // If the tabs are not displayed, draw the background of the shadow
          if (DisplayMode != CollapsingPanelDisplayMode.HeaderAndTab &&
            DisplayMode != CollapsingPanelDisplayMode.TabsOnly &&
            style != CollapsingPanelStyle.Flat)
          {
            g.FillRectangle(SystemBrushes.ButtonFace, 0, headerHeight, Width, Padding.Top + headerSpace);
          }
          else if (DisplayMode == CollapsingPanelDisplayMode.HeaderAndTab)
          {
            // If there are tabs below the header, draw their background
            if (style == CollapsingPanelStyle.Convex)
              g.FillRectangle(SystemBrushes.ButtonFace, 0, headerHeight, Width, Padding.Top + tabsHeight);
            else
              using (SolidBrush background = new SolidBrush(Color.FromArgb(242, 242, 242)))
                g.FillRectangle(background, 0, headerHeight, Width, Padding.Top + tabsHeight);
            g.DrawLine(SystemPens.ControlDark, 0, headerHeight + Padding.Top + tabsHeight - 1,
              Width, headerHeight + Padding.Top + tabsHeight - 1);
          }

          // Draw shadow image
          if (style != CollapsingPanelStyle.Flat)
          {
            Rectangle shadowRect = new Rectangle(0, headerHeight,
              Width + 30, Resources.collapsing_panel_header_shadow.Height);
            g.DrawImageUnscaledAndClipped(Resources.collapsing_panel_header_shadow,
              shadowRect);
          }
        }
      }
      else
        // Draw the Plus icon
        if (!disableExpansionIcon && !disableExpansion && showHeader)
          g.DrawImageUnscaled(style == CollapsingPanelStyle.Convex ?
            Resources.collapsing_panel_plus : Resources.collapsing_panel_plus_flat, 10, 6);


      // If tabs should be displayed
      if (Expanded && showTabs)
      {
        // Draw the tab header background for TabsOnly
        if (DisplayMode == CollapsingPanelDisplayMode.TabsOnly)
        {
          if (style == CollapsingPanelStyle.Convex)
            g.FillRectangle(SystemBrushes.ButtonFace, 0, 0, Width, headerHeightTabsOnly);
          else
            using (SolidBrush background = new SolidBrush(Color.FromArgb(242, 242, 242)))
              g.FillRectangle(background, 0, 0, Width, headerHeightTabsOnly);

          g.DrawLine(SystemPens.ControlDark, 0, headerHeightTabsOnly - 1,
            Width, headerHeightTabsOnly - 1);
          g.DrawImageUnscaledAndClipped(Resources.collapsing_panel_bg,
            new Rectangle(0, 0, Width + 30, Resources.collapsing_panel_bg.Height));
        }

        // Get the current number of tabs.
        int tabCount;
        if (TabCountNeeded != null)
          tabCount = TabCountNeeded(this);
        else
          tabCount = 2;

        cachedTabWidths.Clear();

        // The X position of the current item
        int itemPos = tabPaintOffset + ((DisplayMode == CollapsingPanelDisplayMode.HeaderWithTabs) ?
          (disableExpansion ? 0 : 32) : 8);
        // The Y helper position
        int y = (DisplayMode == CollapsingPanelDisplayMode.TabsOnly) ?
          7 : headerHeight + Padding.Top;

        // Loop over the tab headers and draw them
        for (int i = 0; i < tabCount; i++)
        {
          int x = itemPos;
          int itemWidth = 0;
          string caption = "", description = "";
          int iconId = -1;
          bool drawIcon = false;
          float captionWidth = 0, descriptionWidth = 0;

          try
          {
            if (TabInfoNeeded != null)
            {
              if (!TabInfoNeeded(this, i, out iconId, out caption, out description))
                continue;
            }
            else
            {
              caption = "Caption";
              description = "Description";
            }

            if (DisplayMode == CollapsingPanelDisplayMode.HeaderWithTabs)
            {
              // Get caption width
              captionWidth = g.MeasureString(caption, HeaderFont).Width;

              // Check if the is the selected tab
              if (i == selectedTabIndex)
              {
                Bitmap tabLeft = style == CollapsingPanelStyle.Convex ?
                  Resources.collapsing_panel_header_tab_left :
                  Resources.collapsing_panel_header_tab_left_flat;
                Bitmap tabMiddle = style == CollapsingPanelStyle.Convex ?
                  Resources.collapsing_panel_header_tab_middle :
                  Resources.collapsing_panel_header_tab_middle_flat;
                Bitmap tabRight = style == CollapsingPanelStyle.Convex ?
                  Resources.collapsing_panel_header_tab_right :
                  Resources.collapsing_panel_header_tab_right_flat;

                if (DisableExpansion && i > 0)
                {
                  g.DrawImageUnscaled(tabLeft, x, 0);
                  x += tabLeft.Width;
                  itemWidth += tabLeft.Width;
                }

                g.DrawImageUnscaledAndClipped(tabMiddle, new Rectangle(x, 0,
                  Convert.ToInt16(captionWidth) + 16, tabMiddle.Height));

                x += 8;
                itemWidth += 8;

                // Draw the header caption
                if (style == CollapsingPanelStyle.Convex)
                  g.DrawString(caption, HeaderFont, Brushes.Black,
                    x, Expanded ? 3 : 1);
                else
                {
                  g.DrawString(caption, HeaderFont, fontDarkBrush,
                    x, 2);
                  g.DrawString(caption, HeaderFont, Brushes.White,
                    x, 1);
                }

                x += Convert.ToInt16(captionWidth) + 8;
                itemWidth += Convert.ToInt16(captionWidth) + 8;

                g.DrawImageUnscaled(tabRight, x, 0);

                itemWidth += tabRight.Width;
              }
              // if the tab is not selected
              else
              {
                Bitmap sep = style == CollapsingPanelStyle.Convex ?
                  Resources.collapsing_panel_header_tab_separator :
                  Resources.collapsing_panel_header_tab_separator_flat;

                // Draw first separator
                if (!DisableExpansion && i == 0)
                {
                  g.DrawImageUnscaled(sep, x, 0);

                  itemWidth = sep.Width;
                  x += itemWidth;
                }

                // space to last separator
                itemWidth += 8;
                x += 8;

                // Draw the header caption
                g.DrawString(caption, HeaderFont, style == CollapsingPanelStyle.Convex ?
                  Brushes.White : fontDarkBrush, x, 1);

                // Add caption width
                x += Convert.ToInt16(captionWidth) + 8;
                itemWidth += Convert.ToInt16(captionWidth) + 8;

                // Draw ending separator only if the next tab is not selected
                if (i != selectedTabIndex - 1)
                {
                  g.DrawImageUnscaled(sep, x, 0);

                  itemWidth += sep.Width;
                }
              }
            }
            else
            {
              // Check if an icon is assigned to this tab
              if (tabHeaderImageList != null &&
                iconId >= 0 && iconId < tabHeaderImageList.Images.Count)
              {
                drawIcon = true;

                // Add icon width and spacing
                itemWidth = tabHeaderImageList.ImageSize.Width + 16;
              }

              // Get caption and description width
              if (!caption.Equals(""))
                captionWidth = g.MeasureString(caption, tabHeaderCaptionFont).Width;

              if (!description.Equals(""))
                descriptionWidth = g.MeasureString(description, tabHeaderDescriptionFont).Width;

              // Add the longer text with to the itemWidth
              itemWidth += (int)Math.Ceiling(Math.Max(captionWidth, descriptionWidth));

              // If there was a text at all, add spacing
              if (captionWidth + descriptionWidth > 0)
                if (drawIcon)
                  itemWidth += 6;
                else
                  itemWidth += 16;

              // if there is nothing to draw, continue
              if (itemWidth == 0)
                continue;

              // Check if the is the selected tab
              if (i == selectedTabIndex)
              {
                // Get rectangle for the middle image
                Rectangle middleRect = new Rectangle(
                  itemPos + Resources.collapsing_panel_tab_left.Width,
                  y, itemWidth -
                    Resources.collapsing_panel_tab_left.Width -
                    Resources.collapsing_panel_tab_right.Width + 1,
                    Resources.collapsing_panel_tab_middle.Height);

                // Draw the left border, middle image and right border
                g.DrawImageUnscaled(Resources.collapsing_panel_tab_left, itemPos,
                  y);
                g.DrawImageUnscaledAndClipped(Resources.collapsing_panel_tab_middle,
                  middleRect);
                g.DrawImage(Resources.collapsing_panel_tab_right,
                  itemPos + itemWidth - Resources.collapsing_panel_tab_right.Width,
                  y);
              }


              // indent for the icon or text
              x += 8;

              // Draw the icon
              if (drawIcon)
              {
                g.DrawImageUnscaled(tabHeaderImageList.Images[iconId],
                  x, (!showHeader ? 0 : headerHeight + Padding.Top) +
                    (((!showHeader) ? headerHeightTabsOnly : tabsHeight) -
                    tabHeaderImageList.Images[iconId].Height) / 2);

                x += tabHeaderImageList.ImageSize.Width + 6;
              }

              // Draw the caption
              if (!caption.Equals(""))
                g.DrawString(caption, tabHeaderCaptionFont, Brushes.Black,
                  x, y + 15);
              // Draw the description
              if (!description.Equals(""))
                g.DrawString(description, tabHeaderDescriptionFont, Brushes.Gray,
                  x, y + 15 + 16 + descriptionSpace);
            }

            itemPos += itemWidth;

            // Don't try to optimize painting here by dropping out early if the available space
            // is filled up. The paint code also computes the cachedTabWidths array.
          }
          finally
          {
            cachedTabWidths.Add(itemWidth);
          }
        }

        // Draw popup button as overlay. Compute its position here too (for hit testing).
        popupButtonBounds.Width = Resources.DockPaneCaption_Options1.Width;
        popupButtonBounds.Height = Resources.DockPaneCaption_Options1.Height;
        popupButtonBounds.X = Width - Resources.DockPaneCaption_Options1.Width - 4;
        popupButtonBounds.Y = headerHeight + (showHeader ? tabsHeight : 0) - popupButtonBounds.Height;
        g.DrawImageUnscaled(Resources.DockPaneCaption_Options1, popupButtonBounds.X, popupButtonBounds.Y);

        // Draw the background of the shadow below the tab header
        using (SolidBrush b = new SolidBrush(BackColor))
          g.FillRectangle(b, 0, headerHeight + Padding.Top + tabsHeight, Width, headerSpace);
      }

      if (showHeader)
      {
        float h = g.MeasureString(HeaderCaption, HeaderFont).Height;

        // Draw the header caption
        g.DrawString(HeaderCaption, HeaderFont,
          style == CollapsingPanelStyle.Convex ? Brushes.White : fontDarkBrush,
          (disableExpansion ? 6 : 30), Convert.ToInt16((headerHeight - h) / 2));
      }
    }

    private Control GetPreviousControl()
    {
      if (Parent != null)
      {
        int i = Parent.Controls.IndexOf(this);
        if (i < Parent.Controls.Count - 1)
        {
          Control c = Parent.Controls[i + 1];

          while (c is CollapsingPanel && !(c as CollapsingPanel).Expanded &&
            i < Parent.Controls.Count - 2)
            c = Parent.Controls[++i + 1];

          if (!(c is CollapsingPanel) || (c as CollapsingPanel).Expanded)
            return c;
        }
      }

      return null;
    }

    protected override void OnMouseDown(MouseEventArgs e)
    {
      if (resizable && (e.Button == MouseButtons.Left) && (e.X > 10 + 12) && (e.X < buttonOffset) && 
        (e.Y < headerHeight / 3))
      {
        Control c = GetPreviousControl();
        if (c != null)
        {
          doPreviousControlResize = true;
          mouseDownPoint = this.PointToScreen(e.Location);
          prevControlHeight = c.Height;
        }
      }

      base.OnMouseDown(e);
    }

    protected override void OnMouseMove(MouseEventArgs e)
    {
      if (resizable && (e.Y < headerHeight / 3))
      {
        if ((e.X >= 10 + 12 || DisableExpansion)
          && (TabAddButtonClicked == null || (e.X < buttonOffset))
          && GetPreviousControl() != null)
        {
          if (Cursor != Cursors.HSplit)
            Cursor = Cursors.HSplit;
        }
        else
          Cursor = Cursors.Default;
      }
      else
        Cursor = Cursors.Default;

      if (doPreviousControlResize && e.Button == MouseButtons.Left)
      {
        Control c = GetPreviousControl();
        if (c != null)
        {
          c.Height = prevControlHeight + this.PointToScreen(e.Location).Y - mouseDownPoint.Y;
          c.Update();
        }
      }

      base.OnMouseMove(e);
    }

    protected override void OnMouseUp(MouseEventArgs e)
    {
      if (doPreviousControlResize)
        doPreviousControlResize = false;

      if (TabHeaderMouseUp != null && e.Button == MouseButtons.Right)
        TabHeaderMouseUp(this, e);

      base.OnMouseUp(e);
    }

    protected override void OnMouseClick(MouseEventArgs e)
    {
      bool handled = false;

      if (e.Button == MouseButtons.Left)
      {
        // Check for the tab popup. The bounding rectangle is only set if there are tabs.
        if (popupButtonBounds.Contains(new Point(e.X, e.Y)))
        {
          handled = true;
          tabMenu.Show(this, new Point(popupButtonBounds.X, popupButtonBounds.Y + popupButtonBounds.Height));
        }

        // Change the expanded state only when header is not hidden
        if (!handled && !disableExpansion && DisplayMode != CollapsingPanelDisplayMode.TabsOnly &&
          DisplayMode != CollapsingPanelDisplayMode.NoHeader
          && e.Y < headerHeight)
        {
          if (e.X <= 10 + 12)
          {
            if (!disableExpansionIcon)
              Expanded = !Expanded;
            handled = true;
          }
          else
          {
            int offsetX = Width;

            // Test for - button hit.
            if (displayTabActionButtons)
            {
              offsetX -= Resources.collapsing_panel_header_tab_del.Width;
              if (e.X > offsetX && TabDelButtonClicked != null)
              {
                TabDelButtonClicked(this, new EventArgs());
                handled = true;
              }
              else
              {
                // + button hit.
                offsetX -= Resources.collapsing_panel_header_tab_add.Width;
                if (e.X > offsetX && TabAddButtonClicked != null)
                {
                  TabAddButtonClicked(this, new EventArgs());
                  SelectedTabIndex = cachedTabWidths.Count;
                  handled = true;
                }
              }
              ScrollIntoView(selectedTabIndex);
            }

            if (!handled)
            {
              // Any custom button.
              if (CustomButtonClicked != null)
              {
                for (int index = customButtons.Count - 1; index >= 0; index--)
                {
                  CustomButton button = customButtons[index];
                  offsetX -= button.boundingBox.Width;
                  if (e.X > offsetX)
                  {
                    if (activeCustomButton != null)
                      Invalidate(activeCustomButton.boundingBox);

                    activeCustomButton = button;
                    CustomButtonClicked(this, index);
                    handled = true;
                    Invalidate(activeCustomButton.boundingBox);

                    break;
                  }
                }
              }
            }
          }
        }

        if (!handled)
        {
          // Was a tab hit?
          int tabFound = GetTabAtPosition(e.X, e.Y);
          if (tabFound > -1)
          {
            SelectedTabIndex = tabFound;
            handled = true;
          }
        }
      }

      if (!handled)
        base.OnMouseClick(e);
    }

    protected override void OnMouseDoubleClick(MouseEventArgs e)
    {
      bool handled = false;

      if (e.Button == MouseButtons.Left)
      {
        // Switching the expand state.
        if (!disableExpansion && e.Y <= headerHeight && e.X < buttonOffset)
        {
          if (!disableExpansionIcon)
            Expanded = !Expanded;
          handled = true;
        }

        // Hitting the menu button (handle same as with single click).
        if (!handled && popupButtonBounds.Contains(e.Location))
        {
          handled = true;
          tabMenu.Show(this, new Point(popupButtonBounds.X, popupButtonBounds.Y + popupButtonBounds.Height));
        }

        if (!handled)
        {
          int tabFound = GetTabAtPosition(e.X, e.Y);
          handled = true;

          // Create a new tab if double clicked on free space.
          if (tabFound == -1 && TabAddButtonClicked != null)
          {
            TabAddButtonClicked(this, new EventArgs());
            SelectedTabIndex = cachedTabWidths.Count;
          }

          if (TabDoubleClicked != null)
            TabDoubleClicked(this, e);
        }
      }

      if (!handled)
        base.OnMouseDoubleClick(e);
    }

    private void refreshDockedControls()
    {
      foreach (Control c in this.Controls)
      {
        if (c.Dock != DockStyle.None)
        {
          DockStyle oldDockstyle = c.Dock;
          c.Dock = DockStyle.None;
          c.Dock = oldDockstyle;
        }
      }

      Invalidate();
    }

    protected override void OnResize(EventArgs eventargs)
    {
      base.OnResize(eventargs);

      ComputeButtonOffsets();
      ScrollIntoView(selectedTabIndex);
      Invalidate();
    }

    #endregion

    public int GetTabAtPosition(int xpos, int ypos)
    {
      if (DisplayMode == CollapsingPanelDisplayMode.HeaderAndTab ||
          DisplayMode == CollapsingPanelDisplayMode.TabsOnly ||
          DisplayMode == CollapsingPanelDisplayMode.HeaderWithTabs ||
          (xpos > headerHeight + Padding.Top && ypos < headerHeight + Padding.Top + tabsHeight))
      {
        int x = tabPaintOffset + ((DisplayMode == CollapsingPanelDisplayMode.HeaderWithTabs) ?
          (disableExpansion ? 0 : 32) : 8);

        for (int i = 0; i < cachedTabWidths.Count; i++)
        {
          if (i >= cachedTabWidths.Count)
            break;

          if (cachedTabWidths[i] == 0)
            continue;

          if (xpos >= x && xpos < x + cachedTabWidths[i])
          {
            return i;
          }

          x += cachedTabWidths[i];
        }
      }
      return -1;
    }

    /// <summary>
    /// Adds a new custom button to the header with the given image.
    /// </summary>
    /// <returns>
    /// The index of the button in the list of custom buttons. Can be used in the click handler.
    /// </returns>
    public int AddCustomButton(int width, int height, Bitmap bitmap, bool active)
    {
      CustomButton button = new CustomButton(bitmap, new Rectangle(0, 0, width, height));
      customButtons.Add(button);
      if (active)
        activeCustomButton = button;
      ComputeButtonOffsets();

      return customButtons.Count - 1;
    }

    /// <summary>
    /// This version allows to specify a resource identifier which is then used to load the
    /// specific bitmap.
    /// </summary>
    public int AddCustomButton(int width, int height, String id, bool active)
    {
      object obj = Resources.ResourceManager.GetObject(id, Resources.Culture);
      return AddCustomButton(width, height, (System.Drawing.Bitmap)(obj), active);
    }

    /// <summary>
    /// Removes all custom buttons.
    /// </summary>
    public void ClearCustomButtons()
    {
      activeCustomButton = null;
      customButtons.Clear();
      ComputeButtonOffsets();
    }

    /// <summary>
    /// Computes the left position of the most left button (either custom or action button) and
    /// stores it in a member for later use. Also the bounding box for all custom buttons is updated
    /// to reflect the real position.
    /// </summary>
    private void ComputeButtonOffsets()
    {
      // For now we don't need any vertical adjustment, just the horizontal offset for all buttons.
      buttonOffset = 0;
      if (displayTabActionButtons)
        buttonOffset = Resources.collapsing_panel_header_tab_add.Width +
          Resources.collapsing_panel_header_tab_del.Width;
      foreach (CustomButton button in customButtons)
        buttonOffset += button.boundingBox.Width;
      buttonOffset = Width - buttonOffset;

      // Do a second loop over all buttons and do the final update now that we have
      // the correct left offset.
      int offset = buttonOffset;
      foreach (CustomButton button in customButtons)
      {
        button.boundingBox.X = offset;
        offset += button.boundingBox.Width;
      }
    }

    public void AdjustExpandedHeight(int requiredDisplayArea)
    {
      // getHeaderOffset() already includes the top padding value.
      if (!resizable)
      {
        computedExpandedHeight = requiredDisplayArea + GetHeaderOffset() + Padding.Bottom;
        if (expanded)
          Height = computedExpandedHeight;
      }
    }

    /// <summary>
    /// Computes the current paint offest so that the given index is fully visible in the current
    /// tab area. If the tab area is smaller than the tabulator then the tabulator is left aligned
    /// in the available space.
    /// As a side effect, the function enusures there is no free space to the right of the tabs unless
    /// the total width of all tabs is less than the available space.
    /// </summary>
    /// <param name="value">The index of the tabulator to make visible.</param>
    public void ScrollIntoView(int index)
    {
      if (index < 0 || index >= cachedTabWidths.Count)
        return;

      // Compute the needed width for all tabs including the one to make visible and also
      // collect the overall width of all tabs.
      int tabWidth = 0;
      int totalWidth = 0;
      for (int i = 0; i < cachedTabWidths.Count; i++)
      {
        if (i <= index)
          tabWidth += cachedTabWidths[i];
        totalWidth += cachedTabWidths[i];
      }

      // Move the tabs so they are right aligned in the available space (if necessary).
      if (totalWidth + tabPaintOffset < Width)
        tabPaintOffset = Width - totalWidth;

      if (tabWidth + tabPaintOffset > Width)
        tabPaintOffset = Width - tabWidth;

      tabWidth -= cachedTabWidths[index];
      if (tabWidth + tabPaintOffset < 0)
        tabPaintOffset = -tabWidth;

      if (tabPaintOffset > 0)
        tabPaintOffset = 0;
    }

    #region Properties

    public override LayoutEngine LayoutEngine
    {
      get
      {
        if (layoutEngine == null)
          layoutEngine = new ColumnLayout();
        return layoutEngine;
      }
    }

    public override Rectangle DisplayRectangle
    {
      get
      {
        // get the current header (+ tabs) height
        int h = GetHeaderOffset();

        // Modify current DisplayRectangle and subtract our header + tab area.
        Rectangle r = base.DisplayRectangle;
        r.Y += h;
        r.Height -= h;

        return r;
      }
    }

    /// <summary>
    /// Returns the needed space (height) for the header and tabs depending
    /// on the current display mode.
    /// </summary>
    /// <returns></returns>
    private int GetHeaderOffset()
    {
      int h;

      switch (DisplayMode)
      {
        case CollapsingPanelDisplayMode.HeaderAndTab:
          h = headerHeight + Padding.Top + headerSpace + tabsHeight;
          break;
        case CollapsingPanelDisplayMode.TabsOnly:
          h = headerHeightTabsOnly + headerSpace;
          break;
        case CollapsingPanelDisplayMode.NoHeader:
          h = 0;
          break;
        default:
          if (style == CollapsingPanelStyle.Flat)
            h = headerHeight;
          else
            h = headerHeight + Padding.Top + headerSpace;
          break;
      }
      return h;
    }

    [Bindable(true), Category("Appearance"),
    Description("The mode the panel is displayed.")]
    public CollapsingPanelDisplayMode DisplayMode
    {
      get { return displayMode; }
      set
      {
        displayMode = value;
        Invalidate();
      }
    }

    [Bindable(true), Category("Appearance"),
    Description("The visual style of the panel.")]
    public CollapsingPanelStyle Style
    {
      get { return style; }
      set
      {
        if (style != value)
        {
          style = value;

          if (style == CollapsingPanelStyle.Flat &&
            displayMode == CollapsingPanelDisplayMode.HeaderWithTabs)
            headerHeight = 25;
          else
            headerHeight = 23;

          if (!Expanded)
            Height = headerHeight;
          else if (computedExpandedHeight > -1)
            Height = computedExpandedHeight;

          Invalidate();
        }
      }
    }

    [Bindable(true), Category("Appearance"),
    Description("The caption displayed in the panel header.")]
    public string HeaderCaption
    {
      get { return headerCaption; }
      set
      {
        headerCaption = value;
        Invalidate();
      }
    }

    [Bindable(true), Category("Appearance"),
    Description("The Font used for the panel header.")]
    public Font HeaderFont
    {
      get { return headerFont; }
      set
      {
        headerFont = value;
        Invalidate();
      }
    }

    [Bindable(true), Category("Appearance"),
    Description("The vertical space between the header and the client region.")]
    public int HeaderSpace
    {
      get { return headerSpace; }
      set
      {
        headerSpace = value;

        refreshDockedControls();
      }
    }

    [Bindable(true), Category("Appearance"),
    Description("The vertical space used for the header bar.")]
    public int HeaderHeight
    {
      get { return headerHeight; }
      set
      {
        headerHeight = value;
        Invalidate();
      }
    }

    [Bindable(true), Category("Appearance"),
    Description("The Font used for the tab header captions.")]
    public Font TabHeaderCaptionFont
    {
      get { return tabHeaderCaptionFont; }
      set
      {
        tabHeaderCaptionFont = value;
        Invalidate();
      }
    }

    [Bindable(true), Category("Appearance"),
    Description("The Font used for the tab header descriptions.")]
    public Font TabHeaderDescriptionFont
    {
      get { return tabHeaderDescriptionFont; }
      set
      {
        tabHeaderDescriptionFont = value;
        Invalidate();
      }
    }

    [Bindable(true), Category("Appearance"),
    Description("The ImageList holding the icons of the tab headers.")]
    public ImageList TabHeaderImageList
    {
      get { return tabHeaderImageList; }
      set { tabHeaderImageList = value; }
    }

    [Bindable(true), Category("Behavior"), DefaultValue(false),
    Description("Specifies the expanding state of the control.")]
    public bool Resizable
    {
      get { return resizable; }
      set
      {
        if (resizable != value)
        {
          resizable = value;
          Invalidate();
        }
      }
    }

    [Bindable(true), Category("Behavior"), DefaultValue(true),
    Description("Specifies the expanded state of the control.")]
    public bool Expanded
    {
      get { return expanded; }
      set
      {
        if (expanded != value)
        {
          expanded = value;
          if (!expanded)
          {
            if (resizable)
              manualHeight = Height;
            Height = headerHeight;
          }
          else
          {
            // Let the layout engine determine which height we need if not manually resizable.
            // It sets computedExpandedHeight to what is necessary.
            if (resizable)
              Height = manualHeight;
            else
            {
              PerformLayout();
              Height = computedExpandedHeight;
            }
          }
          Invalidate();
        }
      }
    }

    [Bindable(true), Category("Behavior"), DefaultValue(false),
    Description("Removes the expension icon and deactivates the expension of doubleclick.")]
    public bool DisableExpansion
    {
      get { return disableExpansion; }
      set
      {
        if (value != disableExpansion)
        {
          disableExpansion = value;
          Invalidate();
        }
      }
    }

    [Bindable(true), Category("Behavior"), DefaultValue(false),
    Description("Removes the expension icon disregarding whether expension is enabled or not.")]
    public bool DisableExpansionIcon
    {
      get { return disableExpansionIcon; }
      set
      {
        if (value != disableExpansionIcon)
        {
          disableExpansionIcon = value;
          Invalidate();
        }
      }
    }

    [Bindable(true), Category("Behavior"),
    Description("Specifies the index of the currently selected tab. The tab index is 1 based.")]
    public int SelectedTabIndex
    {
      get { return selectedTabIndex; }
      set
      {
        if (TabCountNeeded != null)
        {
          int tabCount = TabCountNeeded(this);
          if (tabCount <= value)
            return;
        }

        if (TabChanging != null)
          if (!TabChanging(this, value))
            return;

        if (selectedTabIndex != value)
        {
          selectedTabIndex = value;

          if (TabChanged != null)
            TabChanged(this, selectedTabIndex);

          ScrollIntoView(value);
          Invalidate();
        }
      }
    }

    [Bindable(true), Category("Behavior"),
    Description("If set to true the tab action buttons will be displayed and activated.")]
    public bool DisplayTabActionButtons
    {
      get { return displayTabActionButtons; }
      set
      {
        if (value != displayTabActionButtons)
        {
          displayTabActionButtons = value;
          ComputeButtonOffsets();
          Invalidate();
        }
      }
    }

    [Bindable(true), Category("Behavior"),
    Description("If set to true the custom buttons will be displayed and activated.")]
    public bool DisplayCustomButtons
    {
      get { return displayCustomButtons; }
      set
      {
        if (value != displayCustomButtons)
        {
          displayCustomButtons = value;
          ComputeButtonOffsets();
          Invalidate();
        }
      }
    }

    [Bindable(true), Category("Behavior"),
    Description("Allows to activate a certain custom button by code.")]
    public int ActiveCustomButton
    {
      get
      {
        for (int i = 0; i < customButtons.Count; i++)
          if (customButtons[i] == activeCustomButton)
            return i;
        return -1;
      }
      set
      {
        if (activeCustomButton != null)
          Invalidate(activeCustomButton.boundingBox);

        // Be tolerant with indices out of bounds.
        if (value >= 0 && value < customButtons.Count)
        {
          activeCustomButton = customButtons[value];
          CustomButtonClicked(this, value);
          Invalidate(activeCustomButton.boundingBox);
        }
        else
          activeCustomButton = null;
      }
    }

    #endregion

    #region Control Events

    [Category("Action")]
    public event CustomButtonEvent CustomButtonClicked;

    [Category("Behavior")]
    public event TabInfoHandler TabInfoNeeded;

    [Category("Behavior")]
    public event TabCountHandler TabCountNeeded;

    [Category("Action")]
    public event TabChangingHandler TabChanging;

    [Category("Action")]
    public event TabChangedHandler TabChanged;

    [Category("Action")]
    public event EventHandler TabAddButtonClicked;

    [Category("Action")]
    public event EventHandler TabDelButtonClicked;

    [Category("Action")]
    public event MouseEventHandler TabDoubleClicked;

    [Category("Action")]
    public event MouseEventHandler TabHeaderMouseUp;

    #endregion
  }

  public class ColumnLayout : LayoutEngine
  {
    public override bool Layout(object container, LayoutEventArgs layoutEventArgs)
    {
      CollapsingPanel parent = container as CollapsingPanel;

      // Use DisplayRectangle so that parent.Padding is honored.
      Rectangle parentDisplayRectangle = parent.DisplayRectangle;
      Point nextControlLocation = parentDisplayRectangle.Location;

      // Loop over all controls and compute the overall size needed. Also move the controls to
      // their final place.
      int totalHeight = 0;
      foreach (Control c in parent.Controls)
      {
        // Only apply layout to visible controls.
        if (!c.Visible)
          continue;

        // Respect the margin of the control:
        // shift over the left and the top.
        nextControlLocation.Offset(c.Margin.Left, c.Margin.Top);

        // Set the location of the control and stretch it over the full width.
        if (c.Dock != DockStyle.Fill)
        {
          c.Location = nextControlLocation;
          c.Width = parentDisplayRectangle.Width - c.Margin.Left - c.Margin.Right;

          // Set the auto sized controls to their auto sized heights.
          if (c.AutoSize)
          {
            // Handle Listview specially.
            if (c is ListView)
            {
              ListView listView = c as ListView;
              uint rawSize = Win32.SendMessage(listView.Handle, (int)Win32.LVM_APPROXIMATEVIEWRECT,
                unchecked((uint) -1), Win32.MakeLParam(ushort.MaxValue, ushort.MaxValue));

              int singleSize = Win32.HiWord(rawSize);

              // Nothing is perfect and neither is Listview's space computation. There is too much room in
              // large icon mode and a bit too few in tile mode. Correct that.
              switch (listView.View)
              {
                case View.LargeIcon:
                  singleSize -= 20;
                  if (singleSize < 0)
                    singleSize = 0;
                  break;
                case View.Tile:
                  singleSize += 5;
                  break;
              }
              if (listView.Groups.Count == 0)
                c.Height = singleSize;
              else
                c.Height = listView.Groups.Count * singleSize;
            }
            else
            {
              Size size = c.GetPreferredSize(new Size(parentDisplayRectangle.Width, int.MaxValue));
              c.Height = size.Height;
            }
          }
        }
        else
        {
          // If a control is set to fill the full area then we cannot layout it differently.
          c.SetBounds(parentDisplayRectangle.Left, parentDisplayRectangle.Top, parentDisplayRectangle.Width,
            parentDisplayRectangle.Height);
        }

        // Move X back to the display rectangle origin.
        nextControlLocation.X = parentDisplayRectangle.X;

        // Increment Y by the height of the control 
        // and the bottom margin.
        nextControlLocation.Y += c.Height + c.Margin.Bottom;

        totalHeight += c.Height + c.Margin.Top + c.Margin.Bottom;
      }

      parent.AdjustExpandedHeight(totalHeight);

      return false;
    }
  }

}
