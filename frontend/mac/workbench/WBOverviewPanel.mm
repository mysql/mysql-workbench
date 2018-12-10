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

#import "WBOverviewPanel.h"
#import "MTogglePane.h"
#import "WBOverviewListController.h"
#import "WBSchemaTabView.h"
#import "WBSchemaTabItem.h"
#import "WBOverviewComponents.h"
#import "MVerticalLayoutView.h"

#include "workbench/wb_context.h"
#include "base/string_utilities.h"

//----------------------------------------------------------------------------------------------------------------------

@interface WBOverviewBackgroundView : MVerticalLayoutView {
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation WBOverviewBackgroundView

- (instancetype)initWithFrame: (NSRect)rect {
  self = [super initWithFrame:rect];
  if (self != nil) {
    [self setExpandSubviewsByDefault: NO];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)drawRect: (NSRect)rect {
  [NSColor.textBackgroundColor set];
  NSRectFill(rect);
  return;
}

@end

//----------------------------------------------------------------------------------------------------------------------

#pragma mark -

@implementation WBOverviewPanel

static NSString *stringFromNodeId(const bec::NodeId &node) {
  return @(node.toString().c_str());
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setupWithOverviewBE: (wb::OverviewBE*)overview {
  _overview = overview;
  _overview->set_frontend_data((__bridge void *)self);
  
  _panelId = @(_overview->identifier().c_str());
  
  self.hasVerticalScroller = YES;
  self.hasHorizontalScroller = NO;
  self.borderType = NSNoBorder;
  
  _backgroundView = [[WBOverviewBackgroundView alloc] initWithFrame:
                     NSMakeRect(0, 0, self.contentSize.width, self.contentSize.height)];
  _backgroundView.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin| NSViewMinXMargin | NSViewMaxXMargin;
  self.documentView = _backgroundView;
  
  _itemContainers = [[NSMutableDictionary alloc] init];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setNoHeader {
  _noHeaders = YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  delete _lastFoundNode;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)willClose {
  if (_overview->can_close()) {
    _overview->close();
    _overview = NULL;
    return YES;
  }
  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSImage*)tabIcon {
  return [NSImage imageNamed: [NSString stringWithFormat: @"tab.%s.16x16", _overview->get_form_context_name().c_str()]];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)searchString:(NSString*)text {
  if (_searchText && [text hasPrefix:_searchText]) {
    // nothing
  } else {
    delete _lastFoundNode;
    _lastFoundNode= 0;
  }
  
  _searchText = text;
  
  bec::NodeId node= _overview->search_child_item_node_matching(bec::NodeId(), 
                                                             _lastFoundNode ? *_lastFoundNode : bec::NodeId(),
                                                             _searchText.UTF8String);
  if (node.is_valid()) {
    _lastFoundNode= new bec::NodeId(node);
    
    id container= _itemContainers[stringFromNodeId(_overview->get_parent(node))];
    
    for (id cont in [_itemContainers objectEnumerator]) {
      if (cont == container) {
        if ([container respondsToSelector:@selector(selectNode:)])
          [container selectNode: *_lastFoundNode];
      } else {
        if ([cont respondsToSelector:@selector(clearSelection)])
          [cont clearSelection];
      }
    }
    
    std::string label;
    _overview->get_field(*_lastFoundNode, wb::OverviewBE::Label, label);
    
    bec::GRTManager::get()->replace_status_text(base::strfmt(_("Found '%s'"), label.c_str()));
  } else {
    delete _lastFoundNode;
    _lastFoundNode= 0;
    bec::GRTManager::get()->replace_status_text(_("No matches found."));
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSView*)topView {
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString*)title {
  try {
    return @(_overview->get_title().c_str());
  } catch (...) {
    return @"Overview";
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString*)panelId {
  return _panelId;
}

//----------------------------------------------------------------------------------------------------------------------

- (bec::UIForm*)formBE {
  return _overview;
}

//----------------------------------------------------------------------------------------------------------------------

- (wb::OverviewBE*)backend {
  return _overview;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)refreshAll {
  for (MTogglePane *item in _backgroundView.subviews) {
    id view = item.contentView;
    
    if ([view respondsToSelector:@selector(refreshChildren)])
      [view refreshChildren];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)rebuildAll {
  if (!_overview)
    return;

  // remember name of all selected tabs
  NSMutableDictionary *selectedTabs = [NSMutableDictionary dictionary];
  
  for (id key in [_itemContainers keyEnumerator]) {
    id item = _itemContainers[key];
    if ([item isKindOfClass: [WBOverviewGroupContainer class]]) {
      WBOverviewGroupContainer *group = item;
      NSInteger index = [group indexOfTabViewItem: group.selectedTabViewItem];
      if (index != NSNotFound)
        selectedTabs[key] = @((int)index);
    }
  }

  [_itemContainers removeAllObjects];
  [self buildMainSections];
  [self refreshAll];

  if (NSHeight(_backgroundView.frame) > NSHeight(self.visibleRect))
    [self.contentView scrollToPoint: NSMakePoint(0, NSHeight(_backgroundView.frame) - NSHeight(self.visibleRect))];
}

//----------------------------------------------------------------------------------------------------------------------

- (id)itemContainerForNode:(const bec::NodeId&)node {
  return _itemContainers[@(node.toString().c_str())];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)registerContainer: (id)container forItem: (NSString*)item {
  _itemContainers[item] = container;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)unregisterContainerForItem: (NSString*)item {
  [_itemContainers removeObjectForKey:item];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)refreshNode: (const bec::NodeId&)node {
  id container= _itemContainers[stringFromNodeId(node)];

  if (container && [container respondsToSelector:@selector(refreshInfo)])
    [container refreshInfo];
  else {
    container= _itemContainers[stringFromNodeId(_overview->get_parent(node))];
  
    if ([container respondsToSelector:@selector(refreshChildInfo:)])
      [container refreshChildInfo:node];
    else
      NSLog(@"node %s does not handle refreshing", node.toString().c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)refreshNodeChildren: (const bec::NodeId&)node {
  if (node.is_valid()) {
    try {
      ssize_t type;
          
      if (!_overview->get_field(node, wb::OverviewBE::ChildNodeType, type))
        return;
      
      switch ((wb::OverviewBE::OverviewNodeType)type) {
        case wb::OverviewBE::OGroup:
          [_itemContainers[stringFromNodeId(node)] refreshChildren];
          break;
          
        case wb::OverviewBE::OItem:
          [_itemContainers[stringFromNodeId(node)] refreshChildren];
          break;
          
        default: break;
      }
      
      // find the container group of the refreshed node to request a size update
      bec::NodeId parent;
      
      parent= _overview->get_parent(node);
      while (parent.is_valid()) {
        id container = _itemContainers[stringFromNodeId(parent)];
        if ([container isKindOfClass: [WBOverviewGroupContainer class]]) {
          [container tile];
          break;
        }
        parent= _overview->get_parent(parent);
      }
      
      return;
    }
    catch (const std::exception &exc) {
      // ignore
    }
  }
  [self rebuildAll];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSView*)buildDivision: (const bec::NodeId&)node inPane: (MTogglePane*)pane {
  ssize_t child_type;
  if (!_overview->get_field(node, wb::OverviewBE::ChildNodeType, child_type))
    return nil;
  
  switch (child_type) {
    case wb::OverviewBE::OGroup: {
      WBOverviewGroupContainer *groups = [[WBOverviewGroupContainer alloc] initWithOverview: self
                                                                                     nodeId: node];
      pane.contentView = groups;

      [pane addButton: [NSImage imageNamed: @"collapsing_panel_header_tab_add"]
           withAction: @selector(performGroupAdd:)
               target: groups];
      [pane addButton: [NSImage imageNamed: @"collapsing_panel_header_tab_del"]
           withAction: @selector(performGroupDelete:)
               target: groups];

      _itemContainers[@(node.toString().c_str())] = groups;
      [groups buildChildren];

      return groups;
    }

    case wb::OverviewBE::OItem: {
      WBOverviewItemContainer *itemList = [[WBOverviewItemContainer alloc] initWithOverview: self
                                                                                     nodeId: node];
      _itemContainers[@(node.toString().c_str())] = itemList;
      pane.contentView = itemList;

      return itemList;
    }

    case wb::OverviewBE::OSection: {
      WBOverviewGroup *group = [[WBOverviewGroup alloc] initWithOverview:self nodeId:node tabItem:nil];
      _itemContainers[@(node.toString().c_str())] = group;
      [group buildChildren];
      pane.contentView = group;

      return group;
    }
  }

  return nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)buildMainSections {
  bec::NodeId root, node;
  CGFloat width= NSWidth(_backgroundView.frame);

  for (NSView *subview in [_backgroundView.subviews reverseObjectEnumerator])
    [subview removeFromSuperview];

  _overview->refresh();
  for (size_t i = 0; i < _overview->count_children(root); i++) {
    std::string label;
    wb::OverviewBE::OverviewNodeType nodeType = wb::OverviewBE::ODivision;
    ssize_t expanded;

    node = _overview->get_child(root, i);

    if (node.is_valid()) {
      _overview->get_field(node, wb::OverviewBE::Label, label);
      ssize_t temp;
      _overview->get_field(node, wb::OverviewBE::NodeType, temp);
      nodeType = (wb::OverviewBE::OverviewNodeType)temp;
      _overview->get_field(node, wb::OverviewBE::Expanded, expanded);
      
      if (nodeType == wb::OverviewBE::ODivision) {
        MTogglePane *pane = [[MTogglePane alloc] initWithFrame: NSMakeRect(0, 0, width, 100)
                                                 includeHeader: !_noHeaders];
        [pane setLabel: @(label.c_str())];
        pane.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
        [_backgroundView addSubview: pane];
        
        id view = [self buildDivision: node
                               inPane: pane];
        if (view != nil)
          [pane setExpanded: expanded != 0];
      } else {
        NSLog(@"ERROR: unexpected node type in Overview");
      }
    }
  }
  [_backgroundView tile];
}

@end

//----------------------------------------------------------------------------------------------------------------------
