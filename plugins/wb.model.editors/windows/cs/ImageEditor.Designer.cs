namespace MySQL.GUI.Workbench.Plugins
{
  partial class ImageEditor
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ImageEditor));
      this.mainTabControl = new MySQL.Controls.FlatTabControl();
      this.tabPage1 = new System.Windows.Forms.TabPage();
      this.keepAspectCheck = new System.Windows.Forms.CheckBox();
      this.resetSizeButton = new System.Windows.Forms.Button();
      this.heightText = new System.Windows.Forms.TextBox();
      this.widthText = new System.Windows.Forms.TextBox();
      this.label4 = new System.Windows.Forms.Label();
      this.label1 = new System.Windows.Forms.Label();
      this.button1 = new System.Windows.Forms.Button();
      this.pictureBox2 = new System.Windows.Forms.PictureBox();
      this.label3 = new System.Windows.Forms.Label();
      this.label2 = new System.Windows.Forms.Label();
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.nameTextBox = new System.Windows.Forms.TextBox();
      this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
      this.mainTabControl.SuspendLayout();
      this.tabPage1.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
      this.SuspendLayout();
      // 
      // mainTabControl
      // 
      this.mainTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.mainTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      this.mainTabControl.Alignment = System.Windows.Forms.TabAlignment.Bottom;
      this.mainTabControl.Controls.Add(this.tabPage1);
      this.mainTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainTabControl.Location = new System.Drawing.Point(3, 3);
      this.mainTabControl.Name = "mainTabControl";
      this.mainTabControl.SelectedIndex = 0;
      this.mainTabControl.Size = new System.Drawing.Size(741, 222);
      this.mainTabControl.TabIndex = 2;
      // 
      // tabPage1
      // 
      this.tabPage1.BackColor = System.Drawing.SystemColors.Control;
      this.tabPage1.Controls.Add(this.keepAspectCheck);
      this.tabPage1.Controls.Add(this.resetSizeButton);
      this.tabPage1.Controls.Add(this.heightText);
      this.tabPage1.Controls.Add(this.widthText);
      this.tabPage1.Controls.Add(this.label4);
      this.tabPage1.Controls.Add(this.label1);
      this.tabPage1.Controls.Add(this.button1);
      this.tabPage1.Controls.Add(this.pictureBox2);
      this.tabPage1.Controls.Add(this.label3);
      this.tabPage1.Controls.Add(this.label2);
      this.tabPage1.Controls.Add(this.pictureBox1);
      this.tabPage1.Controls.Add(this.nameTextBox);
      this.tabPage1.Location = new System.Drawing.Point(4, 4);
      this.tabPage1.Name = "tabPage1";
      this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
      this.tabPage1.Size = new System.Drawing.Size(733, 193);
      this.tabPage1.TabIndex = 0;
      this.tabPage1.Text = "Image";
      // 
      // keepAspectCheck
      // 
      this.keepAspectCheck.AutoSize = true;
      this.keepAspectCheck.Location = new System.Drawing.Point(469, 112);
      this.keepAspectCheck.Name = "keepAspectCheck";
      this.keepAspectCheck.Size = new System.Drawing.Size(115, 17);
      this.keepAspectCheck.TabIndex = 29;
      this.keepAspectCheck.Text = "Keep Aspect Ratio";
      this.keepAspectCheck.UseVisualStyleBackColor = true;
      this.keepAspectCheck.CheckedChanged += new System.EventHandler(this.keepAspectCheck_CheckedChanged);
      // 
      // resetSizeButton
      // 
      this.resetSizeButton.Location = new System.Drawing.Point(469, 135);
      this.resetSizeButton.Name = "resetSizeButton";
      this.resetSizeButton.Size = new System.Drawing.Size(75, 23);
      this.resetSizeButton.TabIndex = 28;
      this.resetSizeButton.Text = "Reset Size";
      this.resetSizeButton.UseVisualStyleBackColor = true;
      this.resetSizeButton.Click += new System.EventHandler(this.resetSizeButton_Click);
      // 
      // heightText
      // 
      this.heightText.Location = new System.Drawing.Point(587, 86);
      this.heightText.Name = "heightText";
      this.heightText.Size = new System.Drawing.Size(67, 20);
      this.heightText.TabIndex = 27;
      this.heightText.Leave += new System.EventHandler(this.heightText_Leave);
      this.heightText.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.heightText_KeyPress);
      // 
      // widthText
      // 
      this.widthText.Location = new System.Drawing.Point(469, 86);
      this.widthText.Name = "widthText";
      this.widthText.Size = new System.Drawing.Size(67, 20);
      this.widthText.TabIndex = 26;
      this.widthText.Leave += new System.EventHandler(this.widthText_Leave);
      this.widthText.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.widthText_KeyPress);
      // 
      // label4
      // 
      this.label4.AutoSize = true;
      this.label4.Location = new System.Drawing.Point(540, 89);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(41, 13);
      this.label4.TabIndex = 25;
      this.label4.Text = "Height:";
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(425, 89);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(38, 13);
      this.label1.TabIndex = 24;
      this.label1.Text = "Width:";
      // 
      // button1
      // 
      this.button1.Location = new System.Drawing.Point(435, 4);
      this.button1.Name = "button1";
      this.button1.Size = new System.Drawing.Size(75, 23);
      this.button1.TabIndex = 23;
      this.button1.Text = "Browse ...";
      this.button1.UseVisualStyleBackColor = true;
      this.button1.Click += new System.EventHandler(this.button1_Click);
      // 
      // pictureBox2
      // 
      this.pictureBox2.BackColor = System.Drawing.Color.White;
      this.pictureBox2.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.pictureBox2.Location = new System.Drawing.Point(134, 34);
      this.pictureBox2.Name = "pictureBox2";
      this.pictureBox2.Size = new System.Drawing.Size(231, 128);
      this.pictureBox2.TabIndex = 22;
      this.pictureBox2.TabStop = false;
      // 
      // label3
      // 
      this.label3.Location = new System.Drawing.Point(62, 5);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(66, 21);
      this.label3.TabIndex = 21;
      this.label3.Text = "Filename:";
      this.label3.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label2
      // 
      this.label2.Location = new System.Drawing.Point(61, 33);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(66, 21);
      this.label2.TabIndex = 19;
      this.label2.Text = "Image:";
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
      this.nameTextBox.Name = "nameTextBox";
      this.nameTextBox.ReadOnly = true;
      this.nameTextBox.Size = new System.Drawing.Size(295, 20);
      this.nameTextBox.TabIndex = 14;
      // 
      // openFileDialog1
      // 
      this.openFileDialog1.FileName = "openFileDialog1";
      // 
      // ImageEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.White;
      this.ClientSize = new System.Drawing.Size(747, 228);
      this.Controls.Add(this.mainTabControl);
      this.Name = "ImageEditor";
      this.Padding = new System.Windows.Forms.Padding(3);
      this.TabText = "ImageEditor";
      this.Text = "ImageEditor";
      this.mainTabControl.ResumeLayout(false);
      this.tabPage1.ResumeLayout(false);
      this.tabPage1.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox2)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.ResumeLayout(false);

    }

    #endregion

    private MySQL.Controls.FlatTabControl mainTabControl;
    private System.Windows.Forms.TabPage tabPage1;
    private System.Windows.Forms.PictureBox pictureBox2;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.PictureBox pictureBox1;
    private System.Windows.Forms.TextBox nameTextBox;
    private System.Windows.Forms.Button button1;
    private System.Windows.Forms.OpenFileDialog openFileDialog1;
    private System.Windows.Forms.CheckBox keepAspectCheck;
    private System.Windows.Forms.Button resetSizeButton;
    private System.Windows.Forms.TextBox heightText;
    private System.Windows.Forms.TextBox widthText;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.Label label1;
  }
}