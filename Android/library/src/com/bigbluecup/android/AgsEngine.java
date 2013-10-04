package com.bigbluecup.android;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnLongClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Toast;

public class AgsEngine extends Activity
{
	public static final String TAG = "AGS Engine";
	
	private static final IAgsApp defaultApp = new DefaultApp();
	
	private static IAgsApp app = defaultApp;
	
	public boolean isInGame = false;
	
	private Toast toast = null;
	private EngineGlue glue;
	private PowerManager.WakeLock wakeLock;
	private AudioManager audio;
	public CustomGlSurfaceView surfaceView;
	public MessageHandler handler;
	
	boolean ignoreNextPointerUp = false;
	boolean ignoreMovement = false;
	boolean initialized = false;
	boolean stopLongclick = false;
	boolean enableLongclick = false;
	private float lastX = 0.0f;
	private float lastY = 0.0f;
	private float downX = 0.0f;
	private float downY = 0.0f;	

	private boolean ignoreNextActionUp_Back = false;
	private boolean ignoreNextActionUp_Menu = false;

	private boolean draggingMouse = false;
	
	public static class DefaultApp implements IAgsApp
	{
		public void onMenuKeyPressed(AgsEngine engine, boolean longPress)
		{
			if (longPress) engine.toggleKeyboard();
			else engine.showInGameMenu();
		}
		
		public void onBackKeyPressed(AgsEngine engine, boolean longPress)
		{
			if (longPress) engine.showExitConfirmation();
			else engine.keyboardEvent(KeyCode.KeyBackHardKey);
		}
		
		public boolean onInGameMenuItemSelected(AgsEngine engine, MenuItem item)
		{
			int id = item.getItemId();

			// This must be if-else instead of switch-case because it is in a library
			if (id == R.id.key_f1)
				engine.keyboardEvent(KeyCode.KeyF1);
			else if (id == R.id.key_f2)
				engine.keyboardEvent(KeyCode.KeyF2);
			else if (id == R.id.key_f3)
				engine.keyboardEvent(KeyCode.KeyF3);
			else if (id == R.id.key_f4)
				engine.keyboardEvent(KeyCode.KeyF4);
			else if (id == R.id.key_f5)
				engine.keyboardEvent(KeyCode.KeyF5);
			else if (id == R.id.key_f6)
				engine.keyboardEvent(KeyCode.KeyF6);
			else if (id == R.id.key_f7)
				engine.keyboardEvent(KeyCode.KeyF7);
			else if (id == R.id.key_f8)
				engine.keyboardEvent(KeyCode.KeyF8);
			else if (id == R.id.key_f9)
				engine.keyboardEvent(KeyCode.KeyF9);
			else if (id == R.id.key_f10)
				engine.keyboardEvent(KeyCode.KeyF10);
			else if (id == R.id.key_f11)
				engine.keyboardEvent(KeyCode.KeyF11);
			else if (id == R.id.key_f12)
				engine.keyboardEvent(KeyCode.KeyF12);
			else if (id == R.id.exitgame)
				engine.showExitConfirmation();
			else if (id == R.id.toggle_keyboard)
				engine.toggleKeyboard();
			else
				return false;

			return true;
		}

		@Override
		public void onKeyboardEvent(AgsEngine engine, KeyCode key)
		{
			engine.keyboardEvent(key);
		}
		
		public int getInGameMenuID(AgsEngine engine)
		{
			return R.menu.default_ingame;
		}
		
		public static boolean DefaultInGameMenuItemSelection(AgsEngine engine, MenuItem item)
		{
			return defaultApp.onInGameMenuItemSelected(engine, item);
		}
		
		public static void DefaultKeyboardEvent(AgsEngine engine, KeyCode key)
		{
			defaultApp.onKeyboardEvent(engine, key);
		}
		
		public static void DefaultMenuKeyPress(AgsEngine engine, boolean longPress)
		{
			defaultApp.onMenuKeyPressed(engine, longPress);
		}
		
		public static void DefaultBackKeyPress(AgsEngine engine, boolean longPress)
		{
			defaultApp.onBackKeyPressed(engine, longPress);
		}
	};

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// Get the game filename from the launcher activity
		Bundle extras = getIntent().getExtras();
		String gameFilename = extras.getString("filename");
		String baseDirectory = extras.getString("directory");
		boolean loadLastSave = extras.getBoolean("loadLastSave");
		
		// Get app directory
		String appDirectory = "";
		try
		{
			appDirectory = getPackageManager().getPackageInfo(getPackageName(), 0).applicationInfo.dataDir;
		}
		catch (NameNotFoundException e) { }

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
		isInGame = true;
		setContentView(R.layout.loading);
		glue = new EngineGlue(this, gameFilename, baseDirectory, appDirectory, loadLastSave);
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
		pauseGame();
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		wakeLock.acquire();
		resumeGame();
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
			switch (msg.what)
			{
				case EngineGlue.MSG_SWITCH_TO_INGAME:
					switchToIngame();
					break;
					
				case EngineGlue.MSG_SHOW_MESSAGE:
					showMessage(msg.getData().getString("message"));
					break;
					
				case EngineGlue.MSG_SHOW_TOAST:
					showToast(msg.getData().getString("message"));
					break;
					
				case EngineGlue.MSG_SET_ORIENTATION:
					setRequestedOrientation(msg.getData().getInt("orientation"));
					break;
					
				case EngineGlue.MSG_ENABLE_LONGCLICK:
					enableLongclick = true;
					break;
			}
		}
	}
	
	
	@Override
	public boolean dispatchTouchEvent(MotionEvent ev)
	{
		switch (ev.getAction() & 0xFF)
		{
			case MotionEvent.ACTION_DOWN:
			{
				downX = lastX = ev.getX();
				downY = lastY = ev.getY();
				ignoreMovement = false;
				initialized = false;
				stopLongclick = false;
				glue.moveMouse(0, 0, downX, downY);				
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
				
				if (!ignoreMovement)
				{
					float x = ev.getX() - lastX;
					float y = ev.getY() - lastY;

					lastX = ev.getX();
					lastY = ev.getY();
					
					glue.moveMouse(x, y, lastX, lastY);
					
					try
					{
						// Delay a bit to not get flooded with events
						Thread.sleep(50, 0);
					}
					catch (InterruptedException e) {}
				}
				
				break;
			}

			case MotionEvent.ACTION_UP:
			{
				ignoreMovement = false;

				long down_time = ev.getEventTime() - ev.getDownTime();

				if (down_time < 200)
				{
					// Quick tap for clicking the left mouse button
					glue.clickMouse(EngineGlue.MOUSE_CLICK_LEFT);
					draggingMouse = false;
				}
		
				try
				{
					// Delay a bit to not get flooded with events
					Thread.sleep(50, 0);
				}
				catch (InterruptedException e) {}
				
				break;
			}		
			
			// Second finger down
			case 5: //MotionEvent.ACTION_POINTER_DOWN:
			{
				stopLongclick = true;
				ignoreMovement = true;
				ignoreNextPointerUp = true;
			}
			
			// Second finger lifted
			case 6: //MotionEvent.ACTION_POINTER_UP:
			{
				if (!ignoreNextPointerUp)
				{
					glue.clickMouse(EngineGlue.MOUSE_CLICK_RIGHT);
					
					ignoreMovement = false;
					try
					{
						// Delay a bit to not get flooded with events
						Thread.sleep(50, 0);
					}
					catch (InterruptedException e) {}
				}
				ignoreNextPointerUp = false;
				break;
			}			
		}
		
		return super.dispatchTouchEvent(ev);
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		if (app.onInGameMenuItemSelected(this, item) != false) return true;
		return super.onOptionsItemSelected(item);
	}	
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(app.getInGameMenuID(this), menu);
		return true;
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
				
				if ((key == KeyEvent.KEYCODE_BACK) && ((ev.getFlags() & 0x80) > 0)) // FLAG_LONG_PRESS
				{
					ignoreNextActionUp_Back = true;
					app.onBackKeyPressed(this, true);
				}
				
				if ((key == KeyEvent.KEYCODE_MENU) && ((ev.getFlags() & 0x80) > 0)) // FLAG_LONG_PRESS
				{
					ignoreNextActionUp_Menu = true;
					app.onMenuKeyPressed(this, true);
				}
				
				if (key == KeyEvent.KEYCODE_VOLUME_UP)
					audio.adjustStreamVolume(AudioManager.STREAM_MUSIC, AudioManager.ADJUST_RAISE, AudioManager.FLAG_SHOW_UI);
				
				if (key == KeyEvent.KEYCODE_VOLUME_DOWN)
					 audio.adjustStreamVolume(AudioManager.STREAM_MUSIC, AudioManager.ADJUST_LOWER, AudioManager.FLAG_SHOW_UI);
				
				break;
			}

			case KeyEvent.ACTION_UP:
			{
				int key = ev.getKeyCode();
				
				if (key == KeyEvent.KEYCODE_MENU)
				{
					if (!ignoreNextActionUp_Menu) app.onMenuKeyPressed(this, false);
					ignoreNextActionUp_Menu = false;
					return isInGame;
				}
				else if (key == KeyEvent.KEYCODE_BACK)
				{
					if (!ignoreNextActionUp_Back)
					{
						app.onBackKeyPressed(this, false);
					}
					ignoreNextActionUp_Back = false;
					return isInGame;
				}

				if (   (key == KeyEvent.KEYCODE_MENU)
					|| (key == KeyEvent.KEYCODE_VOLUME_UP)
					|| (key == KeyEvent.KEYCODE_VOLUME_DOWN)
					|| (key == 164) // KEYCODE_VOLUME_MUTE
					|| (key == KeyEvent.KEYCODE_SEARCH)
					|| (key == KeyEvent.KEYCODE_ALT_LEFT)
					|| (key == KeyEvent.KEYCODE_ALT_RIGHT)
					|| (key == KeyEvent.KEYCODE_SHIFT_LEFT)
					|| (key == KeyEvent.KEYCODE_SHIFT_RIGHT))
				{
					return isInGame;
				}
				// key wasn't a special key, allow the app to intercept
				app.onKeyboardEvent(this, KeyCode.findByAndroidKeyCode(key, ev.isShiftPressed()));
				break;
			}
		}
		
		return isInGame;
	}


	public void toggleKeyboard()
	{
		InputMethodManager manager = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
		manager.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
	}


	// Exit confirmation dialog displayed when hitting the "back" button
	public void showExitConfirmation()
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
	
	public void showInGameMenu()
	{
		openOptionsMenu();
	}

	// Switch to the game view after loading is done
	public void switchToIngame()
	{
		surfaceView = new CustomGlSurfaceView(this);
		setContentView(surfaceView);
		
		surfaceView.setOnLongClickListener(new OnLongClickListener()
		{
			public boolean onLongClick(View v)
			{
				if (!draggingMouse && !stopLongclick && (Math.abs(downX - lastX) < 4.0f) && (Math.abs(downY - lastY) < 4.0f))
				{
					draggingMouse = true;
					glue.clickMouse(EngineGlue.MOUSE_HOLD_LEFT);
					return true; // Produces haptic feedback (vibration)
				}
				return false;
			}
		});
		
		surfaceView.setLongClickable(enableLongclick);
		
		isInGame = true;
	}
	
	public void keyboardEvent(KeyCode key)
	{
		glue.keyboardEvent(key.getAndroidKeyCode(), key.getUnicodeChar(), key.hasShiftKey());
	}
	
	public void pauseGame()
	{
		if (isInGame) glue.pauseGame();
	}
	
	public void resumeGame()
	{
		if (isInGame) glue.resumeGame();
	}
	
	public static void setApp(IAgsApp app)
	{
		if (app == null) AgsEngine.app = defaultApp;
		else AgsEngine.app = app;
	}
	
	public static IAgsApp getApp()
	{
		return app;
	}
}
