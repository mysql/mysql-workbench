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
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

using MySQL.Utilities.SysUtils;
using System.Threading;

namespace MySQL.Utilities
{
  public partial class HUDForm : Form
  {
    // Note: currentInstance relies on the fact that we never have more than one HUDForm instance.
    static private HUDForm currentInstance = null;
    static private Object instanceLock = new Object();

    public delegate bool CancelDelegate();
    public delegate void ReadyDelegate();

    private Size baseSize;
    private float borderSize = 3;
    private float shadowSize = 20;
    private Color shadowColor = Color.Black;
    private float shadowOffset = 5;
    private float cornerSize = 20;
    private int animationSteps = 6;
    private bool useAnimation = false;
    private RectangleF cancelButtonBounds;

    private CancelDelegate cancelDelegate = null;
    private Bitmap contentBitmap;

    HUDForm()
    {
      InitializeComponent();
      baseSize = Size;
      SetStyle(ControlStyles.Selectable, false);
      UpdateStyles();

      cancelButtonPicture.UseWaitCursor = false;
      cancelButtonPicture.Cursor = Cursors.AppStarting;
    }

    public static DialogResult ShowModal(String title, String message, bool animated,
      ReadyDelegate notifyReady, CancelDelegate deleg)
    {
      using (HUDForm instance = new HUDForm())
      {
        // Don't use lock() here as we might dead lock otherwise.
        if (!Monitor.TryEnter(instanceLock))
        {
          Application.DoEvents(); // See if we can let the application finish pending stuff first.
          if (!Monitor.TryEnter(instanceLock))
            return DialogResult.Abort; // Still no lock possible? In this case we can only leave without showing the HUD form.
        }

        currentInstance = instance;
        Monitor.Exit(instanceLock);

        instance.label1.Text = title;
        instance.label2.Text = message;
        instance.cancelDelegate = deleg;

        // Can happen the form is cancelled already while we are still doing the setup 
        // (separate thread).
        if (instance.IsDisposed || instance.Disposing)
        {
          if (Monitor.TryEnter(instanceLock))
          {
            currentInstance = null;
            Monitor.Exit(instanceLock);
          }
          return DialogResult.Abort;
        }

        instance.Reshow(animated, true);

        // Need to set Visible back to false to avoid an exception in ShowDialog about an already
        // visible window. Doesn't usually have a visual effect.
        instance.Visible = false;

        if (notifyReady != null)
          notifyReady();

        DialogResult result = DialogResult.Abort;
        if (!instance.IsDisposed && !instance.Disposing)
          result = instance.ShowDialog();
        if (Monitor.TryEnter(instanceLock))
        {
          instance.Dispose();
          currentInstance = null;
          Monitor.Exit(instanceLock);
        }

        return result;
      }
    }

    public static void Show(String title, String message, bool animated)
    {
      lock (instanceLock) // Hard lock here. Must never happen.
      {
        HUDForm instance = new HUDForm();
        currentInstance = instance;
        instance.label1.Text = title;
        instance.label2.Text = message;

        instance.Reshow(animated, false);
      }
    }

    public void Reshow(bool animated, bool modal)
    {
      lock (instanceLock)
      {
        // If there's no current instance then we are just a left over that is going to
        // be closed/disposed shortly anyway.
        if (currentInstance == null)
          return;

        Form mainForm = Application.OpenForms["MainForm"];
        if (mainForm == null)
          throw new Exception("HUD display needs main form to be named \"MainForm\" to find it.");

        mainForm.Update();

        // The (transparent) HUD will cover the entire application window.
        Bounds = mainForm.Bounds;

        PrepareBitmap(modal);

        // Don't use animations in a terminal session (remote desktop).
        useAnimation = animated && !SystemInformation.TerminalServerSession;

        if (IsDisposed || Disposing)
          return;

        if (useAnimation)
        {
          ControlUtilities.SetBitmap(this, contentBitmap, 0);
          Win32.ShowWindow(Handle, (uint)SW.SHOWNOACTIVATE);
          for (float i = 1; i <= animationSteps; i++)
            ControlUtilities.SetBitmap(this, contentBitmap, (int)(i / animationSteps * 255));
        }
        else
        {
          ControlUtilities.SetBitmap(this, contentBitmap, 255);
          Win32.ShowWindow(Handle, (uint)SW.SHOWNOACTIVATE);
        }
      }
    }

    /// <summary>
    /// Hides the HUD form and releases any used bitmap resource.
    /// </summary>
    private void DoHide()
    {
      if (useAnimation && contentBitmap != null)
      {
        for (float i = animationSteps; i >= 0; i--)
          ControlUtilities.SetBitmap(this, contentBitmap, (int)(i / animationSteps * 255));
      }

      base.Hide();

      if (contentBitmap != null)
      {
        contentBitmap.Dispose();
        contentBitmap = null;
      }
      cancelDelegate = null;
    }

    /// <summary>
    /// Reintroduced Hide() function to allow a fade out animation.
    /// Call only from main thread.
    /// </summary>
    public new void Hide()
    {
      if (!IsHandleCreated)
        return;

      Form mainForm = Application.OpenForms["MainForm"];
      if (mainForm != null)
          mainForm.Update();

      DoHide();
    }

    /// <summary>
    /// Hide the HUD and return OK as dialog result.
    /// Always called in the context of the main thread with active lock.
    /// </summary>
    protected void DoFinish()
    {
      if (IsHandleCreated)
        Hide();
      this.DialogResult = DialogResult.Abort;
      Dispose();
      currentInstance = null;
    }

    /// <summary>
    /// Called when the HUD is no longer needed. Returns success for the modal result.
    /// </summary>
    public static void Finish()
    {
      if (currentInstance == null)
        return;

      lock (instanceLock)
      {
        if (currentInstance.InvokeRequired)
          currentInstance.Invoke((Action)(() => currentInstance.DoFinish()));
        else
          currentInstance.DoFinish();
      }
    }

    #region Properties

    public static bool IsVisible
    {
      get {
        return (currentInstance == null) ? false : currentInstance.Visible;
      }
    }

    #endregion

    #region Drawing

    protected GraphicsPath GetPath()
    {
      // Generate the outline of the actual content area. Center it around the origin.
      // It will later get transformed to the final position.
      GraphicsPath result = new GraphicsPath();

      float width = baseSize.Width;// -2 * borderSize; excluding the border here hasn't any effect it seems
      float height = baseSize.Height;// -2 * borderSize;
      RectangleF bounds = new RectangleF(-width / 2, -height / 2, width, height);
      result.AddArc(bounds.Left, bounds.Top, cornerSize, cornerSize, 180, 90);
      result.AddArc(bounds.Right - cornerSize, bounds.Top, cornerSize, cornerSize, -90, 90);
      result.AddArc(bounds.Right - cornerSize, bounds.Bottom - cornerSize, cornerSize, cornerSize, 0, 90);
      result.AddArc(bounds.Left, bounds.Bottom - cornerSize, cornerSize, cornerSize, 90, 90);
      result.CloseAllFigures();
      return result;
    }

    /// <summary>
    /// Prepares the bitmap used to draw the window. Layered windows (like this one) use a bitmap for their
    /// content, including alpha channel.
    /// </summary>
    protected void PrepareBitmap(bool showCancelButton)
    {
      if (contentBitmap != null)
        contentBitmap.Dispose();
      contentBitmap = new Bitmap(Width, Height);

      GraphicsPath path = GetPath();
      GraphicsPath innerPath = (GraphicsPath)path.Clone();

      // Increase size of the outline by the shadow size and move it to the center.
      // The inner path keeps the original bounds for clipping.
      Matrix matrix = new Matrix();

      float offsetX = Width / 2;
      float offsetY = Height / 2;
      matrix.Translate(offsetX + shadowOffset, offsetY + shadowOffset);
      matrix.Scale(1 + (2 * shadowSize + borderSize) / (float)baseSize.Width, 1 + (2 * shadowSize + borderSize) / (float)baseSize.Height);
      path.Transform(matrix);

      // Also move the inner part to its final place.
      matrix.Reset();
      matrix.Translate(offsetX, offsetY);
      innerPath.Transform(matrix);

      Graphics g = Graphics.FromImage(contentBitmap);
      g.SmoothingMode = SmoothingMode.HighQuality;
      using (Brush brush = new SolidBrush(Color.FromArgb(10, 0, 0, 0)))
        g.FillRectangle(brush, ClientRectangle);

      // Fill interior.
      using (Brush brush = new SolidBrush(Color.FromArgb(191, 0, 0, 0)))
        g.FillPath(brush, innerPath);

      // ... and draw border around the interior.
      using (Pen borderPen = new Pen(Color.FromArgb(200, Color.White)))
      {
        borderPen.EndCap = LineCap.Round;
        borderPen.StartCap = LineCap.Round;
        borderPen.Width = borderSize;
        GraphicsPath borderPath = (GraphicsPath)innerPath.Clone();
        borderPath.Widen(borderPen);
        using (SolidBrush brush = new SolidBrush(Color.FromArgb(255, Color.White)))
          g.FillPath(brush, borderPath);

        // Clip out interior. Exclude both, the panel itself as well as its border.
        using (Region region = new Region(innerPath))
          g.SetClip(region, CombineMode.Exclude);

        innerPath.Widen(borderPen);
        using (Region region = new Region(innerPath))
          g.SetClip(region, CombineMode.Exclude);
      }

      using (PathGradientBrush backStyle = new PathGradientBrush(path))
      {
        backStyle.CenterColor = shadowColor;
        backStyle.SurroundColors = new Color[] { Color.Transparent };

        // Make a smooth fade out of the shadow color using the built-in sigma bell curve generator.
        backStyle.SetSigmaBellShape(0.4f, 1f);

        // Now draw the shadow.
        g.FillPath(backStyle, path);
      }

      // Remove clipping for the remaining interior.
      g.ResetClip();
      RectangleF innerBounds = innerPath.GetBounds();
      Point targetLocation = pictureBox1.Location;
      targetLocation.Offset((int)innerBounds.Left, (int)innerBounds.Top);
      g.DrawImageUnscaled(pictureBox1.Image, targetLocation);

      // Message text output.
      using (StringFormat format = new StringFormat(StringFormatFlags.FitBlackBox))
      {
        targetLocation = label1.Location;
        targetLocation.Offset((int)innerBounds.Left, (int)innerBounds.Top);
        RectangleF textBounds = new RectangleF(targetLocation,
          new SizeF(
            innerBounds.Right - targetLocation.X - Padding.Right,
            innerBounds.Bottom - targetLocation.Y - Padding.Bottom
          )
        );

        path.Dispose();
        innerPath.Dispose();

        using (Brush textBrush = new SolidBrush(label1.ForeColor))
          g.DrawString(label1.Text, label1.Font, textBrush, textBounds, format);

        targetLocation = label2.Location;
        targetLocation.Offset((int)innerBounds.Left, (int)innerBounds.Top);
        textBounds = new RectangleF(targetLocation,
          new SizeF(
            innerBounds.Right - targetLocation.X - Padding.Right,
            innerBounds.Bottom - targetLocation.Y - Padding.Bottom
          )
        );

        using (Brush textBrush = new SolidBrush(label2.ForeColor))
          g.DrawString(label2.Text, label2.Font, textBrush, textBounds, format);

        if (showCancelButton)
        {
          Point buttonLocation = cancelButtonPicture.Location;
          buttonLocation.Offset((int)innerBounds.Left, (int)innerBounds.Top);
          cancelButtonBounds = new RectangleF(buttonLocation, cancelButtonPicture.Size);
          g.DrawImageUnscaled(cancelButtonPicture.Image, buttonLocation);
        }
        else
          cancelButtonBounds = new RectangleF();
      }
    }

    #endregion

    #region Properties

    protected override bool ShowWithoutActivation
    {
      get { return true; }
    }

    #endregion

    protected override CreateParams CreateParams	
		{
			get 
			{
				CreateParams cp = base.CreateParams;

        cp.ExStyle |= (int) WS.EX_LAYERED;
        cp.ExStyle |= (int) WS.EX_NOACTIVATE;
				return cp;
			}
		}

    #region Events and message handling

    protected override void WndProc(ref Message m)
    {
      switch ((WM) m.Msg)
      {
        case WM.LBUTTONDOWN:
          {
            int x = m.LParam.ToInt32() & 0xFFFF;
            int y = m.LParam.ToInt32() >> 16;
            if (cancelButtonBounds.Contains(x, y) && cancelDelegate != null && cancelDelegate())
            {
              Hide();
              DialogResult = DialogResult.Cancel;
              Close();
            }
            break;
          }
        case WM.SETCURSOR:
          {
            Point position = PointToClient(MousePosition);
            if (cancelButtonBounds.Contains(position))
            {
              // Check before set. Windows XP will otherwise cause an endless recursion.
              if (Cursor.Current != Cursors.AppStarting)
                Cursor.Current = Cursors.AppStarting;
            }
            else
            {
              if (Cursor.Current != Cursors.WaitCursor)
                Cursor.Current = Cursors.WaitCursor;
            }

            m.Result = new IntPtr(1);
            return;
          }
        case WM.MOUSEACTIVATE:
          m.Result = new IntPtr(Win32.MA_NOACTIVATE);
          return;
        case WM.ACTIVATEAPP:
          if (m.WParam.ToInt32() != 0)
            Win32.SetWindowPos(Handle, Win32.HWND_TOP, 0, 0, 0, 0,
              Win32.SWP_NOACTIVATE | Win32.SWP_NOMOVE | Win32.SWP_NOSIZE);
          break;
        case WM.WINDOWPOSCHANGED:
          Win32.SetWindowPos(Handle, Win32.HWND_TOP, 0, 0, 0, 0,
            Win32.SWP_NOACTIVATE | Win32.SWP_NOMOVE | Win32.SWP_NOSIZE);
          break;

      }
      base.WndProc(ref m);
    }

    #endregion

  }
}
