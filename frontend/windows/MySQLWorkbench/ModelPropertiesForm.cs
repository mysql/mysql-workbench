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
using System.Collections.Generic;
using System.Drawing;

using MySQL.Base;
using MySQL.Workbench;
using MySQL.Grt;
using MySQL.Controls;

namespace MySQL.GUI.Workbench
{
  public partial class ModelPropertiesForm : TabDocument
  {
    WbContext wbContext;
    GrtValueInspector valueInspector;
    SimpleGrtTreeModel valueInspectorModel;
    private NodeMultiTypeBox _valueNodeControl;

    public ModelPropertiesForm()
    {
      InitializeComponent();
      CreateValueNodeControl();
    }

    public ModelPropertiesForm(WbContext WbContext)
    {
      wbContext = WbContext;

      InitializeComponent();
      CreateValueNodeControl();
    }

    void CreateValueNodeControl()
    {
      // 
      // _valueNodeControl
      // 
      _valueNodeControl = new NodeMultiTypeBox();
      propertiesTreeView.NodeControls.Add(_valueNodeControl);
      _valueNodeControl.DataPropertyName = "Text";
      _valueNodeControl.EditEnabled = true;
      _valueNodeControl.EditOnClick = true;
      _valueNodeControl.IncrementalSearchEnabled = true;
      _valueNodeControl.LeftMargin = 3;
      _valueNodeControl.ParentColumn = valueTreeColumn;
      _valueNodeControl.VirtualMode = true;
      _valueNodeControl.Trimming = StringTrimming.EllipsisCharacter;
      _valueNodeControl.ValueColumn = (int)GrtValueInspector.Columns.Value;
      _valueNodeControl.IsReadonlyColumn = (int)GrtValueInspector.Columns.IsReadonly;
      _valueNodeControl.EditMethodColumn = (int)GrtValueInspector.Columns.EditMethod;
    }


    public void UpdateForForm(UIForm form)
    {
      if (valueInspectorModel != null)
        valueInspectorModel.DetachEvents();

      propertiesTreeView.SuspendLayout();
      propertiesTreeView.Model = null;
      valueInspector = null;
      valueInspectorModel = null;

      if (form != null)
      {
        List<String> items;

        valueInspector = wbContext.get_inspector_for_selection(form, out items);

        if (valueInspector != null)
        {
          valueInspectorModel = new SimpleGrtTreeModel(propertiesTreeView, valueInspector, false);
          valueInspectorModel.AddColumn(nameNodeControl, (int)GrtValueInspector.Columns.Name, false);
          //!valueInspectorModel.AddColumn(_valueNodeControl, (int)GrtValueInspector.Columns.Value, true);
          _valueNodeControl.GrtTreeModel = valueInspectorModel.GrtTree;

          {
            TreeModelTooltipProvider tp = new TreeModelTooltipProvider();
            tp.Model = valueInspectorModel.GrtTree;
            tp.TooltipColumn = (int)GrtValueInspector.Columns.Description;
            nameNodeControl.ToolTipProvider = tp;
            _valueNodeControl.ToolTipProvider = tp;
          }

          propertiesTreeView.Model = valueInspectorModel;

         // TabText = "Selection Properties";
        }
      }

      propertiesTreeView.ResumeLayout();
    }


    /*protected override void OnPaint(PaintEventArgs e)
    {
      base.OnPaint(e);

      Graphics g = e.Graphics;
      SolidBrush solid = new SolidBrush(System.Drawing.SystemColors.Window);
      Pen pen = new Pen(System.Drawing.SystemColors.ControlDark);

      g.FillRectangle(solid, 0, 0, Width, Height);

      if (Pane != null && Pane.Contents.Count > 1)
          g.DrawRectangle(pen, 0, -1, Width - 1, Height + 1);
      else
          g.DrawRectangle(pen, 0, -1, Width - 1, Height);
		}*/
  }
}
