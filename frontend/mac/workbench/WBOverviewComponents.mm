/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

#import "WBOverviewComponents.h"
#import "WBOverviewListController.h"
#import "MCollectionViewItemView.h"
#import "WBOverviewPanel.h"
#import "MCPPUtilities.h"
#import "WBMenuManager.h"


@implementation WBOverviewGroupContainer

static NSString *stringFromNodeId(const bec::NodeId &node)
{
  return [NSString stringWithCPPString: node.repr()];
}


- (id)initWithOverview:(WBOverviewPanel*)owner
                nodeId:(const bec::NodeId&)node
{
  self= [super initWithFrame:NSMakeRect(0, 0, 100, 100)];
  if (self != nil)
  {
    [self setTabViewType:NSTopTabsBezelBorder];
    
    [self doCustomize];
    [self setDelegate:self];
   
    _owner= owner;
    _be= [owner backend];
    _nodeId= new bec::NodeId(node);
    
    _extraHeight= NSHeight([self frame]) - [self contentSize].height;
  }
  return self;
}

- (void)dealloc
{
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  delete _nodeId;
  [super dealloc];
}


- (WBOverviewGroup*)buildChildGroup:(bec::NodeId)child
{
  std::string oid= _be->get_node_unique_id(child);

  NSTabViewItem *item= [[NSTabViewItem alloc] initWithIdentifier:[NSString stringWithCPPString: oid.c_str()]];

  std::string label;
  _be->get_field(child, 0, label);

  WBOverviewGroup *group= [[[WBOverviewGroup alloc] initWithOverview:_owner
                                                              nodeId:child
                                                             tabItem:item] autorelease];

  [item setLabel:[NSString stringWithUTF8String:label.c_str()]];
  [item setView:group];
  [group setAutoresizingMask:NSViewWidthSizable|NSViewMaxYMargin];
  
  [_owner registerContainer:group forItem:stringFromNodeId(child)];
  
  [group buildChildren];
  
  [self addTabViewItem:[item autorelease]];
  
  return group;
}


- (void)tile
{
  if (!_resizing)
    [self resizeWithOldSuperviewSize: NSZeroSize];
}


- (void)resizeWithOldSuperviewSize:(NSSize)oldBoundsSize
{
  float maxHeight = 0;

  _resizing = YES;
  
  for (NSTabViewItem *tabItem in [self tabViewItems])
  {
    [[tabItem view] tile];
    
    maxHeight= MAX(maxHeight, [[tabItem view] minimumSize].height);
  }
  [self setFrameSize:NSMakeSize([self contentSize].width, maxHeight + _extraHeight)];
  
  _resizing = NO;
}


- (void)refreshChildren
{  
  std::set<std::string> existing_groups;
  float maxHeight= 0;
  NSTabViewItem *oldItem= [self selectedTabViewItem];

  // prevent the TabChanged handler from calling focus_node
  _updating = YES;
  
  _be->refresh_node(*_nodeId, true);
  
  // remove items that don't exist anymore
  for (NSInteger t= [self numberOfTabViewItems]-1; t >= 0; --t)
  {
    NSTabViewItem *item= [self tabViewItemAtIndex:t];
    std::string item_uid= [[item identifier] UTF8String]; // item identifier is node_unique_id
    BOOL found= NO;

    for (int c= _be->count_children(*_nodeId), i= 0; i < c; i++)
    {
      if (item_uid == _be->get_node_unique_id(_be->get_child(*_nodeId, i)))
      {
        found= YES;
        break;
      }
    }
    
    if (!found)
    {
      [_owner unregisterContainerForItem:stringFromNodeId([[item view] nodeId])];
      [self removeTabViewItem:item];
      if (oldItem == item)
        oldItem= nil;
    }
    existing_groups.insert(item_uid);
  }
    
  // insert new items
  for (int c= _be->count_children(*_nodeId), i= 0; i < c; i++)
  {
    bec::NodeId child(_be->get_child(*_nodeId, i));
    if (existing_groups.find(_be->get_node_unique_id(child)) == existing_groups.end())
    {
      [self buildChildGroup:child];
      // because of a bug in NSCollectionView, we need to make it visible on screen before refreshing
      // otherwise we get a crash
//      [self selectLastTabViewItem:nil]; 
//      [group refreshChildren];
    }
  }

  // reorder
  //MISSING CODE HERE

  // refresh everythng
  NSTabViewItem *tabItem;
  if ([self numberOfTabViewItems] > 0)
  {
    [self selectFirstTabViewItem:nil];
    tabItem= [self selectedTabViewItem];
    for (int i= 0;; i++)
    {
      bec::NodeId child(_be->get_child(*_nodeId, i));

      // update the represented NodeId
      [_owner unregisterContainerForItem:stringFromNodeId([[tabItem view] nodeId])];
      [_owner registerContainer:[tabItem view] forItem:stringFromNodeId([[tabItem view] nodeId])];
      [[tabItem view] updateNodeId: child];
      
      [[tabItem view] refreshChildren]; // refresh
      [[tabItem view] tile];
      
      maxHeight= MAX(maxHeight, [[tabItem view] minimumSize].height);
            
      [self selectNextTabViewItem:nil];
      NSTabViewItem *nextItem= [self selectedTabViewItem]; 
      if (nextItem == tabItem)
        break;
      tabItem= nextItem;
    }
  }
  
  [self setFrameSize:NSMakeSize([self contentSize].width, maxHeight + _extraHeight)];
  
  _updating = NO;

  int tab = _be->get_default_tab_page_index();

  [self tabView:self willSelectTabViewItem:[self selectedTabViewItem]];

  if (tab >= 0)
    [self performSelector: @selector(selectTabViewItemWithIdentifier:)
               withObject: [[self tabViewItemAtIndex: tab] identifier]
               afterDelay: 0.1];

  /// this raises an exception, check why
 // if (oldItem)
 //   [self selectTabViewItem:oldItem];
}


- (void)buildChildren
{
  float maxHeight= 0;

  _updating = YES;

  // create a tabview page for each group
  for (int c= _be->count_children(*_nodeId), i= 0; i < c; i++)
  {
    bec::NodeId child(_be->get_child(*_nodeId, i));

    WBOverviewGroup *group= [self buildChildGroup:child];
    
    maxHeight= MAX(maxHeight, NSHeight([group frame]));
  }

  [self setFrameSize:NSMakeSize([self contentSize].width, maxHeight + _extraHeight)];

  _updating = NO;
}

- (void) setLargeIconMode
{
  NSTabViewItem *activeTab = [self selectedTabViewItem];
  WBOverviewGroup *group = [activeTab view];
  [group setListMode: ListModeLargeIcon];
}

- (void) setSmallIconMode
{
  NSTabViewItem *activeTab = [self selectedTabViewItem];
  WBOverviewGroup *group = [activeTab view];
  [group setListMode: ListModeSmallIcon];
}

- (void) setDetailsMode
{
  NSTabViewItem *activeTab = [self selectedTabViewItem];
  WBOverviewGroup *group = [activeTab view];
  [group setListMode: ListModeDetails];
}

- (void)performGroupAdd:(id)sender
{
  _be->request_add_object(*_nodeId);
}


- (void)performGroupDelete:(id)sender
{
  if ([self numberOfTabViewItems] > 0)
  {
    int selected= [self indexOfTabViewItem: [self selectedTabViewItem]];
    _be->request_delete_object(_be->get_child(*_nodeId, selected));
  }
}



- (void)tabViewItemDidReceiveDoubleClick:(NSTabViewItem*)item
{
  bec::NodeId node(_be->get_child(*_nodeId, [self indexOfTabViewItem:item]));
  
  _be->activate_node(node);
}


- (NSMenu*)tabView: (NSTabView*) tabView
 menuForIdentifier: (id) identifier;
{
  bec::MenuItemList menuitems;
  std::vector<bec::NodeId> nodes;
  bec::NodeId node(*_nodeId);
  
  node.append([self indexOfTabViewItemWithIdentifier: identifier]);
  nodes.push_back(node);
  
  menuitems= _be->get_popup_items_for_nodes(nodes);
  
  if (!menuitems.empty())
  {
    NSMenu *menu = [[[NSMenu alloc] initWithTitle: [NSString stringWithCPPString: node.repr()]] autorelease];

    [WBMenuManager fillMenu:menu withItems:menuitems selector:@selector(activateContextMenu:) target:self];
  
    return menu;
  }
  return nil;
}


- (void)activateContextMenu:(id)sender
{
  bec::NodeId node([[[sender menu] title] UTF8String]);
  std::vector<bec::NodeId> nodes;

  // we need to pass the icon under the cursor
  // since all attempts to get that has failed so far,
  // we'll just use the 1st selected item for that purpose
  nodes.push_back(node);

  _be->activate_popup_item_for_nodes([[sender representedObject] UTF8String], nodes);
}


- (void)tabView:(NSTabView *)tabView willSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
  if (!_updating)
  {
    bec::NodeId node(_be->get_child(*_nodeId, [self indexOfTabViewItem:tabViewItem]));
    
    try
    {
      _be->focus_node(node);
    }
    catch (std::exception &exc)
    {
      NSLog(@"Can't focus node %s: %s", node.repr().c_str(), exc.what());
    }
  }
}


- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
  [[tabViewItem view] tile];
}


@end


@implementation WBOverviewGroup

- (id)initWithOverview:(WBOverviewPanel*)owner
                nodeId:(const bec::NodeId&)node
               tabItem:(NSTabViewItem*)tabItem
{
  self= [super initWithFrame:NSMakeRect(0, 0, 100, 100)];
  if (self != nil)
  {    
    _owner= owner;
    _be= [owner backend];
    _nodeId= new bec::NodeId(node);
    _tabItem= tabItem;
    
    [self setExpandSubviewsByDefault: NO];
    [self setBackgroundColor:[NSColor whiteColor]];
    [self setSpacing: 12];
    [self setPaddingLeft: 12 right: 12 top: 12 bottom: 12];
  }
  
  return self;
}


- (void)dealloc
{
  delete _nodeId;
  [super dealloc];
}

- (void)updateNodeId:(const bec::NodeId&)node
{
  delete _nodeId;
  _nodeId= new bec::NodeId(node);
  [_owner registerContainer:self forItem:stringFromNodeId(*_nodeId)];
  
  int i= 0;
  for (id subview in [self subviews])
  {
    if ([subview respondsToSelector:@selector(updateNodeId:)])
      [subview updateNodeId:_be->get_child(*_nodeId, i++)];
  }
}


- (bec::NodeId&)nodeId
{
  return *_nodeId;
}

- (void)buildChildren
{
  // build the child items
  int child_type;
  _be->get_field(*_nodeId, wb::OverviewBE::ChildNodeType, child_type);
  
  NSAssert(child_type == wb::OverviewBE::OSection, @"unexpected child type for group");
  
  for (int c= _be->count_children(*_nodeId), i= 0; i < c; i++)
  {
    bec::NodeId child(_be->get_child(*_nodeId, i));
    
    WBOverviewSection *section= [[[WBOverviewSection alloc] initWithOverview:_owner
                                                                      nodeId:child] autorelease];
    [section setFrameSize:NSMakeSize(100, 20)];
    [section setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    [self addSubview:section];
    
    std::string title;
    _be->get_field(child, wb::OverviewBE::Label, title);
    [section setTitle:[NSString stringWithUTF8String:title.c_str()]];

    [_owner registerContainer:section forItem:[NSString stringWithCPPString: child.repr().c_str()]];
    
  // NSContainerView doens't like being refreshed here
 //   [section refreshChildren];
  }
}

/**
 * Sets the view mode of all attached collection views to the given mode.
 */
- (void) setListMode: (ListMode) mode
{
  for (id subview in [self subviews])
  {
    WBOverviewSection* section = (WBOverviewSection*) subview;
    switch (mode)
    {
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

- (void)refreshChildren
{
  _be->refresh_node(*_nodeId, true);

  for (id subview in [self subviews])
  {
    if ([subview respondsToSelector:@selector(refreshChildren)])
      [subview refreshChildren];
  }
}


- (void)refreshInfo
{
  std::string value;
  try
  {
    _be->refresh_node(*_nodeId, false);
    
    _be->get_field(*_nodeId, wb::OverviewBE::Label, value);
    if (_tabItem)
      [_tabItem setLabel:[NSString stringWithCPPString:value]];
  }
  catch (std::exception exc) 
  {
    g_message("Error refreshing overview: %s", exc.what());
  }
}


- (NSSize)minimumSize
{
  NSSize size = NSMakeSize(leftPadding + rightPadding, topPadding + bottomPadding);
  NSArray *items = [self subviews];
  for (NSView *subview in items)
    size.height += NSHeight([subview frame]);
  if ([items count] > 0)
    size.height += ([items count] - 1) * spacing;
  return size;
}

@end
     
     



@implementation WBOverviewItemContainer


- (id)initWithOverview:(WBOverviewPanel*)owner
                nodeId:(const bec::NodeId&)node
{
  self = [super initWithFrame:NSMakeRect(0, 0, 100, 20)];
  if (self != nil)
  {
    _owner= owner;
    _be= [owner backend];
    _nodeId= new bec::NodeId(node);
    
    std::string label= _be->get_field_description(node, 0);
    if (!label.empty())
    {
      _descriptionLabel= [[[NSTextField alloc] initWithFrame:NSZeroRect] autorelease];
      [_descriptionLabel setStringValue: [NSString stringWithCPPString: label]];
      [_descriptionLabel setEditable: NO];
      [_descriptionLabel setBordered: NO];
      [_descriptionLabel setTextColor: [NSColor lightGrayColor]];
      [_descriptionLabel setFont: [NSFont systemFontOfSize: [NSFont labelFontSize]]];
      [_descriptionLabel sizeToFit];
      [self addSubview: _descriptionLabel];
    }
    
    _nibObjects= [[NSMutableArray array] retain];
    NSDictionary *nameTable= [NSDictionary dictionaryWithObject:_nibObjects forKey:NSNibTopLevelObjects];

    [NSBundle loadNibFile:[[NSBundle mainBundle] pathForResource:@"IconCollectionView"
                                                          ofType:@"nib"]
        externalNameTable:nameTable withZone:nil];

    // catch some objects from the nib
    for (id object in _nibObjects)
    {
      if ([object isKindOfClass:[NSCollectionView class]])
        _iconView= (NSCollectionView*)object;

      if ([object isKindOfClass:[WBOverviewListController class]])
        _iconController= (WBOverviewListController*)object;

      if ([object isKindOfClass:[NSMenu class]])
        _contextMenu= (NSMenu*)object;
    }
    
    int displayMode= wb::OverviewBE::MSmallIcon;
    if (_be->get_field(node, wb::OverviewBE::DisplayMode, displayMode) &&
        displayMode == wb::OverviewBE::MLargeIcon)
    {
      [_iconController setShowLargeIcons: YES];
    }
    else
      [_iconController setShowLargeIcons: NO];
    
    _displayMode= (wb::OverviewBE::OverviewDisplayMode)displayMode;

    // listen to size changes from the icon view so we can resize to accomodate it
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(subviewResized:)
                                                 name:NSViewFrameDidChangeNotification
                                               object:_iconView];
    
    [_iconView setAllowsMultipleSelection: NO];
    [_iconView setPostsFrameChangedNotifications:YES];
      
    [self addSubview:_iconView];
    
    [_iconView setMenu: _contextMenu];
    [self setMenu: _contextMenu];
    [_contextMenu setDelegate: self];
    
    [_iconController setOverviewBE:_be];
  }
  return self;
}


- (void)dealloc
{
  for (id object in _nibObjects)
    [object release];
  [_nibObjects release];
  
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  
  delete _nodeId;
  
  [super dealloc];
}


- (void)selectNode:(const bec::NodeId&)node
{
  int index= node.back();
  
  [_iconController setSelectedIndexes: [NSIndexSet indexSetWithIndex: index]];
}


- (void)clearSelection
{
  [_iconController setSelectedIndexes: [NSIndexSet indexSet]];
}


- (void)updateNodeId:(const bec::NodeId&)node
{
  delete _nodeId;
  _nodeId= new bec::NodeId(node);
  
  [_owner registerContainer:self forItem:stringFromNodeId(*_nodeId)];
}


- (void)setDisplayMode:(wb::OverviewBE::OverviewDisplayMode)mode
{
  if (_displayMode != mode)
  {
    _displayMode= mode;
    
    if (_displayMode == wb::OverviewBE::MLargeIcon)
    {
      [_iconController setShowLargeIcons: YES];
    }
    else if (_displayMode == wb::OverviewBE::MSmallIcon)
    {
      [_iconController setShowLargeIcons: NO];
    }

    [self refreshChildren];
  }
}



- (void)subviewResized:(NSNotification*)notif
{
  id sender= [notif object];
  NSSize size= [sender frame].size;
  
  size.height+= 16 + (_descriptionLabel ? NSHeight([_descriptionLabel frame]) + 4 : 0);
  size.width+= 16;
  
  [self setFrameSize:size];
}


- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  NSRect frame= [self frame];
  
  if (_descriptionLabel)
  {
    NSRect dframe= [_descriptionLabel frame];
    dframe.origin.x= 8;
    dframe.origin.y= NSHeight(frame) - NSHeight(dframe) - 5;
    [_descriptionLabel setFrame: dframe];
    
    frame.size.height-= NSHeight([_descriptionLabel frame]) + 4;
  }
  frame.size.height-= 16;
  frame.size.width-= 16;
  if (frame.size.width < 0)
    frame.size.width= 0;
  
  frame.origin.x+= 8;
  frame.origin.y= 8;
  
  [_iconView setFrame:frame];
}


- (void)refreshChildren
{  
  _be->refresh_node(*_nodeId, true);

  [_iconController fillFromChildrenOf: *_nodeId
                             overview: _be
                             iconSize: _displayMode == wb::OverviewBE::MLargeIcon ? bec::Icon32 : bec::Icon16];  
}


- (void)refreshChildInfo:(const bec::NodeId&)node
{
  _be->refresh_node(node, false);
  
  [self refreshChildren];
  //NSLog(@"refresh node info");
}

- (void)drawRect:(NSRect)rect
{
  if (_descriptionLabel)
  {
    [self lockFocus];
    
    NSImage *bar= [NSImage imageNamed:@"header_bar_gray.png"];
   
    NSRect frame= [self frame];
    
    [bar drawAtPoint:NSMakePoint(8, NSHeight(frame) - NSHeight([_descriptionLabel frame]) - 10)
            fromRect:NSMakeRect(0, 0, [bar size].width, [bar size].height)
           operation:NSCompositeSourceOver
            fraction:0.4];
    
    [self unlockFocus];
  }
}

/*
- (void) menuNeedsUpdate: (NSMenu*) menu
{
  bec::MenuItemList menuitems;
  std::vector<bec::NodeId> nodes;
  
  if ([[_iconController selectedIndexes] count] > 0)
  {
    NSIndexSet* indices = [_iconController selectedIndexes];
    for (NSUInteger index = [indices firstIndex]; index < [indices count]; index++)
      if ([indices containsIndex: index])
      {
        bec::NodeId node;
        NSDictionary* entry = [[_iconController items] objectAtIndex: index];
        NSString *path= [entry objectForKey: @"path"];
        node= bec::NodeId([path UTF8String]);
        nodes.push_back(node);
      }
    
    menuitems = _be->get_popup_items_for_nodes(nodes);
  }
  else
  {
    NSMutableArray* items = [_iconController items];
    for (int index = 0; index < [items count]; index++)
    {
      bec::NodeId node;
      NSDictionary* entry = [items objectAtIndex: index];
      NSString *path= [entry objectForKey: @"path"];
      node= bec::NodeId([path UTF8String]);
      nodes.push_back(node);
    }
    
    menuitems = _be->get_popup_items_for_nodes(nodes);
  }
  if (!menuitems.empty())
    [[_owner menuManager] refreshMenu: menu withItems: menuitems];
}*/

- (void)menuNeedsUpdate:(NSMenu *)menu
{
  bec::MenuItemList menuitems;
  bec::NodeId node(*_nodeId);
  std::vector<bec::NodeId> nodes;
  
  // we need to pass the icon under the cursor
  // since all attempts to get that has failed so far,
  // we'll just use the 1st selected item for that purpose
  if ([[_iconController selectedIndexes] count] > 0)
    nodes.push_back(node.append([[_iconController selectedIndexes] firstIndex]));
  else
    nodes.push_back(node);
  
  menuitems= _be->get_popup_items_for_nodes(nodes);
  std::string s;
  _be->get_field(node, 0, s);
  //NSLog(@"=== menu for %s has %i items", s.c_str(), menuitems.size());
  
  if (!menuitems.empty())
  {
    [WBMenuManager fillMenu:menu withItems:menuitems selector:@selector(activateContextMenu:) target:self];
  }
}

- (void)activateContextMenu:(id)sender
{
  bec::NodeId node(*_nodeId);
  std::vector<bec::NodeId> nodes;
  
  // we need to pass the icon under the cursor
  // since all attempts to get that has failed so far,
  // we'll just use the 1st selected item for that purpose
  if ([[_iconController selectedIndexes] count] > 0)
    nodes.push_back(node.append([[_iconController selectedIndexes] firstIndex]));
  else
    nodes.push_back(node);
  _be->activate_popup_item_for_nodes([[sender representedObject] UTF8String], nodes);
}


@end



@implementation WBOverviewSection

- (id)initWithOverview:(WBOverviewPanel*)owner
                nodeId:(const bec::NodeId&)node
{
  self = [super initWithOverview:owner nodeId:node];
  if (self != nil)
  {
//    [self setDisplayMode:wb::OverviewBE::MSmallIcon];
  }
  return self;
}

- (void) dealloc
{
  [_title release];
  [_subTitle release];
  [super dealloc];
}


- (void)subviewResized:(NSNotification*)notif
{
  id sender= [notif object];
  NSSize size= [sender frame].size;
  
  size.height+= 16;
  size.width+= 16;
  
  [self setFrameSize:size];
}


- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  NSRect frame= [self frame];
  
  frame.size.height-= 16;
  frame.size.width-= 16;
  frame.origin.x+= 8;
  frame.origin.y= 0;
  
  [_iconView setFrame:frame];
}


- (void)setTitle:(NSString*)title
{
  if (_title != title)
  {
    [_title release];
    _title= [title retain];
    [self setNeedsDisplay:YES];
  }
}


- (void)setSubTitle:(NSString*)title
{
  if (_subTitle != title)
  {
    [_subTitle release];
    _subTitle= [title retain];
    [self setNeedsDisplay:YES];
  }
}


- (NSString*)title
{
  return _title;
}


- (void)drawRect:(NSRect)rect
{
  [self lockFocus];
  
  NSImage *bar= [NSImage imageNamed:@"header_bar_gray.png"];
  
  [[NSColor darkGrayColor] set];
  
  NSRect frame= [self frame];
  
  NSDictionary *attribs= [NSDictionary dictionaryWithObjectsAndKeys:
                          [NSFont boldSystemFontOfSize:11], NSFontAttributeName,
                          nil];
  [_title drawAtPoint:NSMakePoint(10, NSHeight(frame) - 12)
       withAttributes:attribs];
  NSSize size= [_title sizeWithAttributes: attribs];

  [_subTitle drawAtPoint:NSMakePoint(10 + size.width + 12, NSHeight(frame) - 12)
       withAttributes:[NSDictionary dictionaryWithObjectsAndKeys:
                       [NSFont systemFontOfSize: 9], NSFontAttributeName,
                       [NSColor grayColor], NSForegroundColorAttributeName,
                       nil]];
  
  [bar drawAtPoint:NSMakePoint(8, NSHeight(frame) - 17)
          fromRect:NSMakeRect(0, 0, [bar size].width, [bar size].height)
         operation:NSCompositeSourceOver
          fraction:0.4];
  
  [self unlockFocus];
}


- (void)refreshChildren
{  
  [super refreshChildren];
  
  int count= _be->count_children(*_nodeId) - 1;
  if (count != 1)
    [self setSubTitle: [NSString stringWithFormat: NSLocalizedString(@"(%i items)", @"!=1 items"), count]];
  else
    [self setSubTitle: [NSString stringWithFormat: NSLocalizedString(@"(%i item)", @"==1 item"), count]];
}

@end
