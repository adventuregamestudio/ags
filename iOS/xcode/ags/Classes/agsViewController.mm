#import <QuartzCore/QuartzCore.h>

#import "agsViewController.h"
#import "EAGLView.h"

extern void startEngine(char* filename, char* directory, int loadLastSave);
extern "C" void ios_render();

@interface agsViewController ()
@property (nonatomic, retain) EAGLContext *context;
@property (nonatomic, assign) CADisplayLink *displayLink;
@end

@implementation agsViewController

@synthesize animating, context, displayLink;


agsViewController* agsviewcontroller;

int mouse_button = 0;
int mouse_position_x = 0;
int mouse_position_y = 0;
int mouse_relative_position_x = 0;
int mouse_relative_position_y = 0;

extern "C"
{

	int ios_poll_mouse_buttons()
	{
		int temp_button = mouse_button;
		mouse_button = 0;
		return temp_button;
	}

	void ios_poll_mouse_relative(int* x, int* y)
	{
		*x = mouse_relative_position_x;
		*y = mouse_relative_position_y;
	}


	void ios_poll_mouse_absolute(int* x, int* y)
	{
		*x = mouse_position_x;
		*y = mouse_position_y;
	}

}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
	[super viewDidLoad];
	[self.view setMultipleTouchEnabled:YES];
	[self createGestureRecognizers];
	agsviewcontroller = self;
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	// Return YES for supported orientations
	return YES;
}


- (void)createGestureRecognizers
{
	UITapGestureRecognizer* singleFingerTap = [[UITapGestureRecognizer alloc]
	initWithTarget:self action:@selector(handleSingleFingerTap:)];
	singleFingerTap.numberOfTapsRequired = 1;
	singleFingerTap.numberOfTouchesRequired = 1;
	[self.view addGestureRecognizer:singleFingerTap];
	[singleFingerTap release];

	UITapGestureRecognizer* twoFingerTap = [[UITapGestureRecognizer alloc]
	initWithTarget:self action:@selector(handleTwoFingerTap:)];
	twoFingerTap.numberOfTapsRequired = 1;
	twoFingerTap.numberOfTouchesRequired = 2;
	[self.view addGestureRecognizer:twoFingerTap];
	[twoFingerTap release];	
	
	UILongPressGestureRecognizer* longPressGesture = [[UILongPressGestureRecognizer alloc]
	initWithTarget:self action:@selector(handleLongPress:)];
	[self.view addGestureRecognizer:longPressGesture];
	[longPressGesture release];
}


- (IBAction)handleSingleFingerTap:(UIGestureRecognizer *)sender
{
	mouse_button = 1;
}

- (IBAction)handleTwoFingerTap:(UIGestureRecognizer *)sender
{
	mouse_button = 2;
}

- (IBAction)handleLongPress:(UIGestureRecognizer *)sender
{
	mouse_button = 10;
}


- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
	UITouch* touch = [touches anyObject];
	CGPoint touchPoint = [touch locationInView:self.view];	
	mouse_position_x = touchPoint.x;
	mouse_position_y = touchPoint.y;
}


-(void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	UITouch* touch = [touches anyObject];
	CGPoint touchPoint = [touch locationInView:self.view];	
	mouse_position_x = touchPoint.x;
	mouse_position_y = touchPoint.y;
}


- (void)awakeFromNib
{
	EAGLContext* aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
	
	if (!aContext)
	NSLog(@"Failed to create ES context");
	else if (![EAGLContext setCurrentContext:aContext])
	NSLog(@"Failed to set ES context current");
	
	self.context = aContext;
	[aContext release];
	
	[(EAGLView *)self.view setContext:context];
	[(EAGLView *)self.view setFramebuffer];
	
	animating = FALSE;
	animationFrameInterval = 1;
	self.displayLink = nil;	

	[NSThread detachNewThreadSelector:@selector(startThread) toTarget:self withObject:nil];  
}

- (void)startThread
{
	// Handle any foreground procedures not related to animation here.
	NSArray *searchPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentPath = [searchPaths objectAtIndex:0];
	
	const char* bla = [documentPath UTF8String];
	char path[300];
	strcpy(path, bla);
	strcat(path, "/game/");
	
	char filename[300];
	strcpy(filename, path);
	strcat(filename, "ac2game.dat");
	
	animating = FALSE;
	animationFrameInterval = 1;
	self.displayLink = nil;
	startEngine(filename, path, 0);	
}


- (void)dealloc
{
	if ([EAGLContext currentContext] == context)
	[EAGLContext setCurrentContext:nil];
	
	[context release];
	
	[super dealloc];
}

- (void)viewWillAppear:(BOOL)animated
{
	[self startAnimation];
	
	[super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
	[self stopAnimation];
	
	[super viewWillDisappear:animated];
}

- (void)viewDidUnload
{
	[super viewDidUnload];

	// Tear down context.
	if ([EAGLContext currentContext] == context)
	[EAGLContext setCurrentContext:nil];
	self.context = nil;	
}

- (NSInteger)animationFrameInterval
{
	return animationFrameInterval;
}

- (void)setAnimationFrameInterval:(NSInteger)frameInterval
{
	/*
	Frame interval defines how many display frames must pass between each time the display link fires.
	The display link will only fire 30 times a second when the frame internal is two on a display that refreshes 60 times a second. The default frame interval setting of one will fire 60 times a second when the display refreshes at 60 times a second. A frame interval setting of less than one results in undefined behavior.
	*/
	if (frameInterval >= 1)
	{
		animationFrameInterval = frameInterval;
		
		if (animating)
		{
			[self stopAnimation];
			[self startAnimation];
		}
	}
}

- (void)startAnimation
{
	if (!animating)
	{
		CADisplayLink *aDisplayLink = [[UIScreen mainScreen] displayLinkWithTarget:self selector:@selector(drawFrame)];
		[aDisplayLink setFrameInterval:animationFrameInterval];
		[aDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		self.displayLink = aDisplayLink;
		
		animating = TRUE;
	}
}

- (void)stopAnimation
{
	if (animating)
	{
		[self.displayLink invalidate];
		self.displayLink = nil;
		animating = FALSE;
	}
}

- (void)drawFrame
{
	[(EAGLView *)self.view setFramebuffer];
	
	ios_render();
	
	[(EAGLView *)self.view presentFramebuffer];
	
}

- (void)didReceiveMemoryWarning
{
	// Releases the view if it doesn't have a superview.
	[super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc. that aren't in use.
}


@end
