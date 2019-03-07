/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

using MySQL.Grt;
using MySQL.Utilities;

namespace MySQL.GUI.Workbench.Plugins
{
  using Pages = LinkedList<PageInfo>;
  using MySQL.Workbench;

  public class WizardPluginPages : Form
  {
    protected WizardPlugin wizard;
    private Pages pages;
    protected PageTasks pageTasks;

    public WizardPluginPages(WizardPlugin wizard)
    {
      // construct list of wizard pages
      pages = new Pages();

      // tasks for current page (to be set for pages with multiple interpedependent pageTasks)
      pageTasks = new PageTasks();

      Wizard = wizard;
    }

    public WizardPlugin Wizard
    {
      get { return wizard; }
      set
      {
        wizard = value;
        pageTasks.wizard = value;
        if (null != value)
        {
          if (this != value.WizardPages)
            value.WizardPages = this;
        }
      }
    }

    public Pages Pages
    {
      get { return pages; }
    }

    protected void AddPage(Panel panel, PageRoutine initializePage, PageRoutine cleanupPage, PageRoutine processPage, PageRoutine processToggleAdvanced, PageRoutine isPageToBeSkipped, String caption, String description)
    {
      pages.AddLast(new PageInfo(panel, initializePage, cleanupPage, processPage, processToggleAdvanced, isPageToBeSkipped, caption, description));
    }

    protected void StepProlog(Label stepResultLabel, Label stepStageLabel)
    {
      stepResultLabel.ForeColor = DefaultForeColor;
      stepResultLabel.Text = "Executing...";
      stepStageLabel.Text = "";
    }

    protected void TaskMediator(Panel stepPanel, TextBox stepLogTextBox, List<String> messages)
    {
      stepPanel.Update();
      FlushMessages(messages, stepLogTextBox);
      stepPanel.Update();
    }

    protected void StepEpilog(String errMsg, Label stepResultLabel, Label stepStageLabel)
    {
      stepStageLabel.Text = "";
      if (0 == errMsg.Length)
        stepResultLabel.Text = "Execution completed successfully.";
      else
      {
        stepResultLabel.ForeColor = Color.Red;
        stepResultLabel.Text = "An error occured. See the log.";
      }
    }

    protected void FlushMessages(List<string> messages, TextBox textBox)
    {
      for (int n = 0; n < messages.Count; ++n)
        textBox.Text += messages[n] + System.Console.Out.NewLine;
      messages.Clear();
      textBox.Update();
    }
  }

  #region aux classes
  public class PageTask
  {
    public PageTasks pageTasks;
    public bool async;
    public bool failedAsync = false;
    public bool runningAsync = false;

    public bool ErrMsgExists = false;
    public bool WarnMsgExists = false;
    public bool continueOnErrMsg = false;

    public String desc;
    public MySQLTaskStatusLabel label;
    public Label stageLabel;
    public Label resultLabel;
    public TextBox logTextBox;
    public ProgressBar progressBar;
    public Label progressDetailsLabel;

    public Panel panel;

    public delegate String RunTaskDelegate(); //! todo: replace String with void
    public RunTaskDelegate execTask;

    public delegate void PrepareToExecDelegate(PageTask task);
    public PrepareToExecDelegate prepareToExec;
    public delegate void ProcessTaskFailDelegate(PageTask task);
    public ProcessTaskFailDelegate processTaskFail;
    public delegate void ProcessTaskFinishDelegate(PageTask task);
    public ProcessTaskFinishDelegate processTaskFinish;

    public void Init()
    {
      label.TaskState = MySQLTaskStatusLabelState.TaskOpen;
      resultLabel.Text = "";
      stageLabel.Text = "";
      if (null != progressBar)
        progressBar.Value = progressBar.Minimum;
      if (null != progressDetailsLabel)
        progressDetailsLabel.Text = "";
      panel.Update();
    }
    public void PrepareToExec(int tasksCount, int initialTasksCount)
    {
      resultLabel.ForeColor = Control.DefaultForeColor;
      resultLabel.Text = "Executing...";
      stageLabel.Text = "";
      label.Text = desc;
      label.TaskState = MySQLTaskStatusLabelState.TaskOpen;
      panel.Update();
      if (null != prepareToExec)
      {
        prepareToExec(this);
        panel.Update();
      }

      {
        // header for task in message log panel
        String msg = "Phase [" + (initialTasksCount - tasksCount + 1) + "/" + initialTasksCount + "]: " + desc;
        ProcessTaskMsg((int)Msg_type.MT_info, msg);
      }
    }
    public void MarkSucceded(int tasksCount, int initialTasksCount)
    {
      label.TaskState = MySQLTaskStatusLabelState.TaskCompleted;
      stageLabel.Text = "";
      resultLabel.Text = "Execution completed successfully.";
      if (null != progressBar)
        progressBar.Value = progressBar.Maximum;
      panel.Update();
      ProcessTaskMsg((int)Msg_type.MT_info, "OK");
    }
    public void MarkFailed(int tasksCount, int initialTasksCount)
    {
      label.TaskState = MySQLTaskStatusLabelState.TaskError;
      stageLabel.Text = "";
      resultLabel.ForeColor = Color.Red;
      resultLabel.Text = "An error occured. See the log.";
    }
    public int ProcessTaskMsg(int msgType, String message)
    {
      String msgTypeStr;
      if ((Msg_type)msgType == Msg_type.MT_error)
      {
        ErrMsgExists = true;
        pageTasks.ErrMsgExists = true;
        if (!continueOnErrMsg)
          failedAsync = true;
        msgTypeStr = "ERROR: ";
      }
      else if ((Msg_type)msgType == Msg_type.MT_warning)
      {
        WarnMsgExists = true;
        pageTasks.WarnMsgExists = true;
        msgTypeStr = "WARNING: ";
      }
      else
        msgTypeStr = "";

      message = msgTypeStr + message;

      if (null != logTextBox)
      {
        logTextBox.Text += message + System.Console.Out.NewLine;
        logTextBox.Update();
      }
      return 0;
    }
    public int ProcessTaskProgress(float progressState, String message)
    {
      if (null != progressBar)
      {
        progressBar.Value = (int)(progressState * (float)100);
        progressBar.Update();
      }
      if (null != progressDetailsLabel)
      {
        progressDetailsLabel.Text = message;
        progressDetailsLabel.Update();
      }
      return 0;
    }
    public int ProcessTaskFail(String message)
    {
      failedAsync = true;
      if (null != processTaskFail)
        processTaskFail(this);
      pageTasks.Exec();

      return 0;
    }
    public int ProcessTaskFinish()
    {
      if (null != processTaskFinish)
        processTaskFinish(this);
      pageTasks.Exec();
      return 0;
    }
  };

  public class PageTasks
  {
    public WizardPlugin wizard;
    private List<PageTask> pageTasks = new List<PageTask>();
    private int initialCount = -1;
    private bool needRestoreControlsState = false;

    public bool ErrMsgExists = false;
    public bool WarnMsgExists = false;
    private bool failed = false;
    public bool Failed
    {
      get { return failed; }
    }

    private Label resultLabel;
    public delegate void AfterExecTaskDelegate();
    private AfterExecTaskDelegate afterExecTask;

    private void Clear()
    {
      failed = false;
      ErrMsgExists = false;
      WarnMsgExists = false;
      initialCount = -1;
      resultLabel = null;
      afterExecTask = null;
      needRestoreControlsState = false;
      pageTasks.Clear();
    }

    public void Setup(Label resultLabel, AfterExecTaskDelegate afterExecTask)
    {
      Clear();
      this.resultLabel = resultLabel;
      this.afterExecTask = afterExecTask;
    }

    void SetResultState()
    {
      if (null == resultLabel)
        return;

      if (failed)
      {
        resultLabel.ForeColor = Color.Red;
        resultLabel.Text = "An error occured. See the log.";
      }
      else if (ErrMsgExists)
      {
        resultLabel.ForeColor = Color.Red;
        resultLabel.Text = "At least 1 non-critical error appears in the log.";
      }
      else if (WarnMsgExists)
      {
        resultLabel.ForeColor = Color.Red;
        resultLabel.Text = "At least 1 warning appears in the log.";
      }
      else
        resultLabel.Text = "Execution completed successfully.";
    }

    public String Exec() //! todo: replace String with void
    {
      String errMsg = "";
      try
      {
        if (-1 == initialCount)
          initialCount = pageTasks.Count;

        while (0 < pageTasks.Count)
        {
          PageTask task = pageTasks[0];

          if (task.runningAsync)
          {
            task.runningAsync = false;
            if (task.failedAsync)
              throw new Exception();
            else
            {
              task.MarkSucceded(pageTasks.Count, initialCount);
              task.ProcessTaskMsg((int)Msg_type.MT_info, ""); // delimit task log messages with empty line
              pageTasks.Remove(task);
              continue;
            }
          }

          task.PrepareToExec(pageTasks.Count, initialCount);

          if (task.async)
          {
            wizard.SkipRestoreControlsStateOnce();
            needRestoreControlsState = true;
          }

          task.execTask();

          if (!task.async)
          {
            task.MarkSucceded(pageTasks.Count, initialCount);
            task.ProcessTaskMsg((int)Msg_type.MT_info, ""); // delimit task log messages with empty line
            pageTasks.Remove(task);
          }
          else
          {
            task.runningAsync = true;
            return errMsg;
          }
        }
      }
      catch (Exception e)
      {
        pageTasks[0].ProcessTaskMsg((int)Msg_type.MT_info, "FAILED" + ((0 < e.Message.Length) ? ": " + e.Message : ""));
        foreach (PageTask task in pageTasks)
          task.MarkFailed(pageTasks.Count, initialCount);
        pageTasks.Clear();
        failed = true;
        ErrMsgExists = true;
        errMsg = "-";
      }
      if (needRestoreControlsState)
        wizard.RestoreControlsState(errMsg);
      if (0 == pageTasks.Count)
      {
        SetResultState();
        if (null != afterExecTask)
          afterExecTask();
      }
      return errMsg;
    }

    public PageTask Add(
      bool async,
      bool continueOnErrMsg,
      String desc,
      MySQLTaskStatusLabel label,
      Label stageLabel,
      Label resultLabel,
      Panel panel,
      TextBox logTextBox,
      PageTask.PrepareToExecDelegate prepareToExec,
      PageTask.RunTaskDelegate execTask,
      PageTask.ProcessTaskFailDelegate processTaskFail,
      PageTask.ProcessTaskFinishDelegate processTaskFinish,
      ProgressBar progressBar,
      Label progressDetailsLabel
      )
    {
      PageTask task = new PageTask();
      task.pageTasks = this;

      task.async = async;
      task.continueOnErrMsg = continueOnErrMsg;

      task.desc = desc;
      task.label = label;
      task.stageLabel = stageLabel;
      task.resultLabel = resultLabel;
      task.panel = panel;
      task.logTextBox = logTextBox;

      task.prepareToExec = prepareToExec;
      task.execTask = execTask;
      task.processTaskFail = processTaskFail;
      task.processTaskFinish = processTaskFinish;

      task.progressBar = progressBar;
      task.progressDetailsLabel = progressDetailsLabel;

      task.Init();

      pageTasks.Add(task);

      return task;
    }
  };
  #endregion aux classes
}
