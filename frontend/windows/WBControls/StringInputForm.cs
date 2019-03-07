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

using System.Windows.Forms;

namespace MySQL.Utilities
{
  public partial class StringInputForm : Form
  {
    protected StringInputForm()
    {
      InitializeComponent();
    }

    public static DialogResult ShowModal(string title, string description, string prompt,
      ref string InputString)
    {
      using (StringInputForm stringInputForm = new StringInputForm())
      {
        stringInputForm.Text = title;
        if (description.Length > 0)
          stringInputForm.descLabel.Text = description;
        stringInputForm.promptLabel.Text = prompt;

        stringInputForm.inputTextBox.Text = InputString;

        DialogResult res = stringInputForm.ShowDialog();

        InputString = stringInputForm.inputTextBox.Text;
        return res;
      }
    }
  }
}