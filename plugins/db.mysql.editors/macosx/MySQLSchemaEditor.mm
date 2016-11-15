/* 
 * Copyright (c) 2009, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "base/geometry.h"
#include "base/string_utilities.h"

#import "MySQLSchemaEditor.h"

#import "MCPPUtilities.h"

@implementation DbMysqlSchemaEditor

static void call_refresh(void *theEditor)
{
  DbMysqlSchemaEditor *editor = (__bridge DbMysqlSchemaEditor *)theEditor;
  [editor performSelectorOnMainThread: @selector(refresh) withObject:nil waitUntilDone: YES];
}


- (instancetype)initWithModule: (grt::Module*)module
                     arguments: (const grt::BaseListRef &)args
{
  self = [super initWithNibName: @"MySQLSchemaEditor" bundle: [NSBundle bundleForClass: [self class]]];
  if (self != nil)
  {
    // load GUI. Top level view in the nib is the NSTabView that will be docked to the main window
    [self loadView];

    [self enablePluginDocking: tabView];

    // take the minimum size of the view from the initial size in the nib.
    // Therefore the nib should be designed as small as possible
    // note: the honouring of the min size is not yet implemented
    [self setMinimumSize: [tabView frame].size];
    
    // setup the editor backend with the schema object (args[0])
    mBackEnd= new MySQLSchemaEditorBE(db_mysql_SchemaRef::cast_from(args[0]));
    
    // fill the collation Popup with the list of supported collations taken from the backend
    MFillPopupButtonWithStrings(collationPopup, mBackEnd->get_charset_collation_list());
    
    // register a callback that will make [self refresh] get called
    // whenever the backend thinks its needed to refresh the UI from the backend data (ie, the
    // edited object was changed from somewhere else in the application)
    mBackEnd->set_refresh_ui_slot(std::bind(call_refresh, (__bridge void *)self));
    
    if (mBackEnd->is_editing_live_object())
    {
      if (mBackEnd->get_schema()->oldName() != "")
      {
        [nameText setEnabled: NO];
        [[[commentText superview] viewWithTag: 101] setEnabled: NO];
      }
      [mRefactorButton setEnabled: NO];
    }
    else
    {
      [[commentText enclosingScrollView] setHidden: NO];
      [[[[commentText enclosingScrollView] superview] viewWithTag: 103] setHidden: NO];
      [mRefactorButton setEnabled: NO];
    }
    
    // update the UI
    [self refresh];
  }
  return self;
}

- (instancetype)initWithNibName: (NSString *)nibNameOrNil bundle: (NSBundle *)nibBundleOrNil
{
  return [self initWithModule: nil arguments: grt::BaseListRef()];
}

-(instancetype)initWithCoder: (NSCoder *)coder
{
  return [self initWithModule: nil arguments: grt::BaseListRef()];
}

- (void)reinitWithArguments:(const grt::BaseListRef&)args
{
  [super reinitWithArguments: args];
  
  delete mBackEnd;
  
  mBackEnd = new MySQLSchemaEditorBE(db_mysql_SchemaRef::cast_from(args[0]));
  
  MFillPopupButtonWithStrings(collationPopup, mBackEnd->get_charset_collation_list());
  
  mBackEnd->set_refresh_ui_slot(std::bind(call_refresh, (__bridge void *)self));
  
  if (mBackEnd->is_editing_live_object())
  {
    if (mBackEnd->get_schema()->oldName() != "")
    {
      [nameText setEnabled: NO];
      [[[commentText superview] viewWithTag: 101] setEnabled: NO];
    }
  }
  else
  {
    [[commentText enclosingScrollView] setHidden: NO];
    [[[[commentText enclosingScrollView] superview] viewWithTag: 103] setHidden: NO];
  }
  // update the UI
  [self refresh];

  [self notifyObjectSwitched];
}


- (void) dealloc
{
  delete mBackEnd;
}


/** Fetches object info from the backend and update the UI
 */
- (void)refresh
{
  if (mBackEnd)
  {
    if (!mChanging)
      [nameText setStringValue: [NSString stringWithCPPString:mBackEnd->get_name()]];
    
    // select the current value of option "CHARACTER SET - COLLATE" in the collation popup
    [collationPopup selectItemWithTitle: [NSString stringWithCPPString:mBackEnd->get_schema_option_by_name("CHARACTER SET - COLLATE")]];
    
    if (!mChanging)
      [commentText setString: [NSString stringWithCPPString:mBackEnd->get_comment()]];

    [mRefactorButton setEnabled: mBackEnd->refactor_possible()];
  }
}


- (BOOL)matchesIdentifierForClosingEditor:(NSString*)identifier
{
  return mBackEnd->should_close_on_delete_of([identifier UTF8String]);
}


- (id)identifier
{
  // an identifier for this editor (just take the object id)
  return [NSString stringWithCPPString:mBackEnd->get_object().id()];
}


- (void)pluginDidShow: (id)sender
{
  [[tabView window] makeFirstResponder: nameText];

  [super pluginDidShow: sender];
}


- (void)activateCollectionItem:(id)sender
{
  if (sender == collationPopup)
  {
    mChanging = YES;
    // set the collation and charset of the schema from the selected value
    mBackEnd->set_schema_option_by_name("CHARACTER SET - COLLATE", [[collationPopup titleOfSelectedItem] UTF8String]);
    mChanging = NO;
  }
}

- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
  if ([aNotification object] == nameText)
  {
    // set name of the schema
    mBackEnd->set_name([[nameText stringValue] UTF8String]);
    [self updateTitle: [self title]];
    [mRefactorButton setEnabled: mBackEnd->refactor_possible()];
  }  
}


- (void)textDidEndEditing:(NSNotification *)aNotification
{
  if ([aNotification object] == commentText)
  {
    // set comment for the schema
    mBackEnd->set_comment([[commentText string] UTF8String]);
  }
}

- (bec::BaseEditor*)editorBE
{
  return mBackEnd;
}


- (IBAction)actionButtonClicked:(id)sender
{
  mBackEnd->refactor_catalog();
}

@end
