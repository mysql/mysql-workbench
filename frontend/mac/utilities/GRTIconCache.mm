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

#import "WBExtras/GRTIconCache.h"
#import "NSString_extras.h"

@implementation GRTIconCache

static GRTIconCache *instance = NULL;

+ (GRTIconCache*)sharedIconCache
{
  if (!instance)
    instance = [[GRTIconCache alloc] init];
  return instance;
}

- (instancetype)init
{
  if ((self= [super init]) != nil)
  {
    _folderIcon16 = [[[NSWorkspace sharedWorkspace] iconForFile: @"/usr"] copy];
    _folderIcon16.size = NSMakeSize(15, 15);
  }
  return self;
}

- (NSImage*)imageForFolder: (bec::IconSize)size
{
  return _folderIcon16;
}


- (NSImage*)imageForFileName: (NSString *)fname
{
  std::string path = bec::IconManager::get_instance()->get_icon_path(fname.UTF8String);

  return [[NSImage alloc] initWithContentsOfFile:@(path.c_str())];
}


- (NSImage*)imageForIconId: (bec::IconId)icon
{
  std::map<bec::IconId, NSImage*>::const_iterator iter;
  if ((iter = _cache.find(icon)) == _cache.end())
  {
    NSImage *image = [self uncachedImageForIconId: icon];
    _cache[icon] = image;
    return image;
  }
  return iter->second;
}


- (NSImage *)uncachedImageForIconId: (bec::IconId)icon
{
  std::string path = bec::IconManager::get_instance()->get_icon_path(icon);
  if (path.empty())
    return nil;

  return [[NSImage alloc] initWithContentsOfFile: [NSString stringWithCPPString: path]];
}

@end
