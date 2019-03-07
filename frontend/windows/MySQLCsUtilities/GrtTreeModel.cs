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

namespace MySQL.Grt
{
	/// <summary>
	/// A generic GRT tree model class implementing ITreeModel from the TreeViewAdv control
	/// </summary>
	public class GrtTreeModel : ITreeModel
	{
		/// <summary>
		/// The tree view control this model is used for
		/// </summary>
		protected TreeViewAdv treeControl;

		/// <summary>
		/// The GRT tree this model is representing
		/// </summary>
		protected MySQL.Grt.TreeModelWrapper model;

		/// <summary>
		/// The top node if any, otherwise null
		/// </summary>
		protected GrtTreeNode topNode = null;

		/// <summary>
		/// The NodeStateIcon Node Control that displays the icon
		/// </summary>
		protected NodeStateIcon nodeStateIcon = null;

		/// <summary>
		/// Hide the default constructor
		/// </summary>
		private GrtTreeModel()
		{
		}

		/// <summary>
		/// The constructor that has to be overwritten in the derived model classes
		/// </summary>
		/// <param name="TreeView">The TreeViewAdv control this model is used for</param>
		/// <param name="GrtTree">The GRT tree this model is representing</param>
    /// <param name="DynamicContextMenu">Use context menu definition provided by backend</param>
		protected GrtTreeModel(TreeViewAdv TreeView, MySQL.Grt.TreeModelWrapper GrtTree, bool DynamicContextMenu)
			: this()
		{
			model = GrtTree;
			treeControl = TreeView;

			treeControl.Expanding += new EventHandler<TreeViewAdvEventArgs>(TreeViewExpanding);
			treeControl.Collapsing += new EventHandler<TreeViewAdvEventArgs>(TreeViewCollapsing);

      if (DynamicContextMenu)
      {
        treeControl.ContextMenuStrip = new System.Windows.Forms.ContextMenuStrip();
        treeControl.ContextMenuStrip.Opening += new System.ComponentModel.CancelEventHandler(ContextMenuStrip_Opening);
      }
		}

		/// <summary>
		/// The constructor that has to be overwritten in the derived model classes
		/// </summary>
		/// <param name="TreeView">The TreeViewAdv control this model is used for</param>
		/// <param name="GrtTree">The GRT tree this model is representing</param>
		/// <param name="StateIcon">The NodeStateIcon Node Control that displays the icon</param>
    /// <param name="DynamicContextMenu">Use context menu definition provided by backend</param>
		protected GrtTreeModel(TreeViewAdv TreeView, MySQL.Grt.TreeModelWrapper GrtTree, Aga.Controls.Tree.NodeControls.NodeStateIcon NodeStateIcon, bool DynamicContextMenu)
			: this(TreeView, GrtTree, DynamicContextMenu)
		{
			nodeStateIcon = NodeStateIcon;

      if (nodeStateIcon != null)
      {
        // Ensure that the VirtualMode is enabled
        nodeStateIcon.VirtualMode = true;

        // Assign virtual value events for displaying and processing the edited value content
        nodeStateIcon.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(StateIconNeeded);
      }
		}

		/// <summary>
		/// Virtual function that needs to be overwritten in derived model classes. 
		/// Has to return a list of child nodes for the given path
		/// </summary>
		/// <param name="treePath">The path of the parent node</param>
		/// <returns>The list of child nodes for the given parent path node</returns>
		public virtual System.Collections.IEnumerable GetChildren(TreePath treePath)
		{
			return null;
		}

		/// <summary>
		/// Virtual function that needs to be overwritten in derived model classes. 
		/// Used to specify whether the given node can be expanded
		/// </summary>
		/// <param name="treePath">The path of the node</param>
		/// <returns>False if the node can be expanded, true if the node is a leaf</returns>
		public virtual bool IsLeaf(TreePath treePath)
		{
			GrtTreeNode node = treePath.LastNode as GrtTreeNode;

			if (node != null && model.is_expandable(node.NodeId))
				return false;
			else
				return true;
		}

		/// <summary>
		/// Event handler catching the collapsing event and updating the GRT tree model
		/// </summary>
		/// <param name="sender">Object that triggered the event</param>
		/// <param name="e">The event parameters</param>
		private void TreeViewCollapsing(object sender, TreeViewAdvEventArgs e)
		{
			if (e.Node != null && e.Node.Tag != null)
			{
				GrtTreeNode node = e.Node.Tag as GrtTreeNode;

				if (node != null)
					model.collapse_node(node.NodeId);
			}
		}

    private void reexpandChildren(TreeNodeAdv node)
    {
      foreach (TreeNodeAdv child in node.Children)
      {
        GrtTreeNode grtnode = child.Tag as GrtTreeNode;

        if (child.IsExpanded)
        {
          model.expand_node(grtnode.NodeId);

          reexpandChildren(child);
        }
      }
    }


    void ContextMenuStrip_Opening(object sender, System.ComponentModel.CancelEventArgs e)
    {
      e.Cancel = false;

      System.Windows.Forms.ContextMenuStrip menu = sender as System.Windows.Forms.ContextMenuStrip;

      menu.SuspendLayout();

      // repopulate the context menu with stuff provided by the backend
      menu.Items.Clear();

      List<NodeIdWrapper> selection = new List<NodeIdWrapper>();

      foreach (TreeNodeAdv node in treeControl.SelectedNodes)
      {
        GrtTreeNode treeNode = node.Tag as GrtTreeNode;
        selection.Add(treeNode.NodeId);
      }

      List<MySQL.Base.MenuItem> itemsBE = model.get_popup_items_for_nodes(selection);
      FillMenuItems(itemsBE, menu.Items);

      menu.ResumeLayout();
    }

    void FillMenuItems(List<MySQL.Base.MenuItem> itemsBE, ToolStripItemCollection itemsFE)
    {
      foreach (MySQL.Base.MenuItem itemBE in itemsBE)
      {
        switch (itemBE.get_type())
        {
          case MySQL.Base.MenuItemType.MenuSeparator:
            {
              itemsFE.Add(new ToolStripSeparator());
            }
            break;
          default:
            {
              ToolStripMenuItem itemFE = new ToolStripMenuItem();
              itemFE.Name = itemBE.getInternalName();
              itemFE.Text = itemBE.get_caption();
              itemFE.Enabled = itemBE.get_enabled();
              if (MySQL.Base.MenuItemType.MenuCascade == itemBE.get_type())
              {
                FillMenuItems(itemBE.get_subitems(), itemFE.DropDownItems);
              }
              else
              {
                itemFE.Click += new EventHandler(contextMenuItem_Click);
              }
              itemsFE.Add(itemFE);
            }
            break;
        }
      }
    }

    void contextMenuItem_Click(object sender, EventArgs e)
    {
      System.Windows.Forms.ToolStripMenuItem mitem = sender as System.Windows.Forms.ToolStripMenuItem;

      List<NodeIdWrapper> selection = new List<NodeIdWrapper>();

      foreach (TreeNodeAdv node in treeControl.SelectedNodes)
      {
        GrtTreeNode listNode = node.Tag as GrtTreeNode;
        selection.Add(listNode.NodeId);
      }

      model.activate_popup_item_for_nodes(mitem.Name, selection);
    }

		/// <summary>
		/// Event handler catching the expanding event and updating the GRT tree model
		/// </summary>
		/// <param name="sender">Object that triggered the event</param>
		/// <param name="e">The event parameters</param>
		private void TreeViewExpanding(object sender, TreeViewAdvEventArgs e)
		{
			if (e.Node != null)
			{
				GrtTreeNode node = e.Node.Tag as GrtTreeNode;

        if (node != null)
          model.expand_node(node.NodeId);

        // This expands the nodes in the UI which where expanded before.
        reexpandChildren(e.Node);
			}
		}

		#region Events
		public event EventHandler<TreeModelEventArgs> NodesChanged;
		internal void OnNodesChanged(TreeModelEventArgs args)
		{
			if (NodesChanged != null)
				NodesChanged(this, args);
		}

		public event EventHandler<TreePathEventArgs> StructureChanged;
		public void OnStructureChanged(TreePathEventArgs args)
		{
			if (StructureChanged != null)
				StructureChanged(this, args);
		}

		public event EventHandler<TreeModelEventArgs> NodesInserted;
		internal void OnNodeInserted(Node parent, int index, Node node)
		{
			if (NodesInserted != null)
			{
				TreeModelEventArgs args = new TreeModelEventArgs(GetPath(parent), new int[] { index }, new object[] { node });
				NodesInserted(this, args);
			}
		}

		public event EventHandler<TreeModelEventArgs> NodesRemoved;
		internal void OnNodeRemoved(Node parent, int index, Node node)
		{
			if (NodesRemoved != null)
			{
				TreeModelEventArgs args = new TreeModelEventArgs(GetPath(parent), new int[] { index }, new object[] { node });
				NodesRemoved(this, args);
			}
		}
		#endregion

		#region Properties
		public MySQL.Grt.TreeModelWrapper GrtTree
		{
			get { return model; }
		}
		#endregion

		/// <summary>
		/// Returns the path of the given node
		/// </summary>
		/// <param name="node">Node of interest</param>
		/// <returns>The path to the given node</returns>
		public virtual TreePath GetPath(Node node)
		{
			if (node == topNode)
				return TreePath.Empty;
			else
			{
				Stack<object> stack = new Stack<object>();
				while (node != topNode)
				{
					stack.Push(node);
					node = node.Parent;
				}
				return new TreePath(stack.ToArray());
			}
		}

		/// <summary>
		/// Refreshes the whole tree
		/// </summary>
		public virtual void RefreshModel()
		{
			model.refresh();
			OnStructureChanged(new TreePathEventArgs());
		}

    /// <summary>
    /// Refreshes a single node.
    /// </summary>
    public virtual void RefreshModel(TreePath path)
    {
      model.refresh();
      OnStructureChanged(new TreePathEventArgs(path));
    }

    /// <summary>
		/// Event handler that gets the icon for the node
		/// </summary>
		/// <param name="sender">The object triggering the event</param>
		/// <param name="e">The event parameter</param>
		protected virtual void StateIconNeeded(object sender, NodeControlValueEventArgs e)
		{
			if (e.Node != null && e.Node.Tag != null)
			{
				// Use the GRT Icon manager to get the correct icon
				GrtTreeNode node = e.Node.Tag as GrtTreeNode;

				if (node != null)
				{
					int iconId = GrtTree.get_field_icon(node.NodeId, 0, IconSize.Icon16);
					Image icon = IconManagerWrapper.get_instance().get_icon(iconId);

					if (icon != null)
						e.Value = icon;
				}
			}
		}

		/// <summary>
		/// Detaches the events handlers from the tree
		/// </summary>
		public virtual void DetachEvents()
		{
      if (nodeStateIcon != null)
        nodeStateIcon.ValueNeeded -= StateIconNeeded;
      treeControl.Expanding -= TreeViewExpanding;
      treeControl.Collapsing -= TreeViewCollapsing;
		}
	}
}
