
#import <UIKit/UIKit.h>

#import <OpenGLES/EAGL.h>

#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

@interface agsViewController : UIViewController <UIAlertViewDelegate, UIKeyInput, UITextInputTraits>
{
    EAGLContext *context;
    BOOL isInPortraitOrientation;
    BOOL isKeyboardActive;
    BOOL isIPad;
}

- (void)createGestureRecognizers;
- (void)createKeyboardButtonBar:(int)openedKeylist;
- (void)moveViewAnimated:(BOOL)upwards duration:(float)duration;
- (BOOL)prefersStatusBarHidden;

@end
