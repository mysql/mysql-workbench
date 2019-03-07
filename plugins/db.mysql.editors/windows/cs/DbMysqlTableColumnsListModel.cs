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
using System.Collections.Generic;
using System.Windows.Forms;

using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

using MySQL.Forms;
using MySQL.Utilities;

namespace MySQL.Grt.Db
{
	public class DbMysqlTableColumnsListModel : GrtListModel
	{
		private AdvNodeTextBox nameNodeControl;
    private AdvNodeComboBox datatypeComboBoxNodeControl;
		private NodeCheckBox pkNodeControl;
		private NodeCheckBox nnNodeControl;
    private NodeCheckBox uqNodeControl;
    private NodeCheckBox binNodeControl;
    private NodeCheckBox unNodeControl;
    private NodeCheckBox zfNodeControl;
    private NodeCheckBox aiNodeControl;
    private NodeCheckBox gNodeControl;
    private AdvNodeTextBox defaultNodeControl;
    private MySQLTableEditorWrapper mySQLTableEditorWrapper;

    private IModelChangeListener listener;

		/// <summary>
		/// Constructor that initializes the model with the given objects
		/// </summary>
		/// <param name="TreeView">The TreeViewAdv control this model belongs to</param>
		/// <param name="GrtTree">The GRT tree this model belongs to</param>
		/// <param name="NodeStateIcon">The NodeStateIcon NodeControl that displays the icon</param>
		public DbMysqlTableColumnsListModel(IModelChangeListener listener, TreeViewAdv tree,
      MySQLTableColumnsListWrapper grtList,
			NodeIcon columnIconNodeControl, AdvNodeTextBox nameNodeControl, AdvNodeComboBox datatypeComboBoxNodeControl,
      NodeCheckBox pkNodeControl, NodeCheckBox nnNodeControl, NodeCheckBox uqNodeControl, NodeCheckBox binNodeControl, 
      NodeCheckBox unNodeControl, NodeCheckBox zfNodeControl, NodeCheckBox aiNodeControl,
      NodeCheckBox gNodeControl, AdvNodeTextBox defaultNodeControl, MySQLTableEditorWrapper wrapper)
			: base(tree, grtList, columnIconNodeControl, true)
		{
      this.listener = listener;

      this.nameNodeControl = nameNodeControl;
      this.datatypeComboBoxNodeControl = datatypeComboBoxNodeControl;
      this.pkNodeControl = pkNodeControl;
      this.nnNodeControl = nnNodeControl;
      this.uqNodeControl = uqNodeControl;
      this.binNodeControl = binNodeControl;
      this.unNodeControl = unNodeControl;
      this.zfNodeControl = zfNodeControl;
      this.aiNodeControl = aiNodeControl;
      this.gNodeControl = gNodeControl;
      this.defaultNodeControl = defaultNodeControl;
      this.mySQLTableEditorWrapper = wrapper;

			// assign virtual value events for displaying and processing the edited value content
			nameNodeControl.EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
			nameNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
			nameNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      datatypeComboBoxNodeControl.EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
      datatypeComboBoxNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      datatypeComboBoxNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);

      pkNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      pkNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      nnNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
			nnNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      uqNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      uqNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      binNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      binNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      unNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      unNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      zfNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      zfNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      aiNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
			aiNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      gNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      gNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      
      defaultNodeControl.EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
			defaultNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
			defaultNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
		}

		public override void DetachEvents()
		{
			base.DetachEvents();

			// remove virtual value events
			nameNodeControl.EditorInitialize -= EditorInitialize;
			nameNodeControl.ValueNeeded -= ValueNeeded;
			nameNodeControl.ValuePushed -= ValuePushed;

      datatypeComboBoxNodeControl.EditorInitialize -= EditorInitialize;
      datatypeComboBoxNodeControl.ValueNeeded -= ValueNeeded;
      datatypeComboBoxNodeControl.ValuePushed -= ValuePushed;

      pkNodeControl.ValueNeeded -= ValueNeeded;
      pkNodeControl.ValuePushed -= ValuePushed;
      nnNodeControl.ValueNeeded -= ValueNeeded;
			nnNodeControl.ValuePushed -= ValuePushed;
      uqNodeControl.ValueNeeded -= ValueNeeded;
      uqNodeControl.ValuePushed -= ValuePushed;
      binNodeControl.ValueNeeded -= ValueNeeded;
      binNodeControl.ValuePushed -= ValuePushed;
      unNodeControl.ValueNeeded -= ValueNeeded;
      unNodeControl.ValuePushed -= ValuePushed;
      zfNodeControl.ValueNeeded -= ValueNeeded;
      zfNodeControl.ValuePushed -= ValuePushed;
      aiNodeControl.ValueNeeded -= ValueNeeded;
			aiNodeControl.ValuePushed -= ValuePushed;
      gNodeControl.ValueNeeded -= ValueNeeded;
      gNodeControl.ValuePushed -= ValuePushed;

      defaultNodeControl.EditorInitialize -= EditorInitialize;
			defaultNodeControl.ValueNeeded -= ValueNeeded;
			defaultNodeControl.ValuePushed -= ValuePushed;
		}

		/// <summary>
		/// Returns a node list of all child nodes of a given parent node
		/// </summary>
		/// <param name="treePath">The path of the parent node</param>
		/// <returns>The list of child nodes for the given parent path node</returns>
		public override System.Collections.IEnumerable GetChildren(TreePath treePath)
		{
			List<GrtListNode> items = new List<GrtListNode>();

			int count = grtList.count();
			for (int i = 0; i < count; i++)
			{
				NodeIdWrapper nodeId = grtList.get_node(i);
				GrtListNode node;
				string caption;

				grtList.get_field(nodeId, 0, out caption);

				node = new GrtListNode(caption, nodeId, null, this);

				items.Add(node);
			}
			return items;
		}

		#region event handlers
		/// <summary>
		/// Event handler that gets the caption for the value column
		/// </summary>
		/// <param name="sender">The object triggering the event</param>
		/// <param name="e">The event parameter</param>
		private void ValueNeeded(object sender, NodeControlValueEventArgs e)
		{
			if (e.Node != null && e.Node.Tag != null)
			{
				GrtListNode node = e.Node.Tag as GrtListNode;

				if (node != null)
				{
					if (sender == nameNodeControl)
					{
						string caption;

						if (grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name, out caption))
              e.Value = caption;
					}
          else if (sender == datatypeComboBoxNodeControl)
          {
            string caption;

            if (grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Type, out caption))
              e.Value = caption;
          }
					else if (sender == pkNodeControl)
					{
						int value;

						if (grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsPK, out value))
              e.Value = (value == 1);
					}
          else if (sender == nnNodeControl)
          {
            int notNull;

            if (grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsNotNull, out notNull))
              e.Value = (notNull == 1);
          }
          else if (sender == uqNodeControl)
          {
            int unique;

            if (grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsUnique, out unique))
              e.Value = (unique == 1);
          }
          else if (sender == binNodeControl)
          {
            int value;

            if (grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsBinary, out value))
              e.Value = (value == 1);
          }
          else if (sender == unNodeControl)
          {
            int value;

            if (grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsUnsigned, out value))
              e.Value = (value == 1);
          }
          else if (sender == zfNodeControl)
          {
            int value;

            if (grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsZerofill, out value))
              e.Value = (value == 1);
          }
          else if (sender == aiNodeControl)
					{
						int autoInc;

            // Show the actual value, even if the field would not allow auto increment.
            // While editing allow to reset the auto inc value but prevent setting it if the
            // column data type does not allow it.

            if (grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsAutoIncrement, out autoInc))
              e.Value = (autoInc == 1);
					}
          else if (sender == gNodeControl)
          {
            int generated;
            if (grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsGenerated, out generated))
              e.Value = (generated == 1);
          }
          else if (sender == defaultNodeControl)
					{
						string caption;

						if (grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Default, out caption))
              e.Value = caption;
					}
				}
			}
		}

    /// <summary>
    /// Converts the current placeholder (the last line in the columns grid) into a real column.
    /// </summary>
    void activateColumnPlaceholder(NodeIdWrapper node)
    {
      // The following code is a bit involved, but it makes the table grid
      // properly display the default PK column name and all its other settings.
      
      // Tell the backend we are editing now the placeholder row.
      grtList.set_field(node, (int) MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name, 1);
      
      // Get the default value for the name field...
      String value;
      grtList.get_field(node, (int) MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name, out value);
      
      // ... and set it in the backend. This way the backend will know next time
      // we set a value that we need a new place holder.
      grtList.set_field(node, (int) MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name, value);
    }


		/// <summary>
		/// Event handler that sets the new value for the value column
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void ValuePushed(object sender, NodeControlValueEventArgs e)
		{
			if (e.Node != null && e.Node.Tag != null)
			{
        int selIndex = e.Node.Index;

        GrtListNode node = e.Node.Tag as GrtListNode;

        // Activate column placeholder row if we a value for it must be set here.
        if (e.Node.Index == TreeView.Root.Children.Count - 1)
          activateColumnPlaceholder(node.NodeId);
				
				if (node != null)
				{
					if (sender == nameNodeControl)
					{
						String value = e.Value as String;
						if (value != null)
							grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name, e.Value as String);
					}
          else if (sender == datatypeComboBoxNodeControl)
          {
            String value = e.Value as String;
            if (value != null)
            {
              if (!grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Type, e.Value as String))
              {
                CustomMessageBox.Show(MessageType.MessageError, "Could not set new data type",
                  "The given data type\n\n" + value + "\n\ncontains errors and cannot be accepted. " + 
                  "The previous value is kept instead.",  "Close");
              }
            }
          }
					else if (sender == pkNodeControl)
					{
						int intValue = Convert.ToInt16(e.Value);
						grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsPK, intValue);
					}
          else if (sender == nnNodeControl)
          {
            int intValue = Convert.ToInt16(e.Value);
            grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsNotNull, intValue);
          }
          else if (sender == uqNodeControl)
          {
            int intValue = Convert.ToInt16(e.Value);
            grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsUnique, intValue);
          }
          else if (sender == binNodeControl)
          {
            int intValue = Convert.ToInt16(e.Value);
            grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsBinary, intValue);
          }
          else if (sender == unNodeControl)
          {
            int intValue = Convert.ToInt16(e.Value);
            grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsUnsigned, intValue);
          }
          else if (sender == zfNodeControl)
          {
            int intValue = Convert.ToInt16(e.Value);
            grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsZerofill, intValue);
          }
          else if (sender == aiNodeControl)
					{
						int intValue = Convert.ToInt16(e.Value);
						grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsAutoIncrement, intValue);
					}
          else if (sender == gNodeControl)
          {
            int intValue = Convert.ToInt16(e.Value);
            grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsGenerated, intValue);
          }
          else if (sender == defaultNodeControl)
					{
						String value = e.Value as String;
						if (value != null)
							grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Default, e.Value as String);
					}

					RefreshModel();

          if (selIndex > -1)
            TreeView.SelectedNode = TreeView.Root.Children[selIndex];

          if (listener != null)
            listener.ValueChanged(sender as BindableControl, e.Value);
				}
			}
		}

		private void EditorInitialize(object sender, EditorInitializeEventArgs e)
		{
			if (sender == nameNodeControl)
			{
				TextBox textBox = e.Editor as TextBox;
        if (textBox != null)
        {
          // Only do for the placeholder row.
          if (TreeView.CurrentNode.Index == grtList.count() - 1)
          {
            if (e.Node != null && e.Node.Tag != null)
            {
              GrtListNode node = e.Node.Tag as GrtListNode;

              String value;
              grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name, out value);

              if (value.Equals(""))
              {
                // Mark this row as placeholder. This activates special handling in the backend
                // (e.g. default value generation).
                grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name, 1);

                // Read the default value and initialize the editor control with it.
                grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name, out value);
                textBox.Text = value;
              }
            }
          }

          textBox.KeyDown += new KeyEventHandler(textBox_KeyDown);
          textBox.Leave += new EventHandler(textBox_Leave);
        }
			}
      else if (sender == datatypeComboBoxNodeControl)
      {
        ComboBox comboBox = e.Editor as ComboBox;
        if (comboBox != null)
        {
          if (e.Node != null && e.Node.Tag != null)
          {
            GrtListNode node = e.Node.Tag as GrtListNode;

            String value;
            grtList.get_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Type, out value);

            if (value.Equals(""))
            {
              grtList.set_field(node.NodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name, 1);
              comboBox.Text = value;
            }
            else
              comboBox.Text = value;
          }

          // BOOL/BOOLEAN as well as a few others types listed here 
          // http://dev.mysql.com/doc/refman/5.6/en/other-vendor-data-types.html
          // are not supported by server but rather converted to the 
          // corresponding native types. WB pre-loads these types as user-defined.

          comboBox.Items.Clear();
          comboBox.Items.AddRange(((MySQLTableColumnsListWrapper)grtList).get_datatype_names().ToArray());

          comboBox.DropDownHeight = 450;
          comboBox.KeyDown += new KeyEventHandler(textBox_KeyDown);
          comboBox.Leave += new EventHandler(comboBox_Leave);
        }
      }
      else if (sender == defaultNodeControl)
      {
        TextBox textBox = e.Editor as TextBox;
        if (textBox != null)
        {
          textBox.KeyDown += new KeyEventHandler(textBox_KeyDown);
          textBox.Leave += new EventHandler(textBox_Leave);
        }
      }
		}

    void comboBox_Leave(object sender, EventArgs e)
    {
      // Remember current index
      int selIndex = 0;
      if (TreeView.SelectedNode != null)
        selIndex = TreeView.SelectedNode.Index;

      ComboBox c = sender as ComboBox;
      if (c != null && c.Tag == datatypeComboBoxNodeControl)
      {
        datatypeComboBoxNodeControl.ApplyChanges();

        // Try to select previous column again
        if (selIndex < TreeView.Root.Children.Count)
        {
          TreeView.SelectedNode = TreeView.Root.Children[selIndex];
        }

        c.KeyDown -= textBox_KeyDown;
        c.Leave -= comboBox_Leave;
      }
    }

    void textBox_Leave(object sender, EventArgs e)
    {
      TextBox textBox = sender as TextBox;

      // notify cancel editing placeholder
      if (TreeView.SelectedNode != null)
      {
        if (textBox.Tag != defaultNodeControl)
        {
          grtList.set_field(new NodeIdWrapper(TreeView.SelectedNode.Index), 0, 0);
          RefreshModel();
        }

        if (textBox != null)
        {
          textBox.KeyDown -= textBox_KeyDown;
          textBox.Leave -= textBox_Leave;
        }
      }
    }

		void textBox_KeyDown(object sender, KeyEventArgs e)
		{
      if (e.KeyCode == Keys.Escape)
      {
        // notify cancel editing placeholder
        if (TreeView.SelectedNode != null)
          grtList.set_field(new NodeIdWrapper(TreeView.SelectedNode.Index), 0, 0);
        RefreshModel();
      }
			else if (e.KeyCode == Keys.Tab || e.KeyCode == Keys.Enter || e.KeyCode == Keys.Return)
			{
				Control c = sender as Control;
				if (c != null)
				{
					if (c.Tag == nameNodeControl)
					{
						// Remember current index
						int selIndex = 0;
						if (TreeView.SelectedNode != null)
							selIndex = TreeView.SelectedNode.Index;
						TextBox t = c as TextBox;
						string value = "";
						if (t != null)
							value = t.Text;

						nameNodeControl.EndEdit(true);

						// Try to select previous index again
            if (selIndex < TreeView.Root.Children.Count && value.Length > 0)
            {
              TreeView.SelectedNode = TreeView.Root.Children[selIndex];

              //datatypeNodeControl.BeginEdit();
              datatypeComboBoxNodeControl.BeginEdit();
            }
            else
            {
              if (TreeView.SelectedNode != null)
              {
                // notify cancel editing placeholder
                grtList.set_field(new NodeIdWrapper(selIndex), 0, 0);
                RefreshModel();
              }
            }
						e.Handled = true;
					}
          else if (c.Tag == datatypeComboBoxNodeControl)
          {
            // Remember current index
            int selIndex = 0;
            if (TreeView.SelectedNode != null)
              selIndex = TreeView.SelectedNode.Index;
            ComboBox t = c as ComboBox;
            string value = "";
            if (t != null)
              value = t.Text;

            datatypeComboBoxNodeControl.EndEdit(true);

            // Try to select previous index again
            if (selIndex < TreeView.Root.Children.Count && value.Length > 0)
            {
              TreeView.SelectedNode = TreeView.Root.Children[selIndex];

              // automatically go to the next row if this is the last "real" row
              if (TreeView.SelectedNode != null &&
                TreeView.SelectedNode.NextNode != null &&
                TreeView.SelectedNode.NextNode.NextNode == null)
              {
                TreeView.SelectedNode =
                  TreeView.SelectedNode.NextNode;

                nameNodeControl.BeginEdit();
              }
              else
              {
                if (TreeView.SelectedNode != null)
                {
                  // notify cancel editing placeholder
                  grtList.set_field(new NodeIdWrapper(selIndex), 0, 0);
                  RefreshModel();
                }
              }
            }
            else
            {
              if (TreeView.SelectedNode != null)
              {
                // notify cancel editing placeholder
                grtList.set_field(new NodeIdWrapper(TreeView.SelectedNode.Index), 0, 0);
                RefreshModel();
              }
            }

            e.Handled = true;
          }
          else if (c.Tag == defaultNodeControl)
          {
            defaultNodeControl.EndEdit(true);
            e.Handled = true;
          }
				}
			}
		}

		#endregion
	}
}
