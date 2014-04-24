namespace MySQL.GUI.Workbench.Plugins
{
  partial class DbMysqlRoutineGroupEditor
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DbMysqlRoutineGroupEditor));
      this.mainTabControl = new MySQL.Controls.FlatTabControl();
      this.tabPage1 = new System.Windows.Forms.TabPage();
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.splitContainer1 = new System.Windows.Forms.SplitContainer();
      this.label4 = new System.Windows.Forms.Label();
      this.nameTextBox = new System.Windows.Forms.TextBox();
      this.label1 = new System.Windows.Forms.Label();
      this.splitContainer2 = new System.Windows.Forms.SplitContainer();
      this.label2 = new System.Windows.Forms.Label();
      this.commentTextBox = new System.Windows.Forms.TextBox();
      this.panel1 = new System.Windows.Forms.Panel();
      this.routineNamesListBox = new System.Windows.Forms.ListBox();
      this.panel2 = new System.Windows.Forms.Panel();
      this.label3 = new System.Windows.Forms.Label();
      this.tabPage2 = new System.Windows.Forms.TabPage();
      this.mainTabControl.SuspendLayout();
      this.tabPage1.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
      this.splitContainer1.Panel1.SuspendLayout();
      this.splitContainer1.Panel2.SuspendLayout();
      this.splitContainer1.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer2)).BeginInit();
      this.splitContainer2.Panel1.SuspendLayout();
      this.splitContainer2.Panel2.SuspendLayout();
      this.splitContainer2.SuspendLayout();
      this.panel1.SuspendLayout();
      this.panel2.SuspendLayout();
      this.SuspendLayout();
      // 
      // mainTabControl
      // 
      this.mainTabControl.Alignment = System.Windows.Forms.TabAlignment.Bottom;
      this.mainTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.mainTabControl.CanCloseLastTab = false;
      this.mainTabControl.CanReorderTabs = false;
      this.mainTabControl.ContentPadding = new System.Windows.Forms.Padding(0);
      this.mainTabControl.Controls.Add(this.tabPage1);
      this.mainTabControl.Controls.Add(this.tabPage2);
      this.mainTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainTabControl.HideWhenEmpty = false;
      this.mainTabControl.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.mainTabControl.Location = new System.Drawing.Point(0, 0);
      this.mainTabControl.Margin = new System.Windows.Forms.Padding(0);
      this.mainTabControl.MaxTabSize = 200;
      this.mainTabControl.Name = "mainTabControl";
      this.mainTabControl.SelectedIndex = 0;
      this.mainTabControl.ShowCloseButton = false;
      this.mainTabControl.ShowFocusState = true;
      this.mainTabControl.Size = new System.Drawing.Size(750, 225);
      this.mainTabControl.TabIndex = 0;
      this.mainTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      // 
      // tabPage1
      // 
      this.tabPage1.BackColor = System.Drawing.SystemColors.ButtonFace;
      this.tabPage1.Controls.Add(this.pictureBox1);
      this.tabPage1.Controls.Add(this.splitContainer1);
      this.tabPage1.Location = new System.Drawing.Point(0, 0);
      this.tabPage1.Name = "tabPage1";
      this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
      this.tabPage1.Size = new System.Drawing.Size(750, 204);
      this.tabPage1.TabIndex = 2;
      this.tabPage1.Text = "Routine Group";
      // 
      // pictureBox1
      // 
      this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
      this.pictureBox1.Location = new System.Drawing.Point(11, 11);
      this.pictureBox1.Name = "pictureBox1";
      this.pictureBox1.Size = new System.Drawing.Size(48, 48);
      this.pictureBox1.TabIndex = 24;
      this.pictureBox1.TabStop = false;
      // 
      // splitContainer1
      // 
      this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
      this.splitContainer1.IsSplitterFixed = true;
      this.splitContainer1.Location = new System.Drawing.Point(3, 3);
      this.splitContainer1.Name = "splitContainer1";
      this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
      // 
      // splitContainer1.Panel1
      // 
      this.splitContainer1.Panel1.Controls.Add(this.label4);
      this.splitContainer1.Panel1.Controls.Add(this.nameTextBox);
      this.splitContainer1.Panel1.Controls.Add(this.label1);
      // 
      // splitContainer1.Panel2
      // 
      this.splitContainer1.Panel2.Controls.Add(this.splitContainer2);
      this.splitContainer1.Size = new System.Drawing.Size(744, 198);
      this.splitContainer1.SplitterDistance = 28;
      this.splitContainer1.SplitterWidth = 1;
      this.splitContainer1.TabIndex = 0;
      // 
      // label4
      // 
      this.label4.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label4.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label4.Location = new System.Drawing.Point(351, 2);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(331, 22);
      this.label4.TabIndex = 23;
      this.label4.Text = "The name of the routine group. It is recommended to use only alpha-numeric charac" +
    "ters. Spaces should be avoided and be replaced by _";
      this.label4.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // nameTextBox
      // 
      this.nameTextBox.Location = new System.Drawing.Point(131, 2);
      this.nameTextBox.Name = "nameTextBox";
      this.nameTextBox.Size = new System.Drawing.Size(204, 21);
      this.nameTextBox.TabIndex = 22;
      this.nameTextBox.TextChanged += new System.EventHandler(this.nameTextBox_TextChanged);
      // 
      // label1
      // 
      this.label1.Location = new System.Drawing.Point(59, 2);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(66, 20);
      this.label1.TabIndex = 21;
      this.label1.Text = "Name:";
      this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // splitContainer2
      // 
      this.splitContainer2.Dock = System.Windows.Forms.DockStyle.Fill;
      this.splitContainer2.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
      this.splitContainer2.IsSplitterFixed = true;
      this.splitContainer2.Location = new System.Drawing.Point(0, 0);
      this.splitContainer2.Name = "splitContainer2";
      // 
      // splitContainer2.Panel1
      // 
      this.splitContainer2.Panel1.Controls.Add(this.label2);
      // 
      // splitContainer2.Panel2
      // 
      this.splitContainer2.Panel2.Controls.Add(this.commentTextBox);
      this.splitContainer2.Panel2.Controls.Add(this.panel1);
      this.splitContainer2.Size = new System.Drawing.Size(744, 169);
      this.splitContainer2.SplitterDistance = 127;
      this.splitContainer2.TabIndex = 0;
      // 
      // label2
      // 
      this.label2.Location = new System.Drawing.Point(59, 0);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(66, 20);
      this.label2.TabIndex = 22;
      this.label2.Text = "Routines:";
      this.label2.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // commentTextBox
      // 
      this.commentTextBox.AcceptsReturn = true;
      this.commentTextBox.AcceptsTab = true;
      this.commentTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
      this.commentTextBox.Location = new System.Drawing.Point(294, 0);
      this.commentTextBox.Multiline = true;
      this.commentTextBox.Name = "commentTextBox";
      this.commentTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
      this.commentTextBox.Size = new System.Drawing.Size(319, 169);
      this.commentTextBox.TabIndex = 22;
      this.commentTextBox.TextChanged += new System.EventHandler(this.commentTextBox_TextChanged);
      // 
      // panel1
      // 
      this.panel1.Controls.Add(this.routineNamesListBox);
      this.panel1.Controls.Add(this.panel2);
      this.panel1.Dock = System.Windows.Forms.DockStyle.Left;
      this.panel1.Location = new System.Drawing.Point(0, 0);
      this.panel1.Name = "panel1";
      this.panel1.Size = new System.Drawing.Size(294, 169);
      this.panel1.TabIndex = 23;
      // 
      // routineNamesListBox
      // 
      this.routineNamesListBox.Dock = System.Windows.Forms.DockStyle.Fill;
      this.routineNamesListBox.FormattingEnabled = true;
      this.routineNamesListBox.IntegralHeight = false;
      this.routineNamesListBox.Location = new System.Drawing.Point(0, 0);
      this.routineNamesListBox.Name = "routineNamesListBox";
      this.routineNamesListBox.Size = new System.Drawing.Size(204, 169);
      this.routineNamesListBox.TabIndex = 1;
      this.routineNamesListBox.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.routineNamesListBox_MouseDoubleClick);
      // 
      // panel2
      // 
      this.panel2.Controls.Add(this.label3);
      this.panel2.Dock = System.Windows.Forms.DockStyle.Right;
      this.panel2.Location = new System.Drawing.Point(204, 0);
      this.panel2.Name = "panel2";
      this.panel2.Size = new System.Drawing.Size(90, 169);
      this.panel2.TabIndex = 0;
      // 
      // label3
      // 
      this.label3.Location = new System.Drawing.Point(21, 0);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(66, 20);
      this.label3.TabIndex = 23;
      this.label3.Text = "Comment:";
      this.label3.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // tabPage2
      // 
      this.tabPage2.BackColor = System.Drawing.SystemColors.ButtonFace;
      this.tabPage2.Location = new System.Drawing.Point(0, 0);
      this.tabPage2.Name = "tabPage2";
      this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
      this.tabPage2.Size = new System.Drawing.Size(750, 204);
      this.tabPage2.TabIndex = 1;
      this.tabPage2.Text = "Routines";
      // 
      // DbMysqlRoutineGroupEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.White;
      this.ClientSize = new System.Drawing.Size(750, 225);
      this.Controls.Add(this.mainTabControl);
      this.Font = new System.Drawing.Font("Tahoma", 8.25F);
      this.Name = "DbMysqlRoutineGroupEditor";
      this.TabText = "DbMysqlRoutineGroupEditor";
      this.Text = "DbMysqlRoutineGroupEditor";
      this.Load += new System.EventHandler(this.DbMysqlRoutineGroupEditor_Load);
      this.mainTabControl.ResumeLayout(false);
      this.tabPage1.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.splitContainer1.Panel1.ResumeLayout(false);
      this.splitContainer1.Panel1.PerformLayout();
      this.splitContainer1.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
      this.splitContainer1.ResumeLayout(false);
      this.splitContainer2.Panel1.ResumeLayout(false);
      this.splitContainer2.Panel2.ResumeLayout(false);
      this.splitContainer2.Panel2.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer2)).EndInit();
      this.splitContainer2.ResumeLayout(false);
      this.panel1.ResumeLayout(false);
      this.panel2.ResumeLayout(false);
      this.ResumeLayout(false);

    }

    #endregion

    private MySQL.Controls.FlatTabControl mainTabControl;
    private System.Windows.Forms.TabPage tabPage2;
    private System.Windows.Forms.TabPage tabPage1;
    private System.Windows.Forms.PictureBox pictureBox1;
    private System.Windows.Forms.SplitContainer splitContainer1;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.TextBox nameTextBox;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.SplitContainer splitContainer2;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.TextBox commentTextBox;
    private System.Windows.Forms.Panel panel1;
    private System.Windows.Forms.Panel panel2;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.ListBox routineNamesListBox;
  }
}