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
#include "wb_editor_image.h"

#import "WbModelImageEditor.h"
#import "MCPPUtilities.h"

@interface ImageEditor () {
  IBOutlet NSTabView *tabView;

  IBOutlet NSImageView *imageView;
  IBOutlet NSButton *browseButton;
  IBOutlet NSTextField *widthField;
  IBOutlet NSTextField *heightField;
  IBOutlet NSButton *resetSizeButton;
  IBOutlet NSButton *keepAspectRatio;

  ImageEditorBE *mBackEnd;
}

@end

@implementation ImageEditor

static void call_refresh(void *theEditor) {
  ImageEditor *editor = (__bridge ImageEditor *)theEditor;
  [editor refresh];
}

- (instancetype)initWithModule: (grt::Module *)module arguments: (const grt::BaseListRef &)args {
  self = [super initWithNibName: @"WbModelImageEditor" bundle: [NSBundle bundleForClass: [self class]]];
  if (self != nil) {
    // load GUI. Top level view in the nib is the NSTabView that will be docked to the main window
    [self loadView];

    // take the minimum size of the view from the initial size in the nib.
    // Therefore the nib should be designed as small as possible
    // note: the honouring of the min size is not yet implemented
    [self setMinimumSize: [tabView frame].size];

    [self reinitWithArguments: args];
  }
  return self;
}

- (void)reinitWithArguments: (const grt::BaseListRef &)args {
  [super reinitWithArguments: args];
  delete mBackEnd;

  // setup the editor backend with the image object (args[0])
  mBackEnd = new ImageEditorBE(workbench_model_ImageFigureRef::cast_from(args[0]));

  // register a callback that will make [self refresh] get called
  // whenever the backend thinks its needed to refresh the UI from the backend data (ie, the
  // edited object was changed from somewhere else in the application)
  mBackEnd->set_refresh_ui_slot(std::bind(call_refresh, (__bridge void *)self));

  // update the UI
  [self refresh];
}

- (void)dealloc {
  delete mBackEnd;
}

/** Fetches object info from the backend and update the UI
 */
- (void)refresh {
  if (mBackEnd) {
    NSImage *image =
      [[NSImage alloc] initWithContentsOfFile: [NSString stringWithCPPString: mBackEnd->get_attached_image_path()]];
    [imageView setImage: image];

    int w, h;
    mBackEnd->get_size(w, h);

    [widthField setIntegerValue: w];
    [heightField setIntegerValue: h];
    [keepAspectRatio setState: mBackEnd->get_keep_aspect_ratio() ? NSControlStateValueOn : NSControlStateValueOff];
  }
}

- (IBAction)setSize: (id)sender {
  if (sender == widthField) {
    int w, h;
    mBackEnd->get_size(w, h);
    if (w != [widthField integerValue]) {
      mBackEnd->set_width([widthField intValue]);
      [self refresh];
    }
  } else if (sender == heightField) {
    int w, h;
    mBackEnd->get_size(w, h);
    if (h != [heightField integerValue]) {
      mBackEnd->set_height([heightField intValue]);
      [self refresh];
    }
  }
}

- (IBAction)toggleAspectRatio: (id)sender {
  mBackEnd->set_keep_aspect_ratio([keepAspectRatio state] == NSControlStateValueOn);
}

- (IBAction)resetSize: (id)sender {
  NSSize size = [[imageView image] size];

  mBackEnd->set_size(size.width, size.height);
}

- (IBAction)browse: (id)sender {
  NSOpenPanel *panel = [NSOpenPanel openPanel];

  [panel setAllowsMultipleSelection: NO];
  [panel setCanChooseFiles: YES];
  [panel setCanChooseDirectories: NO];

  [panel setTitle: @"Open Image"];
  [panel setAllowedFileTypes: @[ @"png" ]];
  if ([panel runModal] == NSModalResponseOK) {
    NSString *path = panel.URL.path;
    NSImage *image = [[NSImage alloc] initWithContentsOfFile: path];
    if (!image) {
      NSAlert *alert = [NSAlert new];
      alert.messageText = @"Invalid Image";
      alert.informativeText = @"Could not load the image.";
      alert.alertStyle = NSAlertStyleWarning;
      [alert addButtonWithTitle: @"Close"];
      [alert runModal];

      return;
    }

    mBackEnd->set_filename([path UTF8String]);

    [self refresh];
  }
}

- (id)panelId {
  // an identifier for this editor (just take the object id)
  return [NSString stringWithCPPString:mBackEnd->get_object().id()];
}

- (BOOL)matchesIdentifierForClosingEditor:(NSString *)identifier {
  return mBackEnd->should_close_on_delete_of([identifier UTF8String]);
}

- (bec::BaseEditor *)editorBE {
  return mBackEnd;
}

@end
