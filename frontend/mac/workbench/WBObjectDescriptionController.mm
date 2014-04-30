/*
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "grtpp.h"
#include "grtpp_util.h"

#import "WBObjectDescriptionController.h"

#define COMMIT_TIMEOUT 2

@implementation WBObjectDescriptionController

- (void)setWBContext:(wb::WBContextUI*)be
{
  _wbui= be;
  [self updateForForm: 0];
}

- (void) dealloc
{
  delete _objectList;
  [super dealloc];
}



- (void)updateForForm:(bec::UIForm*)form
{
  if (_timer)
    [self commit];
  
  _initializing= YES;
  
  std::vector<std::string> items;
  grt::ListRef<GrtObject> new_object_list;
  std::string description;
  
//  _selected_form= form;
  
  if (form)
    description = _wbui->get_description_for_selection(form, new_object_list, items);
  else
    description = _wbui->get_description_for_selection(new_object_list, items);
  
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
        [popup addItemWithTitle:[NSString stringWithUTF8String:iter->c_str()]];
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
      [text setString: [NSString stringWithUTF8String:description.c_str()]];
    }
    else
    {
      [popup removeAllItems];
      [popup addItemWithTitle: @"No Selection"];
      [popup selectItemAtIndex: 0];
      
      [text setString:@""];
      [text setEditable:NO];
      [forceEditButton setHidden:YES];
    }
  }/* no change in selection, dont do anything
  else
  {
    [popup removeAllItems];
    [popup addItemWithTitle: @"No Selection"];
    [popup selectItemAtIndex: 0];
    
    [text setString:@""];
    [text setEditable:NO];
  }*/

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
    _wbui->set_description_for_selection(*_objectList, [[text string] UTF8String]);
}


- (void)textDidEndEditing:(NSNotification *)aNotification
{
  if ([text isEditable])
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
