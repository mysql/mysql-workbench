/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MFTreeView.h"
#import "NSString_extras.h"
#import "NSColor_extras.h"
#import "MFView.h"
#import "MFMForms.h"
#include "base/log.h"
#include "base/string_utilities.h"

#include <cstdlib>

#import "MTextImageCell.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_COCOA);

static NSString *RowReorderPasteboardDatatype = @"com.mysql.workbench.row-reorder";

class TreeNodeImpl;

//----------------------------------------------------------------------------------------------------------------------

@interface MFTreeNodeImpl : NSObject {
@public
  MFTreeNodeImpl *mParent;
  MFTreeViewImpl *mTree; // someone else
  NSMutableDictionary *mData;
}

- (instancetype)initWithOwner: (MFTreeViewImpl *)owner NS_DESIGNATED_INITIALIZER;
@property(readonly, weak) MFTreeViewImpl *treeView;
@property(readonly) mforms::TreeNodeRef nodeRef;
- (void)setObject: (id)anObject forKey: (id)aKey;
- (id)objectForKey: (id)key;
- (void)removeObjectForKey: (id)key;
- (NSMutableArray *)createChildrenWithCapacity: (int)count;
@property(readonly, strong) NSMutableArray *children;
@property(readonly, strong) MFTreeNodeImpl *parent;
- (void)removeFromParent;

@property(readonly, copy) NSString *text;

@end

//----------------------------------------------------------------------------------------------------------------------

inline TreeNodeImpl *from_ref(mforms::TreeNodeRef node);

@interface TreeNodeDataRef : NSObject {
  mforms::TreeNodeData *_data;
}
- (instancetype)initWithCPPPointer: (mforms::TreeNodeData *)data;
@property(readonly) mforms::TreeNodeData *CPPPointer;
@end

//----------------------------------------------------------------------------------------------------------------------

@implementation TreeNodeDataRef

- (instancetype)initWithCPPPointer: (mforms::TreeNodeData *)data {
  self = [self init];
  if (self) {
    self->_data = data;
    if (self->_data)
      self->_data->retain();
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::TreeNodeData *)CPPPointer {
  return _data;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  if (_data)
    _data->release();
}

@end

//----------------------------------------------------------------------------------------------------------------------

class TreeNodeImpl : public mforms::TreeNode {
  MFTreeNodeImpl *_self;
  int _refcount;

public:
  TreeNodeImpl(MFTreeNodeImpl *owner) : _self(owner), _refcount(0) {
  }

  virtual ~TreeNodeImpl() {
  }

  virtual void release() {
    _refcount--;
    if (_refcount == 0)
      delete this;
  }

  virtual void retain() {
    _refcount++;
  }

  void invalidate() {
    _self = nil;
  }

  virtual bool is_valid() const {
    return _self != nil;
  }

  virtual bool equals(const TreeNode &other) {
    const TreeNodeImpl *oth = dynamic_cast<const TreeNodeImpl *>(&other);
    if (oth)
      return oth->_self == _self;
    return false;
  }

  MFTreeNodeImpl *self() {
    return _self;
  }

  virtual int count() const {
    NSMutableArray *children = _self.children;
    return (int)children.count;
  }

  virtual mforms::TreeNodeRef insert_child(int index) {
    if (is_valid()) {
      MFTreeNodeImpl *child = [[MFTreeNodeImpl alloc] initWithOwner: _self.treeView];

      mforms::TreeNodeRef node(child.nodeRef);

      {
        TreeNodeImpl *nodei = from_ref(node);
        NSMutableArray *children = _self.children;
        if (!children)
          children = [_self createChildrenWithCapacity:0];

        child->mParent = _self;

        if (index < 0 || index >= (int)children.count)
          [children addObject: nodei->self()];
        else
          [children insertObject: nodei->self() atIndex: index];

        if (!_self.treeView.frozen && (!get_parent() || is_expanded() || children.count == 1)) {
          [_self.treeView setNeedsReload];
        }
      }

      // when node deallocs from stack, ref is returned to 1 and the parent owns that only ref
      // unless the caller keeps a ref to our return value
      return node;
    }
    return mforms::TreeNodeRef();
  }

  virtual int get_child_index(mforms::TreeNodeRef node) const {
    id child = from_ref(node)->self();
    return (int)[_self.children indexOfObject: child];
  }

  virtual mforms::TreeNodeRef previous_sibling() const {
    NSUInteger index = [_self.parent.children indexOfObject: _self];
    if (index == 0 || index == NSNotFound)
      return mforms::TreeNodeRef();

    MFTreeNodeImpl *child = (_self.parent.children)[index - 1];
    return child.nodeRef;
  }

  virtual mforms::TreeNodeRef next_sibling() const {
    NSUInteger index = [_self.parent.children indexOfObject: _self];
    if (index == _self.parent.children.count - 1 || index == NSNotFound)
      return mforms::TreeNodeRef();

    MFTreeNodeImpl *child = (_self.parent.children)[index + 1];
    return child.nodeRef;
  }

  virtual std::vector<mforms::TreeNodeRef> add_node_collection(const mforms::TreeNodeCollectionSkeleton &nodes,
                                                               int position = -1) {
    std::vector<mforms::TreeNodeRef> result;

    if (is_valid()) {
      id columnKey = [_self.treeView keyForColumn:0];

      // Creates an array to hold each equal child for all the parents
      NSMutableArray *added_nodes = [NSMutableArray arrayWithCapacity: (int)nodes.captions.size()];

      NSImage *image = get_icon(nodes.icon);

      for (std::vector<std::string>:: const_iterator v = nodes.captions.begin(); v != nodes.captions.end(); ++v) {
        MFTreeNodeImpl *child = [[MFTreeNodeImpl alloc] initWithOwner: _self.treeView];

        [child setObject: [NSString stringWithCPPString:*v] forKey: columnKey];

        if (image) {
          [child setObject: image forKey: [[_self.treeView keyForColumn:0] stringByAppendingString: @"icon"]];
        }

        [added_nodes addObject: child];
      }

      // Creates the substructure if any
      if (!nodes.children.empty())
        add_children_from_skeletons(added_nodes, nodes.children);

      NSMutableArray *children = _self.children;
      if (!children)
        children = [_self createChildrenWithCapacity: (int)nodes.captions.size()];

      NSEnumerator *child_enumerator = [added_nodes objectEnumerator];
      MFTreeNodeImpl *child;

      while (child = [child_enumerator nextObject]) {
        child->mParent = _self;

        if (position < 0 || position >= (int)children.count)
          [children addObject: child];
        else
          [children insertObject: child atIndex:position++];

        result.push_back(child.nodeRef);
      }

      if (!_self.treeView.frozen) {
        [_self.treeView setNeedsReload];
      }
    }

    return result;
  }

  void add_children_from_skeletons(NSMutableArray *parents, const std::vector<mforms::TreeNodeSkeleton> &children) {
    if (is_valid()) {
      // Creates the parent children lists
      NSEnumerator *parent_enumerator = [parents objectEnumerator];
      MFTreeNodeImpl *parent;
      while (parent = [parent_enumerator nextObject]) {
        NSMutableArray *children_array = parent.children;
        if (!children_array)
          children_array = [parent createChildrenWithCapacity: (int)children.size()];
      }

      id columnKey = [_self.treeView keyForColumn:0];

      // Now enters the process if creating each children at this level
      std::vector<mforms::TreeNodeSkeleton>:: const_iterator it, end = children.end();
      for (it = children.begin(); it != end; it++) {
        // Creates an array to hold each equal child for all the parents
        NSMutableArray *added_nodes = [NSMutableArray arrayWithCapacity:parents.count];

        // Gets the data to be set on the current child
        NSImage *image = get_icon((*it).icon);
        NSString *caption = [NSString stringWithCPPString: (*it).caption];
        NSString *tag = [NSString stringWithCPPString: (*it).tag];

        // Setups the child for all the parents (same name, icon)
        for (unsigned int index = 0; index < parents.count; index++) {
          MFTreeNodeImpl *child = [[MFTreeNodeImpl alloc] initWithOwner: _self.treeView];
          [child setObject: caption forKey: columnKey];

          [child setObject:tag forKey: @"tag"];

          if (image) {
            [child setObject: image forKey: [columnKey stringByAppendingString: @"icon"]];
          }

          [added_nodes addObject: child];
        }

        // If the children have descendence, then calls this method to create the substructure
        // on them
        if (!(*it).children.empty())
          add_children_from_skeletons(added_nodes, (*it).children);

        // Now assigns each child to its corresponding parent
        NSEnumerator *child_enumerator = [added_nodes objectEnumerator];
        parent_enumerator = [parents objectEnumerator];
        MFTreeNodeImpl *child;
        while (parent = [parent_enumerator nextObject]) {
          child = [child_enumerator nextObject];
          child->mParent = parent;
          [parent.children addObject: child];
        }
      }
    }
  }

  virtual void remove_from_parent() {
    if (is_valid()) {
      NSString *tag = [_self objectForKey: @"tag"];
      if (tag)
        [_self->mTree setNode: nil forTag:tag.UTF8String];

      if (_self.parent)
        [_self removeFromParent];
      else
        throw std::logic_error("Cannot remove root node");
    }
  }

  virtual void remove_children() {
    // XXX
    if (is_valid()) {
      for (id ch in _self.children) {
        NSString *tag = ch[@"tag"];
        if (tag)
          [_self->mTree setNode: nil forTag:tag.UTF8String];
      }
      [_self.children removeAllObjects];
    }
  }

  void move_node(mforms::TreeNodeRef node, bool before) {
    if (is_valid()) {
      TreeNodeImpl *other = from_ref(node);
      if (_self->mTree != other->_self->mTree)
        return;

      [_self removeFromParent];

      MFTreeNodeImpl *parent = other->_self->mParent;
      NSUInteger index = [parent.children indexOfObject:other->_self];
      if (!before)
        ++index;

      _self->mParent = parent;
      [parent.children insertObject: _self atIndex: index];

      if (!_self->mTree.frozen) {
        [_self->mTree setNeedsReload];
      }
    }
  }

  virtual mforms::TreeNodeRef get_child(int index) const {
    if (is_valid()) {
      MFTreeNodeImpl *child = _self.children[index];
      if (child)
        return child.nodeRef;
    }
    return mforms::TreeNodeRef();
  }

  virtual mforms::TreeNodeRef get_parent() const {
    if (is_valid()) {
      if (_self.parent)
        return _self.parent.nodeRef;
    }
    return mforms::TreeNodeRef();
  }

  virtual void expand() {
    if (!is_expanded()) {
      _self.treeView.backend->expand_toggle(_self.nodeRef, true);
      mforms::TreeNodeRef parent(get_parent());

      if (parent)
        parent->expand();

      [_self.treeView.outlineView performSelector: @selector(expandItem:)
                                       withObject: _self
                                       afterDelay:0.0
                                          inModes: @[ NSModalPanelRunLoopMode, NSDefaultRunLoopMode ]];
    }
  }

  /**
   * If there are already subnodes then we can obviously expand.
   * But if there are none (yet) we have to ask the treeview (which in turn might query
   * its data model).
   */
  virtual bool can_expand() {
    if (count() > 0)
      return true;

    return _self.treeView.backend->can_expand(_self.nodeRef);
  }

  virtual void collapse() {
    [_self.treeView.outlineView collapseItem: _self];
  }

  virtual bool is_expanded() {
    return [_self.treeView.outlineView isItemExpanded: _self];
  }

  virtual void set_tag(const std::string &tag) {
    NSString *tstr = [_self objectForKey: @"tag"];

    if (tstr)
      [_self->mTree setNode: nil forTag:tstr.UTF8String];
    [_self->mTree setNode: _self forTag:tag];
    [_self setObject: [NSString stringWithCPPString:tag] forKey: @"tag"];
  }

  virtual std::string get_tag() const {
    const char *s = [[_self objectForKey: @"tag"] UTF8String];
    return s ? s : "";
  }

  virtual void set_data(mforms::TreeNodeData *data) {
    [_self setObject: [[TreeNodeDataRef alloc] initWithCPPPointer: data] forKey: @"data"];
  }

  virtual mforms::TreeNodeData *get_data() const {
    return [[_self objectForKey: @"data"] CPPPointer];
  }

  NSImage *get_icon(const std::string &icon_path) {
    NSImage *image = nil;
    if (icon_path == "folder") {
      static NSImage *folderIcon = [[[NSWorkspace sharedWorkspace] iconForFile: @"/usr"] copy];
      image = folderIcon;
    } else
      image = [_self.treeView iconForFile: [NSString stringWithCPPString: icon_path]];

    if (image) {
      NSSize size = image.size;
      float rowHeight = _self.treeView.outlineView.rowHeight;
      if (size.height > rowHeight) {
        rowHeight -= 2;
        size.width *= rowHeight / size.height;
        size.height = rowHeight;
        image.size = size;
      }
    }
    return image;
  }

  virtual int level() const {
    // 0 for the root node, 1 for top level nodes etc.
    // NSOutlineView returns 0 for the top level nodes, however.
    return (int)[_self.treeView.outlineView levelForItem: _self] + 1;
  }

  virtual void set_icon_path(int column, const std::string &icon) {
    NSImage *image = get_icon(icon);

    if (image)
      [_self setObject: image forKey: [[_self.treeView keyForColumn: column] stringByAppendingString: @"icon"]];
    else
      [_self removeObjectForKey: [[_self.treeView keyForColumn: column] stringByAppendingString: @"icon"]];
  }

  virtual void set_attributes(int column, const mforms::TreeNodeTextAttributes &attrs) {
    if (attrs.bold || attrs.italic || attrs.color.is_valid()) {
      NSString *attrstr = [NSString stringWithFormat: @"%s%s%s", attrs.bold ? "b" : "", attrs.italic ? "i" : "",
                           attrs.color.is_valid() ? attrs.color.to_html().c_str() : ""];
      [_self setObject:attrstr forKey: [[_self.treeView keyForColumn: column] stringByAppendingString: @"attrs"]];
    } else
      [_self removeObjectForKey: [[_self.treeView keyForColumn: column] stringByAppendingString: @"attrs"]];
  }

  virtual void set_string(int column, const std::string &value) {
    [_self setObject: [NSString stringWithCPPString:value] forKey: [_self.treeView keyForColumn: column]];
  }

  virtual void set_int(int column, int value) {
    id key = [_self.treeView keyForColumn: column];

    [_self setObject: @(value) forKey:key];
  }

  virtual void set_long(int column, std:: int64_t value) {
    [_self setObject: @(value) forKey: [_self.treeView keyForColumn: column]];
  }

  virtual void set_float(int column, double value) {
    id key = [_self.treeView keyForColumn: column];

    [_self setObject: @(value) forKey:key];
  }

  virtual void set_bool(int column, bool value) {
    [_self setObject: @(value) forKey: [_self.treeView keyForColumn: column]];
  }

  virtual std::string get_string(int column) const {
    id o = [_self objectForKey: [_self.treeView keyForColumn: column]];
    if (o != nil) {
      if ([o isKindOfClass:NSString.class])
        return ((NSString *)o).UTF8String;
      if ([o isKindOfClass:NSNumber.class])
        return ((NSNumber *)o).stringValue.UTF8String;
    }
    return "";
  }

  virtual int get_int(int column) const {
    NSNumber *n = [_self objectForKey: [_self.treeView keyForColumn: column]];
    if (n)
      return n.intValue;
    return 0;
  }

  virtual std:: int64_t get_long(int column) const {
    NSNumber *n = [_self objectForKey: [_self.treeView keyForColumn: column]];
    if (n)
      return n.longLongValue;
    return 0;
  }

  virtual double get_float(int column) const {
    NSNumber *n = [_self objectForKey: [_self.treeView keyForColumn: column]];
    if (n)
      return n.doubleValue;
    return 0.0;
  }

  virtual bool get_bool(int column) const {
    NSNumber *n = [_self objectForKey: [_self.treeView keyForColumn: column]];
    if (n)
      return n.boolValue;
    return false;
  }
};

//----------------------------------------------------------------------------------------------------------------------

inline TreeNodeImpl *from_ref(mforms::TreeNodeRef node) {
  return dynamic_cast<TreeNodeImpl *>(node.ptr());
}

//----------------------------------------------------------------------------------------------------------------------

@implementation MFTreeNodeImpl

- (instancetype)initWithOwner: (MFTreeViewImpl *)owner {
  self = [super init];
  if (self) {
    mData = [NSMutableDictionary new];
    mTree = owner;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)init {
  return [self initWithOwner: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (MFTreeViewImpl *)treeView {
  return mTree;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setObject: (id)anObject forKey: (id)aKey {
  mData[aKey] = anObject;

  [mTree.outlineView setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)removeObjectForKey: (id)aKey {
  [mData removeObjectForKey:aKey];
}

//----------------------------------------------------------------------------------------------------------------------

- (id)objectForKey: (id)key {
  return mData[key];
}

//----------------------------------------------------------------------------------------------------------------------

- (id)valueForKey: (id)key // for KVC
{
  return mData[key];
}

//----------------------------------------------------------------------------------------------------------------------

- (id)objectForKeyedSubscript: (id)key {
  return mData[key];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setObject: (id)anObject forKeyedSubscript: (id)aKey {
  mData[aKey] = anObject;

  [mTree.outlineView setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSMutableArray *)createChildrenWithCapacity: (int)count {
  NSMutableArray *children = count > 0 ? [NSMutableArray arrayWithCapacity: count] : [NSMutableArray array];
  mData[@"children"] = children;
  return children;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSMutableArray *)children {
  return mData[@"children"];
}

//----------------------------------------------------------------------------------------------------------------------

- (MFTreeNodeImpl *)parent {
  return mParent;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)removeFromParent {
  NSMutableArray *children = mParent.children;
  if (children) {
    // if the node being removed is selected, unselect it, so that the selection isn't passed to a different node
    NSIndexSet *rows = self.treeView.outlineView.selectedRowIndexes;
    if (rows && rows.count == 1) {
      if ([rows containsIndex: [self.treeView.outlineView rowForItem: self]])
        [self.treeView.outlineView deselectAll: nil];
    }

    [children removeObject: self];
    mParent = nil;

    if (!self.treeView.frozen)
      [self.treeView.outlineView reloadData];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::TreeNodeRef)nodeRef {
  if (self == nil)
    return mforms::TreeNodeRef();
  return mforms::TreeNodeRef(new TreeNodeImpl(self));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns a concatenated string of all column values, separated by tab.
 */
- (NSString *)text {
  NSString *result = @"";
  NSArray *columns = mTree.outlineView.tableColumns;
  for (NSTableColumn *column in columns) {
    id data = mData[column.identifier];
    if ([data isKindOfClass:NSString.class] && [data length] > 0) {
      if (result.length > 0)
        result = [result stringByAppendingString: @"\t"];
      result = [result stringByAppendingString: data];
    }
  }
  return result;
}

@end

//----------------------------------------------------------------------------------------------------------------------

static NSImage *ascendingSortIndicator = nil;
static NSImage *descendingSortIndicator = nil;

//----------------------------------------------------------------------------------------------------------------------

@interface TreeNodeHeaderView : NSTableHeaderView {
}
@end

//----------------------------------------------------------------------------------------------------------------------

@interface TreeViewOutlineView : NSOutlineView {
@public
  mforms::TreeView *mOwner;
  NSTrackingArea *mTrackingArea;

  NSInteger mOverlayedRow;
  NSMutableArray *mOverlayIcons;

  int mOverOverlay;
  int mClickingOverlay;
  BOOL mMouseInside;
}
@end

//----------------------------------------------------------------------------------------------------------------------

@implementation TreeViewOutlineView

- (instancetype)initWithFrame: (NSRect)frame owner: (mforms::TreeView *)ownerTreeView useSourceListStyle: (BOOL)sourceList {
  self = [super initWithFrame: frame];
  if (self != nil) {
    mOverOverlay = -1;
    mClickingOverlay = -1;
    mOwner = ownerTreeView;
    self.headerView = [[TreeNodeHeaderView alloc] init];

    if (sourceList)
      self.selectionHighlightStyle = NSTableViewSelectionHighlightStyleSourceList;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::Object *)mformsObject {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)rightMouseDown: (NSEvent *)theEvent {
  NSInteger row = [self rowAtPoint: [self convertPoint:theEvent.locationInWindow fromView: nil]];
  if (row >= 0) {
    if (![self isRowSelected:row])
      [self selectRowIndexes: [NSIndexSet indexSetWithIndex:row] byExtendingSelection: NO];
  }
  [super rightMouseDown:theEvent];
}

//----------------------------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING_NO_RIGHT_BUTTON(self) // Add handling for mouse events.
STANDARD_FOCUS_HANDLING(self)                 // Notify backend when getting first responder status.

#define OVERLAY_ICON_RIGHT_PADDING 8
#define OVERLAY_ICON_SPACING 2

//----------------------------------------------------------------------------------------------------------------------

- (NSMenu *)menuForEvent: (NSEvent *)event {
  if (mOwner == nil)
    return nil;

  mforms::ContextMenu *menu = mOwner->get_context_menu();
  if (menu)
    return menu->get_data();
  return nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleMouseMove: (NSEvent *)event owner: (mforms::View *)owner {
  NSPoint p = [self convertPoint:event.locationInWindow fromView: nil];
  NSInteger row = [self rowAtPoint:p];

  if (NSPointInRect(p, self.visibleRect)) {
    if (mClickingOverlay < 0) {
      mOverlayIcons = nil;
      mOverOverlay = -1;
      mOverlayedRow = -1;
      [self setNeedsDisplay: YES];
    }
    if (row >= 0 && NSPointInRect(p, [self rectOfRow:row])) {
      mforms::TreeNodeRef node(mOwner->node_at_row((int)row));
      if (node) {
        std::vector<std::string> icons = mOwner->overlay_icons_for_node(node);
        if (!icons.empty()) {
          NSRect iconRect = [self rectOfRow:row];

          iconRect.origin.x = NSMaxX(self.visibleRect) - OVERLAY_ICON_RIGHT_PADDING;
          iconRect.size.width = 0;

          mOverlayIcons = [[NSMutableArray alloc] initWithCapacity: icons.size()];
          int i = 0;

          // Iterating the icons reversely to simplify hit computation.
          for (std::vector<std::string>::reverse_iterator icon = icons.rbegin(); icon != icons.rend(); ++icon, ++i) {
            NSImage *img = icon->empty() ? nil : [(MFTreeViewImpl *)[self delegate]
                                                  iconForFile: [NSString stringWithCPPString:*icon]];
            if (img)
              [mOverlayIcons insertObject: img atIndex:0];
            else
              [mOverlayIcons insertObject:NSNull.null atIndex:0];

            iconRect.origin.x -= img.size.width + OVERLAY_ICON_SPACING;
            iconRect.size.width = img.size.width;

            if (NSPointInRect(p, iconRect) && mOverOverlay < 0)
              mOverOverlay = int(icons.size() - i - 1);
          }
          [self setNeedsDisplay: YES];
        }
        mOverlayedRow = row;
      }
    }
  }
  return [super handleMouseMove:event owner:owner];
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleMouseEntered: (NSEvent *)event owner: (mforms::View *)owner {
  mMouseInside = YES;
  return [super handleMouseEntered:event owner:owner];
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleMouseExited: (NSEvent *)event owner: (mforms::View *)owner {
  mMouseInside = NO;
  mOverlayIcons = nil;
  mOverOverlay = -1;
  mClickingOverlay = -1;
  [self setNeedsDisplay: YES];

  return [super handleMouseExited:event owner:owner];
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleMouseDown: (NSEvent *)event owner: (mforms::View *)owner {
  if (mOverOverlay >= 0) {
    mClickingOverlay = mOverOverlay;
    return true;
  }
  return [super handleMouseDown:event owner:owner];
}

//----------------------------------------------------------------------------------------------------------------------

- (bool)handleMouseUp: (NSEvent *)event owner: (mforms::View *)owner {
  if (mOverOverlay >= 0 && mOverOverlay == mClickingOverlay) {
    mforms::TreeNodeRef node(mOwner->node_at_row((int)mOverlayedRow));
    if (node)
      mOwner->overlay_icon_for_node_clicked(node, mClickingOverlay);
    else
      logDebug("Error getting node for row %li, shouldn't be NULL\n", mOverlayedRow);

    mClickingOverlay = -1;
    return true;
  }
  mClickingOverlay = -1;
  return [super handleMouseUp:event owner:owner];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRow: (NSInteger)rowIndex clipRect: (NSRect)clipRect {
  [super drawRow:rowIndex clipRect: clipRect];
  if (mOverlayIcons && rowIndex == mOverlayedRow) {
    NSRect rowRect = [self rectOfRow:rowIndex];
    int i = 0;

    CGFloat x = NSMaxX(self.visibleRect) - OVERLAY_ICON_RIGHT_PADDING;
    for (id icon in mOverlayIcons) {
      if ([icon isKindOfClass:NSImage.class]) {
        NSSize size = ((NSImage *)icon).size;
        x -= size.width + OVERLAY_ICON_SPACING;
      }
    }

    for (id icon in mOverlayIcons) {
      if ([icon isKindOfClass: [NSImage class]]) {
        NSSize size = ((NSImage *)icon).size;
        [(NSImage *)icon
         drawInRect:NSMakeRect(floorf(x), floorf(NSMinY(rowRect) + (NSHeight(rowRect) - size.height) / 2),
                               size.width, size.height)
         fromRect:NSZeroRect
         operation:NSCompositingOperationSourceOver
         fraction:mOverOverlay == i ? 1.0 : 0.4f
         respectFlipped: YES
         hints: nil];
        x += size.width + OVERLAY_ICON_SPACING;
      }
      i++;
    }
  }
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation TreeNodeHeaderView

- (NSMenu *)menuForEvent: (NSEvent *)event {
  TreeViewOutlineView *outline = (TreeViewOutlineView *)self.tableView;

  if (outline) {
    mforms::ContextMenu *menu = outline->mOwner->get_header_menu();
    int column =
    (int)[outline.headerView columnAtPoint: [outline.headerView convertPoint:event.locationInWindow fromView: nil]];
    outline->mOwner->header_clicked(column);

    if (menu)
      return menu->get_data();
  }
  return nil;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFTreeViewImpl

- (instancetype)initWithObject: (mforms::TreeView *)aTreeView useSourceListStyle: (BOOL)sourceList {
  if (!ascendingSortIndicator) {
    ascendingSortIndicator = [NSImage imageNamed: @"NSAscendingSortIndicator"];
    descendingSortIndicator = [NSImage imageNamed: @"NSDescendingSortIndicator"];
  }

  self = [super initWithFrame:NSMakeRect(0, 0, 40, 40)];
  if (self != nil) {
    mOwner = aTreeView;
    mOwner->set_data(self);

    mColumnKeys = [[NSMutableArray alloc] init];
    mIconCache = [[NSMutableDictionary alloc] init];

    [self setHasHorizontalScroller: YES];
    [self setHasVerticalScroller: YES];
    [self setAutohidesScrollers: YES];

    mAttributedFonts = [[NSMutableDictionary alloc] init];
    mAttributedFonts[@""] = [NSFont systemFontOfSize: [NSFont systemFontSize]];
    NSRect rect;
    rect.origin = NSMakePoint(0, 0);
    rect.size = [NSScrollView contentSizeForFrameSize: self.frame.size
                              horizontalScrollerClass: [NSScroller class]
                                verticalScrollerClass: [NSScroller class]
                                           borderType: NSBezelBorder
                                          controlSize: NSControlSizeRegular
                                        scrollerStyle: NSScrollerStyleOverlay];

    mOutline = [[TreeViewOutlineView alloc] initWithFrame:rect owner: mOwner useSourceListStyle: sourceList];

    self.documentView = mOutline;
    mOutline.columnAutoresizingStyle = NSTableViewLastColumnOnlyAutoresizingStyle;

    [mOutline setAllowsEmptySelection: YES];

    mRootNode = [[MFTreeNodeImpl alloc] initWithOwner: self];
    [mOutline setDataSource: self];
    [mOutline setDelegate: self];
    mOutline.doubleAction = @selector(rowDoubleClicked:);
    mOutline.target = self;

    mSortColumn = -1;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)destroy {
  mOwner = nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)enableIndexing {
  mTagIndexEnabled = YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [NSObject cancelPreviousPerformRequestsWithTarget: self];

  // Setting nil is necessary since there are still datasouce and delegate messages comming in after dealloc.
  // Might become unnecessary if we don't auto release mOutline.
  [mOutline setDataSource: nil];
  [mOutline setDelegate: nil];

  mTagMap.clear();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)reloadTreeData {
  if (mPendingReload) {
    mPendingReload = NO;
    [mOutline reloadData];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setNeedsReload {
  if (!mPendingReload)
    [self performSelectorOnMainThread: @selector(reloadTreeData) withObject: nil waitUntilDone: NO];
  mPendingReload = YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isPendingReload {
  return mPendingReload;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSOutlineView *)outlineView {
  return mOutline;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setEnabled: (BOOL)flag {
  mOutline.enabled = flag;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isEnabled {
  return mOutline.enabled;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSInteger)addColumnWithTitle: (NSString *)title
                           type: (mforms::TreeColumnType)type
                       editable: (BOOL)editable
                          width: (int)width {
  NSUInteger idx = mOutline.tableColumns.count;
  NSString *columnKey = [NSString stringWithFormat: @"%lu", idx];
  NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier: columnKey];

  [mColumnKeys addObject: columnKey];

  [mOutline addTableColumn: column];
  [column.headerCell setTitle:title];
  if (!mColumnsAutoResize)
    column.resizingMask = NSTableColumnUserResizingMask;
  
  switch (type) {
    case mforms::CheckColumnType: {
      NSButtonCell *cell = [[NSButtonCell alloc] init];
      column.dataCell = cell;
      cell.title = @"";
      [cell setButtonType: NSButtonTypeSwitch];
      break;
    }
    case mforms::TriCheckColumnType: {
      NSButtonCell *cell = [[NSButtonCell alloc] init];
      column.dataCell = cell;
      cell.title = @"";
      [cell setButtonType: NSButtonTypeSwitch];
      [cell setAllowsMixedState: YES];
      break;
    }
    case mforms::StringColumnType:
    case mforms::StringLTColumnType:
      break;
    case mforms::NumberWithUnitColumnType:
      [column.dataCell setAlignment: NSTextAlignmentRight];
      break;
    case mforms::IconColumnType: {
      MTextImageCell *cell = [[MTextImageCell alloc] init];
      column.dataCell = cell;
      break;
    }
    case mforms::FloatColumnType: {
      NSNumberFormatter *nf = [[NSNumberFormatter alloc] init];
      nf.numberStyle = (NSNumberFormatterStyle)kCFNumberFormatterDecimalStyle;
      [column.dataCell setAlignment: NSTextAlignmentRight];
      [column.dataCell setFormatter: nf];
      break;
    }
    case mforms::IntegerColumnType:
    case mforms::LongIntegerColumnType: {
      NSNumberFormatter *nf = [[NSNumberFormatter alloc] init];
      nf.numberStyle = NSNumberFormatterNoStyle;
      [column.dataCell setAlignment: NSTextAlignmentRight];
      [column.dataCell setFormatter: nf];
      break;
    }
  }

  [column.dataCell setEditable:editable];
  [column.dataCell setTruncatesLastVisibleLine: YES];
  if (type == mforms::StringLTColumnType)
    [column.dataCell setLineBreakMode:NSLineBreakByTruncatingHead];
  if (idx == 0) // && !mFlatTable)
    mOutline.outlineTableColumn = column;
  if (width >= 0)
    column.width = width;
  if (mSmallFont)
    [column.dataCell setFont: [NSFont systemFontOfSize: [NSFont smallSystemFontSize]]];

  return idx;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString *)keyForColumn: (int)column {
  if (column >= (int)mColumnKeys.count)
    throw std:: invalid_argument(base::strfmt("invalid column %i in TreeView, last column is %s", column,
                                              mOutline.tableColumns.lastObject.headerCell.stringValue.UTF8String));

  return mColumnKeys[column];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSImage *)iconForFile: (NSString *)path {
  if (path && path.length > 0) {
    NSImage *image = nil;
    image = mIconCache[path];
    if (!image) {
      std::string full_path = g_file_test(path.UTF8String, G_FILE_TEST_EXISTS)
      ? path.UTF8String
      : mforms::App::get()->get_resource_path(path.UTF8String);
      image = [[NSImage alloc] initWithContentsOfFile:wrap_nsstring(full_path)];
      if (image && image.valid)
        mIconCache[path] = image;
      else
        image = nil;
    }
    if (!image)
      NSLog(@"Can't load icon %@", path);
    return image;
  }
  return nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)outlineView: (NSOutlineView *)outlineView didClickTableColumn: (NSTableColumn *)tableColumn {
  if (mSortColumnEnabled) {
    int column = (tableColumn.identifier).intValue;
    BOOL ascending;

    if ([outlineView indicatorImageInTableColumn:tableColumn] == ascendingSortIndicator) {
      ascending = NO;
      [outlineView setIndicatorImage:descendingSortIndicator inTableColumn:tableColumn];
    } else {
      ascending = YES;
      [outlineView setIndicatorImage:ascendingSortIndicator inTableColumn:tableColumn];
    }
    outlineView.highlightedTableColumn = tableColumn;
    if (mSortColumn >= 0 && mSortColumn < outlineView.numberOfColumns && mSortColumn != column)
      [outlineView setIndicatorImage: nil inTableColumn:outlineView.tableColumns[mSortColumn]];
    mSortColumn = column;

    NSSortDescriptor *sd;

    if ([tableColumn.dataCell alignment] == NSTextAlignmentRight) {
      if ([tableColumn.dataCell formatter])
        sd = [NSSortDescriptor sortDescriptorWithKey:tableColumn.identifier
                                           ascending:ascending
                                          comparator:^(id o1, id o2) {
                                            double d = [o1 doubleValue] - [o2 doubleValue];
                                            if (d > 0)
                                              return (NSComparisonResult)NSOrderedAscending;
                                            else if (d < 0)
                                              return (NSComparisonResult)NSOrderedDescending;
                                            else
                                              return (NSComparisonResult)NSOrderedSame;
                                          }];
      else
        sd = [NSSortDescriptor sortDescriptorWithKey:tableColumn.identifier
                                           ascending:ascending
                                          comparator:^(id o1, id o2) {
                                            double d = mforms::TreeView::parse_string_with_unit([o1 UTF8String]) -
                                            mforms::TreeView::parse_string_with_unit([o2 UTF8String]);
                                            if (d > 0)
                                              return (NSComparisonResult)NSOrderedAscending;
                                            else if (d < 0)
                                              return (NSComparisonResult)NSOrderedDescending;
                                            else
                                              return (NSComparisonResult)NSOrderedSame;
                                          }];
    } else
      sd = [NSSortDescriptor sortDescriptorWithKey:tableColumn.identifier ascending:ascending];
    outlineView.sortDescriptors = @[ sd ];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void sortChildrenOfNode(MFTreeNodeImpl *node, NSArray *sortDescriptors) {
  if (sortDescriptors && node) {
    NSMutableArray *array = node.children;
    if (array) {
      [array sortUsingDescriptors:sortDescriptors];
      for (MFTreeNodeImpl *object in array)
        sortChildrenOfNode(object, sortDescriptors);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setViewFlags: (ViewFlags)flags {
  mOutline.viewFlags = flags;
}

//----------------------------------------------------------------------------------------------------------------------

- (ViewFlags)viewFlags {
  return mOutline.viewFlags;
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::TreeView *)backend {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark - NSOutlineView delegate

- (void)outlineView: (NSOutlineView *)outlineView sortDescriptorsDidChange: (NSArray *)oldDescriptors {
  NSInteger selectedRow = outlineView.selectedRow;
  id selectedItem = [outlineView itemAtRow:selectedRow];

  sortChildrenOfNode(mRootNode, outlineView.sortDescriptors);

  [outlineView reloadData];

  if (selectedRow >= 0) {
    [outlineView selectRowIndexes: [NSIndexSet indexSetWithIndex: [outlineView rowForItem:selectedItem]]
             byExtendingSelection: NO];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (id)outlineView: (NSOutlineView *)outlineView child: (NSInteger)index ofItem: (id)item {
  if (!item)
    item = mRootNode;

  NSArray *children = [item children];
  if (children && index < (NSInteger)children.count)
    return [item children][index];
  return nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)outlineView: (NSOutlineView *)outlineView isItemExpandable: (id)item {
  if (mOwner == nil)
    return NO;

  if (item) {
    MFTreeNodeImpl *node = (MFTreeNodeImpl *)item;
    return node.nodeRef->can_expand();
  }
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSInteger)outlineView: (NSOutlineView *)outlineView numberOfChildrenOfItem: (id)item {
  NSInteger c;
  if (!item)
    c = mRootNode.children.count;
  else
    c = [item children].count;
  return c;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)outlineViewItemDidExpand: (NSNotification *)notification {
  if (mOwner == nil)
    return;

  // Tell the backend now would be a good time to add child nodes if not yet done.
  try {
    MFTreeNodeImpl *node = notification.userInfo[@"NSObject"];
    if (node)
      mOwner->expand_toggle(node.nodeRef, true);
  } catch (std::exception &exc) {
    logError("Exception in expand_toggle(true) handler\n");
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)outlineViewItemDidCollapse: (NSNotification *)notification {
  if (mOwner == nil)
    return;

  try {
    MFTreeNodeImpl *node = notification.userInfo[@"NSObject"];
    if (node)
      mOwner->expand_toggle(node.nodeRef, false);
  } catch (std::exception &exc) {
    logError("Exception in expand_toggle(false) handler\n");
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (id)outlineView: (NSOutlineView *)outlineView objectValueForTableColumn: (NSTableColumn *)tableColumn byItem: (id)item {
  return item[tableColumn.identifier];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)outlineView: (NSOutlineView *)outlineView
    willDisplayCell: (id)cell
     forTableColumn: (NSTableColumn *)tableColumn
               item: (id)item {
  if ([cell isKindOfClass: [MTextImageCell class]])
    [cell setImage: item[[tableColumn.identifier stringByAppendingString: @"icon"]]];

  BOOL canSetColor = [cell respondsToSelector: @selector(setTextColor:)];

  NSString *attributes = item[[tableColumn.identifier stringByAppendingString: @"attrs"]];
  if (attributes.length > 0) {
    NSString *fontKey = attributes;
    NSRange range = [attributes rangeOfString: @"#"];
    if (range.length > 0 && canSetColor) {
      fontKey = [fontKey substringToIndex:range.location];
      if (![cell isHighlighted]) {
        NSColor *color = [NSColor colorFromHexString: [attributes substringWithRange:NSMakeRange(range.location, 7)]];
        [cell setTextColor: color];
      } else
        [cell setTextColor:NSColor.whiteColor];
    }

    if (fontKey.length > 0) {
      NSFont *font = mAttributedFonts[fontKey];
      if (!font) {
        int traits = 0;
        if ([attributes rangeOfString: @"b"].length > 0)
          traits |= NSBoldFontMask;
        if ([attributes rangeOfString: @"i"].length > 0)
          traits |= NSItalicFontMask;

        font = [[NSFontManager sharedFontManager] convertFont: [[tableColumn.dataCell font] copy] toHaveTrait:traits];
        mAttributedFonts[fontKey] = font;
      }
      [cell setFont:font];
    } else
      [cell setFont:mAttributedFonts[@""]];
  } else {
    [cell setFont:mAttributedFonts[@""]];

    // Restore default colors. The outline doesn't seem to auto reset.
    if (canSetColor) {
      [cell setTextColor:NSColor.controlTextColor];
    }
  }

  // This is a work around to properly apply font + coloring to the cell's text.
  // We should instead work with attributed strings.
  [cell setStringValue: [cell stringValue]];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)outlineView: (NSOutlineView *)outlineView
     setObjectValue: (id)object
     forTableColumn: (NSTableColumn *)tableColumn
             byItem: (id)item {
  id value = [object respondsToSelector: @selector(stringValue)] ? [object stringValue] : object;

  if ([tableColumn.dataCell isKindOfClass: [NSButtonCell class]] && [tableColumn.dataCell allowsMixedState]) {
    // if new state is -1, force it to 1
    if ([value isEqualToString: @"-1"])
      value = @"1";
  }

  if (mOwner->cell_edited([item nodeRef], atoi(tableColumn.identifier.UTF8String), [value description].UTF8String))
    item[tableColumn.identifier] = value;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)outlineViewSelectionDidChange: (NSNotification *)notification {
  if (mOwner != nil && !self.frozen)
    mOwner->changed();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)rowDoubleClicked: (id)sender {
  NSInteger row = mOutline.clickedRow;
  if (row >= 0) {
    id item = [mOutline itemAtRow:mOutline.clickedRow];
    mOwner->node_activated([item nodeRef], (int)mOutline.clickedColumn);
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)outlineViewColumnDidResize: (NSNotification *)notification {
  if (mOwner == nil)
    return;

  mOwner->column_resized((int)mOutline.headerView.resizedColumn);
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark - Drag'n drop

/**
 * We need to redirect drop format registration from this (container) view to the actual drop target
 * (the wrapped outline).
 */
- (NSArray *)acceptableDropFormats {
  return mOutline.acceptableDropFormats;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setAcceptableDropFormats: (NSArray *)formats {
  mOutline.acceptableDropFormats = formats;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Called when the outline view starts dragging implicitly (not via our drag_data or drag_text methods in MFView).
 */
- (BOOL)outlineView: (NSOutlineView *)outlineView writeItems: (NSArray *)items toPasteboard: (NSPasteboard *)pboard {
  if (!mCanBeDragSource)
    return NO;

  self.lastDropPosition = mforms::DropPositionUnknown;

  // First write a special datatype for row reordering if enabled. This is the preferred type in that case.
  if (mCanReorderRows && items.count == 1) {
    NSNumber *number = @([outlineView rowForItem: items.lastObject]);
    NSData *data = [NSKeyedArchiver archivedDataWithRootObject: @[ number ] requiringSecureCoding: NO error: nil];
    [pboard declareTypes: @[ RowReorderPasteboardDatatype ] owner: nil];
    [pboard setData: data forType: RowReorderPasteboardDatatype];
  }

  mforms::DragDetails details;
  void *data = NULL;
  std::string format;
  mDraggedNodes = items; // Will be queried when the backend wants to have the current selection.
  if (mOwner->get_drag_data(details, &data, format)) {
    [pboard writeNativeData: data typeAsChar:format.c_str()];
    self.allowedDragOperations = details.allowedOperations;
  } else {
    self.allowedDragOperations = mforms::DragOperationCopy;
    [pboard addTypes: @[ NSPasteboardTypeString ] owner: self];

    // If the tree is enabled as drag source but no custom format is given then we implictely take this as sign
    // to allow dragging text (not only related to the schema tree). This is to ease cases where we just want
    // node captions to be dragged around.
    NSString *text = @"";
    for (MFTreeNodeImpl *node in items) {
      if (text.length > 0)
        text = [text stringByAppendingString: @", "];
      text = [text stringByAppendingString: node.text];
    }
    [pboard setString:text forType:NSPasteboardTypeString];
  }

  mDraggedNodes = nil; // Would probably better to reset that in draggingSessionEndedAt but
  // that is not available before 10.7.
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSDragOperation)outlineView: (NSOutlineView *)outlineView
                  validateDrop: (id<NSDraggingInfo>)info
                  proposedItem: (id)item
            proposedChildIndex: (NSInteger)index {
  if (item == nil) // nil is passed in sometimes even if a node is hit.
    return NSDragOperationNone;

  // There are only 2 possible drop positions in an outline view: on and above.
  // Except if the given index is beyond the item's last child.
  if (index == -1)
    self.lastDropPosition = mforms::DropPositionOn;
  else {
    if (index == (NSInteger)[item children].count)
      self.lastDropPosition = mforms::DropPositionBottom;
    else
      self.lastDropPosition = mforms::DropPositionTop;
  }

  NSDragOperation op = [self draggingUpdated: info];

  if (op != NSDragOperationNone) {
    [mOutline setDropItem: item dropChildIndex: index];
  } else if (mCanReorderRows) {
    NSPasteboard *pb = [info draggingPasteboard];
    NSData *data = [pb dataForType:RowReorderPasteboardDatatype];
    NSAssert((data != nil), @"Drag flavour was not found.");

    NSArray *indexes = [NSKeyedUnarchiver unarchivedObjectOfClass: [NSArray class] fromData: data error: nil];
    NSInteger oldIndex = [indexes.lastObject integerValue];

    // Right now only allow reordering flat lists, not trees.
    if (oldIndex != index && item == nil) {
      item = mRootNode;
      if (index < (int)[item children].count) {
        [outlineView setDropItem: nil dropChildIndex: index];
        op = NSDragOperationMove;
      }
    }
  }

  return op;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)outlineView: (NSOutlineView *)outlineView
         acceptDrop: (id<NSDraggingInfo>)info
               item: (id)item
         childIndex: (NSInteger)index {
  if ([self performDragOperation: info])
    return YES;

  if (mCanReorderRows) {
    NSPasteboard *pb = [info draggingPasteboard];
    NSData *data = [pb dataForType:RowReorderPasteboardDatatype];
    NSAssert((data != nil), @"Drag flavour was not found.");

    NSArray *indexes = [NSKeyedUnarchiver unarchivedObjectOfClass: [NSArray class] fromData: data error: nil];
    NSInteger oldIndex = [indexes.lastObject integerValue];

    if (index != oldIndex) {
      if (item == nil)
        item = mRootNode;
      NSMutableArray *list = item[@"children"];
      id draggedItem = list[oldIndex];

      [list removeObjectAtIndex:oldIndex];
      if (index < 0)
        [list addObject:draggedItem];
      else if (index < oldIndex)
        [list insertObject:draggedItem atIndex: index];
      else
        [list insertObject:draggedItem atIndex: index - 1];

      [outlineView reloadData];
    }
    return YES;
  }

  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setUseSmallFont: (BOOL)flag {
  mSmallFont = flag;
  mAttributedFonts[@""] = [NSFont systemFontOfSize: [NSFont smallSystemFontSize]];
  if (flag)
    mOutline.rowHeight = [NSFont smallSystemFontSize] + 3;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)frozen {
  return mFreezeCount > 0;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setBackgroundColor: (NSColor *)color {
  super.backgroundColor = color;
  mOutline.backgroundColor = color;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setNode: (MFTreeNodeImpl *)node forTag: (const std::string &)tag {
  if (!tag.empty() && mTagIndexEnabled) {
    if (!node)
      mTagMap.erase(tag);
    else
      mTagMap[tag] = node; // reference only
  }
}

//----------------------------------------------------------------------------------------------------------------------

static bool treeview_create(mforms::TreeView *self, mforms::TreeOptions options) {
  MFTreeViewImpl *tree = [[MFTreeViewImpl alloc] initWithObject: self
                                             useSourceListStyle: (options & mforms::TreeTranslucent) != 0];

  if ((options & mforms::TreeAllowReorderRows) != 0 || (options & mforms::TreeCanBeDragSource) != 0) {
    tree->mCanBeDragSource = YES;

    // For row-reordering register also as drag target.
    if (options & mforms::TreeAllowReorderRows) {
      tree->mCanReorderRows = YES;
      [tree->mOutline registerForDraggedTypes: @[ RowReorderPasteboardDatatype ]];
    }
    [tree->mOutline setDraggingSourceOperationMask: NSDragOperationCopy forLocal: NO];
    [tree->mOutline setDraggingSourceOperationMask: NSDragOperationMove | NSDragOperationCopy forLocal: YES];
  }

  if ((options & mforms::TreeColumnsAutoResize) != 0) {
    tree->mColumnsAutoResize = YES;
  } else {
    tree->mColumnsAutoResize = NO;
  }
  
  if (options & mforms::TreeNoHeader)
    [tree->mOutline setHeaderView: nil];

  int mask = 0;
  if (options & mforms::TreeShowColumnLines)
    mask |= NSTableViewSolidVerticalGridLineMask;
  if (options & mforms::TreeShowRowLines)
    mask |= NSTableViewSolidHorizontalGridLineMask;

  if (options & mforms::TreeNoBorder)
    tree.borderType = NSNoBorder;
  else
    tree.borderType = NSBezelBorder;

  if (options & mforms::TreeSizeSmall) {
    [tree setUseSmallFont: YES];
  }

  if (options & mforms::TreeSidebar) {
    /*
     // maintain the row height
     float rowHeight = tree->mOutline.rowHeight;
     [tree setUseSmallFont: YES];
     tree->mOutline.rowHeight = rowHeight;
     */
    tree->mOutline.focusRingType = NSFocusRingTypeNone;
  }

  if (options & mforms::TreeIndexOnTag)
    [tree enableIndexing];
  tree->mOutline.gridStyleMask = mask;

  if (options & mforms::TreeAltRowColors)
    [tree->mOutline setUsesAlternatingRowBackgroundColors: YES];

  tree->mFlatTable = (options & mforms::TreeFlatList) != 0;
  if (tree->mFlatTable)
    tree->mOutline.indentationPerLevel = 0;

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

static int treeview_add_column(mforms::TreeView *self, mforms::TreeColumnType type, const std::string &name, int width,
                               bool editable, bool attributed) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree) {
    return (int)[tree addColumnWithTitle:wrap_nsstring(name) type:type editable:editable width:width];
  }
  return -1;
}

//----------------------------------------------------------------------------------------------------------------------

static void treeview_end_columns(mforms::TreeView *self) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree) {
    [tree->mOutline sizeLastColumnToFit];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void treeview_clear(mforms::TreeView *self) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree) {
    [tree->mRootNode.children removeAllObjects];
    [tree->mOutline reloadData];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static mforms::TreeNodeRef treeview_root_node(mforms::TreeView *self) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree)
    return tree->mRootNode.nodeRef;
  return mforms::TreeNodeRef();
}

//----------------------------------------------------------------------------------------------------------------------

static mforms::TreeNodeRef treeview_get_selected(mforms::TreeView *self) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree) {
    NSArray *draggedNodes = tree->mDraggedNodes;
    if (draggedNodes.count > 0)
      return [draggedNodes[0] nodeRef];
    else {
      NSInteger row = tree->mOutline.selectedRow;
      if (row >= 0)
        return [[tree->mOutline itemAtRow:row] nodeRef];
    }
  }
  return mforms::TreeNodeRef();
}

//----------------------------------------------------------------------------------------------------------------------

static std::list<mforms::TreeNodeRef> treeview_get_selection(mforms::TreeView *self) {
  MFTreeViewImpl *tree = self->get_data();
  std::list<mforms::TreeNodeRef> selection;
  if (tree) {
    NSArray *draggedNodes = tree->mDraggedNodes;
    if (draggedNodes.count > 0) {
      for (MFTreeNodeImpl *item in draggedNodes)
        selection.push_back(item.nodeRef);
    } else {
      NSIndexSet *indexes = tree->mOutline.selectedRowIndexes;
      NSUInteger currentIndex = indexes.firstIndex;
      while (currentIndex != NSNotFound) {
        MFTreeNodeImpl *node = [tree->mOutline itemAtRow: currentIndex];
        if (node != nil)
          selection.push_back(node.nodeRef);
        currentIndex = [indexes indexGreaterThanIndex: currentIndex];
      }
    }
  }
  return selection;
}

//----------------------------------------------------------------------------------------------------------------------

static int treeview_row_for_node(mforms::TreeView *self, mforms::TreeNodeRef node);

//----------------------------------------------------------------------------------------------------------------------

static void treeview_set_selected(mforms::TreeView *self, mforms::TreeNodeRef node, bool flag) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree && node) {
    if (flag) {
      [tree reloadTreeData];
      [tree->mOutline selectRowIndexes: [NSIndexSet indexSetWithIndex:treeview_row_for_node(self, node)]
                  byExtendingSelection: YES];
    } else
      [tree->mOutline deselectRow:treeview_row_for_node(self, node)];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void treeviewScrollToNode(mforms::TreeView *backend, mforms::TreeNodeRef node) {
  MFTreeViewImpl *tree = backend->get_data();
  if (tree != NULL && node.is_valid()) {
    [tree->mOutline scrollRowToVisible:treeview_row_for_node(backend, node)];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void treeview_set_row_height(mforms::TreeView *self, int height) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree)
    tree->mOutline.rowHeight = height;
}

//----------------------------------------------------------------------------------------------------------------------

static void treeview_clear_selection(mforms::TreeView *self) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree)
    [tree->mOutline deselectAll: nil];
}

//----------------------------------------------------------------------------------------------------------------------

static void treeview_allow_sorting(mforms::TreeView *self, bool flag) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree) {
    tree->mSortColumnEnabled = flag;
    if (!flag) {
      for (NSTableColumn *column in tree->mOutline.tableColumns)
        [tree->mOutline setIndicatorImage: nil inTableColumn: column];
      [tree->mOutline setHighlightedTableColumn: nil];
      tree->mSortColumn = -1;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void treeview_freeze_refresh(mforms::TreeView *self, bool flag) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree) {
    if (flag)
      tree->mFreezeCount++;
    else {
      tree->mFreezeCount--;
      if (tree->mFreezeCount == 0) {
        // remember and restore row selection
        NSInteger row = tree->mOutline.selectedRow;
        id item = row >= 0 ? [tree->mOutline itemAtRow:row] : nil;

        sortChildrenOfNode(tree->mRootNode, tree->mOutline.sortDescriptors);
        [tree->mOutline reloadData];

        if (item) {
          row = [tree->mOutline rowForItem: item];
          if (row >= 0)
            [tree->mOutline selectRowIndexes: [NSIndexSet indexSetWithIndex:row] byExtendingSelection: NO];
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void treeview_set_selection_mode(mforms::TreeView *self, mforms::TreeSelectionMode mode) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree) {
    switch (mode) {
      case mforms::TreeSelectSingle:
        [tree->mOutline setAllowsEmptySelection: YES];
        [tree->mOutline setAllowsMultipleSelection: NO];
        break;
      case mforms::TreeSelectMultiple:
        [tree->mOutline setAllowsEmptySelection: YES];
        [tree->mOutline setAllowsMultipleSelection: YES];
        break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static mforms::TreeSelectionMode treeview_get_selection_mode(mforms::TreeView *self) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree) {
    mforms::TreeSelectionMode mode;
    if (tree->mOutline.allowsMultipleSelection)
      return mforms::TreeSelectMultiple;
    else
      return mforms::TreeSelectSingle;
    return mode;
  }
  return mforms::TreeSelectSingle;
}

//----------------------------------------------------------------------------------------------------------------------

static int count_rows_in_node(NSOutlineView *outline, MFTreeNodeImpl *node) {
  if ([outline isItemExpanded: node]) {
    NSUInteger count = node.children.count;
    for (NSUInteger i = 0, c = count; i < c; i++) {
      MFTreeNodeImpl *child = node.children[i];
      if (child)
        count += count_rows_in_node(outline, child);
    }
    return (int)count;
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static int row_for_node(NSOutlineView *outline, MFTreeNodeImpl *node) {
  MFTreeNodeImpl *parent = node.parent;
  NSUInteger node_index = [parent.children indexOfObject: node];
  NSUInteger row = node_index;

  if (parent) {
    for (NSUInteger i = 0; i < node_index; i++)
      row += count_rows_in_node(outline, parent.children[i]);

    row += row_for_node(outline, parent);
  }
  return (int)row;
}

//----------------------------------------------------------------------------------------------------------------------

static int treeview_row_for_node(mforms::TreeView *self, mforms::TreeNodeRef node) {
  MFTreeViewImpl *tree = self->get_data();
  TreeNodeImpl *nodei = from_ref(node);
  if (tree && nodei) {
    if (tree.frozen) {
      if (tree->mFlatTable)
        return (int)[tree->mRootNode.children indexOfObject: nodei->self()];
      else
        return row_for_node(tree->mOutline, nodei->self());
    } else
      return (int)[tree->mOutline rowForItem: nodei->self()]; // doesn't work if reloadData not called yet
  }
  return -1;
}

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

static mforms::TreeNodeRef treeview_node_with_tag(mforms::TreeView *self, const std::string &tag) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree->mTagIndexEnabled) {
    std::map<std::string, MFTreeNodeImpl *>:: iterator iter;
    if ((iter = tree->mTagMap.find(tag)) != tree->mTagMap.end())
      return iter->second.nodeRef;
  }
  return mforms::TreeNodeRef();
}

//----------------------------------------------------------------------------------------------------------------------

static mforms::TreeNodeRef treeview_node_at_row(mforms::TreeView *self, int row) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree && row >= 0) {
    if (tree.frozen) {
      if (tree->mFlatTable) {
        id node = tree->mRootNode.children[row];
        if (node)
          return [node nodeRef];
      } else {
        int counter = 0;
        return find_node_at_row(self->root_node(), counter, row);
      }
    } else {
      if ([tree isPendingReload])
        [tree reloadTreeData];

      id n = [tree->mOutline itemAtRow:row];
      if (n)
        return [n nodeRef];
    }
  }
  return mforms::TreeNodeRef();
}

//----------------------------------------------------------------------------------------------------------------------

mforms::TreeNodeRef treeview_node_at_position(mforms::TreeView *self, base::Point position) {
  MFTreeViewImpl *tree = self->get_data();
  NSInteger row = [tree->mOutline rowAtPoint:NSMakePoint((float)position.x, (float)position.y)];
  if (row < 0)
    return mforms::TreeNodeRef();

  id item = [tree->mOutline itemAtRow:row];
  if (item != nil)
    return [item nodeRef];

  return mforms::TreeNodeRef();
}

//----------------------------------------------------------------------------------------------------------------------

static void treeview_set_column_visible(mforms::TreeView *self, int column, bool flag) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree)
    [tree->mOutline tableColumnWithIdentifier: [NSString stringWithFormat: @"%i", column]].hidden = !flag;
}

//----------------------------------------------------------------------------------------------------------------------

static bool treeview_get_column_visible(mforms::TreeView *self, int column) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree)
    return ![tree->mOutline tableColumnWithIdentifier: [NSString stringWithFormat: @"%i", column]].hidden;
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

static void treeview_set_column_title(mforms::TreeView *self, int column, const std::string &title) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree)
    [tree->mOutline tableColumnWithIdentifier: [NSString stringWithFormat: @"%i", column]].headerCell.stringValue =
    [NSString stringWithCPPString:title];
}

//----------------------------------------------------------------------------------------------------------------------

static void treeview_set_column_width(mforms::TreeView *self, int column, int width) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree)
    [tree->mOutline tableColumnWithIdentifier: [NSString stringWithFormat: @"%i", column]].width = width;
}

//----------------------------------------------------------------------------------------------------------------------

static int treeview_get_column_width(mforms::TreeView *self, int column) {
  MFTreeViewImpl *tree = self->get_data();
  if (tree)
    return (int)[tree->mOutline tableColumnWithIdentifier: [NSString stringWithFormat: @"%i", column]].width;
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

static void beginUpdate(mforms::TreeView *self) {
}

//----------------------------------------------------------------------------------------------------------------------

static void endUpdate(mforms::TreeView *self) {
}

//----------------------------------------------------------------------------------------------------------------------

void cf_treeview_init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_treeview_impl.create = &treeview_create;
  f->_treeview_impl.add_column = &treeview_add_column;
  f->_treeview_impl.end_columns = &treeview_end_columns;

  f->_treeview_impl.clear = &treeview_clear;

  f->_treeview_impl.clear_selection = &treeview_clear_selection;
  f->_treeview_impl.get_selected_node = &treeview_get_selected;
  f->_treeview_impl.get_selection = &treeview_get_selection;
  f->_treeview_impl.set_selected = &treeview_set_selected;
  f->_treeview_impl.scrollToNode = &treeviewScrollToNode;

  f->_treeview_impl.set_selection_mode = &treeview_set_selection_mode;
  f->_treeview_impl.get_selection_mode = &treeview_get_selection_mode;

  f->_treeview_impl.set_row_height = &treeview_set_row_height;

  f->_treeview_impl.root_node = &treeview_root_node;

  f->_treeview_impl.set_allow_sorting = &treeview_allow_sorting;
  f->_treeview_impl.freeze_refresh = &treeview_freeze_refresh;

  f->_treeview_impl.row_for_node = &treeview_row_for_node;
  f->_treeview_impl.node_at_row = &treeview_node_at_row;
  f->_treeview_impl.node_at_position = &treeview_node_at_position;
  f->_treeview_impl.node_with_tag = &treeview_node_with_tag;
  
  f->_treeview_impl.set_column_visible = &treeview_set_column_visible;
  f->_treeview_impl.get_column_visible = &treeview_get_column_visible;
  
  f->_treeview_impl.set_column_title = &treeview_set_column_title;
  
  f->_treeview_impl.set_column_width = &treeview_set_column_width;
  f->_treeview_impl.get_column_width = &treeview_get_column_width;
    
  f->_treeview_impl.BeginUpdate = &beginUpdate;
  f->_treeview_impl.EndUpdate = &endUpdate;
}

@end

//----------------------------------------------------------------------------------------------------------------------
