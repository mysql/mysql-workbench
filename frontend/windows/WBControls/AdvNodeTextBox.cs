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
using System.Drawing;
using System.Windows.Forms;

using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

namespace MySQL.Utilities
{
	public class AdvNodeTextBox : NodeTextBox
	{
		private const int MinTextBoxWidth = 30;

		private TextBox EditorTextBox
		{
			get
			{
				return CurrentEditor as TextBox;
			}
		}

		public AdvNodeTextBox()
		{
		}

		protected override Size CalculateEditorSize(EditorContext context)
		{
			if (Parent.UseColumns)
				return context.Bounds.Size;
			else
			{
				Size size = GetLabelSize(context.CurrentNode, context.DrawContext, _label);
				int width = Math.Max(size.Width + Font.Height, MinTextBoxWidth); // reserve a place for new typed character
				return new Size(width, size.Height);
			}
		}

		protected override Control CreateEditor(TreeNodeAdv node)
		{
			AdvTextBox textBox = new AdvTextBox();
			textBox.TextAlign = TextAlign;
			textBox.Text = GetLabel(node);
			textBox.BorderStyle = BorderStyle.FixedSingle;

			SetEditControlProperties(textBox, node);

			textBox.Tag = this;
      OnEditorInitialize(new EditorInitializeEventArgs(node, textBox));

      textBox.TextChanged += new EventHandler(textBox_TextChanged);
      _label = textBox.Text;

			return textBox;
		}

		private string _label;
		private void textBox_TextChanged(object sender, EventArgs e)
		{
			_label = EditorTextBox.Text;
			Parent.UpdateEditorBounds();
		}

		protected override void DoApplyChanges(TreeNodeAdv node, Control editor)
		{
			string oldLabel = GetLabel(node);
			if (oldLabel != _label)
			{
				SetLabel(node, _label);
				OnLabelChanged();
			}
		}

		protected override void EditorKeyDown(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Escape)
				EndEdit(false);
		}

		public event EditorInitializeEventHandler EditorInitialize;
		protected void OnEditorInitialize(EditorInitializeEventArgs args)
		{
			if (EditorInitialize != null)
				EditorInitialize(this, args);
		}
	}

	class AdvTextBox : TextBox
	{
		public AdvTextBox()
		{
		}

		protected override bool IsInputKey(Keys keyData)
		{
			if (keyData == Keys.Tab) 
				return true;
			else
				return base.IsInputKey(keyData);
		}
	}

  public delegate void EditorInitializeEventHandler(object sender, EditorInitializeEventArgs e);

  public class EditorInitializeEventArgs : EventArgs
  {
    private Object _editor;
    private TreeNodeAdv _node;

    public EditorInitializeEventArgs()
    {
      _editor = null;
    }

    public EditorInitializeEventArgs(Object Editor)
    {
      _editor = Editor;
    }

    public EditorInitializeEventArgs(TreeNodeAdv Node, Object Editor)
      : this(Editor)
    {
      _node = Node;
    }

    public Object Editor
    {
      get { return _editor; }
    }

    public TreeNodeAdv Node
    {
      get { return _node; }
    }
  }
}
