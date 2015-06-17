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

#import <Cocoa/Cocoa.h>
#import "WBSplitPanel.h"
#import "MCanvasViewer.h"

@class GRTTreeDataSource;
@class MCanvasScrollView;
@class WBObjectPropertiesController;

namespace bec
{
  class UIForm;
};
namespace wb
{
  class ModelDiagramForm;
};

@class MTabSwitcher;
@class WBModelSidebarController;
@class WBObjectDescriptionController;

@interface WBModelDiagramPanel : WBSplitPanel <CanvasViewerDelegate>
{
  NSString *_identifier;
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

  IBOutlet WBObjectPropertiesController* mPropertiesController;

  IBOutlet MTabSwitcher *mSwitcherT;
  IBOutlet MTabSwitcher *mSwitcherM;
  IBOutlet MTabSwitcher *mSwitcherB;

  MCanvasViewer *_viewer;
  
  wb::ModelDiagramForm *_formBE;
  
  BOOL _miniViewReady;
}

- (instancetype)initWithId: (NSString *)oid formBE: (wb::ModelDiagramForm *)be NS_DESIGNATED_INITIALIZER;

@property (readonly, strong) NSView *topView;

@property (readonly, copy) NSString *identifier;
@property (readonly) bec::UIForm *formBE;

- (void)updateCursor;

- (void)searchString:(NSString*)text;

- (void)canvasToolChanged:(mdc::CanvasView*)canvas;

- (void)setRightSidebar:(BOOL)flag;

@property (readonly, strong) MCanvasViewer *canvasViewer;
@property (readonly) mdc::CanvasView *canvas;

@property (getter=isClosed, readonly) BOOL closed;

- (void)refreshZoom;

- (IBAction)setZoom:(id)sender;

@end
