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

#include "grtpp.h"
#include "grt/grt_value_inspector.h"

#import "WBObjectPropertiesController.h"
#import "WBColorCell.h"

@interface NSString (WBExtensions)
- (NSString*) stringBySplittingCamelCase;
@end

@implementation NSString (WBExtensions)
- (NSString*) stringBySplittingCamelCase;
{
  // Insert spaces in a CamelCase string.
  int i, c = [self length];
  for (i = 1; i < c; i++) {
    NSRange r = NSMakeRange(i, 1);
    NSString* one = [self substringWithRange: r];
    if ([one isEqualToString: [one uppercaseString]]) {
      NSString* pre = [self substringToIndex: i];
      NSString* post = [self substringFromIndex: i];
      self = [NSString stringWithFormat: @"%@ %@", pre, post];
      i++;
      c++;
    }
  }
  
  return self;
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
    NSString* theType = [NSString stringWithUTF8String: t.c_str()];
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
    if ([[aTableColumn identifier] isEqualToString: @"name"]) {
      mValueInspector->get_field(node, ::bec::ValueInspectorBE::Name, s);
      NSString* name = [NSString stringWithUTF8String: s.c_str()];
      name = [name stringBySplittingCamelCase];
      name = [name stringByAppendingString: @":"];
      retVal = [name capitalizedString];
    }
    else {
      mValueInspector->get_field(node, ::bec::ValueInspectorBE::Value, s);
      retVal = [NSString stringWithUTF8String: s.c_str()];
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
  
  if ([[tableColumn identifier] isEqualToString: @"value"]) {
    PropertyType pt = [self propertyTypeForRowIndex: rowIndex];
    if (pt == PROPERTY_TYPE_COLOR) {
      cell = mColorCell;
    }
    else if (pt == PROPERTY_TYPE_BOOL) {
      cell = mCheckBoxCell;
    }
  }
  
  if (cell == nil) {
    cell = [tableColumn dataCell];
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
  NSString* hex = [NSString stringWithUTF8String: s.c_str()];
  if ([hex length] > 1) {
    hex = [hex substringFromIndex: 1];
  }

  if ([hex length] == 6) {
    NSColorPanel* scp = [NSColorPanel sharedColorPanel];
    NSColor* color = [WBColorCell colorWithHexString: hex];
    [scp setColor: color];
    
    hasColor = YES;
  }
  
  return hasColor;
}



- (void) userDoubleClick: (id) sender;
{
  NSInteger rowIndex = [mTableView selectedRow];
  PropertyType pt = [self propertyTypeForRowIndex: rowIndex];
  if (pt == PROPERTY_TYPE_COLOR) {
    BOOL hasColor = [self updateColorPickerPanelWithColorAtRowIndex: rowIndex];
    if (hasColor) {
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
  const char* cStr = [hex UTF8String];
  NSInteger rowIndex = [mTableView selectedRow];
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
    mValueInspector = _wbui->create_inspector_for_selection(form, items);
    
    // Update color of color picker to match new selection.
    NSInteger rowIndex = [mTableView selectedRow];
    PropertyType pt = [self propertyTypeForRowIndex: rowIndex];
    if (pt == PROPERTY_TYPE_COLOR) {
      [self updateColorPickerPanelWithColorAtRowIndex: rowIndex];
    }
  }
  
  [mTableView reloadData];
}



- (void) setWBContext: (wb::WBContextUI*) be;
{
  _wbui = be;
}



#pragma mark Create + Destroy



- (void) awakeFromNib;
{
  mColorCell = [WBColorCell new];
  [mColorCell setEditable: YES];
  
  mCheckBoxCell = [NSButtonCell new];
  [mCheckBoxCell setButtonType: NSSwitchButton];
  [mCheckBoxCell setControlSize: NSSmallControlSize];
  [mCheckBoxCell setTitle: @""];
  
  [mTableView setTarget: self];
  [mTableView setDoubleAction: @selector(userDoubleClick:)];
}



- (void) dealloc
{
  [mColorCell release];
  [mCheckBoxCell release];
  
  delete mValueInspector;
  
  [super dealloc];
}



@end














