namespace MySQL.GUI.Workbench.Plugins
{
  partial class WizardPlugin
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WizardPlugin));
      this.panel1 = new System.Windows.Forms.Panel();
      this.btnAdvanced = new System.Windows.Forms.Button();
      this.btnBack = new System.Windows.Forms.Button();
      this.btnCancel = new System.Windows.Forms.Button();
      this.btnNext = new System.Windows.Forms.Button();
      this.main_panel = new System.Windows.Forms.Panel();
      this.panel = new System.Windows.Forms.Panel();
      this.stepDescriptionPanel = new System.Windows.Forms.Panel();
      this.descriptionLabel = new System.Windows.Forms.Label();
      this.captionLabel = new System.Windows.Forms.Label();
      this.bevel2 = new MySQL.Utilities.Bevel();
      this.bevel1 = new MySQL.Utilities.Bevel();
      this.panel1.SuspendLayout();
      this.main_panel.SuspendLayout();
      this.stepDescriptionPanel.SuspendLayout();
      this.SuspendLayout();
      // 
      // panel1
      // 
      this.panel1.Controls.Add(this.btnAdvanced);
      this.panel1.Controls.Add(this.btnBack);
      this.panel1.Controls.Add(this.btnCancel);
      this.panel1.Controls.Add(this.btnNext);
      this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
      this.panel1.Location = new System.Drawing.Point(0, 550);
      this.panel1.Name = "panel1";
      this.panel1.Size = new System.Drawing.Size(714, 36);
      this.panel1.TabIndex = 4;
      // 
      // btnAdvanced
      // 
      this.btnAdvanced.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
      this.btnAdvanced.Location = new System.Drawing.Point(12, 6);
      this.btnAdvanced.Name = "btnAdvanced";
      this.btnAdvanced.Size = new System.Drawing.Size(92, 23);
      this.btnAdvanced.TabIndex = 0;
      this.btnAdvanced.Text = "Advanced";
      this.btnAdvanced.UseVisualStyleBackColor = true;
      this.btnAdvanced.Click += new System.EventHandler(this.btnAdvanced_Click);
      // 
      // btnBack
      // 
      this.btnBack.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.btnBack.Location = new System.Drawing.Point(458, 6);
      this.btnBack.Name = "btnBack";
      this.btnBack.Size = new System.Drawing.Size(77, 23);
      this.btnBack.TabIndex = 1;
      this.btnBack.Text = "< Back";
      this.btnBack.UseVisualStyleBackColor = true;
      this.btnBack.Click += new System.EventHandler(this.btnBack_Click);
      // 
      // btnCancel
      // 
      this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
      this.btnCancel.Location = new System.Drawing.Point(624, 6);
      this.btnCancel.Name = "btnCancel";
      this.btnCancel.Size = new System.Drawing.Size(78, 23);
      this.btnCancel.TabIndex = 3;
      this.btnCancel.Text = "Cancel";
      this.btnCancel.UseVisualStyleBackColor = true;
      this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
      // 
      // btnNext
      // 
      this.btnNext.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.btnNext.Location = new System.Drawing.Point(541, 6);
      this.btnNext.Name = "btnNext";
      this.btnNext.Size = new System.Drawing.Size(77, 23);
      this.btnNext.TabIndex = 2;
      this.btnNext.Text = "Next >";
      this.btnNext.UseVisualStyleBackColor = true;
      this.btnNext.Click += new System.EventHandler(this.btnNext_Click);
      // 
      // main_panel
      // 
      this.main_panel.Controls.Add(this.panel);
      this.main_panel.Controls.Add(this.stepDescriptionPanel);
      this.main_panel.Dock = System.Windows.Forms.DockStyle.Fill;
      this.main_panel.Location = new System.Drawing.Point(0, 0);
      this.main_panel.Name = "main_panel";
      this.main_panel.Size = new System.Drawing.Size(714, 550);
      this.main_panel.TabIndex = 2;
      // 
      // panel
      // 
      this.panel.BackColor = System.Drawing.SystemColors.Control;
      this.panel.Dock = System.Windows.Forms.DockStyle.Fill;
      this.panel.Location = new System.Drawing.Point(0, 53);
      this.panel.Name = "panel";
      this.panel.Size = new System.Drawing.Size(714, 497);
      this.panel.TabIndex = 1;
      // 
      // stepDescriptionPanel
      // 
      this.stepDescriptionPanel.BackColor = System.Drawing.SystemColors.Window;
      this.stepDescriptionPanel.Controls.Add(this.descriptionLabel);
      this.stepDescriptionPanel.Controls.Add(this.captionLabel);
      this.stepDescriptionPanel.Controls.Add(this.bevel2);
      this.stepDescriptionPanel.Dock = System.Windows.Forms.DockStyle.Top;
      this.stepDescriptionPanel.Location = new System.Drawing.Point(0, 0);
      this.stepDescriptionPanel.Name = "stepDescriptionPanel";
      this.stepDescriptionPanel.Size = new System.Drawing.Size(714, 53);
      this.stepDescriptionPanel.TabIndex = 0;
      // 
      // descriptionLabel
      // 
      this.descriptionLabel.AutoSize = true;
      this.descriptionLabel.BackColor = System.Drawing.SystemColors.Window;
      this.descriptionLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
      this.descriptionLabel.Location = new System.Drawing.Point(31, 29);
      this.descriptionLabel.Name = "descriptionLabel";
      this.descriptionLabel.Size = new System.Drawing.Size(121, 13);
      this.descriptionLabel.TabIndex = 1;
      this.descriptionLabel.Text = "Active Page Description";
      this.descriptionLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
      // 
      // captionLabel
      // 
      this.captionLabel.AutoSize = true;
      this.captionLabel.BackColor = System.Drawing.SystemColors.Window;
      this.captionLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
      this.captionLabel.Location = new System.Drawing.Point(20, 11);
      this.captionLabel.Name = "captionLabel";
      this.captionLabel.Size = new System.Drawing.Size(123, 13);
      this.captionLabel.TabIndex = 0;
      this.captionLabel.Text = "Active Page Caption";
      this.captionLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
      // 
      // bevel2
      // 
      this.bevel2.BevelStyle = MySQL.Utilities.BevelStyleType.Lowered;
      this.bevel2.BorderSide = System.Windows.Forms.Border3DSide.Top;
      this.bevel2.Dock = System.Windows.Forms.DockStyle.Bottom;
      this.bevel2.Location = new System.Drawing.Point(0, 51);
      this.bevel2.Margin = new System.Windows.Forms.Padding(8, 3, 8, 3);
      this.bevel2.Name = "bevel2";
      this.bevel2.Size = new System.Drawing.Size(714, 2);
      this.bevel2.TabIndex = 5;
      this.bevel2.Text = "bevel2";
      // 
      // bevel1
      // 
      this.bevel1.BevelStyle = MySQL.Utilities.BevelStyleType.Lowered;
      this.bevel1.BorderSide = System.Windows.Forms.Border3DSide.Top;
      this.bevel1.Dock = System.Windows.Forms.DockStyle.Bottom;
      this.bevel1.Location = new System.Drawing.Point(0, 548);
      this.bevel1.Margin = new System.Windows.Forms.Padding(8, 3, 8, 3);
      this.bevel1.Name = "bevel1";
      this.bevel1.Size = new System.Drawing.Size(714, 2);
      this.bevel1.TabIndex = 4;
      this.bevel1.Text = "bevel1";
      // 
      // WizardPlugin
      // 
      this.AcceptButton = this.btnNext;
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.CancelButton = this.btnCancel;
      this.ClientSize = new System.Drawing.Size(714, 586);
      this.Controls.Add(this.bevel1);
      this.Controls.Add(this.main_panel);
      this.Controls.Add(this.panel1);
      this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
      this.Name = "WizardPlugin";
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
      this.TabText = "Import";
      this.Text = "";
      this.panel1.ResumeLayout(false);
      this.main_panel.ResumeLayout(false);
      this.stepDescriptionPanel.ResumeLayout(false);
      this.stepDescriptionPanel.PerformLayout();
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Panel panel1;
    private System.Windows.Forms.Button btnBack;
    private System.Windows.Forms.Button btnCancel;
    private System.Windows.Forms.Button btnNext;
    private System.Windows.Forms.Panel main_panel;
    private System.Windows.Forms.Panel panel;
    private System.Windows.Forms.Panel stepDescriptionPanel;
    private System.Windows.Forms.Label descriptionLabel;
    private System.Windows.Forms.Label captionLabel;
    private System.Windows.Forms.Button btnAdvanced;
    private MySQL.Utilities.Bevel bevel1;
    private MySQL.Utilities.Bevel bevel2;
  }
}