
#import <QuartzCore/QuartzCore.h>

#import "EAGLView.h"

@interface EAGLView (PrivateMethods)
- (void)createFramebuffer;
- (void)deleteFramebuffer;
@end

@implementation EAGLView

@dynamic context;

EAGLView* eaglview;

// From agsViewController.mm
extern int mouse_position_x;
extern int mouse_position_y;


void ios_swap_buffers()
{
	if (eaglview)
		[eaglview presentFramebuffer];
}

void ios_select_buffer()
{
	if (eaglview)
		[eaglview setFramebuffer];
}

// You must implement this method
+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

//The EAGL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:.
- (id)initWithCoder:(NSCoder*)coder
{
	self = [super initWithCoder:coder];
	if (self)
	{
		CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
		
		eaglLayer.opaque = TRUE;
		eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking,
			kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
	}
	
	eaglview = self;
	
	return self;
}

- (void)dealloc
{
	[self deleteFramebuffer];    
	[context release];
	
	[super dealloc];
}

- (EAGLContext *)context
{
	return context;
}

- (void)setContext:(EAGLContext *)newContext
{
	if (context != newContext)
	{
		[self deleteFramebuffer];
		
		[context release];
		context = [newContext retain];
		
		[EAGLContext setCurrentContext:nil];
	}
}

extern void ios_initialize_renderer(int w, int h);

- (void)createFramebuffer
{
	if (context && !defaultFramebuffer)
	{
		[EAGLContext setCurrentContext:context];
		
		// Create default framebuffer object.
		glGenFramebuffers(1, &defaultFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
		
		// Create color render buffer and allocate backing store.
		glGenRenderbuffers(1, &colorRenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
		[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &framebufferWidth);
		glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &framebufferHeight);
		
		ios_initialize_renderer(framebufferWidth, framebufferHeight);
		mouse_position_x = framebufferWidth / 2;
		mouse_position_y = framebufferHeight / 2;
		
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
		
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));

		glClear(GL_COLOR_BUFFER_BIT);
	}
}

- (void)deleteFramebuffer
{
	if (context)
	{
		[EAGLContext setCurrentContext:context];
		
		if (defaultFramebuffer)
		{
			glDeleteFramebuffers(1, &defaultFramebuffer);
			defaultFramebuffer = 0;
		}
		
		if (colorRenderbuffer)
		{
			glDeleteRenderbuffers(1, &colorRenderbuffer);
			colorRenderbuffer = 0;
		}
	}
}


extern volatile int is_in_foreground;
extern volatile int drawing_in_progress;

- (void)setFramebuffer
{
	while (!is_in_foreground)
	{
		//printf("looping in background\n");
		usleep(1000 * 1000);
	}

	drawing_in_progress = 1;

	if (changedOrientation)
	{
		[self deleteFramebuffer];
		changedOrientation = false;
	}
	
	if (context)
	{
		[EAGLContext setCurrentContext:context];
		
		if (!defaultFramebuffer)
			[self createFramebuffer];
		
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);		
	}	
}

- (void)presentFramebuffer
{
	if (context)
	{
		[EAGLContext setCurrentContext:context];
		
		glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
		
		[context presentRenderbuffer:GL_RENDERBUFFER];
	}

	drawing_in_progress = 0;
}

- (void)layoutSubviews
{
	// The framebuffer will be re-created at the beginning of the next setFramebuffer method call.
	changedOrientation = true;
}

@end
