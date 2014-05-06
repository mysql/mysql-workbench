/* 
 * Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.
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
//
//  MFWizard.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 7/Feb/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "mforms/wizard.h"

@interface MFWizardBox : NSBox
{
}

@end


@interface MFWizardImpl : NSWindowController {
  mforms::Wizard *mOwner;
  IBOutlet NSButton *nextButton;
  IBOutlet NSButton *backButton;
  IBOutlet NSButton *extraButton;
  IBOutlet NSBox *contentBox;
  IBOutlet NSTextField *headingText;
  IBOutlet NSImage *backImage;
  IBOutlet NSView *stepList;
  
  float mOriginalButtonWidth;
  
  BOOL mAllowCancel;
}

- (IBAction)performNext:(id)sender;
- (IBAction)performBack:(id)sender;
- (IBAction)performExtra:(id)sender;

@end
