#import <QuartzCore/QuartzCore.h>

#import "agsViewController.h"
#import "EAGLView.h"

// From the engine
extern void startEngine(char* filename, char* directory, int loadLastSave);
extern int psp_rotation;



@interface agsViewController ()
@property (nonatomic, retain) EAGLContext *context;
@property (readwrite, retain) UIView *inputAccessoryView;
@property (readwrite, assign) BOOL isInPortraitOrientation;
@property (readwrite, assign) BOOL isKeyboardActive;
@property (readwrite, assign) BOOL isIPad;
@end

@implementation agsViewController

@synthesize context, inputAccessoryView, isInPortraitOrientation, isKeyboardActive, isIPad;


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

int lastChar;

extern "C" int ios_get_last_keypress()
{
	int result = lastChar;
	lastChar = 0;
	return result;
}


extern "C" void ios_show_keyboard()
{
	if (agsviewcontroller)
		[agsviewcontroller performSelectorOnMainThread:@selector(showKeyboard) withObject:nil waitUntilDone:YES];	
}

extern "C" void ios_hide_keyboard()
{
	if (agsviewcontroller)
		[agsviewcontroller performSelectorOnMainThread:@selector(hideKeyboard) withObject:nil waitUntilDone:YES];	
}

extern "C" int ios_is_keyboard_visible()
{
	return (agsviewcontroller.isKeyboardActive ? 1 : 0);
}

- (void)showKeyboard
{
	if (!self.isKeyboardActive)
	{
		[self becomeFirstResponder];

		if (self.isInPortraitOrientation)
			[self moveViewAnimated:YES duration:0.25];

		self.isKeyboardActive = TRUE;
	}
}

- (void)hideKeyboard
{
	if (self.isKeyboardActive)
	{
		[self resignFirstResponder];
			
		if (self.isInPortraitOrientation)
			[self moveViewAnimated:NO duration:0.25];
		
		self.isKeyboardActive = FALSE;
	}
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


- (void)createKeyboardButtonBar:(int)openedKeylist
{
	UIToolbar *toolbar;
	BOOL alreadyExists = (self.inputAccessoryView != NULL);
	
	if (alreadyExists)
		toolbar = self.inputAccessoryView;
	else
		toolbar = [[UIToolbar alloc] initWithFrame:CGRectMake(0, 0, self.view.frame.size.width, 30)];

	toolbar.barStyle = UIBarStyleBlackTranslucent;

	NSMutableArray* array = [[NSMutableArray alloc] initWithCapacity:6];
	
	UIBarButtonItem *esc = [[UIBarButtonItem alloc] initWithTitle:@"ESC" style:UIBarButtonItemStyleDone target:self action:@selector(buttonClicked:)];
	[array addObject:esc];
	
	if ((openedKeylist == 1) || self.isIPad)
	{
		UIBarButtonItem* f1 = [[UIBarButtonItem alloc] initWithTitle:@"F1" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		UIBarButtonItem* f2 = [[UIBarButtonItem alloc] initWithTitle:@"F2" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		UIBarButtonItem* f3 = [[UIBarButtonItem alloc] initWithTitle:@"F3" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		UIBarButtonItem* f4 = [[UIBarButtonItem alloc] initWithTitle:@"F4" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		[array addObject:f1];
		[array addObject:f2];
		[array addObject:f3];
		[array addObject:f4];
	}
	else
	{
		UIBarButtonItem* openf1 = [[UIBarButtonItem alloc] initWithTitle:@"F1..." style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		[array addObject:openf1];
	}

	if ((openedKeylist == 5) || self.isIPad)
	{
		UIBarButtonItem* f5 = [[UIBarButtonItem alloc] initWithTitle:@"F5" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		UIBarButtonItem* f6 = [[UIBarButtonItem alloc] initWithTitle:@"F6" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		UIBarButtonItem* f7 = [[UIBarButtonItem alloc] initWithTitle:@"F7" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		UIBarButtonItem* f8 = [[UIBarButtonItem alloc] initWithTitle:@"F8" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		[array addObject:f5];
		[array addObject:f6];
		[array addObject:f7];
		[array addObject:f8];
	}
	else
	{
		UIBarButtonItem* openf5 = [[UIBarButtonItem alloc] initWithTitle:@"F5..." style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		[array addObject:openf5];
	}
	
	if ((openedKeylist == 9) || self.isIPad)
	{
		UIBarButtonItem* f9 = [[UIBarButtonItem alloc] initWithTitle:@"F9" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		UIBarButtonItem* f10 = [[UIBarButtonItem alloc] initWithTitle:@"F10" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		UIBarButtonItem* f11 = [[UIBarButtonItem alloc] initWithTitle:@"F11" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		UIBarButtonItem* f12 = [[UIBarButtonItem alloc] initWithTitle:@"F12" style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		[array addObject:f9];
		[array addObject:f10];
		[array addObject:f11];
		[array addObject:f12];
	}
	else
	{
		UIBarButtonItem* openf9 = [[UIBarButtonItem alloc] initWithTitle:@"F9..." style:UIBarButtonItemStyleBordered target:self action:@selector(buttonClicked:)];
		[array addObject:openf9];
	}

	[toolbar setItems:array animated:YES];

	if (!alreadyExists)
	{
		self.inputAccessoryView = toolbar;
		[toolbar release];
	}
}


- (IBAction)buttonClicked:(UIBarButtonItem *)sender
{
	if (sender.title == @"ESC")
		lastChar = 27;
	else if (sender.title == @"F1")
		lastChar = 0x1000 + 47;
	else if (sender.title == @"F2")
		lastChar = 0x1000 + 48;
	else if (sender.title == @"F3")
		lastChar = 0x1000 + 49;
	else if (sender.title == @"F4")
		lastChar = 0x1000 + 50;
	else if (sender.title == @"F5")
		lastChar = 0x1000 + 51;
	else if (sender.title == @"F6")
		lastChar = 0x1000 + 52;
	else if (sender.title == @"F7")
		lastChar = 0x1000 + 53;
	else if (sender.title == @"F8")
		lastChar = 0x1000 + 54;
	else if (sender.title == @"F9")
		lastChar = 0x1000 + 55;
	else if (sender.title == @"F10")
		lastChar = 0x1000 + 56;
	else if (sender.title == @"F11")
		lastChar = 0x1000 + 57;
	else if (sender.title == @"F12")
		lastChar = 0x1000 + 58;
	else if (sender.title == @"F1...")
		[self createKeyboardButtonBar:1];
	else if (sender.title == @"F5...")
		[self createKeyboardButtonBar:5];
	else if (sender.title == @"F9...")
		[self createKeyboardButtonBar:9];
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


- (void)moveViewAnimated:(BOOL)upwards duration:(float)duration
{
	[UIView beginAnimations:nil context:NULL];
	[UIView setAnimationDuration:duration];
	
	int newTop = 0;
	if (upwards)
	{
		if (self.isIPad)
			newTop = self.view.frame.size.height / -6;
		else
			newTop = self.view.frame.size.height / -4;
	}

	self.view.frame = CGRectMake(0, newTop, self.view.frame.size.width, self.view.frame.size.height);
	[UIView commitAnimations];
}

- (IBAction)handleLongPress:(UIGestureRecognizer *)sender
{
	if (sender.state != UIGestureRecognizerStateBegan)
	  return;

	if (self.isKeyboardActive)
	{
		[self hideKeyboard];
	}
	else
	{
		[self showKeyboard];
	}
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
	self.isIPad = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
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


//- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
	self.isInPortraitOrientation = UIInterfaceOrientationIsLandscape(fromInterfaceOrientation);
	if (self.isKeyboardActive && self.isInPortraitOrientation)
		[self moveViewAnimated:YES duration:0.1];
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
	
	self.isKeyboardActive = FALSE;
	self.isInPortraitOrientation = TRUE;
	
	[self createKeyboardButtonBar:1];
	
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