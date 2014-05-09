namespace MySQL.GUI.Workbench
{
  partial class ModelObjectDescriptionForm
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

    #region Windows Form Designer generated code

    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
      this.components = new System.ComponentModel.Container();
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ModelObjectDescriptionForm));
      this.topPanel = new System.Windows.Forms.Panel();
      this.objSelComboBox = new System.Windows.Forms.ComboBox();
      this.descriptionTextBox = new System.Windows.Forms.TextBox();
      this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
      this.headerPanel1 = new MySQL.Controls.HeaderPanel();
      this.panel1 = new System.Windows.Forms.Panel();
      this.topPanel.SuspendLayout();
      this.headerPanel1.SuspendLayout();
      this.panel1.SuspendLayout();
      this.SuspendLayout();
      // 
      // topPanel
      // 
      this.topPanel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(189)))), ((int)(((byte)(199)))), ((int)(((byte)(222)))));
      this.topPanel.Controls.Add(this.objSelComboBox);
      this.topPanel.Dock = System.Windows.Forms.DockStyle.Top;
      this.topPanel.Location = new System.Drawing.Point(0, 24);
      this.topPanel.Name = "topPanel";
      this.topPanel.Padding = new System.Windows.Forms.Padding(4);
      this.topPanel.Size = new System.Drawing.Size(184, 28);
      this.topPanel.TabIndex = 0;
      // 
      // objSelComboBox
      // 
      this.objSelComboBox.Dock = System.Windows.Forms.DockStyle.Top;
      this.objSelComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.objSelComboBox.FormattingEnabled = true;
      this.objSelComboBox.Location = new System.Drawing.Point(4, 4);
      this.objSelComboBox.Name = "objSelComboBox";
      this.objSelComboBox.Size = new System.Drawing.Size(176, 21);
      this.objSelComboBox.TabIndex = 1;
      this.objSelComboBox.SelectedIndexChanged += new System.EventHandler(this.objSelComboBox_SelectedIndexChanged);
      // 
      // descriptionTextBox
      // 
      this.descriptionTextBox.AcceptsReturn = true;
      this.descriptionTextBox.AcceptsTab = true;
      this.descriptionTextBox.BackColor = System.Drawing.SystemColors.Window;
      this.descriptionTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
      this.descriptionTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
      this.descriptionTextBox.Location = new System.Drawing.Point(4, 4);
      this.descriptionTextBox.Margin = new System.Windows.Forms.Padding(0);
      this.descriptionTextBox.Multiline = true;
      this.descriptionTextBox.Name = "descriptionTextBox";
      this.descriptionTextBox.Size = new System.Drawing.Size(176, 206);
      this.descriptionTextBox.TabIndex = 1;
      this.toolTip1.SetToolTip(this.descriptionTextBox, "Press Ctrl-Enter to apply changes");
      this.descriptionTextBox.TextChanged += new System.EventHandler(this.descriptionTextBox_TextChanged);
      this.descriptionTextBox.DoubleClick += new System.EventHandler(this.descriptionTextBox_DoubleClick);
      this.descriptionTextBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.descriptionTextBox_KeyDown);
      this.descriptionTextBox.Leave += new System.EventHandler(this.descriptionTextBox_Leave);
      // 
      // headerPanel1
      // 
      this.headerPanel1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.headerPanel1.Controls.Add(this.panel1);
      this.headerPanel1.Controls.Add(this.topPanel);
      this.headerPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.headerPanel1.ForeColor = System.Drawing.Color.White;
      this.headerPanel1.ForeColorFocused = System.Drawing.Color.White;
      this.headerPanel1.HeaderColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.headerPanel1.HeaderColorFocused = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.headerPanel1.HeaderPadding = new System.Windows.Forms.Padding(5, 0, 5, 0);
      this.headerPanel1.Location = new System.Drawing.Point(4, 3);
      this.headerPanel1.Name = "headerPanel1";
      this.headerPanel1.Padding = new System.Windows.Forms.Padding(0, 24, 0, 0);
      this.headerPanel1.Size = new System.Drawing.Size(184, 266);
      this.headerPanel1.TabIndex = 2;
      this.headerPanel1.Text = "Description Editor";
      // 
      // panel1
      // 
      this.panel1.BackColor = System.Drawing.Color.White;
      this.panel1.Controls.Add(this.descriptionTextBox);
      this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.panel1.Location = new System.Drawing.Point(0, 52);
      this.panel1.Name = "panel1";
      this.panel1.Padding = new System.Windows.Forms.Padding(4);
      this.panel1.Size = new System.Drawing.Size(184, 214);
      this.panel1.TabIndex = 2;
      // 
      // ModelObjectDescriptionForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.SystemColors.Window;
      this.ClientSize = new System.Drawing.Size(192, 272);
      this.Controls.Add(this.headerPanel1);
      this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
      this.Name = "ModelObjectDescriptionForm";
      this.Padding = new System.Windows.Forms.Padding(4, 3, 4, 3);
      this.TabText = "Description";
      this.Text = "Model Object Description";
      this.topPanel.ResumeLayout(false);
      this.headerPanel1.ResumeLayout(false);
      this.panel1.ResumeLayout(false);
      this.panel1.PerformLayout();
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Panel topPanel;
    private System.Windows.Forms.TextBox descriptionTextBox;
    private System.Windows.Forms.ToolTip toolTip1;
    private System.Windows.Forms.ComboBox objSelComboBox;
    private MySQL.Controls.HeaderPanel headerPanel1;
    private System.Windows.Forms.Panel panel1;
  }
}