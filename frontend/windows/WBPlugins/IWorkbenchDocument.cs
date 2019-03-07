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
