/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include "wb_context.h"

#import "WBMainWindow.h"
#import "WBMainController.h"
#import "WBOverviewPanel.h"
#import "WBModelOverviewPanel.h"
#import "WBModelDiagramPanel.h"
#import "WBMFormsPluginPanel.h"
#import "WBPluginPanel.h"
#import "MCanvasViewer.h"
#import "MCanvasScrollView.h"
#import "MContainerView.h"
#import "WBTabView.h"
#import "WBTabItem.h"
#import "WBSplitView.h"
#import "WBSplitViewUnbrokenizerDelegate.h"
#import "WBSQLQueryPanel.h"
#import "WBPluginEditorBase.h"
#import "MCPPUtilities.h"
#import "MTabSwitcher.h"
#include "workbench/wb_context_ui.h"
#include "model/wb_context_model.h"
#include "model/wb_model_diagram_form.h"
#include "model/wb_overview_physical.h"

#import "WBMainWindow_Model.h"
#include "base/string_utilities.h"
#include "base/log.h"
#include "base/notifications.h"

#include "mforms/toolbar.h"
#include "mforms/form.h"

DEFAULT_LOG_DOMAIN("Workbench")

@interface MyAutoreleasePool : NSAutoreleasePool
{
}
@end

@implementation MyAutoreleasePool
// avoid NSInvalidArgumentException -- *** -[NSAutoreleasePool retain]: Cannot retain an autorelease pool
- (id)retain
{
  return self;
}
@end



class MacNotificationObserver : base::Observer
{
  WBMainWindow *mainWindow;
public:
  MacNotificationObserver(WBMainWindow *window)
  : mainWindow(window)
  {
    base::NotificationCenter::get()->add_observer(this, "GNFormTitleDidChange");
  }
  
  virtual ~MacNotificationObserver()
  {
    base::NotificationCenter::get()->remove_observer(this);
  }
  
  virtual void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) 
  {
    if (name == "GNFormTitleDidChange")
    {
      WBBasePanel *panel = [mainWindow findMainPanelForUIForm: bec::UIForm::form_with_id(info["form"])];
      if (panel)
        [mainWindow setTitle: [NSString stringWithCPPString: info["title"]]
                    forPanel: panel];
    }
  }
};



@implementation WBWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)aStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag
{
  self = [super initWithContentRect: contentRect styleMask: aStyle backing: bufferingType defer: flag];
  if (self)
  {
    // setup pointer to main window in mforms
    NSInteger rc = [self retainCount];
    mforms::Form::main_form()->set_data(self);
    if ((int)[self retainCount] > rc) // don't let the mforms placeholder retain us
      [self release];
    
    //[self setBackgroundColor: [NSColor whiteColor]];
  }
  return self;
}

/** Override to catch changes to firstResponders
 *
 * This method is overriden so that we can notify the backend when the 
 * first responder is changed, to know when the active 'form' changes, which 
 * would trigger updates to the menus, toolbars etc.
 */
- (BOOL)makeFirstResponder:(NSResponder *)responder
{
  BOOL changed= [super makeFirstResponder: responder];
  if (changed)
    [[self windowController] firstResponderChanged: responder];

  return changed;
}


- (void)becomeKeyWindow
{
  mforms::Form::main_form()->activated();
  [super becomeKeyWindow];
}

- (void)resignKeyWindow
{
  [super resignKeyWindow];
  mforms::Form::main_form()->deactivated();
}

@end


// mforms integration
void setup_mforms_app(WBMainWindow *mwin);

@implementation WBMainWindow


- (void)setOwner:(WBMainController*)owner
{
  _mainController= owner;
}

- (WBMainController*)owner
{
  return _mainController;
}

- (void)setWBContext:(wb::WBContextUI*)wbui
{
  _backendObserver = new MacNotificationObserver(self);
  
  _wbui= wbui;
  
  setup_mforms_app(self);
    
//  [(WBToolbarTabView*)topTabView setAllowsTabReordering:NO];
    
//  _wbui->get_wb()->get_grt()->get_undo_manager()->signal_changed().connect(sigc::bind(sigc::ptr_fun(history_changed), self));
}


- (wb::WBContextUI*)context
{
  return _wbui;
}


/** Called after the window is shown on screen
 * The amount of work done here should be minimal, so that there isn't any visible window
 * relayouting.
 */
- (void)setupReady
{  
  [[self window] setTitle:[NSString stringWithUTF8String:_wbui->get_title().c_str()]];

  [self setupModel];
}


- (NSTabViewItem*)addTopPanel:(WBBasePanel*)panel
{
  id tabItem= [[[NSTabViewItem alloc] initWithIdentifier:[panel identifier]] autorelease];

  if ([panel title])
    [tabItem setView:[panel decoratedTopView]];
  else
    [tabItem setView:[panel topView]]; // no toolbar for Home tab
  [tabItem setLabel:[panel title]];
  if ([panel respondsToSelector: @selector(initialFirstResponder)])
    [tabItem setInitialFirstResponder: [(id)panel initialFirstResponder]];
  
  [_panels setObject:panel forKey:[panel identifier]];
  
  [topTabView addTabViewItem:tabItem];

//  NSImage *icon= [panel tabIcon];
//  if (icon)
//    [(id)topTabView setIcon:icon forTabViewItem:[panel identifier]];
    
  return tabItem;
}

- (NSTabViewItem*)addTopPanelAndSwitch:(WBBasePanel*)panel
{
  NSTabViewItem* item= [self addTopPanel: panel];
  [topTabView selectTabViewItemWithIdentifier: [item identifier]]; 
  //[topTabView selectLastTabViewItem:nil];
  return item;
}


- (void)addBottomPanel:(WBBasePanel*)panel
{
  WBBasePanel *mainPanel = [self selectedTopPanel];
  if ([mainPanel respondsToSelector: @selector(addEditor:)])
    [(id)mainPanel addEditor: panel];
  else
    NSLog(@"No valid main-tab is selected to receive the bottom panel");
}


- (void)switchToDiagramWithIdentifier:(const char*)identifier
{
  NSString *ident= [NSString stringWithCPPString: identifier];
  id form= [_panels objectForKey: ident]; 
  
  // check if the diagram was closed
  for (NSTabViewItem *item in _closedTopTabs)
  {
    if ([[item identifier] isEqual: ident])
    {
      [topTabView addTabViewItem:item];
      [topTabView selectLastTabViewItem:nil];
      [_closedTopTabs removeObject:item];
        
      if ([form respondsToSelector:@selector(didOpen)])
        [form didOpen];
      
      return;
    }
  }
  
  [topTabView selectTabViewItemWithIdentifier:ident];
}


- (void)updateBackendTimer
{
  [_backendTimer invalidate];
  _backendTimer= 0;

  double interval= _wbui->get_wb()->get_grt_manager()->delay_for_next_timeout();
  
  if (interval >= 0.0)
    _backendTimer= [NSTimer scheduledTimerWithTimeInterval:interval
                                                    target:self
                                                  selector:@selector(fireBackendTimer:)
                                                  userInfo:nil
                                                   repeats:NO];
}


- (void)fireBackendTimer:(NSTimer*)timer
{
  _wbui->get_wb()->get_grt_manager()->flush_timers();

  [self updateBackendTimer];
}



- (void)refreshGUI:(wb::RefreshType)type
         argument1:(const std::string&)arg1
         argument2:(NativeHandle)arg2
{
  const char* RefreshTypeStr[] = {
    "RefreshNeeded",
    "RefreshNothing",
    "RefreshSchemaNoReload",
    "RefreshNewView",
    "RefreshSelection",
    "RefreshMessages",
    "RefreshCloseEditor", // argument: object-id (close all if "")
    "RefreshFindOutput",
    "RefreshNewModel",
    "RefreshOverviewNodeInfo",    // argument: node-id
    "RefreshOverviewNodeChildren",// argument: node-id
    "RefreshViewName",
    "RefreshDocument",
    "RefreshCloseDocument",
    "RefreshZoom",
    "RefreshTimer",
    "RefreshFinishEdits"
  };
  
  switch (type)
  {
    case wb::RefreshNeeded:
      [_mainController requestRefresh];
      break;
      
    case wb::RefreshDocument:
      // refresh the title of the document
      [[self window] setTitle:[NSString stringWithUTF8String:_wbui->get_title().c_str()]];
      [[self window] setRepresentedFilename:[NSString stringWithUTF8String:_wbui->get_wb()->get_filename().c_str()]];
      [[self window] setDocumentEdited:_wbui->get_wb()->has_unsaved_changes()];
      break;

    case wb::RefreshNewModel:
      [self handleModelCreated];
      _wbui->get_wb()->new_model_finish();
      break;
      
    case wb::RefreshCloseDocument:
      [self closeEditorsMatching:nil]; // close all editors

      // flush anything waiting to be executed 
      _wbui->get_wb()->flush_idle_tasks();
  
      _wbui->get_wb()->close_document_finish();
      
      [self handleModelClosed];

      [[self window] setTitle:[NSString stringWithUTF8String:_wbui->get_title().c_str()]];
      break;
      
    case wb::RefreshOverviewNodeInfo:
      try
      {
        if (!arg2 || arg2 == [_physicalOverview formBE])
          [[_physicalOverview overview] refreshNode: bec::NodeId(arg1)];
      }
      catch (std::exception &exc)
      {
        NSLog(@"exception caught refreshing overview '%p' node: %s", arg2, exc.what());
      }
      break;

    case wb::RefreshOverviewNodeChildren:
      try
      {
        if (!arg2 || arg2 == [_physicalOverview formBE])
        {
          if (arg1.empty())
          {
            NSInteger i= [topTabView indexOfTabViewItemWithIdentifier: [_physicalOverview identifier]];
            if (i != NSNotFound)
              [[topTabView tabViewItemAtIndex: i] setLabel: [_physicalOverview title]];
          }
          [[_physicalOverview overview] refreshNodeChildren: bec::NodeId(arg1)];
        }
        //else
        //  NSLog(@"invalid %p", arg2);
      }
      catch (std::exception &exc)
      {
        NSLog(@"exception caught refreshing overview '%p' node children: %s", arg2, exc.what());
      }
      break;
      
    case wb::RefreshNewDiagram:
    {
      id form= [_panels objectForKey: [NSString stringWithUTF8String: arg1.c_str()]];
      if (![form isClosed])
        [self switchToDiagramWithIdentifier: arg1.c_str()];
      break;
    }
      
    case wb::RefreshSchemaNoReload:
      break;

    case wb::RefreshZoom:
    {
      id panel = [self selectedTopPanel];
      if ([panel respondsToSelector:@selector(refreshZoom)])
        [panel refreshZoom];
      break;
    }
    case wb::RefreshSelection:
    {
      id panel = [self selectedTopPanel];
      if ([panel respondsToSelector:@selector(selectionChanged)])
        [panel selectionChanged];
      break;
    } 
    case wb::RefreshCloseEditor:
      [self closeEditorsMatching: arg1.empty() ? nil : [NSString stringWithCPPString: arg1]];
      break;
      
    case wb::RefreshTimer:
      [self updateBackendTimer];
      break;
      
    case wb::RefreshFinishEdits:
    {
      NSWindow *window = [NSApp keyWindow];
      // remove focus from active textfield to force any changes to get commited then restore it
      id oldResponder = [window firstResponder];
      
      // if the 1st responder is a field editor, the actual 1st responder is its delegate (ie, the NSTextField)
      if (oldResponder != nil && [oldResponder isKindOfClass:[NSTextView class]] && [oldResponder isFieldEditor])
        oldResponder= [[oldResponder delegate] isKindOfClass:[NSResponder class]] ? [oldResponder delegate] : nil;
      
      if ([oldResponder isKindOfClass: [NSTableView class]] || [oldResponder isKindOfClass: [NSTextView class]])
      {
        [window makeFirstResponder: nil];
        [window makeFirstResponder: oldResponder];
      }
      break;
    }
    default:
      NSLog(@"unhandled %s", RefreshTypeStr[type]);
      break;
  }
}


- (void)setStatusText:(NSString*)text
{  
  [statusBarText setStringValue:text];
//  [statusBarText sizeToFit];
  [statusBarText display];
  
  [text release];
}


- (void)blockGUI:(BOOL)lock
{
}


- (mdc::CanvasView*)createView:(const char*)oid
                          name:(const char*)name
{
  wb::ModelDiagramForm *dform= _wbui->get_wb()->get_model_context()->get_diagram_form_for_diagram_id(oid);
  WBModelDiagramPanel *panel= [[WBModelDiagramPanel alloc] initWithId: [NSString stringWithUTF8String:oid]
                                                               formBE: dform];

  NSTabViewItem* item= [self addTopPanel:panel];
  [panel release];
  
  if (dform->is_closed())
  {
    [topTabView removeTabViewItem: item];
    [_closedTopTabs addObject: item];
  }
  return [panel canvas];
}


- (void)destroyView:(mdc::CanvasView*)view
{
  NSString *identifier= [NSString stringWithUTF8String:view->get_tag().c_str()];
  WBModelDiagramPanel *panel= [_panels objectForKey:identifier];

  if (panel)
  {
    NSUInteger i= [topTabView indexOfTabViewItemWithIdentifier:identifier];
    if (i != NSNotFound)
    {
      NSTabViewItem *item= [topTabView tabViewItemAtIndex:i];    
      [topTabView removeTabViewItem:item];
    }
    [_panels removeObjectForKey:identifier];
  }

  for (NSTabViewItem *item in [_closedTopTabs reverseObjectEnumerator])
  {
    if ([[item identifier] isEqual: identifier])
    {
      [_closedTopTabs removeObject: item];
      break;
    }
  }
}


- (void) tabViewDidChangeNumberOfTabViewItems:(NSTabView *)TabView
{
  if (TabView == topTabView)
    [tabSwitcher setNeedsDisplay:YES];
}


- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
  id panel = [_panels objectForKey: [tabViewItem identifier]];

  [self activatePanel: panel];
  if (tabView == topTabView)
  {
    NSMenu *menu = [panel menuBar];
    if (menu)
      [NSApp setMainMenu: menu];
    else
      [NSApp setMainMenu: _defaultMainMenu];

    mforms::App::get()->view_switched();

    [tabSwitcher setNeedsDisplay:YES];
  }
  
  if ([panel respondsToSelector: @selector(didActivate)])
    [panel didActivate];
  else
  {
    // look in the subviews of the tabView for the topone one that can become firstResponder
    // and then make it 
    id view= [tabViewItem view];
    NSMutableArray *viewStack= [NSMutableArray array];
    NSView *topMostView= nil; // the editable view that's at the topmost position
    CGFloat topMostViewY= MAXFLOAT;
    NSView *firstView= nil; // 1st focusable view used as plan B
    
    [viewStack addObject: view];
    
    while ([viewStack count] > 0)
    {
      view= [viewStack objectAtIndex: 0];
      [viewStack removeObjectAtIndex: 0];
      
      if ([view acceptsFirstResponder] && [view class] != [NSView class] && [view respondsToSelector:@selector(isEditable)] && [view isEditable])
      {
        NSPoint pos= [view convertPointToBase: [view frame].origin];
        if (!topMostView || pos.y < topMostViewY)
        {
          topMostView= view;
          topMostViewY= pos.y;
        }
      }
      else if ([view canBecomeKeyView] && !firstView)
        firstView= view;
      
      for (id subview in [view subviews])
      {
        if ([subview acceptsFirstResponder] && 
            [subview class] != [NSView class] && 
            [subview respondsToSelector:@selector(isEditable)] && [subview isEditable])
        {
          NSPoint pos= [subview convertPointToBase: [subview frame].origin];
          if (!topMostView || pos.y < topMostViewY)
          {
            topMostView= subview;
            topMostViewY= pos.y;
          }        
        }
        else if ([subview canBecomeKeyView] && !firstView)
          firstView= subview;
        
        [viewStack addObject: subview];
      }
    }
    
    if (!topMostView)
      [[self window] makeFirstResponder: firstView];
    else
      [[self window] makeFirstResponder: topMostView];
  }
}

//--------------------------------------------------------------------------------------------------

- (BOOL)tabView:(NSTabView *)tabView willCloseTabViewItem:(NSTabViewItem*)tabViewItem
{
  id form= [_panels objectForKey:[tabViewItem identifier]]; 
  
  return [self closePanel: form];
}

//--------------------------------------------------------------------------------------------------

- (BOOL)tabView:(NSTabView*)tabView itemHasCloseButton:(NSTabViewItem*)tabViewItem
{
  id form= [_panels objectForKey:[tabViewItem identifier]]; 
  bec::UIForm *uif = [form formBE];
  if (uif && uif->get_form_context_name() == WB_CONTEXT_HOME_GLOBAL)
    return NO;
  return YES;
}

//--------------------------------------------------------------------------------------------------

- (IBAction)handleMenuAction:(id)sender
{
  // Close Other Tabs Like This from the main tab
  if ([sender tag] == 1002)
  {
    id form= [_panels objectForKey:[[tabSwitcher clickedItem] identifier]];
    if (form && [form formBE])
    {
      std::string context_name;
      if ([form isKindOfClass: [WBMFormsPluginPanel class]])
        context_name = [form formBE]->get_form_context_name();
      for (NSTabViewItem *item in [[topTabView tabViewItems] reverseObjectEnumerator])
      {
        if (item != [tabSwitcher clickedItem] && [self tabView: topTabView itemHasCloseButton: item])
        {
          id itemForm = [_panels objectForKey: [item identifier]];
          if (context_name.empty())
          {
            if ([itemForm isKindOfClass: [form class]])
              [tabSwitcher closeTabViewItem: item];
          }
          else
          {
            bec::UIForm *itemFormBE = [itemForm formBE];
            if (itemFormBE && [itemForm isKindOfClass: [WBMFormsPluginPanel class]]
                && itemFormBE->get_form_context_name() == context_name)
              [tabSwitcher closeTabViewItem: item];
          }
        }
      }
    }
    [tabSwitcher setNeedsDisplay: YES];
  }
}

//--------------------------------------------------------------------------------------------------

- (BOOL)closePanel:(WBBasePanel*)panel
{
  id identifier = [[[panel identifier] retain] autorelease];

  // Remove first responder status from current first responder to trigger any pending processing
  // done on end-editing. Necessary for instance to make code editors store their content in their backend.
  [self.window makeFirstResponder:  nil];
  if ([panel respondsToSelector:@selector(willClose)] && ![panel willClose])
    return NO;
  if ([topTabView indexOfTabViewItemWithIdentifier: identifier] != NSNotFound)
    [self closeTopPanelWithIdentifier: identifier];
  else
    [self closeBottomPanelWithIdentifier: identifier];
  return YES;
}

//--------------------------------------------------------------------------------------------------

- (void)closeTopPanelWithIdentifier:(id)identifier
{
  BOOL hideOnly = NO;
  id form= [_panels objectForKey:identifier];
  if ([form isKindOfClass: [WBModelDiagramPanel class]] ||
      [form isKindOfClass: [WBModelOverviewPanel class]])
    hideOnly = YES;
  [self closeTopPanelWithIdentifier: identifier hideOnly: hideOnly];
}


- (void)closeTopPanelWithIdentifier:(id)identifier hideOnly:(BOOL)hideOnly
{
  NSUInteger index= [topTabView indexOfTabViewItemWithIdentifier: identifier];
  if (index != NSNotFound)
  {
    NSTabViewItem *item= [topTabView tabViewItemAtIndex: index];
    id form= [_panels objectForKey:[item identifier]];
    
    [form retain];
    
    if (hideOnly)
      [_closedTopTabs addObject: item];
    else
      [_panels removeObjectForKey: [item identifier]];
    
    [topTabView removeTabViewItem: item];
    
    // need to release now instead of autoreleasing because plugins should be released synchronously
    // now, and not in runloop
    [form release];
  }
}


//--------------------------------------------------------------------------------------------------

- (void)closeBottomPanelWithIdentifier:(id)identifier
{
  for (NSTabViewItem *item in [topTabView tabViewItems])
  {
    id p = [_panels objectForKey: [item identifier]];
    if ([p respondsToSelector:@selector(hasEditorWithIdentifier:)] && [p hasEditorWithIdentifier: identifier])
    {
      [p closeEditorWithIdentifier: identifier];
      break;
    }
  }  
}

//--------------------------------------------------------------------------------------------------

- (void)tabView:(NSTabView *)tabView willSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
  if (tabView == topTabView)
  {
    WBBasePanel *panel= [_panels objectForKey:[tabViewItem identifier]];
    
    NSAssert(![panel isKindOfClass:[WBModelDiagramPanel class]] || [(WBModelDiagramPanel*)panel canvas], @"diagram panel has no canvas at moment of tab switch");

    _wbui->set_active_form([panel formBE]);
  }
}

//--------------------------------------------------------------------------------------------------

- (void) forwardCommandToPanels: (const std::string) command
{
  if (command == "wb.next_tab")
  {
    if ([topTabView selectedTabViewItem] == [[topTabView tabViewItems] lastObject])
      [topTabView selectFirstTabViewItem: nil];
    else
      [topTabView selectNextTabViewItem:nil];
  }
  else if (command == "wb.back_tab")
  {
    if ([topTabView selectedTabViewItem] == [[topTabView tabViewItems] objectAtIndex: 0])
      [topTabView selectLastTabViewItem: nil];
    else
      [topTabView selectPreviousTabViewItem:nil];
  }
  else
  {
    for (NSTabViewItem *item in [topTabView tabViewItems])
    {
      id p = [_panels objectForKey: [item identifier]];
      if ([p respondsToSelector: @selector(performCommand:)])
        [p performCommand: command];
    }
  }
}

//--------------------------------------------------------------------------------------------------

- (void)changedIdentifierOfPanel: (WBBasePanel*)panel
                  fromIdentifier: (id)identifier
{
  [[panel retain] autorelease];
  [_panels removeObjectForKey: identifier];
  [_panels setObject: panel forKey: [panel identifier]];

  /*XXX
  NSUInteger index= [bottomTabView indexOfTabViewItemWithIdentifier: identifier];
  if (index != NSNotFound)
  {
    id item= [bottomTabView tabViewItemAtIndex: index];
    [bottomTabView removeTabViewItem: item]; // needed to workaround WBTabView bug
    [item setIdentifier: [panel identifier]];
    [bottomTabView insertTabViewItem: item atIndex: index]; // needed to workaround WBTabView bug
  }*/
}

//--------------------------------------------------------------------------------------------------

- (void)reopenTopTabViewItem:(id)identifier
{
  for (NSTabViewItem *item in _closedTopTabs)
  {
    if ([[item identifier] isEqual:identifier])
    {
      id form= [_panels objectForKey: identifier];
      
      [topTabView addTabViewItem:item];
      [_closedTopTabs removeObject:item];
      
      if ([form respondsToSelector:@selector(didOpen)])
        [form didOpen];
      break;
    }
  }
  [topTabView selectTabViewItemWithIdentifier:identifier];
}

//--------------------------------------------------------------------------------------------------

- (void)reopenEditor:(id)editor
{  
  id panel = [self activePanel];
  if ([panel isKindOfClass: [WBSplitPanel class]])
  {
    WBSplitPanel *spanel = (WBSplitPanel*)panel;
    if (![spanel hasEditor: editor])
    {
      [[editor retain] autorelease];

      // remove the editor from the current tab
      for (NSTabViewItem *item in [topTabView tabViewItems])
      {
        id p = [_panels objectForKey: [item identifier]];
        if ([p respondsToSelector:@selector(hasEditor:)] && [p hasEditor: editor])
        {
          [p closeEditor: editor];
          break;
        }
      }
      
      [spanel addEditor: editor];
    }
  }
}

//--------------------------------------------------------------------------------------------------

- (IBAction)showMySQLOverview:(id)sender
{
  if (_physicalOverview)
  {
    id identifier= [_physicalOverview identifier];
    
    for (NSTabViewItem *item in _closedTopTabs)
    {
      if ([[item identifier] isEqual:identifier])
      {
        [topTabView addTabViewItem:item];
        [_closedTopTabs removeObject:item];
        break;
      }
    }
    [topTabView selectTabViewItemWithIdentifier:identifier];
  }
}

//--------------------------------------------------------------------------------------------------

- (IBAction)showOutput:(id)sender
{
  [self reopenTopTabViewItem: @"output"];
}

//--------------------------------------------------------------------------------------------------

- (void)focusSearchField:(id)sender
{
  bec::UIForm *main_form = _wbui->get_active_main_form();
  if (main_form && main_form->get_toolbar())
  {
    mforms::ToolBarItem *item = main_form->get_toolbar()->find_item("find");
    if (item)
    {
      id view = item->get_data();
      if (view && [view isKindOfClass: [NSTextField class]])
      {
        [[self window] makeFirstResponder: view];
        return;
      }
    }
  }
  NSBeep();
}

//--------------------------------------------------------------------------------------------------

- (void)performSearchObject:(id)sender
{
  bec::UIForm *main_form= _wbui->get_active_main_form();
  if (main_form && main_form->get_frontend_data() && main_form->get_toolbar())
  {
    mforms::ToolBarItem *item = main_form->get_toolbar()->find_item("find");
    if (item)
    {
      id form= reinterpret_cast<id>(main_form->get_frontend_data());
      [self focusSearchField: nil];
      if ([form respondsToSelector:@selector(searchString:)])
        [form searchString: [NSString stringWithCPPString: item->get_text()]];
    }
  }
}

//--------------------------------------------------------------------------------------------------

- (void)resetWindowLayout
{
  [self showMySQLOverview:nil];
}

//--------------------------------------------------------------------------------------------------

- (WBBasePanel*)findPanelForView:(NSView*)view inTabView:(NSTabView*)tabView
{
  for (NSTabViewItem *item in [tabView tabViewItems])
  {
    if ([item view] == view)
    {
      id form= [_panels objectForKey: [item identifier]];
      if (!form)
        NSLog(@"Item in toplevel tabview is not a known form %@", [item label]);

      return form;
    }
  }
  return nil;
}

//--------------------------------------------------------------------------------------------------

/**
 * Iterates over all currently open editors in all tabs and returns the first one it encounters
 * that matches the given plugin type.
 */
- (WBBasePanel*) findPanelForPluginType: (Class) type
{
  for (NSTabViewItem *item in [topTabView tabViewItems])
  {
    id panel = [_panels objectForKey: [item identifier]];
    if ([panel respondsToSelector: @selector(findPanelForPluginType:)])
    {
      id editor = [panel findPanelForPluginType: type];
      if (editor)
        return editor;
    }
  }
  return nil;
}

//--------------------------------------------------------------------------------------------------

- (WBBasePanel*)findMainPanelForUIForm: (bec::UIForm*)form
{
  for (NSTabViewItem *item in [topTabView tabViewItems])
  {
    id panel = [_panels objectForKey: [item identifier]];
    if ([panel formBE] == form)
      return panel;
  }
  return nil;  
}
                        
//--------------------------------------------------------------------------------------------------

- (void)closeEditorsMatching: (NSString*)identifier
{  
  for (NSString *key in [_panels keyEnumerator])
  {
    id form= [_panels objectForKey: key];
  
    if ([form respondsToSelector: @selector(closeEditorWithIdentifier:)])
      [form closeEditorWithIdentifier: identifier];
  }
}

//--------------------------------------------------------------------------------------------------

- (WBBasePanel*)panelForResponder: (NSResponder*)aResponder
{
  NSResponder *responder = aResponder;
  // look up the responder chain until we reach one of the known toplevel containers
  NSResponder *previous= nil;
  NSResponder *previousPrevious= nil;
  
  while (responder && (responder != topTabView))
  {
    previous= responder;
    responder= [responder nextResponder];    
  }
  previousPrevious= previous;

  if (responder && [responder isKindOfClass: [NSTabView class]])
  {
    WBBasePanel *panel = [self findPanelForView: (NSView*)previousPrevious inTabView: (NSTabView*)responder];
    if (panel && previousPrevious && [panel isKindOfClass: [WBSplitPanel class]])
    {
      // check if the responder is in a panel inside the found panel (like an editor in the model diagram panel)
      responder = aResponder;
      while (responder && responder != previousPrevious)
      {
        WBBasePanel *subpanel;
        
        subpanel = [(WBSplitPanel*)panel findPanelForView: (NSView*)responder];
        if (subpanel)
          return subpanel;
        
        responder = [responder nextResponder];
      }
    }
    return panel;
  }
  
  return nil;
}


//--------------------------------------------------------------------------------------------------

/** Find the active panel based on firstResponder
 */
- (WBBasePanel*)activePanel
{
  return [self panelForResponder: [[self window] firstResponder]];
}

//--------------------------------------------------------------------------------------------------

- (WBBasePanel*)selectedMainPanel
{
  return [_panels objectForKey: [[topTabView selectedTabViewItem] identifier]];
}

//--------------------------------------------------------------------------------------------------

- (void)firstResponderChanged: (NSResponder*)responder
{
  WBBasePanel *panel= [self panelForResponder:responder];

  BOOL changedActivePanel = panel.formBE != _wbui->get_active_form();

  // replace Edit menu with the standard one so that copy/paste works without intervention
  if ([responder isKindOfClass: [NSTextView class]])
  {
    NSLog(@"TODO: restore edit menu");
  }
  else if (!changedActivePanel)
    _wbui->get_command_ui()->revalidate_edit_menu_items();
}

//--------------------------------------------------------------------------------------------------

/**
 * Selects (activates) the given panel in the UI.
 */
- (void) activatePanel: (WBBasePanel*) panel
{
  if (panel)
    _wbui->set_active_form([panel formBE]);
  else
    _wbui->set_active_form(0);
}

//--------------------------------------------------------------------------------------------------

- (void)setTitle:(NSString*)title
        forPanel:(WBBasePanel*)panel
{
  NSInteger i;
  
  i = [topTabView indexOfTabViewItemWithIdentifier: [panel identifier]];
  if (i >= 0 && i != NSNotFound)
  {
    [[topTabView tabViewItemAtIndex: i] setLabel: title];
    [[self window] display];
  }
}

//--------------------------------------------------------------------------------------------------

#pragma mark Creation, Destruction

- (id)init
{
  self = [super init];
  if (self != nil) 
  {
    _panels= [[NSMutableDictionary alloc] init];
    _closedTopTabs= [[NSMutableArray alloc] init];
    _closedBottomTabs= [[NSMutableArray alloc] init];
    
    _defaultMainMenu = [[NSApp mainMenu] retain];
  }
  return self;
}

//--------------------------------------------------------------------------------------------------

- (void)load
{
  log_info("Loading main window\n");
  if (![NSBundle loadNibNamed: @"MainWindow" owner: self])
    log_error("Failed loading MainWindow.nib");
}

//--------------------------------------------------------------------------------------------------

- (void)awakeFromNib
{
  [tabSwitcher setTabStyle: MMainTabSwitcher];
  if (NSAppKitVersionNumber > NSAppKitVersionNumber10_6)
    [self.window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];

  [statusBarText setStringValue: @""];
}

//--------------------------------------------------------------------------------------------------

- (BOOL)closeAllPanels
{
  id panelsCopy = [[_panels copy] autorelease];
  id homeTabId = [[topTabView tabViewItemAtIndex: 0] identifier];
  for (id panel in [panelsCopy objectEnumerator])
  {
    if ([homeTabId isEqualTo: [panel identifier]])
      continue;
    if ([topTabView indexOfTabViewItemWithIdentifier: [panel identifier]] != NSNotFound)
      [topTabView selectTabViewItemWithIdentifier: [panel identifier]];
    if (![self closePanel: panel])
      return NO;
  }
  [_panels removeAllObjects];

  return YES;
}

//--------------------------------------------------------------------------------------------------

- (void)dealloc
{
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  delete _backendObserver;
  
  [_closedTopTabs release];
  [_closedBottomTabs release];
  
  [_panels release];
  [_physicalOverview release];
  
  [super dealloc];
}

//--------------------------------------------------------------------------------------------------

- (BOOL)windowShouldClose:(id)window
{
  return _wbui->get_wb()->quit_application();
}

//--------------------------------------------------------------------------------------------------

- (WBBasePanel*)selectedTopPanel
{
  return [_panels objectForKey: [[topTabView selectedTabViewItem] identifier]];
}

//--------------------------------------------------------------------------------------------------

// mforms integration
#import <mforms/../cocoa/MFMForms.h>

class MainWindowDockingPoint : public mforms::DockingPointDelegate
{
  WBMainWindow *self;
  
public:
  MainWindowDockingPoint(WBMainWindow *mainWindow) : self(mainWindow) {}
  
  virtual std::string get_type()
  {
    return "MainWindow";
  }
  
  virtual void dock_view(mforms::AppView *view, const std::string &arg1, int arg2)
  {
    WBMFormsPluginPanel *panel = [WBMFormsPluginPanel panelOfAppView: view];
    view->set_managed();
    
    if (arg1 == "maintab" || arg1.empty())
    {
      if (panel)
        [self->topTabView selectTabViewItemWithIdentifier: [panel identifier]]; 
      else
      {
        panel = [[WBMFormsPluginPanel alloc] initWithAppView: view];
        if (!view->get_menubar())
          [panel setDefaultMenuBar: [self context]->get_command_ui()->create_menubar_for_context(view->is_main_form() ?
                                                                                                 view->get_form_context_name()
                                                                                                 : "")];
        [self addTopPanelAndSwitch: [panel autorelease]];
      }
    }    
  }
  
  virtual bool select_view(mforms::AppView *view)
  {
    NSString *ident = [NSString stringWithUTF8String: view->identifier().c_str()];
    
    if ([self->topTabView indexOfTabViewItemWithIdentifier: ident] < 0)
      return false;
    [self->topTabView selectTabViewItemWithIdentifier: ident];
    return false;
  }
  
  virtual void undock_view(mforms::AppView *view)
  {
    WBMFormsPluginPanel *panel = [WBMFormsPluginPanel panelOfAppView: view];
    
    if (panel)
    {
      // close the panel bypassing the willClose handler (that should've already been called much earlier)
      id identifier = [[[panel identifier] retain] autorelease];

      if ([self->topTabView indexOfTabViewItemWithIdentifier: identifier] != NSNotFound)
        [self closeTopPanelWithIdentifier: identifier];
      else
        [self closeBottomPanelWithIdentifier: identifier];
    }
  }
  
  virtual void set_view_title(mforms::AppView *view, const std::string &title)
  {
    WBMFormsPluginPanel *panel = [WBMFormsPluginPanel panelOfAppView: view];
    if (panel)
    {
      [panel setTitle: [NSString stringWithCPPString: title]];
      [self setTitle: [panel title] forPanel: panel];
    }
    else
      NSLog(@"set_view_title called for undocked view");
  }
  
  virtual std::pair<int, int> get_size()
  {
    NSRect rect = [[self window] frame];
    return std::make_pair(NSWidth(rect), NSHeight(rect));
  }

  virtual int view_count()
  {
    return [self->topTabView numberOfTabViewItems];
  }

  virtual mforms::AppView *selected_view()
  {
    id view = [[self->topTabView selectedTabViewItem] view];
    id panel = [self findPanelForView:view inTabView: self->topTabView];

    if ([panel isKindOfClass: [WBMFormsPluginPanel class]])
    {
      return [(WBMFormsPluginPanel*)panel appView];
    }
    return NULL;
  }

  virtual mforms::AppView *view_at_index(int index)
  {
    id view = [[self->topTabView tabViewItemAtIndex: index] view];
    id panel = [self findPanelForView:view inTabView: self->topTabView];

    if ([panel isKindOfClass: [WBMFormsPluginPanel class]])
    {
      return [(WBMFormsPluginPanel*)panel appView];
    }
    return NULL;
  }
};

//--------------------------------------------------------------------------------------------------

static void set_status_text(mforms::App *app, const std::string &text)
{
  WBMainWindow *mwin = app->get_data();

  NSString *string= [[NSString alloc] initWithUTF8String:text.c_str()];
  
  // setStatusText must release the param
  if ([NSThread isMainThread])
    [mwin setStatusText:string];
  else    
    [mwin performSelectorOnMainThread:@selector(setStatusText:)
                           withObject:string
                        waitUntilDone:NO];  
}

//--------------------------------------------------------------------------------------------------

static std::string get_resource_path(mforms::App *app, const std::string &file)
{
  if (file.empty())
    return [[[NSBundle mainBundle] resourcePath] UTF8String];
  
  if (!file.empty() && file[0] == '/')
    return file;

  std::string path;
  if (g_str_has_suffix(file.c_str(), ".png"))
  {
    path = bec::IconManager::get_instance()->get_icon_path(file);
    if (!path.empty())
      return path;
  }

  {
    std::string fn = base::basename(file);
    NSString *filename = [NSString stringWithUTF8String:fn.c_str()];
    
    id str = [[NSBundle mainBundle] pathForResource: [filename stringByDeletingPathExtension]
                                             ofType: [filename pathExtension]];
    if (str)
      return [str UTF8String];

    // Look for the same image but with tiff extension, in case the actual image is
    // a combined art file.
    str = [[NSBundle mainBundle] pathForResource: [filename stringByDeletingPathExtension]
                                          ofType: @"tiff"];
    if (str)
      return [str UTF8String];
  }
  return "";
}

//--------------------------------------------------------------------------------------------------

static std::string get_executable_path(mforms::App *app, const std::string &file)
{
  if (file.empty())
    return [[[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent] UTF8String];
  
  if (!file.empty() && file[0] == '/')
    return file;
  
  {
    std::string fn = base::basename(file);
    NSString *filename = [NSString stringWithUTF8String:fn.c_str()];

    NSString *path = [[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent];
    return [[path stringByAppendingPathComponent: filename] UTF8String];
  }
  return "";
}


//--------------------------------------------------------------------------------------------------

static base::Rect get_main_window_bounds(mforms::App *app)
{
  WBMainWindow *mwin = app->get_data();

  NSRect r = [[mwin window] frame];
  return base::Rect(NSMinX(r), NSMaxY(r), NSWidth(r), NSHeight(r));
}

//--------------------------------------------------------------------------------------------------

static void alloc_ar_pool()
{
  NSAutoreleasePool *pool = [[MyAutoreleasePool alloc] init];
  [[[NSThread currentThread] threadDictionary] setObject:pool forKey:@"MFormsAutoReleasePool"];
}


static void free_ar_pool()
{  
  [[[NSThread currentThread] threadDictionary] removeObjectForKey:@"MFormsAutoReleasePool"];
}

//--------------------------------------------------------------------------------------------------

static int enter_event_loop(mforms::App *app, float timeout)
{
  WBMainWindow *mwin = app->get_data();
  if (mwin)
  {
    NSDate *later = timeout > 0.0 ? [NSDate dateWithTimeIntervalSinceNow: timeout] : [NSDate distantFuture];
    mwin->_eventLoopRetCode = -0xdead1009;

    while (mwin->_eventLoopRetCode == -0xdead1009)
    {
      NSEvent *e = [NSApp nextEventMatchingMask: NSAnyEventMask
                                      untilDate: later
                                         inMode: NSDefaultRunLoopMode
                                        dequeue: YES];
      
      if (e != nil)
        [NSApp sendEvent: e];
      else
        if ([[NSDate date] earlierDate: later] == later)
        {
          mwin->_eventLoopRetCode = -1;
          break;
        }
    }
    
    return mwin->_eventLoopRetCode;
  }
  return -1;
}


static void exit_event_loop(mforms::App *app, int retcode)
{
  WBMainWindow *mwin = app->get_data();
  if (mwin)
  {
    mwin->_eventLoopRetCode = retcode;
  }
}

static base::Color get_system_color(mforms::SystemColor color)
{
  switch (color)
  {
    case mforms::SystemColorHighlight:
    {
      NSColor *c = [[NSColor alternateSelectedControlColor] colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
      return base::Color([c redComponent], [c greenComponent], [c blueComponent]);
    }
    case mforms::SystemColorEditor:
    {
      NSColor *c = [[NSColor controlBackgroundColor] colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
      return base::Color([c redComponent], [c greenComponent], [c blueComponent]);
    }
    case mforms::SystemColorDisabled:
    {
      NSColor *c = [[NSColor controlHighlightColor] colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
      return base::Color([c redComponent], [c greenComponent], [c blueComponent]);
    }
    case mforms::SystemColorContainer:
    {
      NSColor *c = [[NSColor colorWithDeviceWhite: 240/255.0 alpha: 1.0] colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
      return base::Color([c redComponent], [c greenComponent], [c blueComponent]);
    }
  }
  return base::Color::Color(1, 0, 0);
}

static float backing_scale_factor(mforms::App *app)
{
  WBMainWindow *controller = app->get_data();
  if ([controller.window respondsToSelector: @selector(backingScaleFactor)])
    return [controller.window backingScaleFactor]; // Available since 10.7.
  return 1.0;
}


void setup_mforms_app(WBMainWindow *mwin)
{
  mforms::ControlFactory *cf = mforms::ControlFactory::get_instance();
  g_assert(cf);
  
  mforms::App::instantiate(new MainWindowDockingPoint(mwin), true);
  mforms::App::get()->set_data(mwin);
  
  cf->_app_impl.get_resource_path = get_resource_path;
  cf->_app_impl.get_executable_path = get_executable_path;
  cf->_app_impl.set_status_text = set_status_text;
  cf->_app_impl.get_application_bounds = get_main_window_bounds;
  cf->_app_impl.begin_thread_loop = alloc_ar_pool;
  cf->_app_impl.end_thread_loop = free_ar_pool;
  cf->_app_impl.enter_event_loop = enter_event_loop;
  cf->_app_impl.exit_event_loop = exit_event_loop;
  cf->_app_impl.get_system_color = get_system_color;
  cf->_app_impl.backing_scale_factor = backing_scale_factor;
}

//--------------------------------------------------------------------------------------------------

@end
