//
//  MStatusBar.m
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 6/May/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import "MStatusBar.h"


@implementation MStatusBar

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
      mGradient= [[NSGradient alloc] initWithColorsAndLocations:
                  [NSColor colorWithCalibratedRed:197/256.0 green:197/256.0 blue:196/256.0 alpha:1.0],
                  (CGFloat)0.0,
                  [NSColor colorWithCalibratedRed:151/256.0 green:150/256.0 blue:149/256.0 alpha:1.0],
                  (CGFloat)0.76,
                  [NSColor colorWithCalibratedRed:148/256.0 green:147/256.0 blue:147/256.0 alpha:1.0],
                  (CGFloat)1.0,
                  nil];
      
      [[NSNotificationCenter defaultCenter] addObserver: self
                                               selector:@selector(windowBecameKey:)
                                                   name:NSWindowDidBecomeKeyNotification
                                                 object:nil];
      [[NSNotificationCenter defaultCenter] addObserver: self
                                               selector:@selector(windowResignedKey:)
                                                   name:NSWindowDidResignKeyNotification
                                                 object:nil];
    }
    return self;
}

- (void) dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver: self];
  [mGradient release];
  [super dealloc];
}

- (void)windowBecameKey:(NSNotification*)notif
{
  if ([notif object] == [self window])
    [self setNeedsDisplay:YES];
}


- (void)windowResignedKey:(NSNotification*)notif
{
  if ([notif object] == [self window])
    [self setNeedsDisplay:YES];
}


- (void)drawRect:(NSRect)rect 
{
  if ([[self window] isKeyWindow])
  {
    NSRect b= [self bounds];    
            
    [mGradient drawInRect:b angle: 270];

    [[NSColor colorWithCalibratedRed:224/255.0 green:224/255.0 blue:224/255.0 alpha:1.0] set];
    [NSBezierPath strokeLineFromPoint: NSMakePoint(0, NSHeight(b)-1.5) toPoint: NSMakePoint(NSWidth(b), NSHeight(b)-1.5)];

    [[NSColor colorWithDeviceWhite: 81/255.0 alpha: 1.0] set];
    [NSBezierPath strokeLineFromPoint: NSMakePoint(0, NSHeight(b)-0.5) toPoint: NSMakePoint(NSWidth(b), NSHeight(b)-0.5)];
  }
  else
  {
    [[NSColor controlColor] set];
    NSRectFill([self bounds]);
  }
}


- (BOOL)mouseDownCanMoveWindow
{
  return YES;
}


@end
