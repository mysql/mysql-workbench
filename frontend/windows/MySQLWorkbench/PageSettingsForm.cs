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
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Text.RegularExpressions;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  public partial class PageSettingsForm : Form
  {
    WbContext WbContext;
    double TopMargin, LeftMargin, BottomMargin, RightMargin;

    public PageSettingsForm()
    {
      InitializeComponent();
    }

    public PageSettingsForm(WbContext wbcontext)
    {
      InitializeComponent();

      WbContext = wbcontext;
    }


    private void okButton_Click(object sender, EventArgs e)
    {
      PageSettings settings= new PageSettings();

      settings.paper_type = WbContext.get_paper_sizes()[paperSizeCombo.SelectedIndex].name;
      settings.margin_top = double.Parse(topMarginText.Text);
      settings.margin_bottom= double.Parse(bottomMarginText.Text);
      settings.margin_left= double.Parse(leftMarginText.Text);
      settings.margin_right= double.Parse(rightMarginText.Text);
      if (landscapeRadio.Checked)
        settings.orientation = "landscape";
      else
        settings.orientation = "portrait";

      WbContext.set_page_settings(settings);

      Close();
    }

    private void cancelButton_Click(object sender, EventArgs e)
    {
      Close();
    }

    private void PageSettingsForm_Shown(object sender, EventArgs e)
    {
      PageSettings settings = WbContext.get_page_settings();

      paperSizeCombo.Items.Clear();

      List<PaperSize> paperSizes = WbContext.get_paper_sizes();
      foreach (PaperSize paper in paperSizes)
      {
        int i= paperSizeCombo.Items.Add(paper.caption);
        if (settings.paper_type == paper.name)
          paperSizeCombo.SelectedIndex= i;
      }
      //paperSizeCombo.Items.Add("Add Custom Size...");

      TopMargin = settings.margin_top;
      BottomMargin = settings.margin_bottom;
      LeftMargin = settings.margin_left;
      RightMargin = settings.margin_right;
      topMarginText.Text = settings.margin_top.ToString();
      bottomMarginText.Text = settings.margin_bottom.ToString();
      leftMarginText.Text = settings.margin_left.ToString();
      rightMarginText.Text = settings.margin_right.ToString();
      if (settings.orientation == "landscape")
        landscapeRadio.Checked = true;
      else
        portraitRadio.Checked = true;

      if (paperSizeCombo.SelectedIndex >= 0)
        paperSizeLabel.Text = paperSizes[paperSizeCombo.SelectedIndex].description;
    }

    private void paperSizeCombo_SelectedIndexChanged(object sender, EventArgs e)
    {
      PaperSize size = WbContext.get_paper_sizes()[paperSizeCombo.SelectedIndex];

      paperSizeLabel.Text= size.description;
      if (size.margins_set)
      {
        topMarginText.Text = size.margin_top.ToString();
        bottomMarginText.Text = size.margin_bottom.ToString();
        leftMarginText.Text = size.margin_left.ToString();
        rightMarginText.Text = size.margin_right.ToString();
      }
      else
      {
        topMarginText.Text = TopMargin.ToString();
        bottomMarginText.Text = BottomMargin.ToString();
        leftMarginText.Text = LeftMargin.ToString();
        rightMarginText.Text = RightMargin.ToString();
      }
    }


    private void marginOptionTextBox_Validating(object sender, CancelEventArgs e)
    {
      Regex validator = new Regex(@"^[0-9,.]{1,5}$");

      e.Cancel = !validator.IsMatch(((Control)sender).Text);
    }

  }
}