#import <Cocoa/Cocoa.h>

#include <stdio.h>

void AGSMacInitPaths(char appdata[PATH_MAX])
{
  @autoreleasepool {
  NSBundle *bundle = [NSBundle mainBundle];
  NSString *resourcedir = [bundle resourcePath];
  [[NSFileManager defaultManager] changeCurrentDirectoryPath:resourcedir];

  NSURL *path = [[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:NULL];
  
  snprintf(appdata, PATH_MAX, "%s", [[path path] UTF8String]);
  }
}

// e.g. "/Users/<username>/Library/Application Support/Steam/steamapps/common/<gamename>"
void AGSMacGetBundleDir(char gamepath[PATH_MAX])
{
  @autoreleasepool {
  NSBundle *bundle = [NSBundle mainBundle];
  NSString *bundleDir = [bundle bundlePath];

  NSString *parentDir = [bundleDir stringByDeletingLastPathComponent];
  strcpy(gamepath, [parentDir UTF8String]);
  }
}

int AGSMacGetFreeSpaceInMB(const char path[PATH_MAX])
{
  @autoreleasepool {
  NSString *dirpath = @(path);
  NSError *error;
  unsigned long long freeSpace = 0;

  if (@available(macOS 10.13, iOS 11.0, *)) {
      NSURL *fileURL = [[NSURL alloc] initFileURLWithPath:dirpath];
      NSDictionary *results = [fileURL resourceValuesForKeys:@[NSURLVolumeAvailableCapacityForImportantUsageKey] error:&error];
      if(results) {
          NSNumber *availableImportantSpace  = results[NSURLVolumeAvailableCapacityForImportantUsageKey];
          freeSpace = availableImportantSpace.longLongValue;
      }
  }

  if(freeSpace == 0) {
      NSDictionary *fileAttributes = [[NSFileManager defaultManager] attributesOfFileSystemForPath:dirpath
                                                                                             error:&error];
      freeSpace = [[fileAttributes objectForKey:NSFileSystemFreeSize] longLongValue];
  }
  return (int)(freeSpace / (1024*1024));
  }
}