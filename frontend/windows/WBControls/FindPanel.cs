/*
 * Copyright (c) 2012, 2019, Oracle and/or its affiliates. All rights reserved.
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

using System.Windows.Forms;
using System.Drawing;

namespace MySQL.Utilities
{
  public partial class FindPanel : UserControl
  {
    public FindPanel()
    {
      InitializeComponent();
    }

    #region Properties

    public IFindPanel Backend { get; set; }

    private bool showReplace = true;
    public bool ShowReplace
    {
      get { return showReplace; }
      set
      {
        if (showReplace != value)
        {
          showReplace = value;
          replaceAllButton.Visible = value;
          replaceButton.Visible = value;
          replaceTextBox.Visible = value;

          if (value)
          {
            modeComboBox.SelectedIndex = 1;
            Size = new Size(Width, layoutPanel.Height);
          }
          else
          {
            modeComboBox.SelectedIndex = 0;
            Size = new Size(Width, layoutPanel.Height / 2);
          }
        }
      }
    }

    public string SearchText { get { return searchTextBox.Text; }}
    public string ReplaceText { get { return replaceTextBox.Text; }}
    public bool IgnoreCase { get { return ignoreCaseToolStripMenuItem.Checked; }}
    public bool WholeWords { get { return matchWholeWordsToolStripMenuItem.Checked; }}
    public bool Wrap { get { return wrapAroundToolStripMenuItem.Checked; }}
    public bool RegularExpression { get { return regularExpressionToolStripMenuItem.Checked; }}

    #endregion

    public void FocusSearchField()
    {
      searchTextBox.Focus();
    }

    /// <summary>
    /// Checks if the current search text is already in the "recent searches" list and if not
    /// adds it.
    /// </summary>
    private void AddRecentSearchItem()
    {
      string text = searchTextBox.Text.Trim();
      int index = 8;
      while (!(optionsMenuStrip.Items[index] is ToolStripSeparator))
      {
        bool foundIt = (optionsMenuStrip.Items[index].Text == text);
        if (foundIt || index > 17)
        {
          // If the entry exists already or the list is longer than 9 entries remove it.
          // Below we add it to the top of the list (again).
          optionsMenuStrip.Items.RemoveAt(index);
          if (foundIt)
            break;
        }
        index++;
      }

      // With this new entry we have at most 10 recent search entries.
      ToolStripItem item = new ToolStripMenuItem(text);
      item.Click +=new System.EventHandler(searchMenuItem_Click);
      optionsMenuStrip.Items.Insert(8, item);
    }

    #region Event Handling

    private void searchMenuItem_Click(object sender, System.EventArgs e)
    {
      ToolStripItem item = sender as ToolStripItem;
      searchTextBox.Text = item.Text;
      searchTextBox.SelectAll();

      Backend.FindReplaceAction(FindPanelAction.FindNext);
      AddRecentSearchItem();
    }

    private void doneButton_Click(object sender, System.EventArgs e)
    {
      Backend.Close();
    }
    
    private void modeComboBox_SelectedIndexChanged(object sender, System.EventArgs e)
    {
      ShowReplace = (sender as ComboBox).SelectedIndex == 1;
    }

    private void optionsMenuStrip_ItemClicked(object sender, ToolStripItemClickedEventArgs e)
    {
      if (e.ClickedItem == stringMatchingToolStripMenuItem)
        regularExpressionToolStripMenuItem.Checked = false;
      if (e.ClickedItem == regularExpressionToolStripMenuItem)
        stringMatchingToolStripMenuItem.Checked = false;
      if (e.ClickedItem == clearRecentSearchesToolStripMenuItem)
      {
        // Remove all search items from the menu.
        while (optionsMenuStrip.Items.Count > 10)
          optionsMenuStrip.Items.RemoveAt(8);
      }

      // Search entries are handled in a separate event handler.
    }

    private void optionsButton_Click(object sender, System.EventArgs e)
    {
      if (optionsMenuStrip.Visible)
        optionsMenuStrip.Hide();
      else
      {
        Button button = sender as Button;
        optionsMenuStrip.Show(button, new Point(0, button.Height));
      }
    }

    private void searchClearButton_Click(object sender, System.EventArgs e)
    {
      searchTextBox.Clear();
    }

    private void textBox_KeyDown(object sender, KeyEventArgs e)
    {
      switch (e.KeyCode)
      {
        case Keys.Return:
          e.SuppressKeyPress = true;
          if (sender == searchTextBox)
          {
            searchTextBox.SelectAll();
            Backend.FindReplaceAction(FindPanelAction.FindNext);
            AddRecentSearchItem();
          }
          else
            AddRecentSearchItem();
          break;
        case Keys.Escape:
          Backend.Close();
          e.SuppressKeyPress = true;
          break;
      }
    }

    private void button_Click(object sender, System.EventArgs e)
    {
      int? result;
      switch ((sender as Control).Tag.ToString())
      {
        case "2": // Next button
          result = Backend.FindReplaceAction(FindPanelAction.FindNext);
          break;
        case "1": // Back button
          result = Backend.FindReplaceAction(FindPanelAction.FindPrevious);
          break;
        case "3": // Replace All button
          result = Backend.FindReplaceAction(FindPanelAction.ReplaceAll);
          break;
        case "4": // Replace button
          result = Backend.FindReplaceAction(FindPanelAction.FindAndReplace);
          break;
      }
      AddRecentSearchItem();
    }

    #endregion

  }

  /// <summary>
  /// Actions for this panel, same layout as in mforms.
  /// </summary>
  public enum FindPanelAction
  {
    FindNext,
    FindPrevious,
    FindAndReplace,
    ReplaceAll
  };


  public interface IFindPanel
  {
    void Close();
    int FindReplaceAction(FindPanelAction action);
  }
}
