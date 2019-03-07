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
using System.Windows.Forms;

using MySQL.Grt;
using MySQL.Grt.Db;
using MySQL.Controls;

namespace MySQL.GUI.Workbench.Plugins
{
	public partial class DbObjectEditorPages : TabDocument
	{
    private GrtManager grtManager;
    private DBObjectEditorWrapper dbObjectEditorBE;

    private ObjectRoleListWrapper objectRoleListBE;
    private SimpleGrtListModel objectRoleListModel;

    private ObjectPrivilegeListBE objectPrivsListBE;

    private RoleTreeBE roleTreeBE;
    private SimpleGrtTreeModel roleTreeModel;
		protected DbObjectEditorPages()
		{
			InitializeComponent();
		}

		public DbObjectEditorPages(GrtManager GrtManager, DBObjectEditorWrapper DbObjectEditorBE)
			: this()
		{
			grtManager = GrtManager;
      dbObjectEditorBE = DbObjectEditorBE;

      // initialize roles based on passed object
      roleTreeBE = new RoleTreeBE(DbObjectEditorBE.get_catalog());

      roleTreeModel = new SimpleGrtTreeModel(allRolesTreeView, roleTreeBE, false);
      roleTreeModel.AddColumn(allRolesNameNodeControl, (int)RoleTreeBE.Columns.Name, false);
      allRolesTreeView.Model = roleTreeModel;

      objectRoleListBE = new ObjectRoleListWrapper(dbObjectEditorBE);
      objectRoleListModel = new SimpleGrtListModel(rolesTreeView, objectRoleListBE, false);
      objectRoleListModel.AddColumn(roleNameNodeControl, (int)ObjectRoleListWrapper.Columns.Name, false);
      rolesTreeView.Model = objectRoleListModel;

      objectPrivsListBE = objectRoleListBE.get_privilege_list();

      RefreshPrivilegesList();
    }

    public System.Windows.Forms.TabPage PrivilegesTabPage
    {
      get { return privilegesTabPage; }
    }


    private void RefreshPrivilegesList()
    {
      privCheckedListBox.Items.Clear();

      int count = objectPrivsListBE.count();
      String caption;
      int enabled;

      for (short i = 0; i < count; i++)
      {
        objectPrivsListBE.get_field(new NodeIdWrapper(i), (int)ObjectPrivilegeListBE.Columns.Name, out caption);
        objectPrivsListBE.get_field(new NodeIdWrapper(i), (int)ObjectPrivilegeListBE.Columns.Enabled, out enabled);
        privCheckedListBox.Items.Add(caption, enabled != 0);
      }
    }


    private void roleAssignButton_Click(object sender, EventArgs e)
    {
      foreach (Aga.Controls.Tree.TreeNodeAdv node in allRolesTreeView.SelectedNodes)
      {
        GrtTreeNode aNode = node.Tag as GrtTreeNode;

        objectRoleListBE.add_role_for_privileges(roleTreeBE.get_role_with_id(aNode.NodeId));
      }
      objectRoleListModel.RefreshModel();
    }

    private void roleRemoveButton_Click(object sender, EventArgs e)
    {
      foreach (Aga.Controls.Tree.TreeNodeAdv node in rolesTreeView.SelectedNodes)
      {
        GrtListNode aNode = node.Tag as GrtListNode;

        objectRoleListBE.remove_role_from_privileges(roleTreeBE.get_role_with_id(aNode.NodeId));
      }
      objectRoleListModel.RefreshModel();
    }

    private void privCheckedListBox_ItemCheck(object sender, ItemCheckEventArgs e)
    {
      objectPrivsListBE.set_field(new NodeIdWrapper((short)e.Index), (int)ObjectPrivilegeListBE.Columns.Enabled,
        e.NewValue == CheckState.Checked ? 1 : 0);
    }

    private void allRolesTreeView_SelectionChanged(object sender, EventArgs e)
    {
      if (allRolesTreeView.SelectedNode == null)
        roleAssignButton.Enabled = false;
      else
        roleAssignButton.Enabled = true;
    }

    private void rolesTreeView_SelectionChanged(object sender, EventArgs e)
    {
      if (rolesTreeView.SelectedNode == null)
      {
        roleRemoveButton.Enabled = false;
        objectRoleListBE.set_selected(new NodeIdWrapper());
      }
      else
      {
        GrtListNode aNode = rolesTreeView.SelectedNode.Tag as GrtListNode;

        roleRemoveButton.Enabled = true;
        objectRoleListBE.set_selected(aNode.NodeId);
      }
      RefreshPrivilegesList();
    }
	}
}
