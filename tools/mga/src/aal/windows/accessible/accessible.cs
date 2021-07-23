/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Automation.Text;
using System.Windows.Forms;

namespace aal {
  public class AccessibleNet {

    #region Members

    private AutomationElement _element;
    private TreeWalker _treeWalker;
    private bool _isHighlighted = false;

    #endregion

    #region Constructors

    public AccessibleNet(AutomationElement element, TreeWalker walker) {
      _element = element;
      _treeWalker = walker;
      IsVirtualColumn = false;
      IsVirtualRow = false;
      IsCell = false;

    }

    #endregion

    #region Container

    public AccessibleNet[] Children() {
      List<AccessibleNet> list = new List<AccessibleNet>();
      if (!Valid)
        return list.ToArray();

      AutomationElement child = _treeWalker.GetFirstChild(_element);
      while (child != null) {
        list.Add(new AccessibleNet(child, _treeWalker));
        child = _treeWalker.GetNextSibling(child);
      }

      return list.ToArray();
    }

    #endregion

    #region Static functions

    [DllImport("Kernel32.dll")]
    private static extern bool QueryFullProcessImageName([In] IntPtr hProcess, [In] uint dwFlags, [Out] StringBuilder lpExeName, [In, Out] ref uint lpdwSize);

    public static string GetMainModuleFileName(Process process, int buffer = 1024) {
      var fileNameBuilder = new StringBuilder(buffer);
      uint bufferLength = (uint)fileNameBuilder.Capacity + 1;
      return QueryFullProcessImageName(process.Handle, 0, fileNameBuilder, ref bufferLength) ?
          fileNameBuilder.ToString() :
          null;
    }

    public static int GetRunningProcess(string fileName) {
      string moduleName = fileName.Replace("/", "\\");
      Process[] processes = Process.GetProcesses();
      foreach (Process process in processes) {
        try {
          if (process.MainWindowHandle != IntPtr.Zero) { // We can only deal with GUI processes.
            string guiModuleName = GetMainModuleFileName(process);
            if (guiModuleName.Equals(fileName, StringComparison.CurrentCultureIgnoreCase)) {
              return process.Id;
            }
          }
        } catch { }
      }
      return 0;
    }

    public static int[] GetRunningProcessByName(string processName)
    {
      var processList = new List<int>();
      Process[] processes = Process.GetProcesses();
      foreach (Process process in processes) {
        try {
          if (process.MainWindowHandle != IntPtr.Zero) { // We can only deal with GUI processes.
            string module = process.ProcessName;
            if (module.Equals(processName, StringComparison.CurrentCultureIgnoreCase)) {
              processList.Add(process.Id);
            }
          }
        } catch { }
      }
      return processList.ToArray();
    }

    public static AccessibleNet GetByPid(int pid) {
      Condition condition = new PropertyCondition(AutomationElement.ProcessIdProperty, pid);
      TreeWalker treeWalker = new TreeWalker(condition);
      AutomationElement element = AutomationElement.RootElement.FindFirst(TreeScope.Children, condition);
      if (element == null)
        return null;

      //element = treeWalker.Normalize(element);
      return new AccessibleNet(element, treeWalker);
    }

    static public AccessibleNet FromPoint(System.Windows.Point point) {
      AutomationElement child = AutomationElement.FromPoint(point);
      if (child == null)
        return null;

      Condition condition = new PropertyCondition(AutomationElement.ProcessIdProperty, child.Current.ProcessId);
      TreeWalker treeWalker = new TreeWalker(condition);
      child = treeWalker.Normalize(child);
      return new AccessibleNet(child, treeWalker);
    }

    public static String ClipboardText {
      get {
        throw new NotImplementedException();
      }
      set {
        throw new NotImplementedException();
      }
    }

    #endregion

    #region Scaling

    [DllImport("User32.dll")]
    public static extern IntPtr GetDC(IntPtr hwnd);
    [DllImport("User32.dll")]
    public static extern void ReleaseDC(IntPtr hwnd, IntPtr dc);

    [DllImport("gdi32.dll")]
    static extern int GetDeviceCaps(IntPtr hdc, int nIndex);
    public enum DeviceCap {
      VERTRES = 10,
      LOGPIXELSX = 88,
      LOGPIXELSY = 90,
      DESKTOPVERTRES = 117,
    }

    private float getScalingFactor() {
      Graphics g = Graphics.FromHwnd(IntPtr.Zero);
      IntPtr desktop = g.GetHdc();
      int logicalScreenHeight = GetDeviceCaps(desktop, (int)DeviceCap.VERTRES);
      int physicalScreenHeight = GetDeviceCaps(desktop, (int)DeviceCap.DESKTOPVERTRES);
      float screenScalingFactor = (float)physicalScreenHeight / (float)logicalScreenHeight;
      return screenScalingFactor; // 1.25 = 125%
    }

    public void TransformUsingScaling(double x, double y, out int newX, out int newY) {
      float scaling = getScalingFactor();
      newX = Convert.ToInt32((float)x * scaling);
      newY = Convert.ToInt32((float)y * scaling);
    }

    #endregion

    #region Properties

    public bool IsVirtualColumn { get; private set; }

    public bool HasHeader { get; private set; }

    public bool IsVirtualRow { get; private set; }
    public bool IsCell { get; set; }

    public int ColumnId { get; private set; }

    public bool Valid => _element != null;

    public bool IsRoot => Parent == null;

    public ControlType ControlType => _element.Current.ControlType;

    public string Name => _element != null ? _element.Current.Name : "";

    public string ID => _element != null ? _element.Current.AutomationId : "";

    public bool MenuShown {
      get {
        if (!Valid)
          return false;
        Object expandCollapsePattern = new Object();
        if (this._element.TryGetCurrentPattern(ExpandCollapsePattern.Pattern, out expandCollapsePattern)) {
          if (((ExpandCollapsePattern)expandCollapsePattern).Current.ExpandCollapseState == ExpandCollapseState.Expanded)
            return true;
          if (((ExpandCollapsePattern)expandCollapsePattern).Current.ExpandCollapseState == ExpandCollapseState.Collapsed)
            return false;
        } else {
          RaiseExpandCollapsePatternError(expandCollapsePattern);
        }
        return false;
      }
    }

    public Rect Bounds {
      set {
        if (!Valid)
          return;
        var transformPattern = new Object();
        if (this._element.TryGetCurrentPattern(TransformPattern.Pattern, out transformPattern)) {

          if (((TransformPattern)transformPattern).Current.CanMove) {
            ((TransformPattern)transformPattern).Move(value.X, value.Y);
          }
          if (((TransformPattern)transformPattern).Current.CanResize) {
            ((TransformPattern)transformPattern).Resize(value.Width, value.Height);
          }
        } else {
          RaiseTransformPatternError(transformPattern);
        }
      }
      get {
        if (_element != null) {
          return _element.Current.BoundingRectangle;
        } else {
          return Rect.Empty;
        }
      }
    }

    public AccessibleNet Parent {
      get {
        if (!Valid)
          return null;

        try {
          // The tree walker has a condition to only search our pid, so it will not return the desktop as parent.
          AutomationElement parent = _treeWalker.GetParent(_element);
          if (parent == null)
            return null;

          AccessibleNet result = new AccessibleNet(parent, _treeWalker);
          return result;
        } catch {
          return null;
        }
      }
    }

    public string ClassName {
      get {
        // WindowsForms10.SysMonthCal32.app.0.141b42a_r9_ad1
        return MatchClassName(_element.Current.ClassName);
      }
    }

    private string MatchClassName(String className) {
      Match match = Regex.Match(className, @"\.(?<classname>[a-zA-Z0-9]+)");
      if (match.Success)
        return match.Groups["classname"].Value;

      return className;
    }

    public bool IsEnabled => _element.Current.IsEnabled;

    public bool IsReadOnly {
      get {
        if (!Valid)
          return false;
        if (this._element.Current.IsEnabled && this._element.Current.IsKeyboardFocusable) {
          Object valuePattern = new Object();
          if (this._element.TryGetCurrentPattern(ValuePattern.Pattern, out valuePattern)) {
            return ((ValuePattern)valuePattern).Current.IsReadOnly;
          } else {
            RaiseValuePatternError(valuePattern);
          }
        } else {
          RaiseControlAccessError();
        }
        return false;
      }
    }

    public bool IsEditable {
      get {
        if (!Valid || _element.Current.ControlType != ControlType.ComboBox)
          return false;
        AutomationElement child = _treeWalker.GetFirstChild(_element);
        while (child != null) {
          if (child.Current.Name == _element.Current.Name && MatchClassName(child.Current.ClassName).Equals("edit", StringComparison.CurrentCultureIgnoreCase))
            return true;
          child = _treeWalker.GetNextSibling(child);
        }
        return false;
      }
    }

    public bool IsInternal {
      get {
        if (!Valid)
          return false;
        AutomationElement parent = _treeWalker.GetParent(_element);
        // spinner -->
        if (parent != null && parent.Current.ControlType == ControlType.Spinner && this._element.Current.Name == parent.Current.Name &&
          this.ClassName.Equals("edit", StringComparison.CurrentCultureIgnoreCase))
          return true;
        if (parent != null && parent.Current.ControlType == ControlType.Spinner && this._element.Current.ControlType == ControlType.Spinner) {
          Condition cond = new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.Button);
          var button = _element.FindFirst(TreeScope.Children, cond);
          if (button != null)
            return true;
        }
        // <-- spinner 
        // combobox -->
        if (parent != null && parent.Current.ControlType == ControlType.ComboBox) {
          if (this._element.Current.ControlType == ControlType.Edit || this._element.Current.ControlType == ControlType.List &&
            this._element.Current.Name == parent.Current.Name) {
            return true;
          }
          if (this._element.Current.ControlType == ControlType.Button && String.IsNullOrEmpty(this._element.Current.ClassName)) {
            return true;
          }
        }
        if (parent != null && parent.Current.ControlType == ControlType.List && MatchClassName(parent.Current.ClassName).Equals("ComboLBox", StringComparison.CurrentCultureIgnoreCase)) {
          var gp = _treeWalker.GetParent(parent);
          if (gp != null && gp.Current.ControlType == ControlType.ComboBox) {
            return true;
          }
        }
        // <-- combobox
        // scrollbar in texentry -->
        if (parent != null && parent.Current.ControlType == ControlType.Document && this._element.Current.Name == parent.Current.Name &&
          MatchClassName(parent.Current.ClassName).Equals("edit", StringComparison.CurrentCultureIgnoreCase)) {
          if (this._element.Current.ControlType == ControlType.ScrollBar) {
            return true;
          }
        }
        // <-- scrollbar in texentry
        // scrollbar in listview -->
        if (parent != null && parent.Current.ControlType == ControlType.List && this._element.Current.Name == parent.Current.Name &&
          this._element.Current.ControlType == ControlType.ScrollBar)
          return true;
        // <-- scrollbar in listview
        // scrollbar in scrollbox -->
        if (parent != null && parent.Current.ControlType == ControlType.Pane && this._element.Current.Name == parent.Current.Name &&
          this._element.Current.ControlType == ControlType.ScrollBar) {
          return (Parent.Scrollable) ? true : false;
        }
        // <-- scrollbar in scrollbox
        return false;
      }
    }

    #endregion

    #region Text entry

    public String Text => InternalGetText(_element, 0, -1);

    public String SelectedText {
      get {
        return GetSelectedText();
      }
      set {
        // A TextPatternRange will become invalid if one of the following occurs: 
        //  * the text in the TextPattern container changes because of some user activity
        //  * the SetValue method of ValuePattern is used to programmatically change the value of the text in the TextPattern container
        this._element.SetFocus();
        // Pause before sending keyboard input.
        Thread.Sleep(100);
        SendKeys.SendWait(value);
      }
    }

    public String Title {
      get {
        if (!Valid)
          return string.Empty;
        return _element.Current.Name;
      }
    }

    public String Description {
      get {
        // If tooltip is not set description text is return
        return HelpText;
      }
    }

    public bool IsSecure => this._element.Current.IsPassword;

    public String HelpText {
      get {
        if (!Valid)
          return string.Empty;
        // This information is typically obtained from tooltips specified by providers.
        // The following code retrieves the current value of the property, but specifies that if the element itself 
        // does not provide a value for the property, NotSupported is to be returned instead of a default value.
        var description = _element.GetCurrentPropertyValue(AutomationElement.HelpTextProperty, true);
        if (description == AutomationElement.NotSupported) {
          description = string.Empty; ;
        }
        return (string)description; ;
      }
    }

    public void InsertText(int offset, String value) {
      if (!Valid)
        return;

      if (this._element.Current.IsEnabled && this._element.Current.IsKeyboardFocusable) {
        Object valuePattern = new Object();
        Object textPattern = new Object();
        if (this._element.TryGetCurrentPattern(TextPattern.Pattern, out textPattern)) {
          TextPatternRange range = ((TextPattern)textPattern).DocumentRange;
          if (range != null) {
            // NOTE: Elements that support TextPattern 
            //       do not support ValuePattern and TextPattern
            //       does not support setting the text of 
            //       multi-line edit or document controls.
            //       For this reason, text input must be simulated
            //       using one of the following methods.
            this._element.SetFocus();
            if (offset < 0) {
              // Delete existing content in the control and insert new content.
              SendKeys.SendWait("^{HOME}");   // Move to start of control
              SendKeys.SendWait("^+{END}");   // Select everything
              SendKeys.SendWait("{DEL}");     // Delete selection
            } else {
              range.MoveEndpointByUnit(TextPatternRangeEndpoint.Start, TextUnit.Character, (int)offset);
              range.MoveEndpointByUnit(TextPatternRangeEndpoint.End, TextUnit.Character, (int)offset);
              range.Select();
            }
            // Pause before sending keyboard input.
            Thread.Sleep(100);
            SendKeys.SendWait(value);
          }
        } else if (this._element.TryGetCurrentPattern(ValuePattern.Pattern, out valuePattern)) {
          this._element.SetFocus();
          if (this._element.Current.ControlType == ControlType.ComboBox) {
            SetComboBoxValue(value);
          } else {
            ((ValuePattern)valuePattern).SetValue(value);
          }
        } else {
          RaiseTextPatternError(textPattern);
        }
      } else {
        RaiseControlAccessError();
      }
    }

    public int CaretPosition {
      get {
        if (this._element.Current.IsEnabled && this._element.Current.IsKeyboardFocusable) {
          Object textPattern = new Object();
          if (this._element.TryGetCurrentPattern(TextPattern.Pattern, out textPattern)) {
            TextPatternRange doc = ((TextPattern)textPattern).DocumentRange;
            int length = doc.GetText(-1).Length;
            TextPatternRange[] array = ((TextPattern)textPattern).GetSelection();
            if ((array != null) && (array.Length > 0)) {
              TextPatternRange copy = array[0].Clone();
              copy.ExpandToEnclosingUnit(TextUnit.Character);
              int moved = copy.MoveEndpointByUnit(TextPatternRangeEndpoint.Start, TextUnit.Character, length);
              if (moved <= length)
                return length - moved;
            }
          } else {
            RaiseTextPatternError(textPattern);
          }
        } else {
          RaiseControlAccessError();
        }
        return 0;
      }
      set {
        if (0 == value) {
          return;
        }
        if (this._element.Current.IsEnabled && this._element.Current.IsKeyboardFocusable) {
          Object textPattern = new Object();
          if (this._element.TryGetCurrentPattern(TextPattern.Pattern, out textPattern)) {
            TextPatternRange doc = ((TextPattern)textPattern).DocumentRange;
            int length = doc.GetText(-1).Length;
            if (value <= length) {
              doc.MoveEndpointByUnit(TextPatternRangeEndpoint.End, TextUnit.Character, -1 * value);
              doc.MoveEndpointByUnit(TextPatternRangeEndpoint.Start, TextUnit.Character, value);
              doc.Select();
            }
          } else {
            RaiseTextPatternError(textPattern);
          }
        }
      }
    }

    public uint getCharacterCount() {
      int length = Text.Length;
      return length > 0 ? (uint)length : 0;
    }

    private String GetSelectedText() {
      if (!Valid)
        return String.Empty;

      if (this._element.Current.IsEnabled && this._element.Current.IsKeyboardFocusable) {
        Object textPattern = new Object();
        if (this._element.TryGetCurrentPattern(TextPattern.Pattern, out textPattern)) {
          this._element.SetFocus();
          if (((TextPattern)textPattern).SupportedTextSelection == SupportedTextSelection.None) {
            RaiseTextSelectionError(textPattern);
          }
          TextPatternRange[] range = ((TextPattern)textPattern).GetSelection();
          if (range != null && range.Length > 0)
            return range[0].GetText(-1);
        } else {
          RaiseTextPatternError(textPattern);
        }
      } else {
        RaiseControlAccessError();
      }
      return String.Empty;
    }

    public void SetSelectionRange(ulong start, ulong end) {
      if (!Valid)
        return;

      if (this._element.Current.IsEnabled && this._element.Current.IsKeyboardFocusable) {
        Object textPattern = new Object();
        if (this._element.TryGetCurrentPattern(TextPattern.Pattern, out textPattern)) {
          if (((TextPattern)textPattern).SupportedTextSelection == SupportedTextSelection.None) {
            RaiseTextSelectionError(textPattern);
          }
          TextPatternRange doc = ((TextPattern)textPattern).DocumentRange;
          uint length = (uint)doc.GetText(-1).Length;
          if (start >= end || start >= length) {
            if (((TextPattern)textPattern).SupportedTextSelection == SupportedTextSelection.Multiple) {
              TextPatternRange[] range = ((TextPattern)textPattern).GetSelection();
              foreach (TextPatternRange item in range) {
                item.RemoveFromSelection();
              }
              return;
            } else {
              doc.MoveEndpointByUnit(TextPatternRangeEndpoint.Start, TextUnit.Character, (int)0);
              doc.MoveEndpointByUnit(TextPatternRangeEndpoint.End, TextUnit.Character, -1 * (int)length);
              doc.Select();
            }
          }
          ulong offset = (end < length) ? length - end : 0;
          doc.MoveEndpointByUnit(TextPatternRangeEndpoint.Start, TextUnit.Character, (int)start);
          doc.MoveEndpointByUnit(TextPatternRangeEndpoint.End, TextUnit.Character, -1 * (int)offset);
          doc.Select();
        } else {
          RaiseTextPatternError(textPattern);
        }
      } else {
        RaiseControlAccessError();
      }
    }

    public void GetSelectionRange(out ulong start, out ulong end) {
      start = 0; end = 0;
      if (!Valid)
        return;

      if (this._element.Current.IsEnabled && this._element.Current.IsKeyboardFocusable) {
        Object textPattern = new Object();
        if (this._element.TryGetCurrentPattern(TextPattern.Pattern, out textPattern)) {
          this._element.SetFocus();
          if (((TextPattern)textPattern).SupportedTextSelection == SupportedTextSelection.None) {
            string message = "The control with an AutomationID of " + this._element.Current.AutomationId;
            if (textPattern == null) {
              message += " does not support text selection.\n";
              throw new Exception(message);
            }
          }
          TextPatternRange doc = ((TextPattern)textPattern).DocumentRange;
          int length = doc.GetText(-1).Length;
          TextPatternRange[] range = ((TextPattern)textPattern).GetSelection();
          TextPatternRange copy = range[0].Clone();
          start = (ulong)copy.MoveEndpointByUnit(TextPatternRangeEndpoint.Start, TextUnit.Character, (int)length);
          if (start <= (ulong)length)
            start = (ulong)length - start;
          copy = range[0].Clone();
          end = (ulong)copy.MoveEndpointByUnit(TextPatternRangeEndpoint.End, TextUnit.Character, (int)length);
          if (end <= (ulong)length)
            end = (ulong)length - end;
        } else {
          RaiseTextPatternError(textPattern);
        }
      } else {
        RaiseControlAccessError();
      }
    }

    #endregion

    #region Tab control

    public void ActivateControl() {
      if (!Valid)
        return;

      if (this._element.Current.ControlType == ControlType.TabItem)
        ((SelectionItemPattern)_element.GetCurrentPattern(SelectionItemPattern.Pattern)).Select();

      if (this._element.Current.ControlType == ControlType.MenuItem)
        ((InvokePattern)_element.GetCurrentPattern(InvokePattern.Pattern)).Invoke();
    }

    public String ActiveTabPage {
      get {
        if (!Valid || this._element.Current.ControlType != ControlType.Tab)
          return string.Empty;

        AutomationElement child = _treeWalker.GetFirstChild(_element);
        while (child != null) {
          Object selectionPattern;
          if (child.TryGetCurrentPattern(SelectionItemPattern.Pattern, out selectionPattern)) {
            if ((selectionPattern as SelectionItemPattern).Current.IsSelected)
              return child.Current.Name;
          } else {
            RaiseSelectionItemPatternError(selectionPattern);
          }

          child = _treeWalker.GetNextSibling(child);
        }

        return string.Empty;
      }

      set {
        if (!Valid || this._element.Current.ControlType != ControlType.Tab)
          return;

        AutomationElement child = _treeWalker.GetFirstChild(_element);
        while (child != null) {
          if (child.Current.Name == value) {
            Object selectionPattern;
            if (child.TryGetCurrentPattern(SelectionItemPattern.Pattern, out selectionPattern)) {
              (selectionPattern as SelectionItemPattern).Select();
            } else {
              RaiseSelectionItemPatternError(selectionPattern);
            }

            break;
          }
          child = _treeWalker.GetNextSibling(child);
        }
      }
    }

    public bool IsActiveTabPage {
      get {
        if (!Valid)
          return false;

        Object selectionPattern;
        if (_element.TryGetCurrentPattern(SelectionItemPattern.Pattern, out selectionPattern)) {
          return ((SelectionItemPattern)selectionPattern).Current.IsSelected;
        } else {
          RaiseSelectionItemPatternError(selectionPattern);
          return false;
        }
      }
    }

    public AccessibleNet[] TabPages() {
      List<AccessibleNet> list = new List<AccessibleNet>();
      if (!Valid)
        return list.ToArray();

      AutomationElement child = _treeWalker.GetFirstChild(_element);
      while (child != null) {
        list.Add(new AccessibleNet(child, _treeWalker));
        child = _treeWalker.GetNextSibling(child);
      }

      return list.ToArray();
    }

    #endregion

    #region Expand and collapse 

    public bool IsExpandable() {
      if (!Valid)
        return false;

      if (this._element.Current.IsEnabled) {
        Object expandCollapsePattern = new Object();
        if (this._element.TryGetCurrentPattern(ExpandCollapsePattern.Pattern, out expandCollapsePattern)) {
          return true;
        }
      }
      return false;
    }

    public bool IsExpanded() {
      if (!Valid)
        return false;

      if (this._element.Current.IsEnabled && this._element.Current.IsKeyboardFocusable) {
        Object expandCollapsePattern = new Object();
        if (this._element.TryGetCurrentPattern(ExpandCollapsePattern.Pattern, out expandCollapsePattern)) {
          this._element.SetFocus();
          if (((ExpandCollapsePattern)expandCollapsePattern).Current.ExpandCollapseState == ExpandCollapseState.Expanded)
            return true;
          else
            return false;
        } else {
          RaiseExpandCollapsePatternError(expandCollapsePattern);
        }
      }
      return false;
    }

    public void SetExpanded(bool value) {
      if (!Valid)
        return;

      if (this._element.Current.IsEnabled && this._element.Current.IsKeyboardFocusable) {
        Object expandCollapsePattern = new Object();
        if (this._element.TryGetCurrentPattern(ExpandCollapsePattern.Pattern, out expandCollapsePattern)) {
          this._element.SetFocus();
          if (value) {
            if (((ExpandCollapsePattern)expandCollapsePattern).Current.ExpandCollapseState != ExpandCollapseState.Expanded)
              ((ExpandCollapsePattern)expandCollapsePattern).Expand();
          } else {
            if (((ExpandCollapsePattern)expandCollapsePattern).Current.ExpandCollapseState != ExpandCollapseState.Collapsed)
              ((ExpandCollapsePattern)expandCollapsePattern).Collapse();
          }
        } else {
          RaiseExpandCollapsePatternError(expandCollapsePattern);
        }
      }
    }

    #endregion

    #region Tree/List/Grid view 

    public AccessibleNet ContainingRow {
      get {
        var row = new AccessibleNet(_element, _treeWalker);
        row.IsVirtualRow = true;
        return row;
      }
    }


    public AccessibleNet[] Columns() {
      List<AccessibleNet> list = new List<AccessibleNet>();
      if (!Valid)
        return list.ToArray();
      if (_element.Current.ControlType == ControlType.Tree) {
        var element = new AccessibleNet(this._element, _treeWalker);
        element.IsVirtualColumn = true;
        list.Add(element);
      } else if (_element.Current.ControlType == ControlType.Table) {
        HasHeader = false;
        Condition condition = new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.Custom);
        AutomationElement child = _element.FindFirst(TreeScope.Children, condition);
        if (child != null) {
          var item = _treeWalker.GetFirstChild(child);
          if (item != null && item.Current.ControlType == ControlType.Header)
            HasHeader = true;
          int idx = 0;
          while (item != null) {
            var column = new AccessibleNet(child, _treeWalker);
            column.IsVirtualColumn = true;
            column.ColumnId = idx++;
            list.Add(column);
            item = _treeWalker.GetNextSibling(item);
          }
        }
      }
      return list.ToArray();
    }
    public AccessibleNet[] ColumnEntries() {
      List<AccessibleNet> list = new List<AccessibleNet>();
      if (!Valid)
        return list.ToArray();
      var controlType = ControlType.TreeItem;
      if (_element.Current.ControlType == ControlType.Tree && IsVirtualColumn) {
        Condition condition = new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.TreeItem);
        var children = _element.FindAll(TreeScope.Subtree, condition);
        foreach (AutomationElement item in children) {
          list.Add(new AccessibleNet(item, _treeWalker));
        }
      } else {
        AutomationElement parent = _treeWalker.GetParent(_element);
        if (_element.Current.ControlType == ControlType.Custom && IsVirtualColumn && parent.Current.ControlType == ControlType.Table) {
          var element = _element;
          while (element != null && element.Current.ControlType == ControlType.Custom) { //skip scroll bars
            int idx = 0;
            var item = _treeWalker.GetFirstChild(element);
            while (item != null && item.Current.ControlType != ControlType.Header) {
              if (idx == ColumnId) {
                var cell = new AccessibleNet(item, _treeWalker);
                cell.IsCell = true;
                list.Add(cell);
                break;
              }
              idx++;
              item = _treeWalker.GetNextSibling(item);
            }
            element = _treeWalker.GetNextSibling(element);
          }
        }
      }
      return list.ToArray();
    }
    public AccessibleNet ColumnHeader() {
      if (!Valid)
        return null;
      AutomationElement parent = _treeWalker.GetParent(_element);
      if (_element.Current.ControlType == ControlType.Custom && IsVirtualColumn && parent.Current.ControlType == ControlType.Table) {
        var item = _treeWalker.GetFirstChild(_element);
        int idx = 0;
        while (item != null && item.Current.ControlType == ControlType.Header) {
          if (idx == ColumnId) {
            return new AccessibleNet(item, _treeWalker);
          }
          idx++;
          item = _treeWalker.GetNextSibling(item);
        }
      }
      return null;
    }

    public AccessibleNet[] Rows() {
      List<AccessibleNet> list = new List<AccessibleNet>();
      if (!Valid)
        return list.ToArray();

      if (_element.Current.ControlType == ControlType.Tree) {
        Condition condition = new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.TreeItem);
        var children = _element.FindAll(TreeScope.Subtree, condition);
        foreach (AutomationElement item in children) {
          var row = new AccessibleNet(item, _treeWalker);
          row.IsVirtualRow = true;
          list.Add(row);
        }

      } else if (_element.Current.ControlType == ControlType.Table) {
        Condition condition = new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.Custom);
        AutomationElementCollection children = _element.FindAll(TreeScope.Children, condition);
        if (children != null && children.Count > 0) {
          foreach (AutomationElement child in children) {
            var item = _treeWalker.GetFirstChild(child);
            if (item != null && item.Current.ControlType == ControlType.Header) // skip header row
              continue;
            var row = new AccessibleNet(child, _treeWalker);
            row.IsVirtualRow = true;
            list.Add(row);
          }
        }
      }
      return list.ToArray();
    }

    public AccessibleNet[] RowEntries() {
      List<AccessibleNet> list = new List<AccessibleNet>();
      if (!Valid)
        return list.ToArray();

      if (_element.Current.ControlType == ControlType.TreeItem) {
        list.Add(new AccessibleNet(_element, _treeWalker));
      } else {
        AutomationElement parent = _treeWalker.GetParent(_element);
        if (_element.Current.ControlType == ControlType.Custom && parent != null && parent.Current.ControlType == ControlType.Table) {
          var item = _treeWalker.GetFirstChild(_element);
          while (item != null) {
            var cell = new AccessibleNet(item, _treeWalker);
            cell.IsCell = true;
            list.Add(cell);
            item = _treeWalker.GetNextSibling(item);
          }
        }
      }
      return list.ToArray();
    }

    public bool Selected {
      get {
        if (!Valid)
          return false;
        if (IsVirtualColumn)
          return false;
        Object selectionPattern = new Object();
        if (_element.TryGetCurrentPattern(SelectionItemPattern.Pattern, out selectionPattern)) {
          return ((SelectionItemPattern)selectionPattern).Current.IsSelected;
        }
        return false;
      }
      set {
        Object selectionPattern = new Object();
        if (!IsVirtualColumn && _element.TryGetCurrentPattern(SelectionItemPattern.Pattern, out selectionPattern)) {
          ((SelectionItemPattern)selectionPattern).Select();
        }
      }
    }

    public ulong[] SelectedIndexes {
      get {
        var list = new List<ulong>();
        Object selectionPattern = new Object();
        var element = this._element;
        if (element.Current.ControlType == ControlType.ComboBox) {
          element = element.FindFirst(TreeScope.Children, new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.List));
          if (element == null)
            return list.ToArray();
        }
        AutomationElementCollection listItems = element.FindAll(TreeScope.Children, new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.ListItem));
        AutomationElement[] elements = new AutomationElement[listItems.Count];
        listItems.CopyTo(elements, 0);
        if (element.TryGetCurrentPattern(SelectionPattern.Pattern, out selectionPattern)) {
          AutomationElement[] selected = ((SelectionPattern)selectionPattern).Current.GetSelection();
          foreach (AutomationElement automationElement in selected) {
            int idx = Array.IndexOf(elements, automationElement);
            if (idx >= 0)
              list.Add((ulong)idx);
          }
        } else {
          RaiseSelectionItemPatternError(selectionPattern);
        }
        return list.ToArray();
      }
      set {
        var element = this._element;
        if (element.Current.ControlType == ControlType.ComboBox) {
          element = element.FindFirst(TreeScope.Children, new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.List));
          if (element == null)
            return;
        }
        AutomationElementCollection listItems = element.FindAll(TreeScope.Children, new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.ListItem));
        bool firstTry = true;
        foreach (int idx in value) {
          if (idx < listItems.Count) {
            var item = listItems[idx];
            Object selectionPattern = new Object();
            if (item.TryGetCurrentPattern(SelectionItemPattern.Pattern, out selectionPattern)) {
              if (firstTry) {
                ((SelectionItemPattern)selectionPattern).Select();
                firstTry = false;
                continue;
              }
              ((SelectionItemPattern)selectionPattern).AddToSelection();
            }
          }
        }
      }
    }

    #endregion

    #region ComboBox

    private void SetComboBoxValue(String value) {
      bool collapseOnFinish = false;
      if (!IsExpanded()) {
        SetExpanded(true);
        collapseOnFinish = true;
      }
      Condition condition1 = new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.ListItem);
      Condition condition2 = new PropertyCondition(AutomationElement.NameProperty, value);
      AutomationElement item = _element.FindFirst(TreeScope.Subtree, new AndCondition(condition1, condition2));
      Object selectionPattern = new Object();
      if (item != null && item.TryGetCurrentPattern(SelectionItemPattern.Pattern, out selectionPattern)) {
        ((SelectionItemPattern)selectionPattern).Select();
      }
      if (collapseOnFinish)
        SetExpanded(false);
    }

    #endregion

    #region Progress / Slider

    private double GetRangeValue(ValueRangeKind kind) {
      if (!Valid)
        return 0;
      double value = 0;
      Object rangePattern = new Object();
      if (this._element.TryGetCurrentPattern(RangeValuePattern.Pattern, out rangePattern)) {
        switch (kind) {
          case ValueRangeKind.NormalValue:
            value = ((RangeValuePattern)rangePattern).Current.Value;
            break;
          case ValueRangeKind.MinValue:
            value = ((RangeValuePattern)rangePattern).Current.Minimum;
            break;
          case ValueRangeKind.MaxValue:
            value = ((RangeValuePattern)rangePattern).Current.Maximum;
            break;
          default:
            value = ((RangeValuePattern)rangePattern).Current.Value;
            break;
        }
      } else {
        RaiseValueRangePatternError(rangePattern);
      }
      return value;
    }


    private void SetRangeValue(double value) {
      if (!Valid)
        return;
      if (this._element.Current.IsEnabled) {
        Object rangePattern = new Object();
        if (this._element.TryGetCurrentPattern(RangeValuePattern.Pattern, out rangePattern)) {
          Object valuePattern = new Object();
          if (this.ControlType == ControlType.Slider && value < 0)
            value = 0;
          ((RangeValuePattern)rangePattern).SetValue(value);
        } else {
          RaiseValueRangePatternError(rangePattern);
        }
      } else {
        RaiseControlAccessError();
      }
    }

    public double MaxValue => GetRangeValue(ValueRangeKind.MaxValue);

    public double MinValue => GetRangeValue(ValueRangeKind.MinValue);

    #endregion

    #region Spinner control

    public double Value {
      get {
        if (this.ControlType == ControlType.Spinner) {
          return InternalSpinnerGetValue();
        } else {
          return GetRangeValue(ValueRangeKind.NormalValue);
        }
      }
      set {
        if (this.ControlType == ControlType.Spinner) {
          InternalSpinnerSetValue(value);
        } else {
          SetRangeValue(value);
        }
      }
    }

    public void StepUp() {
      if (!Valid || this._element.Current.ControlType != ControlType.Spinner)
        return;
      InternalStepperClick("Forward");
    }

    public void StepDown() {
      if (!Valid || this._element.Current.ControlType != ControlType.Spinner)
        return;
      InternalStepperClick("Backward");
    }
    #endregion

    #region Slider 

    public void Increment() {
      var maxValue = GetRangeValue(ValueRangeKind.MaxValue);
      var minValue = GetRangeValue(ValueRangeKind.MinValue);
      var value = GetRangeValue(ValueRangeKind.NormalValue);
      if (value + 1 >= minValue && value + 1 <= maxValue) {
        Value = value + 1;
      }
    }
    public void Decrement() {
      var maxValue = GetRangeValue(ValueRangeKind.MaxValue);
      var minValue = GetRangeValue(ValueRangeKind.MinValue);
      var value = GetRangeValue(ValueRangeKind.NormalValue);
      if (value - 1 >= minValue && value - 1 <= maxValue) {
        Value = value - 1;
      }
    }

    #endregion

    #region Scrolling

    public bool Scrollable {
      get {
        Object scrollPattern = new Object();
        if (this._element.TryGetCurrentPattern(ScrollPattern.Pattern, out scrollPattern)) {
          return ((ScrollPattern)scrollPattern).Current.HorizontallyScrollable ||
            ((ScrollPattern)scrollPattern).Current.HorizontallyScrollable;
        }
        return false;
      }
    }

    public void ScrollUp() {
      ScrollPattern scrollPattern = GetScrollPaternInternal();
      if (scrollPattern != null) {
        scrollPattern.ScrollVertical(ScrollAmount.LargeDecrement);
      }
    }

    public void ScrollDown() {
      ScrollPattern scrollPattern = GetScrollPaternInternal();
      if (scrollPattern != null) {
        scrollPattern.ScrollVertical(ScrollAmount.LargeIncrement);
      }
    }

    public void ScrollLeft() {
      ScrollPattern scrollPattern = GetScrollPaternInternal();
      if (scrollPattern != null) {
        scrollPattern.ScrollHorizontal(ScrollAmount.LargeDecrement);
      }
    }

    public void ScrollRight() {
      ScrollPattern scrollPattern = GetScrollPaternInternal();
      if (scrollPattern != null) {
        scrollPattern.ScrollHorizontal(ScrollAmount.LargeIncrement);
      }
    }

    private ScrollPattern GetScrollPaternInternal() {
      if (!Valid)
        return null;

      if (this._element.Current.IsEnabled) {
        Object scrollPattern = new Object();
        if (this._element.TryGetCurrentPattern(ScrollPattern.Pattern, out scrollPattern)) {
          return (ScrollPattern)scrollPattern;
        } else {
          RaiseScrollPatternError(scrollPattern);
        }
      } else {
        RaiseControlAccessError();
      }
      return null;
    }

    public AccessibleNet VerticalScrollBar {
      get {
        ScrollPattern pattern = GetScrollPaternInternal();
        if (pattern.Current.VerticallyScrollable) {
          Condition condition1 = new PropertyCondition(AutomationElement.OrientationProperty, OrientationType.Vertical);
          Condition condition2 = new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.ScrollBar);
          Condition condition = new AndCondition(condition1, condition2);
          TreeWalker treeWalker = new TreeWalker(condition);
          AutomationElement child = treeWalker.GetFirstChild(_element);
          if (child != null) {
            return new AccessibleNet(child, _treeWalker);
          }
        }
        return null;
      }
    }

    public AccessibleNet HorizontalScrollBar {
      get {
        ScrollPattern pattern = GetScrollPaternInternal();
        if (pattern.Current.HorizontallyScrollable) {
          Condition condition1 = new PropertyCondition(AutomationElement.OrientationProperty, OrientationType.Horizontal);
          Condition condition2 = new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.ScrollBar);
          Condition condition = new AndCondition(condition1, condition2);
          TreeWalker treeWalker = new TreeWalker(condition);
          AutomationElement child = treeWalker.GetFirstChild(_element);
          if (child != null) {
            return new AccessibleNet(child, _treeWalker);
          }
        }
        return null;
      }
    }

    public double ScrollPosition {
      get {
        return GetRangeValue(ValueRangeKind.NormalValue);
      }
      set {
        SetRangeValue(value);
      }
    }

    public bool IsHorizontal {
      get {
        if (!Valid)
          return false;
        return _element.Current.Orientation == OrientationType.Horizontal;
      }
    }

    #endregion

    #region Mouse click, move functions

    public enum MouseMovePosition { Absolute = 1, Relative = 2 };
    public enum MouseButton { Left = 1, Right = 2, Middle = 3 };
    private enum ValueRangeKind { NormalValue, MinValue, MaxValue };

    public bool Click() {
      if (!Valid)
        return false;

      InternalClickElement(_element);
      return true;
    }

    public ToggleState CheckState {
      get {
        if (!Valid)
          return ToggleState.Indeterminate;
        Object togglePattern = new Object();
        if (this._element.TryGetCurrentPattern(TogglePattern.Pattern, out togglePattern)) {
          return ((TogglePattern)togglePattern).Current.ToggleState;
        } else {
          if (this._element.Current.ControlType == ControlType.RadioButton) {
            if (((SelectionItemPattern)_element.GetCurrentPattern(SelectionItemPattern.Pattern)).Current.IsSelected)
              return ToggleState.On;
            else
              return ToggleState.Off;
          } else {
            RaiseTogglePatternError(togglePattern);
          }
        }
        return ToggleState.Indeterminate;
      }

      set {
        if (!Valid)
          return;
        Object togglePattern = new Object();
        if (this._element.TryGetCurrentPattern(TogglePattern.Pattern, out togglePattern)) {
          var oldState = ((TogglePattern)togglePattern).Current.ToggleState;
          if (oldState == value)
            return;

          ((TogglePattern)togglePattern).Toggle();
          if (((TogglePattern)togglePattern).Current.ToggleState == value)
            return;

          ((TogglePattern)togglePattern).Toggle();
          if (((TogglePattern)togglePattern).Current.ToggleState == value)
            return;

          if (value == ToggleState.Indeterminate)
            throw new Exception("Indeterminate state is not supported");
        } else {
          if (this._element.Current.ControlType == ControlType.RadioButton) {
            throw new Exception("Setting state is not supported");
          } else {
            RaiseTogglePatternError(togglePattern);
          }
        }
      }
    }

    public void MouseDown(System.Drawing.Point pt, MouseButton button) {
      uint flag = 0;
      switch (button) {
        case MouseButton.Left:
          flag = MOUSEEVENTF_LEFTDOWN;
          break;
        case MouseButton.Right:
          flag = MOUSEEVENTF_RIGHTDOWN;
          break;
        case MouseButton.Middle:
          flag = MOUSEEVENTF_MIDDLEDOWN;
          break;
        default:
          return;
      }

      MouseMove(pt, MouseMovePosition.Absolute);
      INPUT input = new INPUT() { type = 0, mi = new MOUSEINPUT() { dx = 0, dy = 0, dwFlags = flag, dwExtraInfo = IntPtr.Zero } };
      INPUT[] inputs = new INPUT[] { input };
      if (SendInput(1, inputs, Marshal.SizeOf(typeof(INPUT))) == 0)
        throw new Exception("SendInput failed. Cannot release mouse button.");
    }

    public void MouseUp(System.Drawing.Point pt, MouseButton button) {
      uint flag = 0;
      switch (button) {
        case MouseButton.Left:
          flag = MOUSEEVENTF_LEFTUP;
          break;
        case MouseButton.Right:
          flag = MOUSEEVENTF_RIGHTUP;
          break;
        case MouseButton.Middle:
          flag = MOUSEEVENTF_MIDDLEUP;
          break;
        default:
          return;
      }

      MouseMove(pt, MouseMovePosition.Absolute);
      INPUT input = new INPUT() { type = 0, mi = new MOUSEINPUT() { dx = 0, dy = 0, dwFlags = flag, dwExtraInfo = IntPtr.Zero } };
      INPUT[] inputs = new INPUT[] { input };
      if (SendInput(1, inputs, Marshal.SizeOf(typeof(INPUT))) == 0)
        throw new Exception("SendInput failed. Cannot release mouse button.");
    }

    public void MouseMove(System.Drawing.Point pt, MouseMovePosition position) {
      IntPtr desktop = GetDesktopWindow();
      int width = Screen.PrimaryScreen.Bounds.Size.Width;
      int height = Screen.PrimaryScreen.Bounds.Size.Height;
      TransformUsingScaling(width, height, out width, out height);

      // Normalized absolute coordinates always range between (0,0) in the top-left corner to (65535,65535) 
      // in the bottom-right corner, no matter what the desktop size happens to be.
      if (position != MouseMovePosition.Absolute) {
        System.Drawing.Point point = Cursor.Position;
        pt.X = point.X + pt.X;
        pt.Y = point.Y + pt.Y;
      }
      Cursor.Position = pt;
    }

    public System.Drawing.Point MousePosition() => Cursor.Position;

    #endregion

    #region Key press functions

    public void KeyDown(uint code, bool extended) {
      uint flags = EVENTF_SCANCODE;
      if (extended)
        flags |= KEYEVENTF_EXTENDEDKEY;
      InternalKeyAction(code, flags);
    }

    public void KeyUp(uint code, bool extended) {
      uint flags = EVENTF_KEYUP | EVENTF_SCANCODE;
      if (extended)
        flags = KEYEVENTF_EXTENDEDKEY;
      InternalKeyAction(code, flags);
    }

    public void TypeString(string input) {
      throw new NotImplementedException();
    }

    public bool IsVisible {
      get {
        if (!Valid)
          return false;

        return !_element.Current.IsOffscreen;
      }
    }

    public void SetFocused() {
      if (!Valid)
        return;

      _element.SetFocus();

      // Need to read FocusedElement once to update it, so it points to _element on return.
      AutomationElement e = AutomationElement.FocusedElement;
    }

    public bool IsFocused {
      get {
        if (!Valid)
          return false;

        if (_element.Current.ControlType == ControlType.ComboBox && AutomationElement.FocusedElement.Current.Name == _element.Current.Name) {
          AutomationElement child = _treeWalker.GetFirstChild(_element);
          while (child != null) {
            bool ret = child.Equals(AutomationElement.FocusedElement);
            if (ret)
              return true;
            child = _treeWalker.GetNextSibling(child);
          }
        } else {
          return _element.Equals(AutomationElement.FocusedElement);
        }
        return false;
      }
    }

    public bool CanFocus {
      get {
        if (!Valid)
          return false;

        return _element.Current.IsKeyboardFocusable;
      }
    }

    #endregion

    #region Others

    public void BringToFront() {
      throw new NotImplementedException();
    }

    public void ShowMenu() {
      throw new NotImplementedException();
    }

    public bool Equals(AccessibleNet other) => other._element.Equals(_element);

    public void Highlight(bool remove) {
      if (!Valid)
        return;

      if (!_element.Current.IsOffscreen) {
        Rect rect = _element.Current.BoundingRectangle;
        int x = (int)rect.X - 1;
        int y = (int)rect.Y - 1;
        TransformUsingScaling(x, y, out x, out y);
        int width = (int)rect.Width + 2;
        int height = (int)rect.Height + 2;
        TransformUsingScaling(width, height, out width, out height);
        if (_isHighlighted && remove) {
          ControlPaint.DrawReversibleFrame(new Rectangle(x, y, width, height), Color.FromArgb(72, 255, 0), FrameStyle.Thick);
          _isHighlighted = false;
          return;
        }
        if (!_isHighlighted) {
          ControlPaint.DrawReversibleFrame(new Rectangle(x, y, width, height), Color.FromArgb(72, 255, 0), FrameStyle.Thick);
        } else {
          ControlPaint.DrawReversibleFrame(new Rectangle(x, y, width, height), Color.FromArgb(72, 255, 0), FrameStyle.Thick);
          ControlPaint.DrawReversibleFrame(new Rectangle(x, y, width, height), Color.FromArgb(72, 255, 0), FrameStyle.Thick);
        }
        _isHighlighted = true;
      }
    }

    public void PrintNativeInfo() {
      var parents = new List<string>();
      var run = Parent;
      while (run != null && run.Valid) {
        parents.Insert(0, run.ControlType.ToString() + " :: " + run.Name);
        run = run.Parent;
      }
      int i = 0;
      foreach (var item in parents) {
        for (var j = 0; j < i; ++j)
          Console.Write("*");
        i++;
        Console.WriteLine(" " + item);
      }
      Console.WriteLine("--------------------------------------------");
      Console.WriteLine("ControlType: " + this._element.Current.ControlType.ProgrammaticName);
      Console.WriteLine("FrameworkId: " + this._element.Current.FrameworkId);
      Console.WriteLine("Orientation: " + this._element.Current.Orientation);
      Console.WriteLine("IsOffscreen: " + this._element.Current.IsOffscreen);
      Console.WriteLine("ProcessId: " + this._element.Current.ProcessId);
      Console.WriteLine("NativeWindowHandle: " + this._element.Current.NativeWindowHandle);
      Console.WriteLine("ClassName: " + this._element.Current.ClassName);
      Console.WriteLine("IsPassword: " + this._element.Current.IsPassword);
      Console.WriteLine("ItemType: " + this._element.Current.ItemType);
      Console.WriteLine("AutomationId: " + this._element.Current.AutomationId);
      Console.WriteLine("LabeledBy: " + this._element.Current.LabeledBy);
      Console.WriteLine("IsContentElement: " + this._element.Current.IsContentElement);
      Console.WriteLine("IsControlElement: " + this._element.Current.IsControlElement);
      Console.WriteLine("HelpText: " + this._element.Current.HelpText);
      Console.WriteLine("BoundingRectangle: " + this._element.Current.BoundingRectangle);
      Console.WriteLine("IsEnabled: " + this._element.Current.IsEnabled);
      Console.WriteLine("IsKeyboardFocusable: " + this._element.Current.IsKeyboardFocusable);
      Console.WriteLine("HasKeyboardFocus: " + this._element.Current.HasKeyboardFocus);
      Console.WriteLine("AccessKey: " + this._element.Current.AccessKey);
      Console.WriteLine("AcceleratorKey: " + this._element.Current.AcceleratorKey);
      Console.WriteLine("Name: " + this._element.Current.Name);
      Console.WriteLine("IsRequiredForForm: " + this._element.Current.IsRequiredForForm);
      Console.WriteLine("ItemStatus: " + this._element.Current.ItemStatus);
      Console.WriteLine("------Supported AutomationPattern ----------");
      foreach (AutomationPattern item in this._element.GetSupportedPatterns()) {
        Console.WriteLine("Id: " + item.Id);
        Console.WriteLine("ProgrammaticName: " + item.ProgrammaticName);
      }
      Console.WriteLine("------Supported AutomationProperty ---------");
      foreach (AutomationProperty item in this._element.GetSupportedProperties()) {
        Console.WriteLine("Id: " + item.Id);
        Console.WriteLine("ProgrammaticName: " + item.ProgrammaticName);
      }
    }

    #endregion

    #region Screen shots

    // P/Invoke declarations
    [DllImport("gdi32.dll")]
    static extern bool BitBlt(IntPtr hdcDest, int xDest, int yDest, int
    wDest, int hDest, IntPtr hdcSource, int xSrc, int ySrc, CopyPixelOperation rop);
    [DllImport("gdi32.dll")]
    static extern IntPtr DeleteDC(IntPtr hDc);
    [DllImport("gdi32.dll")]
    static extern IntPtr DeleteObject(IntPtr hDc);
    [DllImport("gdi32.dll")]
    static extern IntPtr CreateCompatibleBitmap(IntPtr hdc, int nWidth, int nHeight);
    [DllImport("gdi32.dll")]
    static extern IntPtr CreateCompatibleDC(IntPtr hdc);
    [DllImport("gdi32.dll")]
    static extern IntPtr SelectObject(IntPtr hdc, IntPtr bmp);
    [DllImport("user32.dll")]
    public static extern IntPtr GetDesktopWindow();
    [DllImport("user32.dll")]
    public static extern IntPtr GetWindowDC(IntPtr ptr);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
      public int Left;
      public int Top;
      public int Right;
      public int Bottom;
    }
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hwnd, out RECT rectangle);


    public void TakeScreenShot(String path, bool onlyWindow, Rect rect) {
      IntPtr window;
      int width;
      int height;
      if (onlyWindow) {
        Process process = Process.GetProcessById(_element.Current.ProcessId);
        window = process.MainWindowHandle;
        RECT rectangle;
        GetWindowRect(window, out rectangle);
        width = rectangle.Right - rectangle.Left;
        height = rectangle.Bottom - rectangle.Top;
      } else {
        window = GetDesktopWindow();
        width = Screen.PrimaryScreen.Bounds.Size.Width;
        height = Screen.PrimaryScreen.Bounds.Size.Height;
      }
      if (rect.Width > 0)
        width = (int)rect.Width;
      if (rect.Height > 0)
        height = (int)rect.Height;
      TransformUsingScaling(width, height, out width, out height);
      IntPtr screen = GetWindowDC(window);
      IntPtr hDest = CreateCompatibleDC(screen);
      IntPtr bitmap = CreateCompatibleBitmap(screen, width, height);
      IntPtr hOldBmp = SelectObject(hDest, bitmap);
      int x = (int)rect.X;
      int y = (int)rect.Y;
      TransformUsingScaling(x, y, out x, out y);
      bool b = BitBlt(hDest, x, y, width, height, screen, 0, 0,
        CopyPixelOperation.SourceCopy | CopyPixelOperation.CaptureBlt);
      Bitmap bmp = Image.FromHbitmap(bitmap);
      SelectObject(hDest, hOldBmp);
      DeleteObject(bitmap);
      DeleteDC(hDest);
      ReleaseDC(window, screen);
      bmp.Save(path);
      bmp.Dispose();
    }

    #endregion

    #region error reporting

    private void RaiseExpandCollapsePatternError(object expandCollapsePattern) {
      string message = "The control with an AutomationID of " + this._element.Current.AutomationId;
      if (expandCollapsePattern == null) {
        message += " does not support the ExpandCollapsePattern.\n";
        throw new Exception(message);
      }
      message += " is not accessible. \n";
      throw new Exception(message);
    }

    private void RaiseTogglePatternError(object togglePattern) {
      string message = "The control with an AutomationID of " + this._element.Current.AutomationId;
      if (togglePattern == null) {
        message += " does not support the TogglePattern.\n";
        throw new Exception(message);
      }
      message += " is not accessible. \n";
      throw new Exception(message);
    }

    private void RaiseControlAccessError() {
      string message = "The control with an AutomationID of " + this._element.Current.AutomationId;
      if (!this._element.Current.IsEnabled) {
        message += " is not enabled.\n";
        throw new Exception(message);
      }
      if (!this._element.Current.IsKeyboardFocusable) {
        message += " is read-only.\n";
        throw new Exception(message);
      }
    }

    private void RaiseTextPatternError(object textPattern) {
      string message = "The control with an AutomationID of " + this._element.Current.AutomationId;
      if (textPattern == null) {
        message += " does not support the TextPattern.\n";
        throw new Exception(message);
      }
      message += " is not accessible. \n";
      throw new Exception(message);
    }

    private void RaiseValueRangePatternError(object rangePattern) {
      string message = "The control with an AutomationID of " + this._element.Current.AutomationId;
      if (rangePattern == null) {
        message += " does not support the RangeValuePattern.\n";
        throw new Exception(message);
      }
      message += " is not accessible. \n";
      throw new Exception(message);
    }

    private void RaiseValuePatternError(object rangePattern) {
      string message = "The control with an AutomationID of " + this._element.Current.AutomationId;
      if (rangePattern == null) {
        message += " does not support the ValuePattern.\n";
        throw new Exception(message);
      }
      message += " is not accessible. \n";
      throw new Exception(message);
    }
    void RaiseSelectionItemPatternError(object selectionPattern) {
      string message = "The control with an AutomationID of " + this._element.Current.AutomationId;
      if (selectionPattern == null) {
        message += " does not support the SelectionItemPattern.\n";
        throw new Exception(message);
      }
      message += " is not accessible. \n";
      throw new Exception(message);
    }

    private void RaiseTextSelectionError(object textPattern) {
      string message = "The control with an AutomationID of " + this._element.Current.AutomationId;
      if (textPattern == null) {
        message += " not support text selection.\n";
        throw new Exception(message);
      }
    }

    private void RaiseScrollPatternError(object scrollPattern) {
      string message = "The control with an AutomationID of " + this._element.Current.AutomationId;
      if (scrollPattern == null) {
        message += " does not support the ScrollItemPattern.\n";
        throw new Exception(message);
      }
      message += " is not accessible. \n";
      throw new Exception(message);
    }

    private void RaiseTransformPatternError(object transformPattern)
    {
      string message = "The control with an AutomationID of " + this._element.Current.AutomationId;
      if (transformPattern == null) {
        message += " does not support the TransformPattern.\n";
        throw new Exception(message);
      }
      message += " is not accessible. \n";
      throw new Exception(message);
    }

    #endregion

    #region Internal Support

    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern int RegisterWindowMessage(string lpString);

    [DllImport("user32.dll", EntryPoint = "SendMessage", CharSet = CharSet.Auto)] //
    public static extern bool SendMessage(IntPtr hWnd, uint Msg, int wParam, StringBuilder lParam);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr SendMessage(int hWnd, int Msg, int wparam, int lparam);

    private string GetControlText(IntPtr hWnd) {
      const int WM_GETTEXT = 0x000D;
      const int WM_GETTEXTLENGTH = 0x000E;
      StringBuilder title = new StringBuilder();
      // Get the size of the string required to hold the window title. 
      Int32 size = SendMessage((int)hWnd, WM_GETTEXTLENGTH, 0, 0).ToInt32();
      // If the return is 0, there is no title. 
      if (size > 0) {
        title = new StringBuilder(size + 1);
        SendMessage(hWnd, (int)WM_GETTEXT, title.Capacity, title);
      }
      return title.ToString();
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct MOUSEINPUT {
      public int dx;
      public int dy;
      public uint mouseData;
      public uint dwFlags;
      public uint time;
      public IntPtr dwExtraInfo;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct KEYBDINPUT {
      public ushort wVk;
      public ushort wScan;
      public uint dwFlags;
      public uint time;
      public IntPtr dwExtraInfo;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct HARDWAREINPUT {
#pragma warning disable CRR0026 // Unused member
      uint uMsg;
      ushort wParamL;
      ushort wParamH;
#pragma warning restore CRR0026 // Unused member
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct INPUT {
      [FieldOffset(0)]
      public int type;
      [FieldOffset(8)] //*
      public MOUSEINPUT mi;
      [FieldOffset(8)] //*
      public KEYBDINPUT ki;
      [FieldOffset(8)] //*
      public HARDWAREINPUT hi;
    }

    [DllImport("User32.dll")]
    internal static extern uint SendInput(uint numberOfInputs, [MarshalAs(UnmanagedType.LPArray, SizeConst = 1)] INPUT[] input, int structSize);

    [DllImport("user32.dll")]
    internal static extern uint MapVirtualKey(uint uCode, uint uMapType);

    internal const int MOUSEEVENTF_LEFTDOWN = 0x0002;
    internal const int MOUSEEVENTF_LEFTUP = 0x0004;
    internal const int MOUSEEVENTF_MIDDLEDOWN = 0x0020;
    internal const int MOUSEEVENTF_MIDDLEUP = 0x0040;
    internal const int MOUSEEVENTF_RIGHTDOWN = 0x0008;
    internal const int MOUSEEVENTF_RIGHTUP = 0x0010;
    internal const int MOUSEEVENTF_ABSOLUTE = 0x8000;
    internal const int MOUSEEVENTF_MOVE = 0x0001;
    internal const int MOUSEEVENTF_VIRTUALDESK = 0x4000;

    internal const int KEYEVENTF_EXTENDEDKEY = 0x0001;
    internal const int EVENTF_KEYUP = 0x0002;
    internal const int EVENTF_SCANCODE = 0x0008;
    internal const int EVENTF_UNICODE = 0x0004;

    /// <summary>
    /// Returns the text for the given element (if supported), in the given range.
    /// </summary>
    private String InternalGetText(AutomationElement element, int offset, int len) {
      if (element == null)
        return string.Empty;

      Object pattern = new Object();
      if (element.TryGetCurrentPattern(TextPattern.Pattern, out pattern)) {
        TextPatternRange range = ((TextPattern)pattern).DocumentRange;
        if (range != null) {
          range.Move(TextUnit.Character, offset);
          return range.GetText(len);
        }
      } else if (element.TryGetCurrentPattern(ValuePattern.Pattern, out pattern)) {
        var valuePattern = (ValuePattern)pattern;
        return valuePattern.Current.Value;
      } else if (element.Current.ControlType == ControlType.ComboBox && !IsEditable) {
        var listElement = element.FindFirst(TreeScope.Children, new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.List));
        if (listElement != null) {
          AutomationElementCollection listItems = listElement.FindAll(TreeScope.Children, new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.ListItem));
          Object selectionPattern = new Object();
          foreach (AutomationElement item in listItems) {
            if (item.TryGetCurrentPattern(SelectionItemPattern.Pattern, out selectionPattern)) {
              if (((SelectionItemPattern)selectionPattern).Current.IsSelected)
                return item.Current.Name;
            }
          }
        }
      } else {
        var handle = element.Current.NativeWindowHandle;
        return GetControlText((IntPtr)handle);
      }
      return string.Empty;
    }

    private bool InternalIsSelected(AutomationElement element) {
      Object selectionPattern = new Object();
      if (element.TryGetCurrentPattern(SelectionItemPattern.Pattern, out selectionPattern)) {
        return ((SelectionItemPattern)selectionPattern).Current.IsSelected;
      } else {
        RaiseSelectionItemPatternError(selectionPattern);
        return false;
      }
    }

    private static void InternalKeyAction(uint code, uint flags) {
      uint scanKey = MapVirtualKey(code, (uint)0x0);

      INPUT input = new INPUT() { type = 1, ki = new KEYBDINPUT() { wVk = 0, wScan = (ushort)scanKey, dwFlags = flags, dwExtraInfo = IntPtr.Zero } };
      INPUT[] inputs = new INPUT[] { input };
      if (SendInput(1, inputs, Marshal.SizeOf(typeof(INPUT))) == 0)
        throw new Exception("SendInput failed. Cannot simulate key press.");
    }

    private void InternalLeftClick(System.Drawing.Point pt) {
      MouseDown(pt, MouseButton.Left);
      MouseUp(pt, MouseButton.Left);
    }

    private void InternalClickElement(AutomationElement element) {
      System.Windows.Point pt;
      if (element.TryGetClickablePoint(out pt)) {
        var point = new System.Drawing.Point((int)pt.X, (int)pt.Y);
        //Cursor.Position = point;
        InternalLeftClick(point);
      };
    }

    private void InternalStepperClick(String buttonName) {
      Condition condition1 = new PropertyCondition(AutomationElement.ControlTypeProperty, ControlType.Button);
      Condition condition2 = new PropertyCondition(AutomationElement.NameProperty, buttonName);
      Condition conditionAnd = new AndCondition(condition1, condition2);
      var children = _element.FindAll(TreeScope.Children, conditionAnd);
      if (children.Count == 1) {
        System.Windows.Point pt;
        if (children[0].TryGetClickablePoint(out pt)) {
          var point = new System.Drawing.Point((int)pt.X, (int)pt.Y);
          InternalLeftClick(point);
        };
      }
    }

    private double InternalSpinnerGetValue() {
      AutomationElement child = _treeWalker.GetFirstChild(_element);
      while (child != null) {
        if (child.Current.ControlType == ControlType.Spinner && MatchClassName(child.Current.ClassName).Equals("edit", StringComparison.CurrentCultureIgnoreCase)) {
          Object textPattern = new Object();
          if (child.TryGetCurrentPattern(TextPattern.Pattern, out textPattern)) {
            return double.Parse(((TextPattern)textPattern).DocumentRange.GetText(-1));
          }
        }
        child = _treeWalker.GetNextSibling(child);
      }
      return 0;
    }
    private void InternalSpinnerSetValue(double value) {
      AutomationElement child = _treeWalker.GetFirstChild(_element);
      while (child != null) {
        if (child.Current.ControlType == ControlType.Spinner && MatchClassName(child.Current.ClassName).Equals("edit", StringComparison.CurrentCultureIgnoreCase)) {
          Object valuePattern = new Object();
          if (child.TryGetCurrentPattern(ValuePattern.Pattern, out valuePattern)) {
            ((ValuePattern)valuePattern).SetValue(value.ToString());
          } else {
            child.SetFocus();
            // Pause before sending keyboard input.
            Thread.Sleep(100);
            SendKeys.SendWait("^{HOME}");
            SendKeys.SendWait("^+{END}");
            SendKeys.SendWait("{DEL}");
            SendKeys.SendWait(value.ToString());
          }
          break;
        }
        child = _treeWalker.GetNextSibling(child);
      }
    }

    #endregion
  }
}
