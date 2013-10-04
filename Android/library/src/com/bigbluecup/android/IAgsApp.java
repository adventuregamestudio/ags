package com.bigbluecup.android;

import android.view.MenuItem;

public interface IAgsApp
{
	public void onMenuKeyPressed(AgsEngine engine, boolean longPress);
	public void onBackKeyPressed(AgsEngine engine, boolean longPress);
	public boolean onInGameMenuItemSelected(AgsEngine engine, MenuItem item);
	public void onKeyboardEvent(AgsEngine engine, KeyCode key);
	public int getInGameMenuID(AgsEngine engine);
}
