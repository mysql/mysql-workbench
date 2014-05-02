/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#import "mforms/app.h"
#import "base/geometry.h"

#import "WBPluginEditorBase.h"
#include "grt/editor_base.h"
#import "MCPPUtilities.h"
#import "WBTabView.h"
#import "MVerticalLayoutView.h"
#import "TabViewDockingDelegate.h"

#include "objimpl/ui/mforms_ObjectReference_impl.h"
#include "objimpl/ui/ui_ObjectEditor_impl.h"

#import "editor_base.h"

#import "mforms/../cocoa/MFView.h"
#include "mforms/dockingpoint.h"
#include "mforms/code_editor.h"

#include "mforms/find_panel.h"

@implementation WBPluginEditorBase

- (void)dealloc
{
  if (mEditorGRTObject.is_valid())
    mEditorGRTObject->get_data()->notify_did_close();

  if (mDockingPoint)
    mDockingPoint->release();
  [[NSNotificationCenter defaultCenter] removeObserver: self];
  [super dealloc];
}


- (bec::GRTManager*)grtManager
{
  return _grtm;
}


- (id)identifier
{
  return nil;
}


- (NSString*)title
{
  bec::BaseEditor *ed = [self editorBE];
  if (ed)
    return [NSString stringWithCPPString: ed->get_title()];
  return nil;
}


- (NSImage*)titleIcon
{
  return [NSImage imageNamed: @"tab_icon_editor"];
}


- (void)setCompactMode:(BOOL)flag
{
  // no op by default if there's no compact/big modes supported
}

- (BOOL)enableLiveChangeButtons
{
  if (mApplyButton && mRevertButton)
  {
    [mApplyButton setHidden: NO];
    [mApplyButton setTarget: self];
    [mApplyButton setAction: @selector(applyLiveChanges:)];

    [mRevertButton setHidden: NO];
    [mRevertButton setTarget: self];
    [mRevertButton setAction: @selector(revertLiveChanges:)];
  }
  return NO;
}

- (void)reinitWithArguments:(const grt::BaseListRef&)args
{
}


- (void)notifyObjectSwitched
{
  if (mEditorGRTObject.is_valid())
    mEditorGRTObject->get_data()->notify_did_switch_object([self editorBE]);
}


- (void)updateTitle:(NSString*)title
{
  id superview = [[self view] superview];
  if ([superview isKindOfClass: [NSTabView class]])
  {
    NSTabView *tabView= (NSTabView*)superview;
    // workaround for stupid tabview hack
    if ([[tabView superview] isKindOfClass: [WBTabView class]])
      tabView = (NSTabView*)[tabView superview];
    NSInteger index = [tabView indexOfTabViewItemWithIdentifier: [self identifier]];
    if (index != NSNotFound)
    {
      id item= [tabView tabViewItemAtIndex: index];
      [item setLabel: title];
    }
  }
}


- (void)setMinimumSize:(NSSize)size
{
  mMinumumSize= size;
}


- (void)refresh
{
}


- (NSSize)minimumSize
{
  return mMinumumSize;
}


- (BOOL)matchesIdentifierForClosingEditor:(NSString*)identifier
{
  return NO;
}

- (bec::BaseEditor*) editorBE
{
  return NULL;
}

- (void)applyLiveChanges:(id)sender
{
  // set focus to nothing to force ongoing text changes to be commited
  [[[self view] window] makeFirstResponder: nil];

  bec::BaseEditor *editor = [self editorBE];
  if (editor && editor->is_editing_live_object())
  {
    if (mEditorGRTObject.is_valid())
      mEditorGRTObject->get_data()->notify_will_save();
    editor->apply_changes_to_live_object();
  }
}

- (void)revertLiveChanges:(id)sender
{
  // set focus to nothing to force ongoing text changes to be commited
  [[[self view] window] makeFirstResponder: nil];

  bec::BaseEditor *editor = [self editorBE];
  if (editor && editor->is_editing_live_object())
  {
    editor->revert_changes_to_live_object();
    if (mEditorGRTObject.is_valid())
      mEditorGRTObject->get_data()->notify_did_revert();
  }
}


- (void)pluginDidShow:(id)sender
{
  if (mEditorGRTObject.is_valid())
    mEditorGRTObject->get_data()->notify_will_open();
}

- (BOOL)pluginWillClose: (id)sender
{
  // set focus to nothing to force ongoing text changes to be commited
  [[[self view] window] makeFirstResponder: nil];

  if (mEditorGRTObject.is_valid())
    if (!mEditorGRTObject->get_data()->notify_will_close())
      return NO;

  bec::BaseEditor *editor = [self editorBE];
  if (editor && !editor->can_close())
    return NO;
  
  // by default, we check if the firstResponder belongs to this plugin and make it
  // resign so that edits are commited
  id first= [[[self view] window] firstResponder];
  id dockableView= [self view];
  NSWindow *window= [[self view] window];
  
  while (first && first != dockableView && first != window)
    first= [first superview];

  if (first != window)
    [first resignFirstResponder];
  
  return YES;
}

//--------------------------------------------------------------------------------------------------

static void text_changed(int line, int linesAdded, WBPluginEditorBase* self)
{
  bool dirty = [self editorBE]->get_sql_editor()->get_editor_control()->is_dirty();
  if ([self->mApplyButton isEnabled] != dirty)
    [self updateTitle: [self title]];
  [self->mApplyButton setEnabled: dirty];
  [self->mRevertButton setEnabled: dirty];
}

//--------------------------------------------------------------------------------------------------

- (void)setupEditorOnHost: (NSView*)host
{
  Sql_editor::Ref backend_editor = [self editorBE]->get_sql_editor();
  mforms::CodeEditor* mforms_editor = backend_editor->get_editor_control();
  mforms_editor->set_status_text("");

  NSView *container = nsviewForView(backend_editor->get_container());

  // Listen to focus-lost notifications.
  [[NSNotificationCenter defaultCenter] addObserver: self
                                           selector: @selector(textDidEndEditing:)
                                               name: NSTextDidEndEditingNotification
                                             object: nsviewForView(mforms_editor)];

  std::string font = grt::StringRef::cast_from([self editorBE]->get_grt_manager()->get_app_option("workbench.general.Editor:Font"));
  mforms_editor->set_font(font); 

  mforms_editor->signal_changed()->connect(boost::bind(text_changed, _1, _2, self));

  for (id subview in [[host subviews] reverseObjectEnumerator])
  {
    [subview removeFromSuperview];
  }
  [host addSubview: container];
  [container setFrame: [host bounds]];
  [host setAutoresizesSubviews: YES];
  [container setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
}

//--------------------------------------------------------------------------------------------------

- (void)enablePluginDocking:(NSTabView*)tabView
{
  mEditorGRTObject = ui_ObjectEditorRef(_grtm->get_grt());

  // setup docking point for mUpperTabView
  mDockingPoint = mforms::manage(new mforms::DockingPoint(new TabViewDockingPointDelegate(tabView, "editor"), true));

  mEditorGRTObject->dockingPoint(mforms_to_grt(_grtm->get_grt(), mDockingPoint));
}

@end
