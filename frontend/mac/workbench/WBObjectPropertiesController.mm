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

#include "grt.h"
#include "grt/grt_value_inspector.h"

#import "WBObjectPropertiesController.h"
#import "WBColorCell.h"

@implementation NSString (WBExtensions)

- (NSString*)stringBySplittingCamelCase;
{
  // Returns a copy of the receiver with spaces inserted before upper case letters.
  NSString *result = [self copy];
  for (NSUInteger i = 1; i < result.length; ++i) {
    NSRange r = NSMakeRange(i, 1);
    NSString *one = [result substringWithRange: r];
    if ([one isEqualToString: one.uppercaseString]) {
      NSString* pre = [result substringToIndex: i];
      NSString* post = [result substringFromIndex: i];
      result = [NSString stringWithFormat: @"%@ %@", pre, post];
      i++;
    }
  }
  
  return result;
}

@end


@interface WBObjectPropertiesController()
{
  IBOutlet NSTableView* mTableView;

  NSCell* mColorCell;
  NSButtonCell * mCheckBoxCell;

  bec::ValueInspectorBE* mValueInspector;
}

@end

@implementation WBObjectPropertiesController



- (PropertyType) propertyTypeForRowIndex: (NSInteger) rowIndex;
{
  PropertyType propertyType = PROPERTY_TYPE_UNDEFINED;
  
  if ( (mValueInspector != nil) && (rowIndex >= 0) ) {
    bec::NodeId node;
    node.append(rowIndex);
    
    std::string t;
    mValueInspector->get_field(node, ::bec::ValueInspectorBE::EditMethod, t);
    NSString* theType = @(t.c_str());
    if ([theType isEqualToString: @"bool"]) {
      propertyType = PROPERTY_TYPE_BOOL;
    }
    else if ([theType isEqualToString: @"color"]) {
      propertyType = PROPERTY_TYPE_COLOR;
    }
    else {
      propertyType = PROPERTY_TYPE_STRING;
    }
  }
  
  return propertyType;
}



#pragma mark NSTableView supprot



- (NSInteger) numberOfRowsInTableView: (NSTableView*) aTableView;
{
  return (mValueInspector == NULL ? 0 : mValueInspector->count());
}



- (id) tableView: (NSTableView*) aTableView
objectValueForTableColumn: (NSTableColumn*) aTableColumn
             row: (NSInteger) rowIndex;
{
  id retVal = nil;
  
  if (mValueInspector != nil) {
    bec::NodeId node;
    node.append(rowIndex);
    
    std::string s;
    if ([aTableColumn.identifier isEqualToString: @"name"]) {
      mValueInspector->get_field(node, ::bec::ValueInspectorBE::Name, s);
      NSString* name = @(s.c_str());
      name = name.stringBySplittingCamelCase;
      name = [name stringByAppendingString: @":"];
      retVal = name.capitalizedString;
    }
    else {
      mValueInspector->get_field(node, ::bec::ValueInspectorBE::Value, s);
      retVal = @(s.c_str());
    }
  }
  
  return retVal;
}



- (void) tableView: (NSTableView*) aTableView
    setObjectValue: (id) anObject
    forTableColumn: (NSTableColumn*) aTableColumn
               row: (NSInteger) rowIndex;
{
  bec::NodeId node;
  node.append(rowIndex);

  PropertyType pt = [self propertyTypeForRowIndex: rowIndex];
  if (pt == PROPERTY_TYPE_COLOR) {
  }
  else if (pt == PROPERTY_TYPE_BOOL) {
    ssize_t yn = ([anObject boolValue] ? 1 : 0);
    mValueInspector->set_field(node, ::bec::ValueInspectorBE::Value, yn);
  }
  else {
    const char* cStr = [anObject UTF8String];
    std::string s = std::string(cStr);
    mValueInspector->set_convert_field(node, ::bec::ValueInspectorBE::Value, s);
  }
  
  [mTableView reloadData];
}



- (NSCell*) tableView: (NSTableView*) tableView
dataCellForTableColumn: (NSTableColumn*) tableColumn
                  row: (NSInteger) rowIndex;
{
  NSCell* cell = nil;
  
  if ([tableColumn.identifier isEqualToString: @"value"]) {
    PropertyType pt = [self propertyTypeForRowIndex: rowIndex];
    if (pt == PROPERTY_TYPE_COLOR) {
      cell = mColorCell;
    }
    else if (pt == PROPERTY_TYPE_BOOL) {
      cell = mCheckBoxCell;
    }
  }
  
  if (cell == nil) {
    cell = tableColumn.dataCell;
  }
  
  return cell;
}



- (BOOL) tableView: (NSTableView*) aTableView
shouldEditTableColumn: (NSTableColumn*) aTableColumn
               row: (NSInteger) rowIndex;
{
  PropertyType pt = [self propertyTypeForRowIndex: rowIndex];
  return (pt != PROPERTY_TYPE_COLOR);
}



- (void) tableViewSelectionDidChange: (NSNotification*) aNotification;
{
  NSColorPanel* scp = [NSColorPanel sharedColorPanel];
  [scp orderOut: self];
}



#pragma mark GUI



- (BOOL) updateColorPickerPanelWithColorAtRowIndex: (int) rowIndex;
{
  BOOL hasColor = NO;
  
  bec::NodeId node;
  node.append(rowIndex);
  std::string s;
  mValueInspector->get_field(node, ::bec::ValueInspectorBE::Value, s);
  NSString* hex = @(s.c_str());
  if (hex.length > 1) {
    hex = [hex substringFromIndex: 1];
  }

  if (hex.length == 6) {
    NSColorPanel* scp = [NSColorPanel sharedColorPanel];
    NSColor* color = [WBColorCell colorWithHexString: hex];
    scp.color = color;
    
    hasColor = YES;
  }
  
  return hasColor;
}



- (void) userDoubleClick: (id) sender;
{
  NSInteger rowIndex = mTableView.selectedRow;
  PropertyType pt = [self propertyTypeForRowIndex: rowIndex];
  if (pt == PROPERTY_TYPE_COLOR) {
    [self updateColorPickerPanelWithColorAtRowIndex: (int)rowIndex];
    {
      NSColorPanel* scp = [NSColorPanel sharedColorPanel];
      [scp setTarget: self];
      [scp setAction: @selector(userPickColor:)];
      [scp makeKeyAndOrderFront: self];
    }
  }
}



// Called when user picks any color in the color picker.
- (void) userPickColor: (id) sender;
{
  NSColor* c = [sender color];
  NSString* hex = [WBColorCell hexStringWithColor: c];
  const char* cStr = hex.UTF8String;
  NSInteger rowIndex = mTableView.selectedRow;
  if (rowIndex >= 0)
  {
    bec::NodeId node;
    node.append(rowIndex);
    std::string s = std::string(cStr);
    mValueInspector->set_field(node, ::bec::ValueInspectorBE::Value, s);
    [mTableView reloadData];
  }
}



#pragma mark -



- (void) updateForForm: (bec::UIForm*) form;
{
  delete mValueInspector;
  mValueInspector = NULL;
  
  if (form != nil) {
    std::vector<std::string> items;
    mValueInspector = wb::WBContextUI::get()->create_inspector_for_selection(form, items);
    
    // Update color of color picker to match new selection.
    NSInteger rowIndex = mTableView.selectedRow;
    PropertyType pt = [self propertyTypeForRowIndex: rowIndex];
    if (pt == PROPERTY_TYPE_COLOR) {
      [self updateColorPickerPanelWithColorAtRowIndex: (int)rowIndex];
    }
  }
  
  [mTableView reloadData];
}


#pragma mark Create + Destroy

- (void) awakeFromNib;
{
  mColorCell = [WBColorCell new];
  [mColorCell setEditable: YES];
  
  mCheckBoxCell = [NSButtonCell new];
  [mCheckBoxCell setButtonType: NSButtonTypeSwitch];
  mCheckBoxCell.controlSize = NSControlSizeSmall;
  mCheckBoxCell.title = @"";
  
  mTableView.target = self;
  mTableView.doubleAction = @selector(userDoubleClick:);
}

- (void) dealloc
{
  delete mValueInspector;
}



@end














