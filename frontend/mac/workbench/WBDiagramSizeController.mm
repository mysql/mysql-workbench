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

#import "WBDiagramSizeController.h"
#import "MCanvasViewer.h"
#import "NSString_extras.h"

#include "base/ui_form.h"
#include "grt.h"

#include "workbench/wb_context_ui.h"
#include "model/wb_diagram_options.h"

@interface TestPanel : NSPanel

@end

@interface WBDiagramSizeController() {
  __weak IBOutlet TestPanel *panel;
  __weak IBOutlet MCanvasViewer *canvas;
  __weak IBOutlet NSTextField *nameField;
  __weak IBOutlet NSTextField *widthField;
  __weak IBOutlet NSTextField *heightField;

  wb::DiagramOptionsBE *_be;

  NSMutableArray *nibObjects;
}

@end

@implementation WBDiagramSizeController

- (instancetype)init {
  self = [super init];
  if (self != nil)  {
    // The diagram size controller is a panel (window) which can be set to auto release on close.
    // However, in order to keep the pattern with all our nib loading this setting is off and we do it manually.
    NSMutableArray *temp;
    if ([NSBundle.mainBundle loadNibNamed: @"DiagramOptions" owner: self topLevelObjects: &temp]) {
      nibObjects = temp;
      [canvas setupQuartz];

      _be = wb::WBContextUI::get()->create_diagram_options_be(canvas.canvas);
      _be->update_size();
      _be->signal_changed()->connect(std::bind(update_size_entries, (__bridge void *)self));

      nameField.stringValue = [NSString stringWithCPPString: _be->get_name()];

      update_size_entries((__bridge void *)self);
    }
  }
  return self;
}

- (void)dealloc {
  delete _be;
}

static void update_size_entries(void *theController) {
  WBDiagramSizeController *controller = (__bridge WBDiagramSizeController *)theController;
  controller->widthField.integerValue = controller->_be->get_xpages();
  controller->heightField.integerValue = controller->_be->get_ypages();
}

- (IBAction)okClicked: (id)sender {
  _be->set_name(nameField.stringValue.UTF8String);
  
  _be->commit();
  [panel performClose:nil];
}

- (void)windowWillClose:(NSNotification *)notification {
  [NSApp stopModal];
}

- (void)controlTextDidEndEditing:(NSNotification *)aNotification {
  if (aNotification.object == widthField)
    _be->set_xpages((int)widthField.integerValue);
  else if (aNotification.object == heightField)
    _be->set_ypages((int)heightField.integerValue);
}  

- (void)showModal {
  [panel makeKeyAndOrderFront:nil];
  
  [NSApp runModalForWindow:panel];
}

@end
