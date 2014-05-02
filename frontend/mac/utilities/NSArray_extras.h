
#import <Cocoa/Cocoa.h>
#include <vector>
#include <list>
#include <string>

@interface NSArray(CPPExtras)

+ (NSArray*)arrayWithCPPStringVector:(const std::vector<std::string> &)v;
+ (NSArray*)arrayWithCPPStringList:(const std::list<std::string> &)l;

@end
