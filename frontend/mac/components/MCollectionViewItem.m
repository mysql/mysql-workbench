//
//  MCollectionViewItem.m
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 12/Oct/08.
//  Copyright 2008 Sun Microsystems Inc. All rights reserved.
//

#import "MCollectionViewItem.h"
#import "MCollectionViewItemView.h"

@implementation MCollectionViewItem


- (id)copy
{
  id other= [super copy];
  [[other view] setMenu: [[self view] menu]];
  [(MCollectionViewItemView*)[other view] setOwner: other];
  return other;
}


- (void)setView:(NSView*)view
{
  [super setView:view];
  [(MCollectionViewItemView*)view setOwner:self];
}


- (void)setSelected:(BOOL)flag 
{
  [super setSelected:flag];
  
  MCollectionViewItemView* itemView= (MCollectionViewItemView*)[self view];
  if ([itemView isKindOfClass:[MCollectionViewItemView class]]) 
  {
    [itemView setSelected:flag];
    [itemView setNeedsDisplay:YES];
  }
}

@end
