/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "MySQLSchemaEditor.h"

#import "MCPPUtilities.h"

extern const char *DEFAULT_CHARSET_CAPTION;
extern const char *DEFAULT_COLLATION_CAPTION;

//----------------------------------------------------------------------------------------------------------------------

@interface DbMysqlSchemaEditor () {
  IBOutlet NSTabView *tabView; // this editor has a single Tab, but we put in a TabView for homegeneity
  IBOutlet MTabSwitcher *tabSwitcher;

  IBOutlet NSTextField *nameText;
  IBOutlet NSPopUpButton *collationPopup;
  IBOutlet NSPopUpButton *charsetPopup;

  IBOutlet NSTextView *commentText;
  IBOutlet NSButton *mRefactorButton;

  MySQLSchemaEditorBE *mBackEnd; //!< schema editor backend

  BOOL mChanging;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation DbMysqlSchemaEditor

static void call_refresh(void *theEditor) {
  DbMysqlSchemaEditor *editor = (__bridge DbMysqlSchemaEditor *)theEditor;
  [editor performSelectorOnMainThread: @selector(refresh) withObject: nil waitUntilDone: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithModule: (grt::Module *)module arguments: (const grt::BaseListRef &)args {
  self = [super initWithNibName: @"MySQLSchemaEditor" bundle: [NSBundle bundleForClass: [self class]]];
  if (self != nil) {
    // Load GUI. Top level view in the nib is the NSTabView that will be docked to the main window.
    [self loadView];

    [self enablePluginDocking:tabView];

    // Take the minimum size of the view from the initial size in the nib.
    [self setMinimumSize: tabView.frame.size];

    // Setup the editor backend with the schema object (args[0]).
    mBackEnd = new MySQLSchemaEditorBE(db_mysql_SchemaRef::cast_from(args[0]));

    // Fill the charset and collation Popups with the list of supported charsets/collations taken from the backend.
    MFillPopupButtonWithStrings(charsetPopup, mBackEnd->get_charset_list());
    [[charsetPopup menu] insertItem: [NSMenuItem separatorItem] atIndex:0];
    [charsetPopup insertItemWithTitle: [NSString stringWithUTF8String: DEFAULT_COLLATION_CAPTION] atIndex: 0];
    MFillPopupButtonWithStrings(collationPopup, mBackEnd->get_charset_collation_list(DEFAULT_CHARSET_CAPTION));
    [[collationPopup menu] insertItem: [NSMenuItem separatorItem] atIndex: 0];
    [collationPopup insertItemWithTitle: [NSString stringWithUTF8String: DEFAULT_COLLATION_CAPTION] atIndex: 0];

    // Register a callback that will make [self refresh] get called whenever the backend thinks its needed to refresh
    // the UI from the backend data (ie, the edited object was changed from somewhere else in the application)
    mBackEnd->set_refresh_ui_slot(std::bind(call_refresh, (__bridge void *)self));

    if (mBackEnd->is_editing_live_object()) {
      if (mBackEnd->get_schema()->oldName() != "") { // Cannot rename a live schema.
        [nameText setEnabled: NO];
        [[[commentText superview] viewWithTag: 101] setEnabled: NO];
      }
      [mRefactorButton setEnabled: NO];
    } else {
      [[commentText enclosingScrollView] setHidden: NO];
      [[[[commentText enclosingScrollView] superview] viewWithTag: 103] setHidden: NO];
      [mRefactorButton setEnabled: NO];
    }

    // update the UI
    [self refresh];
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithNibName: (NSString *)nibNameOrNil bundle: (NSBundle *)nibBundleOrNil {
  return [self initWithModule: nil arguments: grt::BaseListRef()];
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithCoder: (NSCoder *)coder {
  return [self initWithModule: nil arguments:grt::BaseListRef()];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)reinitWithArguments: (const grt::BaseListRef &)args {
  [super reinitWithArguments: args];

  delete mBackEnd;

  mBackEnd = new MySQLSchemaEditorBE(db_mysql_SchemaRef::cast_from(args[0]));

  MFillPopupButtonWithStrings(charsetPopup, mBackEnd->get_charset_list());
  [[charsetPopup menu] insertItem: [NSMenuItem separatorItem] atIndex:0];
  [charsetPopup insertItemWithTitle: [NSString stringWithUTF8String: DEFAULT_COLLATION_CAPTION] atIndex:0];
  MFillPopupButtonWithStrings(collationPopup, mBackEnd->get_charset_collation_list(DEFAULT_CHARSET_CAPTION));
  [[charsetPopup menu] insertItem: [NSMenuItem separatorItem] atIndex:0];
  [charsetPopup insertItemWithTitle: [NSString stringWithUTF8String: DEFAULT_COLLATION_CAPTION] atIndex: 0];

  mBackEnd->set_refresh_ui_slot(std::bind(call_refresh, (__bridge void *)self));

  if (mBackEnd->is_editing_live_object()) {
    if (mBackEnd->get_schema()->oldName() != "") {
      [nameText setEnabled: NO];
      [[[commentText superview] viewWithTag: 101] setEnabled: NO];
    }
  } else {
    [[commentText enclosingScrollView] setHidden: NO];
    [[[[commentText enclosingScrollView] superview] viewWithTag: 103] setHidden: NO];
  }

  [self refresh];
  [self notifyObjectSwitched];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  delete mBackEnd;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Fetches object info from the backend and update the UI
 */
- (void)refresh {
  if (mBackEnd) {
    if (!mChanging)
      [nameText setStringValue: [NSString stringWithCPPString:mBackEnd->get_name()]];
    // Select the current value of option "CHARACTER SET" in the charset popup.
    NSString *charset = [NSString stringWithCPPString:mBackEnd->get_schema_option_by_name("CHARACTER SET")];
    if ([charset isEqualToString: @"DEFAULT"] or [charset length] == 0)
      charset = [NSString stringWithUTF8String: DEFAULT_CHARSET_CAPTION];
    [charsetPopup selectItemWithTitle: charset];
    // Select the current value of option "COLLATE" in the collation popup.
    [self updateCollationPopup: charset.UTF8String];
    NSString* collate = [NSString stringWithCPPString: mBackEnd->get_schema_option_by_name("COLLATE")];
    if ([collate isEqualToString: @"DEFAULT"] or [collate length] == 0)
      collate = [NSString stringWithUTF8String: DEFAULT_COLLATION_CAPTION];
    [collationPopup selectItemWithTitle: collate];

    if (!mChanging)
      [commentText setString: [NSString stringWithCPPString: mBackEnd->get_comment()]];
    [mRefactorButton setEnabled: mBackEnd->refactor_possible()];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)matchesIdentifierForClosingEditor: (NSString *)identifier {
  return mBackEnd->should_close_on_delete_of([identifier UTF8String]);
}

//----------------------------------------------------------------------------------------------------------------------

- (id)panelId {
  // An identifier for this editor (just take the object id).
  return [NSString stringWithCPPString:mBackEnd->get_object().id()];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)pluginDidShow: (id)sender {
  [[tabView window] makeFirstResponder:nameText];

  [super pluginDidShow:sender];
}

//----------------------------------------------------------------------------------------------------------------------

- (IBAction)activateCharsetCollationPopup: (id)sender {
  if (sender == collationPopup) {
    mChanging = YES;
    NSString* collate = collationPopup.titleOfSelectedItem;
    if ([collate isEqualToString: [NSString stringWithUTF8String: DEFAULT_CHARSET_CAPTION]])
      collate = @"DEFAULT";
    // set the collation and charset of the schema from the selected value
    mBackEnd->set_schema_option_by_name("COLLATE", collate.UTF8String);
    mChanging = NO;
  } else if (sender == charsetPopup) {
    mChanging = YES;
    NSString* charset = charsetPopup.titleOfSelectedItem;
    if ([charset isEqualToString: [NSString stringWithUTF8String: DEFAULT_CHARSET_CAPTION]])
      charset = @"DEFAULT";
    // set the collation and charset of the schema from the selected value
    mBackEnd->set_schema_option_by_name("CHARACTER SET", charset.UTF8String);
    mChanging = NO;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)controlTextDidEndEditing: (NSNotification *)aNotification {
  if ([aNotification object] == nameText) {
    // set name of the schema
    mBackEnd->set_name([[nameText stringValue] UTF8String]);
    [self updateTitle: [self title]];
    [mRefactorButton setEnabled: mBackEnd->refactor_possible()];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)textDidEndEditing: (NSNotification *)aNotification {
  if ([aNotification object] == commentText) {
    // set comment for the schema
    mBackEnd->set_comment([[commentText string] UTF8String]);
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (bec::BaseEditor *)editorBE {
  return mBackEnd;
}

//----------------------------------------------------------------------------------------------------------------------

- (IBAction)actionButtonClicked: (id)sender {
  mBackEnd->refactor_catalog();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)updateCollationPopup: (const char *)charset {
  MFillPopupButtonWithStrings(collationPopup, mBackEnd->get_charset_collation_list(charset));
  [[collationPopup menu] insertItem: [NSMenuItem separatorItem] atIndex: 0];
  [collationPopup insertItemWithTitle: [NSString stringWithUTF8String: DEFAULT_COLLATION_CAPTION]
                              atIndex:0];
}

@end

//----------------------------------------------------------------------------------------------------------------------
