/* 
 * Copyright (c) 2009, 2012, Oracle and/or its affiliates. All rights reserved.
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


#import "MFRadioButton.h"

#import "MFView.h"
#import "MFMForms.h"

#import "MFPanel.h"

@implementation MFRadioButtonImpl


- (id)initWithObject:(::mforms::RadioButton*)aRadioButton
{
  self= [super initWithObject:aRadioButton buttonType: ::mforms::PushButton];
  if (self)
  {
    [self setButtonType: NSRadioButton];
    [self setBezelStyle: NSRegularSquareBezelStyle];
    
    mTopLeftOffset= NSMakePoint(0, 0);
    mBottomRightOffset= NSMakePoint(0, 0);
    mAddPadding= NO;
    
    [self setTarget:self];
    [self setAction:@selector(performCallback:)];
  }
  return self;
}


- (void)performCallback:(id)sender
{
  dynamic_cast<mforms::RadioButton*>(mOwner)->callback();
}


- (NSSize)minimumSize
{
  return [[self cell] cellSize];
}



static bool radiobutton_create(::mforms::RadioButton *self, int)
{
  [[[MFRadioButtonImpl alloc] initWithObject:self] autorelease];
  
  return true;  
}


static void radiobutton_set_active(::mforms::RadioButton *self, bool flag)
{
  if ( self )
  {
    MFRadioButtonImpl* radiobutton = self->get_data();
    
    if ( radiobutton )
    {
      [radiobutton setState: flag ? NSOnState : NSOffState];
    }
  }
}

static bool radiobutton_get_active(::mforms::RadioButton *self)
{
  if ( self )
  {
    MFRadioButtonImpl* radiobutton = self->get_data();
    
    if ( radiobutton )
    {
      return [radiobutton state] == NSOnState;
    }
  }
  return false;
}


void cf_radiobutton_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_radio_impl.create= &radiobutton_create;
  f->_radio_impl.set_active= &radiobutton_set_active;
  f->_radio_impl.get_active= &radiobutton_get_active;
}


@end

