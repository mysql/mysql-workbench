/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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