/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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
using System.ComponentModel;
using System.Windows.Forms;

using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

namespace MySQL.Utilities
{
	public class AdvNodeComboBox : NodeComboBox
	{
    private ComboBoxStyle dropDownStyle = ComboBoxStyle.DropDownList;

		public AdvNodeComboBox()
		{
		}

		protected override Control CreateEditor(TreeNodeAdv node)
		{
			AdvComboBox comboBox = new AdvComboBox();
			if (DropDownItems != null)
				comboBox.Items.AddRange(DropDownItems.ToArray());
			comboBox.SelectedItem = GetValue(node);
      comboBox.DropDownStyle = dropDownStyle;
			comboBox.DropDownClosed += new EventHandler(EditorDropDownClosed);
			SetEditControlProperties(comboBox, node);

			comboBox.Tag = this;
      OnEditorInitialize(new EditorInitializeEventArgs(node, comboBox));

			return comboBox;
		}

    protected override void DoApplyChanges(TreeNodeAdv node, Control editor)
    {
      if (editor != null)
        SetValue(node, (editor as ComboBox).Text);
    }

		void EditorDropDownClosed(object sender, EventArgs e)
		{
      if (sender is ComboBox)
      {
        ComboBox c = sender as ComboBox;
        if (c.DropDownStyle == ComboBoxStyle.DropDownList)
          EndEdit(true);
      }
      else
        EndEdit(true);
		}

    public override void UpdateEditor(Control control)
    {
      ComboBox c = control as ComboBox;
      if (c != null)
      {
        if (c.DropDownStyle == ComboBoxStyle.DropDownList)
          c.DroppedDown = true;
      }
    }

		public event EditorInitializeEventHandler EditorInitialize;
		protected void OnEditorInitialize(EditorInitializeEventArgs args)
		{
			if (EditorInitialize != null)
				EditorInitialize(this, args);
		}

    [Category("Behavior")]
    [EditorBrowsable(EditorBrowsableState.Always)]
    [Browsable(true)]
    [DesignerSerializationVisibility(DesignerSerializationVisibility.Visible)]
    [Bindable(true)]
    [Description("Specifies the dropdown style.")]
    public ComboBoxStyle DropDownStyle
    {
      get { return dropDownStyle; }
      set { dropDownStyle = value; }
    }
	}

	class AdvComboBox : ComboBox
	{
    // use timer as hack to place text selection after DropDownClosed
    protected Timer selectionTimer;

		public AdvComboBox()
		{
      // init timer
      selectionTimer = new Timer();
      selectionTimer.Tick += new EventHandler(selectionTimer_Tick);
      selectionTimer.Interval = 100;
		}

    void selectionTimer_Tick(object sender, EventArgs e)
    {
      selectionTimer.Enabled = false;

      // detect '(' and set selection
      int p = Text.IndexOf('(');
      if (p > 0)
      {
        SelectionStart = p + 1;
        SelectionLength = 0;
      }
    }

		protected override bool IsInputKey(Keys keyData)
		{
			if (keyData == Keys.Tab)
				return true;
			else
				return base.IsInputKey(keyData);
		}

    protected override void OnDropDownClosed(EventArgs e)
    {
      base.OnDropDownClosed(e);

      if (DropDownStyle != ComboBoxStyle.DropDownList)
      {
        // enable timer hack
        selectionTimer.Enabled = true;
      }
    }
	}
}
