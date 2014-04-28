namespace MySQL.GUI.Workbench.Plugins
{
  partial class NoteEditor
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(NoteEditor));
      this.mainTabControl = new MySQL.Controls.FlatTabControl();
      this.tabPage1 = new System.Windows.Forms.TabPage();
      this.comboBox1 = new System.Windows.Forms.ComboBox();
      this.label1 = new System.Windows.Forms.Label();
      this.label3 = new System.Windows.Forms.Label();
      this.sqlTextBox = new System.Windows.Forms.TextBox();
      this.label2 = new System.Windows.Forms.Label();
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.nameTextBox = new System.Windows.Forms.TextBox();
      this.mainTabControl.SuspendLayout();
      this.tabPage1.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
      this.SuspendLayout();
      // 
      // mainTabControl
      // 
      this.mainTabControl.Alignment = System.Windows.Forms.TabAlignment.Bottom;
      this.mainTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.mainTabControl.CanCloseLastTab = false;
      this.mainTabControl.ContentPadding = new System.Windows.Forms.Padding(0);
      this.mainTabControl.Controls.Add(this.tabPage1);
      this.mainTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainTabControl.HideWhenEmpty = false;
      this.mainTabControl.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.mainTabControl.Location = new System.Drawing.Point(3, 3);
      this.mainTabControl.Margin = new System.Windows.Forms.Padding(0);
      this.mainTabControl.MaxTabSize = 200;
      this.mainTabControl.Name = "mainTabControl";
      this.mainTabControl.SelectedIndex = 0;
      this.mainTabControl.ShowCloseButton = true;
      this.mainTabControl.ShowFocusState = true;
      this.mainTabControl.Size = new System.Drawing.Size(741, 233);
      this.mainTabControl.TabIndex = 1;
      this.mainTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      // 
      // tabPage1
      // 
      this.tabPage1.BackColor = System.Drawing.SystemColors.Control;
      this.tabPage1.Controls.Add(this.comboBox1);
      this.tabPage1.Controls.Add(this.label1);
      this.tabPage1.Controls.Add(this.label3);
      this.tabPage1.Controls.Add(this.sqlTextBox);
      this.tabPage1.Controls.Add(this.label2);
      this.tabPage1.Controls.Add(this.pictureBox1);
      this.tabPage1.Controls.Add(this.nameTextBox);
      this.tabPage1.Location = new System.Drawing.Point(0, 0);
      this.tabPage1.Name = "tabPage1";
      this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
      this.tabPage1.Size = new System.Drawing.Size(741, 212);
      this.tabPage1.TabIndex = 0;
      this.tabPage1.Text = "Note";
      // 
      // comboBox1
      // 
      this.comboBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.comboBox1.FormattingEnabled = true;
      this.comboBox1.Location = new System.Drawing.Point(687, 5);
      this.comboBox1.Name = "comboBox1";
      this.comboBox1.Size = new System.Drawing.Size(48, 21);
      this.comboBox1.TabIndex = 23;
      this.comboBox1.Visible = false;
      // 
      // label1
      // 
      this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
      this.label1.Location = new System.Drawing.Point(617, 5);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(66, 21);
      this.label1.TabIndex = 22;
      this.label1.Text = "Color:";
      this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      this.label1.Visible = false;
      // 
      // label3
      // 
      this.label3.Location = new System.Drawing.Point(62, 5);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(66, 21);
      this.label3.TabIndex = 21;
      this.label3.Text = "Name:";
      this.label3.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // sqlTextBox
      // 
      this.sqlTextBox.AcceptsReturn = true;
      this.sqlTextBox.AcceptsTab = true;
      this.sqlTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                  | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.sqlTextBox.Font = new System.Drawing.Font("Bitstream Vera Sans Mono", 9F);
      this.sqlTextBox.Location = new System.Drawing.Point(134, 32);
      this.sqlTextBox.Multiline = true;
      this.sqlTextBox.Name = "sqlTextBox";
      this.sqlTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
      this.sqlTextBox.Size = new System.Drawing.Size(601, 177);
      this.sqlTextBox.TabIndex = 20;
      this.sqlTextBox.TextChanged += new System.EventHandler(this.sqlTextBox_TextChanged);
      // 
      // label2
      // 
      this.label2.Location = new System.Drawing.Point(62, 32);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(66, 21);
      this.label2.TabIndex = 19;
      this.label2.Text = "Text:";
      this.label2.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // pictureBox1
      // 
      this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
      this.pictureBox1.Location = new System.Drawing.Point(11, 11);
      this.pictureBox1.Name = "pictureBox1";
      this.pictureBox1.Size = new System.Drawing.Size(48, 48);
      this.pictureBox1.TabIndex = 16;
      this.pictureBox1.TabStop = false;
      // 
      // nameTextBox
      // 
      this.nameTextBox.Location = new System.Drawing.Point(134, 5);
      this.nameTextBox.MaxLength = 40;
      this.nameTextBox.Name = "nameTextBox";
      this.nameTextBox.Size = new System.Drawing.Size(204, 20);
      this.nameTextBox.TabIndex = 14;
      this.nameTextBox.TextChanged += new System.EventHandler(this.nameTextBox_TextChanged);
      // 
      // NoteEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.White;
      this.ClientSize = new System.Drawing.Size(747, 239);
      this.Controls.Add(this.mainTabControl);
      this.Name = "NoteEditor";
      this.Padding = new System.Windows.Forms.Padding(3);
      this.TabText = "Note Editor";
      this.Text = "Note Editor";
      this.mainTabControl.ResumeLayout(false);
      this.tabPage1.ResumeLayout(false);
      this.tabPage1.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.ResumeLayout(false);

    }

    #endregion

    private MySQL.Controls.FlatTabControl mainTabControl;
    private System.Windows.Forms.TabPage tabPage1;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.TextBox sqlTextBox;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.PictureBox pictureBox1;
    private System.Windows.Forms.TextBox nameTextBox;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.ComboBox comboBox1;
  }
}

