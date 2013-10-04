package com.bigbluecup.android;

import android.content.Context;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import javax.microedition.khronos.opengles.GL;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

class CustomGlSurfaceView extends SurfaceView implements SurfaceHolder.Callback
{
	public interface Renderer
	{
		void onSurfaceChanged(int width, int height);
	}

	@SuppressWarnings("unused")
	private GL gl;
	private EGL10 egl;
	private EGLDisplay display;
	private EGLSurface surface;
	private EGLConfig config;
	private EGLContext eglContext;

	private boolean hasSurface;

	private Renderer renderer;

	private SurfaceHolder surfaceHolder;

	public boolean created;

	CustomGlSurfaceView(Context context)
	{
		super(context);
		surfaceHolder = getHolder();
		surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
		surfaceHolder.addCallback(this);
	}

	public void surfaceCreated(SurfaceHolder holder)
	{
		created = true;
	}

	public void surfaceDestroyed(SurfaceHolder holder)
	{
		created = false;
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h)
	{
		hasSurface = false;

		// Wait here till the engine thread calls swapBuffers() and the surface is recreated
		while (!hasSurface)
		{
			try
			{
				Thread.sleep(100, 0);
			}
			catch (InterruptedException e) {}
		}
		
		renderer.onSurfaceChanged(w, h);
	}

	public void initialize(int[] configSpec, Renderer rendererInterface)
	{
		renderer = rendererInterface;
		
		egl = (EGL10)EGLContext.getEGL();

		display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

		egl.eglInitialize(display, null);

		EGLConfig[] configs = new EGLConfig[1];
		int[] num_config = new int[1];
		egl.eglChooseConfig(display, configSpec, configs, 1, num_config);
		config = configs[0];

		eglContext = egl.eglCreateContext(display, config, EGL10.EGL_NO_CONTEXT, null);

		hasSurface = false;
		surface = null;
		createSurface();
	}

	private void createSurface()
	{
		hasSurface = false;
		
		if (surface != null)
		{
			egl.eglMakeCurrent(display, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
			egl.eglDestroySurface(display, surface);
		}

		surface = egl.eglCreateWindowSurface(display, config, surfaceHolder, null);
		egl.eglMakeCurrent(display, surface, surface, eglContext);

		gl = eglContext.getGL();

		hasSurface = true;
	}

	public void swapBuffers()
	{
		egl.eglSwapBuffers(display, surface);
		
		// Must be called from the rendering thread
		if (!hasSurface)
			createSurface();
	}
}
