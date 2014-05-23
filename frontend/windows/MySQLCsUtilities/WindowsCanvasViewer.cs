/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

using System;
using System.Windows.Forms;

using MySQL.GUI.Mdc;

namespace MySQL.Utilities
{
  public class WindowsCanvasViewer : Panel
  {
    #region Member Variables

    private WindowsCanvasViewerPanel canvasPanel = null;

    #endregion

    #region Constructor

    public WindowsCanvasViewer()
    {
      canvasPanel = new WindowsCanvasViewerPanel();

      canvasPanel.VScrollbar = new VScrollBar();
      canvasPanel.HScrollbar = new HScrollBar();

      canvasPanel.VScrollbar.Scroll += new ScrollEventHandler(canvasPanel.HandleScroll);
      canvasPanel.HScrollbar.Scroll += new ScrollEventHandler(canvasPanel.HandleScroll);

      Controls.Add(canvasPanel.VScrollbar);
      Controls.Add(canvasPanel.HScrollbar);
      Controls.Add(canvasPanel);

      canvasPanel.Dock = DockStyle.Fill;
      canvasPanel.HScrollbar.Dock = DockStyle.Bottom;
      canvasPanel.VScrollbar.Dock = DockStyle.Right;

      canvasPanel.Width = ClientSize.Width - canvasPanel.VScrollbar.Width;
      canvasPanel.Height = ClientSize.Height - canvasPanel.HScrollbar.Height;
    }

    protected override void Dispose(bool disposing)
    {
      canvasPanel.Dispose();
      base.Dispose(disposing);
    }

    #endregion

    #region Properties

    public BaseWindowsCanvasView Canvas
    {
      get { return canvasPanel.Canvas; }
    }

    public Form OwnerForm
    {
      get { return canvasPanel.OwnerForm; }
      set { canvasPanel.OwnerForm = value; }
    }

    public Panel CanvasPanel
    {
      get { return canvasPanel; }
    }

    #endregion

    #region Public Functions

    /// <summary>
    /// Initializes a new canvas viewer. Normally we use OpenGL for rendering, but this can be
    /// switched off by the user (either via the application options or via command line).
    /// The opposite force switch can override the sw rendering switch if the user really wants this.
    /// It is usually only used to override forced sw rendering for certain chip sets.
    /// </summary>
    /// <param name="ownerForm">The hosting WB form for the view.</param>
    /// <param name="payload">Additionaly info passed on to the backend canvas.</param>
    /// <param name="handleInput">True if the view should act to user input (mouse/keyboard).</param>
    /// <returns>The newly created canvas view</returns>
    public BaseWindowsCanvasView CreateCanvasView(Form ownerForm, IntPtr payload, bool handleInput,
      bool software_rendering_enforced, bool opengl_rendering_enforced)
    {
      //Logger.LogDebug();

      BaseWindowsCanvasView result = null;

      if (!software_rendering_enforced || opengl_rendering_enforced)
        result = canvasPanel.CreateGLCanvas(ownerForm, payload, handleInput);

      if (result == null)
        result = canvasPanel.CreateGDICanvas(ownerForm, payload, handleInput);

      return result;
    }

    public void FinalizeCanvas()
    {
      canvasPanel.FinalizeCanvas();
    }

    #endregion
    
  }

  public class WindowsCanvasViewerPanel : Panel, MySQL.Forms.ICanvasViewer
	{
		#region Member Variables

    private ScrollBar vScrollbar= null;
    private ScrollBar hScrollbar= null;

		private BaseWindowsCanvasView canvas = null;
		private bool canvasInitialized = false;
	
    private bool scrolling = false;

    private bool handleInput = false;

    public ToolStripLabel canvasFPSLabel = null;

		#endregion

    #region Constructor

    public WindowsCanvasViewerPanel()
    {
      Click += new EventHandler(ScrollablePanel_Click);
      handleInput = true;
    }

    #endregion

    #region Public Functions

    public WindowsGLCanvasView CreateGLCanvas(Form ownerForm, IntPtr payload, bool handleInput)
    {
      canvas = new WindowsGLCanvasView(Handle, payload, ClientRectangle.Width, ClientRectangle.Height);
      if (!canvas.initialize())
      {
        // OpenGL initialization can fail.
        canvas.Dispose();
        return null;
      }

      this.handleInput = handleInput;

      canvas.set_on_queue_repaint(OnNeedsRepaint);
      canvas.set_on_viewport_changed(OnViewportChanged);
      OwnerForm = ownerForm;
      canvasInitialized = true;

      UpdateScrollBarSizes();
      return (WindowsGLCanvasView)canvas;
    }

    public WindowsGDICanvasView CreateGDICanvas(Form ownerForm, IntPtr payload, bool handleInput)
    {
      SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.Opaque |
        ControlStyles.OptimizedDoubleBuffer, true);
      UpdateStyles();

      canvas = new WindowsGDICanvasView(Handle, payload, ClientRectangle.Width, ClientRectangle.Height);
      if (!canvas.initialize())
      {
        // Should never happen.
        canvas.Dispose();
        return null;
      }

      this.handleInput = handleInput;

      canvas.set_on_queue_repaint(OnNeedsRepaint);
      canvas.set_on_viewport_changed(OnViewportChanged);
      OwnerForm = ownerForm;
      canvasInitialized = true;

      UpdateScrollBarSizes();

      return (WindowsGDICanvasView)canvas;
    }

    public void FinalizeCanvas()
    {
      canvasInitialized = false;
      canvas.SetOwnerForm(null);
      canvas.Dispose();
      canvas = null;
    }

    #endregion

    #region Properties

    public BaseWindowsCanvasView Canvas
    {
      get { return canvas; }
    }

    public Form OwnerForm
    {
      get
      {
        if (canvas != null)
          return canvas.GetOwnerForm();
        else
          return null;
      }
      set
      {
        if (canvas != null)
          canvas.SetOwnerForm(value);
      }
    }

    public ScrollBar VScrollbar
    {
      get { return vScrollbar; }
      set { vScrollbar = value; }
    }

    public ScrollBar HScrollbar
    {
      get { return hScrollbar; }
      set { hScrollbar = value; }
    }

    #endregion

    #region Event Handling

		private void OnNeedsRepaint(int x, int y, int w, int h)
		{
			Invalidate(new System.Drawing.Rectangle(x, y, w, h));
		}

    private void OnViewportChanged()
    {
      if (scrolling)
        return;
      UpdateScrollbars();
      canvas.scroll_to(hScrollbar.Value, vScrollbar.Value);
      Refresh();
    }

    protected override void OnMouseMove(MouseEventArgs e)
    {
      if (canvasInitialized && handleInput)
        canvas.OnMouseMove(e, ModifierKeys, MouseButtons);
      base.OnMouseMove(e);
    }

    protected override void OnMouseDown(MouseEventArgs e)
    {
      if (canvasInitialized && handleInput)
        canvas.OnMouseDown(e, ModifierKeys, MouseButtons);
      base.OnMouseDown(e);
    }

    protected override void OnMouseUp(MouseEventArgs e)
    {
      if (canvasInitialized && handleInput)
        canvas.OnMouseUp(e, ModifierKeys, MouseButtons);
      base.OnMouseUp(e);
    }

    protected override void OnMouseDoubleClick(MouseEventArgs e)
    {
      if (canvasInitialized && handleInput)
        canvas.OnMouseDoubleClick(e, ModifierKeys, MouseButtons);
      base.OnMouseDoubleClick(e);
    }

    protected override void OnMouseWheel(MouseEventArgs e)
    {
      if (canvasInitialized)
      {
        if ((ModifierKeys & Keys.Alt) != 0)
        {
          // Zoom change.
          if (e.Delta > 0)
          {
            if (canvas.Zoom < 2)
              canvas.Zoom = canvas.Zoom + 0.01f;
          }
          else
          {
            if (canvas.Zoom > 0.1f)
              canvas.Zoom = canvas.Zoom - 0.01f;
          }
        }
        else
        {
          // Scroll change.
          double x, y, w, h;
          canvas.get_viewport(out x, out y, out w, out h);

          if ((ModifierKeys & Keys.Control) != 0)
            x -= e.Delta / 5;
          else
            y -= e.Delta / 5;

          if (y < 0)
            y = 0;
          else
            if (y > vScrollbar.Maximum) y = vScrollbar.Maximum;
          if (x < 0)
            x = 0;
          else
            if (x > hScrollbar.Maximum) x = hScrollbar.Maximum;

          if (vScrollbar.Value != (int)y)
            vScrollbar.Value = (int)y;
          if (hScrollbar.Value != (int)x)
            hScrollbar.Value = (int)x;

          HandleScroll(null, null);
        }
      }
      base.OnMouseWheel(e);
    }

    protected override void OnKeyDown(KeyEventArgs e)
    {
      if (canvasInitialized)
        canvas.OnKeyDown(e, ModifierKeys);
      base.OnKeyDown(e);
    }

    protected override void OnKeyUp(KeyEventArgs e)
    {
      if (canvasInitialized)
        canvas.OnKeyUp(e, ModifierKeys);
      base.OnKeyUp(e);
    }

    protected override void OnSizeChanged(EventArgs e)
    {
      base.OnSizeChanged(e);
      if (canvasInitialized)
        canvas.OnSizeChanged(ClientRectangle.Width, ClientRectangle.Height);
    }

		private void ScrollablePanel_Click(object sender, EventArgs e)
		{
			this.Focus();
		}

		#endregion

    #region Drawing

    protected override void OnPaintBackground(PaintEventArgs e)
    {
      // Don't do anything to avoid flickering.
      if (canvas == null)
        base.OnPaintBackground(e);
    }


    protected override void OnPaint(PaintEventArgs e)
    {
      try
      {
        if (canvasInitialized)
        {
          IntPtr hdc = e.Graphics.GetHdc();
          canvas.repaint(hdc, e.ClipRectangle.Left, e.ClipRectangle.Top,
            e.ClipRectangle.Width, e.ClipRectangle.Height);
          e.Graphics.ReleaseHdc();

          if (canvasFPSLabel != null)
            canvasFPSLabel.Text = String.Format("{0:0.00} fps", canvas.get_fps());
        }
      }
      catch (Exception exc)
      {
        MessageBox.Show(exc.Message);
      }
    }

    #endregion

    #region Other implementation

    private void UpdateScrollbars()
    {
      UpdateScrollBarSizes();
      UpdateScrollBarPositions();
    }

    private void UpdateScrollBarPositions()
    {
      double x, y, w, h;

      if (canvas != null)
      {
        canvas.get_viewport(out x, out y, out w, out h);

        if (y < 0)
          y = 0;
        else
          if (y > vScrollbar.Maximum)
            y = vScrollbar.Maximum;

        if (x < 0)
          x = 0;
        else if (x > hScrollbar.Maximum)
          x = hScrollbar.Maximum;

        if (vScrollbar.Value != (int)y)
          vScrollbar.Value = (int)y;
        if (hScrollbar.Value != (int)x)
          hScrollbar.Value = (int)x;
      }
    }

    private void UpdateScrollBarSizes()
    {
      double x, y, w, h;
      double total_w, total_h;

      if (canvas != null)
      {
        canvas.get_total_view_size(out total_w, out total_h);

        canvas.get_viewport(out x, out y, out w, out h);

        vScrollbar.Minimum = 0;
        hScrollbar.Minimum = 0;

        vScrollbar.Visible = (total_h > h);
        vScrollbar.Maximum = (int)(total_h);
        vScrollbar.SmallChange = ClientSize.Height / 20;
        vScrollbar.LargeChange = (int)(h);

        hScrollbar.Visible = (total_w > w);
        hScrollbar.Maximum = (int)(total_w);
        hScrollbar.SmallChange = ClientSize.Width / 20;
        hScrollbar.LargeChange = (int)(w);
      }
    }

    public void HandleScroll(Object sender, ScrollEventArgs args)
    {
      scrolling = true;
      canvas.scroll_to(hScrollbar.Value, vScrollbar.Value);
      UpdateScrollBarPositions();
      scrolling = false;
      Update();
    }

    public void DoMouseMove(MouseEventArgs e)
    {
      if (canvasInitialized)
        canvas.OnMouseMove(e, ModifierKeys, MouseButtons);
    }


    #endregion

    public Control control()
    {
      return this;
    }
  }
}
