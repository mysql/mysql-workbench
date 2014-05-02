/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import "MFTreeNodeView.h"
#import "NSString_extras.h"
#import "MFView.h"
#import "MFMForms.h"
#include "base/log.h"
#include "base/string_utilities.h"

#include <cstdlib>

#import "MTextImageCell.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_COCOA);

static NSString *RowReorderPasteboardDatatype = @"com.mysql.workbench.row-reorder";

class TreeNodeImpl;

@interface MFTreeNodeImpl : NSObject
{
@public
  MFTreeNodeImpl *mParent;
  MFTreeNodeViewImpl *mTree;
  NSMutableDictionary *mData;
}

- (id)initWithOwner:(MFTreeNodeViewImpl*)owner;
- (MFTreeNodeViewImpl*)treeNodeView;
- (mforms::TreeNodeRef)nodeRef;
- (void)setObject:(id)anObject forKey:(id)aKey;
- (id)objectForKey:(id)key;
- (void)removeObjectForKey:(id)key;
- (NSMutableArray*)createChildrenWithCapacity:(int)count;
- (NSMutableArray*)children;
- (MFTreeNodeImpl*)parent;
- (void)removeFromParent;

- (NSString *)text;

@end

inline TreeNodeImpl *from_ref(mforms::TreeNodeRef node);


@interface TreeNodeDataRef : NSObject
{
  mforms::TreeNodeData *_data;
}
- (id)initWithCPPPointer:(mforms::TreeNodeData*)data;
- (mforms::TreeNodeData*)CPPPointer;
@end

@implementation TreeNodeDataRef

- (id)initWithCPPPointer:(mforms::TreeNodeData*)data
{
  self = [self init];
  if (self)
  {
    self->_data = data;
    if (self->_data)
      self->_data->retain();
  }
  return self;
}

- (mforms::TreeNodeData*)CPPPointer
{
  return _data;
}

- (void)dealloc
{
  if (_data)
    _data->release();
  [super dealloc];
}

@end


class TreeNodeImpl : public mforms::TreeNode
{
  MFTreeNodeImpl *_self;
  int _refcount;

public:
  TreeNodeImpl(MFTreeNodeImpl *owner)
  : _self(owner), _refcount(0)
  {
  }
  
  virtual ~TreeNodeImpl()
  {
  }

  virtual void release()
  {
    _refcount--;
    if (_refcount == 0)
      delete this;
  }
  
  virtual void retain()
  {
    _refcount++;
  }
    
  void invalidate()
  {
    _self = nil;
  }
  
  virtual bool is_valid() const
  {
    return _self != nil;
  }

  virtual bool equals(const TreeNode &other)
  {
    const TreeNodeImpl *oth = dynamic_cast<const TreeNodeImpl*>(&other);
    if (oth)
      return oth->_self == _self;
    return false;
  }
  
  MFTreeNodeImpl *self()
  {
    return _self;
  }
  
  virtual int count() const
  {
    NSMutableArray *children = [_self children];
    return [children count];
  }
  
  virtual mforms::TreeNodeRef insert_child(int index)
  {
    if (is_valid())
    {
      MFTreeNodeImpl *child = [[[MFTreeNodeImpl alloc] initWithOwner: [_self treeNodeView]] autorelease];
      
      mforms::TreeNodeRef node([child nodeRef]);
      
      {
        TreeNodeImpl *nodei = from_ref(node);
        NSMutableArray *children = [_self children];
        if (!children)
          children = [_self createChildrenWithCapacity: 0];
        
        child->mParent = _self;
        
        if (index < 0 || index >= (int)[children count])
          [children addObject: nodei->self()];
        else
          [children insertObject: nodei->self() atIndex: index];
        
        if (![[_self treeNodeView] frozen] && (!get_parent() || is_expanded() || [children count] == 1))
        {
          [[_self treeNodeView] setNeedsReload];
        }
      }
            
      // when node deallocs from stack, ref is returned to 1 and the parent owns that only ref
      // unless the caller keeps a ref to our return value
      return node;
    }
    return mforms::TreeNodeRef();
  }
    
  virtual std::vector<mforms::TreeNodeRef> add_node_collection(const mforms::TreeNodeCollectionSkeleton &nodes, int position = -1)
  {
    std::vector<mforms::TreeNodeRef> result;
    
    if (is_valid())
    {
      id columnKey = [[_self treeNodeView] keyForColumn: 0];

      // Creates an array to hold each equal child for all the parents
      NSMutableArray *added_nodes = [NSMutableArray arrayWithCapacity: (int)nodes.captions.size()];
      
      NSImage *image = get_icon(nodes.icon);
      
      for (std::vector<std::string>::const_iterator v = nodes.captions.begin(); v != nodes.captions.end(); ++v)
      {
        MFTreeNodeImpl *child = [[MFTreeNodeImpl alloc] initWithOwner: [_self treeNodeView]];
        
        [child setObject: [NSString stringWithCPPString: *v]
                  forKey: columnKey];
        
        if (image)
        {
          [child setObject: image
                    forKey: [[[_self treeNodeView] keyForColumn: 0] stringByAppendingString: @"icon"]];
        }
        
        [added_nodes addObject: child];
      }
      
      // Creates the substructure if any
      if (!nodes.children.empty())
        add_children_from_skeletons(added_nodes, nodes.children);

      NSMutableArray *children = [_self children];
      if (!children)
        children = [_self createChildrenWithCapacity: (int)nodes.captions.size()];
      
      NSEnumerator *child_enumerator = [added_nodes objectEnumerator];
      MFTreeNodeImpl *child;

      while(child = [child_enumerator nextObject])
      {
        child->mParent = _self;
        
        if (position < 0 || position >= (int)[children count])
          [children addObject: child];
        else
         [children insertObject: child atIndex: position++];
        
        result.push_back([child nodeRef]);

        [child release];
      }
                                     
      if (![[_self treeNodeView] frozen] && (!get_parent() || is_expanded() || [added_nodes count] == 1))
      {
        [[_self treeNodeView] setNeedsReload];
      }
    }
    
    
    return result;
  }

  void add_children_from_skeletons(NSMutableArray *parents, const std::vector<mforms::TreeNodeSkeleton> &children)
  {
    if (is_valid())
    {
      // Creates the parent children lists
      NSEnumerator *parent_enumerator = [parents objectEnumerator];
      MFTreeNodeImpl *parent;
      while(parent = [parent_enumerator nextObject])
      {
        NSMutableArray *children_array = [parent children];
        if (!children_array)
          children_array = [parent createChildrenWithCapacity: (int)children.size()];
      }
      
      id columnKey = [[_self treeNodeView] keyForColumn: 0];
      
      
      // Now enters the process if creating each children at this level
      std::vector<mforms::TreeNodeSkeleton>::const_iterator it, end = children.end();
      for(it=children.begin(); it != end; it++)
      {
        // Creates an array to hold each equal child for all the parents
        NSMutableArray *added_nodes = [NSMutableArray arrayWithCapacity: [parents count]];
        
        // Gets the data to be set on the current child
        NSImage *image = get_icon((*it).icon);
        NSString *caption = [NSString stringWithCPPString: (*it).caption];
        NSString *tag     = [NSString stringWithCPPString: (*it).tag];
        
        
        // Setups the child for all the parents (same name, icon)
        for(int index = 0; index < [parents count]; index++)
        {
          MFTreeNodeImpl *child = [[[MFTreeNodeImpl alloc] initWithOwner: [_self treeNodeView]] autorelease];
          [child setObject: caption
                    forKey: columnKey];
          
          [child setObject: tag
                    forKey: @"tag"];

          if (image)
          {
            [child setObject: image
                      forKey: [columnKey stringByAppendingString: @"icon"]];
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
        while(parent = [parent_enumerator nextObject])
        {
          child = [child_enumerator nextObject];
          
          child->mParent = parent;
          [[parent children] addObject: child];
          
//          [child release];
        }
      }
    }
  }
  
  virtual void remove_from_parent()
  { 
    if (is_valid())
    {
      NSString *tag = [_self objectForKey: @"tag"];
      if (tag)
        [_self->mTree setNode: nil forTag: [tag UTF8String]];

      if ([_self parent])
        [_self removeFromParent];
      else
        throw std::logic_error("Cannot remove root node");
    }
  }

  virtual void remove_children()
  {
    //XXX
    if (is_valid())
    {
      for (id ch in [_self children])
      {
        NSString *tag = [ch objectForKey: @"tag"];
        if (tag)
          [_self->mTree setNode: nil forTag: [tag UTF8String]];
      }
      [[_self children] removeAllObjects];
    }
  }
  
  virtual mforms::TreeNodeRef get_child(int index) const
  {
    if (is_valid())
    {
      MFTreeNodeImpl *child = [[_self children] objectAtIndex: index];
      if (child)
        return [child nodeRef];
    }
    return mforms::TreeNodeRef();   
  }
  
  virtual mforms::TreeNodeRef get_parent() const
  {
    if (is_valid())
    {
      if ([_self parent])
        return [[_self parent] nodeRef];
    }
    return mforms::TreeNodeRef();
  }
  
  virtual void expand()
  {
    if (!is_expanded())
    {
      _self.treeNodeView.backend->expand_toggle(_self.nodeRef, true);
      mforms::TreeNodeRef parent(get_parent());

      if (parent)
        parent->expand();

      [[[_self treeNodeView] outlineView] performSelector: @selector(expandItem:)
                                               withObject: _self
                                               afterDelay: 0.0
                                               inModes: [NSArray arrayWithObjects: NSModalPanelRunLoopMode, NSDefaultRunLoopMode, nil]];
    }
  }

  /**
   * If there are already subnodes then we can obviously expand.
   * But if there are none (yet) we have to ask the treeview (which in turn might query
   * its data model).
   */
  virtual bool can_expand()
  {
    if (count() > 0)
      return true;

    return _self.treeNodeView.backend->can_expand(_self.nodeRef);
  }

  virtual void collapse()
  {
    [[[_self treeNodeView] outlineView] collapseItem: _self];
  }
                
  virtual bool is_expanded()
  {
    return [[[_self treeNodeView] outlineView] isItemExpanded: _self];
  }
  
  virtual void set_tag(const std::string &tag)
  {
    NSString *tstr = [_self objectForKey: @"tag"];

    if (tstr)
      [_self->mTree setNode: nil forTag: [tstr UTF8String]];
    [_self->mTree setNode: _self forTag: tag];
    [_self setObject: [NSString stringWithCPPString: tag]
              forKey: @"tag"];
  }
  
  virtual std::string get_tag() const
  {
    const char *s = [[_self objectForKey: @"tag"] UTF8String];
    return s ? s : "";
  }

  virtual void set_data(mforms::TreeNodeData *data)
  {
    [_self setObject: [[[TreeNodeDataRef alloc] initWithCPPPointer: data] autorelease]
              forKey: @"data"];
  }
  
  virtual mforms::TreeNodeData *get_data() const
  {
    return [[_self objectForKey: @"data"] CPPPointer];
  }

  NSImage* get_icon(const std::string &icon_path)
  {
    NSImage *image = nil;
    if (icon_path == "folder")
    {
      static NSImage *folderIcon= [[[[NSWorkspace sharedWorkspace] iconForFile:@"/usr"] copy] retain];
      image = folderIcon;
    }
    else
      image = [[_self treeNodeView] iconForFile: [NSString stringWithCPPString: icon_path]];
    
    if (image)
    {
      NSSize size = [image size];
      float rowHeight = [[[_self treeNodeView] outlineView] rowHeight];
      if (size.height > rowHeight)
      {
        rowHeight -= 2;
        size.width *= rowHeight / size.height;
        size.height = rowHeight;
        [image setScalesWhenResized: YES];
        [image setSize: size];
      }
    }
    return image;
  }
  
  virtual void set_icon_path(int column, const std::string &icon)
  {
    NSImage *image = get_icon(icon);

    if (image)
      [_self setObject: image
                forKey: [[[_self treeNodeView] keyForColumn: column] stringByAppendingString: @"icon"]];
    else
      [_self removeObjectForKey: [[[_self treeNodeView] keyForColumn: column] stringByAppendingString: @"icon"]];
  }
  
  virtual void set_attributes(int column, const mforms::TreeNodeTextAttributes& attrs)
  {
    if (attrs.bold || attrs.italic)
    {
      NSString *attrstr = [NSString stringWithFormat: @"%s%s", attrs.bold ? "b" : "", attrs.italic ? "i":""];
      [_self setObject: attrstr
                forKey: [[[_self treeNodeView] keyForColumn: column] stringByAppendingString: @"attrs"]];
    }
    else
      [_self removeObjectForKey: [[[_self treeNodeView] keyForColumn: column] stringByAppendingString: @"attrs"]];
  }

  virtual void set_string(int column, const std::string &value)
  {
    [_self setObject: [NSString stringWithCPPString: value]
              forKey: [[_self treeNodeView] keyForColumn: column]];
  }

  
  virtual void set_int(int column, int value)
  {
    id key = [[_self treeNodeView] keyForColumn: column];
      
    [_self setObject: [NSNumber numberWithInt: value]
              forKey: key];
  }
  
  virtual void set_long(int column, boost::int64_t value)
  {
    [_self setObject: [NSNumber numberWithLongLong: value]
              forKey: [[_self treeNodeView] keyForColumn: column]];
  }

  virtual void set_float(int column, double value)
  {
    id key = [[_self treeNodeView] keyForColumn: column];

    [_self setObject: [NSNumber numberWithDouble: value]
              forKey: key];
  }

  virtual void set_bool(int column, bool value)
  {
    [_self setObject: [NSNumber numberWithBool: value]
              forKey: [[_self treeNodeView] keyForColumn: column]];
  }
  
  virtual std::string get_string(int column) const
  {
    NSString *s = [_self objectForKey: [[_self treeNodeView] keyForColumn: column]];
    if (s)
      return [s UTF8String];
//    NSLog(@"Invalid column %i for TreeNode::grt_string()", column);
    return "";
  }

  virtual int get_int(int column) const
  {
    NSNumber *n = [_self objectForKey: [[_self treeNodeView] keyForColumn: column]];
    if (n)
      return [n intValue];
    return 0;
  }

  virtual boost::int64_t get_long(int column) const
  {
    NSNumber *n = [_self objectForKey: [[_self treeNodeView] keyForColumn: column]];
    if (n)
      return [n longLongValue];
    return 0;
  }

  virtual double get_float(int column) const
  {
    NSNumber *n = [_self objectForKey: [[_self treeNodeView] keyForColumn: column]];
    if (n)
      return [n doubleValue];
    return 0.0;
  }

  virtual bool get_bool(int column) const
  {
    NSNumber *n = [_self objectForKey: [[_self treeNodeView] keyForColumn: column]];
    if (n)
      return [n boolValue];
    return false;    
  }
};



inline TreeNodeImpl *from_ref(mforms::TreeNodeRef node)
{
  return dynamic_cast<TreeNodeImpl*>(node.ptr());
}



@implementation MFTreeNodeImpl

- (id)initWithOwner:(MFTreeNodeViewImpl*)owner
{
  self = [super init];
  if (self)
  {
    mData = [[NSMutableDictionary alloc] init];
    mTree = owner;
  }
  return self;
}

- (void)dealloc
{
  [mData release];
  [super dealloc];
}

- (MFTreeNodeViewImpl*)treeNodeView
{
  return mTree;
}

- (void)setObject:(id)anObject forKey:(id)aKey
{
  [mData setObject: anObject forKey: aKey];
  
  [[mTree outlineView] setNeedsDisplay: YES];
}

- (void)removeObjectForKey:(id)aKey
{
  [mData removeObjectForKey: aKey];
}

- (id)objectForKey: (id)key
{
  return [mData objectForKey: key];
}

- (id)valueForKey: (id)key // for KVC
{
  return [mData objectForKey: key];
}

- (NSMutableArray*)createChildrenWithCapacity:(int)count
{
  NSMutableArray *children = count > 0 ? [NSMutableArray arrayWithCapacity:count] : [NSMutableArray array];
  [mData setObject: children forKey: @"children"];
  return children;
}

- (NSMutableArray*)children
{
  return [mData objectForKey: @"children"];
}

- (MFTreeNodeImpl*)parent
{
  return mParent;
}


- (void)removeFromParent
{
  NSMutableArray *children = [mParent children];
  if (children)
  {
    // if the node being removed is selected, unselect it, so that the selection isn't passed to a different node
    NSIndexSet *rows = [[[self treeNodeView] outlineView] selectedRowIndexes];
    if (rows && [rows count] == 1)
    {
      if ([rows containsIndex: [[[self treeNodeView] outlineView] rowForItem: self]])
        [[[self treeNodeView] outlineView] deselectAll: nil];
    }

    [[self retain] autorelease];
    [children removeObject: self];
    mParent = nil;
   
    if (![[self treeNodeView] frozen])
      [[[self treeNodeView] outlineView] reloadData];
  }
}

- (mforms::TreeNodeRef)nodeRef
{
  if (self == nil)
    return mforms::TreeNodeRef();
  return mforms::TreeNodeRef(new TreeNodeImpl(self));
}

/**
 * Returns a concatenated string of all column values, separated by tab.
 */
- (NSString *)text
{
  NSString *result = @"";
  NSArray *columns = mTree.outlineView.tableColumns;
  for (NSTableColumn *column in columns)
  {
    id data = [mData objectForKey: column.identifier];
    if ([data isKindOfClass: NSString.class] && [data length] > 0)
    {
      if (result.length > 0)
        result = [result stringByAppendingString: @"\t"];
      result = [result stringByAppendingString: data];
    }
  }
  return result;
}

@end

//--------------------------------------------------------------------------------------------------

static NSImage *ascendingSortIndicator = nil;
static NSImage *descendingSortIndicator = nil;

@interface TreeNodeViewOutlineView : NSOutlineView
{
  mforms::TreeNodeView *mOwner;
  NSTrackingArea *mTrackingArea;
}
@end

@implementation TreeNodeViewOutlineView

- (id)initWithFrame: (NSRect)frame owner: (mforms::TreeNodeView *)treeView
{
  self = [super initWithFrame: frame];
  if (self != nil)
  {
    mOwner = treeView;
  }
  return self;
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
  NSInteger row = [self rowAtPoint: [self convertPoint: [theEvent locationInWindow] fromView: nil]];
  if (row >= 0)
  {
    if (![self isRowSelected: row])
      [self selectRowIndexes: [NSIndexSet indexSetWithIndex: row]
        byExtendingSelection: NO];
  }
  [super rightMouseDown: theEvent];
}

//--------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING_NO_RIGHT_BUTTON(self) // Add handling for mouse events.

//--------------------------------------------------------------------------------------------------

- (NSMenu*)menuForEvent:(NSEvent *)event
{
  mforms::ContextMenu *menu = mOwner->get_context_menu();
  if (menu)
    return menu->get_data();
  return nil;
}

@end

// ------------------------------------------------------------------------------------------------

@implementation MFTreeNodeViewImpl


- (id)initWithObject:(::mforms::TreeNodeView*)aTreeView
{
  if (!ascendingSortIndicator)
  {
    ascendingSortIndicator= [[NSImage imageNamed:@"NSAscendingSortIndicator"] retain];
    descendingSortIndicator= [[NSImage imageNamed:@"NSDescendingSortIndicator"] retain];
  }
  
  self= [super initWithFrame:NSMakeRect(0, 0, 40, 40)];
  if (self)
  { 
    mOwner= aTreeView;
    mOwner->set_data(self);
    
    mColumnKeys = [[NSMutableArray alloc] init];
    
    mIconCache = [[NSMutableDictionary alloc] init];
    
    [self setHasHorizontalScroller:YES];
    [self setHasVerticalScroller:YES];
    [self setAutohidesScrollers:YES];
    
    mAttributedFonts = [[NSMutableDictionary alloc] init];
    [mAttributedFonts setObject: [NSFont systemFontOfSize: [NSFont systemFontSize]]
                         forKey: @""];
    NSRect rect;
    rect.origin= NSMakePoint(0, 0);
    rect.size= [NSScrollView contentSizeForFrameSize:[self frame].size hasHorizontalScroller:YES hasVerticalScroller:YES
                                          borderType:NSBezelBorder];

    // TODO: might be worth making this not auto released but release it manually so we can control when it goes.
    mOutline= [[[TreeNodeViewOutlineView alloc] initWithFrame: rect owner: mOwner] autorelease];

    [self setDocumentView: mOutline];
    [mOutline setColumnAutoresizingStyle: NSTableViewLastColumnOnlyAutoresizingStyle];

    [mOutline setAllowsEmptySelection: YES];

    mRootNode= [[MFTreeNodeImpl alloc] initWithOwner: self];
    [mOutline setDataSource: self];
    [mOutline setDelegate: self];
    [mOutline setDoubleAction: @selector(rowDoubleClicked:)];
    [mOutline setTarget: self];
    
    mSortColumn= -1;
  }
  return self;
}


- (void)enableIndexing
{
  mTagIndexEnabled = YES;
}


- (void) dealloc
{
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  // Setting nil is necessary since there are still datasouce and delegate messages comming in after dealloc.
  // Might become unnecessary if we don't auto release mOutline.
  [mOutline setDataSource: nil];
  [mOutline setDelegate: nil];
  mTagMap.clear();
  [mAttributedFonts release];
  [mColumnKeys release];
  [mIconCache release];
  [mRootNode release];
  [super dealloc];
}

- (void)reloadTreeData
{
  if (mPendingReload)
  {
    mPendingReload = NO;
    [mOutline reloadData];
  }
}

- (void)setNeedsReload
{  
  if (!mPendingReload)
    [self performSelectorOnMainThread: @selector(reloadTreeData) 
                           withObject: nil
                        waitUntilDone: NO];
  mPendingReload = YES;
}

- (BOOL)isPendingReload
{
  return mPendingReload;
}

- (NSOutlineView*)outlineView
{
  return mOutline;
}


- (void)setEnabled:(BOOL)flag
{
  [mOutline setEnabled: flag];
}


- (NSSize)minimumSize
{
  return NSMakeSize(40, 50);
}


- (NSInteger)addColumnWithTitle:(NSString*)title type:(mforms::TreeColumnType)type editable:(BOOL)editable
                          width:(int)width
{
  int idx= [[mOutline tableColumns] count];
  NSString *columnKey = [NSString stringWithFormat:@"%i", idx];
  NSTableColumn *column= [[[NSTableColumn alloc] initWithIdentifier: columnKey] autorelease];

  [mColumnKeys addObject: columnKey];
  
  [mOutline addTableColumn: column];
  [[column headerCell] setTitle: title];
  [column setResizingMask: NSTableColumnUserResizingMask];
  switch (type)
  {
    case mforms::CheckColumnType:
    {
      NSButtonCell *cell = [[[NSButtonCell alloc] init] autorelease];
      [column setDataCell: cell];
      [cell setTitle: @""];
      [cell setButtonType: NSSwitchButton];
      break;
    }
    case mforms::TriCheckColumnType:
    {
      NSButtonCell *cell = [[[NSButtonCell alloc] init] autorelease];
      [column setDataCell: cell];
      [cell setTitle: @""];
      [cell setButtonType: NSSwitchButton];
      [cell setAllowsMixedState: YES];
      break;
    }
    case mforms::StringColumnType:
    case mforms::StringLTColumnType:
      break;
    case mforms::NumberWithUnitColumnType:
      [[column dataCell] setAlignment: NSRightTextAlignment];
      break;
    case mforms::IconColumnType:
    {
      MTextImageCell *cell = [[[MTextImageCell alloc] init] autorelease];
      [column setDataCell: cell];
      break;
    }
    case mforms::FloatColumnType:
    {
      NSNumberFormatter *nf = [[[NSNumberFormatter alloc] init] autorelease];
      [nf setNumberStyle: kCFNumberFormatterDecimalStyle];
      [[column dataCell] setAlignment: NSRightTextAlignment];
      [[column dataCell] setFormatter: nf];
      break;
    }
    case mforms::IntegerColumnType:
    case mforms::LongIntegerColumnType:
    {
      NSNumberFormatter *nf = [[[NSNumberFormatter alloc] init] autorelease];
      [nf setNumberStyle: NSNumberFormatterNoStyle];
      [[column dataCell] setAlignment: NSRightTextAlignment];
      [[column dataCell] setFormatter: nf];
      break;
    }
  }
  
  [[column dataCell] setEditable: editable];
  [[column dataCell] setTruncatesLastVisibleLine: YES];
  if (type == mforms::StringLTColumnType)
    [[column dataCell] setLineBreakMode: NSLineBreakByTruncatingHead];
  if (idx == 0) // && !mFlatTable)
    [mOutline setOutlineTableColumn: column];
  if (width >= 0)
    [column setWidth: width];
  if (mSmallFont)
    [[column dataCell] setFont: [NSFont systemFontOfSize: [NSFont smallSystemFontSize]]];

  return idx;
}


- (NSString*)keyForColumn:(int)column
{
  if (column >= [mColumnKeys count])
      throw std::invalid_argument(base::strfmt("invalid column %i in TreeNodeView, last column is %s", column,
                                               [[[[[mOutline tableColumns] lastObject] headerCell] stringValue] UTF8String]));
    
  return [mColumnKeys objectAtIndex: column];
}

- (NSImage*)iconForFile: (NSString*)path
{
  if (path && [path length] > 0)
  {
    NSImage *image = nil;
    image = [mIconCache objectForKey: path];
    if (!image)
    {
      std::string full_path= g_file_test([path UTF8String], G_FILE_TEST_EXISTS) ? [path UTF8String] : mforms::App::get()->get_resource_path([path UTF8String]);
      image = [[[NSImage alloc] initWithContentsOfFile: wrap_nsstring(full_path)] autorelease];
      if (image && [image isValid])
        [mIconCache setObject: image forKey: path];
      else
        image = nil;
    }
    if (!image)
      NSLog(@"Can't load icon %@", path);
    return image;
  }
  return nil;
}


- (void) outlineView: (NSOutlineView *) outlineView
didClickTableColumn: (NSTableColumn *) tableColumn
{
  if (mSortColumnEnabled)
  {
    int column = [[tableColumn identifier] intValue];
    BOOL ascending;
    
    if ([outlineView indicatorImageInTableColumn: tableColumn] == ascendingSortIndicator)
    {
      ascending = NO;
      [outlineView setIndicatorImage: descendingSortIndicator inTableColumn: tableColumn];
    }
    else 
    {
      ascending = YES;
      [outlineView setIndicatorImage: ascendingSortIndicator inTableColumn: tableColumn];      
    }
    [outlineView setHighlightedTableColumn: tableColumn];
    if (mSortColumn >= 0 && mSortColumn < [outlineView numberOfColumns] && mSortColumn != column)
      [outlineView setIndicatorImage: nil inTableColumn: [[outlineView tableColumns] objectAtIndex: mSortColumn]];
    mSortColumn = column;
    
    NSSortDescriptor *sd;

    if ([[tableColumn dataCell] alignment] == NSRightTextAlignment)
    {
      if ([[tableColumn dataCell] formatter])
        sd = [NSSortDescriptor sortDescriptorWithKey: [tableColumn identifier]
                                           ascending: ascending
                                          comparator: ^(id o1, id o2) {
                                            double d = [o1 doubleValue] - [o2 doubleValue];
                                            if (d > 0)
                                              return (NSComparisonResult)NSOrderedAscending;
                                            else if (d < 0)
                                              return (NSComparisonResult)NSOrderedDescending;
                                            else
                                              return (NSComparisonResult)NSOrderedSame;
                                          }];
      else
        sd = [NSSortDescriptor sortDescriptorWithKey: [tableColumn identifier]
                                           ascending: ascending
                                          comparator: ^(id o1, id o2) {
                                            double d = mforms::TreeNodeView::parse_string_with_unit([o1 UTF8String]) - mforms::TreeNodeView::parse_string_with_unit([o2 UTF8String]);
                                            if (d > 0)
                                              return (NSComparisonResult)NSOrderedAscending;
                                            else if (d < 0)
                                              return (NSComparisonResult)NSOrderedDescending;
                                            else
                                              return (NSComparisonResult)NSOrderedSame;
                                          }];
    }
    else
      sd = [NSSortDescriptor sortDescriptorWithKey: [tableColumn identifier]
                                         ascending: ascending];
    [outlineView setSortDescriptors: [NSArray arrayWithObject: sd]];
  }
}


static void sortChildrenOfNode(MFTreeNodeImpl *node, 
                                     NSArray *sortDescriptors)
{
  if (sortDescriptors && node)
  {
    NSMutableArray *array = [node children];
    if (array)
    {
      [array sortUsingDescriptors: sortDescriptors];
      for (MFTreeNodeImpl *object in array)
        sortChildrenOfNode(object, sortDescriptors);
    }
  }
}

- (void)setViewFlags: (NSInteger)tag
{
  mOutline.viewFlags = tag;
}

- (NSInteger)viewFlags
{
  return mOutline.viewFlags;
}

- (mforms::TreeNodeView *)backend
{
  return mOwner;
}

#pragma mark - NSOutlineView delegate

- (void)outlineView:(NSOutlineView *)outlineView
sortDescriptorsDidChange:(NSArray *)oldDescriptors
{
  int selectedRow = [outlineView selectedRow];
  id selectedItem = [outlineView itemAtRow: selectedRow];
  
  sortChildrenOfNode(mRootNode, [outlineView sortDescriptors]);

  [outlineView reloadData];
  
  if (selectedRow >= 0)
  {
    [outlineView selectRowIndexes: [NSIndexSet indexSetWithIndex: [outlineView rowForItem: selectedItem]]
             byExtendingSelection: NO];
  }
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item
{
  if (!item)
    item= mRootNode;

  NSArray *children = [item children];
  if (children && index < [children count])
    return [[item children] objectAtIndex: index];
  return nil;
}


- (BOOL)outlineView: (NSOutlineView *)outlineView isItemExpandable: (id)item
{
  if (item)
  {
    MFTreeNodeImpl *node = (MFTreeNodeImpl *)item;
    return [node nodeRef]->can_expand();
  }
  return YES;
}


- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
  NSInteger c;
  if (!item)
    c = [[mRootNode children] count];
  else
    c = [[item children] count];
  return c;
}

- (void)outlineViewItemDidExpand: (NSNotification *)notification
{
  // Tell the backend now would be a good time to add child nodes if not yet done.
  try
  {
    MFTreeNodeImpl *node = [[notification userInfo] objectForKey: @"NSObject"];
    if (node)
      mOwner->expand_toggle([node nodeRef], true);
  }
  catch (std::exception &exc)
  {
    log_error("Exception in expand_toggle(true) handler\n");
  }
}

- (void)outlineViewItemDidCollapse:(NSNotification *)notification
{
  try
  {
    MFTreeNodeImpl *node = [[notification userInfo] objectForKey: @"NSObject"];
    if (node)
      mOwner->expand_toggle([node nodeRef], false);     
  }
  catch (std::exception &exc)
  {
    log_error("Exception in expand_toggle(false) handler\n");
  }  
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
  return [item objectForKey: [tableColumn identifier]];
}

- (void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
  if ([cell isKindOfClass: [MTextImageCell class]])
    [cell setImage: [item objectForKey: [[tableColumn identifier] stringByAppendingString: @"icon"]]];
  
  NSString *attributes = [item objectForKey: [[tableColumn identifier] stringByAppendingString: @"attrs"]];
  if (attributes)
  {
    NSFont *font = [mAttributedFonts objectForKey: attributes];
    if (!font)
    {
      int traits = 0;
      if ([attributes rangeOfString: @"b"].length > 0)
        traits |= NSBoldFontMask;
      if ([attributes rangeOfString: @"i"].length > 0)
        traits |= NSItalicFontMask;

      font = [[NSFontManager sharedFontManager] convertFont: [[[[tableColumn dataCell] font] copy] autorelease]
                                                toHaveTrait: traits];
      [mAttributedFonts setObject: font forKey: attributes];
    }
    [cell setFont: font];
  }
  else
    [cell setFont: [mAttributedFonts objectForKey: @""]];
}

- (void)outlineView:(NSOutlineView *)outlineView setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
  id value = [object respondsToSelector:@selector(stringValue)] ? [object stringValue] : object;
  
  if ([[tableColumn dataCell] isKindOfClass: [NSButtonCell class]] &&
      [[tableColumn dataCell] allowsMixedState])
  {
    // if new state is -1, force it to 1
    if ([value isEqualToString: @"-1"])
      value = @"1";
  }

  if (mOwner->cell_edited([item nodeRef], atoi([[tableColumn identifier] UTF8String]), [[value description] UTF8String]))
    [item setObject:value forKey: [tableColumn identifier]];
}


- (void)outlineViewSelectionDidChange:(NSNotification *)notification
{
  if (![self frozen])
    mOwner->changed();
}

- (void)rowDoubleClicked:(id)sender
{
  NSInteger row = [mOutline clickedRow];
  if (row >= 0)
  {
    id item = [mOutline itemAtRow: [mOutline clickedRow]];
    mOwner->node_activated([item nodeRef], [mOutline clickedColumn]);
  }
}

- (void)outlineViewColumnDidResize:(NSNotification *)notification
{
  mOwner->column_resized([[mOutline headerView] resizedColumn]);
}

#pragma mark - Drag'n drop

- (BOOL)outlineView: (NSOutlineView *)outlineView
         writeItems: (NSArray *)items
       toPasteboard: (NSPasteboard *)pboard
{
  if (!mCanBeDragSource)
    return NO;

  // First write a special datatype for row reordering if enabled. This is the preferred type in that case.
  if (mCanReorderRows && [items count] == 1)
  {
    NSNumber *number = @([outlineView rowForItem: items.lastObject]);
    NSData *data = [NSKeyedArchiver archivedDataWithRootObject: @[number]];
    [pboard declareTypes: @[RowReorderPasteboardDatatype] owner: nil];
    [pboard setData: data forType: RowReorderPasteboardDatatype];
  }

  mforms::DragDetails details;
  void *data = NULL;
  std::string format;
  mDraggedNodes = items; // Will be queried when the backend wants to have the current selection.
  if (mOwner->get_drag_data(details, &data, format))
    [pboard writeNativeData: data typeAsChar: format.c_str()];
  else
  {
    [pboard addTypes: @[NSPasteboardTypeString] owner: self];

    // Further add string expressions for the selected nodes so we can drag their text to other controls.
    // TODO: move this to the schema tree. It's too specific to be here (same for other platforms).
    NSString *text = @"";
    for (MFTreeNodeImpl *node in items)
    {
      if (text.length > 0)
        text = [text stringByAppendingString: @", "];
      text = [text stringByAppendingString: node.text];
    }
    [pboard setString: text forType: NSPasteboardTypeString];
  }

  mDraggedNodes = nil; // Would probably better to reset that in draggingSessionEndedAt but
                       // that is not available before 10.7.
  return YES;
}

- (NSDragOperation)outlineView: (NSOutlineView *)outlineView
                  validateDrop: (id <NSDraggingInfo>)info
                  proposedItem: (id)item
            proposedChildIndex: (NSInteger)index
{
  NSDragOperation op = NSDragOperationNone;

  if (mCanReorderRows)
  {
    NSPasteboard *pb = [info draggingPasteboard];
    NSData *data = [pb dataForType: RowReorderPasteboardDatatype];
    NSAssert((data != nil), @"Drag flavour was not found.");

    NSArray *indexes = [NSKeyedUnarchiver unarchiveObjectWithData: data];
    NSInteger oldIndex = [indexes.lastObject integerValue];

    // Right now only allow reordering flat lists, not trees.
    if (oldIndex != index && item == nil)
    {
      item= mRootNode;
      if (index < (int)[[item children] count])
      {
        [outlineView setDropItem: nil dropChildIndex: index];
        op = NSDragOperationMove;
      }
    }
  }

  return op;
}

- (BOOL)outlineView: (NSOutlineView *)outlineView
         acceptDrop: (id <NSDraggingInfo>)info
               item: (id)item
         childIndex: (NSInteger)index
{
  NSPasteboard* pb = [info draggingPasteboard];
  NSData *data = [pb dataForType: RowReorderPasteboardDatatype];
  NSAssert((data != nil), @"Drag flavour was not found.");

  NSArray *indexes = [NSKeyedUnarchiver unarchiveObjectWithData: data];
  NSInteger oldIndex = [indexes.lastObject integerValue];

  if (index != oldIndex)
  {
    if (item == nil)
      item = mRootNode;
    NSMutableArray *list = [item objectForKey: @"children"];
    id draggedItem = [list objectAtIndex: oldIndex];

    [list removeObjectAtIndex: oldIndex];
    if (index < 0)
      [list addObject: draggedItem];
    else
      if (index < oldIndex)
        [list insertObject: draggedItem atIndex: index];
      else
        [list insertObject: draggedItem atIndex: index-1];

    [outlineView reloadData];
  }
  return YES;
}

- (void)setUseSmallFont: (BOOL)flag
{
  mSmallFont = flag;
  [mAttributedFonts setObject: [NSFont systemFontOfSize: [NSFont smallSystemFontSize]]
                       forKey: @""];
  if (flag)
    [mOutline setRowHeight: [NSFont smallSystemFontSize]+3];
}

- (BOOL)frozen
{
  return mFreezeCount > 0;
}

- (void)setBackgroundColor:(NSColor *)color
{
  [super setBackgroundColor: color];
  [mOutline setBackgroundColor: color];
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize
{
  if (!mOwner->is_destroying())
  {
    [super resizeSubviewsWithOldSize: oldSize];
    NSArray *columns = [mOutline tableColumns];
    if ([columns count] == 1)
    {
      float w = NSWidth([mOutline frame]);
      if ([[columns lastObject] width] < w)
        [[columns lastObject] setWidth: w];
    }
  }
}

- (void)setNode: (MFTreeNodeImpl*)node forTag: (const std::string&)tag
{
  if (!tag.empty() && mTagIndexEnabled)
  {
    if (!node)
      mTagMap.erase(tag);
    else
      mTagMap[tag] = node; // reference only
  }
}

//--------------------------------------------------------------------------------------------------

static bool treeview_create(mforms::TreeNodeView *self, mforms::TreeOptions options)
{
  MFTreeNodeViewImpl *tree= [[[MFTreeNodeViewImpl alloc] initWithObject: self] autorelease];

  if (options & mforms::TreeAllowReorderRows || options & mforms::TreeCanBeDragSource)
  {
    tree->mCanBeDragSource = YES;

    [tree->mOutline setDraggingSourceOperationMask: NSDragOperationCopy forLocal: NO];

    // For row-reordering register also as drag target.
    if (options & mforms::TreeAllowReorderRows)
    {
      tree->mCanReorderRows = YES;
      [tree->mOutline registerForDraggedTypes: @[RowReorderPasteboardDatatype]];
      [tree->mOutline setDraggingSourceOperationMask: NSDragOperationMove | NSDragOperationCopy forLocal: YES];
    }
    else
      [tree->mOutline setDraggingSourceOperationMask: NSDragOperationCopy forLocal: YES];
  }
  
  if (options & mforms::TreeNoHeader)
    [tree->mOutline setHeaderView: nil];
  int mask = 0;
  if (options & mforms::TreeShowColumnLines)
    mask |= NSTableViewSolidVerticalGridLineMask;
  if (options & mforms::TreeShowRowLines)
    mask |= NSTableViewSolidHorizontalGridLineMask;
  if (options & mforms::TreeNoBorder)
    [tree setBorderType: NSNoBorder];
  else
    [tree setBorderType: NSBezelBorder];
  if (options & mforms::TreeNoHeader)
    [tree->mOutline setHeaderView:nil];
  if (options & mforms::TreeSizeSmall)
  {
    [tree setUseSmallFont: YES];
    [tree setAutohidesScrollers: YES];
  }
  if (options & mforms::TreeSidebar)
  {
    // maintain the row height
    float rowHeight = [tree->mOutline rowHeight];
    [tree setUseSmallFont: YES];
    [tree->mOutline setRowHeight: rowHeight];
    [tree->mOutline setFocusRingType: NSFocusRingTypeNone];
    [tree setAutohidesScrollers: YES];
  }
  if (options & mforms::TreeIndexOnTag)
    [tree enableIndexing];
  [tree->mOutline setGridStyleMask: mask];

  if (options & mforms::TreeAltRowColors)
    [tree->mOutline setUsesAlternatingRowBackgroundColors: YES];

  tree->mFlatTable = (options & mforms::TreeFlatList) != 0;
  if (tree->mFlatTable)
    [tree->mOutline setIndentationPerLevel: 0];
  
  return true;
}

static int treeview_add_column(mforms::TreeNodeView *self, mforms::TreeColumnType type, const std::string &name, int width, bool editable, bool attributed)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
  {
    return [tree addColumnWithTitle:wrap_nsstring(name) type:type editable:editable width:width];
  }
  return -1;
}


static void treeview_end_columns(mforms::TreeNodeView *self)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
  {
    [tree->mOutline sizeLastColumnToFit];
  }
}


static void treeview_clear(mforms::TreeNodeView *self)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
  {
    [[tree->mRootNode children] removeAllObjects];
    [tree->mOutline reloadData];
  }
}

static mforms::TreeNodeRef treeview_root_node(mforms::TreeNodeView *self)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
    return [tree->mRootNode nodeRef];
  return mforms::TreeNodeRef();
}

static mforms::TreeNodeRef treeview_get_selected(mforms::TreeNodeView *self)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
  {
    int row = [tree->mOutline selectedRow];
    if (row >= 0)
      return [[tree->mOutline itemAtRow: row] nodeRef];
  }
  return mforms::TreeNodeRef();
}

static std::list<mforms::TreeNodeRef> treeview_get_selection(mforms::TreeNodeView *self)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  std::list<mforms::TreeNodeRef> selection;
  if (tree)
  {
    NSArray *draggedNodes = tree->mDraggedNodes;
    if (draggedNodes.count > 0)
    {
      for (MFTreeNodeImpl *item in draggedNodes)
        selection.push_back(item.nodeRef);
    }
    else
    {
      NSIndexSet *indexes = [tree->mOutline selectedRowIndexes];

      if ([indexes count] > 0)
      {
        for (int i = [indexes firstIndex]; i <= (int)[indexes lastIndex]; i = [indexes indexGreaterThanIndex: i])
        {
          MFTreeNodeImpl *node = [tree->mOutline itemAtRow: i];
          if (node)
            selection.push_back([node nodeRef]);
        }
      }
    }
  }
  return selection;
}

static int treeview_row_for_node(mforms::TreeNodeView *self, mforms::TreeNodeRef node);

static void treeview_set_selected(mforms::TreeNodeView *self, mforms::TreeNodeRef node, bool flag)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree && node)
  {
    if ([tree frozen])
      NSLog(@"WARNING: Selecting items on a frozen TreeNodeView will probably not work");
    
    if (flag)
    {
      [tree reloadTreeData];
      [tree->mOutline selectRowIndexes: [NSIndexSet indexSetWithIndex: treeview_row_for_node(self, node)]
                  byExtendingSelection: YES];
    }
    else
      [tree->mOutline deselectRow: treeview_row_for_node(self, node)];
  }
}


static void treeview_set_row_height(mforms::TreeNodeView *self, int height)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
    [tree->mOutline setRowHeight: height];
}


static void treeview_clear_selection(mforms::TreeNodeView *self)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
    [tree->mOutline deselectAll: nil];
}

static void treeview_allow_sorting(mforms::TreeNodeView *self, bool flag)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
  {
    tree->mSortColumnEnabled = flag;
    if (!flag)
    {
      for (NSTableColumn *column in [tree->mOutline tableColumns])
        [tree->mOutline setIndicatorImage:nil inTableColumn: column];
      [tree->mOutline setHighlightedTableColumn: nil];
      tree->mSortColumn = -1;
    }
  }
}


static void treeview_freeze_refresh(mforms::TreeNodeView *self, bool flag)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
  {
    if (flag)
      tree->mFreezeCount++;
    else 
    {
      tree->mFreezeCount--;
      if (tree->mFreezeCount == 0)
      {
        // remember and restore row selection
        int row = [tree->mOutline selectedRow];
        id item = row >= 0 ? [tree->mOutline itemAtRow: row] : nil;
        
        sortChildrenOfNode(tree->mRootNode, [tree->mOutline sortDescriptors]);
        [tree->mOutline reloadData];
        
        if (item)
        {
          row = [tree->mOutline rowForItem: item];
          if (row >= 0)
            [tree->mOutline selectRowIndexes: [NSIndexSet indexSetWithIndex: row]
                        byExtendingSelection: NO];
        }
      }
    }
  }
}


static void treeview_set_selection_mode(mforms::TreeNodeView *self, mforms::TreeSelectionMode mode)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
  {
    switch (mode)
    {
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


static mforms::TreeSelectionMode treeview_get_selection_mode(mforms::TreeNodeView *self)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
  {
    mforms::TreeSelectionMode mode;
    if ([tree->mOutline allowsMultipleSelection])
      return mforms::TreeSelectMultiple;
    else
      return mforms::TreeSelectSingle;
    return mode;
  }
  return mforms::TreeSelectSingle;
}


static int count_rows_in_node(NSOutlineView *outline, MFTreeNodeImpl *node)
{
  if ([outline isItemExpanded: node])
  {
    int count = [[node children] count];
    for (int i = 0, c = count; i < c; i++)
    {
      MFTreeNodeImpl *child = [[node children] objectAtIndex: i];
      if (child)
        count += count_rows_in_node(outline, child);
    }
    return count;
  }
  return 0;
}


static int row_for_node(NSOutlineView *outline, MFTreeNodeImpl *node)
{
  MFTreeNodeImpl *parent = [node parent];
  int node_index = [[parent children] indexOfObject: node];
  int row = node_index;

  if (parent)
  {
    for (int i = 0; i < node_index; i++)
      row += count_rows_in_node(outline, [[parent children] objectAtIndex: i]);

    row += row_for_node(outline, parent);
  }
  return row;
}


static int treeview_row_for_node(mforms::TreeNodeView *self, mforms::TreeNodeRef node)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  TreeNodeImpl *nodei = from_ref(node);
  if (tree && nodei)
  {
    if ([tree frozen])
    {
      if (tree->mFlatTable)
        return [[tree->mRootNode children] indexOfObject: nodei->self()];
      else
        return row_for_node(tree->mOutline, nodei->self());
    }
    else
      return [tree->mOutline rowForItem: nodei->self()]; // doesn't work if reloadData not called yet
  }
  return -1;
}



static mforms::TreeNodeRef find_node_at_row(mforms::TreeNodeRef node, int &row_counter, int row)
{
  mforms::TreeNodeRef res;
  for (int i = 0, c = node->count(); i < c; i++)
  {
    mforms::TreeNodeRef child = node->get_child(i);
    if (row_counter == row)
      return child;
    row_counter++;
    if (child->is_expanded())
    {
      res = find_node_at_row(child, row_counter, row);
      if (res)
        return res;
    }
  }
  return res;
}


static mforms::TreeNodeRef treeview_node_with_tag(mforms::TreeNodeView *self, const std::string &tag)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree->mTagIndexEnabled)
  {
    std::map<std::string, MFTreeNodeImpl*>::iterator iter;
    if ((iter = tree->mTagMap.find(tag)) != tree->mTagMap.end())
      return [iter->second nodeRef];
  }
  return mforms::TreeNodeRef();
}


static mforms::TreeNodeRef treeview_node_at_row(mforms::TreeNodeView *self, int row)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree && row >= 0)
  {
    if ([tree frozen])
    {
      if (tree->mFlatTable)
      {
        id node = [[tree->mRootNode children] objectAtIndex: row];
        if (node)
          return [node nodeRef];
      }
      else
      {
        int counter = 0;
        return find_node_at_row(self->root_node(), counter, row);
      }
    }
    else
    {
      if ([tree isPendingReload])
        [tree reloadTreeData];
        
      id n = [tree->mOutline itemAtRow: row];
      if (n)
        return [n nodeRef];
    }
  }
  return mforms::TreeNodeRef();
}


static void treeview_set_column_visible(mforms::TreeNodeView *self, int column, bool flag)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
    [[tree->mOutline tableColumnWithIdentifier: [NSString stringWithFormat:@"%i", column]] setHidden: !flag];
}


static bool treeview_get_column_visible(mforms::TreeNodeView *self, int column)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
    return ![[tree->mOutline tableColumnWithIdentifier: [NSString stringWithFormat:@"%i", column]] isHidden];
  return true;
}


static void treeview_set_column_width(mforms::TreeNodeView *self, int column, int width)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
    [[tree->mOutline tableColumnWithIdentifier: [NSString stringWithFormat:@"%i", column]] setWidth: width];
}


static int treeview_get_column_width(mforms::TreeNodeView *self, int column)
{
  MFTreeNodeViewImpl *tree= self->get_data();
  if (tree)
    return (int)[[tree->mOutline tableColumnWithIdentifier: [NSString stringWithFormat:@"%i", column]] width];
  return 0;
}

void cf_treenodeview_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_treenodeview_impl.create= &treeview_create;
  f->_treenodeview_impl.add_column= &treeview_add_column;
  f->_treenodeview_impl.end_columns= &treeview_end_columns;
  
  f->_treenodeview_impl.clear= &treeview_clear;

  f->_treenodeview_impl.clear_selection= &treeview_clear_selection;
  f->_treenodeview_impl.get_selected_node= &treeview_get_selected;
  f->_treenodeview_impl.get_selection= &treeview_get_selection;
  f->_treenodeview_impl.set_selected= &treeview_set_selected;

  f->_treenodeview_impl.set_selection_mode = &treeview_set_selection_mode;
  f->_treenodeview_impl.get_selection_mode = &treeview_get_selection_mode;
  
  f->_treenodeview_impl.set_row_height= &treeview_set_row_height;
  
  f->_treenodeview_impl.root_node= &treeview_root_node;

  f->_treenodeview_impl.set_allow_sorting= &treeview_allow_sorting;
  f->_treenodeview_impl.freeze_refresh= &treeview_freeze_refresh;
  
  f->_treenodeview_impl.row_for_node = &treeview_row_for_node;
  f->_treenodeview_impl.node_at_row = &treeview_node_at_row;
  f->_treenodeview_impl.node_with_tag = &treeview_node_with_tag;

  f->_treenodeview_impl.set_column_visible = &treeview_set_column_visible;
  f->_treenodeview_impl.get_column_visible = &treeview_get_column_visible;
    
  f->_treenodeview_impl.set_column_width = &treeview_set_column_width;
  f->_treenodeview_impl.get_column_width = &treeview_get_column_width;
}


@end

