namespace MySQL.GUI.Workbench.Plugins
{
  partial class ObjectEditorView
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
      this.editorContextMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
      this.undoItem = new System.Windows.Forms.ToolStripMenuItem();
      this.redoItem = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
      this.cutItem = new System.Windows.Forms.ToolStripMenuItem();
      this.copyItem = new System.Windows.Forms.ToolStripMenuItem();
      this.pasteItem = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
      this.selectAllItem = new System.Windows.Forms.ToolStripMenuItem();
      this.label1 = new System.Windows.Forms.Label();
      this.labelTooltip = new System.Windows.Forms.ToolTip(this.components);
      this.editorContextMenu.SuspendLayout();
      this.SuspendLayout();
      // 
      // editorContextMenu
      // 
      this.editorContextMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.undoItem,
            this.redoItem,
            this.toolStripSeparator2,
            this.cutItem,
            this.copyItem,
            this.pasteItem,
            this.toolStripSeparator3,
            this.selectAllItem});
      this.editorContextMenu.Name = "editorContextMenu";
      this.editorContextMenu.Size = new System.Drawing.Size(175, 148);
      // 
      // undoItem
      // 
      this.undoItem.Name = "undoItem";
      this.undoItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Z)));
      this.undoItem.Size = new System.Drawing.Size(174, 22);
      this.undoItem.Text = "Undo";
      // 
      // redoItem
      // 
      this.redoItem.Name = "redoItem";
      this.redoItem.ShortcutKeys = ((System.Windows.Forms.Keys)(((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift)
                  | System.Windows.Forms.Keys.Z)));
      this.redoItem.Size = new System.Drawing.Size(174, 22);
      this.redoItem.Text = "Redo";
      // 
      // toolStripSeparator2
      // 
      this.toolStripSeparator2.Name = "toolStripSeparator2";
      this.toolStripSeparator2.Size = new System.Drawing.Size(171, 6);
      // 
      // cutItem
      // 
      this.cutItem.Name = "cutItem";
      this.cutItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.X)));
      this.cutItem.Size = new System.Drawing.Size(174, 22);
      this.cutItem.Text = "Cut";
      // 
      // copyItem
      // 
      this.copyItem.Name = "copyItem";
      this.copyItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.C)));
      this.copyItem.Size = new System.Drawing.Size(174, 22);
      this.copyItem.Text = "Copy";
      // 
      // pasteItem
      // 
      this.pasteItem.Name = "pasteItem";
      this.pasteItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.V)));
      this.pasteItem.Size = new System.Drawing.Size(174, 22);
      this.pasteItem.Text = "Paste";
      // 
      // toolStripSeparator3
      // 
      this.toolStripSeparator3.Name = "toolStripSeparator3";
      this.toolStripSeparator3.Size = new System.Drawing.Size(171, 6);
      // 
      // selectAllItem
      // 
      this.selectAllItem.Name = "selectAllItem";
      this.selectAllItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.A)));
      this.selectAllItem.Size = new System.Drawing.Size(174, 22);
      this.selectAllItem.Text = "Select All";
      // 
      // label1
      // 
      this.label1.Location = new System.Drawing.Point(62, 5);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(66, 20);
      this.label1.TabIndex = 13;
      this.label1.Text = "Name:";
      this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // labelTooltip
      // 
      this.labelTooltip.IsBalloon = true;
      this.labelTooltip.ToolTipIcon = System.Windows.Forms.ToolTipIcon.Info;
      this.labelTooltip.ToolTipTitle = "Parser Information";
      // 
      // ObjectEditorView
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.ClientSize = new System.Drawing.Size(834, 464);
      this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.Name = "ObjectEditorView";
      this.Padding = new System.Windows.Forms.Padding(4);
      this.Text = "ObjectEditorView";
      this.editorContextMenu.ResumeLayout(false);
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.ToolTip labelTooltip;
    private System.Windows.Forms.ContextMenuStrip editorContextMenu;
    private System.Windows.Forms.ToolStripMenuItem undoItem;
    private System.Windows.Forms.ToolStripMenuItem redoItem;
    private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
    private System.Windows.Forms.ToolStripMenuItem cutItem;
    private System.Windows.Forms.ToolStripMenuItem copyItem;
    private System.Windows.Forms.ToolStripMenuItem pasteItem;
    private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
    private System.Windows.Forms.ToolStripMenuItem selectAllItem;
  }
}