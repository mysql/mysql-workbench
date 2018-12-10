/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "model/wb_model_diagram_form.h"
#include "model/wb_layer_tree.h"
#include "wb_context.h"
#include "wb_context_model.h"

#import "mforms/../cocoa/MFView.h"
#import "NSString_extras.h"
#import "WBModelDiagramPanel.h"
#import "MCanvasScrollView.h"
#import "GRTIconCache.h"
#import "GRTTreeDataSource.h"
#import "MTabSwitcher.h"
#import "WBSplitView.h"
#import "WBObjectDescriptionController.h"
#import "WBObjectPropertiesController.h"
#import "WBModelSidebarController.h"
#import "MCPPUtilities.h"
#import "MContainerView.h"

static int zoom_levels[]= {
  200,
  150,
  100,
  95,
  90,
  85,
  80,
  75,
  70,
  60,
  50,
  40,
  30,
  20,
  10
};

//----------------------------------------------------------------------------------------------------------------------

@interface WBModelDiagramPanel () {
  NSString *_panelId;
  IBOutlet NSView *toolbar;
  IBOutlet NSView *optionsToolbar;
  IBOutlet MCanvasScrollView *scrollView;
  IBOutlet NSTabViewItem *layerTab;

  IBOutlet NSSplitView *sideSplitview;

  IBOutlet WBModelSidebarController *sidebarController;

  IBOutlet MCanvasViewer *navigatorViewer;
  IBOutlet NSSlider *zoomSlider;
  IBOutlet NSComboBox *zoomCombo;

  IBOutlet WBObjectDescriptionController *descriptionController;

  IBOutlet WBObjectPropertiesController* propertiesController;

  IBOutlet MTabSwitcher *mSwitcherT;
  IBOutlet MTabSwitcher *mSwitcherM;
  IBOutlet MTabSwitcher *mSwitcherB;

  NSMutableArray *nibObjects;

  MCanvasViewer *_viewer;

  wb::ModelDiagramForm *_formBE;

  BOOL _miniViewReady;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation WBModelDiagramPanel

static void *backend_destroyed(void *ptr) {
  ((__bridge WBModelDiagramPanel*)ptr)->_formBE = NULL;
  return NULL;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithId: (NSString *)oid formBE: (wb::ModelDiagramForm *)be {
  self = [super init];
  if (self != nil)   {
    _formBE = be;
    NSMutableArray *temp;
    if (_formBE != NULL && [NSBundle.mainBundle loadNibNamed: @"WBModelDiagram" owner: self topLevelObjects: &temp]) {
      nibObjects = temp;

      _formBE->set_frontend_data((__bridge void *)self);

      _formBE->add_destroy_notify_callback((__bridge void *)self, backend_destroyed);

      _panelId = oid;
      _viewer = [[MCanvasViewer alloc] initWithFrame:NSMakeRect(0, 0, 300, 300)];

      [descriptionController setup];

      self.splitView.backgroundColor = [NSColor colorWithDeviceWhite: 128 / 255.0 alpha : 1.0];

      // setup layer tree
      layerTab.view = nsviewForView(_formBE->get_layer_tree());

      // setup navigator
      for (int i= 0; i < (int)(sizeof(zoom_levels)/sizeof(int)); i++)
        [zoomCombo addItemWithObjectValue:@((float)zoom_levels[i])];
      [navigatorViewer setupQuartz];
      [navigatorViewer setPostsFrameChangedNotifications:YES];

      [[NSNotificationCenter defaultCenter] addObserver: self
                                               selector: @selector(navigatorFrameChanged:)
                                                   name: NSViewFrameDidChangeNotification
                                                 object: navigatorViewer];
      [_viewer setupQuartz];
      _viewer.delegate = self;
      [scrollView setContentCanvas: _viewer];

      [sidebarController setupWithDiagramForm: _formBE];

      _viewer.canvas->set_user_data((__bridge void *)self);

      [_viewer registerForDraggedTypes: @[@WB_DBOBJECT_DRAG_TYPE]];

      [self setRightSidebar: be->get_wb()->get_wb_options().get_int("Sidebar:RightAligned", 0)];

      self.splitView.autosaveName = @"diagramSplitPosition";

      mSwitcherT.tabStyle = MSectionTabSwitcher;
      mSwitcherM.tabStyle = MSectionTabSwitcher;
      mSwitcherB.tabStyle = MSectionTabSwitcher;

      NSWindow *window = NSApplication.sharedApplication.mainWindow;
      [window addObserver: self forKeyPath: @"effectiveAppearance" options: 0 context: nil];
      [self updateColors];

      // setup tools toolbar
      mforms::ToolBar *tbar = _formBE->get_tools_toolbar();
      if (tbar) {
        NSView *view = tbar->get_data();
        [toolbar addSubview: view];
        view.autoresizingMask = NSViewHeightSizable|NSViewMinXMargin|NSViewMaxYMargin;
        view.frame = toolbar.bounds;
      }

      // setup options toolbar
      tbar = _formBE->get_options_toolbar();
      if (tbar) {
        NSView *view = tbar->get_data();
        [optionsToolbar addSubview: view];
        view.autoresizingMask = NSViewWidthSizable|NSViewMinXMargin|NSViewMaxYMargin;
        view.frame = optionsToolbar.bounds;
      }
      
      [self restoreSidebarsFor: "ModelDiagram" toolbar: _formBE->get_toolbar()];
    }
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)init {
  return [self initWithId: nil formBE: NULL];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  NSWindow *window = NSApplication.sharedApplication.mainWindow;
  [window removeObserver: self forKeyPath: @"effectiveAppearance"];

  if (_formBE)
    _formBE->remove_destroy_notify_callback((__bridge void *)self);
  [[NSNotificationCenter defaultCenter] removeObserver: self];
  [sidebarController invalidate];
  
  [_viewer setDelegate: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)observeValueForKeyPath: (NSString *)keyPath
                      ofObject: (id)object
                        change: (NSDictionary *)change
                       context: (void *)context {
  if ([keyPath isEqualToString: @"effectiveAppearance"]) {
    [self updateColors];
    return;
  }
  [super observeValueForKeyPath: keyPath ofObject: object change: change context: context];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)updateColors {
  _formBE->setBackgroundColor(base::Color::getSystemColor(base::TextBackgroundColor));
}

//----------------------------------------------------------------------------------------------------------------------

- (void)showOptionsToolbar: (BOOL)flag {
  if (optionsToolbar.hidden != !flag) {
    id parent = optionsToolbar.superview;
    optionsToolbar.hidden = !flag;
    [optionsToolbar removeFromSuperview];
    [parent addSubview: optionsToolbar];
    [optionsToolbar setNeedsDisplay:YES];
  } else
    [optionsToolbar setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (MCanvasViewer*)canvasViewer {
  return _viewer;
}

//----------------------------------------------------------------------------------------------------------------------

- (mdc::CanvasView*)canvas {
  return _viewer.canvas;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString*)panelId {
  return _panelId;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSString*)title {
  return @(_formBE->get_title().c_str());
}

//----------------------------------------------------------------------------------------------------------------------

- (void)searchString: (NSString*)text {
  _formBE->search_and_focus_object(text.UTF8String);
}

//----------------------------------------------------------------------------------------------------------------------

- (NSImage*)tabIcon {
  return [NSImage imageNamed:@"tab.diagram.16x16.png"];
}

//----------------------------------------------------------------------------------------------------------------------

static NSPoint loadCursorHotspot(const std::string &path) {
  gsize size;
  guint8 *buffer;
  
  if (g_file_get_contents(path.c_str(), (gchar**)&buffer, &size, NULL)) {
    if (buffer[0] != 0 || buffer[1] != 0 || buffer[2] != 2 || buffer[3] != 0) {
      g_free(buffer);
      return NSMakePoint(0.0, 0.0);
    }

    int xspot = buffer[6+4]|buffer[6+5]<<8;
    int yspot = buffer[6+6]|buffer[6+7]<<8;
    g_free(buffer);
    return NSMakePoint(xspot, yspot);
  }

  return NSMakePoint(0.0, 0.0);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)updateCursor {
  std::string cursorName= _formBE->get_cursor();
  NSCursor *cursor= nil;
  
  if (!cursorName.empty()) {
    NSImage *image= [[GRTIconCache sharedIconCache] imageForFileName:[NSString stringWithFormat:@"%s.png", cursorName.c_str()]];
    NSString *path= [[NSBundle mainBundle] pathForResource: @(cursorName.c_str())
                                                    ofType: @"png"
                                               inDirectory: @""];
    
    if (path)
      cursor= [[NSCursor alloc] initWithImage:image hotSpot:loadCursorHotspot(path.fileSystemRepresentation)];
  }
  [_viewer setCursor:cursor];
}

//----------------------------------------------------------------------------------------------------------------------

- (bec::UIForm*)formBE {
  return _formBE;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSView*)initialFirstResponder {
  return _viewer;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isClosed {
  return _formBE->is_closed();
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)willClose {
  if (_formBE)
    _formBE->set_closed(true);
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)selectionChanged {
  [propertiesController updateForForm: _formBE];
  [descriptionController updateForForm: _formBE];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)navigatorFrameChanged:(NSNotification*)notif {
  _formBE->update_mini_view_size(NSWidth(navigatorViewer.frame), NSHeight(navigatorViewer.frame));
  [navigatorViewer setNeedsDisplay:YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (IBAction)setZoom: (id)sender {
  if (sender == zoomSlider || sender == zoomCombo) {
    _formBE->set_zoom([sender floatValue]/100.0);
    
    [self refreshZoom];
  } else if (NSMinX([sender frame]) < NSMinX(zoomSlider.frame)) {
    _formBE->zoom_out();
    
    [self refreshZoom];
  } else if (NSMaxX([sender frame]) > NSMaxX(zoomSlider.frame)) {
    _formBE->zoom_in();
    
    [self refreshZoom];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)refreshZoom {
  zoomSlider.integerValue = _formBE->get_zoom()*100;
  zoomCombo.integerValue = _formBE->get_zoom()*100;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)didActivate {
  NSView *view = nsviewForView(_formBE->get_wb()->get_model_context()->shared_secondary_sidebar());
  if (view.superview) {
    [view removeFromSuperview];
  }
  [secondarySidebar addSubview: view];
  view.autoresizingMask = NSViewWidthSizable|NSViewHeightSizable|NSViewMinXMargin|NSViewMinYMargin|NSViewMaxXMargin|NSViewMaxYMargin;
  view.frame = secondarySidebar.bounds;

  [self refreshZoom];
  [(self.topView).window makeFirstResponder: _viewer];
  
  if (!_miniViewReady) {
    _formBE->setup_mini_view(navigatorViewer.canvas);
    _formBE->update_mini_view_size(NSWidth(navigatorViewer.frame), NSHeight(navigatorViewer.frame));
    _miniViewReady = YES;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)didOpen {
  _formBE->set_closed(false);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)canvasToolChanged: (mdc::CanvasView*)canvas {
  mforms::ToolBar *tb = _formBE->get_options_toolbar();
  [self showOptionsToolbar: tb && !tb->get_items().empty()];
  [self updateCursor];
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)canvasMouseDown: (mdc::MouseButton)button
               location: (NSPoint)pos
                  state: (mdc::EventState)state {
  _formBE->handle_mouse_button(button, true, pos.x, pos.y, state);  
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)canvasMouseDoubleClick: (mdc::MouseButton)button
                      location: (NSPoint)pos
                         state: (mdc::EventState)state {
  _formBE->handle_mouse_double_click(button, pos.x, pos.y, state);  
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)canvasMouseUp: (mdc::MouseButton)button
             location: (NSPoint)pos
                state: (mdc::EventState)state {
  _formBE->handle_mouse_button(button, false, pos.x, pos.y, state);
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)canvasMouseMoved: (NSPoint)pos
                   state: (mdc::EventState)state {
  _formBE->handle_mouse_move(pos.x, pos.y, state);
  
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)canvasKeyDown: (mdc::KeyInfo)key state: (mdc::EventState)state {
  _formBE->handle_key(key, true, state);
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)canvasKeyUp :(mdc::KeyInfo)key state: (mdc::EventState)state {
  _formBE->handle_key(key, false, state);
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

// drag drop
- (NSDragOperation)canvasDraggingEntered: (id<NSDraggingInfo>)sender {
  NSPasteboard *pboard = [sender draggingPasteboard];
  void *data = [pboard nativeDataForTypeAsChar: WB_DBOBJECT_DRAG_TYPE];
  if (data == NULL)
    return NSDragOperationNone;

  NSPoint pos = [_viewer convertPoint: [sender draggingLocation] fromView: nil];
  std::list<GrtObjectRef> *list = reinterpret_cast<std::list<GrtObjectRef> *>(data);
  if (_formBE->accepts_drop(pos.x, pos.y, WB_DBOBJECT_DRAG_TYPE, *list))
    return NSDragOperationCopy;

  return NSDragOperationNone;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSDragOperation)draggingUpdated: (id <NSDraggingInfo>)sender {
  NSPasteboard *pboard = [sender draggingPasteboard];
  void *data = [pboard nativeDataForTypeAsChar: WB_DBOBJECT_DRAG_TYPE];
  if (data == NULL)
    return NSDragOperationNone;

  return NSDragOperationCopy;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)canvasPerformDragOperation:(id < NSDraggingInfo >)sender {
  NSPasteboard *pboard = [sender draggingPasteboard];
  void *data = [pboard nativeDataForTypeAsChar: WB_DBOBJECT_DRAG_TYPE];
  if (data == NULL)
    return false;

  NSPoint pos= [_viewer convertPoint: [sender draggingLocation] fromView: nil];
  std::list<GrtObjectRef> *list = reinterpret_cast<std::list<GrtObjectRef> *>(data);
  return _formBE->perform_drop(pos.x, pos.y, WB_DBOBJECT_DRAG_TYPE, *list);
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)splitView: (NSSplitView *)splitView shouldAdjustSizeOfSubview: (NSView *)subview {
  if (subview == bottomContainer)
    return NO;
  return [super splitView: splitView shouldAdjustSizeOfSubview: subview];
}

//----------------------------------------------------------------------------------------------------------------------

  - (CGFloat)splitView: (NSSplitView *)splitView
constrainMinCoordinate: (CGFloat)proposedMin
           ofSubviewAt: (NSInteger)dividerIndex {
  if (splitView == sideSplitview) {
    if (dividerIndex == 0)
      return proposedMin + 80;
    else if (dividerIndex == 1)
      return proposedMin + 30;
  } else if (splitView == self.splitView)
    return proposedMin + 120;
  return [super splitView: splitView constrainMinCoordinate: proposedMin ofSubviewAt: dividerIndex];
}

//----------------------------------------------------------------------------------------------------------------------

  - (CGFloat)splitView: (NSSplitView *)splitView
constrainMaxCoordinate: (CGFloat)proposedMax
           ofSubviewAt: (NSInteger)dividerIndex {
  if (splitView == sideSplitview) {
    if (dividerIndex == 0)
      return proposedMax - 30;
    else if (dividerIndex == 1)
      return proposedMax - 80;
  } else if (splitView == self.splitView)
    return proposedMax - 120;

  return [super splitView: splitView constrainMaxCoordinate: proposedMax ofSubviewAt: dividerIndex];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setRightSidebar: (BOOL)flag {
  mSidebarAtRight = flag;
  
  id view1 = (self.topView).subviews[0];
  id view2 = (self.topView).subviews[1];
  
  if (mSidebarAtRight) {
    if (view2 != sidebar) {
      [view1 removeFromSuperview];
      [self.topView addSubview: view1];
    }    
  } else {
    if (view1 != sidebar) {
      [view1 removeFromSuperview];
      [self.topView addSubview: view1];
    }
  }
}

@end

//----------------------------------------------------------------------------------------------------------------------
