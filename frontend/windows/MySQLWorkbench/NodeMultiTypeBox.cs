/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

using System;
using System.Drawing;
using System.Windows.Forms;

using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

using MySQL.Grt;

namespace MySQL.GUI.Workbench
{
  public class NodeMultiTypeBox : NodeCustomBox
  {
    private MySQL.Grt.TreeModelWrapper _grtTree;
    private int _valueColumn;
    private int _isReadonlyColumn;
    private int _editMethodColumn;


    public NodeMultiTypeBox()
    {
    }


    public MySQL.Grt.TreeModelWrapper GrtTreeModel
    {
      get { return _grtTree; }
      set
      {
        if (null != _grtTree && null == value)
        {
          IsNodeReadonly -= IsNodeReadonly_;
          GetNodeEditMethod -= GetNodeEditMethod_;
          ValueNeeded -= ValueNeeded_;
          ValuePushed -= ValuePushed_;
          BeforeEdit -= BeforeEdit_;
          BeforeApply -= BeforeApply_;
          RunExtEditor -= RunExtEditor_;
          ValidatingExtEditor -= ValidatingExtEditor_;
        }
        else if (null == _grtTree && null != value)
        {
          IsNodeReadonly += new NodeCustomBox.IsNodeReadonlyDelegate(IsNodeReadonly_);
          GetNodeEditMethod += new NodeCustomBox.GetNodeEditMethodDelegate(GetNodeEditMethod_);
          ValueNeeded += new EventHandler<NodeControlValueEventArgs>(ValueNeeded_);
          ValuePushed += new EventHandler<NodeControlValueEventArgs>(ValuePushed_);
          BeforeEdit += new NodeCustomBox.BeforeEditDelegate(BeforeEdit_);
          BeforeApply += new NodeCustomBox.BeforeApplyDelegate(BeforeApply_);
          RunExtEditor += new NodeCustomBox.RunExtEditorDelegate(RunExtEditor_);
          ValidatingExtEditor += new NodeCustomBox.ValidatingExtEditorDelegate(ValidatingExtEditor_);
        }
        _grtTree = value;
      }
    }


    public int ValueColumn
    {
      get { return _valueColumn; }
      set { _valueColumn = value; }
    }


    public int IsReadonlyColumn
    {
      get { return _isReadonlyColumn; }
      set { _isReadonlyColumn = value; }
    }


    public int EditMethodColumn
    {
      get { return _editMethodColumn; }
      set { _editMethodColumn = value; }
    }


    bool IsNodeReadonly_(TreeNodeAdv treeNode)
    {
      if (null != treeNode)
      {
        GrtTreeNode node = treeNode.Tag as GrtTreeNode;
        if (null != node)
        {
          string isReadonly;
          _grtTree.get_field(node.NodeId, _isReadonlyColumn, out isReadonly);
          return (isReadonly == "1");
        }
      }
      return false;
    }


    NodeCustomBox.EditMethod GetNodeEditMethod_(TreeNodeAdv treeNode)
    {
      if (null != treeNode)
      {
        GrtTreeNode node = treeNode.Tag as GrtTreeNode;

        if (null != node)
        {
          string editMethod;
          _grtTree.get_field(node.NodeId, _editMethodColumn, out editMethod);

          if (editMethod.StartsWith("numeric"))
            return NodeCustomBox.EditMethod.Numeric;
          else if (editMethod.StartsWith("bool"))
            return NodeCustomBox.EditMethod.Bool;
          else if (editMethod.StartsWith("longtext"))
            return NodeCustomBox.EditMethod.LongText;
          else if (editMethod.StartsWith("color"))
            return NodeCustomBox.EditMethod.Color;
          else if (editMethod.StartsWith("file"))
            return NodeCustomBox.EditMethod.File;
          else if (editMethod.StartsWith("datetime"))
            return NodeCustomBox.EditMethod.DateTime;
          else if (editMethod.StartsWith("date"))
            return NodeCustomBox.EditMethod.Date;
          else if (editMethod.StartsWith("time"))
            return NodeCustomBox.EditMethod.Time;
          else
            return NodeCustomBox.EditMethod.Text;
        }
      }
      return NodeCustomBox.EditMethod.Text;
    }


    private void ValueNeeded_(object sender, NodeControlValueEventArgs e)
    {
      if (e.Node != null && e.Node.Tag != null)
      {
        GrtTreeNode node = e.Node.Tag as GrtTreeNode;

        if (node != null)
        {
          string caption;
          _grtTree.get_field(node.NodeId, _valueColumn, out caption);
          e.Value = caption;

          NodeCustomBox.EditMethod editMethod = GetNodeEditMethod(e.Node);
          switch (editMethod)
          {
            case NodeCustomBox.EditMethod.Bool:
              switch (e.Value as string)
              {
                case "":
                case "0":
                  e.Value = "False";
                  break;
                default:
                  e.Value = "True";
                  break;
              }
              break;
          }
        }
      }
    }


		private void ValuePushed_(object sender, NodeControlValueEventArgs e)
		{
			if (e.Node != null && e.Node.Tag != null)
			{
				GrtTreeNode node = e.Node.Tag as GrtTreeNode;
				String value = e.Value as String;

				if (node != null && value != null)
				{
					GrtValueType nodeType = (GrtValueType)_grtTree.get_field_type(node.NodeId, _valueColumn);
          NodeCustomBox.EditMethod editMethod = GetNodeEditMethod(e.Node);

          switch (nodeType)
          {
            case GrtValueType.StringValue:
              PushStringValue(node, value, editMethod);
              break;
            case GrtValueType.IntValue:
              PushIntValue(node, value, editMethod);
              break;
            case GrtValueType.RealValue:
              PushRealValue(node, value, editMethod);
              break;
            default:
              MessageBox.Show("This value cannot be edited.");
              break;
          }
				}
			}
		}


    private void PushStringValue(GrtTreeNode node, string value, NodeCustomBox.EditMethod editMethod)
    {
      _grtTree.set_field(node.NodeId, _valueColumn, value);
    }


    private void PushIntValue(GrtTreeNode node, string value, NodeCustomBox.EditMethod editMethod)
    {
      try
      {
        int intValue = 0;

        switch (editMethod)
        {
          case NodeCustomBox.EditMethod.Bool:
            switch (value)
            {
              case "":
              case "False":
                intValue = 0;
                break;
              default:
                intValue = 1;
                break;
            }
            break;

          default:
            try
            {
              Decimal d = Decimal.Parse(value, System.Globalization.NumberStyles.Float);
              intValue = Decimal.ToInt32(Decimal.Round(d, 0));
            }
            catch (Exception)
            {
            }
            break;
        }

        _grtTree.set_field(node.NodeId, _valueColumn, intValue);
      }
      catch (Exception ex)
      {
        MessageBox.Show(String.Format("The value you have entered is not an integer value.\n\n({0})", ex.Message));
      }
    }


    private void PushRealValue(GrtTreeNode node, string value, NodeCustomBox.EditMethod editMethod)
    {
      try
      {
        double doubleValue = double.Parse(value);
        _grtTree.set_field(node.NodeId, _valueColumn, doubleValue);
      }
      catch (Exception ex)
      {
        MessageBox.Show(String.Format("The value you have entered is not a float value.\n\n({0})", ex.Message));
      }
    }


    void BeforeEdit_(TreeNodeAdv treeNode, NodeCustomBox customBox)
    {
      GrtTreeNode node = treeNode.Tag as GrtTreeNode;
      if (null == node)
        return;

      string editMethod;
      _grtTree.get_field(node.NodeId, _editMethodColumn, out editMethod);

      if (editMethod.StartsWith("numeric"))
      {
        string[] sa = editMethod.Split(new Char[] { ':', ',' });
        if (1 < sa.Length)
        {
          for (int n = 1; sa.Length > n; ++n)
          {
            try
            {
              int v = int.Parse((string)sa.GetValue(n));
              switch (n)
              {
                case 1: customBox.Minimum = v; break;
                case 2: customBox.Maximum = v; break;
                case 3: customBox.Increment = v; break;
                case 4: customBox.DecimalPlaces = v; break;
              }
            }
            catch (Exception)
            {
            }
          }
        }
      }
    }


    void BeforeApply_(TreeNodeAdv treeNode, ref string value, EditMethod editMethod)
    {
      switch (editMethod)
      {
        case EditMethod.Color:
          value = value.ToUpper();
          if (7 != value.Length)
            value = String.Empty;
          break;
      }
    }


    public void ValidatingExtEditor_(TreeNodeAdv node, EditMethod editMethod, String value, System.ComponentModel.CancelEventArgs e)
    {
      switch (editMethod)
      {
        case NodeCustomBox.EditMethod.Color:
          e.Cancel = !ValidateColor(value);
          break;
      }
    }


    bool ValidateColor(String value)
    {
      if (0 == value.Length || "#" == value)
        return true;

      System.Text.RegularExpressions.Regex regex = new System.Text.RegularExpressions.Regex("#([0-9a-fA-F]){6}");
      System.Text.RegularExpressions.Match match = regex.Match(value);
      return match.Success && (match.Length == value.Length);
    }


    private string RunExtEditor_(TreeNodeAdv treeNodeAdv, string value, NodeCustomBox.EditMethod editMethod)
    {
      switch (editMethod)
      {
        case NodeCustomBox.EditMethod.Color:
          value = RunColorEditor(value);
          break;
        case NodeCustomBox.EditMethod.File:
          value = RunFilenameEditor(value);
          break;
        default:
          value = RunTextEditor(value);
          break;
      }

      return value;
    }


    private string RunColorEditor(string value)
    {
      string _value = value; // store original value
      try
      {
        ColorDialog dlg = new ColorDialog();

        // convert from string to Color struct
        try
        {
          dlg.Color = System.Drawing.ColorTranslator.FromHtml(value);
        }
        catch (Exception)
        {
        }

        if (DialogResult.OK != dlg.ShowDialog())
          return value;

        // Cannot use ColorTranslator.ToHtml(dlg.Color) here, as it would convert known colors
        // to text (e.g. White, Black).
        Color color = dlg.Color;
        value = String.Format("#{0:X2}{1:X2}{2:X2}", color.R, color.G, color.B);
      }
      catch (Exception)
      {
        value = _value;
      }

      return value;
    }


    private string RunTextEditor(string value)
    {
      try
      {
        TextEditorForm frm = new TextEditorForm();
        frm.RichTextBox.Text = value;
        DialogResult res = frm.ShowDialog();
        switch (res)
        {
          case DialogResult.Cancel:
            break;
          default:
            value = frm.RichTextBox.Text;
            break;
        }
      }
      catch (Exception)
      {
      }
      return value;
    }


    private string RunFilenameEditor(string value)
    {
      try
      {
        OpenFileDialog dlg = new OpenFileDialog();
        dlg.RestoreDirectory = true;
        dlg.ValidateNames = false;
        dlg.FileName = value;
        if (DialogResult.OK == dlg.ShowDialog())
          value = dlg.FileName;
      }
      catch (Exception)
      {
      }
      return value;
    }
  }

}
