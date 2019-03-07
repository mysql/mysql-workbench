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

using MySQL.Grt;
using MySQL.Grt.Db;

namespace MySQL.GUI.Workbench.Plugins
{
  public partial class DbMysqlRoleEditor : ObjectEditorPlugin
  {
    #region Member Variables

    private RoleEditorBE RoleEditorBackend { get { return Backend as RoleEditorBE; } } 

    private RoleTreeBE roleTreeBE;
    private SimpleGrtTreeModel roleTreeModel;

    private RoleObjectListWrapper roleObjectListWrapper;
    private RolePrivilegeListWrapper rolePrivilegeListWrapper;
    private SimpleGrtListModel roleObjectListModel;

    #endregion

    #region Constructors

    public DbMysqlRoleEditor(GrtManager manager, GrtValue value)
      : base(manager)
    {
      InitializeComponent();
      ReinitWithArguments(value);

      rolesTreeView.ItemDrag += new ItemDragEventHandler(rolesTreeView_ItemDrag);
      rolesTreeView.DragEnter += new DragEventHandler(rolesTreeView_DragEnter);
      rolesTreeView.DragOver += new DragEventHandler(rolesTreeView_DragOver);
      rolesTreeView.DragDrop += new DragEventHandler(rolesTreeView_DragDrop);
    }

    #endregion

    #region ObjectEditorPlugin Overrides

    public override bool ReinitWithArguments(GrtValue value)
    {
      InitializingControls = true;

      try
      {
        Backend = new RoleEditorBE(value);

        // (Re) Initialize backend.
        roleTreeBE = RoleEditorBackend.get_role_tree();
        roleTreeBE.refresh();

        roleObjectListWrapper = RoleEditorBackend.get_object_list();
        roleObjectListWrapper.refresh();
        rolePrivilegeListWrapper = RoleEditorBackend.get_privilege_list();
        rolePrivilegeListWrapper.refresh();

        if (roleTreeModel != null)
          roleTreeModel.DetachEvents();
        roleTreeModel = new SimpleGrtTreeModel(rolesTreeView, roleTreeBE, false);
        roleTreeModel.AddColumn(roleNameNodeControl, (int)RoleTreeBE.Columns.Name, false);
        rolesTreeView.Model = roleTreeModel;

        if (roleObjectListModel != null)
          roleObjectListModel.DetachEvents();
        roleObjectListModel = new SimpleGrtListModel(roleObjectsTreeView, roleObjectListWrapper, objectIconNodeControl, false);
        roleObjectListModel.AddColumn(objectNameNodeControl, (int)RoleObjectListWrapper.Columns.Name, false);
        roleObjectsTreeView.Model = roleObjectListModel;

        RefreshFormData();
      }
      finally
      {
        InitializingControls = false;
      }

      Invalidate();

      return true;
    }

    #endregion

    #region Form implementation

    void rolesTreeView_DragDrop(object sender, DragEventArgs e)
    {
      TreeNodeAdv[] nodes = (TreeNodeAdv[])e.Data.GetData(typeof(TreeNodeAdv[]));
      TreeNodeAdv dropNode = rolesTreeView.DropPosition.Node;
      if (dropNode == null) return;

      NodeIdWrapper dropNodeId = ((GrtTreeNode)dropNode.Tag).NodeId;

      if (rolesTreeView.DropPosition.Position == NodePosition.Inside)
      {
        foreach (TreeNodeAdv node in nodes)
        {
          if (node.Equals(dropNode))
            continue;

          NodeIdWrapper next_id = ((GrtTreeNode)node.Tag).NodeId;
          roleTreeBE.append_child(dropNodeId, next_id);
        }

        roleTreeBE.refresh();

        rolesTreeView.Model = null;
        rolesTreeView.Model = roleTreeModel;
        
        //rolesTreeView.DropPosition.Node.IsExpanded = true;
      }
      else
      {
        foreach (TreeNodeAdv node in nodes)
        {
          NodeIdWrapper next_id = ((GrtTreeNode)(node.Tag)).NodeId;
          roleTreeBE.move_to_top_level(next_id);
        }

        roleTreeBE.refresh();

        rolesTreeView.Model = null;
        rolesTreeView.Model = roleTreeModel;
      }
    }

    void rolesTreeView_DragOver(object sender, DragEventArgs e)
    {
      e.Effect = DragDropEffects.None;

      if ((rolesTreeView.DropPosition.Position == NodePosition.Inside) && (e.Data.GetDataPresent(typeof(TreeNodeAdv[])) == true))
      {
        e.Effect = DragDropEffects.Move;
      }
      else if (rolesTreeView.DropPosition.Node != null)
      {
        NodeIdWrapper nid = roleTreeBE.get_parent(((GrtTreeNode)rolesTreeView.DropPosition.Node.Tag).NodeId);
        bool is_root = !nid.is_valid();
        if (is_root)
        {
          bool dragging_non_root = false;
          TreeNodeAdv[] nodes = (TreeNodeAdv[])e.Data.GetData(typeof(TreeNodeAdv[]));
          if (nodes != null)
          {
            foreach (TreeNodeAdv node in nodes)
            {
              NodeIdWrapper next_id = ((GrtTreeNode)node.Tag).NodeId;
              dragging_non_root = roleTreeBE.get_parent(next_id).is_valid();
              if (dragging_non_root)
              {
                e.Effect = DragDropEffects.Move;
                break;
              }
            }
          }
        }
      }
    }

    void rolesTreeView_DragEnter(object sender, DragEventArgs e)
    {
      if ((rolesTreeView.DropPosition.Position == NodePosition.Inside) && (e.Data.GetDataPresent(typeof(TreeNodeAdv[])) == true))
        e.Effect = DragDropEffects.Move;
      else
        e.Effect = DragDropEffects.None;
    }

    void rolesTreeView_ItemDrag(object sender, ItemDragEventArgs e)
    {
      TreeNodeAdv[] nodes = new TreeNodeAdv[rolesTreeView.SelectedNodes.Count];
      rolesTreeView.SelectedNodes.CopyTo(nodes, 0);
      DoDragDrop(nodes, DragDropEffects.Move);
    }

    private void RefreshPrivilegesList()
    {
      privCheckedListBox.Items.Clear();
      
      int count = rolePrivilegeListWrapper.count();
      String caption;
      int enabled;

      for (int i= 0; i < count; i++)
      {
        rolePrivilegeListWrapper.get_field(new NodeIdWrapper(i), (int)RolePrivilegeListWrapper.Columns.Name, out caption);
        rolePrivilegeListWrapper.get_field(new NodeIdWrapper(i), (int)RolePrivilegeListWrapper.Columns.Enabled, out enabled);
        privCheckedListBox.Items.Add(caption, enabled != 0);
      }
    }

    private void roleObjectsTreeView_SelectionChanged(object sender, EventArgs e)
    {
      if (roleObjectsTreeView.SelectedNode != null)
      {
        NodeIdWrapper nodeId = new NodeIdWrapper(roleObjectsTreeView.SelectedNode.Index);
        roleObjectListWrapper.set_selected_node(nodeId);
        RefreshPrivilegesList();
      }
      else
      {
        roleObjectListWrapper.set_selected_node(new NodeIdWrapper());
        RefreshPrivilegesList();
      }
    }

    private void privCheckedListBox_ItemCheck(object sender, ItemCheckEventArgs e)
    {
      rolePrivilegeListWrapper.set_field(new NodeIdWrapper(e.Index), (int)RolePrivilegeListWrapper.Columns.Enabled,
        e.NewValue==CheckState.Checked ? 1: 0);
    }

    private void roleObjectsTreeView_DragEnter(object sender, DragEventArgs e)
    {
      if (e.Data.GetDataPresent(typeof(List<GrtValue>)) == true || e.Data.GetDataPresent(typeof(GrtValue))==true)
        e.Effect = DragDropEffects.Copy;
    }

    private void roleObjectsTreeView_DragDrop(object sender, DragEventArgs e)
    {
      if (e.Data.GetDataPresent(typeof(List<GrtValue>)) == true)
      {
        List<GrtValue> selGrtValues = (List<GrtValue>)e.Data.GetData(typeof(List<GrtValue>));
        foreach (GrtValue grtValue in selGrtValues)
        {
          RoleEditorBackend.add_object(grtValue);
        }
      }
      else if (e.Data.GetDataPresent(typeof(GrtValue))==true)
      {
        GrtValue selGrtValue = (GrtValue)e.Data.GetData(typeof(GrtValue));
        RoleEditorBackend.add_object(selGrtValue);
      }
      roleObjectListWrapper.refresh();
      roleObjectListModel.RefreshModel();
    }

    protected override void RefreshFormData() 
    {
      TabText = RoleEditorBackend.get_title();
      nameTextBox.Text = RoleEditorBackend.get_name();
      ParentComboBox.Items.Clear();
      ParentComboBox.Items.AddRange(RoleEditorBackend.get_role_list().ToArray());
      String parentRole = RoleEditorBackend.get_parent_role();
      ParentComboBox.SelectedIndex = ParentComboBox.FindString(parentRole);

      roleTreeModel.RefreshModel();
      RefreshPrivilegesList();
    }

    #endregion


    private void ParentComboBox_SelectedIndexChanged(object sender, EventArgs e)
    {
      RoleEditorBackend.set_parent_role(ParentComboBox.Text);
    }

    private void nameTextBox_TextChanged(object sender, EventArgs e)
    {
      RoleEditorBackend.set_name(nameTextBox.Text);
    }

    private void checkAllButton_Click(object sender, EventArgs e)
    {
      for (int i = 0; i < privCheckedListBox.Items.Count; i++)
        privCheckedListBox.SetItemChecked(i, true);
    }

    private void uncheckAllButton_Click(object sender, EventArgs e)
    {
      for (int i = 0; i < privCheckedListBox.Items.Count; i++)
        privCheckedListBox.SetItemChecked(i, false);
    }

    private void objectsContextMenu_Opening(object sender, System.ComponentModel.CancelEventArgs e)
    {
      e.Cancel = false;

      List<NodeIdWrapper> nodes = new List<NodeIdWrapper>();
      foreach (TreeNodeAdv node in roleObjectsTreeView.SelectedNodes)
        nodes.Add(new NodeIdWrapper(node.Index));

      List<MySQL.Base.MenuItem> items = roleObjectListWrapper.get_popup_items_for_nodes(nodes);
      MySQL.Utilities.MenuManager manager = new MySQL.Utilities.MenuManager();
      ToolStripItem[] menuItems = manager.buildMenu(items, new EventHandler(objectsContextMenu_Click));
      objectsContextMenu.Items.Clear();
      objectsContextMenu.Items.AddRange(menuItems);
    }

    private void objectsContextMenu_Click(object sender, EventArgs e)
    {
      List<NodeIdWrapper> nodes = new List<NodeIdWrapper>();
      foreach (TreeNodeAdv node in roleObjectsTreeView.SelectedNodes)
        nodes.Add(new NodeIdWrapper(node.Index));

      roleObjectListWrapper.activate_popup_item_for_nodes((sender as ToolStripItem).Name, nodes);
      roleObjectListModel.RefreshModel();
    }
  }
}


