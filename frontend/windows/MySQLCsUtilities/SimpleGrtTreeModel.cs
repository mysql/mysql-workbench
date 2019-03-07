/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

using MySQL.Utilities;

namespace MySQL.Grt
{
  /// <summary>
  /// A generic GRT tree model class implementing the general case for a GrtTreeModel
  /// </summary>
  public class SimpleGrtTreeModel : GrtTreeModel, IToolTipProvider
  {
    protected class ColumnContent
    {
      public bool editable;
      public NodeControl control;
      public int index;
    };

    protected List<ColumnContent> columns = new List<ColumnContent>();
    protected int rowColorSource = -1;
    protected List<BaseTextControl> nodesWithEnabledRowColor = new List<BaseTextControl>();

    /// <summary>
    /// The constructor that has to be overwritten in the derived model classes
    /// </summary>
    /// <param name="TreeView">The TreeViewAdv control this model is used for</param>
    /// <param name="GrtList">The GRT list this model is representing</param>
    public SimpleGrtTreeModel(TreeViewAdv TreeView, MySQL.Grt.TreeModelWrapper GrtTree, bool DynamicContextMenu)
      : base(TreeView, GrtTree, DynamicContextMenu)
    {
      model = GrtTree;
      treeControl = TreeView;
    }


    public SimpleGrtTreeModel(TreeViewAdv TreeView, MySQL.Grt.TreeModelWrapper GrtTree, NodeStateIcon nodeIcon, bool DynamicContextMenu)
      : base(TreeView, GrtTree, nodeIcon, DynamicContextMenu)
    {
      model = GrtTree;
      treeControl = TreeView;
    }


    public void EnableRowColor(int grtIndex)
    {
      rowColorSource = grtIndex;

      foreach (ColumnContent column in columns)
      {
        BaseTextControl textNode = column.control as BaseTextControl;
        if (textNode != null)
        {
          textNode.DrawText += new EventHandler<DrawEventArgs>(DrawText_cb);

          // remember node for DetachEvents
          nodesWithEnabledRowColor.Add(textNode);
        }
      }
    }

    public int AddColumn(NodeControl treeControl, int grtIndex, bool editable)
    {
      ColumnContent column= new ColumnContent();
      column.editable = editable;
      column.index = grtIndex;
      column.control = treeControl;

      if (treeControl is BindableControl)
      {
        BindableControl control = treeControl as BindableControl;
        control.VirtualMode = true;
        control.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded);
        if (editable)
        {
          if (control is NodeTextBox)
            (control as NodeTextBox).EditEnabled = true;
          control.ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed);
          if (control is AdvNodeTextBox)
            ((AdvNodeTextBox) control).EditorInitialize += new EditorInitializeEventHandler(EditorInitialize);
        }
        else
          if (control is NodeTextBox)
            (control as NodeTextBox).EditEnabled = false;
      }

      columns.Add(column);

      return column.index;
    }

    public int AddColumn(NodeIcon treeControl, int grtIndex)
    {
      ColumnContent column = new ColumnContent();
      column.editable = false;
      column.index = grtIndex;
      column.control = treeControl;

      treeControl.VirtualMode = true;
      treeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(StateIconNeeded);
      
      columns.Add(column);

      return column.index;
    }

    public override void DetachEvents()
    {
      base.DetachEvents();

      foreach (ColumnContent column in columns)
      {
        if (column.control is NodeIcon)
          ((NodeIcon)column.control).ValueNeeded -= StateIconNeeded;
        else
        {
          NodeTextBox node = column.control as NodeTextBox;
          if (node != null)
          {
            node.ValueNeeded -= ValueNeeded;
            if (column.editable)
            {
              // remove virtual value events
              node.ValuePushed -= ValuePushed;

              if (node is AdvNodeTextBox)
                ((AdvNodeTextBox)node).EditorInitialize -= EditorInitialize;
            }
          }
        }
      }

      foreach(BaseTextControl textNode in nodesWithEnabledRowColor)
      {
        textNode.DrawText -= DrawText_cb;
      }
      nodesWithEnabledRowColor.Clear();
    }

    /// <summary>
    /// Returns a node list of all child nodes of a given parent node
    /// </summary>
    /// <param name="treePath">The path of the parent node</param>
    /// <returns>The list of child nodes for the given parent path node</returns>
    public override System.Collections.IEnumerable GetChildren(TreePath treePath)
    {
      NodeIdWrapper parentNodeId;
      bool settingTopNode = false;

      if (treePath.IsEmpty())
      {
        settingTopNode = true;
        parentNodeId = model.get_root();
      }
      else
      {
        GrtTreeNode parent = treePath.LastNode as GrtTreeNode;
        if (parent != null)
          parentNodeId = parent.NodeId;
        else
          parentNodeId = null;
      }

      if (parentNodeId != null)
      {
        // The backend has child nodes on a node not before it was expanded.
        int childCount = model.count_children(parentNodeId);
        if (childCount == 0)
        {
          // But expand only if we don't have any children yet. Otherwise
          // get many unnecessary refresh calls.
          model.expand_node(parentNodeId);
          childCount = model.count_children(parentNodeId);
        }

        for (int i = 0; i < childCount; i++)
        {
          NodeIdWrapper nodeId = model.get_child(parentNodeId, i);
          GrtTreeNode node;
          string caption;

          model.get_field(nodeId, columns[0].index, out caption);

          node = new GrtTreeNode(caption, nodeId, null, this);
          if (settingTopNode)
            topNode = node;

          //items.Add(node);
          yield return node;
        }
      }

      //return items;
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

            model.get_field(node.NodeId, column.index, out caption);

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
      treeControl.DefaultToolTipProvider = this;
    }

    /// <summary>
    /// Returns the grt index of the column to which the control is bound.
    /// </summary>
    /// <param name="control">The control for which to return the column index.</param>
    /// <returns>The index of the column for the given control if found or 0 (to emulate the way GrtTreeModel does this
    /// for icons).</returns>
    public int GetColumnIndex(NodeControl control)
    {
      foreach (ColumnContent column in columns)
        if (control == column.control)
        {
          return column.index;
        }
      return 0;
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
        GrtTreeNode node = e.Node.Tag as GrtTreeNode;

        if (node != null)
        {
          int index = GetColumnIndex(sender as NodeControl);
          if (index > -1)
          {
            string caption;

            model.get_field(node.NodeId, index, out caption);
            e.Value = caption;
          }
        }
      }
    }


    private void ValuePushed(object sender, NodeControlValueEventArgs e)
    {
      if (e.Node != null && e.Node.Tag != null)
      {
        GrtTreeNode node = e.Node.Tag as GrtTreeNode;

        if (node != null)
        {
          int index = GetColumnIndex(sender as NodeControl);
          if (index > -1)
          {
            NodeTextBox tnode = sender as NodeTextBox;
            model.set_convert_field(node.NodeId, index, e.Value as String);
          }
          treeControl.Refresh();
        }
      }
    }

    /// <summary>
    /// Event handler that gets the icon for the node in a given column.
    /// </summary>
    /// <param name="sender">The object triggering the event</param>
    /// <param name="e">The event parameter</param>
    protected override void StateIconNeeded(object sender, NodeControlValueEventArgs e)
    {
      if (e.Node != null && e.Node.Tag != null)
      {
        // Use the GRT Icon manager to get the correct icon
        GrtTreeNode node = e.Node.Tag as GrtTreeNode;

        if (node != null)
        {
          int index = GetColumnIndex(sender as NodeControl);
          if (index > -1)
          {
            int iconId = GrtTree.get_field_icon(node.NodeId, index, IconSize.Icon16);
            Image icon = IconManagerWrapper.get_instance().get_icon(iconId);

            if (icon != null)
              e.Value = icon;
          }
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
          textBox.KeyDown += new KeyEventHandler(textBox_KeyDown);
        }
      }
    }

    private void DrawText_cb(object sender, DrawEventArgs e)
    {
      /*if (rowColorSource >= 0 && e.BackgroundBrush == null)
      {
        string color;
        GrtTreeNode gnode = e.Node.Tag as GrtTreeNode;

        if (gnode != null)
        {
          grtTree.get_field(gnode.NodeId, rowColorSource, out color);

          if (color != "")
            e.BackgroundBrush = new SolidBrush(System.Drawing.ColorTranslator.FromHtml(color));
        }
      }*/
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
            if (treeControl.SelectedNode != null)
              selIndex = treeControl.SelectedNode.Index;

            c.EndEdit(true);

            e.Handled = true;
          }
        }
      }
    }
     
    #endregion


    #region IToolTipProvider Members

    public string GetToolTip(TreeNodeAdv node, NodeControl nodeControl)
    {
      if (node != null && node.Tag != null)
      {
        GrtTreeNode lnode = node.Tag as GrtTreeNode;
        int index = GetColumnIndex(nodeControl);
        if (index < 0)
          index= 0;

        return model.get_field_description(lnode.NodeId, index);
      }
      return "";
    }

    #endregion
  }

  /// <summary>
  /// This model acts like the SimpleGrtTreeModel, which it inherits from
  /// except it doenst show nodes from the 'black list'. The black list is 
  /// passed as DisabledNames parameter to the constrcutor or can be set 
  /// via SetDisabledList()
  /// </summary>
  public class DifferenceByNameGrtTreeModel : SimpleGrtTreeModel
  {
    private List<String> disabledNames;
 
    /// <summary>
    /// The constructor that has to be overwritten in the derived model classes
    /// </summary>
    /// <param name="TreeView">The TreeViewAdv control this model is used for</param>
    /// <param name="DisabledNames">the list of the nodes that shouldn't be shown</param>
    /// <param name="GrtList">The GRT list this model is representing</param>
    public DifferenceByNameGrtTreeModel(TreeViewAdv TreeView, List<String> DisabledNames,
      MySQL.Grt.TreeModelWrapper GrtTree, bool DynamicContextMenu)
      : base(TreeView, GrtTree, DynamicContextMenu)
    {
      model = GrtTree;
      treeControl = TreeView;
      disabledNames = DisabledNames;
    }

    public void SetDisabledList(List<String> DisabledNames)
    {
      disabledNames = DisabledNames;
    }

    /// <summary>
    /// Returns a node list of all child nodes of a given parent node
    /// </summary>
    /// <param name="treePath">The path of the parent node</param>
    /// <returns>The list of child nodes for the given parent path node</returns>
    public override System.Collections.IEnumerable GetChildren(TreePath treePath)
    {
      List<GrtTreeNode> items = null;
      NodeIdWrapper parentNodeId;
      bool settingTopNode = false;

      if (treePath.IsEmpty())
      {
        settingTopNode = true;
        parentNodeId = model.get_root();
      }
      else
      {
        GrtTreeNode parent = treePath.LastNode as GrtTreeNode;
        if (parent != null)
          parentNodeId = parent.NodeId;
        else
          parentNodeId = null;
      }

      if (parentNodeId != null)
      {
        int childCount = model.count_children(parentNodeId);

        items = new List<GrtTreeNode>();

        for (int i = 0; i < childCount; i++)
        {
          NodeIdWrapper nodeId = model.get_child(parentNodeId, i);
          GrtTreeNode node;
          string caption;

          model.get_field(nodeId, columns[0].index, out caption);

          if (disabledNames.Contains(caption))
            continue;

          node = new GrtTreeNode(caption, nodeId, null, this);
          if (settingTopNode)
            topNode = node;

          items.Add(node);
        }
      }
      return items;
    }
  }
}
