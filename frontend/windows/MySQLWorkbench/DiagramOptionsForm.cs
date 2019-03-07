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
using System.Drawing.Drawing2D;
using System.Windows.Forms;

using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  public partial class DiagramOptionsForm : Form
  {
    WbContext wbContext;
    DiagramOptionsBE optionsBE= null;
    MySQL.GUI.Mdc.WindowsGDICanvasView canvas;


    public DiagramOptionsForm()
    {
      InitializeComponent();
    }

    public DiagramOptionsForm(WbContext wbContext)
    {
      InitializeComponent();

      contentPanel.CustomBackground = true;

      this.wbContext = wbContext;

      canvas = new MySQL.GUI.Mdc.WindowsGDICanvasView(contentPanel.Handle,
        contentPanel.Width, contentPanel.Height);

      canvas.set_on_queue_repaint(canvasNeedsRepaint);

      canvas.initialize();

      optionsBE = new DiagramOptionsBE(canvas, wbContext, PropertyChanged);

      optionsBE.update_size();

      diagramNameEdit.Text = optionsBE.get_name();
      PropertyChanged();
    }


    void PropertyChanged()
    {
      widthUpDown.Value = optionsBE.get_xpages();
      heightUpDown.Value = optionsBE.get_ypages();
    }

    protected void canvasNeedsRepaint(int x, int y, int w, int h)
    {
      contentPanel.Invalidate(new System.Drawing.Rectangle(x,y,w,h));
    }

    private void contentPanel_Paint(object sender, PaintEventArgs e)
    {
      if (canvas != null)
      {
        IntPtr hdc = e.Graphics.GetHdc();
        canvas.repaint(hdc);
        e.Graphics.ReleaseHdc(hdc);
      }
      else
      {
        Brush brush = new HatchBrush(HatchStyle.ForwardDiagonal,
          Color.FromArgb(0xff, 0x4a, 0x61, 0x84), Color.FromArgb(0xff, 0x28, 0x37, 0x52));
        e.Graphics.FillRectangle(brush, ClientRectangle);
      }
    }

    private void contentPanel_SizeChanged(object sender, EventArgs e)
    {
      if (canvas != null)
        canvas.OnSizeChanged(contentPanel.Width, contentPanel.Height);
    }


    void contentPanel_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
    {
      if (canvas != null)
        canvas.OnMouseMove(e, ModifierKeys, MouseButtons);
    }

    void contentPanel_MouseUp(object sender, System.Windows.Forms.MouseEventArgs e)
    {
      if (canvas != null)
        canvas.OnMouseUp(e, ModifierKeys, MouseButtons);
    }

    void contentPanel_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
    {
      if (canvas != null)
        canvas.OnMouseDown(e, ModifierKeys, MouseButtons);
    }

    private void heightUpDown_ValueChanged(object sender, EventArgs e)
    {
      optionsBE.set_ypages((int)heightUpDown.Value);
    }

    private void widthUpDown_ValueChanged(object sender, EventArgs e)
    {
      optionsBE.set_xpages((int)widthUpDown.Value);
    }

    private void button2_Click(object sender, EventArgs e)
    {
      optionsBE.commit();
    }

    private void diagramNameEdit_TextChanged(object sender, EventArgs e)
    {
      optionsBE.set_name(diagramNameEdit.Text);
    }

  }
}
