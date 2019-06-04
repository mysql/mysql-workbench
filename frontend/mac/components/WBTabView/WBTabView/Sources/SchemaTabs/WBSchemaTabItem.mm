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

#import "WBTabView.h"

#import "WBSchemaTabItem.h"
#import "CGColorUtilities.h"

//----------------------------------------------------------------------------------------------------------------------

static NSColor *activeTextColorLight = [NSColor colorWithDeviceWhite: 0.1 alpha: 1];
static NSColor *inactiveTextColorLight = [NSColor colorWithDeviceWhite: 0.2 alpha: 1];
static NSColor *activeTextColorDark = [NSColor colorWithDeviceWhite: 1 alpha: 1];
static NSColor *inactiveTextColorDark = [NSColor colorWithDeviceWhite: 0.8 alpha: 1];

static NSColor *activeBackgroundColorLight = [NSColor colorWithDeviceWhite: 1 alpha: 1];
static NSColor *inactiveBackgroundColorLight = [NSColor colorWithDeviceWhite: 0.9 alpha: 1];
static NSColor *activeBackgroundColorDark = [NSColor colorWithDeviceWhite: 0.25 alpha: 1];
static NSColor *inactiveBackgroundColorDark = [NSColor colorWithDeviceWhite: 0.15 alpha: 1];

static NSColor *lineColorLight = [NSColor colorWithDeviceWhite: 0.6 alpha: 1];
static NSColor *lineColorDark = [NSColor colorWithDeviceWhite: 0.3 alpha: 1];

@implementation WBSchemaTabItem

- (void)updateAppearance {
	[super updateAppearance];
		
  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = owner.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }
  BOOL isActive = owner.window.mainWindow;

	if (mState == NSControlStateValueOn) {
		mSideLeft.opacity = 1;
		mSideRight.opacity = 1;
    mTabBackground.opacity = 1;
	} else {
		mSideLeft.opacity = 0;
		mSideRight.opacity = 0;
    mTabBackground.opacity = 0;
	}

  NSBundle *bundle = [NSBundle bundleForClass: self.class];
  if (isDark) {
    mIcon.contents = [bundle imageForResource: @"SchemaTabIconDark"];
    mSideLeft.backgroundColor = lineColorDark.CGColor;
    mSideRight.backgroundColor = lineColorDark.CGColor;

    if (isActive) {
      mTabBackground.backgroundColor = activeBackgroundColorDark.CGColor;
      mTitleLayer.foregroundColor = activeTextColorDark.CGColor;
    } else {
      mTabBackground.backgroundColor = inactiveBackgroundColorDark.CGColor;
      mTitleLayer.foregroundColor = inactiveTextColorDark.CGColor;
    }
  } else {
    mIcon.contents = [bundle imageForResource: @"SchemaTabIconLight"];
    mSideLeft.backgroundColor = lineColorLight.CGColor;
    mSideRight.backgroundColor = lineColorLight.CGColor;

    if (isActive) {
      mTabBackground.backgroundColor = activeBackgroundColorLight.CGColor;
      mTitleLayer.foregroundColor = activeTextColorLight.CGColor;
    } else {
      mTabBackground.backgroundColor = inactiveBackgroundColorLight.CGColor;
      mTitleLayer.foregroundColor = inactiveTextColorLight.CGColor;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setLabel: (NSString*)label {
  [super setLabel: label];
	
	CGRect r = mTitleLayer.frame;
  r.size = mTitleLayer.preferredFrameSize;
  mTitleLayer.frame = r;
}

//----------------------------------------------------------------------------------------------------------------------

- (CGFloat)preferredWidth {
  CGFloat result = mTitleLayer.preferredFrameSize.width;
  result += 24 + mIcon.preferredFrameSize.width; // 24 = 2 x spacing (left/right) + gap between text and icon.

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

- (WBSchemaTabItem*)initWithOwner: (WBTabView *)owner
                       identifier: (id)identifier
                            label: (NSString*)label {
	self = [super init];
	
	if (self != nil) {
    self->owner = owner;
		mIdentifier = identifier;
		
		mHasIcon = YES;
    mState = NSControlStateValueOn;
		mTabDirection = WBTabDirectionUp;
		mTabPlacement = WBTabPlacementTop;
		
		CGRect frame = CGRectZero;
		frame.size = CGSizeMake(120, 84);
    self.frame = frame;

		CGFloat horizon = frame.size.height / 2;
		
    NSBundle *bundle = [NSBundle bundleForClass: self.class];

    // Icon layer.
    NSImage *image = [bundle imageForResource: @"SchemaTabIconLight"];

    mIcon = [CALayer layer];
    mIcon.frame = {{ 9, horizon + (horizon - image.size.height) / 2 }, image.size };
    mIcon.contents = image;
    mIcon.contentsScale = owner.window.backingScaleFactor;
    [self addSublayer: mIcon];

    // Title layer.
    mTitleLayer = [CATextLayer layer];
    mTitleLayer.autoresizingMask = (kCALayerMaxXMargin | kCALayerMinYMargin);

    NSFont *font = [NSFont boldSystemFontOfSize: 14];
    mTitleLayer.font = (__bridge CFTypeRef _Nullable)(font);
    mTitleLayer.fontSize = 14;
    mTitleLayer.contentsScale = owner.window.backingScaleFactor;

    [self addSublayer: mTitleLayer];
    [self setLabel: label];

    CGRect titleFrame = mTitleLayer.frame;
    titleFrame.origin = { 15 + image.size.width, horizon + (horizon - mTitleLayer.preferredFrameSize.height) / 2};
    mTitleLayer.frame = titleFrame;

    // Side lines.
    CGRect r = CGRectMake(0, horizon, 1, horizon);
    mSideLeft = [CALayer layer];
    mSideLeft.frame = r;
    mSideLeft.autoresizingMask = (kCALayerMaxXMargin | kCALayerMinYMargin);
    [self addSublayer: mSideLeft];

    r = CGRectMake(frame.size.width - 1, horizon, 1, horizon);
    mSideRight = [CALayer layer];
    mSideRight.frame = r;
    mSideRight.autoresizingMask = (kCALayerMinXMargin | kCALayerMinYMargin);
    [self addSublayer: mSideRight];

    // Background layer displayed when the tab is selected.
    mTabBackground = [CALayer layer];
    r = CGRectMake(0, horizon, frame.size.width, horizon);
    mTabBackground.frame = r;
    mTabBackground.autoresizingMask = (kCALayerWidthSizable | kCALayerHeightSizable);
    mTabBackground.zPosition = -1;
    [self addSublayer: mTabBackground];

		frame.size.width = self.preferredWidth;
		self.frame = frame;

		[self setEnabled: YES]; // Also updates colors.
	}
	
	return self;
}

//----------------------------------------------------------------------------------------------------------------------

+ (WBTabItem*)tabItemWithOwner: (WBTabView *)owner
                    identifier: (id)identifier
                         label: (NSString*)label {
	return [[WBSchemaTabItem alloc] initWithOwner: owner
                                     identifier: identifier
                                          label: label];
}

@end

//----------------------------------------------------------------------------------------------------------------------
