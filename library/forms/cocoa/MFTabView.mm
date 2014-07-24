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

#import "MFTabView.h"
#import "MFMForms.h"

@implementation MFTabViewItemView

- (NSView*)superview
{
  return mTabView;
}


- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  NSRect frame= NSMakeRect(0, 0, 0, 0);
  
  frame.size= [self frame].size;
  
  if (NSEqualRects(frame, [[[self subviews] lastObject] frame]))
    [[[self subviews] lastObject] resizeSubviewsWithOldSize: oldBoundsSize];
  else
    [[[self subviews] lastObject] setFrame: frame];
}


- (void)setEnabled:(BOOL)flag
{
  [[[self subviews] lastObject] setEnabled: flag];
}

- (NSSize)minimumSize
{
  // A tabview item usually has only one subview attached (the content view), so use this
  // to determine the minimum size.
  if ([[self subviews] count] == 0)
    return NSMakeSize(0, 0);
  return [[[self subviews] objectAtIndex: 0] minimumSize];
}

@end

@interface DraggingTabView : NSTabView
{
  mforms::TabView *mOwner;
  NSTrackingArea *mTrackingArea;
}

@end

@implementation DraggingTabView

- (id)initWithFrame: (NSRect)frame owner: (mforms::TabView *)aTabView
{
  self = [super initWithFrame: frame];
  if (self != nil)
  {
    mOwner = aTabView;
  }
  return self;
}

//--------------------------------------------------------------------------------------------------

STANDARD_MOUSE_HANDLING(self) // Add standard mouse handling.
STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.

//--------------------------------------------------------------------------------------------------

@end

@implementation MFTabViewImpl

- (id)initWithObject:(::mforms::TabView*)aTabView tabType:(mforms::TabViewType)tabType
{
  self = [super initWithFrame:NSMakeRect(10, 10, 100, 100)];
  if (self)
  {
    BOOL tabSwitcherBelow = NO;
    mTabView = [[[DraggingTabView alloc] initWithFrame: NSMakeRect(0, 0, 100, 100) owner: aTabView] autorelease];

    switch (tabType)
    {
      case mforms::TabViewSystemStandard:
        break;

      case mforms::TabViewTabless:
        [mTabView setTabViewType: NSNoTabsNoBorder];
        break;

      case mforms::TabViewMainClosable:
        [mTabView setTabViewType: NSNoTabsNoBorder];
        mTabSwitcher = [[MTabSwitcher alloc] initWithFrame: NSMakeRect(0, 0, 100, 26)];
        [mTabSwitcher setTabStyle: MMainTabSwitcher];
        [mTabSwitcher setTabView: mTabView];
        break;

      case mforms::TabViewDocument:
      case mforms::TabViewDocumentClosable:
        [mTabView setTabViewType: NSNoTabsNoBorder];
        mTabSwitcher = [[MTabSwitcher alloc] initWithFrame: NSMakeRect(0, 0, 100, 26)];
        [mTabSwitcher setTabStyle: MEditorTabSwitcher];
        [mTabSwitcher setTabView: mTabView];
        break;

      case mforms::TabViewPalette:
        [mTabView setControlSize: NSSmallControlSize];
        [mTabView setFont: [NSFont systemFontOfSize: [NSFont smallSystemFontSize]]];
        break;
        
      case mforms::TabViewSelectorSecondary:
        [mTabView setTabViewType: NSNoTabsNoBorder];
        mTabSwitcher = [[MTabSwitcher alloc] initWithFrame: NSMakeRect(0, 0, 100, 26)];
        [mTabSwitcher setTabStyle: MPaletteTabSwitcherSmallText];
        [mTabSwitcher setTabView: mTabView];
        break;

      case mforms::TabViewEditorBottom:
        [mTabView setTabViewType: NSNoTabsNoBorder];
        mTabSwitcher = [[MTabSwitcher alloc] initWithFrame: NSMakeRect(0, 0, 100, 26)];
        [mTabSwitcher setTabStyle: MEditorBottomTabSwitcher];
        tabSwitcherBelow = YES;
        [mTabSwitcher setTabView: mTabView];
        break;
    }
    [mTabView setDrawsBackground: NO];
    if (tabSwitcherBelow)
    {
      [self addSubview: mTabView];
      [self addSubview: mTabSwitcher];
    }
    else
    {
      [self addSubview: mTabSwitcher];
      [self addSubview: mTabView];
    }
    mExtraSize = [mTabView minimumSize];
    {
      NSRect contentRect = [mTabView contentRect];
      mExtraSize.width -= NSWidth(contentRect);
      mExtraSize.height -= NSHeight(contentRect);
    }
    mOwner= aTabView;
    mOwner->set_data(self);
    if (mTabSwitcher)
      [mTabSwitcher setDelegate: self];
    else
      [mTabView setDelegate:self];
  }
  return self;
}

STANDARD_FOCUS_HANDLING(self) // Notify backend when getting first responder status.

- (mforms::Object*)mformsObject
{
  return mOwner;
}

- (NSTabView*)tabView
{
  return mTabView;
}

- (NSSize)minimumSize
{
  if (mOwner == NULL || mOwner->is_destroying())
    return NSZeroSize;
  
  NSSize minSize= NSZeroSize;
  
  for (NSTabViewItem *item in [mTabView tabViewItems])
  {
    NSSize size= [[item view] minimumSize];
    
    minSize.width= MAX(minSize.width, size.width);
    minSize.height= MAX(minSize.height, size.height);
  }
  
  minSize.width += mExtraSize.width;
  minSize.height += mExtraSize.height;
    
  return minSize;
}

// necessary or rebuilding the UI won't work (test case: connection editor)
- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  [super resizeSubviewsWithOldSize: oldBoundsSize];
  if (mTabSwitcher)
  {
    NSRect srect = [mTabSwitcher frame];
    NSRect rect = [self bounds];

    srect.size.width = NSWidth(rect);
    if (mOwner->get_type() == mforms::TabViewEditorBottom)
    {
      srect.origin.y = 0;
      rect.origin.y = NSHeight(srect);
    }
    else
      srect.origin.y = NSHeight(rect) - NSHeight(srect);
    [mTabSwitcher setFrame: srect];

    rect.size.height -= NSHeight(srect);
    [mTabView setFrame: rect];
  }
  else
    [mTabView setFrame: [self bounds]];
  for (NSTabViewItem *item in [mTabView tabViewItems])
    [[item view] resizeSubviewsWithOldSize: oldBoundsSize];

  if (mforms::View *view = mOwner->get_aux_view())
  {
    view->set_size(view->get_preferred_width(), NSHeight([mTabSwitcher frame]));
    view->set_position(mOwner->get_width() - view->get_width() - 11, 0);
  }
}

- (void)setEnabled:(BOOL)flag
{
  for (NSTabViewItem *item in [mTabView tabViewItems])
    [[item view] setEnabled: flag];
}


- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
  if (!mOwner->is_destroying())
    (*mOwner->signal_tab_changed())();
}


- (void)tabView:(NSTabView*)tabView didReorderTabViewItem:(NSTabViewItem *)tabViewItem toIndex:(NSInteger)index
{
  MFTabViewItemView *itemView = [tabViewItem view];
  mOwner->reordered(itemView->mOwner, index);
}


- (void)tabView:(NSTabView*)tabView willDisplayMenu:(NSMenu*)menu forTabViewItem:(NSTabViewItem *)tabViewItem
{
  mOwner->set_menu_tab([tabView indexOfTabViewItem: tabViewItem]);
}


- (BOOL)tabView:(NSTabView*)tabView itemHasCloseButton:(NSTabViewItem*)item
{
  if (mOwner->get_type() == mforms::TabViewEditorBottom || mOwner->get_type() == mforms::TabViewDocumentClosable)
    return YES;
  return NO;
}


- (BOOL)tabView:(NSTabView*)tabView willCloseTabViewItem:(NSTabViewItem*)item
{
  if (mOwner->get_type() == mforms::TabViewEditorBottom || mOwner->get_type() == mforms::TabViewDocumentClosable)
  {
    return mOwner->can_close_tab([mTabView indexOfTabViewItem: item]);
  }
  return NO;
}

static bool tabview_create(::mforms::TabView *self, ::mforms::TabViewType tabType)
{
  [[[MFTabViewImpl alloc] initWithObject:self tabType:tabType] autorelease];
    
  return true;  
}


static void tabview_set_active_tab(::mforms::TabView *self, int tab)
{
  if ( self )
  {
    MFTabViewImpl* tabView = self->get_data();
    
    if ( tabView )
    {
      [tabView->mTabView selectTabViewItem: [tabView->mTabView tabViewItemAtIndex:tab]];
    }
  }
}


static void tabview_set_tab_title(::mforms::TabView *self, int tab, const std::string &title)
{
  if ( self )
  {
    MFTabViewImpl* tabView = self->get_data();
    
    if ( tabView )
    {
      [[tabView->mTabView tabViewItemAtIndex:tab] setLabel: wrap_nsstring(title)];
      [tabView->mTabSwitcher setNeedsDisplay: YES];
    }
  }
}


static int tabview_get_active_tab(::mforms::TabView *self)
{
  if ( self )
  {
    MFTabViewImpl* tabView = self->get_data();
    
    if ( tabView )
    {
      return [tabView->mTabView indexOfTabViewItem: [tabView->mTabView selectedTabViewItem]];
    }
  }
  return 0;
}


static int tabview_add_page(::mforms::TabView *self, mforms::View *tab, const std::string &label)
{
  if ( self )
  {
    MFTabViewImpl* tabView = self->get_data();
    
    if ( tabView )
    {
      NSTabViewItem *item= [[[NSTabViewItem alloc] initWithIdentifier: [NSString stringWithFormat:@"%p", tab]] autorelease];
      MFTabViewItemView *view= [[MFTabViewItemView alloc] init];

      view->mOwner = tab;
      view->mTabView= tabView->mTabView;
      
      [item setLabel: wrap_nsstring(label)];
      [item setView: view];
      
      [view addSubview: tab->get_data()];
      
      [tabView->mTabView addTabViewItem: item];

      if (tabView->mTabSwitcher && self->get_tab_menu())
        [tabView->mTabSwitcher setMenu: self->get_tab_menu()->get_data()];
      
      return [tabView->mTabView numberOfTabViewItems]-1;
    }
  }
  return -1;
}


static void tabview_remove_page(::mforms::TabView *self, mforms::View *tab)
{
  if (self)
  {
    MFTabViewImpl* tabView = self->get_data();
    
    if (tabView)
    {
      NSInteger i= [tabView->mTabView indexOfTabViewItemWithIdentifier: [NSString stringWithFormat:@"%p", tab]];
      if (i != NSNotFound)
      {
        NSTabViewItem *item= [tabView->mTabView tabViewItemAtIndex: i];
        if (item)
        {
          MFTabViewItemView *view= [item view];
          [[[view subviews] lastObject] removeFromSuperview];
          [view release];
          
          [tabView->mTabView removeTabViewItem: item];
        }      
      }
    }
    else
      NSLog(@"Attempt to remove invalid mforms tabview page");
  }
}


static void tabview_set_aux_view(::mforms::TabView *self, mforms::View *view)
{
  if (self->get_type() != mforms::TabViewEditorBottom)
    throw std::invalid_argument("set_aux_view called for invalid Tab type\n");

  MFTabViewImpl* tabView = self->get_data();
  if (tabView)
  {
    [tabView->mTabSwitcher addSubview: nsviewForView(view)];
    view->set_size(view->get_preferred_width(), NSHeight([tabView->mTabSwitcher frame]));
    view->set_position(self->get_width() - view->get_width(), 0);
  }
}


static void tabview_set_allow_reordering(::mforms::TabView *self, bool flag)
{
  if (self->get_type() != mforms::TabViewEditorBottom || self->get_type() == mforms::TabViewDocumentClosable)
    throw std::invalid_argument("TabView is not of a reorderable type\n");

  MFTabViewImpl* tabView = self->get_data();
  if (tabView)
  {
    [tabView->mTabSwitcher setAllowTabReordering: flag];
  }
}


void cf_tabview_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_tabview_impl.create= &tabview_create;
  f->_tabview_impl.set_active_tab= &tabview_set_active_tab;
  f->_tabview_impl.get_active_tab= &tabview_get_active_tab;
  f->_tabview_impl.set_tab_title= &tabview_set_tab_title;
  f->_tabview_impl.add_page= &tabview_add_page;
  f->_tabview_impl.remove_page= &tabview_remove_page;
  f->_tabview_impl.set_aux_view= &tabview_set_aux_view;
  f->_tabview_impl.set_allows_reordering= &tabview_set_allow_reordering;
}

@end
