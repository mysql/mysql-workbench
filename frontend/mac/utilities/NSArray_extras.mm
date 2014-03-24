
#import "NSArray_extras.h"


@implementation NSArray(CPPExtras)

+ (NSArray*)arrayWithCPPStringVector:(const std::vector<std::string> &)v
{
  NSMutableArray *array = [NSMutableArray arrayWithCapacity: v.size()];
  for (std::vector<std::string>::const_iterator i = v.begin(); i != v.end(); ++i)
    [array addObject: [NSString stringWithUTF8String: i->c_str()]];
  return array;
}


+ (NSArray*)arrayWithCPPStringList:(const std::list<std::string> &)l
{
  NSMutableArray *array = [NSMutableArray arrayWithCapacity: l.size()];
  for (std::list<std::string>::const_iterator i = l.begin(); i != l.end(); ++i)
    [array addObject: [NSString stringWithUTF8String: i->c_str()]];
  return array;  
}

@end
