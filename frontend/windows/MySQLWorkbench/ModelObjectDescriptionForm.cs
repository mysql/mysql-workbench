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
using System.Drawing;
using System.Windows.Forms;

using MySQL.Base;
using MySQL.Controls;
using MySQL.Grt;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  public partial class ModelObjectDescriptionForm : TabDocument
  {
    #region Variables

    WbContext wbContext;
    UIForm form;

    private bool initializing = false;
    private bool multipleObjectsSelected = false;
    private bool noItemsSelected = true;
    private bool textHasChanged = false;
    private GrtValue activeObjList;

    private Timer changeTimer;

    #endregion

    #region Constructors

    /// <summary>
    /// Private core constructor
    /// </summary>
    private ModelObjectDescriptionForm()
    {
      InitializeComponent();
    }

    /// <summary>
    /// Public constrcutor taking WbContext, calling core constructor
    /// </summary>
    /// <param name="WbContext">A WbContext</param>
    public ModelObjectDescriptionForm(WbContext WbContext) : this()
    {
      wbContext = WbContext;

      changeTimer = new Timer();
      changeTimer.Interval = 700;
      changeTimer.Tick += new EventHandler(changeTimer_Tick);

      ObjectDescriptionEnabled = false;
    }

    #endregion

    #region Properties

    /// <summary>
    /// Use to set the object description text
    /// </summary>
    public String ObjectDescription
    {
      get { return descriptionTextBox.Text; }
      set { descriptionTextBox.Text = value; }
    }

    /// <summary>
    /// Enables or disables the object description text box
    /// </summary>
    public bool ObjectDescriptionEnabled
    {
      get { return !descriptionTextBox.ReadOnly; }
      set
      { 
        descriptionTextBox.ReadOnly = !value;
        /*
        if (value)
          descriptionTextBox.BackColor = Color.White;
        else
          descriptionTextBox.BackColor = Color.WhiteSmoke;
         * */
      }
    }

    /// <summary>
    /// Set true when initializing the ObjectDescription
    /// </summary>
    private bool Initializing
    {
      get { return initializing; }
      set
      {
        initializing = value;

        // When initializing, also stop the change timer
        if (initializing)
          changeTimer.Stop();
      }
    }

    public bool MultipleItemsSelected
    {
      get { return multipleObjectsSelected; }
      set { multipleObjectsSelected = value; }
    }

    public bool NoItemsSelected
    {
      get { return noItemsSelected; }
      set { noItemsSelected = value; }
    }

    #endregion

    #region UI Logic

    public void UpdateColors()
    {
      topPanel.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelToolbar, false);
    }

    public void UpdateForView(UIForm Form)
    {
      if (changeTimer.Enabled)
        PushChanges();

      // Make sure we are initializing
      Initializing = true;

      try
      {
        List<String> items;
        GrtValue newObjList;
        String objDescr;
        form = Form;

        if (null != form)
          objDescr = wbContext.get_description_for_selection(form, out newObjList, out items);
        else
          objDescr = wbContext.get_description_for_selection(out newObjList, out items);

        // update only if selection was changed
        if (!wbContext.are_lists_equal(activeObjList, newObjList))
        {
          objSelComboBox.Items.Clear();

          // Set description text
          if (null != activeObjList)
            activeObjList.Dispose();
          activeObjList = newObjList;

          // Set properties
          NoItemsSelected = (null == items) || (0 == items.Count);
          MultipleItemsSelected = (items.Count > 1);

          // handle different number of selected items
          if (!NoItemsSelected)
          {
            objSelComboBox.SuspendLayout();
            objSelComboBox.Items.Clear();
            objSelComboBox.Items.AddRange(items.ToArray());
            objSelComboBox.ResumeLayout();

            objSelComboBox.SelectedIndex = 0;

            // lock on multi selection
            if (MultipleItemsSelected)
            {
              objDescr = "<double-click to overwrite multiple objects>";
              ObjectDescriptionEnabled = false;
            }
            else
              ObjectDescriptionEnabled = true;
          }
          else
          {
            objSelComboBox.SuspendLayout();
            objSelComboBox.Items.Clear();
            objSelComboBox.Items.Add("No Selection");
            objSelComboBox.ResumeLayout();

            objSelComboBox.SelectedIndex = 0;

            objDescr = "";
            ObjectDescriptionEnabled = false;
          }

          ObjectDescription = objDescr;
        }
      }
      finally
      {
        Initializing = false;
      }
    }

    /// <summary>
    /// applies changes
    /// </summary>
    void PushChanges()
    {
      changeTimer.Stop();
      if (activeObjList != null && textHasChanged)
      {
        textHasChanged = false;
        wbContext.set_description_for_selection(activeObjList, ObjectDescription);
      }
    }

    #endregion

    #region Events

    /// <summary>
    /// if not initalizing and change timer is triggert, push changes
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    void changeTimer_Tick(object sender, EventArgs e)
    {
      // call push with original text
      if (!Initializing)
        PushChanges();
    }

    /// <summary>
    /// if not initializing, trigger change timer
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void descriptionTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!Initializing)
      {
        textHasChanged = true;

        // Start or restart change timer
        changeTimer.Stop();
        changeTimer.Start();
      }
    }

    /// <summary>
    /// Handle special key input
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void descriptionTextBox_KeyDown(object sender, KeyEventArgs e)
    {
      switch (e.KeyCode)
      {
        case Keys.Enter:
          // in case of Ctrl + Enter, push changes immediately
          if (e.Control && ObjectDescriptionEnabled)
          {
            e.SuppressKeyPress = true;
            e.Handled = true;

            PushChanges();
          }
          break;
      }
    }

    /// <summary>
    /// if multiple objects are selected, and the edit is still locked, unlock on double click and empty text
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void descriptionTextBox_DoubleClick(object sender, EventArgs e)
    {
      if (MultipleItemsSelected && !ObjectDescriptionEnabled)
      {
        try
        {
          Initializing = true;

          ObjectDescription = "";

          ObjectDescriptionEnabled = true;
        }
        finally
        {
          Initializing = false;
        }
      }
    }

    /// <summary>
    /// Prevent the user from selecting a different index than 0
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void objSelComboBox_SelectedIndexChanged(object sender, EventArgs e)
    {
      objSelComboBox.SelectedIndex = 0;
    }

    /// <summary>
    /// Store the text as soon as the user leaves the control
    /// </summary>
    /// <param name="sender"></param>
    /// <param name="e"></param>
    private void descriptionTextBox_Leave(object sender, EventArgs e)
    {
      PushChanges();
    }

    #endregion

  }
}