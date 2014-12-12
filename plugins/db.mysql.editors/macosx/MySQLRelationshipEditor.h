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


#import <Cocoa/Cocoa.h>

#import "WBPluginEditorBase.h"

#include "mysql_relationship_editor.h"


@interface DbMysqlRelationshipEditor : WBPluginEditorBase {
  IBOutlet NSTabView *tabView; // this editor has a single Tab, but we put in a TabView for homegeneity
  
  IBOutlet NSTextField *caption1Edit;
  IBOutlet NSTextField *caption2Edit;
  IBOutlet NSTextView *commentText;
  IBOutlet NSMatrix *visibilityRadios;
  
  IBOutlet NSImageView *previewImage;

  IBOutlet NSTextField *caption1FullText;
  IBOutlet NSTextField *caption2FullText;
  
  IBOutlet NSTextField *caption1Text;
  IBOutlet NSTextField *caption2Text;
  IBOutlet NSTextField *table1NameText;
  IBOutlet NSTextField *table2NameText;
  IBOutlet NSTextField *table1FKText;
  IBOutlet NSTextField *table2FKText;
  IBOutlet NSTextField *table1ColumnText;
  IBOutlet NSTextField *table2ColumnText;
  
  IBOutlet NSButton *mandatory1Check;
  IBOutlet NSButton *mandatory2Check;
  
  IBOutlet NSButton *identifyingCheck;
  
  IBOutlet NSMatrix *cardinalityRadios;
  IBOutlet NSButtonCell *oneToManyRadio;
  IBOutlet NSButtonCell *oneToOneRadio;

  RelationshipEditorBE *mBackEnd; //!< schema editor backend
}

- (IBAction)editTable:(id)sender;
- (IBAction)invertRelationship:(id)sender;
- (IBAction)changeVisibility:(id)sender;
- (IBAction)userToggleCheck:(id)sender;
- (IBAction)changeCardinality:(id)sender;

- (instancetype)initWithModule: (grt::Module*)module
                    grtManager: (bec::GRTManager*)grtm
                     arguments: (const grt::BaseListRef&)args NS_DESIGNATED_INITIALIZER;


@end
