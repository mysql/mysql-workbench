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

#import "MFWizard.h"

#import "MFBase.h"
#import "MFMForms.h"

#import "MFTable.h"

@implementation MFWizardBox

- (void)subviewMinimumSizeChanged
{
  NSView *content= [[self subviews] lastObject];
  [content resizeSubviewsWithOldSize: [content frame].size];
}


@end


@implementation MFWizardImpl

- (id)initWithObject:(::mforms::Wizard*)aWizard
{
  self= [super init];
  if (self)
  {
    [NSBundle loadNibNamed:@"WizardWindow" owner:self];
    [[self window] setContentSize: NSMakeSize(900, 620)];
    [[self window] setMinSize: [[self window] frame].size];
    [[self window] center];
    
    mOriginalButtonWidth = NSWidth([nextButton frame]);
    
    mAllowCancel= YES;
    mOwner= aWizard;
    mOwner->set_data(self);
  }
  return self;
}


- (mforms::Object*)mformsObject
{
  return mOwner;
}


- (IBAction)performNext:(id)sender
{
  mOwner->next_clicked();
}


- (IBAction)performBack:(id)sender
{
  mOwner->back_clicked();
}


- (IBAction)performExtra:(id)sender
{
  mOwner->extra_clicked();
}


- (BOOL)windowShouldClose:(id)window
{
  if (mAllowCancel && (!mOwner->_cancel_slot || mOwner->_cancel_slot()))
  {
    [NSApp stopModal];
    return YES;
  }
  return NO;
}


- (void)setCurrentStep:(NSInteger)step
{
  BOOL unexecuted= NO;
  for (int i= 0; i < (int)[[stepList subviews] count]/2; i++)
  {
    if (i == step)
    {
      [[stepList viewWithTag: i] setImage:[NSImage imageNamed:@"DotBlue"]];
      unexecuted= YES;
    }
    else if (!unexecuted)
    {
      [[stepList viewWithTag: i] setImage:[NSImage imageNamed:@"DotGrey"]];
    }
    else
    {
      [[stepList viewWithTag: i] setImage:[NSImage imageNamed:@"DotDisabled"]];
    }
  }
}


- (void)setStepTitles:(NSArray*)titles
{
  float y;
  float width;
  int row= 0;
  NSImage *disabled= [NSImage imageNamed: @"DotDisabled"];
  NSImage *current= [NSImage imageNamed: @"DotBlue"];
  NSImage *executed= [NSImage imageNamed: @"DotGrey"];
  
  {
    int delcount= [titles count] - [[stepList subviews] count];
    
    if (delcount > 0)
    {
      // if there are more steps than needed, remove them
      for (id step in [[stepList subviews] reverseObjectEnumerator])
      {
        delcount--;
        [step removeFromSuperview];
        if (delcount == 0)
          break;
      }
    }
  }
  
  y= NSHeight([stepList frame]) - 18;
  width= NSWidth([stepList frame]);

  for (NSString *title in titles)
  {
    NSImageView *image;
    NSTextField *text;
   
    image= [stepList viewWithTag: row*2];
    text= [stepList viewWithTag: row*2+1];
    if (!image)
    {
      image= [[[NSImageView alloc] initWithFrame:NSMakeRect(0, y-1, 16, 16)] autorelease];
      [image setTag: row*2];
      [stepList addSubview: image];
      [image setAutoresizingMask: NSViewMinXMargin|NSViewMaxYMargin];
    }
    switch ([title characterAtIndex:0])
    {
      case '*': // current
        [image setImage: current];
        break;
        
      case '.': // executed
        [image setImage: executed];
        break;
        
      case '-': // disabled
      default:
        [image setImage: disabled];
        break;
    }
    if (!text)
    {
      text= [[[NSTextField alloc] initWithFrame:NSMakeRect(16, y, width, 16)] autorelease];
      [text setTag: row*2+1];
      [text setEditable: NO];
      [text setBordered: NO];
      [text setDrawsBackground: NO];
      [stepList addSubview: text];
      [text setAutoresizingMask: NSViewMinXMargin|NSViewMaxYMargin];
    }
    [text setStringValue: [title substringFromIndex: 1]];
    
    row++;
    y-= 25;
  }
}


- (void)rearrangeButtons
{
  float windowWidth = NSWidth([[[self window] contentView] frame]);
  NSRect frame;
  float nextX;

  // resize and rearrange next/back buttons to fit
  [nextButton sizeToFit];
  if (NSWidth([nextButton frame]) < mOriginalButtonWidth)
  {
    // enforce a min width
    frame = [nextButton frame];
    frame.size.width = mOriginalButtonWidth;
    [nextButton setFrame: frame];
  }
  else
  {
    frame = [nextButton frame];
    frame.size.width += 10;
  }
  nextX = frame.origin.x = windowWidth - NSWidth(frame) - 16;
  [nextButton setFrame: frame];
  
  frame = [backButton frame];
  frame.origin.x = nextX - 3 - NSWidth(frame);
  [backButton setFrame: frame];
}


static bool wizard_create(::mforms::Wizard *self, mforms::Form *owner)
{
  /*MFWizardImpl *wizard = */[[[MFWizardImpl alloc] initWithObject:self] autorelease];
    
  return true;
}


static void wizard_set_title(mforms::Wizard *self, const std::string &title)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      [[wizard window] setTitle:wrap_nsstring(title)];
    }
  }
}


static void wizard_run_modal(mforms::Wizard *self)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      [[wizard window] makeKeyAndOrderFront:nil];
      try
      {
        [NSApp runModalForWindow:[wizard window]];
      }
      catch (const std::exception &exc)
      {
        [NSApp stopModal];
        [[NSAlert alertWithMessageText: @"Unhandled Exception"
                         defaultButton: nil
                       alternateButton: nil
                           otherButton: nil
             informativeTextWithFormat: @"An unhandled exception has occurred while executing the wizard. "
          "Please restart Workbench at the first opportunity.\nException: %s", exc.what()] runModal];
      }
      [[wizard window] orderOut:nil];
    }
  }
}


static void wizard_close(mforms::Wizard *self)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      [NSApp stopModal];
    }
  }
}


static void wizard_set_content(mforms::Wizard *self, mforms::View *view)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      [[wizard->contentBox contentView] removeFromSuperview];
      if (view)
      {
        [wizard->contentBox setContentView: view->get_data()];
        [view->get_data() resizeSubviewsWithOldSize:[wizard->contentBox frame].size];
      }
    }
  }
}


static void wizard_set_heading(mforms::Wizard *self, const std::string &text)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      [wizard->headingText setStringValue:wrap_nsstring(text)];
    }
  }
}


static void wizard_set_step_list(mforms::Wizard *self, const std::vector<std::string> &steps)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      NSMutableArray *array= [NSMutableArray array];
      for (std::vector<std::string>::const_iterator step= steps.begin(); step != steps.end(); ++step)
      {
        [array addObject: wrap_nsstring(*step)];
      }
      [wizard setStepTitles: array];
    }
  }
}


static void wizard_set_allow_cancel(mforms::Wizard *self, bool flag)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      wizard->mAllowCancel= flag;
    }
  }
}


static void wizard_set_allow_back(mforms::Wizard *self, bool flag)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      [wizard->backButton setEnabled:flag];
    }
  }
}


static void wizard_set_allow_next(mforms::Wizard *self, bool flag)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      [wizard->nextButton setEnabled:flag];
    }
  }
}


static void wizard_set_show_extra(mforms::Wizard *self, bool flag)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      [wizard->extraButton setHidden:flag?NO:YES];
    }
  }
}


static void wizard_set_extra_caption(mforms::Wizard *self, const std::string &caption)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      [wizard->extraButton setTitle:wrap_nsstring(caption)];
      [wizard->extraButton sizeToFit];
    }
  }
}


static void wizard_set_next_caption(mforms::Wizard *self, const std::string &caption)
{
  if (self)
  {
    MFWizardImpl* wizard = self->get_data();
    
    if ( wizard )
    {
      if (caption.empty())
        [wizard->nextButton setTitle:@"Continue"];
      else
        [wizard->nextButton setTitle:wrap_nsstring(caption)];
      
      [wizard rearrangeButtons];
    }
  }
}



void cf_wizard_init()
{
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


