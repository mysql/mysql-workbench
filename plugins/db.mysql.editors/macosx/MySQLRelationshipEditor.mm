/* 
 * Copyright (c) 2009, 2012, Oracle and/or its affiliates. All rights reserved.
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

#import "MySQLRelationshipEditor.h"

#import "MCPPUtilities.h"
#include "grtdb/db_object_helpers.h" // get_rdbms_for_db_object()

@implementation DbMysqlRelationshipEditor


static void call_refresh(DbMysqlRelationshipEditor *self)
{
  [self performSelectorOnMainThread:@selector(refresh) withObject:nil waitUntilDone:YES];
}


- (id)initWithModule:(grt::Module*)module GRTManager:(bec::GRTManager*)grtm arguments:(const grt::BaseListRef&)args
{
  self= [super initWithNibName: @"MySQLRelationshipEditor" bundle: [NSBundle bundleForClass:[self class]]];
  if (self != nil)
  {
    _grtm = grtm;
    [self loadView];
    
    [self setMinimumSize: [tabView frame].size];
    
    [self reinitWithArguments: args];
  }
  return self;
}
    

- (void)reinitWithArguments: (const grt::BaseListRef&)args
{
  [super reinitWithArguments: args];
  
  delete mBackEnd;
  
  mBackEnd= new RelationshipEditorBE(_grtm, workbench_physical_ConnectionRef::cast_from(args[0]));
  
  mBackEnd->set_refresh_ui_slot(boost::bind(call_refresh, self));
  
  // update the UI
  [self refresh];
}


- (void) dealloc
{
  delete mBackEnd;
  [super dealloc];
}


/** Fetches object info from the backend and update the UI
 */
- (void)refresh
{
  if (mBackEnd)
  {
    [caption1Text setStringValue: [NSString stringWithCPPString:mBackEnd->get_caption()]];
    [caption2Text setStringValue: [NSString stringWithCPPString:mBackEnd->get_extra_caption()]];
    [caption1Edit setStringValue: [NSString stringWithCPPString:mBackEnd->get_caption()]];
    [caption2Edit setStringValue: [NSString stringWithCPPString:mBackEnd->get_extra_caption()]];

    [caption1FullText setStringValue: [NSString stringWithCPPString:mBackEnd->get_caption_long()]];
    [caption2FullText setStringValue: [NSString stringWithCPPString:mBackEnd->get_extra_caption_long()]];
    
    [table1NameText setStringValue: [NSString stringWithCPPString:mBackEnd->get_left_table_name()]];
    [table2NameText setStringValue: [NSString stringWithCPPString:mBackEnd->get_right_table_name()]];
    [table1ColumnText setStringValue: [NSString stringWithCPPString:mBackEnd->get_left_table_info()]];
    [table2ColumnText setStringValue: [NSString stringWithCPPString:mBackEnd->get_right_table_info()]];
    [table1FKText setStringValue: [NSString stringWithCPPString:mBackEnd->get_left_table_fk()]];
    
    [mandatory1Check setState: mBackEnd->get_left_mandatory() ? NSOnState : NSOffState];
    [mandatory2Check setState: mBackEnd->get_right_mandatory() ? NSOnState : NSOffState];
    
    [identifyingCheck setState: mBackEnd->get_is_identifying() ? NSOnState : NSOffState];

    [visibilityRadios selectCellWithTag: (int)mBackEnd->get_visibility()];
    
    if (mBackEnd->get_to_many())
      [cardinalityRadios selectCell: oneToManyRadio];
    else
      [cardinalityRadios selectCell: oneToOneRadio];
    
    [commentText setString: [NSString stringWithCPPString:mBackEnd->get_comment()]];
  }
}


- (id)identifier
{
  // an identifier for this editor (just take the object id)
  return [NSString stringWithCPPString:mBackEnd->get_object().id()];
}


- (BOOL)matchesIdentifierForClosingEditor:(NSString*)identifier
{
  return mBackEnd->should_close_on_delete_of([identifier UTF8String]);
}



- (IBAction)editTable:(id)sender
{
  if ([sender tag] == 100)
  {
    mBackEnd->open_editor_for_left_table();
  }
  else
  {
    mBackEnd->open_editor_for_right_table();
  }
}


- (IBAction)invertRelationship:(id)sender
{
}


- (IBAction)userToggleCheck:(id)sender
{
  if (sender == mandatory1Check)
    mBackEnd->set_left_mandatory([mandatory1Check state] == NSOnState);
  else if (sender == mandatory2Check)
    mBackEnd->set_right_mandatory([mandatory2Check state] == NSOnState);
  else if (sender == identifyingCheck)
    mBackEnd->set_is_identifying([identifyingCheck state] == NSOnState);
}


- (IBAction)changeCardinality:(id)sender
{
  if ([sender selectedCell] == oneToOneRadio)
    mBackEnd->set_to_many(false);
  else
    mBackEnd->set_to_many(true);
}


- (IBAction)changeVisibility:(id)sender
{
  switch ([[visibilityRadios selectedCell] tag])
  {
    case 1: // full
      mBackEnd->set_visibility(RelationshipEditorBE::Visible);
      break;
    case 2: // splitted
      mBackEnd->set_visibility(RelationshipEditorBE::Splitted);
      break;      
    case 3: // hide
      mBackEnd->set_visibility(RelationshipEditorBE::Hidden);
      break;
  }
}


- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
  if ([aNotification object] == caption1Edit)
  {
    // set name of the schema
    mBackEnd->set_caption([[caption1Edit stringValue] UTF8String]);
    [self refresh];
  }
  else if ([aNotification object] == caption2Edit)
  {
    // set name of the schema
    mBackEnd->set_extra_caption([[caption2Edit stringValue] UTF8String]);
    [self refresh];
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
@end
