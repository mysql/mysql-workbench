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

#import <Cocoa/Cocoa.h>
#import "MGridView.h"
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>

class Recordset;

@interface MResultsetViewer : NSObject
{
  IBOutlet NSView *mView;
  IBOutlet MGridView *mTableView;

  NSFont *mFont;
  
  std::list<boost::signals2::connection> mSigConns;
  boost::shared_ptr<Recordset> *mData;

  int mWarnedManyColumns;
  BOOL mPendingRefresh;
}

- (id)initWithRecordset:(boost::shared_ptr<Recordset>)rset;
- (boost::shared_ptr<Recordset>)recordset;
- (NSView*)view;
- (MGridView*)gridView;

- (BOOL)hasPendingChanges;

- (void)rebuildColumns;
- (void)refresh;
- (void)refreshFull;
- (void)close;

- (void)setFont: (NSFont*)font;

- (void)setHeaderIndicator:(int)indicator forColumn:(int)column;
@end
