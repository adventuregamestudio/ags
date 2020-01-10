#import <QuartzCore/QuartzCore.h>

#import "agsViewController.h"
#import "EAGLView.h"

// From the engine
extern void startEngine(char* filename, char* directory, int loadLastSave);
extern int psp_rotation;

@interface agsViewController ()
@property (nonatomic, retain) EAGLContext *context;
@property (readwrite, retain) UIView *inputAccessoryView;
@property (readwrite, assign) BOOL isIPad;
@end

@implementation agsViewController

@synthesize context, inputAccessoryView, isIPad;

char* ios_document_directory;

agsViewController* agsviewcontroller;

//JG Is this an iPhone or iPad?
extern "C" bool isPhone()
{
    UIUserInterfaceIdiom idiom = UI_USER_INTERFACE_IDIOM();
    
    return (idiom==UIUserInterfaceIdiomPhone);
}

// Mouse

int mouse_button = 0;
int mouse_button_queued = 0;
bool mouse_button_held = false;
int mouse_position_x = 0;
int mouse_position_y = 0;
int mouse_relative_position_x = 0;
int mouse_relative_position_y = 0;

extern "C"
{
    int ios_poll_mouse_buttons()
    {
        int temp_button = mouse_button;
        if (!mouse_button_held)
            mouse_button = 0; //JG - Drag'n'Drop
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
    
    float get_device_scale()
    {
        float deviceScale = [[UIScreen mainScreen] scale];
        return deviceScale;
    }
}

- (void)updateMouseCoords:(int) x y:(int) y
{
    mouse_relative_position_x = mouse_position_x - x;
    mouse_relative_position_y = mouse_position_y - y;
    mouse_position_x = x;
    mouse_position_y = y;
}

- (void)updateMouseButton
{
    mouse_button = mouse_button_queued;
    mouse_button_queued = 0;
}

// Keyboard


- (BOOL)canBecomeFirstResponder
{
    return YES;
}

- (void)registerForKeyboardNotifications
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillBeShown:)
                                                 name:UIKeyboardWillShowNotification object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillBeHidden:)
                                                 name:UIKeyboardWillHideNotification object:nil];
}

- (void) keyboardWillBeShown:(NSNotification*)aNotification
{
    if (self.isInPortraitOrientation)
        [self moveViewAnimated:YES duration:0.25];
}

- (void) keyboardWillBeHidden:(NSNotification*)aNotification
{
    if (self.isInPortraitOrientation)
        [self moveViewAnimated:NO duration:0.25];
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
    return ([agsviewcontroller isFirstResponder]);
}

- (void)showKeyboard
{
    if (![self isFirstResponder])
    {
        [self becomeFirstResponder];
    }
}

- (void)hideKeyboard
{
    if ([self isFirstResponder])
    {
        [self resignFirstResponder];
    }
}

- (BOOL)isKeyboardActive
{
    return [self isFirstResponder];
}

- (BOOL)hasText
{
    return NO;
}

- (void)insertText:(NSString *)theText
{
    const char* text = [theText cStringUsingEncoding:NSASCIIStringEncoding];
    if (text) {
        lastChar = text[0];
    }
}

- (void)deleteBackward
{
    lastChar = 8; // Backspace
}

- (void)createKeyboardButtonBar:(int)openedKeylist
{
    UIToolbar *toolbar;
    BOOL alreadyExists = (self.view.inputAccessoryView != NULL);
    
    if (alreadyExists)
        toolbar = self.view.inputAccessoryView;
    else
        toolbar = [[UIToolbar alloc] initWithFrame:CGRectMake(0, 0, self.view.frame.size.width, 42)];
    
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
    
    [array release]; //JG fix memory leak
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
    CGPoint point = [sender locationInView:self.view];
    
    [self updateMouseCoords:point.x y:point.y];
    
    mouse_button_queued = 1;
    
    [self performSelector:@selector(updateMouseButton) withObject:nil afterDelay:0.1];
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

//JG - Drag'n'Drop

- (IBAction)handleTwoFingerSwipe:(UISwipeGestureRecognizer *)sender
{
    if (sender.direction == UISwipeGestureRecognizerDirectionUp)
        [self showKeyboard];
    else
        [self hideKeyboard];
}

- (IBAction)handleShortLongPress:(UIGestureRecognizer *)sender
{
    CGPoint point = [sender locationInView:self.view];
    
    [self updateMouseCoords:point.x y:point.y];
    
    if (sender.state == UIGestureRecognizerStateBegan)
    {
        mouse_button_queued = 1;
        mouse_button_held = true;
        
        [self performSelector:@selector(updateMouseButton) withObject:nil afterDelay:0.1];
    }
    else if(sender.state == UIGestureRecognizerStateEnded)
    {
        mouse_button_held = false;
    }
}

- (IBAction)handlePanOneFinger:(UIGestureRecognizer *)sender
{
    CGPoint point = [sender locationInView:self.view];
    
    [self updateMouseCoords:point.x y:point.y];
    
    if (sender.state == UIGestureRecognizerStateEnded)
    {
        // Touches aren't accurate at the bottom edge so if touches end around there we put you on the edge
        if(self.view.bounds.size.height - point.y < 8)
            [self updateMouseCoords:point.x y:self.view.bounds.size.height];
    }
    
}

- (void)createGestureRecognizers
{
    //JG - Drag'n'Drop
    UIPanGestureRecognizer *panOneFinger = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePanOneFinger:)];
    panOneFinger.maximumNumberOfTouches = 1;
    panOneFinger.delaysTouchesBegan = NO;
    panOneFinger.delaysTouchesEnded = NO;
    panOneFinger.cancelsTouchesInView = YES;
    [self.view addGestureRecognizer:panOneFinger];
    [panOneFinger release];
    
    UITapGestureRecognizer* singleFingerTap = [[UITapGestureRecognizer alloc]
                                               initWithTarget:self action:@selector(handleSingleFingerTap:)];
    singleFingerTap.numberOfTapsRequired = 1;
    singleFingerTap.numberOfTouchesRequired = 1;
    singleFingerTap.delaysTouchesBegan = NO;
    singleFingerTap.delaysTouchesEnded = NO;
    [self.view addGestureRecognizer:singleFingerTap];
    [singleFingerTap release];
    
    UITapGestureRecognizer* twoFingerTap = [[UITapGestureRecognizer alloc]
                                            initWithTarget:self action:@selector(handleTwoFingerTap:)];
    twoFingerTap.numberOfTapsRequired = 1;
    twoFingerTap.numberOfTouchesRequired = 2;
    twoFingerTap.delaysTouchesBegan = NO;
    twoFingerTap.delaysTouchesEnded = NO;
    [self.view addGestureRecognizer:twoFingerTap];
    [twoFingerTap release];
    
    UISwipeGestureRecognizer* twoFingerSwipeUp = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(handleTwoFingerSwipe:)];
    twoFingerSwipeUp.direction = UISwipeGestureRecognizerDirectionUp;
    twoFingerSwipeUp.numberOfTouchesRequired = 2;
    twoFingerSwipeUp.delaysTouchesBegan = NO;
    twoFingerSwipeUp.delaysTouchesEnded = NO;
    twoFingerSwipeUp.cancelsTouchesInView = NO;
    [self.view addGestureRecognizer:twoFingerSwipeUp];
    [twoFingerSwipeUp release];
    
    UISwipeGestureRecognizer* twoFingerSwipeDown = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(handleTwoFingerSwipe:)];
    twoFingerSwipeDown.direction = UISwipeGestureRecognizerDirectionDown;
    twoFingerSwipeDown.numberOfTouchesRequired = 2;
    twoFingerSwipeDown.delaysTouchesBegan = NO;
    twoFingerSwipeDown.delaysTouchesEnded = NO;
    twoFingerSwipeDown.cancelsTouchesInView = NO;
    [self.view addGestureRecognizer:twoFingerSwipeDown];
    [twoFingerSwipeUp release];
    
    /*
     UILongPressGestureRecognizer* longPressGesture = [[UILongPressGestureRecognizer alloc]
     initWithTarget:self action:@selector(handleLongPress:)];
     longPressGesture.minimumPressDuration = 1.5;
     [self.view addGestureRecognizer:longPressGesture];
     [longPressGesture release];
     */
    UILongPressGestureRecognizer* shortLongPressGesture = [[UILongPressGestureRecognizer alloc]
                                                           initWithTarget:self action:@selector(handleShortLongPress:)];
    shortLongPressGesture.minimumPressDuration = 0.3;
    shortLongPressGesture.allowableMovement = 10;
    shortLongPressGesture.delaysTouchesBegan = NO;
    shortLongPressGesture.delaysTouchesEnded = NO;
    shortLongPressGesture.cancelsTouchesInView = NO;
    //[shortLongPressGesture requireGestureRecognizerToFail:longPressGesture];
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
    [super viewDidLoad];
    [self showActivityIndicator];
    [self.view setMultipleTouchEnabled:YES];
    [self createGestureRecognizers];
    agsviewcontroller = self;
    self.isIPad = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
}

- (BOOL)isInPortraitOrientation
{
    return UIInterfaceOrientationIsPortrait([UIApplication sharedApplication].statusBarOrientation);
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
    else
        return NO;
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    if (self.isKeyboardActive)
    {
        if (self.isInPortraitOrientation)
            [self moveViewAnimated:YES duration:0.1];
        else
            [self moveViewAnimated:NO duration:0];
    }
}


- (void)awakeFromNib
{
    [super awakeFromNib];
    
    EAGLContext* aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
    
    if (!aContext)
        NSLog(@"Failed to create ES context");
    else if (![EAGLContext setCurrentContext:aContext])
        NSLog(@"Failed to set ES context current");
    else NSLog(@"Successfully set ES context current");
    
    self.context = aContext;
    [aContext release];
    
    [(EAGLView *)self.view setContext:context];
    [(EAGLView *)self.view setFramebuffer];
    
    [self registerForKeyboardNotifications];
    [self createKeyboardButtonBar:1];
    
    [NSThread detachNewThreadSelector:@selector(startThread) toTarget:self withObject:nil];
}

- (void)startThread
{
    //NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    // Handle any foreground procedures not related to animation here.
    NSArray *searchPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentPath = [searchPaths objectAtIndex:0];
    
    const char* temp_document_dir = [documentPath UTF8String];
    ios_document_directory = (char*)malloc(strlen(temp_document_dir) + 1);
    strcpy(ios_document_directory, temp_document_dir);
    
    char path[300];
    strcpy(path, temp_document_dir);
#if !defined (AGS_PLATFORM_OS_IOS)
    strcat(path, "/ags/game/");
#else
    strcat(path, "/");
#endif
    char filename[300];
    
    /*#if defined (IOS_VERSION)
     //JG
     NSString *filePath = [[NSBundle mainBundle] pathForResource:@"ac2game"
     ofType:@"dat"];
     const char * resourceChars = [filePath UTF8String];
     strcpy(filename, resourceChars);
     #endif*/
    strcpy(filename, path);
    //strcat(filename, "ac2game.dat");
    
    startEngine(filename, path, 0);
    
    //[pool release];
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
    // Tear down context.
    if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
    self.context = nil;
    
    [super viewDidUnload];
}


- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc. that aren't in use.
}

- (BOOL) prefersStatusBarHidden
{
    return YES;
}

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures
{
    return UIRectEdgeAll;
}

@end
