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
using System.Windows.Forms;

using MySQL.Grt;

namespace MySQL.GUI.Workbench.Plugins
{
  using Pages = LinkedList<PageInfo>;
  using PagesNode = LinkedListNode<PageInfo>;

  public delegate string PageRoutine();

  public partial class WizardPlugin : DockablePlugin
  {
    #region aux classes
    public class ControlsState
    {
      private List<Control> controls;
      private List<bool> states;

      public void SaveControlsState(List<Control> controls)
      {
        if (controls == null)
          return;

        this.controls = controls;
        states = new List<bool>(controls.Count);
        foreach (Control control in controls)
        {
          if (control == null)
            states.Add(false);
          else
            states.Add(control.Enabled);
        }
      }

      public void RestoreContolsState(List<Control> controlsToSkipOnRestore)
      {
        if (controls == null)
          return;

        for (int n = 0; n < controls.Count; ++n)
        {
          Control control = controls[n];
          if (control != null)
          {
            bool skip = false;
            if (null != controlsToSkipOnRestore)
            {
              foreach (Control c in controlsToSkipOnRestore)
              {
                if (c.Text == control.Text)
                {
                  skip = true;
                  break;
                }
              }
            }
            if (!skip)
              control.Enabled = states[n];
          }
        }
      }

      public void Reset()
      {
        controls = null;
        states = null;
      }
    }
    #endregion aux classes

    #region Member Variables

    private WizardPluginPages wizardPages;
    private Pages pages;
    private PagesNode activePageNode;
    private PageInfo activePage;
    private ControlsState savedControlsState;
    List<Control> controlsToSkipOnRestore; // needed to disable 'Next' button from page initialization routine
    bool skipRestoreControlsStateOnce; // needed when async tasks are to be used from within pages logic
    string lastActionErrMsg;
    delegate void UpdateControlsStateAfterPageAction(string errMsg);
    UpdateControlsStateAfterPageAction updateControlsStateAfterPageAction;

    #endregion

    #region Constructor

    public WizardPlugin(GrtManager manager)
      : base(manager)
    {
      InitializeComponent();

      savedControlsState = new ControlsState();
    }

    #endregion

    #region Properties

    public WizardPluginPages WizardPages
    {
      get { return wizardPages; }
      set
      {
        wizardPages = value;
        if (null != value)
        {
          if (this != value.Wizard)
            value.Wizard = this;
        }
      }
    }

    private Pages Pages
    {
      get { return pages; }
      set { pages = value; }
    }

    public Button NextPageButton
    {
      get { return btnNext; }
    }

    private PageInfo ActivePage
    {
      set
      {
        if (activePage != value)
        {
          if (null != activePage && null != activePage.Panel)
            activePage.Panel.Parent = activePage.InitialParent;

          activePage = value;

          if (null != activePage)
          {
            captionLabel.Text = activePage.Caption;
            descriptionLabel.Text = activePage.Description;
            activePage.Panel.Parent = panel;
            stepDescriptionPanel.Update();

            // update navigation buttons state
            btnBack.Enabled = !IsFirstPage();//! && !IsLastPage();
            btnCancel.Enabled = !IsLastPage() || (pages.Count < 2);
            if (IsLastPage())
              btnNext.Text = "Finish";
            else
              btnNext.Text = "Next >";

            // hide 'advanced' if not appropriate
            btnAdvanced.Visible = (null != activePage.ToggleAdvanced);
          }
        }
      }
    }

    #endregion

    #region Class functions

    public void Start()
    {
      if (null == wizardPages)
        return;
      Pages = wizardPages.Pages;
      if (null == Pages)
        return;

      LeafThroughPages(true);
      InitializeActivePage();
    }

    private void InitializeActivePage()
    {
      // initialize 'next' page of wizard process & if failed to do so, disable 'next' button
      if (null != activePage)
      {
        if (null != activePage.InitializePage)
        {
          SaveControlsStateAndDisable();
          lastActionErrMsg = activePage.InitializePage();
        }
      }
      updateControlsStateAfterPageAction = UpdateAfterPageInitialization;
      RestoreControlsState(lastActionErrMsg);
    }

    void UpdateAfterPageInitialization(string errMsg)
    {
      if (null == errMsg)
        return;

      if (0 < errMsg.Length)
      {
        if ("-" != errMsg)
          MessageBox.Show(errMsg);
      }
      if (0 != errMsg.Length)
        btnNext.Enabled = false;
    }

    private void SetActivePage(PagesNode pageNode)
    {
      activePageNode = pageNode;
      ActivePage = pageNode.Value;
    }

    private void LeafThroughPages(bool forward)
    {
      if (activePageNode == null)
        SetActivePage(forward ? Pages.First : Pages.Last);
      else
        SetActivePage(forward ? activePageNode.Next : activePageNode.Previous);

      while ((activePage != null)
          && (activePage.IsPageToBeSkipped != null)
          && (activePage.IsPageToBeSkipped().Length != 0))
        SetActivePage(forward ? activePageNode.Next : activePageNode.Previous);

      if (activePage == null)
        SetActivePage(forward ? Pages.Last : Pages.First);
    }

    private bool IsLastPage()
    {
      if (Pages.Last.Equals(activePageNode))
        return true;

      LinkedListNode<PageInfo> pageNode = activePageNode.Next;
      while ((null != pageNode) && (null != pageNode.Value)
          && (pageNode.Value.IsPageToBeSkipped != null)
          && (pageNode.Value.IsPageToBeSkipped().Length != 0))
        pageNode = pageNode.Next;

      return ((null == pageNode) || (null == pageNode.Value));
    }

    private bool IsFirstPage()
    {
      if (Pages.First.Equals(activePageNode))
        return true;

      LinkedListNode<PageInfo> pageNode = activePageNode.Previous;
      while ((null != pageNode) && (null != pageNode.Value)
          && (pageNode.Value.IsPageToBeSkipped != null)
          && (pageNode.Value.IsPageToBeSkipped().Length != 0))
        pageNode = pageNode.Previous;

      return ((null == pageNode) || (null == pageNode.Value));
    }

    private void SaveControlsState()
    {
      List<Control> controls = new List<Control>();
      controls.Add(btnAdvanced);
      controls.Add(btnBack);
      controls.Add(btnNext);
      controls.Add(btnCancel);
      savedControlsState.SaveControlsState(controls);
    }

    public void RestoreControlsState(string errMsg)
    {
      if (skipRestoreControlsStateOnce)
        return;

      if ((null != lastActionErrMsg) && (lastActionErrMsg.Length == 0))
        lastActionErrMsg = errMsg;

      savedControlsState.RestoreContolsState(controlsToSkipOnRestore);
      savedControlsState.Reset();
      controlsToSkipOnRestore = null;

      if (updateControlsStateAfterPageAction != null)
      {
        updateControlsStateAfterPageAction(lastActionErrMsg);
        if (!skipRestoreControlsStateOnce) // it could be changed since first check because this proc is reentrant
          updateControlsStateAfterPageAction = null;
      }

      lastActionErrMsg = "";
    }

    private void SaveControlsStateAndDisable()
    {
      SaveControlsState();

      btnAdvanced.Enabled = true;
      btnBack.Enabled = false;
      btnNext.Enabled = false;
      btnCancel.Enabled = false;
    }

    public void SkipRestoreControlsStateOnce()
    {
      skipRestoreControlsStateOnce = true;
    }

    public void ControlsToSkipOnce(List<Control> controls)
    {
      controlsToSkipOnRestore = controls;
    }

    public void GoToNextPage()
    {
      if (null != activePage.ProcessPage)
      {
        SaveControlsStateAndDisable();
        lastActionErrMsg = activePage.ProcessPage();
      }

      updateControlsStateAfterPageAction = UpdateAfterPageProcessing;
      RestoreControlsState(lastActionErrMsg);

      if (skipRestoreControlsStateOnce)
        skipRestoreControlsStateOnce = false;
    }

    void UpdateAfterPageProcessing(string errMsg)
    {
      if (0 == errMsg.Length)
      {
        if (!IsLastPage())
        {
          LeafThroughPages(true);
          InitializeActivePage();
        }
        else
          Close();
      }
      else
        MessageBox.Show(errMsg);
    }

    public void GoToPreviousPage()
    {
      if (!IsFirstPage())
      {
        // cleanup page before stepping back through wizard process & if failed to do so, inhibit transition
        if (null != activePage && null != activePage.CleanupPage)
        {
          SaveControlsStateAndDisable();
          lastActionErrMsg = activePage.CleanupPage();
        }

        updateControlsStateAfterPageAction = UpdateAfterPageBack;
        RestoreControlsState(lastActionErrMsg);

        if (skipRestoreControlsStateOnce)
          skipRestoreControlsStateOnce = false;
      }
    }

    void UpdateAfterPageBack(string errMsg)
    {
      if (0 == errMsg.Length)
      {
        LeafThroughPages(false);
        btnNext.Enabled = true;
      }
      else
        MessageBox.Show(errMsg);
    }

    #endregion

    #region UI Event Handling

    private void btnNext_Click(object sender, EventArgs e)
    {
      try
      {
        GoToNextPage();
      }
      catch(System.ApplicationException ex)
      {
        System.Windows.Forms.MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        GoToPreviousPage();
      }
    }

    private void btnBack_Click(object sender, EventArgs e)
    {
      GoToPreviousPage();
    }

    private void btnCancel_Click(object sender, EventArgs e)
    {
      Close();
    }

    private void btnAdvanced_Click(object sender, EventArgs e)
    {
      if (null != activePage.ToggleAdvanced)
        activePage.ToggleAdvanced();
    }

    #endregion
  }

  public class PageInfo
  {
    #region Member Variables

    private Panel panel;
    private PageRoutine initializePage;
    private PageRoutine cleanupPage;
    private PageRoutine processPage;
    private PageRoutine toggleAdvanced;
    private PageRoutine isPageToBeSkipped;
    private string caption;
    private string description;
    private Control initialParent;

    #endregion

    #region Constructor

    public PageInfo(Panel panel,
      PageRoutine initializePage,
      PageRoutine cleanupPage,
      PageRoutine processPage,
      PageRoutine toggleAdvanced,
      PageRoutine isPageToBeSkipped,
      string caption,
      string description)
    {
      this.panel = panel;
      this.initializePage = initializePage;
      this.cleanupPage = cleanupPage;
      this.processPage = processPage;
      this.toggleAdvanced = toggleAdvanced;
      this.isPageToBeSkipped = isPageToBeSkipped;
      this.caption = caption;
      this.description = description;
      if (null != panel)
        initialParent = panel.Parent;
    }

    #endregion

    #region Properties

    public Panel Panel
    {
      get { return panel; }
    }

    public PageRoutine InitializePage
    {
      get { return initializePage; }
    }

    public PageRoutine CleanupPage
    {
      get { return cleanupPage; }
    }

    public PageRoutine ProcessPage
    {
      get { return processPage; }
    }

    public PageRoutine ToggleAdvanced
    {
      get { return toggleAdvanced; }
    }

    public PageRoutine IsPageToBeSkipped
    {
      get { return isPageToBeSkipped; }
    }

    public string Caption
    {
      get { return caption; }
    }

    public string Description
    {
      get { return description; }
    }

    public Control InitialParent
    {
      get { return initialParent; }
    }

    #endregion
  }

}