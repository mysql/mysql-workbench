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

#import "MFTabView.h"
#import "MFMForms.h"

#include "base/string_utilities.h"

@implementation MFTabViewItemView

- (NSView *)superview {
  return mTabView;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize {
  self.subviews.lastObject.frame = {{ 0, 0 }, self.frame.size };
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setEnabled: (BOOL)flag {
  [self.subviews.lastObject setEnabled: flag];
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize {
  // A tabview item usually has only one subview attached (the content view), so use this
  // to determine the minimum size.
  NSSize minSize = super.minimumSize;
  if (self.subviews.count == 0)
    return minSize;

  NSSize childMinSize = self.subviews[0].minimumSize;
  return { MAX(minSize.width, childMinSize.width), MAX(minSize.height, childMinSize.height) };
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityPageRole;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface DraggingTabView : NSTabView {
  mforms::TabView *mOwner;
  NSTrackingArea *mTrackingArea;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation DraggingTabView

- (instancetype)initWithFrame: (NSRect)frame owner: (mforms::TabView *)aTabView {
  self = [super initWithFrame: frame];
  if (self != nil) {
    mOwner = aTabView;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING(self) // Add standard mouse handling.
STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MFTabViewImpl

- (instancetype)initWithObject: (::mforms::TabView *)aTabView tabType: (mforms::TabViewType)tabType {
  self = [super initWithFrame: NSMakeRect(10, 10, 100, 100)];
  if (self) {
    BOOL tabSwitcherBelow = NO;
    mTabView = [[DraggingTabView alloc] initWithFrame: NSMakeRect(0, 0, 100, 100) owner: aTabView];

    switch (tabType) {
      case mforms::TabViewSystemStandard:
        break;

      case mforms::TabViewTabless:
        mTabView.tabViewType = NSNoTabsNoBorder;
        break;

      case mforms::TabViewMainClosable:
        mTabView.tabViewType = NSNoTabsNoBorder;
        mTabSwitcher = [[MTabSwitcher alloc] initWithFrame: NSMakeRect(0, 0, 100, 26)];
        mTabSwitcher.tabStyle = MMainTabSwitcher;
        [mTabSwitcher setTabView: mTabView];
        break;

      case mforms::TabViewDocument:
      case mforms::TabViewDocumentClosable:
        mTabView.tabViewType = NSNoTabsNoBorder;
        mTabSwitcher = [[MTabSwitcher alloc] initWithFrame: NSMakeRect(0, 0, 100, 26)];
        mTabSwitcher.tabStyle = MEditorTabSwitcher;
        [mTabSwitcher setTabView: mTabView];
        break;

      case mforms::TabViewPalette:
        mTabView.controlSize = NSControlSizeSmall;
        mTabView.font = [NSFont systemFontOfSize: [NSFont smallSystemFontSize]];
        break;

      case mforms::TabViewSelectorSecondary:
        mTabView.tabViewType = NSNoTabsNoBorder;
        mTabSwitcher = [[MTabSwitcher alloc] initWithFrame: NSMakeRect(0, 0, 100, 26)];
        mTabSwitcher.tabStyle = MSectionTabSwitcher;
        [mTabSwitcher setTabView: mTabView];
        break;

      case mforms::TabViewEditorBottom:
        mTabView.tabViewType = NSNoTabsNoBorder;
        mTabSwitcher = [[MTabSwitcher alloc] initWithFrame: NSMakeRect(0, 0, 100, 26)];
        mTabSwitcher.tabStyle = MEditorBottomTabSwitcher;
        tabSwitcherBelow = YES;
        [mTabSwitcher setTabView: mTabView];
        break;

      case mforms::TabViewEditorBottomPinnable:
        mTabView.tabViewType = NSNoTabsNoBorder;
        mTabSwitcher = [[MTabSwitcher alloc] initWithFrame: NSMakeRect(0, 0, 100, 26)];
        mTabSwitcher.tabStyle = MEditorBottomTabSwitcherPinnable;
        tabSwitcherBelow = YES;
        [mTabSwitcher setTabView: mTabView];
        break;

      default:
        throw std::runtime_error("mforms: invalid tab type: " + std::to_string(tabType));
        break;
    }
    [mTabView setDrawsBackground: NO];
    if (tabSwitcherBelow) {
      [self addSubview: mTabView];
      [self addSubview: mTabSwitcher];
    } else {
      [self addSubview: mTabSwitcher];
      [self addSubview: mTabView];
    }
    mExtraSize = mTabView.minimumSize;
    {
      NSRect contentRect = mTabView.contentRect;
      mExtraSize.width -= NSWidth(contentRect);
      mExtraSize.height -= NSHeight(contentRect);
    }
    mOwner = aTabView;
    mOwner->set_data(self);
    if (mTabSwitcher)
      mTabSwitcher.delegate = self;
    else
      mTabView.delegate = self;
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.

//----------------------------------------------------------------------------------------------------------------------

- (mforms::Object *)mformsObject {
  return mOwner;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSTabView *)tabView {
  return mTabView;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize {
  if (mOwner == NULL || mOwner->is_destroying())
    return super.minimumSize;

  NSSize minSize = super.minimumSize;

  for (NSTabViewItem *item in mTabView.tabViewItems) {
    NSSize size = item.view.minimumSize;

    minSize.width = MAX(minSize.width, size.width);
    minSize.height = MAX(minSize.height, size.height);
  }

  minSize.width += mExtraSize.width;
  minSize.height += mExtraSize.height;

  return minSize;
}

//----------------------------------------------------------------------------------------------------------------------

// necessary or rebuilding the UI won't work (test case: connection editor)
- (void)resizeSubviewsWithOldSize: (NSSize)oldBoundsSize {
  [super resizeSubviewsWithOldSize:oldBoundsSize];
  if (mTabSwitcher) {
    NSRect srect = mTabSwitcher.frame;
    NSRect rect = self.bounds;

    srect.size.width = NSWidth(rect);
    if (mOwner->get_type() == mforms::TabViewEditorBottom ||
        mOwner->get_type() == mforms::TabViewEditorBottomPinnable) {
      srect.origin.y = 0;
      rect.origin.y = NSHeight(srect);
    } else
      srect.origin.y = NSHeight(rect) - NSHeight(srect);
    mTabSwitcher.frame = srect;

    rect.size.height -= NSHeight(srect);
    mTabView.frame = rect;
  } else
    mTabView.frame = self.bounds;
  for (NSTabViewItem *item in mTabView.tabViewItems)
    [item.view resizeSubviewsWithOldSize:oldBoundsSize];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setEnabled: (BOOL)flag {
  for (NSTabViewItem *item in mTabView.tabViewItems) {
    MFTabViewItemView *itemView = (MFTabViewItemView *)item.view;
    [itemView setEnabled: flag];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isClosable {
  return
    mOwner->get_type() == mforms::TabViewEditorBottom
    || mOwner->get_type() == mforms::TabViewEditorBottomPinnable
    || mOwner->get_type() == mforms::TabViewDocumentClosable;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tabView: (NSTabView *)tabView didSelectTabViewItem: (NSTabViewItem *)tabViewItem {
  if (!mOwner->is_destroying())
    (*mOwner->signal_tab_changed())();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tabView: (NSTabView *)tabView didReorderTabViewItem: (NSTabViewItem *)tabViewItem toIndex: (NSInteger)index {
  MFTabViewItemView *itemView = (MFTabViewItemView *)tabViewItem.view;
  mOwner->reordered(itemView->mOwner, (int)index);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tabView: (NSTabView *)tabView willDisplayMenu: (NSMenu *)menu forTabViewItem: (NSTabViewItem *)tabViewItem {
  mOwner->set_menu_tab((int)[tabView indexOfTabViewItem:tabViewItem]);
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)tabView: (NSTabView *)tabView itemIsPinned: (NSTabViewItem *)item {
  if (mOwner->is_pinned)
    return mOwner->is_pinned((int)[tabView indexOfTabViewItem:item]);
  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)tabView: (NSTabView *)tabView itemHasCloseButton: (NSTabViewItem *)item {
  if ([self isClosable]) {
    if ([item.view isKindOfClass:MFTabViewItemView.class]) {
      MFTabViewItemView *view = (MFTabViewItemView *)item.view;
      return view->showCloseButton;
    }
    return YES;
  }
  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)tabView: (NSTabView *)tabView itemPinClicked: (NSTabViewItem *)item {
  int i = (int)[tabView indexOfTabViewItem:item];
  mOwner->pin_changed(i, !mOwner->is_pinned(i));
  [self setNeedsDisplay:YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)tabView: (NSTabView *)tabView willCloseTabViewItem: (NSTabViewItem *)item {
  if ([self isClosable]) {
    return mOwner->can_close_tab((int)[mTabView indexOfTabViewItem:item]);
  }
  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityGroupRole;
}

//----------------------------------------------------------------------------------------------------------------------

static bool tabview_create(::mforms::TabView *self, ::mforms::TabViewType tabType) {
  return [[MFTabViewImpl alloc] initWithObject:self tabType:tabType] != nil;
}

static void tabview_set_active_tab(::mforms::TabView *self, int tab) {
  if (self) {
    MFTabViewImpl *tabView = self->get_data();

    if (tabView) {
      [tabView->mTabView selectTabViewItem: [tabView->mTabView tabViewItemAtIndex:tab]];
    }
  }
}

static void tabview_set_tab_title(::mforms::TabView *self, int tab, const std::string &title) {
  if (self) {
    MFTabViewImpl *tabView = self->get_data();

    if (tabView) {
      [tabView->mTabView tabViewItemAtIndex: tab].label = wrap_nsstring(title);
      [tabView->mTabSwitcher setNeedsDisplay: YES];
    }
  }
}

static int tabview_get_active_tab(::mforms::TabView *self) {
  if (self) {
    MFTabViewImpl *tabView = self->get_data();

    if (tabView) {
      return (int)[tabView->mTabView indexOfTabViewItem: tabView->mTabView.selectedTabViewItem];
    }
  }
  return 0;
}

static int tabview_add_page(::mforms::TabView *self, mforms::View *tab, const std::string &label, bool hasCloseButton) {
  if (self) {
    MFTabViewImpl *tabView = self->get_data();

    if (tabView) {
      NSTabViewItem *item = [[NSTabViewItem alloc] initWithIdentifier: [NSString stringWithFormat: @"%p", tab]];
      MFTabViewItemView *view = [MFTabViewItemView new];

      view->mOwner = tab;
      view->mTabView = tabView->mTabView;
      view->showCloseButton = hasCloseButton;

      item.label = wrap_nsstring(label);
      item.view = view;

      [view addSubview:tab->get_data()];

      [tabView->mTabView addTabViewItem:item];

      if (tabView->mTabSwitcher && self->get_tab_menu())
        tabView->mTabSwitcher.menu = self->get_tab_menu()->get_data();

      return (int)tabView->mTabView.numberOfTabViewItems - 1;
    }
  }
  return -1;
}

static void tabview_remove_page(::mforms::TabView *self, mforms::View *tab) {
  if (self) {
    MFTabViewImpl *tabView = self->get_data();

    if (tabView) {
      NSInteger i = [tabView->mTabView indexOfTabViewItemWithIdentifier: [NSString stringWithFormat:@"%p", tab]];
      if (i != NSNotFound) {
        NSTabViewItem *item = [tabView->mTabView tabViewItemAtIndex:i];
        if (item) {
          MFTabViewItemView *view = (MFTabViewItemView *)item.view;
          [view.subviews.lastObject removeFromSuperview];

          [tabView->mTabView removeTabViewItem:item];
        }
      }
    }
  }
}

static void tabview_set_aux_view(::mforms::TabView *self, mforms::View *view) {
  if (self->get_type() != mforms::TabViewEditorBottom && self->get_type() != mforms::TabViewEditorBottomPinnable)
    throw std::invalid_argument("set_aux_view called for invalid Tab type");

  MFTabViewImpl *tabView = self->get_data();
  if (tabView) {
    [tabView->mTabSwitcher addSubview: nsviewForView(view)];
    // view->set_size(view->get_preferred_width(), NSHeight(tabView->mTabSwitcher.frame));
    view->set_position(self->get_width() - view->get_width(), 0);
  }
}

static void tabview_set_allow_reordering(::mforms::TabView *self, bool flag) {
  if (self->get_type() != mforms::TabViewEditorBottom && self->get_type() != mforms::TabViewEditorBottomPinnable &&
      self->get_type() != mforms::TabViewDocumentClosable)
    throw std::invalid_argument("TabView is not of a reorderable type");

  MFTabViewImpl *tabView = self->get_data();
  if (tabView != nil)
    tabView->mTabSwitcher.allowTabReordering = flag;
}

void cf_tabview_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_tabview_impl.create = &tabview_create;
  f->_tabview_impl.set_active_tab = &tabview_set_active_tab;
  f->_tabview_impl.get_active_tab = &tabview_get_active_tab;
  f->_tabview_impl.set_tab_title = &tabview_set_tab_title;
  f->_tabview_impl.add_page = &tabview_add_page;
  f->_tabview_impl.remove_page = &tabview_remove_page;
  f->_tabview_impl.set_aux_view = &tabview_set_aux_view;
  f->_tabview_impl.set_allows_reordering = &tabview_set_allow_reordering;
}

@end
