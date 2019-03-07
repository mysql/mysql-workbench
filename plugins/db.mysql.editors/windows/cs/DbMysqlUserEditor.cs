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

using Aga.Controls.Tree;

using MySQL.Grt;
using MySQL.Grt.Db;

namespace MySQL.GUI.Workbench.Plugins
{
  public partial class DbMysqlUserEditor : ObjectEditorPlugin
  {
    #region Member Variables

    private UserEditorBE userEditorBE { get { return Backend as UserEditorBE; } } 
    private RoleTreeBE roleTreeBE;
    //private SimpleGrtTreeModel roleTreeModel;
    private DifferenceByNameGrtTreeModel roleTreeModel;
    private bool assigningRole = false;

    #endregion

    #region Constructors

    public DbMysqlUserEditor(GrtManager manager, GrtValue value)
      : base(manager)
    {
      InitializeComponent();
      ReinitWithArguments(value);
    }

    #endregion

    #region ObjectEditorPlugin Overrides

    public override bool ReinitWithArguments(GrtValue value)
    {
      InitializingControls = true;

      try
      {
        Backend = new UserEditorBE(value);

        nameTextBox.Text = userEditorBE.get_name();
        passwordTextBox.Text = userEditorBE.get_password();
        commentTextBox.Text = userEditorBE.get_comment();

        roleTreeBE = userEditorBE.get_role_tree();
        roleTreeBE.refresh();

        if (roleTreeModel != null)
          roleTreeModel.DetachEvents();
        roleTreeModel = new DifferenceByNameGrtTreeModel(roleTreeView, new List<String>(), roleTreeBE, false);
        roleTreeModel.AddColumn(roleTreeNodeText, (int)RoleTreeBE.Columns.Name, false);
        roleTreeView.Model = roleTreeModel;
        
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

    #region Form implemenation

    private void RefreshAssignedRoles()
    {
      if (!assigningRole)
      {
        List<String> selectedRoles = new List<String>();

        assignedRoleList.Items.Clear();
        foreach (String role in userEditorBE.get_roles())
        {
          assignedRoleList.Items.Add(role);
          selectedRoles.Add(role);
        }

        roleTreeModel.SetDisabledList(selectedRoles);
        roleTreeView.Model = null;
        roleTreeView.Model = roleTreeModel;
      }
    }

    private void addRoleButton_Click(object sender, EventArgs e)
    {
      assigningRole = true;
      foreach (TreeNodeAdv node in roleTreeView.SelectedNodes)
      {
        userEditorBE.add_role(node.ToString());
      }
      assigningRole = false;
      RefreshAssignedRoles();
    }

    private void removeRoleButton_Click(object sender, EventArgs e)
    {
      assigningRole = true;
      foreach (String node in assignedRoleList.SelectedItems)
      {
        userEditorBE.remove_role(node);
      }
      assigningRole = false;
      RefreshAssignedRoles();
    }

    private void nameTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!InitializingControls)
        userEditorBE.set_name(nameTextBox.Text);

      TabText = userEditorBE.get_title();
    }

    private void passwordTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!InitializingControls)
        userEditorBE.set_password(passwordTextBox.Text);
    }

    void commentTextBox_TextChanged(object sender, System.EventArgs e)
    {
      if (!InitializingControls)
        userEditorBE.set_comment(commentTextBox.Text);
    }

    protected override void RefreshFormData() 
    {
      RefreshAssignedRoles();
      TabText = userEditorBE.get_title();
      nameTextBox.Text = userEditorBE.get_name();
    }

    #endregion
  }
}