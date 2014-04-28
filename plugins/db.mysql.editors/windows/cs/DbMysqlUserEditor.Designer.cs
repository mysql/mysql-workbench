namespace MySQL.GUI.Workbench.Plugins
{
  partial class DbMysqlUserEditor
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DbMysqlUserEditor));
      this.flatTabControl1 = new MySQL.Controls.FlatTabControl();
      this.tabPage1 = new System.Windows.Forms.TabPage();
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.splitContainer1 = new System.Windows.Forms.SplitContainer();
      this.label3 = new System.Windows.Forms.Label();
      this.passwordTextBox = new System.Windows.Forms.TextBox();
      this.label5 = new System.Windows.Forms.Label();
      this.label4 = new System.Windows.Forms.Label();
      this.nameTextBox = new System.Windows.Forms.TextBox();
      this.label1 = new System.Windows.Forms.Label();
      this.splitContainer2 = new System.Windows.Forms.SplitContainer();
      this.label2 = new System.Windows.Forms.Label();
      this.splitContainer3 = new System.Windows.Forms.SplitContainer();
      this.assignedRoleList = new System.Windows.Forms.ListBox();
      this.roleTreeView = new Aga.Controls.Tree.TreeViewAdv();
      this.roleTreeNameColumn = new Aga.Controls.Tree.TreeColumn();
      this.roleTreeNodeText = new MySQL.Utilities.AdvNodeTextBox();
      this.panel1 = new System.Windows.Forms.Panel();
      this.removeRoleButton = new System.Windows.Forms.Button();
      this.addRoleButton = new System.Windows.Forms.Button();
      this.tabPage2 = new System.Windows.Forms.TabPage();
      this.commentTextBox = new System.Windows.Forms.TextBox();
      this.flatTabControl1.SuspendLayout();
      this.tabPage1.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
      this.splitContainer1.Panel1.SuspendLayout();
      this.splitContainer1.Panel2.SuspendLayout();
      this.splitContainer1.SuspendLayout();
      this.splitContainer2.Panel1.SuspendLayout();
      this.splitContainer2.Panel2.SuspendLayout();
      this.splitContainer2.SuspendLayout();
      this.splitContainer3.Panel1.SuspendLayout();
      this.splitContainer3.Panel2.SuspendLayout();
      this.splitContainer3.SuspendLayout();
      this.panel1.SuspendLayout();
      this.tabPage2.SuspendLayout();
      this.SuspendLayout();
      // 
      // flatTabControl1
      // 
      this.flatTabControl1.Alignment = System.Windows.Forms.TabAlignment.Bottom;
      this.flatTabControl1.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.flatTabControl1.CanCloseLastTab = false;
      this.flatTabControl1.ContentPadding = new System.Windows.Forms.Padding(0, 0, 0, 0);
      this.flatTabControl1.Controls.Add(this.tabPage1);
      this.flatTabControl1.Controls.Add(this.tabPage2);
      this.flatTabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.flatTabControl1.HideWhenEmpty = false;
      this.flatTabControl1.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.flatTabControl1.Location = new System.Drawing.Point(0, 0);
      this.flatTabControl1.Margin = new System.Windows.Forms.Padding(0);
      this.flatTabControl1.Name = "flatTabControl1";
      this.flatTabControl1.SelectedIndex = 0;
      this.flatTabControl1.ShowCloseButton = false;
      this.flatTabControl1.ShowFocusState = true;
      this.flatTabControl1.Size = new System.Drawing.Size(791, 227);
      this.flatTabControl1.TabIndex = 0;
      this.flatTabControl1.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      // 
      // tabPage1
      // 
      this.tabPage1.BackColor = System.Drawing.SystemColors.ButtonFace;
      this.tabPage1.Controls.Add(this.pictureBox1);
      this.tabPage1.Controls.Add(this.splitContainer1);
      this.tabPage1.Location = new System.Drawing.Point(0, 0);
      this.tabPage1.Name = "tabPage1";
      this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
      this.tabPage1.Size = new System.Drawing.Size(791, 206);
      this.tabPage1.TabIndex = 0;
      this.tabPage1.Text = "User";
      // 
      // pictureBox1
      // 
      this.pictureBox1.BackColor = System.Drawing.Color.Transparent;
      this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
      this.pictureBox1.Location = new System.Drawing.Point(11, 11);
      this.pictureBox1.Name = "pictureBox1";
      this.pictureBox1.Size = new System.Drawing.Size(48, 48);
      this.pictureBox1.TabIndex = 25;
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
      this.splitContainer1.Panel1.Controls.Add(this.label3);
      this.splitContainer1.Panel1.Controls.Add(this.passwordTextBox);
      this.splitContainer1.Panel1.Controls.Add(this.label5);
      this.splitContainer1.Panel1.Controls.Add(this.label4);
      this.splitContainer1.Panel1.Controls.Add(this.nameTextBox);
      this.splitContainer1.Panel1.Controls.Add(this.label1);
      // 
      // splitContainer1.Panel2
      // 
      this.splitContainer1.Panel2.Controls.Add(this.splitContainer2);
      this.splitContainer1.Size = new System.Drawing.Size(785, 200);
      this.splitContainer1.SplitterDistance = 56;
      this.splitContainer1.SplitterWidth = 1;
      this.splitContainer1.TabIndex = 1;
      // 
      // label3
      // 
      this.label3.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label3.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label3.Location = new System.Drawing.Point(351, 28);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(331, 22);
      this.label3.TabIndex = 26;
      this.label3.Text = "The password used for this user account. Make sure to use special characters to p" +
          "revent brute force attacks.";
      this.label3.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // passwordTextBox
      // 
      this.passwordTextBox.Location = new System.Drawing.Point(131, 28);
      this.passwordTextBox.Name = "passwordTextBox";
      this.passwordTextBox.PasswordChar = '*';
      this.passwordTextBox.Size = new System.Drawing.Size(204, 21);
      this.passwordTextBox.TabIndex = 25;
      this.passwordTextBox.TextChanged += new System.EventHandler(this.passwordTextBox_TextChanged);
      // 
      // label5
      // 
      this.label5.Location = new System.Drawing.Point(59, 28);
      this.label5.Name = "label5";
      this.label5.Size = new System.Drawing.Size(66, 20);
      this.label5.TabIndex = 24;
      this.label5.Text = "Password:";
      this.label5.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label4
      // 
      this.label4.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label4.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label4.Location = new System.Drawing.Point(351, 2);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(331, 22);
      this.label4.TabIndex = 23;
      this.label4.Text = "The name of the user. It is recommended to use only alpha-numeric characters. Spa" +
          "ces should be avoided and be replaced by _";
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
      this.splitContainer2.Panel2.Controls.Add(this.splitContainer3);
      this.splitContainer2.Size = new System.Drawing.Size(785, 143);
      this.splitContainer2.SplitterDistance = 131;
      this.splitContainer2.SplitterWidth = 1;
      this.splitContainer2.TabIndex = 0;
      // 
      // label2
      // 
      this.label2.Location = new System.Drawing.Point(59, 0);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(66, 20);
      this.label2.TabIndex = 22;
      this.label2.Text = "Roles:";
      this.label2.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // splitContainer3
      // 
      this.splitContainer3.Dock = System.Windows.Forms.DockStyle.Fill;
      this.splitContainer3.Location = new System.Drawing.Point(0, 0);
      this.splitContainer3.Name = "splitContainer3";
      // 
      // splitContainer3.Panel1
      // 
      this.splitContainer3.Panel1.Controls.Add(this.assignedRoleList);
      // 
      // splitContainer3.Panel2
      // 
      this.splitContainer3.Panel2.Controls.Add(this.roleTreeView);
      this.splitContainer3.Panel2.Controls.Add(this.panel1);
      this.splitContainer3.Size = new System.Drawing.Size(653, 143);
      this.splitContainer3.SplitterDistance = 281;
      this.splitContainer3.TabIndex = 23;
      // 
      // assignedRoleList
      // 
      this.assignedRoleList.Dock = System.Windows.Forms.DockStyle.Fill;
      this.assignedRoleList.FormattingEnabled = true;
      this.assignedRoleList.IntegralHeight = false;
      this.assignedRoleList.Location = new System.Drawing.Point(0, 0);
      this.assignedRoleList.Margin = new System.Windows.Forms.Padding(0);
      this.assignedRoleList.Name = "assignedRoleList";
      this.assignedRoleList.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
      this.assignedRoleList.Size = new System.Drawing.Size(281, 143);
      this.assignedRoleList.TabIndex = 0;
      // 
      // roleTreeView
      // 
      this.roleTreeView.BackColor = System.Drawing.SystemColors.Window;
      this.roleTreeView.Columns.Add(this.roleTreeNameColumn);
      this.roleTreeView.DefaultToolTipProvider = null;
      this.roleTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
      this.roleTreeView.DragDropMarkColor = System.Drawing.Color.Black;
      this.roleTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
      this.roleTreeView.Location = new System.Drawing.Point(35, 0);
      this.roleTreeView.Model = null;
      this.roleTreeView.Name = "roleTreeView";
      this.roleTreeView.NodeControls.Add(this.roleTreeNodeText);
      this.roleTreeView.SelectedNode = null;
      this.roleTreeView.Size = new System.Drawing.Size(333, 143);
      this.roleTreeView.TabIndex = 2;
      // 
      // roleTreeNameColumn
      // 
      this.roleTreeNameColumn.Header = "Role";
      this.roleTreeNameColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.roleTreeNameColumn.TooltipText = null;
      // 
      // roleTreeNodeText
      // 
      this.roleTreeNodeText.EditEnabled = false;
      this.roleTreeNodeText.IncrementalSearchEnabled = true;
      this.roleTreeNodeText.LeftMargin = 1;
      this.roleTreeNodeText.ParentColumn = null;
      this.roleTreeNodeText.VirtualMode = true;
      // 
      // panel1
      // 
      this.panel1.Controls.Add(this.removeRoleButton);
      this.panel1.Controls.Add(this.addRoleButton);
      this.panel1.Dock = System.Windows.Forms.DockStyle.Left;
      this.panel1.Location = new System.Drawing.Point(0, 0);
      this.panel1.Name = "panel1";
      this.panel1.Size = new System.Drawing.Size(35, 143);
      this.panel1.TabIndex = 1;
      // 
      // removeRoleButton
      // 
      this.removeRoleButton.Location = new System.Drawing.Point(3, 32);
      this.removeRoleButton.Name = "removeRoleButton";
      this.removeRoleButton.Size = new System.Drawing.Size(24, 23);
      this.removeRoleButton.TabIndex = 1;
      this.removeRoleButton.Text = ">";
      this.removeRoleButton.UseVisualStyleBackColor = true;
      this.removeRoleButton.Click += new System.EventHandler(this.removeRoleButton_Click);
      // 
      // addRoleButton
      // 
      this.addRoleButton.Location = new System.Drawing.Point(3, 3);
      this.addRoleButton.Name = "addRoleButton";
      this.addRoleButton.Size = new System.Drawing.Size(24, 23);
      this.addRoleButton.TabIndex = 0;
      this.addRoleButton.Text = "<";
      this.addRoleButton.UseVisualStyleBackColor = true;
      this.addRoleButton.Click += new System.EventHandler(this.addRoleButton_Click);
      // 
      // tabPage2
      // 
      this.tabPage2.BackColor = System.Drawing.SystemColors.ButtonFace;
      this.tabPage2.Controls.Add(this.commentTextBox);
      this.tabPage2.Location = new System.Drawing.Point(3, 24);
      this.tabPage2.Name = "tabPage2";
      this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
      this.tabPage2.Size = new System.Drawing.Size(776, 194);
      this.tabPage2.TabIndex = 1;
      this.tabPage2.Text = "Comment";
      // 
      // commentTextBox
      // 
      this.commentTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
      this.commentTextBox.Location = new System.Drawing.Point(3, 3);
      this.commentTextBox.Multiline = true;
      this.commentTextBox.Name = "commentTextBox";
      this.commentTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
      this.commentTextBox.Size = new System.Drawing.Size(770, 188);
      this.commentTextBox.TabIndex = 0;
      this.commentTextBox.TextChanged += new System.EventHandler(this.commentTextBox_TextChanged);
      // 
      // DbMysqlUserEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.White;
      this.ClientSize = new System.Drawing.Size(791, 227);
      this.Controls.Add(this.flatTabControl1);
      this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.Name = "DbMysqlUserEditor";
      this.TabText = "DbMysqlUserEditor";
      this.Text = "DbMysqlUserEditor";
      this.flatTabControl1.ResumeLayout(false);
      this.tabPage1.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.splitContainer1.Panel1.ResumeLayout(false);
      this.splitContainer1.Panel1.PerformLayout();
      this.splitContainer1.Panel2.ResumeLayout(false);
      this.splitContainer1.ResumeLayout(false);
      this.splitContainer2.Panel1.ResumeLayout(false);
      this.splitContainer2.Panel2.ResumeLayout(false);
      this.splitContainer2.ResumeLayout(false);
      this.splitContainer3.Panel1.ResumeLayout(false);
      this.splitContainer3.Panel2.ResumeLayout(false);
      this.splitContainer3.ResumeLayout(false);
      this.panel1.ResumeLayout(false);
      this.tabPage2.ResumeLayout(false);
      this.tabPage2.PerformLayout();
      this.ResumeLayout(false);

    }

    #endregion

    private MySQL.Controls.FlatTabControl flatTabControl1;
    private System.Windows.Forms.TabPage tabPage1;
    private System.Windows.Forms.TabPage tabPage2;
    private System.Windows.Forms.SplitContainer splitContainer1;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.TextBox nameTextBox;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.SplitContainer splitContainer2;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.SplitContainer splitContainer3;
    private System.Windows.Forms.Panel panel1;
    private System.Windows.Forms.Button addRoleButton;
    private Aga.Controls.Tree.TreeViewAdv roleTreeView;
    private MySQL.Utilities.AdvNodeTextBox roleTreeNodeText;
    private Aga.Controls.Tree.TreeColumn roleTreeNameColumn;
    private System.Windows.Forms.Button removeRoleButton;
    private System.Windows.Forms.TextBox commentTextBox;
    private System.Windows.Forms.PictureBox pictureBox1;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.TextBox passwordTextBox;
    private System.Windows.Forms.Label label5;
    private System.Windows.Forms.ListBox assignedRoleList;
  }
}