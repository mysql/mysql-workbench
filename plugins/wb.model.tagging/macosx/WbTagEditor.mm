//
//  WbTagEditor.mm
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 10/Apr/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import "WbTagEditor.h"
#import "MCPPUtilities.h"
#import "GRTIconCache.h"

@implementation WbTagEditor

- (id)initWithModule:(grt::Module*)module arguments:(const grt::BaseListRef&)args
{
  self= [super initWithModule:module arguments:args];
  if (self)
  {
    if (![[NSBundle bundleForClass:[self class]] loadNibFile:@"WbTagEditor"
                                           externalNameTable:[NSDictionary dictionaryWithObject:self forKey:NSNibOwner] 
                                                    withZone:nil])
      NSLog(@"Could not load WbTagEditor.xib");

    [self setMinimumSize: [editorTabView frame].size];
    
    [objectTable registerForDraggedTypes:[NSArray arrayWithObject:@"x-mysql-wb/db.DatabaseObject"]];
    
    mBackEnd= new TagEditorBE(workbench_physical_ModelRef::cast_from(args[0]));
    
    [self refresh];
  }
  return self;
}

- (void)dealloc
{
  delete mBackEnd;
  [mTagArray release];
  [super dealloc];
}

- (NSView*)dockableView
{
  return editorTabView;
}



- (id)identifier
{
  return @"tageditor";
}


- (NSString*)title
{
  return @"Object Tags";
}


- (void)refreshTagTable
{
  std::vector<std::string> tags(mBackEnd->get_tags());
  mTagArray= [[NSMutableArray arrayWithCapacity: tags.size()] retain];
  for (std::vector<std::string>::const_iterator iter= tags.begin();
    iter != tags.end(); ++iter)
  {
    [mTagArray addObject: [NSString stringWithCPPString: *iter]];
  }
  [tagTable reloadData];
}


- (void)saveChanges
{
  mBackEnd->begin_save();
  
  mBackEnd->set_tag_name([[tagNameText stringValue] UTF8String]);
  mBackEnd->set_tag_label([[tagLabelText stringValue] UTF8String]);
  mBackEnd->set_tag_color([[[tagColor color] hexString] UTF8String]);
  mBackEnd->set_tag_comment([[tagComment string] UTF8String]);
  
  mBackEnd->end_save();
}


- (void)activateCollectionItem:(id)sender
{
  if (sender == categoryPop)
  {
    NSInteger i= [categoryPop indexOfSelectedItem];
    if (i == [categoryPop numberOfItems]-1)
    {
      [categoryPop selectItemAtIndex: mBackEnd->get_selected_category()];
      mBackEnd->edit_categories();
      
      [self refresh];
    }
    else
    {
      mBackEnd->set_selected_category([categoryPop indexOfSelectedItem]);
      
      [self refreshTagTable];
    }
  }
}


- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
  if (aNotification == nil || [aNotification object] == tagTable)
  {
    NSInteger row= [tagTable selectedRow];
    
    [self saveChanges];
    [self refreshTagTable];
    
    mBackEnd->set_selected_tag(row);
    
    [tagNameText setStringValue: [NSString stringWithCPPString: mBackEnd->get_tag_name()]];
    [tagLabelText setStringValue: [NSString stringWithCPPString: mBackEnd->get_tag_label()]];
    [tagColor setColor: [NSColor colorFromHexString:[NSString stringWithCPPString: mBackEnd->get_tag_color()]] ?: [NSColor blackColor]];
    [tagComment setString: [NSString stringWithCPPString: mBackEnd->get_tag_comment()]];
    
    if (row < 0)
    {
      [tagNameText setEnabled: NO];
      [tagLabelText setEnabled: NO];
      [tagColor setEnabled: NO];
      [tagComment setEditable: NO];
      
      [deleteTagButton setEnabled: NO];
    }
    else
    {
      [tagNameText setEnabled: YES];
      [tagLabelText setEnabled: YES];
      [tagColor setEnabled: YES];
      [tagComment setEditable: YES];
      
      [deleteTagButton setEnabled: YES];
    }
    
    [objectTable reloadData];
  }
  
  if (aNotification == nil || [aNotification object] == objectTable)
  {
    if ([objectTable selectedRow] < 0)
    {
      [objectText setString: @""];
      [objectText setEditable: NO];
    }
    else
    {
      std::string text;
      mBackEnd->get_object_list()->get_field([objectTable selectedRow], TagObjectListBE::Documentation, text);
      [objectText setString: [NSString stringWithCPPString: text]];
      [objectText setEditable: YES];
    }
  }
}


- (IBAction)addTag:(id)sender
{
  mBackEnd->add_tag();
  [self refreshTagTable];
  [tagTable selectRow: [self numberOfRowsInTableView:tagTable]-1 byExtendingSelection:NO];
}


- (IBAction)deleteTag:(id)sender
{
  mBackEnd->delete_tag();
  [self refreshTagTable];
}


- (void)pluginWillClose:(id)sender
{
  [self saveChanges];
}


- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
  if (aTableView == tagTable)
  {
    return [mTagArray count];
  }
  else if (aTableView == objectTable)
  {
    if (mBackEnd)
      return mBackEnd->get_object_list()->count();
  }
    
  return 0;
}


- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  if (aTableView == tagTable)
  {
    return [mTagArray objectAtIndex: rowIndex];
  }
  else if (aTableView == objectTable)
  {
    std::string value;
    mBackEnd->get_object_list()->get_field(rowIndex, 0, value);
    return [NSString stringWithCPPString: value];
  }
  
  return nil;
}

- (void)refresh
{
  if (mBackEnd)
  {
    std::vector<std::string> cats(mBackEnd->get_categories());
    
    [categoryPop removeAllItems];
    for (std::vector<std::string>::const_iterator c= cats.begin(); c!= cats.end(); ++c)
    {
      [categoryPop addItemWithTitle: [NSString stringWithCPPString: *c]];
    }
    [[categoryPop menu] addItem: [NSMenuItem separatorItem]];
    [categoryPop addItemWithTitle: @"Edit Categories..."];
    
    [self tableViewSelectionDidChange: nil];
  }
}

- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
  if ([aNotification object] == tagNameText)
  {
    if ([tagTable selectedRow] >= 0)
    {
      [mTagArray replaceObjectAtIndex: [tagTable selectedRow]
                           withObject: [tagNameText stringValue]];
      [tagTable reloadData];
    }
  }
  else if ([aNotification object] == tagLabelText)
  {
    [self saveChanges];
  }
}




- (NSDragOperation)tableView:(NSTableView*)tv
                validateDrop:(id <NSDraggingInfo>)info
                 proposedRow:(int)row
       proposedDropOperation:(NSTableViewDropOperation)op
{
  if ([[[info draggingPasteboard] types] containsObject: @"x-mysql-wb/db.DatabaseObject"])
    return NSDragOperationGeneric;
  
  return NSDragOperationNone;
}


- (BOOL)tableView:(NSTableView *)aTableView acceptDrop:(id <NSDraggingInfo>)info
              row:(int)row dropOperation:(NSTableViewDropOperation)operation
{  
  if (aTableView == objectTable)
  {
    NSPasteboard *pboard= [info draggingPasteboard];
    NSString *data= [pboard stringForType:@"x-mysql-wb/db.DatabaseObject"];
    bool flag= mBackEnd->get_object_list()->add_dropped_objectdata([data UTF8String]);
  
    if (flag)
      [objectTable reloadData];
  
    return flag;
  }
  return NO;
}


- (void)tableView:(NSTableView *)aTableView willDisplayCell:(id)aCell forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
  if (aTableView == objectTable)
  {
    bec::IconId icon_id= mBackEnd->get_object_list()->get_field_icon(rowIndex, 0, bec::Icon16);
    
    if (icon_id != 0)
    {
      NSImage *image= [[GRTIconCache sharedIconCache] imageForIconId:icon_id];
      [aCell setImage:image];
    }
    else
      [aCell setImage:nil];
  }
}


- (void)textDidChange:(NSNotification *)aNotification
{
  if ([aNotification object] == objectText)
  {
    if ([objectTable selectedRow] >= 0)
      mBackEnd->get_object_list()->set_field([objectTable selectedRow], TagObjectListBE::Documentation,
                                             [[objectText string] UTF8String]);
  }
}


@end
