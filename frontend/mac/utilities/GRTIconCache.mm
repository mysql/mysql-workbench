//
//  GRTIconCache.mm
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 12/Oct/08.
//  Copyright 2008 Sun Microsystems Inc. All rights reserved.
//

#import "WBExtras/GRTIconCache.h"


@implementation GRTIconCache

static GRTIconCache *instance= 0;

+ (GRTIconCache*)sharedIconCache
{
  if (!instance)
    instance= [[GRTIconCache alloc] init];
  return instance;
}

- (id)init
{
  if ((self= [super init]) != nil)
  {
    _cache= new std::map<bec::IconId, NSImage*>();

    _folderIcon16= [[[[NSWorkspace sharedWorkspace] iconForFile:@"/usr"] copy] retain];
    [_folderIcon16 setSize:NSMakeSize(15, 15)];
  }
  return self;
}


- (void)dealloc
{
  [_folderIcon16 release];
  
  for (std::map<bec::IconId, NSImage*>::const_iterator iter= _cache->begin();
       iter != _cache->end(); ++iter)
  {
    [iter->second release];
  }
  delete _cache;
  
  [super dealloc];
}


- (NSImage*)imageForFolder:(bec::IconSize)size
{
  return _folderIcon16;
}


- (NSImage*)imageForFileName:(NSString*)fname
{
  std::string path= bec::IconManager::get_instance()->get_icon_path([fname UTF8String]);

  return [[[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]] autorelease];
}


- (NSImage*)imageForIconId:(bec::IconId)icon
{
  std::map<bec::IconId, NSImage*>::const_iterator iter;
  if ((iter= _cache->find(icon)) == _cache->end())
  {
    NSImage *image= [self uncachedImageForIconId:icon];
    (*_cache)[icon]= [image retain];
    return image;
  }
  return iter->second;
}


- (NSImage*)uncachedImageForIconId:(bec::IconId)icon
{
  std::string path= bec::IconManager::get_instance()->get_icon_path(icon);
  if (path.empty())
    return nil;

  NSImage *image = [[[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]] autorelease];
  [image setScalesWhenResized: NO];
  return image;
}

@end
