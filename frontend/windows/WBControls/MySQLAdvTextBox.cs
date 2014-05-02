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

using System;
using System.ComponentModel;
using System.Windows.Forms;

// XXX: candidate for removal. No longer used.
namespace MySQL.Utilities
{
  public delegate void OnTextWasChanged(Object sender, EventArgs eventArgs);
  public delegate void OnDocAction(Object sender, EventArgs eventArgs);

  public partial class MySQLAdvTextBox : UserControl
  {
    #region Member Variables

    protected Timer changeTimer = new Timer();

    protected EventArgs lastTextChangedEventArgs = null;

    protected bool displayClearIcon = false;
    protected bool displayDocActionIcon = false;
    // if true the Icon will be hidden after firing DocAction 
    protected bool autoHideDocActionIcon = false;

    protected bool fireTextWasChangedWithEmptyString = false;

    protected bool initializingControl = false;

    #endregion

    #region Constructor

    public MySQLAdvTextBox()
    {
      InitializeComponent();

      changeTimer.Tick += new EventHandler(changeTimer_Tick);
      changeTimer.Interval = 800;
    }

    #endregion

    #region Properties

    [DefaultValue(false), Category("Appearance")]
    public bool DisplaySearchIcon
    {
      get { return searchPictureBox.Visible; }
      set { searchPictureBox.Visible = value; }
    }

    [Category("Appearance")]
    [EditorBrowsable(EditorBrowsableState.Always)]
    [Browsable(true)]
    [DesignerSerializationVisibility(DesignerSerializationVisibility.Visible)]
    [Bindable(true)]
    public override string Text
    {
      get { return textBox.Text; }
      set { textBox.Text = value; }
    }


    [DefaultValue(false), Category("Behavior")]
    public bool DisplayClearIcon
    {
      get { return displayClearIcon; }
      set 
      {
        displayClearIcon = value;

        if (!value)
          searchPictureBox.Visible = false; 
      }
    }

    [DefaultValue(false), Category("Behavior")]
    public bool DisplayDocActionIcon
    {
      get { return displayDocActionIcon; }
      set
      {
        displayDocActionIcon = value;

        if (!value)
          docActionPictureBox.Visible = false;
      }
    }

    [DefaultValue(false), Category("Behavior")]
    public bool AutoHideDocActionIcon
    {
      get { return autoHideDocActionIcon; }
      set { autoHideDocActionIcon = value; }
    }


    [Category("Behavior")]
    public int ChangeDelayTime
    {
      get { return changeTimer.Interval; }
      set { changeTimer.Interval = value; }
    }

    [Category("Behavior")]
    public bool FireTextWasChangedWithEmptyString
    {
      get { return fireTextWasChangedWithEmptyString; }
      set { fireTextWasChangedWithEmptyString = value; }
    }

    [Category("Behavior")]
    [Browsable(false)]
    public bool InitializingControl
    {
      get { return initializingControl; }
      set { initializingControl = value; }
    }

    #endregion

    #region Events

    public event OnTextWasChanged TextWasChanged;

    public event OnDocAction DocAction;

    #endregion

    #region Public Functions

    public void HideDocActionPictureBox()
    {
      docActionPictureBox.Visible = false;
    }

    public void CheckTextChangedNow()
    {
      changeTimer_Tick(this, null);
    }

    #endregion

    #region UI Handling

    protected void changeTimer_Tick(object sender, EventArgs e)
    {
      changeTimer.Stop();

      if (TextWasChanged != null && 
        (!textBox.Text.Equals("") || 
          fireTextWasChangedWithEmptyString))
        TextWasChanged(this, lastTextChangedEventArgs);
    }

    private void textBox_TextChanged(object sender, EventArgs e)
    {
      changeTimer.Stop();

      if (!initializingControl)
      {
        if (!textBox.Text.Equals(""))
        {
          changeTimer.Start();
          lastTextChangedEventArgs = e;

          if (displayClearIcon)
            clearSearchPictureBox.Visible = true;

          if (displayDocActionIcon)
            docActionPictureBox.Visible = true;
        }
        else
        {
          if (displayClearIcon)
            clearSearchPictureBox.Visible = false;

          if (displayDocActionIcon)
            docActionPictureBox.Visible = false;
        }
      }
    }

    private void clearSearchPictureBox_Click(object sender, EventArgs e)
    {
      textBox.Text = "";
    }

    private void docActionPictureBox_Click(object sender, EventArgs e)
    {
      if (AutoHideDocActionIcon)
        docActionPictureBox.Visible = false;

      if (DocAction != null)
        DocAction(sender, e);
    }

    private void textBox_KeyDown(object sender, KeyEventArgs e)
    {
      OnKeyDown(e);
    }

    private void MySQLAdvTextBox_Enter(object sender, EventArgs e)
    {
      ActiveControl = textBox;
    }

    #endregion

  }
}
