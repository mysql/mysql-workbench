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
using System.Drawing;
using System.Windows.Forms;

namespace MySQL.GUI.Workbench
{
  public partial class ModelNavigatorForm : Form
  {
    ModelDiagramForm form;

    MySQL.GUI.Mdc.WindowsGDICanvasView canvas;
    bool initialized = false;

    public ModelNavigatorForm()
    {
      InitializeComponent();
    }


    public ModelNavigatorForm(ModelDiagramForm form)
    {
      InitializeComponent();
      this.form = form;

      DoubleBuffered = true;

      canvas = new MySQL.GUI.Mdc.WindowsGDICanvasView(miniViewHost.Handle,
        miniViewHost.Width, miniViewHost.Height);
      canvas.set_on_queue_repaint(canvasNeedsRepaint);
    }

    private void Destroy()
    {
      canvas.Dispose();
    }

    public void UpdateColors()
    {
      controlPanel.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelToolbar, false);
    }

    public void UpdateDisplay()
    {
      zoomComboBox.Enabled = true;
      zoomComboBox.Text = System.Math.Round(form.Zoom * 100).ToString() + "%";
      Refresh();
    }

    protected void canvasNeedsRepaint(int x, int y, int w, int h)
    {
      miniViewHost.Invalidate(new System.Drawing.Rectangle(x, y, w, h));

      // Note: don't do an immediate update here or the main canvas will get very slow
      //       with its own updates.
    }

    private void navImagePanel_Paint(object sender, PaintEventArgs e)
    {
      if (!initialized)
      {
        form.DiagramWrapper.setup_mini_view(canvas);
        Size size = miniViewHost.DisplayRectangle.Size;
        form.DiagramWrapper.update_mini_view_size(size.Width, size.Height);
        initialized = true;
      }

      IntPtr hdc = e.Graphics.GetHdc();
      canvas.repaint(hdc);
      e.Graphics.ReleaseHdc(hdc);
    }

    private void navImagePanel_MouseDown(object sender, MouseEventArgs e)
    {
      canvas.OnMouseDown(e, ModifierKeys, MouseButtons);
    }

    private void navImagePanel_MouseMove(object sender, MouseEventArgs e)
    {
      if ((MouseButtons & MouseButtons.Left) == MouseButtons.Left)
      {
        canvas.OnMouseMove(e, ModifierKeys, MouseButtons);
      }
    }

    private void navImagePanel_MouseUp(object sender, MouseEventArgs e)
    {
      canvas.OnMouseUp(e, ModifierKeys, MouseButtons);
    }

    private void navImagePanel_SizeChanged(object sender, EventArgs e)
    {
      if (canvas != null && !form.ShuttingDown)
      {
        Size size = miniViewHost.DisplayRectangle.Size;
        form.DiagramWrapper.update_mini_view_size(size.Width, size.Height);
        miniViewHost.Invalidate();
      }
    }

    private void ModelNavigatorForm_Shown(object sender, EventArgs e)
    {
      Size size = miniViewHost.DisplayRectangle.Size;
      form.DiagramWrapper.update_mini_view_size(size.Width, size.Height);
    }

    private void zoomInPictureBox_Click(object sender, EventArgs e)
    {
      form.Zoom = form.Zoom + 0.25;
    }

    private void zoomOutPictureBox_Click(object sender, EventArgs e)
    {
      form.Zoom = form.Zoom - 0.25;
    }

    private void zoomComboBox_KeyDown(object sender, KeyEventArgs e)
    {
      if (e.KeyCode == Keys.Enter)
        UpdateZoom();
    }

    private void zoomComboBox_SelectedIndexChanged(object sender, EventArgs e)
    {
      UpdateZoom();
    }

    private void UpdateZoom()
    {
      String zoomstr = zoomComboBox.Text;
      int percentPosition = zoomstr.IndexOf("%");
      if (percentPosition >= 0)
        zoomstr = zoomstr.Remove(percentPosition);

      double zoom;
      double zoom_max;
      double zoom_min;

      if (zoomstr == "")
      {
        zoomComboBox.Text = System.Math.Round(form.Zoom * 100).ToString() + "%";
        return;
      }

      try
      {
        zoom = double.Parse(zoomstr);

        // Exclude percent character.
        zoomstr = zoomComboBox.Items[0].ToString();
        zoomstr = zoomstr.Remove(zoomstr.Length - 1);
        zoom_max = double.Parse(zoomstr);

        zoomstr = zoomComboBox.Items[zoomComboBox.Items.Count - 1].ToString();
        zoomstr = zoomstr.Remove(zoomstr.Length - 1);
        zoom_min = double.Parse(zoomstr);
      }
      catch
      {
        zoomComboBox.Text = System.Math.Round(form.Zoom * 100).ToString() + "%";
        return;
      }

      // Check boundaries .
      if (zoom > zoom_max)
        zoom = zoom_max;
      if (zoom < zoom_min)
        zoom = zoom_min;

      form.Zoom = zoom / 100.0;
      zoomComboBox.Text = System.Math.Round(form.Zoom * 100).ToString() + "%";
    }
  }
}