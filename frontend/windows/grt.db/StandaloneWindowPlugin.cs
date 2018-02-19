/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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
using System.Windows.Forms;
using System.Drawing;
using System.Runtime.InteropServices;

using MySQL.Utilities.SysUtils;
using System.IO;

namespace MySQL.GUI.Workbench.Plugins
{
  public partial class StandaloneWindowPlugin : ObjectEditorView
  {
    public StandaloneWindowPlugin(ObjectEditorPlugin EditorPlugin)
      : base(EditorPlugin)
    {
      InitializeComponent();
      
      Text = EditorPlugin.TabText;
      TopLevel = true;
      FormBorderStyle = FormBorderStyle.SizableToolWindow;
      if (File.Exists("images/icons/MySQLWorkbench.ico"))
        Icon= new Icon("images/icons/MySQLWorkbench.ico", new Size(16, 16));
    }

    #region Native Code

    protected override void WndProc(ref Message m)
    {
      base.WndProc(ref m);
      switch ((WM)m.Msg)
      {
        case WM.SETTINGCHANGE:
          if ((SPI) m.WParam.ToInt32() == SPI.SETWORKAREA)
          {
            if (WindowState == FormWindowState.Maximized)
            {
              // When the taskbar or other docked windows change the available work area
              // then we need to adjust ourself if we are currently maximized.
              // However this works only if we shortly get unmaximized.
              WindowState = FormWindowState.Normal;
              WindowState = FormWindowState.Maximized;
            }
          }
          break;

        case WM.GETMINMAXINFO:
          {
            // We need to handle this event here because on certain resize actions
            // and taskbar positions parts of the window are hidden. We need to ensure this
            // doesn't happen.
            Screen currentScreen = Screen.FromControl(this);
            Rectangle validArea = currentScreen.WorkingArea;

            Win32.MINMAXINFO info = (Win32.MINMAXINFO)Marshal.PtrToStructure(m.LParam, typeof(Win32.MINMAXINFO));

            // Don't use the position of the screen (or its valid area). The position given here
            // is local to the screen area.
            info.ptMaxPosition.x = 0;
            info.ptMaxPosition.y = 0;
            if (info.ptMaxSize.x > validArea.Width)
              info.ptMaxSize.x = validArea.Width;
            if (info.ptMaxSize.y > validArea.Height)
              info.ptMaxSize.y = validArea.Height;

            Marshal.StructureToPtr(info, m.LParam, true);
            m.Result = IntPtr.Zero;

            break;
          }
      }
    }

    #endregion
  }
}