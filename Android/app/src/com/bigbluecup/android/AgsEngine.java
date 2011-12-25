package com.bigbluecup.android;

import com.bigbluecup.android.EngineGlue;
import com.bigbluecup.android.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.res.Configuration;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Toast;

public class AgsEngine extends Activity
{
	public boolean isInGame = false;
	
	private Toast toast = null;
	private EngineGlue glue;
	private PowerManager.WakeLock wakeLock;
	private AudioManager audio;
	public CustomGlSurfaceView surfaceView;
	public MessageHandler handler;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// Get the game filename from the launcher activity
		String gameFilename = getIntent().getExtras().getString("filename");
		String baseDirectory = getIntent().getExtras().getString("directory");
		
		// Set windows options
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
		setDefaultKeyMode(DEFAULT_KEYS_DISABLE);
				
		// Stop the device from saving power
		PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
		wakeLock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, "fullwakelock"); 
		wakeLock.acquire();
		
		// Set message handler for thread communication
		handler = new MessageHandler();
		
		audio = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
		
		// Switch to the loading view and start the game
		setContentView(R.layout.loading);
		glue = new EngineGlue(this, gameFilename, baseDirectory);
		glue.start();
	}
	
	@Override
	public void onDestroy()
	{
		glue.shutdownEngine();
		super.onDestroy();
	}
	
	
	@Override
	protected void onPause()
	{
		super.onPause();
		wakeLock.release();
		if (isInGame)
			glue.pauseGame();
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		wakeLock.acquire();
		if (isInGame)
			glue.resumeGame();
	}
	
	// Prevent the activity from being destroyed on a configuration change
	@Override
	public void onConfigurationChanged(Configuration newConfig)
	{
		super.onConfigurationChanged(newConfig);
	}
	
	// Handle messages from the engine thread
	class MessageHandler extends Handler
	{  
		@Override  
		public void handleMessage(Message msg)
		{
			if (msg.what == EngineGlue.MSG_SWITCH_TO_INGAME)
			{
				switchToIngame();
			}
			else if (msg.what == EngineGlue.MSG_SHOW_MESSAGE)
			{
				showMessage(msg.getData().getString("message"));
			}
			else if (msg.what == EngineGlue.MSG_SHOW_TOAST)
			{
				showToast(msg.getData().getString("message"));
			}
		}
	}
	
	boolean initialized = false;
	private float lastX = 0.0f;
	private float lastY = 0.0f;
	
	@Override
	public boolean dispatchTouchEvent(MotionEvent ev)
	{
		switch (ev.getAction())
		{
			case MotionEvent.ACTION_DOWN:
			{
				initialized = false;
				break;
			}
			case MotionEvent.ACTION_MOVE:
			{
				
				if (!initialized)
				{
					lastX = ev.getX();
					lastY = ev.getY();
					initialized = true;
				}

				float x = ev.getX() - lastX;
				float y = ev.getY() - lastY;

				glue.moveMouse(x, y);
				
				lastX = ev.getX();
				lastY = ev.getY();
				
				try
				{
					// Delay a bit to not get flooded with events
					Thread.sleep(50, 0);
				}
				catch (InterruptedException e) {}

				break;
			}

			case MotionEvent.ACTION_UP:
			{
				long down_time = ev.getEventTime() - ev.getDownTime();

				if (down_time < 100)
				{
					// Quick tap for clicking the left mouse button
					glue.clickMouse(EngineGlue.MOUSE_CLICK_LEFT);
				}
				else if (down_time < 300)
				{
					// Slightly slower tap for clicking the right mouse button					
					glue.clickMouse(EngineGlue.MOUSE_CLICK_RIGHT);
				}
				
				try
				{
					// Delay a bit to not get flooded with events
					Thread.sleep(100, 0);
				}
				catch (InterruptedException e) {}
			}			
		}
		
		return isInGame;
	}
	
	@Override
	public boolean dispatchKeyEvent(KeyEvent ev)
	{
		// Very simple key processing for now, just one key event per poll
		switch (ev.getAction())
		{
			case KeyEvent.ACTION_DOWN:
			{
				int key = ev.getKeyCode();
				
				if (key == KeyEvent.KEYCODE_BACK)
					showExitConfirmation();
				
				if (key == KeyEvent.KEYCODE_VOLUME_UP)
					audio.adjustStreamVolume(AudioManager.STREAM_MUSIC, AudioManager.ADJUST_RAISE, AudioManager.FLAG_SHOW_UI);
				
				if (key == KeyEvent.KEYCODE_VOLUME_DOWN)
					 audio.adjustStreamVolume(AudioManager.STREAM_MUSIC, AudioManager.ADJUST_LOWER, AudioManager.FLAG_SHOW_UI);
				
				if (key == KeyEvent.KEYCODE_MENU)
				{
					InputMethodManager manager = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
					manager.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);					
				}
				
				break;
			}

			case KeyEvent.ACTION_UP:
			{
				int key = ev.getKeyCode();
				
				if (   (key == KeyEvent.KEYCODE_MENU)
					|| (key == KeyEvent.KEYCODE_BACK)
					|| (key == KeyEvent.KEYCODE_VOLUME_UP)
					|| (key == KeyEvent.KEYCODE_VOLUME_DOWN)
					|| (key == 164)) // KEYCODE_VOLUME_MUTE
					return isInGame;

				glue.keyboardEvent(key, ev.getUnicodeChar(ev.getMetaState()), ev.isShiftPressed());
				break;
			}
		}
		
		return isInGame;
	}
	
	@Override
	public boolean dispatchTrackballEvent(MotionEvent ev)
	{ 
		switch (ev.getAction())
		{		
			case MotionEvent.ACTION_MOVE:
			{
				glue.mouseMoveX = (short) (ev.getX() * 10);
				glue.mouseMoveY = (short) (ev.getY() * 10);

			}
			case MotionEvent.ACTION_DOWN:
			{
				glue.mouseClick = 1;
			}			
		}
		return isInGame;
	}	

	
	// Exit confirmation dialog displayed when hitting the "back" button
	private void showExitConfirmation()
	{
		onPause();
		
		AlertDialog.Builder ad = new AlertDialog.Builder(this);
		ad.setMessage("Are you sure you want to quit?");
		
		ad.setOnCancelListener(new OnCancelListener()
		{
			public void onCancel(DialogInterface dialog)
			{
				onResume();
			}
		});

		ad.setPositiveButton("Yes", new OnClickListener()
		{
			public void onClick(DialogInterface dialog, int which)
			{
				onResume();
				onDestroy();
			}
		});
		
		ad.setNegativeButton("No", new OnClickListener()
		{
			public void onClick(DialogInterface dialog, int which)
			{
				onResume();
			}
		});
		
		ad.show();
	}
	
	
	// Display a game message
	public void showMessage(String message)
	{
		onPause();
		
		AlertDialog.Builder dialog = new AlertDialog.Builder(this);
		dialog.setTitle("Error");
		dialog.setMessage(message);

		dialog.setPositiveButton("OK", new OnClickListener()
		{
			public void onClick(DialogInterface dialog, int which)
			{
				onResume();
			}
		});
		
		dialog.show();
	}
	
	public void showToast(String message)
	{
		if (toast == null)
			toast = Toast.makeText(this, message, Toast.LENGTH_LONG);
		else
			toast.setText(message);
		
		toast.show();
	}

	// Switch to the game view after loading is done
	public void switchToIngame()
	{
		surfaceView = new CustomGlSurfaceView(this);
		setContentView(surfaceView);

		isInGame = true;
	}

}
