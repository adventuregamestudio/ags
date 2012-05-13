
#import <UIKit/UIKit.h>

#import <OpenGLES/EAGL.h>

#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

@interface agsViewController : UIViewController <UIAlertViewDelegate, UIKeyInput, UITextInputTraits>
{
	EAGLContext *context;
}

- (void)createGestureRecognizers;

@end