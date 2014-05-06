/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
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
