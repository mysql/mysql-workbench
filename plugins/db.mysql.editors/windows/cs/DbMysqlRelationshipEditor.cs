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

using MySQL.Grt;
using MySQL.Grt.Db;


namespace MySQL.GUI.Workbench.Plugins
{
  public partial class DbMysqlRelationshipEditor : ObjectEditorPlugin
  {
    private MySQLRelationshipEditorWrapper relationshipEditorBE { get { return Backend as MySQLRelationshipEditorWrapper; } }
    private bool refreshing = false;

    #region Constructors

    public DbMysqlRelationshipEditor(GrtManager manager, GrtValue value)
      : base(manager)
    {
      InitializeComponent();

      if (IsEditingLiveObject)
        AdjustEditModeControls(mainTabControl);
      ReinitWithArguments(value);
    }

    #endregion

    protected override void RefreshFormData()
    {
      refreshing = true;

      captionTextBox.Text = relationshipEditorBE.get_caption();
      extraCaptionTextBox.Text = relationshipEditorBE.get_extra_caption();
      commentsTextBox.Text = relationshipEditorBE.get_comment();
      leftMandatoryCheckBox.Checked = relationshipEditorBE.get_left_mandatory();
      rightMandatoryCheckBox.Checked = relationshipEditorBE.get_right_mandatory();

      captionFull1.Text = relationshipEditorBE.get_caption_long();
      captionFull2.Text = relationshipEditorBE.get_extra_caption_long();

      leftTableName.Text = relationshipEditorBE.get_left_table_name();
      rightTableName.Text = relationshipEditorBE.get_right_table_name();
      leftForeignKey.Text = relationshipEditorBE.get_left_table_fk();
      leftColumns.Text = relationshipEditorBE.get_left_table_info();
      rightColumns.Text = relationshipEditorBE.get_right_table_info();

      if (relationshipEditorBE.get_to_many())
        oneToManyRadioButton.Checked = true;
      else
        oneToOneRadioButton.Checked = true;

      switch (relationshipEditorBE.get_visibility())
      { 
        case RelationshipVisibilityType.Visible:
          visibleRadioButton.Checked = true;
          break;
        case RelationshipVisibilityType.Splitted:
          splittedRadioButton.Checked = true;
          break;
        case RelationshipVisibilityType.Hidden:
          hiddenRadioButton.Checked = true;
          break;
      }
      Text = relationshipEditorBE.get_caption();
      TabText = relationshipEditorBE.get_title();
      identifyingRelationshipCheckbox.Checked = relationshipEditorBE.get_is_identifying();

      refreshing = false;
    }

    public override bool ReinitWithArguments(GrtValue value)
    {
      InitializingControls = true;
      SuspendLayout();

      try
      {
        Backend = new MySQLRelationshipEditorWrapper(value);

        RefreshFormData();
      }
      finally
      {
        ResumeLayout();
        InitializingControls = false;
      }

      Invalidate();

      return true;
    }

    private void extraCaptionTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!refreshing)
      {
        relationshipEditorBE.set_extra_caption(extraCaptionTextBox.Text);
        captionFull2.Text = relationshipEditorBE.get_extra_caption_long();
      }
    }

    private void commentsTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!refreshing)
        relationshipEditorBE.set_comment(commentsTextBox.Text);
    }


    private void leftMandatoryCheckBox_CheckedChanged(object sender, System.EventArgs e)
    {
      if (!refreshing)
        relationshipEditorBE.set_left_mandatory(leftMandatoryCheckBox.Checked);
    }

    private void oneToOneRadioButton_CheckedChanged(object sender, System.EventArgs e)
    {
      if (!refreshing)
        relationshipEditorBE.set_to_many(false);
    }

    private void oneToManyRadioButton_CheckedChanged(object sender, System.EventArgs e)
    {
      if (!refreshing)
        relationshipEditorBE.set_to_many(true);
    }

    private void captionTextBox_TextChanged(object sender, System.EventArgs e)
    {
      if (!refreshing)
      {
        relationshipEditorBE.set_caption(captionTextBox.Text);
        Text = relationshipEditorBE.get_caption();
        TabText = relationshipEditorBE.get_title();

        captionFull1.Text = relationshipEditorBE.get_caption_long();
      }
    }

    private void rightMandatoryCheckBox_CheckedChanged(object sender, EventArgs e)
    {
      if (!refreshing)
        relationshipEditorBE.set_right_mandatory(rightMandatoryCheckBox.Checked);
    }

    private void visibleRadioButton_CheckedChanged(object sender, EventArgs e)
    {
      if (!refreshing && visibleRadioButton.Checked)
      {
        relationshipEditorBE.set_visibility(RelationshipVisibilityType.Visible);
      }
    }

    private void splittedRadioButton_CheckedChanged(object sender, EventArgs e)
    {
      if (!refreshing && splittedRadioButton.Checked)
      {
        relationshipEditorBE.set_visibility(RelationshipVisibilityType.Splitted);
      }
    }

    private void hiddenRadioButton_CheckedChanged(object sender, EventArgs e)
    {
      if (!refreshing && hiddenRadioButton.Checked)
      {
        relationshipEditorBE.set_visibility(RelationshipVisibilityType.Hidden);
      }
    }

    private void leftEdit_Click(object sender, EventArgs e)
    {
      relationshipEditorBE.open_editor_for_left_table();
    }

    private void rightEdit_Click(object sender, EventArgs e)
    {
      relationshipEditorBE.open_editor_for_right_table();
    }

    private void identifyingRelationshipCheckbox_CheckedChanged(object sender, EventArgs e)
    {
      relationshipEditorBE.set_is_identifying(identifyingRelationshipCheckbox.Checked);
    }
  }
}