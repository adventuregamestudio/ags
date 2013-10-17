#import <Cocoa/Cocoa.h>

void AGSMAcInitPaths(char gamename[256])
{
  NSAutoreleasePool *p = [[NSAutoreleasePool alloc] init];

  NSBundle *bundle = [NSBundle mainBundle];
  NSString *resourcedir = [bundle resourcePath];
  [[NSFileManager defaultManager] changeCurrentDirectoryPath:resourcedir];

  strcpy(gamename, "agsgame.dat");

  [p drain];
}