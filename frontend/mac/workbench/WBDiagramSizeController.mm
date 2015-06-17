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

#import "WBDiagramSizeController.h"
#import "MCanvasViewer.h"
#import "NSString_extras.h"

#include "base/ui_form.h"
#include "grtpp.h"

#include "workbench/wb_context_ui.h"
#include "model/wb_diagram_options.h"

@implementation WBDiagramSizeController

- (void)dealloc
{
  delete _be;
  [super dealloc];
}


static void update_size_entries(WBDiagramSizeController *self)
{
  [self->widthField setIntegerValue: self->_be->get_xpages()];
  [self->heightField setIntegerValue: self->_be->get_ypages()];
}


- (instancetype)initWithWBContext:(wb::WBContextUI*)wbui
{
  self= [super init];
  if (self)
  {    
    [NSBundle loadNibNamed:@"DiagramOptions" owner:self];

    [canvas lockFocus];
    [canvas setupQuartz];
    [canvas unlockFocus];
    
    _be= wbui->create_diagram_options_be([canvas canvas]);
    _be->update_size();
    _be->signal_changed()->connect(boost::bind(update_size_entries, self));
    
    [nameField setStringValue: [NSString stringWithCPPString:_be->get_name()]];
    
    update_size_entries(self);
  }
  return self;
}


- (IBAction)okClicked:(id)sender
{
  _be->set_name([[nameField stringValue] UTF8String]);
  
  _be->commit();
  [panel performClose:nil];
}


- (void)windowWillClose:(NSNotification *)notification
{
  [NSApp stopModal];
}


- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
  if ([aNotification object] == widthField)
    _be->set_xpages([widthField integerValue]);
  else if ([aNotification object] == heightField)
    _be->set_ypages([heightField integerValue]);
}  


- (void)showModal
{
  [panel makeKeyAndOrderFront:nil];
  
  [NSApp runModalForWindow:panel];
}

@end
