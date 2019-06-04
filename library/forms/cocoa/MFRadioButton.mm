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

#import "MFRadioButton.h"

#import "MFView.h"
#import "MFMForms.h"

#import "MFPanel.h"

@implementation MFRadioButtonImpl

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithObject:(::mforms::RadioButton*)aRadioButton
{
  self = [super initWithObject: aRadioButton buttonType: mforms::PushButton];
  if (self)
  {
    [self setButtonType: NSButtonTypeRadio];
    self.bezelStyle = NSBezelStyleRegularSquare;
    
    mTopLeftOffset= NSMakePoint(0, 0);
    mBottomRightOffset= NSMakePoint(0, 0);
    mAddPadding= NO;
    
    self.target = self;
    self.action = @selector(performCallback:);
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithObject: (mforms::Button *)button buttonType: (mforms::ButtonType)type
{
  mforms::RadioButton *radio = dynamic_cast<mforms::RadioButton *>(button);
  if (radio == NULL)
    return nil;
  return [self initWithObject: radio];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)performCallback:(id)sender
{
  dynamic_cast<mforms::RadioButton*>(mOwner)->callback();
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)minimumSize
{
  NSSize size = super.minimumSize;
  return { MAX(self.cell.cellSize.width, size.width), MAX(self.cell.cellSize.height, size.height) };
}

//----------------------------------------------------------------------------------------------------------------------

- (NSAccessibilityRole)accessibilityRole {
  return NSAccessibilityRadioButtonRole;
}

//----------------------------------------------------------------------------------------------------------------------

- (nullable NSNumber *)accessibilityValue {
    MFRadioButtonImpl* radiobutton = mOwner->get_data();
    short int value = 0;
    if(radiobutton.state == NSControlStateValueOn)
        value = 1;
    else if (radiobutton.state == NSControlStateValueMixed)
        value = 2;
    return @(value);
}

//----------------------------------------------------------------------------------------------------------------------

static bool radiobutton_create(::mforms::RadioButton *self, int)
{
  return [[MFRadioButtonImpl alloc] initWithObject: self] != nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void radiobutton_set_active(::mforms::RadioButton *self, bool flag)
{
  if ( self )
  {
    MFRadioButtonImpl* radiobutton = self->get_data();
    
    if ( radiobutton )
    {
      radiobutton.state = flag ? NSControlStateValueOn : NSControlStateValueOff;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static bool radiobutton_get_active(::mforms::RadioButton *self)
{
  if ( self )
  {
    MFRadioButtonImpl* radiobutton = self->get_data();
    
    if ( radiobutton )
    {
      return radiobutton.state == NSControlStateValueOn;
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void cf_radiobutton_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_radio_impl.create= &radiobutton_create;
  f->_radio_impl.set_active= &radiobutton_set_active;
  f->_radio_impl.get_active= &radiobutton_get_active;
}

@end
