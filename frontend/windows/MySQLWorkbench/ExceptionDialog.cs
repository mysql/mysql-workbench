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
using System.Windows.Forms;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  public partial class ExceptionDialog : Form
  {
    private static ExceptionDialog singleton = new ExceptionDialog();

    private String errorInfo;

    private WbContext wbContext;

    protected ExceptionDialog()
    {
      InitializeComponent();
    }

    public static void Show(String message, String info, WbContext wbcontext)
    {
      singleton.messageLabel.Text = message;
      singleton.errorInfo = info;
      singleton.wbContext = wbcontext;
      singleton.ShowDialog();
    }

    private void reportBugButton_Click(object sender, EventArgs e)
    {
      if (wbContext.is_commercial())
        System.Diagnostics.Process.Start("http://support.oracle.com");
      else
        //wbContext.report_bug(errorInfo);
        System.Diagnostics.Process.Start("http://bugs.mysql.com");
    }

    private void copyInfoButton_Click(object sender, EventArgs e)
    {
      Clipboard.SetText(errorInfo);
    }

    private void copyStackTraceToClipboardToolStripMenuItem_Click(object sender, EventArgs e)
    {
      Clipboard.SetText(errorInfo);
    }

  }
}
