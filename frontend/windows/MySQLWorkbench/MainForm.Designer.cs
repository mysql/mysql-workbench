namespace MySQL.GUI.Workbench
{
	partial class MainForm
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
      this.components = new System.ComponentModel.Container();
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
      this.mainStatusStrip = new System.Windows.Forms.StatusStrip();
      this.statusText = new System.Windows.Forms.ToolStripStatusLabel();
      this.statusProgress = new System.Windows.Forms.ToolStripProgressBar();
      this.mainFormToolTip = new System.Windows.Forms.ToolTip(this.components);
      this.tabsContextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
      this.closeTabMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.closeTabsOfSameTypeItem = new System.Windows.Forms.ToolStripMenuItem();
      this.closeOtherTabsMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.contentTabControl = new MySQL.Controls.FlatTabControl();
      this.mainStatusStrip.SuspendLayout();
      this.tabsContextMenuStrip.SuspendLayout();
      this.SuspendLayout();
      // 
      // mainStatusStrip
      // 
      this.mainStatusStrip.AutoSize = false;
      this.mainStatusStrip.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.mainStatusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.statusText});
      this.mainStatusStrip.Location = new System.Drawing.Point(0, 660);
      this.mainStatusStrip.Name = "mainStatusStrip";
      this.mainStatusStrip.Size = new System.Drawing.Size(1008, 22);
      this.mainStatusStrip.TabIndex = 0;
      this.mainStatusStrip.Text = "statusStrip1";
      this.mainStatusStrip.Visible = false;
      this.mainStatusStrip.Paint += new System.Windows.Forms.PaintEventHandler(this.mainStatusStrip_Paint);
      // 
      // statusText
      // 
      this.statusText.AutoSize = false;
      this.statusText.ForeColor = System.Drawing.Color.White;
      this.statusText.Name = "statusText";
      this.statusText.Size = new System.Drawing.Size(993, 17);
      this.statusText.Spring = true;
      this.statusText.Text = "Ready for take-off";
      this.statusText.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // statusProgress
      // 
      this.statusProgress.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
      this.statusProgress.Name = "statusProgress";
      this.statusProgress.Size = new System.Drawing.Size(100, 16);
      this.statusProgress.Visible = false;
      // 
      // tabsContextMenuStrip
      // 
      this.tabsContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.closeTabMenuItem,
            this.closeTabsOfSameTypeItem,
            this.closeOtherTabsMenuItem});
      this.tabsContextMenuStrip.Name = "recordsetPageContextMenuStrip";
      this.tabsContextMenuStrip.Size = new System.Drawing.Size(170, 70);
      // 
      // closeTabMenuItem
      // 
      this.closeTabMenuItem.Name = "closeTabMenuItem";
      this.closeTabMenuItem.Size = new System.Drawing.Size(169, 22);
      this.closeTabMenuItem.Tag = "0";
      this.closeTabMenuItem.Text = "Close";
      this.closeTabMenuItem.Click += new System.EventHandler(this.tabsContextMenuItemClick);
      // 
      // closeTabsOfSameTypeItem
      // 
      this.closeTabsOfSameTypeItem.Name = "closeTabsOfSameTypeItem";
      this.closeTabsOfSameTypeItem.Size = new System.Drawing.Size(169, 22);
      this.closeTabsOfSameTypeItem.Tag = "2";
      this.closeTabsOfSameTypeItem.Text = "Close All Like This";
      this.closeTabsOfSameTypeItem.Click += new System.EventHandler(this.tabsContextMenuItemClick);
      // 
      // closeOtherTabsMenuItem
      // 
      this.closeOtherTabsMenuItem.Name = "closeOtherTabsMenuItem";
      this.closeOtherTabsMenuItem.Size = new System.Drawing.Size(169, 22);
      this.closeOtherTabsMenuItem.Tag = "1";
      this.closeOtherTabsMenuItem.Text = "Close All But This";
      this.closeOtherTabsMenuItem.Click += new System.EventHandler(this.tabsContextMenuItemClick);
      // 
      // contentTabControl
      // 
      this.contentTabControl.AllowDrop = true;
      this.contentTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.contentTabControl.CanCloseLastTab = false;
      this.contentTabControl.CanReorderTabs = true;
      this.contentTabControl.ContentPadding = new System.Windows.Forms.Padding(0);
      this.contentTabControl.DefaultTabSwitch = false;
      this.contentTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.contentTabControl.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.contentTabControl.HideWhenEmpty = true;
      this.contentTabControl.ItemPadding = new System.Windows.Forms.Padding(12, 0, 12, 0);
      this.contentTabControl.ItemSize = new System.Drawing.Size(10, 22);
      this.contentTabControl.Location = new System.Drawing.Point(0, 0);
      this.contentTabControl.Margin = new System.Windows.Forms.Padding(0, 3, 0, 0);
      this.contentTabControl.MaxTabSize = 200;
      this.contentTabControl.Name = "contentTabControl";
      this.contentTabControl.RenderWithGlow = true;
      this.contentTabControl.SelectedIndex = 0;
      this.contentTabControl.ShowCloseButton = true;
      this.contentTabControl.ShowFocusState = false;
      this.contentTabControl.Size = new System.Drawing.Size(1008, 682);
      this.contentTabControl.TabIndex = 0;
      this.contentTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.TopTransparent;
      this.contentTabControl.TabClosing += new System.EventHandler<MySQL.Controls.TabClosingEventArgs>(this.tabControl_TabClosing);
      this.contentTabControl.TabClosed += new System.EventHandler<MySQL.Controls.TabClosedEventArgs>(this.contentTabControl_TabClosed);
      this.contentTabControl.SelectedIndexChanged += new System.EventHandler(this.tabControl_SelectedIndexChanged);
      this.contentTabControl.ControlAdded += new System.Windows.Forms.ControlEventHandler(this.tabControl_ContentAdded);
      this.contentTabControl.ControlRemoved += new System.Windows.Forms.ControlEventHandler(this.tabControl_ContentRemoved);
      this.contentTabControl.MouseClick += new System.Windows.Forms.MouseEventHandler(this.contentTabControl_MouseClick);
      // 
      // MainForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(48)))), ((int)(((byte)(48)))), ((int)(((byte)(48)))));
      this.ClientSize = new System.Drawing.Size(1008, 682);
      this.Controls.Add(this.contentTabControl);
      this.Controls.Add(this.mainStatusStrip);
      this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
      this.KeyPreview = true;
      this.Location = new System.Drawing.Point(40, 40);
      this.MinimumSize = new System.Drawing.Size(980, 600);
      this.Name = "MainForm";
      this.StartPosition = System.Windows.Forms.FormStartPosition.Manual;
      this.Text = "MySQL Workbench";
      this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainForm_FormClosing);
      this.Load += new System.EventHandler(this.MainForm_Load);
      this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyDown);
      this.mainStatusStrip.ResumeLayout(false);
      this.mainStatusStrip.PerformLayout();
      this.tabsContextMenuStrip.ResumeLayout(false);
      this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.StatusStrip mainStatusStrip;
    private MySQL.Controls.FlatTabControl contentTabControl;
    private System.Windows.Forms.ToolStripProgressBar statusProgress;
    private System.Windows.Forms.ToolTip mainFormToolTip;
    private System.Windows.Forms.ToolStripStatusLabel statusText;
    private System.Windows.Forms.ContextMenuStrip tabsContextMenuStrip;
    private System.Windows.Forms.ToolStripMenuItem closeTabMenuItem;
    private System.Windows.Forms.ToolStripMenuItem closeOtherTabsMenuItem;
    private System.Windows.Forms.ToolStripMenuItem closeTabsOfSameTypeItem;
	}
}

