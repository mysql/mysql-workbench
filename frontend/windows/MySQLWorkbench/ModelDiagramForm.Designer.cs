namespace MySQL.GUI.Workbench
{
    partial class ModelDiagramForm
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
          // Removes control in the secondarySidebar as it is 
          // shared among all the model tabs and should not be
          // disposed but on the model overview.
          secondarySidebarPanel.Controls.Clear();

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
      this.canvasViewer = new MySQL.Utilities.WindowsCanvasViewer();
      this.diagramPanel = new MySQL.Controls.HeaderPanel();
      this.mainSplitContainer = new System.Windows.Forms.SplitContainer();
      this.sideSplitContainer = new System.Windows.Forms.SplitContainer();
      this.sideTopTabControl = new MySQL.Controls.FlatTabControl();
      this.sideBottomTabControl = new MySQL.Controls.FlatTabControl();
      this.navigatorHost = new MySQL.Controls.HeaderPanel();
      this.mainContentSplitContainer = new System.Windows.Forms.SplitContainer();
      this.contentSplitContainer = new System.Windows.Forms.SplitContainer();
      this.bottomTabControl = new MySQL.Controls.FlatTabControl();
      this.secondarySidebarPanel = new MySQL.Controls.HeaderPanel();
      this.diagramPanel.SuspendLayout();
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
      // canvasViewer
      // 
      this.canvasViewer.AllowDrop = true;
      this.canvasViewer.AutoScroll = true;
      this.canvasViewer.BackColor = System.Drawing.Color.White;
      this.canvasViewer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.canvasViewer.Location = new System.Drawing.Point(0, 24);
      this.canvasViewer.Name = "canvasViewer";
      this.canvasViewer.OwnerForm = null;
      this.canvasViewer.Size = new System.Drawing.Size(400, 493);
      this.canvasViewer.TabIndex = 0;
      this.canvasViewer.DragDrop += new System.Windows.Forms.DragEventHandler(this.canvasViewer_DragDrop);
      this.canvasViewer.DragEnter += new System.Windows.Forms.DragEventHandler(this.canvasViewer_DragEnter);
      // 
      // diagramPanel
      // 
      this.diagramPanel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.diagramPanel.Controls.Add(this.canvasViewer);
      this.diagramPanel.Dock = System.Windows.Forms.DockStyle.Fill;
      this.diagramPanel.ForeColor = System.Drawing.Color.White;
      this.diagramPanel.ForeColorFocused = System.Drawing.Color.White;
      this.diagramPanel.HeaderColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.diagramPanel.HeaderColorFocused = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.diagramPanel.HeaderPadding = new System.Windows.Forms.Padding(5, 0, 5, 0);
      this.diagramPanel.Location = new System.Drawing.Point(0, 0);
      this.diagramPanel.Name = "diagramPanel";
      this.diagramPanel.Padding = new System.Windows.Forms.Padding(0, 24, 0, 0);
      this.diagramPanel.Size = new System.Drawing.Size(400, 517);
      this.diagramPanel.TabIndex = 8;
      this.diagramPanel.Text = "Diagram";
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
      this.mainSplitContainer.Panel1.Controls.Add(this.navigatorHost);
      this.mainSplitContainer.Panel1MinSize = 0;
      // 
      // mainSplitContainer.Panel2
      // 
      this.mainSplitContainer.Panel2.Controls.Add(this.mainContentSplitContainer);
      this.mainSplitContainer.Size = new System.Drawing.Size(776, 517);
      this.mainSplitContainer.SplitterDistance = 201;
      this.mainSplitContainer.SplitterWidth = 6;
      this.mainSplitContainer.TabIndex = 14;
      this.mainSplitContainer.TabStop = false;
      this.mainSplitContainer.SplitterMoving += new System.Windows.Forms.SplitterCancelEventHandler(this.splitterMoving);
      this.mainSplitContainer.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.mainSplitContainer_SplitterMoved);
      // 
      // sideSplitContainer
      // 
      this.sideSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.sideSplitContainer.Location = new System.Drawing.Point(0, 214);
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
      this.sideSplitContainer.Size = new System.Drawing.Size(201, 303);
      this.sideSplitContainer.SplitterDistance = 148;
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
      this.sideTopTabControl.Size = new System.Drawing.Size(201, 148);
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
      this.sideBottomTabControl.Size = new System.Drawing.Size(201, 149);
      this.sideBottomTabControl.TabIndex = 0;
      this.sideBottomTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      // 
      // navigatorHost
      // 
      this.navigatorHost.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.navigatorHost.Dock = System.Windows.Forms.DockStyle.Top;
      this.navigatorHost.ForeColor = System.Drawing.Color.White;
      this.navigatorHost.ForeColorFocused = System.Drawing.Color.White;
      this.navigatorHost.HeaderColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.navigatorHost.HeaderColorFocused = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.navigatorHost.HeaderPadding = new System.Windows.Forms.Padding(5, 0, 5, 0);
      this.navigatorHost.Location = new System.Drawing.Point(0, 0);
      this.navigatorHost.Margin = new System.Windows.Forms.Padding(0);
      this.navigatorHost.Name = "navigatorHost";
      this.navigatorHost.Padding = new System.Windows.Forms.Padding(0, 24, 0, 6);
      this.navigatorHost.Size = new System.Drawing.Size(201, 214);
      this.navigatorHost.TabIndex = 0;
      this.navigatorHost.Text = "Bird\'s Eye";
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
      this.mainContentSplitContainer.Size = new System.Drawing.Size(569, 517);
      this.mainContentSplitContainer.SplitterDistance = 400;
      this.mainContentSplitContainer.SplitterWidth = 6;
      this.mainContentSplitContainer.TabIndex = 15;
      this.mainContentSplitContainer.SplitterMoving += new System.Windows.Forms.SplitterCancelEventHandler(this.splitterMoving);
      this.mainContentSplitContainer.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.mainContentSplitContainer_SplitterMoved);
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
      this.contentSplitContainer.Panel1.Controls.Add(this.diagramPanel);
      // 
      // contentSplitContainer.Panel2
      // 
      this.contentSplitContainer.Panel2.Controls.Add(this.bottomTabControl);
      this.contentSplitContainer.Panel2Collapsed = true;
      this.contentSplitContainer.Size = new System.Drawing.Size(400, 517);
      this.contentSplitContainer.SplitterDistance = 294;
      this.contentSplitContainer.SplitterWidth = 6;
      this.contentSplitContainer.TabIndex = 1;
      // 
      // bottomTabControl
      // 
      this.bottomTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.bottomTabControl.CanCloseLastTab = true;
      this.bottomTabControl.CanReorderTabs = false;
      this.bottomTabControl.ContentPadding = new System.Windows.Forms.Padding(0, 4, 0, 4);
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
      this.bottomTabControl.Size = new System.Drawing.Size(150, 46);
      this.bottomTabControl.TabIndex = 0;
      this.bottomTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.TopNormal;
      this.bottomTabControl.TabClosing += new System.EventHandler<MySQL.Controls.TabClosingEventArgs>(this.tabControl_TabClosing);
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
      this.secondarySidebarPanel.Size = new System.Drawing.Size(163, 517);
      this.secondarySidebarPanel.TabIndex = 0;
      this.secondarySidebarPanel.Text = "Modeling Additions";
      // 
      // ModelDiagramForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(776, 517);
      this.Controls.Add(this.mainSplitContainer);
      this.Name = "ModelDiagramForm";
      this.TabText = "ModelView";
      this.Text = "ModelView";
      this.Shown += new System.EventHandler(this.ModelDiagramForm_Shown);
      this.diagramPanel.ResumeLayout(false);
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

        private MySQL.Utilities.WindowsCanvasViewer canvasViewer;
      private MySQL.Controls.HeaderPanel diagramPanel;
      private System.Windows.Forms.SplitContainer mainSplitContainer;
      private System.Windows.Forms.SplitContainer contentSplitContainer;
      private MySQL.Controls.FlatTabControl bottomTabControl;
      private System.Windows.Forms.SplitContainer sideSplitContainer;
      private MySQL.Controls.FlatTabControl sideTopTabControl;
      private MySQL.Controls.FlatTabControl sideBottomTabControl;
      private MySQL.Controls.HeaderPanel navigatorHost;
      private System.Windows.Forms.SplitContainer mainContentSplitContainer;
      private Controls.HeaderPanel secondarySidebarPanel;

			}
}