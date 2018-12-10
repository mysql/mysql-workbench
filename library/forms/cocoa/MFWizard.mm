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

#import "MFWizard.h"

#import "MFBase.h"
#import "MFMForms.h"

#import "MFTable.h"

//----------------------------------------------------------------------------------------------------------------------

@interface MFWizardBox : NSBox

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFWizardBox

-(void)drawRect: (NSRect)rect {
  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = self.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  NSColor *background = [NSColor.textBackgroundColor colorWithAlphaComponent: 0.7];
  [background set];
  NSRectFill(rect);
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface MFWizardImpl () {
  mforms::Wizard *mOwner;
  IBOutlet NSButton *nextButton;
  IBOutlet NSButton *backButton;
  IBOutlet NSButton *extraButton;
  IBOutlet NSBox *contentBox;
  IBOutlet NSTextField *headingText;
  IBOutlet NSView *stepList;

  float mOriginalButtonWidth;
  BOOL mAllowCancel;

  NSMutableArray *nibObjects;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFWizardImpl

- (instancetype)initWithObject: (::mforms::Wizard*)aWizard {
  self = [super init];
  if (self != nil) {
    NSMutableArray *temp;
    if ([NSBundle.mainBundle loadNibNamed: @"WizardWindow" owner: self topLevelObjects: &temp]) {
      nibObjects = temp;
      self.window.contentSize = NSMakeSize(900, 620);
      self.window.minSize = self.window.frame.size;
      [self.window center];

      mOriginalButtonWidth = NSWidth(nextButton.frame);

      mAllowCancel = YES;
      mOwner = aWizard;
      mOwner->set_data(self);
    }
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::Object*)mformsObject {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (IBAction)performNext: (id)sender {
  mOwner->next_clicked();
}

//----------------------------------------------------------------------------------------------------------------------

- (IBAction)performBack: (id)sender {
  mOwner->back_clicked();
}

//----------------------------------------------------------------------------------------------------------------------

- (IBAction)performExtra:(id)sender {
  mOwner->extra_clicked();
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)windowShouldClose: (id)window {
  if (mAllowCancel && (!mOwner->_cancel_slot || mOwner->_cancel_slot())) {
    [NSApp stopModal];
    return YES;
  }
  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setCurrentStep: (NSInteger)step {
  BOOL unexecuted = NO;
  for (NSInteger i= 0; i < (int)stepList.subviews.count / 2; i++) {
    if (i == step) {
      [[stepList viewWithTag: i] setImage: [NSImage imageNamed: @"DotBlue"]];
      unexecuted= YES;
    } else if (!unexecuted) {
      [[stepList viewWithTag: i] setImage: [NSImage imageNamed: @"DotGrey"]];
    } else {
      [[stepList viewWithTag: i] setImage: [NSImage imageNamed: @"DotDisabled"]];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setStepTitles: (NSArray*)titles {
  float y;
  float width;
  int row= 0;
  NSImage *disabled = [NSImage imageNamed: @"DotDisabled"];
  NSImage *current = [NSImage imageNamed: @"DotBlue"];
  NSImage *executed = [NSImage imageNamed: @"DotGrey"];
  
  int delcount= int(titles.count - stepList.subviews.count);

  if (delcount > 0) {
    // if there are more steps than needed, remove them
    for (id step in [stepList.subviews reverseObjectEnumerator]) {
      delcount--;
      [step removeFromSuperview];
      if (delcount == 0)
        break;
    }
  }

  y = NSHeight(stepList.frame) - 18;
  width = NSWidth(stepList.frame);

  for (NSString *title in titles) {
    NSImageView *image;
    NSTextField *text;
   
    image = [stepList viewWithTag: row * 2];
    text = [stepList viewWithTag: row * 2 + 1];
    if (image == nil) {
      image = [[NSImageView alloc] initWithFrame: NSMakeRect(0, y - 1, 16, 16)];
      image.tag = row * 2;
      [stepList addSubview: image];
      image.autoresizingMask = NSViewMinXMargin|NSViewMaxYMargin;
    }

    switch ([title characterAtIndex: 0]) {
      case '*': // current
        image.image = current;
        break;
        
      case '.': // executed
        image.image = executed;
        break;
        
      case '-': // disabled
      default:
        image.image = disabled;
        break;
    }

    if (!text) {
      text = [[NSTextField alloc] initWithFrame: NSMakeRect(16, y, width, 16)];
      text.tag = row * 2 + 1;
      [text setEditable: NO];
      [text setBordered: NO];
      [text setDrawsBackground: NO];
      [stepList addSubview: text];
      text.autoresizingMask = NSViewMinXMargin|NSViewMaxYMargin;
    }
    text.stringValue = [title substringFromIndex: 1];
    
    row++;
    y-= 25;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)rearrangeButtons {
  float windowWidth = NSWidth(self.window.contentView.frame);
  NSRect frame;
  float nextX;

  // resize and rearrange next/back buttons to fit
  [nextButton sizeToFit];
  if (NSWidth(nextButton.frame) < mOriginalButtonWidth) {
    // enforce a min width
    frame = nextButton.frame;
    frame.size.width = mOriginalButtonWidth;
    nextButton.frame = frame;
  } else {
    frame = nextButton.frame;
    frame.size.width += 10;
  }

  nextX = frame.origin.x = windowWidth - NSWidth(frame) - 16;
  nextButton.frame = frame;
  
  frame = backButton.frame;
  frame.origin.x = nextX - 3 - NSWidth(frame);
  backButton.frame = frame;
}

//----------------------------------------------------------------------------------------------------------------------

static bool wizard_create(::mforms::Wizard *self, mforms::Form *owner) {
  return [[MFWizardImpl alloc] initWithObject: self] != nil;
    
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_set_title(mforms::Wizard *self, const std::string &title) {
  if (self) {
    MFWizardImpl* wizard = self->get_data();
    if (wizard != nil) {
      wizard.window.title = wrap_nsstring(title);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_run_modal(mforms::Wizard *self) {
  MFWizardImpl* wizard = self->get_data();

  if (wizard != nil) {
    [wizard.window makeKeyAndOrderFront:nil];
    try {
      [NSApp runModalForWindow:wizard.window];
    } catch (const std::exception &exc) {
      [NSApp stopModal];
      NSAlert *alert = [NSAlert new];
      alert.messageText = @"Unhandled Exception";
      alert.informativeText = [NSString stringWithFormat: @"An unhandled exception has occurred while executing the wizard. "
                               "Please restart Workbench at the first opportunity.\nException: %s", exc.what()];
      alert.alertStyle = NSAlertStyleCritical;
      [alert addButtonWithTitle: @"Close"];
      [alert runModal];
    }

    [wizard.window orderOut: nil];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_close(mforms::Wizard *self) {
  MFWizardImpl* wizard = self->get_data();

  if (wizard != nil) {
    [NSApp stopModal];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_set_content(mforms::Wizard *self, mforms::View *view) {
  MFWizardImpl* wizard = self->get_data();

  if (wizard != nil) {
    [wizard->contentBox.contentView removeFromSuperview];
    if (view != nullptr) {
      wizard->contentBox.contentView = view->get_data();
      [view->get_data() resizeSubviewsWithOldSize:wizard->contentBox.frame.size];
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_set_heading(mforms::Wizard *self, const std::string &text) {
  MFWizardImpl* wizard = self->get_data();
  if (wizard != nil) {
    wizard->headingText.stringValue = wrap_nsstring(text);
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_set_step_list(mforms::Wizard *self, const std::vector<std::string> &steps) {
  MFWizardImpl* wizard = self->get_data();
  if (wizard != nil) {
    NSMutableArray *array= [NSMutableArray array];
    for (std::vector<std::string>::const_iterator step= steps.begin(); step != steps.end(); ++step) {
      [array addObject: wrap_nsstring(*step)];
    }
    [wizard setStepTitles: array];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_set_allow_cancel(mforms::Wizard *self, bool flag) {
  MFWizardImpl* wizard = self->get_data();
  if (wizard != nil) {
    wizard->mAllowCancel= flag;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_set_allow_back(mforms::Wizard *self, bool flag) {
  MFWizardImpl* wizard = self->get_data();
  if (wizard != nil) {
    wizard->backButton.enabled = flag;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_set_allow_next(mforms::Wizard *self, bool flag) {
  MFWizardImpl* wizard = self->get_data();
  if (wizard != nil) {
    wizard->nextButton.enabled = flag;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_set_show_extra(mforms::Wizard *self, bool flag) {
  MFWizardImpl* wizard = self->get_data();
  if (wizard != nil) {
    wizard->extraButton.hidden = flag?NO:YES;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_set_extra_caption(mforms::Wizard *self, const std::string &caption) {
  MFWizardImpl* wizard = self->get_data();
  if (wizard != nil) {
    wizard->extraButton.title = wrap_nsstring(caption);
    [wizard->extraButton sizeToFit];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void wizard_set_next_caption(mforms::Wizard *self, const std::string &caption) {
  MFWizardImpl* wizard = self->get_data();
  if (wizard != nil) {
    if (caption.empty())
      wizard->nextButton.title = @"Continue";
    else
      wizard->nextButton.title = wrap_nsstring(caption);

    [wizard rearrangeButtons];
  }
}

//----------------------------------------------------------------------------------------------------------------------

void cf_wizard_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_wizard_impl.create= wizard_create;
  f->_wizard_impl.set_title= wizard_set_title;
  f->_wizard_impl.run_modal= wizard_run_modal;
  f->_wizard_impl.close= wizard_close;

  f->_wizard_impl.set_content= wizard_set_content;
  f->_wizard_impl.set_heading= wizard_set_heading;
  f->_wizard_impl.set_step_list= wizard_set_step_list;
  f->_wizard_impl.set_allow_cancel= wizard_set_allow_cancel;
  f->_wizard_impl.set_allow_back= wizard_set_allow_back;
  f->_wizard_impl.set_allow_next= wizard_set_allow_next;
  f->_wizard_impl.set_show_extra= wizard_set_show_extra;
  
  f->_wizard_impl.set_extra_caption= wizard_set_extra_caption;
  f->_wizard_impl.set_next_caption= wizard_set_next_caption;  
}

@end

//----------------------------------------------------------------------------------------------------------------------
