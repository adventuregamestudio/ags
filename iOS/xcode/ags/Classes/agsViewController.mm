#import <QuartzCore/QuartzCore.h>

#import "agsViewController.h"
#import "EAGLView.h"

// From the engine
extern void startEngine(char* filename, char* directory, int loadLastSave);
extern int psp_rotation;



@interface agsViewController ()
@property (nonatomic, retain) EAGLContext *context;
@end

@implementation agsViewController

@synthesize context;


agsViewController* agsviewcontroller;



// Mouse

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



// Keyboard


- (BOOL)canBecomeFirstResponder
{
	return YES;
}

- (void)registerForKeyboardNotifications
{
	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(keyboardWasShown:)
		name:UIKeyboardDidShowNotification object:nil];

	[[NSNotificationCenter defaultCenter] addObserver:self
		selector:@selector(keyboardWillBeHidden:)
		name:UIKeyboardWillHideNotification object:nil];
}

- (void)keyboardWasShown:(NSNotification*)aNotification
{
	NSDictionary* info = [aNotification userInfo];
	CGSize kbSize = [[info objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
	UIEdgeInsets contentInsets = UIEdgeInsetsMake(0.0, 0.0, kbSize.height, 0.0);
}

- (void)keyboardWillBeHidden:(NSNotification*)aNotification
{
	UIEdgeInsets contentInsets = UIEdgeInsetsZero;
}

char lastChar;

extern "C" int ios_get_last_keypress()
{
	int result = lastChar;
	lastChar = 0;
	return result;
}

- (BOOL)hasText
{
	return NO;
}

- (void)insertText:(NSString *)theText
{
	const char* text = [theText cStringUsingEncoding:NSASCIIStringEncoding];
	if (text)
		lastChar = text[0];
}

- (void)deleteBackward
{
	lastChar = 8; // Backspace
}



// Touching


- (IBAction)handleSingleFingerTap:(UIGestureRecognizer *)sender
{
	mouse_button = 1;
}

- (IBAction)handleTwoFingerTap:(UIGestureRecognizer *)sender
{
	mouse_button = 2;
}

BOOL keyboard_active = FALSE;

- (IBAction)handleLongPress:(UIGestureRecognizer *)sender
{
	if (sender.state != UIGestureRecognizerStateBegan)
	  return;

	if (keyboard_active)
		[self resignFirstResponder];
	else
		[self becomeFirstResponder];

	keyboard_active = !keyboard_active;
}

- (IBAction)handleShortLongPress:(UIGestureRecognizer *)sender
{
	if (sender.state != UIGestureRecognizerStateBegan)
	  return;

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
	longPressGesture.minimumPressDuration = 1.5;
	[self.view addGestureRecognizer:longPressGesture];
	[longPressGesture release];
	
	UILongPressGestureRecognizer* shortLongPressGesture = [[UILongPressGestureRecognizer alloc]
	initWithTarget:self action:@selector(handleShortLongPress:)];
	shortLongPressGesture.minimumPressDuration = 0.7;
	[shortLongPressGesture requireGestureRecognizerToFail:longPressGesture];
	[self.view addGestureRecognizer:shortLongPressGesture];
	[shortLongPressGesture release];
}






- (void)showActivityIndicator
{
	UIActivityIndicatorView* indicatorView = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
	[indicatorView startAnimating];
	indicatorView.center = self.view.center;
	indicatorView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
	[self.view addSubview:indicatorView];
	[indicatorView release];
}

- (void)hideActivityIndicator
{
	NSArray *subviews = [self.view subviews];
	for (UIView *view in subviews)
		[view removeFromSuperview];
}


extern "C" void ios_create_screen()
{
	[agsviewcontroller performSelectorOnMainThread:@selector(hideActivityIndicator) withObject:nil waitUntilDone:YES];
}



// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
	[self showActivityIndicator];

	[super viewDidLoad];
	[self.view setMultipleTouchEnabled:YES];
	[self createGestureRecognizers];
	agsviewcontroller = self;
	[self registerForKeyboardNotifications];
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	// Return YES for supported orientations
	if (psp_rotation == 0)
		return YES;
	else if (psp_rotation == 1)
		return UIInterfaceOrientationIsPortrait(interfaceOrientation);
	else if (psp_rotation == 2)
		return UIInterfaceOrientationIsLandscape(interfaceOrientation);	
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

	[NSThread detachNewThreadSelector:@selector(startThread) toTarget:self withObject:nil];  
}

- (void)startThread
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	// Handle any foreground procedures not related to animation here.
	NSArray *searchPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentPath = [searchPaths objectAtIndex:0];
	
	const char* bla = [documentPath UTF8String];
	char path[300];
	strcpy(path, bla);
	strcat(path, "/ags/game/");
	
	char filename[300];
	strcpy(filename, path);
	strcat(filename, "ac2game.dat");
	
	startEngine(filename, path, 0);	

	[pool release];
}


extern volatile int ios_wait_for_ui;

void ios_show_message_box(char* buffer)
{
	NSString* string = [[NSString alloc] initWithUTF8String: buffer];
	[agsviewcontroller performSelectorOnMainThread:@selector(showMessageBox:) withObject:string waitUntilDone:YES];
}

- (void)showMessageBox:(NSString*)text
{
	ios_wait_for_ui = 1;
	UIAlertView *message = [[UIAlertView alloc] initWithTitle:@"Error" message:text delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
	[message show];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
   ios_wait_for_ui = 0;
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
	[super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
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

- (void)didReceiveMemoryWarning
{
	// Releases the view if it doesn't have a superview.
	[super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc. that aren't in use.
}


@end