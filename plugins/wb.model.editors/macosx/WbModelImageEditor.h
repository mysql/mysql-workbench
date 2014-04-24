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


#import <Cocoa/Cocoa.h>
#import "WBPluginEditorBase.h"

#include "wb_editor_image.h"


@interface ImageEditor : WBPluginEditorBase {
  IBOutlet NSTabView *tabView; // this editor has a single Tab, but we put in a TabView for homegeneity
  
  IBOutlet NSImageView *imageView;
  IBOutlet NSButton *browseButton;
  IBOutlet NSTextField *widthField;
  IBOutlet NSTextField *heightField;  
  IBOutlet NSButton *resetSizeButton;
  IBOutlet NSButton *keepAspectRatio;
  
  ImageEditorBE *mBackEnd; //!< iamge editor backend
}

- (IBAction)browse:(id)sender;
- (IBAction)resetSize:(id)sender;
- (IBAction)setSize:(id)sender;
- (IBAction)toggleAspectRatio:(id)sender;

- (id)initWithModule:(grt::Module*)module GRTManager:(bec::GRTManager*)grtm arguments:(const grt::BaseListRef&)args;

@end
