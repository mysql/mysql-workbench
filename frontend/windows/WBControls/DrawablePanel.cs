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
using System.Drawing;
using System.Windows.Forms;

using MySQL.Utilities;

namespace MySQL.Controls
{
  public class DrawablePanel : Panel
  {
    private bool customBackground = false;

    public DrawablePanel()
    {
      DoubleBuffered = true;
    }

    public bool CustomBackground
    {
      get { return customBackground; }
      set { customBackground = value; }
    }

    public event EventHandler<PaintEventArgs> PaintBackground;
    protected override void OnPaintBackground(PaintEventArgs e)
    {
      if (!customBackground && BackColor != Color.Transparent)
      {
        // With Aero enabled a panel behaves strange when placed on a transparent window.
        if (ControlUtilities.IsCompositionEnabled())
          using (SolidBrush brush = new SolidBrush(BackColor))
            e.Graphics.FillRectangle(brush, ClientRectangle);
        else
          base.OnPaintBackground(e);
      }
      
      // Since there is no event handler triggered in the ancestor we do it here.

      if (PaintBackground != null)
        PaintBackground(this, e);
    }
  }
}
