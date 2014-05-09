namespace MySQL.GUI.Workbench
{
  partial class PageSettingsForm
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PageSettingsForm));
      this.okButton = new System.Windows.Forms.Button();
      this.cancelButton = new System.Windows.Forms.Button();
      this.landscapeRadio = new System.Windows.Forms.RadioButton();
      this.portraitRadio = new System.Windows.Forms.RadioButton();
      this.groupBox1 = new System.Windows.Forms.GroupBox();
      this.unitLabel2 = new System.Windows.Forms.Label();
      this.unitLabel1 = new System.Windows.Forms.Label();
      this.unitLabel4 = new System.Windows.Forms.Label();
      this.unitLabel3 = new System.Windows.Forms.Label();
      this.leftMarginText = new System.Windows.Forms.TextBox();
      this.label8 = new System.Windows.Forms.Label();
      this.rightMarginText = new System.Windows.Forms.TextBox();
      this.label7 = new System.Windows.Forms.Label();
      this.bottomMarginText = new System.Windows.Forms.TextBox();
      this.label6 = new System.Windows.Forms.Label();
      this.topMarginText = new System.Windows.Forms.TextBox();
      this.label5 = new System.Windows.Forms.Label();
      this.groupBox2 = new System.Windows.Forms.GroupBox();
      this.groupBox3 = new System.Windows.Forms.GroupBox();
      this.paperSizeLabel = new System.Windows.Forms.Label();
      this.paperSizeCombo = new System.Windows.Forms.ComboBox();
      this.label1 = new System.Windows.Forms.Label();
      this.groupBox1.SuspendLayout();
      this.groupBox2.SuspendLayout();
      this.groupBox3.SuspendLayout();
      this.SuspendLayout();
      // 
      // okButton
      // 
      this.okButton.Location = new System.Drawing.Point(246, 212);
      this.okButton.Name = "okButton";
      this.okButton.Size = new System.Drawing.Size(75, 23);
      this.okButton.TabIndex = 9;
      this.okButton.Text = "&OK";
      this.okButton.UseVisualStyleBackColor = true;
      this.okButton.Click += new System.EventHandler(this.okButton_Click);
      // 
      // cancelButton
      // 
      this.cancelButton.Location = new System.Drawing.Point(332, 212);
      this.cancelButton.Name = "cancelButton";
      this.cancelButton.Size = new System.Drawing.Size(75, 23);
      this.cancelButton.TabIndex = 9;
      this.cancelButton.Text = "&Cancel";
      this.cancelButton.UseVisualStyleBackColor = true;
      this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
      // 
      // landscapeRadio
      // 
      this.landscapeRadio.AutoSize = true;
      this.landscapeRadio.Location = new System.Drawing.Point(18, 59);
      this.landscapeRadio.Name = "landscapeRadio";
      this.landscapeRadio.Size = new System.Drawing.Size(78, 17);
      this.landscapeRadio.TabIndex = 44;
      this.landscapeRadio.TabStop = true;
      this.landscapeRadio.Text = "Landscape";
      this.landscapeRadio.UseVisualStyleBackColor = true;
      // 
      // portraitRadio
      // 
      this.portraitRadio.AutoSize = true;
      this.portraitRadio.Location = new System.Drawing.Point(18, 25);
      this.portraitRadio.Name = "portraitRadio";
      this.portraitRadio.Size = new System.Drawing.Size(58, 17);
      this.portraitRadio.TabIndex = 45;
      this.portraitRadio.TabStop = true;
      this.portraitRadio.Text = "Portrait";
      this.portraitRadio.UseVisualStyleBackColor = true;
      // 
      // groupBox1
      // 
      this.groupBox1.Controls.Add(this.unitLabel2);
      this.groupBox1.Controls.Add(this.unitLabel1);
      this.groupBox1.Controls.Add(this.unitLabel4);
      this.groupBox1.Controls.Add(this.unitLabel3);
      this.groupBox1.Controls.Add(this.leftMarginText);
      this.groupBox1.Controls.Add(this.label8);
      this.groupBox1.Controls.Add(this.rightMarginText);
      this.groupBox1.Controls.Add(this.label7);
      this.groupBox1.Controls.Add(this.bottomMarginText);
      this.groupBox1.Controls.Add(this.label6);
      this.groupBox1.Controls.Add(this.topMarginText);
      this.groupBox1.Controls.Add(this.label5);
      this.groupBox1.Location = new System.Drawing.Point(128, 100);
      this.groupBox1.Name = "groupBox1";
      this.groupBox1.Size = new System.Drawing.Size(279, 100);
      this.groupBox1.TabIndex = 46;
      this.groupBox1.TabStop = false;
      this.groupBox1.Text = "Margins";
      // 
      // unitLabel2
      // 
      this.unitLabel2.AutoSize = true;
      this.unitLabel2.Location = new System.Drawing.Point(115, 56);
      this.unitLabel2.Name = "unitLabel2";
      this.unitLabel2.Size = new System.Drawing.Size(23, 13);
      this.unitLabel2.TabIndex = 59;
      this.unitLabel2.Text = "mm";
      // 
      // unitLabel1
      // 
      this.unitLabel1.AutoSize = true;
      this.unitLabel1.Location = new System.Drawing.Point(115, 30);
      this.unitLabel1.Name = "unitLabel1";
      this.unitLabel1.Size = new System.Drawing.Size(23, 13);
      this.unitLabel1.TabIndex = 58;
      this.unitLabel1.Text = "mm";
      // 
      // unitLabel4
      // 
      this.unitLabel4.AutoSize = true;
      this.unitLabel4.Location = new System.Drawing.Point(239, 57);
      this.unitLabel4.Name = "unitLabel4";
      this.unitLabel4.Size = new System.Drawing.Size(23, 13);
      this.unitLabel4.TabIndex = 61;
      this.unitLabel4.Text = "mm";
      // 
      // unitLabel3
      // 
      this.unitLabel3.AutoSize = true;
      this.unitLabel3.Location = new System.Drawing.Point(239, 30);
      this.unitLabel3.Name = "unitLabel3";
      this.unitLabel3.Size = new System.Drawing.Size(23, 13);
      this.unitLabel3.TabIndex = 60;
      this.unitLabel3.Text = "mm";
      // 
      // leftMarginText
      // 
      this.leftMarginText.Location = new System.Drawing.Point(189, 27);
      this.leftMarginText.Name = "leftMarginText";
      this.leftMarginText.Size = new System.Drawing.Size(48, 20);
      this.leftMarginText.TabIndex = 54;
      this.leftMarginText.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
      this.leftMarginText.Validating += new System.ComponentModel.CancelEventHandler(this.marginOptionTextBox_Validating);
      // 
      // label8
      // 
      this.label8.AutoSize = true;
      this.label8.Location = new System.Drawing.Point(155, 30);
      this.label8.Name = "label8";
      this.label8.Size = new System.Drawing.Size(28, 13);
      this.label8.TabIndex = 53;
      this.label8.Text = "Left:";
      this.label8.TextAlign = System.Drawing.ContentAlignment.TopRight;
      // 
      // rightMarginText
      // 
      this.rightMarginText.Location = new System.Drawing.Point(189, 54);
      this.rightMarginText.Name = "rightMarginText";
      this.rightMarginText.Size = new System.Drawing.Size(48, 20);
      this.rightMarginText.TabIndex = 56;
      this.rightMarginText.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
      this.rightMarginText.Validating += new System.ComponentModel.CancelEventHandler(this.marginOptionTextBox_Validating);
      // 
      // label7
      // 
      this.label7.AutoSize = true;
      this.label7.Location = new System.Drawing.Point(148, 57);
      this.label7.Name = "label7";
      this.label7.Size = new System.Drawing.Size(35, 13);
      this.label7.TabIndex = 50;
      this.label7.Text = "Right:";
      // 
      // bottomMarginText
      // 
      this.bottomMarginText.Location = new System.Drawing.Point(65, 53);
      this.bottomMarginText.Name = "bottomMarginText";
      this.bottomMarginText.Size = new System.Drawing.Size(48, 20);
      this.bottomMarginText.TabIndex = 57;
      this.bottomMarginText.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
      this.bottomMarginText.Validating += new System.ComponentModel.CancelEventHandler(this.marginOptionTextBox_Validating);
      // 
      // label6
      // 
      this.label6.AutoSize = true;
      this.label6.Location = new System.Drawing.Point(16, 57);
      this.label6.Name = "label6";
      this.label6.Size = new System.Drawing.Size(43, 13);
      this.label6.TabIndex = 51;
      this.label6.Text = "Bottom:";
      this.label6.TextAlign = System.Drawing.ContentAlignment.TopRight;
      // 
      // topMarginText
      // 
      this.topMarginText.Location = new System.Drawing.Point(65, 27);
      this.topMarginText.Name = "topMarginText";
      this.topMarginText.Size = new System.Drawing.Size(48, 20);
      this.topMarginText.TabIndex = 55;
      this.topMarginText.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
      this.topMarginText.Validating += new System.ComponentModel.CancelEventHandler(this.marginOptionTextBox_Validating);
      // 
      // label5
      // 
      this.label5.AutoSize = true;
      this.label5.Location = new System.Drawing.Point(30, 30);
      this.label5.Name = "label5";
      this.label5.Size = new System.Drawing.Size(29, 13);
      this.label5.TabIndex = 52;
      this.label5.Text = "Top:";
      this.label5.TextAlign = System.Drawing.ContentAlignment.TopRight;
      // 
      // groupBox2
      // 
      this.groupBox2.Controls.Add(this.portraitRadio);
      this.groupBox2.Controls.Add(this.landscapeRadio);
      this.groupBox2.Location = new System.Drawing.Point(16, 100);
      this.groupBox2.Name = "groupBox2";
      this.groupBox2.Size = new System.Drawing.Size(106, 100);
      this.groupBox2.TabIndex = 47;
      this.groupBox2.TabStop = false;
      this.groupBox2.Text = "Orientation";
      // 
      // groupBox3
      // 
      this.groupBox3.Controls.Add(this.paperSizeLabel);
      this.groupBox3.Controls.Add(this.paperSizeCombo);
      this.groupBox3.Controls.Add(this.label1);
      this.groupBox3.Location = new System.Drawing.Point(16, 11);
      this.groupBox3.Name = "groupBox3";
      this.groupBox3.Size = new System.Drawing.Size(391, 79);
      this.groupBox3.TabIndex = 48;
      this.groupBox3.TabStop = false;
      this.groupBox3.Text = "Paper";
      // 
      // paperSizeLabel
      // 
      this.paperSizeLabel.AutoSize = true;
      this.paperSizeLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.paperSizeLabel.Location = new System.Drawing.Point(86, 52);
      this.paperSizeLabel.Name = "paperSizeLabel";
      this.paperSizeLabel.Size = new System.Drawing.Size(24, 12);
      this.paperSizeLabel.TabIndex = 36;
      this.paperSizeLabel.Text = "1 x 1";
      // 
      // paperSizeCombo
      // 
      this.paperSizeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.paperSizeCombo.FormattingEnabled = true;
      this.paperSizeCombo.Location = new System.Drawing.Point(73, 28);
      this.paperSizeCombo.Name = "paperSizeCombo";
      this.paperSizeCombo.Size = new System.Drawing.Size(301, 21);
      this.paperSizeCombo.TabIndex = 35;
      this.paperSizeCombo.SelectedIndexChanged += new System.EventHandler(this.paperSizeCombo_SelectedIndexChanged);
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(37, 31);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(30, 13);
      this.label1.TabIndex = 33;
      this.label1.Text = "Size:";
      // 
      // PageSettingsForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(427, 247);
      this.Controls.Add(this.groupBox3);
      this.Controls.Add(this.groupBox2);
      this.Controls.Add(this.groupBox1);
      this.Controls.Add(this.cancelButton);
      this.Controls.Add(this.okButton);
      this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
      this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
      this.Name = "PageSettingsForm";
      this.ShowIcon = false;
      this.Text = "Page Setup";
      this.Shown += new System.EventHandler(this.PageSettingsForm_Shown);
      this.groupBox1.ResumeLayout(false);
      this.groupBox1.PerformLayout();
      this.groupBox2.ResumeLayout(false);
      this.groupBox2.PerformLayout();
      this.groupBox3.ResumeLayout(false);
      this.groupBox3.PerformLayout();
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Button okButton;
    private System.Windows.Forms.Button cancelButton;
    private System.Windows.Forms.RadioButton landscapeRadio;
    private System.Windows.Forms.RadioButton portraitRadio;
    private System.Windows.Forms.GroupBox groupBox1;
    private System.Windows.Forms.Label unitLabel2;
    private System.Windows.Forms.Label unitLabel1;
    private System.Windows.Forms.Label unitLabel4;
    private System.Windows.Forms.Label unitLabel3;
    private System.Windows.Forms.TextBox leftMarginText;
    private System.Windows.Forms.Label label8;
    private System.Windows.Forms.TextBox rightMarginText;
    private System.Windows.Forms.Label label7;
    private System.Windows.Forms.TextBox bottomMarginText;
    private System.Windows.Forms.Label label6;
    private System.Windows.Forms.TextBox topMarginText;
    private System.Windows.Forms.Label label5;
    private System.Windows.Forms.GroupBox groupBox2;
    private System.Windows.Forms.GroupBox groupBox3;
    private System.Windows.Forms.Label paperSizeLabel;
    private System.Windows.Forms.ComboBox paperSizeCombo;
    private System.Windows.Forms.Label label1;

  }
}