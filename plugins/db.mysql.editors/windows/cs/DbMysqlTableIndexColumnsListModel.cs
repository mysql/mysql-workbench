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

using MySQL.Grt;
using MySQL.Grt.Db;
using MySQL.Utilities;

namespace MySQL.GUI.Workbench.Plugins
{
	class DbMysqlTableIndexColumnsListModel : GrtListModel
	{
    private NodeCheckBox enabledNodeControl;
		private AdvNodeComboBox nameNodeControl;
		private AdvNodeComboBox orderNodeControl;
    private AdvNodeComboBox storageNodeControl;
    private AdvNodeTextBox lengthNodeControl;

		private MySQLTableColumnsListWrapper tableColumnGrtList;

		/// <summary>
		/// Constructor that initializes the model with the given objects
		/// </summary>
		/// <param name="TreeView">The TreeViewAdv control this model belongs to</param>
		/// <param name="GrtTree">The GRT tree this model belongs to</param>
		public DbMysqlTableIndexColumnsListModel(TreeViewAdv ListView, IndexColumnsListWrapper GrtList,
      MySQLTableColumnsListWrapper TableColumnGrtList,
      NodeCheckBox EnabledNodeControl,
			AdvNodeComboBox NameNodeControl,
      AdvNodeComboBox OrderNodeControl,
      AdvNodeComboBox StorageNodeControl,
      AdvNodeTextBox LengthNodeControl)
      : base(ListView, GrtList, false)
		{
			tableColumnGrtList = TableColumnGrtList;

      enabledNodeControl = EnabledNodeControl;
			nameNodeControl = NameNodeControl;
      orderNodeControl = OrderNodeControl;
      storageNodeControl = StorageNodeControl;
      lengthNodeControl = LengthNodeControl;

			// assign virtual value events for displaying and processing the edited value content
      enabledNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      enabledNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      nameNodeControl.EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
			nameNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
			nameNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      orderNodeControl.EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
      orderNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      orderNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      storageNodeControl.EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
			storageNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
			storageNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      lengthNodeControl.EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
      lengthNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      lengthNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
		}

		public override void DetachEvents()
		{
			base.DetachEvents();

			// remove virtual value events
      enabledNodeControl.ValueNeeded -= ValueNeeded;
      enabledNodeControl.ValuePushed -= ValuePushed;

			nameNodeControl.EditorInitialize -= EditorInitialize;
			nameNodeControl.ValueNeeded -= ValueNeeded;
			nameNodeControl.ValuePushed -= ValuePushed;

      orderNodeControl.EditorInitialize -= EditorInitialize;
      orderNodeControl.ValueNeeded -= ValueNeeded;
      orderNodeControl.ValuePushed -= ValuePushed;

			storageNodeControl.ValueNeeded -= ValueNeeded;
			storageNodeControl.ValuePushed -= ValuePushed;

      lengthNodeControl.EditorInitialize -= EditorInitialize;
      lengthNodeControl.ValueNeeded -= ValueNeeded;
      lengthNodeControl.ValuePushed -= ValuePushed;
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
          if (sender == enabledNodeControl)
          {
            e.Value = ((IndexColumnsListWrapper)grtList).get_column_enabled(node.NodeId);
          }
          else if (sender == nameNodeControl)
					{
						string caption;

						grtList.get_field(node.NodeId, (int)IndexColumnsListWrapper.IndexColumnsListColumns.Name, out caption);

						e.Value = caption;
					}
          else if (sender == orderNodeControl)
          {
            string caption;

            grtList.get_field(node.NodeId, (int)IndexColumnsListWrapper.IndexColumnsListColumns.OrderIndex, out caption);

            e.Value = caption;
          }
          else if (sender == storageNodeControl)
          {
            int val;

            grtList.get_field(node.NodeId, (int)IndexColumnsListWrapper.IndexColumnsListColumns.Descending, out val);

            if (val == 1)
              e.Value = "DESC";
            else
              e.Value = "ASC";
          }
          else if (sender == lengthNodeControl)
          {
            int val;

            grtList.get_field(node.NodeId, (int)IndexColumnsListWrapper.IndexColumnsListColumns.Length, out val);

            if (val > 0)
              e.Value = Convert.ToString(val);
            else
              e.Value = "";
          }
				}
			}
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
				GrtListNode node = e.Node.Tag as GrtListNode;

				if (node != null)
				{
          if (sender == enabledNodeControl)
          {
            ((IndexColumnsListWrapper) grtList).set_column_enabled(node.NodeId, (CheckState) e.Value == CheckState.Checked);
          }
					else if (sender == nameNodeControl)
					{
						String value = e.Value as String;
						if (value != null)
							grtList.set_field(node.NodeId, (int)IndexColumnsListWrapper.IndexColumnsListColumns.Name, value);
					}
          else if (sender == orderNodeControl)
          {
            String value = e.Value as String;
            if (value != null)
            {
              //int intValue = Convert.ToInt32(value);
              int intValue = 0;
              try
              {
                intValue = Convert.ToInt32(value);
                grtList.set_field(node.NodeId, (int)IndexColumnsListWrapper.IndexColumnsListColumns.OrderIndex, intValue);
              } 
              catch(FormatException)
              {
              }
            }
          }
          if (sender == storageNodeControl)
          {
            String value = e.Value as String;

            if (value != null)
            {
              int intValue = 0;
              if (value.Equals("DESC"))
                intValue = 1;

              grtList.set_field(node.NodeId, (int)IndexColumnsListWrapper.IndexColumnsListColumns.Descending, intValue);
            }
          }
          else if (sender == lengthNodeControl)
          {
            String value = e.Value as String;
            if (value != null)
            {
              try
              {
                int intValue = Convert.ToInt32(value);
                grtList.set_field(node.NodeId, (int)IndexColumnsListWrapper.IndexColumnsListColumns.Length, intValue);
              }
              catch (Exception)
              {
                grtList.set_field(node.NodeId, (int)IndexColumnsListWrapper.IndexColumnsListColumns.Length, "");
              }
            }
          }
					RefreshModel();
				}
			}
		}

		private void EditorInitialize(object sender, EditorInitializeEventArgs e)
		{
			if (sender == nameNodeControl)
			{
				ComboBox comboBox = e.Editor as ComboBox;
				if (comboBox != null)
				{
					comboBox.Items.Clear();

					// Add table columns
					for (int i = 0; i < tableColumnGrtList.count(); i++)
					{
						string caption;

						tableColumnGrtList.get_field(tableColumnGrtList.get_node(i),
							(int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name, out caption);

						comboBox.Items.Add(caption);
					}
					
					comboBox.KeyDown += new KeyEventHandler(textBox_KeyDown);
				}
			}
			else if (sender == orderNodeControl)
			{
				ComboBox comboBox = e.Editor as ComboBox;
				if (comboBox != null)
				{
					comboBox.Items.Clear();
          int maxOrderIndex = ((IndexColumnsListWrapper)grtList).get_max_order_index();
          for (int i = 1; i <= maxOrderIndex; i++)
					  comboBox.Items.Add(i);
					
					//comboBox.DropDownStyle = ComboBoxStyle.DropDown;
					comboBox.KeyDown += new KeyEventHandler(textBox_KeyDown);
				}
			}
      else if (sender == storageNodeControl)
      {
        ComboBox comboBox = e.Editor as ComboBox;
        if (comboBox != null)
        {
          comboBox.Items.Clear();
          comboBox.Items.Add("ASC");
          comboBox.Items.Add("DESC");


          //comboBox.DropDownStyle = ComboBoxStyle.DropDown;
          comboBox.KeyDown += new KeyEventHandler(textBox_KeyDown);
        }
      }
			else if (sender == lengthNodeControl)
			{
				TextBox textBox = e.Editor as TextBox;
				if (textBox != null)
					textBox.KeyDown += new KeyEventHandler(textBox_KeyDown);
			}
		}

		void textBox_KeyDown(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Tab || e.KeyCode == Keys.Enter || e.KeyCode == Keys.Return)
			{
				Control c = sender as Control;
				if (c != null)
				{
          if (c.Tag == nameNodeControl)
					{
						// Remember current index
						int selIndex = 0;
						if (this.TreeView.SelectedNode != null)
							selIndex = this.TreeView.SelectedNode.Index;
						ComboBox t = c as ComboBox;
						string value = "";
						if (t != null)
							value = t.Text;

						lengthNodeControl.EndEdit(true);

						// Try to select previous index again
						if (selIndex < this.TreeView.Root.Children.Count && value.Length > 0)
						{
							this.TreeView.SelectedNode = this.TreeView.Root.Children[selIndex];

							// automatically go to the next row if this is the last "real" row
							if (this.TreeView.SelectedNode != null &&
								this.TreeView.SelectedNode.NextNode != null &&
								this.TreeView.SelectedNode.NextNode.NextNode == null)
							{
								this.TreeView.SelectedNode =
									this.TreeView.SelectedNode.NextNode;

								nameNodeControl.BeginEdit();
							}
						}

						e.Handled = true;
					}
				}
			}
		}

		#endregion
	}
}
