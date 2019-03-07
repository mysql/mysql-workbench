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

using MySQL.Grt;

namespace MySQL.GUI.Workbench.Plugins
{
  public partial class ImageEditor : ObjectEditorPlugin
  {
    #region Member Variables

    private MySQL.Grt.ImageEditorWrapper imageEditorBE { get { return Backend as MySQL.Grt.ImageEditorWrapper; } }
    private int originalWidth, originalHeight;

    #endregion

    #region Constructors

    public ImageEditor(GrtManager manager, GrtValue value)
      : base(manager)
    {
      InitializeComponent();
      ReinitWithArguments(value);
    }

    #endregion

    #region ObjectEditorPlugin Overrides

    public override bool ReinitWithArguments(GrtValue value)
    {
      InitializingControls = true;
      SuspendLayout();

      try
      {
        Backend = new MySQL.Grt.ImageEditorWrapper(value);
        RefreshFormData();
      }
      finally
      {
        ResumeLayout();
        InitializingControls = false;
      }

      Invalidate();

      return true;
    }

    #endregion

    #region Form implemenation

    protected override void RefreshFormData()
    {
      InitializingControls = true;
      nameTextBox.Text = imageEditorBE.get_filename();
      int w, h;
      imageEditorBE.get_size(out w, out h);
      widthText.Text = w.ToString();
      heightText.Text = h.ToString();

      keepAspectCheck.Checked = imageEditorBE.get_keep_aspect_ratio();

      String path= imageEditorBE.get_attached_image_path();

      Image image = Image.FromFile(path);
      originalWidth = image.Width;
      originalHeight = image.Height;
      pictureBox2.Image = image;

      InitializingControls = false;
    }

    private void button1_Click(object sender, EventArgs e)
    {
      openFileDialog1.Title = "Select PNG Image";
      openFileDialog1.Filter = "PNG Images (*.png)|*.png";
			openFileDialog1.FileName = "";
      openFileDialog1.RestoreDirectory = true;
      if (openFileDialog1.ShowDialog() == DialogResult.OK)
      {
        imageEditorBE.set_filename(openFileDialog1.FileName);
        RefreshFormData();
      }
    }

    private void resetSizeButton_Click(object sender, EventArgs e)
    {
      imageEditorBE.set_size(originalWidth, originalHeight);

      RefreshFormData();
    }

    private void widthText_KeyPress(object sender, KeyPressEventArgs e)
    {
      if (e.KeyChar == '\r')
      {
        if (!InitializingControls)
        {
          int w = Convert.ToInt32(widthText.Text);
          imageEditorBE.set_width(w);
          RefreshFormData();
        }
      }
    }

    private void heightText_KeyPress(object sender, KeyPressEventArgs e)
    {
      if (e.KeyChar == '\r')
      {
        if (!InitializingControls)
        {
          int h = Convert.ToInt32(heightText.Text);
          imageEditorBE.set_height(h);
          RefreshFormData();
        }
      }
    }

    private void keepAspectCheck_CheckedChanged(object sender, EventArgs e)
    {
      if (!InitializingControls)
      {
        imageEditorBE.set_keep_aspect_ratio(keepAspectCheck.Checked);
        RefreshFormData();
      }
    }

    private void widthText_Leave(object sender, EventArgs e)
    {
      if (!InitializingControls)
      {
        int w = Convert.ToInt32(widthText.Text);
        imageEditorBE.set_width(w);
        RefreshFormData();
      }
    }

    private void heightText_Leave(object sender, EventArgs e)
    {
      if (!InitializingControls)
      {
        int h = Convert.ToInt32(heightText.Text);
        imageEditorBE.set_height(h);
        RefreshFormData();
      }
    }

    #endregion
  }
}