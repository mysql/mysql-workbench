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
using System.Drawing.Imaging;
using System.Reflection;
using System.Security.Principal;
using System.Windows.Forms;

using MySQL.Utilities.SysUtils;

namespace MySQL.Utilities
{
  /// <summary>
  /// Collection of helper functions working with .NET controls.
  /// </summary>
  public class ControlUtilities
  {
    /// <summary>
    /// Helper method to find out if this control or one of its children has the input focus.
    /// </summary>
    /// <returns></returns>
    public static bool IsHierarchyFocused(Control parent)
    {
      if (parent.Focused)
        return true;

      foreach (Control control in parent.Controls)
        if (IsHierarchyFocused(control))
          return true;

      return false;
    }

    /// <summary>
    /// Does a depth-first search for a control under root. 
    /// </summary>
    /// <returns>Returns the control with the given name or null if not found.</returns>
    public static Control FindControl(Control root, string target)
    {
      if (root.Name == target)
        return root;

      foreach (Control control in root.Controls)
      {
        Control child = FindControl(control, target);
        if (child != null)
          return child;
      }

      return null;
    }

    /// <summary>
    /// Determines if a control is a child within the hierarchy formed by root.
    /// Does a depth-first search for a control.
    /// </summary>
    /// <returns>Returns true if the control was found, false otherwise.</returns>
    public static bool ContainsControl(Control root, Control target)
    {
      if (root == target)
        return true;

      foreach (Control control in root.Controls)
      {
        if (ContainsControl(control, target))
          return true;
      }

      return false;
    }

    /// <summary>
    /// Returns the the currently active terminal control under parent.
    /// </summary>
    /// <param name="parent">The root of the control hierarchy to search in. This must not be null.</param>
    /// <returns>The currently active non-container control under parent.</returns>
    public static Control GetLeafActiveControl(ContainerControl parent)
    {
      Control control = parent.ActiveControl;
      while (control != null)
      {
        ContainerControl child;
        child = control as ContainerControl;
        if (child == null)
          break;
        if (child.ActiveControl == null)
          break;

        control = child.ActiveControl;
      }
      return control;
    }

    /// <summary>
    /// Prevents any drawing from happening in that control. Must be accompanied by ResumeControl or the control
    /// will never show any content anymore.
    /// </summary>
    /// <param name="control">The control to stop drawing.</param>

    private delegate void DelegateFunc();

    public static void SuspendDrawing(Control control)
    {
      if (control.IsHandleCreated)
      {
        if (control.InvokeRequired)
        {
          DelegateFunc f = delegate
          {
            Win32.SendMessage(control.Handle, (int)WM.SETREDRAW, 0, 0);
          }; 
          control.Invoke(f);
          return;
        }
        Win32.SendMessage(control.Handle, (int)WM.SETREDRAW, 0, 0);
      }
    }

    /// <summary>
    /// Counterpart to SuspendDrawing to re-enable drawing again for a control.
    /// </summary>
    /// <param name="control">The control to enable drawing.</param>
    public static void ResumeDrawing(Control control)
    {
      if (control.IsHandleCreated)
      {
        if (control.InvokeRequired)
        {
          DelegateFunc f = delegate
          {
            Win32.SendMessage(control.Handle, (int)WM.SETREDRAW, 1, 0);
          };
          control.Invoke(f);
          return;
        }
        Win32.SendMessage(control.Handle, (int)WM.SETREDRAW, 1, 0);
      }
    }

    /// <summary>
    /// Sets the given bitmap as window content with transparent parts via the
    /// layered windows API.
    /// </summary>
    /// <param name="bitmap">The bitmap to set.</param>
    /// <param name="opacity">The overall opacity (255 = opaque).</param>
    public static void SetBitmap(Control control, Bitmap bitmap, int opacity)
    {
      if (bitmap.PixelFormat != PixelFormat.Format32bppArgb)
        throw new ApplicationException("The bitmap must be 32ppp with alpha-channel.");

      IntPtr screenDc = Win32.GetDC(IntPtr.Zero);
      IntPtr memDc = Win32.CreateCompatibleDC(screenDc);
      IntPtr hBitmap = IntPtr.Zero;
      IntPtr oldBitmap = IntPtr.Zero;

      try
      {
        hBitmap = bitmap.GetHbitmap(Color.FromArgb(0));  // grab a GDI handle from this GDI+ bitmap
        oldBitmap = Win32.SelectObject(memDc, hBitmap);

        Win32.SIZE size = new Win32.SIZE(bitmap.Width, bitmap.Height);
        Win32.POINT pointSource = new Win32.POINT(0, 0);
        Win32.POINT topPos = new Win32.POINT(control.Left, control.Top);
        Win32.BLENDFUNCTION blend = new Win32.BLENDFUNCTION();
        blend.BlendOp = Win32.AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.SourceConstantAlpha = (byte)opacity;
        blend.AlphaFormat = Win32.AC_SRC_ALPHA;

        if (!control.IsDisposed && !control.Disposing)
        {
          Win32.UpdateLayeredWindow(control.Handle, screenDc, ref topPos, ref size, memDc,
            ref pointSource, 0, ref blend, Win32.ULW_ALPHA);
        }
      }
      finally
      {
        Win32.ReleaseDC(IntPtr.Zero, screenDc);
        if (hBitmap != IntPtr.Zero)
        {
          Win32.SelectObject(memDc, oldBitmap);
          Win32.DeleteObject(hBitmap);
        }
        Win32.DeleteDC(memDc);
      }
    }

    /// <summary>
    /// Returns a new font object by parsing the font description. This must be in the format:
    ///   [font family] [style] [size]
    /// Style and size are optional. Default values for them are Regular 12 pt.
    /// </summary>
    /// <param name="fontDescription"></param>
    /// <returns></returns>
    public static Font GetFont(String fontDescription)
    {
      // When parsing the font description keep in mind font family names can contain space characters
      // too, so parse from the end to filter out size and style first.
      String[] parts = fontDescription.Split(' ');
      int index = parts.Length - 1;

      int fontSize = 12;
      if (index > 0)
      {
        try
        {
          fontSize= int.Parse(parts[index]);
          --index;
        }
        catch
        {
          // Swallow any conversion error.
        }
      }

      FontStyle style = FontStyle.Regular;
      for (int i = 0; i < 2; i++)
      {
        if (index > 0)
        {
          if (parts[index].ToLower() == "bold")
          {
            style = style | FontStyle.Bold;
            --index;
          }
          else 
            if (parts[index].ToLower() == "italic")
            {
              style = style | FontStyle.Italic;
              --index;
            }
        }
      }

      String familyName = parts[0];
      for (int i = 1; i <= index; i++)
        familyName += " " + parts[i];

      return GetFont(familyName, fontSize, style);
    }

    /// <summary>
    /// Returns a new font object for the given parameters. If the given font family is not available
    /// then a generic font is returned instead.
    /// </summary>
    /// <param name="familyName">The name of the font family.</param>
    /// <param name="size">Size in points of the new font.</param>
    /// <param name="style">Font styles like bold, italic.</param>
    /// <returns>A new font object. This is guaranteed to be a valid font.</returns>
    public static Font GetFont(String familyName, float size)
    {
      return GetFont(familyName, size, FontStyle.Regular);
    }

    public static Font GetFont(String familyName, float size, FontStyle style)
    {
      Font result = null;
      FontFamily family = null;
      try
      {
        family = new FontFamily(familyName);
        if (family.IsStyleAvailable(style))
          result = new Font(familyName, size, style);
        else
        {
          // On some systems even the regular style throws an exception (which actually should not happen
          // as Windows should automatically pick an alternative font if the one requested does not exist, but
          // there are cases...).
          result = new Font(familyName, size, FontStyle.Regular);
        }
      }
      catch
      {
        result = new Font(FontFamily.GenericSansSerif, size, style);
      }

      if (family != null)
        family.Dispose();
      return result;
    }

    /// <summary>
    /// Determines if the application currently runs as full administrator. Returns false when run
    /// by limited users or restricted (i.e. un-elevated) administrators.
    /// </summary>
    /// <returns></returns>
    static public bool ApplicationHasAdminRights()
    {
      WindowsIdentity id = WindowsIdentity.GetCurrent();
      WindowsPrincipal principal = new WindowsPrincipal(id);
      return principal.IsInRole(WindowsBuiltInRole.Administrator);
    }

    // Windows 8                  6.2
    // Windows Server 2012        6.2
    // Windows 7                  6.1
    // Windows Server 2008 R2     6.1
    // Windows Server 2008        6.0
    // Windows Vista              6.0
    // Windows Server 2003 R2     5.2
    // Windows Server 2003        5.2
    // Windows XP 64-Bit Edition  5.2
    // Windows XP                 5.1
    // Windows 2000               5.0

    /// <summary>
    /// Determines if we are running on Vista or a newer OS.
    /// </summary>
    /// <returns>True if this is Vista, Windows 7 or above, otherwise false.</returns>
    static public bool IsVistaOrAbove()
    {
      OperatingSystem system = Environment.OSVersion;
      if (system.Platform != PlatformID.Win32NT)
        return false;

      return system.Version.Major > 5;
    }

    /// <summary>
    /// Ditto for Win8.
    /// </summary>
    static public bool IsWin8OrAbove()
    {
      OperatingSystem system = Environment.OSVersion;
      if (system.Platform != PlatformID.Win32NT)
        return false;

      return system.Version.Major > 6 || (system.Version.Major == 6 && system.Version.Minor >= 2);
    }

    /// <summary>
    /// Tells the caller if we are running on an Aero-enabled desktop.
    /// Note that on Win8 also composition is marked as enabled even though it doesn't use Aero.
    /// </summary>
    /// <returns>True, if Aero is available and enabled, otherwise false.</returns>
    static public bool IsCompositionEnabled()
    {
      if (!IsVistaOrAbove())
        return false;

      return Win32.DwmIsCompositionEnabled();
    }
    
    /// <summary>
    /// Since the default size of a control is protected we use reflection to read it.
    /// </summary>
    /// <param name="control"></param>
    /// <returns></returns>
    static public Size GetDefaultSize(Control control)
    {
      PropertyInfo pi = control.GetType().GetProperty("DefaultSize", BindingFlags.NonPublic | BindingFlags.Instance);
      return (Size)pi.GetValue(control, null);
    }

  } // ControlUtilities
} // MySQL.Utilities
