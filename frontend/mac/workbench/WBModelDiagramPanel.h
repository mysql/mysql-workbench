/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#import "WBSplitPanel.h"
#import "MCanvasViewer.h"

namespace bec {
  class UIForm;
};
namespace wb {
  class ModelDiagramForm;
};

@interface WBModelDiagramPanel : WBSplitPanel<CanvasViewerDelegate>

- (instancetype)initWithId:(NSString *)oid formBE:(wb::ModelDiagramForm *)be NS_DESIGNATED_INITIALIZER;

@property(readonly, copy) NSString *identifier;
@property(readonly) bec::UIForm *formBE;

@property(readonly, weak) MCanvasViewer *canvasViewer;
@property(readonly) mdc::CanvasView *canvas;

@property(getter=isClosed, readonly) BOOL closed;

- (void)updateCursor;

- (void)searchString:(NSString *)text;

- (void)canvasToolChanged:(mdc::CanvasView *)canvas;

- (void)setRightSidebar:(BOOL)flag;

- (void)refreshZoom;

- (IBAction)setZoom:(id)sender;

@end
