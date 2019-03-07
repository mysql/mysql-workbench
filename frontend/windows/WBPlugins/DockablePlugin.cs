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
using System.Runtime.InteropServices;

using MySQL.Base;
using MySQL.Controls;
using MySQL.Grt;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench.Plugins
{
	/// <summary>
	/// Generic GRT Object Editor
	/// </summary>
  public partial class DockablePlugin : TabDocument, IWorkbenchDocument
  {
    #region Member Variables

    /// <summary>
		/// The GRT Manager that controls the GRT.
		/// </summary>
		private GrtManager grtManager;

		/// <summary>
		/// GCHandle, needed for fixed pointer
		/// </summary>
		GCHandle gcHandle;

    #endregion

    #region Constructors

    /// <summary>
		/// Standard constructor
		/// </summary>
		protected DockablePlugin()
		{
			InitializeComponent();
		}

		/// <summary>
		/// Overloaded constructor taking the GRT Manager and GRT object to edit
		/// </summary>
		/// <param name="grtManager">The GRT Manager</param>
		public DockablePlugin(GrtManager grtManager)
			: this()
		{
			this.grtManager = grtManager;
		}

    private void Destroy()
    {
      if (gcHandle.IsAllocated)
        gcHandle.Free();
    }

    #endregion

    #region Properties

    public GrtManager GrtManager { get { return grtManager;} }

    public WbContext Context { get; set; }

    #endregion Properties

    #region Memory Handling

    /// <summary>
		/// Returns a fixed pointer to this object that will not be modified by the GC
		/// </summary>
		/// <returns>fixed int pointer to this object</returns>
		public virtual IntPtr GetFixedPtr()
		{
			if (!gcHandle.IsAllocated)
				 gcHandle = GCHandle.Alloc(this);

			return GCHandle.ToIntPtr(gcHandle);
		}

		/// <summary>
		/// Returns the object based on the fixed pointer retrieved by GetFixedPtr()
		/// </summary>
		/// <param name="ptr">The pointer to look up</param>
		/// <returns>The corresponding instance</returns>
		static public DockablePlugin GetFromFixedPtr(IntPtr ptr)
		{
			GCHandle gcHandle = GCHandle.FromIntPtr(ptr);
      return (DockablePlugin)gcHandle.Target;
    }

    #endregion

    #region IWorkbenchDocument Members

    public virtual UIForm BackendForm
    {
      get { return null; }
    }

    public virtual void RefreshGUI(MySQL.Workbench.RefreshType refresh, string str, IntPtr ptr)
    {
    }

    public virtual void PerformCommand(string command)
    {
    }

    /// <summary>
    /// Reassign all application colors the user can change.
    /// Usually called when colors have been changed.
    /// </summary>
    public virtual void UpdateColors()
    {
      if (Controls.Count > 0 && Controls[0] is DrawablePanel)
        Controls[0].BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      else
        BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      ApplyColors(this);
    }

    public virtual DockablePlugin FindPluginOfType(Type type)
    {
      return null;
    }

    public virtual bool ClosePluginOfType(Type type)
    {
      return false;
    }

    virtual public bool CanCloseDocument()
    {
      return true;
    }

    virtual public void CloseDocument()
    {
      Context.close_gui_plugin(GetFixedPtr());
      Close();
    }

    #endregion

    #region Miscellaneous

    private void ApplyColors(Control parent)
    {
      foreach (Control control in parent.Controls)
      {
        if (control is FlatTabControl)
        {
          FlatTabControl tabView = control as FlatTabControl;
          tabView.UpdateColors();
          // TabControls embedded in plugins probably do not use the main background color.
          // If that is required we have to reach through to those specific places and fix that individually.
          //tabView.BackgroundColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
        }
        else
          if (control is HeaderPanel)
          {
            HeaderPanel panel = control as HeaderPanel;
            panel.HeaderColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelHeader, false);
            panel.ForeColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelHeader, true);
            panel.HeaderColorFocused = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelHeaderFocused, false);
            panel.ForeColorFocused = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelHeaderFocused, true);
          }
          else
            if (control is ToolStrip)
            {
              ToolStrip toolStrip = control as ToolStrip;
              toolStrip.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelToolbar, false);
              toolStrip.ForeColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelToolbar, true);
            }
            else
              if (control is TabPage)
              {
                TabPage page = control as TabPage;
                if (page.Parent is FlatTabControl)
                {
                  FlatTabControl view = page.Parent as FlatTabControl;
                  if (view.TabStyle == FlatTabControl.TabStyleType.BottomNormal)
                    page.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelContentArea, false);
                }
              }

        if (control.ContextMenuStrip != null)
        {
          if (Conversions.UseWin8Drawing())
            control.ContextMenuStrip.Renderer = new Win8MenuStripRenderer();
          else
            control.ContextMenuStrip.Renderer = new TransparentMenuStripRenderer();
        }
        ApplyColors(control);
      }
    }

    #endregion

  }
}