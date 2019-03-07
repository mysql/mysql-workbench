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

using System;
using System.Collections.Generic;
using System.Windows.Forms;

using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

using MySQL.Utilities;

namespace MySQL.Grt
{
  /// <summary>
  /// A generic GRT tree model class implementing the general case for a GrtListModel
  /// </summary>
  public class SimpleGrtListModel : GrtListModel, IToolTipProvider
  {
    protected class ColumnContent
    {
      public bool editable;
      public NodeControl control;
      public int index;
    };

    protected List<ColumnContent> columns = new List<ColumnContent>();

    /// <summary>
    /// The constructor that has to be overwritten in the derived model classes
    /// </summary>
    /// <param name="TreeView">The TreeViewAdv control this model is used for</param>
    /// <param name="GrtList">The GRT list this model is representing</param>
    public SimpleGrtListModel(TreeViewAdv tree, MySQL.Grt.ListModelWrapper grtList, bool dynamicContextMenu)
      : base(tree, grtList, dynamicContextMenu)
    {
      this.grtList = grtList;
      treeView = tree;
    }

    public SimpleGrtListModel(TreeViewAdv tree, MySQL.Grt.ListModelWrapper grtList, NodeIcon nodeIcon, bool dynamicContextMenu)
      : base(tree, grtList, nodeIcon, dynamicContextMenu)
    {
      this.grtList = grtList;
      treeView = tree;
    }


    public void AddColumn(NodeTextBox treeControl, int grtIndex, bool editable)
    {
      ColumnContent column = new ColumnContent();
      column.editable = editable;
      column.index = grtIndex;
      column.control = treeControl;

      treeControl.VirtualMode = true;
      treeControl.DataPropertyName = "Text";
      treeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
      if (editable)
      {
        treeControl.EditEnabled = true;
        treeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
        if (treeControl is AdvNodeTextBox)
          ((AdvNodeTextBox)treeControl).EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);

      }
      else
        treeControl.EditEnabled = false;

      columns.Add(column);
    }


    public override void DetachEvents()
    {
      base.DetachEvents();

      foreach (ColumnContent column in columns)
      {
        NodeTextBox node = column.control as NodeTextBox;
        node.ValueNeeded -= ValueNeeded;
        if (column.editable)
        {
          if (node != null)
          {
            node.ValuePushed -= ValuePushed;

            if (node is AdvNodeTextBox)
              ((AdvNodeTextBox)node).EditorInitialize -= EditorInitialize;
          }
        }
      }
      columns = null;
    }

    /// <summary>
    /// Returns a node list of all child nodes of a given parent node
    /// </summary>
    /// <param name="treePath">The path of the parent node</param>
    /// <returns>The list of child nodes for the given parent path node</returns>
    public override System.Collections.IEnumerable GetChildren(TreePath treePath)
    {
      //List<GrtListNode> items = new List<GrtListNode>();

      // Get count but add the additional row for new columns
      int count = grtList.count();
      for (int i = 0; i < count; i++)
      {
        NodeIdWrapper nodeId = grtList.get_node(i);
        GrtListNode node;
        string caption;

        grtList.get_field(nodeId, columns[0].index, out caption);

        node = new GrtListNode(caption, nodeId, null, this);

        yield return node;
      }
    }

    public String GetRowAsText(TreeNodeAdv treeNode)
    {
      string rowText = "";

      if (treeNode != null && treeNode.Tag != null)
      {
        GrtListNode node = treeNode.Tag as GrtListNode;

        if (node != null)
        {
          foreach (ColumnContent column in columns)
          {
            string caption;

            grtList.get_field(node.NodeId, column.index, out caption);

            if (rowText.Equals(""))
              rowText = caption;
            else
              rowText += " | " + caption;
          }
        }
      }

      return rowText;
    }

    public void EnableTooltips()
    {
      treeView.DefaultToolTipProvider = this;
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
          foreach (ColumnContent column in columns)
          {
            if (sender == column.control)
            {
              string caption;

              grtList.get_field(node.NodeId, column.index, out caption);

              e.Value = caption;
              break;
            }
          }
        }
      }
    }


    private void ValuePushed(object sender, NodeControlValueEventArgs e)
    {
      if (e.Node != null && e.Node.Tag != null)
      {
        GrtListNode node = e.Node.Tag as GrtListNode;

        if (node != null)
        {
          foreach (ColumnContent column in columns)
          {
            if (sender == column.control)
            {
              NodeTextBox tnode = column.control as NodeTextBox;

              grtList.set_convert_field(node.NodeId, column.index, e.Value as String);
              break;
            }
          }

          // save selcted node
          int selected= -1;
          if (node != null && treeView.SelectedNode!=null)
            selected = treeView.SelectedNode.Index;

          RefreshModel();

          if (selected >= 0)
            treeView.SelectedNode = treeView.Root.Children[selected];
        }
      }
    }


    private void EditorInitialize(object sender, EditorInitializeEventArgs e)
    {
      if (sender is AdvNodeTextBox)
      {
        TextBox textBox = e.Editor as TextBox;
        if (textBox != null)
        {
          textBox.KeyDown += new KeyEventHandler(textBox_KeyDown);//LLL
        }
      }
    }


    void textBox_KeyDown(object sender, KeyEventArgs e)
    {
      if (e.KeyCode == Keys.Tab || e.KeyCode == Keys.Enter || e.KeyCode == Keys.Return)
      {
        TextBox textBox = sender as TextBox;
        if (textBox != null)
        {
          AdvNodeTextBox c = textBox.Tag as AdvNodeTextBox;
          if (c != null)
          {
            // Remember current index
            int selIndex = 0;
            if (treeView.SelectedNode != null)
              selIndex = treeView.SelectedNode.Index;

            c.EndEdit(true);

            e.Handled = true;
          }
        }
      }
    }
    #endregion
  
    #region IToolTipProvider Members

    public string  GetToolTip(TreeNodeAdv node, NodeControl nodeControl)
    {
      if (node != null && node.Tag != null)
      {
        GrtListNode lnode = node.Tag as GrtListNode;
        int c= 0;

        foreach (ColumnContent column in columns)
        {
          if (column.control == nodeControl)
          {
            c= column.index;
            break;
          }
        }

        return grtList.get_field_description(lnode.NodeId, c);
      }
      return "";
    }

    #endregion

}
}
