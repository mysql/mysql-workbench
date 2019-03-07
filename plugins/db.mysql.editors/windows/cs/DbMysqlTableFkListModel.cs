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
using System.Drawing;
using System.Windows.Forms;

using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

using MySQL.Grt;
using MySQL.Grt.Db;
using MySQL.Utilities;

namespace MySQL.GUI.Workbench.Plugins
{
  class DbMysqlTableFkListModel : GrtListModel
  {
    private AdvNodeTextBox nameNodeControl;
    private AdvNodeComboBox targetNodeControl;
    private MySQLTableEditorWrapper mySQLTableEditorWrapper;
    private NodeCheckBox columnEnabledFkNodeControl;

		/// <summary>
		/// Constructor that initializes the model with the given objects
		/// </summary>
		/// <param name="TreeView">The TreeViewAdv control this model belongs to</param>
		/// <param name="GrtTree">The GRT tree this model belongs to</param>
		/// <param name="NodeStateIcon">The NodeStateIcon NodeControl that displays the icon</param>
    public DbMysqlTableFkListModel(TreeViewAdv tree, FKConstraintListWrapper grtList,
      AdvNodeTextBox nameNodeControl, AdvNodeComboBox targetNodeControl,
      MySQLTableEditorWrapper wrapper, NodeCheckBox columnEnabledFkNodeControl)
      : base(tree, grtList, true)
		{
      this.nameNodeControl = nameNodeControl;
      this.targetNodeControl = targetNodeControl;
      this.mySQLTableEditorWrapper = wrapper;

      this.columnEnabledFkNodeControl = columnEnabledFkNodeControl;

			// Assign virtual value events for displaying and processing the edited value content.
			nameNodeControl.EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
			nameNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
			nameNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      targetNodeControl.EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
      targetNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      targetNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
		}

    public override void DetachEvents()
    {
      base.DetachEvents();

      // remove virtual value events
      nameNodeControl.EditorInitialize -= EditorInitialize;
      nameNodeControl.ValueNeeded -= ValueNeeded;
      nameNodeControl.ValuePushed -= ValuePushed;

      targetNodeControl.EditorInitialize -= EditorInitialize;
      targetNodeControl.ValueNeeded -= ValueNeeded;
      targetNodeControl.ValuePushed -= ValuePushed;
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

            grtList.get_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.Name, out caption);

            e.Value = caption;
          }
          else if (sender == targetNodeControl)
          {
            string caption;

            grtList.get_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.RefTable, out caption);

            e.Value = caption;
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
          if (sender == nameNodeControl)
          {
            String value = e.Value as String;
            if (value != null)
              grtList.set_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.Name, e.Value as String);
          }
          else if (sender == targetNodeControl)
          {
            String value = e.Value as String;
            if (value != null)
              grtList.set_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.RefTable, e.Value as String);
          }

          RefreshModel();
        }
      }
    }

    private void EditorInitialize(object sender, EditorInitializeEventArgs e)
    {
      if (sender == nameNodeControl)
      {
        TextBox textBox = e.Editor as TextBox;
        if (textBox != null)
          textBox.KeyDown += new KeyEventHandler(textBox_KeyDown);
      }
      else if (sender == targetNodeControl)
      {
        ComboBox comboBox = e.Editor as ComboBox;
        if (comboBox != null)
        {
          if (e.Node != null && e.Node.Tag != null)
          {
            GrtListNode node = e.Node.Tag as GrtListNode;

            String value;
            grtList.get_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.RefTable, out value);

            comboBox.Items.Clear();
            using (Graphics graphics = comboBox.CreateGraphics())
            {
              float maxWidth = 200; // Minimum width
              List<string> tables = mySQLTableEditorWrapper.get_all_table_names();
              foreach (string table in tables)
              {
                comboBox.Items.Add(table);
                SizeF size = graphics.MeasureString(table, comboBox.Font);
                if (size.Width > maxWidth)
                  maxWidth = size.Width;

                // Make sure the previous selected item is selected again
                if (table.Equals(value))
                  comboBox.SelectedIndex = comboBox.Items.Count - 1;
              }

              // Limit width to a sane value.
              if (maxWidth > 700)
                maxWidth = 700;
              comboBox.DropDownWidth = (int)Math.Ceiling(maxWidth);
            }
          }

          comboBox.KeyDown += new KeyEventHandler(textBox_KeyDown);
          comboBox.DropDownClosed += new EventHandler(comboBox_DropDownClosed);
        }
      }
    }

    void comboBox_DropDownClosed(object sender, EventArgs e)
    {
      // Remember current index
      int selIndex = 0;
      if (TreeView.SelectedNode != null)
        selIndex = TreeView.SelectedNode.Index;

      Control c = sender as Control;
      if (c != null && c.Tag == targetNodeControl)
      {
        targetNodeControl.EndEdit(true);

        // Try to select previous index again
        if (selIndex < TreeView.Root.Children.Count)
        {
          TreeView.SelectedNode = TreeView.Root.Children[selIndex];

          columnEnabledFkNodeControl.Parent.Focus();
        }
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

              targetNodeControl.BeginEdit();
            }
            e.Handled = true;
          }
          else if (c.Tag == targetNodeControl)
          {
            // Remember current index
            int selIndex = 0;
            if (TreeView.SelectedNode != null)
              selIndex = TreeView.SelectedNode.Index;
            ComboBox t = c as ComboBox;
            string value = "";
            if (t != null)
              value = t.Text;

            targetNodeControl.EndEdit(true);

            // Try to select previous fk again
            if (selIndex < TreeView.Root.Children.Count && value.Length > 0)
            {
              TreeView.SelectedNode = TreeView.Root.Children[selIndex];
            }

            columnEnabledFkNodeControl.Parent.Focus();
            //columnEnabledFkNodeControl.Parent.SelectedNode = columnEnabledFkNodeControl.Parent.Root;

            e.Handled = true;
          }
        }
      }
    }

    #endregion

  }
}
