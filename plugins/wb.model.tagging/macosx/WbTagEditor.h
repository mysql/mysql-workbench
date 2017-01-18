//
//  WbTagEditor.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 10/Apr/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "WBPluginEditorBase.h"

#include "wb_tag_editor.h"

@class GRTListDataSource;

@interface WbTagEditor : WBPluginEditorBase {
  IBOutlet NSTabView *editorTabView;

  IBOutlet NSPopUpButton *categoryPop;
  IBOutlet NSTableView *tagTable;

  IBOutlet NSTextField *tagNameText;
  IBOutlet NSTextField *tagLabelText;
  IBOutlet NSColorWell *tagColor;
  IBOutlet NSTextView *tagComment;

  IBOutlet NSTableView *objectTable;
  IBOutlet NSTextView *objectText;

  IBOutlet NSButton *deleteTagButton;

  NSMutableArray *mTagArray;

  TagEditorBE *mBackEnd;
}

- (IBAction)addTag:(id)sender;
- (IBAction)deleteTag:(id)sender;

@end
