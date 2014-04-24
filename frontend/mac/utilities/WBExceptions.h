/*!
 Copyright 2009 Sun Microsystems, Inc.
*/ 


#import <ExceptionHandling/NSExceptionHandler.h>



/*!
 A catagory on NSException for printing symbolic stack traces.
 */
@interface NSException (WBExceptionExtensions)

- (void) logStackTrace;

@end



/*!
 Implements a delegate to the Cocoa exception handling object.
 */
@interface WBExceptionHandlerDelegate : NSObject
{
}
@end

