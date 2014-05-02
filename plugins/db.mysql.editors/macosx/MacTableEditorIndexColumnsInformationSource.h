/*!
 Copyright 2009 Sun Microsystems, Inc.
 */



#import <Cocoa/Cocoa.h>

#import "MacTableEditorInformationSource.h"



class MySQLTableEditorBE;



@interface MacTableEditorIndexColumnsInformationSource : MacTableEditorInformationSource
{
  MySQLTableEditorBE* mBackEnd;
}



- (BOOL) rowEnabled: (NSInteger) rowIndex;

- (void) setRow: (NSInteger) indexRowIndex
        enabled: (BOOL) yn;


- (id) initWithListModel: (bec::ListModel*) model
            tableBackEnd: (MySQLTableEditorBE*) tableBackend;



@end
