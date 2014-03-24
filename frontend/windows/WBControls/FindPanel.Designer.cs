namespace MySQL.Utilities
{
  partial class FindPanel
  {
    /// <summary> 
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary> 
    /// Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
      if (disposing && (components != null))
      {
        components.Dispose();
      }
      base.Dispose(disposing);
    }

    #region Component Designer generated code

    /// <summary> 
    /// Required method for Designer support - do not modify 
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
      this.components = new System.ComponentModel.Container();
      this.replaceTextBox = new System.Windows.Forms.TextBox();
      this.searchTextBox = new System.Windows.Forms.TextBox();
      this.optionsButton = new System.Windows.Forms.Button();
      this.optionsMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
      this.stringMatchingToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.regularExpressionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
      this.ignoreCaseToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.matchWholeWordsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.wrapAroundToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripSeparator();
      this.recentSearchesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripMenuItem2 = new System.Windows.Forms.ToolStripSeparator();
      this.clearRecentSearchesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
      this.searchClearButton = new System.Windows.Forms.Button();
      this.modeComboBox = new System.Windows.Forms.ComboBox();
      this.replaceAllButton = new System.Windows.Forms.Button();
      this.replaceButton = new System.Windows.Forms.Button();
      this.replaceAndFindButton = new System.Windows.Forms.Button();
      this.navButtonNext = new System.Windows.Forms.Button();
      this.navButtonBack = new System.Windows.Forms.Button();
      this.layoutPanel = new System.Windows.Forms.TableLayoutPanel();
      this.doneButton = new System.Windows.Forms.Button();
      this.panel1 = new System.Windows.Forms.Panel();
      this.optionsMenuStrip.SuspendLayout();
      this.layoutPanel.SuspendLayout();
      this.panel1.SuspendLayout();
      this.SuspendLayout();
      // 
      // replaceTextBox
      // 
      this.replaceTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.replaceTextBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.replaceTextBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
      this.replaceTextBox.Location = new System.Drawing.Point(280, 27);
      this.replaceTextBox.Margin = new System.Windows.Forms.Padding(3, 0, 3, 1);
      this.replaceTextBox.Name = "replaceTextBox";
      this.replaceTextBox.Size = new System.Drawing.Size(352, 20);
      this.replaceTextBox.TabIndex = 1;
      this.replaceTextBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.textBox_KeyDown);
      // 
      // searchTextBox
      // 
      this.searchTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.searchTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
      this.searchTextBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
      this.searchTextBox.Location = new System.Drawing.Point(32, 3);
      this.searchTextBox.Margin = new System.Windows.Forms.Padding(0);
      this.searchTextBox.Multiline = true;
      this.searchTextBox.Name = "searchTextBox";
      this.searchTextBox.Size = new System.Drawing.Size(300, 15);
      this.searchTextBox.TabIndex = 0;
      this.searchTextBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.textBox_KeyDown);
      // 
      // optionsButton
      // 
      this.optionsButton.Dock = System.Windows.Forms.DockStyle.Left;
      this.optionsButton.FlatAppearance.BorderSize = 0;
      this.optionsButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
      this.optionsButton.Image = global::MySQL.Controls.Properties.Resources.search_dropdown;
      this.optionsButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.optionsButton.Location = new System.Drawing.Point(0, 0);
      this.optionsButton.Margin = new System.Windows.Forms.Padding(3, 0, 3, 0);
      this.optionsButton.Name = "optionsButton";
      this.optionsButton.Size = new System.Drawing.Size(32, 19);
      this.optionsButton.TabIndex = 4;
      this.optionsButton.TabStop = false;
      this.optionsButton.UseVisualStyleBackColor = true;
      this.optionsButton.Click += new System.EventHandler(this.optionsButton_Click);
      // 
      // optionsMenuStrip
      // 
      this.optionsMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.stringMatchingToolStripMenuItem,
            this.regularExpressionToolStripMenuItem,
            this.toolStripSeparator1,
            this.ignoreCaseToolStripMenuItem,
            this.matchWholeWordsToolStripMenuItem,
            this.wrapAroundToolStripMenuItem,
            this.toolStripMenuItem1,
            this.recentSearchesToolStripMenuItem,
            this.toolStripMenuItem2,
            this.clearRecentSearchesToolStripMenuItem});
      this.optionsMenuStrip.Name = "contextMenuStrip1";
      this.optionsMenuStrip.Size = new System.Drawing.Size(190, 176);
      this.optionsMenuStrip.ItemClicked += new System.Windows.Forms.ToolStripItemClickedEventHandler(this.optionsMenuStrip_ItemClicked);
      // 
      // stringMatchingToolStripMenuItem
      // 
      this.stringMatchingToolStripMenuItem.Checked = true;
      this.stringMatchingToolStripMenuItem.CheckOnClick = true;
      this.stringMatchingToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
      this.stringMatchingToolStripMenuItem.Name = "stringMatchingToolStripMenuItem";
      this.stringMatchingToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
      this.stringMatchingToolStripMenuItem.Text = "String Matching";
      // 
      // regularExpressionToolStripMenuItem
      // 
      this.regularExpressionToolStripMenuItem.CheckOnClick = true;
      this.regularExpressionToolStripMenuItem.Name = "regularExpressionToolStripMenuItem";
      this.regularExpressionToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
      this.regularExpressionToolStripMenuItem.Text = "Regular Expression";
      // 
      // toolStripSeparator1
      // 
      this.toolStripSeparator1.Name = "toolStripSeparator1";
      this.toolStripSeparator1.Size = new System.Drawing.Size(186, 6);
      // 
      // ignoreCaseToolStripMenuItem
      // 
      this.ignoreCaseToolStripMenuItem.Checked = true;
      this.ignoreCaseToolStripMenuItem.CheckOnClick = true;
      this.ignoreCaseToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
      this.ignoreCaseToolStripMenuItem.Name = "ignoreCaseToolStripMenuItem";
      this.ignoreCaseToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
      this.ignoreCaseToolStripMenuItem.Text = "Ignore Case";
      // 
      // matchWholeWordsToolStripMenuItem
      // 
      this.matchWholeWordsToolStripMenuItem.CheckOnClick = true;
      this.matchWholeWordsToolStripMenuItem.Name = "matchWholeWordsToolStripMenuItem";
      this.matchWholeWordsToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
      this.matchWholeWordsToolStripMenuItem.Text = "Match Whole Words";
      // 
      // wrapAroundToolStripMenuItem
      // 
      this.wrapAroundToolStripMenuItem.Checked = true;
      this.wrapAroundToolStripMenuItem.CheckOnClick = true;
      this.wrapAroundToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
      this.wrapAroundToolStripMenuItem.Name = "wrapAroundToolStripMenuItem";
      this.wrapAroundToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
      this.wrapAroundToolStripMenuItem.Text = "Wrap Around";
      // 
      // toolStripMenuItem1
      // 
      this.toolStripMenuItem1.Name = "toolStripMenuItem1";
      this.toolStripMenuItem1.Size = new System.Drawing.Size(186, 6);
      // 
      // recentSearchesToolStripMenuItem
      // 
      this.recentSearchesToolStripMenuItem.Enabled = false;
      this.recentSearchesToolStripMenuItem.Name = "recentSearchesToolStripMenuItem";
      this.recentSearchesToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
      this.recentSearchesToolStripMenuItem.Text = "Recent Searches";
      // 
      // toolStripMenuItem2
      // 
      this.toolStripMenuItem2.Name = "toolStripMenuItem2";
      this.toolStripMenuItem2.Size = new System.Drawing.Size(186, 6);
      // 
      // clearRecentSearchesToolStripMenuItem
      // 
      this.clearRecentSearchesToolStripMenuItem.Name = "clearRecentSearchesToolStripMenuItem";
      this.clearRecentSearchesToolStripMenuItem.Size = new System.Drawing.Size(189, 22);
      this.clearRecentSearchesToolStripMenuItem.Text = "Clear Recent Searches";
      // 
      // searchClearButton
      // 
      this.searchClearButton.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
      this.searchClearButton.BackColor = System.Drawing.Color.White;
      this.searchClearButton.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
      this.searchClearButton.Dock = System.Windows.Forms.DockStyle.Right;
      this.searchClearButton.FlatAppearance.BorderSize = 0;
      this.searchClearButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
      this.searchClearButton.Image = global::MySQL.Controls.Properties.Resources.search_clear;
      this.searchClearButton.Location = new System.Drawing.Point(330, 0);
      this.searchClearButton.Margin = new System.Windows.Forms.Padding(0);
      this.searchClearButton.Name = "searchClearButton";
      this.searchClearButton.Padding = new System.Windows.Forms.Padding(0, 0, 0, 1);
      this.searchClearButton.Size = new System.Drawing.Size(20, 19);
      this.searchClearButton.TabIndex = 3;
      this.searchClearButton.TabStop = false;
      this.toolTip1.SetToolTip(this.searchClearButton, "Clear search text");
      this.searchClearButton.UseVisualStyleBackColor = false;
      this.searchClearButton.Click += new System.EventHandler(this.searchClearButton_Click);
      // 
      // modeComboBox
      // 
      this.modeComboBox.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.layoutPanel.SetColumnSpan(this.modeComboBox, 2);
      this.modeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.modeComboBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
      this.modeComboBox.FormattingEnabled = true;
      this.modeComboBox.ItemHeight = 13;
      this.modeComboBox.Items.AddRange(new object[] {
            "Find",
            "Find & Replace"});
      this.modeComboBox.Location = new System.Drawing.Point(4, 2);
      this.modeComboBox.Margin = new System.Windows.Forms.Padding(4, 0, 3, 1);
      this.modeComboBox.MaxDropDownItems = 2;
      this.modeComboBox.Name = "modeComboBox";
      this.modeComboBox.Size = new System.Drawing.Size(160, 21);
      this.modeComboBox.TabIndex = 3;
      this.modeComboBox.SelectedIndexChanged += new System.EventHandler(this.modeComboBox_SelectedIndexChanged);
      // 
      // replaceAllButton
      // 
      this.replaceAllButton.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                  | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.replaceAllButton.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
      this.replaceAllButton.Location = new System.Drawing.Point(3, 26);
      this.replaceAllButton.Margin = new System.Windows.Forms.Padding(3, 1, 3, 0);
      this.replaceAllButton.Name = "replaceAllButton";
      this.replaceAllButton.Size = new System.Drawing.Size(80, 22);
      this.replaceAllButton.TabIndex = 6;
      this.replaceAllButton.Tag = "3";
      this.replaceAllButton.Text = "Replace All";
      this.replaceAllButton.UseVisualStyleBackColor = true;
      this.replaceAllButton.Click += new System.EventHandler(this.button_Click);
      // 
      // replaceButton
      // 
      this.replaceButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
      this.replaceButton.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
      this.replaceButton.Location = new System.Drawing.Point(89, 26);
      this.replaceButton.Margin = new System.Windows.Forms.Padding(3, 1, 3, 0);
      this.replaceButton.Name = "replaceButton";
      this.replaceButton.Size = new System.Drawing.Size(75, 22);
      this.replaceButton.TabIndex = 7;
      this.replaceButton.Tag = "4";
      this.replaceButton.Text = "Replace";
      this.replaceButton.UseVisualStyleBackColor = true;
      this.replaceButton.Click += new System.EventHandler(this.button_Click);
      // 
      // replaceAndFindButton
      // 
      this.replaceAndFindButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
      this.layoutPanel.SetColumnSpan(this.replaceAndFindButton, 2);
      this.replaceAndFindButton.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
      this.replaceAndFindButton.Location = new System.Drawing.Point(170, 26);
      this.replaceAndFindButton.Margin = new System.Windows.Forms.Padding(3, 1, 3, 0);
      this.replaceAndFindButton.Name = "replaceAndFindButton";
      this.replaceAndFindButton.Size = new System.Drawing.Size(104, 22);
      this.replaceAndFindButton.TabIndex = 8;
      this.replaceAndFindButton.Tag = "5";
      this.replaceAndFindButton.Text = "Replace && Find";
      this.replaceAndFindButton.UseVisualStyleBackColor = true;
      this.replaceAndFindButton.Click += new System.EventHandler(this.button_Click);
      // 
      // navButtonNext
      // 
      this.navButtonNext.Anchor = System.Windows.Forms.AnchorStyles.None;
      this.navButtonNext.Image = global::MySQL.Controls.Properties.Resources.Next;
      this.navButtonNext.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
      this.navButtonNext.Location = new System.Drawing.Point(252, 2);
      this.navButtonNext.Margin = new System.Windows.Forms.Padding(0, 0, 3, 0);
      this.navButtonNext.Name = "navButtonNext";
      this.navButtonNext.Size = new System.Drawing.Size(22, 22);
      this.navButtonNext.TabIndex = 5;
      this.navButtonNext.Tag = "2";
      this.navButtonNext.UseVisualStyleBackColor = true;
      this.navButtonNext.Click += new System.EventHandler(this.button_Click);
      // 
      // navButtonBack
      // 
      this.navButtonBack.Anchor = System.Windows.Forms.AnchorStyles.Right;
      this.navButtonBack.Image = global::MySQL.Controls.Properties.Resources.Back;
      this.navButtonBack.Location = new System.Drawing.Point(230, 2);
      this.navButtonBack.Margin = new System.Windows.Forms.Padding(0, 0, 0, 1);
      this.navButtonBack.Name = "navButtonBack";
      this.navButtonBack.Size = new System.Drawing.Size(22, 22);
      this.navButtonBack.TabIndex = 4;
      this.navButtonBack.Tag = "1";
      this.navButtonBack.UseVisualStyleBackColor = true;
      this.navButtonBack.Click += new System.EventHandler(this.button_Click);
      // 
      // layoutPanel
      // 
      this.layoutPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
      this.layoutPanel.ColumnCount = 6;
      this.layoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.layoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.layoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 85F));
      this.layoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 25F));
      this.layoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
      this.layoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.layoutPanel.Controls.Add(this.replaceAllButton, 0, 1);
      this.layoutPanel.Controls.Add(this.navButtonBack, 2, 0);
      this.layoutPanel.Controls.Add(this.replaceButton, 1, 1);
      this.layoutPanel.Controls.Add(this.navButtonNext, 3, 0);
      this.layoutPanel.Controls.Add(this.replaceAndFindButton, 2, 1);
      this.layoutPanel.Controls.Add(this.modeComboBox, 0, 0);
      this.layoutPanel.Controls.Add(this.replaceTextBox, 4, 1);
      this.layoutPanel.Controls.Add(this.doneButton, 5, 1);
      this.layoutPanel.Controls.Add(this.panel1, 4, 0);
      this.layoutPanel.Dock = System.Windows.Forms.DockStyle.Top;
      this.layoutPanel.Location = new System.Drawing.Point(0, 0);
      this.layoutPanel.Margin = new System.Windows.Forms.Padding(0);
      this.layoutPanel.Name = "layoutPanel";
      this.layoutPanel.Padding = new System.Windows.Forms.Padding(0, 2, 0, 2);
      this.layoutPanel.RowCount = 2;
      this.layoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.layoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.layoutPanel.Size = new System.Drawing.Size(686, 50);
      this.layoutPanel.TabIndex = 11;
      // 
      // doneButton
      // 
      this.doneButton.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.doneButton.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F);
      this.doneButton.Location = new System.Drawing.Point(638, 26);
      this.doneButton.Margin = new System.Windows.Forms.Padding(3, 0, 3, 0);
      this.doneButton.Name = "doneButton";
      this.doneButton.Size = new System.Drawing.Size(45, 22);
      this.doneButton.TabIndex = 2;
      this.doneButton.Text = "Done";
      this.doneButton.UseVisualStyleBackColor = true;
      this.doneButton.Click += new System.EventHandler(this.doneButton_Click);
      // 
      // panel1
      // 
      this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
      this.panel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
      this.panel1.BackColor = System.Drawing.SystemColors.Window;
      this.panel1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.panel1.Controls.Add(this.searchTextBox);
      this.panel1.Controls.Add(this.optionsButton);
      this.panel1.Controls.Add(this.searchClearButton);
      this.panel1.Location = new System.Drawing.Point(280, 2);
      this.panel1.Margin = new System.Windows.Forms.Padding(3, 0, 3, 1);
      this.panel1.Name = "panel1";
      this.panel1.Size = new System.Drawing.Size(352, 21);
      this.panel1.TabIndex = 0;
      this.panel1.TabStop = true;
      // 
      // FindPanel
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
      this.BackColor = System.Drawing.SystemColors.ControlLight;
      this.Controls.Add(this.layoutPanel);
      this.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.5F);
      this.Name = "FindPanel";
      this.Size = new System.Drawing.Size(686, 52);
      this.optionsMenuStrip.ResumeLayout(false);
      this.layoutPanel.ResumeLayout(false);
      this.layoutPanel.PerformLayout();
      this.panel1.ResumeLayout(false);
      this.panel1.PerformLayout();
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.TextBox replaceTextBox;
    private System.Windows.Forms.TextBox searchTextBox;
    private System.Windows.Forms.Button searchClearButton;
    private System.Windows.Forms.Button optionsButton;
    private System.Windows.Forms.ContextMenuStrip optionsMenuStrip;
    private System.Windows.Forms.ToolStripMenuItem ignoreCaseToolStripMenuItem;
    private System.Windows.Forms.ToolStripMenuItem matchWholeWordsToolStripMenuItem;
    private System.Windows.Forms.ToolStripMenuItem wrapAroundToolStripMenuItem;
    private System.Windows.Forms.ToolStripSeparator toolStripMenuItem1;
    private System.Windows.Forms.ToolStripMenuItem recentSearchesToolStripMenuItem;
    private System.Windows.Forms.ToolStripSeparator toolStripMenuItem2;
    private System.Windows.Forms.ToolStripMenuItem clearRecentSearchesToolStripMenuItem;
    private System.Windows.Forms.ToolTip toolTip1;
    private System.Windows.Forms.ComboBox modeComboBox;
    private System.Windows.Forms.Button replaceAllButton;
    private System.Windows.Forms.Button replaceButton;
    private System.Windows.Forms.Button replaceAndFindButton;
    private System.Windows.Forms.Button navButtonNext;
    private System.Windows.Forms.Button navButtonBack;
    private System.Windows.Forms.TableLayoutPanel layoutPanel;
    private System.Windows.Forms.ToolStripMenuItem stringMatchingToolStripMenuItem;
    private System.Windows.Forms.ToolStripMenuItem regularExpressionToolStripMenuItem;
    private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
    private System.Windows.Forms.Button doneButton;
    private System.Windows.Forms.Panel panel1;
  }
}
