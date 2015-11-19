/* 
 * Copyright (c) 2008, 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "model/wb_model_diagram_form.h"
#include "model/wb_layer_tree.h"

#import "mforms/../cocoa/MFView.h"
#import "NSString_extras.h"
#import "WBModelDiagramPanel.h"
#import "MCanvasScrollView.h"
#import "GRTIconCache.h"
#import "GRTTreeDataSource.h"
#import "MTabSwitcher.h"
#import "WBObjectDescriptionController.h"
#import "WBModelSidebarController.h"
#import "MCPPUtilities.h"

#include "wb_context.h"
#include "wb_context_model.h"

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


@implementation WBModelDiagramPanel

static void *backend_destroyed(void *ptr)
{
  ((WBModelDiagramPanel*)ptr)->_formBE = 0;
  return 0;
}

- (instancetype)initWithId: (NSString *)oid formBE: (wb::ModelDiagramForm *)be
{
  self = [super init];
  if (self != nil)
  {
    _formBE = be;
    if (_formBE != NULL)
    {
      _formBE->set_frontend_data(self);
      grtm = be->get_wb()->get_grt_manager();

      _formBE->add_destroy_notify_callback(self, backend_destroyed);

      [NSBundle loadNibNamed:@"WBModelDiagram" owner:self];
      _identifier= [oid retain];
      _viewer= [[[MCanvasViewer alloc] initWithFrame:NSMakeRect(0, 0, 300, 300)] autorelease];

      [descriptionController setWBContext: _formBE->get_wb()->get_ui()];
      [mPropertiesController setWBContext: _formBE->get_wb()->get_ui()];

      [topView setDividerThickness: 1];
      [topView setBackgroundColor: [NSColor colorWithDeviceWhite:128/255.0 alpha:1.0]];

      // setup layer tree
      [layerTab setView: nsviewForView(_formBE->get_layer_tree())];

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
      [_viewer setDelegate: self];
      [scrollView setContentCanvas: _viewer];

      [sidebarController setupWithDiagramForm: _formBE];

      [_viewer canvas]->set_user_data(self);

      [_viewer registerForDraggedTypes: @[@WB_DBOBJECT_DRAG_TYPE]];

      [self setRightSidebar: be->get_wb()->get_wb_options().get_int("Sidebar:RightAligned", 0)];

      [topView setAutosaveName: @"diagramSplitPosition"];

      [mSwitcherT setTabStyle: MPaletteTabSwitcherSmallText];
      [mSwitcherM setTabStyle: MPaletteTabSwitcherSmallText];
      [mSwitcherB setTabStyle: MPaletteTabSwitcherSmallText];

      // setup tools toolbar
      mforms::ToolBar *tbar = _formBE->get_tools_toolbar();
      if (tbar)
      {
        NSView *view = tbar->get_data();
        [toolbar addSubview: view];
        [view setAutoresizingMask: NSViewHeightSizable|NSViewMinXMargin|NSViewMaxYMargin];
        [view setFrame: [toolbar bounds]];
      }

      // setup options toolbar
      tbar = _formBE->get_options_toolbar();
      if (tbar)
      {
        NSView *view = tbar->get_data();
        [optionsToolbar addSubview: view];
        [view setAutoresizingMask: NSViewWidthSizable|NSViewMinXMargin|NSViewMaxYMargin];
        [view setFrame: [optionsToolbar bounds]];
      }
      
      [self restoreSidebarsFor: "ModelDiagram" toolbar: _formBE->get_toolbar()];
    }
  }
  return self;
}

- (instancetype)init
{
  return [self initWithId: nil formBE: NULL];
}

- (void)dealloc
{
  if (_formBE)
    _formBE->remove_destroy_notify_callback(self);
  [_identifier release];
  [[NSNotificationCenter defaultCenter] removeObserver: self];
  [sidebarController invalidate];
  
  [_viewer setDelegate: nil];
  [topView release];
  [sidebarController release];
  [descriptionController release];
  [mPropertiesController release];
  [mainSplitViewDelegate release];
  
  [super dealloc];
}


- (NSView*)topView
{
  return topView;
}


- (void)showOptionsToolbar:(BOOL)flag
{
  if ([optionsToolbar isHidden] != !flag)
  {
    id parent = [optionsToolbar superview];
    [optionsToolbar setHidden: !flag];
    [optionsToolbar retain];
    [optionsToolbar removeFromSuperview];
    [parent addSubview: optionsToolbar];
    [optionsToolbar release];
    [optionsToolbar setNeedsDisplay:YES];
/*  
    NSRect rect= [scrollView frame];
    if (flag)
      rect.size.height-= NSHeight([optionsToolbar frame]);
    else
      rect.size.height+= NSHeight([optionsToolbar frame]);
    [scrollView setFrame: rect];
*/
  }
  else
    [optionsToolbar setNeedsDisplay: YES];
}


- (MCanvasViewer*)canvasViewer
{
  return _viewer;
}


- (mdc::CanvasView*)canvas
{
  return [_viewer canvas];
}


- (NSString*)identifier
{
  return _identifier;
}


- (NSString*)title
{
  return @(_formBE->get_title().c_str());
}


- (void)searchString:(NSString*)text
{
  if (!_formBE->search_and_focus_object([text UTF8String]))
    ;
    //NSBeep();
}


- (NSImage*)tabIcon
{
  return [NSImage imageNamed:@"tab.diagram.16x16.png"];
}


static NSPoint loadCursorHotspot(const std::string &path)
{
  gsize size;
  guint8 *buffer;
  
  if (g_file_get_contents(path.c_str(), (gchar**)&buffer, &size, NULL))
  {
    if (buffer[0] != 0 || buffer[1] != 0 || buffer[2] != 2 || buffer[3] != 0)
    {
      g_free(buffer);
      return NSMakePoint(0.0, 0.0);
    }
    int xspot= buffer[6+4]|buffer[6+5]<<8;
    int yspot= buffer[6+6]|buffer[6+7]<<8;
    g_free(buffer);
    return NSMakePoint(xspot, yspot);
  } 
  return NSMakePoint(0.0, 0.0);
}


- (void)updateCursor
{
  std::string cursorName= _formBE->get_cursor();
  NSCursor *cursor= nil;
  
  if (!cursorName.empty())
  {
    NSImage *image= [[GRTIconCache sharedIconCache] imageForFileName:[NSString stringWithFormat:@"%s.png", cursorName.c_str()]];
    
    NSString *path= [[NSBundle mainBundle] pathForResource:@(cursorName.c_str())
                                                    ofType:@"png" inDirectory:@""];
    
    if (path)
      cursor= [[NSCursor alloc] initWithImage:image hotSpot:loadCursorHotspot([path fileSystemRepresentation])];
  }
  [_viewer setCursor:cursor];
  [cursor release];
}


- (bec::UIForm*)formBE
{
  return _formBE;
}


- (NSView*)initialFirstResponder
{
  return _viewer;
}

- (BOOL)isClosed
{
  return _formBE->is_closed();
}


- (BOOL)willClose
{
  if (_formBE)
    _formBE->set_closed(true);
  return YES;
}

- (void)selectionChanged
{
  [mPropertiesController updateForForm: _formBE];
  [descriptionController updateForForm: _formBE];
}

- (void)navigatorFrameChanged:(NSNotification*)notif
{
  _formBE->update_mini_view_size(NSWidth([navigatorViewer frame]), NSHeight([navigatorViewer frame]));
  [navigatorViewer setNeedsDisplay:YES];
}

- (IBAction)setZoom:(id)sender
{
  if (sender == zoomSlider || sender == zoomCombo)
  {
    _formBE->set_zoom([sender floatValue]/100.0);
    
    [self refreshZoom];
  }
  else if (NSMinX([sender frame]) < NSMinX([zoomSlider frame]))
  {
    _formBE->zoom_out();
    
    [self refreshZoom];
  }
  else if (NSMaxX([sender frame]) > NSMaxX([zoomSlider frame]))
  {
    _formBE->zoom_in();
    
    [self refreshZoom];
  }
}


- (void)refreshZoom
{
  [zoomSlider setIntegerValue:_formBE->get_zoom()*100];
  [zoomCombo setIntegerValue:_formBE->get_zoom()*100];
}


- (void)didActivate
{
  NSView *view = nsviewForView(_formBE->get_wb()->get_model_context()->shared_secondary_sidebar());
  if ([view superview])
  {
    [view retain];
    [view removeFromSuperview];
  }
  [secondarySidebar addSubview: view];
  [view setAutoresizingMask: NSViewWidthSizable|NSViewHeightSizable|NSViewMinXMargin|NSViewMinYMargin|NSViewMaxXMargin|NSViewMaxYMargin];
  [view setFrame: [secondarySidebar bounds]];


  [self refreshZoom];
  [[topView window] makeFirstResponder: _viewer];
  
  if (!_miniViewReady)
  {
    _formBE->setup_mini_view([navigatorViewer canvas]);
    _formBE->update_mini_view_size(NSWidth([navigatorViewer frame]), NSHeight([navigatorViewer frame]));
    _miniViewReady = YES;
  }
}


- (void)didOpen
{
  _formBE->set_closed(false);
}


- (void)canvasToolChanged:(mdc::CanvasView*)canvas
{
  mforms::ToolBar *tb = _formBE->get_options_toolbar();
  
  [self showOptionsToolbar: tb && !tb->get_items().empty()];
  
  [self updateCursor];
}


- (BOOL)canvasMouseDown:(mdc::MouseButton)button
               location:(NSPoint)pos
                  state:(mdc::EventState)state
{
  _formBE->handle_mouse_button(button, true, pos.x, pos.y, state);  
  return YES;
}

- (BOOL)canvasMouseDoubleClick:(mdc::MouseButton)button
               location:(NSPoint)pos
                  state:(mdc::EventState)state
{
  _formBE->handle_mouse_double_click(button, pos.x, pos.y, state);  
  return YES;
}

- (BOOL)canvasMouseUp:(mdc::MouseButton)button
             location:(NSPoint)pos
                state:(mdc::EventState)state
{
  _formBE->handle_mouse_button(button, false, pos.x, pos.y, state);
  return YES;
}

- (BOOL)canvasMouseMoved:(NSPoint)pos
                   state:(mdc::EventState)state
{
  _formBE->handle_mouse_move(pos.x, pos.y, state);
  
  return YES;
}


- (BOOL)canvasKeyDown:(mdc::KeyInfo)key state:(mdc::EventState)state
{
  _formBE->handle_key(key, true, state);
  return YES;
}


- (BOOL)canvasKeyUp:(mdc::KeyInfo)key state:(mdc::EventState)state
{
  _formBE->handle_key(key, false, state);
  return YES;
}



// drag drop
- (NSDragOperation)canvasDraggingEntered: (id<NSDraggingInfo>)sender
{
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

- (NSDragOperation)draggingUpdated: (id <NSDraggingInfo>)sender
{
  NSPasteboard *pboard = [sender draggingPasteboard];
  void *data = [pboard nativeDataForTypeAsChar: WB_DBOBJECT_DRAG_TYPE];
  if (data == NULL)
    return NSDragOperationNone;

  return NSDragOperationCopy;
}

- (BOOL)canvasPerformDragOperation:(id < NSDraggingInfo >)sender
{  
  NSPasteboard *pboard = [sender draggingPasteboard];
  void *data = [pboard nativeDataForTypeAsChar: WB_DBOBJECT_DRAG_TYPE];
  if (data == NULL)
    return false;

  NSPoint pos= [_viewer convertPoint: [sender draggingLocation] fromView: nil];
  std::list<GrtObjectRef> *list = reinterpret_cast<std::list<GrtObjectRef> *>(data);
  return _formBE->perform_drop(pos.x, pos.y, WB_DBOBJECT_DRAG_TYPE, *list);
}

- (BOOL)splitView:(NSSplitView *)splitView shouldAdjustSizeOfSubview:(NSView *)subview
{
  if (subview == bottomContainer)
    return NO;
  return [super splitView: splitView shouldAdjustSizeOfSubview: subview];
}


- (CGFloat)splitView:(NSSplitView *)splitView constrainMinCoordinate:(CGFloat)proposedMin ofSubviewAt:(NSInteger)dividerIndex
{
  if (splitView == sideSplitview)
  {
    if (dividerIndex == 0)
      return proposedMin + 80;
    else if (dividerIndex == 1)
      return proposedMin + 30;
  }
  else if (splitView == topView)
    return proposedMin + 120;
  return [super splitView: splitView constrainMinCoordinate: proposedMin ofSubviewAt: dividerIndex];
}


- (CGFloat)splitView:(NSSplitView *)splitView constrainMaxCoordinate:(CGFloat)proposedMax ofSubviewAt:(NSInteger)dividerIndex
{
  if (splitView == sideSplitview)
  {
    if (dividerIndex == 0)
      return proposedMax - 30;
    else if (dividerIndex == 1)
      return proposedMax - 80;
  }
  else if (splitView == topView)
    return proposedMax - 120;

  return [super splitView: splitView constrainMaxCoordinate: proposedMax ofSubviewAt: dividerIndex];
}


//--------------------------------------------------------------------------------------------------

- (void)setRightSidebar:(BOOL)flag
{
  mSidebarAtRight = flag;
  
  id view1 = [topView subviews][0];
  id view2 = [topView subviews][1];
  
  if (mSidebarAtRight)
  {
    if (view2 != sidebar)
    {
      [[view1 retain] autorelease];
      [view1 removeFromSuperview];
      [topView addSubview: view1];
    }    
  }
  else
  {
    if (view1 != sidebar)
    {
      [[view1 retain] autorelease];
      [view1 removeFromSuperview];
      [topView addSubview: view1];
    }
  }
}

@end
