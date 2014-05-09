/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

using System;
using System.Windows.Forms;

using MySQL.Base;
using MySQL.Forms;

namespace MySQL.GUI.Workbench.Plugins
{
	public interface IWorkbenchDocument
	{
    // Function that returns a pointer to the backend used
    UIForm BackendForm { get; }

    // Refresh command for a specific workbench document.
    void RefreshGUI(MySQL.Workbench.RefreshType refresh, String str, IntPtr ptr);

    // General command handling.
    void PerformCommand(String command);

    void UpdateColors();

    // Plugin handling.
    // Return the first plugin found of a given type.
    DockablePlugin FindPluginOfType(Type type);

    // Close the first plugin of a given type.
    bool ClosePluginOfType(Type type);

    bool CanCloseDocument();
    void CloseDocument();
	}
}
