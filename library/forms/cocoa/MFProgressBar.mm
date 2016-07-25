/* 
 * Copyright (c) 2009, 2016, Oracle and/or its affiliates. All rights reserved.
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

#import "MFProgressBar.h"
#import "MFMForms.h"
#import "MFView.h"

@implementation MFProgressBarImpl

- (instancetype)initWithObject: (mforms::ProgressBar*)pbar
{
  self = [super initWithFrame:NSMakeRect(10, 10, 10, 10)];
  if (self != nil)
  {
    self.minValue = 0;
    self.maxValue = 1.0;
    [self setIndeterminate: NO];
    self.minimumSize = { 100, 20 };

    mOwner = pbar;
    mOwner->set_data(self);
  }
  return self;
}


- (mforms::Object*)mformsObject
{
  return mOwner;
}

static bool progressbar_create(mforms::ProgressBar *image)
{
  return [[MFProgressBarImpl alloc] initWithObject: image] != nil;
}


static void progressbar_set_value(mforms::ProgressBar *self, float pct)
{
  if (self)
  {
    static NSTimeInterval lastRedraw= 0.0;
    NSTimeInterval now;
    MFProgressBarImpl *impl= self->get_data();
    float lastValue= impl.doubleValue;
    impl.doubleValue = pct;

    // force a redraw with a time and pct based throttling
    now= [NSDate timeIntervalSinceReferenceDate];
    if (now - lastRedraw > 0.1 || pct - lastValue > 0.01f || pct == 0.0f || pct == 1.0f)
    {
      [impl.window displayIfNeeded];
      lastRedraw= now;
    }
  }
}


static void progressbar_set_started(mforms::ProgressBar *self, bool flag)
{
  if (self)
  {
    MFProgressBarImpl *impl= self->get_data();
    if (flag)
      [impl startAnimation:nil];
    else
      [impl stopAnimation:nil];
  }
}


static void progressbar_set_indeterminate(mforms::ProgressBar *self, bool flag)
{
  if (self)
  {
    MFProgressBarImpl *impl= self->get_data();
    impl.indeterminate = flag;
  }
}

void cf_progressbar_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_progressbar_impl.create= &progressbar_create;
  f->_progressbar_impl.set_value= &progressbar_set_value;
  f->_progressbar_impl.set_started= &progressbar_set_started;
  f->_progressbar_impl.set_indeterminate= &progressbar_set_indeterminate;
}




@end
