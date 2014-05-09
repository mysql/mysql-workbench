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

#import "WBPluginWindowBase.h"


@implementation WBPluginWindowBase

- (id)initWithModule:(grt::Module*)module GRTManager:(bec::GRTManager*)grtm arguments:(const grt::BaseListRef&)args
{
  self= [super init];
  if (self)
  {
    _module= module;
    _grtm= grtm;
  }
  return self;
}


- (bec::GRTManager*)grtManager
{
  return _grtm;
}

- (void)dealloc
{
  // should be removed once it's made sure that whatever adds an observer, removes it
  [[NSNotificationCenter defaultCenter] removeObserver: self];
  
  [super dealloc];
}


- (void)show
{
}

@end
