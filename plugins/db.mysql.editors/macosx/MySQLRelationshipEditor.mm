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

#include "base/geometry.h"
#include "base/string_utilities.h"

#include "mysql_relationship_editor.h"

#import "MySQLRelationshipEditor.h"

#import "MCPPUtilities.h"

@interface DbMysqlRelationshipEditor()
{
  IBOutlet NSTabView *tabView;

  IBOutlet NSTextField *caption1Edit;
  IBOutlet NSTextField *caption2Edit;
  IBOutlet NSTextView *commentText;
  IBOutlet NSMatrix *visibilityRadios;

  IBOutlet NSImageView *previewImage;

  IBOutlet NSTextField *caption1FullText;
  IBOutlet NSTextField *caption2FullText;

  IBOutlet NSTextField *table1NameText;
  IBOutlet NSTextField *table2NameText;
  IBOutlet NSTextField *table1FKText;
  IBOutlet NSTextField *table1ColumnText;
  IBOutlet NSTextField *table2ColumnText;

  IBOutlet NSButton *mandatory1Check;
  IBOutlet NSButton *mandatory2Check;

  IBOutlet NSButton *identifyingCheck;

  IBOutlet NSMatrix *cardinalityRadios;
  IBOutlet NSButtonCell *oneToManyRadio;
  IBOutlet NSButtonCell *oneToOneRadio;

  RelationshipEditorBE *mBackEnd;
}

@end

@implementation DbMysqlRelationshipEditor


static void call_refresh(void *theEditor)
{
  DbMysqlRelationshipEditor *editor = (__bridge DbMysqlRelationshipEditor *)theEditor;
  [editor performSelectorOnMainThread: @selector(refresh) withObject: nil waitUntilDone: YES];
}


- (instancetype)initWithModule: (grt::Module*)module
                     arguments: (const grt::BaseListRef &)args
{
  self = [super initWithNibName: @"MySQLRelationshipEditor" bundle: [NSBundle bundleForClass: [self class]]];
  if (self != nil)
  {
    [self loadView];
    
    [self setMinimumSize: [tabView frame].size];
    
    [self reinitWithArguments: args];
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

- (void)reinitWithArguments: (const grt::BaseListRef&)args
{
  [super reinitWithArguments: args];
  
  delete mBackEnd;
  
  mBackEnd = new RelationshipEditorBE(workbench_physical_ConnectionRef::cast_from(args[0]));
  
  mBackEnd->set_refresh_ui_slot(std::bind(call_refresh, (__bridge void *)self));
  
  // update the UI
  [self refresh];
}


- (void) dealloc
{
  delete mBackEnd;
}

/**
 * Fetches object info from the backend and updates the UI.
 */
- (void)refresh
{
  if (mBackEnd)
  {
    [caption1Edit setStringValue: [NSString stringWithCPPString:mBackEnd->get_caption()]];
    [caption2Edit setStringValue: [NSString stringWithCPPString:mBackEnd->get_extra_caption()]];

    [caption1FullText setStringValue: [NSString stringWithCPPString:mBackEnd->get_caption_long()]];
    [caption2FullText setStringValue: [NSString stringWithCPPString:mBackEnd->get_extra_caption_long()]];
    
    [table1NameText setStringValue: [NSString stringWithCPPString:mBackEnd->get_left_table_name()]];
    [table2NameText setStringValue: [NSString stringWithCPPString:mBackEnd->get_right_table_name()]];
    [table1ColumnText setStringValue: [NSString stringWithCPPString:mBackEnd->get_left_table_info()]];
    [table2ColumnText setStringValue: [NSString stringWithCPPString:mBackEnd->get_right_table_info()]];
    [table1FKText setStringValue: [NSString stringWithCPPString:mBackEnd->get_left_table_fk()]];
    
    [mandatory1Check setState: mBackEnd->get_left_mandatory() ? NSControlStateValueOn : NSControlStateValueOff];
    [mandatory2Check setState: mBackEnd->get_right_mandatory() ? NSControlStateValueOn : NSControlStateValueOff];
    
    [identifyingCheck setState: mBackEnd->get_is_identifying() ? NSControlStateValueOn : NSControlStateValueOff];

    [visibilityRadios selectCellWithTag: (int)mBackEnd->get_visibility()];
    
    if (mBackEnd->get_to_many())
      [cardinalityRadios selectCell: oneToManyRadio];
    else
      [cardinalityRadios selectCell: oneToOneRadio];
    
    [commentText setString: [NSString stringWithCPPString:mBackEnd->get_comment()]];
  }
}


- (id)panelId
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
    mBackEnd->set_left_mandatory([mandatory1Check state] == NSControlStateValueOn);
  else if (sender == mandatory2Check)
    mBackEnd->set_right_mandatory([mandatory2Check state] == NSControlStateValueOn);
  else if (sender == identifyingCheck)
    mBackEnd->set_is_identifying([identifyingCheck state] == NSControlStateValueOn);
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
