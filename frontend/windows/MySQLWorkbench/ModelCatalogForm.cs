/*
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

using System.Windows.Forms;

using Aga.Controls.Tree;

using MySQL.Controls;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  public partial class ModelCatalogForm : TabDocument
  {
    private ModelCatalogForm()
    {
      InitializeComponent();
    }

    public ModelCatalogForm(ModelDiagramFormWrapper form)
      : this()
    {
      TreeViewAdv catalogTree = form.get_catalog_tree();
      catalogTree.ShowNodeToolTips = true;
      headerPanel1.Controls.Add(catalogTree);
      headerPanel1.Controls.SetChildIndex(catalogTree, 0);
      catalogTree.Dock = DockStyle.Fill;
    }

    public void UpdateColors()
    {
      topPanel.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelToolbar, false);
    }

  }
}