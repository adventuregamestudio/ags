package com.bigbluecup.android;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.opengl.GLSurfaceView;

public class Renderer extends GLSurfaceView
{
	private OGLRenderer renderer;
	private EngineGlue glue;

	public Renderer(Context context, EngineGlue glue)
	{
		super(context);
		this.glue = glue;
		renderer = new OGLRenderer();
		setRenderer(renderer);
	}

	private class OGLRenderer implements GLSurfaceView.Renderer
	{
		public void onDrawFrame(GL10 gl)
		{
			glue.nativeRender();
		}

		public void onSurfaceChanged(GL10 gl, int width, int height)
		{
			glue.nativeInitializeRenderer(width, height);
		}

		public void onSurfaceCreated(GL10 gl, EGLConfig config)
		{
		}
	}
}
