namespace MySQL.GUI.Workbench.Plugins
{
  partial class DbMysqlRoutineEditor
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DbMysqlRoutineEditor));
      this.mainTabControl = new MySQL.Controls.FlatTabControl();
      this.tabPage1 = new System.Windows.Forms.TabPage();
      this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
      this.pictureBox2 = new System.Windows.Forms.PictureBox();
      this.label3 = new System.Windows.Forms.Label();
      this.label5 = new System.Windows.Forms.Label();
      this.nameTextBox = new System.Windows.Forms.TextBox();
      this.panel1 = new System.Windows.Forms.Panel();
      this.label4 = new System.Windows.Forms.Label();
      this.panel2 = new System.Windows.Forms.Panel();
      this.commentTabPage = new System.Windows.Forms.TabPage();
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.commentTextBox = new System.Windows.Forms.TextBox();
      this.label1 = new System.Windows.Forms.Label();
      this.labelTooltip = new System.Windows.Forms.ToolTip(this.components);
      this.mainTabControl.SuspendLayout();
      this.tabPage1.SuspendLayout();
      this.tableLayoutPanel1.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).BeginInit();
      this.panel1.SuspendLayout();
      this.commentTabPage.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
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
      this.mainTabControl.Controls.Add(this.commentTabPage);
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
      this.mainTabControl.Size = new System.Drawing.Size(751, 225);
      this.mainTabControl.TabIndex = 0;
      this.mainTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      // 
      // tabPage1
      // 
      this.tabPage1.BackColor = System.Drawing.Color.White;
      this.tabPage1.Controls.Add(this.tableLayoutPanel1);
      this.tabPage1.Location = new System.Drawing.Point(0, 0);
      this.tabPage1.Name = "tabPage1";
      this.tabPage1.Padding = new System.Windows.Forms.Padding(0, 0, 5, 5);
      this.tabPage1.Size = new System.Drawing.Size(751, 204);
      this.tabPage1.TabIndex = 1;
      this.tabPage1.Text = "Routine";
      // 
      // tableLayoutPanel1
      // 
      this.tableLayoutPanel1.ColumnCount = 4;
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel1.Controls.Add(this.pictureBox2, 0, 0);
      this.tableLayoutPanel1.Controls.Add(this.label3, 1, 0);
      this.tableLayoutPanel1.Controls.Add(this.label5, 1, 1);
      this.tableLayoutPanel1.Controls.Add(this.nameTextBox, 2, 0);
      this.tableLayoutPanel1.Controls.Add(this.panel1, 3, 0);
      this.tableLayoutPanel1.Controls.Add(this.panel2, 2, 1);
      this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
      this.tableLayoutPanel1.Name = "tableLayoutPanel1";
      this.tableLayoutPanel1.RowCount = 2;
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
      this.tableLayoutPanel1.Size = new System.Drawing.Size(746, 199);
      this.tableLayoutPanel1.TabIndex = 0;
      // 
      // pictureBox2
      // 
      this.pictureBox2.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox2.Image")));
      this.pictureBox2.Location = new System.Drawing.Point(11, 11);
      this.pictureBox2.Margin = new System.Windows.Forms.Padding(11);
      this.pictureBox2.Name = "pictureBox2";
      this.tableLayoutPanel1.SetRowSpan(this.pictureBox2, 2);
      this.pictureBox2.Size = new System.Drawing.Size(48, 48);
      this.pictureBox2.TabIndex = 25;
      this.pictureBox2.TabStop = false;
      // 
      // label3
      // 
      this.label3.Location = new System.Drawing.Point(73, 6);
      this.label3.Margin = new System.Windows.Forms.Padding(3, 6, 3, 3);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(66, 20);
      this.label3.TabIndex = 26;
      this.label3.Text = "Name:";
      this.label3.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label5
      // 
      this.label5.Location = new System.Drawing.Point(73, 33);
      this.label5.Name = "label5";
      this.label5.Size = new System.Drawing.Size(66, 20);
      this.label5.TabIndex = 27;
      this.label5.Text = "DDL:";
      this.label5.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // nameTextBox
      // 
      this.nameTextBox.Dock = System.Windows.Forms.DockStyle.Top;
      this.nameTextBox.Location = new System.Drawing.Point(147, 6);
      this.nameTextBox.Margin = new System.Windows.Forms.Padding(5, 6, 6, 6);
      this.nameTextBox.Name = "nameTextBox";
      this.nameTextBox.ReadOnly = true;
      this.nameTextBox.Size = new System.Drawing.Size(208, 21);
      this.nameTextBox.TabIndex = 28;
      // 
      // panel1
      // 
      this.panel1.Controls.Add(this.label4);
      this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.panel1.Location = new System.Drawing.Point(361, 0);
      this.panel1.Margin = new System.Windows.Forms.Padding(0);
      this.panel1.Name = "panel1";
      this.panel1.Size = new System.Drawing.Size(385, 33);
      this.panel1.TabIndex = 33;
      // 
      // label4
      // 
      this.label4.Dock = System.Windows.Forms.DockStyle.Left;
      this.label4.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label4.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label4.Location = new System.Drawing.Point(0, 0);
      this.label4.Margin = new System.Windows.Forms.Padding(6);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(266, 33);
      this.label4.TabIndex = 32;
      this.label4.Text = "The name of the routine is parsed automatically from the DDL statement. The DDL i" +
    "s parsed automatically while you type.";
      this.label4.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // panel2
      // 
      this.panel2.BackColor = System.Drawing.SystemColors.ActiveCaption;
      this.tableLayoutPanel1.SetColumnSpan(this.panel2, 2);
      this.panel2.Dock = System.Windows.Forms.DockStyle.Fill;
      this.panel2.Location = new System.Drawing.Point(145, 36);
      this.panel2.Name = "panel2";
      this.panel2.Padding = new System.Windows.Forms.Padding(1);
      this.panel2.Size = new System.Drawing.Size(598, 160);
      this.panel2.TabIndex = 34;
      // 
      // commentTabPage
      // 
      this.commentTabPage.BackColor = System.Drawing.Color.White;
      this.commentTabPage.Controls.Add(this.pictureBox1);
      this.commentTabPage.Controls.Add(this.commentTextBox);
      this.commentTabPage.Controls.Add(this.label1);
      this.commentTabPage.Location = new System.Drawing.Point(0, 0);
      this.commentTabPage.Name = "commentTabPage";
      this.commentTabPage.Size = new System.Drawing.Size(751, 204);
      this.commentTabPage.TabIndex = 2;
      this.commentTabPage.Text = "Comments";
      // 
      // pictureBox1
      // 
      this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
      this.pictureBox1.Location = new System.Drawing.Point(11, 11);
      this.pictureBox1.Margin = new System.Windows.Forms.Padding(11);
      this.pictureBox1.Name = "pictureBox1";
      this.pictureBox1.Size = new System.Drawing.Size(48, 48);
      this.pictureBox1.TabIndex = 26;
      this.pictureBox1.TabStop = false;
      // 
      // commentTextBox
      // 
      this.commentTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.commentTextBox.Location = new System.Drawing.Point(86, 27);
      this.commentTextBox.Multiline = true;
      this.commentTextBox.Name = "commentTextBox";
      this.commentTextBox.Size = new System.Drawing.Size(653, 164);
      this.commentTextBox.TabIndex = 1;
      this.commentTextBox.Leave += new System.EventHandler(this.commentTextBox_Leave);
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(83, 11);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(56, 13);
      this.label1.TabIndex = 0;
      this.label1.Text = "Comment:";
      // 
      // labelTooltip
      // 
      this.labelTooltip.IsBalloon = true;
      this.labelTooltip.ToolTipIcon = System.Windows.Forms.ToolTipIcon.Info;
      this.labelTooltip.ToolTipTitle = "Parser Information";
      // 
      // DbMysqlRoutineEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.White;
      this.ClientSize = new System.Drawing.Size(751, 225);
      this.Controls.Add(this.mainTabControl);
      this.Font = new System.Drawing.Font("Tahoma", 8.25F);
      this.Name = "DbMysqlRoutineEditor";
      this.TabText = "Routine Editor";
      this.Text = "DbMysqlRoutineEditor";
      this.Load += new System.EventHandler(this.DbMysqlRoutineEditor_Load);
      this.mainTabControl.ResumeLayout(false);
      this.tabPage1.ResumeLayout(false);
      this.tableLayoutPanel1.ResumeLayout(false);
      this.tableLayoutPanel1.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).EndInit();
      this.panel1.ResumeLayout(false);
      this.commentTabPage.ResumeLayout(false);
      this.commentTabPage.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.ResumeLayout(false);

    }

    #endregion

    private MySQL.Controls.FlatTabControl mainTabControl;
    private System.Windows.Forms.TabPage tabPage1;
    private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
    private System.Windows.Forms.PictureBox pictureBox2;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.Label label5;
    private System.Windows.Forms.TextBox nameTextBox;
    private System.Windows.Forms.Panel panel1;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.Panel panel2;
    private System.Windows.Forms.ToolTip labelTooltip;
    private System.Windows.Forms.TabPage commentTabPage;
    private System.Windows.Forms.TextBox commentTextBox;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.PictureBox pictureBox1;
  }
}