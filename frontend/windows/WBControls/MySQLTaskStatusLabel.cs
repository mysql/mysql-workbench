/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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

using System.ComponentModel;
using System.Windows.Forms;

using MySQL.Controls.Properties;

namespace MySQL.Utilities
{
  public enum MySQLTaskStatusLabelState { TaskOpen, TaskCompleted, TaskError, TaskDisabled };

  public partial class MySQLTaskStatusLabel : UserControl
  {
    #region Member Variables

    private MySQLTaskStatusLabelState taskState = MySQLTaskStatusLabelState.TaskDisabled;

    #endregion

    #region Constructor

    public MySQLTaskStatusLabel()
    {
      InitializeComponent();

      TaskState = MySQLTaskStatusLabelState.TaskOpen;
    }

    #endregion

    #region Properties

    [Category("Appearance")]
    [EditorBrowsable(EditorBrowsableState.Always)]
    [Browsable(true)]
    [DesignerSerializationVisibility(DesignerSerializationVisibility.Visible)]
    [Bindable(true)]
    [Description("The caption of the label.")]
    public override string Text
    {
      get { return taskLabel.Text; }
      set { taskLabel.Text = value; }
    }

    [Bindable(true), Category("Behavior"),
    Description("The state of the task.")]
    public MySQLTaskStatusLabelState TaskState
    {
      get { return taskState; }
      set
      {
        if (taskState != value)
        {
          switch (value)
          {
            case MySQLTaskStatusLabelState.TaskOpen:
              taskPictureBox.Image = Resources.task_unchecked;
              break;
            case MySQLTaskStatusLabelState.TaskCompleted:
              taskPictureBox.Image = Resources.task_checked;
              break;
            case MySQLTaskStatusLabelState.TaskError:
              taskPictureBox.Image = Resources.task_error;
              break;
            default:
              taskPictureBox.Image = Resources.task_disabled;
              break;
          }
          taskState = value;
        }
      }
    }

    #endregion
  }
}
