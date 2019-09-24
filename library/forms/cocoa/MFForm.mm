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

#import "MFMForms.h"
#import "MFForm.h"

@interface MFFormImpl () {
  mforms::Form *mOwner;
  BOOL mIsModal;
  NSWindow *mParentWindow;
  NSMenu *mOriginalMenu;
}
@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFFormImpl

- (instancetype)initWithObject: (mforms::Form *)form owner: (mforms::Form *)ownerWindow flags: (mforms::FormFlag)formFlags {

  NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;

  if ((formFlags & mforms::FormResizable) != 0)
    styleMask |= NSWindowStyleMaskResizable;

  if ((formFlags & mforms::FormMinimizable) != 0)
    styleMask |= NSWindowStyleMaskMiniaturizable;

  if ((formFlags & mforms::FormStayOnTop) != 0)
    styleMask |= NSWindowStyleMaskDocModalWindow;

  if ((formFlags & mforms::FormToolWindow) != 0)
    styleMask |= NSWindowStyleMaskUtilityWindow;

  self = [super
    initWithContentRect: NSMakeRect(100, 100, 100, 100)
              styleMask: styleMask
                backing: NSBackingStoreBuffered
                  defer: YES];
  if (self) {

    [self setHidesOnDeactivate: NO];
    [self setReleasedWhenClosed: NO]; // Don't use hide-on-close setting here. We manage the lifetime via mforms.

    if (ownerWindow) {
      id owner = ownerWindow->get_data();
      if ([owner isKindOfClass: [NSWindowController class]])
        mParentWindow = [owner window];
      else
        mParentWindow = owner;
    }

    mOwner = form;
    mOwner->set_data(self);
    self.delegate = self;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
}

//----------------------------------------------------------------------------------------------------------------------

- (mforms::Object *)mformsObject {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isHidden {
  return !self.visible;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setHidden: (BOOL)flag {
  if (!flag) {
    [self makeKeyAndOrderFront:nil];
  } else
    [self orderOut:nil];
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Computes the entire layout of the form.
 *
 * @param proposedSize The size to start from layouting.
 * @param resizeChildren Tells the function whether the computed client control bounds should be applied
 *                      (when doing a relayout) or not (when computing the preferred size).
 * Note: in opposition to Windows we don't need to apply the child size, because in an NSPanel the content view
 *       is always filling the entire panel. We keep this parameter only to keep the pattern for all computeLayout
 * functions.
 * @return The resulting size of the content view.
 */
- (NSSize)computeLayout: (NSSize)proposedSize resizeChildren: (BOOL)doResize {
  if (self.contentView != nil && doResize) {
    self.contentView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    return [self.contentView preferredSize: proposedSize]; // Includes the minimum size if needed.
  }
  return proposedSize;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)preferredSize: (NSSize)proposedSize {
  return [self computeLayout: proposedSize resizeChildren: NO];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize {
  [self computeLayout: self.frame.size resizeChildren: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)relayout {
  [self resizeSubviewsWithOldSize:self.frame.size];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setFrameSize: (NSSize)size {
  [self setContentSize: size];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)windowShouldClose: (id)sender {
  return mOwner->can_close();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)windowWillClose: (NSNotification *)notification {
  [self makeFirstResponder:nil];
  if (mIsModal)
    [NSApp stopModal];
  if (mOwner)
    mOwner->was_closed();

  // automatically set parent to key window status
  if (mParentWindow) {
    if (mParentWindow.visible)
      [mParentWindow makeKeyWindow];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)doCommandBySelector: (SEL)aSelector {
  if (aSelector == @selector(cancel:)) {
    // Don't close windows on Escape for non-modal windows
    if (!mIsModal)
      return;
  }

  [super doCommandBySelector:aSelector];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSInteger)runModal {
  NSInteger ret;
  mIsModal = YES;

  if (mParentWindow) {
    NSRect rect = mParentWindow.frame;
    NSSize size = self.frame.size;
    NSPoint pos;

    pos.x = rect.origin.x + (NSWidth(rect) - size.width) / 2;
    pos.y = rect.origin.y + (NSHeight(rect) - size.height) / 2;

    [self setFrameOrigin: pos];
  }

  [self makeKeyAndOrderFront: nil];
  ret = [NSApp runModalForWindow: self];

  // set 1st responder to nil to force textfields being edited to commit changes
  [self makeFirstResponder: nil];
  mIsModal = NO;

  return ret;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)endModal: (BOOL)ok {
  [NSApp stopModalWithCode: (ok ? NSModalResponseOK : NSModalResponseCancel)];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)destroy {
  [self setContentView: nil];
  mOwner = 0;
  [self close];
  //  [self release];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)windowDidBecomeKey: (NSNotification *)notification {
  if (mOwner->get_menubar()) {
    mOriginalMenu = NSApp.mainMenu;
    NSApp.mainMenu = mOwner->get_menubar()->get_data();
  }
  if (mOwner)
    mOwner->activated();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)windowDidResignKey: (NSNotification *)notification {
  if (mOriginalMenu) {
    NSApp.mainMenu = mOriginalMenu;
    mOriginalMenu = nil;
  }
  if (mOwner)
    mOwner->deactivated();
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityWindowRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

static bool form_create(mforms::Form *self, mforms::Form *owner, mforms::FormFlag flags) {
  return [[MFFormImpl alloc] initWithObject: self owner: owner flags: flags] != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void form_set_title(mforms::Form *self, const std::string &title) {
  id form = self->get_data();
  if (form) {
    [form setTitle: wrap_nsstring(title)];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void show_modal_button_action(void *form, mforms::Button *btn) {
  [(__bridge id)form makeFirstResponder: nil];
  if (form != NULL)
    [(__bridge id)form close];
}

//----------------------------------------------------------------------------------------------------------------------

static void form_show_modal(mforms::Form *self, mforms::Button *accept, mforms::Button *cancel) {
  id form = self->get_data();
  if (form) {
    if (accept) {
      [form setDefaultButtonCell: [accept->get_data() cell]];
      accept->signal_clicked()->connect(std::bind(show_modal_button_action, (__bridge void *)form, accept));
    }
    if (cancel)
      cancel->signal_clicked()->connect(std::bind(show_modal_button_action, (__bridge void *)form, cancel));

    [form makeKeyAndOrderFront:nil];
    [form performSelectorOnMainThread:@selector(runModal) withObject:nil waitUntilDone:YES];
    /// XXX this should not block
  }
}

//----------------------------------------------------------------------------------------------------------------------

static bool form_run_modal(mforms::Form *self, mforms::Button *accept, mforms::Button *cancel) {
  MFFormImpl *form = self->get_data();
  if (form) {
    if (accept) {
      form.defaultButtonCell = [accept->get_data() cell];
      accept->signal_clicked()->connect(std::bind(&mforms::Form::end_modal, self, true));
    }
    if (cancel)
      cancel->signal_clicked()->connect(std::bind(&mforms::Form::end_modal, self, false));

    long dialog_result = NSModalResponseCancel;
    if ([NSThread isMainThread])
      dialog_result = [form runModal];
    else {
      NSInvocation *invocation =
        [NSInvocation invocationWithMethodSignature: [form methodSignatureForSelector: @selector(runModal)]];
      invocation.target = form;
      invocation.selector = @selector(runModal);
      [invocation performSelectorOnMainThread: @selector(invoke) withObject: nil waitUntilDone: YES];
      [invocation getReturnValue: &dialog_result];
    }
    if (dialog_result == NSModalResponseOK) {
      [form close];
      return true;
    }
    [form close];
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

static void form_end_modal(mforms::Form *self, bool result) {
  [self->get_data() makeFirstResponder: nil];
  [self->get_data() endModal: result];
}

//----------------------------------------------------------------------------------------------------------------------

static void form_close(mforms::Form *self) {
  id form = self->get_data();
  if (form) {
    [form makeFirstResponder: nil];
    [form close];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void form_set_content(mforms::Form *self, mforms::View *child) {
  id form = self->get_data();
  if (form != nil) {
    NSView *content = child->get_data();
    NSSize formSize = [form frame].size;

    NSSize size = [content preferredSize: { formSize.width, formSize.height}];
    if (size.width > formSize.width)
      formSize.width = size.width;
    if (size.height > formSize.height)
      formSize.height = size.height;
    [form setFrameSize: size];
    [form setMinSize: size];
    [form setContentView: content];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void form_center(mforms::Form *self) {
  NSPanel *form = self->get_data();
  if (form) {
    [form center];
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void form_flush_events(mforms::Form *self) {
}

//----------------------------------------------------------------------------------------------------------------------

static void form_set_menubar(mforms::Form *self, mforms::MenuBar *menubar) {
  // nop
}

//----------------------------------------------------------------------------------------------------------------------

void cf_form_init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_form_impl.create = &form_create;
  f->_form_impl.close = &form_close;
  f->_form_impl.set_title = &form_set_title;
  f->_form_impl.show_modal = &form_show_modal;
  f->_form_impl.run_modal = &form_run_modal;
  f->_form_impl.end_modal = &form_end_modal;
  f->_form_impl.set_content = &form_set_content;
  f->_form_impl.flush_events = &form_flush_events;
  f->_form_impl.center = &form_center;
  f->_form_impl.set_menubar = &form_set_menubar;
}
