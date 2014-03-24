using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;

namespace Aga.Controls.Tree.NodeControls
{
  public class NodeCustomBox : BaseTextControl
  {
    //-----------------------------------------------------------------------------
    // COMMON
    //-----------------------------------------------------------------------------
    public enum EditMethod
    {
      Text,
      LongText,
      Bool,
      DateTime,
      Date,
      Time,
      File,
      Color,
      Numeric,
    }

    private EditMethod _editMethod = EditMethod.Text;
    private bool _cancelEdit = false;
    private bool _validated = false;
    private MaskedTextBox _maskedTextBox;

    public delegate bool IsNodeReadonlyDelegate(TreeNodeAdv node);
    public IsNodeReadonlyDelegate IsNodeReadonly;
    private bool IsReadonly(TreeNodeAdv node)
    {
      return (IsNodeReadonly == null) ? false : IsNodeReadonly(node);
    }

    public delegate EditMethod GetNodeEditMethodDelegate(TreeNodeAdv node);
    public GetNodeEditMethodDelegate GetNodeEditMethod;
    private EditMethod GetEditMethod(TreeNodeAdv node)
    {
      return (GetNodeEditMethod == null) ? EditMethod.Text : GetNodeEditMethod(node);
    }

    public delegate void BeforeEditDelegate(TreeNodeAdv node, NodeCustomBox customBox);
    public BeforeEditDelegate BeforeEdit;
    private void OnBeforeEdit(TreeNodeAdv node)
    {
      if (null != BeforeEdit)
        BeforeEdit(node, this);
    }

    public delegate void BeforeApplyDelegate(TreeNodeAdv node, ref string value, EditMethod editMethod);
    public BeforeApplyDelegate BeforeApply;
    private void OnBeforeApply(TreeNodeAdv node, ref string value, EditMethod editMethod)
    {
      if (null != BeforeApply)
        BeforeApply(node, ref value, editMethod);
    }

    public NodeCustomBox()
    {
      EditorHided += new EventHandler(EditorHided_);
      IsEditEnabledValueNeeded += new EventHandler<NodeControlValueEventArgs>(IsEditEnabledValueNeeded_);
    }

    public override void Draw(TreeNodeAdv node, DrawContext context)
    {
      /* //! disable background highlighting of readonly members
      if (IsNodeReadonly(node))
      {
        Brush brush = new SolidBrush(Color.LightGray);
        context.Graphics.FillRectangle(brush, context.Bounds);
        brush.Dispose();
      }*/

      EditMethod editMethod = GetEditMethod(node);
      switch (editMethod)
      {
        case EditMethod.Color:
          Color_Draw(node, ref context);
          break;
      }
      base.Draw(node, context);
    }

    public void Color_Draw(TreeNodeAdv node, ref DrawContext context)
    {
      // draw color rectangle visualizing chosen color
      {
        Rectangle bounds = context.Bounds;
        int colorRectPaddingX = LeftMargin;
        int colorRectPaddingY = LeftMargin;
        int colorRectWidth = Math.Max(0, bounds.Width - 2 * colorRectPaddingX);
        int colorRectHeight = Math.Max(0, bounds.Height - 2 * colorRectPaddingY);
        int colorRectSide = (int)(1.5 * colorRectHeight);
        Rectangle colorRect = new Rectangle(bounds.X + 2 * colorRectPaddingX, bounds.Y + colorRectPaddingY,
          Math.Min(colorRectWidth, colorRectSide),
          Math.Min(colorRectHeight, colorRectSide));

        Color color;
        // determine color
        try
        {
          string value = GetValue(node) as string;
          if (value!="" && value[0] == '#')
            color = System.Drawing.ColorTranslator.FromHtml(value);
          else
            color = Color.FromArgb(0);
        }
        catch (Exception)
        {
          color = Color.FromArgb(0);
        }

        if (0 != color.A)
        {
          Brush brush = new SolidBrush(color);
          context.Graphics.FillRectangle(brush, colorRect);
          brush.Dispose();
        }
        else
        {
          Pen pen = new Pen(SystemColors.ControlText);
          pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Solid;
          context.Graphics.DrawRectangle(pen, colorRect);
          Point[] points = new Point[4] {
            colorRect.Location, new Point(colorRect.Right, colorRect.Bottom),
            new Point(colorRect.Right, colorRect.Y), new Point(colorRect.X, colorRect.Bottom),
          };
          context.Graphics.DrawLines(pen, points);
          pen.Dispose();
        }

        // shift & shrink bounds for text value, representing color
        int diffX = Math.Min(bounds.Width, colorRectPaddingX + colorRectSide);
        bounds.X += diffX;
        bounds.Width -= diffX;
        context.Bounds = bounds;
      }
    }

    protected override Size CalculateEditorSize(EditorContext context)
    {
      switch (_editMethod)
      {
        case EditMethod.Numeric: return Numeric_CalculateEditorSize(context);
        case EditMethod.Bool: return ComboBox_CalculateEditorSize(context);
        case EditMethod.Date:
        case EditMethod.Time:
        case EditMethod.DateTime: return DateTimePicker_CalculateEditorSize(context);
        case EditMethod.LongText:
        case EditMethod.Color:
        case EditMethod.File:
          return CustomEditor_CalculateEditorSize(context);
        default: return TextBox_CalculateEditorSize(context);
      }
    }

    protected void EditorHided_(object sender, EventArgs e)
    {
      _maskedTextBox = null;
    }

    private void IsEditEnabledValueNeeded_(object sender, NodeControlValueEventArgs args)
    {
      EditMethod editMethod = GetEditMethod(args.Node);
      switch (_editMethod)
      {
        case EditMethod.Bool:
        case EditMethod.Date:
        case EditMethod.Time:
        case EditMethod.DateTime:
        case EditMethod.Color:
        case EditMethod.File:
      args.Value = !IsReadonly(args.Node);
          break;

        case EditMethod.Numeric:
        case EditMethod.LongText:
          break;
      }
    }

    protected override Control CreateEditor(TreeNodeAdv node)
    {
      if (null != CurrentEditor)
        return null;

      _editMethod = GetEditMethod(node);
      _cancelEdit = false;
      
      OnBeforeEdit(node);

      switch (_editMethod)
      {
        case EditMethod.Numeric: return Numeric_CreateEditor(node);
        case EditMethod.Bool: return ComboBox_CreateEditor(node);
        case EditMethod.Date:
        case EditMethod.Time:
        case EditMethod.DateTime: return DateTimePicker_CreateEditor(node);
        case EditMethod.LongText:
        case EditMethod.Color:
        case EditMethod.File:
          return CustomEditor_CreateEditor(node);
        default: return TextBox_CreateEditor(node);
      }
    }

    protected override void DoApplyChanges(TreeNodeAdv node, Control editor)
    {
      switch (_editMethod)
      {
        case EditMethod.Numeric: Numeric_DoApplyChanges(node, editor); break;
        case EditMethod.Bool: ComboBox_DoApplyChanges(node, editor); break;
        case EditMethod.Date:
        case EditMethod.Time:
        case EditMethod.DateTime: DateTimePicker_DoApplyChanges(node, editor); break;
        case EditMethod.LongText:
        case EditMethod.Color:
        case EditMethod.File:
          CustomEditor_DoApplyChanges(); break;
        default: TextBox_DoApplyChanges(node, editor); break;
      }
    }

    public override void KeyDown(KeyEventArgs args)
    {
      switch (_editMethod)
      {
        case EditMethod.Numeric:
        case EditMethod.Bool:
        case EditMethod.Date:
        case EditMethod.Time:
        case EditMethod.DateTime: base.KeyDown(args); break;
        case EditMethod.LongText:
        case EditMethod.Color:
        case EditMethod.File:
          CustomEditor_KeyDown(null, args); break;
        default: TextBox_KeyDown(args); break;
      }      
    }

    public override void UpdateEditor(Control control)
    {
      switch (_editMethod)
      {
        case EditMethod.Bool: ComboBox_UpdateEditor(control); break;
        default: base.UpdateEditor(control); break;
      }
    }

    protected override void SetEditControlProperties(Control control, TreeNodeAdv node)
    {
      switch (_editMethod)
      {
        case EditMethod.File:
        case EditMethod.Color:
          {
            DrawContext context = new DrawContext();
            context.Font = control.Font;
            control.Font = GetDrawingFont(node, context);
          }
          break;

        default:
          base.SetEditControlProperties(control, node);
          break;
      }
    }

    //-----------------------------------------------------------------------------
    // NodeComboBox
    //-----------------------------------------------------------------------------
    #region Properties

		private int _editorWidth = 100;
		[DefaultValue(100)]
		public int EditorWidth
		{
			get { return _editorWidth; }
			set { _editorWidth = value; }
		}

		private List<object> _dropDownItems;
		[System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1002:DoNotExposeGenericLists")]
    // ml: switched to separate design time assembly for .NET 4.0 client profile framework.
		//[Editor(typeof(StringCollectionEditor), typeof(UITypeEditor)), DesignerSerializationVisibility(DesignerSerializationVisibility.Content)]
    [Editor("StringCollectionEditor", "UITypeEditor"), DesignerSerializationVisibility(DesignerSerializationVisibility.Content)]
		public List<object> DropDownItems
		{
			get { return _dropDownItems; }
		}

		#endregion
    /*
		public NodeComboBox()
		{
			_dropDownItems = new List<object>();
		}
    */
		protected /*override*/ Size ComboBox_CalculateEditorSize(EditorContext context)
		{
			if (Parent.UseColumns)
				return context.Bounds.Size;
			else
				return new Size(EditorWidth, context.Bounds.Height);
		}

    protected /*override*/ Control ComboBox_CreateEditor(TreeNodeAdv node)
		{
      _dropDownItems = new List<object>();
      _dropDownItems.Add("True");
      _dropDownItems.Add("False");

      ComboBox comboBox = new ComboBox();
			if (DropDownItems != null)
				comboBox.Items.AddRange(DropDownItems.ToArray());
      comboBox.FlatStyle = FlatStyle.Popup;
			comboBox.DropDownStyle = ComboBoxStyle.DropDownList;
			comboBox.DropDownClosed += new EventHandler(EditorDropDownClosed);
      comboBox.SelectedItem = GetValue(node);
      comboBox.Capture = false;

      SetEditControlProperties(comboBox, node);
			return comboBox;
		}
    
    void EditorDropDownClosed(object sender, EventArgs e)
		{
			EndEdit(true);
      (sender as Control).Capture = false;
		}

		public /*override*/ void ComboBox_UpdateEditor(Control control)
		{
			// (control as ComboBox).DroppedDown = true;
		}

    protected /*override*/ void ComboBox_DoApplyChanges(TreeNodeAdv node, Control editor)
		{
			SetValue(node, (editor as ComboBox).SelectedItem);
		}

    //-----------------------------------------------------------------------------
    // DateTimePicker
    //-----------------------------------------------------------------------------
    protected /*override*/ Size DateTimePicker_CalculateEditorSize(EditorContext context)
    {
      if (Parent.UseColumns)
        return context.Bounds.Size;
      else
        return new Size(EditorWidth, context.Bounds.Height);
    }

    protected /*override*/ Control DateTimePicker_CreateEditor(TreeNodeAdv node)
    {
      DateTimePicker dateTimePicker = new DateTimePicker();
      dateTimePicker.ShowCheckBox = true;
      dateTimePicker.Format = DateTimePickerFormat.Custom;
      string customFormat;
      switch (_editMethod)
      {
        case EditMethod.Date:
          customFormat = "dd.MM.yyyy";
          break;
        case EditMethod.Time:
          dateTimePicker.ShowUpDown = true;
          customFormat = "hh:mm:ss";
          break;
        default:
          customFormat = "dd.MM.yyyy hh:mm:ss";
          break;
      }
      dateTimePicker.CustomFormat = customFormat;
      try
      {
        dateTimePicker.Text = GetValue(node) as string;
        dateTimePicker.Checked = true;
      }
      catch (Exception)
      {
        dateTimePicker.Checked = false;
      }
      dateTimePicker.CloseUp += new EventHandler(EditorDropDownClosed);

      SetEditControlProperties(dateTimePicker, node);
      return dateTimePicker;
    }

    protected /*override*/ void DateTimePicker_DoApplyChanges(TreeNodeAdv node, Control editor)
    {
      DateTimePicker control = editor as DateTimePicker;
      String value = control.Checked ? control.Text : String.Empty;
      SetValue(node, value);
    }

    //-----------------------------------------------------------------------------
    // NodeTextBox
    //-----------------------------------------------------------------------------
		private const int MinTextBoxWidth = 30;

		private TextBox EditorTextBox
		{
			get
			{
				return CurrentEditor as TextBox;
			}
		}
    /*
    public NodeTextBox()
    {
    }
    */
    protected /*override*/ Size TextBox_CalculateEditorSize(EditorContext context)
    {
      if (Parent.UseColumns)
        return context.Bounds.Size;
      else
      {
        Size size = GetLabelSize(context.CurrentNode, context.DrawContext, _label);
        int width = Math.Max(size.Width + Font.Height, MinTextBoxWidth); // reserve a place for new typed character
        return new Size(width, size.Height);
      }
    }

    public /*override*/ void TextBox_KeyDown(KeyEventArgs args)
		{
			if (args.KeyCode == Keys.F2 && Parent.CurrentNode != null)
			{
				args.Handled = true;
				BeginEditByUser();
			}
		}

    protected /*override*/ Control TextBox_CreateEditor(TreeNodeAdv node)
		{
			TextBox textBox = new TextBox();
			textBox.TextAlign = TextAlign;
			textBox.Text = GetLabel(node);
			textBox.BorderStyle = BorderStyle.FixedSingle;
			textBox.TextChanged += new EventHandler(textBox_TextChanged);
      textBox.ReadOnly = IsReadonly(node);
			_label = textBox.Text;
			SetEditControlProperties(textBox, node);
			return textBox;
		}
    
		private string _label;
		private void textBox_TextChanged(object sender, EventArgs e)
		{
			_label = EditorTextBox.Text;
			Parent.UpdateEditorBounds();
		}

    protected /*override*/ void TextBox_DoApplyChanges(TreeNodeAdv node, Control editor)
		{
			string oldLabel = GetLabel(node);
			if (oldLabel != _label)
			{
				SetLabel(node, _label);
				OnLabelChanged();
			}
		}

    public void TextBox_Cut()
		{
			if (EditorTextBox != null)
				EditorTextBox.Cut();
		}

    public void TextBox_Copy()
		{
			if (EditorTextBox != null)
				EditorTextBox.Copy();
		}

    public void TextBox_Paste()
		{
			if (EditorTextBox != null)
				EditorTextBox.Paste();
		}

    public void TextBox_Delete()
		{
			if (EditorTextBox != null)
			{
				int len = Math.Max(EditorTextBox.SelectionLength, 1);
				if (EditorTextBox.SelectionStart < EditorTextBox.Text.Length)
				{
					int start = EditorTextBox.SelectionStart;
					EditorTextBox.Text = EditorTextBox.Text.Remove(EditorTextBox.SelectionStart, len);
					EditorTextBox.SelectionStart = start;
				}
			}
		}

		public event EventHandler LabelChanged;
		protected void OnLabelChanged()
		{
			if (LabelChanged != null)
				LabelChanged(this, EventArgs.Empty);
		}

    //-----------------------------------------------------------------------------
    // RichTextBox
    //-----------------------------------------------------------------------------
		private RichTextBox EditorRichTextBox
		{
			get
			{
				return CurrentEditor as RichTextBox;
			}
		}
    /*
    public NodeTextBox()
    {
    }
    */
    protected /*override*/ Size RichTextBox_CalculateEditorSize(EditorContext context)
    {
      if (Parent.UseColumns)
        return context.Bounds.Size;
      else
      {
        Size size = GetLabelSize(context.CurrentNode, context.DrawContext, _label);
        int width = Math.Max(size.Width + Font.Height, MinTextBoxWidth); // reserve a place for new typed character
        return new Size(width, size.Height);
      }
    }

    public /*override*/ void RichTextBox_KeyDown(KeyEventArgs args)
		{
			if (args.KeyCode == Keys.F2 && Parent.CurrentNode != null)
			{
				args.Handled = true;
				BeginEditByUser();
			}
		}

    protected /*override*/ Control RichTextBox_CreateEditor(TreeNodeAdv node)
		{
			RichTextBox textBox = new RichTextBox();
			textBox.Text = GetLabel(node);
			textBox.BorderStyle = BorderStyle.FixedSingle;
			textBox.TextChanged += new EventHandler(richTextBox_TextChanged);
      textBox.ScrollBars = RichTextBoxScrollBars.None;
      textBox.BorderStyle = BorderStyle.None;
      textBox.ReadOnly = IsReadonly(node);
			_label = textBox.Text;
			SetEditControlProperties(textBox, node);
			return textBox;
		}
    
		private void richTextBox_TextChanged(object sender, EventArgs e)
		{
			_label = EditorRichTextBox.Text;
			Parent.UpdateEditorBounds();
		}

    protected /*override*/ void RichTextBox_DoApplyChanges(TreeNodeAdv node, Control editor)
		{
			string oldLabel = GetLabel(node);
			if (oldLabel != _label)
			{
				SetLabel(node, _label);
				OnLabelChanged();
			}
		}

    public void RichTextBox_Cut()
		{
			if (EditorRichTextBox != null)
        EditorRichTextBox.Cut();
		}

    public void RichTextBox_Copy()
		{
      if (EditorRichTextBox != null)
        EditorRichTextBox.Copy();
		}

    public void RichTextBox_Paste()
		{
      if (EditorRichTextBox != null)
				EditorRichTextBox.Paste();
		}

    public void RichTextBox_Delete()
		{
			if (EditorRichTextBox != null)
			{
        int len = Math.Max(EditorRichTextBox.SelectionLength, 1);
        if (EditorRichTextBox.SelectionStart < EditorRichTextBox.Text.Length)
				{
          int start = EditorRichTextBox.SelectionStart;
          EditorRichTextBox.Text = EditorTextBox.Text.Remove(EditorTextBox.SelectionStart, len);
          EditorRichTextBox.SelectionStart = start;
				}
			}
		}
    /*
		public event EventHandler LabelChanged;
		protected void OnLabelChanged()
		{
			if (LabelChanged != null)
				LabelChanged(this, EventArgs.Empty);
		}
    */

    //-----------------------------------------------------------------------------
    // NodeNumericUpDown
    //-----------------------------------------------------------------------------
		#region Properties

		private int _decimalPlaces = 0;
		[Category("Data"), DefaultValue(0)]
		public int DecimalPlaces
		{
			get
			{
				return this._decimalPlaces;
			}
			set
			{
				this._decimalPlaces = value;
			}
		}

		private decimal _increment = 1;
		[Category("Data"), DefaultValue(1)]
		public decimal Increment
		{
			get
			{
				return this._increment;
			}
			set
			{
				this._increment = value;
			}
		}

		private decimal _minimum = 0;
		[Category("Data"), DefaultValue(0)]
		public decimal Minimum
		{
			get
			{
				return _minimum;
			}
			set
			{
				_minimum = value;
			}
		}

		private decimal _maximum = 100;
		[Category("Data"), DefaultValue(100)]
		public decimal Maximum
		{
			get
			{
				return this._maximum;
			}
			set
			{
				this._maximum = value;
			}
		}

		#endregion
    /*
		public NodeNumericUpDown()
		{
		}
    */
		protected /*override*/ Size Numeric_CalculateEditorSize(EditorContext context)
		{
			if (Parent.UseColumns)
				return context.Bounds.Size;
			else
				return new Size(EditorWidth, context.Bounds.Height);
		}

    protected /*override*/ Control Numeric_CreateEditor(TreeNodeAdv node)
		{
			NumericUpDown num = new NumericUpDown();
      num.BorderStyle = BorderStyle.FixedSingle;
			num.Increment = Increment;
			num.DecimalPlaces = DecimalPlaces;
			num.Minimum = Minimum;
			num.Maximum = Maximum;
      try
      {
        string value = (string)GetValue(node);
        string[] sa = value.Split(new Char[] { '.' });
        if (1 < sa.Length)
          num.DecimalPlaces = sa.GetValue(1).ToString().Length;
        value = value.Replace(".", System.Globalization.CultureInfo.CurrentCulture.NumberFormat.CurrencyDecimalSeparator);
        num.Value = Decimal.Parse(value);
        num.ReadOnly = IsReadonly(node);
      }
      catch (Exception)
      {
      }
			SetEditControlProperties(num, node);
			return num;
		}

    protected /*override*/ void Numeric_DoApplyChanges(TreeNodeAdv node, Control editor)
		{
			SetValue(node, (editor as NumericUpDown).Value.ToString());
		}

    //-----------------------------------------------------------------------------
    // CustomEditor
    //-----------------------------------------------------------------------------
    public delegate string RunExtEditorDelegate(TreeNodeAdv node, string value, EditMethod editMethod);
    public RunExtEditorDelegate RunExtEditor;

    protected /*override*/ Size CustomEditor_CalculateEditorSize(EditorContext context)
    {
      if (Parent.UseColumns)
        return context.Bounds.Size;
      else
        return new Size(EditorWidth, context.Bounds.Height);
    }

    protected /*override*/ Control CustomEditor_CreateEditor(TreeNodeAdv node)
    {
      Panel panel = new Panel();
      _maskedTextBox = new MaskedTextBox();
      Button extEditorButton = new Button();

      // 
      // panel
      // 
      panel.AutoSizeMode = AutoSizeMode.GrowAndShrink;
      panel.Controls.Add(extEditorButton);
      panel.Controls.Add(_maskedTextBox);
      panel.Dock = DockStyle.None;
      panel.Location = new System.Drawing.Point(0, 0);
      panel.Margin = new Padding(0);
      panel.Name = "panel";
      panel.Size = new System.Drawing.Size(292, 20);
      panel.TabIndex = 0;
      // 
      // _maskedTextBox
      // 
      _maskedTextBox.Anchor = ((AnchorStyles)(((AnchorStyles.Top | AnchorStyles.Left)
                  | AnchorStyles.Right)));
      _maskedTextBox.Location = new System.Drawing.Point(0, 0);
      _maskedTextBox.Margin = new Padding(0);
      _maskedTextBox.Name = "_maskedTextBox";
      _maskedTextBox.Size = new System.Drawing.Size(278, 15);
      _maskedTextBox.BorderStyle = BorderStyle.FixedSingle;
      _maskedTextBox.TabIndex = 0;
      _maskedTextBox.KeyDown += new KeyEventHandler(CustomEditor_KeyDown);
      _maskedTextBox.KeyPress += new KeyPressEventHandler(CustomEditor_KeyPress);
      _maskedTextBox.Validating += new CancelEventHandler(CustomEditor_EditorValidating);
      _maskedTextBox.TextChanged += new EventHandler(CustomEditor_TextChanged);
      switch (_editMethod)
      {
        case EditMethod.Color:
          _maskedTextBox.AsciiOnly = true;
          _maskedTextBox.Mask = "\\#>AAAAAA";
          break;
      }

      // 
      // extEditorButton
      // 
      extEditorButton.Image = global::Aga.Controls.Properties.Resources._3dots;
      extEditorButton.FlatStyle = FlatStyle.Flat;
      extEditorButton.Anchor = ((AnchorStyles)((AnchorStyles.Top | AnchorStyles.Right)));
      extEditorButton.Location = new System.Drawing.Point(277, 0);
      extEditorButton.Margin = new Padding(0);
      extEditorButton.Name = "extEditorButton";
      extEditorButton.Size = new System.Drawing.Size(15, 15);
      extEditorButton.TabIndex = 1;
      extEditorButton.Text = "";
      extEditorButton.UseVisualStyleBackColor = true;
      extEditorButton.Click += new System.EventHandler(CustomEditor_Click);

      _maskedTextBox.Text = GetValue(node) as string;
      
      SetEditControlProperties(panel, node);
      return panel;
    }

    private void CustomEditor_EditorValidating(object sender, CancelEventArgs e)
    {
      _validated = false;
      if (null != ValidatingExtEditor && !_cancelEdit)
      {
        ValidatingExtEditor(EditNode, _editMethod, _maskedTextBox.Text, e);
        _validated = !e.Cancel;
      }
    }

    protected virtual void CustomEditor_KeyDown(object sender, KeyEventArgs e)
    {
      switch (e.KeyCode)
      {
        case Keys.Escape:
          _cancelEdit = true;
          EndEdit(false);
          break;
        case Keys.Enter:
          EndEdit(true);
          break;
      }
    }

    protected virtual void CustomEditor_KeyPress(object sender, KeyPressEventArgs e)
    {
      switch (_editMethod)
      {
        case EditMethod.Color:
          if (!('0' <= e.KeyChar && '9' >= e.KeyChar) &&
              !('a' <= e.KeyChar && 'f' >= e.KeyChar) &&
              !('A' <= e.KeyChar && 'F' >= e.KeyChar))
          {
            e.KeyChar = '.'; // this symbol doesn't conform to mask
            e.Handled = true;
          }
          break;
      }
    }

    protected /*override*/ void CustomEditor_Click(object sender, EventArgs e)
    {
      if (null == RunExtEditor)
        return;
      _maskedTextBox.Text = RunExtEditor(EditNode, _maskedTextBox.Text, _editMethod);
      _maskedTextBox.Focus();
    }

    protected void CustomEditor_TextChanged(object sender, EventArgs e)
    {
      CustomEditor_EditorValidating(sender, new CancelEventArgs());
    }

    public delegate void ValidatingExtEditorDelegate(TreeNodeAdv node, EditMethod editMethod, String value, CancelEventArgs e);
    public ValidatingExtEditorDelegate ValidatingExtEditor;

    protected /*override*/ void CustomEditor_DoApplyChanges()
    {
      if (_validated && !_cancelEdit)
      {
        string value = _maskedTextBox.Text;
        OnBeforeApply(EditNode, ref value, _editMethod);
        SetValue(EditNode, value);
      }
    }
  }
}
