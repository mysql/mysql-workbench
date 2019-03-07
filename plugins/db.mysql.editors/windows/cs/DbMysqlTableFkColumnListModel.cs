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
  class DbMysqlTableFkColumnListModel : GrtListModel
  {
    private NodeCheckBox columnEnabledFkNodeControl;
    private AdvNodeTextBox nameNodeControl;
    private AdvNodeComboBox targetColumnNodeControl;
    private MySQLTableEditorWrapper mySQLTableEditorWrapper;

		/// <summary>
		/// Constructor that initializes the model with the given objects
		/// </summary>
		/// <param name="TreeView">The TreeViewAdv control this model belongs to</param>
		/// <param name="GrtTree">The GRT tree this model belongs to</param>
		/// <param name="NodeStateIcon">The NodeStateIcon NodeControl that displays the icon</param>
    public DbMysqlTableFkColumnListModel(TreeViewAdv tree, FKConstraintColumnsListWrapper grtList,
      NodeCheckBox columnEnabledFkNodeControl, AdvNodeTextBox nameNodeControl, 
      AdvNodeComboBox targetNodeControl, MySQLTableEditorWrapper wrapper)
      : base(tree, grtList, false)
		{
      this.columnEnabledFkNodeControl = columnEnabledFkNodeControl;
      this.nameNodeControl = nameNodeControl;
      this.targetColumnNodeControl = targetNodeControl;
      this.mySQLTableEditorWrapper = wrapper;

      // Assign virtual value events for displaying and processing the edited value content.
      columnEnabledFkNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      columnEnabledFkNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
			nameNodeControl.EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
			nameNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
			nameNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
      targetColumnNodeControl.EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
      targetColumnNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      targetColumnNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
		}

    public override void DetachEvents()
    {
      base.DetachEvents();

      // Remove virtual value events.
      columnEnabledFkNodeControl.ValueNeeded -= ValueNeeded;
      columnEnabledFkNodeControl.ValuePushed -= ValuePushed;

      nameNodeControl.EditorInitialize -= EditorInitialize;
      nameNodeControl.ValueNeeded -= ValueNeeded;
      nameNodeControl.ValuePushed -= ValuePushed;

      targetColumnNodeControl.EditorInitialize -= EditorInitialize;
      targetColumnNodeControl.ValueNeeded -= ValueNeeded;
      targetColumnNodeControl.ValuePushed -= ValuePushed;
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

        node = new GrtListNode(null, nodeId, null, this);

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
          if (sender == columnEnabledFkNodeControl)
          {
            e.Value = ((FKConstraintColumnsListWrapper)grtList).get_column_is_fk(node.NodeId);
          }
          else if (sender == nameNodeControl)
          {
            string caption;

            grtList.get_field(node.NodeId, (int)FKConstraintColumnsListWrapper.FKConstraintColumnsListColumns.Column, out caption);

            e.Value = caption;
          }
          else if (sender == targetColumnNodeControl)
          {
            string caption;

            grtList.get_field(node.NodeId, (int)FKConstraintColumnsListWrapper.FKConstraintColumnsListColumns.RefColumn, out caption);

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
          if (sender == columnEnabledFkNodeControl)
          {
            bool isFK = (CheckState)e.Value == CheckState.Checked;
            if (((FKConstraintColumnsListWrapper)grtList).set_column_is_fk(node.NodeId, isFK) && isFK)
            {
              // Sart editing the column only if it is enabled as FK column.
              targetColumnNodeControl.Parent.SelectedNode = e.Node;
              targetColumnNodeControl.BeginEdit();
            }
          }
          else if (sender == nameNodeControl)
          {
            String value = e.Value as String;
            if (value != null)
              grtList.set_field(node.NodeId, (int)FKConstraintColumnsListWrapper.FKConstraintColumnsListColumns.Column, e.Value as String);
          }
          else if (sender == targetColumnNodeControl)
          {
            String value = e.Value as String;
            if (value != null)
            {
              grtList.set_field(node.NodeId, (int)FKConstraintColumnsListWrapper.FKConstraintColumnsListColumns.RefColumn, e.Value as String);

              // Setting a target column implicitly enables this columns a FK column.
              ((FKConstraintColumnsListWrapper)grtList).set_column_is_fk(node.NodeId, true);
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
        TextBox textBox = e.Editor as TextBox;
        if (textBox != null)
        {
          if (TreeView.CurrentNode.Index == 0)
            textBox.Text = "id" + mySQLTableEditorWrapper.get_name();

          textBox.KeyDown += new KeyEventHandler(textBox_KeyDown);
        }
      }
      else if (sender == targetColumnNodeControl)
      {
        ComboBox comboBox = e.Editor as ComboBox;
        if (comboBox != null)
          comboBox.KeyDown += new KeyEventHandler(textBox_KeyDown);

        GrtListNode node = treeView.CurrentNode.Tag as GrtListNode;

        List<string> columns = ((FKConstraintColumnsListWrapper)GrtList).get_ref_columns_list(node.NodeId, false);

        string selected;
        grtList.get_field(node.NodeId, (int)FKConstraintColumnsListWrapper.FKConstraintColumnsListColumns.RefColumn, out selected);

        comboBox.Items.Clear();
        comboBox.Items.AddRange(columns.ToArray());

        int i= 0;
        foreach (String col in columns)
        {
          if (col == selected)
          {
            comboBox.SelectedIndex = i;
          }
          i++;
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

              targetColumnNodeControl.BeginEdit();
            }
            e.Handled = true;
          }
          else if (c.Tag == targetColumnNodeControl)
          {
            // Remember current index
            int selIndex = 0;
            if (TreeView.SelectedNode != null)
              selIndex = TreeView.SelectedNode.Index;
            TextBox t = c as TextBox;
            string value = "";
            if (t != null)
              value = t.Text;

            targetColumnNodeControl.EndEdit(true);

            e.Handled = true;
          }
        }
      }
    }

    #endregion

  }
}
