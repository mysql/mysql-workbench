using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;
using System.Windows.Forms;
using System.Reflection;
using System.ComponentModel;

namespace Aga.Controls.Tree.NodeControls
{
	public abstract class BaseTextControl : EditableControl
	{
		private TextFormatFlags _baseFormatFlags;
        private TextFormatFlags _formatFlags;
        private Pen _focusPen;
		private StringFormat _format;

		#region Properties

		private Font _font = null;
		public Font Font
		{
			get
			{
				if (_font == null)
					return Control.DefaultFont;
				else
					return _font;
			}
			set
			{
				if (value == Control.DefaultFont)
					_font = null;
				else
					_font = value;
			}
		}

		protected bool ShouldSerializeFont()
		{
			return (_font != null);
		}

		private HorizontalAlignment _textAlign = HorizontalAlignment.Left;
		[DefaultValue(HorizontalAlignment.Left)]
		public HorizontalAlignment TextAlign
		{
			get { return _textAlign; }
			set 
			{ 
				_textAlign = value;
				SetFormatFlags();
			}
		}

		private StringTrimming _trimming = StringTrimming.None;
		[DefaultValue(StringTrimming.None)]
		public StringTrimming Trimming
		{
			get { return _trimming; }
			set 
			{ 
				_trimming = value;
				SetFormatFlags();
			}
		}

		private bool _displayHiddenContentInToolTip = true;
		[DefaultValue(true)]
		public bool DisplayHiddenContentInToolTip
		{
			get { return _displayHiddenContentInToolTip; }
			set { _displayHiddenContentInToolTip = value; }
		}

		private bool _useCompatibleTextRendering = false;
		[DefaultValue(false)]
		public bool UseCompatibleTextRendering
		{
			get { return _useCompatibleTextRendering; }
			set { _useCompatibleTextRendering = value; }
		}

		#endregion

		protected BaseTextControl()
		{
			IncrementalSearchEnabled = true;
			_focusPen = new Pen(Color.Black);
			_focusPen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;

      // ml: added flags for vertical centering and no wrapping.
			_format = new StringFormat(StringFormatFlags.NoClip | StringFormatFlags.FitBlackBox |
        StringFormatFlags.MeasureTrailingSpaces | StringFormatFlags.NoWrap);
			_baseFormatFlags = TextFormatFlags.PreserveGraphicsClipping | TextFormatFlags.VerticalCenter |
                           TextFormatFlags.PreserveGraphicsTranslateTransform;
			SetFormatFlags();
			LeftMargin = 3;
		}

		private void SetFormatFlags()
		{
			_format.Alignment = TextHelper.TranslateAligment(TextAlign);
			_format.Trimming = Trimming;
      _format.LineAlignment = StringAlignment.Center;

			_formatFlags = _baseFormatFlags | TextHelper.TranslateAligmentToFlag(TextAlign)
				| TextHelper.TranslateTrimmingToFlag(Trimming);
		}

		public override Size MeasureSize(TreeNodeAdv node, DrawContext context)
		{
			return GetLabelSize(node, context);
		}

		protected Size GetLabelSize(TreeNodeAdv node, DrawContext context)
		{
			return GetLabelSize(node, context, GetLabel(node));
		}

		protected Size GetLabelSize(TreeNodeAdv node, DrawContext context, string label)
		{
			CheckThread();
			Font font = GetDrawingFont(node, context);
			Size s = Size.Empty;
			if (UseCompatibleTextRendering)
				s = TextRenderer.MeasureText(label, font);
			else
			{
				SizeF sf = context.Graphics.MeasureString(label, font);
				s = new Size((int)Math.Ceiling(sf.Width), (int)Math.Ceiling(sf.Height)); 
			}

			if (!s.IsEmpty)
				return s;
			else
				return new Size(10, Font.Height);
		}

		protected Font GetDrawingFont(TreeNodeAdv node, DrawContext context)
		{
			Font font = context.Font;
			//if (DrawText != null) ml: don't check in advance, descendents might override OnDrawText
			{
				DrawEventArgs args = new DrawEventArgs(node, context);
				args.Font = context.Font;
				OnDrawText(args);
				font = args.Font;
			}
			return font;
		}

    protected virtual void SetEditControlProperties(Control control, TreeNodeAdv node)
		{
			DrawContext context = new DrawContext();
			context.Font = control.Font;
			control.Font = GetDrawingFont(node, context);
		}

		public override void Draw(TreeNodeAdv node, DrawContext context)
		{
			if (context.CurrentEditorOwner == this && node == Parent.CurrentNode)
				return;

			string label = GetLabel(node);
			Rectangle bounds = GetBounds(node, context);
			Rectangle focusRect = new Rectangle(bounds.X, context.Bounds.Y,	
				bounds.Width, context.Bounds.Height);

			Brush backgroundBrush;
			Color textColor;
			Font font;
			CreateBrushes(node, context, out backgroundBrush, out textColor, out font, ref label);

			if (backgroundBrush != null)
				context.Graphics.FillRectangle(backgroundBrush, focusRect);
			if (context.DrawFocus)
			{
				focusRect.Width--;
				focusRect.Height--;
				if (context.DrawSelection == DrawSelectionMode.None)
					_focusPen.Color = SystemColors.ControlText;
				else
					_focusPen.Color = SystemColors.InactiveCaption;
				context.Graphics.DrawRectangle(_focusPen, focusRect);
			}

      // ml: vertically center text.
      bounds.Height = context.Bounds.Height;
			if (UseCompatibleTextRendering)
				TextRenderer.DrawText(context.Graphics, label, font, bounds, textColor, _formatFlags);
			else
			{
				Brush textBrush = new SolidBrush(textColor);
				context.Graphics.DrawString(label, font, textBrush, bounds, _format);
				textBrush.Dispose();
			}
		}

		private void CreateBrushes(TreeNodeAdv node, DrawContext context, out Brush backgroundBrush, out Color textColor, out Font font, ref string label)
		{
			textColor = SystemColors.ControlText;
			backgroundBrush = null;
			font = context.Font;

      switch (context.DrawSelection)
      {
        case DrawSelectionMode.Active:
          if (TreeViewAdv.isWin8OrAbove)
          {
            textColor = SystemColors.ControlText;
            backgroundBrush = new SolidBrush(Color.FromArgb(0xFF, 0xD1, 0xE8, 0xFF));
          }
          else
          {
            textColor = SystemColors.HighlightText;
            backgroundBrush = SystemBrushes.Highlight;
          }
          break;
        case DrawSelectionMode.Inactive:
          textColor = SystemColors.ControlText;
          backgroundBrush = TreeViewAdv.isWin8OrAbove ? new SolidBrush(Color.FromArgb(0xFF, 0xF7, 0xF7, 0xF7)) : SystemBrushes.InactiveBorder;
          break;
        case DrawSelectionMode.FullRowSelect:
          //textColor = SystemColors.HighlightText;
          break;
      }

			if (!context.Enabled)
				textColor = SystemColors.GrayText;

			//if (DrawText != null) // ml: don't check in advance, descendents might overright OnDrawText
			{
				DrawEventArgs args = new DrawEventArgs(node, context);
				args.Text = label;
				args.TextColor = textColor;
				args.BackgroundBrush = backgroundBrush;
				args.Font = font;

				OnDrawText(args);

				textColor = args.TextColor;
				backgroundBrush = args.BackgroundBrush;
				font = args.Font;
				label = args.Text;
			}
		}

		public string GetLabel(TreeNodeAdv node)
		{
			if (node != null && node.Tag != null)
			{
				object obj = GetValue(node);
				if (obj != null)
					return FormatLabel(obj);
			}
			return string.Empty;
		}

		protected virtual string FormatLabel(object obj)
		{
			return obj.ToString();
		}

		public void SetLabel(TreeNodeAdv node, string value)
		{
			SetValue(node, value);
		}

		protected override void Dispose(bool disposing)
		{
			base.Dispose(disposing);
			if (disposing)
			{
				_focusPen.Dispose();
				_format.Dispose();
			}
		}

		/// <summary>
		/// Fires when control is going to draw a text. Can be used to change text or back color
		/// </summary>
		public event EventHandler<DrawEventArgs> DrawText;
		protected virtual void OnDrawText(DrawEventArgs args)
		{
			if (DrawText != null)
				DrawText(this, args);
		}
	}
}
