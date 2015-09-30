namespace MySQL.GUI.Workbench.Plugins
{
  partial class DbMysqlViewEditor
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DbMysqlViewEditor));
      this.mainTabControl = new MySQL.Controls.FlatTabControl();
      this.tabPage1 = new System.Windows.Forms.TabPage();
      this.panel1 = new System.Windows.Forms.Panel();
      this.label3 = new System.Windows.Forms.Label();
      this.label2 = new System.Windows.Forms.Label();
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.label4 = new System.Windows.Forms.Label();
      this.nameTextBox = new System.Windows.Forms.TextBox();
      this.commentsTabpage = new System.Windows.Forms.TabPage();
      this.commentTextBox = new System.Windows.Forms.TextBox();
      this.pictureBox2 = new System.Windows.Forms.PictureBox();
      this.label7 = new System.Windows.Forms.Label();
      this.label1 = new System.Windows.Forms.Label();
      this.labelTooltip = new System.Windows.Forms.ToolTip(this.components);
      this.mainTabControl.SuspendLayout();
      this.tabPage1.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
      this.commentsTabpage.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).BeginInit();
      this.SuspendLayout();
      // 
      // mainTabControl
      // 
      this.mainTabControl.Alignment = System.Windows.Forms.TabAlignment.Bottom;
      this.mainTabControl.AuxControl = null;
      this.mainTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.mainTabControl.CanCloseLastTab = false;
      this.mainTabControl.CanReorderTabs = false;
      this.mainTabControl.ContentPadding = new System.Windows.Forms.Padding(0);
      this.mainTabControl.Controls.Add(this.tabPage1);
      this.mainTabControl.Controls.Add(this.commentsTabpage);
      this.mainTabControl.DefaultTabSwitch = true;
      this.mainTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainTabControl.HideWhenEmpty = false;
      this.mainTabControl.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.mainTabControl.Location = new System.Drawing.Point(0, 0);
      this.mainTabControl.Margin = new System.Windows.Forms.Padding(0);
      this.mainTabControl.MaxTabSize = 200;
      this.mainTabControl.Name = "mainTabControl";
      this.mainTabControl.RenderWithGlow = true;
      this.mainTabControl.SelectedIndex = 0;
      this.mainTabControl.ShowCloseButton = false;
      this.mainTabControl.ShowFocusState = true;
      this.mainTabControl.Size = new System.Drawing.Size(747, 228);
      this.mainTabControl.TabIndex = 0;
      this.mainTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      // 
      // tabPage1
      // 
      this.tabPage1.BackColor = System.Drawing.SystemColors.Control;
      this.tabPage1.Controls.Add(this.panel1);
      this.tabPage1.Controls.Add(this.label3);
      this.tabPage1.Controls.Add(this.label2);
      this.tabPage1.Controls.Add(this.pictureBox1);
      this.tabPage1.Controls.Add(this.label4);
      this.tabPage1.Controls.Add(this.nameTextBox);
      this.tabPage1.Location = new System.Drawing.Point(0, 0);
      this.tabPage1.Name = "tabPage1";
      this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
      this.tabPage1.Size = new System.Drawing.Size(747, 203);
      this.tabPage1.TabIndex = 0;
      this.tabPage1.Text = "View";
      // 
      // panel1
      // 
      this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.panel1.BackColor = System.Drawing.SystemColors.ActiveCaption;
      this.panel1.Location = new System.Drawing.Point(134, 38);
      this.panel1.Name = "panel1";
      this.panel1.Padding = new System.Windows.Forms.Padding(1);
      this.panel1.Size = new System.Drawing.Size(607, 159);
      this.panel1.TabIndex = 25;
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
      // label2
      // 
      this.label2.Location = new System.Drawing.Point(61, 33);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(66, 21);
      this.label2.TabIndex = 19;
      this.label2.Text = "DDL:";
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
      // label4
      // 
      this.label4.AutoEllipsis = true;
      this.label4.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label4.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label4.Location = new System.Drawing.Point(325, 5);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(275, 22);
      this.label4.TabIndex = 15;
      this.label4.Text = "The name of the view is parsed automatically from the DDL statement. The DDL is p" +
    "arsed automatically while you type.";
      this.label4.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // nameTextBox
      // 
      this.nameTextBox.Location = new System.Drawing.Point(134, 5);
      this.nameTextBox.Name = "nameTextBox";
      this.nameTextBox.ReadOnly = true;
      this.nameTextBox.Size = new System.Drawing.Size(185, 24);
      this.nameTextBox.TabIndex = 14;
      this.nameTextBox.TextChanged += new System.EventHandler(this.nameTextBox_TextChanged);
      // 
      // commentsTabpage
      // 
      this.commentsTabpage.BackColor = System.Drawing.SystemColors.Control;
      this.commentsTabpage.Controls.Add(this.commentTextBox);
      this.commentsTabpage.Controls.Add(this.pictureBox2);
      this.commentsTabpage.Controls.Add(this.label7);
      this.commentsTabpage.Location = new System.Drawing.Point(0, 0);
      this.commentsTabpage.Name = "commentsTabpage";
      this.commentsTabpage.Padding = new System.Windows.Forms.Padding(3);
      this.commentsTabpage.Size = new System.Drawing.Size(747, 203);
      this.commentsTabpage.TabIndex = 1;
      this.commentsTabpage.Text = "Comments";
      // 
      // commentTextBox
      // 
      this.commentTextBox.AcceptsReturn = true;
      this.commentTextBox.AcceptsTab = true;
      this.commentTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.commentTextBox.Location = new System.Drawing.Point(86, 27);
      this.commentTextBox.Multiline = true;
      this.commentTextBox.Name = "commentTextBox";
      this.commentTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
      this.commentTextBox.Size = new System.Drawing.Size(100, 26);
      this.commentTextBox.TabIndex = 19;
      this.commentTextBox.TextChanged += new System.EventHandler(this.commentTextBox_TextChanged);
      // 
      // pictureBox2
      // 
      this.pictureBox2.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox2.Image")));
      this.pictureBox2.Location = new System.Drawing.Point(11, 11);
      this.pictureBox2.Name = "pictureBox2";
      this.pictureBox2.Size = new System.Drawing.Size(48, 48);
      this.pictureBox2.TabIndex = 17;
      this.pictureBox2.TabStop = false;
      // 
      // label7
      // 
      this.label7.Location = new System.Drawing.Point(73, 10);
      this.label7.Name = "label7";
      this.label7.Size = new System.Drawing.Size(66, 21);
      this.label7.TabIndex = 20;
      this.label7.Text = "Comment:";
      this.label7.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label1
      // 
      this.label1.Location = new System.Drawing.Point(62, 5);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(66, 20);
      this.label1.TabIndex = 13;
      this.label1.Text = "Name:";
      this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // labelTooltip
      // 
      this.labelTooltip.IsBalloon = true;
      this.labelTooltip.ToolTipIcon = System.Windows.Forms.ToolTipIcon.Info;
      this.labelTooltip.ToolTipTitle = "Parser Information";
      // 
      // DbMysqlViewEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 17F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.White;
      this.ClientSize = new System.Drawing.Size(747, 228);
      this.Controls.Add(this.mainTabControl);
      this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.Name = "DbMysqlViewEditor";
      this.TabText = "DbMysqlViewEditor";
      this.Text = "DbMysqlViewEditor";
      this.Load += new System.EventHandler(this.DbMysqlViewEditor_Load);
      this.mainTabControl.ResumeLayout(false);
      this.tabPage1.ResumeLayout(false);
      this.tabPage1.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.commentsTabpage.ResumeLayout(false);
      this.commentsTabpage.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).EndInit();
      this.ResumeLayout(false);

    }

    #endregion

    private MySQL.Controls.FlatTabControl mainTabControl;
    private System.Windows.Forms.TabPage tabPage1;
    private System.Windows.Forms.PictureBox pictureBox1;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.TextBox nameTextBox;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.Panel panel1;
    private System.Windows.Forms.TabPage commentsTabpage;
    private System.Windows.Forms.PictureBox pictureBox2;
    private System.Windows.Forms.Label label7;
    private System.Windows.Forms.TextBox commentTextBox;
    private System.Windows.Forms.ToolTip labelTooltip;
  }
}