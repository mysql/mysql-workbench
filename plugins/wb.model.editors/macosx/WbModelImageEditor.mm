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

#import "WBModelImageEditor.h"
#import "MCPPUtilities.h"

@implementation ImageEditor

static void call_refresh(ImageEditor *self)
{
  [self refresh];
}


- (id)initWithModule:(grt::Module*)module GRTManager:(bec::GRTManager*)grtm arguments:(const grt::BaseListRef&)args
{
  self= [super initWithNibName: @"WbModelImageEditor" bundle: [NSBundle bundleForClass:[self class]]];
  if (self != nil)
  {
    _grtm = grtm;
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


- (void)reinitWithArguments:(const grt::BaseListRef&)args
{
  [super reinitWithArguments: args];
  delete mBackEnd;
  
    // setup the editor backend with the image object (args[0])
  mBackEnd= new ImageEditorBE(_grtm, workbench_model_ImageFigureRef::cast_from(args[0]));
    
  // register a callback that will make [self refresh] get called
  // whenever the backend thinks its needed to refresh the UI from the backend data (ie, the
  // edited object was changed from somewhere else in the application)
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
    NSImage *image= [[[NSImage alloc] initWithContentsOfFile: 
                      [NSString stringWithCPPString: mBackEnd->get_attached_image_path()]] autorelease];
    [imageView setImage: image];
    
    int w, h;
    mBackEnd->get_size(w, h);
    
    [widthField setIntegerValue: w];
    [heightField setIntegerValue: h];
    [keepAspectRatio setState: mBackEnd->get_keep_aspect_ratio() ? NSOnState : NSOffState];
  }
}


- (IBAction)setSize:(id)sender
{
  if (sender == widthField)
  {
    int w, h;
    mBackEnd->get_size(w, h);
    if (w != [widthField integerValue])
    {
      mBackEnd->set_width([widthField integerValue]);
      [self refresh];
    }
  }
  else if (sender == heightField)
  {
    int w, h;
    mBackEnd->get_size(w, h);
    if (h != [heightField integerValue])
    {
      mBackEnd->set_height([heightField integerValue]);
      [self refresh];
    }
  }
}


- (IBAction)toggleAspectRatio:(id)sender
{
  mBackEnd->set_keep_aspect_ratio([keepAspectRatio state] == NSOnState);
}


- (IBAction)resetSize:(id)sender
{
  NSSize size= [[imageView image] size];
  
  mBackEnd->set_size(size.width, size.height);
}


- (IBAction)browse:(id)sender
{
  NSOpenPanel *panel= [NSOpenPanel openPanel];
  
  [panel setAllowsMultipleSelection: NO];
  [panel setCanChooseFiles: YES];
  [panel setCanChooseDirectories: NO];
  
  [panel setTitle: @"Open Image"];
  [panel setAllowedFileTypes: [NSArray arrayWithObject: @"png"]];
  if ([panel runModal] == NSOKButton)
  {
    NSString *path= panel.URL.path;
    NSImage *image= [[[NSImage alloc] initWithContentsOfFile: path] autorelease];
    if (!image)
    {
      NSRunAlertPanel(NSLocalizedString(@"Invalid Image", nil),
                      NSLocalizedString(@"Could not load the image.", nil),
                      NSLocalizedString(@"OK", nil), nil, nil, nil);
      return;
    }
    
    mBackEnd->set_filename([path UTF8String]);
    
    [self refresh];
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

- (bec::BaseEditor*)editorBE
{
  return mBackEnd;
}

@end
