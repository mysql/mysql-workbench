namespace MySQL.GUI.Workbench
{
    partial class ModelNavigatorForm
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
          Destroy();

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
          this.miniViewHost = new MySQL.Controls.DrawablePanel();
          this.zoomInPictureBox = new System.Windows.Forms.PictureBox();
          this.zoomOutPictureBox = new System.Windows.Forms.PictureBox();
          this.controlPanel = new System.Windows.Forms.Panel();
          this.zoomComboBox = new System.Windows.Forms.ComboBox();
          this.label1 = new System.Windows.Forms.Label();
          ((System.ComponentModel.ISupportInitialize)(this.zoomInPictureBox)).BeginInit();
          ((System.ComponentModel.ISupportInitialize)(this.zoomOutPictureBox)).BeginInit();
          this.controlPanel.SuspendLayout();
          this.SuspendLayout();
          // 
          // miniViewHost
          // 
          this.miniViewHost.BackColor = System.Drawing.Color.White;
          this.miniViewHost.CustomBackground = true;
          this.miniViewHost.Dock = System.Windows.Forms.DockStyle.Fill;
          this.miniViewHost.Location = new System.Drawing.Point(0, 29);
          this.miniViewHost.MinimumSize = new System.Drawing.Size(100, 100);
          this.miniViewHost.Name = "miniViewHost";
          this.miniViewHost.Size = new System.Drawing.Size(196, 137);
          this.miniViewHost.TabIndex = 0;
          this.miniViewHost.SizeChanged += new System.EventHandler(this.navImagePanel_SizeChanged);
          this.miniViewHost.Paint += new System.Windows.Forms.PaintEventHandler(this.navImagePanel_Paint);
          this.miniViewHost.MouseDown += new System.Windows.Forms.MouseEventHandler(this.navImagePanel_MouseDown);
          this.miniViewHost.MouseMove += new System.Windows.Forms.MouseEventHandler(this.navImagePanel_MouseMove);
          this.miniViewHost.MouseUp += new System.Windows.Forms.MouseEventHandler(this.navImagePanel_MouseUp);
          // 
          // zoomInPictureBox
          // 
          this.zoomInPictureBox.Dock = System.Windows.Forms.DockStyle.Left;
          this.zoomInPictureBox.Image = global::MySQL.GUI.Workbench.Properties.Resources.navigator_zoom_in;
          this.zoomInPictureBox.Location = new System.Drawing.Point(93, 4);
          this.zoomInPictureBox.Margin = new System.Windows.Forms.Padding(0);
          this.zoomInPictureBox.Name = "zoomInPictureBox";
          this.zoomInPictureBox.Size = new System.Drawing.Size(28, 21);
          this.zoomInPictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
          this.zoomInPictureBox.TabIndex = 2;
          this.zoomInPictureBox.TabStop = false;
          this.zoomInPictureBox.Click += new System.EventHandler(this.zoomInPictureBox_Click);
          // 
          // zoomOutPictureBox
          // 
          this.zoomOutPictureBox.Dock = System.Windows.Forms.DockStyle.Left;
          this.zoomOutPictureBox.Image = global::MySQL.GUI.Workbench.Properties.Resources.navigator_zoom_out;
          this.zoomOutPictureBox.Location = new System.Drawing.Point(121, 4);
          this.zoomOutPictureBox.Name = "zoomOutPictureBox";
          this.zoomOutPictureBox.Size = new System.Drawing.Size(28, 21);
          this.zoomOutPictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
          this.zoomOutPictureBox.TabIndex = 1;
          this.zoomOutPictureBox.TabStop = false;
          this.zoomOutPictureBox.Click += new System.EventHandler(this.zoomOutPictureBox_Click);
          // 
          // controlPanel
          // 
          this.controlPanel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(189)))), ((int)(((byte)(199)))), ((int)(((byte)(222)))));
          this.controlPanel.Controls.Add(this.zoomOutPictureBox);
          this.controlPanel.Controls.Add(this.zoomInPictureBox);
          this.controlPanel.Controls.Add(this.zoomComboBox);
          this.controlPanel.Controls.Add(this.label1);
          this.controlPanel.Dock = System.Windows.Forms.DockStyle.Top;
          this.controlPanel.Location = new System.Drawing.Point(0, 0);
          this.controlPanel.Margin = new System.Windows.Forms.Padding(0);
          this.controlPanel.Name = "controlPanel";
          this.controlPanel.Padding = new System.Windows.Forms.Padding(4);
          this.controlPanel.Size = new System.Drawing.Size(196, 29);
          this.controlPanel.TabIndex = 3;
          // 
          // zoomComboBox
          // 
          this.zoomComboBox.Dock = System.Windows.Forms.DockStyle.Left;
          this.zoomComboBox.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
          this.zoomComboBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8F);
          this.zoomComboBox.FormatString = "###0";
          this.zoomComboBox.FormattingEnabled = true;
          this.zoomComboBox.Items.AddRange(new object[] {
            "200%",
            "150%",
            "100%",
            "95%",
            "90%",
            "85%",
            "80%",
            "75%",
            "70%",
            "60%",
            "50%",
            "40%",
            "30%",
            "20%",
            "10"});
          this.zoomComboBox.Location = new System.Drawing.Point(41, 4);
          this.zoomComboBox.Name = "zoomComboBox";
          this.zoomComboBox.Size = new System.Drawing.Size(52, 21);
          this.zoomComboBox.TabIndex = 1;
          this.zoomComboBox.Text = "100%";
          this.zoomComboBox.SelectedIndexChanged += new System.EventHandler(this.zoomComboBox_SelectedIndexChanged);
          this.zoomComboBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.zoomComboBox_KeyDown);
          // 
          // label1
          // 
          this.label1.Dock = System.Windows.Forms.DockStyle.Left;
          this.label1.Location = new System.Drawing.Point(4, 4);
          this.label1.Name = "label1";
          this.label1.Size = new System.Drawing.Size(37, 21);
          this.label1.TabIndex = 3;
          this.label1.Text = "Zoom:";
          this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
          // 
          // ModelNavigatorForm
          // 
          this.BackColor = System.Drawing.SystemColors.Window;
          this.ClientSize = new System.Drawing.Size(196, 166);
          this.Controls.Add(this.miniViewHost);
          this.Controls.Add(this.controlPanel);
          this.DoubleBuffered = true;
          this.ForeColor = System.Drawing.SystemColors.ControlText;
          this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
          this.Name = "ModelNavigatorForm";
          this.Text = "Model Navigator";
          ((System.ComponentModel.ISupportInitialize)(this.zoomInPictureBox)).EndInit();
          ((System.ComponentModel.ISupportInitialize)(this.zoomOutPictureBox)).EndInit();
          this.controlPanel.ResumeLayout(false);
          this.ResumeLayout(false);

        }

        #endregion

        private MySQL.Controls.DrawablePanel miniViewHost;
      private System.Windows.Forms.PictureBox zoomInPictureBox;
      private System.Windows.Forms.PictureBox zoomOutPictureBox;
      private System.Windows.Forms.Panel controlPanel;
      private System.Windows.Forms.ComboBox zoomComboBox;
      private System.Windows.Forms.Label label1;
    }
}