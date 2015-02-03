/* 
 * Copyright (c) 2009, 2015, Oracle and/or its affiliates. All rights reserved.
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

#import "MFMForms.h"
#import "MFForm.h"

@implementation MFFormImpl

- (instancetype)initWithObject:(::mforms::Form*)form
               owner:(::mforms::Form*)ownerWindow
{
  self= [super initWithContentRect:NSMakeRect(100, 100, 1,1)
                         styleMask:NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask|NSResizableWindowMask 
                           backing:NSBackingStoreBuffered 
                             defer:YES];
  if (self)
  {
   // [self setAutorecalculatesKeyViewLoop: YES];
    [self setHidesOnDeactivate: NO];
    [self setReleasedWhenClosed: NO];

    if (ownerWindow)
    {
      id owner = ownerWindow->get_data();
      if ([owner isKindOfClass: [NSWindowController class]])
        mParentWindow = [owner window];
      else
        mParentWindow = owner;
    }
    
    mOwner= form;
    mOwner->set_data(self);
    [self setDelegate:self];
  }
  return self;
}

//--------------------------------------------------------------------------------------------------

- (void)dealloc
{
  [mOriginalMenu release];
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  [super dealloc];
}

- (mforms::Object*)mformsObject
{
  return mOwner;
}


- (BOOL)isHidden
{
  return ![self isVisible];
}


- (void)setHidden:(BOOL)flag
{
  if (!flag)
  {
    [self makeKeyAndOrderFront:nil];
  }
  else
    [self orderOut:nil];
}


- (void)subviewMinimumSizeChanged
{
  NSSize size= [[self contentView] minimumSize];
  NSSize frameSize= [[self contentView] frame].size;
  BOOL flag= NO;

  if (frameSize.width < size.width)
  {
    frameSize.width= size.width;
    flag= YES;
  }

  if (frameSize.height < size.height)
  {
    frameSize.height= size.height;
    flag= YES;
  }

  if (flag)
    [self setContentSize:frameSize];
 // else
    [[self contentView] resizeSubviewsWithOldSize:frameSize];
  
  [self setContentMinSize:size];
}


- (void)setFrameSize:(NSSize)size
{
  [self setContentSize:size];
}

- (BOOL)windowShouldClose: (id)sender {
  return mOwner->can_close();
}

- (void)windowWillClose:(NSNotification *)notification
{
  [self makeFirstResponder: nil];
  if (mIsModal)
    [NSApp stopModal];
  if (mOwner)
    mOwner->was_closed();
  
  // automatically set parent to key window status
  if (mParentWindow)
  {
    if ([mParentWindow isVisible])
      [mParentWindow makeKeyWindow];
  }    
}


- (void)doCommandBySelector:(SEL)aSelector
{
  if (aSelector == @selector(cancel:))
  {
    // Don't close windows on Escape for non-modal windows
    if (!mIsModal)
      return;
  }
  
  [super doCommandBySelector: aSelector];
}


- (NSInteger)runModal
{
  NSInteger ret;
  mIsModal= YES;

  if (mParentWindow)
  {
    NSRect rect = [mParentWindow frame];
    NSSize size = [self frame].size;
    NSPoint pos;
    
    pos.x = rect.origin.x + (NSWidth(rect) - size.width) / 2;
    pos.y = rect.origin.y + (NSHeight(rect) - size.height) / 2;
    
    [self setFrameOrigin: pos];
  }
  
  [self makeKeyAndOrderFront:nil];
  ret= [NSApp runModalForWindow: self];
  
  // set 1st responder to nil to force textfields being edited to commit changes
  [self makeFirstResponder:nil];
  mIsModal= NO;
  
  return ret;
}


- (void)endModal:(BOOL)ok
{
  [NSApp stopModalWithCode: (ok ? NSOKButton : NSCancelButton)];
}


- (void)setFixedFrameSize:(NSSize)size
{
  NSSize curSize = [self frame].size;
  if (size.width < 0)
    size.width = curSize.width;
  if (size.height < 0)
    size.height = curSize.height;
  [self setFrameSize: size];
}


- (void)destroy
{
  [self setContentView: nil];
  mOwner = 0;
  [self close];
//  [self release];
}


- (void)windowDidBecomeKey:(NSNotification *)notification
{
  if (mOwner->get_menubar())
  {
    mOriginalMenu = [[NSApp mainMenu] retain];
    [NSApp setMainMenu: mOwner->get_menubar()->get_data()];
  }
  if (mOwner)
    mOwner->activated();
}


- (void)windowDidResignKey:(NSNotification *)notification
{
  if (mOriginalMenu)
  {
    [NSApp setMainMenu: mOriginalMenu];
    [mOriginalMenu release];
    mOriginalMenu = nil;
  }
  if (mOwner)
    mOwner->deactivated();
}

@end





static bool form_create(::mforms::Form *self, ::mforms::Form *owner, ::mforms::FormFlag flags)
{
 /* MFFormImpl *form=*/ [[[MFFormImpl alloc] initWithObject:self
                                                  owner:owner] autorelease];
    
  return true;
}


static void form_set_title(::mforms::Form *self, const std::string &title)
{
  id form = self->get_data();
  if (form)
  {
    [form setTitle:wrap_nsstring(title)];
  }
}


static void show_modal_button_action(id form, ::mforms::Button *btn)
{
  [form makeFirstResponder:nil];
  if (form)
    [form close];
}


static void form_show_modal(::mforms::Form *self, ::mforms::Button *accept, ::mforms::Button *cancel)
{
  id  form = self->get_data();
  if (form)
  {
    if (accept)
    {
      [form setDefaultButtonCell:[accept->get_data() cell]];
      accept->signal_clicked()->connect(boost::bind(show_modal_button_action, form, accept));
    }
    if (cancel)
      cancel->signal_clicked()->connect(boost::bind(show_modal_button_action, form, cancel));
    
    [form makeKeyAndOrderFront:nil];
    [form performSelectorOnMainThread: @selector(runModal) withObject: nil waitUntilDone: YES];
    ///XXX this should not block
  }
}


static bool form_run_modal(::mforms::Form *self, ::mforms::Button *accept, ::mforms::Button *cancel)
{
  MFFormImpl *form = self->get_data();
  if (form)
  {    
    if (accept)
    {
      [form setDefaultButtonCell:[accept->get_data() cell]];
      accept->signal_clicked()->connect(boost::bind(&::mforms::Form::end_modal, self, true));
    }
    if (cancel)
      cancel->signal_clicked()->connect(boost::bind(&::mforms::Form::end_modal, self, false));
    
    int dialog_result = NSCancelButton;
    if ([NSThread isMainThread])
      dialog_result = [form runModal];
    else
    {
      NSInvocation* invocation = [NSInvocation invocationWithMethodSignature: [form methodSignatureForSelector: @selector(runModal)]];
      [invocation setTarget: form];
      [invocation setSelector: @selector(runModal)];
      [invocation performSelectorOnMainThread: @selector(invoke) withObject: nil waitUntilDone: YES];
      [invocation getReturnValue: &dialog_result];
    }
    if (dialog_result == NSOKButton)
    {
      [form close];
      return true;
    }
    [form close];
  }
  return false;
}


static void form_end_modal(::mforms::Form *self, bool result)
{
  [self->get_data() makeFirstResponder:nil];
  [self->get_data() endModal: result];
}


static void form_close(::mforms::Form *self)
{
  id  form = self->get_data();
  if (form)
  {
    [form makeFirstResponder: nil];
    [form close];
  }
}


static void form_set_content(::mforms::Form *self, ::mforms::View *child)
{
  id  form = self->get_data();
  if (form)
  {
    [form setContentView:child->get_data()];
    [form subviewMinimumSizeChanged];
  }
}


static void form_center(::mforms::Form *self)
{
  id  form = self->get_data();
  if (form)
  {
    [form center];
  }
}



static void form_flush_events(::mforms::Form *self)
{
  
}


static void form_set_menubar(mforms::Form *self, mforms::MenuBar *menubar)
{
  // nop
}


void cf_form_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_form_impl.create     = &form_create;
  f->_form_impl.close      = &form_close;
  f->_form_impl.set_title  = &form_set_title;
  f->_form_impl.show_modal = &form_show_modal;
  f->_form_impl.run_modal = &form_run_modal;
  f->_form_impl.end_modal = &form_end_modal;
  f->_form_impl.set_content= &form_set_content;
  f->_form_impl.flush_events= &form_flush_events;
  f->_form_impl.center= &form_center;
  f->_form_impl.set_menubar = &form_set_menubar;
}

