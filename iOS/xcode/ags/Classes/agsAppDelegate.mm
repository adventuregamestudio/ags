#import "agsAppDelegate.h"
#import "agsViewController.h"


@implementation agsAppDelegate

@synthesize window;
@synthesize viewController;

volatile int is_in_foreground = 1;
volatile int drawing_in_progress = 0;

extern "C" void ios_resume_sound();

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
	self.window.rootViewController = self.viewController;
	return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
	is_in_foreground = 0;
	do
	{
		//printf("waiting for drawing to finish %d...\n", drawing_in_progress);
		usleep(1000 * 100);
	}
	while (drawing_in_progress);
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	is_in_foreground = 1;
	ios_resume_sound();
}

- (void)applicationWillTerminate:(UIApplication *)application
{
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	// Handle any background procedures not related to animation here.
	is_in_foreground = 0;
	while (drawing_in_progress)
	{
		//printf("waiting for drawing to finish (background) %d...\n", drawing_in_progress);
		usleep(1000 * 100);
	};
	glFinish();
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
