//
//  WBOverviewListController.mm
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 12/Oct/08.
//  Copyright 2008 Sun Microsystems Inc. All rights reserved.
//

#import "WBOverviewListController.h"
#import "GRTIconCache.h"
#import "MCPPUtilities.h"
#import "MCollectionViewItemView.h"

#include "grtdb/db_object_helpers.h"
#include "workbench/wb_context.h" // temporary

@implementation WBOverviewListController

- (id)init
{
  if ((self= [super init]) != nil)
  {
    mSelectedIndexes= nil;
  }
  return self;
}


- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  
  [mSelectedIndexes release];
  [mItems release];
  
  [super dealloc];
}


- (void)setCollectionView:(NSCollectionView*)view
{
  collectionView= view;
  
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(collectionFrameChanged:)
                                               name:NSViewFrameDidChangeNotification
                                             object:view];
}


- (void)relayoutCollectionView:(NSCollectionView*)collection
{
  int count= [mItems count];
  NSRect itemRect= [[[collection itemPrototype] view] frame];
  int itemsPerRow;
  int rowCount= 0;
  NSSize newSize;
  
  itemsPerRow= (int)floor(NSWidth([collection frame]) / NSWidth(itemRect));
  if (itemsPerRow > 0)
  {
    rowCount= ceil((count + (itemsPerRow/2)) / itemsPerRow);
    if (rowCount * itemsPerRow < count)
      rowCount++;
  }
  if (rowCount == 0)
    rowCount= 1;
  
  newSize.width= NSWidth([collection frame]);
  newSize.height= NSHeight(itemRect) * rowCount;
  
  if (!NSEqualSizes([collection frame].size, newSize))
    [collection setFrameSize:newSize];
  
  [collection setNeedsDisplay:YES];
}


- (void)collectionFrameChanged:(NSNotification*)notif
{
  NSCollectionView *collection= [notif object];
  
  [self relayoutCollectionView:collection];
}


- (void)fillFromChildrenOf:(const bec::NodeId&)node
                  overview:(wb::OverviewBE*)overview
                  iconSize:(bec::IconSize)iconSize
{
  NSMutableArray *items= [NSMutableArray array];
  GRTIconCache *iconCache= [GRTIconCache sharedIconCache];
  
  for (size_t c= overview->count_children(node), i= 0; i < c; i++)
  {
    bec::NodeId child= overview->get_child(node, i);
    NSImage *icon;
    std::string name;
    
    overview->get_field(child, wb::OverviewBE::Label, name);
    icon= [iconCache imageForIconId:overview->get_field_icon(child, wb::OverviewBE::Label, iconSize)];
    if (!icon)
      icon= [NSImage imageNamed:@"MySQLWorkbench-16.png"];
    
    [items addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:
                      [NSString stringWithUTF8String: name.c_str()], @"name",
                      [NSString stringWithCPPString: child.repr().c_str()], @"path",
                      icon, @"image", // put image as last because it can be nil if the image doesnt exist
                      nil]];
  }
  
  [self setItems:items];
  
  [self relayoutCollectionView:collectionView];
}

//--------------------------------------------------------------------------------------------------

- (void)setShowLargeIcons:(BOOL)flag
{
  if (flag)
    [collectionView setItemPrototype:largeIcon];
  else
    [collectionView setItemPrototype:smallIcon];
}

//--------------------------------------------------------------------------------------------------

- (void)setItems:(NSMutableArray*)items
{
  if (mItems != items)
  {
    [mItems autorelease];
    mItems= [items retain];
  }
}

//--------------------------------------------------------------------------------------------------

- (NSIndexSet*)selectedIndexes
{
  return mSelectedIndexes;
}

//--------------------------------------------------------------------------------------------------

/**
 * Add a single index to the current selection.
 */
- (void) selectIndex: (NSUInteger) index
{
  if (mSelectedIndexes == nil)
    mSelectedIndexes = [[NSMutableIndexSet indexSetWithIndex: index] retain];
  else
    [mSelectedIndexes addIndex: index];
  
  mOverview->begin_selection_marking();
  
  NSDictionary *item= [mItems objectAtIndex: index];
  bec::NodeId node;
  NSString *path= [item objectForKey: @"path"];
  node= bec::NodeId([path CPPString]);
  mOverview->select_node(node);
  
  mOverview->end_selection_marking();
}

//--------------------------------------------------------------------------------------------------

/**
 * Add a range of selected indices to the current selection.
 */
- (void) setSelectedIndexes: (NSIndexSet*) indexes
{  
  if (mSelectedIndexes != indexes)
  {    
    [mSelectedIndexes autorelease];
    if ([indexes isKindOfClass: [NSMutableIndexSet class]])
      mSelectedIndexes= (NSMutableIndexSet*)[indexes retain];
    else
    {
      mSelectedIndexes = [[NSMutableIndexSet indexSet] retain];
      [mSelectedIndexes initWithIndexSet: indexes];
    }
    
    if (mSelectedIndexes)
    {
      mOverview->begin_selection_marking();
      if ([mSelectedIndexes count] > 0)
      {
        for (NSUInteger i= [mSelectedIndexes firstIndex]; i <= [mSelectedIndexes lastIndex]; i= [mSelectedIndexes indexGreaterThanIndex:i])
        {
          NSDictionary *item= [mItems objectAtIndex:i];
          bec::NodeId node;
          NSString *path= [item objectForKey: @"path"];
          node= bec::NodeId([path CPPString]);
          mOverview->select_node(node);
        }
      }
      mOverview->end_selection_marking();
    }    
  }
}

//--------------------------------------------------------------------------------------------------

- (void) clearSelection
{
  if (mSelectedIndexes)
  {
    [mSelectedIndexes autorelease];
    mSelectedIndexes = nil;
    
    mOverview->begin_selection_marking(); // This call clears the selection in the backend.
    mOverview->end_selection_marking();
  }    
}

//--------------------------------------------------------------------------------------------------

- (void)activateCollectionItem:(id)sender
{
  NSDictionary *item= [[sender owner] representedObject]; 
  bec::NodeId node;
  NSString *path= [item objectForKey: @"path"];
  
  node= bec::NodeId([path CPPString]);
  
  mOverview->activate_node(node);
}

//--------------------------------------------------------------------------------------------------

/**
 * Select the given index without removing other entries from the current selection.
 */
- (void) selectCollectionItem: (id) sender
{
  bec::NodeId node([[[[sender owner] representedObject] objectForKey: @"path"] CPPString]);
  
  [self selectIndex: node.back()];
}

//--------------------------------------------------------------------------------------------------

/**
 * Unselect the given index by removing it from current selection.
 */
- (void) unselectCollectionItem: (id) sender
{
  bec::NodeId node([[[[sender owner] representedObject] objectForKey: @"path"] CPPString]);
  
  [mSelectedIndexes removeIndex: node.back()];
}

//--------------------------------------------------------------------------------------------------

/**
 * Queries the selected indices if they contain the given item.
 */
- (BOOL) isCollectionItemSelected: (id) sender
{
  if ([sender owner])	
  {
    bec::NodeId node([[[[sender owner] representedObject] objectForKey: @"path"] CPPString]);
  
    return [mSelectedIndexes containsIndex: node.back()];
  }
  return NO;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by collection item views if they got renamed.
 */
- (void) itemRenameDidEnd: (id) sender withName: (NSString*) newName
{
  NSDictionary *item= [[sender owner] representedObject];
  bec::NodeId node;
  NSString *path= [item objectForKey: @"path"];
  
  node= bec::NodeId([path CPPString]);
  
  mOverview->set_field(node, wb::OverviewBE::Label, [newName CPPString]);
}

/**
 * Called by a collection item view when it is about to start inline editing.
 * It queries the backend if an item is allowed to be edited.
 */
- (BOOL) canRename: (id) sender
{
  NSDictionary *item= [[sender owner] representedObject];
  bec::NodeId node;
  NSString *path= [item objectForKey: @"path"];
  
  node= bec::NodeId([path CPPString]);
  
  return mOverview->is_editable(node);
}

- (void)setOverviewBE:(wb::OverviewBE*)overview
{
  mOverview= overview;
}

- (NSMutableArray*)items
{
  return mItems;
}


- (BOOL) declareDragDataForItem: (id)sender
                     pasteboard: (NSPasteboard*)pasteboard
{
  NSDictionary *item= [[sender owner] representedObject];
  if (item)
  {
    bec::NodeId node;
    NSString *path= [item objectForKey: @"path"];
    
    node= bec::NodeId([path CPPString]);
    
    std::string type= mOverview->get_node_drag_type(node);
    if (!type.empty())
    {
      grt::ListRef<GrtObject> sel = mOverview->get_selection();
      std::list<db_DatabaseObjectRef> objects;
      
      for (grt::ListRef<GrtObject>::const_iterator obj= sel.begin(); obj != sel.end(); ++obj)
      {
        if ((*obj).is_valid() && db_DatabaseObjectRef::can_wrap(*obj))
        {
          objects.push_back(db_DatabaseObjectRef::cast_from(*obj));
        }
      }
      
      if (!objects.empty())
      {
        std::string text= bec::CatalogHelper::dbobject_list_to_dragdata(objects);
        
        [pasteboard declareTypes: [NSArray arrayWithObject: [NSString stringWithCPPString: type]]
                           owner: self];
        [pasteboard setString: [NSString stringWithCPPString: text] 
                      forType: [NSString stringWithCPPString: type]];
        return YES;
      }
    }
  }
  return NO;
}


- (NSArray*) dropTypesForItem: (id)sender
{
  NSDictionary *item= [[sender owner] representedObject];
  if (item)
  {
    bec::NodeId node;
    NSString *path= [item objectForKey: @"path"];
    
    node= bec::NodeId([path CPPString]);
    
    //if (db_RoutineGroupRef::can_wrap(mOverview->get_grt_value(node)))
    {
      //return [NSArray arrayWithObject: [NSString stringWithUTF8String: WB_DBOBJECT_DRAG_TYPE]];
    }
  }
  return nil;
}


- (BOOL) handleDroppedData: (id)data
                   forItem: (id)item
{
  return NO;
}


@end
