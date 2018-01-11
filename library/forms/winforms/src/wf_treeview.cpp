/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/log.h"

#include "wf_base.h"
#include "wf_view.h"
#include "wf_treeview.h"
#include "wf_treenode.h"
#include "wf_menubar.h"

using namespace System::Drawing;
using namespace System::Drawing::Imaging;
using namespace System::Collections::Generic;
using namespace System::Collections::ObjectModel;
using namespace System::IO;
using namespace System::Windows::Forms;

using namespace Aga::Controls::Tree;
using namespace Aga::Controls::Tree::NodeControls;

using namespace MySQL;
using namespace MySQL::Controls;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_WRAPPER)

//----------------- ColumnComparer -----------------------------------------------------------------

/**
  * The class that does the actual text comparison for sorting.
  */
ref class ColumnComparer : public IComparer<Node ^> {
private:
  int column;
  mforms::TreeColumnType type;
  SortOrder direction;
  System::Collections::CaseInsensitiveComparer ^ inner;

public:
  ColumnComparer(int aColumn, SortOrder aDirection, mforms::TreeColumnType aType) {
    column = aColumn;
    direction = aDirection;
    inner = gcnew System::Collections::CaseInsensitiveComparer();
    type = aType;
  }

  //------------------------------------------------------------------------------------------------

  virtual int Compare(Node ^ n1, Node ^ n2) {
    MySQL::Forms::TreeViewNode ^ node1 = dynamic_cast<MySQL::Forms::TreeViewNode ^>(n1);
    MySQL::Forms::TreeViewNode ^ node2 = dynamic_cast<MySQL::Forms::TreeViewNode ^>(n2);

    switch (type) {
      case mforms::StringColumnType:
      case mforms::IconStringColumnType:
        // There's an attached string for icons, which we use to compare.
        if (direction == SortOrder::Ascending)
          return inner->Compare(node1->Caption[column], node2->Caption[column]);
        else
          return inner->Compare(node2->Caption[column], node1->Caption[column]);
        break;

      case mforms::IntegerColumnType:
      case mforms::LongIntegerColumnType:
      case mforms::CheckColumnType:
      case mforms::TriCheckColumnType: {
        Int64 i1 = 0, i2 = 0;
        Int64::TryParse(node1->Caption[column], i1);
        Int64::TryParse(node2->Caption[column], i2);
        if (direction == SortOrder::Ascending)
          return (int)(i1 - i2);
        else
          return int(i2 - i1);
        break;
      }

      case mforms::NumberWithUnitColumnType: {
        Double d1 = mforms::TreeView::parse_string_with_unit(NativeToCppStringRaw(node1->Caption[column]).c_str());
        Double d2 = mforms::TreeView::parse_string_with_unit(NativeToCppStringRaw(node2->Caption[column]).c_str());
        if (direction == SortOrder::Ascending)
          return d1.CompareTo(d2);
        else
          return d2.CompareTo(d1);
        break;
      }

      case mforms::FloatColumnType: {
        System::Double d1 = 0, d2 = 0;
        System::Double::TryParse(node1->Caption[column], d1);
        System::Double::TryParse(node2->Caption[column], d2);
        if (direction == SortOrder::Ascending)
          return d1.CompareTo(d2);
        else
          return d2.CompareTo(d1);
        break;
      }

      default:
        return 0;
    }
  }

  //------------------------------------------------------------------------------------------------
};

//----------------- SortableTreeModel --------------------------------------------------------------

/**
  * A pretty standard tree model which allows to sort nodes.
  * Note: the way sorting is handled is completely unoptimized and hence not recommended
  *       for larger node counts.
  */
ref class SortableTreeModel : Aga::Controls::Tree::TreeModel {
private:
  Collections::Generic::IComparer<Node ^> ^ _comparer;

public:
  virtual Collections::IEnumerable ^ GetChildren(TreePath ^ treePath) override {
    Node ^ node = FindNode(treePath);
    if (node != nullptr) {
      if (_comparer != nullptr)
        node->Nodes->Sort(_comparer);
      return node->Nodes;
    }

    return nullptr;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Workaround for tree manipulations not going via the model (which would trigger the
   * structure change event automatically).
   */
  void SortableTreeModel::Resort() {
    OnStructureChanged(gcnew TreePathEventArgs());
  }

  //------------------------------------------------------------------------------------------------

  property Collections::Generic::IComparer<Node ^> ^ Comparer {
    Collections::Generic::IComparer<Node ^> ^ get() { return _comparer; }

      void
      set(Collections::Generic::IComparer<Node ^> ^ value) {
      _comparer = value;
      OnStructureChanged(gcnew TreePathEventArgs());
    }
  }

  //------------------------------------------------------------------------------------------------
};

//----------------- AttributedNodeText -------------------------------------------------------------

ref class AttributedNodeText : public NodeTextBox {
protected:
  virtual void OnDrawText(DrawEventArgs ^ args) override {
    __super ::OnDrawText(args);

    TreeViewNode ^ node = dynamic_cast<TreeViewNode ^>(args->Node->Tag);

    mforms::TreeNodeTextAttributes attributes = node->Attributes[ParentColumn->Index];
    if (!attributes.bold && !attributes.italic && !attributes.color.is_valid())
      return;

    FontStyle newStyle = FontStyle::Regular;
    if (attributes.bold)
      newStyle = newStyle | FontStyle::Bold;
    if (attributes.italic)
      newStyle = newStyle | FontStyle::Italic;
    args->Font = gcnew Drawing::Font(args->Font, newStyle);

    if (attributes.color.is_valid())
      args->TextColor = Conversions::ColorToNative(attributes.color);
  }
};

//----------------- TriangleNodeControl ------------------------------------------------------------

ref class TriangleNodeControl : public NodeControl {
private:
  System::Drawing::Bitmap ^ _expanded_icon;
  System::Drawing::Bitmap ^ _collapsed_icon;
  System::Drawing::Size _size;

public:
  TriangleNodeControl() {
    _expanded_icon = gcnew Bitmap("images/ui/tree_expanded.png", true);
    _collapsed_icon = gcnew Bitmap("images/ui/tree_collapsed.png", true);
  }

  //------------------------------------------------------------------------------------------------

  virtual Size MeasureSize(TreeNodeAdv ^ node, DrawContext context) override {
    return _expanded_icon->Size;
  }

  //------------------------------------------------------------------------------------------------

  virtual void Draw(TreeNodeAdv ^ node, DrawContext context) override {
    if (node->CanExpand) {
      System::Drawing::Rectangle r = context.Bounds;
      Image ^ img;
      if (node->IsExpanded)
        img = _expanded_icon;
      else
        img = _collapsed_icon;
      int dy = (int)Math::Round((float)(r.Height - img->Height) / 2);
      context.Graphics->DrawImageUnscaled(img, System::Drawing::Point(r.X, r.Y + dy));
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void MouseDown(TreeNodeAdvMouseEventArgs ^ args) override {
    if (args->Button == MouseButtons::Left) {
      args->Handled = true;
      if (args->Node->CanExpand)
        args->Node->IsExpanded = !args->Node->IsExpanded;
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void TriangleNodeControl::MouseDoubleClick(TreeNodeAdvMouseEventArgs ^ args) override {
    args->Handled = true; // Suppress expand/collapse when double clicking.
  }

  //------------------------------------------------------------------------------------------------
};

//----------------- MformsTree ---------------------------------------------------------------------

ref struct NodeOverlay {
  bool isHot;
  Drawing::Rectangle bounds;     // For mouse hit tests.
  Drawing::Rectangle drawBounds; // For drawing (different coordinate system, hence separate).
  Image ^ image;
};

ref class MformsTree : TreeViewAdv {
public:
  SortableTreeModel ^ model;

  int currentSortColumn;
  bool canSortColumn;
  bool flatList;
  bool alternateRowColors;
  bool canReorderRows;
  bool canBeDragSource;
  bool headerRightClick;
  Drawing::Rectangle dragBox;
  int freezeCount;
  SortOrder currentSortOrder;
  List<int> columnTypes;

  Dictionary<String ^, TreeNodeAdv ^> ^ tagMap;

  TreeNodeAdv ^ hotNode;             // The node under the mouse.
  List<NodeOverlay ^> ^ overlayInfo; // Overlay images and their bounds for special actions.

  MformsTree() {
    model = gcnew SortableTreeModel();
    Model = model;
    currentSortColumn = -1;
    canSortColumn = false;
    freezeCount = 0;
    currentSortOrder = SortOrder::None;
    tagMap = nullptr;
    canBeDragSource = false;
    canReorderRows = false;
    dragBox = Rectangle::Empty;
    headerRightClick = false;

    hotNode = nullptr;
    overlayInfo = gcnew List<NodeOverlay ^>();
  }

  //------------------------------------------------------------------------------------------------

  ~MformsTree() {
  }

  //------------------------------------------------------------------------------------------------

  void UseTagMap() {
    tagMap = gcnew Dictionary<String ^, TreeNodeAdv ^>();
  }

  //------------------------------------------------------------------------------------------------

  void CleanUp(bool clearNodes) {
    HideEditor();

    for (int i = 0; i < model->Root->Nodes->Count; ++i)
      dynamic_cast<MySQL::Forms::TreeViewNode ^>(model->Root->Nodes[i])->DestroyDataRecursive();

    if (clearNodes)
      model->Root->Nodes->Clear();
  }

  //------------------------------------------------------------------------------------------------

  int AddColumn(mforms::TreeColumnType type, String ^ name, int initial_width, bool editable) {
    BindableControl ^ nodeControl;
    NodeIcon ^ icon = nullptr;
    switch (type) {
      case mforms::CheckColumnType:
        nodeControl = gcnew NodeCheckBox();
        break;

      case mforms::TriCheckColumnType: {
        NodeCheckBox ^ box = gcnew NodeCheckBox();
        box->ThreeState = true;
        nodeControl = box;
        break;
      }

      case mforms::IntegerColumnType:
      case mforms::LongIntegerColumnType:
      case mforms::NumberWithUnitColumnType:
      case mforms::FloatColumnType: {
        NodeTextBox ^ box =
          gcnew NodeTextBox(); // Does an integer column work differently than a normal text column (editor-wise)?
        box->EditEnabled = editable;
        box->TextAlign = HorizontalAlignment::Right;
        box->Trimming = StringTrimming::EllipsisCharacter;

        // This enables rendering with TextRenderer instead of Graphics.DrawString.
        // The latter doesn't draw ellipses.
        box->UseCompatibleTextRendering = true;
        nodeControl = box;
        break;
      }

      case mforms::IconColumnType: {
        icon = gcnew NodeIcon();

        NodeTextBox ^ box = gcnew AttributedNodeText();
        box->EditEnabled = editable;
        box->Trimming = StringTrimming::EllipsisCharacter;
        nodeControl = box;
        break;
      }

      case mforms::StringLTColumnType: {
        NodeTextBox ^ box = gcnew AttributedNodeText();
        box->EditEnabled = editable;
        box->Trimming = StringTrimming::EllipsisPath;
        nodeControl = box;
        break;
      }

      default: // mforms::StringColumnType
      {
        NodeTextBox ^ box = gcnew AttributedNodeText();
        box->EditEnabled = editable;
        box->Trimming = StringTrimming::EllipsisCharacter;
        nodeControl = box;
      }
    };

    TreeColumn ^ column = gcnew TreeColumn(name, (initial_width < 0) ? 50 : initial_width);
    column->WidthChanged += gcnew System::EventHandler(this, &MformsTree::OnColumnResized);
    Columns->Add(column);
    columnTypes.Add(type);

    if (!flatList && Columns->Count == 1) {
      TriangleNodeControl ^ control = gcnew TriangleNodeControl();
      control->ParentColumn = column;
      NodeControls->Add(control);
    }

    if (icon != nullptr) {
      icon->VirtualMode = true; // Means: get value for that column via event.
      icon->ValueNeeded += gcnew System::EventHandler<NodeControlValueEventArgs ^>(this, &MformsTree::TreeValueNeeded);
      icon->LeftMargin = 3;
      icon->ParentColumn = column;

      NodeControls->Add(icon);
    }

    nodeControl->VirtualMode = true;
    nodeControl->ValueNeeded +=
      gcnew System::EventHandler<NodeControlValueEventArgs ^>(this, &MformsTree::TreeValueNeeded);
    nodeControl->ValuePushed +=
      gcnew System::EventHandler<NodeControlValueEventArgs ^>(this, &MformsTree::TreeValuePushed);
    nodeControl->LeftMargin = 3;
    nodeControl->ParentColumn = column;
    NodeControls->Add(nodeControl);

    return column->Index;
  }

  //------------------------------------------------------------------------------------------------

  void EndColumns() {
    Model = model; // Trigger refresh.
  }

  //------------------------------------------------------------------------------------------------

  mforms::TreeNodeRef NodeFromTag(String ^ tag) {
    if (tagMap != nullptr) {
      TreeNodeAdv ^ node;
      if (tagMap->TryGetValue(tag, node)) {
        TreeViewWrapper *wrapper = TreeViewWrapper::GetWrapper<TreeViewWrapper>(this);
        return mforms::TreeNodeRef(new TreeNodeWrapper(wrapper, node));
      }
      return mforms::TreeNodeRef();
    }
    throw std::logic_error("Tree node <-> tag mapping requires tree creation option TreeIndexOnTag");
  }

  //--------------------------------------------------------------------------------------------------

  void UpdateTagMap(TreeNodeAdv ^ node, String ^ tag) {
    if (tagMap != nullptr) {
      if (node == nullptr)
        tagMap->Remove(tag);
      else
        tagMap[tag] = node;
    }
  }

  //--------------------------------------------------------------------------------------------------

  void AllowSorting(bool flag) {
    canSortColumn = flag;
    if (currentSortColumn > -1)
      Columns[currentSortColumn]->SortOrder = currentSortOrder;
  }

  //--------------------------------------------------------------------------------------------------

  void UpdateSorting(int column) {
    if ((freezeCount == 0) && column == currentSortColumn)
      model->Resort();
  }

  //--------------------------------------------------------------------------------------------------

  void FreezeRefresh(bool flag) {
    if (flag)
      ++freezeCount;
    else if (freezeCount == 0)
      logError("TreeView: attempt to thaw an unfrozen tree");
    else
      --freezeCount;

    if (freezeCount == 1)
      ControlUtilities::SuspendDrawing(this);
    else if (freezeCount == 0) {
      ControlUtilities::ResumeDrawing(this);
      model->Resort();
    }
  }

  //--------------------------------------------------------------------------------------------------

  virtual void OnSelectionChanged() override {
    __super ::OnSelectionChanged();

    if (!SuspendSelectionEvent) {
      mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
      backend->changed();
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnExpanding(TreeNodeAdv ^ node) override {
    __super ::OnExpanding(node);

    mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
    if (backend != NULL) {
      TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
      backend->expand_toggle(mforms::TreeNodeRef(new TreeNodeWrapper(wrapper, node)), true);
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnCollapsed(TreeNodeAdv ^ node) override {
    __super ::OnCollapsed(node);

    mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
    if (backend != NULL) {
      TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
      backend->expand_toggle(mforms::TreeNodeRef(new TreeNodeWrapper(wrapper, node)), false);
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnNodeMouseDoubleClick(TreeNodeAdvMouseEventArgs ^ args) override {
    __super ::OnNodeMouseDoubleClick(args);

    if (args->Control != nullptr) {
      mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
      TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
      int column = args->Control->ParentColumn->Index;
      backend->node_activated(mforms::TreeNodeRef(new TreeNodeWrapper(wrapper, args->Node)), column);
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnColumnClicked(TreeColumn ^ column, ::MouseButtons button) override {
    __super ::OnColumnClicked(column, button);

    // Stores the clicked column...
    mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
    backend->header_clicked(column->Index);

    switch (button) {
      case ::MouseButtons::Left:
        if (canSortColumn) {
          // Note: TreeViewAdv^ uses sort indicators wrongly. They are used as if they were arrows pointing
          //       to the greater value. However common usage is to view them as normal triangles with the
          //       tip showing the smallest value and the base side the greatest (Windows explorer and many
          //       other controls use it that way). Hence I switch the order here for the columns
          //       (though not for the comparer).
          if (column->Index != currentSortColumn) {
            currentSortColumn = column->Index;

            // Initial sort order is always ascending.
            column->SortOrder = SortOrder::Descending;
            currentSortOrder = SortOrder::Ascending;
          } else {
            if (column->SortOrder == SortOrder::Ascending)
              column->SortOrder = SortOrder::Descending;
            else
              column->SortOrder = SortOrder::Ascending;

            // Revert the sort order for our comparer, read above why.
            currentSortOrder =
              (column->SortOrder == SortOrder::Ascending) ? SortOrder::Descending : SortOrder::Ascending;
          }

          model->Comparer = gcnew ColumnComparer(currentSortColumn, currentSortOrder,
                                                 (mforms::TreeColumnType)columnTypes[currentSortColumn]);
        }
        break;

      case ::MouseButtons::Right:
        // Turns ON flag indicating the right click occurred on the table header
        headerRightClick = true;
        break;
    }
  }

  //------------------------------------------------------------------------------------------------

  void OnColumnResized(Object ^ sender, System::EventArgs ^ args) {
    TreeColumn ^ column = (TreeColumn ^)sender;
    mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
    if (backend)
      backend->column_resized(column->Index);
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnBeforeNodeDrawing(TreeNodeAdv ^ node, DrawContext context) override {
    __super ::OnBeforeNodeDrawing(node, context);

    // Alternating color only for odd rows.
    if (alternateRowColors && ((node->Row % 2) != 0)) {
      Graphics ^ graphics = context.Graphics;
      System::Drawing::Rectangle bounds = context.Bounds;
      System::Drawing::Color color = System::Drawing::Color::FromArgb(237, 243, 253);
      Brush ^ brush = gcnew SolidBrush(color);
      graphics->FillRectangle(brush, bounds);
      delete brush;
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnAfterNodeDrawing(TreeNodeAdv ^ node, DrawContext context) override {
    __super ::OnAfterNodeDrawing(node, context);

    // Draw overlay icon(s).
    if (node == hotNode && overlayInfo->Count > 0) {
      Graphics ^ graphics = context.Graphics;

      ColorMatrix ^ matrix = gcnew ColorMatrix();
      ImageAttributes ^ attributes = gcnew ImageAttributes();
      for each(NodeOverlay ^ overlay in overlayInfo) {
          matrix->Matrix33 = overlay->isHot ? 1 : 0.5f;
          attributes->SetColorMatrix(matrix);

          Drawing::Rectangle bounds = overlay->drawBounds;
          bounds.X += context.Bounds.Left;
          bounds.Y += context.Bounds.Top;
          graphics->DrawImage(overlay->image, bounds, 0, 0, overlay->image->Size.Width, overlay->image->Size.Width,
                              GraphicsUnit::Pixel, attributes);
        }
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnMouseDown(MouseEventArgs ^ args) override {
    // First check for clicks on the overlay.
    if (args->Button == ::MouseButtons::Left && overlayInfo->Count > 0) {
      mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
      TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();

      for (int i = 0; i < overlayInfo->Count; ++i) {
        NodeOverlay ^ overlay = overlayInfo[i];
        if (overlay->bounds.Contains(args->Location)) {
          backend->overlay_icon_for_node_clicked(mforms::TreeNodeRef(new TreeNodeWrapper(wrapper, hotNode)), i);
          return;
        }
      }
    }

    __super ::OnMouseDown(args);

    switch (args->Button) {
      case ::MouseButtons::Left:
        if (canBeDragSource) {
          System::Drawing::Size dragSize = SystemInformation::DragSize;
          dragBox =
            Drawing::Rectangle(Point(args->X - (dragSize.Width / 2), args->Y - (dragSize.Height / 2)), dragSize);
        }
        break;

      case ::MouseButtons::Right: {
        // Update the associated standard context menu.
        mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
        mforms::ContextMenu *context_menu = nullptr;

        // If the click occurred on the table header and there's aheader context menu
        // Uses it, if not uses the regulat context menu.
        if (headerRightClick && backend->get_header_menu())
          context_menu = backend->get_header_menu();
        else
          context_menu = backend->get_context_menu();

        if (context_menu) {
          ToolStrip ^ menu = MenuBarWrapper::GetManagedObject<ToolStrip>(context_menu);
          if (menu != ContextMenuStrip) {
            ContextMenuStrip = (System::Windows::Forms::ContextMenuStrip ^)menu;
            if (Conversions::UseWin8Drawing())
              ContextMenuStrip->Renderer = gcnew Win8MenuStripRenderer();
            else
              ContextMenuStrip->Renderer = gcnew TransparentMenuStripRenderer();

            context_menu->will_show();
          }
        } else
          ContextMenuStrip = nullptr;

        headerRightClick = false;
        break;
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnMouseMove(MouseEventArgs ^ args) override {
    if (dragBox != Rectangle::Empty && !dragBox.Contains(args->Location)) {
      // We come here only if canBeDragSource is true. So we start a drag operation in any case.
      dragBox = Rectangle::Empty;

      mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);

      mforms::DragDetails details;
      details.location = base::Point(args->X, args->Y);

      void *data = NULL;
      std::string format;

      // First see if there is custom data in the tree backend.
      if (backend->get_drag_data(details, &data, format)) {
        mforms::DragOperation operation = backend->do_drag_drop(details, data, format);
        backend->drag_finished(operation);
      } else {
        // if there's no custom data then collect all selected node's text and use that for dragging.
        // In addition, if row reordering is allowed, add an own data format for that too.
        if (SelectedNodes->Count > 0) {
          // Set all selected nodes as strings too, so we can drag their names to text editors etc.
          String ^ text = "";
          for each(TreeNodeAdv ^ nodeAdv in SelectedNodes) {
              TreeViewNode ^ node = (TreeViewNode ^)nodeAdv->Tag;

              if (text->Length > 0)
                text += ", ";
              text += node->FullCaption;
            }

          System::Windows::Forms::DataObject ^ dataObject =
            gcnew System::Windows::Forms::DataObject(gcnew MySQL::Utilities::DataObject());
          WBIDataObjectExtensions::SetDataEx(dataObject, DataFormats::UnicodeText, text);

          // Store the backend pointer in the data object, so we can distinguish between internal and
          // external drag sources.
          WBIDataObjectExtensions::SetDataEx(dataObject, DRAG_SOURCE_FORMAT_NAME, gcnew IntPtr(backend));

          DragDropEffects effects = DragDropEffects::Copy;
          if (canReorderRows)
            effects = static_cast<DragDropEffects>(effects | DragDropEffects::Move);

          DoDragDrop(dataObject, effects);
          delete dataObject;
        }
      }
    } else {
      // Any overlay icon to show?
      mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
      NodeControlInfo ^ info = GetNodeControlInfoAt(Point(args->X, args->Y));
      if (hotNode != info->Node) {
        if (hotNode != nullptr) {
          overlayInfo->Clear();
          Drawing::Rectangle bounds = GetRealNodeBounds(hotNode);
          Invalidate(Drawing::Rectangle(0, bounds.Top, Width, bounds.Height));
        }
        hotNode = info->Node;

        if (hotNode != nullptr) {
          TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
          std::vector<std::string> overlay_icons =
            backend->overlay_icons_for_node(mforms::TreeNodeRef(new TreeNodeWrapper(wrapper, info->Node)));

          if (!overlay_icons.empty()) {
            // info->Bounds is unreliable (e.g. can be empty).
            Drawing::Rectangle scrolledBounds = GetRealNodeBounds(hotNode);
            Drawing::Rectangle drawBounds = GetNodeBounds(hotNode);
            int total_width = 0;
            for (size_t i = 0; i < overlay_icons.size(); ++i) {
              NodeOverlay ^ overlay = gcnew NodeOverlay();
              overlay->isHot = false;
              String ^ name = CppStringToNativeRaw(overlay_icons[i]);
              if (!File::Exists(name))
                continue;

              overlay->image = Drawing::Image::FromFile(name);
              if (overlay->image == nullptr)
                continue;

              // Compute bounding rectangles of the image for drawing and hit tests.
              // The left offset is computed separately, after we have all images loaded (and know their widths).
              Drawing::Point position =
                Drawing::Point(0, scrolledBounds.Top + (scrolledBounds.Height - overlay->image->Size.Height) / 2);

              // The draw coordinates are relative to the node. We add offset during paint.
              Drawing::Point drawPosition = Drawing::Point(0, (drawBounds.Height - overlay->image->Size.Height) / 2);
              overlay->bounds = Drawing::Rectangle(position, overlay->image->Size);
              overlay->drawBounds = Drawing::Rectangle(drawPosition, overlay->image->Size);
              overlayInfo->Add(overlay);

              total_width += overlay->bounds.Size.Width;
            }

            int offset = DisplayRectangle.Size.Width - total_width;
            for each(NodeOverlay ^ overlay in overlayInfo) {
                overlay->bounds.X = offset;
                overlay->drawBounds.X = offset;
                offset += overlay->bounds.Width;
              }

            Invalidate(Drawing::Rectangle(0, scrolledBounds.Top, Width, scrolledBounds.Height));
          }
        } else {
          hotNode = nullptr;
          overlayInfo->Clear();
        }
      } else {
        // No change to a new node, but maybe a different overlay.
        if (overlayInfo->Count > 0) {
          for each(NodeOverlay ^ overlay in overlayInfo) {
              bool willBeHighlighted = overlay->bounds.Contains(args->Location);
              if (overlay->isHot != willBeHighlighted) {
                overlay->isHot = willBeHighlighted;
                Invalidate(overlay->bounds);
              }
            }
        }
      }
      Update();

      __super ::OnMouseMove(args);
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnMouseLeave(EventArgs ^ args) override {
    if (hotNode != nullptr) {
      hotNode = nullptr;
      overlayInfo->Clear();
      Invalidate();
    }
    __super ::OnMouseLeave(args);
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnMouseUp(MouseEventArgs ^ args) override {
    if (args->Button == ::MouseButtons::Left)
      dragBox = Rectangle::Empty;

    __super ::OnMouseUp(args);
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnDragOver(DragEventArgs ^ args) override {
    __super ::OnDragOver(args); // Draws the drop mark.

    if (canReorderRows) {
      // Only accept row move events from this tree instance.
      mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
      if (ViewWrapper::source_view_from_data(args->Data) == backend)
        args->Effect = DragDropEffects::Move;
      else
        args->Effect = DragDropEffects::None;
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnDragDrop(DragEventArgs ^ args) override {
    __super ::OnDragDrop(args);

    if (canReorderRows) {
      mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
      if (ViewWrapper::source_view_from_data(args->Data) == backend) {
        // Make a copy of the selection. We are going to modify the tree structure.
        Collections::Generic::List<TreeViewNode ^> ^ selection = gcnew Collections::Generic::List<TreeViewNode ^>();
        for each(TreeNodeAdv ^ node in SelectedNodes) selection->Add((TreeViewNode ^)node->Tag);

        Point p = PointToClient(Point(args->X, args->Y));
        NodeControlInfo ^ info = GetNodeControlInfoAt(p);
        if (info->Node == nullptr) {
          // No target node means either insert before the first one or after the last.
          for each(TreeViewNode ^ node in selection) {
              node->Parent->Nodes->Remove(node);

              if (p.Y < ColumnHeaderHeight)
                model->Root->Nodes->Insert(0, node);
              else
                model->Root->Nodes->Add(node);
            }
        } else {
          NodePosition position = DropPosition.Position;
          TreeViewNode ^ targetNode = (TreeViewNode ^)info->Node->Tag;

          for each(TreeViewNode ^ node in selection) {
              if (node != targetNode) // Don't move a node to the place it is currently.
              {
                node->Parent->Nodes->Remove(node);
                int index = targetNode->Index + ((position == NodePosition::After) ? 1 : 0);
                targetNode->Parent->Nodes->Insert(index, node);
              }
            }
        }
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Returns the text for the given node control (passed in as sender, representing the column) and for
   * the given node (passed in the event arguments).
   */
  void TreeValueNeeded(System::Object ^ sender, NodeControlValueEventArgs ^ args) {
    BindableControl ^ control = (BindableControl ^)sender;
    TreeNodeAdv ^ treeNode = args->Node;
    TreeViewNode ^ ourNode = dynamic_cast<TreeViewNode ^>(treeNode->Tag);

    // The passed in TreeNodeAdv can be the hidden root node, so better check it.
    if (ourNode != nullptr) {
      String ^ value = ourNode->Caption[control->ParentColumn->Index];
      if (control->GetType() == NodeCheckBox::typeid) {
        if ((value == "Checked") || (value == "1"))
          args->Value = CheckState::Checked;
        else if ((value == "Unchecked") || (value == "0"))
          args->Value = CheckState::Unchecked;
        else
          args->Value = CheckState::Indeterminate;
      } else if (control->GetType() == NodeIcon::typeid)
        args->Value = ourNode->Icon[control->ParentColumn->Index];
      else
        args->Value = value;
    }
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Sets new text for the given node control (passed in as sender, representing the column) and for
   * the given node (passed in the event arguments).
   */
  void TreeValuePushed(System::Object ^ sender, NodeControlValueEventArgs ^ args) {
    BindableControl ^ control = (BindableControl ^)sender;

    // First check if the backend allows editing that value.
    std::string new_value = NativeToCppString(args->Value->ToString());
    if (args->Value->GetType() == Boolean::typeid)
      new_value = (bool)args->Value ? "1" : "0";
    else if (args->Value->GetType() == CheckState::typeid) {
      CheckState state = (CheckState)args->Value;
      new_value = (state == CheckState::Checked) ? "1" : ((state == CheckState::Unchecked) ? "0" : "-1");
    }

    mforms::TreeView *backend = TreeViewWrapper::GetBackend<mforms::TreeView>(this);
    TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
    if (backend->cell_edited(mforms::TreeNodeRef(new TreeNodeWrapper(wrapper, args->Node)),
                             control->ParentColumn->Index, new_value)) {
      // Backend says ok, so update the model.
      TreeViewNode ^ ourNode = dynamic_cast<TreeViewNode ^>(args->Node->Tag);
      if (ourNode != nullptr)
        ourNode->Caption[control->ParentColumn->Index] = args->Value->ToString();
    }
  }

  //--------------------------------------------------------------------------------------------------
};

//----------------- TreeViewWrapper ------------------------------------------------------------

TreeViewWrapper::TreeViewWrapper(mforms::TreeView *backend) : ViewWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

TreeViewWrapper::~TreeViewWrapper() {
  MformsTree ^ control = TreeViewWrapper::GetManagedObject<MformsTree>();
  control->CleanUp(false);
};

//--------------------------------------------------------------------------------------------------

bool TreeViewWrapper::create(mforms::TreeView *backend, mforms::TreeOptions options) {
  TreeViewWrapper *wrapper = new TreeViewWrapper(backend);

  MformsTree ^ tree = TreeViewWrapper::Create<MformsTree>(backend, wrapper);
  if ((options & mforms::TreeIndexOnTag) != 0)
    tree->UseTagMap();

  if ((options & mforms::TreeCanBeDragSource) != 0 || (options & mforms::TreeAllowReorderRows) != 0) {
    tree->canBeDragSource = true;
    if ((options & mforms::TreeAllowReorderRows) != 0) {
      tree->AllowDrop = true;
      tree->canReorderRows = true;
    }
  }

  tree->FullRowSelect = true; // Same as in Explorer.
  tree->AsyncExpanding = false;
  tree->LoadOnDemand = false;
  tree->UseColumns = options & mforms::TreeNoColumns ? false : true;
  tree->ShowHeader = tree->UseColumns && (options & mforms::TreeNoHeader) == 0;

  if (options & mforms::TreeNoBorder) // Default is 3d border.
  {
    tree->BorderStyle = BorderStyle::None;

    // Add some padding or the content will directly start at the boundaries.
    tree->Padding = Padding(2);
  } else
    tree->BorderStyle = BorderStyle::FixedSingle; // Make the border a single line if there's any.

  tree->ShowLines = false;
  tree->ShowPlusMinus = false;

  tree->flatList = (options & mforms::TreeFlatList) != 0;
  tree->alternateRowColors = (options & mforms::TreeAltRowColors) != 0;

  return true;
}

//--------------------------------------------------------------------------------------------------

int TreeViewWrapper::add_column(mforms::TreeView *backend, mforms::TreeColumnType type, const std::string &name,
                                int initial_width, bool editable) {
  MformsTree ^ control = TreeViewWrapper::GetManagedObject<MformsTree>(backend);
  return control->AddColumn(type, CppStringToNative(name), initial_width, editable);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::end_columns(mforms::TreeView *backend) {
  MformsTree ^ control = TreeViewWrapper::GetManagedObject<MformsTree>(backend);
  control->EndColumns();
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::clear(mforms::TreeView *backend) {
  MformsTree ^ control = TreeViewWrapper::GetManagedObject<MformsTree>(backend);
  control->CleanUp(true);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_row_height(mforms::TreeView *backend, int h) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  wrapper->set_row_height(h);
}

//--------------------------------------------------------------------------------------------------

std::list<mforms::TreeNodeRef> TreeViewWrapper::get_selection(mforms::TreeView *backend) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  return wrapper->get_selection();
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeViewWrapper::get_selected_node(mforms::TreeView *backend) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  TreeViewAdv ^ tree = wrapper->GetManagedObject<TreeViewAdv>();
  TreeNodeAdv ^ node = tree->SelectedNode;
  if (node == nullptr || node->Tag == nullptr)
    return mforms::TreeNodeRef();
  return mforms::TreeNodeRef(new TreeNodeWrapper(wrapper, node));
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeViewWrapper::root_node(mforms::TreeView *backend) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  return wrapper->root_node();
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_selected(mforms::TreeView *backend, mforms::TreeNodeRef node, bool flag) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  wrapper->set_selected(node, flag);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::scrollToNode(mforms::TreeView *backend, mforms::TreeNodeRef node) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  wrapper->scrollToNode(node);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::clear_selection(mforms::TreeView *backend) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  wrapper->clear_selection();
}

//--------------------------------------------------------------------------------------------------

mforms::TreeSelectionMode TreeViewWrapper::get_selection_mode(mforms::TreeView *backend) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  return wrapper->get_selection_mode();
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_selection_mode(mforms::TreeView *backend, mforms::TreeSelectionMode mode) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  wrapper->set_selection_mode(mode);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_allow_sorting(mforms::TreeView *backend, bool flag) {
  MformsTree ^ control = TreeViewWrapper::GetManagedObject<MformsTree>(backend);
  control->AllowSorting(flag);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::freeze_refresh(mforms::TreeView *backend, bool flag) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  wrapper->freeze_refresh(flag);
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeViewWrapper::node_at_row(mforms::TreeView *backend, int row) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  return wrapper->node_at_row(row);
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeViewWrapper::node_at_position(mforms::TreeView *backend, base::Point position) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  return wrapper->node_at_position(position);
}

//--------------------------------------------------------------------------------------------------

int TreeViewWrapper::row_for_node(mforms::TreeView *backend, mforms::TreeNodeRef node) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  return wrapper->row_for_node(node);
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeViewWrapper::node_with_tag(mforms::TreeView *backend, const std::string &tag) {
  MformsTree ^ control = TreeViewWrapper::GetManagedObject<MformsTree>(backend);
  return control->NodeFromTag(CppStringToNative(tag));
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_column_title(mforms::TreeView *backend, int column, const std::string &title) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  wrapper->set_column_title(column, title);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_column_visible(mforms::TreeView *backend, int column, bool flag) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  wrapper->set_column_visible(column, flag);
}

//--------------------------------------------------------------------------------------------------

bool TreeViewWrapper::get_column_visible(mforms::TreeView *backend, int column) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  return wrapper->is_column_visible(column);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_column_width(mforms::TreeView *backend, int column, int width) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  wrapper->set_column_width(column, width);
}

//--------------------------------------------------------------------------------------------------

int TreeViewWrapper::get_column_width(mforms::TreeView *backend, int column) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  return wrapper->get_column_width(column);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::BeginUpdate(mforms::TreeView *backend) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  wrapper->BeginUpdate();
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::EndUpdate(mforms::TreeView *backend) {
  TreeViewWrapper *wrapper = backend->get_data<TreeViewWrapper>();
  wrapper->EndUpdate();
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_row_height(int h) {
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  tree->RowHeight = h;
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::clear_selection() {
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  tree->ClearSelection();
}

//--------------------------------------------------------------------------------------------------

std::list<mforms::TreeNodeRef> TreeViewWrapper::get_selection() {
  std::list<mforms::TreeNodeRef> selection;
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  ReadOnlyCollection<TreeNodeAdv ^> ^ sel = tree->SelectedNodes;
  if (sel != nullptr) {
    for (int i = 0; i < sel->Count; i++)
      selection.push_back(mforms::TreeNodeRef(new TreeNodeWrapper(this, sel[i])));
  }
  return selection;
}

//--------------------------------------------------------------------------------------------------

mforms::TreeSelectionMode TreeViewWrapper::get_selection_mode() {
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  switch (tree->SelectionMode) {
    case Aga::Controls::Tree::TreeSelectionMode::Single:
      return mforms::TreeSelectSingle;

    case Aga::Controls::Tree::TreeSelectionMode::Multi:
      return mforms::TreeSelectMultiple;

    default:
      return mforms::TreeSelectSingle;
  }
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_selection_mode(mforms::TreeSelectionMode mode) {
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  switch (mode) {
    case mforms::TreeSelectSingle:
      tree->SelectionMode = Aga::Controls::Tree::TreeSelectionMode::Single;
      break;
    case mforms::TreeSelectMultiple:
      tree->SelectionMode = Aga::Controls::Tree::TreeSelectionMode::Multi;
      break;
  }
}
//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeViewWrapper::root_node() {
  return mforms::TreeNodeRef(new TreeNodeWrapper(this));
}

//--------------------------------------------------------------------------------------------------

static mforms::TreeNodeRef find_node_at_row(mforms::TreeNodeRef node, int &row_counter, int row) {
  mforms::TreeNodeRef res;
  for (int i = 0, c = node->count(); i < c; i++) {
    mforms::TreeNodeRef child = node->get_child(i);
    if (row_counter == row)
      return child;
    row_counter++;
    if (child->is_expanded()) {
      res = find_node_at_row(child, row_counter, row);
      if (res)
        return res;
    }
  }
  return res;
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeViewWrapper::node_at_row(int row) {
  int i = 0;
  mforms::TreeNodeRef node = find_node_at_row(root_node(), i, row);
  return node;
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeViewWrapper::node_at_position(base::Point position) {
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  TreeNodeAdv ^ node = tree->GetNodeAt(::Point((int)position.x, (int)position.y));
  if (node == nullptr)
    return mforms::TreeNodeRef();

  return mforms::TreeNodeRef(new TreeNodeWrapper(this, node));
}

//--------------------------------------------------------------------------------------------------

static int count_rows_in_node(mforms::TreeNodeRef node) {
  if (node->is_expanded()) {
    int count = node->count();
    for (int i = 0, c = node->count(); i < c; i++) {
      mforms::TreeNodeRef child(node->get_child(i));
      if (child)
        count += count_rows_in_node(child);
    }
    return count;
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

int TreeViewWrapper::row_for_node(mforms::TreeNodeRef node) {
  TreeNodeWrapper *impl = dynamic_cast<TreeNodeWrapper *>(node.ptr());
  if (impl) {
    mforms::TreeNodeRef parent = node->get_parent();
    int node_index = impl->node_index();
    int row = node_index;

    if (parent) {
      for (int i = 0; i < node_index; i++)
        row += count_rows_in_node(parent->get_child(i));
      if (parent != root_node())
        row += row_for_node(parent) + 1; // One more for the parent node.
    } else {
      for (int i = 0; i < node_index; i++)
        row += count_rows_in_node(root_node()->get_child(i));
    }
    return row;
  }
  return -1;
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_column_title(int column, const std::string &title) {
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  tree->Columns[column]->Header = CppStringToNative(title);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_column_visible(int column, bool flag) {
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  tree->Columns[column]->IsVisible = flag;
}

//--------------------------------------------------------------------------------------------------

bool TreeViewWrapper::is_column_visible(int column) {
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  return tree->Columns[column]->IsVisible;
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_column_width(int column, int width) {
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  tree->Columns[column]->Width = width;
}

//--------------------------------------------------------------------------------------------------

int TreeViewWrapper::get_column_width(int column) {
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  return tree->Columns[column]->Width;
}

//--------------------------------------------------------------------------------------------------

mforms::DropPosition TreeViewWrapper::get_drop_position() {
  TreeViewAdv ^ tree = GetManagedObject<TreeViewAdv>();
  switch (tree->DropPosition.Position) {
    case NodePosition::Inside:
      return mforms::DropPositionOn;
    case NodePosition::Before:
      return mforms::DropPositionTop;
    case NodePosition::After:
      return mforms::DropPositionBottom;

    default:
      return mforms::DropPositionUnknown;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Adds, removes or changes a node <-> tag mapping (if mapping is enabled).
 */
void TreeViewWrapper::process_mapping(TreeNodeAdv ^ node, const std::string &tag) {
  MformsTree ^ tree = GetManagedObject<MformsTree>();
  tree->UpdateTagMap(node, CppStringToNative(tag));
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by a treeview node if new text was set.
 */
void TreeViewWrapper::node_value_set(int column) {
  MformsTree ^ tree = GetManagedObject<MformsTree>();
  tree->UpdateSorting(column);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::set_selected(mforms::TreeNodeRef node, bool flag) {
  TreeNodeWrapper *wrapper = dynamic_cast<TreeNodeWrapper *>(node.ptr());
  wrapper->set_selected(flag);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::scrollToNode(mforms::TreeNodeRef node) {
  TreeNodeWrapper *wrapper = dynamic_cast<TreeNodeWrapper *>(node.ptr());
  wrapper->scrollToNode();
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::freeze_refresh(bool flag) {
  MformsTree ^ tree = GetManagedObject<MformsTree>();
  tree->FreezeRefresh(flag);
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::BeginUpdate() {
  MformsTree ^ tree = GetManagedObject<MformsTree>();
  tree->BeginUpdate();
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::EndUpdate() {
  MformsTree ^ tree = GetManagedObject<MformsTree>();
  tree->EndUpdate();
}

//--------------------------------------------------------------------------------------------------

void TreeViewWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_treeview_impl.create = &TreeViewWrapper::create;
  f->_treeview_impl.add_column = &TreeViewWrapper::add_column;
  f->_treeview_impl.end_columns = &TreeViewWrapper::end_columns;
  f->_treeview_impl.clear = &TreeViewWrapper::clear;
  f->_treeview_impl.clear_selection = &TreeViewWrapper::clear_selection;
  f->_treeview_impl.get_selection = &TreeViewWrapper::get_selection;
  f->_treeview_impl.get_selected_node = &TreeViewWrapper::get_selected_node;
  f->_treeview_impl.set_selected = &TreeViewWrapper::set_selected;
  f->_treeview_impl.scrollToNode = &TreeViewWrapper::scrollToNode;
  f->_treeview_impl.set_allow_sorting = &TreeViewWrapper::set_allow_sorting;
  f->_treeview_impl.set_row_height = &TreeViewWrapper::set_row_height;
  f->_treeview_impl.freeze_refresh = &TreeViewWrapper::freeze_refresh;
  f->_treeview_impl.root_node = &TreeViewWrapper::root_node;
  f->_treeview_impl.row_for_node = &TreeViewWrapper::row_for_node;
  f->_treeview_impl.node_at_row = &TreeViewWrapper::node_at_row;
  f->_treeview_impl.node_at_position = &TreeViewWrapper::node_at_position;
  f->_treeview_impl.set_selection_mode = &TreeViewWrapper::set_selection_mode;
  f->_treeview_impl.get_selection_mode = &TreeViewWrapper::get_selection_mode;
  f->_treeview_impl.node_with_tag = &TreeViewWrapper::node_with_tag;

  f->_treeview_impl.set_column_title = &TreeViewWrapper::set_column_title;

  f->_treeview_impl.set_column_visible = &TreeViewWrapper::set_column_visible;
  f->_treeview_impl.get_column_visible = &TreeViewWrapper::get_column_visible;

  f->_treeview_impl.set_column_width = &TreeViewWrapper::set_column_width;
  f->_treeview_impl.get_column_width = &TreeViewWrapper::get_column_width;

  f->_treeview_impl.BeginUpdate = &TreeViewWrapper::BeginUpdate;
  f->_treeview_impl.EndUpdate = &TreeViewWrapper::EndUpdate;
}

//--------------------------------------------------------------------------------------------------
