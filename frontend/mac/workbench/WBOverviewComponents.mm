/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "WBOverviewComponents.h"
#import "WBOverviewListController.h"
#import "MCollectionViewItemView.h"
#import "WBOverviewPanel.h"
#import "MCPPUtilities.h"
#import "WBMenuManager.h"

//----------------------------------------------------------------------------------------------------------------------

static NSString *stringFromNodeId(const bec::NodeId &node) {
  return [NSString stringWithCPPString: node.toString()];
}

//----------------------------------------------------------------------------------------------------------------------

@implementation WBOverviewGroup

- (instancetype)initWithOverview: (WBOverviewPanel *)owner
                          nodeId: (const bec::NodeId &)node
                         tabItem: (NSTabViewItem *)tabItem {
  if (owner == nil)
    return nil;

  self = [super initWithFrame: {{ 0, 0 }, { 100, 100 }}];
  if (self != nil) {
    self.accessibilityTitle = @"Model Overview Group";
    _owner = owner;
    _be = owner.backend;
    _nodeId = new bec::NodeId(node);
    _tabItem = tabItem;

    [self setExpandSubviewsByDefault: NO];
    [self setBackgroundColor: NSColor.textBackgroundColor];
    [self setSpacing: 12];
    [self setPaddingLeft: 12 right: 12 top: 12 bottom: 12];
  }

  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithFrame: (NSRect)frame {
  return [self initWithOverview: nil nodeId: bec::NodeId() tabItem: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithOverview: nil nodeId: bec::NodeId() tabItem: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  delete _nodeId;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)updateNodeId:(const bec::NodeId&)node {
  delete _nodeId;
  _nodeId= new bec::NodeId(node);
  [_owner registerContainer:self forItem:stringFromNodeId(*_nodeId)];

  int i= 0;
  for (id subview in self.subviews) {
    if ([subview respondsToSelector:@selector(updateNodeId:)])
      [subview updateNodeId:_be->get_child(*_nodeId, i++)];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (bec::NodeId&)nodeId {
  return *_nodeId;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)buildChildren {
  // build the child items
  ssize_t child_type;
  _be->get_field(*_nodeId, wb::OverviewBE::ChildNodeType, child_type);

  NSAssert(child_type == wb::OverviewBE::OSection, @"unexpected child type for group");

  for (size_t c = _be->count_children(*_nodeId), i = 0; i < c; i++) {
    bec::NodeId child(_be->get_child(*_nodeId, i));

    WBOverviewSection *section = [[WBOverviewSection alloc] initWithOverview: _owner nodeId: child];
    [section setFrameSize: NSMakeSize(100, 20)];
    section.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [self addSubview: section];

    std::string title;
    _be->get_field(child, wb::OverviewBE::Label, title);
    [section setTitle: @(title.c_str())];

    [_owner registerContainer: section forItem: [NSString stringWithCPPString: child.toString().c_str()]];
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Sets the view mode of all attached collection views to the given mode.
 */
- (void)setListMode: (ListMode)mode {
  for (id subview in self.subviews) {
    WBOverviewSection* section = (WBOverviewSection*) subview;
    switch (mode) {
      case ListModeSmallIcon:
        [section setDisplayMode: wb::OverviewBE::MSmallIcon];
        break;
      case ListModeDetails:
        [section setDisplayMode: wb::OverviewBE::MList];
        break;
      default:
        [section setDisplayMode: wb::OverviewBE::MLargeIcon];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)refreshChildren {
  _be->refresh_node(*_nodeId, true);

  for (id subview in self.subviews) {
    if ([subview respondsToSelector:@selector(refreshChildren)])
      [subview refreshChildren];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)refreshInfo {
  std::string value;
  try {
    _be->refresh_node(*_nodeId, false);

    _be->get_field(*_nodeId, wb::OverviewBE::Label, value);
    if (_tabItem)
      _tabItem.label = [NSString stringWithCPPString:value];
  } catch (std::exception exc) {
    g_message("Error refreshing overview: %s", exc.what());
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize {
  NSSize size = NSMakeSize(leftPadding + rightPadding, topPadding + bottomPadding);
  NSArray *items = self.subviews;
  for (NSView *subview in items)
    size.height += NSHeight(subview.frame);

  if (items.count > 0)
    size.height += (items.count - 1) * spacing;
  return size;
}

@end

//----------------------------------------------------------------------------------------------------------------------

#pragma mark -

@implementation WBOverviewGroupContainer

- (instancetype)initWithOverview: (WBOverviewPanel *)owner
                          nodeId: (const bec::NodeId &)node {
  if (owner == nil)
    return nil;

  self = [super initWithFrame:NSMakeRect(0, 0, 100, 100)];
  if (self != nil) {
    self.tabViewType = NSTopTabsBezelBorder;
    
    [self doCustomize];
    self.delegate = self;
   
    _owner = owner;
    _be= owner.backend;
    _nodeId= new bec::NodeId(node);
    
    _extraHeight= NSHeight(self.frame) - self.contentSize.height;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithFrame: (NSRect)frame {
  return [self initWithOverview: nil nodeId: bec::NodeId()];
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithOverview: nil nodeId: bec::NodeId()];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  delete _nodeId;
}

//----------------------------------------------------------------------------------------------------------------------

- (WBOverviewGroup*)buildChildGroup:(bec::NodeId)child {
  std::string oid = _be->get_node_unique_id(child);

  NSTabViewItem *item = [[NSTabViewItem alloc] initWithIdentifier: [NSString stringWithCPPString: oid.c_str()]];

  std::string label;
  _be->get_field(child, 0, label);

  WBOverviewGroup *group = [[WBOverviewGroup alloc] initWithOverview: _owner
                                                              nodeId: child
                                                             tabItem: item];

  item.label = @(label.c_str());
  item.view = group;
  group.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
  
  [_owner registerContainer:group forItem: stringFromNodeId(child)];
  
  [group buildChildren];
  
  [self addTabViewItem: item];
  
  return group;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tile {
  if (!_resizing)
    [self resizeWithOldSuperviewSize: NSZeroSize];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeWithOldSuperviewSize: (NSSize)oldBoundsSize {
  float maxHeight = 0;

  _resizing = YES;
  
  for (NSTabViewItem *tabItem in self.tabViewItems) {
    WBOverviewGroup *group = (WBOverviewGroup *)tabItem.view;
    [group tile];
    maxHeight = MAX(maxHeight, [group minimumSize].height);
  }
  [self setFrameSize:NSMakeSize(self.contentSize.width, maxHeight + _extraHeight)];
  
  _resizing = NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)refreshChildren {
  std::set<std::string> existing_groups;
  float maxHeight = 0;
  NSTabViewItem *oldItem = self.selectedTabViewItem;

  // prevent the TabChanged handler from calling focus_node
  _updating = YES;
  
  _be->refresh_node(*_nodeId, true);
  
  // remove items that don't exist anymore
  for (NSInteger t = self.numberOfTabViewItems-1; t >= 0; --t) {
    NSTabViewItem *item = [self tabViewItemAtIndex:t];
    std::string item_uid = [item.identifier UTF8String]; // item identifier is node_unique_id
    BOOL found = NO;

    for (size_t c = _be->count_children(*_nodeId), i = 0; i < c; i++) {
      if (item_uid == _be->get_node_unique_id(_be->get_child(*_nodeId, i))) {
        found= YES;
        break;
      }
    }
    
    if (!found) {
      WBOverviewGroup *group = (WBOverviewGroup *)item.view;
      [_owner unregisterContainerForItem:stringFromNodeId(group.nodeId)];
      [self removeTabViewItem:item];
      if (oldItem == item)
        oldItem= nil;
    }
    existing_groups.insert(item_uid);
  }
    
  // insert new items
  for (size_t c = _be->count_children(*_nodeId), i = 0; i < c; i++) {
    bec::NodeId child(_be->get_child(*_nodeId, i));
    if (existing_groups.find(_be->get_node_unique_id(child)) == existing_groups.end()) {
      [self buildChildGroup: child];
    }
  }

  // Refresh the groups.
  NSTabViewItem *tabItem;
  if (self.numberOfTabViewItems > 0) {
    tabItem = self.tabViewItems[0];
    for (NSInteger i = 0; i < self.numberOfTabViewItems; ++i) {
      tabItem = self.tabViewItems[i];
      bec::NodeId child(_be->get_child(*_nodeId, i));

      // Update the represented NodeId.
      WBOverviewGroup *group = (WBOverviewGroup *)tabItem.view;
      [_owner unregisterContainerForItem: stringFromNodeId(group.nodeId)];
      [group updateNodeId: child];
      
      [group refreshChildren];
      [group tile];
      
      maxHeight =  MAX(maxHeight, group.minimumSize.height);
    }
  }
  
  [self setFrameSize: NSMakeSize(self.contentSize.width, maxHeight + _extraHeight)];
  
  _updating = NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)buildChildren {
  float maxHeight= 0;

  _updating = YES;

  // create a tabview page for each group
  for (size_t c = _be->count_children(*_nodeId), i = 0; i < c; i++) {
    bec::NodeId child(_be->get_child(*_nodeId, i));

    WBOverviewGroup *group = [self buildChildGroup: child];
    maxHeight = MAX(maxHeight, NSHeight([group frame]));
  }

  [self setFrameSize: NSMakeSize(self.contentSize.width, maxHeight + _extraHeight)];

  _updating = NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void) setLargeIconMode {
  NSTabViewItem *activeTab = self.selectedTabViewItem;
  WBOverviewGroup *group = (WBOverviewGroup *)activeTab.view;
  [group setListMode: ListModeLargeIcon];
}

//----------------------------------------------------------------------------------------------------------------------

- (void) setSmallIconMode {
  NSTabViewItem *activeTab = self.selectedTabViewItem;
  WBOverviewGroup *group = (WBOverviewGroup *)activeTab.view;
  [group setListMode: ListModeSmallIcon];
}

//----------------------------------------------------------------------------------------------------------------------

- (void) setDetailsMode {
  NSTabViewItem *activeTab = self.selectedTabViewItem;
  WBOverviewGroup *group = (WBOverviewGroup *)activeTab.view;
  [group setListMode: ListModeDetails];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)performGroupAdd: (id)sender {
  _be->request_add_object(*_nodeId);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)performGroupDelete:(id)sender {
  if (self.numberOfTabViewItems > 0) {
    NSInteger selected = [self indexOfTabViewItem: self.selectedTabViewItem];
    _be->request_delete_object(_be->get_child(*_nodeId, selected));
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tabViewItemDidReceiveDoubleClick: (NSTabViewItem*)item {
  bec::NodeId node(_be->get_child(*_nodeId, [self indexOfTabViewItem:item]));
  
  _be->activate_node(node);
}

//----------------------------------------------------------------------------------------------------------------------

- (NSMenu*)tabView: (NSTabView*) tabView menuForIdentifier: (id) identifier {
  bec::MenuItemList menuitems;
  std::vector<bec::NodeId> nodes;
  bec::NodeId node(*_nodeId);
  
  node.append([self indexOfTabViewItemWithIdentifier: identifier]);
  nodes.push_back(node);
  
  menuitems= _be->get_popup_items_for_nodes(nodes);
  
  if (!menuitems.empty()) {
    NSMenu *menu = [[NSMenu alloc] initWithTitle: [NSString stringWithCPPString: node.toString()]];
    [WBMenuManager fillMenu:menu withItems:menuitems selector:@selector(activateContextMenu:) target:self];
  
    return menu;
  }
  return nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)activateContextMenu:(id)sender {
  bec::NodeId node([sender menu].title.UTF8String);
  std::vector<bec::NodeId> nodes;

  // we need to pass the icon under the cursor
  // since all attempts to get that has failed so far,
  // we'll just use the 1st selected item for that purpose
  nodes.push_back(node);

  _be->activate_popup_item_for_nodes([[sender representedObject] UTF8String], nodes);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tabView:(NSTabView *)tabView willSelectTabViewItem:(NSTabViewItem *)tabViewItem {
  if (!_updating) {
    bec::NodeId node(_be->get_child(*_nodeId, [self indexOfTabViewItem:tabViewItem]));
    
    try {
      _be->focus_node(node);
    } catch (std::exception &exc) {
      NSLog(@"Can't focus node %s: %s", node.toString().c_str(), exc.what());
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tabView: (NSTabView *)tabView didSelectTabViewItem: (NSTabViewItem *)tabViewItem {
  WBOverviewGroup *group = (WBOverviewGroup *)tabViewItem.view;
  [group tile];
}

@end

//----------------------------------------------------------------------------------------------------------------------

#pragma mark -

@interface WBOverviewItemContainer () {
@protected
  __weak IBOutlet WBOverviewListController *iconController;
  __weak IBOutlet NSCollectionView *collectionView;
  __weak IBOutlet NSMenu *contextMenu;

  NSMutableArray *nibObjects;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation WBOverviewItemContainer

- (instancetype)initWithOverview: (WBOverviewPanel *)owner nodeId: (const bec::NodeId &)node {
  if (owner == nil)
    return nil;

  self = [super initWithFrame:NSMakeRect(0, 0, 100, 20)];
  if (self != nil) {
    NSMutableArray *temp;
    if ([NSBundle.mainBundle loadNibNamed: @"IconCollectionView" owner: self topLevelObjects: &temp]) {
      nibObjects = temp;

      _owner = owner;
      _be = owner.backend;
      _nodeId = new bec::NodeId(node);

      std::string label = _be->get_field_description(node, 0);
      if (!label.empty()) {
        _descriptionLabel= [[NSTextField alloc] initWithFrame: NSZeroRect];
        _descriptionLabel.stringValue = [NSString stringWithCPPString: label];
        [_descriptionLabel setEditable: NO];
        [_descriptionLabel setBordered: NO];
        _descriptionLabel.font = [NSFont systemFontOfSize: [NSFont labelFontSize]];
        [_descriptionLabel sizeToFit];
        [self addSubview: _descriptionLabel];
      }

      ssize_t displayMode = wb::OverviewBE::MSmallIcon;
      if (_be->get_field(node, wb::OverviewBE::DisplayMode, displayMode) &&
          displayMode == wb::OverviewBE::MLargeIcon) {
        [iconController setShowLargeIcons: YES];
      } else
        [iconController setShowLargeIcons: NO];

      _displayMode = (wb::OverviewBE::OverviewDisplayMode)displayMode;

      // listen to size changes from the icon view so we can resize to accomodate it
      [[NSNotificationCenter defaultCenter] addObserver: self
                                               selector: @selector(subviewResized:)
                                                   name: NSViewFrameDidChangeNotification
                                                 object: collectionView];

      [collectionView setAllowsMultipleSelection: NO];
      [collectionView setPostsFrameChangedNotifications: YES];

      [self addSubview: collectionView];

      self.menu = contextMenu;
      contextMenu.delegate = self;
      
      [iconController setOverviewBE: _be];
    }
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithFrame: (NSRect)frame {
  return [self initWithOverview: nil nodeId: bec::NodeId()];
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithOverview: nil nodeId: bec::NodeId()];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver: self];
  delete _nodeId;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)selectNode: (const bec::NodeId&)node {
  iconController.selectedIndexes = [NSIndexSet indexSetWithIndex: node.back()];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)clearSelection {
  iconController.selectedIndexes = [NSIndexSet indexSet];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)updateNodeId: (const bec::NodeId&)node {
  delete _nodeId;
  _nodeId = new bec::NodeId(node);
  
  [_owner registerContainer: self forItem: stringFromNodeId(*_nodeId)];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setDisplayMode: (wb::OverviewBE::OverviewDisplayMode)mode {
  if (_displayMode != mode) {
    _displayMode= mode;
    
    if (_displayMode == wb::OverviewBE::MLargeIcon) {
      [iconController setShowLargeIcons: YES];
    } else if (_displayMode == wb::OverviewBE::MSmallIcon) {
      [iconController setShowLargeIcons: NO];
    }

    [self refreshChildren];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)subviewResized: (NSNotification*)notif {
  id sender = notif.object;
  NSSize size = [sender frame].size;
  
  size.height += 16 + (_descriptionLabel ? NSHeight(_descriptionLabel.frame) + 4 : 0);
  size.width += 16;
  
  [self setFrameSize: size];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize {
  NSRect frame = self.frame;
  
  if (_descriptionLabel) {
    NSRect dframe = _descriptionLabel.frame;
    dframe.origin.x = 8;
    dframe.origin.y = NSHeight(frame) - NSHeight(dframe) - 5;
    _descriptionLabel.frame = dframe;
    
    frame.size.height -= NSHeight(_descriptionLabel.frame) + 4;
  }

  frame.size.height -= 16;
  frame.size.width -= 16;
  if (frame.size.width < 0)
    frame.size.width = 0;
  
  frame.origin.x += 8;
  frame.origin.y = 8;
  
  collectionView.frame = frame;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)refreshChildren {
  _be->refresh_node(*_nodeId, true);

  [iconController fillFromChildrenOf: *_nodeId
                            overview: _be
                            iconSize: _displayMode == wb::OverviewBE::MLargeIcon ? bec::Icon32 : bec::Icon16];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)refreshChildInfo: (const bec::NodeId&)node {
  _be->refresh_node(node, false);
  
  [self refreshChildren];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  if (_descriptionLabel != nil) {
    // Draw the separator line.
    NSImage *bar = [NSImage imageNamed:@"header_bar_gray.png"];
    NSRect frame = self.frame;
    [bar drawAtPoint: NSMakePoint(8, NSHeight(frame) - NSHeight(_descriptionLabel.frame) - 10)
            fromRect: NSMakeRect(0, 0, bar.size.width, bar.size.height)
           operation: NSCompositingOperationSourceOver
            fraction: 0.4];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)menuNeedsUpdate: (NSMenu *)menu {
  bec::MenuItemList menuitems;
  bec::NodeId node(*_nodeId);
  std::vector<bec::NodeId> nodes;
  
  // we need to pass the icon under the cursor
  // since all attempts to get that has failed so far,
  // we'll just use the 1st selected item for that purpose
  if (iconController.selectedIndexes.count > 0)
    nodes.push_back(node.append(iconController.selectedIndexes.firstIndex));
  else
    nodes.push_back(node);
  
  menuitems = _be->get_popup_items_for_nodes(nodes);
  std::string s;
  _be->get_field(node, 0, s);

  if (!menuitems.empty()) {
    [WBMenuManager fillMenu:menu withItems:menuitems selector:@selector(activateContextMenu:) target:self];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)activateContextMenu: (id)sender {
  bec::NodeId node(*_nodeId);
  std::vector<bec::NodeId> nodes;
  
  // we need to pass the icon under the cursor
  // since all attempts to get that has failed so far,
  // we'll just use the 1st selected item for that purpose
  if (iconController.selectedIndexes.count > 0)
    nodes.push_back(node.append(iconController.selectedIndexes.firstIndex));
  else
    nodes.push_back(node);
  _be->activate_popup_item_for_nodes([[sender representedObject] UTF8String], nodes);
}

@end

//----------------------------------------------------------------------------------------------------------------------

static NSDictionary *titleTextAttributes;
static NSDictionary *subtitleTextAttributes;

@implementation WBOverviewSection

+(void)initialize {
  titleTextAttributes = @{
    NSFontAttributeName: [NSFont boldSystemFontOfSize: 12],
    NSForegroundColorAttributeName: NSColor.labelColor
  };

  subtitleTextAttributes = @{
    NSFontAttributeName: [NSFont systemFontOfSize: 11],
    NSForegroundColorAttributeName: NSColor.secondaryLabelColor
  };

}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithOverview: (WBOverviewPanel*)owner nodeId: (const bec::NodeId&)node {
  self = [super initWithOverview: owner nodeId: node];
  if (self != nil) {
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)subviewResized: (NSNotification*)notif {
  id sender = notif.object;
  NSSize size = [sender frame].size;
  
  size.height += 16;
  size.width += 16;
  
  [self setFrameSize:size];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize {
  NSRect frame = self.frame;
  
  frame.size.height -= 16;
  frame.size.width -= 16;
  frame.origin.x += 8;
  frame.origin.y = 0;
  
  collectionView.frame = frame;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setTitle: (NSString*)title {
  if (_title != title) {
    _title= title;
    [self setNeedsDisplay: YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setSubTitle: (NSString*)title {
  if (_subTitle != title) {
    _subTitle= title;
    [self setNeedsDisplay: YES];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString*)title {
  return _title;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  NSImage *bar;
  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = self.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  if (isDark)
    bar = [NSImage imageNamed:@"header_bar_orange.png"];
  else
    bar = [NSImage imageNamed:@"header_bar_gray.png"];
  
  [[NSColor darkGrayColor] set];
  
  NSRect frame = self.frame;
  
  [_title drawAtPoint: NSMakePoint(10, NSHeight(frame) - 12)
       withAttributes: titleTextAttributes];
  NSSize size= [_title sizeWithAttributes: titleTextAttributes];

  [_subTitle drawAtPoint: NSMakePoint(10 + size.width + 12, NSHeight(frame) - 12)
          withAttributes: subtitleTextAttributes];
  
  [bar drawAtPoint: NSMakePoint(8, NSHeight(frame) - 17)
          fromRect: NSMakeRect(0, 0, bar.size.width, bar.size.height)
         operation: NSCompositingOperationSourceOver
          fraction: 0.4];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)refreshChildren {
  [super refreshChildren];
  
  size_t count = _be->count_children(*_nodeId) - 1;
  if (count != 1)
    [self setSubTitle: [NSString stringWithFormat: NSLocalizedString(@"(%i items)", @"!=1 items"), count]];
  else
    [self setSubTitle: [NSString stringWithFormat: NSLocalizedString(@"(%i item)", @"==1 item"), count]];
}

@end

//----------------------------------------------------------------------------------------------------------------------
