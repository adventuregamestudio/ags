
#import <UIKit/UIKit.h>

@class agsViewController;

@interface agsAppDelegate : NSObject <UIApplicationDelegate> {
	UIWindow *window;
	agsViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet agsViewController *viewController;

@end

