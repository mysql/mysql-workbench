using System.Windows.Forms;

namespace MySQL.GUI.Workbench
{
	partial class ModelOverviewForm
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
      Application.RemoveMessageFilter(wheelMessageFilter);
      foreach (ListView listview in listsByNode.Values)
      {
        listview.SmallImageList = null;
        listview.LargeImageList = null;
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
      this.contentHeaderPanel = new MySQL.Controls.HeaderPanel();
      this.scrollPanel = new System.Windows.Forms.Panel();
      this.mainSplitContainer = new System.Windows.Forms.SplitContainer();
      this.sideSplitContainer = new System.Windows.Forms.SplitContainer();
      this.sideTopTabControl = new MySQL.Controls.FlatTabControl();
      this.sideBottomTabControl = new MySQL.Controls.FlatTabControl();
      this.mainContentSplitContainer = new System.Windows.Forms.SplitContainer();
      this.contentSplitContainer = new System.Windows.Forms.SplitContainer();
      this.bottomTabControl = new MySQL.Controls.FlatTabControl();
      this.secondarySidebarPanel = new MySQL.Controls.HeaderPanel();
      this.contentHeaderPanel.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).BeginInit();
      this.mainSplitContainer.Panel1.SuspendLayout();
      this.mainSplitContainer.Panel2.SuspendLayout();
      this.mainSplitContainer.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.sideSplitContainer)).BeginInit();
      this.sideSplitContainer.Panel1.SuspendLayout();
      this.sideSplitContainer.Panel2.SuspendLayout();
      this.sideSplitContainer.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.mainContentSplitContainer)).BeginInit();
      this.mainContentSplitContainer.Panel1.SuspendLayout();
      this.mainContentSplitContainer.Panel2.SuspendLayout();
      this.mainContentSplitContainer.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.contentSplitContainer)).BeginInit();
      this.contentSplitContainer.Panel1.SuspendLayout();
      this.contentSplitContainer.Panel2.SuspendLayout();
      this.contentSplitContainer.SuspendLayout();
      this.SuspendLayout();
      // 
      // contentHeaderPanel
      // 
      this.contentHeaderPanel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.contentHeaderPanel.Controls.Add(this.scrollPanel);
      this.contentHeaderPanel.Dock = System.Windows.Forms.DockStyle.Fill;
      this.contentHeaderPanel.ForeColor = System.Drawing.Color.White;
      this.contentHeaderPanel.ForeColorFocused = System.Drawing.Color.White;
      this.contentHeaderPanel.HeaderColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.contentHeaderPanel.HeaderColorFocused = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.contentHeaderPanel.HeaderPadding = new System.Windows.Forms.Padding(5, 0, 5, 0);
      this.contentHeaderPanel.Location = new System.Drawing.Point(0, 0);
      this.contentHeaderPanel.Name = "contentHeaderPanel";
      this.contentHeaderPanel.Padding = new System.Windows.Forms.Padding(0, 24, 0, 0);
      this.contentHeaderPanel.Size = new System.Drawing.Size(493, 252);
      this.contentHeaderPanel.TabIndex = 11;
      this.contentHeaderPanel.Text = "Model Overview";
      // 
      // scrollPanel
      // 
      this.scrollPanel.AutoScroll = true;
      this.scrollPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
      this.scrollPanel.BackColor = System.Drawing.Color.White;
      this.scrollPanel.Dock = System.Windows.Forms.DockStyle.Fill;
      this.scrollPanel.Location = new System.Drawing.Point(0, 24);
      this.scrollPanel.Name = "scrollPanel";
      this.scrollPanel.Size = new System.Drawing.Size(493, 228);
      this.scrollPanel.TabIndex = 13;
      // 
      // mainSplitContainer
      // 
      this.mainSplitContainer.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.mainSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
      this.mainSplitContainer.Location = new System.Drawing.Point(0, 0);
      this.mainSplitContainer.Margin = new System.Windows.Forms.Padding(0);
      this.mainSplitContainer.Name = "mainSplitContainer";
      // 
      // mainSplitContainer.Panel1
      // 
      this.mainSplitContainer.Panel1.AutoScroll = true;
      this.mainSplitContainer.Panel1.Controls.Add(this.sideSplitContainer);
      this.mainSplitContainer.Panel1MinSize = 0;
      // 
      // mainSplitContainer.Panel2
      // 
      this.mainSplitContainer.Panel2.Controls.Add(this.mainContentSplitContainer);
      this.mainSplitContainer.Size = new System.Drawing.Size(858, 594);
      this.mainSplitContainer.SplitterDistance = 158;
      this.mainSplitContainer.SplitterWidth = 6;
      this.mainSplitContainer.TabIndex = 13;
      this.mainSplitContainer.TabStop = false;
      this.mainSplitContainer.SplitterMoving += new System.Windows.Forms.SplitterCancelEventHandler(this.SplitterMoving);
      this.mainSplitContainer.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.mainSplitContainer_SplitterMoved);
      // 
      // sideSplitContainer
      // 
      this.sideSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.sideSplitContainer.Location = new System.Drawing.Point(0, 0);
      this.sideSplitContainer.Margin = new System.Windows.Forms.Padding(0);
      this.sideSplitContainer.Name = "sideSplitContainer";
      this.sideSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
      // 
      // sideSplitContainer.Panel1
      // 
      this.sideSplitContainer.Panel1.Controls.Add(this.sideTopTabControl);
      this.sideSplitContainer.Panel1MinSize = 100;
      // 
      // sideSplitContainer.Panel2
      // 
      this.sideSplitContainer.Panel2.Controls.Add(this.sideBottomTabControl);
      this.sideSplitContainer.Panel2MinSize = 100;
      this.sideSplitContainer.Size = new System.Drawing.Size(158, 594);
      this.sideSplitContainer.SplitterDistance = 292;
      this.sideSplitContainer.SplitterWidth = 6;
      this.sideSplitContainer.TabIndex = 2;
      this.sideSplitContainer.TabStop = false;
      // 
      // sideTopTabControl
      // 
      this.sideTopTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.sideTopTabControl.CanCloseLastTab = false;
      this.sideTopTabControl.CanReorderTabs = false;
      this.sideTopTabControl.ContentPadding = new System.Windows.Forms.Padding(0);
      this.sideTopTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.sideTopTabControl.HideWhenEmpty = false;
      this.sideTopTabControl.HotTrack = true;
      this.sideTopTabControl.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.sideTopTabControl.ItemSize = new System.Drawing.Size(50, 24);
      this.sideTopTabControl.Location = new System.Drawing.Point(0, 0);
      this.sideTopTabControl.Margin = new System.Windows.Forms.Padding(0);
      this.sideTopTabControl.MaxTabSize = 200;
      this.sideTopTabControl.Name = "sideTopTabControl";
      this.sideTopTabControl.Padding = new System.Drawing.Point(0, 0);
      this.sideTopTabControl.RenderWithGlow = true;
      this.sideTopTabControl.SelectedIndex = 0;
      this.sideTopTabControl.ShowCloseButton = false;
      this.sideTopTabControl.ShowFocusState = true;
      this.sideTopTabControl.Size = new System.Drawing.Size(158, 292);
      this.sideTopTabControl.TabIndex = 1;
      this.sideTopTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      // 
      // sideBottomTabControl
      // 
      this.sideBottomTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.sideBottomTabControl.CanCloseLastTab = false;
      this.sideBottomTabControl.CanReorderTabs = false;
      this.sideBottomTabControl.ContentPadding = new System.Windows.Forms.Padding(0);
      this.sideBottomTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.sideBottomTabControl.HideWhenEmpty = false;
      this.sideBottomTabControl.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.sideBottomTabControl.ItemSize = new System.Drawing.Size(73, 24);
      this.sideBottomTabControl.Location = new System.Drawing.Point(0, 0);
      this.sideBottomTabControl.Margin = new System.Windows.Forms.Padding(0);
      this.sideBottomTabControl.MaxTabSize = 200;
      this.sideBottomTabControl.Name = "sideBottomTabControl";
      this.sideBottomTabControl.RenderWithGlow = true;
      this.sideBottomTabControl.SelectedIndex = 0;
      this.sideBottomTabControl.ShowCloseButton = false;
      this.sideBottomTabControl.ShowFocusState = true;
      this.sideBottomTabControl.Size = new System.Drawing.Size(158, 296);
      this.sideBottomTabControl.TabIndex = 0;
      this.sideBottomTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      // 
      // mainContentSplitContainer
      // 
      this.mainContentSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainContentSplitContainer.Location = new System.Drawing.Point(0, 0);
      this.mainContentSplitContainer.Name = "mainContentSplitContainer";
      // 
      // mainContentSplitContainer.Panel1
      // 
      this.mainContentSplitContainer.Panel1.Controls.Add(this.contentSplitContainer);
      // 
      // mainContentSplitContainer.Panel2
      // 
      this.mainContentSplitContainer.Panel2.Controls.Add(this.secondarySidebarPanel);
      this.mainContentSplitContainer.Size = new System.Drawing.Size(694, 594);
      this.mainContentSplitContainer.SplitterDistance = 493;
      this.mainContentSplitContainer.TabIndex = 14;
      // 
      // contentSplitContainer
      // 
      this.contentSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.contentSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
      this.contentSplitContainer.Location = new System.Drawing.Point(0, 0);
      this.contentSplitContainer.Name = "contentSplitContainer";
      this.contentSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
      // 
      // contentSplitContainer.Panel1
      // 
      this.contentSplitContainer.Panel1.Controls.Add(this.contentHeaderPanel);
      this.contentSplitContainer.Panel1.RightToLeft = System.Windows.Forms.RightToLeft.No;
      // 
      // contentSplitContainer.Panel2
      // 
      this.contentSplitContainer.Panel2.Controls.Add(this.bottomTabControl);
      this.contentSplitContainer.Panel2.RightToLeft = System.Windows.Forms.RightToLeft.No;
      this.contentSplitContainer.Size = new System.Drawing.Size(493, 594);
      this.contentSplitContainer.SplitterDistance = 252;
      this.contentSplitContainer.SplitterWidth = 6;
      this.contentSplitContainer.TabIndex = 1;
      // 
      // bottomTabControl
      // 
      this.bottomTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.bottomTabControl.CanCloseLastTab = true;
      this.bottomTabControl.CanReorderTabs = false;
      this.bottomTabControl.ContentPadding = new System.Windows.Forms.Padding(0, 2, 0, 0);
      this.bottomTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.bottomTabControl.HideWhenEmpty = true;
      this.bottomTabControl.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.bottomTabControl.ItemSize = new System.Drawing.Size(73, 24);
      this.bottomTabControl.Location = new System.Drawing.Point(0, 0);
      this.bottomTabControl.Margin = new System.Windows.Forms.Padding(0);
      this.bottomTabControl.MaxTabSize = 200;
      this.bottomTabControl.Name = "bottomTabControl";
      this.bottomTabControl.RenderWithGlow = true;
      this.bottomTabControl.SelectedIndex = 0;
      this.bottomTabControl.ShowCloseButton = true;
      this.bottomTabControl.ShowFocusState = true;
      this.bottomTabControl.Size = new System.Drawing.Size(493, 336);
      this.bottomTabControl.TabIndex = 0;
      this.bottomTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.TopNormal;
      this.bottomTabControl.TabClosing += new System.EventHandler<MySQL.Controls.TabClosingEventArgs>(this.bottomTabControl_TabClosing);
      this.bottomTabControl.TabClosed += new System.EventHandler<MySQL.Controls.TabClosedEventArgs>(this.bottomTabControl_TabClosed);
      // 
      // secondarySidebarPanel
      // 
      this.secondarySidebarPanel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.secondarySidebarPanel.Dock = System.Windows.Forms.DockStyle.Fill;
      this.secondarySidebarPanel.ForeColor = System.Drawing.Color.White;
      this.secondarySidebarPanel.ForeColorFocused = System.Drawing.Color.White;
      this.secondarySidebarPanel.HeaderColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.secondarySidebarPanel.HeaderColorFocused = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.secondarySidebarPanel.HeaderPadding = new System.Windows.Forms.Padding(5, 0, 5, 0);
      this.secondarySidebarPanel.Location = new System.Drawing.Point(0, 0);
      this.secondarySidebarPanel.Name = "secondarySidebarPanel";
      this.secondarySidebarPanel.Padding = new System.Windows.Forms.Padding(0, 24, 0, 0);
      this.secondarySidebarPanel.Size = new System.Drawing.Size(197, 594);
      this.secondarySidebarPanel.TabIndex = 0;
      this.secondarySidebarPanel.Text = "Modeling Additions";
      // 
      // ModelOverviewForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.AutoScroll = true;
      this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
      this.ClientSize = new System.Drawing.Size(858, 594);
      this.Controls.Add(this.mainSplitContainer);
      this.DoubleBuffered = true;
      this.KeyPreview = true;
      this.Name = "ModelOverviewForm";
      this.TabText = "MySQL Model";
      this.Text = "MySQL Model";
      this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.ModelOverviewForm_FormClosing);
      this.Shown += new System.EventHandler(this.ModelOverviewForm_Shown);
      this.contentHeaderPanel.ResumeLayout(false);
      this.mainSplitContainer.Panel1.ResumeLayout(false);
      this.mainSplitContainer.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).EndInit();
      this.mainSplitContainer.ResumeLayout(false);
      this.sideSplitContainer.Panel1.ResumeLayout(false);
      this.sideSplitContainer.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.sideSplitContainer)).EndInit();
      this.sideSplitContainer.ResumeLayout(false);
      this.mainContentSplitContainer.Panel1.ResumeLayout(false);
      this.mainContentSplitContainer.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.mainContentSplitContainer)).EndInit();
      this.mainContentSplitContainer.ResumeLayout(false);
      this.contentSplitContainer.Panel1.ResumeLayout(false);
      this.contentSplitContainer.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.contentSplitContainer)).EndInit();
      this.contentSplitContainer.ResumeLayout(false);
      this.ResumeLayout(false);

		}

		#endregion

    public MySQL.Controls.HeaderPanel contentHeaderPanel;
    private System.Windows.Forms.SplitContainer mainSplitContainer;
    private System.Windows.Forms.SplitContainer contentSplitContainer;
    private MySQL.Controls.FlatTabControl bottomTabControl;
    private System.Windows.Forms.SplitContainer sideSplitContainer;
    private MySQL.Controls.FlatTabControl sideTopTabControl;
    private MySQL.Controls.FlatTabControl sideBottomTabControl;
    private System.Windows.Forms.Panel scrollPanel;
    private SplitContainer mainContentSplitContainer;
    private Controls.HeaderPanel secondarySidebarPanel;

  }
}