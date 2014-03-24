namespace MySQL.GUI.Workbench.Plugins
{
  partial class DbMysqlSchemaEditor
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DbMysqlSchemaEditor));
      this.mainTabControl = new MySQL.Controls.FlatTabControl();
      this.tabPage1 = new System.Windows.Forms.TabPage();
      this.label3 = new System.Windows.Forms.Label();
      this.refactorButton = new System.Windows.Forms.Button();
      this.optComments = new System.Windows.Forms.TextBox();
      this.label7 = new System.Windows.Forms.Label();
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.label5 = new System.Windows.Forms.Label();
      this.label4 = new System.Windows.Forms.Label();
      this.label2 = new System.Windows.Forms.Label();
      this.optCollation = new System.Windows.Forms.ComboBox();
      this.nameTextBox = new System.Windows.Forms.TextBox();
      this.label1 = new System.Windows.Forms.Label();
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
      this.mainTabControl.CanReorderTabs = false;
      this.mainTabControl.ContentPadding = new System.Windows.Forms.Padding(0);
      this.mainTabControl.Controls.Add(this.tabPage1);
      this.mainTabControl.DefaultTabSwitch = true;
      this.mainTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainTabControl.HideWhenEmpty = false;
      this.mainTabControl.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.mainTabControl.Location = new System.Drawing.Point(3, 3);
      this.mainTabControl.Margin = new System.Windows.Forms.Padding(0);
      this.mainTabControl.MaxTabSize = 200;
      this.mainTabControl.Name = "mainTabControl";
      this.mainTabControl.RenderWithGlow = true;
      this.mainTabControl.SelectedIndex = 0;
      this.mainTabControl.ShowCloseButton = false;
      this.mainTabControl.ShowFocusState = true;
      this.mainTabControl.Size = new System.Drawing.Size(781, 220);
      this.mainTabControl.TabIndex = 0;
      this.mainTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      // 
      // tabPage1
      // 
      this.tabPage1.BackColor = System.Drawing.Color.White;
      this.tabPage1.Controls.Add(this.label3);
      this.tabPage1.Controls.Add(this.refactorButton);
      this.tabPage1.Controls.Add(this.optComments);
      this.tabPage1.Controls.Add(this.label7);
      this.tabPage1.Controls.Add(this.pictureBox1);
      this.tabPage1.Controls.Add(this.label5);
      this.tabPage1.Controls.Add(this.label4);
      this.tabPage1.Controls.Add(this.label2);
      this.tabPage1.Controls.Add(this.optCollation);
      this.tabPage1.Controls.Add(this.nameTextBox);
      this.tabPage1.Controls.Add(this.label1);
      this.tabPage1.Location = new System.Drawing.Point(0, 0);
      this.tabPage1.Name = "tabPage1";
      this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
      this.tabPage1.Size = new System.Drawing.Size(781, 199);
      this.tabPage1.TabIndex = 0;
      this.tabPage1.Text = "Schema";
      // 
      // label3
      // 
      this.label3.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.label3.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label3.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label3.Location = new System.Drawing.Point(352, 31);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(426, 22);
      this.label3.TabIndex = 23;
      this.label3.Text = "Refactor model, changing all references found in view, triggers, stored procedure" +
    "s and functions from the old schema name to the new one.";
      this.label3.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // refactorButton
      // 
      this.refactorButton.Location = new System.Drawing.Point(207, 30);
      this.refactorButton.Name = "refactorButton";
      this.refactorButton.Size = new System.Drawing.Size(130, 23);
      this.refactorButton.TabIndex = 22;
      this.refactorButton.Text = "Rename References";
      this.refactorButton.UseVisualStyleBackColor = true;
      this.refactorButton.Click += new System.EventHandler(this.refactorButton_Click);
      // 
      // optComments
      // 
      this.optComments.AcceptsReturn = true;
      this.optComments.AcceptsTab = true;
      this.optComments.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.optComments.Location = new System.Drawing.Point(133, 97);
      this.optComments.Multiline = true;
      this.optComments.Name = "optComments";
      this.optComments.ScrollBars = System.Windows.Forms.ScrollBars.Both;
      this.optComments.Size = new System.Drawing.Size(642, 96);
      this.optComments.TabIndex = 21;
      this.optComments.TextChanged += new System.EventHandler(this.optComments_TextChanged);
      // 
      // label7
      // 
      this.label7.Location = new System.Drawing.Point(61, 97);
      this.label7.Name = "label7";
      this.label7.Size = new System.Drawing.Size(66, 21);
      this.label7.TabIndex = 20;
      this.label7.Text = "Comments:";
      this.label7.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // pictureBox1
      // 
      this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
      this.pictureBox1.Location = new System.Drawing.Point(10, 11);
      this.pictureBox1.Name = "pictureBox1";
      this.pictureBox1.Size = new System.Drawing.Size(48, 48);
      this.pictureBox1.TabIndex = 19;
      this.pictureBox1.TabStop = false;
      // 
      // label5
      // 
      this.label5.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.label5.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label5.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label5.Location = new System.Drawing.Point(352, 67);
      this.label5.Name = "label5";
      this.label5.Size = new System.Drawing.Size(423, 22);
      this.label5.TabIndex = 18;
      this.label5.Text = "Specifies which charset/collations the schema\'s tables will use if they do not ha" +
    "ve an explicit setting. Common choices are Latin1 or UTF8.";
      this.label5.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label4
      // 
      this.label4.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.label4.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label4.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label4.Location = new System.Drawing.Point(352, 5);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(426, 22);
      this.label4.TabIndex = 17;
      this.label4.Text = "The name of the schema. It is recommended to use only alpha-numeric characters. S" +
    "paces should be avoided and be replaced by _";
      this.label4.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label2
      // 
      this.label2.Location = new System.Drawing.Point(61, 67);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(66, 21);
      this.label2.TabIndex = 16;
      this.label2.Text = "Collation:";
      this.label2.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // optCollation
      // 
      this.optCollation.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.optCollation.FormattingEnabled = true;
      this.optCollation.Location = new System.Drawing.Point(133, 67);
      this.optCollation.Name = "optCollation";
      this.optCollation.Size = new System.Drawing.Size(204, 21);
      this.optCollation.TabIndex = 15;
      this.optCollation.SelectedIndexChanged += new System.EventHandler(this.optCollation_SelectedIndexChanged);
      // 
      // nameTextBox
      // 
      this.nameTextBox.BackColor = System.Drawing.SystemColors.Window;
      this.nameTextBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.nameTextBox.Location = new System.Drawing.Point(133, 5);
      this.nameTextBox.Name = "nameTextBox";
      this.nameTextBox.Size = new System.Drawing.Size(204, 20);
      this.nameTextBox.TabIndex = 14;
      this.nameTextBox.TextChanged += new System.EventHandler(this.nameTextBox_TextChanged);
      // 
      // label1
      // 
      this.label1.Location = new System.Drawing.Point(61, 5);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(66, 20);
      this.label1.TabIndex = 13;
      this.label1.Text = "Name:";
      this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // DbMysqlSchemaEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.White;
      this.ClientSize = new System.Drawing.Size(787, 226);
      this.Controls.Add(this.mainTabControl);
      this.Name = "DbMysqlSchemaEditor";
      this.Padding = new System.Windows.Forms.Padding(3);
      this.TabText = "MySQL Schema Editor";
      this.Text = "DbMysqlSchemaEditor";
      this.Shown += new System.EventHandler(this.DbMysqlSchemaEditor_Shown);
      this.mainTabControl.ResumeLayout(false);
      this.tabPage1.ResumeLayout(false);
      this.tabPage1.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.ResumeLayout(false);

    }

    #endregion

    private MySQL.Controls.FlatTabControl mainTabControl;
    private System.Windows.Forms.TabPage tabPage1;
    private System.Windows.Forms.TextBox optComments;
    private System.Windows.Forms.Label label7;
    private System.Windows.Forms.PictureBox pictureBox1;
    private System.Windows.Forms.Label label5;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.ComboBox optCollation;
    private System.Windows.Forms.TextBox nameTextBox;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.Button refactorButton;
  }
}