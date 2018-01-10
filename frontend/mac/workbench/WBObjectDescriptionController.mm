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

#include "grt.h"
#include "grtpp_util.h"

#import "WBObjectDescriptionController.h"

#define COMMIT_TIMEOUT 2

@implementation WBObjectDescriptionController

- (void)setup
{
  [self updateForForm: 0];
}

- (void) dealloc
{
  delete _objectList;
}

- (void)updateForForm:(bec::UIForm*)form
{
  if (_timer)
    [self commit];
  
  _initializing= YES;
  
  std::vector<std::string> items;
  grt::ListRef<GrtObject> new_object_list;
  std::string description;

  if (form)
    description = wb::WBContextUI::get()->get_description_for_selection(form, new_object_list, items);
  else
    description = wb::WBContextUI::get()->get_description_for_selection(new_object_list, items);
  
  // update only if selection was changed
  if (!_objectList || !grt::compare_list_contents(*_objectList, new_object_list))
  {
    [popup removeAllItems];
    
    delete _objectList;
    // Set description text
    _objectList= new grt::ListRef<GrtObject>(new_object_list);
    
    // Set properties
    _multipleItems= items.size() > 1;
    
    // handle different number of selected items
    if (!items.empty())
    {
      for (std::vector<std::string>::const_iterator iter= items.begin(); iter != items.end(); ++iter)
        [popup addItemWithTitle:@(iter->c_str())];
      [popup selectItemAtIndex: 0];
      
      // lock on multi selection
      if (_multipleItems)
      {
        [text setEditable: NO];
        [forceEditButton setHidden:NO];
      }
      else
      {
        [text setEditable: YES];
        [forceEditButton setHidden:YES];
      }
      text.string = @(description.c_str());
    }
    else
    {
      [popup removeAllItems];
      [popup addItemWithTitle: @"No Selection"];
      [popup selectItemAtIndex: 0];
      
      text.string = @"";
      [text setEditable:NO];
      [forceEditButton setHidden:YES];
    }
  }

  _initializing= NO;
}


- (IBAction)changePopup:(id)sender
{
}


- (IBAction)forceEdit:(id)sender
{
  [forceEditButton setHidden:YES];
  [text setEditable:YES];
}



- (void)commit
{
  [_timer invalidate];
  _timer= nil;

  if (_objectList)
    wb::WBContextUI::get()->set_description_for_selection(*_objectList, text.string.UTF8String);
}


- (void)textDidEndEditing:(NSNotification *)aNotification
{
  if (text.editable)
    [self commit];
}


- (void)timerFired:(id)uinfo
{
  [self commit];
}


- (void)textDidChange:(NSNotification *)aNotification
{
  [_timer invalidate];
  _timer= [NSTimer scheduledTimerWithTimeInterval:COMMIT_TIMEOUT
                                           target:self
                                         selector:@selector(timerFired:)
                                         userInfo:nil
                                          repeats:NO];
}

@end
