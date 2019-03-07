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

using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

namespace MySQL.Grt
{
	/// <summary>
	/// A generic GRT tree model class implementing ITreeModel from the TreeViewAdv control
	/// </summary>
	public class GrtListModel : ITreeModel
	{
		/// <summary>
		/// The tree view control this model is used for
		/// </summary>
		protected TreeViewAdv treeView;

		/// <summary>
		/// The GRT tree this model is representing
		/// </summary>
		protected MySQL.Grt.ListModelWrapper grtList;

		/// <summary>
		/// The top node if any, otherwise null
		/// </summary>
		protected GrtTreeNode topNode = null;

		/// <summary>
		/// The NodeIcon Node Control that displays the icon
		/// </summary>
		protected NodeIcon nodeIcon = null;

		/// <summary>
		/// Hide the default constructor
		/// </summary>
		private GrtListModel()
		{
		}

		/// <summary>
		/// The constructor that has to be overwritten in the derived model classes
		/// </summary>
		/// <param name="TreeView">The TreeViewAdv control this model is used for</param>
		/// <param name="GrtList">The GRT list this model is representing</param>
    /// <param name="DynamicContextMenu">Use context menu definition provided by backend</param>
    protected GrtListModel(TreeViewAdv tree, MySQL.Grt.ListModelWrapper GrtList, bool DynamicContextMenu)
			: this()
		{
			grtList = GrtList;
			treeView = tree;

      if (DynamicContextMenu)
      {
        tree.ContextMenuStrip = new System.Windows.Forms.ContextMenuStrip();
        tree.ContextMenuStrip.Opening += new System.ComponentModel.CancelEventHandler(ContextMenuStrip_Opening);
      }
		}

		protected GrtListModel(TreeViewAdv ListView, MySQL.Grt.ListModelWrapper GrtList, NodeIcon NodeIcon,
                          bool DynamicContextMenu)
			: this(ListView, GrtList, DynamicContextMenu)
		{
			nodeIcon = NodeIcon;

			// Ensure that the VirtualMode is enabled
			nodeIcon.VirtualMode = true;

			// Assign virtual value events for displaying and processing the edited value content
			nodeIcon.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(IconNeeded);
		}

    void ContextMenuStrip_Opening(object sender, System.ComponentModel.CancelEventArgs e)
    {
      e.Cancel = false;

      System.Windows.Forms.ContextMenuStrip menu = sender as System.Windows.Forms.ContextMenuStrip;

      menu.SuspendLayout();

      // repopulate the context menu with stuff provided by the backend
      menu.Items.Clear();

      List<NodeIdWrapper> selection = new List<NodeIdWrapper>();

      foreach (TreeNodeAdv node in treeView.SelectedNodes)
      {
        GrtListNode listNode = node.Tag as GrtListNode;
        selection.Add(listNode.NodeId);
      }

      List<MySQL.Base.MenuItem> items = grtList.get_popup_items_for_nodes(selection);
      foreach (MySQL.Base.MenuItem item in items)
      {
        if (item.get_type() == MySQL.Base.MenuItemType.MenuSeparator)
        {
          menu.Items.Add(new System.Windows.Forms.ToolStripSeparator());
        }
        else
        {
          System.Windows.Forms.ToolStripMenuItem mitem = new System.Windows.Forms.ToolStripMenuItem();

          mitem.Name = item.getInternalName();
          mitem.Size = new System.Drawing.Size(203, 22);
          mitem.Text = item.get_caption();
          mitem.Enabled = item.get_enabled();
          mitem.Click += new EventHandler(contextMenuItem_Click);
          menu.Items.Add(mitem);
        }
      }

      menu.ResumeLayout();
    }

    void contextMenuItem_Click(object sender, EventArgs e)
    {
      System.Windows.Forms.ToolStripMenuItem mitem = sender as System.Windows.Forms.ToolStripMenuItem;

      List<NodeIdWrapper> selection = new List<NodeIdWrapper>();

      foreach (TreeNodeAdv node in treeView.SelectedNodes)
      {
        GrtListNode listNode = node.Tag as GrtListNode;
        if (listNode.IsValid)
          selection.Add(listNode.NodeId);
      }

      grtList.activate_popup_item_for_nodes(mitem.Name, selection);
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
		/// As this is a list, every node is a leaf
		/// </summary>
		/// <param name="treePath">The path of the node</param>
		/// <returns>False if the node can be expanded, true if the node is a leaf</returns>
		public virtual bool IsLeaf(TreePath treePath)
		{
			return true;
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
		public MySQL.Grt.ListModelWrapper GrtList
		{
			get { return grtList; }
		}

		public TreeViewAdv TreeView
		{
			get { return treeView; }
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
		/// Refreshes the tree
		/// </summary>
		public virtual void RefreshModel()
		{
			grtList.refresh();
			OnStructureChanged(new TreePathEventArgs());
		}

		/// <summary>
		/// Event handler that gets the icon for the node
		/// </summary>
		/// <param name="sender">The object triggering the event</param>
		/// <param name="e">The event parameter</param>
		private void IconNeeded(object sender, NodeControlValueEventArgs e)
		{
			if (e.Node != null && e.Node.Tag != null)
			{
				// Use the GRT Icon manager to get the correct icon
				GrtListNode node = e.Node.Tag as GrtListNode;

				if (node != null)
				{
					int iconId = GrtList.get_field_icon(node.NodeId, 0, IconSize.Icon16);
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
			// remove virtual value events
			if (nodeIcon != null)
				nodeIcon.ValueNeeded -= IconNeeded;
		}
	}

  /// <summary>
  /// Interface that can be used by the models to notify a registered consumer about values
  /// pushed, so they can update other elements not directly connected to this model.
  /// </summary>
  public interface IModelChangeListener
  {
    void ValueChanged(BindableControl control, Object value);
  }
}
