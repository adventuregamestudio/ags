
#import "agsAppDelegate.h"
#import "agsViewController.h"


@implementation agsAppDelegate

@synthesize window;
@synthesize viewController;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
	self.window.rootViewController = self.viewController;
	return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
}

- (void)applicationWillTerminate:(UIApplication *)application
{
}

int is_in_foreground = 1;

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	// Handle any background procedures not related to animation here.
	is_in_foreground = 0;
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	// Handle any foreground procedures not related to animation here.
	is_in_foreground = 1;
}

- (void)dealloc
{
	[viewController release];
	[window release];
	
	[super dealloc];
}

@end
