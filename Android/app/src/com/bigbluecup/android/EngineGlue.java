package com.bigbluecup.android;

import javax.microedition.khronos.egl.EGL10;
import com.bigbluecup.android.AgsEngine;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Message;
import android.widget.Toast;

public class EngineGlue extends Thread implements CustomGlSurfaceView.Renderer
{
	public static int MSG_SWITCH_TO_INGAME = 1;
	public static int MSG_SHOW_MESSAGE = 2;
	public static int MSG_SHOW_TOAST = 3;
	
	public static int MOUSE_CLICK_LEFT = 1;
	public static int MOUSE_CLICK_RIGHT = 2;
	
	public int keyboardKeycode = 0;
	public short mouseMoveX = 0;
	public short mouseMoveY = 0;
	public int mouseClick = 0;
	
	private int screenPhysicalWidth = 480;
	private int screenPhysicalHeight = 320;
	private int screenVirtualWidth = 320;
	private int screenVirtualHeight = 200;
	
	private boolean paused = false;

	private AudioTrack audioTrack;
	private byte[] audioBuffer;	
	private int bufferSize = 0;
	private int audioVolume = 50;
	private int minAudioVolume = 0;
	private int maxAudioVolume = 100;
	private boolean audioMuted = false;
	
	private AgsEngine activity;

	private String gameFilename = "";
	private String baseDirectory = "";
	
	public native void nativeInitializeRenderer(int width, int height);
	public native void shutdownEngine();
	private native boolean startEngine(Object object, String filename, String directory);
	private native void pauseEngine();
	private native void resumeEngine();
	
	public EngineGlue(AgsEngine activity, String filename, String directory)
	{
		this.activity = activity;
		gameFilename = filename;
		baseDirectory = directory;
		
		System.loadLibrary("agsengine");
	}	
	
	public void run()
	{
		startEngine(this, gameFilename, baseDirectory);
	}	

	public void pauseGame()
	{
		paused = true;
		audioTrack.pause();
		pauseEngine();
	}

	public void resumeGame()
	{
		audioTrack.play();
		resumeEngine();
		paused = false;
	}
	
	public void moveMouse(float x, float y)
	{
		// The mouse movement is scaled to the game screen size
		mouseMoveX = (short) (x * (float)screenVirtualWidth / (float)screenPhysicalWidth);
		mouseMoveY = (short) (y * (float)screenVirtualHeight / (float)screenPhysicalHeight);
	}
	
	public void clickMouse(int button)
	{
		mouseClick = button;
	}
	
	public void keyboardEvent(int keycode, int character, boolean shiftPressed)
	{
		keyboardKeycode = keycode | (character << 16) | ((shiftPressed ? 1 : 0) << 30);
	}
	
	private void showMessage(String message)
	{
		Bundle data = new Bundle();
		data.putString("message", message);
		sendMessageToActivity(MSG_SHOW_MESSAGE, data);
	}
	
	private void showToast(String message)
	{
		Bundle data = new Bundle();
		data.putString("message", message);
		sendMessageToActivity(MSG_SHOW_TOAST, data);
	}
	
	private void sendMessageToActivity(int messageId, Bundle data)
	{
		Message m = activity.handler.obtainMessage();
		m.what = messageId;
		if (data != null)
			m.setData(data);
		activity.handler.sendMessage(m);
	}
	
	private void createScreen(int width, int height, int color_depth)
	{
		screenVirtualWidth = width;
		screenVirtualHeight = height;
		sendMessageToActivity(MSG_SWITCH_TO_INGAME, null);
		
		int[] configSpec = 
		{
			EGL10.EGL_DEPTH_SIZE, 0,
			EGL10.EGL_NONE
		};
		
		while (!activity.isInGame)
		{
			try
			{
				Thread.sleep(100, 0);
			}
			catch (InterruptedException e) {}
		}

		activity.surfaceView.initialize(configSpec, this);
	}
	
	private void swapBuffers()
	{
		activity.surfaceView.swapBuffers();
	}
	
	public void onSurfaceChanged(int width, int height)
	{
		setPhysicalScreenResolution(width, height);
		nativeInitializeRenderer(width, height);
	}
		
	// Called from Allegro
	private int pollKeyboard()
	{
		int result = keyboardKeycode;
		keyboardKeycode = 0;
		return result;
	}

	private int pollMouseX()
	{
		int result = mouseMoveX;
		mouseMoveX = 0;
		return result;
	}
	
	private int pollMouseY()
	{
		int result = mouseMoveY;
		mouseMoveY = 0;
		return result;
	}	

	private int pollMouseButtons()
	{
		int result = mouseClick;
		mouseClick = 0;
		return result;
	}
	
	private void blockExecution()
	{
		while (paused)
		{
			try
			{
				Thread.sleep(100, 0);
			}
			catch (InterruptedException e) {}
		}
	}

	// Called from Allegro, the buffer is allocated in native code
	public void initializeSound(byte[] buffer, int bufferSize)
	{
		audioBuffer = buffer;
		this.bufferSize = bufferSize;
		
		int sampleRate = 44100;
		int minBufferSize = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_CONFIGURATION_STEREO, AudioFormat.ENCODING_PCM_16BIT);
		
		if (minBufferSize < bufferSize * 4)
			minBufferSize = bufferSize * 4;
		
		audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, AudioFormat.CHANNEL_CONFIGURATION_STEREO, AudioFormat.ENCODING_PCM_16BIT, minBufferSize, AudioTrack.MODE_STREAM);
		minAudioVolume = (int)(AudioTrack.getMinVolume() * 100);
		maxAudioVolume = (int)(AudioTrack.getMaxVolume() * 100);
		
		audioVolume = (maxAudioVolume - minAudioVolume) / 2;
		audioTrack.setStereoVolume((float)audioVolume / 100, (float)audioVolume / 100);
		audioTrack.play();
	}
	
	public void updateSound()
	{
		audioTrack.write(audioBuffer, 0, bufferSize);
	}
	
	public void decreaseSoundVolume()
	{
		if (audioTrack == null)
			return;

		audioMuted = false;

		audioVolume -= 5;
		if (audioVolume < minAudioVolume)
			audioVolume = minAudioVolume;
		
		audioTrack.setStereoVolume((float)audioVolume / 100, (float)audioVolume / 100);
		
		showToast("Volume: " + audioVolume + " %");
	}
	
	public void increaseSoundVolume()
	{
		if (audioTrack == null)
			return;
		
		audioMuted = false;
		
		audioVolume += 5;
		if (audioVolume > maxAudioVolume)
			audioVolume = maxAudioVolume;
		
		audioTrack.setStereoVolume((float)audioVolume / 100, (float)audioVolume / 100);
		
		showToast("Volume: " + audioVolume + " %");
	}
	
	public void muteSound()
	{
		if (audioMuted)
		{
			audioTrack.setStereoVolume((float)audioVolume / 100, (float)audioVolume / 100);
			showToast("Volume: " + audioVolume + " %");
		}
		else
		{
			audioTrack.setStereoVolume(0, 0);
			showToast("Volume: Muted");
		}
		audioMuted = !audioMuted;
	}
	
	public void setPhysicalScreenResolution(int width, int height)
	{
		screenPhysicalWidth = width;
		screenPhysicalHeight = height;
	}
}
