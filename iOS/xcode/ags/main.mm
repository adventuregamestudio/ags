
#import <UIKit/UIKit.h>
#import <stdio.h>
#import "Classes/agsAppDelegate.h"

int main(int argc, char *argv[])
{
    // Disable buffering for stdout
    setvbuf(stdout, NULL, _IONBF, 0);
    
    //j removed for storyboard
    /*
     NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
     int result = UIApplicationMain(argc, argv, nil, nil);
     [pool release];*/
    
    @autoreleasepool {
        
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([agsAppDelegate class]));
        
    }
    
    //j return result;
}
