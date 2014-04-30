/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

using Aga.Controls.Tree.NodeControls;

namespace MySQL.Utilities
{
  public class AdvNodeCheckBox : NodeCheckBox
  {
    public AdvNodeCheckBox() : base()
    {
    }

    public override void KeyDown(KeyEventArgs args)
    {
      if (args.KeyCode == Keys.Space && EditEnabled)
      {
        Parent.BeginUpdate();
        try
        {
          if (Parent.CurrentNode != null)
          {
            CheckState value = GetNewState(GetCheckState(Parent.CurrentNode));

            if (IsEditEnabled(Parent.CurrentNode))
              SetCheckState(Parent.CurrentNode, value);
            /*foreach (TreeNodeAdv node in Parent.SelectedNodes)
              if (IsEditEnabled(node))
                SetCheckState(node, value);*/
          }
        }
        finally
        {
          Parent.EndUpdate();
        }
        args.Handled = true;
      }
    }

    private CheckState GetNewState(CheckState state)
    {
      if (state == CheckState.Indeterminate)
        return CheckState.Unchecked;
      else if (state == CheckState.Unchecked)
        return CheckState.Checked;
      else
        return ThreeState ? CheckState.Indeterminate : CheckState.Unchecked;
    }
  }
}
