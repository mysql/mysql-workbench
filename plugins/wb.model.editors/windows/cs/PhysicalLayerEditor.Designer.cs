namespace MySQL.GUI.Workbench.Plugins
{
  partial class PhysicalLayerEditor
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PhysicalLayerEditor));
      this.mainTabControl = new MySQL.Controls.FlatTabControl();
      this.tabPage1 = new System.Windows.Forms.TabPage();
      this.label1 = new System.Windows.Forms.Label();
      this.label3 = new System.Windows.Forms.Label();
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.nameTextBox = new System.Windows.Forms.TextBox();
      this.layerColorDialog = new System.Windows.Forms.ColorDialog();
      this.colorDialogButton = new System.Windows.Forms.Button();
      this.colorEdit = new System.Windows.Forms.TextBox();
      this.mainTabControl.SuspendLayout();
      this.tabPage1.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
      this.SuspendLayout();
      // 
      // mainTabControl
      // 
      this.mainTabControl.Alignment = System.Windows.Forms.TabAlignment.Bottom;
      this.mainTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.mainTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      this.mainTabControl.Controls.Add(this.tabPage1);
      this.mainTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainTabControl.Location = new System.Drawing.Point(3, 3);
      this.mainTabControl.Name = "mainTabControl";
      this.mainTabControl.SelectedIndex = 0;
      this.mainTabControl.Size = new System.Drawing.Size(741, 222);
      this.mainTabControl.TabIndex = 1;
      // 
      // tabPage1
      // 
      this.tabPage1.BackColor = System.Drawing.SystemColors.Control;
      this.tabPage1.Controls.Add(this.colorEdit);
      this.tabPage1.Controls.Add(this.colorDialogButton);
      this.tabPage1.Controls.Add(this.label1);
      this.tabPage1.Controls.Add(this.label3);
      this.tabPage1.Controls.Add(this.pictureBox1);
      this.tabPage1.Controls.Add(this.nameTextBox);
      this.tabPage1.Location = new System.Drawing.Point(4, 4);
      this.tabPage1.Name = "tabPage1";
      this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
      this.tabPage1.Size = new System.Drawing.Size(733, 193);
      this.tabPage1.TabIndex = 0;
      this.tabPage1.Text = "Layer";
      // 
      // label1
      // 
      this.label1.Location = new System.Drawing.Point(104, 38);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(66, 21);
      this.label1.TabIndex = 22;
      this.label1.Text = "Color:";
      this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      this.label1.Visible = false;
      // 
      // label3
      // 
      this.label3.Location = new System.Drawing.Point(65, 6);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(105, 21);
      this.label3.TabIndex = 21;
      this.label3.Text = "Layer Name:";
      this.label3.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
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
      this.nameTextBox.Location = new System.Drawing.Point(176, 7);
      this.nameTextBox.MaxLength = 40;
      this.nameTextBox.Name = "nameTextBox";
      this.nameTextBox.Size = new System.Drawing.Size(390, 20);
      this.nameTextBox.TabIndex = 14;
      this.nameTextBox.TextChanged += new System.EventHandler(this.nameTextBox_TextChanged);
      // 
      // layerColorDialog
      // 
      this.layerColorDialog.AnyColor = true;
      this.layerColorDialog.FullOpen = true;
      // 
      // colorDialogButton
      // 
      this.colorDialogButton.BackColor = System.Drawing.SystemColors.ControlText;
      this.colorDialogButton.ForeColor = System.Drawing.SystemColors.ControlText;
      this.colorDialogButton.Location = new System.Drawing.Point(282, 37);
      this.colorDialogButton.Name = "colorDialogButton";
      this.colorDialogButton.Size = new System.Drawing.Size(59, 23);
      this.colorDialogButton.TabIndex = 24;
      this.colorDialogButton.UseVisualStyleBackColor = false;
      this.colorDialogButton.Click += new System.EventHandler(this.colorDialogButton_Click);
      // 
      // colorEdit
      // 
      this.colorEdit.Location = new System.Drawing.Point(176, 39);
      this.colorEdit.Name = "colorEdit";
      this.colorEdit.Size = new System.Drawing.Size(100, 20);
      this.colorEdit.TabIndex = 25;
      this.colorEdit.TextChanged += new System.EventHandler(this.colorEdit_TextChanged);
      // 
      // PhysicalLayerEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.White;
      this.ClientSize = new System.Drawing.Size(747, 228);
      this.Controls.Add(this.mainTabControl);
      this.Name = "PhysicalLayerEditor";
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
    private System.Windows.Forms.PictureBox pictureBox1;
    private System.Windows.Forms.TextBox nameTextBox;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.ColorDialog layerColorDialog;
    private System.Windows.Forms.Button colorDialogButton;
    private System.Windows.Forms.TextBox colorEdit;
  }
}

